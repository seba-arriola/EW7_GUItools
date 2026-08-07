/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ora_trace_fetch.c,v 1.22 2003/02/04 17:59:55 davidk Exp $
 *
 *    Revision history:
 *     $Log: ora_trace_fetch.c,v $
 *     Revision 1.22  2003/02/04 17:59:55  davidk
 *     Changed fprintf() calls to logit() calls
 *
 *     Revamped debug/logit messages to use to multi-level debugging scheme.
 *
 *     Removed unneccessary debug messages that were left over from testing.
 *
 *     Extended the sleep duration in the main loop to 4 seconds from 1 second
 *     in hopes of limiting superfluous processing.
 *
 *     Adapted config code to set the appropriate ws_clientIII debug level based
 *     upon ora_trace_fetch's debug level.
 *
 *     Added a warning comment about memory allocation and ws_clientIII routines.
 *
 *     Added an ew_status message for reporting errors from the snippet request scheduling
 *     algorithm.
 *
 *     Revision 1.21  2002/06/27 23:51:26  davidk
 *     silenced some debugging statements.
 *     Fixed a bug in the snippet update logic, so that
 *     the code does not attempt to update the snippet if the
 *     scheduling algorithm indicates that it has already deleted
 *     the snippet.
 *
 *     Revision 1.20  2002/06/07 16:42:02  patton
 *     Made logit changes.
 *
 *     Revision 1.19  2002/05/16 17:11:02  davidk
 *     Moved a bunch of constants from ora_trace_fetch.c to ora_trace_fetch.h
 *     so that they could also be used by schedule.c.
 *
 *     Revision 1.18  2002/05/15 21:07:19  davidk
 *     Fixed a misspelled constatn.
 *
 *     Revision 1.17  2002/05/15 20:58:24  davidk
 *     fixed a bug in the handling of a deleted request, when it
 *     is deleted by the scheduling algorithm, due to lack of success
 *     instead of by the main function because of successful retrieval.
 *
 *     Revision 1.16  2002/05/14 21:04:28  davidk
 *     moved scheduling content to schedule.c
 *
 *     Revision 1.15  2002/05/13 23:18:37  davidk
 *     Fully updated to work with Concierge Mark2 api and database.
 *
 *     Revision 1.14  2002/04/23 16:20:41  davidk
 *     Added ScheduleNextAttemptForSnippetRequest() function, to calculate
 *     attempt scheduling.
 *
 *     Revision 1.12  2002/04/16 20:51:13  davidk
 *     concierge - Mark 2.
 *
 *     Revision 1.11  2002/03/15 22:36:27  davidk
 *     Added code to issue a heartbeat if due, within the
 *     "process snippet request" loop.  This should fix a bug where we
 *     would get busy processing trace requests and forget to tell statmgr
 *     that we are still alive.
 *
 *     Revision 1.10  2002/03/15 02:31:23  davidk
 *     Added some variable documentation.
 *     Modified/removed/added debugging statements.
 *     Changed several default values trying to stabilize ora_trace_fetch.
 *     This version has run for several days without noticable problems on gldjimmy.
 *
 *     Revision 1.9  2002/02/18 18:16:47  davidk
 *     fixed a bug in the update_snippet_request logic, that modifies the snippet request
 *     when we get a partial fill of our request from the wave_server.  There
 *     was a bug where we were always updating the start of the request, even when
 *     we meant to update the end because we only got a partial fill at the beginning.
 *     Modified some debug statements and some formatting.  (no functional effect).
 *
 *     Revision 1.6  2001/07/01 21:55:27  davidk
 *     Cleanup of the Earthworm Database API and the applications that utilize it.
 *     The "ewdb_api" was cleanup in preparation for Earthworm v6.0, which is
 *     supposed to contain an API that will be backwards compatible in the
 *     future.  Functions were removed, parameters were changed, syntax was
 *     rewritten, and other stuff was done to try to get the code to follow a
 *     general format, so that it would be easier to read.
 *
 *     Applications were modified to handle changed API calls and structures.
 *     They were also modified to be compiler warning free on WinNT.
 *
 *     Revision 1.5  2001/05/15 02:15:34  davidk
 *     Moved functions around between the apps, DB API, and DB API INTERNAL
 *     levels.  Renamed functions and files.  Added support for amplitude
 *     magnitude types.  Reformatted makefiles.
 *
 *     Revision 1.4  2001/05/10 20:14:34  dietz
 *     Changed to shut down gracefully if the transport flag is
 *     set to TERMINATE or MyPid.
 *
 *     Revision 1.3  2001/02/28 17:29:10  lucky
 *     Massive schema redesign and cleanup.
 *
 *     Revision 1.2  2001/02/21 11:50:54  davidk
 *     changed the include for ws_clientIII.h to local instead of system.
 *
 *     Revision 1.1  2001/01/18 17:12:48  davidk
 *     Initial revision
 *
 *
 */

/*

   ora_trace_fetch.c: adapted from ora_trace_fetch.c 

   ora_trace_fetch.c : Cloned from dbtrace_save, which was cloned from 
     dbreport (which was written by Lynn Dietz). reads trigger messages and
     writes specified trace snippets to an Oracle db. Different from
     dbtrace_save in that it uses the API, written by Dave Kragness.

Story: we do not pick up TYPE_TRIG messages(generated by various
detector algorithms) the way that ora_trace_fetch did, that is done
by a program called ora_trace_req, which converts the trigger 
messages into snippet requests and stores those snippet requests
in the DB.  We(ora_trace_fetch) retrieve a list of snippet requests
from the DB and attempt to fetch trace for each request.  We update
the DB based on our results. 

MAIN LOOP
We retrieve a list of snippet requests, and create a corresponding
list of TRACE_REQs.  We submit the list of TRACE_REQs to the
concierge wsClient routines.  We then process one TRACE_REQ structure
at a time, updating the DB with the results from each.

OTHER
We run the main loop periodically, based on a given time interval.  
We can be jumpstarted into running the main loop by an EW_ALERT
message with subject(EW_NEW_SNIPPET_REQUEST or EW_NETWORK_UP).
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time_ew.h>
#include <earthworm.h>
#include <kom.h>
#include <transport.h>
#include <math.h>

/* Do not include ewdb_apps_utils.h
It has the missing prototype we need for 
ewdb_apps_putaway_NextSnippetForAnEvent() which we would like
to have to resolve all compiler warnings, but it causes ws_clientII.h
to be included, which causes problems for us because it defines some
structures with the same name as ws_clientIII.h which is what we
are using.  We will have to work this out later....
#include <ewdb_apps_utils.h>
DK 06/28/01
******************************/

#include "ws_clientIII.h"
#include "ora_trace_fetch.h"



/* Functions in this source file 
 *******************************/
int  dbtsv_config ( char * );
int  dbtsv_lookup ( void );
void  dbtsv_status ( unsigned char, short, char * );

int CheckForNewMessages(void);
int CheckForSnippetRequests(int IN_bModifyAttemptParams);

/* Functions related to Scheduling mechanisms */
int CheckForSchedulingConfigVariables();
int ScheduleNextAttemptForSnippetRequest(EWDB_SnippetRequestStruct * psrCurrent, 
                                         int iAttemptRetCode);

/* Transport global variables
 ****************************/
static  SHM_INFO  Region;      /* shared memory region to use for i/o    */

#define   MAXLOGO   10
MSG_LOGO  GetLogo[MAXLOGO];    /* array for requesting module,type,instid */
short     nLogo;



char errText[MAXTXT];   /* string for log/error messages          */
        
/* Globals to set from configuration file
 ****************************************/
static int     RingSize;            /* max messages in output circular buffer       */
static char    RingName[MAX_RING_STR];        /* name of transport ring for i/o      */
static char    MyModName[MAX_MOD_STR];       /* speak as this module name/id        */
static int     LogSwitch;           /* 0 if no logfile should be written   */
static long    HeartBeatInt;        /* seconds between heartbeats          */
long           TimeoutSeconds=-1;      /* seconds to wait for reply from ws */
long           MaxTraces;      /* max traces per message we'll ever deal with  */
int             nServer;        /* number of wave servers we know about */
char           wsIp[MAX_WAVESERVERS][MAX_ADRLEN];
char           wsPort[MAX_WAVESERVERS][MAX_ADRLEN];
            /* list of available waveServers, from config. file */
double         dGracePeriod = 2.0/*seconds*/;  /* number of seconds of leeway when filling reqs */
int iProgramMode = OTF_PROGRAM_MODE_STANDARD_DATA_MODE;
int iLoopInterval = 20;
int iRequestGroup = -1;  /* by default grab all request groups */
int iStatusReportInterval = 600;  /* 10 minutes */


