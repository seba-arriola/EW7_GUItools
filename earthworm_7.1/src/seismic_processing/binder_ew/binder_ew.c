
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: binder_ew.c,v 1.13 2007/02/26 13:44:40 paulf Exp $
 *
 *    Revision history:
 *     $Log: binder_ew.c,v $
 *     Revision 1.13  2007/02/26 13:44:40  paulf
 *     fixed heartbeat sprintf() to cast time_t as long
 *
 *     Revision 1.12  2005/08/01 21:12:31  dietz
 *     Added optional "EventIdFile" command to set the name of the file
 *     which will store the next valid eventid for this instance of
 *     binder (default = quake_id.d). This will allow multiple instances
 *     of binder to run on the same host.
 *
 *     Revision 1.11  2005/08/01 20:08:25  dietz
 *     Added optional "BufferRing" command (default=BINDER_RING) to enable
 *     multiple instances of binder_ew to run on one host.
 *
 *     Revision 1.10  2005/04/15 17:35:04  dietz
 *     Modified to terminate cleanly on request.
 *
 *     Revision 1.9  2004/10/29 18:59:29  dietz
 *     added bind_init before grid_init
 *
 *     Revision 1.8  2004/10/21 16:49:27  dietz
 *     Modified to allow pick association with entire quake list (previously
 *     only attempted assoc with 10 most recent quakes). Changes required
 *     keeping track of the earliest pick sequence number associated with each
 *     quake so that a hypocenter update is not attempted if some of the
 *     supporting picks are no longer in the pick FIFO.
 *
 *     Revision 1.7  2004/05/14 23:35:37  dietz
 *     modified to work with TYPE_PICK_SCNL messages only
 *
 *     Revision 1.6  2002/09/05 22:02:32  dietz
 *     Changed to log all picks, even if SCN is not in station list
 *
 *     Revision 1.5  2002/05/31 21:45:17  dietz
 *     changed to use logit instead of fprintf when processing config file
 *
 *     Revision 1.4  2002/05/16 15:04:53  patton
 *     Made Logit changes.
 *
 *     Revision 1.3  2001/05/09 18:12:33  dietz
 *     Changed to shut down gracefully if the transport flag is
 *     set to TERMINATE or MyPID.
 *
 *     Revision 1.2  2000/07/24 20:38:44  lucky
 *     Implemented global limits to module, installation, ring, and message type strings.
 *
 *     Revision 1.1  2000/02/14 16:08:53  lucky
 *     Initial revision
 *
 *     Revision 1.1  2000/02/14 16:07:49  lucky
 *     Initial revision
 *
 *
 */

/*
 * binder_ew.c : Associator main program.  Written by Carl Johnson.
 */

/* 950207:LDD started major overhaul to change the fifo routines to
              my transport layer code LDD */
/* 941216:LDD added error logging for when picks are missed in Pick fifo */
/* 940808:LDD added heartbeat */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <earthworm.h>
#include <transport.h>
#include <kom.h>
#include <tlay.h>
#include <site.h>
#include <chron3.h>
#include <rdpickcoda.h>
#include "bind.h"

SHM_INFO   Region1;               /* main shared memory region              */
SHM_INFO   Region2;               /* binder_ew's private memory region      */
const long region2_size = 250000; /* size of binder_ew's private memory region */

#define MAXLOGO   2
MSG_LOGO  GetLogo[MAXLOGO];       /* array of requested module,type,instid  */
static short  nLogo;              /* number of logos being requested        */

/* Things to look up in the earthworm.h tables with getutil.c functions
 **********************************************************************/
static long          RingKey   = 0;  /* key to transport ring             */
static long          BufferKey = 0;  /* key to binder_ew's buffering ring */
static unsigned char InstId;         /* local installation id             */
static unsigned char MyModId;        /* binder_ew's module id             */
static unsigned char TypeHeartBeat;
static unsigned char TypeError;
static unsigned char TypePickSCNL;
static unsigned char TypeQuake2K;
static unsigned char TypeLink;

/* Things to read from configuration file
 ****************************************/
static char   RingName[MAX_RING_STR];   /* name of transport ring for i/o            */
static char   BufferRing[MAX_RING_STR]; /* name of transport ring for private buffer */
static char   MyModName[MAX_MOD_STR];   /* module id for this module                 */
static int    LogSwitch;                /* 0 if no logging should be done to disk    */
static time_t HeartbeatInt;             /* seconds between heartbeats                */
       char   EventIdFile[MAX_DIR_LEN]; /* file forstoring next valid eventid        */ 

