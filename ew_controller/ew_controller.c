#include <gtk/gtk.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <locale.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#include <earthworm.h>
#include <transport.h>
#include <kom.h>

#define MAX_STR 256
#define MAX_ROWS 256
#define STATUS_MAX 16384
#define LOG_TAIL 32768

/* --- CONFIGURACION --- */
char MyModName[MAX_STR] = "MOD_CONTROL";
char RingName[MAX_STR] = "CONTROL_RING";
char LogDir[MAX_STR] = "";
int  HeartBeatInt = 30;
int  LogFile = 1;
int  PollInt = 5;

pid_t MyPid;
time_t timeLastBeat = 0;

unsigned char TypeHeartBeat = 0;
unsigned char TypeReqStatus = 0;
unsigned char TypeStatus = 0;
unsigned char TypeStop = 0;
unsigned char TypeRestart = 0;
unsigned char TypeReconfig = 0;

unsigned char MyInstId = 0;
unsigned char MyModId = 0;
SHM_INFO Region;
long g_ring_key = -1;
int  g_attached = 0;

/* --- MODELO DE DATOS --- */
typedef struct {
   char  name[32];
   int   pid;
   char  status[16];
   char  detalle[192];
   char  config[64];
   char  cfgfile[64];
} MODROW;

typedef struct {
   char  name[64];
   int   key;
   int   size;
} RINGROW;

/* --- EDITOR DE CONFIGURACION --- */
typedef struct {
   int   kind;      /* 0=nombre/valor, 1=comentario, 2=en blanco */
   char *prefix;    /* espacios iniciales */
   char *name;      /* nombre de la variable */
   char *gap1;      /* espacios entre nombre y valor */
   char *value;     /* valor (conserva comillas originales) */
   char *gap2;      /* espacios entre valor y comentario */
   char *comment;   /* comentario desde '#' al final */
   char *raw;       /* linea original (para kind 1/2) */
} CFGLINE;

typedef struct {
   char     path[512];
   int      nlines;
   int      last_newline;  /* 1 si el archivo termina en '\n' */
   CFGLINE *lines;
} CFGFILE;

MODROW g_mods[MAX_ROWS];
int    g_nmods = 0;
RINGROW g_rings[MAX_ROWS];
int    g_nrings = 0;

char g_hostname[128] = "-";
char g_starttime[64] = "-";
char g_curtime[64] = "-";
char g_disk[64] = "-";
char g_version[64] = "-";

time_t g_last_status = 0;

/* --- GTK --- */
GtkWidget *g_tree, *g_rings_tree;
GtkListStore *g_store, *g_rings_store;
GtkWidget *g_btn_start, *g_btn_restart, *g_btn_stop, *g_btn_reconfig, *g_btn_refresh;
GtkWidget *g_lbl_header, *g_lbl_statusbar;
GtkWidget *g_combo, *g_logview;
GtkTextBuffer *g_logbuf;

/* pestaña Configuración */
GtkWidget *g_cfg_combo, *g_cfg_grid, *g_lbl_cfgpath, *g_lbl_cfg_status;
GtkWidget *g_btn_cfg_save, *g_btn_cfg_reconfig, *g_btn_cfg_reload;
CFGFILE g_cfg;
GtkWidget **g_cfg_entries;
int g_cfg_loaded = 0;
int g_cfg_modidx = -1;
int g_cfg_dirty = 0;
int g_cfg_suppress = 0;

int g_sel_idx = -1;
int g_sel_pid = -1;
time_t g_last_user_click = 0;
GtkWidget *g_window;

static void aplicar_botones(void);

/* ***********************************************************
 *  Ring I/O helpers                                          *
 * ***********************************************************/
static void enviar(unsigned char type, const char *payload)
{
   MSG_LOGO logo;
   char msg[512];
   logo.instid = MyInstId;
   logo.mod    = MyModId;
   logo.type   = type;
   strncpy(msg, payload, sizeof(msg) - 2);
   msg[sizeof(msg) - 2] = '\0';
   strcat(msg, "\n");
   if (tport_putmsg(&Region, &logo, strlen(msg), msg) != PUT_OK)
      logit("t", "ew_controller: Error enviando mensaje tipo %d al anillo.\n", (int) type);
}

void pedir_estado(void)
{
   enviar(TypeReqStatus, "?");
}

void detener_mod(int pid)
{
   char buf[32];
   snprintf(buf, sizeof(buf), "%d", pid);
   enviar(TypeStop, buf);
}

void reiniciar_mod(int pid)
{
   char buf[32];
   snprintf(buf, sizeof(buf), "%d", pid);
   enviar(TypeRestart, buf);
}

void reconfigurar(void)
{
   enviar(TypeReconfig, "");
}

/* ***********************************************************
 *  Config                                                    *
 * ***********************************************************/
int ReadConfig(char *configfile)
{
   int ncommand = 5, nmiss = 0, i;
   char init[8] = {0};
   char *com, *str;

   if (!k_open(configfile)) {
      fprintf(stderr, "ew_controller: Error abriendo config <%s>\n", configfile);
      return -1;
   }

   while (k_rd()) {
      com = k_str();
      if (!com || com[0] == '#') continue;

      if (k_its("MyModuleId")) { str = k_str(); if (str) strcpy(MyModName, str); init[0] = 1; }
      else if (k_its("Ring")) { str = k_str(); if (str) strcpy(RingName, str); init[1] = 1; }
      else if (k_its("HeartBeatInt")) { HeartBeatInt = k_int(); init[2] = 1; }
      else if (k_its("LogFile")) { LogFile = k_int(); init[3] = 1; }
      else if (k_its("PollInt")) { PollInt = k_int(); init[4] = 1; }
      else if (k_its("LogDir")) { str = k_str(); if (str) strcpy(LogDir, str); init[5] = 1; }
      else continue;

      if (k_err()) {
         fprintf(stderr, "ew_controller: Error parseando <%s> en <%s>\n", com, configfile);
         return -1;
      }
   }
   for (i = 0; i < ncommand; i++) if (!init[i]) nmiss++;
   k_close();
   if (nmiss > 0) {
      fprintf(stderr, "ew_controller: ERROR, faltan parametros en <%s>\n", configfile);
      return -1;
   }
   return 0;
}

void ConnectToEarthworm(void)
{
   long RingKey = GetKey(RingName);
   if (RingKey == -1) {
      fprintf(stderr, "ew_controller: Anillo <%s> no registrado en earthworm.d\n", RingName);
      exit(-1);
   }
   if (GetType("TYPE_HEARTBEAT", &TypeHeartBeat) != 0) TypeHeartBeat = 0;
   if (GetType("TYPE_REQSTATUS", &TypeReqStatus) != 0) TypeReqStatus = 0;
   if (GetType("TYPE_STATUS", &TypeStatus) != 0) TypeStatus = 0;
   if (GetType("TYPE_STOP", &TypeStop) != 0) TypeStop = 0;
   if (GetType("TYPE_RESTART", &TypeRestart) != 0) TypeRestart = 0;
   if (GetType("TYPE_RECONFIG", &TypeReconfig) != 0) TypeReconfig = 0;
   if (GetLocalInst(&MyInstId) != 0) MyInstId = 0;
   if (GetModId(MyModName, &MyModId) != 0) {
      if (GetModId("MOD_WILDCARD", &MyModId) != 0) MyModId = 0;
   }
   g_ring_key = RingKey;
}

