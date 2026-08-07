/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: mag2ora.c,v 1.17 2004/03/18 01:47:28 patton Exp $
 *
 *    Revision history:
 *     $Log: mag2ora.c,v $
 *     Revision 1.17  2004/03/18 01:47:28  patton
 *     *** empty log message ***
 *
 *     Revision 1.16  2003/07/22 14:21:28  patton
 *     Added meaningfull heartbeats.
 *     JMP 7/22/2003.
 *
 *     Revision 1.15  2003/03/11 17:15:17  lucky
 *     *** empty log message ***
 *
 *     Revision 1.14  2002/10/08 18:15:30  lucky
 *     Added nearest town option to alarm formats
 *
 *     Revision 1.13  2002/09/10 17:15:55  lucky
 *     Stable scaffold
 *
 *     Revision 1.12  2002/06/28 21:06:22  lucky
 *     Lucky's pre-departure checkin. Most changes probably had to do with bug fixes
 *     in connection with the GIOC scaffold.
 *
 *     Revision 1.11  2002/06/12 21:39:38  lucky
 *     removed limitation on the type of magnitude. Used to be we only inserted localmags, but now
 *     we are smart enough to handle all kinds of amplitude based mags
 *
 *     Revision 1.10  2002/06/12 21:36:37  patton
 *     *** empty log message ***
 *
 *     Revision 1.9  2002/05/28 17:26:28  lucky
 *     *** empty log message ***
 *
 *     Revision 1.8  2001/07/31 20:53:00  lucky
 *      After API cleanup and bug fixes with alarms.
 *
 *     Revision 1.7  2001/07/28 00:43:53  lucky
 *      State of the code after debugging and testing prior to v6.0 release freeze.
 *
 *     Revision 1.6  2001/07/20 17:41:18  davidk
 *     changed return EW_FAILURE calls in MessageStuffer to "goto XXXAbort"
 *     calls, so that there is only one failure return, thus limiting the
 *     number of warning messages on NT about returning a value from a void
 *     function.
 *
 *     Revision 1.5  2001/07/20 17:35:16  lucky
 *     Added InvString to DetermineAlarms and ExecuteAlarms
 *
 *     Revision 1.4  2001/07/20 17:00:36  lucky
 *     *** empty log message ***
 *
 *     Revision 1.3  2001/07/05 20:43:28  lucky
 *     Use nchannels instead of nstations as the number of magnitudes
 *
 *     Revision 1.2  2001/07/01 21:55:24  davidk
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
 *     Revision 1.1  2001/06/21 21:24:11  lucky
 *     Initial revision
 *


 mag2ora:
 To pick up a message of TYPE_MAGNITUDE and to stuff it into the DB

This module picks up automatically calculated magnitudes, and stuffs
them into the DB. In general, an event can have many origins, and each
magnitude must belong to some origin. So how do we know which origin
this magnitude belongs to? The cleanest way is to somehow be told what
the origin id is. But we don't have that yet. So:  The assumption here
is that the event id (which we find in the incoming magnitude message)
was created by some automatic source. We know the author id of that
module. So, we find the latest origin for this event by this author,
and tack the magnitude to that origin. This is overkill:  eqproc
produces only one origin per event. But event if there were several,
attaching all magnitudes to the 'latest and greatest" origin seems like
a safe bet. We assume that the final magnitude for an event will be for
the final origin.

 
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <earthworm.h>
#include <kom.h>
#include <transport.h>
#include <rw_mag.h>
#include <mem_circ_queue.h>
#include <ewdb_ora_api.h>
#include <ewdb_apps_utils.h>
#include <alarms.h>

/* Defines 
*********/
#define TRUE		1
#define FALSE		0

#define MAX_STR		 255
#define MAXLINE		1000
#define MAX_NAME_LEN	  50
#define MAXCOLS		 100
#define MAXTXT           150
#define MAX_WAVESERVERS   100
#define MAX_ADRLEN        20
#define MAX_CHANNELS	  MAX_PHS_PER_EQ
#define     INIT_NUM_ALARMS     100


/* Functions in this source file 
*******************************/
void  config ( char * );
void  lookup ( void );
void  dbtsv_status ( unsigned char, short, char * );
thr_ret MessageStacker( void * );
thr_ret MessageStuffer( void * );

/* The message queue
*******************/
QUEUE MsgQueue;                 /* from queue.h, queue.c; sets up linked */

								/* Thread things
***************/
#define THREAD_STACK 8192
static unsigned tidStuffer;      /* Message processing thread id */
static unsigned tidStacker;      /* Thread moving messages from transport */
/*   to queue */
int MessageStackerStatus=0;      /* 0=> Stacker thread ok. <0 => dead */
int MessageStufferStatus=0;        /* 0=> Snipper thread ok. <0 => dead */

								   /* Transport global variables
****************************/
static  SHM_INFO  Region;      /* shared memory region to use for i/o    */

#define   MAXLOGO   10
MSG_LOGO  GetLogo[MAXLOGO];    /* array for requesting module,type,instid */
short 	  nLogo;


