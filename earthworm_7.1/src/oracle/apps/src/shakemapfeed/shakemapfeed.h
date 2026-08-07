
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: shakemapfeed.h,v 1.9 2004/03/04 18:10:36 dietz Exp $
 *
 *    Revision history:
 *     $Log: shakemapfeed.h,v $
 *     Revision 1.9  2004/03/04 18:10:36  dietz
 *     Replaced the DelayFirstFeed and UpdateInterval parameters with
 *     Schedule parameter to allow for variable time intervals between
 *     feeds to shakemap.
 *
 *     Revision 1.8  2004/02/13 19:37:06  dietz
 *     Added new optional command, SMQueryMethod, to control how
 *     strong motion data is requested from the DBMS.
 *
 *     Revision 1.7  2004/02/05 22:27:17  dietz
 *     Changed definition of LONG_STA_LEN from 20 to 50
 *
 *     Revision 1.6  2002/04/12 22:35:21  dietz
 *     Added support for new command, UseEventID.
 *
 *     Revision 1.5  2001/05/15 02:15:46  davidk
 *     Moved functions around between the apps, DB API, and DB API INTERNAL
 *     levels.  Renamed functions and files.  Added support for amplitude
 *     magnitude types.  Reformatted makefiles.
 *
 *     Revision 1.4  2001/05/01 20:35:01  dietz
 *     *** empty log message ***
 *
 *     Revision 1.3  2001/03/22 18:14:39  lombard
 *     Added support for XML writer and the mapping file.
 *     changed to use standard macros for Earthworm logo fields
 *
 *     Revision 1.2  2001/03/15 00:02:33  dietz
 *     *** empty log message ***
 *
 *     Revision 1.1  2000/02/15 20:20:15  lucky
 *     Initial revision
 *
 *
 */


/*
 *  shakemapfeed.h
 */

#ifndef _SHAKEMAPFEED_H_
#define _SHAKEMAPFEED_H_

#include <earthworm.h>
#include <chron3.h>
#include <kom.h>
#include <tlay.h>
#include <transport.h>
#include <rw_strongmotionII.h>
#include <rw_mag.h>
#include <read_arc.h>

/* Constant Definitions                       
 **********************/
#define  INIT_FATAL       -1   /* fatal error in initialization          */
#define  INIT_OK           0   /* initialization went fine               */

#define  ERR_UNKNOWN       0   /* unknown return code from a function    */
#define  ERR_TOOBIG        1   /* retreived msg too large for buffer     */
#define  ERR_NOTRACK       2   /* msg retreived; tracking limit exceeded */
#define  ERR_MISSLAPMSG    3   /* message missed (lap) in transport ring */
#define  ERR_MISSGAPMSG    4   /* sequence gap in transport ring         */

#define  NOTELEN         256   /* string length for notes (pages,errors) */
#define  PATHLEN          56   /* string length for directory names      */
#define  WORDLEN          31   /* string length for config words         */
#define  MAXLOGO          10   /* max # logos to request from transport  */
#define  MAXPAGE          10   /* max # page recipients                  */
#define  MAXSCHED         20   /* max # update times in feeding schedule */

#define  USE_BINDERID      1   /* label output files with binder's id    */
#define  USE_DBMSID        2   /* label output files with DBMS event id  */

/* Structure Definitions
 ***********************/
typedef struct {
  char         net[TRACE_NET_LEN+1]; /* network code of interest         */
  double       ttolerance;         /* how bad can timestamps be for net? */
                                   /* in seconds                         */
} TIMETOLERANCE;

typedef struct {
  int          nsched;             /* number of update times in schedule */
  time_t       tsched[MAXSCHED];   /* time (in seconds) after receiving  */
                                   /* event to try to feed shakemap      */
} FEEDSCHEDULE;

typedef char COMPONENT[TRACE_CHAN_LEN+1]; /* component code */

