
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: shakemapfeed.c,v 1.9 2002/04/12 22:35:21 dietz Exp $
 *
 *    Revision history:
 *     $Log: shakemapfeed.c,v $
 *     Revision 1.9  2002/04/12 22:35:21  dietz
 *     Log file is initialized before config file is read.  Config file
 *     errors will now be logged.
 *
 *     Revision 1.8  2001/07/01 21:55:35  davidk
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
 *     Revision 1.7  2001/05/15 02:15:46  davidk
 *     Moved functions around between the apps, DB API, and DB API INTERNAL
 *     levels.  Renamed functions and files.  Added support for amplitude
 *     magnitude types.  Reformatted makefiles.
 *
 *     Revision 1.6  2001/05/10 20:28:54  dietz
 *     Changed to shut down gracefully if the transport flag is
 *     set to TERMINATE or MyPid.
 *
 *     Revision 1.5  2001/05/01 20:39:03  dietz
 *     changed to work with strongmotionII msgs and schema
 *
 *     Revision 1.4  2001/03/22 18:14:39  lombard
 *     Added support for XML writer and the mapping file.
 *     changed to use standard macros for Earthworm logo fields
 *
 *     Revision 1.3  2001/03/15 00:01:38  dietz
 *     *** empty log message ***
 *
 *     Revision 1.2  2000/07/27 16:10:59  lucky
 *     Changed max event size to their DB equivalents
 *
 *     Revision 1.1  2000/02/15 20:20:15  lucky
 *     Initial revision
 *
 *
 */


/*
 * shakemapfeed.c:  
 * Module to read archive messages from a ring and produce
 * files containing strong motion information to be fed to
 * the shakemap process.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "shakemapfeed.h"

static pid_t MyPid;

int main( int argc, char **argv )
{
  PARAM     param;                     /* config-file parameters        */
  EWD       ewd;                       /* earthworm parameters          */
  char      inbuf[DB_MAX_BYTES_PER_EQ+1]; /* char string to hold event  */
  char      text[256];                 /* string for log/error messages */
  long      timeNow;                   /* current time                  */
  long      timeLastBeat;              /* time last heartbeat was sent  */
  long      recsize;                   /* size of retrieved message     */
  MSG_LOGO  reclogo;                   /* logo of retrieved message     */
  int       result;
  unsigned char seq;

/* Check command line arguments 
 ******************************/
  if( argc != 2 )
  {
     fprintf( stderr, "Usage: shakemapfeed <configfile>\n" );
     return( 0 );
  }
  logit_init( argv[1], 0, 1024, 1 );
           
/* Read the configuration file(s)
 ********************************/
  shakemapfeed_config( argv[1], &param );

/* Look up important info from earthworm.h tables
 ************************************************/
  shakemapfeed_lookup( &param, &ewd );
 
/* Initialize name of log-file & open it 
 ***************************************/
  logit_init( argv[1], 0, 1024, param.LogSwitch );
  logit( "" , "%s: Read command file <%s>\n", argv[0], argv[1] );

/* Get process id
 ****************/
  MyPid = getpid();
  if( MyPid == -1 )
  {
    logit("e","shakemapfeed: Cannot get processid; exiting.\n");
    exit( -1 );
  }

  /* Initialize the mapping tables needed for writing XML files */
  if (param.MappingFile != NULL)
  {
    if (initMappings( param.MappingFile ) < 0)
    {
      logit("et", "shakemapfeed: error initializing mappings; exiting!\n");
      exit( -1 );
    }
  }

/* Attach to Input/Output shared memory ring 
 *******************************************/
  tport_attach( &(ewd.Region), ewd.RingKey );
  logit( "", "shakemapfeed: Attached to public memory region %s: %d\n", 
          param.RingName, ewd.RingKey );

/* Flush the transport ring 
 **************************/
  while( tport_copyfrom( &(ewd.Region), param.GetLogo, param.nLogo, 
             &reclogo, &recsize, inbuf, DB_MAX_BYTES_PER_EQ, &seq ) 
             != GET_NONE );

/* Force a heartbeat to be issued in first pass thru main loop
 *************************************************************/
  timeLastBeat = time(&timeNow) - param.HeartBeatInterval - 1;