/* Globals to set from configuration file
****************************************/
static int     RingSize;            /* max messages in output circular buffer       */
static char    RingName[MAX_RING_STR];        /* name of transport ring for i/o      */
static char    MyModName[MAX_MOD_STR];       /* speak as this module name/id        */
static char    NetworkCode[10]; 
static int     LogSwitch;           /* 0 if no logfile should be written   */
static long    HeartBeatInt;        /* seconds between heartbeats          */
long	       TimeoutSeconds;	    /* seconds to wait for reply from ws */
int	           SetAsPreferred;	    /* Should this magnitude be set as prefered */
int	           DoAlarms;	        /* Should alarms be issued for a new mag */
char  		   DBservice[EQP_MAXWORD]; /* OCI data source to interact with  */
char           DBuser[EQP_MAXWORD];    /* UserId to connect to database as  */
char           DBpassword[EQP_MAXWORD]; /* Password to datasource            */
int            Debug;               /* if non-zero, print debug messages */
int				minPopulation;
int   			showPopulation;




/* Things to look up in the earthworm.h tables with getutil.c functions
**********************************************************************/
static long          RingKey;       /* key of transport ring for i/o      */
static unsigned char InstId;        /* local installation id              */
static unsigned char MyModId;       /* Module Id for this program         */
static unsigned char TypeHeartBeat; 
static unsigned char TypeError;
static unsigned char TypeMag;

/* Error messages used by mag2ora 
***************************************/
#define  ERR_MISSMSG       0   /* message missed in transport ring       */
#define  ERR_TOOBIG        1   /* retreived msg too large for buffer     */
#define  ERR_NOTRACK       2   /* msg retreived; tracking limit exceeded */
#define  ERR_API_INIT_FAILED 3 /* ORA_API_INIT failed */
#define  ERR_BAD_MSG        4   /* can't understand the message contents  */
#define  ERR_UNKNOWN       5   /* msg retreived; unkown error */
#define  ERR_QUEUE         6   /* error queueing message for sending     */
#define  ERR_MSG_STACKER   7   /* error with message stacker thread      */
#define  ERR_MSG_STUFFER    8   /* error with snippet maker thread        */
#define  ERR_WAVE_SERVER   9   /* error getting data from a wave_server  */
#define  ERR_SNIPPET2TRREQ 10  /* error in call to snippet2trReq         */
#define  ERR_ORA_STSNEVENT 11  /* error in call to DB StartSnippetEvent()*/
#define  ERR_WSGETTRACEBIN 12  /* error in call to wsGetTraceBin()       */
#define  ERR_STUFF_SNPT    13  /* can't stuff snippet into dbms          */
static char Text[MAXTXT];      /* string for log/error messages          */

/* Debug debug DEBUG */
int Debug_m2o=0;	/* setting this to one causes all sorsts of log output */

pid_t MyPid;        /* Our own pid, sent with heartbeat for restart 
					purposes */

int	ValidateQdds;


int	numStacked = 0;