/* Se invoca repetidamente hasta que startstop haya creado el anillo.
   Evita que el modulo crashee si se ejecuta antes que startstop. */
static gboolean on_try_attach(gpointer data)
{
   if (g_attached) return G_SOURCE_REMOVE;
   if (g_ring_key < 0 || shmget(g_ring_key, 0, 0) < 0) {
      gtk_label_set_text(GTK_LABEL(g_lbl_statusbar),
         "Esperando el anillo CONTROL_RING (arranca startstop)...");
      return G_SOURCE_CONTINUE;
   }
   tport_attach(&Region, g_ring_key);
   g_attached = 1;
   pedir_estado();
   gtk_label_set_text(GTK_LABEL(g_lbl_statusbar), "Conectado al anillo de control.");
   return G_SOURCE_REMOVE;
}

/* ***********************************************************
 *  Parseo del STATUS de startstop                            *
 * ***********************************************************/
static void campo_despues(char *line, const char *label, char *out, size_t n)
{
   char *c = strstr(line, label);
   if (!c) { out[0] = '\0'; return; }
   c = strchr(c, ':');
   if (!c) { out[0] = '\0'; return; }
   c++;
   while (*c == ' ') c++;
   strncpy(out, c, n - 1);
   out[n - 1] = '\0';
   char *nl = strchr(out, '\n');
   if (nl) *nl = '\0';
   char *e = out + strlen(out) - 1;
   while (e >= out && (*e == ' ' || *e == '\r')) { *e = '\0'; e--; }
}

static void extraer_config_de_args(char *detalle, char *config, size_t n)
{
   char tmp[192], *tok;
   config[0] = '\0';
   strncpy(tmp, detalle, sizeof(tmp) - 1);
   tmp[sizeof(tmp) - 1] = '\0';
   tok = strtok(tmp, " ");
   while (tok) {
      int len = strlen(tok);
      if (len > 3 && !strcmp(&tok[len - 2], ".d")) {
         /* base del log = config sin extension (igual que get_prog_name2) */
         char *slash = strrchr(tok, '/');
         if (slash) tok = slash + 1;
         char *dot = strchr(tok, '.');
         if (dot) *dot = '\0';
         strncpy(config, tok, n - 1);
      }
      tok = strtok(NULL, " ");
   }
   config[n - 1] = '\0';
}

static void extraer_cfgfile_de_args(char *detalle, char *cfgfile, size_t n)
{
   char tmp[192], *tok;
   cfgfile[0] = '\0';
   strncpy(tmp, detalle, sizeof(tmp) - 1);
   tmp[sizeof(tmp) - 1] = '\0';
   tok = strtok(tmp, " ");
   while (tok) {
      int len = strlen(tok);
      if (len > 3 && !strcmp(&tok[len - 2], ".d")) {
         char *slash = strrchr(tok, '/');
         if (slash) tok = slash + 1;
         strncpy(cfgfile, tok, n - 1);
      }
      tok = strtok(NULL, " ");
   }
   cfgfile[n - 1] = '\0';
}

void parse_status(char *buf)
{
   char *save = NULL;
   char *line;

   g_nmods = 0;
   g_nrings = 0;

   for (line = strtok_r(buf, "\n", &save); line; line = strtok_r(NULL, "\n", &save)) {
      char name[32], status[16], rest[192];

      if (strstr(line, "name/key/size")) {
         char *v = strchr(line, ':');
         if (v) {
            RINGROW r;
            if (sscanf(v + 1, "%63s / %d / %d", r.name, &r.key, &r.size) == 3) {
               r.name[sizeof(r.name) - 1] = '\0';
               if (g_nrings < MAX_ROWS) g_rings[g_nrings++] = r;
            }
         }
         continue;
      }
      if (strstr(line, "Hostname-OS")) { campo_despues(line, "Hostname-OS", g_hostname, sizeof(g_hostname)); continue; }
      if (strstr(line, "Start time (UTC)")) { campo_despues(line, "Start time (UTC)", g_starttime, sizeof(g_starttime)); continue; }
      if (strstr(line, "Current time (UTC)")) { campo_despues(line, "Current time (UTC)", g_curtime, sizeof(g_curtime)); continue; }
      if (strstr(line, "Disk space avail")) { campo_despues(line, "Disk space avail", g_disk, sizeof(g_disk)); continue; }
      if (strstr(line, "Stopstart Version")) { campo_despues(line, "Stopstart Version", g_version, sizeof(g_version)); continue; }
      if (strstr(line, "Process  Process") || strstr(line, "Name      Id")) continue;
      if (strstr(line, "-------")) continue;

      if (g_nmods >= MAX_ROWS) break;
      if (sscanf(line, "%31s %d %15s", name, &g_mods[g_nmods].pid, status) == 3) {
         if (g_mods[g_nmods].pid <= 0) continue;
         strncpy(g_mods[g_nmods].name, name, 31);
         g_mods[g_nmods].name[31] = '\0';
         strncpy(g_mods[g_nmods].status, status, 15);
         g_mods[g_nmods].status[15] = '\0';

         /* resto de la linea = detalle */
         char *reststart = strstr(line, status) + strlen(status);
         while (*reststart == ' ') reststart++;
         strncpy(rest, reststart, sizeof(rest) - 1);
         rest[sizeof(rest) - 1] = '\0';
         strncpy(g_mods[g_nmods].detalle, rest, sizeof(g_mods[g_nmods].detalle) - 1);
         g_mods[g_nmods].detalle[sizeof(g_mods[g_nmods].detalle) - 1] = '\0';

         extraer_config_de_args(rest, g_mods[g_nmods].config, sizeof(g_mods[g_nmods].config));
         if (g_mods[g_nmods].config[0] == '\0') {
            /* si no encontro archivo .d, usar el nombre del proceso */
            strncpy(g_mods[g_nmods].config, g_mods[g_nmods].name, sizeof(g_mods[g_nmods].config) - 1);
         }

         extraer_cfgfile_de_args(rest, g_mods[g_nmods].cfgfile, sizeof(g_mods[g_nmods].cfgfile));
         if (g_mods[g_nmods].cfgfile[0] == '\0') {
            /* fallback: derivar del nombre del proceso */
            strncpy(g_mods[g_nmods].cfgfile, g_mods[g_nmods].name, sizeof(g_mods[g_nmods].cfgfile) - 1);
            strncat(g_mods[g_nmods].cfgfile, ".d", sizeof(g_mods[g_nmods].cfgfile) - 1);
            g_mods[g_nmods].cfgfile[sizeof(g_mods[g_nmods].cfgfile) - 1] = '\0';
         }
         if (!strcmp(g_mods[g_nmods].name, "startstop"))
            strncpy(g_mods[g_nmods].cfgfile, "startstop_unix.d", sizeof(g_mods[g_nmods].cfgfile) - 1);
         g_nmods++;
      }
   }
}

/* ***********************************************************
 *  Editor de configuracion: parser/escritor de archivos .d   *
 * ***********************************************************/
static void cfg_libre(CFGFILE *cf)
{
   int i;
   if (!cf) return;
   for (i = 0; i < cf->nlines; i++) {
      CFGLINE *l = &cf->lines[i];
      free(l->prefix); free(l->name); free(l->gap1); free(l->value);
      free(l->gap2); free(l->comment); free(l->raw);
   }
   free(cf->lines);
   cf->lines = NULL;
   cf->nlines = 0;
}

