/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: condenselogo.c,v 1.3 2007/02/26 14:23:17 paulf Exp $
 *
 *    Revision history:
 *     $Log: condenselogo.c,v $
 *     Revision 1.3  2007/02/26 14:23:17  paulf
 *     fixed long casting of time_t for heartbeat sprintf()
 *
 *     Revision 1.2  2007/02/23 15:37:15  paulf
 *     fixed long to time_t declaration
 *
 *     Revision 1.1  2006/11/22 23:06:23  stefan
 *     menlo contrib
 *
 *     Revision 1.1.1.1  2004/04/28 23:15:44  dietz
 *     pre-location code Contrib/Menlo
 *
 *
 *
 */

/*
 * condenselogo.c
 *
 * Reads messages from a list of logos from one transport ring 
 * and writes them to another ring using its own installation and
 * moduleid in the logo.  Does not alter the contents of the 
 * message in any way.
 * Optionally maintains the original installation id.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <earthworm.h>
#include <transport.h>
#include <kom.h>

/* Functions in this source file
 *******************************/
void   condenselogo_config  ( char * );
void   condenselogo_lookup  ( void );
void   condenselogo_status  ( unsigned char, short, char * );

static SHM_INFO  InRegion;    /* shared memory region to use for input  */
static SHM_INFO  OutRegion;   /* shared memory region to use for output */
static pid_t	 MyPid;       /* Our process id is sent with heartbeat  */

/* Things to read or derive from configuration file
 **************************************************/
static int   LogSwitch;            /* 0 if no logfile should be written */
static long  HeartbeatInt;         /* seconds between heartbeats        */
static int   UseOriginalInstid=0;  /* if non-zero, keep original instid */
static long  MaxMessageSize = MAX_BYTES_PER_EQ; 
                           /* size (bytes) of largest message we'll see */
static MSG_LOGO *GetLogo = NULL; /* logo(s) to get from shared memory   */
static short     nLogo   = 0;    /* # logos we're configured to get     */

/* Things to look up in the earthworm.h tables with getutil.c functions
 **********************************************************************/
static long          InRingKey;      /* key of transport ring for input   */
static long          OutRingKey;     /* key of transport ring for output  */
static unsigned char InstId;         /* local installation id             */
static unsigned char MyModId;        /* Module Id for this program        */
static unsigned char TypeHeartBeat;
static unsigned char TypeError;

/* Error messages used by condenselogo
 *************************************/
#define  ERR_MISSGAPMSG    0   /* sequence gap in transport ring         */
#define  ERR_MISSLAPMSG    1   /* missed messages in transport ring      */
#define  ERR_TOOBIG        2   /* retreived msg too large for buffer     */
#define  ERR_NOTRACK       3   /* msg retreived; tracking limit exceeded */
static char  Text[150];        /* string for log/error messages          */