/* Variables for talking to statmgr
 **********************************/
time_t    timeNow;
time_t    timeLastBeat;              /* time last heartbeat was sent */
char      Text[150];                 /* text for logfile/error msg   */
pid_t     MyPID=0;

/* Error words used by binder_ew
 ****************************/
#define   ERR_PICKREAD          0
#define   ERR_UNKNOWN_STA       1
#define   ERR_PTIME_DECODE      2
#define   ERR_MISSMSG           3
#define   ERR_TOOBIG            4
#define   ERR_NOTRACK           5

/* Function prototypes
 *********************/
void bndr_config  ( char * );
void bndr_lookup  ( void );
void bndr_quake2k ( long );
void bndr_link    ( long, long );
void bndr_status  ( unsigned char,  short,  char * );
int  bndr_rdpickscnl( PICK *, char * );
void bndr_wrapup( void );
int  assess_com( void );
int  bind_com( void );
void bind_init( void );
int  bind_pick( PICK * );
int  hyp_com( void );
void hyp_free( void );
int  grid_com( void );
void grid_init( void );
void grid_free( void );

/******************************************************************************/
/*        binder_ew:  Associator main program.  Written by Carl Johnson.      */
/******************************************************************************/

int main( int argc, char **argv )
{
   char        rec[128];                 /* actual retrieved message   */
   long        recsize;                  /* size of retrieved message  */
   MSG_LOGO    reclogo;                  /* logo of retrieved message  */
   PICK        pk;                       /* binder_ew's storage for picks */
   int         res;

/* Check command line arguments
 ******************************/
   if ( argc != 2 )
   {
        fprintf( stderr, "Usage: binder_ew <configfile>\n" );
        exit( 0 );
   }

/* Initialize name of log-file & open it
 ***************************************/
   logit_init( argv[1], 0, 512, 1 );

/* Read the configuration file(s)
 ********************************/
   bndr_config( argv[1] );
   logit( "" , "binder_ew: Configured with command file <%s>\n", argv[1] );

/* Look up important info from earthworm.h tables
 ************************************************/
   bndr_lookup();

/* Store my own processid
 ************************/
   MyPID = getpid();

/* Reinitialize logit to desired logging level
 *********************************************/
   logit_init( argv[1], 0, 512, LogSwitch );
   

/* Attach to public PICK shared memory ring
 ******************************************/
   tport_attach( &Region1, RingKey );
   logit( "", "binder_ew: Attached to public memory region <%s>: %ld\n",
          RingName, Region1.key );

/* Create binder_ew's private memory ring for buffering input
 *********************************************************/
   tport_create( &Region2, region2_size, BufferKey );
   logit( "", "binder_ew: Attached to private memory region <%s>: %ld\n",
          BufferRing, Region2.key );

/* Send first heartbeat
 **********************/
   time(&timeLastBeat);
   bndr_status( TypeHeartBeat, 0, "" );

/* Start input buffer thread
 ***************************/
   if ( tport_buffer( &Region1, &Region2, GetLogo, nLogo, sizeof(rec)-1,
                       MyModId, InstId ) == -1 )
   {
        tport_destroy( &Region2 );
        logit( "et", "binder_ew: Error starting input buffer thread; exiting!\n" );
        return -1;
   }
   logit( "t", "binder_ew: Started input buffer thread.\n" );

/*------------------- setup done; start main loop ------------------------*/
   while (1)
   {
     do {
     /* see if a termination has been requested
      *****************************************/
        if ( tport_getflag( &Region1 ) == TERMINATE  ||
             tport_getflag( &Region1 ) == MyPID )
        {
                bndr_wrapup();
                return 0;
        }

     /* send binder_ew's heartbeat
      *************************/
        if  ( time(&timeNow) - timeLastBeat  >=  HeartbeatInt )
        {
            timeLastBeat = timeNow;
            bndr_status( TypeHeartBeat, 0, "" );
        }

     /* get the next pick-message out of the private ring & process it
      ****************************************************************/
        res = tport_getmsg( &Region2, GetLogo, nLogo,
                            &reclogo, &recsize, rec, sizeof(rec)-1 );
        switch( res )
        {
        case GET_NONE:
                break;

        case GET_TOOBIG:
                sprintf( Text,
                        "Retrieved msg[%ld] (i%u m%u t%u) too big for rec[%d]",
                         recsize, reclogo.instid, reclogo.mod, reclogo.type, sizeof(rec) );
                bndr_status( TypeError, ERR_TOOBIG, Text );
                break;

        case GET_MISS:
                sprintf( Text,
                        "Missed msg(s)  inst:%u mod:%u typ:%u  region:%ld.",
                         reclogo.instid, reclogo.mod, reclogo.type, Region2.key);
                bndr_status( TypeError, ERR_MISSMSG, Text );

        case GET_NOTRACK:
                if(res == GET_NOTRACK)
                {
                   sprintf( Text,
                           "Msg received (i%u m%u t%u); transport.h NTRACK_GET exceeded",
                            reclogo.instid, reclogo.mod, reclogo.type );
                   bndr_status( TypeError, ERR_NOTRACK, Text );
                }
        case GET_OK:
                rec[recsize] = '\0';                /* null-terminate the message */
                if( reclogo.type == TypePickSCNL )
                {
                   logit( "", "\n%s", rec );          /* Print card for debugging */
                   if (!bndr_rdpickscnl( &pk, rec ))  /* decode pick              */
                       break;                         /* skip it if it had errors */
                   bind_pick( &pk );          /* try to associate and/or stack it */
                }
                else 
                {
                   logit( "", "\nUnexpected message type:%d\n%s", 
                          (int) reclogo.type, rec );  /* Print card for debugging */
                }
                break;
        }
        fflush( stdout );
     } while ( res != GET_NONE );  /* end of message-processing loop */

     sleep_ew( 500 );              /* wait for more picks to show up */
   }
}