main( int argc, char **argv )
{
	long    timeNow;          		/* current time                   */       
	long    timeLastBeat;     		/* time last heartbeat was sent   */
	char    LogFileName[128];
	
	/* Check command line arguments 
	******************************/
	if ( argc != 2 )
	{
		fprintf( stderr, "Usage: mag2ora <configfile>\n" );
		exit( 0 );
	}

	/* Initialize name of log-file & open it 
	***************************************/
	fprintf(stderr,"mag2ora:  Initializing logit.\n");
	/* Created LogFileName, to be obtained from config file name
	instead of using the fixed "mag2ora" for logname. */
	strcpy(LogFileName,argv[1]);
	LogFileName[strlen(argv[1])-2/*.d*/]=0;
	
	logit_init( LogFileName, 0, 64000, 1 );
	
	/* Read the configuration file(s)
	********************************/
	fprintf(stderr,"mag2ora:  Reading Config file.\n");
	config( argv[1] );
	logit( "" , "mag2ora: Read command file <%s>\n", argv[1] );	
	
	/* Get our own Pid for restart purposes
	***************************************/
	MyPid = getpid();
	if(MyPid == -1)
	{
		fprintf(stderr,"mag2ora: Cannot get pid. Exitting.\n");
		exit(0);
	}
	
	
	/* Look up unimportant info from earthworm.h tables
	***************************************************/
	fprintf(stderr,"mag2ora:  Performing Earthworm config lookup.\n");
	lookup();
	
	/* Reinitialize logit to desired logging level
	**********************************************/
	logit_init( LogFileName, 0, 1024, LogSwitch );
	
	/* Attach to Input/Output shared memory ring 
	*******************************************/
	tport_attach( &Region, RingKey );
	
	/* Indicate whether the debugger flag is turned on. */
	logit("","Debug_m2o=%d\n",Debug_m2o);
	
	/* If debug mode, log our logo */
	if(Debug_m2o)
	{
		logit("", "mag2ora: Attached to public memory region %s: %d\n", 
			RingName, RingKey );
		logit("","logo:%d %u %u %u\n", nLogo,
			GetLogo[0].type,GetLogo[0].mod,GetLogo[0].instid);
	}
	
	/* Initialize the disposal system
	********************************/
	if( ewdb_api_Init (DBuser,DBpassword,DBservice ) != EWDB_RETURN_SUCCESS )
	{
		sprintf(Text, "OraAPIInit() failure; exitting!\n" );
		dbtsv_status( TypeError, ERR_API_INIT_FAILED, Text ); 
		exit( -1 );
	}
	
	/* Force a heartbeat to be issued in first pass thru main loop
	*************************************************************/
	timeLastBeat = time(&timeNow) - HeartBeatInt - 1;
	
	/* Create a Mutex to control access to queue
	********************************************/
	CreateMutex_ew();  /* a single unnamed mutex? */
	
	/* Initialize the message queue
	*******************************/
	RingSize = 100;  /* that is, we can stack this many magnitude messages */
	
	if(initqueue(&MsgQueue,(unsigned long)RingSize,
		(unsigned long)DB_MAX_BYTES_PER_EQ))
	{
		logit( "", "Initqueue failure; exitting!\n" );
		sprintf(Text, "Initqueue failure; exitting!\n" );
		dbtsv_status( TypeError, ERR_QUEUE, Text ); 
		exit( -1 );
	}
	
	/* Start the  message stacking thread
	*************************************/
	if ( StartThread(MessageStacker, (unsigned)THREAD_STACK, &tidStacker ) == -1)
	{
		logit( "e", "mag2ora: Error starting  MessageStacker thread. "
			"Exitting.\n" );
		sprintf(Text, "mag2ora: Error starting  MessageStacker thread. "
			"Exitting.\n" );
		dbtsv_status( TypeError, ERR_MSG_STACKER, Text ); 
		tport_detach( &Region );
		exit (-1);
	}
	
	MessageStackerStatus=0; /*assume the best*/
	
	/* Start the  message processing thread
	***************************************/
	if ( StartThread(  MessageStuffer, (unsigned)THREAD_STACK, &tidStuffer ) == -1 )
	{
		logit( "e", "mag2ora: Error starting  MessageStacker thread. "
			"Exitting.\n" );
		sprintf(Text, "mag2ora: Error starting  MessageStacker thread. "
			"Exitting.\n" );
		dbtsv_status( TypeError, ERR_MSG_STUFFER, Text ); 
		tport_detach( &Region );
		exit (-1);
	}
	MessageStufferStatus=0; /*assume the best*/
	
	/* Setup done; start main loop: 
	******************************/
	/* Having delegated message collecting, and message processing, there's
	not much for us left to do: watch thread status, and beat the heart */
	
	while( tport_getflag(&Region) != TERMINATE )  	
		/* begin loop till Earthworm shutdown (level 1) */
	{
		/* send mag2ora's heartbeat
		**********************************/
		if  ( time(&timeNow) - timeLastBeat  >=  HeartBeatInt ) 
		{
			timeLastBeat = timeNow;
			dbtsv_status( TypeHeartBeat, 0, "" ); 
		}
		
		/* see how our threads are feeling
		**********************************/
		if ( MessageStufferStatus < 0)
		{
			logit("et","ERROR Snippet making thread died. Exitting\n");
			sprintf(Text,"ERROR Snippet making thread died. Exitting\n");
			dbtsv_status( TypeError, ERR_MSG_STUFFER, Text ); 
			exit (-1);
		}
		if ( MessageStackerStatus < 0)
		{
			logit("et","ERROR Message stacking thread died. Exitting\n");
			sprintf(Text,"ERROR Message stacking thread died. Exitting\n");
			dbtsv_status( TypeError, ERR_MSG_STACKER, Text ); 
			exit (-1);
		}
		
		/* set thread flags to -1 to see if
		 * they're still alive next loop
		 **********************************/
		MessageStackerStatus = -1;
		MessageStufferStatus = -1;

		sleep_ew( 3000 );  	
	}					/* end of while!(shutdown requested)  (level 1) */  
	
	/* Termination has been requested 
	********************************/
	tport_detach( &Region ); /* detach from shared memory */
	logit( "t", "mag2ora: Termination requested; exitting!\n" );
	return(0);
}
/*--------------------------------- end of main() ---------------------------------------*/


/***********************************************************************
*  config() processes command file(s) using kom.c functions;    *
*                  exits if any errors are encountered.	       *
***********************************************************************/
void config( char *configfile )
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
	ncommand = 9;
	for( i=0; i<ncommand; i++ )  init[i] = 0;
	nLogo = 0;
    SetAsPreferred = FALSE;
    DoAlarms = FALSE;
    strcpy (NetworkCode, "  ");

	
	/* Open the main configuration file 
	**********************************/
	nfiles = k_open( configfile ); 
	if ( nfiles == 0 ) {
		logit( "e",
			"mag2ora: Error opening command file <%s>; exitting!\n", 
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
					logit( "e", 
						"mag2ora: Error opening command file <%s>; exitting!\n",
						&com[1] );
					exit( -1 );
				}
				continue;
            }
			
			/* Process anything else as a command 
			************************************/
/*0*/		if( k_its("LogFile") ) 
			{
				LogSwitch = k_int();
				init[0] = 1;
            }
/*1*/		else if( k_its("MyModuleId") ) 
			{
				str = k_str();
				if(str) strcpy( MyModName, str );
				init[1] = 1;
            }
/*2*/		else if( k_its("RingName") ) 
			{
				str = k_str();
				if(str) strcpy( RingName, str );
				init[2] = 1;
            }
/*3*/		else if( k_its("HeartBeatInt") ) 
			{
				HeartBeatInt = k_long();
				init[3] = 1;
            }
			
			/* Enter installation & module to get event messages from
			********************************************************/
