

/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: email_sender.c,v 1.5 2002/06/07 15:02:13 patton Exp $
 *
 *    Revision history:
 *     $Log: email_sender.c,v $
 *     Revision 1.5  2002/06/07 15:02:13  patton
 *     Made logit changes.
 *
 *     Revision 1.4  2002/03/22 20:04:24  lucky
 *     Pre v6.1 checkin
 *
 *     Revision 1.3  2001/07/28 00:50:31  lucky
 *     *** empty log message ***
 *
 *     Revision 1.2  2001/07/01 21:55:08  davidk
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
 *     Revision 1.1  2001/05/18 19:08:07  lucky
 *     Initial revision
 *
 *     Revision 1.1  2001/05/15 02:15:00  davidk
 *     Initial revision
 *
 *
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
#include <mem_circ_queue.h>
#include <alarms.h>
#include <ewdb_apps_utils.h>
#include <ewdb_ora_api.h>


static  SHM_INFO  Region;      /* shared memory region to use for i/o    */


#define   NUM_COMMANDS 	8 	/* how many required commands in the config file */
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
static unsigned tidStacker;      /* Thread moving messages from Ring */
                                 /* to MsgQueue */
int MessageStackerStatus = 0;      /* 0=> Stacker thread ok. <0 => dead */
int ProcessorStatus = 0;           /* 0=> Processor thread ok. <0 => dead */



/* Things to read or derive from configuration file
 **************************************************/
static char    RingName[MAX_RING_STR];   /* name of transport ring for i/o */
static char    MyModName[MAX_MOD_STR];   /* this module's given name */
static char    MyProgName[256];     /* this module's program name */
static char    Subject[2*ALARM_MSG_SIZE]; 
static int     LogSwitch;           /* 0 if no logfile should be written */
static int     Debug;               /* level of debug required   */
static long    HeartBeatInterval;   /* seconds between heartbeats */


/* Things to look up in the earthworm.h tables with getutil.c functions
 **********************************************************************/
static long          RingKey;       /* key of transport ring for i/o     */
static unsigned char InstId;        /* local installation id             */
static unsigned char MyModId;       /* Module Id for this program        */
static unsigned char TypeHeartBeat; 
static unsigned char TypeEmail;
static unsigned char TypeError;
static unsigned char InstWild;
static unsigned char ModWild;

/* Error messages used by email_sender 
 ***************************************/
#define  ERR_MISSMSG       0   /* message missed in transport ring       */
#define  ERR_TOOBIG        1   /* retreived msg too large for buffer     */
#define  ERR_NOTRACK       2   /* msg retreived; tracking limit exceeded */
#define  ERR_QUEUE         4   /* trouble with the MsgQueue operation */

static char  errText[256];    /* string for log/error messages          */

pid_t MyPid;	/** Hold our process ID to be sent with heartbeats **/

/* Functions in this source file 
 *******************************/
static	int  	email_config (char *);
static	int  	email_lookup (void);
static	void  	email_status (unsigned char, short, char *);
thr_ret			MessageStacker (void *);
thr_ret			Processor (void *);


extern	int SendMail (char person[][60], int nmail, char *mailProg,
                      char *subject, char *msg, char *msgPrefix,
                      char *msgSuffix, char *mailServer);


