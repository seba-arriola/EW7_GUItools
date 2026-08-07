#include <gtk/gtk.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <limits.h>
#include <locale.h> 
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
#include "dataprocessing.h" /* Librería externa de DSP */

#define REFRESH_MS 50            
#define PANEL_WIDTH 90
#define BOTTOM_AXIS_H 30 
#define MAX_TRACE_BYTES 4096
#define MAX_STATIONS 512 

#define MAX_MINUTES 120                 
#define MAX_SAMP_RATE 100               
#define MAX_PICKS_PER_STA 50           

#define MWP_SECONDS 60.0 
#define MAX_STR 256      

#define CIRC_IDX(abs_val, size) (int)(((abs_val) % (size) + (size)) % (size))

/* --- CONFIGURACION DEL MODULO --- */
char MyModName[MAX_STR] = "MOD_DEVELO";
char WaveRingName[MAX_STR] = "WAVE_RING";
char PickRingName[MAX_STR] = "PICK_RING";
char StaFile[MAX_STR] = "pick_wcatwc.sta";
char StaDataFile[MAX_STR] = "station.dat"; 
char CalibsFile[MAX_STR] = "calibs";
int  HeartBeatInt = 30;
int  LogFile = 1;

/* --- GLOBALES PARA COLORES PERSONALIZADOS --- */
double g_color_bg[3]   = {1.0, 1.0, 1.0}; 
double g_color_wave[3] = {0.0, 0.0, 0.0}; 
double g_color_font[3] = {1.0, 0.0, 0.0}; 
double g_color_sep[3]  = {0.85, 0.85, 0.85}; /* Nuevo color para el separador */

/* --- GLOBALES PARA FILTROS DINAMICOS --- */
int g_filter_type = 0; /* 0=Raw, 1=HP, 2=LP, 3=BP */
double g_f1 = 0.7;
double g_f2 = 2.0;
int g_order = 4;

SHM_INFO  WaveRegion;
SHM_INFO  PickRegion;
MSG_LOGO  WaveLogo[1];
MSG_LOGO  PickLogo[1];                 

/* VARIABLES GLOBALES */
double    g_latest_time = 0.0;
double    g_smooth_time = 0.0;
int64_t   g_last_frame_realtime = 0;
gboolean  g_is_hold = FALSE;
double    g_t_hold_time = 0.0;
double    g_zoom_factor = 1.0; 
unsigned char TypePickTWC = 0;
unsigned char MyModId = 0;
unsigned char MyInstId = 0;

typedef struct {
    double dTime;       
    char szPhase[8];    
} PICK;

typedef struct {
    char szStation[10];
    char szChannel[10];
    char szNetID[10];
    
    double dSampRate; 
    int32_t *plRawCircBuff;
    long lRawCircSize;
    double dScreenScale;
    
    int64_t lLastAbsIdx; 
    double dLastPacketSysTime;
    
    PICK picks[MAX_PICKS_PER_STA];
    int iNumPicks;
    
    int pick_status;    
    PPICK pick;         
} DEV_STATION;

DEV_STATION *StaArray = NULL; 
STATION *AtwcStaArray = NULL; 
int iNumStas = 0;         

GtkWidget *g_scrolled_window;
GtkWidget *g_drawing_waves;
GtkWidget *drawing_axis; 
GtkWidget *btn_hold;

double dTrackHeight = 60.0;  
int iVisStas = 12;            
int iTimeWindowMinutes = 6;  

/* --------------------------------------------------------------------
 * FUNCIONES BASE (ReadConfig, UI, etc)
 * -------------------------------------------------------------------- */
int ReadConfig(char *configfile) {
    int ncommand = 8, nmiss = 0, i;
    char init[10] = {0};
    char *com, *str;

    if (!k_open(configfile)) {
        fprintf(stderr, "new_develo: Error abriendo archivo config <%s>\n", configfile);
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
        else continue;

        if (k_err()) {
            fprintf(stderr, "new_develo: Error parseando <%s> en <%s>\n", com, configfile);
            return -1;
        }
    }
    for (i = 0; i < ncommand; i++) if (!init[i]) nmiss++;
    k_close();
    if (nmiss > 0) {
        fprintf(stderr, "new_develo: ERROR, faltan parametros en <%s>\n", configfile);
        return -1;
    }
    return 0;
}

