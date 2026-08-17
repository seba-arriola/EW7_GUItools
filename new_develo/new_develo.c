/***********************************************************************
 *                             new_develo                              *
 *                                                                     *
 *  Real-time seismogram display and manual picking tool. It reads     *
 *  the WAVE_RING for trace data, renders the last minutes of every    *
 *  station in a scrollable canvas and lets the operator pick P        *
 *  arrivals that are reported to PICK_RING. Includes HOLD/zoom,       *
 *  colour, filter and time-window controls.                           *
 *                                                                     *
 *  Usage: new_develo <configfile.d>                                   *
 ***********************************************************************/

#include <gtk/gtk.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <limits.h>
#include <locale.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <swap.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#undef TRUE
#undef FALSE

#include <earthworm.h>
#include <transport.h>
#include <trace_buf.h>
#include <kom.h>
#include "earlybirdlib.h"
#include "dataprocessing.h" /* External DSP library */

#define REFRESH_MS 150           /* A1: 150ms ~ 6.7fps (before 50ms = 20fps) */
#define ENV_PROCESS_MS 500       /* A2: processing/envelope cadence (2Hz) */
#define PANEL_WIDTH 90
#define BOTTOM_AXIS_H 30 
#define MAX_TRACE_BYTES 4096
#define MAX_STATIONS 512 

#define MAX_MINUTES 120                 
#define MAX_SAMP_RATE 100               
#define MAX_PICKS_PER_STA 50           

#define MAX_STR 256      

#define MIN_ZOOM 0.05
#define MAX_ZOOM 100.0
#define MAX_REALTIME_SKEW 86400.0
#define DATA_STALE_SECS 10.0

#define CIRC_IDX(abs_val, size) (int)(((abs_val) % (size) + (size)) % (size))

/* --- MODULE CONFIGURATION --- */
char MyModName[MAX_STR] = "MOD_DEVELO";     /* Earthworm module name */
char WaveRingName[MAX_STR] = "WAVE_RING";   /* wave data ring */
char PickRingName[MAX_STR] = "PICK_RING";   /* picks ring */
char StaFile[MAX_STR] = "pick_wcatwc.sta";  /* station list file */
char StaDataFile[MAX_STR] = "station.dat";  /* station data file */
char CalibsFile[MAX_STR] = "calibs";        /* calibration file */
int  HeartBeatInt = 30;                     /* heartbeat interval (seconds) */
int  LogFile = 1;                           /* 1 = write log to disk */

/* --- GLOBALS FOR CUSTOM COLORS --- */
double g_color_bg[3]   = {1.0, 1.0, 1.0};  /* background color */
double g_color_wave[3] = {0.0, 0.0, 0.0};  /* waveform color */
double g_color_font[3] = {1.0, 0.0, 0.0};  /* font color */
double g_color_sep[3]  = {0.85, 0.85, 0.85}; /* separator color *//* New color for the separator */

/* --- GLOBALS FOR DYNAMIC FILTERS --- */
int g_filter_type = 0; /* 0=Raw, 1=HP, 2=LP, 3=BP */
double g_f1 = 0.7;        /* first corner frequency (Hz) */
double g_f2 = 2.0;        /* second corner frequency (Hz, band-pass) */
int g_order = 4;          /* filter order (2 or 4) */

SHM_INFO  WaveRegion;   /* transport region of the wave ring */
SHM_INFO  PickRegion;   /* transport region of the pick ring */
MSG_LOGO  WaveLogo[1];  /* logo to receive trace buffers */
MSG_LOGO  PickLogo[1];  /* logo to receive pick messages */                 

/* GLOBAL VARIABLES */
double    g_latest_time = 0.0;            /* newest time seen in the wave data */
double    g_smooth_time = 0.0;            /* right edge of the display window */
int64_t   g_last_frame_realtime = 0;      /* monotonic time of the last fetch frame */
gboolean  g_is_hold = FALSE;              /* HOLD (frozen screen) active */
double    g_t_hold_time = 0.0;            /* frozen time shown while on HOLD */
double    g_zoom_factor = 1.0;            /* vertical zoom applied while on HOLD */
unsigned char TypePickTWC = 0;            /* TYPE_PICKTWC */
unsigned char MyModId = 0;                /* Earthworm module id */
unsigned char MyInstId = 0;               /* Earthworm installation id */
unsigned char TypeHeartBeat = 0;          /* TYPE_HEARTBEAT */
unsigned char TypeError = 0;              /* TYPE_ERROR */
pid_t         MyPid;                      /* this process id */
time_t        timeLastBeat = 0;           /* time of the last heartbeat sent */
long          g_lPickIndexCounter = 10000;/* next pick index to assign */
double        g_last_data_realtime = 0.0; /* time of the last data packet */
gboolean      g_data_stale = FALSE;       /* true when the data feed is stale */
static int    g_warned_datatype = 0;      /* warns once about an unknown datatype */
double        g_pick_replace_secs = 15.0; /* seconds to replace an existing pick */

/* A1/A2/A3: state of the envelope cache. The envelope is recalculated at
   most every ENV_PROCESS_MS or when something forces it (filter, window, hold,
   width); the draw only reads it and scales it (zoom does not invalidate). */
static int64_t g_last_env_process_ms = 0; /* monotonic ms of the last envelope pass */
static double  g_last_env_t_right = 0.0;  /* t_right of the last envelope pass */
static int     g_last_env_width = 0;      /* canvas width of the last pass */
static gboolean g_last_env_ok = FALSE;    /* envelope cache computed at least once */
static gboolean g_bForceEnv = FALSE;      /* forced recompute request */
static gboolean g_envelope_updated = FALSE; /* set when the pass computed something */

/* A2/A3: prototypes (defined before on_draw_waves) */
static void compute_station_envelope(int i, int width);
static void update_wave_envelopes(void);

typedef struct {
    double dTime;
    char szPhase[8];
    long lPickIndex;
    int iUseMe;
} PICK;

typedef struct {
    char szStation[10];
    char szChannel[10];
    char szNetID[10];
    
    double dSampRate; 
    int32_t *plRawCircBuff;
    long lRawCircSize;
    
    int64_t lLastAbsIdx; 
    double dLastPacketSysTime;
    
    PICK picks[MAX_PICKS_PER_STA];
    int iNumPicks;
    long lPickRingNext;
    
    /* Cache of the processed trace (extract+interp+DC+filter) to avoid
       recomputing everything every frame. It is invalidated if the window, the
       filter change or data arrives within the cached window. */
    long *plProcBuf;
    long lProcCap;
    long lProcAbsStart;
    long lProcAbsEnd;
    int iProcValid;
    int iProcFoundFirst;
    int iProcFilterType;
    double dProcF1;
    double dProcF2;
    int iProcOrder;
    
    /* A3: envelope decimated per pixel column, in raw units (without
       auto_scale/zoom). It is computed in the processing pass (A2) and the draw
       only scales it; thus the zoom does not invalidate it. */
    double  *dEnvMin;
    double  *dEnvMax;
    int     *iEnvHas;
    int      iEnvCap;
    int      iEnvWidth;
    gboolean bEnvValid;
    double   dEnvMaxAbs;
    int      iEnvFoundFirst;
    
    int pick_status;    
    PPICK pick;         
} DEV_STATION;

DEV_STATION *StaArray = NULL;    /* per-station live data */
STATION *AtwcStaArray = NULL;    /* ATWC station list (from ReadStationList) */
int iNumStas = 0;                /* number of loaded stations */

GtkWidget *g_scrolled_window;    /* scrolled window of the waveform canvas */
GtkWidget *g_drawing_waves;      /* waveform drawing area */
GtkWidget *drawing_axis;         /* time axis drawing area */
GtkWidget *btn_hold;             /* HOLD toggle button */

double dTrackHeight = 60.0;      /* height of one station track */
int iVisStas = 12;               /* stations visible per screen */
int iTimeWindowMinutes = 6;      /* time window in minutes */

/* --------------------------------------------------------------------
 * BASE FUNCTIONS (ReadConfig, UI, etc)
 * -------------------------------------------------------------------- */
 /***********************************************************************
  *                           ParseHexColor()                           *
  *             Converts a "#RRGGBB" or "RRGGBB" string to RGB          *
  *             doubles in the range 0..1.                              *
  *               0 if the format is valid, -1 otherwise.               *
  ***********************************************************************/

static int ParseHexColor(const char *s, double rgb[3]) {
    unsigned int r, g, b;
    if (!s || s[0] == '\0') return -1;
    while (*s == '#') s++;
    if (sscanf(s, "%2x%2x%2x", &r, &g, &b) != 3) return -1;
    rgb[0] = (double)r / 255.0;
    rgb[1] = (double)g / 255.0;
    rgb[2] = (double)b / 255.0;
    return 0;
}

 /***********************************************************************
  *                            ColorToHex()                             *
  *             Formats an RGB color (0..1) as a "RRGGBB" string        *
  *             without '#', ready to paste into the .d file.           *
  *               Nothing.                                              *
  ***********************************************************************/

static void ColorToHex(const double rgb[3], char out[8]) {
    snprintf(out, 8, "%02X%02X%02X",
             (int)(rgb[0] * 255.0 + 0.5), (int)(rgb[1] * 255.0 + 0.5), (int)(rgb[2] * 255.0 + 0.5));
}

 /***********************************************************************
  *                          LogColorConfig()                           *
  *             Logs the four color variables in .d format so the       *
  *             current values can be recovered and reused.             *
  *               Nothing.                                              *
  ***********************************************************************/

