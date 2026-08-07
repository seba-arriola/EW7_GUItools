
/*
 *   This file is under RCS - do not modify unless you have
 *   checked it out using the command checkout.
 *
 *    $Id: review.h,v 1.12 2003/01/30 23:12:57 lucky Exp $
 *
 *    Revision history:
 *    $Log: review.h,v $
 *    Revision 1.12  2003/01/30 23:12:57  lucky
 *    *** empty log message ***
 *
 *    Revision 1.11  2002/05/28 19:33:56  lucky
 *    Added header and footer tag text
 *
 *    Revision 1.10  2002/05/28 17:24:05  lucky
 *    *** empty log message ***
 *
 *    Revision 1.9  2002/03/22 18:23:18  lucky
 *    Added retrieval of unpicked snippets on demand to speed things up
 *
 *    Revision 1.7  2002/02/19 16:39:37  lucky
 *    Added options for BackgroundColor, HeaderLogo, FooterLogo
 *
 *    Revision 1.6  2002/02/01 16:57:44  lucky
 *    Added SacBufferLen
 *
 *    Revision 1.5  2001/08/06 21:28:23  lucky
 *    Added NT support
 *
 *    Revision 1.4  2001/07/28 00:43:14  lucky
 *    State of the code after debugging and testing prior to v6.0 release freeze.
 *
 *    Revision 1.3  2001/07/25 20:54:19  lucky
 *    Added MakeLMPreferred
 *
 *    Revision 1.2  2001/07/01 22:25:28  davidk
 *    removed some unneccessary comments.
 *
 *    Revision 1.1  2001/07/01 21:55:17  davidk
 *    Initial revision
 *
 *    Revision 1.2  2001/06/26 17:37:03  lucky
 *    State of the code after all Utah specs have been met
 *
 *    Revision 1.1  2001/06/21 21:26:25  lucky
 *    Initial revision
 *
 *    Revision 1.2  2001/05/15 18:47:36  davidk
 *    Moved functions around between the apps, DB API, and DB API INTERNAL
 *    levels.  Renamed functions and files.  Added support for amplitude
 *    magnitude types.  Reformatted makefiles.
 *
 *    Revision 1.1  2001/02/28 17:32:46  lucky
 *    Initial revision
 *
 *    Revision 1.1  2001/02/28 17:31:04  lucky
 *    Initial revision
 *
 *    Revision 1.1  2001/02/28 17:30:23  lucky
 *    Initial revision
 *
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
  

#include <map_display_structs.h>
#include <ewdb_ora_api.h>
#include <ew_event_info.h>
#include <sacputaway.h>
#include <sac_2_ewevent.h>
#include <html_common.h>


#define     ARC_MSG_LEN         100000

#define EQP_MAXWORD	         50
#define MAXPATH	       		512 

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

#define		MAX_REV_SRCS		10


/* End of External variables 
********************************/


typedef struct _StationEQStruct
{
  EWDB_ArrivalStruct *pPick;
  EWDB_StationMagStruct *pStaMag;
  EWDB_StationStruct *pStation;
} StationEQStruct;


/* Web parameters */
typedef struct _WebOptionsStruct
{
  int idEvent;
  int idOrigin;
  int ShowMap;
  int ShowUnpicked;
  int Action;
  int Type;
} WebOptionsStruct;

/* define some paramaterized macros for easier processing */
#define 	ABS(a) ((a) > 0 ? (a) : -(a))     /* Absolute value */
#define 	KMtoMILE(a) ((a)*.6214)           /* convert km to miles */
#define 	KM_PER_DEG 	100





/* Environment stuff */
extern char  WebHost[MAXPATH];
extern char  envEW_LOG[MAXPATH];        /* where variable EW_LOG will be stored */
extern char  envEW_PARAMS[MAXPATH];     /* where variable PARAMS will be stored */
extern char  DBservice[EQP_MAXWORD];    /* DBMS instance to interact with    */
extern char  DBuser[EQP_MAXWORD];       /* UserId to connect to database as  */
extern char  DBpassword[EQP_MAXWORD];   /* Password to datasource            */
extern char  EvtStructFile[MAXPATH];    /* Full path to the event struct binary file */
extern char  JavascriptFile[MAXPATH];   /* File containing Lomax Javascript */
extern char  SacFormat[MAXPATH];        /* Sparc or intel */
extern char  NetworkCode[EQP_MAXWORD];
extern char  WebDir[MAXPATH];
extern char  ReviewDir[MAXPATH];
extern char  WebTmpDir[MAXPATH];
extern char  ReviewTmpDir[MAXPATH];
extern char  HypoDir[MAXPATH];
extern char  HypoBin[MAXPATH];
extern char  HypoCfg[MAXPATH];
extern char  AlarmRing[MAXPATH];
extern char  MyModuleID[MAXPATH];
extern char  MyInstID[MAXPATH];
extern char  *RevSrcs[EQP_MAXWORD];
extern char  LM_writeWA_configfile[MAXPATH];
extern char  LM_review_configfile[MAXPATH];
extern char  LM_progname[MAXPATH];
extern char  LM_outputfile[MAXPATH];
extern char  LM_method[EQP_MAXWORD];
extern int   LM_LocalMagType;
extern int   NumRevSrcs;
extern int   ValidateQdds;
extern int   NumArrivalPicksToShow;
extern int   NumAmplitudePicksToShow;
extern int   MakeLMPreferred;
extern long  SacBufferLen;
extern double  TaperLength;
extern double  LowFreqTaper1;
extern double  LowFreqTaper2;
extern double  HighFreqTaper1;
extern double  HighFreqTaper2;
extern char BackgroundColor[MAXPATH];
extern char HeaderLogo[MAXPATH];
extern char FooterLogo[MAXPATH];
extern char HeaderTag[MAXPATH];
extern char FooterTag[MAXPATH];
extern int   Alarms_minPopulation;
extern int   Alarms_showPopulation;