/*
 * We pick up Email Alarm messages from the ring, send
 * corresponding emails, and update the sending time
 * in the Alarms Audit table in the database.
 */
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
		fprintf (stderr, "Usage: email_sender <configfile>\n");
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
	logit_init (argv[1], 0, 2*ALARM_MSG_SIZE, 1);
		
	/* Read the configuration file(s)
	 ********************************/
	if (email_config(argv[1]) != EW_SUCCESS)
	{
		fprintf (stderr, "Call to email_config failed \n");
		return EW_FAILURE;
	}
	logit ("" , "%s(%s): Read command file <%s>\n", 
						MyProgName, MyModName, argv[1]);

	/* Look up important info from earthworm.h tables
	 ************************************************/
	if (email_lookup() != EW_SUCCESS)
	{
		fprintf (stderr, "%s(%s): Call to email_lookup failed \n",
						MyProgName, MyModName);
		return EW_FAILURE;
	}


	/* Reinitialize logit to desired logging level
	 *********************************************/
	logit_init (argv[1], (short) MyModId, 2*ALARM_MSG_SIZE, LogSwitch);

	/* Get our process ID
	 **********************/
	if ((MyPid = getpid ()) == -1)
	{
		logit ("e", "%s(%s): Call to getpid failed. Exitting.\n",
										MyProgName, MyModName);
		return (EW_FAILURE);
	}


	/* Attach to Input shared memory ring 
	 *******************************************/
	tport_attach (&Region, RingKey);
	logit ("", "%s(%s): Attached to public memory region %s: %d\n", 
	          MyProgName, MyModName, RingName, RingKey);

	/* Force a heartbeat to be issued in first pass thru main loop
	 *************************************************************/
	timeLastBeat = time (&timeNow) - HeartBeatInterval - 1;


	/* Flush the incomming transport ring 
	 *************************************/
	if ((flushmsg = (char *) malloc (DB_MAX_BYTES_PER_EQ)) ==  NULL)
	{
		logit ("e", "Can't allocate flushmsg; exitting.\n");
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

	/* Start message stacking thread which will read 
	 * messages from the Ring and put them into the Queue 
	 *******************************************************/
	if (StartThread (MessageStacker, (unsigned) THREAD_STACK, &tidStacker) == -1)
	{
		logit( "e", "Error starting MessageStacker thread.  Exitting.\n");
		tport_detach (&Region);
		return EW_FAILURE;
	}

	MessageStackerStatus = 0; /*assume the best*/


	/* Start decimator thread which will read messages from
	 * the Queue, process them and write them to the OutRing
	 *******************************************************/
	if (StartThread (Processor, (unsigned) THREAD_STACK, &tidProcessor) == -1)
	{
		logit( "e", "Error starting Processor thread.  Exitting.\n");
		tport_detach (&Region);
		return EW_FAILURE;
	}

	ProcessorStatus = 0; /*assume the best*/

	/* Initialize the Ora_API */
	if (ewdb_api_Init (DBuser, DBpassword, DBservice) !=
											EWDB_RETURN_SUCCESS)
	{
		logit ("e", "Call to ewdb_api_Init failed.\n");
		ProcessorStatus = -1; /* file a complaint to the main thread */
		KillSelfThread ();    /* main thread will restart us */
	}
	logit ("et", "CONNECTED TO DB. Starting main loop\n");


/*--------------------- setup done; start main loop -------------------------*/

	while (tport_getflag (&Region) != TERMINATE)
	{

		/* send email_sender' heartbeat
		***************************/
		if (time (&timeNow) - timeLastBeat  >=  HeartBeatInterval) 
		{
			timeLastBeat = timeNow;
			email_status (TypeHeartBeat, 0, ""); 
		}

		/* Check on our threads */
		if (MessageStackerStatus < 0)
		{
			logit ("et", "MessageStacker thread died. Exitting\n");
			tport_detach (&Region);
			return EW_FAILURE;
		}

		if (ProcessorStatus < 0)
		{
			logit ("et", "Processor thread died. Exitting\n");
			tport_detach (&Region);
			return EW_FAILURE;
		}

		sleep_ew (1000);

	} /* wait until TERMINATE is raised  */  

	/* Termination has been requested
	 ********************************/
	tport_detach (&Region);
	logit ("t", "Termination requested; exitting!\n" );
	return EW_SUCCESS;

}

/******************************************************************************
 *  email_config() processes command file(s) using kom.c functions;        *
 *                    exits if any errors are encountered.                    *
 ******************************************************************************/