static void LogColorConfig(void) {
    char c_wave[8], c_bg[8], c_font[8], c_sep[8];
    ColorToHex(g_color_wave, c_wave);
    ColorToHex(g_color_bg, c_bg);
    ColorToHex(g_color_font, c_font);
    ColorToHex(g_color_sep, c_sep);
    logit("et", "new_develo: Color config (copy to .d):\n");
    logit("et", "  waveformsColor   %s\n", c_wave);
    logit("et", "  backgroundColor  %s\n", c_bg);
    logit("et", "  fontColor        %s\n", c_font);
    logit("et", "  separatorColor   %s\n", c_sep);
}

 /***********************************************************************
  *                            ReadConfig()                             *
  *             Reads the module command file with kom.c, loading       *
  *             the module, rings, station files, colors and display    *
  *             parameters into the globals.                            *
  *               0 on success, -1 if a parameter is missing or bad.    *
  ***********************************************************************/

int ReadConfig(char *configfile) {
    int ncommand = 8, nmiss = 0, i;
    char init[10] = {0};
    char *com, *str;

    if (!k_open(configfile)) {
        fprintf(stderr, "new_develo: Error opening config file <%s>\n", configfile);
        return -1;
    }

    while (k_rd()) {
        com = k_str();
        if (!com || com[0] == '#') continue;

        if (k_its("MyModuleId")) { str = k_str(); if (str) strcpy(MyModName, str); init[0] = 1; }
        else if (k_its("WaveRing")) { str = k_str(); if (str) strcpy(WaveRingName, str); init[1] = 1; }
        else if (k_its("PickRing")) { str = k_str(); if (str) strcpy(PickRingName, str); init[2] = 1; }
        else if (k_its("HeartBeatInt")) { HeartBeatInt = k_int(); init[3] = 1; }
        else if (k_its("LogFile")) { LogFile = k_int(); init[4] = 1; }
        else if (k_its("StaFile")) { str = k_str(); if (str) strcpy(StaFile, str); init[5] = 1; }
        else if (k_its("StaDataFile")) { str = k_str(); if (str) strcpy(StaDataFile, str); init[6] = 1; }
        else if (k_its("CalibsFile")) { str = k_str(); if (str) strcpy(CalibsFile, str); init[7] = 1; }
        else if (k_its("waveformsColor")) { str = k_str(); if (str && ParseHexColor(str, g_color_wave)) logit("et", "new_develo: waveformsColor <%s> invalid, using default\n", str); }
        else if (k_its("backgroundColor")) { str = k_str(); if (str && ParseHexColor(str, g_color_bg)) logit("et", "new_develo: backgroundColor <%s> invalid, using default\n", str); }
        else if (k_its("fontColor")) { str = k_str(); if (str && ParseHexColor(str, g_color_font)) logit("et", "new_develo: fontColor <%s> invalid, using default\n", str); }
        else if (k_its("separatorColor")) { str = k_str(); if (str && ParseHexColor(str, g_color_sep)) logit("et", "new_develo: separatorColor <%s> invalid, using default\n", str); }
        else if (k_its("StationsPerScreen")) { str = k_str(); if (str) { int v = atoi(str); if (v >= 1 && v <= MAX_STATIONS) iVisStas = v; else logit("et", "new_develo: StationsPerScreen <%s> invalid, using default %d\n", str, iVisStas); } }
        else if (k_its("TimeWindow")) { str = k_str(); if (str) { int v = atoi(str); if (v >= 1 && v <= MAX_MINUTES) iTimeWindowMinutes = v; else logit("et", "new_develo: TimeWindow <%s> invalid, using default %d\n", str, iTimeWindowMinutes); } }
        else if (k_its("PickReplaceWindow")) { str = k_str(); if (str) { double v = atof(str); if (v >= 0.0) g_pick_replace_secs = v; else logit("et", "new_develo: PickReplaceWindow <%s> invalid, using default %.1f\n", str, g_pick_replace_secs); } }
        else continue;

        if (k_err()) {
            fprintf(stderr, "new_develo: Error parsing <%s> in <%s>\n", com, configfile);
            return -1;
        }
    }
    for (i = 0; i < ncommand; i++) if (!init[i]) nmiss++;
    k_close();
    if (nmiss > 0) {
        fprintf(stderr, "new_develo: ERROR, missing parameters in <%s>\n", configfile);
        return -1;
    }
    return 0;
}

 /***********************************************************************
  *                          FreeAllStations()                          *
  *             Frees the per-station buffers (raw, processed and       *
  *             envelope caches) and the station arrays, resetting      *
  *             the station count.                                      *
  *               Nothing.                                              *
  ***********************************************************************/

void FreeAllStations() {
    if (StaArray) {
        for (int i = 0; i < iNumStas; i++) {
            if (StaArray[i].plRawCircBuff) { free(StaArray[i].plRawCircBuff); StaArray[i].plRawCircBuff = NULL; }
            if (StaArray[i].plProcBuf) { free(StaArray[i].plProcBuf); StaArray[i].plProcBuf = NULL; }
            if (StaArray[i].dEnvMin) { free(StaArray[i].dEnvMin); StaArray[i].dEnvMin = NULL; }
            if (StaArray[i].dEnvMax) { free(StaArray[i].dEnvMax); StaArray[i].dEnvMax = NULL; }
            if (StaArray[i].iEnvHas) { free(StaArray[i].iEnvHas); StaArray[i].iEnvHas = NULL; }
        }
        free(StaArray);
        StaArray = NULL;
    }
    if (AtwcStaArray) {
        free(AtwcStaArray);
        AtwcStaArray = NULL;
    }
    iNumStas = 0;
}

 /***********************************************************************
  *                        on_btn_hold_toggled()                        *
  *             GTK handler for the HOLD toggle: freezes or releases    *
  *             the screen time, resets the zoom, forces an envelope    *
  *             recompute and redraws.                                  *
  *               Nothing.                                              *
  ***********************************************************************/

void on_btn_hold_toggled(GtkToggleButton *togglebutton, gpointer user_data) {
    g_is_hold = gtk_toggle_button_get_active(togglebutton);
    g_bForceEnv = TRUE; /* The envelope is recalculated for the (in)active window */
    if (g_is_hold) {
        if (g_smooth_time > 0.0) g_t_hold_time = g_smooth_time;
        else { time_t t; time(&t); g_t_hold_time = (double)t; }
    } else {
        g_zoom_factor = 1.0;
    }
    if (g_drawing_waves) gtk_widget_queue_draw(g_drawing_waves);
    if (drawing_axis) gtk_widget_queue_draw(drawing_axis);
}

 /***********************************************************************
  *                           on_key_press()                            *
  *             GTK handler for key events while on HOLD: Up/Down       *
  *             arrows zoom the waveform vertically in/out.             *
  *               TRUE if handled, FALSE otherwise.                     *
  ***********************************************************************/

static gboolean on_key_press(GtkWidget *widget, GdkEventKey *event, gpointer data) {
    if (!g_is_hold) return FALSE; 
    if (event->keyval == GDK_KEY_Up) {
        g_zoom_factor *= 1.5; 
        if (g_zoom_factor > MAX_ZOOM) g_zoom_factor = MAX_ZOOM;
        if (g_drawing_waves) gtk_widget_queue_draw(g_drawing_waves);
        return TRUE;
    } else if (event->keyval == GDK_KEY_Down) {
        g_zoom_factor /= 1.5; 
        if (g_zoom_factor < MIN_ZOOM) g_zoom_factor = MIN_ZOOM;
        if (g_drawing_waves) gtk_widget_queue_draw(g_drawing_waves);
        return TRUE;
    }
    return FALSE;
}

 /***********************************************************************
  *                         on_colour_select()                          *
  *             Opens a colour chooser dialog for one of the four       *
  *             interface colors (waveform, background, font,           *
  *             separator) and applies the selection.                   *
  *               Nothing.                                              *
  ***********************************************************************/

void on_colour_select(GtkWidget *widget, gpointer data) {
    int type = GPOINTER_TO_INT(data); 
    GtkWidget *dialog = gtk_color_chooser_dialog_new("Select Colour", GTK_WINDOW(gtk_widget_get_toplevel(widget)));
    
    GdkRGBA current_color;
    current_color.alpha = 1.0;
    if (type == 1) { current_color.red = g_color_wave[0]; current_color.green = g_color_wave[1]; current_color.blue = g_color_wave[2]; }
    else if (type == 2) { current_color.red = g_color_bg[0]; current_color.green = g_color_bg[1]; current_color.blue = g_color_bg[2]; }
    else if (type == 3) { current_color.red = g_color_font[0]; current_color.green = g_color_font[1]; current_color.blue = g_color_font[2]; }
    else if (type == 4) { current_color.red = g_color_sep[0]; current_color.green = g_color_sep[1]; current_color.blue = g_color_sep[2]; }
    
    gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(dialog), &current_color);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
        GdkRGBA new_color;
        gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(dialog), &new_color);
        
        if (type == 1) { g_color_wave[0] = new_color.red; g_color_wave[1] = new_color.green; g_color_wave[2] = new_color.blue; }
        else if (type == 2) { g_color_bg[0] = new_color.red; g_color_bg[1] = new_color.green; g_color_bg[2] = new_color.blue; }
        else if (type == 3) { g_color_font[0] = new_color.red; g_color_font[1] = new_color.green; g_color_font[2] = new_color.blue; }
        else if (type == 4) { g_color_sep[0] = new_color.red; g_color_sep[1] = new_color.green; g_color_sep[2] = new_color.blue; }

        LogColorConfig();
        if (g_drawing_waves) gtk_widget_queue_draw(g_drawing_waves);
    }
    gtk_widget_destroy(dialog);
}

 /***********************************************************************
  *                         RecalcTrackHeight()                         *
  *             Recomputes the height of each station track from the    *
  *             visible area and the stations per screen, then          *
  *             redraws the canvas.                                     *
  *               Nothing.                                              *
  ***********************************************************************/

