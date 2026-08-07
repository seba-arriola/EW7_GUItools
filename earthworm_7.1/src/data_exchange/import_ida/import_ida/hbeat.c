/*======================================================================
 *
 * Check for and respond to terminate messages, and send regular
 * heart beats to the transport ring.
 *
 *====================================================================*/
#include "import_ida.h"

static MSG_LOGO Logo;
static time_t   Interval;
static pid_t    myPid;

extern SHM_INFO *Region;
extern unsigned char InstId;
extern unsigned char ModId;

static thr_ret HeartbeatThread(void *dummy)
{
char     msg[20];
unsigned short length;
time_t   now, then;

    then = time(NULL) - Interval; /* force a heartbeat right away */
    logit("t", "heartbeat thread started\n");

    while(1) {
        now = time(NULL);
        if (now - then >= Interval) {
            sprintf(msg, "%d %d\n\0", now, myPid);
            length = strlen(msg);
            if (tport_putmsg(Region, &Logo, length, msg) != PUT_OK) {
                logit( "et", "Error sending heartbeat to transport ring\n");
            }
            then = now;
        }
        sleep_ew(1000);
    }
}

void StartHeartbeatThread(int interval)
{
unsigned unused;
static char *type = "TYPE_HEARTBEAT";

    Logo.instid = InstId;
    Logo.mod    = ModId;
    Interval    = (time_t) interval;

/* Get process ID for heartbeat messages */

    myPid = getpid();
    if( myPid == -1 ) {
        logit("e","import_ida: Cannot get pid. Exitting.\n");
        exit(1);
    }
    
    if (GetType(type, &Logo.type) != 0) {
        logit("et", "FATAL ERROR: invalid message type <%s>\n", type);
        exit(1);
    }

    if ((StartThread(HeartbeatThread, 0, &unused)) != 0) {
        logit("et", "FATAL ERROR: StartThread failed... exitting\n");
        exit(1);
    }
}
