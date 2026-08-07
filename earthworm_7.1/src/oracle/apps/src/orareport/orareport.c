/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: orareport.c,v 1.30 2002/10/08 18:15:43 lucky Exp $
 *
 *    Revision history:
 *     $Log: orareport.c,v $
 *     Revision 1.30  2002/10/08 18:15:43  lucky
 *     Added nearest town option to alarm formats
 *
 *     Revision 1.29  2002/06/07 17:07:29  patton
 *     Made logit changes.
 *
 *     Revision 1.28  2002/06/07 17:00:10  lucky
 *     *** empty log message ***
 *
 *     Revision 1.27  2001/11/28 21:25:55  lucky
 *     Added return value to the parameter list of DetermineAlarms
 *
 *     Revision 1.26  2001/10/26 17:11:19  bogaert
 *     Test log
 *
 *     Revision 1.25  2001/07/31 20:52:41  lucky
 *     After API cleanup and bug fixes with alarms.
 *
 *     Revision 1.24  2001/07/28 00:43:53  lucky
 *      State of the code after debugging and testing prior to v6.0 release freeze.
 *
 *     Revision 1.23  2001/07/20 17:36:01  lucky
 *     Added InvocationString to DetermineAlarms and ExecuteAlarms
 *
 *     Revision 1.22  2001/07/10 16:39:24  lucky
 *     Added several useful logits so that we know when and what we attempted to insert
 *     before we failed miserably.
 *
 *     Revision 1.21  2001/07/01 21:55:27  davidk
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
 *     Revision 1.20  2001/06/26 18:07:49  lucky
 *     Added NetworkCode configuration parameter -- it is needed to build
 *     CUBE format messages in QDDS (alarms)
 *
 *     Revision 1.19  2001/05/15 02:15:33  davidk
 *     Moved functions around between the apps, DB API, and DB API INTERNAL
 *     levels.  Renamed functions and files.  Added support for amplitude
 *     magnitude types.  Reformatted makefiles.
 *
 *     Revision 1.16  2001/04/13 21:31:50  lucky
 *     Updated lengths of database connection parameters to EQP_MAXWORD in
 *     order to conform with the definitions in alarms.h
 *
 *     Revision 1.15  2001/03/26 23:01:08  lucky
 *     Fixed the call to DetermineAlarms
 *
 *     Revision 1.14  2001/03/19 17:49:05  lucky
 *     Added the alarms section
 *
 *     Revision 1.13  2001/02/28 17:29:10  lucky
 *     Massive schema redesign and cleanup.
 *
 *     Revision 1.12  2001/02/21 11:43:57  davidk
 *     Added debug messages that get logged if the Debug flag is set in
 *     the config file.  Set Debug to max if Debug flag is set in
 *     the config file.  Removed (what seemed like) an unneccessary nested
 *     loop within the main loop.
 *
 *     Revision 1.11  2000/10/11 20:18:31  lucky
 *     Free up all allocated channels, not just the ones that were filled in.
 *
 *     Revision 1.10  2000/10/11 17:58:09  lucky
 *     Calling free to free up pChanInfo structs allocated inside ArcMsg2EWEvent
 *
 *     Revision 1.9  2000/10/10 20:55:12  lucky
 *     Fixed the call to InitEWEvent
 *
 *     Revision 1.8  2000/09/12 18:15:15  lucky
 *     Plugged memory leak with pChanInfos -- we now free them after the calls
 *     to ArcMsg2EWEvent and ewdb_apps_PutDBEventInfo
 *
 *     Revision 1.7  2000/08/28 15:37:57  lucky
 *     Fixed the arguments to ewdb_apps_PutDBEventInfo
 *
 *     Revision 1.6  2000/08/25 15:59:06  lucky
 *     Added several Debug statements (only active in Debug mode)
 *
 *     Revision 1.5  2000/08/22 18:10:31  lucky
 *     Replaced LoadHinvArc with calls to ArcMsg2EWEvent and PutDBEvent
 *
 *     Revision 1.4  2000/07/27 16:08:05  lucky
 *     Changed MAX_BYTES_PER_EQ to DB_MAX_BYTES_PER_EQ
 *
 *     Revision 1.3  2000/07/24 20:44:56  lucky
 *     Implemented global limits to module, installation, ring, and message type strings.
 *
 *     Revision 1.2  2000/03/30 18:15:33  davidk
 *     modified to work with schema2 instead of schema1.  Uses
 *     EWDB_LoadHInvArc() instead of PutArc() but the functions are
 *     pretty much the same.
 *
 *     Revision 1.1  1999/11/09 16:37:33  lucky
 *     Initial revision
 *
 *
 */


/*
 * orareport.c : Grabs final event messages and stuffs 
 *              all the info into a database.
 */

 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <earthworm.h>