void RecalcTrackHeight() {
    GtkAllocation alloc;
    if (!g_scrolled_window) return;
    gtk_widget_get_allocation(g_scrolled_window, &alloc);
    if (alloc.height > 0 && iVisStas > 0) {
        dTrackHeight = (double)alloc.height / iVisStas;
        if (dTrackHeight < 2.0) dTrackHeight = 2.0;
        gtk_widget_set_size_request(g_drawing_waves, -1, iNumStas * dTrackHeight);
    }
    if (g_drawing_waves) gtk_widget_queue_draw(g_drawing_waves);
}

 /***********************************************************************
  *                    on_stas_per_screen_activate()                    *
  *             Shows a dialog to set how many stations fit             *
  *             on the screen, then recomputes the track                *
  *               Nothing.                                              *
  ***********************************************************************/

void on_stas_per_screen_activate(GtkWidget *widget, gpointer data) {
    GtkWidget *window = GTK_WIDGET(data);
    GtkWidget *dialog = gtk_dialog_new_with_buttons("Stations per screen", GTK_WINDOW(window),
        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
        "_OK", GTK_RESPONSE_ACCEPT, "_Cancel", GTK_RESPONSE_REJECT, NULL);

    GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_box_pack_start(GTK_BOX(content_area), hbox, TRUE, TRUE, 15);
    GtkWidget *label = gtk_label_new("Visible stations:");
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 5);

    GtkWidget *spin = gtk_spin_button_new_with_range(1, MAX_STATIONS, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin), iVisStas);
    gtk_box_pack_start(GTK_BOX(hbox), spin, FALSE, FALSE, 5);
    gtk_widget_show_all(dialog);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        iVisStas = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(spin));
        RecalcTrackHeight();
    }
    gtk_widget_destroy(dialog);
}

 /***********************************************************************
  *                      on_time_window_activate()                      *
  *             Shows a dialog to set the time window in minutes,       *
  *             forces the envelope to be recomputed and redraws.       *
  *               Nothing.                                              *
  ***********************************************************************/

void on_time_window_activate(GtkWidget *widget, gpointer data) {
    GtkWidget *dialog = gtk_dialog_new_with_buttons("Time window", GTK_WINDOW(data),
        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
        "_OK", GTK_RESPONSE_ACCEPT, "_Cancel", GTK_RESPONSE_REJECT, NULL);

    GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_box_pack_start(GTK_BOX(content_area), hbox, TRUE, TRUE, 15);
    GtkWidget *label = gtk_label_new("Window size (minutes):");
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 5);

    GtkWidget *spin = gtk_spin_button_new_with_range(1, MAX_MINUTES, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin), iTimeWindowMinutes);
    gtk_box_pack_start(GTK_BOX(hbox), spin, FALSE, FALSE, 5);
    gtk_widget_show_all(dialog);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        iTimeWindowMinutes = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(spin));
        g_bForceEnv = TRUE; /* New window: recompute envelope */
        if (g_drawing_waves) gtk_widget_queue_draw(g_drawing_waves);
        if (drawing_axis) gtk_widget_queue_draw(drawing_axis);
    }
    gtk_widget_destroy(dialog);
}

/* --------------------------------------------------------------------
 * INTERACTIVE DYNAMIC FILTER HANDLER
 * -------------------------------------------------------------------- */
 /***********************************************************************
  *                      on_filter_combo_changed()                      *
  *             GTK handler for the filter type combo: enables or       *
  *             disables the F1/F2/order fields according to the        *
  *             selected filter type.                                   *
  *               Nothing.                                              *
  ***********************************************************************/

static void on_filter_combo_changed(GtkComboBox *widget, gpointer data) {
    GtkWidget **entries = (GtkWidget **)data;
    int type = gtk_combo_box_get_active(widget);
    gtk_widget_set_sensitive(entries[0], (type != 0)); /* F1 (HP, LP, BP) */
    gtk_widget_set_sensitive(entries[1], (type == 3)); /* F2 (Only Band-Pass) */
    gtk_widget_set_sensitive(entries[2], (type != 0)); /* Order (HP, LP, BP) */
}

 /***********************************************************************
  *                       LoadStationsFromFile()                        *
  *             Loads the station list from the .sta files, sorts it    *
  *             by latitude (north first) and initializes the per-      *
  *             station structures and circular buffers.                *
  *               Nothing; exits on error or an empty list.             *
  ***********************************************************************/

void LoadStationsFromFile() {
    AtwcStaArray = (STATION *) calloc(MAX_STATIONS, sizeof(STATION));
    if (!AtwcStaArray) exit(-1);
    
    int rtn = ReadStationList(&AtwcStaArray, &iNumStas, StaFile, StaDataFile, CalibsFile, MAX_STATIONS, 0);
    if (rtn == -1 || iNumStas == 0) exit(-1);

    for (int i = 0; i < iNumStas - 1; i++) {
        for (int j = 0; j < iNumStas - i - 1; j++) {
            if (AtwcStaArray[j].dLat < AtwcStaArray[j+1].dLat) {
                STATION temp = AtwcStaArray[j];
                AtwcStaArray[j] = AtwcStaArray[j+1];
                AtwcStaArray[j+1] = temp;
            }
        }
    }

    StaArray = (DEV_STATION *) calloc(iNumStas, sizeof(DEV_STATION));
    for (int i = 0; i < iNumStas; i++) {
        strcpy(StaArray[i].szStation, AtwcStaArray[i].szStation);
        strcpy(StaArray[i].szChannel, AtwcStaArray[i].szChannel);
        strcpy(StaArray[i].szNetID, AtwcStaArray[i].szNetID);
        
        StaArray[i].pick.dLat = AtwcStaArray[i].dLat;
        StaArray[i].pick.dLon = AtwcStaArray[i].dLon;

        StaArray[i].dSampRate = 20.0; 
        StaArray[i].lRawCircSize = MAX_MINUTES * 60 * MAX_SAMP_RATE; 
        StaArray[i].plRawCircBuff = (int32_t *) malloc(StaArray[i].lRawCircSize * sizeof(int32_t));
        
        for(long k=0; k < StaArray[i].lRawCircSize; k++) {
            StaArray[i].plRawCircBuff[k] = INT_MAX;
        }
        
        StaArray[i].iNumPicks = 0;
        StaArray[i].lPickRingNext = 0;
        StaArray[i].lLastAbsIdx = 0; 
        StaArray[i].dLastPacketSysTime = 0.0; 
        StaArray[i].plProcBuf = NULL;
        StaArray[i].lProcCap = 0;
        StaArray[i].lProcAbsStart = 0;
        StaArray[i].lProcAbsEnd = 0;
        StaArray[i].iProcValid = 0;
        StaArray[i].iProcFoundFirst = 0;
        StaArray[i].iProcFilterType = 0;
        StaArray[i].dProcF1 = 0.0;
        StaArray[i].dProcF2 = 0.0;
        StaArray[i].iProcOrder = 0;
        StaArray[i].dEnvMin = NULL;
        StaArray[i].dEnvMax = NULL;
        StaArray[i].iEnvHas = NULL;
        StaArray[i].iEnvCap = 0;
        StaArray[i].iEnvWidth = 0;
        StaArray[i].bEnvValid = FALSE;
        StaArray[i].dEnvMaxAbs = 1.0;
        StaArray[i].iEnvFoundFirst = 0;
        StaArray[i].pick_status = 0;
        InitP(&StaArray[i].pick);
    }
}

 /***********************************************************************
  *                      on_clean_view_activate()                       *
  *             Shows a dialog to drop stations that have not           *
  *             received data for more than the given minutes,          *
  *             freeing their buffers.                                  *
  *               Nothing.                                              *
  ***********************************************************************/

void on_clean_view_activate(GtkWidget *widget, gpointer data) {
    GtkWidget *window = GTK_WIDGET(data);
    GtkWidget *dialog = gtk_dialog_new_with_buttons("Clean View (Remove Inactive)", GTK_WINDOW(window),
        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
        "_OK", GTK_RESPONSE_ACCEPT, "_Cancel", GTK_RESPONSE_REJECT, NULL);

    GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_box_pack_start(GTK_BOX(content_area), hbox, TRUE, TRUE, 15);
    GtkWidget *label = gtk_label_new("Max time without data (minutes):");
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 5);

    GtkWidget *spin = gtk_spin_button_new_with_range(1, 10080, 1); 
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin), 60); 
    gtk_box_pack_start(GTK_BOX(hbox), spin, FALSE, FALSE, 5);
    gtk_widget_show_all(dialog);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        int min_val = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(spin));
        double max_age_secs = min_val * 60.0;
        double sys_time = (double)g_get_real_time() / 1000000.0;
        
        int active_count = 0;
        for (int i = 0; i < iNumStas; i++) {
            if (StaArray[i].dLastPacketSysTime > 0.0 && (sys_time - StaArray[i].dLastPacketSysTime) <= max_age_secs) {
                if (i != active_count) {
                    StaArray[active_count] = StaArray[i]; 
                    AtwcStaArray[active_count] = AtwcStaArray[i]; 
                    StaArray[i].plRawCircBuff = NULL;
                    StaArray[i].plProcBuf = NULL;
                }
                active_count++;
            } else {
                if (StaArray[i].plRawCircBuff) {
                    free(StaArray[i].plRawCircBuff);
                    StaArray[i].plRawCircBuff = NULL;
                }
                if (StaArray[i].plProcBuf) {
                    free(StaArray[i].plProcBuf);
                    StaArray[i].plProcBuf = NULL;
                }
                if (StaArray[i].dEnvMin) { free(StaArray[i].dEnvMin); StaArray[i].dEnvMin = NULL; }
                if (StaArray[i].dEnvMax) { free(StaArray[i].dEnvMax); StaArray[i].dEnvMax = NULL; }
                if (StaArray[i].iEnvHas) { free(StaArray[i].iEnvHas); StaArray[i].iEnvHas = NULL; }
                StaArray[i].iEnvCap = 0;
                StaArray[i].bEnvValid = FALSE;
            }
        }
        iNumStas = active_count; 
        RecalcTrackHeight();
    }
    gtk_widget_destroy(dialog);
}

 /***********************************************************************
  *                    on_reload_default_activate()                     *
  *             Reloads the default station list, discarding the        *
  *             current data, and recomputes the track height.          *
  *               Nothing.                                              *
  ***********************************************************************/