/*4*/		else if( k_its("GetEventsFrom") ) {
				if ( nLogo >= MAXLOGO ) {
					logit( "e", "mag2ora: Too many <GetEventsFrom> commands in <%s>", 
						configfile );
					logit( "e", "; max=%d; exitting!\n", (int) MAXLOGO );
					exit( -1 );
				}
				if( ( str=k_str() ) ) {
					if( GetInst( str, &GetLogo[nLogo].instid ) != 0 ) {
						logit( "e", "mag2ora: Invalid installation name <%s>", str ); 
						logit( "e", " in <GetEventsFrom> cmd; exitting!\n" );
						exit( -1 );
					}
					GetLogo[nLogo+1].instid = GetLogo[nLogo].instid;
				}
				if( ( str=k_str() ) ) {
					if( GetModId( str, &GetLogo[nLogo].mod ) != 0 ) {
						logit( "e", "mag2ora: Invalid module name <%s>", str ); 
						logit( "e", " in <GetEventsFrom> cmd; exitting!\n" );
						exit( -1 );
					}
					GetLogo[nLogo+1].mod = GetLogo[nLogo].mod;
				}
				if( GetType( "TYPE_MAGNITUDE", &GetLogo[nLogo].type ) != 0 ) {
					logit( "e", "mag2ora: Invalid message type <TYPE_MAGNITUDE>" ); 
					logit( "e", "; exitting!\n" );
					exit( -1 );
				}
				nLogo++;
				init[4] = 1;
			}
/*5*/		else if( k_its("DBservice") ) {
				str = k_str();
				if(str) strcpy( DBservice, str );
				init[5] = 1;
            }
/*6*/		else if( k_its("DBpassword") ) {
				str = k_str();
				if(str) strcpy( DBpassword, str );
				init[6] = 1;
            }
/*7*/		else if( k_its("DBuser") ) {
				str = k_str();
				if(str) strcpy( DBuser, str );
				init[7] = 1;
            }
/*8*/		else if( k_its("TimeoutSeconds") ) {
				TimeoutSeconds = k_int(); 
				init[8] = 1;
            }
		    else if( k_its("SetAsPreferred") ) {
				SetAsPreferred = TRUE;
            }
            else if( k_its("DoAlarms") ) {  /*optional command*/
                DoAlarms=TRUE;
            }
            else if( k_its("minPopulation") ) {  /*optional command*/
                minPopulation = k_int();
            }
            else if( k_its("showPopulation") ) {  /*optional command*/
                showPopulation = k_int();
            }
            else if( k_its("NetworkCode") ) { /* optional command */
                str = k_str();
                if(str) strcpy( NetworkCode, str );
            }
            else if( k_its("Debug") ) {  /*optional command*/
                Debug_m2o=1;
            }
			
			/* Unknown command
			*****************/ 
			else {
                logit( "e", "mag2ora: <%s> Unknown command in <%s>.\n", 
					com, configfile );
                continue;
            }
			
			/* See if there were any errors processing the command 
			*****************************************************/
            if( k_err() ) {
				logit( "e", 
					"mag2ora: Bad <%s> command in <%s>; exitting!\n",
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
		logit( "e", "mag2ora: ERROR, no " );
		if ( !init[0] )  logit( "e", "<LogFile> "       );
		if ( !init[1] )  logit( "e", "<MyModuleId> "    );
		if ( !init[2] )  logit( "e", "<RingName> "      );
		if ( !init[3] )  logit( "e", "<HeartBeatInt> "  );
		if ( !init[4] )  logit( "e", "<GetEventsFrom> " );
		if ( !init[5] )  logit( "e", "<DBservice> "     );
		if ( !init[6] )  logit( "e", "<DBpassword> "    );
		if ( !init[7] )  logit( "e", "<DBuser> "        );
		if ( !init[8] )  logit( "e", "<TimeoutSeconds> "     );
		logit( "e", "command(s) in <%s>; exitting!\n", configfile );
		exit( -1 );
	}
   
   return;
}

/************************************************************************
*  lookup( )   Look up important info from earthworm.h tables   *
************************************************************************/
void lookup( void )
{
/* Look up keys to shared memory regions
	*************************************/
	if( ( RingKey = GetKey(RingName) ) == -1 ) {
		fprintf( stderr,
			"mag2ora:  Invalid ring name <%s>; exitting!\n", RingName);
		exit( -1 );
	}
	
	/* Look up installations of interest
	*********************************/
	if ( GetLocalInst( &InstId ) != 0 ) {
		fprintf( stderr, 
			"mag2ora: error getting local installation id; exitting!\n" );
		exit( -1 );
	}
	
	/* Look up modules of interest
	***************************/
	if ( GetModId( MyModName, &MyModId ) != 0 ) {
		fprintf( stderr, 
			"mag2ora: Invalid module name <%s>; exitting!\n", MyModName );
		exit( -1 );
	}
	
	/* Look up message types of interest
	*********************************/
	if ( GetType( "TYPE_HEARTBEAT", &TypeHeartBeat ) != 0 ) {
		fprintf( stderr, 
			"mag2ora: Invalid message type <TYPE_HEARTBEAT>; exitting!\n" );
		exit( -1 );
	}
	if ( GetType( "TYPE_ERROR", &TypeError ) != 0 ) {
		fprintf( stderr, 
			"mag2ora: Invalid message type <TYPE_ERROR>; exitting!\n" );
		exit( -1 );
	}
	if ( GetType( "TYPE_MAGNITUDE", &TypeMag ) != 0 ) {
		fprintf( stderr, 
			"mag2ora: Invalid message type <TYPE_MAGNITUDE>; exitting!\n" );
		exit( -1 );
	}
	return;
} 