/* Globals relating to database; set from configuration file 
 ***********************************************************/
char  DBservice[30];       /* OCI data source to interact with  */
char  DBuser[30];          /* UserId to connect to database as  */
char  DBpassword[30];      /* Password to datasource            */

  
/* Things to look up in the earthworm.h tables with getutil.c functions
 **********************************************************************/
static long          RingKey;       /* key of transport ring for i/o      */
static unsigned char InstId;        /* local installation id              */
static unsigned char MyModId;       /* Module Id for this program         */
static unsigned char TypeHeartBeat; 
static unsigned char TypeError;
static unsigned char TypeTrig;
static unsigned char TypeAlert;
static unsigned char TypeTrigger;


/* Things to be malloc'd
 ***********************/
/* story: Some of these things don't have to be global; they are only
so that we can malloc them from main at startup time. Having them 
malloc'd by the routine which uses them presents the danger that the 
module would start ok, but die (if unable to malloc) only when the 
first event hits. Which could be much later, at a bad time...*/

char* TraceBuffer;      /* where we store the trace snippet  */
long TraceBufferLen;    /* bytes of largest snippet we're up for - from configuration file */
TRACE_REQ*  TraceReq;   /* request forms for wave server. Malloc'd at startup. From ws_client.h */
char * pMessageBuffer;  /* buffer for receiving EW messages */

int Debug_tr_sv=0;  /* setting this to one causes all sorsts of log output */

pid_t MyPid;            /* Our own pid, sent with heartbeat for restart 
                           purposes */
EWDB_SnippetRequestStruct * SnipReqArray;
wsHANDLE wsHandle;

/* BEHAVIOR CONTROL VARIABLES 
******************************/
/* bProcess is time based loop control variable.  bProcess ensures that
   we do not process data in the main loop, unless iLoopInterval seconds
   have expired since the last processing, or unless we know that there is
   data to process.
   In program mode 1, bProcess has no effect, because we do not perform
   any processing unless we know there is processing to be done.  In modes
   2 & 3, bProcess ensures some amount of sanity, by preventing the processing
   logic from being run (CheckForRequests in Mode 2 & ProcessRequests in Mode3)
   more often than iLoopInterval seconds.
 **********************************************/
int bProcess = TRUE;         /* always process at startup */

/* bRequestRefresh controls whether or not we will check for snippet requests
   during the current pass through the main loop.  This value is always true
   for modes 2 & 3.  This value is set to TRUE under mode 1, when we know there
   are snippet requests to be processed, or we get some sort of stimulus, such
   as a trigger message. */
int bRequestRefresh = TRUE;  /* always check for snippet requests at startup */

/* bSpecialRequestRefresh controls whether or not we will attempt special processing
   this time through the loop where we attempt to process all existing snippet requests
   regardless of when they are scheduled to be processed.  This variable is always TRUE
   under Mode 3.  In Modes 1 & 2, it is only true if the the program receives a
   TYPE_ALERT - Network UP message.  (that's why it's called Special)
 ****************************************************************************/
int bSpecialRequestRefresh = FALSE;


/* Earthworm Heartbeat Stuff
 ***************************/
long         timeNow;              /* current time                   */       
long         timeLastBeat;         /* time last heartbeat was sent   */

int main( int argc, char **argv )
{

  /* BORING STUFF - Declaring variables
  ******************************/

  long         timeLastProcessing, timeLastStatsReport;
  int           i;
  int          rc;
  char         LogFileName[128];
  
  time_t       tNextSnippetReqCheck=0;
  time_t       tNextAttempt = 1999999999;  /* set tNextAttempt to a max value,
                                              so that it doesn't get invoked 
                                              unless it deserves too */

  /* BORING STUFF - Initialization
  ******************************/

  /* Check command line arguments 
  ******************************/
  if ( argc != 2 )
  {
    fprintf( stderr, "Usage: ora_trace_fetch <configfile>\n" );
    return(-1);
  }

  /* Zero the wave server arrays 
  *****************************/
  for (i=0; i< MAX_WAVESERVERS; i++)
  {
    memset( wsIp[i], 0, MAX_ADRLEN);
    memset( wsPort[i], 0, MAX_ADRLEN);
  }
 
  /* Initialize name of log-file & open it 
   ***************************************/
  fprintf(stderr,"ora_trace_fetch:  Initializing logit.\n");
  /* Created LogFileName, to be obtained from config file name
     instead of using the fixed "ora_trace_fetch" for logname. */
  strcpy(LogFileName,argv[1]);
  LogFileName[strlen(argv[1])-2/*.d*/]=0;

  logit_init( LogFileName, 0, 1024, 1 );

  /* Read the configuration file(s)
  ********************************/
  fprintf(stderr,"ora_trace_fetch:  Reading Config files.\n");
  if(rc=dbtsv_config(argv[1]))
  {
    fprintf(stderr,"ora_trace_fetch:  ERROR Reading Config files. EXITING!!!\n");
    return(-1);
  }
  logit( "" , "ora_trace_fetch: Read command file <%s>\n", argv[1] );

  /* Get our own Pid for restart purposes
  ***************************************/
  MyPid = getpid();
  if(MyPid == -1)
  {
    logit("et","ora_trace_fetch: Cannot get pid. Exiting.\n");
    return(-1);
  }


  /* Look up important info from earthworm.h tables
  ************************************************/
  logit("et","ora_trace_fetch:  Performing Earthworm config lookup.\n");
  if(rc=dbtsv_lookup())
  {
    logit("et","ora_trace_fetch:  ERROR Performing Earthworm config Lookup. EXITING!!!\n");
    return(-1);
  }
  
  /* Reinitialize logit to desired logging level
   *********************************************/
  logit_init( LogFileName, 0, 1024, LogSwitch );



  /* Attach to Input/Output shared memory ring 
  *******************************************/
  tport_attach( &Region, RingKey );

  /* Indicate whether the debugger flag is turned on. */
  logit("","ora_trace_fetch:  Debug is set to %d\n",Debug_tr_sv);

  /* If debug mode, log debug info */
  if(Debug_tr_sv >= DEBUG_OTF_INFO)
  {
    /* LOGO */
    logit("", "ora_trace_fetch: Attached to public memory region %s: %d\n", 
          RingName, RingKey );
    logit("","logo:%d %u %u %u\n", nLogo,
          GetLogo[0].type,GetLogo[0].mod,GetLogo[0].instid);

    /* WAVE_SERVER INFO */
    logit("","wave server stuff:\n TimeoutSeconds=%d\n",TimeoutSeconds);
    for (i=0; i< MAX_WAVESERVERS; i++)
    {
      if (wsIp[i][0]==0) 
        break;
      /*else*/
      logit(""," wsIp[%d]=(%s) wsPort[%d]=(%s)\n",i,wsIp[i],i,wsPort[i]);
    }

  }


  /* ALLOCATE DYNAMIC MEMORY 
  ***************************************/

  /* Trace request structures */
  if ( (TraceReq = calloc( (size_t)MaxTraces, (size_t)sizeof(TRACE_REQ) ) )
       == NULL )
  {
    sprintf(errText,"ora_trace_fetch: Cant allocate %ld trace request structures. "
             "Exitting\n",MaxTraces);
    dbtsv_status( TypeError, ERR_XALLOC_ERROR, errText ); 

    return(-1);
  }

  /* Snippet request structures */
  if ( (SnipReqArray = calloc((size_t)MaxTraces, 
                              (size_t)sizeof(EWDB_SnippetRequestStruct) ) )
       == NULL )
  {
    sprintf(errText,"ora_trace_fetch: Cant allocate %ld snippet request structures. "
             "Exitting\n",MaxTraces);
    dbtsv_status( TypeError, ERR_XALLOC_ERROR, errText ); 

    return(-1);
  }

  /* Space for input/output messages */
  if ( ( pMessageBuffer = (char *) malloc(DB_MAX_BYTES_PER_EQ) ) == (char *) NULL )
  {
    logit( "e", "export_generic: error allocating pMessageBuffer; exitting!\n" );
    return(-1);
  }


  /* END ALLOCATION
  ***************************************/


  /* END BORING STUFF 
  ******************************/

  /* Initialize the disposal system
  ********************************/
  if( ewdb_api_Init(DBuser,DBpassword,DBservice ) != EWDB_RETURN_SUCCESS )
  {
    sprintf(errText, "ewdb_api_Init() failure; exitting!\n" );
    dbtsv_status( TypeError, ERR_API_INIT_FAILED, errText ); 
    return( -1 );
  }


  /* Initialize the retrieval system
  ********************************/
  if(rc=wsInitialize(&wsHandle,TraceBufferLen) != WS_ERR_NONE)
  {
    logit("et","ora_trace_fetch: wsInitialize() failed with code %d\n", rc);
    return(-1);
  }


  /* Add the wave_servers we got from the config file
  ********************************/
  for (i=0; i< MAX_WAVESERVERS; i++)
  {
    if (wsIp[i][0]==0) 
      break;
    /*else*/
    if(wsAddServer(wsHandle, wsIp[i], wsPort[i], 10000, 0/*default*/) != WS_ERR_NONE)
    {
      if(Debug_tr_sv >= DEBUG_OTF_ERROR)
        logit("et","Error adding server %s:%s\n", wsIp[i], wsPort[i]);
    }
  }

  if(Debug_tr_sv>= DEBUG_OTF_INFO) {logit("t","Finished adding all servers(%d)\n", i-1);}
  /* NO FLUSHING THE RING!  Pretend your married */
  /* We don't utilize the contents of messages, so don't flush the ring.
     Worse case it causes us to run an extra cycle if we restart with
     old data on the ring.
   ******************************************************************/


  /* Force a heartbeat to be issued in first pass thru main loop
  *************************************************************/
  timeLastBeat = 0;
  timeLastProcessing = 0;
  time(&timeLastStatsReport);

  if(Debug_tr_sv >= DEBUG_OTF_ALL){logit("t","Beginning main loop\n");}

  /* begin loop till Earthworm shutdown (level 1) */
  while( tport_getflag(&Region) != TERMINATE  &&
         tport_getflag(&Region) != MyPid )
  {
    rc = EWDB_RETURN_FAILURE;
    
    if(Debug_tr_sv >= DEBUG_OTF_ALL){logit("t","Top of Main Loop\n");}
    
    /* send ora_trace_fetch's heartbeat (if due)
    **********************************/
    if(time(&timeNow) - timeLastBeat  >=  HeartBeatInt ) 
    {
      timeLastBeat = timeNow;
      dbtsv_status( TypeHeartBeat, 0, "" ); 
    }
    
    /* check for and handle EW messages */
    if(Debug_tr_sv >= DEBUG_OTF_ALL) {logit("t","Checking for new messages.\n");}
    
    CheckForNewMessages();
    
    if(Debug_tr_sv >= DEBUG_OTF_ALL){logit("t","Checking Status Report Time\n");}
    
    /* check to see if processing interval time has elapsed */
    if(time(&timeNow) - timeLastProcessing  >=  iLoopInterval) 
    {
      bProcess = TRUE;
      if((timeNow - timeLastStatsReport) > iStatusReportInterval)
      {
        time(&timeLastStatsReport);
        wsPrintServerStats(wsHandle);
      }
    }
    
    if(Debug_tr_sv >= DEBUG_OTF_ALL){logit("t","Checking time(%d) vs. next attempt(%d)\n",timeNow, tNextAttempt);}
    
    if(timeNow > tNextAttempt)
    {
      bProcess = TRUE;
      bRequestRefresh = TRUE;
      if(Debug_tr_sv >= DEBUG_OTF_INFO){logit("t","Scheduling Snippet Check.\n");}
    }
    
    
    /* if it is time, check the DB for snippet_requests */
    if(bProcess  && bRequestRefresh) 
    {
      timeLastProcessing = timeNow;
      
      if(Debug_tr_sv >= DEBUG_OTF_INFO){logit("t","Checking for Snippet Requests.\n");}
      
      /* check for some snippet requests */
      rc = CheckForSnippetRequests(!bSpecialRequestRefresh);

      if(iProgramMode != OTF_PROGRAM_MODE_ALWAYS_GET_SNIPPETS)
        bSpecialRequestRefresh = FALSE;
      if(!(iProgramMode == OTF_PROGRAM_MODE_ALWAYS_GET_SNIPPETS || 
           iProgramMode == OTF_PROGRAM_MODE_ALWAYS_CHECK_LIST
          )
        )
      {
        bRequestRefresh = FALSE;
        if(ewdb_api_GetNextScheduledSnippetRetrievalAttempt(&tNextAttempt)
          != EWDB_RETURN_SUCCESS)
        {
          tNextAttempt = 1999999999;
        }
      if(Debug_tr_sv >= DEBUG_OTF_WARNING)
        logit("t","Next scheduled retrieval attempt is %d\n",tNextAttempt);
      }
    }
    
    if(Debug_tr_sv >= DEBUG_OTF_ALL){logit("t","Checking for sleep(%d).\n", (rc!=EWDB_RETURN_SUCCESS));}
    
    if(rc!=EWDB_RETURN_SUCCESS)
    {
      sleep_ew(4000);    
    }

    bProcess = FALSE;  /* reset bProcess */

    if(Debug_tr_sv >= DEBUG_OTF_ALL){logit("t","Bottom of Working loop\n");}
    
  }          /* end of while!(shutdown requested)  (level 1) */  

  /* Termination has been requested 
  ********************************/
  tport_detach( &Region ); /* detach from shared memory */
  logit( "t", "ora_trace_fetch: Termination requested; exitting!\n" );
  return(0);
}
/*--------------------------------- end of main() ---------------------------------------*/