void on_reload_default_activate(GtkWidget *widget, gpointer data) {
    FreeAllStations();
    LoadStationsFromFile();
    RecalcTrackHeight();
}

 /***********************************************************************
  *                        ConnectToEarthworm()                         *
  *             Resolves the ring keys, the message types and the       *
  *             installation and module ids, and attaches to the        *
  *             wave and pick rings.                                    *
  *               Nothing; exits if a ring is not registered.           *
  ***********************************************************************/

void ConnectToEarthworm() {
    long WaveRingKey = GetKey(WaveRingName);
    long PickRingKey = GetKey(PickRingName);
    unsigned char InstId, ModId, TypeTrace;
    
    if (WaveRingKey == -1) exit(-1);
    if (PickRingKey == -1) exit(-1);

    GetInst( "INST_WILDCARD", &InstId );
    GetModId( "MOD_WILDCARD", &ModId );
    
    GetType( "TYPE_TRACEBUF", &TypeTrace );
    WaveLogo[0].instid = InstId; WaveLogo[0].mod = ModId; WaveLogo[0].type = TypeTrace;
    tport_attach( &WaveRegion, WaveRingKey );
    
    PickLogo[0].instid = 0; PickLogo[0].mod = 0; PickLogo[0].type = 0; 
    tport_attach( &PickRegion, PickRingKey );

    if (GetType("TYPE_PICKTWC", &TypePickTWC) != 0) TypePickTWC = 0;
    if (GetType("TYPE_HEARTBEAT", &TypeHeartBeat) != 0) TypeHeartBeat = 0;
    if (GetType("TYPE_ERROR", &TypeError) != 0) TypeError = 0;
    if (GetLocalInst(&MyInstId) != 0) MyInstId = 0;
    if (GetModId(MyModName, &MyModId) != 0) {
        if (GetModId("MOD_WILDCARD", &MyModId) != 0) MyModId = 0;
    }
}

 /***********************************************************************
  *                              Status()                               *
  *             Sends a heartbeat or error message to the wave ring so  *
  *             startstop knows the module is alive.                    *
  *               Nothing.                                              *
  ***********************************************************************/

void Status(unsigned char type, short ierr, char *note) {
    MSG_LOGO logo; char msg[256]; time_t t;
    logo.instid = MyInstId; logo.mod = MyModId; logo.type = type;
    time(&t);
    if (type == TypeHeartBeat) sprintf(msg, "%ld %d\n", (long) t, (int) MyPid);
    else if (type == TypeError) {
        sprintf(msg, "%ld %hd %s\n", (long) t, ierr, note);
        logit("et", "new_develo: Error: %s\n", note);
    }
    tport_putmsg(&WaveRegion, &logo, (long) strlen(msg), msg);
}

 /***********************************************************************
  *                           ReadSampleAt()                            *
  *             Decodes one TRACE_BUF sample of the given datatype:     *
  *             i2/s2 (int16), i4/s4 (int32) or f4/t4 (float32).        *
  *               0 on success, -1 if the datatype is not supported.    *
  ***********************************************************************/

static int ReadSampleAt(const char *szType, const char *pData, int idx, int32_t *out) {
    if (strcmp(szType, "i2") == 0 || strcmp(szType, "s2") == 0) {
        int16_t v; memcpy(&v, pData + idx * 2, 2); *out = v; return 0;
    }
    if (strcmp(szType, "i4") == 0 || strcmp(szType, "s4") == 0) {
        int32_t v; memcpy(&v, pData + idx * 4, 4); *out = v; return 0;
    }
    if (strcmp(szType, "f4") == 0 || strcmp(szType, "t4") == 0) {
        float v; memcpy(&v, pData + idx * 4, 4);
        *out = (int32_t)(v + (v >= 0.0f ? 0.5f : -0.5f)); return 0;
    }
    *out = 0;
    return -1;
}

 /***********************************************************************
  *                      on_canvas_button_press()                       *
  *             GTK handler for clicks on a waveform while on HOLD:     *
  *             creates a manual P pick at the clicked time,            *
  *             replacing an existing nearby pick, and reports it       *
  *               TRUE if handled, FALSE otherwise.                     *
  ***********************************************************************/

static gboolean on_canvas_button_press(GtkWidget *widget, GdkEventButton *event, gpointer data) {
    if (!g_is_hold) return FALSE; 
    if (event->button != 1) return FALSE; 

    guint width = gtk_widget_get_allocated_width(widget);
    double draw_area_width = width - PANEL_WIDTH;
    if (event->x <= PANEL_WIDTH) return FALSE; 

    int sta_idx = (int)(event->y / dTrackHeight);
    if (sta_idx < 0 || sta_idx >= iNumStas) return FALSE;

    double window_secs = iTimeWindowMinutes * 60.0;
    double t_right = g_is_hold ? g_t_hold_time : g_smooth_time;
    double t_left = t_right - window_secs;

    double fraction = (event->x - PANEL_WIDTH) / draw_area_width;
    double clicked_time = t_left + fraction * window_secs;

    DEV_STATION *dev = &StaArray[sta_idx];

    /* Window-based replacement (PickReplaceWindow): if a pick already exists
       for the same station within +/-g_pick_replace_secs, a KO
       (iUseMe=0) of the old one is sent so that the associator discards it, and it
       is marked as deleted locally. The new pick arrives with a fresh lPickIndex. */
    if (g_pick_replace_secs > 0.0) {
        for (int pi = 0; pi < MAX_PICKS_PER_STA; pi++) {
            if (dev->picks[pi].iUseMe > 0 &&
                fabs(dev->picks[pi].dTime - clicked_time) <= g_pick_replace_secs) {
                PPICK old;
                InitP(&old);
                strcpy(old.szStation, dev->szStation);
                strcpy(old.szChannel, dev->szChannel);
                strcpy(old.szNetID, dev->szNetID);
                old.dLat = AtwcStaArray[sta_idx].dLat;
                old.dLon = AtwcStaArray[sta_idx].dLon;
                GeoCent((LATLON *) &old);
                GetLatLonTrig((LATLON *) &old);
                old.dPTime = dev->picks[pi].dTime;
                strncpy(old.szPhase, dev->picks[pi].szPhase, 7);
                old.szPhase[7] = '\0';
                old.lPickIndex = dev->picks[pi].lPickIndex;
                old.iUseMe = 0;
                ReportPick(&old, &AtwcStaArray[sta_idx], MyModId, PickRegion, TypePickTWC, MyInstId, 2);
                logit("et", "new_develo: replacing pick %ld of %s (%.3f -> %.3f)\n",
                      old.lPickIndex, dev->szStation, old.dPTime, clicked_time);
                dev->picks[pi].iUseMe = 0;
            }
        }
    }

    InitP(&StaArray[sta_idx].pick);
    strcpy(StaArray[sta_idx].pick.szStation, StaArray[sta_idx].szStation);
    strcpy(StaArray[sta_idx].pick.szChannel, StaArray[sta_idx].szChannel);
    strcpy(StaArray[sta_idx].pick.szNetID, StaArray[sta_idx].szNetID);
    
    StaArray[sta_idx].pick.dLat = AtwcStaArray[sta_idx].dLat;
    StaArray[sta_idx].pick.dLon = AtwcStaArray[sta_idx].dLon;
    GeoCent((LATLON *) &StaArray[sta_idx].pick);
    GetLatLonTrig((LATLON *) &StaArray[sta_idx].pick);

    StaArray[sta_idx].pick.dPTime = clicked_time;
    strcpy(StaArray[sta_idx].pick.szPhase, "P");
    StaArray[sta_idx].pick.cFirstMotion = '?';
    StaArray[sta_idx].pick.iUseMe = 1;
    StaArray[sta_idx].pick.lPickIndex = g_lPickIndexCounter;
    g_lPickIndexCounter = (g_lPickIndexCounter < 19999) ? g_lPickIndexCounter + 1 : 10000;
    StaArray[sta_idx].pick_status = 1;

    /* Show the manual pick immediately at the station, without waiting for it
       to circulate back through PICK_RING (D20). */
    {
        int slot = (int)(dev->lPickRingNext % MAX_PICKS_PER_STA);
        dev->picks[slot].dTime = clicked_time;
        strncpy(dev->picks[slot].szPhase, "P", 7);
        dev->picks[slot].szPhase[7] = '\0';
        dev->picks[slot].lPickIndex = dev->pick.lPickIndex;
        dev->picks[slot].iUseMe = 1;
        dev->lPickRingNext++;
        if (dev->iNumPicks < MAX_PICKS_PER_STA) dev->iNumPicks++;
    }

    /* iType=2: "Data from develo" according to report.c (A1) */
    ReportPick(&StaArray[sta_idx].pick, &AtwcStaArray[sta_idx], MyModId, PickRegion, TypePickTWC, MyInstId, 2);
    gtk_widget_queue_draw(widget);
    return TRUE;
}

/* ====================================================================
 * DATA FETCHING: REALTIME ENGINE (Only Raw, No DSP here)
 * ==================================================================== */
 /***********************************************************************
  *                        fetch_realtime_data()                        *
  *             GTK timeout that drains wave packets into the raw       *
  *             circular buffers, updates the screen clock and picks    *
  *             from PICK_RING, and triggers the envelope pass.         *
  *               G_SOURCE_CONTINUE always (keeps the timer alive).     *
  ***********************************************************************/