void FreeAllStations() {
    if (StaArray) {
        for (int i = 0; i < iNumStas; i++) {
            if (StaArray[i].plRawCircBuff) { free(StaArray[i].plRawCircBuff); StaArray[i].plRawCircBuff = NULL; }
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

void on_btn_hold_toggled(GtkToggleButton *togglebutton, gpointer user_data) {
    g_is_hold = gtk_toggle_button_get_active(togglebutton);
    if (g_is_hold) {
        if (g_smooth_time > 0.0) g_t_hold_time = g_smooth_time;
        else { time_t t; time(&t); g_t_hold_time = (double)t; }
    } else {
        g_zoom_factor = 1.0;
    }
}

static gboolean on_key_press(GtkWidget *widget, GdkEventKey *event, gpointer data) {
    if (!g_is_hold) return FALSE; 
    if (event->keyval == GDK_KEY_Up) {
        g_zoom_factor *= 1.5; 
        if (g_drawing_waves) gtk_widget_queue_draw(g_drawing_waves);
        return TRUE;
    } else if (event->keyval == GDK_KEY_Down) {
        g_zoom_factor /= 1.5; 
        if (g_drawing_waves) gtk_widget_queue_draw(g_drawing_waves);
        return TRUE;
    }
    return FALSE;
}

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
        
        if (g_drawing_waves) gtk_widget_queue_draw(g_drawing_waves);
    }
    gtk_widget_destroy(dialog);
}

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
        GtkAllocation alloc;
        gtk_widget_get_allocation(g_scrolled_window, &alloc);
        if (alloc.height > 0) {
            dTrackHeight = (double)alloc.height / iVisStas;
            if (dTrackHeight < 2.0) dTrackHeight = 2.0; 
            gtk_widget_set_size_request(g_drawing_waves, -1, iNumStas * dTrackHeight);
            if (g_drawing_waves) gtk_widget_queue_draw(g_drawing_waves);
        }
    }
    gtk_widget_destroy(dialog);
}

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
        if (g_drawing_waves) gtk_widget_queue_draw(g_drawing_waves);
        if (drawing_axis) gtk_widget_queue_draw(drawing_axis);
    }
    gtk_widget_destroy(dialog);
}

/* --------------------------------------------------------------------
 * MANEJADOR DEL FILTRO DINAMICO INTERACTIVO
 * -------------------------------------------------------------------- */
static void on_filter_combo_changed(GtkComboBox *widget, gpointer data) {
    GtkWidget **entries = (GtkWidget **)data;
    int type = gtk_combo_box_get_active(widget);
    gtk_widget_set_sensitive(entries[0], (type != 0)); /* F1 (HP, LP, BP) */
    gtk_widget_set_sensitive(entries[1], (type == 3)); /* F2 (Solo Band-Pass) */
    gtk_widget_set_sensitive(entries[2], (type != 0)); /* Orden (HP, LP, BP) */
}

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
        
        StaArray[i].dScreenScale = 0.05; 
        StaArray[i].iNumPicks = 0; 
        StaArray[i].lLastAbsIdx = 0; 
        StaArray[i].dLastPacketSysTime = 0.0; 
        StaArray[i].pick_status = 0;
        InitP(&StaArray[i].pick);
    }
}

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
                }
                active_count++;
            } else {
                if (StaArray[i].plRawCircBuff) {
                    free(StaArray[i].plRawCircBuff);
                    StaArray[i].plRawCircBuff = NULL;
                }
            }
        }
        iNumStas = active_count; 

        GtkAllocation alloc;
        gtk_widget_get_allocation(g_scrolled_window, &alloc);
        if (alloc.height > 0 && iVisStas > 0) {
            dTrackHeight = (double)alloc.height / iVisStas;
            if (dTrackHeight < 2.0) dTrackHeight = 2.0; 
            gtk_widget_set_size_request(g_drawing_waves, -1, iNumStas * dTrackHeight);
        }
        if (g_drawing_waves) gtk_widget_queue_draw(g_drawing_waves);
    }
    gtk_widget_destroy(dialog);
}