/* divide una linea "nombre valor # comentario" en campos */
static void cfg_parse_linea(char *line, CFGLINE *l)
{
   char *p = line, *q;
   char *name, *vstart, *vaux;
   int in_q = 0;
   char *hash = NULL;

   memset(l, 0, sizeof(*l));
   l->prefix = strdup(p);
   l->prefix[0] = '\0';

   /* prefix */
   p = line;
   while (*p == ' ' || *p == '\t') p++;
   strncpy(l->prefix, line, p - line);
   l->prefix[p - line] = '\0';

   /* linea en blanco o comentario */
   if (*p == '\0') { l->kind = 2; l->raw = strdup(line); return; }
   if (*p == '#')  { l->kind = 1; l->raw = strdup(line); return; }

   /* nombre = primer token */
   name = p;
   while (*p && *p != ' ' && *p != '\t') p++;
   l->name = malloc(p - name + 1);
   memcpy(l->name, name, p - name);
   l->name[p - name] = '\0';

   /* gap1 */
   q = p;
   while (*q == ' ' || *q == '\t') q++;
   l->gap1 = malloc(q - p + 1);
   memcpy(l->gap1, p, q - p);
   l->gap1[q - p] = '\0';

   /* valor = hasta el primer '#' fuera de comillas */
   vstart = q;
   vaux = q;
   while (*vaux) {
      if (*vaux == '"') in_q = !in_q;
      else if (*vaux == '#' && !in_q) { hash = vaux; break; }
      vaux++;
   }
   if (hash) {
      /* gap2 = espacios entre valor y '#', comment = desde '#' */
      q = hash;
      while (q > vstart && (q[-1] == ' ' || q[-1] == '\t')) q--;
      l->value = malloc(q - vstart + 1);
      memcpy(l->value, vstart, q - vstart);
      l->value[q - vstart] = '\0';
      l->gap2 = malloc(hash - q + 1);
      memcpy(l->gap2, q, hash - q);
      l->gap2[hash - q] = '\0';
      /* comentario completo (incluye espacios/tabs finales) */
      l->comment = strdup(hash);
   } else {
      char *vend = vstart + strlen(vstart);
      char *ve = vend;
      while (ve > vstart && (ve[-1] == ' ' || ve[-1] == '\t')) ve--;
      l->value = malloc(ve - vstart + 1);
      memcpy(l->value, vstart, ve - vstart);
      l->value[ve - vstart] = '\0';
      /* espacios finales de la linea se conservan en gap2 */
      l->gap2 = malloc(vend - ve + 1);
      memcpy(l->gap2, ve, vend - ve);
      l->gap2[vend - ve] = '\0';
      l->comment = strdup("");
   }
   l->kind = 0;
}

/* lee un archivo .d y construye el CFGFILE */
static int cfg_leer(const char *path, CFGFILE *cf)
{
   FILE *f = fopen(path, "r");
   char line[1024];
   int cap = 0;
   int last_nl = 1;

   if (!f) return -1;
   memset(cf, 0, sizeof(*cf));
   snprintf(cf->path, sizeof(cf->path), "%s", path);

   while (fgets(line, sizeof(line), f)) {
      char *nl = strchr(line, '\n');
      last_nl = (nl != NULL);
      if (nl) *nl = '\0';
      if (cf->nlines == cap) {
         cap = cap ? cap * 2 : 32;
         CFGLINE *n = realloc(cf->lines, cap * sizeof(CFGLINE));
         if (!n) { fclose(f); cfg_libre(cf); return -1; }
         cf->lines = n;
      }
      cfg_parse_linea(line, &cf->lines[cf->nlines]);
      cf->nlines++;
   }
   cf->last_newline = last_nl;
   fclose(f);
   return 0;
}

static int copiar_archivo(const char *src, const char *dst)
{
   FILE *in = fopen(src, "rb"), *out;
   char buf[4096];
   size_t n;
   if (!in) return -1;
   out = fopen(dst, "wb");
   if (!out) { fclose(in); return -1; }
   while ((n = fread(buf, 1, sizeof(buf), in)) > 0)
      fwrite(buf, 1, n, out);
   fclose(in);
   fclose(out);
   return 0;
}

/* guarda con backup .bak y escritura atomica (tmp + rename) */
static int cfg_guardar(CFGFILE *cf)
{
   char bak[600], tmp[600];
   FILE *f;
   int i;

   snprintf(bak, sizeof(bak), "%s.bak", cf->path);
   if (copiar_archivo(cf->path, bak) != 0) return -1;

   snprintf(tmp, sizeof(tmp), "%s.tmp", cf->path);
   f = fopen(tmp, "w");
   if (!f) return -1;
   for (i = 0; i < cf->nlines; i++) {
      CFGLINE *l = &cf->lines[i];
      if (l->kind == 0) {
         fputs(l->prefix, f);
         fputs(l->name, f);
         fputs(l->gap1, f);
         fputs(l->value, f);
         fputs(l->gap2, f);
         fputs(l->comment, f);
      } else {
         fputs(l->raw, f);
      }
      if (!(i == cf->nlines - 1 && !cf->last_newline))
         fputc('\n', f);
   }
   if (fclose(f) != 0) return -1;
   if (rename(tmp, cf->path) != 0) return -1;
   return 0;
}

static void ruta_config(const char *cfgfile, char *out, size_t n)
{
   const char *ep = getenv("EW_PARAMS");
   if (cfgfile[0] == '/') { snprintf(out, n, "%s", cfgfile); return; }
   if (ep && *ep) snprintf(out, n, "%s/%s", ep, cfgfile);
   else snprintf(out, n, "%s", cfgfile);
}

/* ***********************************************************
 *  GTK: poblar listas                                        *
 * ***********************************************************/
void poblar_listas(void)
{
   GtkTreeIter iter;
   int i;

   /* conservar el modulo objetivo (por PID) a traves del refresh */
   if (g_sel_idx >= 0 && g_sel_idx < g_nmods) g_sel_pid = g_mods[g_sel_idx].pid;
   int intent_pid = g_sel_pid;

   gtk_list_store_clear(g_store);
   for (i = 0; i < g_nmods; i++) {
      gtk_list_store_append(g_store, &iter);
      gtk_list_store_set(g_store, &iter,
         0, g_mods[i].name,
         1, g_mods[i].pid,
         2, g_mods[i].status,
         3, g_mods[i].detalle,
         -1);
   }

   gtk_list_store_clear(g_rings_store);
   for (i = 0; i < g_nrings; i++) {
      gtk_list_store_append(g_rings_store, &iter);
      gtk_list_store_set(g_rings_store, &iter, 0, g_rings[i].name, 1, g_rings[i].key, 2, g_rings[i].size, -1);
   }

   /* re-ubicar el modulo objetivo en la nueva lista */
   g_sel_idx = -1;
   for (i = 0; i < g_nmods; i++)
      if (g_mods[i].pid == intent_pid) { g_sel_idx = i; break; }

   /* restaurar la seleccion visual solo si el usuario no acaba de interactuar,
      para no "pelearle" el clic cada vez que llega un refresco */
   if (g_sel_idx >= 0 && time(NULL) - g_last_user_click > 3) {
      gboolean ok = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(g_store), &iter);
      while (ok) {
         int pid;
         gtk_tree_model_get(GTK_TREE_MODEL(g_store), &iter, 1, &pid, -1);
         if (pid == intent_pid) {
            GtkTreeSelection *sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(g_tree));
            gtk_tree_selection_select_iter(sel, &iter);
            break;
         }
         ok = gtk_tree_model_iter_next(GTK_TREE_MODEL(g_store), &iter);
      }
   }
   aplicar_botones();

   char hdr[512];
   snprintf(hdr, sizeof(hdr),
            "EARTHWORM SYSTEM STATUS\n%s   Inicio: %s   Actual: %s   Disco: %s   Version: %s",
            g_hostname, g_starttime, g_curtime, g_disk, g_version);
   gtk_label_set_text(GTK_LABEL(g_lbl_header), hdr);
}