gboolean fetch_realtime_data(gpointer user_data) {

    char msg[MAX_TRACE_BYTES]; MSG_LOGO reclogo; long recsize; int res;
    TRACE_HEADER *WaveHead; 

    int64_t current_realtime = g_get_real_time();
    if (g_last_frame_realtime == 0) g_last_frame_realtime = current_realtime;
    double dt = (current_realtime - g_last_frame_realtime) / 1000000.0;
    g_last_frame_realtime = current_realtime;
    double sys_time = (double)current_realtime / 1000000.0;

    /* Earthworm heartbeat (A3) */
    {
        time_t timeNow; time(&timeNow);
        if (timeNow - timeLastBeat >= HeartBeatInt) {
            timeLastBeat = timeNow;
            Status(TypeHeartBeat, 0, "");
        }
    }

    do {
        res = tport_getmsg( &WaveRegion, WaveLogo, 1, &reclogo, &recsize, msg, sizeof(msg) );
        if ( res == GET_OK || res == GET_MISS || res == GET_NOTRACK ) {
            WaveHead = (TRACE_HEADER *) msg;
            if ( WaveMsgMakeLocal( WaveHead ) < 0 ) continue; 
            
            double t_start = WaveHead->starttime;
            double t_end = WaveHead->endtime;
            double rate = WaveHead->samprate;
            if (rate <= 0) rate = 20.0; 
            
            if (fabs(sys_time - t_end) > MAX_REALTIME_SKEW) {
                 if (t_end > g_latest_time) g_latest_time = t_end;
            }

            for ( int i = 0; i < iNumStas; i++ ) {
                if ( !strcmp(WaveHead->sta, StaArray[i].szStation) &&
                     !strcmp(WaveHead->chan, StaArray[i].szChannel) &&
                     !strcmp(WaveHead->net, StaArray[i].szNetID) ) {
                     
                    StaArray[i].dLastPacketSysTime = sys_time;
                    StaArray[i].dSampRate = rate; 
                    g_last_data_realtime = sys_time;
                    
                    int64_t abs_start = (int64_t)(t_start * rate);
                    int64_t abs_end = (int64_t)(t_end * rate);

                    /* Standard gap fill with the empty flag (INT_MAX) */
                    if (StaArray[i].lLastAbsIdx > 0 && abs_start > StaArray[i].lLastAbsIdx) {
                        int64_t gap_samps = abs_start - StaArray[i].lLastAbsIdx;
                        if (gap_samps > StaArray[i].lRawCircSize) gap_samps = StaArray[i].lRawCircSize; 
                        for (int64_t g = 0; g < gap_samps; g++) {
                            int64_t clr_abs = StaArray[i].lLastAbsIdx + g;
                            int idx = CIRC_IDX(clr_abs, StaArray[i].lRawCircSize);
                            StaArray[i].plRawCircBuff[idx] = INT_MAX;
                        }
                        /* The gap modifies data inside the cached window */
                        if (StaArray[i].iProcValid && (abs_start - 1) >= StaArray[i].lProcAbsStart)
                            StaArray[i].iProcValid = 0;
                    }

                    char szType[3]; strncpy(szType, WaveHead->datatype, 2); szType[2] = '\0';
                    char *pRaw = msg + sizeof(TRACE_HEADER);
                    
                    for (int s = 0; s < WaveHead->nsamp; s++) {
                        int32_t x;
                        if (ReadSampleAt(szType, pRaw, s, &x) != 0) {
                            if (!g_warned_datatype) {
                                g_warned_datatype = 1;
                                logit("et", "new_develo: datatype <%s> not supported, reading as int32\n", szType);
                            }
                            memcpy(&x, pRaw + s * sizeof(int32_t), sizeof(int32_t));
                        }
                        
                        double t_samp = t_start + ((double)s / rate);
                        int64_t abs_idx = (int64_t)(t_samp * rate);
                        int idx = CIRC_IDX(abs_idx, StaArray[i].lRawCircSize);
                        
                        StaArray[i].plRawCircBuff[idx] = x;
                    }

                    if (abs_end > StaArray[i].lLastAbsIdx) StaArray[i].lLastAbsIdx = abs_end;
                    /* If the new data touches the processed window in cache, invalidate it */
                    if (StaArray[i].iProcValid && abs_start <= StaArray[i].lProcAbsEnd)
                        StaArray[i].iProcValid = 0;
                    break; 
                }
            }
        }
    } while ( res != GET_NONE ); 

    if (!g_is_hold) {
        if (g_last_data_realtime > 0.0 && (sys_time - g_last_data_realtime) > DATA_STALE_SECS) {
            /* Dead feed: do not advance the screen clock towards the void (D16) */
            g_data_stale = TRUE;
        } else {
            g_data_stale = FALSE;
            gboolean is_realtime = TRUE;
            if (g_latest_time > 0.0 && fabs(sys_time - g_latest_time) > MAX_REALTIME_SKEW) is_realtime = FALSE;

            if (is_realtime) {
                g_latest_time = sys_time - 2.0;
                g_smooth_time = g_latest_time; 
            } else {
                if (g_smooth_time == 0.0) g_smooth_time = g_latest_time;
                else {
                    g_smooth_time += dt;
                    if (fabs(g_latest_time - g_smooth_time) > 2.0) g_smooth_time = g_latest_time;
                    else if (g_latest_time > g_smooth_time) g_smooth_time += dt * 0.1;
                    else if (g_latest_time < g_smooth_time) g_smooth_time -= dt * 0.1;
                }
            }
        }
    }

    gboolean picks_changed = FALSE;

    do {
        res = tport_getmsg( &PickRegion, PickLogo, 1, &reclogo, &recsize, msg, sizeof(msg) - 1 );
        if ( res == GET_OK || res == GET_MISS || res == GET_NOTRACK ) {
            msg[recsize] = '\0'; 
            unsigned char t1, t2;
            GetType("TYPE_TRACEBUF", &t1); GetType("TYPE_TRACEBUF2", &t2);
            if (reclogo.type == t1 || reclogo.type == t2) continue;
            
            if (TypePickTWC != 0 && reclogo.type == TypePickTWC) {
                PPICK pPick;
                memset(&pPick, 0, sizeof(pPick));
                /* B7: robust parsing with PPickStruct (same format of report.c) */
                if (PPickStruct(msg, &pPick, TypePickTWC) == 0) {
                    for ( int i = 0; i < iNumStas; i++ ) {
                        if ( !strcmp(AtwcStaArray[i].szStation, pPick.szStation) &&
                             !strncmp(AtwcStaArray[i].szChannel, pPick.szChannel, 3) &&
                             !strcmp(AtwcStaArray[i].szNetID, pPick.szNetID) ) {
                            DEV_STATION *dev = &StaArray[i];
                            if ( pPick.iUseMe <= 0 ) {
                                /* KO: delete the pick with that lPickIndex locally */
                                for ( int k = 0; k < MAX_PICKS_PER_STA; k++ ) {
                                    if ( dev->picks[k].lPickIndex == pPick.lPickIndex &&
                                         dev->picks[k].iUseMe > 0 ) {
                                        dev->picks[k].iUseMe = 0;
                                    }
                                }
                                picks_changed = TRUE;
                            } else {
                                /* Upsert by lPickIndex: update if it already exists
                                   (avoids the double count of the manual pick when the
                                   message comes back through the ring), otherwise insert. */
                                int found = -1;
                                for ( int k = 0; k < MAX_PICKS_PER_STA; k++ ) {
                                    if ( dev->picks[k].lPickIndex == pPick.lPickIndex &&
                                         dev->picks[k].iUseMe > 0 ) {
                                        found = k;
                                        break;
                                    }
                                }
                                if ( found >= 0 ) {
                                    dev->picks[found].dTime = pPick.dPTime;
                                    memcpy(dev->picks[found].szPhase, pPick.szPhase, 7);
                                    dev->picks[found].szPhase[7] = '\0';
                                    dev->picks[found].iUseMe = pPick.iUseMe;
                                } else {
                                    int slot = (int)(dev->lPickRingNext % MAX_PICKS_PER_STA);
                                    dev->picks[slot].dTime = pPick.dPTime;
                                    memcpy(dev->picks[slot].szPhase, pPick.szPhase, 7);
                                    dev->picks[slot].szPhase[7] = '\0';
                                    dev->picks[slot].lPickIndex = pPick.lPickIndex;
                                    dev->picks[slot].iUseMe = pPick.iUseMe;
                                    dev->lPickRingNext++;
                                    if (dev->iNumPicks < MAX_PICKS_PER_STA) dev->iNumPicks++;
                                }
                                picks_changed = TRUE;
                            }
                            break;
                        }
                    }
                }
            }
        }
    } while ( res != GET_NONE );

    /* A1/A2/A3: refresh the envelope cache at its rhythm (2Hz) and redraw
       only if something changed (new envelope, picks, or forced change). */
    g_envelope_updated = FALSE;
    update_wave_envelopes();

    if (g_envelope_updated || picks_changed) {
        if (g_drawing_waves) gtk_widget_queue_draw(g_drawing_waves);
        if (drawing_axis) gtk_widget_queue_draw(drawing_axis);
    }

    return G_SOURCE_CONTINUE; 
}

/* ====================================================================
 * A2/A3: CACHED PROCESSING AND ENVELOPE DECIMATED BY COLUMN
 * --------------------------------------------------------------------
 * The processing (extract+interp+DC+filter) and the min/max scan per column
 * are done ONLY in the envelope pass (ENV_PROCESS_MS rhythm). The draw
 * reads the raw envelope (without auto_scale) and scales it on the fly, so
 * the vertical zoom does not invalidate the cache.
 * ==================================================================== */
 /***********************************************************************
  *                     compute_station_envelope()                      *
  *             Extracts, interpolates, de-DC and filters one           *
  *             station window (using the cached trace), then           *
  *             computes the min/max envelope per pixel column.         *
  *               Nothing.                                              *
  ***********************************************************************/

