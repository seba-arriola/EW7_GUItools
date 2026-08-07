/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ringdup.c,v 1.9 2007/03/28 16:51:38 paulf Exp $
 *
 *    Revision history:
 *     $Log: ringdup.c,v $
 *     Revision 1.9  2007/03/28 16:51:38  paulf
 *     removed malloc.h and tweeked make for MACOSX
 *
 *     Revision 1.8  2007/02/26 14:23:17  paulf
 *     fixed long casting of time_t for heartbeat sprintf()
 *
 *     Revision 1.7  2007/02/23 05:35:53  stefan
 *     long to time_t
 *
 *     Revision 1.6  2004/05/27 16:45:56  dietz
 *     minor tweak
 *
 *     Revision 1.5  2002/03/22 18:43:43  lucky
 *     Fixed the configure routine which had unmatched '()' and '"' causing it not
 *     to compiled
 *
 *     Revision 1.4  2002/03/20 17:32:17  patton
 *     Made logit changes.
 *     JMP 03/20/2002
 *
 *     Revision 1.3  2001/05/08 22:43:43  dietz
 *     Changed to shut down gracefully if the transport flag is
 *     set to TERMINATE or MyPid.
 *
 *     Revision 1.2  2000/07/24 19:07:06  lucky
 *     Implemented global limits to module, installation, ring, and message type strings.
 *
 *     Revision 1.1  2000/05/24 17:53:07  lucky
 *     Initial revision
 *
 *     Revision 1.1  2000/03/29 16:16:00  whitmore
 *     Initial revision
 *
 *
 *
 */

/*
 *   ringdup.c
 *
 *   Program to read messages (with user given logos) from one ring
 *   and deposit them in another.  This is mainly copied from
 *   export.
 *
 *   Whitmore - 3/21/00
 *
 *   Link ringdup's object file with various filter object files to
 *   create customized ringdup modules (as in export).  Dummy filter functions
 *   live in genericfilter.c.
 *
 *   Modified to re-send crippled message. Alex Nov12 99
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <math.h>
#include <time.h>
#include <errno.h>
#include <signal.h>
#include <earthworm.h>
#include <kom.h>
#include <transport.h>
#include <imp_exp_gen.h>
#include <mem_circ_queue.h>

#include "exportfilter.h"


/* Functions in this source file
 *******************************/
void  ringdup_config  ( char * );
void  ringdup_lookup  ( void );
void  ringdup_status  ( unsigned char, short, char * );
void  ringdup_free    ( void );

/* Thread things
 ***************/
#define THREAD_STACK 8192
static unsigned tidDup;              /* Dup thread id */
static unsigned tidStacker;          /* Thread moving messages from transport */
                                     /*   to queue */

#define MSGSTK_OFF    0              /* MessageStacker has not been started      */
#define MSGSTK_ALIVE  1              /* MessageStacker alive and well            */
#define MSGSTK_ERR   -1              /* MessageStacker encountered error quit    */
volatile int MessageStackerStatus = MSGSTK_OFF;

QUEUE OutQueue;              /* from queue.h, queue.c; sets up linked    */
                                     /*    list via malloc and free              */
thr_ret Dup( void * );
thr_ret MessageStacker( void * );    /* used to pass messages between main thread */
                                     /*   and Dup thread */
/* Message Buffers to be allocated
 *********************************/
static char *Rawmsg = NULL;          /* "raw" retrieved msg for main thread      */
static char *SSmsg = NULL;           /* Dup's incoming message buffer   */
static MSG_LOGO Logo;        /* logo of message to re-send */
static char *MSrawmsg = NULL;        /* MessageStacker's "raw" retrieved message */
static char *MSfilteredmsg = NULL;   /* MessageStacker's "filtered" message to   */
                                     /*    be sent to client                     */

/* Timers
   ******/
time_t now;        /* current time, used for timing heartbeats */
time_t MyLastBeat;         /* time of last local (into Earthworm) hearbeat */

extern int  errno;

static  SHM_INFO  InRegion;     /* shared memory region to use for input  */
static  SHM_INFO  OutRegion;    /* shared memory region to use for output */

#define   MAXLOGO   10
MSG_LOGO  GetLogo[MAXLOGO];     /* array for requesting module,type,instid */
short     nLogo;

