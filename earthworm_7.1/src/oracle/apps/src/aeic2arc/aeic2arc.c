

/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: aeic2arc.c,v 1.1 2004/07/01 18:58:51 labcvs Exp $
 *
 *    Revision history:
 *     $Log: aeic2arc.c,v $
 *     Revision 1.1  2004/07/01 18:58:51  labcvs
 *     Moved aeic2arc from src/data_sources to /src/oracle/apps/src JMP
 *
 *     Revision 1.2  2002/09/10 18:46:45  lucky
 *     Fixed include file
 *
 *     Revision 1.1  2002/03/22 20:13:58  lucky
 *     Initial revision
 *
 *
 *
 */

/*
 * aeicloc2arc -- convert reformatted AEIC location into Hypoinverse format
 * 
 *      Lucky Vidmar, 12/2001
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
#include <ew_event_info.h>


static  SHM_INFO  InRegion;       /* shared memory region to use for input  */
static  SHM_INFO  OutRegion;      /* shared memory region to use for output */

#define   NUM_COMMANDS 	6 	/* how many required commands in the config file */
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


/* Things to look up in the earthworm.h tables with getutil.c functions
 **********************************************************************/
static long          InKey;         /* key of transport ring for i/o     */
static long          OutKey;        /* key of transport ring for i/o     */
static unsigned char InstId;        /* local installation id             */
static unsigned char MyModId;       /* Module Id for this program        */
static unsigned char TypeHeartBeat; 
static unsigned char TypeAeic;
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
static	int  	aeicloc2arc_config (char *);
static	int  	aeicloc2arc_lookup (void);
static	void 	aeicloc2arc_status (unsigned char, short, char *);
thr_ret	MessageStacker (void *);
thr_ret	Processor (void *);

extern int     AeicMsg2EWEvent (EWEventInfoStruct *pEWEvent,
                                char *pAeic, int MsgLen);

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
		

	/* Read the configuration file(s)
	 ********************************/
	if (aeicloc2arc_config(argv[1]) != EW_SUCCESS)
	{
		fprintf (stderr, "Call to aeicloc2arc_config failed \n");
		return EW_FAILURE;
	}


	/* Look up important info from earthworm.h tables
	 ************************************************/
	if (aeicloc2arc_lookup() != EW_SUCCESS)
	{
		fprintf (stderr, "%s(%s): Call to aeicloc2arc_lookup failed \n",
						MyProgName, MyModName);
		return EW_FAILURE;
	}


	/* Initialize name of log-file & open it 
	 ***************************************/
	logit_init (argv[1], (short) MyModId, 1024, LogSwitch);
	logit ("" , "%s(%s): Read command file <%s>\n", 
						MyProgName, MyModName, argv[1]);

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