void on_reload_default_activate(GtkWidget *widget, gpointer data) {
    FreeAllStations();
    LoadStationsFromFile();
    GtkAllocation alloc;
    gtk_widget_get_allocation(g_scrolled_window, &alloc);
    if (alloc.height > 0 && iVisStas > 0) {
        dTrackHeight = (double)alloc.height / iVisStas;
        if (dTrackHeight < 2.0) dTrackHeight = 2.0; 
        gtk_widget_set_size_request(g_drawing_waves, -1, iNumStas * dTrackHeight);
    }
    if (g_drawing_waves) gtk_widget_queue_draw(g_drawing_waves);
}

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
    if (GetLocalInst(&MyInstId) != 0) MyInstId = 0;
    if (GetModId(MyModName, &MyModId) != 0) {
        if (GetModId("MOD_WILDCARD", &MyModId) != 0) MyModId = 0;
    }
}

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
    StaArray[sta_idx].pick_status = 1;

    ReportPick(&StaArray[sta_idx].pick, &AtwcStaArray[sta_idx], MyModId, PickRegion, TypePickTWC, MyInstId, 1);
    gtk_widget_queue_draw(widget);
    return TRUE;
}

/* ====================================================================
 * DATA FETCHING: MOTOR EN TIEMPO REAL (Solo Crudos, Sin DSP aqui)
 * ==================================================================== */
