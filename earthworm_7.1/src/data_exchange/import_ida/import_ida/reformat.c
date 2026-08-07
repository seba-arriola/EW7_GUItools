/*======================================================================
 *
 * Reformat an xfer_packet into a TracePacket
 *
 *====================================================================*/
#include "import_ida.h"

extern struct params *Par;

void Reformat(TracePacket *trace, struct xfer_packet *src)
{
  char szIDAChan[20];
  char *szChan;
  char *szLoc;

#if defined (BIG_ENDIAN_HOST)
  static char *datatype = "s4";
#elif defined (LTL_ENDIAN_HOST)
  static char *datatype = "i4";
#else
# error "Host byte-order not defined!"
#endif

#define IDA_CHAN_LEN 3
  long *data;

/* Fill in the TracePacket header */

    strcpy(trace->trh2.sta,  util_ucase(src->sname));
    memset(szIDAChan, 0, sizeof(szIDAChan));
    strncpy(szIDAChan, util_ucase(src->cname), sizeof(szIDAChan)-1);

    szChan = szIDAChan;
    szLoc = &szIDAChan[IDA_CHAN_LEN]; 
    if(!(*szLoc))
    {
      szLoc = LOC_NULL_STRING;
    }

    /* we are expecting the chan code that we get from IDA to look like:
       CCCLL or  CCC   (BHZ00 or BHZ) */
    strncpy(trace->trh2.chan, szChan, IDA_CHAN_LEN);
    trace->trh2.chan[IDA_CHAN_LEN] = 0x00;
    strncpy(trace->trh2.loc,  szLoc, TRACE2_LOC_LEN-1);
    trace->trh2.loc[TRACE2_LOC_LEN-1] = 0x00;

    strcpy(trace->trh2.net,  Par->Network);
    strcpy(trace->trh2.datatype, datatype);
    trace->trh2.starttime = src->beg;
    trace->trh2.endtime   = src->end;
    trace->trh2.nsamp     = src->nsamp;
    trace->trh2.samprate  = 1.0 / src->sint;
    trace->trh2.version[0]  = TRACE2_VERSION0;
    trace->trh2.version[1]  = TRACE2_VERSION1;
    trace->trh2.pinno       = DEFAULT_PINNO;
    /**************************
     **** Pin# FUNCTIONALITY REMOVED PER Alex Bittenbinder 04/20/2005
     * set_pinno(trace);
     ****************************/

/* Fill in the data */
    
    data = (long *) ((char *) trace + sizeof(TRACE2_HEADER));
    memcpy(data, src->data, src->nsamp * 4);
}