/* Validate Qdds options */
extern double	MinDepth; 
extern double	MaxDepth; 
extern int		NumPhases; 
extern double	MaxGap; 
extern double	MaxDist; 
extern double	MaxRMS; 
extern double	MaxER0; 
extern double	MaxERH; 
extern double	MaxERZ; 
extern double	MinMag; 

typedef struct _codamagtest
{
	double	Mag;
	int		MinCodas;
} M_C_T;

#define		MAX_MCTEST	10
extern int		NumMCTest; 
extern	M_C_T	MC_Test[MAX_MCTEST];


#define MAX_SG2K_OPT        30
#define MAX_SG2K_PARAM      64
#define MAX_SG2K_VALUE    4096

typedef struct _SGOpt
{
    char    param[MAX_SG2K_PARAM];
    char    value[MAX_SG2K_VALUE];
} SGOpt;

extern SGOpt SeisGramOptions[];
extern int   NumSeisGramOptions;





extern int   DEBUG;                     /* debug flag */
extern int errno;   /* system error variable */


extern int iNumTGDs;             /* Number of "seismic display program
                            links to add at bottom of page */
extern TraceGifDisplayStruct TGDS[MAX_TGDS];

/* from review_function.h */
extern char  EventDir[MAXPATH];



/* function prototypes */
/*
static int     SetDefaultParams(SetVarsUserStruct *);
static int     Relocate (int);
static int     CheckSACHeaders (EWEventInfoStruct *, char *);
static void    html_summary (EWEventInfoStruct *);
static void    html_coincidence_summary (EWEventInfoStruct *);
static void    html_links2ora2rsec_gif (int);
static void    html_map (EWEventInfoStruct *, int);
static void    html_nomap (int);
static void    html_review_form (EWEventInfoStruct *);
static int      Retrieve3Components (char *, char *, EWEventInfoStruct *, int);  

extern	int 	SACPABase_end_scn_review (char *);
extern	int 	SetDefaultParams (SetVarsUserStruct *);
extern	int		DrawMap (gdImagePtr *, EWDB_StationStruct *, int, 
					EWDB_EventListStruct *, int, EQSPicture *, 
					LocationProjectionStruct *, int, float,
					char *, char *, char *, int *, int, int, int);

*/

int		GetMapFromMapServer(LocationProjectionStruct * pLPSData, 
                          MapServerImageStruct * pMSISdata);


/* Function prototypes
 *********************/

/*** FROM EQRconfig.c ***/
int ReadConfig(char * configfile);        /* reads configuration file */
void PrintEventInfo(EWEventInfoStruct *pEvent); /* dumps an EventInfo struct */

/*** FROM EQRreview_function.c ***/
int ReviewFunction (int idEvent, int idOrigin, int Action, 
								int ShowMap, int ShowUnpicked);
int compRange (const void* r1, const void* r2);
int PopulateReviewDir (EWEventInfoStruct *pEvent, char *evtDir, int Method);
void BuildAppletString(char *OutString, EWEventInfoStruct *pEvent,
                              char *webhost, char *channelnames);



/*** FROM EQRwebhelper.c ***/
int Webparse_Client_SetVars(char * szVar, char * szVal, void * pUserParams);


/*** FROM EXTERNAL LIBRARIES ***/
void logit_init( char *, short, int, int ); /* logit.c      sys-independent  */
void logit( char *, char *, ... );          /* logit.c      sys-independent  */

/* End of Function prototypes
 ****************************/



#ifdef _WINNT
#define DIR_SLASH 	'\\'
#else
#define DIR_SLASH 	'/'
#endif
