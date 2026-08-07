#include <gtk/gtk.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <locale.h>
#include <sys/types.h>
#include <sys/stat.h>

#pragma warning(disable: 4005)
#include <earthworm.h>
#include <transport.h>
#include <kom.h>
#include "earlybirdlib.h"
#include "dataprocessing.h" /* Libreria externa de procesamiento */

#define MAX_ESTA 512
#define MAX_STR 256

/* --- MODULE CONFIGURATION --- */
char MyModName[MAX_STR] = "MOD_HYPO_DISPLAY";
char InRingName[MAX_STR] = "HYPO_RING";
char OutRingName[MAX_STR] = "PICK_RING";
char StaFile[MAX_STR] = "";
char StaDataFile[MAX_STR] = "";
char CalibsFile[MAX_STR] = "";
char QuakeFile[MAX_STR] = "";
char LocFilePath[MAX_STR] = "";
char WaveFilePath[MAX_STR] = "";
int  HeartBeatInt = 30;
int  LogFile = 1;
int  iFileLength = 10; 

pid_t MyPid;
time_t timeLastBeat = 0;
unsigned char TypeHeartBeat = 0;
unsigned char TypeError = 0;

/* --- Global State Variables --- */
STATION *StaArray = NULL; 
PPICK   PBufG[MAX_ESTA]; 
int     bHasData[MAX_ESTA]; 
double  g_StaDist[MAX_ESTA]; 
int     NumEstaciones = 0;
int     selected_qid = 0;
double  selected_otime = 0;
double  selected_lat = 0;
double  selected_lon = 0;
double  selected_depth = 0;
char    selected_id[32] = "None";

/* TRACKER & DEBOUNCE */
gboolean pick_modificado[MAX_ESTA] = {FALSE};
gboolean pending_waveform_reload = FALSE;

/* --- TIMEOUT & ALERT CONTROL --- */
gboolean waiting_for_ew = FALSE;
guint    ew_timeout_id = 0;
int      expected_qid = 0;
int      expected_min_qver = 0;

GtkWidget *canvas_global = NULL;
GtkWidget *tree_global = NULL;
GtkWidget *btn_repick = NULL;
GtkWidget *btn_force = NULL; 
GtkWidget *lbl_event_info = NULL; /* Nueva Etiqueta de ID y O-Time */
GtkWidget *btn_refresh = NULL;    /* Nuevo boton de Refresh Waveforms */

/* --- COMPONENTES DINAMICOS DEL FILTRO --- */
GtkWidget *combo_filter = NULL; 
GtkWidget *entry_freq1 = NULL;
GtkWidget *entry_freq2 = NULL;
GtkWidget *combo_order = NULL;
GtkWidget *btn_apply_filter = NULL;

/* --- DYNAMIC REPICK TOOLS --- */
GtkWidget *box_fetch = NULL;
GtkWidget *entry_dist = NULL;
GtkWidget *entry_time = NULL; 
double g_max_dist_km = 500.0; 

gboolean edit_mode = FALSE;
double   g_zoom_factor = 1.0; 
int      g_filter_type = 0; /* 0 = Raw, 1 = HP, 2 = LP, 3 = BP */

double g_dWindowStart = 0;
double g_dScreenTime = 120.0; 
int    g_margin_left = 160; 
int    g_spacing = 100;

typedef struct { 
    int idx; 
    double dist; 
    int y_top;    
    int y_bottom; 
} StaNode;

StaNode g_SortedNodes[MAX_ESTA];
int     g_NumSortedNodes = 0;

SHM_INFO InRegion;       
SHM_INFO PRegion;
unsigned char TypePickTWC = 0;

unsigned char MyModId = 0;
unsigned char MyInstId = 0;

/* --- HISTORY FILE MONITORING AND GLOBALS --- */
static time_t g_last_file_mtime = 0;
double g_top_otime = 0.0;
gboolean g_block_waveform_fetch = FALSE;

/* Forward declaration for GTK callback used in RepopulateTable */
static void on_row_selected(GtkTreeSelection *selection, gpointer data);

void ConnectToEarthworm() {
    long InRingKey = GetKey(InRingName); long OutRingKey = GetKey(OutRingName);
    if (InRingKey == -1) { printf("Error: Invalid input ring.\n"); exit(-1); }
    if (OutRingKey == -1) { printf("Error: Invalid output ring.\n"); exit(-1); }
    if (GetType("TYPE_PICKTWC", &TypePickTWC) != 0) TypePickTWC = 0; 
    if (GetLocalInst(&MyInstId) != 0) MyInstId = 0;
    if (GetModId(MyModName, &MyModId) != 0) { if (GetModId("MOD_WILDCARD", &MyModId) != 0) MyModId = 0; }
    
    tport_attach(&InRegion, InRingKey); tport_attach(&PRegion, OutRingKey);
}

int ReadConfig(char *configfile) {
    int ncommand = 10, nmiss = 0, i;
    char init[15] = {0};
    char *com, *str;

    if (!k_open(configfile)) {
        fprintf(stderr, "new_hypo_display: Error opening config file <%s>\n", configfile);
        return -1;
    }

    while (k_rd()) {
        com = k_str();
        if (!com || com[0] == '#') continue;

        if (k_its("MyModuleId")) { str = k_str(); if (str) strcpy(MyModName, str); init[0] = 1; } 
        else if (k_its("InRing")) { str = k_str(); if (str) strcpy(InRingName, str); init[1] = 1; } 
        else if (k_its("OutRing")) { str = k_str(); if (str) strcpy(OutRingName, str); init[2] = 1; } 
        else if (k_its("HeartBeatInt")) { HeartBeatInt = k_int(); init[3] = 1; } 
        else if (k_its("LogFile")) { LogFile = k_int(); init[4] = 1; } 
        else if (k_its("StaFile")) { str = k_str(); if (str) strcpy(StaFile, str); init[5] = 1; } 
        else if (k_its("StaDataFile")) { str = k_str(); if (str) strcpy(StaDataFile, str); init[6] = 1; } 
        else if (k_its("CalibsFile")) { str = k_str(); if (str) strcpy(CalibsFile, str); init[7] = 1; } 
        else if (k_its("QuakeFile")) { str = k_str(); if (str) strcpy(QuakeFile, str); init[8] = 1; } 
        else if (k_its("LocFilePath")) { str = k_str(); if (str) strcpy(LocFilePath, str); init[9] = 1; } 
        else if (k_its("WaveFilePath")) { str = k_str(); if (str) strcpy(WaveFilePath, str); } 
        else if (k_its("FileLength")) { iFileLength = k_int(); } 
        else { continue; }
        
        if (k_err()) {
            fprintf(stderr, "new_hypo_display: Error parsing <%s> in <%s>\n", com, configfile); return -1;
        }
    }
    for (i = 0; i < ncommand; i++) if (!init[i]) nmiss++;
    k_close();
    if (nmiss > 0) {
        fprintf(stderr, "new_hypo_display: ERROR, missing parameters in <%s>\n", configfile); return -1;
    }
    return 0;
}