static int email_config (char *configfile)
{
	char     		init[NUM_COMMANDS];     
						/* init flags, one byte for each required command */
	int      		nmiss;
						/* number of required commands that were missed   */
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
	strcpy (Subject, "Not Set");

	/* Open the main configuration file 
	**********************************/
	nfiles = k_open (configfile); 
	if (nfiles == 0) 
	{
		logit ("e", "Error opening command file <%s>; exitting!\n", 
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
				  	  "Error opening command file <%s>; exitting!\n", &com[1]);
					return EW_FAILURE;
				}
				continue;
			}

			/* Process anything else as a command 
			************************************/
	/*0*/ 	if (k_its ("MyModName")) 
			{
				if ((str = k_str ()) != NULL)
				{
					strcpy (MyModName, str);
					init[0] = 1;
				}
			}
	/*1*/	else if (k_its ("RingName")) 
			{
				if ((str = k_str ()) != NULL)
				{
					strcpy (RingName, str);
					init[1] = 1;
				}
			}
	/*2*/	else if (k_its ("HeartBeatInt")) 
			{
				HeartBeatInterval = k_long ();
				init[2] = 1;
			}
	/*3*/	else if (k_its ("LogFile"))
			{
				LogSwitch = k_int();
				init[3] = 1;
			}

			/* Enter installation & module types to get
			 *******************************************/
	/*4*/	else if (k_its ("GetAlarmsFrom")) 
			{
				if (nLogo >= MAXLOGO) 
				{
					logit ("e", 
						"Too many <GetAlarmsFrom> commands in <%s>; "
						"max=%d; exiting!\n", configfile, (int) MAXLOGO);
					return EW_FAILURE;
				}
				if ((str = k_str())) 
				{
					if (GetInst (str, &GetLogo[nLogo].instid) != 0) 
					{
						logit ("e", "Invalid installation name <%s> in "
										"<GetAlarmsFrom> cmd; exiting!\n", str);
						return EW_FAILURE;
					}
                }
				if ((str = k_str())) 
				{
					if (GetModId (str, &GetLogo[nLogo].mod) != 0) 
					{
						logit ("e", 
							"Invalid module name <%s> in <GetAlarmsFrom> "
							"cmd; exiting!\n", str);
						return EW_FAILURE;
					}
				}

				/* Will always fetch TYPE_EMAIL_MSG */
				if (GetType ("TYPE_EMAIL_MSG", &GetLogo[nLogo].type) != 0) 
				{
					logit ("e", 
							"Invalid msgtype <%s> in <GetAlarmsFrom> "
							"cmd; exiting!\n", "TYPE_EMAIL_MSG");
					return EW_FAILURE;
				}

				nLogo++;
				init[4] = 1;
			}
	/*5*/	else if (k_its ("DBuser")) 
			{
				if ((str = k_str ()) != NULL)
				{
					strcpy (DBuser, str);
					init[5] = 1;
				}
			}
	/*6*/	else if (k_its ("DBpassword")) 
			{
				if ((str = k_str ()) != NULL)
				{
					strcpy (DBpassword, str);
					init[6] = 1;
				}
			}
	/*7*/	else if (k_its ("DBservice")) 
			{
				if ((str = k_str ()) != NULL)
				{
					strcpy (DBservice, str);
					init[7] = 1;
				}
			}

	/*NR*/	else if (k_its ("Subject")) 
			{
				if ((str = k_str ()) != NULL)
				{
					strcpy (Subject, str);
				}
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
					"Bad <%s> command in <%s>; exitting!\n",
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
		logit ("e", "email_sender: ERROR, no ");
		if (!init[0])  logit ("e", "<MyModName> "        );
		if (!init[1])  logit ("e", "<RingName> "          );
		if (!init[2])  logit ("e", "<HeartBeatInt> "     );
		if (!init[3])  logit ("e", "<LogFile> "     );
		if (!init[4])  logit ("e", "<GetAlarmsFrom> "     );
		if (!init[5])  logit ("e", "<DBuser> "     );
		if (!init[6])  logit ("e", "<DBpassword> "     );
		if (!init[7])  logit ("e", "<DBservice> "     );

		logit ("e", "command(s) in <%s>; exitting!\n", configfile);
		return EW_FAILURE;
	}

	return EW_SUCCESS;
}

/******************************************************************************
 *  email_lookup( )   Look up important info from earthworm.h tables       *
 ******************************************************************************/
static int email_lookup (void)
{

	/* Look up keys to shared memory regions
	*************************************/
	if ((RingKey = GetKey (RingName)) == -1) 
	{
		fprintf (stderr, "Invalid ring name <%s>; exitting!\n", RingName);
		return EW_FAILURE;
	}

	/* Look up installations of interest
	*********************************/
	if (GetLocalInst (&InstId) != 0) 
	{
		fprintf (stderr, "error getting local installation id; exitting!\n");
		return EW_FAILURE;
	}


	if (GetInst ("INST_WILDCARD", &InstWild ) != 0) 
	{ 
		fprintf (stderr, "error getting wildcard installation id; exitting!\n");
		return EW_FAILURE;
	}

	/* Look up modules of interest
	******************************/
	if (GetModId (MyModName, &MyModId) != 0) 
	{
		fprintf (stderr, "Invalid module name <%s>; exitting!\n", MyModName);
		return EW_FAILURE;
	}

	if (GetModId ("MOD_WILDCARD", &ModWild) != 0) 
	{
		fprintf (stderr, "Invalid module name <MOD_WILDCARD>; exitting!\n");
		return EW_FAILURE;
	}

	/* Look up message types of interest
	*********************************/
	if (GetType ("TYPE_HEARTBEAT", &TypeHeartBeat) != 0) 
	{
		fprintf (stderr, "Invalid message type <TYPE_HEARTBEAT>; exitting!\n");
		return EW_FAILURE;
	}

	if (GetType ("TYPE_ERROR", &TypeError) != 0) 
	{
		fprintf (stderr, "Invalid message type <TYPE_ERROR>; exitting!\n");
		return EW_FAILURE;
	}

	if (GetType ("TYPE_EMAIL_MSG", &TypeEmail) != 0) 
	{
		fprintf (stderr, "Invalid message type <TYPE_EMAIL_MSG>; exitting!\n");
		return EW_FAILURE;
	}

	return EW_SUCCESS;

} 