/******************************************************************************
 *      bndr_config() processes command file(s) using kom.c functions         *
 *                    exits if any errors are encountered                     *
 ******************************************************************************/
void bndr_config( char *configfile )
{
   int      ncommand;     /* # of required commands you expect to process   */
   char     init[10];     /* init flags, one byte for each required command */
   int      nmiss;        /* number of required commands that were missed   */
   char    *com;
   char    *str;
   char     processor[15];
   int      nfiles;
   int      success;
   int      i;

/* Set to zero one init flag for each required command
 *****************************************************/
   ncommand = 5;
   for( i=0; i<ncommand; i++ )  init[i] = 0;
   nLogo    = 0;

/* Set arguments of optional commands to default values
 ******************************************************/
   strcpy( BufferRing,  "BINDER_RING" ); 
   strcpy( EventIdFile, "quake_id.d"  );

/* Open the main configuration file
 **********************************/
   nfiles = k_open( configfile );
   if ( nfiles == 0 ) {
        logit( "e",
                "binder_ew: Error opening command file <%s>; exiting!\n",
                 configfile );
        exit( -1 );
   }
   logit("","binder_ew: opened command file <%s>\n", configfile );

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
                          "binder_ew: Error opening command file <%s>; exiting!\n",
                           &com[1] );
                  exit( -1 );
               }
               logit("","binder_ew: opened command file <%s>\n", &com[1] );
               continue;
            }

        /* Process anything else as a command
         ************************************/
  /*0*/     if( k_its("HeartbeatInt") ) {
                HeartbeatInt = k_int();
                strcpy( processor, "bndr_config" );
                init[0] = 1;
            }
  /*1*/     else if( k_its("LogFile") ) {
                LogSwitch = k_int();
                strcpy( processor, "bndr_config" );
                init[1] = 1;
            }
  /*2*/     else if( k_its("MyModuleId") ) {
                str = k_str();
                if(str) strcpy( MyModName, str );
                strcpy( processor, "bndr_config" );
                init[2] = 1;
            }
  /*3*/     else if( k_its("RingName") ) {
                str = k_str();
                if( !str || strlen(str)>=MAX_RING_STR ) {
                   logit( "e", "binder_ew: Invalid length ring name in "
                          "<RingName> command; exiting!\n" );
                   exit( -1 );
                }
                strcpy( RingName, str );
                strcpy( processor, "bndr_config" );
                init[3] = 1;
            }
         /* Enter installation & module to get picks & codas from
          *******************************************************/
  /*4*/     else if( k_its("GetPicksFrom") ) {
                if ( nLogo >= MAXLOGO ) {
                    logit( "e",
                            "binder_ew: Too many <GetPicksFrom> commands in <%s>",
                             configfile );
                    logit( "e", "; max=%d; exiting!\n", (int) MAXLOGO );
                    exit( -1 );
                }
                if( ( str=k_str() ) ) {
                   if( GetInst( str, &GetLogo[nLogo].instid ) != 0 ) {
                       logit( "e",
                               "binder_ew: Invalid installation name <%s>", str );
                       logit( "e", " in <GetPicksFrom> cmd; exiting!\n" );
                       exit( -1 );
                   }
                }
                if( ( str=k_str() ) ) {
                   if( GetModId( str, &GetLogo[nLogo].mod ) != 0 ) {
                       logit( "e",
                               "binder_ew: Invalid module name <%s>", str );
                       logit( "e", " in <GetPicksFrom> cmd; exiting!\n" );
                       exit( -1 );
                   }
                }
                if( GetType( "TYPE_PICK_SCNL", &GetLogo[nLogo].type ) != 0 ) {
                    logit( "e",
                               "binder_ew: Invalid message type <TYPE_PICK_SCNL>" );
                    logit( "e", "; exiting!\n" );
                    exit( -1 );
                }
            /*    printf("GetLogo[%d] instid:%d module:%d type:%d\n",
                        nLogo, (int) GetLogo[nLogo].instid,
                               (int) GetLogo[nLogo].mod,
                               (int) GetLogo[nLogo].type ); */  /*DEBUG*/
                nLogo++;
                init[4] = 1;
            }
 
         /* Optional: override default BufferRing name */
 /*opt*/    else if( k_its("BufferRing") ) {
                str = k_str();
                if( !str || strlen(str)>=MAX_RING_STR ) {
                   logit( "e", "binder_ew: Invalid length ring name in "
                          "<BufferRing> command; exiting!\n" );
                   exit( -1 );
                }
                strcpy( BufferRing, str );
                strcpy( processor, "bndr_config" );
            }

         /* Optional: override default eventid file name */
            else if( k_its("EventIdFile") ) {
                str = k_str();
                if( !str || strlen(str)>=MAX_DIR_LEN ) {
                   logit( "e", "binder_ew: Invalid length file name in "
                          "<EventIdFile> command; exiting!\n" );
                   exit( -1 );
                }
                strcpy( EventIdFile, str );

             /* Open eventid file for command-processing */
                success = nfiles+1;
                nfiles  = k_open( EventIdFile );
                if( nfiles != success ) {
                   logit( "e",
                          "binder_ew: Error opening command file <%s>; exiting!\n",
                           EventIdFile );
                   exit( -1 );
                }
                logit("","binder_ew: opened command file <%s>\n", EventIdFile );
                continue;
            }
            else if( hyp_com()    ) strcpy( processor, "hyp_com"  );
            else if( t_com()      ) strcpy( processor, "t_com"    );
            else if( bind_com()   ) strcpy( processor, "bind_com" );
            else if( assess_com() ) strcpy( processor, "assess_com" );
            else if( grid_com()   ) strcpy( processor, "grid_com" );
            else if( site_com()   ) strcpy( processor, "site_com" );
            else {
                logit("e", "binder_ew: <%s> Unknown command in <%s>.\n",
                        com, configfile );
                continue;
            }

        /* See if there were any errors processing the command
         *****************************************************/
            if( k_err() ) {
               logit( "e", "binder_ew: Bad <%s> command for %s() in <%s>; exiting!\n",
                        com, processor, configfile );
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
       logit( "e", "binder_ew: ERROR, no " );
       if ( !init[0] )  logit( "e", "<HeartbeatInt> " );
       if ( !init[1] )  logit( "e", "<LogFile> "      );
       if ( !init[2] )  logit( "e", "<MyModuleId> "   );
       if ( !init[3] )  logit( "e", "<RingName> "     );
       if ( !init[4] )  logit( "e", "<GetPicksFrom> " );
       logit( "e", "command(s) in <%s>; exiting!\n", configfile );
       exit( -1 );
   }