/* ***********************************************************
 *  Acciones GTK                                              *
 * ***********************************************************/
/* Habilita/deshabilita los botones segun el modulo objetivo (g_sel_idx). */
static void aplicar_botones(void)
{
   gboolean sel_ok = (g_sel_idx >= 0 && g_sel_idx < g_nmods);
   gboolean es_startstop = sel_ok && !strcmp(g_mods[g_sel_idx].name, "startstop");
   const char *st = sel_ok ? g_mods[g_sel_idx].status : "";

   gtk_widget_set_sensitive(g_btn_stop, sel_ok && !es_startstop && !strcmp(st, "Alive"));
   gtk_widget_set_sensitive(g_btn_restart, sel_ok && !es_startstop);
   gtk_widget_set_sensitive(g_btn_start, sel_ok && !es_startstop &&
                            (!strcmp(st, "Stop") || !strcmp(st, "Dead")));
}

static void on_row_selected(GtkTreeSelection *sel, gpointer data)
{
   GtkTreeModel *model;
   GtkTreeIter iter;
   gboolean sel_ok;

   sel_ok = gtk_tree_selection_get_selected(sel, &model, &iter);
   g_sel_idx = -1;
   if (sel_ok) {
      gint pid = -1;
      gtk_tree_model_get(model, &iter, 1, &pid, -1);
      for (int i = 0; i < g_nmods; i++)
         if (g_mods[i].pid == pid) { g_sel_idx = i; break; }
      if (g_sel_idx >= 0) g_sel_pid = g_mods[g_sel_idx].pid;
   }
   aplicar_botones();

   /* sincronizar la seleccion con el combo de la pestaña Configuración */
   if (g_cfg_combo && g_sel_idx >= 0 && g_sel_idx < g_nmods) {
      gint cur = gtk_combo_box_get_active(GTK_COMBO_BOX(g_cfg_combo));
      if (cur != g_sel_idx)
         gtk_combo_box_set_active(GTK_COMBO_BOX(g_cfg_combo), g_sel_idx);
   }
}

static gboolean on_tree_button_press(GtkWidget *w, GdkEventButton *ev, gpointer data)
{
   time(&g_last_user_click);
   return FALSE;
}

static gboolean confirmar(const char *msg)
{
   GtkWidget *dlg = gtk_message_dialog_new(GTK_WINDOW(g_window), GTK_DIALOG_MODAL,
                     GTK_MESSAGE_WARNING, GTK_BUTTONS_YES_NO, "%s", msg);
   gint r = gtk_dialog_run(GTK_DIALOG(dlg));
   gtk_widget_destroy(dlg);
   return r == GTK_RESPONSE_YES;
}

static void on_btn_stop(GtkWidget *w, gpointer data)
{
   if (g_sel_idx >= 0) {
      char msg[256];
      snprintf(msg, sizeof(msg), "Detener el modulo '%s' (pid %d)?",
               g_mods[g_sel_idx].name, g_mods[g_sel_idx].pid);
      if (!confirmar(msg)) return;
      detener_mod(g_mods[g_sel_idx].pid);
      gtk_label_set_text(GTK_LABEL(g_lbl_statusbar), "Stop enviado...");
      pedir_estado();
   }
}

static void on_btn_restart(GtkWidget *w, gpointer data)
{
   if (g_sel_idx >= 0) {
      char msg[256];
      snprintf(msg, sizeof(msg), "Reiniciar el modulo '%s' (pid %d)?",
               g_mods[g_sel_idx].name, g_mods[g_sel_idx].pid);
      if (!confirmar(msg)) return;
      reiniciar_mod(g_mods[g_sel_idx].pid);
      gtk_label_set_text(GTK_LABEL(g_lbl_statusbar), "Restart enviado...");
      pedir_estado();
   }
}

static void on_btn_start(GtkWidget *w, gpointer data)
{
   if (g_sel_idx >= 0) {
      char msg[256];
      snprintf(msg, sizeof(msg), "Arrancar el modulo '%s'?",
               g_mods[g_sel_idx].name);
      if (!confirmar(msg)) return;
      reiniciar_mod(g_mods[g_sel_idx].pid);
      gtk_label_set_text(GTK_LABEL(g_lbl_statusbar), "Start enviado...");
      pedir_estado();
   }
}

static void on_btn_reconfig(GtkWidget *w, gpointer data)
{
   reconfigurar();
   gtk_label_set_text(GTK_LABEL(g_lbl_statusbar), "Reconfig enviado...");
   pedir_estado();
}

static void on_btn_refresh(GtkWidget *w, gpointer data)
{
   pedir_estado();
   gtk_label_set_text(GTK_LABEL(g_lbl_statusbar), "Actualizando...");
}

/* ***********************************************************
 *  Visor de logs                                             *
 * ***********************************************************/
static void populate_combo(void)
{
   gchar *sel = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(g_combo));
   gtk_combo_box_text_remove_all(GTK_COMBO_BOX_TEXT(g_combo));
   for (int i = 0; i < g_nmods; i++)
      gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(g_combo), g_mods[i].name);
   if (sel) {
      int idx = -1;
      for (int i = 0; i < g_nmods; i++)
         if (!strcmp(g_mods[i].name, sel)) { idx = i; break; }
      gtk_combo_box_set_active(GTK_COMBO_BOX(g_combo), idx);
      g_free(sel);
   } else if (g_nmods > 0) {
      gtk_combo_box_set_active(GTK_COMBO_BOX(g_combo), 0);
   }

   if (!g_cfg_combo) return;
   /* combo de configuracion: misma poblacion; el handler evita recargar
      si el modulo seleccionado no cambio */
   gchar *csel = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(g_cfg_combo));
   gtk_combo_box_text_remove_all(GTK_COMBO_BOX_TEXT(g_cfg_combo));
   for (int i = 0; i < g_nmods; i++)
      gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(g_cfg_combo), g_mods[i].name);
   if (csel) {
      int idx = -1;
      for (int i = 0; i < g_nmods; i++)
         if (!strcmp(g_mods[i].name, csel)) { idx = i; break; }
      gtk_combo_box_set_active(GTK_COMBO_BOX(g_cfg_combo), idx);
      g_free(csel);
   } else if (g_nmods > 0) {
      gtk_combo_box_set_active(GTK_COMBO_BOX(g_cfg_combo), 0);
   }
}