void Status(unsigned char type, short ierr, char *note) {
    MSG_LOGO logo; char msg[256]; time_t t;
    logo.instid = MyInstId; logo.mod = MyModId; logo.type = type;
    time(&t);
    if (type == TypeHeartBeat) sprintf(msg, "%ld %d\n", (long) t, MyPid);
    else if (type == TypeError) {
        sprintf(msg, "%ld %hd %s\n", (long) t, ierr, note); logit("et", "new_hypo_display: Error: %s\n", note);
    }
    tport_putmsg(&InRegion, &logo, strlen(msg), msg);
}

void actualizar_altura_canvas() {
    if (!canvas_global) return;
    int count = 0;
    for (int i = 0; i < NumEstaciones; i++) {
        if (bHasData[i] == 1 && (g_StaDist[i] * 6371.0) <= g_max_dist_km) count++;
    }
    /* Ajuste de margen superior (+ 20) al eliminar el texto que dibujaba cairo */
    if (count > 0) gtk_widget_set_size_request(canvas_global, -1, (count * g_spacing) + 20);
    else gtk_widget_set_size_request(canvas_global, -1, 600);
}

/* Modificado para soportar filtros dinamicos IIR y validaciones de Nyquist */
void ApplySelectedFilter() {
    double f1 = 0.7, f2 = 2.0; 
    int order = 4;
    
    if (entry_freq1) f1 = atof(gtk_entry_get_text(GTK_ENTRY(entry_freq1)));
    if (entry_freq2) f2 = atof(gtk_entry_get_text(GTK_ENTRY(entry_freq2)));
    if (combo_order) {
        int active = gtk_combo_box_get_active(GTK_COMBO_BOX(combo_order));
        order = (active == 0) ? 2 : 4;
    }

    for(int i = 0; i < NumEstaciones; i++) {
        if (StaArray[i].lRawCircCtr > 0) {
            
            if (StaArray[i].lRawCircCtr > StaArray[i].lRawCircSize) {
                StaArray[i].lRawCircCtr = StaArray[i].lRawCircSize;
            }
            
            double nyquist = StaArray[i].dSampRate / 2.0;
            double safe_f1 = f1;
            double safe_f2 = f2;
            
            /* --- Validaciones Matematicas de Nyquist --- */
            if (g_filter_type != 0) {
                if (safe_f1 >= nyquist) {
                    safe_f1 = nyquist * 0.95; 
                    logit("t", "Aviso: Freq1 (%.2f Hz) excede Nyquist (%.2f Hz) para %s. Reducido a %.2f Hz\n", f1, nyquist, StaArray[i].szStation, safe_f1);
                }
                if (g_filter_type == 3) {
                    if (safe_f2 >= nyquist) {
                        safe_f2 = nyquist * 0.95;
                        logit("t", "Aviso: Freq2 (%.2f Hz) excede Nyquist para %s. Reducido a %.2f Hz\n", f2, StaArray[i].szStation, safe_f2);
                    }
                    if (safe_f1 >= safe_f2) {
                        safe_f1 = safe_f2 * 0.5; 
                        logit("t", "Aviso: Freq1 es mayor que Freq2 para %s. Ajustado F1 a %.2f Hz\n", StaArray[i].szStation, safe_f1);
                    }
                }
            }
            
            DemeanTrace(&StaArray[i]); 
            
            if (g_filter_type == 1) { 
                aplicar_filtro_iir(StaArray[i].plFiltCircBuff, StaArray[i].lRawCircCtr, StaArray[i].dSampRate, 1, safe_f1, order);
            } else if (g_filter_type == 2) { 
                aplicar_filtro_iir(StaArray[i].plFiltCircBuff, StaArray[i].lRawCircCtr, StaArray[i].dSampRate, 2, safe_f1, order);
            } else if (g_filter_type == 3) { 
                aplicar_filtro_iir(StaArray[i].plFiltCircBuff, StaArray[i].lRawCircCtr, StaArray[i].dSampRate, 1, safe_f1, order);
                aplicar_filtro_iir(StaArray[i].plFiltCircBuff, StaArray[i].lRawCircCtr, StaArray[i].dSampRate, 2, safe_f2, order);
            }
        }
    }
}

void ReloadWaveforms() {
    if (selected_qid == 0) return;
    
    logit("t", ">> TRACER: Preparing buffers for Quake ID: %d...\n", selected_qid);
    
    HYPO trkHypo; memset(&trkHypo, 0, sizeof(HYPO));
    trkHypo.dOriginTime = selected_otime; trkHypo.dLat = selected_lat; trkHypo.dLon = selected_lon; trkHypo.dDepth = selected_depth;
    GeoCent( (LATLON *) &trkHypo ); GetLatLonTrig( (LATLON *) &trkHypo );

    InitPBufWithStaLocal( NumEstaciones, PBufG, StaArray );
    
    for(int i=0; i<NumEstaciones; i++) {
        if (StaArray[i].plRawCircBuff) {
            for (long k = 0; k < StaArray[i].lRawCircSize; k++) {
                StaArray[i].plRawCircBuff[k] = INT_MAX;
            }
        }
        StaArray[i].lRawCircCtr = 0; bHasData[i] = 0; PBufG[i].dPTime = 0.0; 
        double dLat1 = trkHypo.dLat, dLon1 = trkHypo.dLon, dLat2 = PBufG[i].dLat, dLon2 = PBufG[i].dLon;
        double cos_angle = sin(dLat1)*sin(dLat2) + cos(dLat1)*cos(dLat2)*cos(dLon1-dLon2);
        if (cos_angle > 1.0) cos_angle = 1.0; if (cos_angle < -1.0) cos_angle = -1.0;
        g_StaDist[i] = acos(cos_angle);
    }

    GetPTimes( NumEstaciones, PBufG, &trkHypo );

    PPICK PBufL[MAX_ESTA];
    int numPAuto = 0; char szPFile[256];
    snprintf(szPFile, sizeof(szPFile), "%s/%04d.dat", LocFilePath, selected_qid);
    
    ReadPTimeFile(&numPAuto, PBufL, szPFile, MAX_ESTA);
    if(numPAuto > 0) UpdatePPickArrayLocal(numPAuto, NumEstaciones, PBufL, PBufG);

    int view_mins = (int)(g_dScreenTime / 60.0) + 1;
    int files_needed = (view_mins / iFileLength) + 2; 
    int safe_req_mins = files_needed * iFileLength;

    logit("t", ">> TRACER: Executing ReadDiskDataForHypo. FileLength=%d, ReqMins=%d\n", iFileLength, safe_req_mins);
    ReadDiskDataForHypo( iFileLength, safe_req_mins, WaveFilePath, ".S", 60.0, NumEstaciones, PBufG, StaArray );
    logit("t", ">> TRACER: ReadDiskDataForHypo finished.\n");
    
    if (canvas_global) {
        for(int i=0; i<NumEstaciones; i++) {
            FindDataEndHypoLocal(&StaArray[i]); 
            FillInternalGaps(&StaArray[i]);     
            if (StaArray[i].lRawCircCtr > 0) { bHasData[i] = 1; }
        }
        
        logit("t", ">> TRACER: Applying filters to the extracted data...\n");
        ApplySelectedFilter();
        
        if (entry_dist) g_max_dist_km = atof(gtk_entry_get_text(GTK_ENTRY(entry_dist)));
        if (entry_time) g_dScreenTime = atof(gtk_entry_get_text(GTK_ENTRY(entry_time))) * 60.0;
        
        actualizar_altura_canvas();
        gtk_widget_queue_draw(canvas_global);
        logit("t", ">> TRACER: UI redraw requested successfully.\n");
    }
}