/*************************************************************************
* dbtsv_status() builds a heartbeat or error message & puts it into    *
*                 shared memory.  Writes errors to log file & screen.   *
*************************************************************************/
void dbtsv_status( unsigned char type, short ierr, char *note )
{
	MSG_LOGO    logo;
	char	       msg[256];
	long	       size;
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
		logit( "et", "mag2ora: %s\n", note );
	}
	
	size = strlen( msg );   /* don't include the null byte in the message */ 	
	
							/* Write the message to shared memory
	************************************/
	if( tport_putmsg( &Region, &logo, size, msg ) != PUT_OK )
	{
        if( type == TypeHeartBeat ) {
			logit("et","mag2ora:  Error sending heartbeat.\n" );
		}
		else if( type == TypeError ) {
			logit("et","mag2ora:  Error sending error:%d.\n", ierr );
		}
	}
	
	return;
}






/********************** Message Stacking Thread *******************
*           Move messages from transport to memory queue         *
******************************************************************/
thr_ret MessageStacker( void *dummy )
{
	int          res;
	long         recsize;        /* size of retrieved message             */
	MSG_LOGO     reclogo;        /* logo of retrieved message             */
	int          ret;
	char	     msg[DB_MAX_BYTES_PER_EQ];
	
	/* Tell the main thread we're ok
	********************************/
	MessageStackerStatus=0;
	
	/* Flush the ring, something a married man should never get	caught saying.  
	Is this the sort of thing a newlywed male would write. Eh Dave? */
	while ( tport_getmsg( &Region, GetLogo, (short)nLogo, &reclogo,
		&recsize, msg, DB_MAX_BYTES_PER_EQ-1 ) != GET_NONE );
	
	
	if(Debug_m2o==1) 
		logit("e",
		"mag2ora:MessageStacker:starting to watch for "
		"trigger messages\n");
	
	/* Start service loop, picking up trigger messages
	**************************************************/
	while( 1 )
	{
		/* Tell the main thread we're ok
		********************************/
		MessageStackerStatus=0;

		/* Get a message from transport ring
		************************************/
		res = tport_getmsg( &Region, GetLogo, nLogo, &reclogo, &recsize, 
			msg, DB_MAX_BYTES_PER_EQ-1 );
		if(Debug_m2o==1  && res==GET_OK) 
			logit("et","Got message from transport of %d bytes,res=%d\n",recsize,res); 
		
		if( res == GET_NONE ) 
		{
			/*wait if no messages for us */
			sleep_ew(100); 
			continue;
		}
		
		/* Check return code; report errors
		***********************************/
		if( res != GET_OK )
		{
			if( res==GET_TOOBIG )
			{
				sprintf(Text, "msg[%ld] i%d m%d t%d too long for target",
					recsize, (int) reclogo.instid,
					(int) reclogo.mod, (int)reclogo.type );
				dbtsv_status( TypeError, ERR_TOOBIG, Text );
				continue;
			}
			else if( res==GET_MISS )
			{
				sprintf(Text, "missed msg(s) i%d m%d t%d in %s",(int) reclogo.instid,
					(int) reclogo.mod, (int)reclogo.type, RingName );
				dbtsv_status( TypeError, ERR_MISSMSG, Text );
			}
			else if( res==GET_NOTRACK )
			{
				sprintf(Text, "no tracking for logo i%d m%d t%d in %s",
					(int) reclogo.instid, (int) reclogo.mod, (int)reclogo.type,
					RingName );
				dbtsv_status( TypeError, ERR_NOTRACK, Text );
			}
			else
			{
				logit("et","ERROR:ERROR:ERROR: MsgStacker():"
					"Unrecognized return code from tport_getmsg() %d.\n",res);
				sprintf(Text,"ERROR:ERROR:ERROR: MsgStacker():"
					"Unrecognized return code from tport_getmsg() %d.\n",res);
				dbtsv_status( TypeError, ERR_UNKNOWN, Text );
			}
		}  /* tport_getmsg() returned !GET_OK */
		
		/* Queue retrieved msg (res==GET_OK,GET_MISS,GET_NOTRACK)
		*********************************************************/
		RequestMutex();
		ret=enqueue( &MsgQueue, msg, recsize, reclogo ); /* put it into the queue */
		/* the MessageStuffer thread is in the biz of de-queueng and processing */
		ReleaseMutex_ew();
logit ("t", "Stacked message %d of size %d\n", numStacked, recsize);
logit ("", "Msg: <%s>\n", msg);
numStacked = numStacked + 1;

		if(Debug_m2o==1)
			logit("","Queued a message\n");
		if ( ret!= 0 )
		{
			if (ret==-2)  /* Serious: quit */
			{
				sprintf(Text,"internal queue error. Terminating.");
				dbtsv_status( TypeError, ERR_QUEUE, Text );
				goto error;
			}
			if (ret==-1)
			{
				sprintf(Text,"queue cannot allocate memory. Lost message.");
				dbtsv_status( TypeError, ERR_QUEUE, Text );
				continue;
			}
			if (ret==-3) 
			{
				sprintf(Text,"Queue full. Message lost.");
				dbtsv_status( TypeError, ERR_QUEUE, Text );
				continue;
			}
		}  /* if enqueue() failed */
		if(Debug_m2o==1) 
			logit("et", "stacker thread: queued msg len:%ld\n",recsize);
	} /* while(1) */
	
	/* we're quitting
	*****************/
error:
    MessageStackerStatus=-1; /* file a complaint to the main thread */
    KillSelfThread(); /* we'll count on the restart mechanism */
}