char *Argv0;            /* pointer to executable name */
pid_t MyPid;        /* Our own pid, sent with heartbeat for restart purposes */

/* Things to read or derive from configuration file
 **************************************************/
static char    InRing[MAX_RING_STR];          /* name of transport ring for input  */
static char    OutRing[MAX_RING_STR];         /* name of transport ring for output */
static char    MyModName[MAX_MOD_STR];       /* speak as this module name/id      */
static int     LogSwitch;           /* 0 if no logfile should be written */
static int     HeartBeatInt;        /* seconds between heartbeats        */
static long    MaxMsgSize;          /* max size for input/output msgs    */
static int     RingSize;        /* max messages in output circular buffer       */

/* Things to look up in the earthworm.h tables with getutil.c functions
 **********************************************************************/
static long          InRingKey;     /* key of transport ring for input    */
static long          OutRingKey;    /* key of transport ring for output   */
static unsigned char InstId;        /* local installation id              */
static unsigned char MyModId;       /* Module Id for this program         */
static unsigned char TypeHeartBeat;
static unsigned char TypeError;

/* Error messages used by export
 ***********************************/
#define  ERR_MISSMSG       0   /* message missed in transport ring        */
#define  ERR_TOOBIG        1   /* retreived msg too large for buffer      */
#define  ERR_NOTRACK       2   /* msg retreived; tracking limit exceeded  */
#define  ERR_QUEUE         3   /* error queueing message for sending      */
static char  errText[256];     /* string for log/error messages           */

