#define _GNU_SOURCE
#include <gtk/gtk.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <locale.h>

/* Cabeceras de Earthworm y ATWC */
#pragma warning(disable: 4005)
#include <earthworm.h>
#include <transport.h>
#include <kom.h>
#include "earlybirdlib.h"

#define MAX_STR 256
#define MAX_QUAKES 1000
#define MAX_ESTA 512

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* --- CONFIGURACION DEL MODULO --- */
char MyModName[MAX_STR] = "MOD_STAEVDISP";
char RingName[MAX_STR] = "HYPO_RING";
char StaFile[MAX_STR] = "";
char StaDataFile[MAX_STR] = "";
char CalibsFile[MAX_STR] = "";
char QuakeFile[MAX_STR] = "";
char MapImageFile[MAX_STR] = "";
int  HeartBeatInt = 30;
int  LogFile = 1;

/* PARAMETRO: Tamaño de los triángulos de estación (Leído obligatoriamente del .d) */
double g_station_size = 5.0;

pid_t MyPid;
time_t timeLastBeat = 0;
unsigned char TypeHeartBeat = 0;
unsigned char TypeError = 0;
unsigned char MyModId = 0;
unsigned char MyInstId = 0;
SHM_INFO Region;

/* --- ESTRUCTURAS DE DATOS --- */
typedef struct {
    double lat;
    double lon;
    double mag;
    int qid;
} QUAKE_MARKER;

QUAKE_MARKER QuakeList[MAX_QUAKES];
int NumQuakes = 0;

STATION *StaArray = NULL;
int NumEstaciones = 0;
double g_min_elev = 99999.0;
double g_max_elev = -99999.0;

time_t last_file_mod_time = 0;
GdkPixbuf *g_map_pixbuf = NULL;

/* --- Variables de Control de Zoom y Paneo del Mapa --- */
double g_map_zoom = 2.0; 
double g_map_pan_x = 0.0;
double g_map_pan_y = 0.0;
gboolean g_is_dragging = FALSE;
double g_last_mouse_x = 0.0;
double g_last_mouse_y = 0.0;

/* Control de Eventos a Mostrar */
int g_max_events_display = 10;

/* Widgets de la Interfaz */
GtkWidget *map_canvas;

/* --------------------------------------------------------------------
 * LECTURA DE CONFIGURACION EARTHWORM
 * -------------------------------------------------------------------- */
int ReadConfig(char *configfile) {
    /* FIX: Incrementado a 10 comandos para forzar la lectura de StationSize */
    int ncommand = 10, nmiss = 0, i;
    char init[10] = {0};
    char *com, *str;

    if (!k_open(configfile)) {
        fprintf(stderr, "new_staevdisp: Error abriendo archivo config <%s>\n", configfile);
        return -1;
    }

    while (k_rd()) {
        com = k_str();
        if (!com || com[0] == '#') continue;

        if (k_its("MyModuleId")) { str = k_str(); if (str) strcpy(MyModName, str); init[0] = 1; } 
        else if (k_its("RingName")) { str = k_str(); if (str) strcpy(RingName, str); init[1] = 1; } 
        else if (k_its("HeartBeatInt")) { HeartBeatInt = k_int(); init[2] = 1; } 
        else if (k_its("LogFile")) { LogFile = k_int(); init[3] = 1; } 
        else if (k_its("QuakeFile")) { str = k_str(); if (str) strcpy(QuakeFile, str); init[4] = 1; } 
        else if (k_its("MapImageFile")) { str = k_str(); if (str) strcpy(MapImageFile, str); init[5] = 1; } 
        else if (k_its("StaFile")) { str = k_str(); if (str) strcpy(StaFile, str); init[6] = 1; }
        else if (k_its("StaDataFile")) { str = k_str(); if (str) strcpy(StaDataFile, str); init[7] = 1; }
        else if (k_its("CalibsFile")) { str = k_str(); if (str) strcpy(CalibsFile, str); init[8] = 1; }
        /* Ahora es OBLIGATORIO para garantizar que Earthworm no lo ignore */
        else if (k_its("StationSize")) { g_station_size = k_val(); init[9] = 1; }
        else { continue; }
        
        if (k_err()) {
            fprintf(stderr, "new_staevdisp: Error parseando <%s> en <%s>\n", com, configfile);
            return -1;
        }
    }
    for (i = 0; i < ncommand; i++) if (!init[i]) nmiss++;
    k_close();
    if (nmiss > 0) {
        fprintf(stderr, "new_staevdisp: ERROR, faltan parametros (Ej. StationSize) en <%s>\n", configfile);
        return -1;
    }
    return 0;
}