/* Make sure bind/stacking info is initialized
 **********************************************/
   bind_init();
   grid_init();  
   return;
}

/*********************************************************************************/
/*  bndr_lookup( ) Look up important info from earthworm.h tables                */
/*********************************************************************************/
void bndr_lookup( )
{
/* Look up keys to shared memory regions of interest
   *************************************************/
   if( (RingKey = GetKey(RingName)) == -1 ) {
        logit("e", "binder_ew: RingName <%s> not in earthworm.d;"
              " exiting!\n", RingName );
        exit( -1 );
   }
   if( (BufferKey = GetKey(BufferRing)) == -1 ) {
        logit("e", "binder_ew: BufferRing <%s> not in earthworm.d;"
              " exiting!\n", BufferRing );
        exit( -1 );
   }

/* Look up installations of interest
   *********************************/
   if ( GetLocalInst( &InstId ) != 0 ) {
      logit("e", "binder_ew: error getting local installation id; exiting!\n" );
      exit( -1 );
   }

/* Look up modules of interest
   ***************************/
   if ( GetModId( MyModName, &MyModId ) != 0 ) {
      logit("e", "binder_ew: Invalid module name <%s>; exiting!\n", MyModName );
      exit( -1 );
   }

/* Look up message types of interest
   *********************************/
   if ( GetType( "TYPE_HEARTBEAT", &TypeHeartBeat ) != 0 ) {
      logit("e", "binder_ew: Invalid message type <TYPE_HEARTBEAT>; exiting!\n" );
      exit( -1 );
   }
   if ( GetType( "TYPE_ERROR", &TypeError ) != 0 ) {
      logit("e", "binder_ew: Invalid message type <TYPE_ERROR>; exiting!\n" );
      exit( -1 );
   }
   if ( GetType( "TYPE_QUAKE2K", &TypeQuake2K ) != 0 ) {
      logit("e", "binder_ew: Invalid message type <TYPE_QUAKE2K>; exiting!\n" );
      exit( -1 );
   }
   if ( GetType( "TYPE_LINK", &TypeLink ) != 0 ) {
      logit("e", "binder_ew: Invalid message type <TYPE_LINK>; exiting!\n" );
      exit( -1 );
   }
   if ( GetType( "TYPE_PICK_SCNL", &TypePickSCNL ) != 0 ) {
      logit("e", "binder_ew: Invalid message type <TYPE_PICK_SCNL>; exiting!\n" );
      exit( -1 );
   }

   return;
}

