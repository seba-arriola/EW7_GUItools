

/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: evt_discr.c,v 1.3 2002/06/07 15:58:14 patton Exp $
 *
 *    Revision history:
 *     $Log: evt_discr.c,v $
 *     Revision 1.3  2002/06/07 15:58:14  patton
 *     Made logit changes.
 *
 *     Revision 1.2  2002/06/07 15:50:16  lucky
 *     *** empty log message ***
 *
 *     Revision 1.1  2002/03/22 20:02:08  lucky
 *     Initial revision
 *
 *
 *
 */

/*
 * evt_discr -- event discriminator
 *
 *   Picks up hypoinverse messages from the InRing.  Then, it gets
 *   all events from the database that fit in the hyperbox defined 
 *   by origin time, location, depth, and magnitude.
 *   If no events are retrieved, the current event is written to the
 *   OutRing as a hypoinverse message for down-stream processing.
 *   Otherwise, for each event retrieved it calls a user supplied function 
 *   which must determine whether the current event is to be preferred 
 *   the database event. If it is, then the current event is written 
 *   out to OutRing for downstream processing. Otherwise, it inserts a
 *   new (non-preferred) Origin into the database and associates it with
 *   the preferred database event.
 *   
 *      Lucky Vidmar, 11/2001
 *   
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <earthworm.h>
#include <kom.h>
#include <swap.h>
#include <transport.h>
#include <trace_buf.h>
#include <mem_circ_queue.h>
#include <sys/types.h>
#include <ewdb_ora_api.h>
#include <ewdb_apps_utils.h>


static  SHM_INFO  InRegion;       /* shared memory region to use for input  */
static  SHM_INFO  OutRegion;      /* shared memory region to use for output */

#define   NUM_COMMANDS 	13 	/* how many required commands in the config file */
#define   APP_MAXWORD 	    30 	/* max size of DB options */

#define   MAXLOGO   5
MSG_LOGO  GetLogo[MAXLOGO];       /* array for requesting module,type,instid */
short     nLogo;
 

/* The message queue
 *******************/
#define	QUEUE_SIZE		50	    /* How many msgs can we queue */
QUEUE 	MsgQueue;				/* from queue.h */

/* Thread stuff */
#define THREAD_STACK 8192
static unsigned tidProcessor;    /* Processor thread id */
static unsigned tidStacker;      /* Thread moving messages from InRing */
                                 /* to MsgQueue */
int MessageStackerStatus = 0;      /* 0=> Stacker thread ok. <0 => dead */
int ProcessorStatus = 0;           /* 0=> Processor thread ok. <0 => dead */



/* Things to read or derive from configuration file
 **************************************************/
static char    InRingName[MAX_RING_STR];   
static char    OutRingName[MAX_RING_STR];   
static char    MyModName[MAX_MOD_STR];    
static char    MyProgName[256];  
static int     LogSwitch;       
static int     Debug;          
static long    HeartBeatInterval; 
static double	OtimeTol;
static double	LatTol;
static double	LonTol;
static double	DepthTol;
static double	MagTol;

static char    DBservice[APP_MAXWORD+1];
static char    DBuser[APP_MAXWORD+1];  
static char    DBpassword[APP_MAXWORD+1];


/* Things to look up in the earthworm.h tables with getutil.c functions
 **********************************************************************/
static long          InKey;         /* key of transport ring for i/o     */
static long          OutKey;        /* key of transport ring for i/o     */
static unsigned char InstId;        /* local installation id             */
static unsigned char MyModId;       /* Module Id for this program        */
static unsigned char TypeHeartBeat; 
static unsigned char TypeHyp2000Arc;
static unsigned char TypeError;
static unsigned char InstWild;
static unsigned char ModWild;

/* Error messages 
 *******************/
#define  ERR_MISSMSG       0   /* message missed in transport ring       */
#define  ERR_TOOBIG        1   /* retreived msg too large for buffer     */
#define  ERR_NOTRACK       2   /* msg retreived; tracking limit exceeded */
#define  ERR_QUEUE         4   /* trouble with the MsgQueue operation */

