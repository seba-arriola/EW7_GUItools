/*======================================================================
 *
 * Forward a trace packet onto the transport ring
 *
 *====================================================================*/
#include "import_ida.h"

extern SHM_INFO *Region;
extern unsigned char InstId;
extern unsigned char ModId;

void forward(TracePacket *trace)
{
long length;
int  status;
int  first = 1;
static MSG_LOGO logo;
static char *type = "TYPE_TRACEBUF2";

    if (first) {
        logo.instid = InstId;
        logo.mod    = ModId;
        if (GetType(type, &logo.type) != 0) {
            logit("et", "FATAL ERROR: invalid message type <%s>\n", type);
            exit(1);
        }
        first = 0;
    }

    length = sizeof(TRACE2_HEADER) + (trace->trh2.nsamp * 4);

    status = tport_putmsg(Region, &logo, length, (char *) trace);
    if (status != PUT_OK) {
        logit("et", "tport_putmsg error (ignored)\n");
    }
}