int main( int argc, char **argv )
{
/* Other variables: */
   int           res;
   long          recsize;   /* size of retrieved message             */
   MSG_LOGO      reclogo;   /* logo of retrieved message             */

   /* Check command line arguments
   ******************************/
   Argv0 = argv[0];
   if ( argc != 2 )
   {
      fprintf( stderr, "Usage: %s <configfile>\n", Argv0 );
      return( 0 );
   }

   /* Initialize name of log-file & open it
   ****************************************/
   logit_init( argv[1], 0, 512, 1 );

   /* Read the configuration file(s)
   ********************************/
   ringdup_config( argv[1] );
   logit( "et" , "%s(%s): Read command file <%s>\n",
           Argv0, MyModName, argv[1] );

   /* Look up important info from earthworm.h tables
   *************************************************/
   ringdup_lookup();

   /* Reinitialize the logging level
   *********************************/
   logit_init( argv[1], 0, 512, LogSwitch );

   /* Get our own Pid for restart purposes
   ***************************************/
   MyPid = getpid();
   if(MyPid == -1)
   {
      logit("e", "%s(%s): Cannot get pid; exiting!\n", Argv0, MyModName);
      return(0);
   }

   /* Initialize export filter
   ***************************/
   if( exportfilter_init() != 0 )
   {
      logit("e", "%s(%s): Error in exportfilter_init(); exiting!\n",
            Argv0, MyModName );
      return(-1);
   }

   /* Allocate space for input/output messages for all threads
   ***********************************************************/
   /* Buffer for main thread: */
   if ( ( Rawmsg = (char *) malloc(MaxMsgSize) ) ==  NULL )
   {
      logit( "e", "%s(%s): error allocating Rawmsg; exiting!\n",
             Argv0, MyModName );
      ringdup_free();
      return( -1 );
   }

   /* Buffers for Dup thread: */
   if ( ( SSmsg = (char *) malloc(MaxMsgSize) ) ==  NULL )
   {
      logit( "e", "%s(%s): error allocating SSmsg; exiting!\n",
              Argv0, MyModName );
      ringdup_free();
      return( -1 );
   }

   /* Buffers for the MessageStacker thread: */
   if ( ( MSrawmsg = (char *) malloc(MaxMsgSize) ) ==  NULL )
   {
      logit( "e", "%s(%s): error allocating MSrawmsg; exiting!\n",
             Argv0, MyModName );
      ringdup_free();
      return( -1 );
   }
   if ( ( MSfilteredmsg = (char *) malloc(MaxMsgSize) ) ==  NULL )
   {
      logit( "e", "%s(%s): error allocating MSfilteredmsg; exiting!\n",
              Argv0, MyModName );
      ringdup_free();
      return( -1 );
   }

   /* Create a Mutex to control access to queue
   ********************************************/
   CreateMutex_ew();

   /* Initialize the message queue
   *******************************/
   initqueue( &OutQueue, (unsigned long)RingSize,(unsigned long)MaxMsgSize );

   /* Attach to Input/Output shared memory ring
   ********************************************/
   tport_attach( &InRegion, InRingKey );
   tport_attach( &OutRegion, OutRingKey );

   /* step over all messages from transport ring
   *********************************************/
   /* As Lynn pointed out: if we're restarted by startstop after hanging,
      we should throw away any of our messages in the transport ring.
      Else we could end up re-sending a previously sent message, causing
      time to go backwards... */
   do
   {
     res = tport_getmsg( &InRegion, GetLogo, nLogo,
                         &reclogo, &recsize, Rawmsg, MaxMsgSize-1 );
   } while (res !=GET_NONE);

   /* One heartbeat to announce ourselves to statmgr
   ************************************************/
   ringdup_status( TypeHeartBeat, 0, "" );
   time(&MyLastBeat);


   /* Start the message stacking thread if it isn't already running.
    ****************************************************************/
   if (MessageStackerStatus != MSGSTK_ALIVE )
   {
     if ( StartThread(  MessageStacker, (unsigned)THREAD_STACK, &tidStacker ) == -1 )
     {
       logit( "e",
              "%s(%s): Error starting  MessageStacker thread; exiting!\n",
          Argv0, MyModName );
       tport_detach( &InRegion );
       tport_detach( &OutRegion );
       return( -1 );
     }
     MessageStackerStatus = MSGSTK_ALIVE;
   }

   /* Start the socket writing thread
   ***********************************/
   if ( StartThread(  Dup, (unsigned)THREAD_STACK, &tidDup ) == -1 )
   {
      logit( "e", "%s(%s): Error starting Dup thread; exiting!\n",
              Argv0, MyModName );
      tport_detach( &InRegion );
      tport_detach( &OutRegion );
      return( -1 );
   }

   /* Start main ringdup service loop
   **********************************/
   while( tport_getflag( &InRegion ) != TERMINATE  &&
          tport_getflag( &InRegion ) != MyPid         )
   {
     /* Beat the heart into the transport ring
      ****************************************/
      time(&now);
      if (difftime(now,MyLastBeat) > (double)HeartBeatInt )
      {
          ringdup_status( TypeHeartBeat, 0, "" );
      time(&MyLastBeat);
      }

      /* take a brief nap; added 970624:ldd
       ************************************/
      sleep_ew(500);
   } /*end while of monitoring loop */

   /* Shut it down
   ***************/
   tport_detach( &InRegion );
   tport_detach( &OutRegion );
   exportfilter_shutdown();
   ringdup_free();
   logit("t", "%s(%s): termination requested; exiting!\n",
          Argv0, MyModName );
   return( 0 );
}
/* *******************  end of main *******************************
 ******************************************************************/

/**************************  Main Dup Thread   ***********************
*          Pull a messsage from the queue, and put it on OutRing     *
**********************************************************************/

thr_ret Dup( void *dummy )
{
   int      ret;
   long     msgSize;

   while (1)   /* main loop */
   {
     /* Get message from queue
      *************************/
     RequestMutex();
     ret=dequeue( &OutQueue, SSmsg, &msgSize, &Logo);
     ReleaseMutex_ew();
     if(ret < 0 )
     { /* -1 means empty queue */
       sleep_ew(500); /* wait a bit (changed from 1000 to 500 on 970624:ldd) */
       continue;
     }

     /* Duplicate message in OutRing
      ******************************/
     if (tport_putmsg (&OutRegion, &Logo, msgSize, SSmsg) != PUT_OK)
         logit ("et", "Dup: Error sending message to out ring\n");
   }   /* End of main loop */

}

/********************** Message Stacking Thread *******************
 *           Move messages from transport to memory queue         *
 ******************************************************************/