static char  errText[256];    /* string for log/error messages          */

pid_t MyPid;	/** Hold our process ID to be sent with heartbeats **/

/* Functions in this source file 
 *******************************/
static	int  	evtdiscr_config (char *);
static	int  	evtdiscr_lookup (void);
static	void 	evtdiscr_status (unsigned char, short, char *);
thr_ret	MessageStacker (void *);
thr_ret	Processor (void *);
static int CompareEventPreferrences (EWEventInfoStruct *, EWEventInfoStruct *);

main (int argc, char **argv)
{
	long			timeNow;		   /* current time                  */ 
	long			timeLastBeat;	   /* time last heartbeat was sent  */
	long			recsize;		   /* size of retrieved message     */
	MSG_LOGO		reclogo;		   /* logo of retrieved message     */
	char			*flushmsg;


	/* Check command line arguments 
	 ******************************/
	if (argc != 2)
	{
		fprintf (stderr, "Usage: evt_discr <configfile>\n");
		return EW_FAILURE;
	}

	/* To be used in loging functions
	 ********************************/
	if (get_prog_name (argv[0], MyProgName) != EW_SUCCESS)
	{
		fprintf (stderr, "Call to get_prog_name failed.\n");
		return EW_FAILURE;
	}

	/* Initialize name of log-file & open it 
	 ***************************************/
	logit_init (argv[1], 0, 1024, 1);	

	/* Read the configuration file(s)
	 ********************************/
	if (evtdiscr_config(argv[1]) != EW_SUCCESS)
	{
		fprintf (stderr, "Call to evtdiscr_config failed \n");
		return EW_FAILURE;
	}
	logit ("" , "%s(%s): Read command file <%s>\n", 
						MyProgName, MyModName, argv[1]);

	/* Look up important info from earthworm.h tables
	 ************************************************/
	if (evtdiscr_lookup() != EW_SUCCESS)
	{
		fprintf (stderr, "%s(%s): Call to evtdiscr_lookup failed \n",
						MyProgName, MyModName);
		return EW_FAILURE;
	}


	/* Reinitialize logit to the desired logging level
	 *************************************************/
	logit_init (argv[1], 0, 1024, LogSwitch);


	/* Get our process ID
	 **********************/
	if ((MyPid = getpid ()) == -1)
	{
		logit ("e", "%s(%s): Call to getpid failed. Exiting.\n",
										MyProgName, MyModName);
		return (EW_FAILURE);
	}

	/* Attach to Input shared memory ring 
	 *******************************************/
	tport_attach (&InRegion, InKey);
	logit ("", "%s(%s): Attached to public memory region %s: %d\n", 
	          MyProgName, MyModName, InRingName, InKey);

	/* Attach to Output shared memory ring 
	 *******************************************/
	tport_attach (&OutRegion, OutKey);
	logit ("", "%s(%s): Attached to public memory region %s: %d\n", 
	          MyProgName, MyModName, OutRingName, OutKey);


	/* Force a heartbeat to be issued in first pass thru main loop
	 *************************************************************/
	timeLastBeat = time (&timeNow) - HeartBeatInterval - 1;

	/* Flush the incomming transport ring 
	 *************************************/
	if ((flushmsg = (char *) malloc (DB_MAX_BYTES_PER_EQ)) ==  NULL)
	{
		logit ("e", "can't allocate flushmsg; exiting.\n");
		return EW_FAILURE;
	}

	while (tport_getmsg (&InRegion, GetLogo, nLogo, &reclogo,
			&recsize, flushmsg, (DB_MAX_BYTES_PER_EQ - 1)) != GET_NONE)

        ;



	/* Create MsgQueue mutex */
	CreateMutex_ew();


	/* Allocate the message Queue
	 ********************************/
	initqueue (&MsgQueue, QUEUE_SIZE, DB_MAX_BYTES_PER_EQ);

	/* Start message stacking thread which will read 
	 * messages from the InRing and put them into the Queue 
	 *******************************************************/
	if (StartThread (MessageStacker, (unsigned) THREAD_STACK, &tidStacker) == -1)
	{
		logit( "e", "Error starting MessageStacker thread.  Exiting.\n");
		tport_detach (&InRegion);
		return EW_FAILURE;
	}

	MessageStackerStatus = 0; /*assume the best*/


	/* Start decimator thread which will read messages from
	 * the Queue, process them and write them to the OutRing
	 *******************************************************/
	if (StartThread (Processor, (unsigned) THREAD_STACK, &tidProcessor) == -1)
	{
		logit( "e", "Error starting Processor thread.  Exiting.\n");
		tport_detach (&InRegion);
		return EW_FAILURE;
	}

	ProcessorStatus = 0; /*assume the best*/

	/* Initialize the Ora_API */
	if (ewdb_api_Init (DBuser, DBpassword, DBservice) != EWDB_RETURN_SUCCESS)
	{
		logit ("e", "Call to ewdb_api_Init failed.\n");
		return EW_FAILURE;
	}


/*--------------------- setup done; start main loop -------------------------*/

  while( tport_getflag(&InRegion) != TERMINATE  &&
			         tport_getflag(&InRegion) != MyPid )
	{

		/* send heartbeat
		**********************/
		if (time (&timeNow) - timeLastBeat  >=  HeartBeatInterval) 
		{
			timeLastBeat = timeNow;
			evtdiscr_status (TypeHeartBeat, 0, ""); 
		}

		/* Check on our threads */
		if (MessageStackerStatus < 0)
		{
			logit ("et", "MessageStacker thread died. Exiting\n");
			tport_detach (&InRegion);
			return EW_FAILURE;
		}

		if (ProcessorStatus < 0)
		{
			logit ("et", "Processor thread died. Exiting\n");
			tport_detach (&InRegion);
			return EW_FAILURE;
		}

		sleep_ew (1000);

	} /* wait until TERMINATE is raised  */  

	/* Termination has been requested
	 ********************************/
	tport_detach (&InRegion);
	logit ("t", "Termination requested; exiting!\n" );
	return EW_SUCCESS;

}