gboolean waveform_reload_timer(gpointer data) {
    if (pending_waveform_reload && !edit_mode) {
        pending_waveform_reload = FALSE;
        ReloadWaveforms();
    }
    return TRUE;
}

/* Evento para forzar la actualizacion manual de datos al clickear Refresh Waveforms */
static void on_btn_refresh_clicked(GtkWidget *widget, gpointer data) {
    if (selected_qid == 0) return;
    ReloadWaveforms();
}

static void on_filter_changed(GtkComboBox *widget, gpointer data) {
    g_filter_type = gtk_combo_box_get_active(widget);
    
    gboolean is_hp_lp = (g_filter_type == 1 || g_filter_type == 2);
    gboolean is_bp = (g_filter_type == 3);
    
    if (entry_freq1) gtk_widget_set_sensitive(entry_freq1, is_hp_lp || is_bp);
    if (entry_freq2) gtk_widget_set_sensitive(entry_freq2, is_bp);
    if (combo_order) gtk_widget_set_sensitive(combo_order, is_hp_lp || is_bp);
    if (btn_apply_filter) gtk_widget_set_sensitive(btn_apply_filter, g_filter_type != 0);

    ApplySelectedFilter();
    if (canvas_global) gtk_widget_queue_draw(canvas_global);
}

static void on_btn_apply_filter_clicked(GtkWidget *widget, gpointer data) {
    ApplySelectedFilter();
    if (canvas_global) gtk_widget_queue_draw(canvas_global);
}

static void on_btn_fetch_clicked(GtkWidget *widget, gpointer data) {
    if (!entry_dist || !entry_time) return;
    double new_dist = atof(gtk_entry_get_text(GTK_ENTRY(entry_dist)));
    double new_time_min = atof(gtk_entry_get_text(GTK_ENTRY(entry_time)));
    gboolean changed = FALSE;
    if (new_dist > 0.0) { g_max_dist_km = new_dist; changed = TRUE; }
    if (new_time_min > 0.0) {
        if (new_time_min > 10.0) { new_time_min = 10.0; gtk_entry_set_text(GTK_ENTRY(entry_time), "10"); }
        g_dScreenTime = new_time_min * 60.0; 
        changed = TRUE;
    }
    if (changed) {
        actualizar_altura_canvas();
        pending_waveform_reload = TRUE; 
    }
}

static void color_rows_func(GtkTreeViewColumn *col, GtkCellRenderer *rend, GtkTreeModel *model, GtkTreeIter *iter, gpointer data) {
    GtkTreePath *path = gtk_tree_model_get_path(model, iter);
    if (path) {
        gint *indices = gtk_tree_path_get_indices(path);
        GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree_global));
        if (gtk_tree_selection_path_is_selected(selection, path)) {
            g_object_set(rend, "cell-background", "#007bff", "foreground", "#ffffff", "weight", 700, NULL);
        } else {
            if (indices && indices[0] % 2 == 0) g_object_set(rend, "cell-background", "#ffcece", "foreground", "#000000", "weight", 400, NULL);
            else g_object_set(rend, "cell-background", "#ffffff", "foreground", "#000000", "weight", 400, NULL);
        }
        gtk_tree_path_free(path);
    }
}

static gboolean on_key_press(GtkWidget *widget, GdkEventKey *event, gpointer data) {
    if (!edit_mode) return FALSE; 
    if (event->keyval == GDK_KEY_Up) { g_zoom_factor *= 1.5; if (canvas_global) gtk_widget_queue_draw(canvas_global); return TRUE; } 
    else if (event->keyval == GDK_KEY_Down) { g_zoom_factor /= 1.5; if (canvas_global) gtk_widget_queue_draw(canvas_global); return TRUE; }
    return FALSE;
}

void mostrar_alerta(const char *mensaje, GtkMessageType tipo) {
    GtkWidget *dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_DESTROY_WITH_PARENT, tipo, GTK_BUTTONS_OK, "%s", mensaje);
    gtk_window_set_title(GTK_WINDOW(dialog), "Earthworm Response");
    g_signal_connect(dialog, "response", G_CALLBACK(gtk_widget_destroy), NULL);
    gtk_widget_show_all(dialog);
}

static gboolean on_ew_timeout(gpointer data) {
    if (waiting_for_ew) {
        waiting_for_ew = FALSE;
        ew_timeout_id = 0;
        mostrar_alerta("No new solution was generated.\n\nThe algorithm might not have converged with the provided picks.", GTK_MESSAGE_WARNING);
        if (btn_force) gtk_button_set_label(GTK_BUTTON(btn_force), "Force Relocation");
    }
    return FALSE;
}

void UpdatePPickArrayLocal( int iNumPIn, int iNumSta, PPICK PIn[], PPICK POut[] ) {
   for ( int i=0; i<iNumPIn; i++ ) {
      char staIn[32] = {0}, netIn[32] = {0};
      if (sscanf(PIn[i].szStation, "%s", staIn) != 1) continue;
      sscanf(PIn[i].szNetID, "%s", netIn);
      for ( int j=0; j<iNumSta; j++ ) {
         char staOut[32] = {0}, netOut[32] = {0};
         if (sscanf(POut[j].szStation, "%s", staOut) != 1) continue;
         sscanf(POut[j].szNetID, "%s", netOut);
         if ( strcmp(staIn, staOut) == 0 && strcmp(netIn, netOut) == 0 ) {
            if (PIn[i].dPTime > 1.0) {
                double exp_time = POut[j].dExpectedPTime, lat = POut[j].dLat, lon = POut[j].dLon;
                char s[16], c[16], n[16];
                strcpy(s, POut[j].szStation); strcpy(c, POut[j].szChannel); strcpy(n, POut[j].szNetID);
                POut[j] = PIn[i];
                POut[j].dExpectedPTime = exp_time; POut[j].dLat = lat; POut[j].dLon = lon;
                strcpy(POut[j].szStation, s); strcpy(POut[j].szChannel, c); strcpy(POut[j].szNetID, n);
            }
            break;
         }
      }
   }
}

void InitPBufWithStaLocal( int iNumSta, PPICK P[], STATION Sta[] ) {
   for ( int i=0; i<iNumSta; i++ ) {
      InitP( &P[i] );
      P[i].dLat = Sta[i].dLat; P[i].dLon = Sta[i].dLon;
      GeoCent( (LATLON *) &P[i] ); GetLatLonTrig( (LATLON *) &P[i] );      
      strcpy( P[i].szStation, Sta[i].szStation ); strcpy( P[i].szChannel, Sta[i].szChannel ); strcpy( P[i].szNetID, Sta[i].szNetID );
      P[i].iUseMe = 1; 
   }
}