thr_ret MessageStacker( void *dummy )
{
   long          recsize;   /* size of retrieved message             */
   MSG_LOGO      reclogo;       /* logo of retrieved message             */
   long      filteredSize;  /* size of message after user-filtering  */
   unsigned char filteredType;  /* type of message after filtering       */
   int       res;
   int       ret;
   int           NumOfTimesQueueLapped= 0; /* number of messages lost due to
                                             queue lap */

   /* Tell the main thread we're ok
   ********************************/
   MessageStackerStatus = MSGSTK_ALIVE;

   /* Start main export service loop for current connection
   ********************************************************/
   while( 1 )
   {
      /* Get a message from transport ring
      ************************************/
      res = tport_getmsg( &InRegion, GetLogo, nLogo,
                          &reclogo, &recsize, MSrawmsg, MaxMsgSize-1 );

      /* Wait if no messages for us
       ****************************/
      if( res == GET_NONE ) {sleep_ew(100); continue;}

      /* Check return code; report errors
      ***********************************/
      if( res != GET_OK )
      {
         if( res==GET_TOOBIG )
         {
            sprintf( errText, "msg[%ld] i%d m%d t%d too long for target",
                            recsize, (int) reclogo.instid,
                (int) reclogo.mod, (int)reclogo.type );
            ringdup_status( TypeError, ERR_TOOBIG, errText );
            continue;
         }
         else if( res==GET_MISS )
         {
            sprintf( errText, "missed msg(s) i%d m%d t%d in %s",(int) reclogo.instid,
                    (int) reclogo.mod, (int)reclogo.type, InRing );
            ringdup_status( TypeError, ERR_MISSMSG, errText );
         }
         else if( res==GET_NOTRACK )
         {
            sprintf( errText, "no tracking for logo i%d m%d t%d in %s",
                     (int) reclogo.instid, (int) reclogo.mod, (int)reclogo.type,
                     InRing );
            ringdup_status( TypeError, ERR_NOTRACK, errText );
         }
      }

      /* Process retrieved msg (res==GET_OK,GET_MISS,GET_NOTRACK)
      ***********************************************************/
      MSrawmsg[recsize] = '\0';
      /* pass it through the filter routine: this may reformat,
         or reject as it chooses. */
      if ( exportfilter( MSrawmsg, recsize, reclogo.type, &MSfilteredmsg,
                        &filteredSize, &filteredType ) == 0 )  continue;
      reclogo.type = filteredType;  /* note the new message type */

      /* put it into the 'to be shipped' queue */
      /* the Dup thread is in the biz of de-queueng and sending */
      RequestMutex();
      ret=enqueue( &OutQueue, MSfilteredmsg, filteredSize, reclogo );
      ReleaseMutex_ew();

      if ( ret!= 0 )
      {
         if (ret==-2)  /* Serious: quit */
         {    /* Currently, eneueue() in mem_circ_queue.c never returns this error. */
        sprintf(errText,"internal queue error. Terminating.");
            ringdup_status( TypeError, ERR_QUEUE, errText );
        goto error;
         }
         if (ret==-1)
         {
            sprintf(errText,"queue cannot allocate memory. Lost message.");
            ringdup_status( TypeError, ERR_QUEUE, errText );
            continue;
         }
         if (ret==-3)  /* Log only while client's connected */
         {
         /* Queue is lapped too often to be logged to screen.
          * Log circular queue laps to logfile.
          * Maybe queue laps should not be logged at all.
          */
            NumOfTimesQueueLapped++;
            if (!(NumOfTimesQueueLapped % 5))
            {
               logit("t",
                     "%s(%s): Circular queue lapped 5 times. Messages lost.\n",
                      Argv0, MyModName);
               if (!(NumOfTimesQueueLapped % 100))
               {
                  logit( "et",
                        "%s(%s): Circular queue lapped 100 times. Messages lost.\n",
                         Argv0, MyModName);
               }
            }
            continue;
         }
      }
   } /* end of while */

   /* we're quitting
   *****************/
error:
   MessageStackerStatus = MSGSTK_ERR; /* file a complaint to the main thread */
   KillSelfThread(); /* main thread will restart us */
}

/*****************************************************************************
 *  ringdup_config() processes command file(s) using kom.c functions;         *
 *                    exits if any errors are encountered.               *
 *****************************************************************************/