int main( int argc, char **argv )
{
   char         *msgbuf;           /* buffer for msgs from ring     */
   time_t        timeNow;          /* current time                  */
   time_t        timeLastBeat;     /* time last heartbeat was sent  */
   long          recsize;          /* size of retrieved message     */
   MSG_LOGO      reclogo;          /* logo of retrieved message     */
   MSG_LOGO      putlogo;          /* logo to use putting message into ring */
   int           res;
   unsigned char seq;


/* Check command line arguments
 ******************************/
   if ( argc != 2 )
   {
        fprintf( stderr, "Usage: condenselogo <configfile>\n" );
        exit( 0 );
   }

/* Read the configuration file(s)
 ********************************/
   condenselogo_config( argv[1] );

/* Look up important info from earthworm.h tables
 ************************************************/
   condenselogo_lookup();

/* Initialize name of log-file & open it
 ***************************************/
   logit_init( argv[1], (short) MyModId, 1024, LogSwitch );
   logit( "" , "condenselogo: Read command file <%s>\n", argv[1] );

/* Get our own process ID for restart purposes
 *********************************************/
   if( (MyPid = getpid()) == -1 )
   {
      logit ("e", "condenselogo: Call to getpid failed. Exiting.\n");
      free( GetLogo );
      exit( -1 );
   }

/* Allocate the message input buffer 
 ***********************************/
  if ( !( msgbuf = (char *) malloc( (size_t)MaxMessageSize ) ) )
  {
      logit( "et", 
             "condenselogo: failed to allocate %d bytes"
             " for message buffer; exiting!\n", MaxMessageSize );
      free( GetLogo );
      exit( -1 );
  }

/* Initialize outgoing logo
 **************************/

/* Attach to shared memory rings
 *******************************/
   tport_attach( &InRegion, InRingKey );
   logit( "", "condenselogo: Attached to public memory region: %ld\n",
          InRingKey );
   tport_attach( &OutRegion, OutRingKey );
   logit( "", "condenselogo: Attached to public memory region: %ld\n",
           OutRingKey );

/* Force a heartbeat to be issued in first pass thru main loop
 *************************************************************/
   timeLastBeat = time(&timeNow) - HeartbeatInt - 1;

/* Flush the incoming transport ring on startup
 **********************************************/ 
   while( tport_copyfrom(&InRegion, GetLogo, nLogo,  &reclogo,
          &recsize, msgbuf, MaxMessageSize, &seq ) != GET_NONE );

/*----------------------- setup done; start main loop -------------------------*/

  while ( tport_getflag( &InRegion ) != TERMINATE  &&
          tport_getflag( &InRegion ) != MyPid )
  {
     /* send condenselogo's heartbeat
      *******************************/
        if( HeartbeatInt  &&  time(&timeNow)-timeLastBeat >= HeartbeatInt )
        {
            timeLastBeat = timeNow;
            condenselogo_status( TypeHeartBeat, 0, "" );
        }

     /* Get msg & check the return code from transport
      ************************************************/
        res = tport_copyfrom( &InRegion, GetLogo, nLogo, &reclogo, 
                              &recsize, msgbuf, MaxMessageSize, &seq );

        switch( res )
        {
        case GET_OK:      /* got a message, no errors or warnings         */
             break;

        case GET_NONE:    /* no messages of interest, check again later   */
             sleep_ew(100); /* milliseconds */
             continue;

        case GET_NOTRACK: /* got a msg, but can't tell if any were missed */
             sprintf( Text,
                     "Msg received (i%u m%u t%u); transport.h NTRACK_GET exceeded",
                      reclogo.instid, reclogo.mod, reclogo.type );
             condenselogo_status( TypeError, ERR_NOTRACK, Text );
             break;

        case GET_MISS_LAPPED:     /* got a msg, but also missed lots      */
             sprintf( Text,
                     "Missed msg(s) from logo (i%u m%u t%u)",
                      reclogo.instid, reclogo.mod, reclogo.type );
             condenselogo_status( TypeError, ERR_MISSLAPMSG, Text );
             break;

        case GET_MISS_SEQGAP:     /* got a msg, but seq gap               */
             sprintf( Text,
                     "Saw sequence# gap for logo (i%u m%u t%u)",
                      reclogo.instid, reclogo.mod, reclogo.type );
             condenselogo_status( TypeError, ERR_MISSGAPMSG, Text );
             break;

       case GET_TOOBIG:  /* next message was too big, resize buffer      */
             sprintf( Text,
                     "Retrieved msg[%ld] (i%u m%u t%u) too big for msgbuf[%ld]",
                      recsize, reclogo.instid, reclogo.mod, reclogo.type,
                      MaxMessageSize );
             condenselogo_status( TypeError, ERR_TOOBIG, Text );
             continue;

       default:         /* Unknown result                                */
             sprintf( Text, "Unknown tport_copyfrom result:%d", res );
             condenselogo_status( TypeError, ERR_TOOBIG, Text );
             continue;
       }

    /* Write the message to OutRing with own logo
     ********************************************/
       if(UseOriginalInstid) putlogo.instid = reclogo.instid;
       else                  putlogo.instid = InstId;  
       putlogo.mod    = MyModId;
       putlogo.type   = reclogo.type;

       if( tport_putmsg( &OutRegion, &putlogo, recsize, msgbuf ) != PUT_OK )
       {
          logit("et","condenselogo: Error writing %d-byte msg to ring; "
                     "original logo (i%u m%u t%u)\n", recsize,
                      reclogo.instid, reclogo.mod, reclogo.type );
       }
   }

/* free allocated memory */
   free(GetLogo);
   free(msgbuf);

/* detach from shared memory */
   tport_detach( &InRegion );
   tport_detach( &OutRegion );
           
/* write a termination msg to log file */
   logit( "t", "condenselogo: Termination requested; exiting!\n" );
   fflush( stdout );
   return( 0 );

/*-----------------------------end of main loop-------------------------------*/
}