/******************************************************************************/
/*  bndr_rdpickscnl() load a TYPE_PICK_SCNL message into binder's PICK struct */
/******************************************************************************/
int bndr_rdpickscnl( PICK *pk, char *msg )
{
   EWPICK   pick;
   int      isite;
    
/* Read the pick into an EWPICK struct
 *************************************/
   if( rd_pick_scnl( msg, strlen(msg), &pick ) != EW_SUCCESS )
   {
      sprintf( Text, "Error reading pick: %s", msg );
      bndr_status( TypeError, ERR_PICKREAD, Text );
      return( 0 );
   }

/* Get site index (isite)
 ************************/
   isite = site_index( pick.site, pick.net, pick.comp, pick.loc );
   if( isite < 0 )
   {
      sprintf( Text, "%s.%s.%s.%s - Not in station list.", 
               pick.site, pick.comp, pick.net, pick.loc );
      bndr_status( TypeError, ERR_UNKNOWN_STA, Text );
      return( 0 );
   }

/* Load all interesting info into binder_ew's PICK structure
 ***********************************************************/
   pk->t      = pick.tpick + (double)GSEC1970; /* use gregorian time            */
   pk->instid = pick.instid;                   /* preserve pick's source instid */
   pk->src    = pick.modid;                    /* preserve pick's source module */
   pk->seq    = pick.seq;                      /* Preserve original sequence#   */
   pk->quake  = 0;                             /* Not associated yet            */
   pk->site   = isite;
   pk->phase  = 0;                             /* Assume P wave for now         */
   pk->ie     = ' ';
   pk->fm     = pick.fm;
   pk->wt     = pick.wt;
   pk->lpick  = 0;

   return ( 1 );
}