void ringdup_config( char *configfile )
{
   int      ncommand;     /* # of required commands you expect to process   */
   char     init[10];     /* init flags, one byte for each required command */
   int      nmiss;        /* number of required commands that were missed   */
   char    *com;
   char     processor[15];
   int      nfiles;
   int      success;
   int      i;
   char*    str;

/* Set to zero one init flag for each required command
 *****************************************************/
   ncommand = 8;
   for( i=0; i<ncommand; i++ )  init[i] = 0;
   nLogo = 0;

/* Open the main configuration file
 **********************************/
   nfiles = k_open( configfile );
   if ( nfiles == 0 ) {
    logit( "e" ,
                "%s: Error opening command file <%s>; exiting!\n",
                Argv0, configfile );
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
                  logit( "e" ,
                          "%s: Error opening command file <%s>; exiting!\n",
                           Argv0, &com[1] );
                  exit( -1 );
               }
               continue;
            }
            strcpy( processor, "ringdup_config" );

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
  /*2*/     else if( k_its("InRing") ) {
                str = k_str();
                if(str) strcpy( InRing, str );
                init[2] = 1;
            }
  /*3*/     else if( k_its("OutRing") ) {
                str = k_str();
                if(str) strcpy( OutRing, str );
                init[3] = 1;
            }
  /*4*/     else if( k_its("HeartBeatInt") ) {
                HeartBeatInt = k_int();
                init[4] = 1;
            }


         /* Enter installation & module & message types to get
          ****************************************************/
  /*5*/     else if( k_its("GetMsgLogo") ) {
                if ( nLogo >= MAXLOGO ) {
                    logit( "e" ,
                            "%s: Too many <GetMsgLogo> commands in <%s>",
                             Argv0, configfile );
                    logit( "e" , "; max=%d; exiting!\n", (int) MAXLOGO );
                    exit( -1 );
                }
                if( ( str=k_str() ) ) {
                   if( GetInst( str, &GetLogo[nLogo].instid ) != 0 ) {
                       logit( "e" ,
                               "%s: Invalid installation name <%s>", Argv0, str );
                       logit( "e" , " in <GetMsgLogo> cmd; exiting!\n" );
                       exit( -1 );
                   }
                }
                if( ( str=k_str() ) ) {
                   if( GetModId( str, &GetLogo[nLogo].mod ) != 0 ) {
                       logit( "e" ,
                               "%s: Invalid module name <%s>", Argv0, str );
                       logit( "e" , " in <GetMsgLogo> cmd; exiting!\n" );
                       exit( -1 );
                   }
                }
                if( ( str=k_str() ) ) {
                   if( GetType( str, &GetLogo[nLogo].type ) != 0 ) {
                       logit( "e" ,
                               "%s: Invalid msgtype <%s>", Argv0, str );
                       logit( "e" , " in <GetMsgLogo> cmd; exiting!\n" );
                       exit( -1 );
                   }
                }
                nLogo++;
                init[5] = 1;
            }

         /* Maximum size (bytes) for incoming/outgoing messages
          *****************************************************/
  /*6*/     else if( k_its("MaxMsgSize") ) {
                MaxMsgSize = k_long();
                init[6] = 1;
            }

         /* Maximum number of messages in outgoing circular buffer
          ********************************************************/
  /*7*/     else if( k_its("RingSize") ) {
                RingSize = k_long();
                init[7] = 1;
            }

         /* Pass it off to the filter's config processor
      **********************************************/
            else if( exportfilter_com() ) strcpy( processor, "exportfilter_com" );

         /* Unknown command
          *****************/
        else {
                logit( "e" , "%s: <%s> Unknown command in <%s>.\n",
                         Argv0, com, configfile );
                continue;
            }

        /* See if there were any errors processing the command
         *****************************************************/
            if( k_err() ) {
               logit( "e" ,
                       "%s: Bad <%s> command for %s() in <%s>; exiting!\n",
                        Argv0, com, processor, configfile );
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
       logit( "e", "%s: ERROR, no ", Argv0 );
       if ( !init[0] )  logit( "e", "<LogFile> "      );
       if ( !init[1] )  logit( "e", "<MyModuleId> "   );
       if ( !init[2] )  logit( "e", "<InRing> "     );
       if ( !init[3] )  logit( "e", "<OutRing> "     );
       if ( !init[4] )  logit( "e", "<HeartBeatInt> " );
       if ( !init[5] )  logit( "e", "<GetMsgLogo> "   );
       if ( !init[6] )  logit( "e", "<MaxMsgSize> "  );
       if ( !init[7] )  logit( "e", "<RingSize> "   );
       logit( "e" , "command(s) in <%s>; exiting!\n", configfile );
       exit( -1 );
   }
   return;
}

