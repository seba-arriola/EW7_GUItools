#ifndef import_ida_h_included
#define import_ida_h_included

#include "earthworm.h"
#include "transport.h"
#include "trace_buf.h"

#include "idatap.h"

#define MAX_SERVERS 64
#define MAX_SERVER_NAMELEN 128
#define MAX_SERVER_SCLEN   1024

#define MINIMAL_GAP   1
#define MINIMAL_DELAY 2

#define VERSION_STRING "Version 2.1"

typedef struct {
	char name[MAX_SERVER_NAMELEN+1];
	char sc[MAX_SERVER_SCLEN+1];
    int  policy;
} IDA_SERVER;

struct params {
    char *MyModName;
    char *OutRing;
    char *PinnoFile;
    char *Network;
    int defpinno;
    long hbeat;
    long maxlag;
    int policy;
    int nserver;
    IDA_SERVER server[MAX_SERVERS];
};

/* Defaults */

#define DEFAULT_PINNO   12345
#define DEFAULT_HBEAT   30
#define DEFAULT_MAXLAG  3600
#define DEFAULT_NETWORK "II"

/* function prototoypes */

/* client.c */
thr_ret ClientThread(void *argptr);

/* forward.c */
void forward(TracePacket *trace);

/* hbeat.c */
void StartHeartbeatThread(int interval);

/* init.c */
void init(int argc, char **argv);

/* params.c */
long read_params(char *path, char *buffer, int len);
void log_params(char *fname, long key);

/* pinno.c */
int read_pinnos(char *path);
void set_pinno(TracePacket *trace);
int list_pinnos();

/* reformat.c */
void Reformat(TracePacket *tpak, struct xfer_packet *src);

/* signals.c */
void start_sighandler();

#endif  /* import_ida_h_included */
