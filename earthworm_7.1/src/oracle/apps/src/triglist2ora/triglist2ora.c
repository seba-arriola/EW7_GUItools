/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: triglist2ora.c,v 1.2 2002/06/07 16:08:16 patton Exp $
 *
 *    Revision history:
 *     $Log: triglist2ora.c,v $
 *     Revision 1.2  2002/06/07 16:08:16  patton
 *     Made logit changes.
 *
 *     Revision 1.1  2002/03/22 20:12:51  lucky
 *     Initial revision
 *
 *
 */



/*
 mag2ora:
 Pick up a message of TYPE_TRIGLIST and to stuff it into the DB
 
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <earthworm.h>
#include <kom.h>
#include <transport.h>
#include <mem_circ_queue.h>
#include <ewdb_ora_api.h>
#include <ewdb_apps_utils.h>

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

WS_MENU_QUEUE_REC MenuList;     /* the list of menues; the menues will be malloced! */

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
static int     LogSwitch;           /* 0 if no logfile should be written   */
static long    HeartBeatInt;        /* seconds between heartbeats          */
char  		   DBservice[32]; /* OCI data source to interact with  */
char           DBuser[32];    /* UserId to connect to database as  */
char           DBpassword[32]; /* Password to datasource            */
int            Debug;               /* if non-zero, print debug messages */
long           WS_TimeoutSeconds;      /* seconds to wait for reply from ws */
int            nServer;         /* number of wave servers we know about */
char           wsIp[MAX_WAVESERVERS][MAX_ADRLEN];
char           wsPort[MAX_WAVESERVERS][MAX_ADRLEN];
long           TraceBufferLen;    /* bytes of largest snippet */



/* Things to look up in the earthworm.h tables with getutil.c functions
**********************************************************************/
static long          RingKey;       /* key of transport ring for i/o      */
static unsigned char InstId;        /* local installation id              */
static unsigned char MyModId;       /* Module Id for this program         */
static unsigned char TypeHeartBeat; 
static unsigned char TypeError;
static unsigned char TypeTriglist;

/* Error messages used by triglist2ora 
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
int Debug_t2o=0;	/* setting this to one causes all sorsts of log output */

pid_t MyPid;        /* Our own pid, sent with heartbeat for restart purposes */

char         *TraceBuffer;  /* where we store the trace snippet  */
TRACE_REQ    *pTraceReq;    /* request forms for wave server.  */