/****************************************************************************
 *  ringdup_lookup( )   Look up important info from earthworm.h tables       *
 ****************************************************************************/
void ringdup_lookup( void )
{
/* Look up keys to shared memory regions
   *************************************/
   if( ( InRingKey = GetKey(InRing) ) == -1 ) {
    fprintf( stderr,
            "%s:  Invalid ring name <%s>; exiting!\n",
                 Argv0, InRing);
    exit( -1 );
   }
   if( ( OutRingKey = GetKey(OutRing) ) == -1 ) {
    fprintf( stderr,
            "%s:  Invalid ring name <%s>; exiting!\n",
                 Argv0, OutRing);
    exit( -1 );
   }

/* Look up installations of interest
   *********************************/
   if ( GetLocalInst( &InstId ) != 0 ) {
      fprintf( stderr,
              "%s: error getting local installation id; exiting!\n",
               Argv0 );
      exit( -1 );
   }

/* Look up modules of interest
   ***************************/
   if ( GetModId( MyModName, &MyModId ) != 0 ) {
      fprintf( stderr,
              "%s: Invalid module name <%s>; exiting!\n",
               Argv0, MyModName );
      exit( -1 );
   }

/* Look up message types of interest
   *********************************/
   if ( GetType( "TYPE_HEARTBEAT", &TypeHeartBeat ) != 0 ) {
      fprintf( stderr,
              "%s: Invalid message type <TYPE_HEARTBEAT>; exiting!\n", Argv0 );
      exit( -1 );
   }
   if ( GetType( "TYPE_ERROR", &TypeError ) != 0 ) {
      fprintf( stderr,
              "%s: Invalid message type <TYPE_ERROR>; exiting!\n", Argv0 );
      exit( -1 );
   }
   return;
}

/***************************************************************************
 * ringdup_status() builds a heartbeat or error message & puts it into      *
 *                 shared memory.  Writes errors to log file & screen.     *
 ***************************************************************************/
void ringdup_status( unsigned char type, short ierr, char *note )
{
   MSG_LOGO    logo;
   char        msg[256];
   long        size;
   time_t      t;

/* Build the message
 *******************/
   logo.instid = InstId;
   logo.mod    = MyModId;
   logo.type   = type;

   time( &t );

   if( type == TypeHeartBeat )
    sprintf( msg, "%ld %ld\n\0", (long) t, (long) MyPid);
   else if( type == TypeError )
   {
    sprintf( msg, "%ld %hd %s\n\0", (long) t, ierr, note);

    logit( "et", "%s(%s): %s\n", Argv0, MyModName, note );
   }

   size = strlen( msg );   /* don't include the null byte in the message */

/* Write the message to shared memory
 ************************************/
   if( tport_putmsg( &InRegion, &logo, size, msg ) != PUT_OK )
   {
        if( type == TypeHeartBeat ) {
           logit("et","%s(%s):  Error sending heartbeat.\n",
                  Argv0, MyModName );
    }
    else if( type == TypeError ) {
           logit("et", "%s(%s):  Error sending error:%d.\n",
                  Argv0, MyModName, ierr );
    }
   }

   return;
}

/***************************************************************************
 * ringdup_free()  free all previously allocated memory                     *
 ***************************************************************************/
void ringdup_free( void )
{
   free (Rawmsg);               /* "raw" retrieved msg for main thread    */
   free (SSmsg);                /* Dup's incoming message buffer   */
   free (MSrawmsg);             /* MessageStacker's "raw" retrieved message */
   free (MSfilteredmsg);        /* MessageStacker's "filtered" message to   */
                                /*    be sent to client                     */
   return;
}
