/*======================================================================
 *
 * Import data from one or more Project IDA data servers and load them
 * into an Earthworm transport ring as TracePackets.
 *
 * The program works by creating individual threads to handle all
 * communication with any given server.  Each thread writes its data
 * packets directly to Earthworm shared memory.
 *
 * This program requires uses a customized version of the UCSD idatap
 * software library, which should be included with this distribution.
 *
 * Import_ida program was written for the U.S. Geological Survey by:
 * -------------------------------------------------------------------
 * Engineering Services & Software                 email: dec@essw.com
 * 16980 Via Tazon, Suite 285                      tel: (858) 385-0909
 * San Diego, CA 92127                             fax: (858) 385-0457
 *====================================================================*/
/* changes: 
  ESSW: March 2002
    switch to multi-threaded design and port to NT

  Lombard: 11/19/98: V4.0 changes: 
     0) no Y2k dates 
     1) changed argument of logit_init to the config file name.
     2) process ID in heartbeat message
     3) flush input transport ring: not applicable
     4) add `restartMe' to .desc file: where is it?
     5) multi-threaded logit
*/

#include "import_ida.h"

/* Globals */

char *ProgName;         /* my name             */
struct params *Par;     /* run time parameters */
SHM_INFO *Region;       /* output ring         */
unsigned char InstId;   /* for the output logo */
unsigned char ModId;    /* for the output logo */

main(int argc, char **argv)
{
int i, status;
unsigned unused;
struct params par;
SHM_INFO region;

    ProgName  = argv[0];
    Par       = &par;
    Region    = &region;
    
/* Startup initialization.  Are attached to transport ring when done. */

    init(argc, argv);

/* Start heartbeat thread */

    StartHeartbeatThread(Par->hbeat);

/* Start up individual threads to handle comm with each server */

    for (i = 0; i < Par->nserver; i++) {
        status = StartThreadWithArg(
            ClientThread,
            (void *) &Par->server[i],
            0,
            &unused
        );
        if (status != 0) {
            logit("e", "StartThreadWithArg failed... exiting\n");
            exit(1);
        }
    }

/* Wait for termination message */

    while (1) {

        if (tport_getflag(Region) == TERMINATE) {
            logit("t", "termination flag set... exiting\n");
            exit(0);
        }
        sleep_ew(1000);
    }
}