void Status(unsigned char type, short ierr, char *note) {
    MSG_LOGO logo; char msg[256]; time_t t;
    logo.instid = MyInstId; logo.mod = MyModId; logo.type = type;
    time(&t);
    if (type == TypeHeartBeat) sprintf(msg, "%ld %d\n", (long) t, MyPid);
    else if (type == TypeError) {
        sprintf(msg, "%ld %hd %s\n", (long) t, ierr, note);
        logit("et", "new_staevdisp: Error: %s\n", note);
    }
    tport_putmsg(&Region, &logo, strlen(msg), msg);
}

void ConnectToEarthworm() {
    long RingKey = GetKey(RingName);
    
    if (RingKey == -1) { printf("Error: Anillo invalido <%s>.\n", RingName); exit(-1); }
    
    if (GetLocalInst(&MyInstId) != 0) MyInstId = 0;
    if (GetModId(MyModName, &MyModId) != 0) {
        if (GetModId("MOD_WILDCARD", &MyModId) != 0) MyModId = 0;
    }
    
    if (GetType("TYPE_HEARTBEAT", &TypeHeartBeat) != 0) TypeHeartBeat = 0; 
    if (GetType("TYPE_ERROR", &TypeError) != 0) TypeError = 0; 
    
    tport_attach(&Region, RingKey);
    printf("=== CONECTADO A %s ===\n", RingName);
}

gboolean ew_background_tasks(gpointer user_data) {
    time_t timeNow; time(&timeNow);
    if (timeNow - timeLastBeat >= HeartBeatInt) {
        timeLastBeat = timeNow;
        Status(TypeHeartBeat, 0, "");
    }

    int flag = tport_getflag(&Region);
    if (flag == TERMINATE || flag == MyPid) {
        printf("new_staevdisp: Señal de terminacion recibida. Cerrando...\n");
        gtk_main_quit();
        return G_SOURCE_REMOVE;
    }
    return G_SOURCE_CONTINUE;
}

/* --------------------------------------------------------------------
 * CARGA DE DATOS ESTÁTICOS Y DINÁMICOS
 * -------------------------------------------------------------------- */
void LoadStations() {
    StaArray = (STATION *) calloc(MAX_ESTA, sizeof(STATION));
    if (StaArray == NULL) {
        fprintf(stderr, ">> FATAL: No se pudo reservar memoria para StaArray.\n");
        exit(-1);
    }
    
    /* Leemos las coordenadas y metadata apoyandonos en la libreria del ATWC */
    if (ReadStationList(&StaArray, &NumEstaciones, StaFile, StaDataFile, CalibsFile, MAX_ESTA, 0) == -1) {
        fprintf(stderr, ">> Error leyendo la lista de estaciones o archivos faltantes.\n");
    } else {
        printf(">> Cargadas %d estaciones para graficar (Tamanio: %.1f px).\n", NumEstaciones, g_station_size);
        
        /* Escanear minimos y maximos de elevacion para el gradiente de color */
        for (int i = 0; i < NumEstaciones; i++) {
            if (StaArray[i].dElevation < g_min_elev) g_min_elev = StaArray[i].dElevation;
            if (StaArray[i].dElevation > g_max_elev) g_max_elev = StaArray[i].dElevation;
        }
        /* Fallback si todas tienen la misma elevacion */
        if (g_min_elev >= g_max_elev) { g_min_elev = 0.0; g_max_elev = 1000.0; }
    }
}