/***********************************************************************
 *  dbtsv_config() processes command file(s) using kom.c functions;    *
 *                  exits if any errors are encountered.         *
 ***********************************************************************/
int dbtsv_config( char *configfile )
{
   int      ncommand;     /* # of required commands you expect to process   */ 
   char     init[20];     /* init flags, one byte for each required command */
   int      nmiss;        /* number of required commands that were missed   */
   char    *com;
   char    *str;
   int      nfiles;
   int      success;
   int      i;

/* Set to zero one init flag for each required command 
 *****************************************************/   
   ncommand = 11;
   for( i=0; i<ncommand; i++ )  init[i] = 0;
   nLogo = 0;
   nServer = 0;

/* Open the main configuration file 
 **********************************/
   nfiles = k_open( configfile ); 
   if ( nfiles == 0 ) {
  logit( "e",
                "ora_trace_fetch: Error opening command file <%s>; returning!\n", 
                 configfile );
  return( -1 );
   }

/* Process all command files
 ***************************/
   while(nfiles > 0)   /* While there are command files open */
   {
        while(k_rd())        /* Read next line from active file  */
        {  
      com = k_str();         /* Get the first token from line */

        /* Ignore blank lines & comments
         *******************************/
            if( !com )           continue;
            if( com[0] == '#' )  continue;

        /* Open a nested configuration file 
         **********************************/
            if( com[0] == '@' ) {
               success = nfiles+1;
               nfiles  = k_open(&com[1]);
               if ( nfiles != success ) {
                  logit( "e", 
                          "ora_trace_fetch: Error opening command file <%s>; returning!\n",
                           &com[1] );
                  return( -1 );
               }
               continue;
            }

        /* Process anything else as a command 
         ************************************/
  /*0*/     if( k_its("LogFile") ) {
                LogSwitch = k_int();
                init[0] = 1;
            }
  /*1*/     else if( k_its("MyModuleId") ) {
                str = k_str();
                if(str) strcpy( MyModName, str );
                init[1] = 1;
            }
  /*2*/     else if( k_its("RingName") ) {
                str = k_str();
                if(str) strcpy( RingName, str );
                init[2] = 1;
            }
  /*3*/     else if( k_its("HeartBeatInt") ) {
                HeartBeatInt = k_long();
                init[3] = 1;
            }

  /*4*/     else if( k_its("DBservice") ) {
                str = k_str();
                if(str) strcpy( DBservice, str );
                init[4] = 1;
            }
  /*5*/     else if( k_its("DBpassword") ) {
                str = k_str();
                if(str) strcpy( DBpassword, str );
                init[5] = 1;
            }
  /*6*/     else if( k_its("DBuser") ) {
                str = k_str();
                if(str) strcpy( DBuser, str );
                init[6] = 1;
            }

         /* wave server addresses and port numbers to get trace snippets from
          *******************************************************************/
  /*7*/     else if( k_its("WaveServer") ) 
    {
                if ( nServer >= MAX_WAVESERVERS ) 
        {
                    logit( "e", 
                            "ora_trace_fetch: Too many <WaveServer> commands in <%s>", 
                             configfile );
                    logit( "e", "; max=%d; returning!\n", (int) MAX_WAVESERVERS );
                    return( -1 );
                    }
                if( ( str=k_str() ) )  strcpy(wsIp[nServer],str);
                if( ( str=k_str() ) )  strcpy(wsPort[nServer],str);
    nServer++;
    init[7]=1;
                }

  /*8*/     else if( k_its("TraceBufferLen") ) {
                TraceBufferLen = k_int() * 1000; /* convert from kilobytes to bytes */
                init[8] = 1;
            }

  /*9*/     else if( k_its("MaxTraces") ) {
                MaxTraces = k_int(); 
                init[9] = 1;
            }
         /* Enter installation & module to get event messages from
          ********************************************************/
  /*10*/     else if( k_its("GetEventsFrom") ) {
                if ( nLogo >= MAXLOGO ) {
                    logit( "e", 
                            "ora_trace_req : Too many <GetEventsFrom> commands in <%s>", 
                             configfile );
                    logit( "e", "; max=%d; exiting!\n", (int) MAXLOGO );
                    exit( -1 );
                }
                if( ( str=k_str() ) ) {
                   if( GetInst( str, &GetLogo[nLogo].instid ) != 0 ) {
                       logit( "e", 
                               "ora_trace_req : Invalid installation name <%s>", str ); 
                       logit( "e", " in <GetEventsFrom> cmd; exiting!\n" );
                       exit( -1 );
                   }
                   GetLogo[nLogo+1].instid = GetLogo[nLogo].instid;
                }
                if( ( str=k_str() ) ) {
                   if( GetModId( str, &GetLogo[nLogo].mod ) != 0 ) {
                       logit( "e", 
                               "ora_trace_req : Invalid module name <%s>", str ); 
                       logit( "e", " in <GetEventsFrom> cmd; exiting!\n" );
                       exit( -1 );
                   }
                   GetLogo[nLogo+1].mod = GetLogo[nLogo].mod;
                }
                if( GetType( "TYPE_TRIGLIST2K", &GetLogo[nLogo].type ) != 0 ) {
                    logit( "e", 
                               "ora_trace_req : Invalid message type <TYPE_TRIGLIST2K>" ); 
                    logit( "e", "; exiting!\n" );
                    exit( -1 );
                }
                nLogo++;
                init[10] = 1;
            }

            /*************************************************************
            OPTIONAL COMMANDS
  *************************************************************/
  else if( k_its("Debug") ) {  /*optional command*/
    int iWsClientDebug=0;
    Debug_tr_sv=k_int();


    /* set appropriate library level debug setting here */
    switch(Debug_tr_sv)
    {
      case DEBUG_OTF_NONE:
           iWsClientDebug = WS_DEBUG_NONE;
           break;
      case DEBUG_OTF_ALL:
           iWsClientDebug = WS_DEBUG_ALL;
           break;
      case DEBUG_OTF_INFO:
           iWsClientDebug |= WS_DEBUG_SERVER_INFO | WS_DEBUG_DATA_INFO;
      case DEBUG_OTF_WARNING:
           iWsClientDebug |= WS_DEBUG_SERVER_WARNINGS | WS_DEBUG_DATA_WARNINGS;
      case DEBUG_OTF_ERROR:
           iWsClientDebug |= WS_DEBUG_SERVER_ERRORS | WS_DEBUG_DATA_ERRORS;
           break;
      default:
           break;
    }
    setWsClient_ewDebug(iWsClientDebug);
  }
  
  else if( k_its("TimeoutSeconds") ) {
    TimeoutSeconds = k_int(); 
  }

  else if( k_its("ProgramMode") ) 
  {
    str = k_str(); 
    if(!strcmp(str,"ALWAYS_CHECK_LIST"))
    {
      iProgramMode = OTF_PROGRAM_MODE_ALWAYS_CHECK_LIST;
    }
    else if(!strcmp(str,"ALWAYS_GET_SNIPPETS"))
    {
      iProgramMode = OTF_PROGRAM_MODE_ALWAYS_GET_SNIPPETS;
      bSpecialRequestRefresh = TRUE;
    }
    else if(!strcmp(str,"STANDARD_DATA_MODE"))
    {
      iProgramMode = OTF_PROGRAM_MODE_STANDARD_DATA_MODE;
    }
    else
    {
      logit( "e", "ora_trace_fetch:  Config File Error: "
              "Illegal Value for \n"
              "\"ProgramMode\" config file command: (%s).  "
              "Returning!\n",str
             );
      return(-1);
    }
  }

  else if( k_its("ActionInterval") ) {
    iLoopInterval = k_int(); 
  }

  else if( k_its("GracePeriod") ) {
    dGracePeriod = k_val(); 
    if(dGracePeriod < 0.0)
      dGracePeriod = 0.0;
  }


  else if( k_its("RequestGroup") ) 
  {
    iRequestGroup = k_int();
  }

  else if(CheckForSchedulingConfigVariables())
  {
  }
  /* Unknown command
  *****************/ 
  else {
    logit( "e", "ora_trace_fetch: <%s> Unknown command in <%s>.\n", 
      com, configfile );
    continue;
  }
  
  /* See if there were any errors processing the command 
  *****************************************************/
  if( k_err() ) {
    logit( "e", 
      "ora_trace_fetch: Bad <%s> command in <%s>; returning!\n",
      com, configfile );
    return( -1 );
  }
  }
  nfiles = k_close();
   }

/* After all files are closed, check init flags for missed commands
 ******************************************************************/
   nmiss = 0;
   for ( i=0; i<ncommand; i++ )  if( !init[i] ) nmiss++;
   if ( nmiss ) {
       logit( "e", "ora_trace_fetch: ERROR, no " );
       if ( !init[0] )  logit( "e", "<LogFile> "       );
       if ( !init[1] )  logit( "e", "<MyModuleId> "    );
       if ( !init[2] )  logit( "e", "<RingName> "      );
       if ( !init[3] )  logit( "e", "<HeartBeatInt> "  );
       if ( !init[4] )  logit( "e", "<DBservice> "     );
       if ( !init[5] )  logit( "e", "<DBpassword> "    );
       if ( !init[6] )  logit( "e", "<DBuser> "        );
       if ( !init[7] )  logit( "e", "<WaveServer> "    );
       if ( !init[8] )  logit( "e", "<TraceBufferLen> ");
       if ( !init[9] )  logit( "e", "<MaxTraces> "     );
       if ( !init[10] ) logit( "e", "<GetEventsFrom> " );

       logit( "e", "command(s) in <%s>; returning!\n", configfile );
       return( -1 );
   }

   return(0);
}