/******************************************************************************
 *  evtdiscr_config() processes command file(s) using kom.c functions;        *
 *                    exits if any errors are encountered.                    *
 ******************************************************************************/
static int evtdiscr_config (char *configfile)
{
	char     		init[NUM_COMMANDS];     
	int      		nmiss;
	char    		*com;
	char    		*str;
	int      		nfiles;
	int      		success;
	int      		i;


	/* Set to zero one init flag for each required command 
	*****************************************************/   
	for (i = 0; i < NUM_COMMANDS; i++)
		init[i] = 0;

	nLogo = 0;

	/* Open the main configuration file 
	**********************************/
	nfiles = k_open (configfile); 
	if (nfiles == 0) 
	{
		logit ("e", "Error opening command file <%s>; exiting!\n", 
															configfile);
		return EW_FAILURE;
	}

	/* Process all command files
	***************************/
	while (nfiles > 0)   /* While there are command files open */
	{
		while (k_rd ())        /* Read next line from active file  */
		{  
			com = k_str ();         /* Get the first token from line */

			/* Ignore blank lines & comments
			*******************************/
			if (!com)
				continue;
			if (com[0] == '#')
				continue;

			/* Open a nested configuration file 
			**********************************/
			if (com[0] == '@') 
			{
				success = nfiles + 1;
				nfiles  = k_open (&com[1]);
				if (nfiles != success) 
				{
					logit ("e", 
				  	  "Error opening command file <%s>; exiting!\n", &com[1]);
					return EW_FAILURE;
				}
				continue;
			}

			/* Process anything else as a command 
			************************************/
	/*0*/ 	if (k_its ("MyModuleId")) 
			{
				if ((str = k_str ()) != NULL)
				{
					strcpy (MyModName, str);
					init[0] = 1;
				}
			}
	/*1*/	else if (k_its ("InRing")) 
			{
				if ((str = k_str ()) != NULL)
				{
					strcpy (InRingName, str);
					init[1] = 1;
				}
			}
	/*2*/	else if (k_its ("OutRing")) 
			{
				if ((str = k_str ()) != NULL)
				{
					strcpy (OutRingName, str);
					init[2] = 1;
				}
			}
	/*3*/	else if (k_its ("HeartBeatInterval")) 
			{
				HeartBeatInterval = k_long ();
				init[3] = 1;
			}
	/*4*/	else if (k_its ("LogFile"))
			{
				LogSwitch = k_int();
				init[4] = 1;
			}

			/* Enter installation & module types to get
			 *******************************************/
	/*5*/	else if (k_its ("GetMsgsFrom")) 
			{
				if (nLogo >= MAXLOGO) 
				{
					logit ("e", "Too many <GetMsgsFrom> commands in <%s>; "
										"; max=%d; exiting!\n", 
											configfile, (int) MAXLOGO);
					return EW_FAILURE;
				}
				if ((str = k_str())) 
				{
					if (GetInst (str, &GetLogo[nLogo].instid) != 0) 
					{
						logit ("e", "Invalid installation name <%s> in "
									"<GetMsgsFrom> cmd; exiting!\n", str);
						return EW_FAILURE;
					}
                }
				if ((str = k_str())) 
				{
					if (GetModId (str, &GetLogo[nLogo].mod) != 0) 
					{
						logit ("e", 
							"Invalid module name <%s> in <GetMsgsFrom> "
										"cmd; exiting!\n", str);
						return EW_FAILURE;
					}
				}
				
				if (GetType ("TYPE_HYP2000ARC", &GetLogo[nLogo].type) != 0) 
				{
					fprintf (stderr, 
						"Could not get numeric value for TYPE_HYP2000ARC");
					return EW_FAILURE;	
				}
				nLogo++;
				init[5] = 1;
			}
	/*6*/	else if (k_its ("DBuser")) 
			{
				if ((str = k_str ()) != NULL)
				{
					strcpy (DBuser, str);
					init[6] = 1;
				}
			}
	/*7*/	else if (k_its ("DBpassword")) 
			{
				if ((str = k_str ()) != NULL)
				{
					strcpy (DBpassword, str);
					init[7] = 1;
				}
			}
	/*8*/	else if (k_its ("DBservice")) 
			{
				if ((str = k_str ()) != NULL)
				{
					strcpy (DBservice, str);
					init[8] = 1;
				}
			}
	/*9*/	else if (k_its ("OtimeTol")) 
			{
				OtimeTol = k_val();
				init[9] = 1;
			}
	/*10*/	else if (k_its ("LatTol")) 
			{
				LatTol = k_val();
				init[10] = 1;
			}
	/*11*/	else if (k_its ("LonTol")) 
			{
				LonTol = k_val();
				init[11] = 1;
			}
	/*12*/	else if (k_its ("DepthTol")) 
			{
				DepthTol = k_val();
				init[12] = 1;
			}
	/*13*/	else if (k_its ("MagTol")) 
			{
				MagTol = k_val();
				init[13] = 1;
			}
	/*NR*/  else if (k_its ("Debug")) 
			{
				Debug = k_int();
			}
 
			/* Unknown command
			*****************/ 
			else 
			{
				logit ("e", "<%s> Unknown command in <%s>.\n", 
								com, configfile);
				continue;
			}

			/* See if there were any errors processing the command 
			*****************************************************/
			if (k_err ()) 
			{
				logit ("e", 
					"Bad <%s> command in <%s>; exiting!\n",
						com, configfile);
				return EW_FAILURE;
			}

		} /** while k_rd() **/

		nfiles = k_close ();

	} /** while nfiles **/

	/* After all files are closed, check init flags for missed commands
	******************************************************************/
	nmiss = 0;
	for (i = 0; i < NUM_COMMANDS; i++)  
		if (!init[i]) 
			nmiss++;

	if (nmiss) 
	{
		logit ("e", "evt_discr: ERROR, no ");
		if (!init[0])  logit ("e", "<MyModuleId> "        );
		if (!init[1])  logit ("e", "<InRing> "          );
		if (!init[2])  logit ("e", "<OutRing> "          );
		if (!init[3])  logit ("e", "<HeartBeatInterval> "     );
		if (!init[4])  logit ("e", "<LogFile> "     );
		if (!init[5])  logit ("e", "<GetMsgsFrom> "     );
		if (!init[6])  logit ("e", "<DBuser> "     );
		if (!init[7])  logit ("e", "<DBpassword> "     );
		if (!init[8])  logit ("e", "<DBservice> "     );
		if (!init[9])  logit ("e", "<OtimeTol> "     );
		if (!init[10]) logit ("e", "<LatTol> "     );
		if (!init[11]) logit ("e", "<LonTol> "     );
		if (!init[12]) logit ("e", "<DepthTol> "     );
		if (!init[13]) logit ("e", "<MagTol> "     );

		logit ("e", "command(s) in <%s>; exiting!\n", configfile);
		return EW_FAILURE;
	}

	return EW_SUCCESS;
}