static char *tail_file(const char *path, long maxbytes)
{
   FILE *fp = fopen(path, "rb");
   if (!fp) return NULL;
   fseek(fp, 0, SEEK_END);
   long sz = ftell(fp);
   long start = (sz > maxbytes) ? sz - maxbytes : 0;
   fseek(fp, start, SEEK_SET);
   char *buf = malloc(maxbytes + 2);
   if (!buf) { fclose(fp); return NULL; }
   long n = fread(buf, 1, maxbytes, fp);
   buf[n] = '\0';
   fclose(fp);
   if (n == 0) return buf;
   /* empezar en un limite de linea */
   char *nl = strchr(buf, '\n');
   if (nl && nl != buf) memmove(buf, nl + 1, strlen(nl + 1) + 1);
   return buf;
}

static void actualizar_log(void)
{
   gchar *sel = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(g_combo));
   if (!sel || !*sel) return;
   char base[64];
   strncpy(base, sel, sizeof(base) - 1);
   base[sizeof(base) - 1] = '\0';
   g_free(sel);

   /* startstop nombra su log con el nombre del ejecutable (argv[0]),
      no con el archivo de config (hace un segundo logit_init con argv[0]) */
   if (!strcmp(base, "startstop")) strcpy(base, "startstop");
   else {
      int k = -1;
      for (int i = 0; i < g_nmods; i++)
         if (!strcmp(g_mods[i].name, base)) { k = i; break; }
      if (k >= 0 && g_mods[k].config[0])
         strncpy(base, g_mods[k].config, sizeof(base) - 1);
      base[sizeof(base) - 1] = '\0';
   }

   DIR *d = opendir(LogDir);
   if (!d) {
      char msg[512];
      snprintf(msg, sizeof(msg), "(LogDir no accesible: %s)", LogDir);
      gtk_text_buffer_set_text(g_logbuf, msg, -1);
      return;
   }
   char best[512] = "";
   long best_mtime = -1;
   struct dirent *e;
   char prefix[128];
   snprintf(prefix, sizeof(prefix), "%s_", base);
   while ((e = readdir(d)) != NULL) {
      if (strncmp(e->d_name, prefix, strlen(prefix)) != 0) continue;
      if (!strstr(e->d_name, ".log")) continue;
      char path[512];
      snprintf(path, sizeof(path), "%s/%s", LogDir, e->d_name);
      struct stat st;
      if (stat(path, &st) != 0) continue;
      if (st.st_mtime > best_mtime) { best_mtime = st.st_mtime; strcpy(best, path); }
   }
   closedir(d);
   if (best[0] == '\0') {
      gtk_text_buffer_set_text(g_logbuf, "(sin archivo de log)", -1);
      return;
   }
   char *txt = tail_file(best, LOG_TAIL);
   gtk_text_buffer_set_text(g_logbuf, txt ? txt : "(no se pudo leer)", -1);
   if (txt) free(txt);
}

static void on_combo_changed(GtkComboBox *combo, gpointer data)
{
   actualizar_log();
}

/* ***********************************************************
 *  Pestaña Configuración                                     *
 * ***********************************************************/
static void actualizar_botones_cfg(void)
{
   gboolean ok = (g_cfg_loaded == 1);
   gtk_widget_set_sensitive(g_btn_cfg_save, ok);
   gtk_widget_set_sensitive(g_btn_cfg_reconfig, ok);
   gtk_widget_set_sensitive(g_btn_cfg_reload, ok);
}

static void on_cfg_entry_changed(GtkWidget *entry, gpointer data)
{
   g_cfg_dirty = 1;
   gtk_label_set_text(GTK_LABEL(g_lbl_cfg_status), "Hay cambios sin guardar...");
}

static void cfg_vaciar_grid(void)
{
   GList *ch, *l;
   ch = gtk_container_get_children(GTK_CONTAINER(g_cfg_grid));
   for (l = ch; l; l = l->next)
      gtk_widget_destroy(GTK_WIDGET(l->data));
   g_list_free(ch);
}

static void cfg_rebuild_grid(void)
{
   int i, r = 0;
   cfg_vaciar_grid();
   if (g_cfg_entries) { free(g_cfg_entries); g_cfg_entries = NULL; }
   g_cfg_entries = calloc(g_cfg.nlines + 1, sizeof(GtkWidget *));

   for (i = 0; i < g_cfg.nlines; i++) {
      CFGLINE *l = &g_cfg.lines[i];
      g_cfg_entries[i] = NULL;
      if (l->kind != 0) continue;

      gchar *mark = g_markup_printf_escaped("<b>%s</b>", l->name);
      GtkWidget *lbl = gtk_label_new(NULL);
      gtk_label_set_markup(GTK_LABEL(lbl), mark);
      g_free(mark);
      gtk_label_set_xalign(GTK_LABEL(lbl), 1.0);
      gtk_widget_set_hexpand(lbl, FALSE);

      GtkWidget *entry = gtk_entry_new();
      gtk_entry_set_text(GTK_ENTRY(entry), l->value);
      gtk_widget_set_hexpand(entry, TRUE);

      gtk_grid_attach(GTK_GRID(g_cfg_grid), lbl, 0, r, 1, 1);
      gtk_grid_attach(GTK_GRID(g_cfg_grid), entry, 1, r, 1, 1);
      g_cfg_entries[i] = entry;
      g_signal_connect(entry, "changed", G_CALLBACK(on_cfg_entry_changed), NULL);
      r++;
   }
   gtk_widget_show_all(g_cfg_grid);
}

static void cfg_cargar_modulo(int idx)
{
   char path[512];
   char msg[512];
   int nedit = 0, i;

   if (idx < 0 || idx >= g_nmods) return;
   ruta_config(g_mods[idx].cfgfile, path, sizeof(path));

   cfg_libre(&g_cfg);
   if (cfg_leer(path, &g_cfg) != 0) {
      g_cfg_loaded = 0;
      gtk_label_set_text(GTK_LABEL(g_lbl_cfgpath), path);
      snprintf(msg, sizeof(msg), "No se pudo leer el archivo de config: %s", path);
      gtk_label_set_text(GTK_LABEL(g_lbl_cfg_status), msg);
      cfg_rebuild_grid();
      actualizar_botones_cfg();
      return;
   }

   g_cfg_loaded = 1;
   g_cfg_modidx = idx;
   g_cfg_dirty = 0;
   for (i = 0; i < g_cfg.nlines; i++)
      if (g_cfg.lines[i].kind == 0) nedit++;
   gtk_label_set_text(GTK_LABEL(g_lbl_cfgpath), path);
   snprintf(msg, sizeof(msg), "Cargado: %s  (%d variables editables)", path, nedit);
   gtk_label_set_text(GTK_LABEL(g_lbl_cfg_status), msg);
   cfg_rebuild_grid();
   actualizar_botones_cfg();
}