/************************************************************************
 *  dbtsv_lookup( )   Look up important info from earthworm.h tables   *
 ************************************************************************/
int dbtsv_lookup( void )
{
/* Look up keys to shared memory regions
   *************************************/
   if( ( RingKey = GetKey(RingName) ) == -1 ) {
  fprintf( stderr,
           "ora_trace_fetch:  Invalid ring name <%s>; returning!\n", RingName);
  return( -1 );
   }

/* Look up installations of interest
   *********************************/
   if ( GetLocalInst( &InstId ) != 0 ) {
      fprintf( stderr, 
              "ora_trace_fetch: error getting local installation id; returning!\n" );
      return( -1 );
   }

/* Look up modules of interest
   ***************************/
   if ( GetModId( MyModName, &MyModId ) != 0 ) {
      fprintf( stderr, 
              "ora_trace_fetch: Invalid module name <%s>; returning!\n", MyModName );
      return( -1 );
   }

/* Look up message types of interest
   *********************************/
   if ( GetType( "TYPE_HEARTBEAT", &TypeHeartBeat ) != 0 ) {
      fprintf( stderr, 
              "ora_trace_fetch: Invalid message type <TYPE_HEARTBEAT>; returning!\n" );
      return( -1 );
   }
   if ( GetType( "TYPE_ERROR", &TypeError ) != 0 ) {
      fprintf( stderr, 
              "ora_trace_fetch: Invalid message type <TYPE_ERROR>; returning!\n" );
      return( -1 );
   }
/* removed support/need for TYPE_ALERT message.  EW Central not willing to add
   new message type at this time.  Functionality not expected to be needed.
   DK 04/02/2002
   ******************************************************************
   if ( GetType( "TYPE_ALERT", &TypeAlert ) != 0 ) {
      fprintf( stderr, 
              "ora_trace_fetch: Invalid message type <TYPE_ALERT>; returning!\n" );
      return( -1 );
   }
*/
   if ( GetType( "TYPE_TRIGLIST2K", &TypeTrigger ) != 0 ) {
      fprintf( stderr, 
              "ora_trace_fetch: Invalid message type <TYPE_TRIGLIST2K>; returning!\n" );
      return( -1 );
   }
   return(0);
} 

/*************************************************************************
 * dbtsv_status() builds a heartbeat or error message & puts it into    *
 *                 shared memory.  Writes errors to log file & screen.   *
 *************************************************************************/