/******************************************************************************
 *  evtdiscr_lookup( )   Look up important info from earthworm.h tables       *
 ******************************************************************************/
static int evtdiscr_lookup( void )
{

	/* Look up keys to shared memory regions
	*************************************/
	if ((InKey = GetKey (InRingName)) == -1) 
	{
		fprintf (stderr, "Invalid ring name <%s>; exiting!\n", InRingName);
		return EW_FAILURE;
	}

	if ((OutKey = GetKey (OutRingName)) == -1) 
	{
		fprintf (stderr, "Invalid ring name <%s>; exiting!\n", OutRingName);
		return EW_FAILURE;
	}

	/* Look up installations of interest
	*********************************/
	if (GetLocalInst (&InstId) != 0) 
	{
		fprintf (stderr, 
			"error getting local installation id; exiting!\n");
		return EW_FAILURE;
	}


	if (GetInst ("INST_WILDCARD", &InstWild ) != 0) 
	{ 
		fprintf (stderr, 
			"error getting wildcard installation id; exiting!\n");
		return EW_FAILURE;
	}

	/* Look up modules of interest
	******************************/
	if (GetModId (MyModName, &MyModId) != 0) 
	{
		fprintf (stderr, 
			"Invalid module name <%s>; exiting!\n", MyModName);
		return EW_FAILURE;
	}

	if (GetModId ("MOD_WILDCARD", &ModWild) != 0) 
	{
		fprintf (stderr, 
			"Invalid module name <MOD_WILDCARD>; exiting!\n");
		return EW_FAILURE;
	}

	/* Look up message types of interest
	*********************************/
	if (GetType ("TYPE_HEARTBEAT", &TypeHeartBeat) != 0) 
	{
		fprintf (stderr, "Invalid message type <TYPE_HEARTBEAT>; exiting!\n");
		return EW_FAILURE;
	}

	if (GetType ("TYPE_ERROR", &TypeError) != 0) 
	{
		fprintf (stderr, "Invalid message type <TYPE_ERROR>; exiting!\n");
		return EW_FAILURE;
	}

	if (GetType ("TYPE_HYP2000ARC", &TypeHyp2000Arc) != 0) 
	{
		fprintf (stderr, "Invalid message type <TYPE_HYP2000ARC>; exiting!\n");
		return EW_FAILURE;
	}


	return EW_SUCCESS;

} 

