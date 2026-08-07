/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: db_event_info.h,v 1.1 2001/05/16 17:13:07 lucky Exp $
 *
 *    Revision history:
 *     $Log: db_event_info.h,v $
 *     Revision 1.1  2001/05/16 17:13:07  lucky
 *     Initial revision
 *
 *     Revision 1.10  2001/04/17 16:34:07  davidk
 *     Added prototypes for InitDBEvent(), InitChan(), and DBEvent2ArcMsg(),
 *     in order to get rid of compiler warnings on NT.
 *
 *     Revision 1.9  2001/03/19 17:20:48  lucky
 *     Added RCS headers
 *
 * 
 */

#ifndef GET_DB_EVENT_INFO_H
# define GET_DB_EVENT_INFO_H
#include <ewdb_ora_api.h>


/* MAX_DATA_PER_CHANNEL_PER_EVENT (MDPCPE) */
#define MDPCPE 6

/* Constant used for allocating Channel structs */
/* How many Chan structures do we allocate initially? */
#define     INIT_NUM_CHANS      50
/* number of chans allocated subsequently */
#define     CHAN_ALLOC_INCR     50


/* GetDBEventInfo_Super() event information type constants */
/* bitwise OR these constants together and pass them as 
   the "Flags" param to GetDBEventInfo_Super() in order
   to control what types of information is retrieved for
   an event.
************************************************************/
#define GETDBEVENTINFO_SUMMARYINFO    0x01
#define GETDBEVENTINFO_PICKS          0x02
#define GETDBEVENTINFO_STAMAGS        0x04
#define GETDBEVENTINFO_COMPINFO       0x08
#define GETDBEVENTINFO_COOKEDTF       0x10
#define GETDBEVENTINFO_STRONGMOTION   0x20
#define GETDBEVENTINFO_WAVEFORM_DESCS 0x40
#define GETDBEVENTINFO_WAVEFORMS      0x80



typedef struct _DBChannelDataStruct
{
  EWDBid                      idChan;
  EWDB_StationStruct          Station;
  EWDB_WaveformStruct         Waveforms[MDPCPE];
  EWDB_ArrivalStruct          Arrivals[MDPCPE];
  EWDB_StationMagStruct       Stamags[MDPCPE];
  int                         iNumArrivals;
  int                         iNumStaMags;
  int                         iNumWaveforms;
  int                         bResponseIsValid;
  EWDB_ChanTCTFStruct         ResponseInfo;
} DBChannelDataStruct;


typedef struct _DBEventInfoStruct
{
  EWDB_EventStruct      Event;
  EWDB_MagStruct        PrefMag;
  EWDB_OriginStruct     PrefOrigin;
  DBChannelDataStruct   *pChanInfo;
  int                   iNumChans;

/* This many DBChannelDataStructs have been allocated */
/* at pChanInfo -- NOTE: this value is set in GetDBEventInfo */
/* and should be reset if any memory is freed later */ 
  int                   iNumAllocChans; 
} DBEventInfoStruct;


/* external functions */
void logit( char *, char *, ... );          /* logit.c      sys-independent  */


/* functions in InitFullEvent.c */
int		InitDBEvent (DBEventInfoStruct *pEventInfo); 
int		InitChan (DBChannelDataStruct *pChan);

/* functions in Arc2FullEvent.c */
int 	DBEvent2ArcMsg (DBEventInfoStruct *, char *, int);

#endif /* GET_DB_EVENT_INFO_H */