void dbtsv_status( unsigned char type, short ierr, char *note )
{
   MSG_LOGO    logo;
   char         msg[256];
   long         size;
   long        t;
 
/* Build the message
 *******************/ 
   logo.instid = InstId;
   logo.mod    = MyModId;
   logo.type   = type;

   time( &t );

   if( type == TypeHeartBeat )
   {
     sprintf( msg, "%ld %ld\n\0", t,MyPid);
   }
   else if( type == TypeError )
   {
  sprintf( msg, "%ld %hd %s\n\0", t, ierr, note);
  logit( "et", "ora_trace_fetch: %s\n", note );
   }

   size = strlen( msg );   /* don't include the null byte in the message */   

/* Write the message to shared memory
 ************************************/
   if( tport_putmsg( &Region, &logo, size, msg ) != PUT_OK )
   {
        if( type == TypeHeartBeat ) {
           logit("et","ora_trace_fetch:  Error sending heartbeat.\n" );
  }
  else if( type == TypeError ) {
           logit("et","ora_trace_fetch:  Error sending error:%d.\n", ierr );
  }
   }

   return;
}


/*******************************************************************************
* given a pointer to a snippet structure (as parsed from the trigger message)  *
* and the array of blank trace request structures, fill them with the requests *
* implied by the snippet. Don't overflow, and return the number of requests    *
*  generated                                                                    *
*                                                                               *
* args:  pmenu: pointer to the client routine's menu list                       *
*  pSnppt:  pointer to the structure containing the parsed trigger line from the *
*    trigger message.                                                            *
*  pTrReq: pointer to array of trace request forms                              *
*  maxTraceReq: size of this array                                               *
*  pnTraceReq: pointer to count of currently used trace request forms           *
*                                                                              *
* Return Values:                                                               *
*  WS_ERR_NONE:                 Success                                        *
*  WS_ERR_SCN_NOT_IN_MENU:      SCN not found in wave_server menus             *
*  WS_ERR_BUFFER_OVERFLOW:      Too many trace requests maxTraceReq handled    *
*******************************************************************************/
int snipreq2trreq(EWDB_SnippetRequestStruct * pSnipReq, TRACE_REQ * pTrReq)
{

  /* clear the tracereq before we start writing data into it */
  memset(pTrReq,0,sizeof(TRACE_REQ));
  
  strncpy( pTrReq->sta,  pSnipReq->ComponentInfo.Sta,  sizeof(pTrReq->sta));
  strncpy( pTrReq->chan, pSnipReq->ComponentInfo.Comp, sizeof(pTrReq->chan));
  strncpy( pTrReq->net,  pSnipReq->ComponentInfo.Net,  sizeof(pTrReq->net));
  pTrReq->sta[sizeof(pTrReq->sta) -1] = 0;
  pTrReq->chan[sizeof(pTrReq->chan) -1] = 0;
  pTrReq->net[sizeof(pTrReq->net) -1] = 0;
  
  pTrReq->reqStarttime = pSnipReq->tStart;
  pTrReq->reqEndtime   = pSnipReq->tEnd;
  pTrReq->pClientData  = (void *) pSnipReq;

  if(pSnipReq->idExistingWaveform)
  {
    if(Debug_tr_sv >= DEBUG_OTF_INFO)
      logit("t", "snipreq2trreq(): Existing Waveform(%d) for (%s,%s,%s  - %d)\n",
          pSnipReq->idExistingWaveform, 
          pTrReq->sta, pTrReq->chan, pTrReq->net,
          pSnipReq->idSnipReq);
  }
  return(WS_ERR_NONE);
}


int ProcessMsg(char * msg, long recsize, MSG_LOGO reclogo);

/********************** Message Stacking Thread *******************
 *           Move messages from transport to memory queue         *
 ******************************************************************/
int CheckForNewMessages(void)
{
  int          res;
  long         recsize;        /* size of retrieved message             */
  MSG_LOGO     reclogo;        /* logo of retrieved message             */
  int          ret;


  /* Get a message from transport ring
  ************************************/
  while((res = tport_getmsg(&Region, GetLogo, nLogo, &reclogo, &recsize, 
                            pMessageBuffer, DB_MAX_BYTES_PER_EQ-1 ))
       != GET_NONE)
  {
    if(Debug_tr_sv >= DEBUG_OTF_ERROR)
      logit("et","Got message from transport of %d bytes,res=%d\n",recsize,res); 
    
      /* Check return code; report errors
    ***********************************/
    if( res != GET_OK && res != GET_MISS)
    {
      if( res==GET_TOOBIG )
      {
        sprintf(errText, "msg[%ld] i%d m%d t%d too long for target",
                recsize, (int) reclogo.instid,
                (int) reclogo.mod, (int)reclogo.type );
        dbtsv_status( TypeError, ERR_TOOBIG, errText );
        continue;
      }
      else if( res==GET_NOTRACK )
      {
        sprintf(errText, "no tracking for logo i%d m%d t%d in %s",
                (int) reclogo.instid, (int) reclogo.mod, (int)reclogo.type,
                RingName );
        dbtsv_status( TypeError, ERR_NOTRACK, errText );
      }
      else
      {
        logit("et","ERROR:ERROR:ERROR: ora_trace_fetch():"
                   "Unrecognized return code from tport_getmsg() %d.\n",res);
        sprintf(errText,"ERROR:ERROR:ERROR: ora_trace_fetch():"
                   "Unrecognized return code from tport_getmsg() %d.\n",res);
        dbtsv_status( TypeError, ERR_UNKNOWN, errText );
      }
    }  /* tport_getmsg() returned !GET_OK */
    else
    {
      if( res==GET_MISS )
      {
        sprintf(errText, "missed msg(s) i%d m%d t%d in %s",(int) reclogo.instid,
                (int) reclogo.mod, (int)reclogo.type, RingName );
        dbtsv_status( TypeError, ERR_MISSMSG, errText );
      }
      /* React to retrieved message (res==GET_OK,GET_MISS,GET_NOTRACK)
       *********************************************************/
      ret=ProcessMsg(pMessageBuffer, recsize, reclogo);
      
      if(Debug_tr_sv >= DEBUG_OTF_INFO) logit("","Processed a message\n"); 

      if ( ret!= EWDB_RETURN_SUCCESS )
      {
        /* do nothing */
      }  /* if ProcessMsg() failed */
    }
  } /* end while(ret != GET_NONE) */

  return(0);
}  /* end CheckForNewMessages() */


int ProcessMsg(char * msg, long recsize, MSG_LOGO reclogo)
{

  /* whatever kind of message we got, we want to do a refresh,
     meaning, we want to wake up and check things out. */
  bProcess = TRUE;
  bRequestRefresh = TRUE;
  
  /*
  if(reclogo.type == TypeAlert)
  {
    / * we are expecting two types of EW_ALERT messages.  
       TYPE 1)  SOMETHING HAPPENED.  Just set RequestRefresh to TRUE
       TYPE 2)  NETWORK UP.  Something major happened to the network,
                and we want to perform a special retry of all requests, its
                kind of like a Govenor's pardon.  Set bSpecialRequestRefresh
                to TRUE as well.  Also, do a menu refresh.* /
    if(msg[0] == EW_ALERT_TYPE_NETWORK_UP)
    {
      wsRefreshAllMenus(wsHandle); 
      bSpecialRequestRefresh = TRUE;
    }
  }
  */
  if(reclogo.type == TypeTrigger)
  {
  }
  else
  {
    return(EWDB_RETURN_FAILURE);
  }

  return(EWDB_RETURN_SUCCESS);
}  /* end ProcessMsg() */


/******************** Message Processing Thread *******************
 *           Take messages from memory queue, create trace        *
 *  snippets, and call the disposal (PutAway) routines        *
 ******************************************************************/
/* story: the trigger message contains lines specifying 'snippets' of trace data
   (EventId, SCN, start time, and duration to save). The S,C, and N specification can
   be a *, meaning wildcard. What we do below is to loop over the lines in the message,
   creating an array of trace request structures (TRACE_REQ). We then loop over those,
   trying to extract the trace from the WaveServers. 
  As we collect, we may be making premature requests. In that case, we're told
   by the WaveServer that we're premature. We accummulate worst case wait time for any
   such stragglers.We wait this amount of time, and then request the stragglers again.     
  This version stuffs the trace data into the Oracle server via oci. Another version
   (or maybe an option on this one), is to write various format files.
*/


int waveformstruct2trreq(EWDB_WaveformStruct *pws, TRACE_REQ * ptr)
{
  memset(ptr,0,sizeof(TRACE_REQ));
  ptr->pBuf = pws->binSnippet;
  ptr->bufLen = ptr->actLen = pws->iByteLen;
  ptr->actStarttime = pws->tStart;
  ptr->actEndtime = pws->tEnd;
  ptr->pClientData = (void *)pws;

  return(EWDB_RETURN_SUCCESS);
   
}

