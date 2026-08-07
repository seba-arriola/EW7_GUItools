/* get_db_event_info.h */

#ifndef GET_DB_EVENT_INFO_H
# define GET_DB_EVENT_INFO_H
#include <p3db_ora_api.h>


  /* MAX_DATA_PER_CHANNEL_PER_EVENT (MDPCPE) */
#define MDPCPE 3

/* We'll see at most this many channels */
#define MAX_EV_CHANS 	1000

typedef struct _DBChannelDataStruct
{
  P3DBid                      idChan;
  P3DB_StationStruct          Station;
  P3DB_WaveformStruct         Waveforms[MDPCPE];
  P3DB_ArrivalStruct          Arrivals[MDPCPE];
  P3DB_StationMagnitudeStruct Stamags[MDPCPE];
  P3DB_PhaseAmpStruct         PhaseAmps[MDPCPE];		  
  int                         iNumArrivals;
  int                         iNumStaMags;
  int                         iNumWaveforms;
  int                         bResponseIsValid;
  P3DB_ChanTCTFStruct         ResponseInfo;
} DBChannelDataStruct;


typedef struct _DBEventInfoStruct
{
  P3DB_EventStruct      Event;
  P3DB_MagStruct        PrefMag;
  P3DB_OriginStruct     PrefOrigin;
  DBChannelDataStruct   pChanInfo[MAX_EV_CHANS];
  int                   iNumChans;
} DBEventInfoStruct;

/* functions in get_db_event_info.c */
int GetDBEventInfo(DBEventInfoStruct * pEventInfo, P3DBid idEvent);

/* external functions */
void logit( char *, char *, ... );          /* logit.c      sys-independent  */

#endif /* GET_DB_EVENT_INFO_H */