void cargar_estaciones_dinamicas() {
    StaArray = (STATION *) calloc(MAX_ESTA, sizeof(STATION));
    if (StaArray == NULL) {
        printf(">> FATAL: Could not allocate memory for StaArray.\n");
        exit(-1);
    }
    int rtn = ReadStationList( &StaArray, &NumEstaciones, StaFile, StaDataFile, CalibsFile, MAX_ESTA, 0 );
    
    if (rtn == -1 || NumEstaciones == 0) {
        printf(">> FATAL: ReadStationList failed or found 0 valid stations.\n");
        exit(-1);
    }
    
    for(int i=0; i<NumEstaciones; i++) {
        if (StaArray[i].dSampRate <= 0.1) StaArray[i].dSampRate = 40.0;
        
        double safe_sps = (StaArray[i].dSampRate > 100.0) ? StaArray[i].dSampRate : 100.0;
        StaArray[i].lRawCircSize = (long)(safe_sps * 20.0 * 60.0); 
        
        StaArray[i].plRawCircBuff = (long *)calloc(1, sizeof(long) * StaArray[i].lRawCircSize);
        StaArray[i].plFiltCircBuff = (long *)calloc(1, sizeof(long) * StaArray[i].lRawCircSize);
        
        if (!StaArray[i].plRawCircBuff || !StaArray[i].plFiltCircBuff) {
             printf(">> FATAL: Out of memory allocating trace buffers.\n");
             exit(-1);
        }
    }
    logit("t", ">> TRACER: Station dynamic memory shielded correctly (Max. %d stations)\n", NumEstaciones);
}

/* --- TABLE POPULATION AND SELECTION LOGIC --- */
void procesar_mensaje_sismo(GtkWidget *tree, const char *payload) {
    GtkListStore *store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(tree)));
    
    double otime, lat, lon, depth, res, azm, pref_mag, mb=0, ml=0, ms=0, mwp=0, mw=0, rec_time;
    int nps, qid, qver, npref=0, nmb=0, nml=0, nms=0, nmwp=0, nmw=0, status;
    char mag_type[16] = "";

    int parsed = sscanf(payload, "%lf %lf %lf %lf %d %15s %lf %d %d %d %lf %lf %lf %d %lf %d %lf %d %lf %d %lf %d %lf %d",
           &otime, &lat, &lon, &pref_mag, &npref, mag_type, &depth, &qid, &qver, &nps, &res, &azm, 
           &mb, &nmb, &ml, &nml, &ms, &nms, &mwp, &nmwp, &mw, &nmw, &rec_time, &status);
           
    if (parsed < 22) return; 

    char fecha[32], hora[32], szLat[32], szLon[32], szDep[32], szRes[32], szAzm[32], szStn[32], szID[32];
    char szMs[32]="--", szMw[32]="--", szMwp[32]="--", szMb[32]="--", szMl[32]="--";

    time_t rawtime = (time_t)(otime + 0.5);
    struct tm *ptm = gmtime(&rawtime); 
    if (ptm) { 
        snprintf(fecha, sizeof(fecha), "%02d/%02d", ptm->tm_mon + 1, ptm->tm_mday); 
        snprintf(hora, sizeof(hora), "%02d:%02d:%02d", ptm->tm_hour, ptm->tm_min, ptm->tm_sec); 
    } else { 
        strcpy(fecha, "--/--"); strcpy(hora, "--:--:--"); 
    }

    snprintf(szLat, sizeof(szLat), "%.2f%c", fabs(lat), lat < 0 ? 'S' : 'N'); 
    snprintf(szLon, sizeof(szLon), "%.2f%c", fabs(lon), lon < 0 ? 'W' : 'E');
    snprintf(szDep, sizeof(szDep), "%.0f", depth); 
    snprintf(szRes, sizeof(szRes), "%.1f", res); 
    snprintf(szAzm, sizeof(szAzm), "%.0f", azm);
    snprintf(szStn, sizeof(szStn), "%d", nps); 
    snprintf(szID, sizeof(szID), "%04d-%02d", qid, qver);
    
    if (ms > 0) snprintf(szMs, sizeof(szMs), "%.1f-%02d", ms, nms); 
    if (mw > 0) snprintf(szMw, sizeof(szMw), "%.1f-%02d", mw, nmw);
    if (mwp > 0) snprintf(szMwp, sizeof(szMwp), "%.1f-%02d", mwp, nmwp);
    if (mb > 0) snprintf(szMb, sizeof(szMb), "%.1f-%02d", mb, nmb);
    if (ml > 0) snprintf(szMl, sizeof(szMl), "%.1f-%02d", ml, nml);

    GtkTreeIter match_iter;
    gtk_list_store_append(store, &match_iter);
    gtk_list_store_set(store, &match_iter, 0, fecha, 1, hora, 2, szLat, 3, szLon, 4, szDep, 5, szRes, 6, szAzm, 7, szStn, 8, szID, 9, szMs, 10, szMw, 11, szMwp, 12, szMb, 13, szMl, 14, otime, 15, qver, 16, qid, 17, lat, 18, lon, 19, depth, -1);
}

void RestoreSelectionByOTime(GtkWidget *tree, double target_otime) {
    GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(tree));
    GtkTreeIter iter;
    if (gtk_tree_model_get_iter_first(model, &iter)) {
        do {
            double otime;
            gtk_tree_model_get(model, &iter, 14, &otime, -1);
            if (fabs(otime - target_otime) < 0.1) {
                GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree));
                
                g_block_waveform_fetch = TRUE; 
                gtk_tree_selection_select_iter(selection, &iter);
                g_block_waveform_fetch = FALSE;
                
                return;
            }
        } while (gtk_tree_model_iter_next(model, &iter));
    }
}

void SelectFirstRow(GtkWidget *tree) {
    GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(tree));
    GtkTreeIter iter;
    if (gtk_tree_model_get_iter_first(model, &iter)) {
        GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree));
        
        g_block_waveform_fetch = FALSE; 
        gtk_tree_selection_select_iter(selection, &iter);
        
        GtkTreePath *path = gtk_tree_model_get_path(model, &iter);
        gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(tree), path, NULL, FALSE, 0.0, 0.0);
        gtk_tree_path_free(path);
    }
}

void RepopulateTable(GtkWidget *tree) {
    GtkListStore *store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(tree)));
    GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree));
    
    g_signal_handlers_block_by_func(selection, G_CALLBACK(on_row_selected), NULL);
    gtk_list_store_clear(store);
    g_signal_handlers_unblock_by_func(selection, G_CALLBACK(on_row_selected), NULL);
    
    FILE *fp = fopen(QuakeFile, "r");
    if (!fp) return;
    
    char line[512]; 
    int count = 0;
    
    while (fgets(line, sizeof(line), fp)) { 
        if (strlen(line) < 20) continue;
        procesar_mensaje_sismo(tree, line);
        count++;
        if (count >= 500) break;
    }
    fclose(fp);
}

