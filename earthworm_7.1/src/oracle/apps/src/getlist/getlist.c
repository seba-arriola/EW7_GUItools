
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: getlist.c,v 1.45 2002/09/10 17:28:34 lucky Exp $
 *    Revision history:
 *
 *    $Log: getlist.c,v $
 *    Revision 1.45  2002/09/10 17:28:34  lucky
 *    Stable scaffold
 *
 *    Revision 1.44  2002/06/19 16:16:59  lucky
 *     Added support for displaying multiple mag types, beyond just Md and ML
 *
 *    Revision 1.43  2002/05/28 19:34:19  lucky
 *    Added header and footer tag text
 *
 *    Revision 1.42  2002/05/28 17:23:45  lucky
 *    *** empty log message ***
 *
 *    Revision 1.41  2001/11/20 16:47:29  lucky
 *    NT fixes, also the eqparams page now comes up in a separate windows;
 *    Added support for paging through the list of events
 *
 *    Revision 1.40  2001/09/26 21:40:26  lucky
 *    Strong motion modifications;
 *    odifications to allow the event list to be broken up
 *     into smaller number of events;
 *    Allow separate maximum event counts - one for the map, another
 *    for the list;
 *
 *    Revision 1.39  2001/08/29 17:15:24  lucky
 *    *** empty log message ***
 *
 *    Revision 1.38  2001/07/28 00:43:53  lucky
 *     State of the code after debugging and testing prior to v6.0 release freeze.
 *
 *    Revision 1.37  2001/07/20 17:37:18  lucky
 *    Reformatted the display of picks into arrivals and amplitudes
 *
 *    Revision 1.36  2001/07/01 21:55:15  davidk
 *    Cleanup of the Earthworm Database API and the applications that utilize it.
 *    The "ewdb_api" was cleanup in preparation for Earthworm v6.0, which is
 *    supposed to contain an API that will be backwards compatible in the
 *    future.  Functions were removed, parameters were changed, syntax was
 *    rewritten, and other stuff was done to try to get the code to follow a
 *    general format, so that it would be easier to read.
 *
 *    Applications were modified to handle changed API calls and structures.
 *    They were also modified to be compiler warning free on WinNT.
 *
 *    Revision 1.35  2001/06/20 20:35:03  lucky
 *    Change ML magtypes to account for two separate types: peak2peak and zero2peak
 *
 *    Revision 1.34  2001/06/18 18:40:41  lucky
 *    Changed Ml to ML
 *
 *    Revision 1.33  2001/05/31 21:45:55  lucky
 *    Added display of ML. We retrieve all magnitudes for the prefered origin
 *    and determine which is Md and which ML.
 *
 *    Revision 1.32  2001/05/23 17:42:31  lucky
 *    Changed get_alarms to review_get_alarms
 *
 *    Revision 1.31  2001/05/18 21:16:09  lucky
 *    Changed references to eqreview to review_event.
 *
 *    Revision 1.30  2001/05/15 02:15:18  davidk
 *    Moved functions around between the apps, DB API, and DB API INTERNAL
 *    levels.  Renamed functions and files.  Added support for amplitude
 *    magnitude types.  Reformatted makefiles.
 *
 *    Revision 1.29  2001/04/13 21:29:22  lucky
 *    Added alarms section, and fixed up some #defines which conflicted with
 *    alarms.h definitions.
 *
 *    Revision 1.28  2001/03/26 23:01:40  lucky
 *    Modified calls to the Alarm system
 *
 *    Revision 1.27  2001/03/19 17:23:10  lucky
 *    Added the Alarms column
 *
 *    Revision 1.26  2001/02/28 17:29:10  lucky
 *    Massive schema redesign and cleanup.
 *
 *    Revision 1.25  2001/02/09 23:00:09  alex
 *    *** empty log message ***
 *
 *    Revision 1.24  2001/02/09 22:41:46  alex
 *    NoSnippetsNoShow mod. Alex
 *
 *    Revision 1.23  2001/01/23 17:09:22  lucky
 *    Added instruction lines for quick review and snippet view
 *
 *    Revision 1.22  2001/01/04 01:01:14  davidk
 *    Modified the code to utilize getimage program to retrieve/delete dynamically generated
 *    images on WinNT too.
 *
 *    Revision 1.21  2000/12/19 20:24:10  lucky
 *    Modified tumbler code so that the tuble file is created in a directory
 *    relative to the bin directory.
 *
 *    Revision 1.20  2000/11/20 18:49:37  lucky
 *    Added code to put up two more columns:
 *     1) number of picks with a link to the event review (??)
 *     2) number of snippets with a link to the ora2snippet_gif page
 *
 *    Revision 1.19  2000/09/20 16:41:00  lucky
 *    Cleanup many un-needed logit calls
 *
 *    Revision 1.18  2000/09/12 16:07:43  lucky
 *    Limited magnitudes to 1 significant digit
 *
 *    Revision 1.17  2000/09/11 19:04:49  lucky
 *    Changed table label from Long to Lon.
 *    Lat and Lon are displayed with 2 significant digits.
 *    Depth is displayed with 1 significant digit.
 *
 *    Revision 1.16  2000/08/09 16:21:40  lucky
 *    Lint cleanup
 *
 *    Revision 1.15  2000/08/07 19:39:54  lucky
 *    Added WebHost option and using html_trailer from libsrc
 *
 *    Revision 1.14  2000/07/24 21:04:21  lucky
 *    *** empty log message ***
 *
 *    Revision 1.13  2000/05/31 16:48:53  lucky
 *    Modified to reuse code in the review/alarm applications
 *
 *    Revision 1.12  2000/03/30 16:04:50  davidk
 *    added code that sorts the event list based on the Origin time.  This is
 *    neccessary when getlist has to split the world in half and query for two
 *    sets of data, otherwise one data set gets placed after the other.  This
 *    works as a merge of the two data sets.  The two data sets are neccessary
 *    because of the 540 degree world problem.
 *
 *    Revision 1.10  2000/03/03 06:22:26  davidk
 *    fixed the URL for the tumbler.  Previously it was hardcoded to
 *    zappa.  It is now a relative URL, so should work on other machines
 *    and other directories.
 *
 *    Revision 1.9  2000/02/22 17:09:09  lucky
 *    Fixed the path to the tumbler -- was broken due to new directory structure
 *
 *    Revision 1.8  2000/02/09 21:21:55  davidk
 *    added function prototype for CreateTumbleFile()
 *      initialized NumEventsRetrieved and NumEventsFound to 0.
 *
 *    Revision 1.7  1999/11/19 16:35:47  lucky
 *    Added the earthquake tumbler code
 *
 *    Revision 1.6  1999/11/09 16:53:02  lucky
 *    *** empty log message ***
 *
 *    Revision 1.5  1999/11/04 21:25:52  davidk
 *    fixed a commend for the last version
 *
 *    Revision 1.4  1999/11/04 21:20:47  davidk
 *    fixed a bug with the checkboxes where they were modifying the HELP hyperlink on the
 *    html page instead of the Map hyperlink.  Modified getlist 2 display a different
 *    legend based on the size of the map, so that the eq sizes in the legend corresponded
 *    to the eq sizes on the map.
 *
 *    Revision 1.2  1999/10/28 19:53:24  davidk
 *    fixed portion of code that disects the map area and retrieves relevant event and station-lists
 *    from the DB, even if the map crosses the 180/-180 Lon line.
 *
 *    Revision 1.1  1999/10/25 18:52:32  davidk
 *    Initial revision
 *
 *    Revision 1.14  1999/10/05 04:35:27  davidk
 *    changed the background of the page to White from 'nothing'
 *
 *    Revision 1.12  1999/10/05 00:35:28  bogaert
 *    changed font to black on titles
 *
 *    Revision 1.11  1999/10/04 18:39:51  alex
 *    Alex: changed button title.
 *     .
 *
 *    Revision 1.10  1999/10/01 23:46:24  davidk
 *    updated getlist interface to contain 2 color coded modes and included
 *    support for recentering a map
 *
 *    Revision 1.9  1999/09/24 06:16:51  davidk
 *    added config file params for maxevents,numberofdays,default click effect.  Done in conjunction
 *    >> with the change in getlist, to do away with the get_events.html page, which means incorporating
 *    >> criteria into the getlist program via the config file and html-displayed page
 *
 *    Revision 1.7  1999/07/20 15:25:43  lucky
 *    Added ability to send email to the webmaster
 *
 *    Revision 1.6  1999/06/24 00:08:06  lucky
 *    Changed End_Time and End_Date to Finish_Time and Finish_Date.
 *    This makes it possible to zoom in and out and view events from the past.
 *
 *    Revision 1.5  1999/06/23 22:43:51  lucky
 *     Added Debug flag to the config file
 *
 *    Revision 1.4  1999/06/04 17:58:04  lucky
 *     more minor adjustments needed to make DrawMap work with other modules.
 *
 *    Revision 1.3  1999/06/04 15:58:06  lucky
 *    moved code into draw_map.c to separate DrawMap functionality
 *
 *    Revision 1.2  1999/06/02 21:38:48  lucky
 *    Took out all of the code which drew events and stations on
 *    top of a GMT generated map. This code is now in draw_map.c
 *    and is accessed via a single call to DrawMap.
 *
 *    Revision 1.1  1999/05/05 18:19:24  lucky
 *    Initial revision
 *
 *
 */
  