static void on_cfg_combo_changed(GtkComboBox *combo, gpointer data)
{
   gint idx;
   if (g_cfg_suppress) return;
   idx = gtk_combo_box_get_active(GTK_COMBO_BOX(combo));
   if (idx < 0) return;

   /* mismo archivo (aunque cambie la posicion en la lista): no recargar */
   if (g_cfg_loaded && g_cfg_modidx >= 0 && g_cfg_modidx < g_nmods &&
       idx < g_nmods && !strcmp(g_mods[g_cfg_modidx].cfgfile, g_mods[idx].cfgfile)) {
      g_cfg_modidx = idx;
      return;
   }

   if (g_cfg_loaded && g_cfg_dirty) {
      char msg[300];
      snprintf(msg, sizeof(msg), "Hay cambios sin guardar en '%s'. ¿Descartarlos?",
               (g_cfg_modidx >= 0 && g_cfg_modidx < g_nmods) ? g_mods[g_cfg_modidx].name : "?");
      if (!confirmar(msg)) {
         g_cfg_suppress = 1;
         gtk_combo_box_set_active(GTK_COMBO_BOX(combo), g_cfg_modidx);
         g_cfg_suppress = 0;
         return;
      }
   }
   cfg_cargar_modulo(idx);
}

static void on_btn_cfg_save(GtkWidget *w, gpointer data)
{
   char msg[512];
   int i;

   if (!g_cfg_loaded) return;
   /* volcar las entradas al modelo */
   for (i = 0; i < g_cfg.nlines; i++) {
      if (!g_cfg_entries[i]) continue;
      const gchar *txt = gtk_entry_get_text(GTK_ENTRY(g_cfg_entries[i]));
      free(g_cfg.lines[i].value);
      g_cfg.lines[i].value = g_strdup(txt);
   }
   if (cfg_guardar(&g_cfg) == 0) {
      g_cfg_dirty = 0;
      snprintf(msg, sizeof(msg), "Guardado en %s (backup .bak)", g_cfg.path);
      gtk_label_set_text(GTK_LABEL(g_lbl_cfg_status), msg);
   } else {
      snprintf(msg, sizeof(msg), "ERROR al guardar %s", g_cfg.path);
      gtk_label_set_text(GTK_LABEL(g_lbl_cfg_status), msg);
   }
}

static void on_btn_cfg_reconfig(GtkWidget *w, gpointer data)
{
   char msg[300];
   if (!g_cfg_loaded || g_cfg_modidx < 0) return;
   on_btn_cfg_save(w, data);
   if (g_cfg_dirty) return;   /* no se pudo guardar */
   snprintf(msg, sizeof(msg), "Reiniciar '%s' para aplicar la config guardada?",
            g_mods[g_cfg_modidx].name);
   if (!confirmar(msg)) return;
   if (!strcmp(g_mods[g_cfg_modidx].name, "startstop")) {
      reconfigurar();
      gtk_label_set_text(GTK_LABEL(g_lbl_statusbar), "Reconfig (startstop) enviado...");
      gtk_label_set_text(GTK_LABEL(g_lbl_cfg_status), "startstop re-lee su config y aplica los cambios.");
   } else {
      reiniciar_mod(g_mods[g_cfg_modidx].pid);
      gtk_label_set_text(GTK_LABEL(g_lbl_statusbar), "Restart enviado...");
      gtk_label_set_text(GTK_LABEL(g_lbl_cfg_status),
         "Config guardada. El modulo relee su config al reiniciar.");
   }
   pedir_estado();
}

static void on_btn_cfg_reload(GtkWidget *w, gpointer data)
{
   if (!g_cfg_loaded) return;
   if (g_cfg_dirty && !confirmar("Hay cambios sin guardar. ¿Recargar el archivo y descartarlos?"))
      return;
   cfg_cargar_modulo(g_cfg_modidx);
}

/* ***********************************************************
 *  Bucles de fondo (GTK main loop)                           *
 * ***********************************************************/
static gboolean on_poll(gpointer data)
{
   if (!g_attached) return G_SOURCE_CONTINUE;
   time_t now;
   time(&now);
   if (now - timeLastBeat >= HeartBeatInt) {
      timeLastBeat = now;
      enviar(TypeHeartBeat, "");
   }
   pedir_estado();
   /* si llevamos demasiado sin respuesta */
   if (g_last_status && (now - g_last_status) > (time_t)(PollInt * 4)) {
      gtk_label_set_text(GTK_LABEL(g_lbl_statusbar), "SIN RESPUESTA de startstop");
      gtk_widget_set_sensitive(g_btn_stop, FALSE);
      gtk_widget_set_sensitive(g_btn_restart, FALSE);
      gtk_widget_set_sensitive(g_btn_start, FALSE);
      gtk_widget_set_sensitive(g_btn_reconfig, FALSE);
   }
   return G_SOURCE_CONTINUE;
}

static gboolean on_read(gpointer data)
{
   MSG_LOGO reclogo;
   char msg[STATUS_MAX + 1];
   long recsize;
   int res;
   MSG_LOGO filtro[1];
   unsigned char wc_inst, wc_mod;
   static int got_inst = 0, got_mod = 0;

   if (!g_attached) return G_SOURCE_CONTINUE;

   if (!got_inst) { if (GetInst("INST_WILDCARD", &wc_inst) == 0) got_inst = 1; else wc_inst = 0; }
   if (!got_mod) { if (GetModId("MOD_WILDCARD", &wc_mod) == 0) got_mod = 1; else wc_mod = 0; }
   filtro[0].instid = wc_inst;
   filtro[0].mod = wc_mod;
   filtro[0].type = TypeStatus;

   do {
      res = tport_getmsg(&Region, filtro, 1, &reclogo, &recsize, msg, sizeof(msg) - 1);
      if (res == GET_OK || res == GET_MISS || res == GET_NOTRACK || res == GET_MISS_SEQGAP) {
         msg[recsize] = '\0';
         parse_status(msg);
         poblar_listas();
         populate_combo();
         time(&g_last_status);
         char sb[256];
         if (g_sel_idx >= 0 && g_sel_idx < g_nmods)
            snprintf(sb, sizeof(sb), "Actualizado %s UTC - %d modulos - objetivo: %s (%s)",
                     g_curtime, g_nmods, g_mods[g_sel_idx].name, g_mods[g_sel_idx].status);
         else
            snprintf(sb, sizeof(sb), "Actualizado %s UTC - %d modulos", g_curtime, g_nmods);
         gtk_label_set_text(GTK_LABEL(g_lbl_statusbar), sb);
         gtk_widget_set_sensitive(g_btn_reconfig, TRUE);
         actualizar_log();
      }
   } while (res == GET_OK);

   return G_SOURCE_CONTINUE;
}

/* ***********************************************************
 *  Main                                                      *
 * ***********************************************************/