/*--------------------- setup done; start main loop -------------------------*/

  while( tport_getflag(&InRegion) != TERMINATE  &&
			         tport_getflag(&InRegion) != MyPid )
	{

		/* send heartbeat
		**********************/
		if (time (&timeNow) - timeLastBeat  >=  HeartBeatInterval) 
		{
			timeLastBeat = timeNow;
			aeicloc2arc_status (TypeHeartBeat, 0, ""); 
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
 *  aeicloc2arc_config() processes command file(s) using kom.c functions;        *
 *                    exits if any errors are encountered.                    *
 ******************************************************************************/
static int aeicloc2arc_config (char *configfile)
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
		fprintf (stderr, "Error opening command file <%s>; exiting!\n", 
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
					fprintf (stderr, 
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
					fprintf (stderr, "Too many <GetMsgsFrom> commands in <%s>; "
										"; max=%d; exiting!\n", 
											configfile, (int) MAXLOGO);
					return EW_FAILURE;
				}
				if ((str = k_str())) 
				{
					if (GetInst (str, &GetLogo[nLogo].instid) != 0) 
					{
						fprintf (stderr, "Invalid installation name <%s> in "
									"<GetMsgsFrom> cmd; exiting!\n", str);
						return EW_FAILURE;
					}
                }
				if ((str = k_str())) 
				{
					if (GetModId (str, &GetLogo[nLogo].mod) != 0) 
					{
						fprintf (stderr, 
							"Invalid module name <%s> in <GetMsgsFrom> "
										"cmd; exiting!\n", str);
						return EW_FAILURE;
					}
				}

				if (GetType ("TYPE_AEIC_LOC", &GetLogo[nLogo].type) != 0) 
				{
					fprintf (stderr, 
						"Invalid msgtype <%s> in <GetMsgsFrom> "
										"cmd; exiting!\n", str);
					return EW_FAILURE;
				}
				nLogo++;
				init[5] = 1;
			}

	/*NR*/  else if (k_its ("Debug")) 
			{
				Debug = k_int();
			}
 
			/* Unknown command
			*****************/ 
			else 
			{
				fprintf (stderr, "<%s> Unknown command in <%s>.\n", 
								com, configfile);
				continue;
			}

			/* See if there were any errors processing the command 
			*****************************************************/
			if (k_err ()) 
			{
				fprintf (stderr, 
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
		fprintf (stderr, "evt_discr: ERROR, no ");
		if (!init[0])  fprintf (stderr, "<MyModuleId> "        );
		if (!init[1])  fprintf (stderr, "<InRing> "          );
		if (!init[2])  fprintf (stderr, "<OutRing> "          );
		if (!init[3])  fprintf (stderr, "<HeartBeatInterval> "     );
		if (!init[4])  fprintf (stderr, "<LogFile> "     );
		if (!init[5])  fprintf (stderr, "<GetMsgsFrom> "     );

		fprintf (stderr, "command(s) in <%s>; exiting!\n", configfile);
		return EW_FAILURE;
	}

	return EW_SUCCESS;
}

/******************************************************************************
 *  aeicloc2arc_lookup( )   Look up important info from earthworm.h tables       *
 ******************************************************************************/
static int aeicloc2arc_lookup( void )
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

	if (GetType ("TYPE_AEIC_LOC", &TypeAeic) != 0) 
	{
		fprintf (stderr, "Invalid message type <TYPE_AEIC_LOC>; exiting!\n");
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
 * aeicloc2arc_status() builds a heartbeat or error message & puts it into       *
 *                   shared memory.  Writes errors to log file & screen.      *
 ******************************************************************************/
static void aeicloc2arc_status( unsigned char type, short ierr, char *note )
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
			aeicloc2arc_status (TypeError, ERR_TOOBIG, errText);
		}
		else if (res == GET_MISS)
		{
			sprintf (errText, "missed msg(s) i%d m%d t%d in %s",
					(int) reclogo.instid, (int) reclogo.mod, 
					(int)reclogo.type, InRingName);
			aeicloc2arc_status (TypeError, ERR_MISSMSG, errText);
		}
		else if (res == GET_NOTRACK)
		{
			sprintf (errText, "no tracking for logo i%d m%d t%d in %s",
					(int) reclogo.instid, (int) reclogo.mod, 
					(int)reclogo.type, InRingName);
			aeicloc2arc_status (TypeError, ERR_NOTRACK, errText);
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
				aeicloc2arc_status (TypeError, ERR_QUEUE, errText);
				MessageStackerStatus = -1; /* file a complaint to the main thread */
				KillSelfThread (); /* main thread will restart us */
				exit (-1);
			}
			if (ret == -1)
			{
				sprintf (errText, 
					"queue cannot allocate memory. Lost message.");
				aeicloc2arc_status (TypeError, ERR_QUEUE, errText);
			}
			if (ret == -3) 
			{
				sprintf (errText, "Queue full. Message lost.");
				aeicloc2arc_status (TypeError, ERR_QUEUE, errText);
			}
		} /* problem from enqueue */

	} /* while (1) */

}



/********************** Message Processing Thread ****************
 ******************************************************************/
thr_ret		Processor (void *dummy)
{

	char         	msg[DB_MAX_BYTES_PER_EQ]; /* "raw" retrieved message */
	char         	ArcMsg[DB_MAX_BYTES_PER_EQ]; 
	int				ret;
	int				gotMsg;
	long			msg_len;
    MSG_LOGO        reclogo;           /* logo of retrieved message */
    MSG_LOGO        outlogo;           /* logo of outgoing message */
	int				i, size;
	EWEventInfoStruct		EventInfo;



	/* Tell the main thread we're ok */
	ProcessorStatus = 0;
	
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

logit ("t", "New AEIC Msg (%d): <%s>\n", msg_len, msg);

		if (AeicMsg2EWEvent (&EventInfo, msg, msg_len) != EW_SUCCESS)
		{
			logit ("", "Call to AeicMsg2EWEvent failed.\n");
			ProcessorStatus = -1;
			KillSelfThread ();
		}

		for (i = 0; i < DB_MAX_BYTES_PER_EQ; i++)
			ArcMsg[i] = '\0';

		if (EWEvent2ArcMsg (&EventInfo, ArcMsg, 
							DB_MAX_BYTES_PER_EQ) != EW_SUCCESS)
		{
			logit ("e", "Call to EWEvent2ArcMsg failed.\n");
			ProcessorStatus = -1;
			KillSelfThread ();
		}

		size = strlen (ArcMsg);

		/*
		 * LOGO CREATION: I think that we should preserve the
		 *  Installation ID and Module ID of the original
		 *  producer of this message, but change the message
		 *  type to Hypoinverse. This will allow for proper
		 *  display of the source of the solution... But, what do
		 *  I know....
		 */

		outlogo.instid = reclogo.instid;
		outlogo.mod = reclogo.mod;
		outlogo.type = TypeHyp2000Arc;
	
logit ("e", "Writing to ring %d, logo=%d.%d.%d, len=%d\n",
OutKey, outlogo.instid, outlogo.mod, outlogo.type, size);
logit ("e", "Msg: <%s>\n", ArcMsg);

		if (tport_putmsg (&OutRegion, &outlogo, size, ArcMsg) != PUT_OK)
		{
			logit ("e", "Error writing message to the ring.\n");
			ProcessorStatus = -1;
			KillSelfThread ();
		}
	} /* infinite loop */
}