/******************************************************************************/
/* bndr_quake2k() builds a TYPE_QUAKE2K message & puts it into shared memory  */
/******************************************************************************/
void bndr_quake2k( long quake )
{
        MSG_LOGO  logo;
        char      msg[256];
        char      stime[18];
        long      size;
        int       iq;
        int       res;

        logo.instid = InstId;
        logo.mod    = MyModId;
        logo.type   = TypeQuake2K;

        iq = quake % mQuake;
        date17( pQuake[iq].t, stime );
        sprintf( msg,
                "%d %d %ld %17s %8.4f %9.4f %6.2f %5.2f %5.1f %5.1f %3.0f %hd\n",
                (int) InstId,
                (int) MyModId,
                quake,
                stime,
                pQuake[iq].lat,
                pQuake[iq].lon,
                pQuake[iq].z,
                pQuake[iq].rms,
                pQuake[iq].dmin,
                pQuake[iq].ravg,
                pQuake[iq].gap,
                pQuake[iq].npix );

/*      logit("t", "TYPE_QUAKE2K: %s", msg ); */  /*DEBUG*/

        size = strlen( msg );   /* don't include the null byte in the message */
        res = tport_putmsg( &Region1, &logo, size, msg );

        switch(res) {
        case PUT_NOTRACK:
                sprintf( Text,
                        "QUAKE msg not sent, NTRACK_PUT exceeded." );
                bndr_status( TypeError, ERR_NOTRACK, Text );
                break;
        case PUT_TOOBIG:
                sprintf( Text,
                        "QUAKE msg[%ld] not sent, too big for region:%ld",
                         size, Region1.key );
                bndr_status( TypeError, ERR_TOOBIG, Text );
        case PUT_OK:
                break;
        }
        return;
}

/******************************************************************************/
/*  bndr_link() builds a TYPE_LINK message and puts it into shared memory     */
/******************************************************************************/
void bndr_link( long quake, long pick )
{
        MSG_LOGO  logo;
        char      msg[256];
        long      size;
        int       ip;
        int       res;

        logo.instid = InstId;
        logo.mod    = MyModId;
        logo.type   = TypeLink;

        ip = pick % mPick;

        sprintf( msg,
                "%ld %u %u %d %d\n",
                 quake,
                 pPick[ip].instid,
                 pPick[ip].src,
                 pPick[ip].seq,
                 pPick[ip].phase );

/*      logit( "t", "TYPE_LINK:  %s", msg ); */          /*DEBUG*/

        size = strlen( msg );
        res = tport_putmsg( &Region1, &logo, size, msg );

        switch(res) {
        case PUT_NOTRACK:
                sprintf( Text,
                        "LINK msg not sent, NTRACK_PUT exceeded." );
                bndr_status( TypeError, ERR_NOTRACK, Text );
                break;
        case PUT_TOOBIG:
                sprintf( Text,
                        "LINK msg[%ld] not sent, too big for region:%ld",
                         size, Region1.key );
                bndr_status( TypeError, ERR_TOOBIG, Text );
        case PUT_OK:
                break;
        }
        return;
}

/******************************************************************************/
/* bndr_status() builds a heartbeat or error msg & puts it into shared memory */
/******************************************************************************/
void bndr_status( unsigned char type,  short ierr,  char *note )
{
        MSG_LOGO    logo;
        char        msg[256];
        long        size;
        time_t      t;

        logo.instid = InstId;
        logo.mod    = MyModId;
        logo.type   = type;

        time( &t );
        if( type == TypeHeartBeat ) {
                sprintf( msg, "%ld %d\n", (long) t, MyPID );
        }
        else if( type == TypeError ) {
                sprintf( msg, "%ld %hd %s\n", (long) t, ierr, note );
                logit( "t", "binder_ew:  %s\n", note );
        }

        size = strlen( msg );   /* don't include the null byte in the message */
        if( tport_putmsg( &Region1, &logo, size, msg ) != PUT_OK )
        {
                if( type == TypeHeartBeat ) {
                    logit("et","binder_ew:  Error sending heartbeat.\n" );
                }
                else if( type == TypeError ) {
                    logit("et","binder_ew:  Error sending error:%d.\n", ierr );
                }
        }
        return;
}

/**************************************************/
/* bndr_wrapup() cleans up things before exiting */
/**************************************************/
void bndr_wrapup( void )
{
        logit( "t", "binder_ew: Termination requested; cleaning up..." );
        tport_putflag( &Region2, TERMINATE ); /* notify buffering thread */
        sleep_ew( 1000 );           /* give buffering thread time to end */
        hyp_free();
        grid_free();
        tport_detach ( &Region1 );
        tport_destroy( &Region2 );
        logit( "", "exiting!\n" );
        return;
}