/******************************************************************************
 * evtdiscr_status() builds a heartbeat or error message & puts it into       *
 *                   shared memory.  Writes errors to log file & screen.      *
 ******************************************************************************/
static void evtdiscr_status( unsigned char type, short ierr, char *note )
{
   MSG_LOGO    logo;
   char        msg[256];
   long        size;
   long        t;
 
	/* Build the message 
	*******************/ 
	logo.instid = InstId;
	logo.mod    = MyModId;
	logo.type   = type;

	time (&t);

	if (type == TypeHeartBeat)
	{
		sprintf (msg, "%ld %ld\n\0", t, MyPid);
	}
	else if (type == TypeError)
	{
		sprintf (msg, "%ld %hd %s\n\0", t, ierr, note);
		logit ("et", "%s(%s): %s\n", MyProgName, MyModName, note);
	}

	size = strlen (msg);   /* don't include the null byte in the message */     

	/* Write the message to shared memory
	************************************/
	if (tport_putmsg (&OutRegion, &logo, size, msg) != PUT_OK)
	{
		if (type == TypeHeartBeat) 
		{
			logit ("et","%s(%s):  Error sending heartbeat.\n",
											MyProgName, MyModName);
		}
		else if (type == TypeError) 
		{
			logit ("et","%s(%s):  Error sending error:%d.\n", 
										MyProgName, MyModName, ierr);
		}
	}

}