/******************************************************************************
 *  condenselogo_config() processes command file(s) using kom.c functions;    *
 *                    exits if any errors are encountered.                    *
 ******************************************************************************/
#define ncommand 6         /* # of required commands you expect to process   */
void condenselogo_config( char *configfile )
{
   char  init[ncommand];   /* init flags, one byte for each required command */
   int   nmiss;            /* number of required commands that were missed   */
   char *com;
   char *str;
   int   nfiles;
   int   success;
   int   i;

/* Set to zero one init flag for each required command
 *****************************************************/
   for( i=0; i<ncommand; i++ )  init[i] = 0;
   nLogo = 0;

/* Open the main configuration file
 **********************************/
   nfiles = k_open( configfile );
   if ( nfiles == 0 ) {
        fprintf( stderr,
                "condenselogo: Error opening command file <%s>; exiting!\n",
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
                  fprintf( stderr,
                          "condenselogo: Error opening command file <%s>; exiting!\n",
                           &com[1] );
                  exit( -1 );
               }
               continue;
            }

        /* Process anything else as a command
         ************************************/
  /*0*/     if( k_its("LogFile") ) {
                LogSwitch = k_int();
                if( LogSwitch<0 || LogSwitch>2 ) {
                   fprintf( stderr,
                           "condenselogo: Invalid <LogFile> value %d; "
                           "must = 0, 1 or 2; exiting!\n", LogSwitch );
                   exit( -1 );
                }
                init[0] = 1;
            }

  /*1*/     else if( k_its("MyModuleId") ) {
                if( str=k_str() ) {
                   if( GetModId( str, &MyModId ) != 0 ) {
                      fprintf( stderr,
                              "condenselogo: Invalid module name <%s> "
                              "in <MyModuleId> command; exiting!\n", str);
                      exit( -1 );
                   }
                }
                init[1] = 1;
            }

  /*2*/     else if( k_its("InRing") ) {
                if( str=k_str() ) {
                   if( ( InRingKey = GetKey(str) ) == -1 ) {
                      fprintf( stderr,
                              "condenselogo: Invalid ring name <%s> "
                              "in <InRing> command; exiting!\n", str);
                      exit( -1 );
                   }
                }
                init[2] = 1;
            }

  /*3*/     else if( k_its("OutRing") ) {
                if( str=k_str() ) {
                   if( ( OutRingKey = GetKey(str) ) == -1 ) {
                      fprintf( stderr,
                              "condenselogo: Invalid ring name <%s> "
                              "in <OutRing> command; exiting!\n", str);
                      exit( -1 );
                   }
                }
                init[3] = 1;
            }

  /*4*/     else if( k_its("HeartbeatInt") ) {
                HeartbeatInt = k_long();
                init[4] = 1;
            }


         /* Enter installation & module to get event messages from
          ********************************************************/
  /*5*/     else if( k_its("GetLogo") ) {
                MSG_LOGO *tlogo = NULL;
                tlogo = (MSG_LOGO *)realloc( GetLogo, (nLogo+1)*sizeof(MSG_LOGO) );
                if( tlogo == NULL )
                {
                   fprintf(stderr, "condenselogo: GetLogo: error reallocing"
                                   " %d bytes; exiting!\n",
                           (nLogo+1)*sizeof(MSG_LOGO) );
                   exit( -1 );
                }
                GetLogo = tlogo;

                if( str=k_str() ) {
                   if( GetInst( str, &GetLogo[nLogo].instid ) != 0 ) {
                       fprintf( stderr,
                               "condenselogo: Invalid installation name <%s>"
                               " in <GetLogo> cmd; exiting!\n", str );
                       exit( -1 );
                   }
                   if( str=k_str() ) {
                      if( GetModId( str, &GetLogo[nLogo].mod ) != 0 ) {
                          fprintf( stderr,
                                  "condenselogo: Invalid module name <%s>"
                                  " in <GetLogo> cmd; exiting!\n", str );
                          exit( -1 );
                      }
                      if( str=k_str() ) {
                         if( GetType( str, &GetLogo[nLogo].type ) != 0 ) {
                             fprintf( stderr,
                                     "condenselogo: Invalid message type <%s>"
                                     " in <GetLogo> cmd; exiting!\n", str );
                             exit( -1 );
                         }
                      }
                   }
                }
              /*printf("GetLogo[%d] inst:%d module:%d type:%d\n",
                        nLogo, (int) GetLogo[nLogo].instid,
                               (int) GetLogo[nLogo].mod,
                               (int) GetLogo[nLogo].type );*/   /*DEBUG*/
                nLogo++;
                init[5] = 1;
            }

  /*opt*/   else if( k_its("MaxMessageSize") ) {
                MaxMessageSize = k_long();
            }

  /*opt*/   else if( k_its("UseOriginalInstid") ) {
                UseOriginalInstid = k_int();
            }


         /* Unknown command
          *****************/
            else {
                fprintf( stderr, "condenselogo: <%s> Unknown command in <%s>.\n",
                         com, configfile );
                continue;
            }

        /* See if there were any errors processing the command
         *****************************************************/
            if( k_err() ) {
               fprintf( stderr,
                       "condenselogo: Bad <%s> command in <%s>; exiting!\n",
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
       fprintf( stderr, "condenselogo: ERROR, no " );
       if ( !init[0] )  fprintf( stderr, "<LogFile> "      );
       if ( !init[1] )  fprintf( stderr, "<MyModuleId> "   );
       if ( !init[2] )  fprintf( stderr, "<InRing> "       );
       if ( !init[3] )  fprintf( stderr, "<OutRing> "      );
       if ( !init[4] )  fprintf( stderr, "<HeartbeatInt> " );
       if ( !init[5] )  fprintf( stderr, "<GetLogo> "      );
       fprintf( stderr, "command(s) in <%s>; exiting!\n", configfile );
       exit( -1 );
   }

   return;
}

/******************************************************************************
 *  condenselogo_lookup( )   Look up important info from earthworm tables     *
 ******************************************************************************/

void condenselogo_lookup( void )
{

/* Look up installations of interest
   *********************************/
   if ( GetLocalInst( &InstId ) != 0 ) {
      fprintf( stderr,
              "condenselogo: error getting local installation id; exiting!\n" );
      exit( -1 );
   }

/* Look up message types of interest
   *********************************/
   if ( GetType( "TYPE_HEARTBEAT", &TypeHeartBeat ) != 0 ) {
      fprintf( stderr,
              "condenselogo: Invalid message type <TYPE_HEARTBEAT>; exiting!\n" );
      exit( -1 );
   }
   if ( GetType( "TYPE_ERROR", &TypeError ) != 0 ) {
      fprintf( stderr,
              "condenselogo: Invalid message type <TYPE_ERROR>; exiting!\n" );
      exit( -1 );
   }

   return;
}

/******************************************************************************
 * condenselogo_status() builds a heartbeat or error message & puts it into   *
 *                   shared memory.  Writes errors to log file & screen.      *
 ******************************************************************************/
void condenselogo_status( unsigned char type, short ierr, char *note )
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
   {
        sprintf( msg, "%ld %ld\n", (long) t, (long) MyPid );
   }
   else if( type == TypeError )
   {
        sprintf( msg, "%ld %hd %s\n", (long) t, ierr, note);
        logit( "et", "condenselogo: %s\n", note );
   }

   size = strlen( msg );   /* don't include the null byte in the message */

/* Write the message to shared memory
 ************************************/
   if( tport_putmsg( &OutRegion, &logo, size, msg ) != PUT_OK )
   {
        if( type == TypeHeartBeat ) {
           logit("et","condenselogo:  Error sending heartbeat.\n" );
        }
        else if( type == TypeError ) {
           logit("et","condenselogo:  Error sending error:%d.\n", ierr );
        }
   }

   return;
}

