
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: getlist.h,v 1.7 2002/06/19 16:17:10 lucky Exp $
 *    Revision history:
 *
 *    $Log: getlist.h,v $
 *    Revision 1.7  2002/06/19 16:17:10  lucky
 *     Added support for displaying multiple mag types, beyond just Md and ML
 *
 *    Revision 1.6  2002/05/28 17:24:05  lucky
 *    *** empty log message ***
 *
 *    Revision 1.5  2001/09/26 21:43:40  lucky
 *    Added support for multi-screen event displays.
 *
 *    Revision 1.4  2001/04/13 21:30:19  lucky
 *    Changed MAXPATH to 512
 *
 *    Revision 1.3  2001/02/28 17:29:10  lucky
 *    Massive schema redesign and cleanup.
 *
 *    Revision 1.2  2000/08/07 18:38:22  lucky
 *    added MAXPATH 128
 *
 *    Revision 1.1  2000/05/31 16:57:10  lucky
 *    Initial revision
 *
 *    Revision 1.3  1999/11/09 16:53:06  lucky
 *    *** empty log message ***
 *
 *    Revision 1.2  1999/11/04 21:26:35  davidk
 *    added a #def for the number of points in a diamond.  Used for displaying
 *    different shapes for a EQ based on the depth of the quake.
 *
 *    Revision 1.8  1999/10/05 04:11:55  davidk
 *    fixed the 'change map' and 'view data' buttons, so that they were only active
 *    in the appropriate mode.  So if you are in the 'view data' mode already
 *    then the 'view data' button does nothign when you click it.  Also enabled
 *    easier color changes, better color constant names, and gray buttons even in X.
 *
 *    Revision 1.7  1999/10/05 00:35:47  bogaert
 *    changed colors for the mode backgrounds.
 *
 *    Revision 1.6  1999/10/04 18:40:31  alex
 *    alex: changed map title.
 *
 *    Revision 1.5  1999/10/01 23:46:24  davidk
 *    updated getlist interface to contain 2 color coded modes and included
 *    support for recentering a map
 *
 *    Revision 1.4  1999/09/24 06:16:51  davidk
 *    added config file params for maxevents,numberofdays,default click effect.  Done in conjunction
 *    >> with the change in getlist, to do away with the get_events.html page, which means incorporating
 *    >> criteria into the getlist program via the config file and html-displayed page
 *
 *    Revision 1.3  1999/06/04 15:58:26  lucky
 *    moved stuff into map_display_structs.h so that map drawing
 *    stuff can be completely separated from getlist.c
 *
 *    Revision 1.2  1999/06/02 21:43:40  lucky
 *    removed prototypes for functions now in draw_map.c
 *
 *    Revision 1.1  1999/05/05 18:19:24  lucky
 *    Initial revision
 *
 *
 */
  

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include <platform.h>
#include <ewdb_ora_api.h>
#include <ew_event_info.h>

/* For GIF drawing */
#include <gd.h>
#include <gdfontg.h>
#include <gdfonts.h>

#ifndef TRUE
# define TRUE 1
# define FALSE 0
#endif


#define RB_ALL_EVENTS 0 
#define RB_ALL_REVIEWED_EVENTS 1 
#define RB_UNREVIEWED_EVENTS 2 
#define RB_REVIEWEDBY_EVENTS 3 

#define ET_ALL_EVENTS 0 
#define ET_QUAKES 1 
#define ET_BLASTS 2 


#define BIG_BUFFER_SIZE 25000
#define STDIN_BUFFER_SIZE 500
#define MISC_BUFFER_SIZE 100


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

#define MAXPATH 	512

/* Conversion from Inches to Pixels for 
   postscript to GIF plotting conversion */
#define PIXELS_PER_INCH 72

/* Triangle Drawing Constants */
/* based on a triangle base of 6*sqrt(3) or 10.4 */
#define TRI_DIST_UP 4 /*6*/ /* x/(sqrt(3)) where x=base of triangle */
#define TRI_DIST_DOWN 2 /*3*/ /* x/(2*sqrt(3)) where x=base of triangle */
#define TRI_DIST_HORIZ 3 /*5*/ /* x*sqrt(3)/2 where x=base of triangle  should be 5.2 */
#define POINTS_IN_TRIANGLE 3
#define POINTS_IN_DIAMOND 4
/* End of Triangle Drawing Constants */