/********************** Message Stacking Thread *******************
 *           Move messages from transport to memory queue         *
 ******************************************************************/
thr_ret		MessageStacker (void *dummy)
{
	char         	msg[DB_MAX_BYTES_PER_EQ]; /* "raw" retrieved message */
	int          	res;
	long         	recsize;        /* size of retrieved message */
	MSG_LOGO     	reclogo;        /* logo of retrieved message */
	int          	ret;
	int				gotMsg;


	/* Tell the main thread we're ok
	 ********************************/
	MessageStackerStatus = 0;

	/* Start service loop, picking up trigger messages
	 **************************************************/
	while (1)
	{
		/* Get a message from transport ring
		 ************************************/
		gotMsg = FALSE;
		while (gotMsg == FALSE)
		{
			res = tport_getmsg (&InRegion, GetLogo, nLogo, &reclogo, 
								&recsize, msg, DB_MAX_BYTES_PER_EQ-1);

			if (res == GET_NONE)
			{
				/*wait if no messages for us */
				sleep_ew (500); 
			} 
			else
				gotMsg = TRUE;
		}



		/* Check return code; report errors
		***********************************/
		if (res == GET_TOOBIG)
		{
			sprintf (errText, "msg[%ld] i%d m%d t%d too long for target",
						recsize, (int) reclogo.instid,
						(int) reclogo.mod, (int)reclogo.type);
			evtdiscr_status (TypeError, ERR_TOOBIG, errText);
		}
		else if (res == GET_MISS)
		{
			sprintf (errText, "missed msg(s) i%d m%d t%d in %s",
					(int) reclogo.instid, (int) reclogo.mod, 
					(int)reclogo.type, InRingName);
			evtdiscr_status (TypeError, ERR_MISSMSG, errText);
		}
		else if (res == GET_NOTRACK)
		{
			sprintf (errText, "no tracking for logo i%d m%d t%d in %s",
					(int) reclogo.instid, (int) reclogo.mod, 
					(int)reclogo.type, InRingName);
			evtdiscr_status (TypeError, ERR_NOTRACK, errText);
		}


		/* Queue retrieved msg (res==GET_OK,GET_MISS,GET_NOTRACK)
			*********************************************************/

		RequestMutex ();
		/* put it into the queue */
		ret = enqueue (&MsgQueue, msg, recsize, reclogo); 
		ReleaseMutex_ew ();

		if (ret != 0)
		{
			if (ret == -2)  /* Serious: quit */
			{
				sprintf (errText, "internal queue error. Terminating.");
				evtdiscr_status (TypeError, ERR_QUEUE, errText);
				MessageStackerStatus = -1; /* file a complaint to the main thread */
				KillSelfThread (); /* main thread will restart us */
				exit (-1);
			}
			if (ret == -1)
			{
				sprintf (errText, 
					"queue cannot allocate memory. Lost message.");
				evtdiscr_status (TypeError, ERR_QUEUE, errText);
			}
			if (ret == -3) 
			{
				sprintf (errText, "Queue full. Message lost.");
				evtdiscr_status (TypeError, ERR_QUEUE, errText);
			}
		} /* problem from enqueue */

	} /* while (1) */

}



/********************** Message Processing Thread ****************
 ******************************************************************/