#include <ew_event_info.h>
#include <kom.h>
#include <mem_circ_queue.h>
#include <transport.h>
#include <alarms.h>
#include <merge.h>
#include <ewdb_ora_api.h>
#include <ewdb_apps_utils.h>


#define MAX_STR    255
#define MAXLINE    1000
#define MAX_NAME_LEN  50
#define MAXCOLS    100
#define MAXTXT          150

#define BUF_SIZE DB_MAX_BYTES_PER_EQ    /* maximum size for event msg  */

#define		INIT_NUM_ALARMS		100
#define		INIT_NUM_MERGES		100


/* Functions in this source file 
 *******************************/
void  dbrpt_config ( char * );
void  dbrpt_lookup ( void );
void  dbrpt_status ( unsigned char, short, char * );
thr_ret Stacker (void *);
thr_ret Processor (void *);


/* Thread stuff */
#define THREAD_STACK 8192
static unsigned tidProcessor;    /* Processor thread id */
static unsigned tidStacker;      /* Thread moving messages from InRing */
                                 /* to MsgQueue */
int StackerStatus = 0;           /* 0=> Stacker thread ok. <0 => dead */
int ProcessorStatus = 0;         /* 0=> Processor thread ok. <0 => dead */

/* The message queue
 *******************/
#define QUEUE_SIZE      50      /* How many msgs can we queue */
QUEUE   MsgQueue;               /* from queue.h */



/* Transport global variables
 ****************************/
static  SHM_INFO  Region;      /* shared memory region to use for i/o    */

#define   MAXLOGO   2
MSG_LOGO  GetLogo[MAXLOGO];    /* array for requesting module,type,instid */
short     nLogo;
pid_t MyPid;            /* Our own pid, sent with heartbeat for restart 
                           purposes */
char         LogFileName[128];  /* Name of LogFile to use with logi() */
        
time_t timeNow;
/* Globals to set from configuration file
 ****************************************/
static char    RingName[MAX_RING_STR];        /* name of transport ring for i/o     */
static char    MyModName[MAX_MOD_STR];       /* speak as this module name/id       */
static char    NetworkCode[10];       /* speak as this module name/id       */
static int     LogSwitch;           /* 0 if no logfile should be written  */
static long    HeartBeatInt;        /* seconds between heartbeats         */
char  DBservice[EQP_MAXWORD];       /* Oracle instance to interact with   */
char  DBuser[EQP_MAXWORD];          /* UserId to connect to database as   */
char  DBpassword[EQP_MAXWORD];      /* Password to datasource             */
int   Debug;                        /* if non-zero, print debug messages  */
int   DoAlarms;                     /* if non-zero, run the alarms        */
int   DoMerge;                     /* if non-zero, check for merges       */
int   MaxMergesPerEvent;           /* an event can be merged with this many others */
int   ValidateQdds;
int   minPopulation;
int   showPopulation;


EWDB_MergeCriteria	MergeCriteria;

/* Things to look up in the earthworm.h tables with getutil.c functions
 **********************************************************************/
static long          RingKey;       /* key of transport ring for i/o      */
static unsigned char InstId;        /* local installation id              */
static unsigned char MyModId;       /* Module Id for this program         */
static unsigned char TypeHeartBeat; 
static unsigned char TypeError;
static unsigned char TypeHyp2000Arc;
static unsigned char TypeH71Sum;

/* Error messages used by orareport 
 **********************************/
#define  ERR_MISSMSG       0   /* message missed in transport ring       */
#define  ERR_TOOBIG        1   /* retreived msg too large for buffer     */
#define  ERR_NOTRACK       2   /* msg retreived; tracking limit exceeded */
#define  ERR_API_INIT_FAILED 3 /* ORA_API_INIT failed */
#define  ERR_PUTARC_FAILED 4   /* msg retreived; tracking limit exceeded */
#define  ERR_UNKNOWN       5   /* msg retreived; unkown error */
#define  ERR_QUEUE         6   /* trouble with the MsgQueue operation */


static char Text[MAXTXT];      /* string for log/error messages          */


void zeropad(char * astring)
{
  int i;
  for(i=0;astring[i]!=0;i++)
  {
    if(astring[i]== ' ')
      astring[i]='0';
  }
}

