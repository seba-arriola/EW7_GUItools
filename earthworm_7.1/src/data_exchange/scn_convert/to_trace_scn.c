/*
 *   This file is managed using Concurrent Versions System (CVS).
 *
 *    $Id: to_trace_scn.c,v 1.1 2006/12/28 23:27:53 lombard Exp $
 *
 */

#include <string.h>
#include <trace_buf.h>
#include "scnl_convert.h"

      /*******************************************************
       *                   to_trace_scn()                    *
       *  Convert a pick of TYPE_TRACEBUF2 to a pick of      *
       *  TYPE_TRACEBUF.                                     *
       *  Returns 0, if no error.                            *
       *          1, if not selected for conversion          *
       *         -1, if an error or buffer overflow occured  *
       *******************************************************/

int to_trace_scn( char *msg )
{
    TRACE_HEADER  *TraceHead;      /* The tracebuf header */
    TRACE2_HEADER *Trace2Head;     /* The tracebuf2 header */
    S2S s;
    
    TraceHead = (TRACE_HEADER *) msg;
    Trace2Head = (TRACE2_HEADER *) msg;
    
    memset(&s, 0, sizeof(S2S));
    s.scnl.s = Trace2Head->sta;
    s.scnl.c = Trace2Head->chan;
    s.scnl.n = Trace2Head->net;
    s.scnl.l = Trace2Head->loc;

    if (scnl2scn(&s) == 0) return 1; /* no match; don't send */
    
    strncpy(TraceHead->sta, s.scn.s, TRACE_STA_LEN);
    strncpy(TraceHead->chan, s.scn.c, TRACE_CHAN_LEN);
    strncpy(TraceHead->net, s.scn.n, TRACE_NET_LEN);
    
    return 0;
}

	
    