int MergeSnippets(TRACE_REQ * ptr1, TRACE_REQ * ptr2)
{
  char * pTemp;
  
  if(ptr1->actStarttime >= ptr2->actEndtime)
  {
    if((long)ptr1->bufLen >= (ptr1->actLen + ptr2->actLen))
    {
      /* simple merge ptr2 first */
      pTemp = malloc(ptr1->actLen);
      if(!pTemp)
      {
        logit("t","MergeSnippets(): Error allocating %d bytes.\n",
              ptr1->actLen);
        return(WS_ERR_MEMORY);
      }
      
      /*else*/
      memcpy(pTemp, ptr1->pBuf, ptr1->actLen);
      memcpy(ptr1->pBuf, ptr2->pBuf, ptr2->actLen);
      memcpy(&(ptr1->pBuf[ptr2->actLen]), pTemp, ptr1->actLen);
      ptr1->actLen += ptr2->actLen;

      /* update our starttime */
      ptr1->actStarttime = ptr2->actStarttime;

      /* free the temporary buffer */
      free(pTemp);

    }  /* end if buffer large enough to hold both snippets */
    else
    {
       sprintf(errText,"Snippet Too Large: Exceeds MaxSnippetSize(%d).  "
               "Snippet (%.3f - %.3f) SCN(%s,%s,%s) SnippetSize(%d)\n",
               (long)ptr1->bufLen, 
               ptr2->actStarttime, ptr1->actEndtime,
               ptr1->sta, ptr1->chan, ptr1->net,
               ptr1->actLen + ptr2->actLen
              );
       dbtsv_status( TypeError, ERR_SNIPPET_TOO_BIG, errText );
       return(WS_ERR_BUFFER_OVERFLOW);
    }
  }  /* end if(ptr1->actStarttime >= ptr2->actEndtime) */
  else if(ptr2->actStarttime >= ptr1->actEndtime)
  {
    if((long)ptr1->bufLen >= (ptr1->actLen + ptr2->actLen))
    {
      memcpy(&(ptr1->pBuf[ptr1->actLen]), ptr2->pBuf, ptr2->actLen);
      ptr1->actLen += ptr2->actLen;

      /* update our endtime */
      ptr1->actEndtime = ptr2->actEndtime;

    }  /* end if buffer large enough to hold both snippets */
    else
    {
       sprintf(errText,"Snippet Too Large: Exceeds MaxSnippetSize(%d).  "
               "Snippet (%.3f - %.3f) SCN(%s,%s,%s) SnippetSize(%d)\n",
               ptr1->actStarttime, ptr2->actEndtime,
               ptr1->sta, ptr1->chan, ptr1->net,
               ptr1->actLen + ptr2->actLen
              );
       dbtsv_status( TypeError, ERR_SNIPPET_TOO_BIG, errText );
       return(WS_ERR_BUFFER_OVERFLOW);
    }
  }
  else
  {
    /* this is too advanced for us, issue a status message,
       so that we know when this happens */
    sprintf(errText,"Snippets Overlap: Unable to Merge.  "
            "Snippets (%.3f - %.3f),(%.3f - %.3f) "
            "SCN(%s,%s,%s)\n",
            ptr1->actStarttime, ptr1->actEndtime,
            ptr2->actStarttime, ptr2->actEndtime,
            ptr1->sta, ptr1->chan, ptr1->net
           );
    dbtsv_status( TypeError, ERR_SNIPPET_OVERLAP, errText );
    return(WS_ERR_UNKNOWN);
  }

  return(WS_ERR_NONE);
}  /* MergeSnippets() */


