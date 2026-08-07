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

/* --- CONFIGURACION DEL MODULO (NUEVO) --- */
char MyModName[MAX_STR] = "MOD_SUMMARY";
char RingName[MAX_STR] = "HYPO_RING";
char QuakeFile[MAX_STR] = "";
char MapImageFile[MAX_STR] = "";
int  HeartBeatInt = 30;
int  LogFile = 1;

pid_t MyPid;
time_t timeLastBeat = 0;
unsigned char TypeHeartBeat = 0;
unsigned char TypeError = 0;
unsigned char MyModId = 0;
unsigned char MyInstId = 0;
SHM_INFO Region;

/* Variables Globales */
HYPO Hypo;
time_t last_file_mod_time = 0;
gboolean has_valid_data = FALSE;
double g_epicenter_lat = 0.0;
double g_epicenter_lon = 0.0;
GdkPixbuf *g_map_pixbuf = NULL;

/* Variables de Control de Zoom y Paneo del Mapa */
double g_map_zoom = 2.0; /* Zoom inicial mayor para vista más regional */
double g_map_pan_x = 0.0;
double g_map_pan_y = 0.0;
gboolean g_is_dragging = FALSE;
double g_last_mouse_x = 0.0;
double g_last_mouse_y = 0.0;

/* Widgets de la Interfaz */
GtkWidget *lbl_origin_time;
GtkWidget *lbl_coordinates;
GtkWidget *lbl_depth;
GtkWidget *lbl_time_elapsed;
GtkWidget *map_canvas;

/* Widgets para la Tabla de Magnitudes */
GtkWidget *lbl_ml_val, *lbl_ml_stn;
GtkWidget *lbl_mwp_val, *lbl_mwp_stn;
GtkWidget *lbl_ms_val, *lbl_ms_stn;
GtkWidget *lbl_mb_val, *lbl_mb_stn;

/* --------------------------------------------------------------------
 * LECTURA DE CONFIGURACION EARTHWORM (NUEVO)
 * -------------------------------------------------------------------- */
int ReadConfig(char *configfile) {
    int ncommand = 6, nmiss = 0, i;
    char init[10] = {0};
    char *com, *str;

    if (!k_open(configfile)) {
        fprintf(stderr, "new_summary: Error abriendo archivo config <%s>\n", configfile);
        return -1;
    }

    while (k_rd()) {
        com = k_str();
        if (!com || com[0] == '#') continue;

        if (k_its("MyModuleId")) {
            str = k_str();
            if (str) strcpy(MyModName, str);
            init[0] = 1;
        } else if (k_its("RingName")) {
            str = k_str();
            if (str) strcpy(RingName, str);
            init[1] = 1;
        } else if (k_its("HeartBeatInt")) {
            HeartBeatInt = k_int();
            init[2] = 1;
        } else if (k_its("LogFile")) {
            LogFile = k_int();
            init[3] = 1;
        } else if (k_its("QuakeFile")) {
            str = k_str();
            if (str) strcpy(QuakeFile, str);
            init[4] = 1;
        } else if (k_its("MapImageFile")) {
            str = k_str();
            if (str) strcpy(MapImageFile, str);
            init[5] = 1;
        } else {
            continue;
        }
        if (k_err()) {
            fprintf(stderr, "new_summary: Error parseando <%s> en <%s>\n", com, configfile);
            return -1;
        }
    }
    for (i = 0; i < ncommand; i++) if (!init[i]) nmiss++;
    k_close();
    if (nmiss > 0) {
        fprintf(stderr, "new_summary: ERROR, faltan parametros en <%s>\n", configfile);
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
        logit("et", "new_summary: Error: %s\n", note);
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
        printf("new_summary: Señal de terminacion recibida de Earthworm. Cerrando...\n");
        gtk_main_quit();
        return G_SOURCE_REMOVE;
    }
    return G_SOURCE_CONTINUE;
}

/* --------------------------------------------------------------------
 * FUNCION AUXILIAR PARA ACTUALIZAR LA TABLA DE MAGNITUDES
 * -------------------------------------------------------------------- */