int main(int argc, char *argv[])
{
   if (argc != 2) {
      fprintf(stderr, "Usage: %s <configfile.d>\n", argv[0]);
      exit(1);
   }
   if (ReadConfig(argv[1]) != 0) exit(1);
   /* LogDir: prioridad EW_LOG (variable de ambiente) > config > "logs" */
   {
      const char *ewlog = getenv("EW_LOG");
      if (ewlog && *ewlog)
         strncpy(LogDir, ewlog, sizeof(LogDir) - 1);
      else if (LogDir[0] == '\0')
         strncpy(LogDir, "logs", sizeof(LogDir) - 1);
      LogDir[sizeof(LogDir) - 1] = '\0';
   }

   logit_init(argv[1], 0, 1024, LogFile);
   MyPid = getpid();

   gtk_init(&argc, &argv);
   setlocale(LC_NUMERIC, "C");

   ConnectToEarthworm();

   GtkCssProvider *provider = gtk_css_provider_new();
   gtk_css_provider_load_from_data(provider,
      "treeview grid-line { border-color: #555555; }\n"
      "#btn_stop { background-image: none; background-color: #dc3545; color: #ffffff; font-weight: bold; padding: 5px 18px; border-radius: 4px; border: 1px solid #bd2130; }\n"
      "#btn_stop:hover { background-color: #c82333; }\n"
      "#btn_start { background-image: none; background-color: #28a745; color: #ffffff; font-weight: bold; padding: 5px 18px; border-radius: 4px; border: 1px solid #218838; }\n"
      "#btn_start:hover { background-color: #218838; }\n"
      "#btn_restart { background-image: none; background-color: #ffc107; color: #212529; font-weight: bold; padding: 5px 18px; border-radius: 4px; border: 1px solid #e0a800; }\n"
      "#btn_restart:hover { background-color: #e0a800; }\n"
      "#btn_reconfig { background-image: none; background-color: #17a2b8; color: #ffffff; font-weight: bold; padding: 5px 18px; border-radius: 4px; border: 1px solid #117a8b; }\n"
      "#btn_reconfig:hover { background-color: #138496; }\n"
      "#btn_refresh { background-image: none; background-color: #6c757d; color: #ffffff; font-weight: bold; padding: 5px 18px; border-radius: 4px; border: 1px solid #5a6268; }\n"
      "#btn_refresh:hover { background-color: #5a6268; }\n"
      "textview { font-family: monospace; font-size: 11px; }\n", -1, NULL);
   gtk_style_context_add_provider_for_screen(gdk_screen_get_default(),
       GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

   GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
   g_window = window;
   gtk_window_set_title(GTK_WINDOW(window), "Earthworm Controller");
   gtk_window_set_default_size(GTK_WINDOW(window), 980, 720);
   g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

   GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
   gtk_container_add(GTK_CONTAINER(window), vbox);

   g_lbl_header = gtk_label_new("");
   gtk_label_set_xalign(GTK_LABEL(g_lbl_header), 0.0);
   gtk_widget_set_margin_top(g_lbl_header, 6);
   gtk_widget_set_margin_bottom(g_lbl_header, 2);
   gtk_box_pack_start(GTK_BOX(vbox), g_lbl_header, FALSE, FALSE, 0);

   /* ---- Notebook: pestañas Módulos / Configuración ---- */
   GtkWidget *notebook = gtk_notebook_new();
   gtk_box_pack_start(GTK_BOX(vbox), notebook, TRUE, TRUE, 0);

   /* ================= Pestaña 1: MÓDULOS ================= */
   GtkWidget *page_mod = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
   gtk_notebook_append_page(GTK_NOTEBOOK(notebook), page_mod, gtk_label_new("Módulos"));

   GtkWidget *paned = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
   gtk_box_pack_start(GTK_BOX(page_mod), paned, TRUE, TRUE, 0);

   GtkWidget *box_mod = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
   gtk_paned_pack1(GTK_PANED(paned), box_mod, TRUE, FALSE);

   g_store = gtk_list_store_new(4, G_TYPE_STRING, G_TYPE_INT, G_TYPE_STRING, G_TYPE_STRING);
   g_tree = gtk_tree_view_new_with_model(GTK_TREE_MODEL(g_store));
   gtk_tree_view_set_grid_lines(GTK_TREE_VIEW(g_tree), GTK_TREE_VIEW_GRID_LINES_HORIZONTAL);

   GtkCellRenderer *renderer;
   GtkTreeViewColumn *col;
   GtkTreeSelection *sel;

   renderer = gtk_cell_renderer_text_new();
   col = gtk_tree_view_column_new_with_attributes("Modulo", renderer, "text", 0, NULL);
   gtk_tree_view_column_set_expand(GTK_TREE_VIEW_COLUMN(col), FALSE);
   gtk_tree_view_append_column(GTK_TREE_VIEW(g_tree), col);

   renderer = gtk_cell_renderer_text_new();
   col = gtk_tree_view_column_new_with_attributes("PID", renderer, "text", 1, NULL);
   gtk_tree_view_append_column(GTK_TREE_VIEW(g_tree), col);

   renderer = gtk_cell_renderer_text_new();
   col = gtk_tree_view_column_new_with_attributes("Estado", renderer, "text", 2, NULL);
   gtk_tree_view_append_column(GTK_TREE_VIEW(g_tree), col);

   renderer = gtk_cell_renderer_text_new();
   col = gtk_tree_view_column_new_with_attributes("Detalle", renderer, "text", 3, NULL);
   gtk_tree_view_column_set_expand(GTK_TREE_VIEW_COLUMN(col), TRUE);
   gtk_tree_view_append_column(GTK_TREE_VIEW(g_tree), col);

   GtkWidget *sw_mod = gtk_scrolled_window_new(NULL, NULL);
   gtk_widget_set_size_request(sw_mod, 620, 300);
   gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw_mod), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
   gtk_container_add(GTK_CONTAINER(sw_mod), g_tree);
   gtk_box_pack_start(GTK_BOX(box_mod), sw_mod, TRUE, TRUE, 0);

   /* botones */
   GtkWidget *hbtn = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
   gtk_box_pack_start(GTK_BOX(box_mod), hbtn, FALSE, FALSE, 0);

   g_btn_stop = gtk_button_new_with_label("Stop");
   gtk_widget_set_name(g_btn_stop, "btn_stop");
   g_signal_connect(g_btn_stop, "clicked", G_CALLBACK(on_btn_stop), NULL);
   gtk_box_pack_start(GTK_BOX(hbtn), g_btn_stop, FALSE, FALSE, 0);

   g_btn_restart = gtk_button_new_with_label("Restart");
   gtk_widget_set_name(g_btn_restart, "btn_restart");
   g_signal_connect(g_btn_restart, "clicked", G_CALLBACK(on_btn_restart), NULL);
   gtk_box_pack_start(GTK_BOX(hbtn), g_btn_restart, FALSE, FALSE, 0);

   g_btn_start = gtk_button_new_with_label("Start");
   gtk_widget_set_name(g_btn_start, "btn_start");
   g_signal_connect(g_btn_start, "clicked", G_CALLBACK(on_btn_start), NULL);
   gtk_box_pack_start(GTK_BOX(hbtn), g_btn_start, FALSE, FALSE, 0);

   g_btn_reconfig = gtk_button_new_with_label("Reconfig");
   gtk_widget_set_name(g_btn_reconfig, "btn_reconfig");
   g_signal_connect(g_btn_reconfig, "clicked", G_CALLBACK(on_btn_reconfig), NULL);
   gtk_box_pack_start(GTK_BOX(hbtn), g_btn_reconfig, FALSE, FALSE, 0);

   g_btn_refresh = gtk_button_new_with_label("Refresh");
   gtk_widget_set_name(g_btn_refresh, "btn_refresh");
   g_signal_connect(g_btn_refresh, "clicked", G_CALLBACK(on_btn_refresh), NULL);
   gtk_box_pack_start(GTK_BOX(hbtn), g_btn_refresh, FALSE, FALSE, 0);

   gtk_widget_set_sensitive(g_btn_stop, FALSE);
   gtk_widget_set_sensitive(g_btn_restart, FALSE);
   gtk_widget_set_sensitive(g_btn_start, FALSE);
   gtk_widget_set_sensitive(g_btn_reconfig, FALSE);

   sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(g_tree));
   g_signal_connect(sel, "changed", G_CALLBACK(on_row_selected), NULL);
   g_signal_connect(g_tree, "button-press-event", G_CALLBACK(on_tree_button_press), NULL);

   /* ---- Panel derecho: anillos + logs ---- */
   GtkWidget *box_der = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
   gtk_paned_pack2(GTK_PANED(paned), box_der, TRUE, FALSE);

   GtkWidget *lbl_rings = gtk_label_new("ANILLOS");
   gtk_label_set_xalign(GTK_LABEL(lbl_rings), 0.0);
   gtk_box_pack_start(GTK_BOX(box_der), lbl_rings, FALSE, FALSE, 0);

   g_rings_store = gtk_list_store_new(3, G_TYPE_STRING, G_TYPE_INT, G_TYPE_INT);
   g_rings_tree = gtk_tree_view_new_with_model(GTK_TREE_MODEL(g_rings_store));
   gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(g_rings_tree), TRUE);

   renderer = gtk_cell_renderer_text_new();
   col = gtk_tree_view_column_new_with_attributes("Ring", renderer, "text", 0, NULL);
   gtk_tree_view_append_column(GTK_TREE_VIEW(g_rings_tree), col);
   renderer = gtk_cell_renderer_text_new();
   col = gtk_tree_view_column_new_with_attributes("Key", renderer, "text", 1, NULL);
   gtk_tree_view_append_column(GTK_TREE_VIEW(g_rings_tree), col);
   renderer = gtk_cell_renderer_text_new();
   col = gtk_tree_view_column_new_with_attributes("Size KB", renderer, "text", 2, NULL);
   gtk_tree_view_append_column(GTK_TREE_VIEW(g_rings_tree), col);

   GtkWidget *sw_rings = gtk_scrolled_window_new(NULL, NULL);
   gtk_widget_set_size_request(sw_rings, 340, 120);
   gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw_rings), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
   gtk_container_add(GTK_CONTAINER(sw_rings), g_rings_tree);
   gtk_box_pack_start(GTK_BOX(box_der), sw_rings, FALSE, FALSE, 0);

   GtkWidget *lbl_log = gtk_label_new("LOGS (tail)");
   gtk_label_set_xalign(GTK_LABEL(lbl_log), 0.0);
   gtk_box_pack_start(GTK_BOX(box_der), lbl_log, FALSE, FALSE, 0);

   g_combo = gtk_combo_box_text_new();
   g_signal_connect(g_combo, "changed", G_CALLBACK(on_combo_changed), NULL);
   gtk_box_pack_start(GTK_BOX(box_der), g_combo, FALSE, FALSE, 0);

   g_logview = gtk_text_view_new();
   gtk_text_view_set_editable(GTK_TEXT_VIEW(g_logview), FALSE);
   gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(g_logview), GTK_WRAP_NONE);
   g_logbuf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(g_logview));
   GtkWidget *sw_log = gtk_scrolled_window_new(NULL, NULL);
   gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw_log), GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
   gtk_container_add(GTK_CONTAINER(sw_log), g_logview);
   gtk_box_pack_start(GTK_BOX(box_der), sw_log, TRUE, TRUE, 0);

   /* ================= Pestaña 2: CONFIGURACIÓN ================= */
   GtkWidget *page_cfg = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
   gtk_notebook_append_page(GTK_NOTEBOOK(notebook), page_cfg, gtk_label_new("Configuración"));

   GtkWidget *cfg_h = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
   gtk_box_pack_start(GTK_BOX(page_cfg), cfg_h, FALSE, FALSE, 0);
   GtkWidget *lbl_cfg_mod = gtk_label_new("Módulo:");
   gtk_box_pack_start(GTK_BOX(cfg_h), lbl_cfg_mod, FALSE, FALSE, 0);

   g_cfg_combo = gtk_combo_box_text_new();
   g_signal_connect(g_cfg_combo, "changed", G_CALLBACK(on_cfg_combo_changed), NULL);
   gtk_box_pack_start(GTK_BOX(cfg_h), g_cfg_combo, FALSE, FALSE, 0);

   g_lbl_cfgpath = gtk_label_new("");
   gtk_label_set_xalign(GTK_LABEL(g_lbl_cfgpath), 0.0);
   gtk_label_set_ellipsize(GTK_LABEL(g_lbl_cfgpath), PANGO_ELLIPSIZE_MIDDLE);
   gtk_box_pack_start(GTK_BOX(cfg_h), g_lbl_cfgpath, TRUE, TRUE, 0);

   g_cfg_grid = gtk_grid_new();
   gtk_grid_set_column_spacing(GTK_GRID(g_cfg_grid), 8);
   gtk_grid_set_row_spacing(GTK_GRID(g_cfg_grid), 4);
   GtkWidget *sw_cfg = gtk_scrolled_window_new(NULL, NULL);
   gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw_cfg), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
   gtk_container_add(GTK_CONTAINER(sw_cfg), g_cfg_grid);
   gtk_box_pack_start(GTK_BOX(page_cfg), sw_cfg, TRUE, TRUE, 0);

   GtkWidget *cfg_btn = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
   gtk_box_pack_start(GTK_BOX(page_cfg), cfg_btn, FALSE, FALSE, 0);

   g_btn_cfg_save = gtk_button_new_with_label("Guardar");
   g_signal_connect(g_btn_cfg_save, "clicked", G_CALLBACK(on_btn_cfg_save), NULL);
   gtk_box_pack_start(GTK_BOX(cfg_btn), g_btn_cfg_save, FALSE, FALSE, 0);

   g_btn_cfg_reconfig = gtk_button_new_with_label("Reconfigurar");
   g_signal_connect(g_btn_cfg_reconfig, "clicked", G_CALLBACK(on_btn_cfg_reconfig), NULL);
   gtk_box_pack_start(GTK_BOX(cfg_btn), g_btn_cfg_reconfig, FALSE, FALSE, 0);

   g_btn_cfg_reload = gtk_button_new_with_label("Recargar");
   g_signal_connect(g_btn_cfg_reload, "clicked", G_CALLBACK(on_btn_cfg_reload), NULL);
   gtk_box_pack_start(GTK_BOX(cfg_btn), g_btn_cfg_reload, FALSE, FALSE, 0);

   g_lbl_cfg_status = gtk_label_new("");
   gtk_label_set_xalign(GTK_LABEL(g_lbl_cfg_status), 0.0);
   gtk_box_pack_start(GTK_BOX(cfg_btn), g_lbl_cfg_status, TRUE, TRUE, 0);

   gtk_widget_set_sensitive(g_btn_cfg_save, FALSE);
   gtk_widget_set_sensitive(g_btn_cfg_reconfig, FALSE);
   gtk_widget_set_sensitive(g_btn_cfg_reload, FALSE);

   g_lbl_statusbar = gtk_label_new("Esperando respuesta de startstop...");
   gtk_label_set_xalign(GTK_LABEL(g_lbl_statusbar), 0.0);
   gtk_box_pack_start(GTK_BOX(vbox), g_lbl_statusbar, FALSE, FALSE, 0);

   gtk_widget_show_all(window);

   g_timeout_add_seconds((guint) PollInt, on_poll, NULL);
   g_timeout_add(500, on_read, NULL);
   g_timeout_add(2000, on_try_attach, NULL);

   gtk_main();
   return 0;
}