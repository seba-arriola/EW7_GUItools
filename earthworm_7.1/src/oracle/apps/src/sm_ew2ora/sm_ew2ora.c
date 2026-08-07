/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: sm_ew2ora.c,v 1.19 2003/06/09 19:55:06 lombard Exp $
 *
 *    Revision history:
 *     $Log: sm_ew2ora.c,v $
 *     Revision 1.19  2003/06/09 19:55:06  lombard
 *     Changed to handle new version of rd_strongmotionII which can handle multiple
 *     channels in a single message.
 *
 *     Revision 1.18  2003/03/13 18:28:56  dietz
 *     Added 2 new Earthworm error codes to distinguish between the
 *     reasons that the SM data insert into the DBMS failed.
 *     New codes are: ERR_UNKNOWNCHAN and ERR_CREATEEVENT.
 *     Previously, all errors were reported at ERR_DBMSPUT
 *
 *     Revision 1.17  2002/05/01 19:59:51  dietz
 *     Will now send error msg if it cannot put SM data in the DBMS because
 *     the channel is not in the DBMS.  Changed to use tport_copyfrom instead
 *     of tport_getmsg. Moved logit_init such that configfile errors can be
 *     logged. Changed fprintf(stderr... to logit("e"...
 *
 *     Revision 1.16  2002/05/01 16:04:08  lucky
 *     *** empty log message ***
 *
 *     Revision 1.15  2001/10/02 16:16:32  dietz
 *     Changed to send first heartbeat before trying to connect to Oracle
 *
 *     Revision 1.14  2001/07/26 17:03:58  davidk
 *     Changed code to handle changes in ewdb_api_PutSMMessage(), so that
 *     we only complain if there is a failure, and we only say that
 *     the message was successfully inserted on success, and on warning,
 *     we warn that the message was not inserted, because we didn't have
 *     a channel or else a lat/lon for the channel from which the message came.
 *
 *     Revision 1.13  2001/07/19 17:41:44  lucky
 *     Fixed the reading of the source logo -- we were reading the entire thing which may
 *     consist of more than one logo. We are only interested in the first originator
 *     in order to get the corresponding eventID
 *
 *     Revision 1.12  2001/07/16 21:00:33  lucky
 *     Increase size of logit buffer; added a couple of informational logit
 *     statements to tell us when a message was inserted.
 *
 *     Revision 1.11  2001/05/26 00:00:23  davidk
 *     Moved the call to ewdb_api_Init() from inside the while loop to above
 *     the loop, so that it is part of the setup code, instead of the per message
 *     code.
 *     ewdb_api_Init() should only be called once to setup the EWDB API environment,
 *     not once each message.  ewdb_api_Init() handles the environment, not the
 *     connection.
 *
 *     Revision 1.10  2001/05/15 02:15:48  davidk
 *     Moved functions around between the apps, DB API, and DB API INTERNAL
 *     levels.  Renamed functions and files.  Added support for amplitude
 *     magnitude types.  Reformatted makefiles.
 *
 *     Revision 1.9  2001/05/10 21:36:54  dietz
 *     Changed to shut down gracefully if the transport flag is
 *     set to TERMINATE or MyPid.
 *
 *     Revision 1.8  2001/04/11 23:37:59  dietz
 *     *** empty log message ***
 *
 *     Revision 1.7  2001/04/06 22:40:20  davidk
 *     converted from strong motion I to strong motion II.  Cleaned up
 *     a lot of left over includes and defines that hadn't been needed
 *     in awhile if ever, and added (as part of strong motion II) support
 *     for event - strongmotion associations.
 *
 *     Revision 1.6  2001/02/28 17:29:10  lucky
 *     Massive schema redesign and cleanup.
 *
 *     Revision 1.5  2000/10/24 23:31:53  dietz
 *     Changed to log raw SM msgs and to report error to statmgr it
 *     DBMS insert fails.  Continues running after failure (used to exit).
 *
 *     Revision 1.4  2000/10/03 23:21:54  dietz
 *     *** empty log message ***
 *
 *     Revision 1.3  2000/10/03 21:56:32  dietz
 *     Added logging of cumulative #msgs processed
 *
 *     Revision 1.2  2000/10/03 00:19:47  dietz
 *     added a bit of logging
 *
 *     Revision 1.1  2000/05/31 17:32:43  lucky
 *     Initial revision
 *
 *     Revision 1.2  2000/02/15 20:07:41  lucky
 *     *** empty log message ***
 *
 *     Revision 1.1  1999/11/09 16:38:01  lucky
 *     Initial revision
 *
 *
 */

/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW API FORMATTED COMMENT
TYPE LIBRARY

LIBRARY  EWDB_APPLICATIONS

SUB_LIBRARY sm_ew2ora

LOCATION THIS_FILE

LINK_NAME Sample Config File
LINK sm_ew2ora.d.txt

DESCRIPTION SM_ew2ora is an earthworm module that
reads TYPE_STRONGMOTIONII messages from an 
earthworm ring, and writes them to an EW DBMS.
<br><br>
Initial version Tue Nov  9 09:27:39 MST 1999   Lucky Vidmar
<br><br>
Converted from TYPE_STRONGMOTION to TYPE_STRONGMOTIONII
on 2001/04/04 by DavidK.
<br><br>

*************************************************
************************************************/


/*
 * sm_ew2ora.c:  
 *
 *
 *     This module looks for TYPE_STRONGMOTIONII messages on the
 *   InRing. When it finds one with a desired logo, it picks 
 *   it up, parses the message, and fills the SM_INFO structure.
 *
 *     Then the real fun starts. We initialize the Oracle API,  
 *   and call an API routine to insert the SM_INFO into the
 *   database.
 *
 *     Initial version (sm_ew2ora.c) Tue Nov  9 09:27:39 MST 1999   Lucky Vidmar
 *
 *     Converted from TYPE_STRONGMOTION to TYPE_STRONGMOTIONII
 *      on 2001/04/04 by DavidK.
 *
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <kom.h>
#include <transport.h>

#include <rw_strongmotionII.h>
#include <ewdb_ora_api.h>

static    SHM_INFO  InRegion;   /* shared memory region to use for i/o    */
#define   NUM_COMMANDS 	8 	/* how many required commands in the config file */

#define   MAXLOGO   5
MSG_LOGO  GetLogo[MAXLOGO];     /* array for requesting module,type,instid */
short     nLogo;

#define SCN_INCREMENT   10   /* how many more are allocated each time we run out */
#define STATION_LEN      6   /* max string-length of station code     */
#define CHAN_LEN         8   /* max string-length of component code   */
#define NETWORK_LEN      8   /* max string-length of network code     */
#define LOGIT_LEN      1024  /* buffer length handed to logit_init    */

#define	DBMAXWORD	30


/* Things to read or derive from configuration file
 **************************************************/
static char InRingName[20];          /* name of transport ring for i/o    */
static char MyModName[32];           /* this module's given name          */
static char MyProgName[256];         /* this module's program name        */
static int  LogSwitch;               /* 0 if no logfile should be written */
static long HeartBeatInterval;       /* seconds between heartbeats        */
static int  Debug = 0;               /* Debug flag, 0-no debug            */ 
                                     /*             1-basic debug         */
                                     /*             2-super debug         */
static int  LogStrongMotion = 0;     /* if non-zero, log SM messages      */             
static char DBservice[DBMAXWORD+1];  /* DBMS instance to interact with    */
static char DBuser[DBMAXWORD+1];     /* UserId to connect to database as  */
static char DBpassword[DBMAXWORD+1]; /* Password to datasource            */


/* Things to look up in the earthworm.h tables with getutil.c functions
 **********************************************************************/
static long          InKey;         /* key of transport ring for i/o     */
static unsigned char InstId;        /* local installation id             */
static unsigned char MyModId;       /* Module Id for this program        */
static unsigned char TypeHeartBeat; 
static unsigned char TypeSM;
static unsigned char TypeError;
static unsigned char InstWild;
static unsigned char ModWild;

/* Error messages used by sm_ew2ora 
 *********************************/
#define  ERR_MISSMSG       0   /* message missed in transport ring       */
#define  ERR_TOOBIG        1   /* retreived msg too large for buffer     */
#define  ERR_NOTRACK       2   /* msg retreived; tracking limit exceeded */
#define  ERR_DBMSPUT       3   /* trouble putting info into DBMS         */

static char  errText[256];     /* string for log/error messages          */

pid_t MyPid;	       /* Hold our process ID to be sent with heartbeats */


#define	BUF_SIZE  65000  /* max size of a strong motion message */

/* Functions in this source file 
 *******************************/
static	int  	sm_ew2ora_config (char *);
static	int  	sm_ew2ora_lookup (void);
static	void  	sm_ew2ora_status (unsigned char, short, char *);
static	void  	sm_ew2ora_logmsg (char *, int );
thr_ret         MessageStacker (void *);
thr_ret         Processor (void *);



main (int argc, char **argv)
{
   long      timeNow;         /* current time                  */ 
   long      timeLastBeat;    /* time last heartbeat was sent  */
   long      recsize;         /* size of retrieved message     */
   MSG_LOGO  reclogo;         /* logo of retrieved message     */
   int       ret;
   int       exitstat=0;      /* exit status */
   int       ntotal=0;        /* cumulative # of msgs processed */
   char      msg[BUF_SIZE];
   char      *msgP;
   SM_INFO   EwSmStruct;	
   EWDBid    idEvent;

/* Check command line arguments 
 ******************************/
   if( argc != 2 )
   {
      fprintf (stderr, "Usage: sm_ew2ora <configfile>\n");
      return EW_FAILURE;
   }

/* To be used in loging functions
 ********************************/
   if( get_prog_name (argv[0], MyProgName) != EW_SUCCESS )
   {
      fprintf (stderr, "sm_ew2ora: Call to get_prog_name failed.\n");
      return EW_FAILURE;
   }		

/* Read the configuration file(s)
 ********************************/
   if( sm_ew2ora_config(argv[1]) != EW_SUCCESS )
   {
      fprintf (stderr, "%s: Call to sm_ew2ora_config failed \n",
               MyProgName);
      return EW_FAILURE;
   }

/* Look up important info from earthworm.h tables
 ************************************************/
   if( sm_ew2ora_lookup() != EW_SUCCESS )
   {
      fprintf (stderr, "%s(%s): Call to sm_ew2ora_lookup failed \n",
               MyProgName, MyModName);
      return EW_FAILURE;
   }

/* Initialize name of log-file & open it 
 ***************************************/
   logit_init( argv[1], (short) MyModId, LOGIT_LEN, LogSwitch );
   logit( "" , "%s(%s): Read command file <%s>\n", 
          MyProgName, MyModName, argv[1] );

/* Get our process ID
 ********************/
   if( (MyPid = getpid ()) == -1 )
   {
      logit ("e", "%s(%s): Call to getpid failed; exiting.\n",
             MyProgName, MyModName);
      return EW_FAILURE;
   }

/* Attach to Input shared memory ring 
 ************************************/
   tport_attach( &InRegion, InKey );
   logit( "", "%s(%s): Attached to public memory region %s: %d\n", 
          MyProgName, MyModName, InRingName, InKey );

/* Beat heart first time, before trying to connect to Oracle
 ***********************************************************/
   sm_ew2ora_status( TypeHeartBeat, 0, "" ); 
   timeLastBeat = time (&timeNow);

/* Flush the incoming transport ring 
 ***********************************/
   while( tport_getmsg (&InRegion, GetLogo, nLogo, &reclogo,
          &recsize, msg, (BUF_SIZE)) != GET_NONE );

/* Intialize the oracle API
 ***************************/
   if((ret=ewdb_api_Init( DBuser, DBpassword, DBservice )) == EWDB_RETURN_FAILURE)
   {
      logit( "et", "Call to ewdb_api_Init failed; Returning!"
             "ret = %d\n", ret );
      exitstat = EW_FAILURE;
      return(EW_FAILURE);
   }
		
   if( Debug > 1 ) logit( "t", "Connected to Oracle.\n" );
		

/*--------------------- setup done; start main loop -------------------------*/

   while( tport_getflag(&InRegion) != TERMINATE  &&
          tport_getflag(&InRegion) != MyPid )
   {

   /* send sm_ew2ora' heartbeat
    ***************************/
      if( time(&timeNow)-timeLastBeat >= HeartBeatInterval ) 
      {
         timeLastBeat = timeNow;
         sm_ew2ora_status( TypeHeartBeat, 0, "" ); 
      }
	
   /* Get a message from transport ring
    ************************************/
      ret = tport_getmsg( &InRegion, GetLogo, nLogo, &reclogo,
                          &recsize, msg, (BUF_SIZE)-1 );	

      if( ret == GET_NONE )        /* no new messages */
      {
         sleep_ew( 200 );
         continue;
      }
      else if( ret == GET_OK )     /* got a good one! */
      {
         if(Debug>1) logit( "t","Got new message!\n" );
      }
      else if( ret == GET_MISS )    /* got a msg, but missed some */
      {
         sprintf( errText,"Missed msg(s)  i%u m%u t%u  %s.",
                  reclogo.instid, reclogo.mod, reclogo.type, InRingName );
         sm_ew2ora_status( TypeError, ERR_MISSMSG, errText );
      }
      else if( ret == GET_NOTRACK ) /* got a msg, but can't tell */
      {                             /* if any were missed        */
         sprintf( errText,"Msg received (i%u m%u t%u); transport.h NTRACK_GET exceeded",
                  reclogo.instid, reclogo.mod, reclogo.type );
         sm_ew2ora_status( TypeError, ERR_NOTRACK, errText );
      }
      else if( ret == GET_TOOBIG ) /* no msg, next one was too big */
      {  
         sprintf( errText, "Retrieved size[%ld] (i%u m%u t%u) too big for msg[%d]",
                  recsize, reclogo.instid, reclogo.mod, reclogo.type,
                  sizeof(msg)-1 );
         sm_ew2ora_status( TypeError, ERR_TOOBIG, errText );
         continue;
      }
      else 
      {
         logit( "t", "%s(%s): Unknown return code (%d) from tport_getmsg.\n",
                MyProgName, MyModName, ret );
         continue;
      }

   /* Process a new message
    ***********************/
      msg[recsize] = '\0';
      ntotal++;

      if( LogStrongMotion ) 
      {
         logit( "et", "SM Message %d read from ring (raw format):\n", ntotal );
         sm_ew2ora_logmsg( msg, recsize );
      }

   /* Fill the SM_DATA struct from the received message
    ***************************************************/
      msgP = msg;
      while ((ret = rd_strongmotionII( &msgP, &EwSmStruct, 1 )) != 0 ) {
	  if (ret < 0) {
	      logit( "et", "SM Message %d call to rd_strongmotion failed!\n",
		     ntotal );
	      continue;
	  }
	  
	  if( Debug>1 ) 
	  {
	      logit( "et", "SM Message %d parsed:\n", ntotal );
	      log_strongmotionII( &EwSmStruct );
	  }
	  
	  /* Get an idEvent for the message if applicable 
           **********************************************/
	  if(EwSmStruct.qauthor[0] == 0x00)
	  {
	      idEvent = 0;
	  }
	  else
	  {
	      EWDB_EventStruct Event;
	      
	      memset(&Event, 0 ,sizeof(Event));

	      /* 
	       * NOTE: rd_strongmotionII fills qauthor with the entire LOGO,
	       * which may consist of one or more logos. In order to get the 
	       * correct EventID we must use the first LOGO which should belong 
	       * to the original author of the event, i.e. eqproc or the like.
	       */
	      /* DK CLEANUP:  This latest fix has a problem.  We should be reading 
		 szSource till the first
		 colon(':'), not just the first 9 characters. */
	      
	      strcpy(Event.szSourceEventID, EwSmStruct.qid);
	      strncpy(Event.szSource, EwSmStruct.qauthor, 9);
	      Event.szSource[9] = '\0';
	      
	      ret = ewdb_api_CreateEvent(&Event);
	      if(ret != EWDB_RETURN_SUCCESS)
	      {
		  logit("et", "sm_ew2ora:  Could not create event in DB for "
			"author(%s), author's eventid(%s).",
			EwSmStruct.qauthor, EwSmStruct.qid);
		  
		  sprintf(errText, "sm_ew2ora:  Could not create event in DB for "
			  "author(%s), author's eventid(%s).",
			  EwSmStruct.qauthor, EwSmStruct.qid);
		  
		  sm_ew2ora_status( TypeError, ERR_DBMSPUT, errText );
		  continue;
	      }
	      else
	      {
		  idEvent = Event.idEvent;
	      }
	  }  /* end else (qauthor is not blank) */


	  logit ("t", "Inserting strong motion message for event %d.\n", idEvent);

	  /* Insert the message into DBMS
           ******************************/
	  if((ret = ewdb_api_PutSMMessage( &EwSmStruct, idEvent )) == EWDB_RETURN_FAILURE)
	  {
	      logit( "et", "SM Message %d call to PutSMMessage failed; "
		     "ret = %d\n", ntotal, ret );
	      sprintf( errText, "Failure inserting SM Message %d into DBMS\n",
		       ntotal );
	      sm_ew2ora_status( TypeError, ERR_DBMSPUT, errText );
	      continue;
	  }
	  else if(ret == EWDB_RETURN_SUCCESS)
	  {
	      logit("t", "SM Message loaded.\n");
	      
	      if( LogStrongMotion || Debug > 1 )
		  logit( "et", "SM Message %d loaded in DBMS\n", ntotal );
	  }
	  else
	  {
	      logit("t", "SM Message was not loaded, because channel not found "
		    "or Lat/Lon not available.\n");
	  }
      } /* while(rd_strongmotionII...) */
      
   } /* while(tport_getflag) */

/*-------------------------- end of main loop -------------------------*/

/* Detach from shared memory 
 ***************************/
   tport_detach( &InRegion );

/* Write appropriate termination msg to log file 
 ***********************************************/
   if( exitstat==EW_FAILURE ) { 
      logit( "t", "%s(%s): Termination on error condition; exiting!\n",
              MyProgName, MyModName );
   }
   else {
      logit( "t", "%s(%s): Termination requested; exiting!\n",
              MyProgName, MyModName );
   }

   fflush( stdout );
   return( exitstat );
}

/******************************************************************************
 *  sm_ew2ora_config() processes command file(s) using kom.c functions;        *
 *                    exits if any errors are encountered.                    *
 ******************************************************************************/
static int sm_ew2ora_config (char *configfile)
{
   char  init[NUM_COMMANDS]; /* init flags, one byte for each required command */
   int   nmiss;              /* number of required commands that were missed   */
   char *com;
   char *str;
   int   nfiles;
   int   success;
   int   i;


/* Set to zero one init flag for each required command 
 *****************************************************/   
   for (i = 0; i < NUM_COMMANDS; i++)  init[i] = 0;
   nLogo = 0;

/* Open the main configuration file 
 **********************************/
   nfiles = k_open (configfile); 
   if (nfiles == 0) 
   {
      fprintf (stderr,
               "sm_ew2ora: Error opening command file <%s>; exiting!\n", 
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
         if (!com)          continue;
         if (com[0] == '#') continue;

      /* Open a nested configuration file 
       **********************************/
         if (com[0] == '@') 
         {
            success = nfiles + 1;
            nfiles  = k_open (&com[1]);
            if (nfiles != success) 
            {
               fprintf(stderr, 
                      "sm_ew2ora: Error opening command file <%s>; "
                      "exiting!\n", &com[1]);
               return EW_FAILURE;
            }
            continue;
         }

      /* Process anything else as a command 
       ************************************/
 /*0*/   if (k_its ("MyModId")) 
         {
            if ((str = k_str ()) != NULL)
            {
               strcpy (MyModName, str);
               init[0] = 1;
            }
         }
 /*1*/   else if (k_its ("InRing")) 
         {
            if ((str = k_str ()) != NULL)
            {
               strcpy (InRingName, str);
               init[1] = 1;
            }
         }
 /*2*/   else if (k_its ("HeartBeatInterval")) 
         {
            HeartBeatInterval = k_long ();
            init[2] = 1;
         }
 /*3*/   else if (k_its ("LogFile"))
         {
            LogSwitch = k_int();
            init[3] = 1;
         }

      /* Enter installation & module types to get
       *******************************************/
 /*4*/   else if (k_its ("GetMsgsFrom")) 
         {
            if (nLogo >= MAXLOGO) 
            {
               fprintf (stderr, "sm_ew2ora: Too many <GetMsgLogo> commands in <%s>; "
                        "; max=%d; exiting!\n", configfile, (int) MAXLOGO);
               return EW_FAILURE;
            }
            if ((str = k_str())) 
            {
               if (GetInst (str, &GetLogo[nLogo].instid) != 0) 
               {
                  fprintf (stderr, "sm_ew2ora: Invalid installation name <%s> in "
                           "<GetMsgsFrom> cmd; exiting!\n", str);
                  return EW_FAILURE;
               }
            }
            if ((str = k_str())) 
            {
               if (GetModId (str, &GetLogo[nLogo].mod) != 0) 
               {
                  fprintf (stderr, "sm_ew2ora: Invalid module name <%s> in"
                           " <GetMsgsFrom> cmd; exiting!\n", str);
                  return EW_FAILURE;
               }
            }
         /* We'll always fetch strongmotion messages */
            if (GetType ("TYPE_STRONGMOTIONII", &GetLogo[nLogo].type) != 0) 
            {
               fprintf (stderr, "sm_ew2ora: Can't convert msgtype <TYPE_STRONGMOTIONII> "
                        "in <GetMsgsFrom> cmd; exiting!\n");
               return EW_FAILURE;
            }
            nLogo++;
            init[4] = 1;
         }

 /*5*/   else if (k_its ("DBservice")) 
         {
            if ((str = k_str()) != NULL)
            {
               if( strlen(str) > DBMAXWORD ) {
                  fprintf( stderr, "sm_ew2ora: DBservice name too long; "
                          "max = %d char; exiting!\n", DBMAXWORD );
                  return EW_FAILURE;
               }
               strcpy (DBservice, str);
               init[5] = 1;
            }
         }

 /*6*/   else if (k_its ("DBuser")) 
         {
            if ((str = k_str ()) != NULL)
            {
               if( strlen(str) > DBMAXWORD ) {
                  fprintf( stderr, "sm_ew2ora: DBuser name too long; "
                          "max = %d char; exiting!\n", DBMAXWORD );
                  return EW_FAILURE;
               }
               strcpy (DBuser, str);
               init[6] = 1;
            }
         }

 /*7*/   else if (k_its ("DBpassword")) 
         {
            if ((str = k_str ()) != NULL)
            {
               if( strlen(str) > DBMAXWORD ) {
                  fprintf( stderr, "sm_ew2ora: DBpassword too long; "
                          "max = %d char; exiting!\n", DBMAXWORD );
                  return EW_FAILURE;
               }
               strcpy (DBpassword, str);
               init[7] = 1;
            }
         }

/*NR*/   else if (k_its ("Debug"))
         {
            Debug = k_int();
         }

/*NR*/   else if (k_its ("LogStrongMotion"))
         {
            LogStrongMotion = k_int();
         }

      /* Unknown command
       *****************/ 
         else 
         {
            fprintf (stderr, "sm_ew2ora: <%s> Unknown command in <%s>.\n", 
                     com, configfile);
            continue;
         }

      /* See if there were any errors processing the command 
       *****************************************************/
         if (k_err ()) 
         {
            fprintf (stderr, 
                     "sm_ew2ora: Bad <%s> command in <%s>; exiting!\n",
                     com, configfile);
            return EW_FAILURE;
         }

      } /** while k_rd() **/

      nfiles = k_close ();

   } /** while nfiles **/

/* After all files are closed, check init flags for missed commands
 ******************************************************************/
   nmiss = 0;
   for (i = 0; i < NUM_COMMANDS; i++) if (!init[i]) nmiss++;

   if (nmiss) 
   {
      fprintf (stderr, "sm_ew2ora: ERROR, no ");
      if (!init[0])  fprintf (stderr, "<MyModId> "        );
      if (!init[1])  fprintf (stderr, "<InRing> "          );
      if (!init[2])  fprintf (stderr, "<HeartBeatInterval> "     );
      if (!init[3])  fprintf (stderr, "<LogFile> "     );
      if (!init[4])  fprintf (stderr, "<GetMsgsFrom> "     );
      if (!init[5])  fprintf (stderr, "<DBservice> "     );
      if (!init[6])  fprintf (stderr, "<DBuser> "     );
      if (!init[7])  fprintf (stderr, "<DBpassword> "     );

      fprintf (stderr, "command(s) in <%s>; exiting!\n", configfile);
      return EW_FAILURE;
   }

   return EW_SUCCESS;
}

/******************************************************************************
 *  sm_ew2ora_lookup( )   Look up important info from earthworm.h tables      *
 ******************************************************************************/
static int sm_ew2ora_lookup( void )
{

/* Look up keys to shared memory regions
 *************************************/
   if ((InKey = GetKey (InRingName)) == -1) 
   {
      fprintf (stderr,
               "sm_ew2ora:  Invalid ring name <%s>; exiting!\n", InRingName);
      return EW_FAILURE;
   }

/* Look up installations of interest
 *********************************/
   if (GetLocalInst (&InstId) != 0) 
   {
      fprintf (stderr, 
               "sm_ew2ora: error getting local installation id; exiting!\n");
      return EW_FAILURE;
   }


   if (GetInst ("INST_WILDCARD", &InstWild ) != 0) 
   { 
      fprintf (stderr, 
               "sm_ew2ora: error getting wildcard installation id; exiting!\n");
      return EW_FAILURE;
   }

/* Look up modules of interest
 ******************************/
   if (GetModId (MyModName, &MyModId) != 0) 
   {
      fprintf (stderr, 
               "sm_ew2ora: Invalid module name <%s>; exiting!\n", MyModName);
      return EW_FAILURE;
   }

   if (GetModId ("MOD_WILDCARD", &ModWild) != 0) 
   {
      fprintf (stderr, 
               "sm_ew2ora: Invalid module name <MOD_WILDCARD>; exiting!\n");
      return EW_FAILURE;
   }

	
/* Look up message types of interest
 *********************************/
   if (GetType ("TYPE_HEARTBEAT", &TypeHeartBeat) != 0) 
   {
      fprintf (stderr, 
               "sm_ew2ora: Invalid message type <TYPE_HEARTBEAT>; exiting!\n");
   return EW_FAILURE;
   }

   if (GetType ("TYPE_ERROR", &TypeError) != 0) 
   {
      fprintf (stderr, 
               "sm_ew2ora: Invalid message type <TYPE_ERROR>; exiting!\n");
      return EW_FAILURE;
   }

   if (GetType ("TYPE_STRONGMOTIONII", &TypeSM) != 0) 
   {
      fprintf (stderr, 
               "sm_ew2ora: Invalid message type <TYPE_TRACEBUF>; exiting!\n");
      return EW_FAILURE;
   }

   return EW_SUCCESS;
} 

/******************************************************************************
 * sm_ew2ora_status() builds a heartbeat or error message & puts it into      *
 *                   shared memory.  Writes errors to log file & screen.      *
 ******************************************************************************/
static void sm_ew2ora_status( unsigned char type, short ierr, char *note )
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
   if (tport_putmsg (&InRegion, &logo, size, msg) != PUT_OK)
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
   return;
}


/******************************************************************************
 * sm_ew2ora_logmsg() logs a message in chunks that are small enough that     *
 *                    they won't overflow logit's buffer                      *
 ******************************************************************************/
void sm_ew2ora_logmsg(char *msg, int msglen )
{
   char  chunk[LOGIT_LEN];
   char *next     = msg;
   char *endmsg   = msg+msglen;
   int   chunklen = LOGIT_LEN-10;

   while( next < endmsg )
   {
      strncpy( chunk, next, chunklen );
      chunk[chunklen]='\0';
      logit( "", "%s", chunk );
      next += chunklen;
   }
   return;
}