/* Things to read or derive from configuration file */
typedef struct {
  char          RingName[MAX_RING_STR]; /* shared memory region name    */
  char          MyModName[MAX_MOD_STR]; /* module name                  */
  int           LogSwitch;         /* 0 if no logfile should be written */
  long          HeartBeatInterval; /* seconds between heartbeats        */
  MSG_LOGO      GetLogo[MAXLOGO];  /* array for requested logos         */
  short         nLogo;             /* number of logos in config file    */
  int           LogShakemapFile;   /* write shakemap file to log?       */
  int           ShakemapFormat;    /* Output format to use (=1,2)       */
  int           Debug;             
  char          DBservice[WORDLEN];/* DBMS instance to interact with    */
  char          DBuser[WORDLEN];   /* UserId to connect to database as  */
  char          DBpassword[WORDLEN];/* Password to datasource           */
  int           MaxDataPerEvent;   /* max #SM channels to handle per eq */
  float         MagThresh;         /* minimum "interesting" magnitude   */
  float         MagWtThresh;       /* min magnitude weight (#stas)      */
  int           UpdateDuration;    /* autofeed for this many seconds    */
  float         LatRange;          /* get data within Y deg lat of eq   */
  float         LonRange;          /* get data within X deg lon of eq   */
  float         LatMin;            /* boundary of interest to shakemap  */
  float         LatMax;            /* boundary of interest to shakemap  */
  float         LonMin;            /* boundary of interest to shakemap  */
  float         LonMax;            /* boundary of interest to shakemap  */
  char          OutputDir[PATHLEN]; /* directory to write files to      */
  char          TempDir[PATHLEN];  /* temporary working directory       */
  int           nPage;             /* number of page recipients         */
  char          Page[MAXPAGE][WORDLEN]; /* array for page recipients    */
  int           nBlock;            /* number of components to block.    */
  COMPONENT    *BlockComp;         /* array of components to block;     */
                                   /*  alloced at config file read.     */  
  int            nNetSlop;         /* number of net timeslop structs    */
  TIMETOLERANCE *NetSlop;          /* array for net-dependent time-slops*/
                                   /*  alloced at config file read      */ 
  double         DefaultTolerance; /* time tolerance for unlisted nets  */ 
  char          *MappingFile;      /* mapping file for instrument types,*/
                                   /*  station names, network agencies  */
  char          *QuakeAuthor;      /* msg logo as 9-char ascii string   */
  int            UseEventID;       /* which eventid to use in output    */
  int            SMQueryMethod;    /* choose how to query DBMS for SM   */
  FEEDSCHEDULE   Schedule;         /* when to schedule shakemap feeds;  */
} PARAM;

/* Things to look up in the Earthworm tables with getutil.c functions */
typedef struct {
  SHM_INFO      Region;            /* shared memory to use for i/o      */
  long          RingKey;           /* key of transport ring for i/o     */
  unsigned char InstId;            /* local installation id             */
  unsigned char MyModId;           /* Module Id for this program        */
  unsigned char TypeHeartBeat; 
  unsigned char TypeError;
  unsigned char TypeHyp2000Arc;
  unsigned char TypeSM;
  unsigned char TypePage;
  unsigned char TypeEQDelete;
  unsigned char TypeMagnitude;
} EWD;

/*********************************************************************
    Define the structure for the event list.
    This is an abbreviated structure.
*********************************************************************/
typedef struct {    /* An Event List */
  double  time;     /* time of Event (sec since 1970)         */
  float   lat;      /* latitude (degrees)                     */
  float   lon;      /* longitude (degrees)                    */
  float   depth;    /* hypocentral depth                      */
  float   mag;      /* magnitude, preferred or coda           */
  char    magtype[MAG_NAME_LEN+1];  /* type of magnitude      */
  float   magwt;    /* magnitude weight (~#stas used in calc) */
  int     nph;      /* number of phases used in the location  */
  float   rms;      /* rms residual of location               */
  long    id;       /* Event ID from binder                   */
  char    idstr[256];  /* Binder's eventid as a char string   */  
  char    author[256]; /* Event author as a char string       */
  long    dbid;     /* Event ID from EW database              */
  double  tlastdata;/* time that last data was loaded in dbms */
  time_t  tinlist;  /* time the event was loaded into list    */
  time_t  tupdate;  /* next time to rebuild shakemap file     */
  time_t  tdone;    /* don't do any updates after this time   */ 
  int     nfeed;    /* number successful feeds to shakemap    */ 
                    /*  for this eq                           */
} EVENTLIST;

/**********************************************
   Structures to make up the Agency, Station,
   and Instrument Type long name tables. 
********************************************/
/* If you change these lengths, make sure you change the *
 * sscanf format statements in initMappings()!!!         */
#define AGENCY_LEN 40
#define LONG_STA_LEN 50
#define INST_TYPE_LEN 50
typedef struct {
  char net[TRACE_NET_LEN+1];
  char agency[AGENCY_LEN+1];
} AGENCY;

typedef struct {
  char sta[TRACE_STA_LEN+1];
  char net[TRACE_NET_LEN+1];
  char longName[LONG_STA_LEN+1];
} LONGSTANAME;

typedef struct {
  char sta[TRACE_STA_LEN+1];
  char comp[TRACE_CHAN_LEN+1];
  char net[TRACE_NET_LEN+1];
  char instType[INST_TYPE_LEN+1];
} INSTTYPE;


/* Function Prototypes
 *********************/
void shakemapfeed_config( char *filename, PARAM *parm );
void shakemapfeed_lookup( PARAM *parm, EWD *ewd );
void shakemapfeed_status( EWD *ewd, unsigned char type, short err, char *s );
void shakemapfeed_page( PARAM *parm, EWD *ewd, char *note );
int  ProcessArc( PARAM *parm, EWD *ewd, char *arcmsg, long msglen );
int  ProcessDelete( PARAM *param, EWD *ewd, char *delmsg, long msglen );
int  ProcessMag( PARAM *parm, EWD *ewd, char *magmsg, long msglen );
int  ReviewEvents( PARAM *parm, EWD *ewd );
int  SMfeedEvent( PARAM *parm, EWD *ewd, EVENTLIST *eqs );
void ShutdownEvent( PARAM *parm );
int  initMappings( char *mapFile );

#endif  /*_SHAKEMAPFEED_H_*/