/*---------------------Main message processing loop----------------------*/
  while( tport_getflag( &(ewd.Region) ) != TERMINATE  &&
         tport_getflag( &(ewd.Region) ) != MyPid )
  {
    int rc;

 /* Check for need to send heartbeat message */
    if( param.HeartBeatInterval  &&
        time( &timeNow ) - timeLastBeat >= param.HeartBeatInterval )
    {
       timeLastBeat = timeNow;
       shakemapfeed_status( &ewd, ewd.TypeHeartBeat, 0, "" );
    }
    
 /* Get next message */
    result = tport_copyfrom( &(ewd.Region), param.GetLogo, param.nLogo,
                             &reclogo, &recsize, inbuf, DB_MAX_BYTES_PER_EQ, 
                             &seq );

 /* Act on the return code from the transport function */
    switch( result )
    {
    case GET_OK:	/* got a message, no errors or warnings		*/
      if( param.Debug )
	logit( "t", "Message received(i%u m%u t%u).\n", 
	       reclogo.instid, reclogo.mod, reclogo.type );
      break;
    case GET_NONE:	/* no messages of interest, check again later	*/
      sleep_ew( 500 );	                  /* go to sleep for millisec   */
      rc = ReviewEvents( &param, &ewd );  /* check timers on old events */
      if( rc == INIT_FATAL ) goto ShutDown;
      continue;
    case GET_NOTRACK:	/* got a msg, but can't tell if any were missed	*/
      sprintf( text,
              "Msg received (i%u m%u t%u); transport.h NTRACK_GET exceeded",
               reclogo.instid, reclogo.mod, reclogo.type );
      shakemapfeed_status( &ewd, ewd.TypeError, ERR_NOTRACK, text );
      break;
    case GET_MISS_LAPPED:	/* got a msg, but also missed lots	*/
      sprintf( text,
              "Msg received(i%u m%u t%u); missed some (lapped)",
               reclogo.instid, reclogo.mod, reclogo.type );
      shakemapfeed_status( &ewd, ewd.TypeError, ERR_MISSLAPMSG, text );
      break;
    case GET_MISS_SEQGAP:	/* got a msg, but saw sequence gap	*/
      sprintf( text,
              "Msg received(i%u m%u t%u); sequence number gap",
               reclogo.instid, reclogo.mod, reclogo.type );
      shakemapfeed_status( &ewd, ewd.TypeError, ERR_MISSGAPMSG, text );
      break;
    case GET_TOOBIG:	/* next message was too big (no msg returned)	*/
      sprintf( text,
              "Skipped msg[%ld] (i%u m%u t%u) too big for inbuf[%d]",
               recsize, reclogo.instid, reclogo.mod, reclogo.type,
               DB_MAX_BYTES_PER_EQ );
      shakemapfeed_status( &ewd, ewd.TypeError, ERR_TOOBIG, text );
      continue;
    default:	             /* Unknown result                         */
      sprintf( text, "Unknown tport_copyfrom result:%d", result );
      shakemapfeed_status( &ewd, ewd.TypeError, ERR_UNKNOWN, text );
      continue;
    }

 /* Process the new message */
    inbuf[recsize] = '\0';  
    if( reclogo.type == ewd.TypeHyp2000Arc ) 
    {
      rc = ProcessArc( &param, &ewd, inbuf, recsize );
      /*  ### What errors should make us quit?				*/
      /* Maybe none; press on, regardless; condition is already logged.	*/
      if( rc == INIT_FATAL ) goto ShutDown; 
    }
    else if( reclogo.type == ewd.TypeEQDelete ) 
    {
      rc = ProcessDelete( &param, &ewd, inbuf, recsize );
      if( rc == INIT_FATAL ) goto ShutDown; 
    }
    else if( reclogo.type == ewd.TypeMagnitude ) 
    {
      rc = ProcessMag( &param, &ewd, inbuf, recsize );
      if( rc == INIT_FATAL ) goto ShutDown; 
    }
    else
    {
      logit( "et", "%s: Unknown or unrequested message (type:%d) received!\n", 
             argv[0], reclogo.type );
    }
  }  
/*--------------------- End of main processing loop --------------------*/

  /* Termination has been requested */
  logit( "t", "%s: Termination requested; cleaning up...", argv[0] );

ShutDown:

  /* Detach from shared memory regions */
  tport_detach( &(ewd.Region) ); 

  /* Clean up other stuff */
  ShutdownEvent( &param );

  /* Flush the output stream */
  fflush( stdout );

  logit( "", "exiting!\n" );
  return( 0 );
}


/******************************************************************************
 * shakemapfeed_status() builds a heartbeat or error message & puts it into   *
 *                   shared memory.  Writes errors to log file & screen.      *
 ******************************************************************************/
void shakemapfeed_status( EWD *ewd, 
                          unsigned char type, short ierr, char *note )
{
   MSG_LOGO      logo;
   char          msg[256];
   long          size;
   long          t;

/* Build the message
 *******************/ 
   logo.instid = ewd->InstId;
   logo.mod    = ewd->MyModId;
   logo.type   = type;

   time( &t );

   if( type == ewd->TypeHeartBeat )
   {
        sprintf( msg, "%ld %ld\n\0", t, MyPid);
   }
   else if( type == ewd->TypeError )
   {
        sprintf( msg, "%ld %hd %s\n\0", t, ierr, note);
        logit( "et", "shakemapfeed: %s\n", note );
   }

   size = strlen( msg );   /* don't include the null byte in the message */     

/* Write the message to shared memory
 ************************************/
   if( tport_putmsg( &(ewd->Region), &logo, size, msg ) != PUT_OK )
   {
        if( type == ewd->TypeHeartBeat ) {
           logit("et","shakemapfeed:  Error sending heartbeat.\n" );
        }
        else if( type == ewd->TypeError ) {
           logit("et","shakemapfeed:  Error sending error:%d.\n", ierr );
        }
   }

   return;
}


/******************************************************************************
 * shakemapfeed_page() builds TYPE_PAGE msg(s) & puts them into shared memory *
 ******************************************************************************/
void shakemapfeed_page( PARAM *parm, EWD *ewd, char *note )
{
   MSG_LOGO  logo;
   char      msg[NOTELEN*2];
   int       i;

   if( parm->nPage==0 ) return;  /* no page recipients, nothing to do */

/* Assemble TYPE_PAGE messages; write to shared memory
   ***************************************************/
   logo.instid = ewd->InstId;
   logo.mod    = ewd->MyModId;
   logo.type   = ewd->TypePage;

   for( i=0; i<parm->nPage; i++ ) {
      sprintf( msg, "group: %s shakemapfeed: %s#\0", parm->Page[i], note );
      if( tport_putmsg( &(ewd->Region), &logo, strlen(msg), msg ) != PUT_OK )
      {
         logit("et", "shakemapfeed: Error sending TYPE_PAGE msg "
                     "to transport region.\n" );
      }
   }
   return;
 }