void update_mag_row(GtkWidget *lbl_val, GtkWidget *lbl_stn, double mag, int stn) {
    char buf[128];
    if (mag > 0.0) {
        snprintf(buf, sizeof(buf), "<span size='large'>%.1f</span>", mag);
        gtk_label_set_markup(GTK_LABEL(lbl_val), buf);
        snprintf(buf, sizeof(buf), "<span size='large'>%d</span>", stn);
        gtk_label_set_markup(GTK_LABEL(lbl_stn), buf);
    } else {
        gtk_label_set_markup(GTK_LABEL(lbl_val), "<span foreground='gray'>--</span>");
        gtk_label_set_markup(GTK_LABEL(lbl_stn), "<span foreground='gray'>--</span>");
    }
}

/* --------------------------------------------------------------------
 * EVENTOS DEL RATON (ZOOM IN/OUT Y PANEO EN EL MAPA)
 * -------------------------------------------------------------------- */
static gboolean on_map_scroll(GtkWidget *widget, GdkEventScroll *event, gpointer data) {
    if (event->direction == GDK_SCROLL_UP) {
        g_map_zoom *= 1.2; /* Acercar */
    } else if (event->direction == GDK_SCROLL_DOWN) {
        g_map_zoom /= 1.2; /* Alejar */
    }
    
    /* Topes maximos y minimos para el zoom */
    if (g_map_zoom < 0.2) g_map_zoom = 0.2;
    if (g_map_zoom > 50.0) g_map_zoom = 50.0;
    
    gtk_widget_queue_draw(widget);
    return TRUE;
}

static gboolean on_map_button_press(GtkWidget *widget, GdkEventButton *event, gpointer data) {
    if (event->button == 1) { /* Clic Izquierdo = Iniciar arrastre */
        g_is_dragging = TRUE;
        g_last_mouse_x = event->x;
        g_last_mouse_y = event->y;
    }
    return TRUE;
}