gboolean fetch_realtime_data(gpointer user_data) {

    char msg[MAX_TRACE_BYTES]; MSG_LOGO reclogo; long recsize; int res;
    TRACE_HEADER *WaveHead; 
    long WaveLongF[MAX_TRACE_BYTES / sizeof(short)]; 

    int64_t current_realtime = g_get_real_time();
    if (g_last_frame_realtime == 0) g_last_frame_realtime = current_realtime;
    double dt = (current_realtime - g_last_frame_realtime) / 1000000.0;
    g_last_frame_realtime = current_realtime;
    double sys_time = (double)current_realtime / 1000000.0;

    do {
        res = tport_getmsg( &WaveRegion, WaveLogo, 1, &reclogo, &recsize, msg, sizeof(msg) );
        if ( res == GET_OK || res == GET_MISS || res == GET_NOTRACK ) {
            WaveHead = (TRACE_HEADER *) msg;
            if ( WaveMsgMakeLocal( WaveHead ) < 0 ) continue; 
            
            double t_start = WaveHead->starttime;
            double t_end = WaveHead->endtime;
            double rate = WaveHead->samprate;
            if (rate <= 0) rate = 20.0; 
            
            if (fabs(sys_time - t_end) > 86400.0) {
                 if (t_end > g_latest_time) g_latest_time = t_end;
            }

            for ( int i = 0; i < iNumStas; i++ ) {
                if ( !strcmp(WaveHead->sta, StaArray[i].szStation) &&
                     !strcmp(WaveHead->chan, StaArray[i].szChannel) &&
                     !strcmp(WaveHead->net, StaArray[i].szNetID) ) {
                     
                    StaArray[i].dLastPacketSysTime = sys_time;
                    StaArray[i].dSampRate = rate; 
                    
                    int64_t abs_start = (int64_t)(t_start * rate);
                    int64_t abs_end = (int64_t)(t_end * rate);

                    /* Llenado estandar del gap con bandera de vacio (INT_MAX) */
                    if (StaArray[i].lLastAbsIdx > 0 && abs_start > StaArray[i].lLastAbsIdx) {
                        int64_t gap_samps = abs_start - StaArray[i].lLastAbsIdx;
                        if (gap_samps > StaArray[i].lRawCircSize) gap_samps = StaArray[i].lRawCircSize; 
                        for (int64_t g = 0; g < gap_samps; g++) {
                            int64_t clr_abs = StaArray[i].lLastAbsIdx + g;
                            int idx = CIRC_IDX(clr_abs, StaArray[i].lRawCircSize);
                            StaArray[i].plRawCircBuff[idx] = INT_MAX;
                        }
                    }

                    char szType[3]; strncpy(szType, WaveHead->datatype, 2); szType[2] = '\0';
                    short *WaveShort = (short *) (msg + sizeof(TRACE_HEADER));
                    int32_t *Wave32 = (int32_t *) (msg + sizeof(TRACE_HEADER));
                    
                    for (int s = 0; s < WaveHead->nsamp; s++) {
                        int32_t x;
                        if ( (strcmp(szType, "i2") == 0) || (strcmp(szType, "s2") == 0) ) x = (int32_t)WaveShort[s];
                        else x = Wave32[s];
                        
                        double t_samp = t_start + ((double)s / rate);
                        int64_t abs_idx = (int64_t)(t_samp * rate);
                        int idx = CIRC_IDX(abs_idx, StaArray[i].lRawCircSize);
                        
                        StaArray[i].plRawCircBuff[idx] = x;
                    }

                    if (abs_end > StaArray[i].lLastAbsIdx) StaArray[i].lLastAbsIdx = abs_end;
                    break; 
                }
            }
        }
    } while ( res != GET_NONE ); 

    if (!g_is_hold) {
        gboolean is_realtime = TRUE;
        if (g_latest_time > 0.0 && fabs(sys_time - g_latest_time) > 86400.0) is_realtime = FALSE;

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

    do {
        res = tport_getmsg( &PickRegion, PickLogo, 1, &reclogo, &recsize, msg, sizeof(msg) - 1 );
        if ( res == GET_OK || res == GET_MISS || res == GET_NOTRACK ) {
            msg[recsize] = '\0'; 
            unsigned char t1, t2;
            GetType("TYPE_TRACEBUF", &t1); GetType("TYPE_TRACEBUF2", &t2);
            if (reclogo.type == t1 || reclogo.type == t2) continue;
            
            if (reclogo.type == TypePickTWC) {
                char s[15][30];
                int n = sscanf(msg, "%29s %29s %29s %29s %29s %29s %29s %29s %29s %29s %29s %29s %29s", 
                               s[0], s[1], s[2], s[3], s[4], s[5], s[6], s[7], s[8], s[9], s[10], s[11], s[12]);
                if (n >= 10) {
                    char *sta = s[3]; 
                    double pTime = 0.0;
                    char phase[20] = "P"; 

                    for (int k = 6; k < n; k++) {
                        if (strchr(s[k], '.') != NULL && strlen(s[k]) > 8) {
                            pTime = atof(s[k]);
                            if (k + 2 < n && strchr(s[k+2], '.') == NULL) strncpy(phase, s[k+2], 7);
                            else if (k + 1 < n && strchr(s[k+1], '.') == NULL) strncpy(phase, s[k+1], 7);
                            phase[7] = '\0';
                            
                            for ( int i = 0; i < iNumStas; i++ ) {
                                if ( !strcmp(AtwcStaArray[i].szStation, sta) ) {
                                    int pIdx = StaArray[i].iNumPicks % MAX_PICKS_PER_STA;
                                    StaArray[i].picks[pIdx].dTime = pTime;
                                    strncpy(StaArray[i].picks[pIdx].szPhase, phase, 7);
                                    StaArray[i].iNumPicks++;
                                    break;
                                }
                            }
                            break;
                        }
                    }
                }
            }
        }
    } while ( res != GET_NONE );

    if (g_drawing_waves) gtk_widget_queue_draw(g_drawing_waves);
    if (drawing_axis) gtk_widget_queue_draw(drawing_axis);

    return G_SOURCE_CONTINUE; 
}

/* ====================================================================
 * RENDER Y PROCESAMIENTO ON-THE-FLY CON PROTECCION ANTI-SPIKES Y FUTURE
 * ==================================================================== */
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

    /* PADDING MAGICO: Extraemos 30s hacia atras para que el ringing del filtro muera antes de entrar a pantalla */
    double pad_secs = 30.0;
    double extract_start = t_left - pad_secs;

    for (int i = 0; i < iNumStas; i++) {
        double rate = StaArray[i].dSampRate > 0 ? StaArray[i].dSampRate : 20.0;
        
        int64_t abs_start = (int64_t)(extract_start * rate);
        int64_t abs_end = (int64_t)(t_right * rate);
        long num_samps = abs_end - abs_start;

        double y_top = i * dTrackHeight;
        double y_center = y_top + (dTrackHeight / 2.0);

        cairo_set_source_rgb(cr, g_color_font[0], g_color_font[1], g_color_font[2]);
        cairo_move_to(cr, 10, y_center + (font_size * 0.35));
        char label[64]; 
        snprintf(label, sizeof(label), "%-5s %s", StaArray[i].szStation, StaArray[i].szNetID); 
        cairo_show_text(cr, label);

        cairo_set_source_rgb(cr, g_color_sep[0], g_color_sep[1], g_color_sep[2]); /* Color dinamico del separador */
        cairo_move_to(cr, PANEL_WIDTH, y_top); cairo_line_to(cr, width, y_top); cairo_stroke(cr);

        if (num_samps <= 0 || num_samps >= StaArray[i].lRawCircSize) continue;

        /* 1. EXTRAMOS EL PEDAZO A UN ARREGLO LINEAL */
        long *trace_buf = (long*)malloc(num_samps * sizeof(long));
        if (!trace_buf) continue;

        for (long k = 0; k < num_samps; k++) {
            int64_t abs_k = abs_start + k;
            if (abs_k > StaArray[i].lLastAbsIdx || abs_k < 0) {
                trace_buf[k] = INT_MAX;
            } else {
                trace_buf[k] = StaArray[i].plRawCircBuff[CIRC_IDX(abs_k, StaArray[i].lRawCircSize)];
            }
        }

        /* 2. INTERPOLACION DE GAPS PARA EVITAR SPIKES DE FILTRADO */
        long last_valid = 0;
        long gap_start = -1;
        int found_first = 0;
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

        /* 3. ELIMINAR LINEA BASE (DC) */
        double sum = 0;
        for (long k = 0; k < num_samps; k++) sum += (double)trace_buf[k];
        long mean = (long)(sum / num_samps);
        for (long k = 0; k < num_samps; k++) trace_buf[k] -= mean;

        /* 4. APLICAR FILTROS USANDO LA LIBRERIA DSP */
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

        /* 5. CÁLCULO DE ESCALA Y DIBUJO */
        long draw_start_idx = (long)(pad_secs * rate);
        if (draw_start_idx > num_samps) draw_start_idx = 0;

        /* FIX: Identificamos hasta donde hay datos reales. Todo el padding que le agregamos 
           del "futuro" para estabilizar el filtro matemático, NO LO DIBUJAMOS NI ESCALAMOS. */
        long real_samps_end = (long)(StaArray[i].lLastAbsIdx - abs_start + 1);
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
        
        /* Escalado simétrico respecto a cero */
        double auto_scale = (dTrackHeight * 0.425) / max_abs; 
        auto_scale *= g_zoom_factor; 

        cairo_save(cr);
        cairo_rectangle(cr, PANEL_WIDTH, y_top, draw_area_width, dTrackHeight);
        cairo_clip(cr);

        cairo_set_source_rgb(cr, g_color_wave[0], g_color_wave[1], g_color_wave[2]); 
        cairo_set_line_width(cr, 1.0); 

        if (found_first) {
            for (int px = 0; px < (int)draw_area_width; px++) {
                double px_t_start = t_left + ((double)px / draw_area_width) * window_secs;
                double px_t_end = t_left + ((double)(px + 1) / draw_area_width) * window_secs;
                
                long p_local_start = (long)((px_t_start - extract_start) * rate);
                long p_local_end = (long)((px_t_end - extract_start) * rate);
                if (p_local_end == p_local_start) p_local_end++;
                
                if (p_local_start < 0) p_local_start = 0;
                if (p_local_end > num_samps) p_local_end = num_samps;

                double p_min = 1e12, p_max = -1e12;
                gboolean px_has_data = FALSE;
                
                for (long local_k = p_local_start; local_k < p_local_end; local_k++) {
                    /* FIX: Si entramos en territorio pardeado del futuro, no extraer min/max, 
                       cortando la línea limpiamente en vez de dejar la línea horizontal. */
                    if (local_k >= real_samps_end) continue; 
                    
                    double val = trace_buf[local_k] * auto_scale;
                    if (val < p_min) p_min = val;
                    if (val > p_max) p_max = val;
                    px_has_data = TRUE;
                }

                if (px_has_data) {
                    /* FIX: Engrosamiento anti-desaparición ampliado a 2.0 px para
                       ruido pre-sismo cuando auto_scale se comprime con eventos masivos. */
                    if (p_max - p_min < 2.0) {
                        double avg = (p_max + p_min) / 2.0;
                        p_max = avg + 1.0;
                        p_min = avg - 1.0;
                    }
                    
                    double x = PANEL_WIDTH + px;
                    cairo_move_to(cr, x, y_center - p_min); 
                    cairo_line_to(cr, x, y_center - p_max);
                }
            }
        }
        cairo_stroke(cr); cairo_restore(cr);
        free(trace_buf);

        cairo_set_source_rgb(cr, 1.0, 0.0, 0.0); cairo_set_line_width(cr, 2.0);

        int max_iter = (StaArray[i].iNumPicks < MAX_PICKS_PER_STA) ? StaArray[i].iNumPicks : MAX_PICKS_PER_STA;
        for (int p = 0; p < max_iter; p++) {
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
    }
    return FALSE;
}

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
        
        if (g_drawing_waves) gtk_widget_queue_draw(g_drawing_waves);
    }
    gtk_widget_destroy(dialog);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Uso: %s <configfile.d>\n", argv[0]);
        exit(1);
    }

    if (ReadConfig(argv[1]) != 0) {
        fprintf(stderr, "Error leyendo configuracion\n");
        exit(1);
    }

    setenv("TZ", "GMT", 1); tzset(); 
    logit_init(argv[1], 0, 1024, LogFile);

    gtk_init(&argc, &argv);
    setlocale(LC_NUMERIC, "C");

    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(provider,
        "#btn_filter { background-image: none; background-color: #28a745; color: #ffffff; font-weight: bold; padding: 2px 10px; border-radius: 4px; }\n"
        "#btn_filter:hover { background-color: #218838; }\n"
        "#btn_reload { background-image: none; background-color: #6c757d; color: #ffffff; font-weight: bold; padding: 2px 10px; border-radius: 4px; }\n"
        "#btn_reload:hover { background-color: #5a6268; }", -1, NULL);
    gtk_style_context_add_provider_for_screen(gdk_screen_get_default(), GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

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

    btn_hold = gtk_toggle_button_new_with_label("HOLD (Congelar Pantalla)"); g_signal_connect(btn_hold, "toggled", G_CALLBACK(on_btn_hold_toggled), NULL);
    gtk_box_pack_start(GTK_BOX(toolbar), btn_hold, FALSE, FALSE, 0);

    GtkWidget *info_label = gtk_label_new(" | (En HOLD) Clic Izq: Picar onda | Flechas Arriba/Abajo: Zoom Vertical"); gtk_box_pack_start(GTK_BOX(toolbar), info_label, FALSE, FALSE, 10);
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
    gtk_main();

    tport_detach( &WaveRegion ); tport_detach( &PickRegion ); FreeAllStations();
    return 0;
}