/******************** Message Processing Thread *********************
*           Take mag messages from memory queue, and put them		*
*			into the DB. They consist of a summary part, and a		*
*			per-channel part.										*
*********************************************************************/
thr_ret MessageStuffer( void *dummy )
{
	char 						magMsg[DB_MAX_BYTES_PER_EQ]; 
	int							i, ret, DBFlags;
	long						msgSize, magChanSize;
	MSG_LOGO					reclogo; 
	MAG_INFO					magSum;			
	MAG_CHAN_INFO				*magChan;
	EWEventInfoStruct			EventInfo;
	EWDB_StationMagStruct		stamag;
	EWDB_External_StationStruct	station;	/* all idChanT's for this SCN */
	EWDBid						idMag;
	static char  				InvString[128]; 
	AlarmList      				*pAlarms;
	int  						NumFound, NumRetr;
	DetAlmsParams				DAP;

	
	
   	/* Tell the main thread we're ok
   	********************************/
   	MessageStufferStatus=0;

	magChan = (MAG_CHAN_INFO *) NULL;
	pAlarms = (AlarmList *) NULL;
	
   	/* Get message from queue
   	*************************/
topOfLoop:

   	/* Tell the main thread we're ok
   	********************************/
   	MessageStufferStatus=0;

	/* Free up thingies so that our memory usage does not grow */
	if (magChan != NULL)
	{
		free (magChan);
		magChan = (MAG_CHAN_INFO *) NULL;
	}

	if (pAlarms != NULL)
	{
		free (pAlarms);
		pAlarms = (AlarmList *) NULL;
	}


   	RequestMutex();
   	ret=dequeue( &MsgQueue, magMsg, &msgSize, &reclogo);
   	ReleaseMutex_ew();
   	if(ret < 0 )
    { /* -1 means empty queue */
		sleep_ew(500); /* wait a bit */
		goto topOfLoop;
    }
   	if( reclogo.type != TypeMag ) /*TYPE_MAGNITUDE*/
	{
		logit("et","ERROR: illegal message in queue\n");
		MessageStufferStatus = -1;
		KillSelfThread();
	}
   	magMsg[msgSize] = '\0';   /*null terminate the message*/

   	if(Debug_m2o==1)
		logit ("e", "Processing a message\n");  

logit ("t", "Dequeued message %d of size %d\n", numStacked, msgSize);
numStacked = numStacked - 1;				

	/* parse the magSum part of the message 
	***************************************/
	if (rd_mag (magMsg, msgSize, &magSum) < 0)
	{
		logit("e", "Call to rd_mag failed.\n"); 
		goto topOfLoop;
	}

	memset( (void*)&EventInfo,0,sizeof(EWEventInfoStruct));


	/* 
	 * Extract author: story - we need the idOrigin in order
	 * to insert this magnitude.  We get it by combining the
   	 * event ID, origin version, and source, which was (hopefully)
	 * previously inserted into the DB.
	 *
	 * Note on the source:
	 * The magnitude message contains the logo as the author string 
	 * of the form xxxxxxxxx:xxxxxxxxx - we want the first part
	 * of that. THe message also tells us the external event id.
	 */
	if (magSum.qauthor[9] == ':')
	{
		strncpy (EventInfo.PrefOrigin.sSource, magSum.qauthor, 9); 
		EventInfo.PrefOrigin.sSource[9] = '\0';
	}
	else
	{
		logit ("e", "Author string not a logo; will use it anyway.\n");
		strcpy (EventInfo.PrefOrigin.sSource, magSum.qauthor); 
	}
	strcpy (EventInfo.PrefOrigin.szSourceEventID, magSum.qid); /* event id string */
	EventInfo.PrefOrigin.iVersionNum = magSum.origin_version;

    /*
     * We have to sleep for a bit just to make sure that the origin that woke us up
     * makes it into the DB.
     */

	/* Tell the main thread we're ok
	 ********************************/
	MessageStufferStatus=0;


    sleep_ew (1000);

    /* Tell the main thread we're ok
	 ********************************/
	MessageStufferStatus=0;


   	if(Debug_m2o==1)
		logit ("", "Calling ewdb_api_GetidOriginFromVersionNum with source=%s, ID=%s, version=%d\n",
						EventInfo.PrefOrigin.sSource, 
						EventInfo.PrefOrigin.szSourceEventID, 
						EventInfo.PrefOrigin.iVersionNum);

	if (ewdb_api_GetidOriginFromVersionNum (&EventInfo.PrefOrigin) == EWDB_RETURN_FAILURE)
	{
		logit ("", "Call to ewdb_api_GetidOriginFromVersionNum returned error\n" );
		goto topOfLoop;
	}

	logit ("t", "Inserting mag %s for origin %d, event %d.\n",  magSum.szmagtype,
						EventInfo.PrefOrigin.idOrigin, EventInfo.PrefOrigin.idEvent);


	EventInfo.Event.idEvent = EventInfo.PrefOrigin.idEvent;


	/* Retrieve all the info for this event -- we'll need it for alarms later
	**************************************************************************/
    DBFlags = GETEWEVENTINFO_SUMMARYINFO | GETEWEVENTINFO_PICKS
                   | GETEWEVENTINFO_STAMAGS; 

	if (ewdb_apps_GetDBEventInfoII (&EventInfo, EventInfo.Event.idEvent, 
					EventInfo.PrefOrigin.idOrigin, DBFlags) != EWDB_RETURN_SUCCESS)
	{
		logit ("", "Call to ewdb_apps_GetDBEventInfoII failed.\n");
		goto topOfLoop;
	}
   	if(Debug_m2o==1)
		logit ("", "GetEvent returned idOrigin=%d\n", EventInfo.PrefOrigin.idOrigin);


	if (EventInfo.iNumMags >= MAX_MAGS_PER_ORIGIN)
	{
		logit ("", "Max number of magnitudes (%d) reached. Skip.\n",
									MAX_MAGS_PER_ORIGIN);
		goto topOfLoop;
	}

	memset (&EventInfo.Mags[EventInfo.iNumMags], 0, sizeof(EWDB_MagStruct));

	if (SetAsPreferred == TRUE)
		EventInfo.iPrefMag = EventInfo.iNumMags;

	EventInfo.iMd = -1;
	EventInfo.iML = -1;

	if (magSum.imagtype == MAGTYPE_DURATION)
		EventInfo.iMd = EventInfo.iNumMags;
	else if ((magSum.imagtype == MAGTYPE_LOCAL_PEAK2PEAK) || 
			 	(magSum.imagtype == MAGTYPE_LOCAL_ZERO2PEAK))
		EventInfo.iML = EventInfo.iNumMags;

	/* Prepare magnitude for insertion
	*************************************************************/

	EventInfo.Mags[EventInfo.iNumMags].iMagType = magSum.imagtype; 
	EventInfo.Mags[EventInfo.iNumMags].dMagAvg = (float)magSum.mag; 


	if (magSum.error >= 0.0)
		EventInfo.Mags[EventInfo.iNumMags].dMagErr = (float)magSum.error;
	else
		EventInfo.Mags[EventInfo.iNumMags].dMagErr = -1.0;

	EventInfo.Mags[EventInfo.iNumMags].iNumMags = magSum.nchannels; 
	EventInfo.Mags[EventInfo.iNumMags].idEvent = EventInfo.Event.idEvent; 
	EventInfo.Mags[EventInfo.iNumMags].bBindToEvent = TRUE; 
 	EventInfo.Mags[EventInfo.iNumMags].bSetPreferred = SetAsPreferred;
	EventInfo.Mags[EventInfo.iNumMags].idOrigin = EventInfo.PrefOrigin.idOrigin;

	/* get the second logo from the author string as external source */
	strncpy(EventInfo.Mags[EventInfo.iNumMags].szSource, &(magSum.qauthor[10]),9);
	EventInfo.Mags[EventInfo.iNumMags].szSource[9]='\0';

	/* call the api routine to do it */

	if (ewdb_api_CreateMagnitude (&EventInfo.Mags[EventInfo.iNumMags]) == EWDB_RETURN_FAILURE)
	{
		logit ("", "ewdb_api_CreateMagnitude returned error\n" );
		goto topOfLoop;
	} 

	idMag = EventInfo.Mags[EventInfo.iNumMags].idMag;
	EventInfo.iNumMags = EventInfo.iNumMags + 1;



	/* Put away the station-specific stuff
	**************************************/
	
	/* retrieve station specific stuff */
	magChanSize = MAX_CHANNELS * sizeof (MAG_CHAN_INFO);
	if ((magChan = (MAG_CHAN_INFO *) malloc (magChanSize)) == NULL)
	{
		logit ("e", "Could not malloc %d MAG_CHAN_INFO structs.\n", MAX_CHANNELS);
		goto topOfLoop;
	}
		
	if (rd_chan_mag (magMsg, msgSize, magChan, magChanSize) < 0)
	{
		logit ("e", "Call to rd_chan_mag failed.\n");
		goto topOfLoop;
	}

	
	/* loop over retrieved channels 
	*****************************/
	for (i = 0; (i < magSum.nchannels && i < MAX_CHANNELS); i++)
	{
	    /* Tell the main thread we're ok
		 ********************************/
		MessageStufferStatus=0;

		/* load the EWDB_StationStruct */
		memset(&station, 0, sizeof(station) );
		strcpy( station.Station.Sta, magChan[i].sta);
		strcpy( station.Station.Comp, magChan[i].comp);
		strcpy( station.Station.Net, magChan[i].net);
		strcpy( station.Station.Loc, "");
			
   		if(Debug_m2o==1)
			logit ("", "Processing %s.%s.%s\n", 
					station.Station.Sta, station.Station.Comp, station.Station.Net);

		/* get the idChanT */
		if (ewdb_apps_RetrieveIdChanExternal (&station) == EWDB_RETURN_FAILURE )
		{
			logit ("", "ewdb_apps_RetrieveIdChanExternal returned error\n" );
			goto topOfLoop;
		} 

   		if(Debug_m2o==1)
			logit ("", "Got idChan = %d\n", station.Station.idChan);


		/* load the EWDB_PhaseAmpStruct 
		*******************************/
		memset(&stamag, 0, sizeof(EWDB_StationMagStruct) );
		stamag.idChan = station.Station.idChan;
		stamag.dMag = (float)(magChan[i].mag);	/* set mag value */
		stamag.idMagnitude = idMag; 			/*DB id of this mag */
		stamag.iMagType = magSum.imagtype;

		/* 
		 * Insert the peak values:
		 * "localmag" can be configured to use either a zero-to-peak or a 
		 * peak-to-peak algorithm. If we find the p2p values are set, use them.
		 * Otherwise, use the z2p values - they're always there.
		 */

		stamag.StaMagUnion.PeakAmp.dAmp1 = (float)(magChan[i].Amp1); 
		stamag.StaMagUnion.PeakAmp.tAmp1 = (double) magChan[i].Time1;
		stamag.StaMagUnion.PeakAmp.dAmpPeriod1 = (float)(magChan[i].Period1);

		stamag.StaMagUnion.PeakAmp.dAmp2 = (float)(magChan[i].Amp2); 
		stamag.StaMagUnion.PeakAmp.tAmp2 = (double) magChan[i].Time2;
		stamag.StaMagUnion.PeakAmp.dAmpPeriod2 = (float)(magChan[i].Period2);

		/* Insert station-specific stuff into DBMS
		******************************************/

   		if(Debug_m2o==1)
			logit ("", "Calling InsertPeakAmpWithMag with magtype=%d \n", 
									stamag.iMagType);

		if (ewdb_apps_InsertPeakAmpWithMag( &stamag ) == EWDB_RETURN_FAILURE )
		{
			logit ("", "ewdb_apps_InsertPeakAmpWithMag returned error\n" );
			continue;
		} 

   		if(Debug_m2o==1)
			logit ("", "InsertPeakAmpWithMag returned "
						"idMagLink=%d, idPeakAmp=%d\n", stamag.idMagLink, 
							stamag.StaMagUnion.PeakAmp.idPeakAmp); 
	}



 	if(Debug_m2o==1)
		logit ("e", "Magnitude message inserted.\n");

	if (DoAlarms == TRUE)
	{
		if (Debug_m2o)
			logit ("", "Executing Alarms\n");

		/* Retrieve the list of alarms from the DB */

		if ((pAlarms = (AlarmList *) malloc (INIT_NUM_ALARMS *
									sizeof (AlarmList))) == NULL)
		{
			logit ("", "Could not malloc alarms buffer \n");
			goto topOfLoop;
		}


		/* 
		 * Build the invocation string which may be included
 		 * in the alarm messages to indicate where the alarm
		 * came from
		 */

		sprintf (InvString, "Automatic magnitude (%s) inserted.", magSum.szmagtype);

		/* Fill in the Parameter struct to be passed into DetermineAlarms */
		DAP.isAuto = 1;
		DAP.isInsert = 1;
		DAP.minPopulation = minPopulation;
		DAP.showPopulation = showPopulation;
		DAP.networkCode = NetworkCode;
		DAP.InvocationString = InvString;

logit ("t", "Calling DetermineAlarms for event %d\n", EventInfo.Event.idEvent);



		if (DetermineAlarms (&EventInfo, &DAP, pAlarms, 
				INIT_NUM_ALARMS, &NumFound, &NumRetr, &ret) != EW_SUCCESS)
   		{
			logit ("", "Call to DetermineAlarms failed with ret=%d.\n", ret);
			goto topOfLoop;
		}

		if (NumRetr < NumFound)
		{

       		if (Debug_m2o)
				logit ("", "Alarm list too small - got %d alarms, allocating.\n", NumRetr);

			free (pAlarms);

			if ((pAlarms = (AlarmList *) malloc (NumRetr * 
									sizeof (AlarmList))) == NULL)
			{
				logit ("", "Could not malloc alarms buffer \n");
				goto topOfLoop;
			}
	
			if (DetermineAlarms (&EventInfo, &DAP, pAlarms, NumRetr, 
									&NumFound, &NumRetr, &ret) != EW_SUCCESS)
   			{
				logit ("", "Call to DetermineAlarms failed with ret=%d.\n", ret);
				goto topOfLoop;
			}
		}


		logit ("t", "Executing %d alarms for event %d.\n", NumRetr, EventInfo.Event.idEvent);

		/* Execute the alarms */
		if (ExecuteAlarms (pAlarms, NumRetr, InvString, RingKey, 
							InstId, MyModId) != EW_SUCCESS)
		{
			logit ("", "Call to ExecuteAlarms failed.\n");
			goto topOfLoop;
		}

		if (Debug_m2o)
			logit ("", "Executed %d Alarms.\n", NumRetr);

	} /* DoAlarms == TRUE */

	logit ("t", "Done processing local magnitude for event %d!\n", 
											EventInfo.Event.idEvent);
	
	goto topOfLoop;	/* do the next message */

}  /* end MessageStuffer() */