main( int argc, char **argv )
{
  /* these variables are original to menlo_report.c */
	long       	timeNow;          /* current time                   */       
	long       	timeLastBeat;     /* time last heartbeat was sent   */
	long       	recsize;    /* size of retrieved message      */
	MSG_LOGO   	reclogo;    /* logo of retrieved message      */
	char		*flushmsg;


  /* Check command line arguments 
  ******************************/
  if ( argc != 2 )
  {
    fprintf( stderr, "Usage: orareport <configfile>\n" );
    exit( 0 );
  }
  Debug=0;  /* assume no debug messages until reading config file */

  /* Initialize name of log-file & open it 
  ***************************************/
  /* Created LogFileName, to be obtained from config file name
     instead of using the fixed "orareport" for logname. */
  strcpy(LogFileName,argv[1]);
  LogFileName[strlen(argv[1])-2/*.d*/]=0;

  logit_init( LogFileName, 0, 1024, 1 );  

  /* Read the configuration file(s)
  ********************************/
  dbrpt_config( argv[1] );
  logit( "" , "orareport: Read command file <%s>\n", argv[1] );

  /* Get our own Pid for restart purposes
  ***************************************/
  MyPid = getpid();
  if(MyPid == -1)
  {
    logit("e","orareport: Cannot get pid. Exiting.\n");
    exit(0);
  }

  /* Look up important info from earthworm.h tables
  ************************************************/
  dbrpt_lookup();
  
  /* Reinitialize logit to the desired logging level
   *************************************************/
  logit_init( LogFileName, (short) MyModId, 1024, LogSwitch );

  /* Attach to Input/Output shared memory ring 
  *******************************************/
  tport_attach( &Region, RingKey );
  logit( "", "orareport: Attached to public memory region %s: %d\n", 
    RingName, RingKey );

  /* Set database Debug flag */
  if(Debug)
    ewdb_api_Set_Debug(-1);

  /* Open connection to database
  *****************************/
  if(ewdb_api_Init(DBuser, DBpassword, DBservice) != EWDB_RETURN_SUCCESS)
  {
    logit( "", "orareport: Trouble connecting to database; exiting!\n" );
    dbrpt_status( TypeError, ERR_API_INIT_FAILED, 
                 "Failed to initialize the ORA_API. Exiting!" );
    exit( -1 );
  }



  /* Force a heartbeat to be issued in first pass thru main loop
  *************************************************************/
  timeLastBeat = time(&timeNow) - HeartBeatInt - 1;



  /*------------------ setup done; start main loop -----------------------*/
  if(Debug) 
    logit("e","orareport: starting to watch for archive messages\n");

/* Flush the transport ring
   ************************/
    if ((flushmsg = (char *) malloc (DB_MAX_BYTES_PER_EQ)) ==  NULL)
    {
        logit ("e", "Could not allocate flushmsg; exiting.\n");
        return EW_FAILURE;
    }

    while (tport_getmsg (&Region, GetLogo, nLogo, &reclogo,
            &recsize, flushmsg, (DB_MAX_BYTES_PER_EQ - 1)) != GET_NONE)
        ;

	free (flushmsg);


	/* Create MsgQueue mutex */
	CreateMutex_ew();

	/* Allocate the message Queue
     ********************************/
	initqueue (&MsgQueue, QUEUE_SIZE, DB_MAX_BYTES_PER_EQ);


	/* Start our threads */
	if (StartThread (Stacker, (unsigned) THREAD_STACK, &tidStacker) == -1)
	{
		logit ("", "Could not start the Stacker thread.\n");
		exit (-1);
	}
	StackerStatus = 0; /*assume the best*/

	if (StartThread (Processor, (unsigned) THREAD_STACK, &tidProcessor) == -1)
	{
		logit ("", "Could not start the Processor thread.\n");
		exit (-1);
	}
	ProcessorStatus = 0; /*assume the best*/




  while( tport_getflag(&Region) != TERMINATE  &&
         tport_getflag(&Region) != MyPid )
  {
  	/* send orareport's heartbeat
    *******************************/
    if  ( time(&timeNow) - timeLastBeat  >=  HeartBeatInt ) 
    {
      timeLastBeat = timeNow;
      dbrpt_status( TypeHeartBeat, 0, "" ); 
    }


	/* Check on our threads */
	if (ProcessorStatus < 0)
	{
		logit ("", "Processor thread died. \n");
		goto shutdown;
	}

	if (StackerStatus < 0)
	{
		logit ("", "Stacker thread died. \n");
		goto shutdown;
	}

	sleep_ew (1000);

  }      /* end while !TERMINATE */

  /*-----------------------------end of main loop-------------------------------*/



  /* Termination has been requested 
  ********************************/

shutdown:
  ewdb_api_Shutdown();         /* disconnect from database  */
  
  tport_detach( &Region ); /* detach from shared memory */
  logit( "t", "orareport: Termination requested; exiting!\n" );
  exit( 0 );
  return(0);
}


/***********************************************************************
 *  dbrpt_config() processes command file(s) using kom.c functions;    *
 *                  exits if any errors are encountered.         *
 ***********************************************************************/