void LoadQuakeHistory() {
    FILE *f = fopen(QuakeFile, "r");
    if (!f) return;
    
    char line[512];
    NumQuakes = 0;
    
    while (fgets(line, sizeof(line), f) && NumQuakes < MAX_QUAKES) {
        if (strlen(line) < 20) continue;

        double otime, lat, lon, depth, res, azm, pref_mag;
        int nps, qid, qver, npref, status;
        char mag_type[16];

        /* Formato universal del oldquakeX.dat (Tomado del parser de new_hypo_display) */
        int parsed = sscanf(line, "%lf %lf %lf %lf %d %15s %lf %d %d %d %lf %lf %*f %*d %*f %*d %*f %*d %*f %*d %*f %*d %*f %d",
               &otime, &lat, &lon, &pref_mag, &npref, mag_type, &depth, &qid, &qver, &nps, &res, &azm, &status);
                
        if (parsed >= 12) {
            QuakeList[NumQuakes].lat = lat;
            
            /* Ajuste de longitud (0 a 360 o -180 a 180) */
            double dLon = lon;
            if (dLon > 180.0) dLon -= 360.0;
            
            QuakeList[NumQuakes].lon = dLon;
            QuakeList[NumQuakes].mag = pref_mag;
            QuakeList[NumQuakes].qid = qid;
            NumQuakes++;
        }
    }
    fclose(f);
}

/* --------------------------------------------------------------------
 * INTERFAZ DE USUARIO: MENUS Y EVENTOS
 * -------------------------------------------------------------------- */
void on_events_number_activate(GtkWidget *widget, gpointer data) {
    GtkWidget *window = GTK_WIDGET(data);
    GtkWidget *dialog = gtk_dialog_new_with_buttons("Events number", GTK_WINDOW(window),
        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
        "_OK", GTK_RESPONSE_ACCEPT, "_Cancel", GTK_RESPONSE_REJECT, NULL);

    GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_box_pack_start(GTK_BOX(content_area), hbox, TRUE, TRUE, 15);
    GtkWidget *label = gtk_label_new("Number of events to display:");
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 5);

    GtkWidget *spin = gtk_spin_button_new_with_range(1, MAX_QUAKES, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin), g_max_events_display);
    gtk_box_pack_start(GTK_BOX(hbox), spin, FALSE, FALSE, 5);
    
    gtk_widget_show_all(dialog);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        g_max_events_display = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(spin));
        if (map_canvas) gtk_widget_queue_draw(map_canvas);
    }
    gtk_widget_destroy(dialog);
}

static gboolean on_map_scroll(GtkWidget *widget, GdkEventScroll *event, gpointer data) {
    if (event->direction == GDK_SCROLL_UP) {
        g_map_zoom *= 1.2; 
    } else if (event->direction == GDK_SCROLL_DOWN) {
        g_map_zoom /= 1.2; 
    }
    
    if (g_map_zoom < 0.2) g_map_zoom = 0.2;
    if (g_map_zoom > 100.0) g_map_zoom = 100.0;
    
    gtk_widget_queue_draw(widget);
    return TRUE;
}

static gboolean on_map_button_press(GtkWidget *widget, GdkEventButton *event, gpointer data) {
    if (event->button == 1) { 
        g_is_dragging = TRUE;
        g_last_mouse_x = event->x;
        g_last_mouse_y = event->y;
    }
    return TRUE;
}

static gboolean on_map_button_release(GtkWidget *widget, GdkEventButton *event, gpointer data) {
    if (event->button == 1) { 
        g_is_dragging = FALSE;
    }
    return TRUE;
}

static gboolean on_map_motion(GtkWidget *widget, GdkEventMotion *event, gpointer data) {
    if (g_is_dragging) {
        double dx = event->x - g_last_mouse_x;
        double dy = event->y - g_last_mouse_y;
        g_map_pan_x += dx;
        g_map_pan_y += dy;
        g_last_mouse_x = event->x;
        g_last_mouse_y = event->y;
        gtk_widget_queue_draw(widget);
    }
    return TRUE;
}