thr_ret		Processor (void *dummy)
{

	char         	msg[DB_MAX_BYTES_PER_EQ]; /* "raw" retrieved message */
	int				ret;
	int				gotMsg;
	long			msg_len;
    MSG_LOGO        reclogo;           /* logo of retrieved message */
    MSG_LOGO        outlogo;           /* logo of outgoing message */
	int				i, GetEvtFlags;
	EWEventInfoStruct		EventInfo;
	EWEventInfoStruct		DBEvt;
	EWDB_EventListStruct    EVMin, EVMax;
	EWDB_EventListStruct	*pEventBuffer;
	int						BufSize, retval, NumRetr;
	int						StartTime, EndTime;



	/* Tell the main thread we're ok */
	ProcessorStatus = 0;
	
	/* Allocate the event buffer */
	BufSize = 1000;
	if ((pEventBuffer = (EWDB_EventListStruct *) 
			malloc (BufSize * sizeof (EWDB_EventListStruct))) == NULL)
	{
		logit ("", "Could not malloc pEventBuffer\n");
		ProcessorStatus = -1;
		KillSelfThread ();
	}

	while (1)
	{
		gotMsg = FALSE;
	
		while (gotMsg == FALSE)
		{
			RequestMutex ();
			ret = dequeue (&MsgQueue, msg, &msg_len, &reclogo);
			ReleaseMutex_ew ();

			if (ret < 0)
			{
				/* empty queue, sleep for a while */
				sleep_ew (500);
			}
			else
			{
				gotMsg = TRUE;
			}
		} /* While waiting for messages on the queue */

		ProcessorStatus = 0;

		/* Process the message 
	 	 **********************/

logit ("e", "Got new message of length %d:\n", msg_len);
logit ("e", "Msg: <%s>\n", msg);

		if (ArcMsg2EWEvent (&EventInfo, msg, msg_len) != EW_SUCCESS)
		{
			logit ("", "Call to Hypo71Msg2EWEvent failed.\n");
			ProcessorStatus = -1;
			KillSelfThread ();
		}

logit ("e", "ArcMsg2EWEvent done -- evtId is %d\n",
EventInfo.Event.idEvent);

		/* Set the tolerance boxes */
		memset(&EVMin,0,sizeof(EWDB_EventListStruct));
		memset(&EVMax,0,sizeof(EWDB_EventListStruct));

		if (LatTol != 0.0)
		{
			EVMin.dLat = EventInfo.PrefOrigin.dLat - LatTol;
			EVMax.dLat = EventInfo.PrefOrigin.dLat + LatTol;
		}
		else 
		{
			EVMin.dLat = 0.0;
			EVMax.dLat = 0.0;
		}

		if (LonTol != 0.0)
		{
			EVMin.dLon = EventInfo.PrefOrigin.dLon - LonTol;
			EVMax.dLon = EventInfo.PrefOrigin.dLon + LonTol;
		}
		else
		{
			EVMin.dLon = 0.0;;
			EVMax.dLon = 0.0;
		}

		if (DepthTol != 0.0)
		{
			EVMin.dDepth = EventInfo.PrefOrigin.dDepth - DepthTol;
			EVMax.dDepth = EventInfo.PrefOrigin.dDepth + DepthTol;
		}
		else
		{
			EVMin.dDepth = 0.0;
			EVMax.dDepth = 0.0;
		}

		if (MagTol != 0.0)
		{
			EVMin.dPrefMag = EventInfo.Mags[0].dMagAvg - MagTol;
			EVMax.dPrefMag = EventInfo.Mags[0].dMagAvg + MagTol;
		}
		else
		{
			EVMin.dPrefMag = 0.0;
			EVMax.dPrefMag = 0.0;
		}

		strcpy (EVMin.Event.szSource, "*");
		strcpy (EVMax.Event.szSource, "*");

		if (OtimeTol != 0.0)
		{
			StartTime = (int) (EventInfo.PrefOrigin.tOrigin - OtimeTol);
			EndTime = (int) EventInfo.PrefOrigin.tOrigin + OtimeTol;
		}
		else
		{
			StartTime = 0;
			EndTime = 0;
		}


logit ("e", "Min %d; %f,%f; %f, %f \n",
StartTime, EVMin.dLat, EVMin.dLon, EVMin.dDepth, EVMin.dPrefMag);
logit ("e", "Max %d; %f,%f; %f, %f \n",
EndTime, EVMax.dLat, EVMax.dLon, EVMax.dDepth, EVMax.dPrefMag);

		/* retrieve all events from the DB within the tolerance */
		retval = ewdb_api_GetEventList (pEventBuffer, BufSize, 
						StartTime, EndTime, &EVMin, &EVMax, &NumRetr);

logit ("e", "done ewdb_api_GetEventList: retval is %d, NumRetr is %d\n", retval, NumRetr);

		if (retval < 0)
		{
			if (retval == -1)
			{
				logit ("e", "Call to ewdb_api_GetEventList failed.\n");
				ProcessorStatus = -1;
				KillSelfThread ();
			}
			else
				NumRetr = 0 - retval;
		}
				
for (i = 0; i < NumRetr; i++)
logit ("e", "Evt %d: %d, %0.2f, %0.2f, %0.2f, %0.2f, %0.2f\n", i, 
pEventBuffer[i].Event.idEvent,
pEventBuffer[i].dOT,
pEventBuffer[i].dLat,
pEventBuffer[i].dLon,
pEventBuffer[i].dDepth,
pEventBuffer[i].dPrefMag);

		/* 
		 * If we retrieved more than one event that 
		 * fits within the tolerance, continue checking to see if it is preferred.
		 * over the db events. Otherwise, write the current event
		 * to the OutRing as a Hypoinverse archive message.
		 */

		/*
		 * LOGO CREATION: I think that we should preserve the
		 *  original logo of the producer of this message. 
		 *  This will allow for proper display of the source of 
		 * the solution... But, what do I know....
		 */
		outlogo.instid = reclogo.instid;
		outlogo.mod = reclogo.mod;
		outlogo.type = reclogo.type;

		GetEvtFlags = GETEWEVENTINFO_SUMMARYINFO | GETEWEVENTINFO_PICKS
						| GETEWEVENTINFO_STAMAGS;

		if (NumRetr > 0)
		{
			logit ("e", "Retrieved %d events for the desired box.\n", NumRetr);

			for (i = 0; i < NumRetr; i++)
			{
				logit ("e", "Checking against event %d\n", pEventBuffer[i].Event.idEvent);

				/* Get everything at once */
				if (ewdb_apps_GetDBEventInfoII (&DBEvt, pEventBuffer[i].Event.idEvent, 
							-1, GetEvtFlags) != EWDB_RETURN_SUCCESS)
				{
					logit ("e", "Call to ewdb_apps_GetDBEventInfo failed\n");
					return EW_FAILURE;
				}

				if (CompareEventPreferrences (&EventInfo, &DBEvt) < 0)
				{
					logit ("e", "Putting message with logo <%d:%d:%d> to OutRing\n", 
									outlogo.instid, outlogo.mod, outlogo.type);

					if (tport_putmsg (&OutRegion, &outlogo, msg_len, msg) != PUT_OK)
					{
						logit ("e", "Error writing message to the ring.\n");
						ProcessorStatus = -1;
						KillSelfThread ();
					}

				} /* current event was preferred -- written out to ring */
				else
				{
					/* Write it to the database as a non-preferred Origin */



				}

			} /* For-loop over retrieved database events */

		} /* if we retrieved any events from the database */

		else
		{
			logit ("e", "No events retrieved for the desired box.\n");

			logit ("e", "Putting message with logo <%d:%d:%d> to OutRing\n", 
							outlogo.instid, outlogo.mod, outlogo.type);

			if (tport_putmsg (&OutRegion, &outlogo, msg_len, msg) != PUT_OK)
			{
				logit ("e", "Error writing message to the ring.\n");
				ProcessorStatus = -1;
				KillSelfThread ();
			}
		}

	} /* infinite loop */
}



/********************** CompareEventPreferrencesd ****************

 if pEvt1 has lower preferrence (i.e., is preferred) return -1;
 if pEvt1 has higher preferrence (i.e., is not preferred) return 1;
 if preferrences are equal, return 0.

 ******************************************************************/
static int CompareEventPreferrences (EWEventInfoStruct *pEvt1, 
												EWEventInfoStruct *pEvt2)
{


	return 0;
}

