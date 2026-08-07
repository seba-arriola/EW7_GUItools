/*
 *   This file is managed using Concurrent Versions System (CVS).
 *
 *    $Id: to_trace_scnl.c,v 1.2 2006/06/28 19:25:45 stefan Exp $
 *
 */

#include <string.h>
#include <trace_buf.h>
#include "scn_convert.h"

      /*******************************************************
       *                   to_trace_scnl()                   *
       *  Convert a pick of TYPE_TRACEBUF to a pick of       *
       *  TYPE_TRACEBUF2.                                    *
       *  Returns 0, if no error.                            *
       *          1, if not selected for conversion          *
       *         -1, if an error or buffer overflow occured  *
       *******************************************************/

int to_trace_scnl( char *msg )
{
    TRACE_HEADER  *TraceHead;      /* The tracebuf header */
    TRACE2_HEADER *Trace2Head;     /* The tracebuf2 header */
    S2S s;
    
    TraceHead = (TRACE_HEADER *) msg;
    Trace2Head = (TRACE2_HEADER *) msg;
    
    memset(&s, 0, sizeof(S2S));
    s.scn.s = TraceHead->sta;
    s.scn.c = TraceHead->chan;
    s.scn.n = TraceHead->net;
    
    if (scn2scnl(&s) == 0) return 1; /* no match; don't send */
    
    strncpy(Trace2Head->sta, s.scnl.s, TRACE2_STA_LEN);
    strncpy(Trace2Head->chan, s.scnl.c, TRACE2_CHAN_LEN);
    strncpy(Trace2Head->net, s.scnl.n, TRACE2_NET_LEN);
    strncpy(Trace2Head->loc, s.scnl.l, TRACE2_LOC_LEN);

	/* Make sure the last character is null; strncpy doesn't automatically append null if
	 * count <= sizeof(strSource), and the TRACE2_*_LEN defines include room for a null.
	 */
	Trace2Head->sta[TRACE2_STA_LEN - 1] = 0;
	Trace2Head->chan[TRACE2_CHAN_LEN - 1] = 0;
	Trace2Head->net[TRACE2_NET_LEN - 1] = 0;
	Trace2Head->loc[TRACE2_LOC_LEN - 1] = 0;

	Trace2Head->version[0] = TRACE2_VERSION0;
    Trace2Head->version[1] = TRACE2_VERSION1;
    
    return 0;
}

	
    