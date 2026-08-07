
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: eqparam2html.h,v 1.15 2003/07/03 16:28:18 patton Exp $
 *    Revision history:
 *
 *    $Log: eqparam2html.h,v $
 *    Revision 1.15  2003/07/03 16:28:18  patton
 *    Added new optional command for NEIC.
 *
 *    Revision 1.14  2002/05/28 19:24:42  lucky
 *    Added header and footer tag text
 *
 *    Revision 1.13  2002/03/19 17:24:40  lucky
 *    Moved links2oragif to html_common
 *
 *    Revision 1.12  2002/03/18 22:19:59  lucky
 *    Moved summary and coincidence_summary HTML routines to html_common.c in libsrc
 *
 *    Revision 1.11  2002/03/15 15:46:53  lucky
 *    Added coincidence trigger stuff
 *
 *    Revision 1.10  2001/06/21 21:52:59  lucky
 *    Added support for two distinct local mag picks - peak2peak and zero2peak.
 *    The choice is installation specific, so there is a new config option LM_method.
 *
 *    Revision 1.9  2001/06/11 17:00:49  lucky
 *    Added support for amplitude picks and magnitudes
 *
 *    Revision 1.8  2001/05/15 02:15:10  davidk
 *    Moved functions around between the apps, DB API, and DB API INTERNAL
 *    levels.  Renamed functions and files.  Added support for amplitude
 *    magnitude types.  Reformatted makefiles.
 *
 *    Revision 1.7  2001/02/28 17:29:10  lucky
 *    Massive schema redesign and cleanup.
 *
 *    Revision 1.6  2000/12/21 00:25:33  davidk
 *    added SeismogramProgramDisplayStruct and several externs to support
 *    the new WaveformLinks config file command and configurable
 *    "
 *    record section display" links.
 *
 *    Revision 1.5  2000/09/20 18:03:08  lucky
 *    Rewrote to look like eqreview: calls ewdb_apps_GetEWEventInfo and processes the Arrivals
 *    from it.
 *    Cleaned up formatting and many silly logit calls.
 *
 *    Revision 1.4  2000/08/07 19:39:34  lucky
 *    Added WebHost option.
 *
 *    Revision 1.3  2000/05/31 17:26:35  lucky
 *    Modified code so that it can be reused in the review/alarm applications. Also, modified
 *    makefiles to use the new libsrc directory structure.
 *
 *    Revision 1.2  1999/11/09 16:49:52  lucky
 *    *** empty log message ***
 *
 *    Revision 1.1  1999/10/20 23:58:20  davidk
 *    Initial revision
 *
 *    Revision 1.1  1999/05/05 18:05:54  lucky
 *    Initial revision
 *
 *
 */
  
/* eqparam2html.h */

#include <ew_event_info.h>
#include <html_common.h>

#define EQP_MAXWORD	         30
#define MAXPATH	        (EQP_MAXWORD)*4+4

/* hard code the Maximum number of phases per Earthquake */
#define MAX_PHS_PER_EQ  250

#define RB_ALL_EVENTS 0
#define RB_ALL_REVIEWED_EVENTS 1
#define RB_UNREVIEWED_EVENTS 2
#define RB_REVIEWEDBY_EVENTS 3

#define ET_ALL_EVENTS 0
#define ET_QUAKES 1
#define ET_BLASTS 2

#define DEFAULT_END_DATE_STR "2050/12/31"
#define DEFAULT_END_TIME_STR "23:59:59"
#define DEFAULT_START_DATE_STR "1900/01/01"
#define DEFAULT_START_TIME_STR "00:00:00"
#define DEFAULT_MINLAT (-90.00)
#define DEFAULT_MAXLAT (90.00)
#define DEFAULT_MINLON (-180.00)
#define DEFAULT_MAXLON (180.00)
#define DEFAULT_MINZ (0.00)
#define DEFAULT_MAXZ (100.00)
#define DEFAULT_MINMAG (0.00)
#define DEFAULT_MAXMAG (10.00)
#define DEFAULT_MAX_EVENTS (500)
#define DEFAULT_NUMBER_OF_DAYS (7)

#define DEFAULT_SOURCE_TYPE (RB_ALL_EVENTS)
#define DEFAULT_EVENT_TYPE (ET_ALL_EVENTS)


/* Typedefs and structures
********************************/
typedef struct _StationEQStruct
{
  EWDB_ArrivalStruct *pPick;
  EWDB_StationMagStruct *pStaMag;
  EWDB_StationStruct *pStation;
} StationEQStruct;

/* End of Typedefs
********************************/



/* Extern variables 
********************************/
extern int   DEBUG;                  /* debug flag */    

/* Environment stuff */
extern char  envEW_LOG[];   /* where environment variable EW_LOG 
                                        will be stored                  */
/* Database connection things */
extern char  DBservice[];        /* DBMS instance to interact with    */
extern char  DBuser[];           /* UserId to connect to database as  */
extern char  DBpassword[];       /* Password to datasource            */

extern char  BackgroundColor[];
extern char  HeaderLogo[];
extern char  FooterLogo[];
extern char  HeaderTag[];
extern char  FooterTag[];

extern char  WebHost[];
extern char  LM_method[];

extern int errno;                /* system error variable */

extern int iNumTGDs;             /* Number of "seismic display program
                                    links to add at bottom of page */

extern TraceGifDisplayStruct TGDS[];

int Origins_not_via_Review;
/* End of External variables 
********************************/



/* Function prototypes
 *********************/

/*** FROM config.c ***/
int  ReadConfig( char * );                      /* reads configuration file */

/*** FROM eqparam2html.c ***/
int  CompareValueDist( const void *, const void * );    /* for sorting filelist */
int  CompareStaMagValue( const void *, const void * );    /* for sorting filelist */
int  CompareArrivalValue( const void *, const void * );    /* for sorting filelist */

/*** FROM webhelper.c ***/
int  InputEventIds( int **, int * );    /* webparse.c  eventid's from stdin  */

/*** FROM htmlreply.c ***/
void 	html_arrivals (EWEventInfoStruct *);
int 	html_map (EWEventInfoStruct *,  int);



/*** FROM EXTERNAL LIBRARIES ***/
void logit_init( char *, short, int, int ); /* logit.c      sys-independent  */
void logit( char *, char *, ... );          /* logit.c      sys-independent  */

/* End of Function prototypes
 ****************************/