/* Some HTML settings including colors and map titles */
#define NUM_HTML_TABLE_COLUMNS 6
#define HTML_TABLE_COLOR1 "#2222AA"
#define HTML_TABLE_COLOR2 "#009988"
#define HTML_SPECIAL_TEXT_COLOR "#FFFFFF"
/* #define HTML_TABLE_COLOR1 "#CCFFFF"
   #define HTML_TABLE_COLOR2 "#FFFFCC"
   #define HTML_SPECIAL_TEXT_COLOR "#000000"
*/




#define HTML_GRAY_COLOR "#C0C0C0"
#define HTML_TITLE_COLOR1 "View Data: Click on event or station"
#define HTML_TITLE_COLOR2 "Change Map: Select zoom or re-Center, then click on map"

/*
   #define PRINT_GRAY_BUTTONS 1
*/
#define PRINT_GRAY_BUTTONS 0

typedef struct _GetListAPIReqStruct
{
   time_t StartTime;
   time_t EndTime;
} GetListAPIReqStruct;

#ifdef OLD_AUX
typedef struct _aux_event_struct
{
    int     NumPhases;
    int     NumSnippets;
    int     NumAlarms;
    int     NumAmpPicks;
    int     NumStas;
    EWDBid  idOrigin;
    double  Md;
    double  ML;
	char	sReadableSource[101];
} AuxEventStruct;
#endif OLD_AUX

typedef struct _aux_event_struct
{
    int					NumPhases;
    int					NumSnippets;
    int					NumAlarms;
    int					NumAmpPicks;
    int					NumStas;
	EWDB_MagStruct      Mags[MAX_MAGS_PER_ORIGIN];
	int         		iPrefMag;  /* Which of the magnitudes is preferred */
	int                 iNumMags;
    EWDBid				idOrigin;
    char				sReadableSource[101];
} AuxEventStruct;


/* Functions defined in getlist.c */
int SetDefaultParams(SetVarsUserStruct * pParams);

int GetMapFromMapServer(LocationProjectionStruct * pLPSData, 
                      MapServerImageStruct * pMSISdata);

int ConvertToOraAPIFormat(SetVarsUserStruct * pParams, 
                       GetListAPIReqStruct * pReq, int * pBufferSize,
                       EWDB_EventListStruct *pEVMin, EWDB_EventListStruct *pEVMax);

int ConvertToOraAPIFormatII (SetVarsUserStruct * pParams, 
                       GetListAPIReqStruct * pReq, int * pBufferSize,
                       EWDB_MergeList *pEVMin, EWDB_MergeList *pEVMax);

int CreateGUIStateStringFromWebParams(SetVarsUserStruct * pParams,
                                      SetVarsUserStruct * pDefaults,
                                      char * pStateString);

/* User Callout function SetVars, passes the user a Variable, it's string value,
   and a pointer to User Defined Params, so that the user can take whatever
   desired action.  Called by the web_common(webparse) library */
int SetVars(char * szVar, char * szVal, void * pUserParams);

/*  Perform any necessary processing or cleanup after all webparams have been
    read and processed via mutliple calls to SetVars(). */
int PostProcessWebParams(SetVarsUserStruct * pWebParams);

/* End functions defined in getlist.c */

/* Functions defined in get_map_thread.c */

thr_ret GetMapThread(void* dummy);

/* End functions defined in get_map_thread.c */


/* Functions defined in config.c   Doh!! */
int    ReadConfig( char *configfile );
EWDBMapStruct * GetMapFromID(char * szDefaultMapID);
int GetMapIDFromMapName(char * szMapName, char * szMapID, 
                        EWDBMapStruct * pMapCurr, char * szCurrentID);
int TraverseMapTree(EWDBMapStruct * pMapCurr, char * sznum);

/* End Functions defined in config.c   Doh!! */


/* Functions defined in draw_map.c*/

int		DrawMap (gdImagePtr *, EWDB_StationStruct *, int, EWDB_EventListStruct *, int, 
					EQSPicture *, LocationProjectionStruct *, int, float,
					char *, char *, char *, int *, int, int, int);

/* End Functions defined in draw_map.c */


int Pix2LatLon(LocationProjectionStruct * pLPSData,
               int XPix, int YPix,
               float* pLat, float* pLon);

/* External functions */
void   logit_init( char *prog, short mid, int bufSize, int logflag );
void   logit( char *, char *, ... );

int     Getlist_FillAuxInfo (AuxEventStruct *pAux,
                EWDB_EventListStruct *pEvent, int MinStasToShow);


/* External Variables */
extern SetVarsUserStruct WebParams;
extern float StationDisplayWidth; 
extern int GetStationListBufferSize;