void dbrpt_config( char *configfile )
{
   int      ncommand;     /* # of required commands you expect to process   */ 
   char     init[10];     /* init flags, one byte for each required command */
   int      nmiss;        /* number of required commands that were missed   */
   char    *com;
   char    *str;
   int      nfiles;
   int      success;
   int      i;

/* Set to zero one init flag for each required command 
 *****************************************************/   
   ncommand = 8;
   for( i=0; i<ncommand; i++ )  init[i] = 0;
   nLogo = 0;

   strcpy (NetworkCode, "  ");
   DoAlarms = FALSE;
   DoMerge = FALSE;
   MaxMergesPerEvent = 10;


	memset (&MergeCriteria, 0, sizeof (EWDB_MergeCriteria));


/* Open the main configuration file 
 **********************************/
   nfiles = k_open( configfile ); 
   if ( nfiles == 0 ) {
  logit ("e",
                "orareport: Error opening command file <%s>; exiting!\n", 
                 configfile );
  exit( -1 );
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
                  logit ("e", 
                          "orareport: Error opening command file <%s>; exiting!\n",
                           &com[1] );
                  exit( -1 );
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

         /* Enter installation & module to get event messages from
          ********************************************************/
  /*4*/     else if( k_its("GetEventsFrom") ) {
                if ( nLogo >= MAXLOGO ) {
                    logit ("e", 
                            "orareport: Too many <GetEventsFrom> commands in <%s>", 
                             configfile );
                    logit ("e", "; max=%d; exiting!\n", (int) MAXLOGO );
                    exit( -1 );
                }
                if( ( str=k_str() ) ) {
                   if( GetInst( str, &GetLogo[nLogo].instid ) != 0 ) {
                       logit ("e", 
                               "orareport: Invalid installation name <%s>", str ); 
                       logit ("e", " in <GetEventsFrom> cmd; exiting!\n" );
                       exit( -1 );
                   }
                   GetLogo[nLogo+1].instid = GetLogo[nLogo].instid;
                }
                if( ( str=k_str() ) ) {
                   if( GetModId( str, &GetLogo[nLogo].mod ) != 0 ) {
                       logit ("e", 
                               "orareport: Invalid module name <%s>", str ); 
                       logit ("e", " in <GetEventsFrom> cmd; exiting!\n" );
                       exit( -1 );
                   }
                   GetLogo[nLogo+1].mod = GetLogo[nLogo].mod;
                }
                if( GetType( "TYPE_HYP2000ARC", &GetLogo[nLogo].type ) != 0 ) {
                    logit ("e", 
                               "orareport: Invalid message type <TYPE_HYP2000ARC>" ); 
                    logit ("e", "; exiting!\n" );
                    exit( -1 );
                }
                nLogo++;
                init[4] = 1;
            }
  /*5*/     else if( k_its("DBservice") ) {
                str = k_str();
                if(str) strcpy( DBservice, str );
                init[5] = 1;
            }
  /*6*/     else if( k_its("DBpassword") ) {
                str = k_str();
                if(str) strcpy( DBpassword, str );
                init[6] = 1;
            }
  /*7*/     else if( k_its("DBuser") ) {
                str = k_str();
                if(str) strcpy( DBuser, str );
                init[7] = 1;
            }
            else if( k_its("Debug") ) {  /*optional command */
                Debug=1;
            }
            else if( k_its("DoAlarms") ) {  /*optional command */
                DoAlarms=TRUE;
            }
            else if( k_its("NetworkCode") ) { /* optional command */
                str = k_str();
                if(str) strcpy( NetworkCode, str );
            }
            else if( k_its("minPopulation") ) {  /*optional command */
                minPopulation = k_int();
            }
            else if( k_its("showPopulation") ) {  /*optional command */
                showPopulation = k_int();
            }

			/* Merge options */
            else if( k_its ("MaxMergesPerEvent") ) {  
                DoMerge=TRUE;
				MaxMergesPerEvent = k_int ();
            }
            else if( k_its ("MergeOriginTimeDiff") ) {  
                DoMerge=TRUE;
				MergeCriteria.OriginTimeDiff = k_val ();
            }
            else if( k_its ("MergeLatDiff") ) {  
                DoMerge=TRUE;
				MergeCriteria.LatDiff = k_val ();
            }
            else if( k_its ("MergeLonDiff") ) {  
                DoMerge=TRUE;
				MergeCriteria.LonDiff = k_val ();
            }
            else if( k_its ("MergeDepthDiff") ) {  
                DoMerge=TRUE;
				MergeCriteria.DepthDiff = k_val ();
            }

        /* Unknown command
         *****************/ 
      else {
                logit ("e", "orareport: <%s> Unknown command in <%s>.\n", 
                         com, configfile );
                continue;
            }

        /* See if there were any errors processing the command 
         *****************************************************/
            if( k_err() ) {
               logit ("e", 
                       "orareport: Bad <%s> command in <%s>; exiting!\n",
                        com, configfile );
               exit( -1 );
            }
  }
  nfiles = k_close();
   }