static void compute_station_envelope(int i, int width) {
    DEV_STATION *sta = &StaArray[i];
    double rate = sta->dSampRate > 0 ? sta->dSampRate : 20.0;
    double window_secs = iTimeWindowMinutes * 60.0;
    double t_right = g_is_hold ? g_t_hold_time : g_smooth_time;
    double t_left = t_right - window_secs;

    /* MAGIC PADDING: 30s backwards so that the filter ringing dies */
    double pad_secs = 30.0;
    double extract_start = t_left - pad_secs;

    int64_t abs_start = (int64_t)(extract_start * rate);
    int64_t abs_end = (int64_t)(t_right * rate);
    long num_samps = abs_end - abs_start;

    if (num_samps <= 0 || num_samps > sta->lRawCircSize) return;

    /* 1. CACHE OF THE PROCESSED TRACE: if the window, the filter and the data
       inside the window did not change, reuse plProcBuf. */
    int cache_hit =
        sta->iProcValid &&
        sta->lProcAbsStart == abs_start &&
        sta->lProcAbsEnd == abs_end &&
        sta->iProcFilterType == g_filter_type &&
        sta->iProcOrder == g_order &&
        sta->dProcF1 == g_f1 &&
        sta->dProcF2 == g_f2;

    long *trace_buf;
    int found_first;

    if (!cache_hit) {
        if (sta->lProcCap < num_samps) {
            long *nb = (long *) realloc(sta->plProcBuf, num_samps * sizeof(long));
            if (!nb) return;
            sta->plProcBuf = nb;
            sta->lProcCap = num_samps;
        }
        trace_buf = sta->plProcBuf;

        /* 2. EXTRACTION TO LINEAR ARRAY */
        for (long k = 0; k < num_samps; k++) {
            int64_t abs_k = abs_start + k;
            if (abs_k > sta->lLastAbsIdx || abs_k < 0) {
                trace_buf[k] = INT_MAX;
            } else {
                trace_buf[k] = sta->plRawCircBuff[CIRC_IDX(abs_k, sta->lRawCircSize)];
            }
        }

        /* 3. GAP INTERPOLATION TO AVOID FILTERING SPIKES */
        long last_valid = 0;
        long gap_start = -1;
        found_first = 0;
        for (long k = 0; k < num_samps; k++) {
            if (trace_buf[k] != INT_MAX) { last_valid = trace_buf[k]; found_first = 1; break; }
        }

        if (found_first) {
            for (long k = 0; k < num_samps; k++) { if (trace_buf[k] != INT_MAX) break; trace_buf[k] = last_valid; }
            for (long k = 0; k < num_samps; k++) {
                if (trace_buf[k] == INT_MAX) {
                    if (gap_start == -1) gap_start = k;
                } else {
                    if (gap_start != -1) {
                        long gap_len = k - gap_start;
                        long next_valid = trace_buf[k];
                        for (long g = gap_start; g < k; g++) {
                            double frac = (double)(g - gap_start + 1) / (double)(gap_len + 1);
                            trace_buf[g] = last_valid + (long)(frac * (next_valid - last_valid));
                        }
                        gap_start = -1;
                    }
                    last_valid = trace_buf[k];
                }
            }
            if (gap_start != -1) { for (long g = gap_start; g < num_samps; g++) trace_buf[g] = last_valid; }
        } else {
            for (long k = 0; k < num_samps; k++) trace_buf[k] = 0;
        }

        /* 4. REMOVE DC (BASELINE) */
        double sum = 0;
        for (long k = 0; k < num_samps; k++) sum += (double)trace_buf[k];
        long mean = (long)(sum / num_samps);
        for (long k = 0; k < num_samps; k++) trace_buf[k] -= mean;

        /* 5. APPLY FILTERS USING THE DSP LIBRARY */
        if (g_filter_type != 0 && found_first) {
            double nyquist = rate / 2.0;
            double safe_f1 = g_f1, safe_f2 = g_f2;

            if (safe_f1 >= nyquist) safe_f1 = nyquist * 0.95;
            if (g_filter_type == 3) {
                if (safe_f2 >= nyquist) safe_f2 = nyquist * 0.95;
                if (safe_f1 >= safe_f2) safe_f1 = safe_f2 * 0.5;
            }

            if (g_filter_type == 1) aplicar_filtro_iir(trace_buf, num_samps, rate, 1, safe_f1, g_order);
            else if (g_filter_type == 2) aplicar_filtro_iir(trace_buf, num_samps, rate, 2, safe_f1, g_order);
            else if (g_filter_type == 3) {
                aplicar_filtro_iir(trace_buf, num_samps, rate, 1, safe_f1, g_order);
                aplicar_filtro_iir(trace_buf, num_samps, rate, 2, safe_f2, g_order);
            }
        }

        sta->iProcValid = 1;
        sta->iProcFoundFirst = found_first;
        sta->lProcAbsStart = abs_start;
        sta->lProcAbsEnd = abs_end;
        sta->iProcFilterType = g_filter_type;
        sta->iProcOrder = g_order;
        sta->dProcF1 = g_f1;
        sta->dProcF2 = g_f2;
    } else {
        trace_buf = sta->plProcBuf;
        found_first = sta->iProcFoundFirst;
    }

    /* 6. max_abs over the drawn range (for auto_scale in the draw) */
    long draw_start_idx = (long)(pad_secs * rate);
    if (draw_start_idx > num_samps) draw_start_idx = 0;

    long real_samps_end = (long)(sta->lLastAbsIdx - abs_start + 1);
    if (real_samps_end > num_samps) real_samps_end = num_samps;
    if (real_samps_end < 0) real_samps_end = 0;

    double max_abs = 0.0;
    gboolean has_data = FALSE;
    for (long k = draw_start_idx; k < real_samps_end; k++) {
        double abs_val = fabs((double)trace_buf[k]);
        if (abs_val > max_abs) max_abs = abs_val;
        has_data = TRUE;
    }
    if (max_abs < 1.0 || !has_data || !found_first) max_abs = 1.0;
    sta->dEnvMaxAbs = max_abs;
    sta->iEnvFoundFirst = found_first;

    /* 7. Envelope (min/max) per pixel column, in raw units */
    if (sta->iEnvCap < width) {
        int new_cap = width + 256;
        double *nmin = (double *) realloc(sta->dEnvMin, new_cap * sizeof(double));
        double *nmax = (double *) realloc(sta->dEnvMax, new_cap * sizeof(double));
        int *nhas = (int *) realloc(sta->iEnvHas, new_cap * sizeof(int));
        if (!nmin || !nmax || !nhas) {
            if (nmin) free(nmin);
            if (nmax) free(nmax);
            if (nhas) free(nhas);
            sta->bEnvValid = FALSE;
            return;
        }
        sta->dEnvMin = nmin; sta->dEnvMax = nmax; sta->iEnvHas = nhas;
        sta->iEnvCap = new_cap;
    }
    sta->iEnvWidth = width;

    if (found_first) {
        for (int px = 0; px < width; px++) {
            double px_t_start = t_left + ((double)px / width) * window_secs;
            double px_t_end = t_left + ((double)(px + 1) / width) * window_secs;

            long p_local_start = (long)((px_t_start - extract_start) * rate);
            long p_local_end = (long)((px_t_end - extract_start) * rate);
            if (p_local_end == p_local_start) p_local_end++;

            if (p_local_start < 0) p_local_start = 0;
            if (p_local_end > num_samps) p_local_end = num_samps;

            double p_min = 1e12, p_max = -1e12;
            gboolean px_has_data = FALSE;

            for (long local_k = p_local_start; local_k < p_local_end; local_k++) {
                if (local_k >= real_samps_end) continue;
                double val = (double)trace_buf[local_k];
                if (val < p_min) p_min = val;
                if (val > p_max) p_max = val;
                px_has_data = TRUE;
            }

            if (px_has_data) {
                sta->dEnvMin[px] = p_min;
                sta->dEnvMax[px] = p_max;
                sta->iEnvHas[px] = 1;
            } else {
                sta->iEnvHas[px] = 0;
            }
        }
    }

    sta->bEnvValid = TRUE;
}

 /***********************************************************************
  *                       update_wave_envelopes()                       *
  *             Controls the envelope recompute cadence (2Hz or one     *
  *             pixel of scroll) and calls compute_station_envelope     *
  *             for every station when a refresh is due.                *
  *               Nothing.                                              *
  ***********************************************************************/

static void update_wave_envelopes(void) {
    if (iNumStas == 0 || !g_drawing_waves) return;

    int width = gtk_widget_get_allocated_width(g_drawing_waves) - PANEL_WIDTH;
    if (width <= 0) return;

    double t_right = g_is_hold ? g_t_hold_time : g_smooth_time;
    int64_t now_ms = g_get_monotonic_time() / 1000;
    gboolean force = g_bForceEnv;
    g_bForceEnv = FALSE;

    gboolean should = FALSE;
    if (force) {
        should = TRUE;
    } else if (g_is_hold) {
        /* Frozen window: only if it has not been computed yet or the width changed */
        if (g_last_env_width != width || !g_last_env_ok) should = TRUE;
    } else {
        /* No motion nor data: nothing to refresh */
        if (!(fabs(t_right - g_last_env_t_right) < 0.5 && g_data_stale)) {
            /* Adaptive cadence: reprocess when the scroll advances ~1px
               (1px of data), with a minimum cap of ENV_PROCESS_MS and a maximum
               1.5s for very long windows. With TimeWindow=20min/934px it gives ~0.8Hz
               instead of 2Hz, which halves the reprocessing. */
            double window_secs = iTimeWindowMinutes * 60.0;
            double interval_ms = (window_secs / width) * 1000.0;
            if (interval_ms < ENV_PROCESS_MS) interval_ms = ENV_PROCESS_MS;
            if (interval_ms > 1500.0) interval_ms = 1500.0;

            if ((now_ms - g_last_env_process_ms) >= (int64_t)interval_ms || g_last_env_width != width)
                should = TRUE;
        }
    }
    if (!should) return;

    g_envelope_updated = TRUE;
    g_last_env_process_ms = now_ms;
    g_last_env_t_right = t_right;
    g_last_env_width = width;
    g_last_env_ok = TRUE;

    for (int i = 0; i < iNumStas; i++) compute_station_envelope(i, width);
}