void cargar_sismos_iniciales(GtkWidget *tree) {
    RepopulateTable(tree);
    
    GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(tree));
    GtkTreeIter iter;
    if (gtk_tree_model_get_iter_first(model, &iter)) {
        gtk_tree_model_get(model, &iter, 14, &g_top_otime, -1);
    }
    
    SelectFirstRow(tree); 
    
    struct stat file_stat;
    if (stat(QuakeFile, &file_stat) == 0) {
        g_last_file_mtime = file_stat.st_mtime;
    }
}

static gboolean check_history_file_loop(gpointer user_data) {
    GtkWidget *tree = GTK_WIDGET(user_data);
    struct stat file_stat;
    
    if (stat(QuakeFile, &file_stat) == 0) {
        if (file_stat.st_mtime != g_last_file_mtime) {
            g_last_file_mtime = file_stat.st_mtime;
            
            double old_top_otime = g_top_otime;
            double prev_selected_otime = selected_otime;
            
            RepopulateTable(tree);
            
            GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(tree));
            GtkTreeIter iter;
            if (gtk_tree_model_get_iter_first(model, &iter)) {
                gtk_tree_model_get(model, &iter, 14, &g_top_otime, -1);
            }
            
            if (fabs(g_top_otime - old_top_otime) > 0.1 && g_top_otime != 0.0) {
                SelectFirstRow(tree);
            } else {
                RestoreSelectionByOTime(tree, prev_selected_otime);
            }
        }
    }
    return TRUE; 
}

static void on_row_selected(GtkTreeSelection *selection, gpointer data) {
    if (edit_mode) return;
    GtkTreeIter iter; GtkTreeModel *model;
    if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
        gchar *date_str, *time_str, *id_str; double otime, lat, lon, depth; int qid;
        gtk_tree_model_get(model, &iter, 0, &date_str, 1, &time_str, 8, &id_str, 14, &otime, 16, &qid, 17, &lat, 18, &lon, 19, &depth, -1);
        if (id_str) { strcpy(selected_id, id_str); }
        
        if (lbl_event_info) {
            char buf[256];
            snprintf(buf, sizeof(buf), "<span foreground='#c82333' weight='bold'>Quake ID: %s    O-Time: %s %s</span>", 
                     selected_id, date_str ? date_str : "--/--", time_str ? time_str : "--:--:--");
            gtk_label_set_markup(GTK_LABEL(lbl_event_info), buf);
        }
        
        if (id_str) g_free(id_str);
        if (date_str) g_free(date_str);
        if (time_str) g_free(time_str);

        selected_otime = otime; selected_qid = qid; selected_lat = lat; selected_lon = lon; selected_depth = depth;
        g_zoom_factor = 1.0; 

        gtk_widget_set_sensitive(btn_repick, TRUE);
        gtk_widget_set_sensitive(btn_force, TRUE);
        for(int i=0; i<MAX_ESTA; i++) pick_modificado[i] = FALSE;

        if (g_block_waveform_fetch) return;

        pending_waveform_reload = TRUE;
    } else {
        gtk_widget_set_sensitive(btn_repick, FALSE);
        gtk_widget_set_sensitive(btn_force, FALSE);
    }
}

static void on_btn_repick_clicked(GtkWidget *widget, gpointer data) {
    if (selected_qid == 0) return;
    if (!edit_mode) {
        edit_mode = TRUE; g_zoom_factor = 1.0; 
        gtk_widget_set_sensitive(tree_global, FALSE); 
        gtk_widget_hide(btn_force); 
        gtk_button_set_label(GTK_BUTTON(btn_repick), "Finish"); gtk_widget_set_name(btn_repick, "btn_relocate");
        gtk_widget_queue_draw(canvas_global);
    } else {
        edit_mode = FALSE; g_zoom_factor = 1.0; 
        for(int i=0; i<MAX_ESTA; i++) pick_modificado[i] = FALSE;
        gtk_widget_set_sensitive(tree_global, TRUE); 
        gtk_widget_show(btn_force); 
        gtk_button_set_label(GTK_BUTTON(btn_repick), "Repick mode"); gtk_widget_set_name(btn_repick, "btn_repick");
        
        pending_waveform_reload = TRUE;
    }
}

static void on_btn_force_clicked(GtkWidget *widget, gpointer data) {
    if (selected_qid == 0) return;
    time_t lTime; time(&lTime);
    if ((lTime - (long)(selected_otime + 0.5)) > (60 * 60)) {
        mostrar_alerta("Cannot force relocation.\nThis earthquake occurred over 1 hour ago and is considered closed.", GTK_MESSAGE_WARNING);
        return;
    }
    PPICK PForce; memset(&PForce, 0, sizeof(PPICK));
    strcpy(PForce.szStation, "LOC"); strcpy(PForce.szChannel, "ATE"); strcpy(PForce.szNetID,   "XX"); strcpy(PForce.szPhase,   "eX");
    PForce.cFirstMotion = '?'; PForce.iHypoID = selected_qid; PForce.iUseMe = 1; 

    ReportPick(&PForce, &StaArray[0], MyModId, PRegion, TypePickTWC, MyInstId, 3);

    expected_qid = selected_qid; expected_min_qver = -1;
    GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(tree_global)); GtkTreeIter iter;
    gboolean valid = gtk_tree_model_get_iter_first(model, &iter);
    while (valid) {
        int row_qid, row_qver; gtk_tree_model_get(model, &iter, 15, &row_qver, 16, &row_qid, -1);
        if (row_qid == expected_qid) { expected_min_qver = row_qver; break; }
        valid = gtk_tree_model_iter_next(model, &iter);
    }
    waiting_for_ew = TRUE;
    gtk_button_set_label(GTK_BUTTON(btn_force), "Processing...");
    if (ew_timeout_id != 0) g_source_remove(ew_timeout_id);
    ew_timeout_id = g_timeout_add_seconds(15, on_ew_timeout, NULL);
}

static gboolean on_canvas_clicked(GtkWidget *widget, GdkEventButton *event, gpointer data) {
    if (!edit_mode) return TRUE;
    int target_idx = -1;
    for(int n=0; n < g_NumSortedNodes; n++) {
        if (event->y >= g_SortedNodes[n].y_top && event->y <= g_SortedNodes[n].y_bottom) { target_idx = g_SortedNodes[n].idx; break; }
    }
    if (target_idx != -1) {
        gboolean pick_valido = FALSE;
        if (event->type == GDK_BUTTON_PRESS && event->button == 1) { 
            int draw_width = gtk_widget_get_allocated_width(widget) - g_margin_left - 10;
            if (event->x >= g_margin_left) {
                double relative_x = (event->x - g_margin_left) / (double)draw_width;
                double clicked_utc = g_dWindowStart + (relative_x * g_dScreenTime);
                PBufG[target_idx].dPTime = clicked_utc;
                strcpy(PBufG[target_idx].szPhase, "eP"); 
                PBufG[target_idx].iUseMe = 2; pick_valido = TRUE;
            }
        } else if (event->type == GDK_BUTTON_PRESS && event->button == 3) {
            if (PBufG[target_idx].dPTime > 1.0 || PBufG[target_idx].iUseMe > 0) {
                PBufG[target_idx].dPTime = 0.0; PBufG[target_idx].iUseMe = 0;   
                strcpy(PBufG[target_idx].szPhase, ""); pick_valido = TRUE;
            }
        }
        if (pick_valido) {
            gtk_widget_queue_draw(widget);
            char szPFile[256]; snprintf(szPFile, sizeof(szPFile), "%s/%04d.dat", LocFilePath, selected_qid);
            PPICK PBufTemp[MAX_ESTA]; int num_to_write = 0;
            for(int i=0; i<NumEstaciones; i++) {
                if (PBufG[i].dPTime > 1.0 && PBufG[i].iUseMe > 0) { PBufTemp[num_to_write] = PBufG[i]; num_to_write++; }
            }
            WritePTimeFile(num_to_write, PBufTemp, szPFile);
            for (int k=0; k<NumEstaciones; k++) {
                if (!strcmp(PBufG[target_idx].szStation, StaArray[k].szStation) && !strcmp(PBufG[target_idx].szNetID, StaArray[k].szNetID)) {
                    PBufG[target_idx].iHypoID = selected_qid;
                    ReportPick(&PBufG[target_idx], &StaArray[k], MyModId, PRegion, TypePickTWC, MyInstId, 2);
                    g_usleep(100000); break;
                }
            }
        }
    }
    return TRUE;
}