/* After all files are closed, check init flags for missed commands
 ******************************************************************/
   nmiss = 0;
   for ( i=0; i<ncommand; i++ )  if( !init[i] ) nmiss++;
   if ( nmiss ) {
       logit ("e", "orareport: ERROR, no " );
       if ( !init[0] )  logit ("e", "<LogFile> "       );
       if ( !init[1] )  logit ("e", "<MyModuleId> "    );
       if ( !init[2] )  logit ("e", "<RingName> "      );
       if ( !init[3] )  logit ("e", "<HeartBeatInt> "  );
       if ( !init[4] )  logit ("e", "<GetEventsFrom> " );
       if ( !init[5] )  logit ("e", "<DBservice> "     );
       if ( !init[6] )  logit ("e", "<DBpassword> "    );
       if ( !init[7] )  logit ("e", "<DBuser> "        );
       logit ("e", "command(s) in <%s>; exiting!\n", configfile );
       exit( -1 );
   }

   return;
}

/************************************************************************
 *  dbrpt_lookup( )   Look up important info from earthworm.h tables   *
 ************************************************************************/
void dbrpt_lookup( void )
{
/* Look up keys to shared memory regions
   *************************************/
   if( ( RingKey = GetKey(RingName) ) == -1 ) {
  fprintf( stderr,
           "orareport:  Invalid ring name <%s>; exiting!\n", RingName);
  exit( -1 );
   }

/* Look up installations of interest
   *********************************/
   if ( GetLocalInst( &InstId ) != 0 ) {
      fprintf( stderr, 
              "orareport: error getting local installation id; exiting!\n" );
      exit( -1 );
   }

/* Look up modules of interest
   ***************************/
   if ( GetModId( MyModName, &MyModId ) != 0 ) {
      fprintf( stderr, 
              "orareport: Invalid module name <%s>; exiting!\n", MyModName );
      exit( -1 );
   }

/* Look up message types of interest
   *********************************/
   if ( GetType( "TYPE_HEARTBEAT", &TypeHeartBeat ) != 0 ) {
      fprintf( stderr, 
              "orareport: Invalid message type <TYPE_HEARTBEAT>; exiting!\n" );
      exit( -1 );
   }
   if ( GetType( "TYPE_ERROR", &TypeError ) != 0 ) {
      fprintf( stderr, 
              "orareport: Invalid message type <TYPE_ERROR>; exiting!\n" );
      exit( -1 );
   }
   if ( GetType( "TYPE_HYP2000ARC", &TypeHyp2000Arc ) != 0 ) {
      fprintf( stderr, 
              "orareport: Invalid message type <TYPE_HYP2000ARC>; exiting!\n" );
      exit( -1 );
   }
   return;
} 

/*************************************************************************
 * dbrpt_status() builds a heartbeat or error message & puts it into    *
 *                 shared memory.  Writes errors to log file & screen.   *
 *************************************************************************/
void dbrpt_status( unsigned char type, short ierr, char *note )
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
     logit( "et", "orareport: %s\n", note );
   }

   size = strlen( msg );   /* don't include the null byte in the message */   

/* Write the message to shared memory
 ************************************/
   if( tport_putmsg( &Region, &logo, size, msg ) != PUT_OK )
   {
        if( type == TypeHeartBeat ) {
           logit("et","orareport:  Error sending heartbeat.\n" );
  }
  else if( type == TypeError ) {
           logit("et","orareport:  Error sending error:%d.\n", ierr );
  }
   }

   return;
}




/********************** Message Stacking Thread *******************
 *           Move messages from transport to memory queue         *
 ******************************************************************/
