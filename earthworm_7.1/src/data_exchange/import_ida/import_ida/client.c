/*======================================================================
 *
 * Thread to deal with a single connection
 *
 *====================================================================*/
#include "import_ida.h"

#define MSEC_PER_SEC 1000

/* Number of seconds to sleep after a socket related error */

#define RETRY_INTERVAL 60

/* Number of seconds to continue to try to recover after a network error */

#define MAXTRY 604800 /* 1 week */

thr_ret ClientThread(void *argptr)
{
IDA_SERVER *server;
TracePacket trace;
XFER *xp = (XFER *) NULL;
int status;
unsigned long packet_count = 0;
time_t elapsed = 0;
struct xfer_packet packet;

    server = (IDA_SERVER *) argptr;

/* Spend eternity here */

    elapsed = 0;
    while (1) {

    /* Connect to the data server */

        do {
            if (elapsed == 0) logit("t", "contacting %s\n", server->name);
            xp = Xfer_Open(
                server->name,        /* hostname              */
                server->sc,          /* station/channel list  */
                XFER_YNGEST,         /* start time            */
                XFER_YNGEST,         /* end time              */
                1,                   /* continuous feed       */
                server->policy       /* reconnect policy flag */
            );
            if (xp == (XFER *) NULL) {
                if (elapsed == 0) {
                    logit("t", "%s: transient(?) connection failure\n",
                        server->name
                    );
                } else if (elapsed > MAXTRY) {
                    logit("t", "%s: repeated connection failures (FATAL)\n",
                        server->name
                    );
                    logit("t", "%s: retry interval expired, giving up!\n",
                        server->name
                    );
                    KillSelfThread();
                }
                sleep_ew(RETRY_INTERVAL * MSEC_PER_SEC);
                elapsed += RETRY_INTERVAL;
            }
        } while (xp == (XFER *) NULL);
        elapsed = 0;

    /* Read packets forever */

        logit("t", "connected to %s, waiting for data\n", server->name);
        while ((status = Xfer_Read(xp, &packet)) == XFER_OK) {
            if (++packet_count == 1) {
                logit("t", "%s: initial packet received\n", server->name);
            }
            Reformat(&trace, &packet);
            forward(&trace);
        }

    /* If we ever get here something other than a time out happened.   */
    /* We'll retry anyway in the hopes that it is a transient problem. */

        logit("t", "%s: server not responding, attempting to reconnect\n",
            server->name
        );
        Xfer_Close(xp);
        xp = (XFER *) NULL;
        packet_count = 0;
    }    
}