/* filename:       getlist.c 
   module:         getlist
   platforms:      solaris 2.7 (& maybe NT 4)
   date:           04/15/1999
   bug creator:    David Kragness

   description:   

   getlist is a program that provides customized maps with 
   earthquakes.  The maps are currently generated by GMT
   a set of map tools developed by Smith & Wessel and is
   available from http://www.soest.hawaii.edu/gmt/.  To my knowledge,
   Smith & Wessel of GMT fame are not related to Smith &
   Wesson, who have developed a set of different tools.

   GMT is accessed via a set of MapServer functions in the
   mapserver.o object.

   The earthquakes are drawn on the map using the GD Lib tools
   developed by Tom Boutell, available at  http://www.boutell.com/gd/.
   The GD Library tools are accessed via C API functions within getlist.

   getlist uses the EWDB C API to get at a list of earthquakes,
   stations, and parameters, that it uses to determine how and where
   to display Quakes and Stations.  

   getlist is meant to be used with a webserver, and as such outputs
   everything in html, with some javascript for easier web-browser 
   implementation.  It utilizes GET/POST CGI-BIN variables to maintain
   state information.

   getlist uses a set of webparsing functions for EWDB with
   source in src/web_common


  TOPICS:

  Map Trees:
  
    At some point, there was a request to have access
    to multiple Preset maps, the way you have several preset stations
    on your car radio, in the hopes that you won't have to waste a lot
    of your time getting from a single default map to your favorite 
    maps.  The vision, as I understand it was not just to have a list
    of several maps, but to have a hierarchal tree of maps, with the
    world at the trunk, and Parkfield, CA at the end of of one of the
    branches.  This resulted in the creation of a map tree, which could
    be traversed up and down as desired by the user.  This map tree is
    implemented as two structs, one of which contains actual map data
    for a given map, and the other which contains the position of a
    map in relation to other maps.  Terms like children, parent, and
    siblings are used.  The structures and tree were implemented quite
    quickly and with limited clarity.  Use them with care.  The two
    structs are EWDBMapDataStruct and EWDBMapStruct, both defined in 
    map_display_structs.h.  A map can be identified by an ID, or by
    its name.

  WebParams:

    There is a lot of parametric constraint data that possibly needs
    to be passed from one instance of getlist to the next, or atleast
    needs to persist through one instance of getlist.  This data
    spans four classifications:  Event Criteria as defined in the
    get_events.html page;  Map Location and Projection as defined in
    the Map config file entry and by the users past map clicks.  The
    Web GUI State information that tracks the current GUI checkbox
    selection and the latest map click;  MapServer image data, which
    tracks what type of image is generated by the mapserver, and what
    it is called.  Each of these classifications of data is stored in
    its own C structure.  The culmination of these structures is stored
    in a Single struct called a SetVarsUserStruct.  There exist two
    instances of this struct, one contains only the default hard coded
    params and is called DefaultParams.  The other contains the current
    valid values and is named WebParams.  See map_display_structs.h for
    definitions of all these structs and data classifications.  Some of
    them may even be explained.

  Marking Start and End Times:

    Throughout the code there are little pseudo profiling statements 
    that check the wall clock time required to execute a portion of 
    code.  The portions are usually logically grouped.  These statements
    are a nice quick way of locating problems when things get slow. 
    Especially as we change schemas and processing this should allow us
    to get a good look at performance changes.

   Let's get started!!!!
  *************************/

/* include a bunch of header files for external functions */
#include <time.h>
#include <time_ew.h>  /* for profiling timing */
#include <time_functions.h>
#include <map_display_structs.h>
#include <getlist.h>
#include <mapserver.h>
#include <webparse.h>
#include <html_common.h>

#include <ewdb_ora_api.h>
#include <ewdb_apps_utils.h>
#include <alarms.h>

/************* END OF #INCLUDES *************/

/************** BEGIN #DEFINES **************/

/* ME */
#define MY_PROG_NAME "getlist"


/* Relative locations of neccessary files. */
#define GL_CONFIG_FILE "../params/getlist.d"

#define GETLIST_MAP_ANCHOR_POSITION 1 /* javascript: index of the Map in the links array for the getlist page */



/************* END OF #DEFINES  *************/

/************* EXTERNALLY DEFINED FUNCTIONS *************/

/* Defined in earthworm.h */
void CreateSpecificMutex( mutex_t * ); 
int  StartThread( thr_ret (void *), unsigned, unsigned * );
void RequestSpecificMutex( mutex_t * );
void ReleaseSpecificMutex( mutex_t * );

/* The following #ifdef is only for a function prototype, don't get excited! */
#ifdef _WINNT  /* so that compiler doesn't warn of underined getcwd */
 /* defined in direct.h as part of CRTLIB */
 char * getcwd( char *buffer, int maxlen );
#endif

/********** END OF EXTERNALLY DEFINED FUNCTIONS *************/


/********** GLOBAL VARIABLES *************/

int		DEBUG = 0;

/* No default passwords.  We gotta have a little security */
char DBuser[50]="user";
char DBpassword[50]="password";
char DBservice[50]="database";
char WebHost[MAXPATH];

char envEW_LOG[MAXPATH];

char BackgroundColor[MAXPATH];
char HeaderLogo[MAXPATH];
char FooterLogo[MAXPATH];
char HeaderTag[MAXPATH];
char FooterTag[MAXPATH];

int	ShowPickCol;
int	ShowStasCol;
int	ShowTraceCol;
int	ShowAlarmsCol;
int	ShowCoincidenceLink;


int  MyModID=1, LogSwitch=1;
int MinNumStasToShow = 0;
int EventsPerPage = -1;
int white,blue,red,yellow,black,triangle_color;
gdImagePtr im_out;

/* PrintWarningMessages is currently set by the FirstPage web flag,
   so that warning messages are only printed on the first page. 04/29/99 DK */
int PrintWarningMessages=0;
  

/********** THREAD STUFF GLOBAL VARIABLES  ********/
/* Copied this outta earthworm*/
#define THREAD_STACK  8096

unsigned int tidGetMapThread;
unsigned int tidGetQuakeListThread;
mutex_t MapServerMutex;
mutex_t DBMSMutex;
/********** END THREAD STUFF GLOBAL VARIABLES  ********/


/* Params I may be passed from the web browser/server.
   Filled out by SetVars().  */
SetVarsUserStruct WebParams;

/* Default values for the params. */
SetVarsUserStruct DefaultParams;

/* MaxWidth of the map in Lon degrees for which stations are 
   to be displayed.  Can be overridden in the config file with
   MaxStationDisplayWidth flag */
float StationDisplayWidth=2.0;

/*  The size of the StationListBuffer in terms of EWDB_StationStructs, that should
    be passed to GetStationList().  Config fileable with
    MaxNumOfStationsDisplayed flag */
int GetStationListBufferSize=400;

/********** MAP TREE GLOBAL VARIABLES  ********/
/* The root Map, which all others are hanged from (maybe). */
EWDBMapStruct MapRoot;

/* The ID of the default map, as loaded from the config file. 
   via the DefaultMapID flag*/
char szDefaultMapID[MAX_MAPID_LENGTH]="1";

/* The current map parameters */
char szCurrentMapID[MAX_MAPID_LENGTH];
EWDBMapDataStruct CurrentMap;
EWDBMapStruct * pDefaultMap;

/********* END OF MAP TREE GLOBAL VARIABLES  ********/

/*************** END GLOBAL VARIABLES  **************/

int CreateTumbleFile (EWDB_EventListStruct *pEvents, 
                      int NumEvents, char *filename);

int ComparEvents_qsort(const void * pEvent1, const void * pEvent2);


#define		DISPMODE_NORMAL			0
#define		DISPMODE_SHOWALL		1
#define		DISPMODE_ENDPAGE		2
#define		DISPMODE_TOPPAGE		3