thr_ret		Stacker (void *dummy)
{
	char			eqmsg[BUF_SIZE];  /* array to hold event message    */
	int          	res;
	long         	recsize;        /* size of retrieved message */
	MSG_LOGO     	reclogo;        /* logo of retrieved message */
	int				gotMsg;


	/* Tell the main thread we're ok
	 ********************************/
	StackerStatus = 0;

	/* Start service loop, picking up trigger messages
	 **************************************************/
	while (1)
	{
		/* Get a message from transport ring
		 ************************************/
		gotMsg = FALSE;
		while (gotMsg == FALSE)
		{
			res = tport_getmsg (&Region, GetLogo, nLogo, &reclogo, 
								&recsize, eqmsg, sizeof(eqmsg)-1);

			/* Check return code; report errors if necessary
			***********************************************/
			if( res != GET_OK )
			{
				if( res == GET_NONE )
				{
					sleep_ew( 1000 );  /* no more messages; wait for new ones to arrive */
				}
				else if( res == GET_TOOBIG )
				{
					sprintf( Text,
						"Retrieved msg[%ld] (i%u m%u t%u) too big for eqmsg[%d]",
								recsize, reclogo.instid, reclogo.mod, 
								reclogo.type, sizeof(eqmsg)-1 );
					dbrpt_status( TypeError, ERR_TOOBIG, Text );
					continue;
				}
				else if( res == GET_MISS )
				{
					sprintf( Text,
						"Missed msg(s)  i%u m%u t%u  %s.",
							reclogo.instid, reclogo.mod, reclogo.type, RingName );
					dbrpt_status( TypeError, ERR_MISSMSG, Text );
				}
				else if( res == GET_NOTRACK )
				{
					sprintf( Text,
						"Msg received (i%u m%u t%u); transport.h NTRACK_GET exceeded",
							reclogo.instid, reclogo.mod, reclogo.type );
					dbrpt_status( TypeError, ERR_NOTRACK, Text );
				}
				else
				{
					sprintf( Text,
						"Msg received (i%u m%u t%u); Unrecognized error %d",
								reclogo.instid, reclogo.mod, reclogo.type, res );
					dbrpt_status( TypeError, ERR_UNKNOWN, Text );
				}  /* end elses from if res!= GET_OK */

			}   /* end if res != GET_OK */


			if(res == GET_OK || res == GET_MISS || res == GET_NOTRACK)
				gotMsg = TRUE;
		}

		/* Queue retrieved msg (res==GET_OK,GET_MISS,GET_NOTRACK)
		*********************************************************/
		RequestMutex ();
		/* put it into the queue */
		res = enqueue (&MsgQueue, eqmsg, recsize, reclogo); 
		ReleaseMutex_ew ();

		if (res != 0)
		{
			if (res == -2)  /* Serious: quit */
			{
				sprintf (Text, "internal queue error. Terminating.");
				dbrpt_status (TypeError, ERR_QUEUE, Text);
				StackerStatus = -1; /* file a complaint to the main thread */
				KillSelfThread (); /* main thread will restart us */
				exit (-1);
			}
			if (res == -1)
			{
				sprintf (Text, "queue cannot allocate memory. Lost message.");
				dbrpt_status (TypeError, ERR_QUEUE, Text);
			}
			if (res == -3) 
			{
				sprintf (Text, "Queue full. Message lost.");
				dbrpt_status (TypeError, ERR_QUEUE, Text);
			}
		} /* problem from enqueue */

	} /* while (1) */

}



/********************** Message Processing Thread ****************
 ******************************************************************/