static gboolean on_draw_signal(GtkWidget *widget, cairo_t *cr, gpointer data) {
    int width = gtk_widget_get_allocated_width(widget);
    cairo_set_source_rgb(cr, 1, 1, 1); cairo_paint(cr);

    int margin_right = 10; int draw_width = width - g_margin_left - margin_right;

    g_NumSortedNodes = 0;
    for (int i = 0; i < NumEstaciones; i++) {
        if (bHasData[i] == 1 && (g_StaDist[i] * 6371.0) <= g_max_dist_km) { 
            g_SortedNodes[g_NumSortedNodes].idx = i; g_SortedNodes[g_NumSortedNodes].dist = g_StaDist[i]; g_NumSortedNodes++; 
        }
    }

    for (int i = 0; i < g_NumSortedNodes - 1; i++) {
        for (int j = 0; j < g_NumSortedNodes - i - 1; j++) {
            if (g_SortedNodes[j].dist > g_SortedNodes[j+1].dist) { StaNode temp = g_SortedNodes[j]; g_SortedNodes[j] = g_SortedNodes[j+1]; g_SortedNodes[j+1] = temp; }
        }
    }

    double earliest_pick = 1e15;
    for (int n = 0; n < g_NumSortedNodes; n++) {
        int i = g_SortedNodes[n].idx;
        double pt = (PBufG[i].dPTime > 1.0) ? PBufG[i].dPTime : PBufG[i].dExpectedPTime;
        if (pt > 1.0 && pt < earliest_pick) earliest_pick = pt;
    }
    if (earliest_pick == 1e15) earliest_pick = selected_otime + 15.0; 
    g_dWindowStart = earliest_pick - 20.0;

    for (int n = 0; n < g_NumSortedNodes; n++) {
        int i = g_SortedNodes[n].idx; 
        /* Ajuste de margen y_center para aprovechar espacio despues de quitar texto con cairo */
        int y_center = (n * g_spacing) + (g_spacing / 2) + 10;
        g_SortedNodes[n].y_top = y_center - (g_spacing / 2);
        g_SortedNodes[n].y_bottom = y_center + (g_spacing / 2);
        double dOldestTime = StaArray[i].dEndTime - ((double)StaArray[i].lRawCircCtr / StaArray[i].dSampRate) + (1.0 / StaArray[i].dSampRate);
        double pick_t = PBufG[i].dPTime;
        int is_real = 1;
        if (pick_t < 1.0 || PBufG[i].iUseMe <= 0) { pick_t = PBufG[i].dExpectedPTime; is_real = 0; }

        cairo_new_path(cr); cairo_set_line_width(cr, 1.0);
        if (is_real) cairo_set_source_rgb(cr, 0.2, 0.8, 0.2); else cairo_set_source_rgb(cr, 0.8, 0.2, 0.2);
        cairo_arc(cr, 15, y_center + 1, 5, 0, 2 * M_PI); cairo_fill_preserve(cr);
        cairo_set_source_rgb(cr, 0.0, 0.0, 0.0); cairo_stroke(cr);

        cairo_set_source_rgb(cr, 0.1, 0.2, 0.7); char sta_label[64]; 
        snprintf(sta_label, sizeof(sta_label), "%s (%.0f km)", StaArray[i].szStation, g_SortedNodes[n].dist * 6371.0);
        
        cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
        cairo_set_font_size(cr, 12); cairo_move_to(cr, 28, y_center + 5); cairo_show_text(cr, sta_label);
        cairo_set_source_rgb(cr, 0.8, 0.8, 0.8); cairo_set_line_width(cr, 1);
        cairo_move_to(cr, g_margin_left, y_center); cairo_line_to(cr, width - margin_right, y_center); cairo_stroke(cr);

        if (StaArray[i].dSampRate > 0.0 && StaArray[i].lRawCircCtr > 0) {
            long start_k = (long)((g_dWindowStart - dOldestTime) * StaArray[i].dSampRate);
            long end_k = (long)(((g_dWindowStart + g_dScreenTime) - dOldestTime) * StaArray[i].dSampRate);

            if (start_k < 0) start_k = 0;
            if (end_k > StaArray[i].lRawCircSize) end_k = StaArray[i].lRawCircSize; 
            if (end_k > StaArray[i].lRawCircCtr) end_k = StaArray[i].lRawCircCtr;

            long max_abs = 0;
            for (long k = start_k; k < end_k; k++) {
                if (StaArray[i].plFiltCircBuff[k] == INT_MAX) continue;
                long abs_val = labs(StaArray[i].plFiltCircBuff[k]); 
                if (abs_val > max_abs) max_abs = abs_val;
            }

            if (max_abs > 0) {
                if (!is_real) cairo_set_source_rgb(cr, 0.7, 0.7, 0.7); else cairo_set_source_rgb(cr, 0.1, 0.1, 0.9); 
                cairo_set_line_width(cr, 0.8);
                double scale = (g_spacing * 0.45) / (double)max_abs; scale *= g_zoom_factor;
                cairo_save(cr); cairo_rectangle(cr, g_margin_left, y_center - (g_spacing / 2.0), draw_width, g_spacing); cairo_clip(cr);

                int first = 1;
                for (long k = start_k; k < end_k; k++) {
                    if (StaArray[i].plFiltCircBuff[k] == INT_MAX) continue;
                    double t = dOldestTime + (double)k / StaArray[i].dSampRate;
                    double px = g_margin_left + ((t - g_dWindowStart) / g_dScreenTime) * draw_width;
                    double y = y_center - ((double)StaArray[i].plFiltCircBuff[k] * scale);
                    if (first) { cairo_move_to(cr, px, y); first = 0; } else { cairo_line_to(cr, px, y); }
                }
                cairo_stroke(cr); cairo_restore(cr);
            }
        }

        if (pick_t > 1.0 && pick_t >= g_dWindowStart && pick_t <= (g_dWindowStart + g_dScreenTime)) {
            int px = g_margin_left + (int)(((pick_t - g_dWindowStart) / g_dScreenTime) * draw_width);
            if (is_real) cairo_set_source_rgb(cr, 1.0, 0.0, 0.0); else cairo_set_source_rgb(cr, 1.0, 0.5, 0.0); 
            cairo_set_line_width(cr, 2);
            cairo_move_to(cr, px, y_center - (g_spacing / 2.5)); cairo_line_to(cr, px, y_center + (g_spacing / 2.5)); cairo_stroke(cr);
            cairo_set_font_size(cr, 11); cairo_move_to(cr, px + 4, y_center - (g_spacing / 3)); 
            if (is_real && strlen(PBufG[i].szPhase) > 0) { cairo_show_text(cr, PBufG[i].szPhase); } else { cairo_show_text(cr, is_real ? "P" : "P(teo)"); }
        }
    }
    return FALSE;
}

