/*======================================================================
 *
 * Quick and dirty program to monitor a transport ring for TracePackets
 * from import_ida, as an aid in testing of the same.
 *
 *====================================================================*/
#include <stdio.h>
#include "idatap.h"
#include "earthworm.h"
#include "transport.h"
#include "trace_buf.h"

main(int argc, char **argv)
{
int status;
long len;
MSG_LOGO logo, want;
SHM_INFO region;
TracePacket trace;
long key, *data;
unsigned char seqno;

    if (argc != 2) {
        fprintf(stderr, "usage: %s shm_key\n", argv[0]);
        exit(1);
    }

    key = atol(argv[1]);
    tport_attach(&region, key);
    printf("Attached to memory region %ld\n", key);

    GetType( "TYPE_TRACEBUF", &want.type   );
    GetModId("MOD_WILDCARD",  &want.mod    );
    GetInst( "INST_WILDCARD", &want.instid );

    data = (long *) ((char *) &trace + sizeof(TRACE_HEADER));

    while (1) {
        if (tport_getflag(&region) == TERMINATE) {
            printf("termination flag set... exiting\n");
            exit(0);
        }
#ifdef USE_GETMSG
        status = tport_getmsg(
            &region, &want, (short) 1, &logo, &len,
            (char *) &trace, (long) sizeof(TracePacket)
        );
#else
        status = tport_copyfrom(
            &region, &want, (short) 1, &logo, &len,
            (char *) &trace, (long) sizeof(TracePacket), &seqno
        );
        if (status == GET_OK) {
#endif
            printf("%d:%d:%d ", want.type, want.mod, want.instid);
            printf("%d:%d:%d ", logo.type, logo.mod, logo.instid);
            printf("[%4s:%-5s:%2s] ",
                trace.trh.sta, trace.trh.chan, trace.trh.net
            );
            printf("%5d ",   trace.trh.pinno);
            printf("%s ",    util_dttostr(trace.trh.starttime, 0));
            printf("%s ",    util_dttostr(trace.trh.endtime, 0));
            printf("%7.2f ", trace.trh.samprate);
            printf("%s ",    trace.trh.datatype);
            printf("%4d ",   trace.trh.nsamp);
            printf("%7ld ",  data[0]);
            printf("%7ld ",  data[trace.trh.nsamp-1]);
            printf("\n");
        } else if (status == GET_NONE) {
            sleep_ew(250);
        } else {
#ifdef USE_GETMSG
            printf("tport_getmsg returns status %d\n", status);
#else
            printf("tport_copyfrom returns status %d\n", status);
#endif
        }
    }
}