int main(int argc, char ** argv)
/******************************************************
  Function:    main()
  Purpose:     main() is the main function.  It calls all
               neccessary support functions to get you a
               map of earthquakes (and stations) on top
               of the region that you request.  It optionally
               will return you a list of events along with the
               map, any of which you can have SAC'd.
  Parameters:    
      Input
      argc:   Number of command like arguments
      argv:   Array of pointers to command line arguments

  Author:
               DK, before 04/15/1999


  **Note:  This is a mother of a function and should probably 
    be shrunk.  But alas there are more important things to
    do right now that redesigning code.
  **********************************************************/
{

	EWDB_EventListStruct 	EVMin,EVMax;  
	EWDB_EventListStruct 	*pEventBuffer;
	EWDB_StationStruct 		*pStationListBuffer;
	EWDBMapDataStruct 		*Maps[MAX_MAP_CHILDREN+2];
	EWDBMapStruct 			*pCurrMap, *pMapTemp;
	EWEventInfoStruct		EventInfo;
	GetListAPIReqStruct 	EventReq;
	AuxEventStruct 			*AuxEventInfo;
	EQSPicture 				Pic={{0,0,0,0},{0.0,0.0,0.0,0.}};
	int						m, l, j, reverse, EventsToMap, rowSpan;
	int 					GetEventListBufferSize;
	int 					RetVal, i, NumOfSiblings, legendnum;
	int 					MapCount, MapsCtr,ChildCtr;
	int 					NumEventsRetrieved=0, NumEventsFound=0, NumEventsDisplayed=0;
	int 					NumStationsFound=0, NumStationsRetrieved=0;
	int   					TimesToRotateTheEarth;
	int   					NumEventsFound1,NumEventsRetrieved1;
	int   					NumStationsFound1,NumStationsRetrieved1;
	int						evtStart, evtEnd, evtDispMode;
	long 					tStationListTime;
	time_t 					TempTime;
	float 					MinLon1,MaxLon1,MinLon2,MaxLon2;
	char  					*AreaBuf, *AreaBuf2;
	char  					pTimeBuffer[MISC_BUFFER_SIZE];
	char 					szTemp[40],szTemp2[40], szBackgroundColor[10];
	char 					szTitle[128], filename[256];
	

  /* Send header of reply back to web server */
  printf("Content-type: text/html\n\n");
  printf("<html>\n");

  /* Malloc Buffers for html image map strings*/
  AreaBuf=malloc(1300000);
  AreaBuf2=malloc(1300000);
  if(!(AreaBuf && AreaBuf2))
  {
    printf("getlist:  Could not malloc "
               "Areabuffers (300k bytes each)");
    return(-1);
  }
  else
  {
    /* Initialize to blank */
    AreaBuf2[0]=AreaBuf[0]=0;
  }

  /* Initialize EVMax and EVMin to blank */
  memset(&EVMax,0,sizeof(EWDB_EventListStruct));
  memset(&EVMin,0,sizeof(EWDB_EventListStruct));


  /* Set Default web and config file params */
  if(SetDefaultParams(&DefaultParams))
  {
    fprintf(stdout,"Failed to set Default Params.\n");
    return(-1);
  }

  /* See "Map Trees" in the intro for information on the Map tree*/
  /* Initialize the root map structure */
  MapRoot.NumOfChildren=0;
  MapRoot.pParent=NULL;
  strcpy(MapRoot.Map.MapName,"Root:NotAMap");
  /* End of Initialize the root map structure */

  /* Read parameters from the config file. */
  if(ReadConfig(GL_CONFIG_FILE))
  {
    fprintf(stdout,"Read of config file failed. Filename: %s\n",GL_CONFIG_FILE);
    return(-1);
  }

  /* Now Copy the defaults to the struct that will be configured. */
  memcpy(&WebParams,&DefaultParams,sizeof(SetVarsUserStruct));

  /* This must be for performance testing DK 990125 ??? */
  EWDB_time_init();

  /* Initialize logging */
  logit_init(MY_PROG_NAME,(short)MyModID,1024,LogSwitch);




  /* Setup the default map, and default map params. */
  pDefaultMap=GetMapFromID(szDefaultMapID);
  if(!pDefaultMap)
  {
    html_logit("","GetMapfromID failed.\n");
    return(-1);
  }

  /* Copy the default map parameters into WebParams */
  /* See WebParams in the intro */
  memcpy(&(WebParams.LPSData.MapData), &(pDefaultMap->Map),
         sizeof(EWDBMapDataStruct));
  strcpy(WebParams.LPSData.MapID,szDefaultMapID);


  /* Get and process web params */
  if(Webparse_GetAndProcessWebParams((void *) &WebParams))
  {
    /* If the call bombed, write an html letter to our congressman 
       explaining what happened. Logit as well. 
    ********************************************/
    html_logit("","Unable to process params from the Web browser.\n");
    return(-1);
  }


  /* Take Care of any web parameter cleanup needed to ensure everything
     is situated.  This is a good time to do any summary or other work
     that needs to be done after all web parameters have been collected
     from the CGI interface.  */
  PostProcessWebParams(&WebParams);

  if(WebParams.LPSData.bResetLatLon)
  {
    /* We got here because somebody clicked on a map name to the
       side of the existing map.
       What we need to do is overwrite the current MapData struct
       in LPSData, with the MapData struct in the record of the map
       selected. */
    /* First get the MapID of the Map selected */
    if(GetMapIDFromMapName(WebParams.LPSData.MapData.MapName,
                        WebParams.LPSData.MapID, &MapRoot,""))
    {
      html_logit("","GetMapIDFromMapName() failed in main() while trying to get "
               "selected map: %s.\n",WebParams.LPSData.MapData.MapName);
      return(-1);
    }
    /* Then get the Map from the MapID. It would of been much more efficient
       just to get the Map from the MapName, but oh well! now */
    if((pMapTemp=GetMapFromID(WebParams.LPSData.MapID))== NULL)
    {
      html_logit("","Error in main, while trying to get selected map, "
               "with MapID %s, derived from MapName %s.\n",
            WebParams.LPSData.MapID,WebParams.LPSData.MapData.MapName);
      return(-1);
    }
    /* Now we've got the map, lets set things right */
    /* We've got a pointer to the new map in pMapTemp */
    memcpy(&(WebParams.LPSData.MapData),&(pMapTemp->Map),
           sizeof(EWDBMapDataStruct));

    /* Now set bResetLatLon to FALSE */
    WebParams.LPSData.bResetLatLon=FALSE;
  } /* end of the WebParams.LPSData.bResetLatLon saga */


  /* This is where we would like to parallelize in the interests of saving
     time.  */

  /* Create Mutexes for tracking completion the Mapserver thread.
     No need to create a mutex for the DBMS thread because the DBMS
     work will be executed by the main(this) thread  */
   CreateSpecificMutex(&MapServerMutex);

  /* Start the Map Server Thread */
  /***********************************/
  if (StartThread(GetMapThread, (unsigned)THREAD_STACK, &tidGetMapThread) == -1)
	{
    html_logit("", "getlist: error starting GetMapThread. Exiting.\n");
    exit(-1);
  }

  if(ConvertToOraAPIFormat(&WebParams,&EventReq,&GetEventListBufferSize,
                            &EVMin,&EVMax) != 0)
  {
    /* Write an html letter to our congressman explaining what happened. 
       Logit as well. 
    ********************************************/
    html_logit("","Unable to convert Web browser info to event list request form.\n");
    return(-1);
  }

	GetEventListBufferSize = WebParams.ECSData.MaxEventsRetr;


  if((pEventBuffer = (EWDB_EventListStruct *)
	malloc (GetEventListBufferSize	* sizeof(EWDB_EventListStruct))) == NULL)
  {
    html_logit("","Unable to allocate space for retrieval of %d Events.\n",GetEventListBufferSize);
    return(-1);
  }


  if((pStationListBuffer = (EWDB_StationStruct *)
      malloc(GetStationListBufferSize*sizeof(EWDB_StationStruct)))
     == NULL)
  {
    html_logit("","Unable to allocate space for retrieval of %d Stations.\n",
          GetStationListBufferSize);
    return(-1);
  }

  /*****************************************************************************/
  /*****************************************************************************/
  /*****************************************************************************/
  /*****************************************************************************/
  /**************** NEW CODE SEGMENT GOES HERE!!!!!!!!!! ***********************/


  /* Fix the CenterLon and LonWidth params from 
     WebParams.LPSData.MapData
     in case they came in all out of whack 
  ******************************************/
  if(WebParams.LPSData.MapData.CenterLon < -180)
  {
    TimesToRotateTheEarth = (int)(WebParams.LPSData.MapData.CenterLon - 180)/360;
  }
  else if (WebParams.LPSData.MapData.CenterLon > 180)
  {
    TimesToRotateTheEarth = (int)(WebParams.LPSData.MapData.CenterLon + 180)/360;
  }
  else  /* -180 to 180 */
  {
    TimesToRotateTheEarth=0;
  }  /* end if(CenterLon) */

  WebParams.LPSData.MapData.CenterLon -= TimesToRotateTheEarth*360;

  if(WebParams.LPSData.MapData.LonWidth > 360)
  {
    html_logit("","The Longitude size of the requested map was %.2f, "
             "based on a starting Lon of %.2f and an ending of %.2f.\n"
             "Please confine your queries to one planet at a time.\n"
             "Limiting Lon Width to 360 deg.\n",
          WebParams.LPSData.MapData.LonWidth,
          WebParams.LPSData.MapData.CenterLon + WebParams.LPSData.MapData.LonWidth/2,
          WebParams.LPSData.MapData.CenterLon - WebParams.LPSData.MapData.LonWidth/2
         );
    WebParams.LPSData.MapData.LonWidth = 360;
  }


  /* Create EQSPicture coordinates from MSISData */
  /* EQSPicture is just a structure that holds the location
     of a map in terms of both degrees and pixels */
  Pic.deg[PIC_LEFT]=  WebParams.LPSData.MapData.CenterLon
                       - (WebParams.LPSData.MapData.LonWidth/2);
  Pic.deg[PIC_TOP]=   WebParams.LPSData.MapData.CenterLat
                       + (WebParams.LPSData.MapData.LatHeight/2);
  Pic.deg[PIC_RIGHT]=  WebParams.LPSData.MapData.CenterLon
                       + (WebParams.LPSData.MapData.LonWidth/2);
  Pic.deg[PIC_BOT]=   WebParams.LPSData.MapData.CenterLat
                       - (WebParams.LPSData.MapData.LatHeight/2);
  Pic.pix[PIC_LEFT]=  0;
  Pic.pix[PIC_TOP]=   0;
  Pic.pix[PIC_RIGHT]= WebParams.LPSData.PixelWidth;
  Pic.pix[PIC_BOT]=   WebParams.LPSData.PixelHeight;

  /* Now we get to the good stuff.  Entering DB interaction */
  if(DEBUG) 
    logit("","Connecting to the database.");

  /* Initialize the Ora_API */
  ewdb_api_Init(DBuser,DBpassword,DBservice);

  /* Get the list of events based upon the criteria that we copied
     in from the web.
  **********************************************/

  /********** THE 540 DEGREE WORLD PROBLEM ***********/
  /* Now before we religiously call GetEventList, we have
     to do some fix'n to get the right earthquakes.  */
  /* The world according to DB lasts from -180 to 180 Lon 
     and so we need to make sure our map is adjusted 
     approriately  */
  /* If our query threatens to fall of the edge of the
     earth (180/-180), then we must segment it, query
     as two, and then reconstitute it. DK 040999 */


  /* Alright, now we've got a map to draw, and it is 
     guaranteed to be within (-360,360), which is
     the famed 540 degree world, give or take 180 degrees. */

  /*  This causes a bug.  04/27/99.  We shouldn't be modifying
      LPSData here, this is only criteria data.  We could modify
      LPSData someplace else so that we stay within the 540 degree
      world.  DAvidk 19990427 */
  /* WebParams.LPSData.MapData.CenterLon=
         OrigMaxLon - ((OrigMaxLon-OrigMinLon)/2);
  */


  if(WebParams.LPSData.MapData.CenterLon - 
      (WebParams.LPSData.MapData.LonWidth/2) < -180)
  {
    MinLon1 = WebParams.LPSData.MapData.CenterLon - 
               (WebParams.LPSData.MapData.LonWidth/2) + 360;
    MaxLon1 = 180;
    MinLon2 = -180;
    MaxLon2 = WebParams.LPSData.MapData.CenterLon + 
                (WebParams.LPSData.MapData.LonWidth/2);
  }
  else if(WebParams.LPSData.MapData.CenterLon + 
           (WebParams.LPSData.MapData.LonWidth/2) > 180)
  {
    MaxLon1 = WebParams.LPSData.MapData.CenterLon + 
           (WebParams.LPSData.MapData.LonWidth/2) - 360;
    MinLon1 = -180;
    MaxLon2 = 180;
    MinLon2 = WebParams.LPSData.MapData.CenterLon - 
           (WebParams.LPSData.MapData.LonWidth/2);
  }
  else
  {
    MaxLon1 = WebParams.LPSData.MapData.CenterLon + 
           (WebParams.LPSData.MapData.LonWidth/2);
    MinLon1 = WebParams.LPSData.MapData.CenterLon - 
           (WebParams.LPSData.MapData.LonWidth/2);
    MaxLon2 = -360;
    MinLon2 = -360;
  }

  EVMax.dLon=MaxLon1;
  EVMin.dLon=MinLon1;

  /* Call Getlist with the first set of Lon coordinates */
  if(DEBUG)
  {
    logit("","Calling GetEventList() Lat,Lon (%.2f,%.2f)\n"
          "                       Lat,Lon (%.2f,%.2f)\n"
          "pEventBuffer %u, NumEventsRetrieved %d,"
          "NumEventsFound %d.\n",
          EVMin.dLat,EVMin.dLon,EVMax.dLat,EVMax.dLon,
          pEventBuffer,NumEventsRetrieved,NumEventsFound);
  }

  RetVal=ewdb_api_GetEventList(pEventBuffer,GetEventListBufferSize,
                           EventReq.StartTime,EventReq.EndTime,
                           &EVMin,&EVMax, &NumEventsRetrieved);

  if(DEBUG)
  {
    logit("","GetEventList done, Retval=%d, found=%d, retr=%d.\n",
			RetVal, NumEventsFound, NumEventsRetrieved);
  }

  if(RetVal < 0)
  {
    if(RetVal == -1)
    {
      html_logit("","GetEventList() failed!  See logfile for details.\n");
      return(-1);
    }
    else  /* Retval != -1, (and < 0) */
    {
      NumEventsFound=0-RetVal;
    }
  }
  else /* Retval >= 0 */
  {
    NumEventsFound=RetVal;
  }  /* end else if Retval < 0 */
    
  if(!(MinLon2==MaxLon2 && MaxLon2==-360))
  {
    EVMax.dLon=MaxLon2;
    EVMin.dLon=MinLon2;

    /* Call Getlist with the 2nd set of Lon coordinates */
    RetVal=ewdb_api_GetEventList(pEventBuffer+NumEventsRetrieved,
                        GetEventListBufferSize-NumEventsRetrieved,
                        EventReq.StartTime,EventReq.EndTime,
                        &EVMin,&EVMax, &NumEventsRetrieved1);

    if(RetVal < 0)
    {
      if(RetVal == -1)
      {
        html_logit("","GetEventList() #2 failed!  See logfile for details.\n");
        return(-1);
      }
      else  /* Retval != -1, (and < 0) */
      {
        NumEventsFound1=0-RetVal;
      }
    }
    else /* Retval >= 0 */
    {
      NumEventsFound1=RetVal;
    }  /* end else if Retval < 0 */

    /* Get the total NumEventsFound and NumEventsRetrieved by 
       adding results from query1 and query 2 */
    NumEventsRetrieved+=NumEventsRetrieved1;
    NumEventsFound+=NumEventsFound1;

    /* We've got two sets of eventlsts in pEventBuffer, we need to integrate them
       based on tOrigin
    *******************************************************************/
    qsort((void *)pEventBuffer,NumEventsRetrieved,
          sizeof(EWDB_EventListStruct),ComparEvents_qsort);

  }  /* end if(!(MinLon2=MaxLon2=-360)) (do 2nd getlist) */



  /* If our map is of the prescribed size and the station display
     switch is set to auto, or if the station display switch is set
     to on, then go get a list of stations to show. */
  if(((Pic.deg[PIC_TOP]- Pic.deg[PIC_BOT] <= StationDisplayWidth)
	  && (WebParams.WGSSData.StationClickEffect == EWDB_STATION_DISPLAY_DEFAULT)
     )
	  || WebParams.WGSSData.StationClickEffect == EWDB_STATION_DISPLAY_STATIONS
	)
  {
    if((int)EventReq.EndTime)
      tStationListTime = (int)(EventReq.EndTime);
    else if((int)EventReq.StartTime)
      tStationListTime = (int)(EventReq.StartTime);
    else
      time(&tStationListTime);

    /* Calling GetStationList with the first set of Lon Coordinates */
    if(DEBUG)
    {
      logit("","Entering GetStationList. NT %.2f, XT %.2f, NN %.2f, XN %.2f,RT %.2f, BufSize %d\n",
           Pic.deg[PIC_BOT], Pic.deg[PIC_TOP], MinLon1,
           MaxLon1,(double)tStationListTime,GetStationListBufferSize);
    }

    RetVal=ewdb_api_GetStationList(Pic.deg[PIC_BOT], Pic.deg[PIC_TOP],MinLon1,
                          MaxLon1, (double)tStationListTime,pStationListBuffer, 
                          &NumStationsFound, &NumStationsRetrieved,
                          GetStationListBufferSize);

    if(DEBUG)
    {
      logit("","After GetStationList(), RetVal=%d, NSR=%d\n",RetVal,
            NumStationsRetrieved);
    }

    if(RetVal == EWDB_RETURN_FAILURE)
    {
      html_logit("","GetStationList() failed!  See logfile for details.\n");
      return(-1);
    }

    /* if we need to do another station search with a 2nd set 
       of coordinates due to the 540 deg world 
    ********************************************/
    if(!(MinLon2==MaxLon2 && MaxLon2==-360))
    {
      /* Calling GetStationList with the 2nd set of Lon Coordinates */
      if(DEBUG)
      {
        logit("","Entering GetStationList. NT %.2f, XT %.2f, NN %.2f, XN %.2f,RT %.2f, BufSize %d\n",
              Pic.deg[PIC_BOT], Pic.deg[PIC_TOP], MinLon2,
              MaxLon2,(double)tStationListTime,GetStationListBufferSize);
      }
      
      RetVal=ewdb_api_GetStationList(Pic.deg[PIC_BOT], Pic.deg[PIC_TOP],MinLon2,
                                     MaxLon2, (double)tStationListTime,
                                     &(pStationListBuffer[NumStationsRetrieved]), 
                                     &NumStationsFound1, &NumStationsRetrieved1,
                                     GetStationListBufferSize-NumStationsRetrieved);

      if(DEBUG)
      {
        logit("","After GetStationList(), RetVal=%d, NSR=%d\n",RetVal,
              NumStationsRetrieved1);
      }

      if(RetVal == EWDB_RETURN_FAILURE)
      {
        html_logit("","GetStationList() failed!  See logfile for details.\n");
        return(-1);
      }

      /* Get the total NumEventsFound and NumEventsRetrieved by 
         adding results from query1 and query 2 */
      NumStationsRetrieved+=NumStationsRetrieved1;
      NumStationsFound+=NumStationsFound1;
    }  /* end if(!(MinLon2=MaxLon2=-360)) (do 2nd getstalist) */

  } /* end if stations should be displayed */

  /*****************************************************************************/
  /*****************************************************************************/
  /*****************************************************************************/
  /*****************************************************************************/
  /*****************************************************************************/

  /* Initialize the number of maps we've already grabbed to 0.
     We display a bunch of map names next to the side of the
     current map, so that the user can jump to them.  We want
     to display as many maps as is convenient, but never more
     than  NumOfChildren+2 */
  MapCount=0;

  /* By default we have no siblings, because we don't neccessarily have
     a valid parent, although we really should.  At any rate , just set
     the number of siblings to be 0 so that it can never come back and
     bite us.  */
  NumOfSiblings=0;

  /* Get a pointer to the current EWDBMapStruct from LPSData */
  pCurrMap=GetMapFromID(WebParams.LPSData.MapID);
  if(!pCurrMap)
  {
    html_logit("","MapID %s not found, using MapRoot.\n",WebParams.LPSData.MapID);
    pCurrMap=&MapRoot;
  }

  if(pCurrMap->pParent == NULL)
  {
    html_logit("","Error: We think MapID %s refers to the root map. Continuing!\n",
          WebParams.LPSData.MapID);
  }
  else
  {
    if(pCurrMap->pParent->pParent == NULL)
    {
      /* we are on one of the base maps, and cannot point to parent */
    }
    else
    {
      /* grab the parent map */
      Maps[MapCount]=&(pCurrMap->pParent->Map);
      MapCount++;
    }
    /* figure out how many cyblings we can grab */
    NumOfSiblings=MAX_MAP_CHILDREN+2 - MapCount -1 - pCurrMap->NumOfChildren;
    /* Number of array elements: MAX_MAP_CHILDREN
       Number of spots used already: MapCount
       Number of spots reserved for current map: 1
       Number of spots reserved for children: pCurrMap->NumOfChildren
    */
    if(NumOfSiblings > pCurrMap->pParent->NumOfChildren)
    {
      NumOfSiblings = pCurrMap->pParent->NumOfChildren;
    }
  }

  /* Add the proper siblings */
  ChildCtr=0;
  for(MapsCtr=MapCount; MapsCtr< MapCount+NumOfSiblings; MapsCtr++)
  {
    while(ChildCtr < MAX_MAP_CHILDREN && pCurrMap->pParent->pChildren[ChildCtr]== NULL)
      ChildCtr++;

    if(pCurrMap->pParent->pChildren[ChildCtr]== NULL)
    {
/*
      logit("","Error, problem with children for parent of %s.\n",
            WebParams.LPSData.MapID);
*/
      break;
    }
    else
    {
      if(pCurrMap->pParent->pChildren[ChildCtr] == pCurrMap)
      {
        /* This is the current map, so it shouldn't get listed as a sibling */
        MapsCtr--;  /* Do this so that we don't skip a spot as we
                       go to the top of the loop and the ctr gets
                       incremented. */
      }
      else
      {  /* A normal sibling */
        Maps[MapsCtr] = &(pCurrMap->pParent->pChildren[ChildCtr]->Map);
      }
      ChildCtr++;
    }
  }
  MapCount = MapsCtr;

  /* Add the current map to Maps */
  Maps[MapCount] = &(pCurrMap->Map);
  MapCount++;

  /* Finally add the children of the current map to Maps */
  /* Add the proper siblings */
  ChildCtr=0;
  for(MapsCtr=MapCount; MapsCtr< MapCount + pCurrMap->NumOfChildren; MapsCtr++)
  {
    while(ChildCtr < MAX_MAP_CHILDREN && pCurrMap->pChildren[ChildCtr]== NULL)
      ChildCtr++;

    if(pCurrMap->pChildren[ChildCtr]== NULL)
    {
/*
      logit("","Error, problem with children of %s.\n",
            WebParams.LPSData.MapID);
*/
      break;
    }
    else
    {
      Maps[MapsCtr] = &(pCurrMap->pChildren[ChildCtr]->Map);
    }
    ChildCtr++;
  }
  MapCount = MapsCtr;

  /* Done with the stinking map tree!!!  */
  if(PrintWarningMessages)
  {
    printf("<center><h5>Note:  These pages can be viewed only with Netscape Navigator <br>\n"
           "version 4.5 or higher or Internet Explorer version 4.0 or higher.<br></h5></center>\n");
  } /* end if PrintWarningMessages */



  /* Write the htmlish headers to the browser */
  printf("<TITLE>Earthworm DBMS - Map of %s</TITLE>\n",WebParams.LPSData.MapData.MapName);

  html_header (BackgroundColor, HeaderLogo, HeaderTag);

  printf("<FORM NAME=\"MAPFORM\" ACTION=\"NULL\"" EXE_EXT
         " METHOD=POST>\n");
  printf("<INPUT NAME=NUMEVENTS TYPE=HIDDEN VALUE=\"%d\">\n",NumEventsRetrieved);
  
  /* Grab the mutexes from the mapserver thread.  This assures that it 
     has completed.  We don't want to start without it. */
  RequestSpecificMutex( &MapServerMutex );

	if (NumEventsRetrieved < WebParams.ECSData.MaxEventsDisp)
		EventsToMap = NumEventsRetrieved;
	else
		EventsToMap = WebParams.ECSData.MaxEventsDisp;

  if (DrawMap(&im_out, pStationListBuffer, NumStationsRetrieved,
		pEventBuffer, EventsToMap, &Pic, &(WebParams.LPSData),
		WebParams.WGSSData.StationClickEffect, StationDisplayWidth,
		WebParams.MSISData.ImageFileName, AreaBuf, AreaBuf2, 
		&MC_triangle, 1, 1, DEBUG))
  {
    html_logit("","Call to DrawMap failed\n");
    return (-1);
  }

  /* print formatting junk including table headers */
  printf("\n<br>\n");
  printf("<center>\n");

  if(WebParams.WGSSData.ClickEffect == CLICK_VIEW_EVENTS)
  {
    strcpy(szBackgroundColor,HTML_TABLE_COLOR1);
    strcpy(szTitle,HTML_TITLE_COLOR1);
  }
  else
  {
    strcpy(szBackgroundColor,HTML_TABLE_COLOR2);
    strcpy(szTitle,HTML_TITLE_COLOR2);
  }

  printf("<table bgcolor=%s cellpadding=8 cellspacing=0 border=2>\n",
         szBackgroundColor);
  printf(" <tr><strong>\n");
  printf("  <td colspan=%d><center><strong>"
         "<font size=+1 color=%s>%s (<A HREF=../getlist_help.html>"
         "<font size=-1 color=%s>HELP</font></A>)</font></td>\n",
         NUM_HTML_TABLE_COLUMNS,HTML_SPECIAL_TEXT_COLOR,szTitle,
         HTML_SPECIAL_TEXT_COLOR);
  printf(" </tr>\n");
  printf(" <tr>\n");
  printf("  <td colspan=%d>\n",NUM_HTML_TABLE_COLUMNS);
  printf("   <table cellpadding=0 cellspacing=0 border=1>\n");
  printf("    <tr>\n");

    if(WebParams.LPSData.MapData.LatHeight <= 40)
      legendnum=1;
    else if(WebParams.LPSData.MapData.LatHeight <= 100)
      legendnum=2;
    else
      legendnum=3;

  /* now insert the legend to the left of the main map */
  printf("     <td rowspan=%d><img height=%d width=%d src=\"../legend%d.gif\">"
         "</td>\n",
         MapCount,
         WebParams.LPSData.PixelHeight
          + PIC_TOP_PIX_BORDER+PIC_BTM_PIX_BORDER,
         114
          * (WebParams.LPSData.PixelHeight
             + PIC_TOP_PIX_BORDER+PIC_BTM_PIX_BORDER)
          / 412,
         legendnum);

  /* now print the rowspan blurb for the main map */
  printf("     <td rowspan=%d>",MapCount);

  /* Now we gotta go through some convoluted logic that figures out what kind
     of funky tapdancing if any we have to do on the client side, and thus
     print out all kinds of mumbo jumbo for the browser to interpret */
  if(WebParams.WGSSData.ClickEffect == CLICK_VIEW_EVENTS)
  {
    /* Write the Quake Map to the browser */
    /* This code adapted from www.ibm.com's use of client side
    image maps 1998/08/05 DK */ 

    printf("    <IMG SRC=\"getimage"EXE_EXT"?GL_IMAGE_ID=%d\" "
           "USEMAP=\"#search\" ALT=\"%s\" "
           "BORDER=0 WIDTH=%d HEIGHT=%d ></td>\n",
           (int) getpid(),WebParams.LPSData.MapData.MapName,
           WebParams.LPSData.PixelWidth
            + PIC_LEFT_PIX_BORDER+PIC_RIGHT_PIX_BORDER,
           WebParams.LPSData.PixelHeight
            + PIC_TOP_PIX_BORDER+PIC_BTM_PIX_BORDER);
  }
  else
  {
    printf("   <A NAME=\"MAP_ANCHOR\" HREF=\"http:getlist?ClickEffect=%d"
           "&StationClickEffect=%d&MapName=%s&%s\">\n",
           WebParams.WGSSData.ClickEffect,
           WebParams.WGSSData.StationClickEffect,
           WebParams.LPSData.MapData.MapName,
		       WebParams.WGSSData.GUIStateString);
    printf("    <IMG SRC=\"getimage"EXE_EXT"?GL_IMAGE_ID=%d\" "
           "ISMAP ALT=\"%s\" "
           "BORDER=0 WIDTH=%d HEIGHT=%d ></A></td>\n",
           (int) getpid(),WebParams.LPSData.MapData.MapName,
           WebParams.LPSData.PixelWidth
            + PIC_LEFT_PIX_BORDER+PIC_RIGHT_PIX_BORDER,
           WebParams.LPSData.PixelHeight
            + PIC_TOP_PIX_BORDER+PIC_BTM_PIX_BORDER);
  }  /* End if(WebParams.WGSSData.ClickEffect == CLICK_VIEW_EVENTS) */

  /*printf("MapCount=%d. After PPQ4.\n",MapCount);*/
  for(MapsCtr=0;MapsCtr<MapCount;MapsCtr++)
  {
    /*printf("MapsCtr=%d\n",MapsCtr);*/
    if(MapsCtr)
    {
      printf("<tr>");
    }
    if(Maps[MapsCtr] == &(pCurrMap->Map))
    {
      printf("<td bgcolor=#00CCFF><font size=+1><a href="
             "getlist?ClickEffect=%d&StationClickEffect=%d&"
             "MapNameClicked=1&MapName=%s&%s"
             ">%s</a></td></tr>\n",
             WebParams.WGSSData.ClickEffect,
             WebParams.WGSSData.StationClickEffect,
             Maps[MapsCtr]->MapName, WebParams.WGSSData.GUIStateString,
             Maps[MapsCtr]->MapName);
    }
    else
    {
      printf("<td bgcolor=#ffffff><font size=+1><a href="
             "getlist?ClickEffect=%d&StationClickEffect=%d&"
             "MapNameClicked=1&MapName=%s&%s"
             ">%s</a></td></tr>\n",
             WebParams.WGSSData.ClickEffect,
             WebParams.WGSSData.StationClickEffect,
             Maps[MapsCtr]->MapName, WebParams.WGSSData.GUIStateString,
             Maps[MapsCtr]->MapName);
    }
  }  /* end for */
/*    printf("At bottom of MapsCtr loop, MapsCtr=%d\n",MapsCtr); */
  printf("   </table>\n");  /* End of the images/maplist internal table*/

  printf("  </td>\n");
  printf(" </tr>\n");
  printf(" <tr>\n");


  /* This is a lot of huffing and puffing, just to make sure that
     the checkbox is checked when it should be and not checked when
     it shouldn't be.  DavidK 19990219  
  **************************************/

  if(WebParams.WGSSData.ClickEffect == CLICK_VIEW_EVENTS)
  {
    printf("  <td align=center colspan=%d><strong><font size=\"+1\" color=%s>\n",
           NUM_HTML_TABLE_COLUMNS,HTML_SPECIAL_TEXT_COLOR);
    printf("   <INPUT TYPE=CHECKBOX %s NAME=\"StationsShown\" "
         "OnClick=\"if (this.checked==true) {window.location.href='getlist?ClickEffect=%d"
		     "&StationClickEffect=%d&MapName=%s&%s';} "
         "else {window.location.href='getlist?ClickEffect=%d"
		     "&StationClickEffect=%d&MapName=%s&%s';}"
         "return true;\" > Show Stations</td>\n",
         (WebParams.WGSSData.StationClickEffect == EWDB_STATION_DISPLAY_STATIONS) ? "CHECKED": "",
         WebParams.WGSSData.ClickEffect,EWDB_STATION_DISPLAY_STATIONS,
         WebParams.LPSData.MapData.MapName,
         WebParams.WGSSData.GUIStateString,
         WebParams.WGSSData.ClickEffect,EWDB_STATION_DONT_DISPLAY_STATIONS,
         WebParams.LPSData.MapData.MapName,
         WebParams.WGSSData.GUIStateString
        );
  

  }
  else /* if CLICK_VIEW_EVENTS */
  {
    printf("  <td align=left colspan=%d><strong><font size=\"+1\" color=%s>\n",
           NUM_HTML_TABLE_COLUMNS/3,HTML_SPECIAL_TEXT_COLOR);

    /******************************************************************************/
    /*** WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING **/
    /******************************************************************************/
    /*  The following code references the Map Docuement Link by number instead of
        by name.  This was done because of all the hoops that we would have to jump
        through in order to get the name to be parse write.  We would have to pass
        the name through 3 parsers (the C Compiler, the HTML parser in the Browser,
        and the javascript parser.  We coule set this code in a special JAVASCRIPT
        functions section in the page, but that seems like a lot of work for a
        page that doesn't change much.
    ********************************************************************************/
    printf("   <INPUT TYPE=CHECKBOX %s NAME=\"ZoomIn\" "
           "OnClick=\"document.links[%d].href='getlist?ClickEffect=%d"
           "&StationClickEffect=%d&MapName=%s&%s'; "
           "%s return true;\"> Zoom In </td>\n",
           (WebParams.WGSSData.ClickEffect == CLICK_ZOOM_IN) ? "CHECKED": "",
           GETLIST_MAP_ANCHOR_POSITION,
           CLICK_ZOOM_IN,
		       WebParams.WGSSData.StationClickEffect,
           WebParams.LPSData.MapData.MapName,
		       WebParams.WGSSData.GUIStateString,
           "this.checked=true; this.form.ZoomOut.checked=false; this.form.ReCenter.checked=false;"
           );

    printf("  <td align=center colspan=%d><strong><font size=\"+1\" color=%s>\n",
           NUM_HTML_TABLE_COLUMNS/3,HTML_SPECIAL_TEXT_COLOR);
    printf("   <INPUT TYPE=CHECKBOX %s NAME=\"ReCenter\" "
           "OnClick=\"document.links[%d].href='getlist?ClickEffect=%d"
		       "&StationClickEffect=%d&MapName=%s&%s'; "
           "%s return true;\" > Re-Center Map </td>\n",
           (WebParams.WGSSData.ClickEffect == CLICK_RECENTER) ? "CHECKED" : "",
           GETLIST_MAP_ANCHOR_POSITION,
           CLICK_RECENTER,
           WebParams.WGSSData.StationClickEffect,
           WebParams.LPSData.MapData.MapName,
           WebParams.WGSSData.GUIStateString,
           "this.checked=true; this.form.ZoomOut.checked=false; this.form.ZoomIn.checked=false;"
           );

    printf("  <td align=left colspan=%d><strong><font size=\"+1\" color=%s>\n",
           NUM_HTML_TABLE_COLUMNS/3,HTML_SPECIAL_TEXT_COLOR);
    printf("   <INPUT TYPE=CHECKBOX %s NAME=\"ZoomOut\" "
           "OnClick=\"document.links[%d].href='getlist?ClickEffect=%d"
		       "&StationClickEffect=%d&MapName=%s&%s'; "
           "%s return true;\" > Zoom Out </td>\n",
           (WebParams.WGSSData.ClickEffect == CLICK_ZOOM_OUT) ? "CHECKED" : "",
           GETLIST_MAP_ANCHOR_POSITION,
           CLICK_ZOOM_OUT,
           WebParams.WGSSData.StationClickEffect,
           WebParams.LPSData.MapData.MapName,
           WebParams.WGSSData.GUIStateString,
           "this.checked=true; this.form.ZoomIn.checked=false; this.form.ReCenter.checked=false;"
           );
  }
  printf(" </tr>\n");




  printf(" <tr>\n");
  printf("  <td align=center colspan=%d bgcolor=%s>\n",
         NUM_HTML_TABLE_COLUMNS/2,HTML_TABLE_COLOR1);


  if(PRINT_GRAY_BUTTONS)
  {
    /* embed the button in a gray cell, so that it will look gray, 
       even in X-windows */
    printf("   <table border=0 cellspacing=0 cellpadding=0 bgcolor=%s>"
           "<tr><td><strong>\n",HTML_GRAY_COLOR);
  }
  else
  {
    printf("   <font color=%s><strong>\n",HTML_SPECIAL_TEXT_COLOR);
  }

  if(WebParams.WGSSData.ClickEffect == CLICK_VIEW_EVENTS)
  {
    printf("    <INPUT TYPE=\"BUTTON\" VALUE=\"View Data\" "
           "OnClick=\"return true;\">\n");
  }
  else
  {
    printf("   <INPUT TYPE=\"BUTTON\" VALUE=\"View Data\" "
           "OnClick=\"window.location.href='getlist?ClickEffect=%d"
  		     "&StationClickEffect=%d&MapName=%s&%s'; "
           "return true;\">",
           CLICK_VIEW_EVENTS,
    		   WebParams.WGSSData.StationClickEffect,
           WebParams.LPSData.MapData.MapName,
           WebParams.WGSSData.GUIStateString);
  }

  if(PRINT_GRAY_BUTTONS)
  {
    printf("</td></tr></table>");
  }
  
  printf("\n  </td>\n");

  printf("  <td align=center colspan=%d bgcolor=%s>\n",
         NUM_HTML_TABLE_COLUMNS/2,HTML_TABLE_COLOR2);

  if(PRINT_GRAY_BUTTONS)
  {
    /* embed the button in a gray cell, so that it will look gray, 
       even in X-windows */
    printf("   <table border=0 cellspacing=0 cellpadding=0 bgcolor=%s>"
           "<tr><td><strong>\n",HTML_GRAY_COLOR);
  }
  else
  {
    printf("   <font color=%s><strong>\n",HTML_SPECIAL_TEXT_COLOR);
  }

  if(WebParams.WGSSData.ClickEffect == CLICK_VIEW_EVENTS)
  {
    printf("    <INPUT TYPE=\"BUTTON\" VALUE=\"Change Map\" "
           "OnClick=\" "
  		     "if (this.form.StationsShown.checked==true) "
  		     "{window.location.href='getlist?ClickEffect=%d"
  		     "&StationClickEffect=%d&MapName=%s&%s';} "
  		     "else "
  		     "{window.location.href='getlist?ClickEffect=%d"
  		     "&StationClickEffect=%d&MapName=%s&%s';} "
           "return true;\">\n",
           CLICK_ZOOM_IN,
     		   WebParams.WGSSData.StationClickEffect,
           WebParams.LPSData.MapData.MapName,
           WebParams.WGSSData.GUIStateString,
           CLICK_ZOOM_IN,
           WebParams.WGSSData.StationClickEffect,
           WebParams.LPSData.MapData.MapName,
           WebParams.WGSSData.GUIStateString
          );
  }
  else
  {
    printf("    <INPUT TYPE=\"BUTTON\" VALUE=\"Change Map\" "
           "OnClick=\"return true;\">");
  }


  printf(" <tr>\n");
  if(PRINT_GRAY_BUTTONS)
  {
    printf("</td></tr></table>");
  }
  

  printf("\n  </td>\n");

  printf(" </tr>\n");
  /* close up the external table, cause we're done with the map portion.
     now lets go get some marguritas.  I don't drink marguritas, but it
     is nice to be done with the those verdampfte tables. */
  printf("</table>\n");

    if(WebParams.WGSSData.ClickEffect == CLICK_VIEW_EVENTS)
  {
    printf("<BR>\n");
    printf("<MAP NAME=\"search\">\n");
    printf(AreaBuf);
    printf("</MAP>\n");
  }

  /* In the print statement below, we assume the RetVal reports
     events found as a negative number, so if we got all of the
     events, then, make RetVal reflect -(How many we got). 
  ************************************************/
  if(NumEventsFound == 0)
    NumEventsFound=NumEventsRetrieved;

  printf("<strong>%u Events found, %u latest displayed.\n",
							NumEventsFound,EventsToMap);
  printf("<br>\n");


  /* End Null Form. */
  printf("</CENTER>\n");
  printf("</FORM>\n\n");


  /* print divider */
  printf("<HR>\n\n");



  /* Start getlist postable form, for time/magnitude changes. */
  printf("<FORM NAME=\"CRITERIAFORM\" ACTION=\"NULL\"" EXE_EXT
         " METHOD=POST>\n");
  printf("<CENTER>\n\n");

  /* Print mini-form title */
  printf("<strong><font  size=\"+1\">Change Time/Magnitude/Depth Criteria</font>\n<br>\n");
  printf("<br>\n");

  /* Header for Time/Depth/Magnitude */
  printf(
    "<table border=1>\n" 
    "<tr bgcolor=\"#2222AA\"><strong>\n"
    "    <td colspan=2><strong><font size=\"+1\" color=\"#FFFFFF\">Origin Time Range </td>\n"
    "    <td><strong><font  size=\"+1\"color=\"#FFFFFF\">Depth (km)</td>\n"
    "    <td colspan=1><strong><font size=\"+1\" color=\"#FFFFFF\">Mag (Preferred) </td>\n"
    "</tr>\n"
        );

  printf("<tr>\n");

  /* Date and Time info */
  printf("    <td colspan=2 valign=top align=right>Start time (UTC)\n"
    "       <INPUT NAME=\"Start_Date\" TYPE=TEXT size=10 maxlength=10 VALUE=\"%s\">  \n"
    "       <INPUT NAME=\"Start_Time\" TYPE=TEXT size=8 maxlength=8 VALUE=\"%s\">\n"
    ,EWDB_tmtoa_date(&(WebParams.ECSData.tmMinDate),szTemp),
     EWDB_tmtoa_time(&(WebParams.ECSData.tmMinDate),szTemp2)
    );

  printf("      <br>End time (UTC) \n"
    "       <INPUT NAME=\"Finish_Date\" TYPE=TEXT size=10 maxlength=10 VALUE=\"%s\">  \n"
    "       <INPUT NAME=\"Finish_Time\" TYPE=TEXT size=8 maxlength=8 VALUE=\"%s\"> </td>\n"
    ,EWDB_tmtoa_date(&(WebParams.ECSData.tmMaxDate),szTemp),
     EWDB_tmtoa_time(&(WebParams.ECSData.tmMaxDate),szTemp2)
    );


  /* Depth and Magnitude info */
  printf("    <td valign=top align=right>Min \n"
    "       <INPUT NAME=\"MinZ\" TYPE=TEXT size=7 maxlength=6 VALUE=\"%4.2f\">\n"
    "      <br>Max \n"
    "       <INPUT NAME=\"MaxZ\" TYPE=TEXT size=7 maxlength=6 VALUE=\"%4.2f\"> </td>\n"
    "    <td valign=top align=center>Min \n"
    "       <INPUT NAME=\"MinMag\" TYPE=TEXT size=7 maxlength=6 VALUE=\"%01.2f\">\n"
    "      <br>Max \n"
    "       <INPUT NAME=\"MaxMag\" TYPE=TEXT size=7 maxlength=6 VALUE=\"%2.2f\"> </td>\n"
    ,WebParams.ECSData.MinZ,WebParams.ECSData.MaxZ,
    WebParams.ECSData.MinMag,WebParams.ECSData.MaxMag
    );

  printf("</tr>\n");
  printf("</Table>\n\n");

  /* Write the submit button at the top of the table */
  printf("<br>\n");
  printf("<INPUT TYPE=\"BUTTON\" VALUE=\"Submit Changed Criteria\" "
         "OnClick=\"window.location.href='getlist?ClickEffect=%d"
         "&StationClickEffect=%d&MapName=%s"
         "&%s"
         "&MaxZ=' + this.form.MaxZ.value + '"
         "&MinZ=' + this.form.MinZ.value + '"
         "&MaxMag=' + this.form.MaxMag.value + '"
         "&MinMag=' + this.form.MinMag.value + '"
         "&Start_Date=' + this.form.Start_Date.value + '"
         "&Start_Time=' + this.form.Start_Time.value + '"
         "&Finish_Date=' + this.form.Finish_Date.value + '"
         "&Finish_Time=' + this.form.Finish_Time.value + '"
         "'; "
         "return true;\" >\n",
         WebParams.WGSSData.ClickEffect,
         WebParams.WGSSData.StationClickEffect,
         WebParams.LPSData.MapData.MapName,
         WebParams.WGSSData.GUIStateString
        );

  printf("<br>\n");

  /* End getlist postable Form. */
  printf("</CENTER>\n");
  printf("</FORM>\n\n");

  /* Ensure things stay centered */
  printf("<CENTER>\n");


  if(WebParams.WGSSData.ClickEffect == CLICK_VIEW_EVENTS)
  {
	/* 
	 * Get auxiliary details about each event:
	 *   We do this so that we can display useful indicators
	 *   such as the number of snippets, number of phases
	 *   whether the event has a shakemap, moment tensor, focal mech...
	 *   
	 *  First, we free up the event list space, and reallocate using    
	 *   the max number of events to consider in the list. This could
	 *   be different than the number of events to show on the map.   
	 */

		if((AuxEventInfo = (AuxEventStruct *)
			malloc(WebParams.ECSData.MaxEventsRetr * sizeof(AuxEventStruct))) == NULL)
		{
			html_logit ("","Unable to allocate space for storage of %d AuxEvents.\n",
												WebParams.ECSData.MaxEventsRetr);
			return(-1);
		}

	reverse = FALSE;
	if (EventsPerPage < 0)
	{
		evtStart = 0;
		evtEnd = NumEventsRetrieved;
		evtDispMode = DISPMODE_SHOWALL; 
	}
	else
	{
		if ((WebParams.WGSSData.NextEvtID < 0) &&
				(WebParams.WGSSData.PrevEvtID < 0))
		{
			/* Start with the youngest event */
			evtStart = 0;
			evtEnd = EventsPerPage;
			evtDispMode = DISPMODE_TOPPAGE; 

			if (evtEnd >= NumEventsRetrieved)
			{
				evtEnd = NumEventsRetrieved;
				evtDispMode = DISPMODE_ENDPAGE;
			}
		}
		if ((WebParams.WGSSData.NextEvtID > 0) &&
				(WebParams.WGSSData.PrevEvtID > 0))
		{
			/* Start with the oldest event */
			evtDispMode = DISPMODE_ENDPAGE;
			evtEnd = NumEventsRetrieved;
			evtStart = evtEnd - EventsPerPage;
			reverse = TRUE;

			if (evtStart <= 0)
			{
				evtStart = 0;
				evtDispMode = DISPMODE_TOPPAGE;
				reverse = FALSE;
			}
		}
		else if (WebParams.WGSSData.NextEvtID >= 0)
		{	
			evtStart = -1;
			evtEnd = -1;

			/* Find the event in the summary list */
			for (i = 0; ((i < NumEventsRetrieved) && (evtStart < 0)); i++)
			{
				if (pEventBuffer[i].Event.idEvent == 
							WebParams.WGSSData.NextEvtID)
				{
					evtStart = i;
				}
			}

			if (evtStart < 0)
			{
				logit ("", "Could not find event %d in the list; show top.\n",
										WebParams.WGSSData.NextEvtID);
				evtStart = 0;
			}

			if (evtStart == 0)
			{
				evtDispMode = DISPMODE_TOPPAGE;
				evtStart = 0; 
				evtEnd = EventsPerPage;
			}
			else
			{
				evtEnd = evtStart + EventsPerPage;
				evtDispMode = DISPMODE_NORMAL;

			}

			/* Check start and end indeces for end of list */
			if (evtEnd >= NumEventsRetrieved)
			{
				evtEnd = NumEventsRetrieved;
				evtDispMode = DISPMODE_ENDPAGE;
			}
		}
		else if (WebParams.WGSSData.PrevEvtID >= 0)
		{	
			evtEnd = -1;
			evtStart = -1;
			reverse = TRUE;

			/* Find the event in the summary list */
			for (i = 0; ((i < NumEventsRetrieved) && (evtEnd < 0)); i++)
			{
				if (pEventBuffer[i].Event.idEvent == 
							WebParams.WGSSData.PrevEvtID)
				{
					evtEnd = i;
				}
			}

			if (evtEnd < 0)
			{
				logit ("", "Could not find event %d in the list; show top.\n",
										WebParams.WGSSData.NextEvtID);
				evtDispMode = DISPMODE_TOPPAGE;
				evtStart = 0; 
				evtEnd = EventsPerPage;
				reverse = FALSE;
			}

			if (evtEnd == (NumEventsRetrieved - 1))
			{
				evtDispMode = DISPMODE_ENDPAGE;
				evtStart = evtEnd - EventsPerPage;
				reverse = TRUE;
			}
			else
			{
				evtDispMode = DISPMODE_NORMAL;
				evtStart = evtEnd - EventsPerPage;
				reverse = TRUE;
			}

			/* Check start index for end of list */
			if (evtStart <= 0)
			{
				evtStart = 0;
				evtDispMode = DISPMODE_TOPPAGE;
				reverse = FALSE;
			}
		}
	}

	if (reverse == FALSE)
	{
	    for (i = evtStart; i < evtEnd; i++)
		{
			Getlist_FillAuxInfo (&AuxEventInfo[i], 
						&pEventBuffer[i], MinNumStasToShow);

			/* 
			 * If number of station is below the limit, we must 
			 * increase the number of events to be shown.
			 */
			if (AuxEventInfo[i].NumStas < MinNumStasToShow)
			{
				evtEnd = evtEnd + 1;
				if (evtEnd > (NumEventsRetrieved - 1))
				{
					evtEnd = NumEventsRetrieved - 1;
					evtDispMode = DISPMODE_ENDPAGE;
				}
			}

		} /* forward for loop over events */

	} /* if reverse flag is not set */
	else
	{
	    for (i = (evtEnd-1); i >= evtStart; i--)
		{
			Getlist_FillAuxInfo (&AuxEventInfo[i], 
						&pEventBuffer[i], MinNumStasToShow);

			/* 
			 * If number of station is below the limit, we must 
			 * increase the number of events to be shown.
			 */
			if (AuxEventInfo[i].NumStas < MinNumStasToShow)
			{
				evtStart = evtStart - 1;
				if (evtStart <= 0)
				{
					evtStart = 0;
					evtDispMode = DISPMODE_TOPPAGE;
					reverse = FALSE;
				}
			}
		} /* reverse for loop over events */

	} /* if reverse flag is set */


    /* print divider */
    printf("<HR>\n\n");

    /* Start ora2sac postable form. */
    printf("<FORM ACTION=\"ora2sactarfile\"" EXE_EXT
           "METHOD=POST TARGET=\"Download Seismograms\">\n");
    printf("<INPUT NAME=NUMEVENTS TYPE=HIDDEN VALUE=\"%d\">\n",NumEventsRetrieved);

    printf("<CENTER><PRE>\n\n");

    printf("To see more information about an event, click on the EventID.\n\n");
    printf("To retrieve seismograms for one or more events, select the events'\n"
			"checkbox, then press 'Retrieve Seismograms'.  NOTE: This operation\n"
			"may take some time for a large number of channels to be saved.\n\n");

	if (ShowPickCol == TRUE)
    	printf("To enter quick review for the event, click on the link in the 'Picks' column.\n\n");


	if (ShowStasCol == TRUE)
    	printf("To enter the strong motion display for the event, click on the link in the 'Stas' column.\n\n");

	if (ShowTraceCol == TRUE)
    	printf("To see a snapshot of the waveforms, click on the link in the 'Traces' column.\n\n");

	if (ShowAlarmsCol == TRUE)
    	printf("To see all alarms issued for this event, click on the link in the 'Alarms' column.\n\n");

    printf("\n</PRE>\n");

    if ((WebParams.WGSSData.ClickEffect == CLICK_VIEW_EVENTS) &&
			(evtDispMode == DISPMODE_SHOWALL))
    {
      /* Write the submit button at the top of the table */
      printf("<INPUT TYPE=\"SUBMIT\" VALUE=\"Retrieve Seismograms\">\n<br>\n");
      printf("<br>\n");
    }


    /*  Write the table header */
    printf("<table border=1> <tr bgcolor=#2222AA VALIGN=TOP align=center>\n");

	if (ShowPickCol == TRUE)
		rowSpan = 2;
	else	
		rowSpan = 1;

    printf("<td colspan=2 rowspan=%d valign=center><strong><font size=\"+1\" color=#FFFFFF>EventID</td>\n", rowSpan);
    printf("<td rowspan=%d valign=center><strong><font size=\"+1\" color=#FFFFFF>Origin Time</td>\n", rowSpan);
    printf("<td rowspan=%d valign=center><strong><font size=\"+1\" color=#FFFFFF>Lat</td>\n", rowSpan);
    printf("<td rowspan=%d valign=center><strong><font size=\"+1\" color=#FFFFFF>Lon</td>\n", rowSpan);
    printf("<td rowspan=%d valign=center><strong><font size=\"+1\" color=#FFFFFF>Depth</td>\n", rowSpan);
    printf("<td rowspan=%d valign=center><strong><font size=\"+1\" color=#FFFFFF>Mags</td>\n", rowSpan);
    printf("<td rowspan=%d valign=center><strong><font size=\"+1\" color=#FFFFFF>Source</td>\n", rowSpan);

	/* 
	 * The following columns are optional, shown depending on what 
	 * is specified in the config file
	 */
	if (ShowPickCol == TRUE)
	    printf("<td rowspan=1 colspan=2 valign=center><strong><font size=\"+1\" color=#FFFFFF>Picks</td>\n");

	if (ShowStasCol == TRUE)
	    printf("<td rowspan=%d valign=center><strong><font size=\"+1\" color=#FFFFFF>Stas</td>\n", rowSpan);


	if (ShowTraceCol == TRUE)
    	printf("<td rowspan=%d valign=center><strong><font size=\"+1\" color=#FFFFFF>Traces</td>\n", rowSpan);

	if (ShowAlarmsCol == TRUE)
    	printf("<td rowspan=%d valign=center><strong><font size=\"+1\" color=#FFFFFF>Alarms</td>\n", rowSpan);

    printf("</tr>\n");

	if (ShowPickCol == TRUE)
	{
	    printf("<tr bgcolor=#2222AA VALIGN=TOP align=center>\n");
		printf("<td rowspan=1 valign=center><strong><font size=\"+1\" color=#FFFFFF>Arr</td>\n");
		printf("<td rowspan=1 valign=center><strong><font size=\"+1\" color=#FFFFFF>Amp</td>\n");
		printf("</tr>\n");
	}

    /* Write a record for each retrieved from the database. */
	NumEventsDisplayed = 0;

    for (i = evtStart; i < evtEnd; i++)
    {
		if (AuxEventInfo[i].NumStas < MinNumStasToShow) 
		{
			continue;
		}


      /* write the checkbox for the event */
      printf("<tr align=center>\n");
      printf("<td> <INPUT NAME=\"EVENT%u\" TYPE=CHECKBOX> </td>\n",
             pEventBuffer[i].Event.idEvent);
      printf("<td> <A HREF=\"eqparam2html"EXE_EXT"?EVENT%u=On\" "
				"TARGET=\"Event Info\">%u</A> </td>\n",
             pEventBuffer[i].Event.idEvent, pEventBuffer[i].Event.idEvent);

      /*Write the origin time in the form YYYY/MM/DD  HH:MM:SS */
      TempTime=(time_t)(pEventBuffer[i].dOT);
      printf("<td>%s</td>\n",EWDB_ttoa(&TempTime,pTimeBuffer));
      

      /* Write the other fields */
      printf("<td> %2.2f </td>\n",pEventBuffer[i].dLat);
      printf("<td> %3.2f </td>\n",pEventBuffer[i].dLon);
      printf("<td> %1.1f </td>\n",pEventBuffer[i].dDepth);

	  /* list of magnitudes, prefered first */
	  if (AuxEventInfo[i].iNumMags > 0)
	  {
		  printf ("<TD>");
			/* Prefered first */
			if (AuxEventInfo[i].iPrefMag >= 0)
			{
				printf ("%4.1f%s ", AuxEventInfo[i].Mags[AuxEventInfo[i].iPrefMag].dMagAvg, 
									MagNames[AuxEventInfo[i].Mags[AuxEventInfo[i].iPrefMag].iMagType]);
			}

			for (m = 0; m < AuxEventInfo[i].iNumMags; m++)
			{
				if (m != AuxEventInfo[i].iPrefMag)
				{
					printf ("%4.1f%s ", AuxEventInfo[i].Mags[m].dMagAvg, 
									MagNames[AuxEventInfo[i].Mags[m].iMagType]);
				}
			}

		printf ("</TD>\n");

	  }
	  else
         printf("<td> --- </td>\n");


      printf("<td> %s </td>\n",pEventBuffer[i].Event.szSourceName);

	  if (ShowPickCol == TRUE)
      {
     
          /* put up a link to the review system from either arrival or amp picks */
           printf ("<td bgcolor=#FFFF00>"
	    			"<A HREF=\"review_event?EventID=%u?OriginID=%u?Action=%d\""
	    			"target=\"Event Review\">%d</A></TD>\n",
	    					pEventBuffer[i].Event.idEvent, 
							AuxEventInfo[i].idOrigin, ACT_GETFROMDB,
	    					AuxEventInfo[i].NumPhases);

           printf ("<td bgcolor=#FFFF00>"
		    		"<A HREF=\"review_event?EventID=%u?OriginID=%u?Action=%d\""
		    		"target=\"Event Review\">%d</A></TD>\n",
		    				pEventBuffer[i].Event.idEvent, 
							AuxEventInfo[i].idOrigin, ACT_GETFROMDB,
		    				AuxEventInfo[i].NumAmpPicks);
       }


	  if (ShowStasCol == TRUE)
      {
      	/* number of stations */
	      if (AuxEventInfo[i].NumStas <= 0)
   	         printf("<td>0</td>\n");
   	      else
   	      {
            /* put up a link to the urban hazards page */
            	printf ("<TD bgcolor=#CC6666> <A HREF=\"uh_review_sm?EVENT%u=On\""
	   			  "target=\"Strong Motion Review \">%d</A></TD>\n", 
   				  pEventBuffer[i].Event.idEvent, AuxEventInfo[i].NumStas);
         }
      }

	  if (ShowTraceCol == TRUE)
      {
           /* snippets */
           if (AuxEventInfo[i].NumSnippets < 0)
                 printf("<td>0</td>\n");
           else
           {
              /* put up a link to the ora2snippet_gif page */
              printf ("<TD bgcolor=#00FF00> <A HREF=\"ora2snippet_gif?EventID=%u\""
	     		  "target=\"All Snippets\">%d</A></TD>\n", 
		     	  pEventBuffer[i].Event.idEvent, AuxEventInfo[i].NumSnippets);
           }
      }



	  if (ShowAlarmsCol == TRUE)
      {
         /* alarms */
         if (AuxEventInfo[i].NumAlarms <= 0)
               printf("<td>0</td>\n");
         else
         {
            /* put up a link to the get_alarms page */
      	      printf ("<TD bgcolor=#00FFFF> <A HREF=\"review_get_alarms?EventID=%u\""
	   			  "target=\"Review Alarms\">%d</A></TD>\n", 
	   			  pEventBuffer[i].Event.idEvent, AuxEventInfo[i].NumAlarms);
         }
      }


      /* Write the close of the row */
      printf("</tr>\n");

	  NumEventsDisplayed = NumEventsDisplayed + 1;

    }  /* end for NumEventsRetrieved */
    

    /* Write the close of the table */
    printf("</table> \n");

    if(NumEventsFound == 0)
      NumEventsFound=NumEventsRetrieved;

    printf("<br>\n");
   	printf("%u Events displayed;  %u latest retrieved out of %u available.\n",
			NumEventsDisplayed, NumEventsRetrieved, NumEventsFound);
   	printf("<br>\n");

  } /* end if clickeffect=2? */

  if(((Pic.deg[PIC_TOP]- Pic.deg[PIC_BOT] <= StationDisplayWidth)
      && (WebParams.WGSSData.StationClickEffect == EWDB_STATION_DISPLAY_DEFAULT))
    || WebParams.WGSSData.StationClickEffect == EWDB_STATION_DISPLAY_STATIONS)
  {
    /* In the print statement below, we assume the RetVal reports
    stations found as a negative number, so if we got all of the
    stations, then, make RetVal reflect -(How many we got). 
    ************************************************/
    if(NumStationsFound == 0)
      NumStationsFound=NumStationsRetrieved;

    printf("%u Stations found, %u displayed.\n",NumStationsFound,NumStationsRetrieved);
    printf("<br>\n");

  }

  printf("<br>\n");


  if(WebParams.WGSSData.ClickEffect == CLICK_VIEW_EVENTS)
  {
#undef DO_COINC
#ifdef DO_COINC
		/* Put up a link to Coincidence Events */
		if (ShowCoincidenceLink == TRUE)
		{
			printf ("<A HREF=\"coinclist?StartTime=%0.2f&EndTime=%0.2f\" "
					"TARGET=\"Coincidences\">View Coincidence Events</A>\n",
								pEventBuffer[evtEnd].dOT, 
								pEventBuffer[evtStart].dOT);
  			printf("<BR><BR>\n");
		}
#endif DO_COINC

		/* Write the submit button at the bottom of the table */
		printf("<INPUT TYPE=\"SUBMIT\" VALUE=\"Retrieve Seismograms\">\n<br>\n");

		/* Close the form */
		printf("</form>\n");

		/* Put up links allowing navigation among event display pages */
		if (evtDispMode == DISPMODE_NORMAL)
		{
		    printf("   <INPUT TYPE=\"BUTTON\" VALUE=\"View Data\" "
           "OnClick=\"window.location.href='getlist?ClickEffect=%d"
  		     "&StationClickEffect=%d&MapName=%s&%s'; "
           "return true;\">",
           CLICK_VIEW_EVENTS,
    		   WebParams.WGSSData.StationClickEffect,
           WebParams.LPSData.MapData.MapName,
           WebParams.WGSSData.GUIStateString);

			printf ("<CENTER><BR><HR><BR>\n");
			printf ("<TABLE><TR align=center>\n");

			printf ("<TD width=1000><A HREF=\"getlist?NextEvtID=%d&PrevEvtID=%d&"
							"ClickEffect=%d&StationClickEffect=%d&MapName=%s&%s\"> "
							"Previous Page</A></TD>\n",
									-1,
									pEventBuffer[evtStart].Event.idEvent,
									WebParams.WGSSData.ClickEffect,
									WebParams.WGSSData.StationClickEffect,
									WebParams.LPSData.MapData.MapName,
									WebParams.WGSSData.GUIStateString);

			printf ("<TD width=1000><A HREF=\"getlist?NextEvtID=%d&PrevEvtID=%d&"
						"ClickEffect=%d&StationClickEffect=%d&MapName=%s&%s\"> "
							"Newest Events</A></TD>\n",
									-1, -1,
									WebParams.WGSSData.ClickEffect,
									WebParams.WGSSData.StationClickEffect,
									WebParams.LPSData.MapData.MapName,
									WebParams.WGSSData.GUIStateString);

			printf ("<TD width=1000><A HREF=\"getlist?NextEvtID=%d&PrevEvtID=%d&"
						"ClickEffect=%d&StationClickEffect=%d&MapName=%s&%s\"> "
							"Oldest Events</A></TD>\n",
									1, 1,
									WebParams.WGSSData.ClickEffect,
									WebParams.WGSSData.StationClickEffect,
									WebParams.LPSData.MapData.MapName,
									WebParams.WGSSData.GUIStateString);

			printf ("<TD width=1000><A HREF=\"getlist?NextEvtID=%d&PrevEvtID=%d&"
						"ClickEffect=%d&StationClickEffect=%d&MapName=%s&%s\"> "
							"Next Page</A></TD>\n",
									pEventBuffer[evtEnd].Event.idEvent,
									-1,
									WebParams.WGSSData.ClickEffect,
									WebParams.WGSSData.StationClickEffect,
									WebParams.LPSData.MapData.MapName,
									WebParams.WGSSData.GUIStateString);

			printf ("</TR></TABLE>\n");
			printf ("<BR><BR></CENTER>\n");
		}
		else if (evtDispMode == DISPMODE_ENDPAGE)
		{
			printf ("<CENTER><BR><HR><BR>\n");
			printf ("<TABLE><TR align=center>\n");

			printf ("<TD width=1000><A HREF=\"getlist?NextEvtID=%d&PrevEvtID=%d&"
					"ClickEffect=%d&StationClickEffect=%d&MapName=%s&%s\"> "
							"Previous Page</A></TD>\n",
									-1,
									pEventBuffer[evtStart].Event.idEvent,
									WebParams.WGSSData.ClickEffect,
									WebParams.WGSSData.StationClickEffect,
									WebParams.LPSData.MapData.MapName,
									WebParams.WGSSData.GUIStateString);

			printf ("<TD width=1000><A HREF=\"getlist?NextEvtID=%d&PrevEvtID=%d&"
					"ClickEffect=%d&StationClickEffect=%d&MapName=%s&%s\"> "
							"Newest Events</A></TD>\n",
									-1, -1,
									WebParams.WGSSData.ClickEffect,
									WebParams.WGSSData.StationClickEffect,
									WebParams.LPSData.MapData.MapName,
									WebParams.WGSSData.GUIStateString);

			printf ("<TD width=1000><A HREF=\"getlist?NextEvtID=%d&PrevEvtID=%d&"
						"ClickEffect=%d&StationClickEffect=%d&MapName=%s&%s\"> "
							"Oldest Events</A></TD>\n",
									1, 1,
									WebParams.WGSSData.ClickEffect,
									WebParams.WGSSData.StationClickEffect,
									WebParams.LPSData.MapData.MapName,
									WebParams.WGSSData.GUIStateString);


			printf ("</TR></TABLE>\n");
			printf ("<BR><BR></CENTER>\n");
		}
		else if (evtDispMode == DISPMODE_TOPPAGE)
		{
			printf ("<CENTER><BR><HR><BR>\n");
			printf ("<TABLE><TR align=center>\n");
			printf ("<TD width=1000><A HREF=\"getlist?NextEvtID=%d&PrevEvtID=%d&"
				"ClickEffect=%d&StationClickEffect=%d&MapName=%s&%s\"> "
							"Newest Events</A></TD>\n",
									-1, -1,
									WebParams.WGSSData.ClickEffect,
									WebParams.WGSSData.StationClickEffect,
									WebParams.LPSData.MapData.MapName,
									WebParams.WGSSData.GUIStateString);

			printf ("<TD width=1000><A HREF=\"getlist?NextEvtID=%d&PrevEvtID=%d&"
						"ClickEffect=%d&StationClickEffect=%d&MapName=%s&%s\"> "
							"Oldest Events</A></TD>\n",
									1, 1,
									WebParams.WGSSData.ClickEffect,
									WebParams.WGSSData.StationClickEffect,
									WebParams.LPSData.MapData.MapName,
									WebParams.WGSSData.GUIStateString);

			printf ("<TD width=1000><A HREF=\"getlist?NextEvtID=%d&PrevEvtID=%d&"
					"ClickEffect=%d&StationClickEffect=%d&MapName=%s&%s\"> "
							"Next Page</A></TD>\n",
									pEventBuffer[evtEnd].Event.idEvent, 
									-1,
									WebParams.WGSSData.ClickEffect,
									WebParams.WGSSData.StationClickEffect,
									WebParams.LPSData.MapData.MapName,
									WebParams.WGSSData.GUIStateString);

			printf ("</TR></TABLE>\n");
			printf ("<BR><BR></CENTER>\n");
		}
  }


  /* free the buffers for the strings we just wrote */
  free(AreaBuf);
  free(AreaBuf2);


  /* Print out the postmortem, allowing people to complain */
  html_trailer (WebHost, FooterLogo, FooterTag);

  /* We survived, now go home */
  return(0);
}