gboolean ew_background_tasks(gpointer user_data) {
    time_t timeNow; time(&timeNow);
    if (timeNow - timeLastBeat >= HeartBeatInt) { timeLastBeat = timeNow; Status(TypeHeartBeat, 0, ""); }
    int flag = tport_getflag(&InRegion);
    if (flag == TERMINATE || flag == MyPid) { gtk_main_quit(); return G_SOURCE_REMOVE; }
    return G_SOURCE_CONTINUE;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <configfile.d>\n", argv[0]); exit(1);
    }
    if (ReadConfig(argv[1]) != 0) exit(1);

    setenv("TZ", "GMT", 1); tzset(); 
    logit_init(argv[1], 0, 1024, LogFile);
    MyPid = getpid();
    
    gtk_init(&argc, &argv); 
    setlocale(LC_NUMERIC, "C");

    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(provider,
        "treeview grid-line { border-color: #555555; }\n" 
        "#btn_repick { background-image: none; background-color: #cce5ff; color: #0a58ca; font-weight: bold; padding: 5px 20px; border-radius: 4px; border: 1px solid #b6d4fe; }\n"
        "#btn_repick:hover { background-color: #b6d4fe; }\n"
        "#btn_relocate { background-image: none; background-color: #28a745; color: #ffffff; font-weight: bold; padding: 5px 20px; border-radius: 4px; border: 1px solid #218838; }\n"
        "#btn_relocate:hover { background-color: #218838; }\n"
        "#btn_force { background-image: none; background-color: #dc3545; color: #ffffff; font-weight: bold; padding: 5px 20px; border-radius: 4px; border: 1px solid #bd2130; }\n"
        "#btn_force:hover { background-color: #c82333; }\n"
        "#btn_fetch { background-image: none; background-color: #e2e3e5; color: #383d41; font-weight: bold; padding: 5px 15px; border-radius: 4px; border: 1px solid #d6d8db; }\n"
        "#btn_fetch:hover { background-color: #d6d8db; }\n"
        "#btn_refresh { background-image: none; background-color: #28a745; color: #ffffff; font-weight: bold; padding: 5px 15px; border-radius: 4px; border: 1px solid #218838; }\n"
        "#btn_refresh:hover { background-color: #218838; }\n"
        "#btn_filter { background-image: none; background-color: #28a745; color: #ffffff; font-weight: bold; padding: 2px 10px; border-radius: 4px; }\n"
        "#btn_filter:hover { background-color: #218838; }", -1, NULL);
        
    gtk_style_context_add_provider_for_screen(gdk_screen_get_default(), GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

    cargar_estaciones_dinamicas();
    ConnectToEarthworm();

    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Hypocenter database and repicker");
    gtk_window_set_default_size(GTK_WINDOW(window), 1100, 750); 
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    gtk_widget_add_events(window, GDK_KEY_PRESS_MASK);
    g_signal_connect(window, "key-press-event", G_CALLBACK(on_key_press), NULL);

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
    gtk_container_add(GTK_CONTAINER(window), vbox);

    GtkWidget *sw_lista = gtk_scrolled_window_new(NULL, NULL);
    gtk_widget_set_size_request(sw_lista, -1, 250);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw_lista), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    
    GtkListStore *store = gtk_list_store_new(20, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_DOUBLE, G_TYPE_INT, G_TYPE_INT, G_TYPE_DOUBLE, G_TYPE_DOUBLE, G_TYPE_DOUBLE);
    gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(store), 14, GTK_SORT_DESCENDING);

    tree_global = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
    gtk_widget_set_name(tree_global, "my_treeview");
    gtk_tree_view_set_grid_lines(GTK_TREE_VIEW(tree_global), GTK_TREE_VIEW_GRID_LINES_BOTH);
    
    GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree_global));
    g_signal_connect(selection, "changed", G_CALLBACK(on_row_selected), NULL);

    const char *headers[] = {"Date", "O-time", "Lat.", "Lon.", "Dep", "Res", "Azm", "#Stn", "ID", "Ms", "Mw", "Mwp", "Mb", "Ml"};
    for (int i = 0; i < 14; i++) {
        GtkCellRenderer *rend = gtk_cell_renderer_text_new();
        GtkTreeViewColumn *col = gtk_tree_view_column_new_with_attributes(headers[i], rend, "text", i, NULL);
        g_object_set(rend, "xalign", 0.5, NULL); 
        gtk_tree_view_column_set_alignment(col, 0.5); 
        gtk_tree_view_column_set_expand(col, TRUE); 
        gtk_tree_view_column_set_cell_data_func(col, rend, color_rows_func, NULL, NULL);
        gtk_tree_view_append_column(GTK_TREE_VIEW(tree_global), col);
    }
    gtk_container_add(GTK_CONTAINER(sw_lista), tree_global); 
    gtk_box_pack_start(GTK_BOX(vbox), sw_lista, FALSE, FALSE, 0);

    /* --- NUEVA BARRA INTERMEDIA (INFO EVENTO Y FETCH VISIBLE SIEMPRE) --- */
    GtkWidget *hbox_middle = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_widget_set_margin_start(hbox_middle, 10);
    gtk_widget_set_margin_end(hbox_middle, 10);
    gtk_widget_set_margin_top(hbox_middle, 5);
    gtk_widget_set_margin_bottom(hbox_middle, 5);

    lbl_event_info = gtk_label_new("");
    gtk_label_set_markup(GTK_LABEL(lbl_event_info), "<span foreground='#c82333' weight='bold'>Quake ID: --    O-Time: --</span>");

    btn_refresh = gtk_button_new_with_label("Refresh waveforms");
    gtk_widget_set_name(btn_refresh, "btn_refresh");
    g_signal_connect(btn_refresh, "clicked", G_CALLBACK(on_btn_refresh_clicked), NULL);

    box_fetch = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    GtkWidget *lbl_dist = gtk_label_new("Station distance (km):");
    entry_dist = gtk_entry_new(); gtk_entry_set_width_chars(GTK_ENTRY(entry_dist), 5); gtk_entry_set_text(GTK_ENTRY(entry_dist), "500");
    GtkWidget *lbl_time = gtk_label_new("Time window (min):");
    entry_time = gtk_entry_new(); gtk_entry_set_width_chars(GTK_ENTRY(entry_time), 5); gtk_entry_set_text(GTK_ENTRY(entry_time), "2");
    GtkWidget *btn_fetch = gtk_button_new_with_label("Fetch");
    gtk_widget_set_name(btn_fetch, "btn_fetch"); 
    g_signal_connect(btn_fetch, "clicked", G_CALLBACK(on_btn_fetch_clicked), NULL);
    
    gtk_box_pack_start(GTK_BOX(box_fetch), lbl_dist, FALSE, FALSE, 0); 
    gtk_box_pack_start(GTK_BOX(box_fetch), entry_dist, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box_fetch), gtk_label_new("  "), FALSE, FALSE, 0); 
    gtk_box_pack_start(GTK_BOX(box_fetch), lbl_time, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box_fetch), entry_time, FALSE, FALSE, 0); 
    gtk_box_pack_start(GTK_BOX(box_fetch), btn_fetch, FALSE, FALSE, 5);

    gtk_box_pack_start(GTK_BOX(hbox_middle), lbl_event_info, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(hbox_middle), btn_refresh, FALSE, FALSE, 15);
    GtkWidget *spacer_mid = gtk_label_new("");
    gtk_box_pack_start(GTK_BOX(hbox_middle), spacer_mid, TRUE, TRUE, 0);
    gtk_box_pack_end(GTK_BOX(hbox_middle), box_fetch, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(vbox), hbox_middle, FALSE, FALSE, 0);

    /* --- CANVAS --- */
    GtkWidget *sw_ondas = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw_ondas), GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
    gtk_box_pack_start(GTK_BOX(vbox), sw_ondas, TRUE, TRUE, 0);

    canvas_global = gtk_drawing_area_new();
    gtk_widget_set_size_request(canvas_global, -1, 600); 
    gtk_widget_add_events(canvas_global, GDK_BUTTON_PRESS_MASK);
    g_signal_connect(G_OBJECT(canvas_global), "button-press-event", G_CALLBACK(on_canvas_clicked), NULL);
    gtk_container_add(GTK_CONTAINER(sw_ondas), canvas_global);
    g_signal_connect(G_OBJECT(canvas_global), "draw", G_CALLBACK(on_draw_signal), NULL);

    /* --- BARRA INFERIOR --- */
    GtkWidget *hbox_bottom = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_widget_set_margin_start(hbox_bottom, 10); gtk_widget_set_margin_end(hbox_bottom, 10); gtk_widget_set_margin_bottom(hbox_bottom, 5);
    
    GtkWidget *hbox_center = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 15);
    
    btn_repick = gtk_button_new_with_label("Repick mode");
    gtk_widget_set_name(btn_repick, "btn_repick"); gtk_widget_set_sensitive(btn_repick, FALSE); 
    g_signal_connect(btn_repick, "clicked", G_CALLBACK(on_btn_repick_clicked), NULL);

    /* --- GESTOR DE FILTROS DINAMICOS --- */
    GtkWidget *box_filter = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
    
    GtkWidget *lbl_filter = gtk_label_new("  Filter:");
    combo_filter = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_filter), "Raw (No Filter)");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_filter), "High-Pass");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_filter), "Low-Pass");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_filter), "Band-Pass");
    gtk_combo_box_set_active(GTK_COMBO_BOX(combo_filter), 0); 
    g_signal_connect(combo_filter, "changed", G_CALLBACK(on_filter_changed), NULL);

    GtkWidget *lbl_f1 = gtk_label_new(" F1(Hz):");
    entry_freq1 = gtk_entry_new();
    gtk_entry_set_width_chars(GTK_ENTRY(entry_freq1), 4);
    gtk_entry_set_text(GTK_ENTRY(entry_freq1), "0.7");
    gtk_widget_set_sensitive(entry_freq1, FALSE);

    GtkWidget *lbl_f2 = gtk_label_new(" F2(Hz):");
    entry_freq2 = gtk_entry_new();
    gtk_entry_set_width_chars(GTK_ENTRY(entry_freq2), 4);
    gtk_entry_set_text(GTK_ENTRY(entry_freq2), "2.0");
    gtk_widget_set_sensitive(entry_freq2, FALSE);

    GtkWidget *lbl_order = gtk_label_new(" Ord:");
    combo_order = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_order), "2");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_order), "4");
    gtk_combo_box_set_active(GTK_COMBO_BOX(combo_order), 1); 
    gtk_widget_set_sensitive(combo_order, FALSE);

    btn_apply_filter = gtk_button_new_with_label("Apply");
    gtk_widget_set_name(btn_apply_filter, "btn_filter");
    gtk_widget_set_sensitive(btn_apply_filter, FALSE);
    g_signal_connect(btn_apply_filter, "clicked", G_CALLBACK(on_btn_apply_filter_clicked), NULL);

    gtk_box_pack_start(GTK_BOX(box_filter), lbl_filter, FALSE, FALSE, 2);
    gtk_box_pack_start(GTK_BOX(box_filter), combo_filter, FALSE, FALSE, 2);
    gtk_box_pack_start(GTK_BOX(box_filter), lbl_f1, FALSE, FALSE, 2);
    gtk_box_pack_start(GTK_BOX(box_filter), entry_freq1, FALSE, FALSE, 2);
    gtk_box_pack_start(GTK_BOX(box_filter), lbl_f2, FALSE, FALSE, 2);
    gtk_box_pack_start(GTK_BOX(box_filter), entry_freq2, FALSE, FALSE, 2);
    gtk_box_pack_start(GTK_BOX(box_filter), lbl_order, FALSE, FALSE, 2);
    gtk_box_pack_start(GTK_BOX(box_filter), combo_order, FALSE, FALSE, 2);
    gtk_box_pack_start(GTK_BOX(box_filter), btn_apply_filter, FALSE, FALSE, 5);

    btn_force = gtk_button_new_with_label("Force Relocation");
    gtk_widget_set_name(btn_force, "btn_force"); gtk_widget_set_sensitive(btn_force, FALSE); 
    g_signal_connect(btn_force, "clicked", G_CALLBACK(on_btn_force_clicked), NULL);

    GtkWidget *spacer = gtk_label_new(""); 

    gtk_box_pack_start(GTK_BOX(hbox_center), btn_repick, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox_center), box_filter, FALSE, FALSE, 5);
    
    gtk_box_pack_start(GTK_BOX(hbox_bottom), hbox_center, FALSE, FALSE, 0); 
    gtk_box_pack_start(GTK_BOX(hbox_bottom), spacer, TRUE, TRUE, 0);
    gtk_box_pack_end(GTK_BOX(hbox_bottom), btn_force, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(vbox), hbox_bottom, FALSE, FALSE, 0);

    cargar_sismos_iniciales(tree_global);
    gtk_widget_show_all(window);
    
    g_timeout_add(1000, ew_background_tasks, NULL);
    g_timeout_add(2000, check_history_file_loop, tree_global);
    g_timeout_add(3000, waveform_reload_timer, NULL);

    gtk_main(); 
    tport_detach(&InRegion); 
    return 0;
}