int CheckForSnippetRequests(int IN_bModifyAttemptParams)
{
   int          rc;
   int          i;
   int           nTrReq=0;  

   TRACE_REQ *  ptrCurrent = NULL;

   EWDB_WaveformStruct ExistingWaveform, * pSecondWaveform = NULL;
   TRACE_REQ    SecondTraceReq;
   
   time_t tThreshhold;
   int iNumItemsFound, iNumItemsRetrieved;
   EWDBid idNewWaveform;

   EWDB_SnippetRequestStruct * psrCurrent = NULL;
   int bNeedNewRequestAfterEnd, bNeedNewRequestBeforeStart;

   /* intialize tThreshhold.  Set to 0, if IN_bModifyAttemptParams
      is FALSE, since that indicates a special search
    *************************************************************/
   if(!IN_bModifyAttemptParams)
     tThreshhold=0;
   else
     time(&tThreshhold);

   /* DK 020502 SnipReqArray should be piecemeal initialized by 
      ewdb_api_GetSnippetRequestList()  */

   /* call ewdb_api_GetSnippetRequestList (SnipReqArray, iNumItemsFound, iNumItemsRetrieved) */
   rc = ewdb_api_GetSnippetRequestList(SnipReqArray, tThreshhold, iRequestGroup,
                                       &iNumItemsFound, &iNumItemsRetrieved,
                                       MaxTraces);
   if(Debug_tr_sv >= DEBUG_OTF_INFO) 
     logit("", "ewdb_api_GetSnippetRequestList() returned(%d) with (%d - %d) snippetreqs.\n",
											 rc, iNumItemsRetrieved, iNumItemsFound);


   if(rc == EWDB_RETURN_FAILURE)
   {
     if(Debug_tr_sv >= DEBUG_OTF_ERROR)
       logit("et","ora_trace_fetch: CheckForSnippetRequests(): "
           "Error in DB call to fetch requests! returning!\n");
     return(EWDB_RETURN_FAILURE);
   }

   if(iNumItemsFound == 0)
   {
     /* nothing found, take a breather */
     return(EWDB_RETURN_WARNING);
   }
   else
   {
     if(Debug_tr_sv >= DEBUG_OTF_WARNING)
     logit("","CheckForSnippetRequests():  Found %d reqs this time through!\n",iNumItemsRetrieved);
   }

   /* for each snipreq in SnipReqArray (iNumItemsRetrieved) */
   for(i=0; i < iNumItemsRetrieved; i++)
   {
     /*  convert to TraceReq 
         assuming snipreq2trreq() will reinitialize the current TraceReq record,
         and not do any movement of memory, only write data into the TraceReq record.
      **************************************************************/
     rc = snipreq2trreq(&(SnipReqArray[i]), &(TraceReq[i]));

     if(rc != WS_ERR_NONE)
     {
       /* log the error, and delete the request so we don't see it again */
       logit("t","snipreq2trreq() returned %d.  "
                 "Assuming bad snippet request.\n",
             rc);
       sprintf(errText,"Bad Snippet Request: id=%d, idChan=%d, tStart=%.2f, "
               "tEnd =%.2f!  Deleting\n",
               SnipReqArray[i].idSnipReq, SnipReqArray[i].idChan, 
               SnipReqArray[i].tStart, SnipReqArray[i].tEnd);

       dbtsv_status( TypeError, ERR_BAD_SNIPREQ, errText );

       ewdb_api_DeleteSnippetRequest(SnipReqArray[i].idSnipReq, 
                                     SnipReqArray[i].iLockTime);

       /* copy the last snipreq over the current, and reduce the list size by 1 */
       memcpy(&(SnipReqArray[i]), &(SnipReqArray[iNumItemsRetrieved-1]), 
              sizeof(EWDB_SnippetRequestStruct));

       /* shorten the list length by 1 */
       iNumItemsRetrieved--;

       /* redo this record since we replaced the contents */
       i--;
     }
     if(Debug_tr_sv >= DEBUG_OTF_ALL) logit("","DEBUG: iLockTime Check 1 (%10d:%d)\n",
           SnipReqArray[i].idSnipReq,SnipReqArray[i].iLockTime);

   }  /* for i in iNumItemsRetrieved */

   /* submit list to wsClientIII routines */
   rc = wsPrepRequestList(TraceReq, iNumItemsRetrieved, WS_TRACE_BINARY,
                          TimeoutSeconds, wsHandle);
   if(rc != WS_ERR_NONE)
   {
     if(Debug_tr_sv >= DEBUG_OTF_ERROR)
       logit("et", "ora_trace_fetch: wsPrepRequestList() returned %d.  "
           "Returning!\n", rc);
     return(EWDB_RETURN_FAILURE);
   }

   /* while wsGetNextTraceFromList() returns !done */
   while((rc = wsGetNextTraceFromRequestList(TraceReq, wsHandle, &ptrCurrent)) 
         != WS_ERR_LIST_FINISHED)
   {
     /***********************************************************
        WARNING WARNING WARNING WARNING WARNING WARNING WARNING
        The wsClientIII routines use a single snippet buffer that
        is malloced durin wsInitialize().  The caller should
        NEVER NEVER NEVER Free ptrCurrent->pBuf, and should copy
        the contents of ptrCurrent->pBuf before the next call
        to wsGetNextTraceFromRequestList(), because the old data
        will be overwritten!!!!!!
        DK 0130 2003
        WARNING WARNING WARNING WARNING WARNING WARNING WARNING
     ***********************************************************/
     if(ptrCurrent)
     {
       psrCurrent = ((EWDB_SnippetRequestStruct*)(ptrCurrent->pClientData));
       if(Debug_tr_sv >= DEBUG_OTF_ALL) logit("","DEBUG: iLockTime Check 2 (%10d:%d)\n",
             psrCurrent->idSnipReq,psrCurrent->iLockTime);
     }
     else
     {
       /* must be some temporary error, so that we didn't get a TRACE_REQ to 
          reference.  just continue. */
       if(Debug_tr_sv >= DEBUG_OTF_WARNING)
         logit("e","NULL TraceReq returned, expecting it was a temp error.\n");
       continue;
     }

     /* DK 20020315 Added heartbeat check here so that statmgr doesn't think
        we died just because we are busy processing trace requests. */
     /* send ora_trace_fetch's heartbeat (if due)
     **********************************/
     if(time(&timeNow) - timeLastBeat  >=  HeartBeatInt ) 
     {
       timeLastBeat = timeNow;
       dbtsv_status( TypeHeartBeat, 0, "" ); 
     }

     /* DEBUG INFO */
     if(Debug_tr_sv >= DEBUG_OTF_INFO)
     {
       logit("","Request: %d (%.0f - %.0f)  Result: (%.0f - %.0f)",
             psrCurrent->idSnipReq,
             ptrCurrent->reqStarttime, ptrCurrent->reqEndtime,
             ptrCurrent->actStarttime, ptrCurrent->actEndtime
            );

       if(psrCurrent->idExistingWaveform != 0)
         logit(""," - idExistingWavefrom = %d\n", psrCurrent->idExistingWaveform);
       else
         logit("","\n");
     }
     /* END DEBUG INFO */

     /* if(successful) */
     if(rc == WS_ERR_NONE)
     {
       if(Debug_tr_sv >= DEBUG_OTF_INFO)
       {
         logit("","SUCCESS - Retrieved snippet from wave_server for (%s,%s,%s)!\n",
               ptrCurrent->sta, ptrCurrent->chan, ptrCurrent->net);
       }

       /* initialize pSecondWaveform to NULL */
       pSecondWaveform = NULL;


       /* if there is an existing waveform, then we have
          to go through a special process */
       if(psrCurrent->idExistingWaveform)
       {
         if(Debug_tr_sv >= DEBUG_OTF_INFO)
           logit("","CheckForSnippetRequests(): Existing waveform found: "
                  " pTrReq(%u), idSnipReq(%d) chan(%d), wave(%d)\n",
               ptrCurrent, 
               psrCurrent->idSnipReq, psrCurrent->idChan, psrCurrent->idExistingWaveform);

         /* initialize ExistingWaveform with idWaveform */
         memset(&ExistingWaveform,0,sizeof(ExistingWaveform));
         ExistingWaveform.idWaveform = psrCurrent->idExistingWaveform;

         /* if we already have a snippet, get it */
         if(ewdb_api_GetWaveformDesc(&ExistingWaveform) == EWDB_RETURN_SUCCESS)
         {
           if(ExistingWaveform.binSnippet = malloc(ExistingWaveform.iByteLen))
           {
             if(ewdb_api_GetWaveformSnippet(ExistingWaveform.idWaveform, 
                                        ExistingWaveform.binSnippet, 
                                        ExistingWaveform.iByteLen)
                == EWDB_RETURN_SUCCESS)
             {
               pSecondWaveform = &ExistingWaveform;
               /* assuming wfs2trreq reinitializes SecondTraceReq */
               waveformstruct2trreq(pSecondWaveform, &SecondTraceReq);
             }
             else
             {
               logit("t","ERROR: failed to retrieve Snippet from DB for idWaveform(%d)-(%d,%d)\n",
                     ExistingWaveform.idWaveform, ExistingWaveform.idChan,
                     psrCurrent->idChan);
               free(ExistingWaveform.binSnippet);
             }
           } /* end if malloc(binSnippet) */
           else
           {
             sprintf(errText,"ora_trace_fetch: CheckForSnippetRequests() failed due to "
                               "memory allocation error of %d bytes for snippet buffer.\n",
                     ExistingWaveform.iByteLen);
             dbtsv_status( TypeError, ERR_XALLOC_ERROR, errText ); 
           }
         }  /* end if ewdb_api_GetWaveformDesc() successful */
         
         if(pSecondWaveform)  /* We got the existing snippet */
         {
           /* MergeSnippets frees exactly the memory it allocates. DK 01302003 */
           rc = MergeSnippets(ptrCurrent,&SecondTraceReq);
           if(rc == WS_ERR_MEMORY)
           {
             sprintf(errText,"ora_trace_fetch: MergeSnippets() failed due to "
                               "memory allocation error.\n");
             dbtsv_status( TypeError, ERR_XALLOC_ERROR, errText ); 

             logit("t","%s(): MergeSnippets() failed due to memory error.  "
                   "Returning error!\n", "CheckForSnippetRequests()");
             return(EWDB_RETURN_FAILURE);
           }
           else if(rc == WS_ERR_NONE)
           {
             ExistingWaveform.tStart   = ptrCurrent->actStarttime;
             ExistingWaveform.tEnd     = ptrCurrent->actEndtime;
             ExistingWaveform.iByteLen = ptrCurrent->actLen;
             free(ExistingWaveform.binSnippet);
             ExistingWaveform.binSnippet = ptrCurrent->pBuf;
           }
           else  /* rc == some error or warning other than memory */
           {
             logit("t","CheckForSnippetRequests(): MergeSnippets() failed due to "
                       "unexpected error(%d).\n",
                   rc);
             free(ExistingWaveform.binSnippet);
             pSecondWaveform = NULL;
             goto SomethingWentWrong;
           }
         }  /* end if we retrieved existing snippet */
         else  
         {
           logit("e","idWaveform(%d) was Not NULL, but no waveform retrieved.\n",
             psrCurrent->idExistingWaveform);
         }

       }   /* end if existing waveform */

       if(pSecondWaveform)
       {
         /* There is an existing waveform, so all we need to do is update it,
            not insert a new one and deal with event binding */
         rc = ewdb_api_UpdateWaveform(pSecondWaveform);
         if(rc != EWDB_RETURN_SUCCESS)
         {
           sprintf(errText,"ora_trace_fetch: %s failed with rc=%d for idSnipReq [%d]\n",
                   "ewdb_api_UpdateWaveform()",rc,
                   psrCurrent->idSnipReq);
           dbtsv_status(TypeError, ERR_STUFF_SNPT, errText);
         }
         idNewWaveform = pSecondWaveform->idWaveform;
       }
       else
       {
         rc =  ewdb_apps_putaway_NextSnippetForAnEvent(psrCurrent->idEvent,
                    ptrCurrent, &idNewWaveform,
                    &(psrCurrent->idChan));
         if(rc != EWDB_RETURN_SUCCESS)
         {
           sprintf(errText,"ora_trace_fetch: %s failed with rc=%d for idSnipReq [%d]\n",
                   "ewdb_apps_putaway_NextSnippetForAnEvent()",rc,
                   psrCurrent->idSnipReq);
           dbtsv_status(TypeError, ERR_STUFF_SNPT, errText);
         }
         else
         {
           psrCurrent->idExistingWaveform = idNewWaveform;
           /* Added by DK 050902  
              We weren't getting the waveform recorded
              in the DB after a partial.
            **********************************************/
         }
       }

       if(rc == EWDB_RETURN_SUCCESS)
       {
         if(ptrCurrent->actStarttime >= (psrCurrent->tStart + dGracePeriod ))
           bNeedNewRequestBeforeStart = TRUE;
         else
           bNeedNewRequestBeforeStart = FALSE;

         if(ptrCurrent->actEndtime <= (psrCurrent->tEnd - dGracePeriod ))
           bNeedNewRequestAfterEnd = TRUE;
         else
           bNeedNewRequestAfterEnd = FALSE;

         if(bNeedNewRequestAfterEnd || bNeedNewRequestBeforeStart)
         {
           /* we didn't get enough data to satisfy the entire request. 
              the way stuff is currently implemented, we can only add
              on to one side of an existing waveform, so if we are missing
              data on both sides, then choose the after stuff, because we
              are much more likely to get new data into the tank that is
              after what we got instead of before, especially considering
              that most of the earthworm system is dependent upon time
              always flowing forward. */

           if(bNeedNewRequestAfterEnd)
             /* to prevent overlapping data at a trace buf boundary point. */
             /* AFTER THINKING ABOUT THIS FOR A WHILE(the .01 hack), 
                I DO NOT THINK THAT 
                IT WILL WORK FOR DATA SLOWER THAN 100HZ.  Wave_server may
                screw up our request, and give us extra data, including data
                from the previous packet, which would be a duplicate of what
                we just got, so we would have an overlap and the snippet merge
                would likely fail if it is still simple.  DK CLEANUP 030802
              *****************************************************************/
             psrCurrent->tStart = ptrCurrent->actEndtime + .01; /* .01 = hack */
           else
             /* to prevent overlapping data at a trace buf boundary point. */
             psrCurrent->tEnd = ptrCurrent->actStarttime - .01; /* .01 = hack */

           if(Debug_tr_sv >= DEBUG_OTF_INFO)
             logit("t","Got a Partial.  Updating snippet request:\n"
                 "idChan %d, idSnipReq %d, idExistingWaveform %d,\n"
                 "idEvent %d (%0f - %0f)\n",
                 psrCurrent->idChan, psrCurrent->idSnipReq,
                 psrCurrent->idExistingWaveform, psrCurrent->idEvent,
                 psrCurrent->tStart, psrCurrent->tEnd);

           if(Debug_tr_sv >= DEBUG_OTF_ALL) 
             logit("","DEBUG: iLockTime Check 5 (%10d:%d)\n",
                 psrCurrent->idSnipReq,psrCurrent->iLockTime);

           rc = ScheduleNextAttemptForSnippetRequest(psrCurrent, 
                                                     SNIPPET_REQUEST_ATTEMPT_PARTIAL_SUCCESS);
           if(rc == EWDB_RETURN_FAILURE)
           {
             sprintf(errText,"ERROR: Failed to schedule new request for PARTIAL SnipReq %d\n", 
                     psrCurrent->idSnipReq); 
             dbtsv_status(TypeError, ERR_SCHED_SNIPREQ, errText);
           }
           else if(rc == EWDB_RETURN_WARNING)
           {
             /* This means that we exhausted all of the allotted attempts
                to fulfill the request, so the request was deleted as part
                of the update operation.
              ************************************************************/
             if(Debug_tr_sv >= DEBUG_OTF_ERROR)
               logit("t","!!!!WARNING!!!! Unable to obtain snippet data for\n"
                 "request(%s,%s,%s) (%.3f-%.3f).  Request Deleted!\n",
                 ptrCurrent->sta, ptrCurrent->chan, ptrCurrent->net,
                 psrCurrent->tStart, psrCurrent->tEnd);
           }
           else
           {
             rc = ewdb_api_UpdateSnippetRequest(psrCurrent, 
                                                IN_bModifyAttemptParams,
                                                TRUE /*IN_bModifyResultParams*/);
             if(rc== EWDB_RETURN_FAILURE)
             {
               if(Debug_tr_sv >= DEBUG_OTF_ERROR)
                 logit("t","ERROR:  Unable to update snippet request"
                     "(%s,%s,%s) (%.3f-%.3f).\n",
                     ptrCurrent->sta, ptrCurrent->chan, ptrCurrent->net,
                     psrCurrent->tStart, psrCurrent->tEnd);
             }
           }  /* end if rc=ScheduleNextAttemptForSnippetRequest*/
           
         }
         else  /* we got enough data to satisfy the whole request */
         {
            ewdb_api_DeleteSnippetRequest(psrCurrent->idSnipReq, psrCurrent->iLockTime);
         }
       }  /* end if ewdb_apps_putaway_NextSnippetForAnEvent() returned success */
     }    /* end if rc=wsGetNextTraceFromRequestList() successful */
     else 
     {
       if(rc == WS_ERR_BUFFER_OVERFLOW)
       {
         sprintf(errText,"ora_trace_fetch:  ERROR! Requested snippet "
                         "(%s,%s,%s) (%.3f - %.3f)  exceeded "
                         " the maximum snippet size (%d)!  Snippet not "
                         " retrieved!!\n",
                 ptrCurrent->sta, ptrCurrent->chan, ptrCurrent->net,
                 ptrCurrent->reqStarttime, ptrCurrent->reqEndtime,
                 ptrCurrent->actLen
                );
         dbtsv_status(TypeError, ERR_SNIPPET_TOO_BIG, errText);
       }
       else if(rc == WS_ERR_SCN_NOT_IN_MENU)
       {
         if(Debug_tr_sv >= DEBUG_OTF_ERROR)
           logit("","ERROR: (%s,%s,%s) not found in any menu.\n",
               ptrCurrent->sta, ptrCurrent->chan, ptrCurrent->net);
       }
       else if(rc == WS_ERR_TIMEOUT)
       {
         if(Debug_tr_sv >= DEBUG_OTF_ERROR)
           logit("","ERROR: Request for (%s,%s,%s) (%.2f - %.2f) TIMED OUT.\n",
               ptrCurrent->sta, ptrCurrent->chan, ptrCurrent->net,
               ptrCurrent->reqStarttime, ptrCurrent->reqEndtime);
       }
       else if(rc == WS_ERR_NO_CONNECTION)
       {
         if(Debug_tr_sv >= DEBUG_OTF_ERROR)
           logit("","ERROR: wsGetNextTraceFromRequestList(%s,%s,%s) failed, "
                    "was NOT ABLE TO CONNECT to wave server.\n",
               ptrCurrent->sta, ptrCurrent->chan, ptrCurrent->net);
       }

       else if(rc == WS_WRN_FLAGGED)
       {
         if(Debug_tr_sv >= DEBUG_OTF_ERROR)
           logit("","WARNING: Data requested for (%s,%s,%s) (%.2f - %.2f) was missing from wave_server.\n",
                 ptrCurrent->sta, ptrCurrent->chan, ptrCurrent->net,
                 ptrCurrent->reqStarttime, ptrCurrent->reqEndtime);
       }

       else
       {
         if(Debug_tr_sv >= DEBUG_OTF_ERROR)
           logit("t","Error(%d) retrieving snippet for req:\n"
                     "\t\t (%s,%s,%s - %d) (%.2f - %.2f) (F%c)\n",
                 rc, ptrCurrent->sta, ptrCurrent->chan, ptrCurrent->net,
                 ((EWDB_SnippetRequestStruct *)(ptrCurrent->pClientData))->idSnipReq,
                 ptrCurrent->reqStarttime, ptrCurrent->reqEndtime,
                 ptrCurrent->retFlag?ptrCurrent->retFlag:' '
                );
       }

       SomethingWentWrong:

       /* something went wrong, update the snipreq if desired */
       if(IN_bModifyAttemptParams)
       {
           if(Debug_tr_sv >= DEBUG_OTF_WARNING)
             logit("t","Got SQUAT.  Updating snippet request:\n"
                   "idChan %d, idSnipReq %d, idExistingWaveform %d, (%0f - %0f)\n\n",
                   psrCurrent->idChan, psrCurrent->idSnipReq,
                   psrCurrent->idExistingWaveform, 
                   psrCurrent->tStart, psrCurrent->tEnd);

           rc = ScheduleNextAttemptForSnippetRequest(psrCurrent, 
                                                     SNIPPET_REQUEST_ATTEMPT_FAILURE);
           if(rc == EWDB_RETURN_FAILURE)
           {
             sprintf(errText,"ERROR: Failed to schedule new request for FAILED SnipReq %d\n",
                     psrCurrent->idSnipReq);
             dbtsv_status(TypeError, ERR_SCHED_SNIPREQ, errText);
           }
           else if(rc == EWDB_RETURN_WARNING)
           {
             /* This means that we exhausted all of the allotted attempts
                to fulfill the request, so the request was deleted as part
                of the update operation.
              ************************************************************/
              logit("t","!!!!WARNING!!!! Unable to obtain snippet data for request"
                 "(%s,%s,%s) (%.3f-%.3f).  Request Deleted!\n",
                 ptrCurrent->sta, ptrCurrent->chan, ptrCurrent->net,
                 psrCurrent->tStart, psrCurrent->tEnd);
           }
           else
           {
             rc = ewdb_api_UpdateSnippetRequest(psrCurrent,  
                                                IN_bModifyAttemptParams,
                                                FALSE /*IN_bModifyResultParams*/);
             if(rc== EWDB_RETURN_FAILURE)
             {
             logit("t","ERROR:  Unable to update snippet request"
               "(%s,%s,%s) (%.3f-%.3f).\n",
               ptrCurrent->sta, ptrCurrent->chan, ptrCurrent->net,
               psrCurrent->tStart, psrCurrent->tEnd);
             }
           }  /* end if rc=ScheduleNextAttemptForSnippetRequest*/
       }  /* end if IN_bModifyAttemptParams */
     }    /* end else from if wsGetNextTraceFromRequestList() successful */
   }      /* end while( list not finished ) */

   return(EWDB_RETURN_SUCCESS);
}  /* end CheckForSnippetRequests() */