static gboolean on_map_button_release(GtkWidget *widget, GdkEventButton *event, gpointer data) {
    if (event->button == 1) { /* Soltar Clic Izquierdo = Fin arrastre */
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
 * FUNCION VIGIA: Lectura segura de history
 * -------------------------------------------------------------------- */
static gboolean update_summary_loop(gpointer data) {
    struct stat file_stat;
    static int last_processed_qid = 0;
    
    if (stat(QuakeFile, &file_stat) == 0) {
        if (file_stat.st_mtime > last_file_mod_time) {
            last_file_mod_time = file_stat.st_mtime;
            
            FILE *f = fopen(QuakeFile, "r");
            if (f) {
                char line[512];
                char target_line[512] = "";
                
                /* Leer la primera linea (el sismo mas reciente) y detenerse */
                while (fgets(line, sizeof(line), f)) {
                    if (strlen(line) > 20) {
                        strcpy(target_line, line);
                        break; 
                    }
                }
                fclose(f);

                if (strlen(target_line) > 0) {
                    double otime, lat, lon, pref_mag, depth, res, azm, mb, ml, ms, mwp, mw, rec_time;
                    int npref, qid, qver, nps, nmb, nml, nms, nmwp, nmw, status;
                    char mag_type[16];

                    int parsed = sscanf(target_line, "%lf %lf %lf %lf %d %15s %lf %d %d %d %lf %lf %lf %d %lf %d %lf %d %lf %d %lf %d %lf %d",
                       &otime, &lat, &lon, &pref_mag, &npref, mag_type, &depth, &qid, &qver, &nps, &res, &azm, 
                       &mb, &nmb, &ml, &nml, &ms, &nms, &mwp, &nmwp, &mw, &nmw, &rec_time, &status);

                    if (parsed >= 22) {
                        has_valid_data = TRUE;
                        Hypo.dOriginTime = otime;
                        g_epicenter_lat = lat;
                        
                        /* Earthworm guarda la longitud. Asegurar rango -180 a 180 para el mapa */
                        if (lon > 180.0) lon -= 360.0;
                        g_epicenter_lon = lon;
                        
                        /* Si es un sismo completamente nuevo, resetear zoom y paneo */
                        if (qid != last_processed_qid) {
                            g_map_zoom = 2.0; /* Zoom inicial mayor */
                            g_map_pan_x = 0.0;
                            g_map_pan_y = 0.0;
                            last_processed_qid = qid;
                        }
                        
                        printf(">> Nuevo sismo procesado! Actualizando interfaz y tabla de magnitudes...\n");
                        
                        char buffer[512]; 
                        
                        time_t rawtime = (time_t)(otime + 0.5);
                        struct tm *ptm = gmtime(&rawtime); 
                        if (ptm) {
                            snprintf(buffer, sizeof(buffer), "<span size='xx-large' weight='bold' foreground='#0055a4'>%02d:%02d:%02d UTC\n%02d/%02d/%04d</span>", 
                                     ptm->tm_hour, ptm->tm_min, ptm->tm_sec,
                                     ptm->tm_mday, ptm->tm_mon + 1, ptm->tm_year + 1900);
                            gtk_label_set_markup(GTK_LABEL(lbl_origin_time), buffer);
                        }
                        
                        snprintf(buffer, sizeof(buffer), "<span size='large'>Lat: %.3f  Lon: %.3f</span>", lat, lon);
                        gtk_label_set_markup(GTK_LABEL(lbl_coordinates), buffer);
                        
                        snprintf(buffer, sizeof(buffer), "<span size='large'>Depth: %.0f km</span>", depth);
                        gtk_label_set_markup(GTK_LABEL(lbl_depth), buffer);

                        /* Actualizamos las filas de magnitud visibles */
                        update_mag_row(lbl_ml_val, lbl_ml_stn, ml, nml);
                        update_mag_row(lbl_mwp_val, lbl_mwp_stn, mwp, nmwp);
                        update_mag_row(lbl_ms_val, lbl_ms_stn, ms, nms);
                        update_mag_row(lbl_mb_val, lbl_mb_stn, mb, nmb);

                        if (map_canvas) gtk_widget_queue_draw(map_canvas);
                    }
                }
            }
        }
    }

    if (has_valid_data && Hypo.dOriginTime > 0.0) {
        time_t current_time;
        time(&current_time);
        
        int diff_seconds = (int)difftime(current_time, (time_t)Hypo.dOriginTime);
        if (diff_seconds < 0) diff_seconds = 0;
        
        int hours = diff_seconds / 3600;
        int minutes = (diff_seconds % 3600) / 60;
        int seconds = diff_seconds % 60;
        
        char clock_buf[256];
        snprintf(clock_buf, sizeof(clock_buf), "<span size='40000' weight='bold' foreground='blue'>%02d:%02d:%02d</span>", 
                hours, minutes, seconds);
        gtk_label_set_markup(GTK_LABEL(lbl_time_elapsed), clock_buf);
    } else {
        gtk_label_set_markup(GTK_LABEL(lbl_time_elapsed), "<span size='40000' weight='bold' foreground='gray'>--:--:--</span>");
    }

    return TRUE; 
}

/* --------------------------------------------------------------------
 * CANVAS DEL MAPA (Interactivo con Zoom, Paneo y Grilla Numérica)
 * -------------------------------------------------------------------- */
static gboolean on_draw_map(GtkWidget *widget, cairo_t *cr, gpointer data) {
    guint width = gtk_widget_get_allocated_width(widget);
    guint height = gtk_widget_get_allocated_height(widget);

    if (g_map_pixbuf != NULL) {
        double img_w = gdk_pixbuf_get_width(g_map_pixbuf);
        double img_h = gdk_pixbuf_get_height(g_map_pixbuf);
        
        /* Por defecto, si no hay sismo, centramos en 0,0 (Ecuador/Meridiano) */
        double epi_x = img_w / 2.0;
        double epi_y = img_h / 2.0;

        if (has_valid_data) {
            /* PROYECCION EQUIRRECTANGULAR ESTANDAR EN PIXELES */
            epi_x = img_w * (g_epicenter_lon + 180.0) / 360.0;
            epi_y = img_h * (90.0 - g_epicenter_lat) / 180.0;
        }

        /* Determinar zoom efectivo */
        double effective_zoom = has_valid_data ? g_map_zoom : fmin((double)width/img_w, (double)height/img_h);

        cairo_save(cr);
        
        /* Mover el origen al CENTRO del widget, incluyendo el desplazamiento por arrastre */
        cairo_translate(cr, width / 2.0 + g_map_pan_x, height / 2.0 + g_map_pan_y);
        
        /* Aplicar el nivel de zoom solicitado */
        cairo_scale(cr, effective_zoom, effective_zoom);
        
        /* Mover el mapa para que el píxel del epicentro quede en el nuevo origen (0,0) */
        cairo_translate(cr, -epi_x, -epi_y);

        /* 1. PINTAR EL MAPA BASE */
        gdk_cairo_set_source_pixbuf(cr, g_map_pixbuf, 0, 0);
        cairo_paint(cr);
        
        /* 2. DIBUJAR LA GRILLA (LATITUD / LONGITUD) */
        double step = 10.0; /* Por defecto separacion cada 10 grados */
        if (effective_zoom > 30.0) step = 1.0;
        else if (effective_zoom > 15.0) step = 2.0;
        else if (effective_zoom > 5.0) step = 5.0;

        cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.4); /* Blanco semi-transparente */
        /* Asegurar que las lineas siempre tengan 1px visual de grosor sin importar el zoom */
        cairo_set_line_width(cr, 1.0 / effective_zoom); 

        /* Lineas Verticales (Longitud) */
        for (double lon = -180.0; lon <= 180.0; lon += step) {
            double x = img_w * (lon + 180.0) / 360.0;
            cairo_move_to(cr, x, 0);
            cairo_line_to(cr, x, img_h);
        }
        /* Lineas Horizontales (Latitud) */
        for (double lat = -90.0; lat <= 90.0; lat += step) {
            double y = img_h * (90.0 - lat) / 180.0;
            cairo_move_to(cr, 0, y);
            cairo_line_to(cr, img_w, y);
        }
        cairo_stroke(cr);
        
        /* 3. DIBUJAR ETIQUETAS NUMERICAS DE LA GRILLA (Ancladas a los bordes de la vista actual) */
        cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.85); /* Blanco más legible para el texto */
        cairo_set_font_size(cr, 12.0 / effective_zoom); /* Tamaño de letra inversamente proporcional al zoom */
        
        /* Calcular los limites "Top" y "Left" de la zona visible del mapa en este momento */
        double world_top_y = epi_y - (height / 2.0 + g_map_pan_y) / effective_zoom;
        double world_left_x = epi_x - (width / 2.0 + g_map_pan_x) / effective_zoom;
        
        /* Prevenir que el texto se dibuje fuera de la imagen del mapa */
        if (world_top_y < 0) world_top_y = 0;
        if (world_left_x < 0) world_left_x = 0;

        char lbl[32];
        for (double lon = -180.0; lon <= 180.0; lon += step) {
            double x = img_w * (lon + 180.0) / 360.0;
            cairo_move_to(cr, x + 4.0 / effective_zoom, world_top_y + 14.0 / effective_zoom);
            /* \xC2\xB0 es el símbolo de grado en UTF-8 */
            snprintf(lbl, sizeof(lbl), "%.0f\xC2\xB0", lon);
            cairo_show_text(cr, lbl);
        }
        for (double lat = -90.0; lat <= 90.0; lat += step) {
            double y = img_h * (90.0 - lat) / 180.0;
            cairo_move_to(cr, world_left_x + 4.0 / effective_zoom, y - 4.0 / effective_zoom);
            snprintf(lbl, sizeof(lbl), "%.0f\xC2\xB0", lat);
            cairo_show_text(cr, lbl);
        }
        
        cairo_restore(cr);
    } else {
        /* Fallback: Si no existe world_map.jpg, dibujar mar azulado */
        cairo_set_source_rgb(cr, 0.85, 0.92, 0.98); 
        cairo_rectangle(cr, 0, 0, width, height);
        cairo_fill(cr);

        cairo_set_source_rgb(cr, 0.5, 0.5, 0.5);
        cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
        cairo_set_font_size(cr, 14);
        cairo_move_to(cr, width / 2 - 120, height / 2);
        cairo_show_text(cr, "Falta archivo: mapa (Equirectangular)");
    }

    /* 4. DIBUJAR EPICENTRO (Siempre en el centro virtual de la pantalla considerando el paneo) */
    if (has_valid_data) {
        double screen_epi_x = width / 2.0 + g_map_pan_x;
        double screen_epi_y = height / 2.0 + g_map_pan_y;

        /* ¡LEVANTAR EL LÁPIZ ANTES DE DIBUJAR EL EPICENTRO! */
        cairo_new_path(cr); 
        
        /* Epicentro: Círculo rojo limpio y profesional */
        cairo_set_source_rgb(cr, 1.0, 0.0, 0.0); /* Rojo */
        cairo_arc(cr, screen_epi_x, screen_epi_y, 6, 0, 2 * M_PI);
        cairo_fill_preserve(cr);
        
        cairo_set_source_rgb(cr, 1.0, 1.0, 1.0); /* Blanco borde para alto contraste */
        cairo_set_line_width(cr, 2.0);
        cairo_stroke(cr);
    }

    /* Marco perimetral del mapa */
    cairo_new_path(cr); /* Y lo volvemos a levantar para el marco exterior */
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
    
    /* Cargar imagen del mapa base en memoria */
    GError *err = NULL;
    g_map_pixbuf = gdk_pixbuf_new_from_file(MapImageFile, &err);
    if (!g_map_pixbuf) {
        printf(">> [AVISO] No se pudo cargar %s: %s\n", MapImageFile, err->message);
        g_error_free(err);
    }

    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Earthquake Summary & Map");
    
    gtk_window_set_default_size(GTK_WINDOW(window), 450, 750);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    GtkWidget *vbox_main = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(vbox_main), 15);
    gtk_container_add(GTK_CONTAINER(window), vbox_main);

    /* O-Time al tope */
    lbl_origin_time = gtk_label_new("<span size='xx-large' weight='bold' foreground='gray'>Esperando Datos...</span>");
    gtk_label_set_use_markup(GTK_LABEL(lbl_origin_time), TRUE);
    gtk_label_set_justify(GTK_LABEL(lbl_origin_time), GTK_JUSTIFY_CENTER);
    gtk_box_pack_start(GTK_BOX(vbox_main), lbl_origin_time, FALSE, FALSE, 5);

    lbl_coordinates = gtk_label_new("");
    gtk_box_pack_start(GTK_BOX(vbox_main), lbl_coordinates, FALSE, FALSE, 0);

    lbl_depth = gtk_label_new("");
    gtk_box_pack_start(GTK_BOX(vbox_main), lbl_depth, FALSE, FALSE, 0);

    GtkWidget *separator1 = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_box_pack_start(GTK_BOX(vbox_main), separator1, FALSE, FALSE, 5);

    GtkWidget *lbl_timer_title = gtk_label_new("<span size='large'>Time Since Quake:</span>");
    gtk_label_set_use_markup(GTK_LABEL(lbl_timer_title), TRUE);
    gtk_box_pack_start(GTK_BOX(vbox_main), lbl_timer_title, FALSE, FALSE, 0);

    lbl_time_elapsed = gtk_label_new("");
    gtk_box_pack_start(GTK_BOX(vbox_main), lbl_time_elapsed, FALSE, FALSE, 0);

    GtkWidget *separator2 = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_box_pack_start(GTK_BOX(vbox_main), separator2, FALSE, FALSE, 5);

    /* TABLA DE MAGNITUDES (GtkGrid) */
    GtkWidget *mag_grid = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(mag_grid), 40);
    gtk_grid_set_row_spacing(GTK_GRID(mag_grid), 5);
    gtk_widget_set_halign(mag_grid, GTK_ALIGN_CENTER);

    /* Cabeceras de Tabla */
    GtkWidget *h1 = gtk_label_new(""); gtk_label_set_markup(GTK_LABEL(h1), "<b>Type</b>");
    GtkWidget *h2 = gtk_label_new(""); gtk_label_set_markup(GTK_LABEL(h2), "<b>Magnitude</b>");
    GtkWidget *h3 = gtk_label_new(""); gtk_label_set_markup(GTK_LABEL(h3), "<b>Stations</b>");
    gtk_grid_attach(GTK_GRID(mag_grid), h1, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(mag_grid), h2, 1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(mag_grid), h3, 2, 0, 1, 1);

    /* Fila 1: Ml */
    GtkWidget *l_ml = gtk_label_new(""); gtk_label_set_markup(GTK_LABEL(l_ml), "<span size='large'>Ml</span>");
    lbl_ml_val = gtk_label_new("--"); lbl_ml_stn = gtk_label_new("--");
    gtk_grid_attach(GTK_GRID(mag_grid), l_ml, 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(mag_grid), lbl_ml_val, 1, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(mag_grid), lbl_ml_stn, 2, 1, 1, 1);

    /* Fila 2: Mwp */
    GtkWidget *l_mwp = gtk_label_new(""); gtk_label_set_markup(GTK_LABEL(l_mwp), "<span size='large'>Mwp</span>");
    lbl_mwp_val = gtk_label_new("--"); lbl_mwp_stn = gtk_label_new("--");
    gtk_grid_attach(GTK_GRID(mag_grid), l_mwp, 0, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(mag_grid), lbl_mwp_val, 1, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(mag_grid), lbl_mwp_stn, 2, 2, 1, 1);

    /* Fila 3: Ms */
    GtkWidget *l_ms = gtk_label_new(""); gtk_label_set_markup(GTK_LABEL(l_ms), "<span size='large'>Ms</span>");
    lbl_ms_val = gtk_label_new("--"); lbl_ms_stn = gtk_label_new("--");
    gtk_grid_attach(GTK_GRID(mag_grid), l_ms, 0, 3, 1, 1);
    gtk_grid_attach(GTK_GRID(mag_grid), lbl_ms_val, 1, 3, 1, 1);
    gtk_grid_attach(GTK_GRID(mag_grid), lbl_ms_stn, 2, 3, 1, 1);

    /* Fila 4: Mb */
    GtkWidget *l_mb = gtk_label_new(""); gtk_label_set_markup(GTK_LABEL(l_mb), "<span size='large'>Mb</span>");
    lbl_mb_val = gtk_label_new("--"); lbl_mb_stn = gtk_label_new("--");
    gtk_grid_attach(GTK_GRID(mag_grid), l_mb, 0, 4, 1, 1);
    gtk_grid_attach(GTK_GRID(mag_grid), lbl_mb_val, 1, 4, 1, 1);
    gtk_grid_attach(GTK_GRID(mag_grid), lbl_mb_stn, 2, 4, 1, 1);

    gtk_box_pack_start(GTK_BOX(vbox_main), mag_grid, FALSE, FALSE, 5);

    /* --- PANEL INFERIOR: MAPA GEOGRÁFICO INTERACTIVO --- */
    map_canvas = gtk_drawing_area_new();
    gtk_widget_set_size_request(map_canvas, 400, 400); 
    
    /* VINCULAR EVENTOS DE RATÓN (SCROLL, CLIC, ARRASTRE) AL LIENZO DEL MAPA */
    gtk_widget_add_events(map_canvas, GDK_SCROLL_MASK | GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_POINTER_MOTION_MASK);
    g_signal_connect(G_OBJECT(map_canvas), "scroll-event", G_CALLBACK(on_map_scroll), NULL);
    g_signal_connect(G_OBJECT(map_canvas), "button-press-event", G_CALLBACK(on_map_button_press), NULL);
    g_signal_connect(G_OBJECT(map_canvas), "button-release-event", G_CALLBACK(on_map_button_release), NULL);
    g_signal_connect(G_OBJECT(map_canvas), "motion-notify-event", G_CALLBACK(on_map_motion), NULL);
    
    g_signal_connect(G_OBJECT(map_canvas), "draw", G_CALLBACK(on_draw_map), NULL);
    gtk_box_pack_start(GTK_BOX(vbox_main), map_canvas, TRUE, TRUE, 10);

    g_timeout_add(1000, update_summary_loop, NULL);
    g_timeout_add(1000, ew_background_tasks, NULL);

    gtk_widget_show_all(window);
    gtk_main();

    tport_detach(&Region);
    if (g_map_pixbuf) g_object_unref(g_map_pixbuf);
    return 0;
}
