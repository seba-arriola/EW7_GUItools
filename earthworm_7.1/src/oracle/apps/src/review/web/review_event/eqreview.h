
/*
 *   This file is under RCS - do not modify unless you have
 *   checked it out using the command checkout.
 *
 *    $Id: eqreview.h,v 1.3 2000/12/06 17:49:21 lucky Exp $
 *
 *    Revision history:
 *    $Log: eqreview.h,v $
 *    Revision 1.3  2000/12/06 17:49:21  lucky
 *    *** empty log message ***
 *
 *    Revision 1.2  2000/09/18 17:24:16  lucky
 *    Final version before v5.1
 *
 *    Revision 1.1  2000/08/07 19:48:27  lucky
 *    Initial revision
 *
 *
 *
 *
 */
  
/* The following header files should be included before including
   this file:

  The Base Stuff
  #include <stdlib.h>
  #include <stdio.h>

  Phase III and earthworm headers 
  #include <ora_api.h>
*/

#include <map_display_structs.h>
#include <p3db_ora_api.h>
#include <db_event_info.h>

#define     ARC_MSG_LEN         100000

#define EQP_MAXWORD	         30
#define MAXPATH	        (EQP_MAXWORD)*4+4

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


#define     ACT_GETFROMDB       100
#define     ACT_RELOCATE        200
#define     ACT_DELETE          300
#define     ACT_INSERT          400
#define     ACT_NULL            500
#define     ACT_OVERRIDE        600
#define     ACT_REFRESH         700


#define		MAX_REV_SRCS		10

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
extern char  JavascriptFile[];  
extern char  SacFormat[];  
extern char  ReviewDir[];
extern char  ReviewTmpDir[];
extern char  WebDir[];  
extern char  WebTmpDir[];  
extern char  HypoDir[];       /* Hypoinverse Directory */
extern char  HypoBin[];       /* Hypoinverse Program */
extern char  HypoCfg[];       /* Hypoinverse config file */
extern char  WebHost[];  
extern char  *RevSrcs[];  
extern int   NumRevSrcs;  

extern int errno;   /* system error variable */
/* End of External variables 
********************************/


typedef struct _StationEQStruct
{
  P3DB_ArrivalStruct *pPick;
  P3DB_StationMagnitudeStruct *pStaMag;
  P3DB_StationStruct *pStation;
} StationEQStruct;


/* Web parameters */
typedef struct _WebOptionsStruct
{
  int idEvent;
  int ShowMap;
  int Action;
} WebOptionsStruct;


/* Function prototypes
 *********************/

/*** FROM config.c ***/
int  ReadConfig( char * );                      /* reads configuration file */

/*** FROM eqparam2html.c ***/
int  CompareValueDist( const void *, const void * );    /* for sorting filelist */
int  CompareStaMagValue( const void *, const void * );    /* for sorting filelist */
int  CompareArrivalValue( const void *, const void * );    /* for sorting filelist */

/*** FROM webhelper.c ***/
int  InputEventIds( int **, int *, WebOptionsStruct *pOptions );    /* webparse.c  eventid's from stdin  */

/*** FROM htmlreply.c ***/
void html_header( void );                  /* htmlreply.c */
void html_summary( P3DB_OriginStruct * por,P3DB_MagStruct * pmag,P3DB_EventStruct *pev);
void html_arrivals (DBEventInfoStruct *pEvent);
void html_links2ora2rsec_gif (int EventID);  /* htmlreply.c */
void html_map (DBEventInfoStruct *pEvent, int debug_flag );
void html_nomap (int idEvent);



/*** FROM EXTERNAL LIBRARIES ***/
void logit_init( char *, short, int, int ); /* logit.c      sys-independent  */
void logit( char *, char *, ... );          /* logit.c      sys-independent  */

/* End of Function prototypes
 ****************************/