/* ====================================================================
 * RENDER AND ON-THE-FLY PROCESSING WITH ANTI-SPIKES AND FUTURE PROTECTION
 * ==================================================================== */
 /***********************************************************************
  *                           on_draw_waves()                           *
  *             Cairo draw handler: paints the station labels and the   *
  *             separators, the cached waveform envelope scaled by the  *
  *             zoom, and the manual picks.                             *
  *               FALSE always.                                         *
  ***********************************************************************/

gboolean on_draw_waves(GtkWidget *widget, cairo_t *cr, gpointer user_data) {
    if (iNumStas == 0 || g_latest_time <= 0.0) return FALSE; 

    guint width = gtk_widget_get_allocated_width(widget);
    guint height = gtk_widget_get_allocated_height(widget);
    
    cairo_set_source_rgb(cr, g_color_bg[0], g_color_bg[1], g_color_bg[2]); cairo_paint(cr);
    cairo_set_source_rgb(cr, 0.96, 0.96, 0.96); cairo_rectangle(cr, 0, 0, PANEL_WIDTH, height); cairo_fill(cr);
    
    double draw_area_width = width - PANEL_WIDTH;
    if (draw_area_width <= 0) return FALSE;

    double font_size = dTrackHeight * 0.7;
    if (font_size > 12.0) font_size = 12.0;
    if (font_size < 4.0) font_size = 4.0; 

    cairo_select_font_face(cr, "Monospace", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, font_size);

    double window_secs = iTimeWindowMinutes * 60.0;
    double t_right = g_is_hold ? g_t_hold_time : g_smooth_time;
    double t_left = t_right - window_secs;

    for (int i = 0; i < iNumStas; i++) {
        double y_top = i * dTrackHeight;
        double y_center = y_top + (dTrackHeight / 2.0);

        cairo_set_source_rgb(cr, g_color_font[0], g_color_font[1], g_color_font[2]);
        cairo_move_to(cr, 10, y_center + (font_size * 0.35));
        char label[64]; 
        snprintf(label, sizeof(label), "%-5s %s", StaArray[i].szStation, StaArray[i].szNetID); 
        cairo_show_text(cr, label);

        cairo_set_source_rgb(cr, g_color_sep[0], g_color_sep[1], g_color_sep[2]); /* Dynamic separator color */
        cairo_move_to(cr, PANEL_WIDTH, y_top); cairo_line_to(cr, width, y_top); cairo_stroke(cr);

        /* A2/A3: the draw uses the cached envelope (raw values). If it is
           not ready yet (startup or resize before the pass), the trace is
           skipped but the picks are still drawn. */
        DEV_STATION *sta = &StaArray[i];
        if (sta->bEnvValid && sta->iEnvCap >= (int)draw_area_width && sta->iEnvFoundFirst) {
            double max_abs = sta->dEnvMaxAbs;
            if (max_abs < 1.0) max_abs = 1.0;
            double auto_scale = (dTrackHeight * 0.425) / max_abs;
            auto_scale *= g_zoom_factor;

            cairo_save(cr);
            cairo_rectangle(cr, PANEL_WIDTH, y_top, draw_area_width, dTrackHeight);
            cairo_clip(cr);
            cairo_set_source_rgb(cr, g_color_wave[0], g_color_wave[1], g_color_wave[2]);
            /* No antialias + integer coordinates: cairo uses its fast-path of
               boxes (a single fill, without per-subpath tessellation). The stroke
               stays pixel-crisp, just like the stroke bars used to look. */
            cairo_set_antialias(cr, CAIRO_ANTIALIAS_NONE);

            /* A4(a): a 1px bar per column from the raw min/max envelope,
               scaled here (so the zoom does not invalidate the cache). */
            int ncol = (int)draw_area_width;
            for (int px = 0; px < ncol; px++) {
                if (!sta->iEnvHas[px]) continue;
                double s_max = sta->dEnvMax[px] * auto_scale;
                double s_min = sta->dEnvMin[px] * auto_scale;
                if (s_max - s_min < 2.0) {
                    double avg = (s_max + s_min) / 2.0;
                    s_max = avg + 1.0;
                    s_min = avg - 1.0;
                }
                double x = PANEL_WIDTH + px;
                cairo_rectangle(cr, x, y_center - s_max, 1.0, s_max - s_min);
            }
            cairo_fill(cr);
            cairo_restore(cr);
        }

        cairo_set_source_rgb(cr, 1.0, 0.0, 0.0); cairo_set_line_width(cr, 2.0);

        /* Picks: traverse the last iNumPicks of the circular ring (D19) */
        int n_draw = (StaArray[i].iNumPicks < MAX_PICKS_PER_STA) ? StaArray[i].iNumPicks : MAX_PICKS_PER_STA;
        int start_slot = (int)((StaArray[i].lPickRingNext - n_draw + MAX_PICKS_PER_STA) % MAX_PICKS_PER_STA);
        for (int pi = 0; pi < n_draw; pi++) {
            int p = (start_slot + pi) % MAX_PICKS_PER_STA;
            if (StaArray[i].picks[p].iUseMe <= 0 || StaArray[i].picks[p].dTime <= 0.0) continue;
            double pTime = StaArray[i].picks[p].dTime;
            if (pTime >= t_left && pTime <= t_right + 15.0) {
                double fraction = (pTime - t_left) / window_secs;
                double x_pos = PANEL_WIDTH + (fraction * draw_area_width);
                if (x_pos > PANEL_WIDTH && x_pos < width) {
                    cairo_move_to(cr, x_pos, y_top);
                    cairo_line_to(cr, x_pos, y_top + dTrackHeight);
                    cairo_stroke(cr);
                    cairo_move_to(cr, x_pos + 4, y_top + font_size + 2);
                    cairo_show_text(cr, StaArray[i].picks[p].szPhase);
                }
            }
        }
    }
    
    cairo_set_source_rgb(cr, 0.7, 0.7, 0.7); cairo_move_to(cr, PANEL_WIDTH, 0); cairo_line_to(cr, PANEL_WIDTH, height); cairo_stroke(cr);
    return FALSE;
}

 /***********************************************************************
  *                           on_draw_axis()                            *
  *             Cairo draw handler for the time axis: draws the UTC     *
  *             clock labels at the left, center and right of the window*
  *             and the STALE DATA warning.                             *
  *               FALSE always.                                         *
  ***********************************************************************/

