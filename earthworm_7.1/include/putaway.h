
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: putaway.h,v 1.2 2007/02/17 02:37:11 stefan Exp $
 *
 *    Revision history:
 *     $Log: putaway.h,v $
 *     Revision 1.2  2007/02/17 02:37:11  stefan
 *     miniseed putaway prototypes
 *
 *     Revision 1.1  2001/04/12 03:08:12  lombard
 *     Initial revision
 *
 *
 *
 */

#ifndef PUTAWAY_H
#define PUTAWAY_H


/* Function prototypes */
int PA_init (char *DataFormat, long TraceBufferLen, long *OutBufferLen, 
             int *FormatInd, char *OutDir, char * OutputFormat, int debug);

int PA_next_ev (char *EventID, TRACE_REQ *trace_req, int num_req, 
                int FormatInd, char *OutDir, char *EventDate, 
                char *EventTime, char *EventSubnet, int debug);

int PA_next (TRACE_REQ *getThis, int FormatInd, 
             double GapThresh, long OutBufferLen, int debug);

int PA_end_ev (int, int);
int PA_close (int, int);

#endif
#ifdef _SOLARIS
int MSEEDPA_init (char *, char *, int);
int MSEEDPA_next_ev (char *,char *, char *, int);
int MSEEDPA_next (TRACE_REQ *, double, int);
int MSEEDPA_end_ev (int);
int MSEEDPA_close (int); 
#endif /* _SOLARIS */