thr_ret		Processor (void *dummy)
{

	char         			eqmsg[BUF_SIZE]; /* "raw" retrieved message */
	char					InvString[BUF_SIZE];
	char					ASCIILogo[50];
	long					recsize;
    MSG_LOGO        		reclogo;           /* logo of retrieved message */
	int						ret, gotMsg, NumFound, NumRetr, NumMerges, i;
	EWEventInfoStruct     	EventInfo;
	AlarmList				*pAlarms;
	EWDB_CandidateList		*pCandidateList;
	EWDB_MergeStruct		Merge;
	EWDB_PhenomenonStruct	Phen;
	DetAlmsParams			DAP;


	/* Tell the main thread we're ok */
	ProcessorStatus = 0;


    /* Initialize the Event structure */
    if (InitEWEvent (&EventInfo) != EW_SUCCESS)
    {
        logit ("", "Call to InitEWEvent failed.\n");
		ProcessorStatus = -1;
		KillSelfThread (); 
    }

	/* Allocate the buffer for candidate merges */
	if ((pCandidateList = (EWDB_CandidateList *) malloc 
				(MaxMergesPerEvent * sizeof (EWDB_CandidateList))) == NULL)
	{
		logit ("", "Could not malloc Candidate buffer of length %d\n", 
													MaxMergesPerEvent);
		ProcessorStatus = -1;
		KillSelfThread (); 
	}


	while (1)
	{
		gotMsg = FALSE;
	
		while (gotMsg == FALSE)
		{
			RequestMutex ();
			ret = dequeue (&MsgQueue, eqmsg, &recsize, &reclogo);
			ReleaseMutex_ew ();

			if (ret < 0)
			{
				/* empty queue, sleep for a while */
				sleep_ew (1000);
			}
			else
			{
				gotMsg = TRUE;
			}

		} /* While waiting for messages on the queue */

		ProcessorStatus = 0;

		/* Process the message 
	 	 **********************/

		/* strip out the original logo */
		sprintf (ASCIILogo,"%3d%3d%3d", reclogo.type, reclogo.mod, reclogo.instid);
		zeropad(ASCIILogo);

		if (Debug)
			logit ("t", "Processing new arc msg, Logo=%s.\n", ASCIILogo);


        if (Debug)
          logit ("t", "Calling ArcMsg2EWEvent()\n");

        /* Note: pChanInfo gets malloc inside ArcMsg2EWEvent */
        if (ArcMsg2EWEvent (&EventInfo, eqmsg, recsize) != EW_SUCCESS)
        {
          	logit ("", "Call to ArcMsg2EWEvent failed.\n");
			ProcessorStatus = -1;
			KillSelfThread (); 
        }
        
        if (Debug)
          logit ("t", "Calling ewdb_apps_PutDBEventInfo()\n");
        

		logit ("t", "Inserting Event: %d.\n", EventInfo.Event.idEvent);

		EventInfo.Event.iEventType = EWDB_EVENT_TYPE_QUAKE;

        if (ewdb_apps_PutDBEventInfo (&EventInfo, ASCIILogo, 
							(char *) NULL, 1) != EWDB_RETURN_SUCCESS)
        {
          	logit ("", "Call to ewdb_apps_PutDBEventInfo failed\n");
			ProcessorStatus = -1;
			KillSelfThread (); 
        }
        
        if (Debug)
          logit ("t", "Event %d Inserted.\n", EventInfo.Event.idEvent);

#define DO_MERGE
#ifdef DO_MERGE
		if (DoMerge == TRUE)
		{
logit ("", "merge\n");

	        if (Debug)
				logit ("t", "Looking for possible merge candidates.\n");

			/* 
			 * This will give us the list of phenomena with which
			 * this event should be merged.
			 */
			if (EWDB_RetrieveMerges (&EventInfo, pCandidateList, 
						MaxMergesPerEvent, &NumMerges, &MergeCriteria) != EW_SUCCESS)
			{
				logit ("", "Call to EWDB_RetrieveMerges failed.\n");
				ProcessorStatus = -1;
				KillSelfThread (); 
			}
logit ("", "EWDB_RetrieveMerges returned %d merges\n", NumMerges);


			if (NumMerges <= 0)
			{
				/* 
				 * No events found to merge with. In our view of things
				 * this means that we are the first observation of this
				 * phenomenon.
				 */

				Phen.idPh = -1;
				Phen.idPrefEvent = EventInfo.Event.idEvent;
				strcpy (Phen.szSource, ASCIILogo);

				if (ewdb_api_CreatePhenomenon (&Phen) != EWDB_RETURN_SUCCESS)
				{
					logit ("", "Call to ewdb_api_CreatePhenomenon failed.\n");
					ProcessorStatus = -1;
					KillSelfThread (); 
				}
logit ("", "ewdb_api_CreatePhenomenon returned %d\n", Phen.idPh);

				if (Phen.idPh < 0)
				{
					logit ("", "Call to ewdb_api_CreatePhenomenon returned %d.\n", Phen.idPh);
					ProcessorStatus = -1;
					KillSelfThread (); 
				}

				/* Now, merge this event with the newly created phenomenon */

				Merge.idMerge = -1;
				Merge.idPh = Phen.idPh;
				Merge.idEvent = EventInfo.Event.idEvent;
				strcpy (Merge.szSource, ASCIILogo);

				if (ewdb_api_CreateMerge (&Merge) != EWDB_RETURN_SUCCESS)
				{
					logit ("", "Call to ewdb_api_CreateMerge failed.\n");
					ProcessorStatus = -1;
					KillSelfThread (); 
				}
logit ("", "ewdb_api_CreateMerge returned %d\n", Merge.idMerge);

				if (Merge.idMerge < 0)
				{
					logit ("", "Call to ewdb_api_CreateMerge returned %d.\n", Merge.idMerge);
					ProcessorStatus = -1;
					KillSelfThread (); 
				}


			} /* No merge candidates found */
			else
			{
				/* merge away */
logit ("", "Merging with %d existing phenomena\n", NumMerges);

				for (i = 0; i < NumMerges; i++)
				{
					Merge.idMerge = -1;
					Merge.idPh = pCandidateList[i].idPh;
					Merge.idEvent = EventInfo.Event.idEvent;
					strcpy (Merge.szSource, ASCIILogo);
	
					if (ewdb_api_CreateMerge (&Merge) != EWDB_RETURN_SUCCESS)
					{
						logit ("", "Call to ewdb_api_CreateMerge failed.\n");
						ProcessorStatus = -1;
						KillSelfThread (); 
					}
logit ("", "ewdb_api_CreateMerge returned %d\n", Merge.idMerge);
	
					if (Merge.idMerge < 0)
					{
						logit ("", "Call to ewdb_api_CreateMerge returned %d.\n", 
													Merge.idMerge);
						ProcessorStatus = -1;
						KillSelfThread (); 
					}


					/* 	
					 * If this event has higher preference than the old
					 * preferred event, update the Phenomenon info	
					 */
logit ("", "Call EWDB_CheckMergePreference\n");
					if (EWDB_CheckMergePreference (&EventInfo, 
							pCandidateList[i].idPrefEvent, &ret) != EW_SUCCESS)
					{
						logit ("", "Call to EWDB_CheckMergePreference failed.\n");
						ProcessorStatus = -1;
						KillSelfThread (); 
					}
logit ("", "EWDB_CheckMergePreference returned %d\n", ret);


					if (ret > 0)
					{
						/* Update the preference */	
logit ("", "Call ewdb_api_UpdateMergePreference for %d, set to %d\n",
Merge.idPh, EventInfo.Event.idEvent);
						if (ewdb_api_UpdateMergePreference (Merge.idPh, 
								EventInfo.Event.idEvent) != EWDB_RETURN_SUCCESS)
						{
							logit ("", "Call to ewdb_api_UpdateMergePreference failed.\n");
							ProcessorStatus = -1;
							KillSelfThread (); 
						}
					}
				} /* for loop over retrieved merges */
			} /* found at least one merge candidate */
		} /* if doMerge is TRUE */
#endif DO_MERGE


		if (DoAlarms == TRUE)
		{
        	if (Debug)
				logit ("", "Determining Alarms\n");

			/* Retrieve the list of alarms from the DB */

			if ((pAlarms = (AlarmList *) malloc (INIT_NUM_ALARMS * 
							sizeof (AlarmList))) == NULL)
			{
				logit ("", "Could not malloc alarms buffer \n");
				ProcessorStatus = -1;
				KillSelfThread (); 
			}
	

			/* 
			 * Build the invocation string which may be included
 			 * in the alarm messages to indicate where the alarm
			 * came from
			 */

			strcpy (InvString, "Automatic hypoinverse solution inserted.");

            /* Fill in the Parameter struct to be passed into DetermineAlarms */
            DAP.isAuto = 1;
            DAP.isInsert = 1;
            DAP.minPopulation = minPopulation;
            DAP.showPopulation = showPopulation;
            DAP.networkCode = NetworkCode;
            DAP.InvocationString = InvString;

			if (DetermineAlarms (&EventInfo, &DAP, pAlarms, 
					INIT_NUM_ALARMS, &NumFound, &NumRetr, &ret) != EW_SUCCESS)
   			{
				logit ("", "Call to DetermineAlarms failed with ret=%d.\n", ret);
				ProcessorStatus = -1;
				KillSelfThread (); 
			}

			if (NumRetr < NumFound)
			{
        		if (Debug)
					logit ("", "Alarm list too small - got %d alarms, allocating.\n", NumRetr);
	
				free (pAlarms);
	
				if ((pAlarms = (AlarmList *) malloc (NumRetr * 
										sizeof (AlarmList))) == NULL)
				{
					logit ("", "Could not malloc alarms buffer \n");
					ProcessorStatus = -1;
					KillSelfThread (); 
				}
	
				if (DetermineAlarms (&EventInfo, &DAP, pAlarms, NumRetr, 
												&NumFound, &NumRetr, &ret) != EW_SUCCESS)
   				{
					logit ("", "Call to DetermineAlarms failed with %d.\n", ret);
					ProcessorStatus = -1;
					KillSelfThread (); 
				}
			}
	
			logit ("t", "Executing %d alarms.\n", NumRetr);

			/* Execute the alarms */
			if (ExecuteAlarms (pAlarms, NumRetr, InvString, RingKey, 
								InstId, MyModId) != EW_SUCCESS)
			{
				logit ("", "Call to ExecuteAlarms failed.\n");
				ProcessorStatus = -1;
				KillSelfThread (); 
			}

       		if (Debug)
          		logit ("", "Executed %d Alarms.\n", NumRetr);

			free ((void *) pAlarms);

       		if (Debug)
          		logit ("", "Freed pAlarms\n");

		} /* DoAlarms == TRUE */


        /* Free pChanInfo malloced inside ArcMsg2EWEvent */
        if(EventInfo.iNumAllocChans)
        {
          free (EventInfo.pChanInfo);
        }
        
        EventInfo.pChanInfo = NULL;
        EventInfo.iNumAllocChans = 0;
        
		logit ("t", "Done processing event %d (Logo=%s)!\n", 
								EventInfo.Event.idEvent, ASCIILogo);
        

	} /* infinite loop */

}