/******************************************************************************
 * email_status() builds a heartbeat or error message & puts it into       *
 *                   shared memory.  Writes errors to log file & screen.      *
 ******************************************************************************/
static void email_status( unsigned char type, short ierr, char *note )
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
	if (tport_putmsg (&Region, &logo, size, msg) != PUT_OK)
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
			res = tport_getmsg (&Region, GetLogo, nLogo, &reclogo, 
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
			email_status (TypeError, ERR_TOOBIG, errText);
		}
		else if (res == GET_MISS)
		{
			sprintf (errText, "missed msg(s) i%d m%d t%d in %s",
					(int) reclogo.instid, (int) reclogo.mod, 
					(int)reclogo.type, RingName);
			email_status (TypeError, ERR_MISSMSG, errText);
		}
		else if (res == GET_NOTRACK)
		{
			sprintf (errText, "no tracking for logo i%d m%d t%d in %s",
					(int) reclogo.instid, (int) reclogo.mod, 
					(int)reclogo.type, RingName);
			email_status (TypeError, ERR_NOTRACK, errText);
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
				email_status (TypeError, ERR_QUEUE, errText);
				MessageStackerStatus = -1; /* file a complaint to the main thread */
				KillSelfThread (); /* main thread will restart us */
				exit (-1);
			}
			if (ret == -1)
			{
				sprintf (errText, 
					"queue cannot allocate memory. Lost message.");
				email_status (TypeError, ERR_QUEUE, errText);
			}
			if (ret == -3) 
			{
				sprintf (errText, "Queue full. Message lost.");
				email_status (TypeError, ERR_QUEUE, errText);
			}
		} /* problem from enqueue */

	} /* while (1) */

}



/********************** Message Processing Thread ****************
 ******************************************************************/
thr_ret		Processor (void *dummy)
{

	char         	msg[2*ALARM_MSG_SIZE]; /* "raw" retrieved message */
	char         	Address[1][60]; 
	char         	Server[2*ALARM_MSG_SIZE]; 
	char         	MsgText[MAX_EMAIL_MSG_SIZE];
	char         	*tmp2;
	char         	*tmp3;
	int				    ret, idEvent;
	int				    gotMsg;
	long			    msg_len;
	MSG_LOGO      reclogo;           /* logo of retrieved message */
	EWDB_AlarmAuditStruct	Audit;


	/* Tell the main thread we're ok */
	ProcessorStatus = 0;

	while (1)
	{

topOfLoop:
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

		msg[msg_len] = '\0';

		if (Debug > 0)
			logit ("t", "Got a message of length %d\n", msg_len);

		/* Process the message 
	 	 **********************/
		/* Decode the message */

		/* Get Address */
		tmp2 = msg;
		tmp3 = strchr (tmp2, '\n');
		*tmp3 = '\0';
		strcpy (Address[0], tmp2);

		/* Get Server */
		tmp2 = tmp3 + 1;
		tmp3 = strchr (tmp2, '\n');
		*tmp3 = '\0';
		strcpy (Server, tmp2);

		/* Get idEvent */
		tmp2 = tmp3 + 1;
		tmp3 = strchr (tmp2, '\n');
		*tmp3 = '\0';
		idEvent = atoi (tmp2);

		/* Get idAudit */
		tmp2 = tmp3 + 1;
		tmp3 = strchr (tmp2, '\n');
		*tmp3 = '\0';
		Audit.idAudit = atoi (tmp2);


		tmp2 = tmp3 + 1;
		strcpy (MsgText, tmp2);

		if (strcmp (Subject, "Not Set") == 0)
			sprintf (Subject, "%d", idEvent);

 
		if (Debug > 0)
			logit ("t", "Sending message to <%s>\n", Address);

		/* Send the message */
		if (SendMail (Address, 1, "/usr/ucb/mail", Subject, MsgText, NULL,
							NULL, Server) != 0)
		{
			logit ("e", "Call to SendMail failed -- alarm NOT delivered.\n");
		}
		else
		{
			if (Debug > 0)
				logit ("t", "Updating Audit for idAudit=%d\n", Audit.idAudit);

			/* insert audit info into the database */
			Audit.tAlarmDeclared = 0.0;
			Audit.tAlarmExecuted = (double) time (NULL);
			if (ewdb_api_CreateAlarmAudit (&Audit) != EWDB_RETURN_SUCCESS)
			{
				logit ("", "Call to ewdb_api_CreateAlarmAudit failed.\n");
				goto topOfLoop;
			}

			if (Debug > 0)
				logit ("t", "Done processing message\n");

		}

	} /* infinite loop */

}