/* --------------------------------------------------------------------
 * FUNCION VIGIA: Refresca el catalogo si el archivo en disco cambia
 * -------------------------------------------------------------------- */
static gboolean check_history_file_loop(gpointer data) {
    struct stat file_stat;
    
    if (stat(QuakeFile, &file_stat) == 0) {
        if (file_stat.st_mtime != last_file_mod_time) {
            last_file_mod_time = file_stat.st_mtime;
            
            LoadQuakeHistory();
            
            if (map_canvas) {
                gtk_widget_queue_draw(map_canvas);
            }
        }
    }
    return TRUE; 
}

/* --------------------------------------------------------------------
 * CANVAS DEL MAPA (Proyección Equirectangular + Renderizado Dinámico)
 * -------------------------------------------------------------------- */
static gboolean on_draw_map(GtkWidget *widget, cairo_t *cr, gpointer data) {
    guint width = gtk_widget_get_allocated_width(widget);
    guint height = gtk_widget_get_allocated_height(widget);

    if (g_map_pixbuf != NULL) {
        double img_w = gdk_pixbuf_get_width(g_map_pixbuf);
        double img_h = gdk_pixbuf_get_height(g_map_pixbuf);
        
        /* Determinar centro virtual de la pantalla */
        double screen_cx = width / 2.0 + g_map_pan_x;
        double screen_cy = height / 2.0 + g_map_pan_y;

        /* Enfoque por defecto en Chile si la imagen es el mapamundi entero (-70.0 Lon, -30.0 Lat) */
        double center_x = img_w * (-70.0 + 180.0) / 360.0;
        double center_y = img_h * (90.0 - (-30.0)) / 180.0;

        cairo_save(cr);
        
        /* 1. Mover el origen al centro de la ventana y aplicar Paneo y Zoom Global */
        cairo_translate(cr, screen_cx, screen_cy);
        cairo_scale(cr, g_map_zoom, g_map_zoom);
        cairo_translate(cr, -center_x, -center_y);

        /* 2. Dibujar Imagen Base */
        gdk_cairo_set_source_pixbuf(cr, g_map_pixbuf, 0, 0);
        cairo_paint(cr);
        
        /* 3. Dibujar Grilla (Latitud / Longitud) */
        double step = 10.0; 
        if (g_map_zoom > 30.0) step = 1.0;
        else if (g_map_zoom > 15.0) step = 2.0;
        else if (g_map_zoom > 5.0) step = 5.0;

        cairo_new_path(cr);
        cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.3); /* Blanco semi-transparente */
        cairo_set_line_width(cr, 1.0 / g_map_zoom);    /* Grosor invariante al zoom */

        for (double lon = -180.0; lon <= 180.0; lon += step) {
            double x = img_w * (lon + 180.0) / 360.0;
            cairo_move_to(cr, x, 0); cairo_line_to(cr, x, img_h);
        }
        for (double lat = -90.0; lat <= 90.0; lat += step) {
            double y = img_h * (90.0 - lat) / 180.0;
            cairo_move_to(cr, 0, y); cairo_line_to(cr, img_w, y);
        }
        cairo_stroke(cr);

        /* 4. Dibujar Estaciones con Color Dinámico de Elevación */
        cairo_set_line_width(cr, 1.0 / g_map_zoom); 
        
        /* USO DE LA VARIABLE GLOBAL DE CONFIGURACIÓN DE TAMAÑO */
        double s_size = g_station_size / g_map_zoom; /* Tamaño del triángulo ajustado al zoom */

        for(int i = 0; i < NumEstaciones; i++) {
            if (StaArray[i].dLat != 0.0 && StaArray[i].dLon != 0.0) {
                double x = img_w * (StaArray[i].dLon + 180.0) / 360.0;
                double y = img_h * (90.0 - StaArray[i].dLat) / 180.0;

                /* Calcular el ratio de elevación para la paleta de colores [0.0 = bajo, 1.0 = alto] */
                double elev_ratio = (StaArray[i].dElevation - g_min_elev) / (g_max_elev - g_min_elev);
                if (elev_ratio < 0.0) elev_ratio = 0.0;
                if (elev_ratio > 1.0) elev_ratio = 1.0;

                /* Gradiente: Verde Claro (Baja elevación) a Verde Oscuro (Alta elevación) */
                double r = 0.6 - (0.6 * elev_ratio); 
                double g = 1.0 - (0.7 * elev_ratio); 
                double b = 0.6 - (0.6 * elev_ratio); 

                cairo_new_path(cr); /* Levantar lápiz, evita lineas anómalas */
                cairo_set_source_rgb(cr, r, g, b);
                cairo_move_to(cr, x, y - s_size);
                cairo_line_to(cr, x - s_size, y + s_size);
                cairo_line_to(cr, x + s_size, y + s_size);
                cairo_close_path(cr);
                cairo_fill_preserve(cr);
                
                cairo_set_source_rgb(cr, 0.0, 0.0, 0.0); /* Borde negro para distinguir */
                cairo_stroke(cr);
            }
        }

        /* 5. Dibujar Sismos (Círculos Rojos con Escalado Lineal Controlado) */
        int events_to_draw = (NumQuakes > g_max_events_display) ? g_max_events_display : NumQuakes;
        
        for(int i = events_to_draw - 1; i >= 0; i--) { /* Pintamos de atras para adelante (los mas nuevos arriba) */
            double x = img_w * (QuakeList[i].lon + 180.0) / 360.0;
            double y = img_h * (90.0 - QuakeList[i].lat) / 180.0;

            double mag = QuakeList[i].mag;
            if (mag < 0.0) mag = 1.0; 
            
            /* Escalado Lineal Ajustado: Reduce superposicion pero mantiene diferencias obvias */
            double radius = (mag * 2.0) / g_map_zoom;

            cairo_new_path(cr); /* Levantar lápiz entre círculos */

            if (i == 0) {
                /* Último Evento: Rojo vivo (100% Opacidad), Borde grueso negro */
                cairo_set_source_rgba(cr, 1.0, 0.0, 0.0, 1.0); 
                cairo_arc(cr, x, y, radius, 0, 2 * M_PI);
                cairo_fill_preserve(cr);
                
                cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 1.0); 
                cairo_set_line_width(cr, 2.0 / g_map_zoom);
                cairo_stroke(cr);
            } else {
                /* Eventos Históricos: Rojo sutil (50% Opacidad), Borde oscuro intermedio */
                cairo_set_source_rgba(cr, 0.9, 0.2, 0.2, 0.50); 
                cairo_arc(cr, x, y, radius, 0, 2 * M_PI);
                cairo_fill_preserve(cr);
                
                cairo_set_source_rgba(cr, 0.4, 0.0, 0.0, 0.6); 
                cairo_set_line_width(cr, 1.0 / g_map_zoom);
                cairo_stroke(cr);
            }
        }

        cairo_restore(cr); /* Restaura la matriz a pixeles absolutos para dibujar UI (Leyendas) */
        
        /* --- LEYENDAS Y ESCALAS VERTICALES (UI FIJA) --- */
        cairo_new_path(cr);
        cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.85); /* Caja semitransparente expandida */
        cairo_rectangle(cr, 10, height - 370, 120, 360);
        cairo_fill_preserve(cr);
        cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
        cairo_set_line_width(cr, 1.0);
        cairo_stroke(cr);
        
        cairo_new_path(cr);
        cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
        cairo_set_font_size(cr, 12.0);
        cairo_move_to(cr, 20, height - 345);
        cairo_show_text(cr, "Magnitude");

        /* Escala de Círculos para Magnitudes (Disposición Vertical) */
        double mag_base_x = 40;
        double mag_base_y = height - 310;
        for (int m = 4; m <= 8; m++) {
            double r = m * 2.0; /* Coherente con el multiplicador ajustado del mapa */
            
            cairo_new_path(cr);
            cairo_set_source_rgba(cr, 0.9, 0.2, 0.2, 0.6); 
            cairo_arc(cr, mag_base_x, mag_base_y, r, 0, 2 * M_PI);
            cairo_fill_preserve(cr);
            
            cairo_set_source_rgba(cr, 0.4, 0.0, 0.0, 0.8);
            cairo_set_line_width(cr, 1.0);
            cairo_stroke(cr);
            
            cairo_new_path(cr);
            char m_lbl[8];
            snprintf(m_lbl, sizeof(m_lbl), "M%d", m);
            cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
            cairo_set_font_size(cr, 12.0);
            cairo_move_to(cr, mag_base_x + 25, mag_base_y + 4);
            cairo_show_text(cr, m_lbl);
            
            mag_base_y += 35; /* Espaciado vertical reajustado */
        }

        /* Escala de Colores para Elevación (Triángulo Verde + Texto) */
        double t_x = 25.0;
        double t_y = height - 120.0;
        
        /* FIX: Vinculamos el triángulo de la leyenda al tamaño del parámetro global */
        double leg_s_size = g_station_size; 

        cairo_new_path(cr);
        cairo_set_source_rgb(cr, 0.0, 0.8, 0.0); /* Verde representativo */
        cairo_move_to(cr, t_x, t_y - leg_s_size);
        cairo_line_to(cr, t_x - leg_s_size, t_y + leg_s_size);
        cairo_line_to(cr, t_x + leg_s_size, t_y + leg_s_size);
        cairo_close_path(cr);
        cairo_fill_preserve(cr);
        
        cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
        cairo_set_line_width(cr, 1.0);
        cairo_stroke(cr);

        cairo_new_path(cr);
        cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
        cairo_set_font_size(cr, 12.0);
        cairo_move_to(cr, t_x + 12.0, t_y + 4.0);
        cairo_show_text(cr, "Elevation (m)");

        cairo_pattern_t *pat = cairo_pattern_create_linear(0, height - 20, 0, height - 100);
        cairo_pattern_add_color_stop_rgb(pat, 0.0, 0.6, 1.0, 0.6); /* Verde Claro (Bajo) */
        cairo_pattern_add_color_stop_rgb(pat, 1.0, 0.0, 0.3, 0.0); /* Verde Oscuro (Alto) */
        
        cairo_new_path(cr);
        cairo_rectangle(cr, 25, height - 100, 20, 80);
        cairo_set_source(cr, pat);
        cairo_fill(cr);
        cairo_pattern_destroy(pat);
        
        cairo_new_path(cr);
        cairo_rectangle(cr, 25, height - 100, 20, 80);
        cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
        cairo_set_line_width(cr, 1.0);
        cairo_stroke(cr);

        /* Redondear valores de la escala de elevación para limpieza visual */
        int min_val = (int)(floor(g_min_elev / 100.0) * 100);
        if (min_val < 0 && g_min_elev >= 0.0) min_val = 0; 
        int max_val = (int)(ceil(g_max_elev / 100.0) * 100);

        char min_el[32], max_el[32];
        snprintf(min_el, sizeof(min_el), "%d", min_val);
        snprintf(max_el, sizeof(max_el), "%d", max_val);
        
        cairo_new_path(cr);
        cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
        cairo_set_font_size(cr, 14.0); /* Números de elevacion más grandes */
        
        cairo_move_to(cr, 55, height - 20);
        cairo_show_text(cr, min_el);
        
        cairo_move_to(cr, 55, height - 90);
        cairo_show_text(cr, max_el);

    } else {
        /* Fallback: Si no existe el archivo JPG, dibujar fondo gris/azulado */
        cairo_set_source_rgb(cr, 0.85, 0.92, 0.98); 
        cairo_rectangle(cr, 0, 0, width, height);
        cairo_fill(cr);

        cairo_set_source_rgb(cr, 0.5, 0.5, 0.5);
        cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
        cairo_set_font_size(cr, 14);
        cairo_move_to(cr, width / 2 - 120, height / 2);
        cairo_show_text(cr, "Falta archivo de mapa (Equirectangular)");
    }

    /* Marco perimetral */
    cairo_new_path(cr);
    cairo_set_source_rgb(cr, 0.3, 0.3, 0.3);
    cairo_set_line_width(cr, 3.0);
    cairo_rectangle(cr, 0, 0, width, height);
    cairo_stroke(cr);

    return FALSE;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Uso: %s <configfile.d>\n", argv[0]);
        exit(1);
    }
    if (ReadConfig(argv[1]) != 0) exit(1);

    setenv("TZ", "GMT", 1);
    tzset();
    logit_init(argv[1], 0, 1024, LogFile);
    MyPid = getpid();
    
    gtk_init(&argc, &argv);
    setlocale(LC_NUMERIC, "C");

    ConnectToEarthworm();
    
    /* 1. Cargar Estaciones (Extraer Lat/Lon y limites de Elevacion) */
    LoadStations();
    
    /* 2. Carga Inicial del Historial de Sismos */
    LoadQuakeHistory();
    
    /* 3. Cargar imagen del mapa base en memoria */
    GError *err = NULL;
    g_map_pixbuf = gdk_pixbuf_new_from_file(MapImageFile, &err);
    if (!g_map_pixbuf) {
        printf(">> [AVISO] No se pudo cargar %s: %s\n", MapImageFile, err->message);
        g_error_free(err);
    }

    /* --- CREACIÓN DE VENTANA GTK --- */
    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Network & Seismicity Viewer");
    gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    GtkWidget *vbox_main = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(window), vbox_main);

    /* --- MENU BAR --- */
    GtkWidget *menu_bar = gtk_menu_bar_new();
    
    GtkWidget *ctrl_panel_item = gtk_menu_item_new_with_label("Control Panel"); 
    GtkWidget *ctrl_panel_menu = gtk_menu_new();
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(ctrl_panel_item), ctrl_panel_menu);
    
    GtkWidget *events_number_item = gtk_menu_item_new_with_label("Events number");
    g_signal_connect(events_number_item, "activate", G_CALLBACK(on_events_number_activate), window);
    gtk_menu_shell_append(GTK_MENU_SHELL(ctrl_panel_menu), events_number_item);
    
    gtk_menu_shell_append(GTK_MENU_SHELL(menu_bar), ctrl_panel_item); 
    gtk_box_pack_start(GTK_BOX(vbox_main), menu_bar, FALSE, FALSE, 0);

    /* --- CANVAS DEL MAPA --- */
    map_canvas = gtk_drawing_area_new();
    gtk_widget_set_size_request(map_canvas, -1, -1); 
    
    /* VINCULAR EVENTOS DE RATÓN */
    gtk_widget_add_events(map_canvas, GDK_SCROLL_MASK | GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_POINTER_MOTION_MASK);
    g_signal_connect(G_OBJECT(map_canvas), "scroll-event", G_CALLBACK(on_map_scroll), NULL);
    g_signal_connect(G_OBJECT(map_canvas), "button-press-event", G_CALLBACK(on_map_button_press), NULL);
    g_signal_connect(G_OBJECT(map_canvas), "button-release-event", G_CALLBACK(on_map_button_release), NULL);
    g_signal_connect(G_OBJECT(map_canvas), "motion-notify-event", G_CALLBACK(on_map_motion), NULL);
    
    g_signal_connect(G_OBJECT(map_canvas), "draw", G_CALLBACK(on_draw_map), NULL);
    
    gtk_box_pack_start(GTK_BOX(vbox_main), map_canvas, TRUE, TRUE, 0);

    /* --- TIMERS DE TAREAS EN SEGUNDO PLANO --- */
    g_timeout_add(2000, check_history_file_loop, NULL);  /* Revisa sismos cada 2s */
    g_timeout_add(1000, ew_background_tasks, NULL);      /* Latidos EW cada 1s */

    gtk_widget_show_all(window);
    gtk_main();

    tport_detach(&Region);
    if (g_map_pixbuf) g_object_unref(g_map_pixbuf);
    if (StaArray) free(StaArray);
    
    return 0;
}