main( int argc, char **argv )
{
	long    timeNow;          		/* current time                   */       
	long    timeLastBeat;     		/* time last heartbeat was sent   */
	char    LogFileName[128];
	int		i;
	
	/* Check command line arguments 
	******************************/
	if ( argc != 2 )
	{
		fprintf( stderr, "Usage: triglist2ora <configfile>\n" );
		exit( 0 );
	}

	
	/* Zero the wave server arrays
	*****************************/
	for (i=0; i< MAX_WAVESERVERS; i++)
	{
		memset( wsIp[i], 0, MAX_ADRLEN);
		memset( wsPort[i], 0, MAX_ADRLEN);
	}
	MenuList.head=NULL; /* As per Carol Bryant. */
	MenuList.tail=NULL;

	/* Initialize name of log-file & open it 
	***************************************/
	fprintf(stderr,"triglist2ora:  Initializing logit.\n");
	strcpy (LogFileName,argv[1]);
	LogFileName[strlen(argv[1])-2]=0;
	
	logit_init( LogFileName, 0, 1024, 1 );
	
	/* Read the configuration file(s)
	********************************/
	fprintf(stderr,"triglist2ora:  Reading Config file.\n");
	config( argv[1] );
	logit( "" , "triglist2ora: Read command file <%s>\n", argv[1] );	
	
	/* Get our own Pid for restart purposes
	***************************************/
	MyPid = getpid();
	if(MyPid == -1)
	{
		fprintf(stderr,"triglist2ora: Cannot get pid. Exitting.\n");
		exit(0);
	}
	
	
	/* Look up unimportant info from earthworm.h tables
	***************************************************/
	fprintf(stderr,"triglist2ora:  Performing Earthworm config lookup.\n");
	lookup();
	
	/* Reinitialize logit to desired logging level
	**********************************************/
	logit_init( LogFileName, 0, 1024, LogSwitch );
	
	/* Attach to Input/Output shared memory ring 
	*******************************************/
	tport_attach( &Region, RingKey );
	
	/* Indicate whether the debugger flag is turned on. */
	logit("","Debug_t2o=%d\n",Debug_t2o);
	
	/* If debug mode, log our logo */
	if(Debug_t2o)
	{
		logit("", "triglist2ora: Attached to public memory region %s: %d\n", 
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
	RingSize = 10;  /* that is, we can stack up to 10 magnitude messages */
	
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
		logit( "e", "triglist2ora: Error starting  MessageStacker thread. "
			"Exitting.\n" );
		sprintf(Text, "triglist2ora: Error starting  MessageStacker thread. "
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
		logit( "e", "triglist2ora: Error starting  MessageStacker thread. "
			"Exitting.\n" );
		sprintf(Text, "triglist2ora: Error starting  MessageStacker thread. "
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
		/* send triglist2ora's heartbeat
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
		
		sleep_ew( 3000 );  	
	}					/* end of while!(shutdown requested)  (level 1) */  
	
	/* Termination has been requested 
	********************************/
	tport_detach( &Region ); /* detach from shared memory */
	logit( "t", "triglist2ora: Termination requested; exitting!\n" );
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
	ncommand = 11;
	for( i=0; i<ncommand; i++ )  init[i] = 0;
	nLogo = 0;
	nServer = 0;

	
	/* Open the main configuration file 
	**********************************/
	nfiles = k_open( configfile ); 
	if ( nfiles == 0 ) {
		logit("e",
			"triglist2ora: Error opening command file <%s>; exitting!\n", 
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
					logit("e", 
						"triglist2ora: Error opening command file <%s>; exitting!\n",
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
					logit("e", "triglist2ora: Too many <GetEventsFrom> commands in <%s>", 
						configfile );
					logit("e", "; max=%d; exitting!\n", (int) MAXLOGO );
					exit( -1 );
				}
				if( ( str=k_str() ) ) {
					if( GetInst( str, &GetLogo[nLogo].instid ) != 0 ) {
						logit("e", "triglist2ora: Invalid installation name <%s>", str ); 
						logit("e", " in <GetEventsFrom> cmd; exitting!\n" );
						exit( -1 );
					}
					GetLogo[nLogo+1].instid = GetLogo[nLogo].instid;
				}
				if( ( str=k_str() ) ) {
					if( GetModId( str, &GetLogo[nLogo].mod ) != 0 ) {
						logit("e", "triglist2ora: Invalid module name <%s>", str ); 
						logit("e", " in <GetEventsFrom> cmd; exitting!\n" );
						exit( -1 );
					}
					GetLogo[nLogo+1].mod = GetLogo[nLogo].mod;
				}
				if( GetType( "TYPE_TRIGLIST2K", &GetLogo[nLogo].type ) != 0 ) {
					logit("e", 
						"triglist2ora: Invalid message type <TYPE_TRIGLIST2K>" ); 
					logit("e", "; exitting!\n" );
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
/*8*/       else if( k_its("TimeoutSeconds") ) {
                WS_TimeoutSeconds = k_int();
                init[8] = 1;
            }


         /* wave server addresses and port numbers to get trace snippets from
          *******************************************************************/
/*9*/      else if( k_its("WaveServer") )
           {
                if ( nServer >= MAX_WAVESERVERS )
                {
                    logit("e",
                            "Too many <WaveServer> commands in <%s>", configfile );
                    logit("e", "; max=%d; exiting!\n", (int) MAX_WAVESERVERS );
                    exit( -1 );
                }
                if ((str = k_str()))  
                   strcpy (wsIp[nServer],str);
                if ((str = k_str()))  
                   strcpy (wsPort[nServer],str);

                nServer++;
                init[9] = 1;
            }

/*10*/      else if( k_its("TraceBufferLen") ) 
            {
                TraceBufferLen = k_int() * 1000; /* convert from kilobytes to bytes */
                init[10] = 1;
            }

            else if( k_its("Debug") ) {  /*optional command*/
                Debug_t2o=1;
            }
			
			/* Unknown command
			*****************/ 
			else {
                logit("e", "triglist2ora: <%s> Unknown command in <%s>.\n", 
					com, configfile );
                continue;
            }
			
			/* See if there were any errors processing the command 
			*****************************************************/
            if( k_err() ) {
				logit("e", 
					"triglist2ora: Bad <%s> command in <%s>; exitting!\n",
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
		logit("e", "triglist2ora: ERROR, no " );
		if ( !init[0] )  logit("e", "<LogFile> "       );
		if ( !init[1] )  logit("e", "<MyModuleId> "    );
		if ( !init[2] )  logit("e", "<RingName> "      );
		if ( !init[3] )  logit("e", "<HeartBeatInt> "  );
		if ( !init[4] )  logit("e", "<GetEventsFrom> " );
		if ( !init[5] )  logit("e", "<DBservice> "     );
		if ( !init[6] )  logit("e", "<DBpassword> "    );
		if ( !init[7] )  logit("e", "<DBuser> "        );
		if ( !init[7] )  logit("e", "<WS_TimeoutSeconds> ");
		if ( !init[8] )  logit("e", "<WaveServer> ");
		if ( !init[9] )  logit("e", "<TraceBufferLen> ");
		logit("e", "command(s) in <%s>; exitting!\n", configfile );
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
			"triglist2ora:  Invalid ring name <%s>; exitting!\n", RingName);
		exit( -1 );
	}
	
	/* Look up installations of interest
	*********************************/
	if ( GetLocalInst( &InstId ) != 0 ) {
		fprintf( stderr, 
			"triglist2ora: error getting local installation id; exitting!\n" );
		exit( -1 );
	}
	
	/* Look up modules of interest
	***************************/
	if ( GetModId( MyModName, &MyModId ) != 0 ) {
		fprintf( stderr, 
			"triglist2ora: Invalid module name <%s>; exitting!\n", MyModName );
		exit( -1 );
	}
	
	/* Look up message types of interest
	*********************************/
	if ( GetType( "TYPE_HEARTBEAT", &TypeHeartBeat ) != 0 ) {
		fprintf( stderr, 
			"triglist2ora: Invalid message type <TYPE_HEARTBEAT>; exitting!\n" );
		exit( -1 );
	}
	if ( GetType( "TYPE_ERROR", &TypeError ) != 0 ) {
		fprintf( stderr, 
			"triglist2ora: Invalid message type <TYPE_ERROR>; exitting!\n" );
		exit( -1 );
	}
	if ( GetType( "TYPE_TRIGLIST2K", &TypeTriglist ) != 0 ) {
		fprintf( stderr, 
			"triglist2ora: Invalid message type <TYPE_TRIGLIST2K>; exitting!\n" );
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
		logit( "et", "triglist2ora: %s\n", note );
	}
	
	size = strlen( msg );   /* don't include the null byte in the message */ 	
	
							/* Write the message to shared memory
	************************************/
	if( tport_putmsg( &Region, &logo, size, msg ) != PUT_OK )
	{
        if( type == TypeHeartBeat ) {
			logit("et","triglist2ora:  Error sending heartbeat.\n" );
		}
		else if( type == TypeError ) {
			logit("et","triglist2ora:  Error sending error:%d.\n", ierr );
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
	
	
	if(Debug_t2o==1) 
		logit("e",
		"triglist2ora:MessageStacker:starting to watch for "
		"trigger messages\n");
	
	/* Start service loop, picking up trigger messages
	**************************************************/
	while( 1 )
	{
		/* Get a message from transport ring
		************************************/
		res = tport_getmsg( &Region, GetLogo, nLogo, &reclogo, &recsize, 
			msg, DB_MAX_BYTES_PER_EQ-1 );
		if(Debug_t2o==1  && res==GET_OK) 
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
		
		if(Debug_t2o==1)
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
		if(Debug_t2o==1) 
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
	char 						trigMsg[DB_MAX_BYTES_PER_EQ]; 
	int							i, ret, rc, NumWavesIn;
	long						msgSize;
	MSG_LOGO					reclogo; 
	EWCoincEvtStruct			Coinc;
	EWDB_WaveformStruct			Waveform;

	
	
   	/* Tell the main thread we're ok
   	********************************/
   	MessageStufferStatus=0;
	
   	/* Get message from queue
   	*************************/
topOfLoop:
   	RequestMutex();
   	ret=dequeue( &MsgQueue, trigMsg, &msgSize, &reclogo);
   	ReleaseMutex_ew();
   	if(ret < 0 )
    { 
		sleep_ew(500); /* wait a bit */
		goto topOfLoop;
    }
   	if (reclogo.type != TypeTriglist) 
	{
		logit("et","ERROR: illegal message in queue\n");
		MessageStufferStatus = -1;
		KillSelfThread();
	}
   	trigMsg[msgSize] = '\0';   /*null terminate the message*/
	
   	if(Debug_t2o==1)
		logit ("e", "Processing a message\n");  


	free (Coinc.pChanTrigs);

	Coinc.iNumAlloc = 0;
	Coinc.iNumTrigs = 0;
	memset (&Coinc.Event, 0, sizeof (EWDB_EventStruct));
	memset (&Coinc.CoincEvt, 0, sizeof (EWDB_CoincEventStruct));
	memset (&Coinc.sAuthor, 0, 256);
	memset (&Coinc.sEventID, 0, 256);
		
				
	/* parse the magSum part of the message 
	***************************************/
	if (read_triglist (trigMsg, msgSize, &Coinc) != EW_SUCCESS)
	{
		logit ("", "Call to read_triglist failed.\n"); 
		goto topOfLoop;
	}

#ifdef DEBUG_MSG
logit ("e", "TrigList message read.\n");
logit ("e", "Trigger Time <%0.2f>, EventID <%s>, Author <%s>, got %d channels:\n",
Coinc.CoincEvt.tCoincidence, Coinc.sEventID, Coinc.sAuthor, Coinc.iNumTrigs);

for (i = 0; i < Coinc.iNumTrigs; i++)
{
 logit ("e", "Chan %d: %s.%s.%s  %0.2f    %0.2f %d\n",
     i, 
	Coinc.pChanTrigs[i].Station.Sta,
	Coinc.pChanTrigs[i].Station.Comp,
	Coinc.pChanTrigs[i].Station.Net,
	Coinc.pChanTrigs[i].Trigger.tTrigger,
	Coinc.pChanTrigs[i].tSnippetStart,
	Coinc.pChanTrigs[i].iSnippetDuration);
}
#endif DEBUG_MSG

	if (ewdb_apps_PutCoincidenceInfo (&Coinc) == EWDB_RETURN_FAILURE)
	{
		logit ("", "ewdb_apps_PutCoincidenceInfo returned error\n" );
		goto topOfLoop;
	}

	logit ("t", "Coincidence event %d inserted.\n", Coinc.Event.idEvent);



	/**************************** 
	 * Insert the waveform data 
	 ****************************/

	/* Build the current wave server menues
	**************************************/
	for (i = 0; i < MAX_WAVESERVERS; i++)
	{	
		if ( wsIp[i][0] == 0 )
			break;
		ret = wsAppendMenu (wsIp[i], wsPort[i], &MenuList, (WS_TimeoutSeconds*1000));
		if (ret != WS_ERR_NONE )
		{
			logit ("", "Error %d!  Failed to get menu from %s:%s\n",
									ret, wsIp[i], wsPort[i]);
		}
	}  /* end for i<MAX_WAVESERVERS */
	

	/* Fill the trace request structures */

	/* ste to null first time (LV NOTE) */
	if (pTraceReq != NULL)
		free (pTraceReq);

	if ((pTraceReq = (TRACE_REQ *) 
					malloc (Coinc.iNumTrigs * sizeof (TRACE_REQ))) == NULL)
	{
		logit ("", "Could not malloc %d Trace Request structs.\n",
													Coinc.iNumTrigs);
		goto topOfLoop;
	}

	if (TraceBuffer != NULL)
		free (TraceBuffer);

	if ((TraceBuffer = (char *) malloc (TraceBufferLen)) == NULL)
	{
		logit ("", "Could not malloc Trace Buffer of size %d.\n", TraceBufferLen);
		goto topOfLoop;
	}
		

	/* 
	 * This is a brute-force method: we don't check to see
	 * if a particular channel is actually available at any 
	 * WS. We'll just ask and see what we get back.
	 */
	for (i = 0; i < Coinc.iNumTrigs; i++)
	{
		strcpy (pTraceReq[i].sta, Coinc.pChanTrigs[i].Station.Sta);
		strcpy (pTraceReq[i].chan, Coinc.pChanTrigs[i].Station.Comp);
		strcpy (pTraceReq[i].net, Coinc.pChanTrigs[i].Station.Net);

		pTraceReq[i].reqStarttime = Coinc.pChanTrigs[i].tSnippetStart;
		pTraceReq[i].reqEndtime = Coinc.pChanTrigs[i].tSnippetStart + 
										Coinc.pChanTrigs[i].iSnippetDuration;

		pTraceReq[i].pBuf = TraceBuffer;
		pTraceReq[i].bufLen = TraceBufferLen;
		pTraceReq[i].timeout = WS_TimeoutSeconds; /* Fron config file */
	}


	/* Get the trace */
	NumWavesIn = 0;
	for (i = 0; i < Coinc.iNumTrigs; i++)
	{
		rc = wsGetTraceBin (&pTraceReq[i], &MenuList, (WS_TimeoutSeconds*1000));

		if (rc < 0)
		{
			logit ("", "Error %d from wsGetTraceBin while retrieving %s.%s.%s.\n", 
							rc, pTraceReq[i].sta, 
							pTraceReq[i].chan, pTraceReq[i].net);

			continue;
		}

		if (rc == WS_ERR_NONE)
		{
			logit ("", "Retrieved %s.%s.%s first time.\n",
						pTraceReq[i].sta, pTraceReq[i].chan, pTraceReq[i].net);
		}

		
		/* Error/Sanity check */
		if ((pTraceReq[i].actLen == 0) || (pTraceReq[i].actStarttime == 0))
		{
			logit ("", "OOPS: %s-%s-%s: Bad data: actLen=%d, actStarttime=%0.2f\n",
					pTraceReq[i].sta, pTraceReq[i].chan, pTraceReq[i].net,
					pTraceReq[i].actLen, pTraceReq[i].actStarttime);
			continue;
		}


		/* Fill the waveform structure */
		memset (&Waveform, 0, sizeof (EWDB_WaveformStruct));

		Waveform.idChan = Coinc.pChanTrigs[i].Trigger.idChan;
		Waveform.tStart = pTraceReq[i].actStarttime;
		Waveform.tEnd = pTraceReq[i].actEndtime;
		Waveform.iDataFormat = EWDB_WAVEFORM_FORMAT_EW_TRACE_BUF;
		Waveform.iByteLen = pTraceReq[i].actLen;
		Waveform.binSnippet = (char *) pTraceReq[i].pBuf;


		/* Save the snippet */
		if (ewdb_api_CreateWaveform (&Waveform, Coinc.Event.idEvent) 
													== EWDB_RETURN_FAILURE)
		{
			logit ("", "Call to ewdb_api_CreateWaveform failed.\n");
			goto topOfLoop;
		}

		NumWavesIn = NumWavesIn + 1;

	} /* For loop over trace requests */


	logit ("t", "%d Waveforms for event %d inserted.\n", 
								NumWavesIn, Coinc.Event.idEvent);

	sleep_ew(500);
	goto topOfLoop;	/* do the next message */

}  /* end MessageStuffer() */