gboolean on_draw_axis(GtkWidget *widget, cairo_t *cr, gpointer user_data) {
    guint width = gtk_widget_get_allocated_width(widget);
    guint height = gtk_widget_get_allocated_height(widget);
    
    cairo_set_source_rgb(cr, 1.0, 1.0, 1.0); cairo_rectangle(cr, 0, 0, width, height); cairo_fill(cr);
    cairo_set_source_rgb(cr, 0.7, 0.7, 0.7); cairo_set_line_width(cr, 2.0); cairo_move_to(cr, 0, 0); cairo_line_to(cr, width, 0); cairo_stroke(cr);

    if (g_latest_time > 0) {
        cairo_set_source_rgb(cr, 0.0, 0.0, 0.0); cairo_set_font_size(cr, 13);
        cairo_select_font_face(cr, "Monospace", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
        
        double window_secs = iTimeWindowMinutes * 60.0; 
        time_t t_right = (time_t)(g_is_hold ? g_t_hold_time : g_smooth_time);
        time_t t_left = (time_t)(t_right - window_secs);
        time_t t_center = (time_t)(t_right - (window_secs / 2.0));

        char str_l[32], str_c[32], str_r[32];
        strftime(str_l, sizeof(str_l), "%H:%M:%S", gmtime(&t_left));
        strftime(str_c, sizeof(str_c), "%H:%M:%S", gmtime(&t_center));
        strftime(str_r, sizeof(str_r), "%H:%M:%S UTC", gmtime(&t_right));

        cairo_move_to(cr, 10, height - 10); cairo_show_text(cr, "UTC TIME");
        cairo_move_to(cr, PANEL_WIDTH + 10, height - 10); cairo_show_text(cr, str_l);
        cairo_move_to(cr, PANEL_WIDTH + (width - PANEL_WIDTH)/2 - 30, height - 10); cairo_show_text(cr, str_c);
        cairo_move_to(cr, width - 100, height - 10); cairo_show_text(cr, str_r);

        if (g_data_stale) {
            cairo_set_source_rgb(cr, 1.0, 0.0, 0.0);
            cairo_move_to(cr, 10, 14);
            cairo_show_text(cr, "STALE DATA");
        }
    }
    return FALSE;
}

 /***********************************************************************
  *                      on_filter_menu_activate()                      *
  *             Shows the filter settings dialog (type, F1, F2,         *
  *             order) and applies the chosen values, forcing a         *
  *               Nothing.                                              *
  ***********************************************************************/

void on_filter_menu_activate(GtkWidget *widget, gpointer data) {
    GtkWidget *window = GTK_WIDGET(data);
    GtkWidget *dialog = gtk_dialog_new_with_buttons("Filter Settings", GTK_WINDOW(window),
        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
        "_Apply", GTK_RESPONSE_ACCEPT, "_Cancel", GTK_RESPONSE_REJECT, NULL);

    GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 5);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 10);
    gtk_container_set_border_width(GTK_CONTAINER(grid), 15);
    
    GtkWidget *cb_type = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(cb_type), "Raw (No Filter)");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(cb_type), "High-Pass");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(cb_type), "Low-Pass");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(cb_type), "Band-Pass");
    gtk_combo_box_set_active(GTK_COMBO_BOX(cb_type), g_filter_type);
    
    GtkWidget *e_f1 = gtk_entry_new();
    char buf[16]; snprintf(buf, sizeof(buf), "%.2f", g_f1);
    gtk_entry_set_text(GTK_ENTRY(e_f1), buf);
    
    GtkWidget *e_f2 = gtk_entry_new();
    snprintf(buf, sizeof(buf), "%.2f", g_f2);
    gtk_entry_set_text(GTK_ENTRY(e_f2), buf);
    
    GtkWidget *cb_order = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(cb_order), "2");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(cb_order), "4");
    gtk_combo_box_set_active(GTK_COMBO_BOX(cb_order), (g_order==2)?0:1);
    
    gtk_grid_attach(GTK_GRID(grid), gtk_label_new("Type:"), 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), cb_type, 1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), gtk_label_new("F1 (Hz):"), 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), e_f1, 1, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), gtk_label_new("F2 (Hz):"), 0, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), e_f2, 1, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), gtk_label_new("Order:"), 0, 3, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), cb_order, 1, 3, 1, 1);
    
    GtkWidget *entries[3] = {e_f1, e_f2, cb_order};
    g_signal_connect(cb_type, "changed", G_CALLBACK(on_filter_combo_changed), entries);
    on_filter_combo_changed(GTK_COMBO_BOX(cb_type), entries);
    
    gtk_box_pack_start(GTK_BOX(content_area), grid, TRUE, TRUE, 0);
    gtk_widget_show_all(dialog);
    
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        g_filter_type = gtk_combo_box_get_active(GTK_COMBO_BOX(cb_type));
        g_f1 = atof(gtk_entry_get_text(GTK_ENTRY(e_f1)));
        g_f2 = atof(gtk_entry_get_text(GTK_ENTRY(e_f2)));
        g_order = (gtk_combo_box_get_active(GTK_COMBO_BOX(cb_order)) == 0) ? 2 : 4;
        
        g_bForceEnv = TRUE; /* New filter: recompute envelope */
        if (g_drawing_waves) gtk_widget_queue_draw(g_drawing_waves);
    }
    gtk_widget_destroy(dialog);
}

 /***********************************************************************
  *                               main()                                *
  *             Entry point: reads the config, loads the stations and   *
  *             connects to Earthworm, builds the GTK interface and runs*
  *             the main loop.                                          *
  *               0 on clean exit.                                      *
  ***********************************************************************/

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <configfile.d>\n", argv[0]);
        exit(1);
    }

    if (ReadConfig(argv[1]) != 0) {
        fprintf(stderr, "Error reading configuration\n");
        exit(1);
    }

    setenv("TZ", "GMT", 1); tzset(); 
    logit_init(argv[1], 0, 1024, LogFile);
    MyPid = getpid();

    gtk_init(&argc, &argv);
    setlocale(LC_NUMERIC, "C");

    GtkWidget *window, *vbox;
    GtkWidget *menu_bar, *ctrl_panel_item, *ctrl_panel_menu;
    GtkWidget *colours_item, *colours_menu;
    GtkWidget *filter_item, *filter_menu, *set_filter_item;
    GtkWidget *waveforms_item, *background_item, *font_item, *separator_item;
    GtkWidget *stas_per_screen_item, *time_window_item;
    GtkWidget *clean_view_item, *reload_default_item;

    LoadStationsFromFile();
    ConnectToEarthworm(); 

    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    char title[128]; snprintf(title, sizeof(title), "Trace view and picking");
    gtk_window_set_title(GTK_WINDOW(window), title);
    gtk_window_set_default_size(GTK_WINDOW(window), 1024, 768);
    gtk_window_set_resizable(GTK_WINDOW(window), TRUE);
    gtk_window_set_type_hint(GTK_WINDOW(window), GDK_WINDOW_TYPE_HINT_NORMAL);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    
    gtk_widget_add_events(window, GDK_KEY_PRESS_MASK);
    g_signal_connect(window, "key-press-event", G_CALLBACK(on_key_press), NULL);

    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0); gtk_container_add(GTK_CONTAINER(window), vbox);

    menu_bar = gtk_menu_bar_new();
    
    /* --- MENU CONTROL PANEL --- */
    ctrl_panel_item = gtk_menu_item_new_with_label("Control Panel"); ctrl_panel_menu = gtk_menu_new();
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(ctrl_panel_item), ctrl_panel_menu);
    
    stas_per_screen_item = gtk_menu_item_new_with_label("Stations per screen"); g_signal_connect(stas_per_screen_item, "activate", G_CALLBACK(on_stas_per_screen_activate), window);
    gtk_menu_shell_append(GTK_MENU_SHELL(ctrl_panel_menu), stas_per_screen_item);

    time_window_item = gtk_menu_item_new_with_label("Time window"); g_signal_connect(time_window_item, "activate", G_CALLBACK(on_time_window_activate), window);
    gtk_menu_shell_append(GTK_MENU_SHELL(ctrl_panel_menu), time_window_item);
    
    clean_view_item = gtk_menu_item_new_with_label("Clean view"); g_signal_connect(clean_view_item, "activate", G_CALLBACK(on_clean_view_activate), window);
    gtk_menu_shell_append(GTK_MENU_SHELL(ctrl_panel_menu), clean_view_item);

    reload_default_item = gtk_menu_item_new_with_label("Reload default stations"); g_signal_connect(reload_default_item, "activate", G_CALLBACK(on_reload_default_activate), window);
    gtk_menu_shell_append(GTK_MENU_SHELL(ctrl_panel_menu), reload_default_item);
    
    gtk_menu_shell_append(GTK_MENU_SHELL(menu_bar), ctrl_panel_item); 
    
    /* --- MENU COLOURS --- */
    colours_item = gtk_menu_item_new_with_label("Colours"); colours_menu = gtk_menu_new();
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(colours_item), colours_menu);

    waveforms_item = gtk_menu_item_new_with_label("Waveforms");
    g_signal_connect(waveforms_item, "activate", G_CALLBACK(on_colour_select), GINT_TO_POINTER(1));
    gtk_menu_shell_append(GTK_MENU_SHELL(colours_menu), waveforms_item);

    background_item = gtk_menu_item_new_with_label("Background");
    g_signal_connect(background_item, "activate", G_CALLBACK(on_colour_select), GINT_TO_POINTER(2));
    gtk_menu_shell_append(GTK_MENU_SHELL(colours_menu), background_item);

    font_item = gtk_menu_item_new_with_label("Font");
    g_signal_connect(font_item, "activate", G_CALLBACK(on_colour_select), GINT_TO_POINTER(3));
    gtk_menu_shell_append(GTK_MENU_SHELL(colours_menu), font_item);

    separator_item = gtk_menu_item_new_with_label("Separator");
    g_signal_connect(separator_item, "activate", G_CALLBACK(on_colour_select), GINT_TO_POINTER(4));
    gtk_menu_shell_append(GTK_MENU_SHELL(colours_menu), separator_item);

    gtk_menu_shell_append(GTK_MENU_SHELL(menu_bar), colours_item);

    /* --- MENU FILTER --- */
    filter_item = gtk_menu_item_new_with_label("Filter"); filter_menu = gtk_menu_new();
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(filter_item), filter_menu);
    
    set_filter_item = gtk_menu_item_new_with_label("Filter Options");
    g_signal_connect(set_filter_item, "activate", G_CALLBACK(on_filter_menu_activate), window);
    gtk_menu_shell_append(GTK_MENU_SHELL(filter_menu), set_filter_item);

    gtk_menu_shell_append(GTK_MENU_SHELL(menu_bar), filter_item);

    gtk_box_pack_start(GTK_BOX(vbox), menu_bar, FALSE, FALSE, 0);

    GtkWidget *toolbar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5); gtk_widget_set_margin_start(toolbar, 10); gtk_widget_set_margin_end(toolbar, 10); gtk_widget_set_margin_top(toolbar, 5); gtk_widget_set_margin_bottom(toolbar, 5);

    btn_hold = gtk_toggle_button_new_with_label("HOLD (Freeze Screen)"); g_signal_connect(btn_hold, "toggled", G_CALLBACK(on_btn_hold_toggled), NULL);
    gtk_box_pack_start(GTK_BOX(toolbar), btn_hold, FALSE, FALSE, 0);

    GtkWidget *info_label = gtk_label_new(" | (In HOLD) Left Click: Pick wave | Up/Down Arrows: Vertical Zoom"); gtk_box_pack_start(GTK_BOX(toolbar), info_label, FALSE, FALSE, 10);

    gtk_box_pack_start(GTK_BOX(vbox), toolbar, FALSE, FALSE, 0);

    g_scrolled_window = gtk_scrolled_window_new(NULL, NULL); gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(g_scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_vexpand(g_scrolled_window, TRUE); gtk_box_pack_start(GTK_BOX(vbox), g_scrolled_window, TRUE, TRUE, 0);

    g_drawing_waves = gtk_drawing_area_new(); gtk_widget_set_size_request(g_drawing_waves, -1, iNumStas * dTrackHeight);
    gtk_widget_add_events(g_drawing_waves, GDK_BUTTON_PRESS_MASK); g_signal_connect(g_drawing_waves, "button-press-event", G_CALLBACK(on_canvas_button_press), NULL);
    gtk_container_add(GTK_CONTAINER(g_scrolled_window), g_drawing_waves); g_signal_connect(g_drawing_waves, "draw", G_CALLBACK(on_draw_waves), NULL);

    drawing_axis = gtk_drawing_area_new(); gtk_widget_set_size_request(drawing_axis, -1, BOTTOM_AXIS_H); gtk_box_pack_start(GTK_BOX(vbox), drawing_axis, FALSE, FALSE, 0);
    g_signal_connect(drawing_axis, "draw", G_CALLBACK(on_draw_axis), NULL);

    g_timeout_add(REFRESH_MS, fetch_realtime_data, window);

    gtk_widget_show_all(window);
    RecalcTrackHeight();
    gtk_main();

    tport_detach( &WaveRegion ); tport_detach( &PickRegion ); FreeAllStations();
    return 0;
}
