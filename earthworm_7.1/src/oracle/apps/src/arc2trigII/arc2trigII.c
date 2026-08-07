/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: arc2trigII.c,v 1.9 2002/06/07 15:46:09 patton Exp $
 *
 *    Revision history:
 *     $Log: arc2trigII.c,v $
 *     Revision 1.9  2002/06/07 15:46:09  patton
 *     Made logit changes.
 *
 *     Revision 1.8  2002/03/25 16:58:30  lucky
 *     Fixed logit statements in debug mode which caused crashes (incorrect number of
 *     parameters to a logit)
 *
 *     Revision 1.7  2002/02/02 00:00:16  alex
 *     debug messages for mysery start times. Alex
 *
 *     Revision 1.6  2001/11/21 17:12:36  alex
 *     fixed snippets from preceeding event. Alex
 *
 *     Revision 1.5  2001/07/01 21:55:12  davidk
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
 *     Revision 1.4  2001/05/15 02:15:04  davidk
 *     Moved functions around between the apps, DB API, and DB API INTERNAL
 *     levels.  Renamed functions and files.  Added support for amplitude
 *     magnitude types.  Reformatted makefiles.
 *
 *     Revision 1.3  2001/05/10 20:09:26  dietz
 *     Changed to shut down gracefully if the transport flag is
 *     set to TERMINATE or MyPid.
 *
 *     Revision 1.2  2001/03/07 18:51:33  alex
 *     *** empty log message ***
 *
 *     Revision 1.1  2001/03/07 00:05:04  alex
 *     Initial revision
 *
 *     Revision 1.6  2000/12/06 18:35:07  lucky
 *     Changed Pph (which no longer exists) to Plabel in Hpck structure
 *
 *     Revision 1.5  2000/08/21 20:01:02  lucky
 *     Fixed to use new Hpck structure which can save both P and S picks
 *
 *     Revision 1.4  2000/08/08 18:26:14  lucky
 *     Lint cleanup
 *
 *     Revision 1.3  2000/07/24 20:15:42  lucky
 *     Implemented global limits to module, installation, ring, and message type strings.
 *
 *     Revision 1.2  2000/03/30 15:37:31  davidk
 *     changed the duration of the trace to be saved.  The trace was
 *     begin saved from (PickTime - PreTime) to (PickTime - PreTime +
 *     codalen + PostTime).  The duration was changed so that trace would
 *     be saved until (PickTime + codalen + PostTime).  This fixes a
 *     problem where not enough trace was being saved, and so the entire coda
 *     was usually not captured.
 *
 *     Revision 1.1  2000/02/14 16:04:49  lucky
 *     Initial revision
 *
 *
 */

/*
 * arc2trigII.c:

Derived from, you guessed it, arc2trig. The enhancements are to save snippets of interest which 
were not listed in the arc message. This is done as follows:

I. Get the coordinates of mandatory stations. Either from the station list, or by a search of the DBMS.

II. For each event:
  1) Select the relevant stations:
		a) All stations within the distance implied by the magnitude-distance table. This can be from 
		a hypoinverse station list or the DBMS. If from a station list, this will have been read 
		once at startup. If from the DBMS, the DBMS will be queried for stations within the appropriate 
		rectangle for this event.

		b) All mandatory stations.

	  2) For each selected station, compute the snippet to be saved:
		a) Within a specified epicentral distance, ignore the travel time table, and save all data from 
		origin time through S arrival time.
		b) Outside this distance, use the travel-time table as in usnsn_loc2trig, and use that to compute snippet times.
		c) All snippets will be padded with a specified safety margin of pre and post computed snippets times.
  Alex 2/21/1
----------------------------------------------

Takes a hypo arc message as input and produces a .trg file
alex 8/5/97

So it says, but I think Lynn wrote this module. I (Alex) am modifying it to do
the author field, 6/16/98:

Story: The grand idea is that messages which relate to an event shall include
an author field. In particular, the trigger message shall have include, after
the "EVENT ID:", an "AUTHOR:", which is followed by the author id. The author
id is to be a series of logos, separated by :'s. Each logo is represented as
three sets of three ascii digits. The first logo is the logo of the module
which originated this event, followed by the logos of the modules which
processed it.

So hypoinverse (eqproc) created the event by emitting an arc message. It
should have stuck its logo in the author field. It doesn't, because we don't
have the stomach to change eqproc. So we make up for it here: we know the logo
of the message as it came to us, and we know our own module id, so we write
eqproc's logo and our own into the trigger message which we produce.

Mon Nov  9 13:21:16 MST 1998 lucky:

      Log file name will be built from the name of the configuration
      file -- argv[1] passed to logit_init().

      Incoming transport ring is flushed at start time to prevent 
      confusion if the module is restarted.

      Process id is sent with the heartbeat for restart purposes.

      ===================== Y2K compliance =====================
      Formats in read_hyp() and read_phs() changed (among other
      things)to include date in the form YYYYMMDD. 

      make_datestr() changed to create date string YYYYMMDD...
      
      calls to julsec15() and date15() replaced by julsec17() and
      date17() respectively (as the Y2K equivalents of old calls)

      Message type names changed to their Y2K equivalents.  */
/* More changes: 12/9/1998, Pete Lombard
   Yanked read_hyp() and read_ph() out to use new versions in 
   libsrc/util/read_arc.c

   Alex Nov 23 99:
	The amount of pre-pick data to be saved is now an optional
   	configuration file parameter. Also, default time changed from 
	5 seconds to 15 seconds.
   */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <chron3.h>
#include <earthworm.h>
#include <trace_buf.h>
#include <read_arc.h>
#include <kom.h>
#include <transport.h>
#include <ewdb_ora_api.h> 
#include <make_triglist.h> 
#include "arc2trigII.h"

/* Time constants
*****************/
/* These variables are set here, but can be changed via
   optional coniguration file commands */
static double PreTime = 15; /* seconds to save before pick time */
static double PostTime = 10; /* seconds past coda or tau time */

static  SHM_INFO  InRegion;      /* shared memory region to use for input  */
static  SHM_INFO  OutRegion;     /* shared memory region to use for output */

#define   MAXLOGO   2
MSG_LOGO  GetLogo[MAXLOGO];    /* array for requesting module,type,instid */
short     nLogo;
MSG_LOGO  PutLogo;  /* outgoing logo                         */

static char ArcMsgBuf[MAX_BYTES_PER_EQ]; /* character string to hold event message
                                        MAX_BYTES_PER_EQ is from earthworm.h */

static char TrigMsgBuf[MAX_TRIG_BYTES]; /* to hold the trigger message / file */

/* Things to read or derive from configuration file
 **************************************************/
static char		InRingName[MAX_RING_STR];   /* name of transport ring for input  */
static char		OutRingName[MAX_RING_STR];  /* name of transport ring for output */
static char		MyModName[MAX_MOD_STR];		/* speak as this module name/id      */
static int		LogSwitch;					/* 0 if no logfile should be written */
static long		HeartBeatInterval;			/* seconds between heartbeats        */
/* DB Stuff */
static char		DBservice[SHORT_STR];		/* name of DB service ala eqs.usgs */
static char		DBpassword[SHORT_STR];      
static char		DBuser[SHORT_STR];
static int		DBtimeoutSeconds;			/* if the db doesn't reply in this many seconds, bad news */
static int		UseDB = 2;					/* 0 => stations from file. 1=> stations from DB. 2 => sys err */
/* box from which we'll get stations */
static double	LatMax = 90;
static double	LatMin = 0;
static double	LonMax = 180;
static double	LonMin = -180
; 
static char		StaListFileName[MAX_STR];		/* name of station list */
/* Magnitude-Distance table */
static double	Mag[MAX_MAGDIST];				/* We'll save snippets within this distance for this magnitude */
static double	Dist[MAX_MAGDIST];	
static int		NmagDist = 0;					/* Number of entries in the tables */
/* Distance within which we won't bother with travel times */
static double	SaveOriginDist;
/* name of travel time table */
static char		TTTable[MEDIUM_STR];

/* The Travel time table 
************************/
#define     EARTH_CIRCUM        40000.0
#define NZMAX 100
#define NDMAX 100
static int Nz;				/* Number of depth vaules in table */
static int Nd;				/* Number of distance values in table */
static double Ddepth;			/* depth increment between table values */
static double Ddist;			/* distance increment between table values */
static double Sphase[NDMAX][NZMAX]; 	/*The S travel times */
static double Pphase[NDMAX][NZMAX]; 	/*The P travel times */

/* The Big-deal structures
**************************/
SCN_NAMES			MandSta[MAX_MAND_STA];		/* List of mandatory stations, as read from config file */
static int			NMand = 0;					/* First empty entry */
SEL_STA				SelSta[MAX_STA];			/* Snippet attributes for each element of StationList */
EWDB_StationStruct	StationList[MAX_STA];		/* Array of DB-type station structures */
static int			NStationList = 0;			/* First empty entry */

/* Things to look up in the earthworm.h tables with getutil.c functions
 **********************************************************************/
static long          InRingKey;      /* key of transport ring for input   */
static long          OutRingKey;     /* key of transport ring for output  */
static unsigned char InstId;         /* local installation id             */
static unsigned char MyModId;        /* Module Id for this program        */
static unsigned char TypeHeartBeat;
static unsigned char TypeError;
static unsigned char TypeHyp2000Arc;
static unsigned char TypeH71Sum2k;
static unsigned char TypeTrigList;

/* Error messages used by arc2trig
 *********************************/
#define  ERR_MISSMSG       0   /* message missed in transport ring       */
#define  ERR_TOOBIG        1   /* retreived msg too large for buffer     */
#define  ERR_NOTRACK       2   /* msg retreived; tracking limit exceeded */
#define  ERR_FILEIO        3   /* error opening trigger file             */
#define  ERR_DECODEARC     4   /* error reading input archive message    */
#define  ERR_TRIGMSG       5   /* error creating TrigList message        */
static char  Text[150];        /* string for log/error messages          */

/* Structures for hyp2000 data */
struct Hsum Sum;               /* Hyp2000 summary data                   */
struct Hpck Pick;              /* Hyp2000 pick structure                 */

pid_t	MyPid; 	/** Out process id is sent with heartbeat **/

int DDebug = 0;

/** length of string required by make_datestr  **/
#define	DATESTR_LEN		22	

int main( int argc, char **argv )
{
	long      timeNow;          /* current time                  */
	long      timeLastBeat;     /* time last heartbeat was sent  */
	long      recsize;          /* size of retrieved message     */
	MSG_LOGO  reclogo;          /* logo of retrieved message     */
	int       res;
	int		 ret;
	int		i;
  int			NStationListFound;			/* Number of stations found in the DB,
                                     not number retrieved.               */

	
	/* Check command line arguments
	******************************/
	if ( argc != 2 )
	{
        fprintf( stderr, "Usage: arc2trig <configfile>\n" );
        exit( 0 );
	}

	/* Initialize name of log-file & open it
	***************************************/
	logit_init( argv[1], 0, 256, 1 );

	/* Read the configuration file(s)
	********************************/
	printf("calling arc2trig_config\n");
	arc2trig_config( argv[1] );
	printf("returning arc2trig_config\n");
	logit( "" , "%s: Read command file <%s>\n", argv[0], argv[1] );
	
	/* Look up important info from earthworm.h tables
	************************************************/
	arc2trig_lookup();
	
	/* Reinitialize logit to desired logging level
	**********************************************/
	logit_init( argv[1], (short) MyModId, 256, LogSwitch );
	
	/* Get our own process ID for restart purposes
	**********************************************/
	if ((MyPid = getpid ()) == -1)
	{
		logit ("e", "arc2trig: Call to getpid failed. Exiting.\n");
		exit (-1);
	}

	/* load outgoing logo
	*********************/
	PutLogo.instid = InstId;
	PutLogo.mod    = MyModId;
	PutLogo.type   = TypeTrigList;
	
	/* Initialize the trigger file
	*****************************/
	if( writetrig_init(TrigFileBase, OutputDir) != 0 ) 
	{
		logit("", "arctrig: error opening file in <%s>",OutputDir);
		logit("", "                          with BaseName  <%s>\n",TrigFileBase );
		exit( -1 );
	}
	logit( "", "arc2trig: Writing trigger files in %s\n", OutputDir );
	
	/* Attach to Input shared memory ring
	************************************/
	tport_attach( &InRegion, InRingKey );
	logit( "", "arc2trig: Attached to public memory region %s: %d\n", InRingName, InRingKey );
	
	/* Attach to Output shared memory ring
	*************************************/
	tport_attach( &OutRegion, OutRingKey );
	logit( "", "arc2trig: Attached to public memory region %s: %d\n",
		OutRingName, OutRingKey );
	
	/* Issue a heartbeat
	********************/
	arc2trig_status( TypeHeartBeat, 0, "" );

	/* Force a heartbeat to be issued in first pass thru main loop
	*************************************************************/
	timeLastBeat = time(&timeNow) - HeartBeatInterval - 1;

	/* Get the station list
	***********************/
	if(DDebug) logit("e","UseDB:%d\n",UseDB);
	if(UseDB == 1) 
	{
		logit("e","Getting station list from DB\n");
		/* connect to db 
		****************/
		ret = ewdb_api_Init( DBuser, DBpassword, DBservice);
		if (ret != EWDB_RETURN_SUCCESS ) 
		{
			logit("e","Error connecting to DBMS: %d. Fatal.\n",ret);
			exit (-1);
		}
	
		/* Get the station list from db
		*******************************/
		if(DDebug) logit("","LatMin: %f, LatMax: %f, LonMin: %f, LonMax: %f \n",LatMin,LatMax, LonMin,LonMax); 
		ret = ewdb_api_GetStationList( LatMin,LatMax, LonMin,LonMax, 
				(double)time(NULL),	StationList,  &NStationListFound, &NStationList, MAX_STA );
		if (ret == EWDB_RETURN_FAILURE || NStationList == MAX_STA)
			{
				logit ("e", "Call to P3DB_GetStationList failed -- %d %d %d\n",
					    ret, MAX_STA, NStationList);
				goto shutdown;
			}
	}
	else  
	{
		logit("e","Getting station list from hypoinverse file\n");

		/* get station list from hypoinverse file 
		*****************************************/
		ret = sonOfSiteRead( StaListFileName );
		if (ret != 1)
		{
			logit("e","Error getting station list: %d. Exiting",ret);
			goto shutdown;
		}
	}

	/* Dump station list
	********************/
	if(DDebug)
	{
		int i;
		logit("e","NStationList:%d\n",NStationList);
		for(i=0;i<NStationList;i++)
		{
			logit("e",".%s.%s.%s.\n",StationList[i].Sta, StationList[i].Comp, StationList[i].Net);
		}
	}
	
	/* Flush the incoming transport ring on startup
	***********************************************/
	while (tport_getmsg (&InRegion, GetLogo, nLogo,  &reclogo,
		&recsize, ArcMsgBuf, sizeof(ArcMsgBuf)- 1) != GET_NONE)
		;
	
	/*----------------------- start main loop --------------------------------*/
	while(1)
	{
	/* send arc2trig's heartbeat
	***************************/
		if  ( time(&timeNow) - timeLastBeat  >=  HeartBeatInterval )
		{
			timeLastBeat = timeNow;
			arc2trig_status( TypeHeartBeat, 0, "" );
		}
		
		/* Process all new messages
		**************************/
		do
		{
			/* see if a termination has been requested
			*****************************************/
			if ( tport_getflag( &InRegion ) == TERMINATE  ||
                             tport_getflag( &InRegion ) == MyPid  )

			{
				writetrig_close();
				/* detach from shared memory */
				tport_detach( &InRegion );
				/* write a termination msg to log file */
				logit( "t", "arc2trig: Termination requested; exiting!\n" );
				fflush( stdout );
				goto shutdown ;
			}
			
			/* Get msg & check the return code from transport
			************************************************/
			res = tport_getmsg( &InRegion, GetLogo, nLogo,
				&reclogo, &recsize, ArcMsgBuf, sizeof(ArcMsgBuf)-1 );
			
			if( res == GET_NONE )          /* no more new messages     */
			{
				break;
			}
			else if( res == GET_TOOBIG )   /* next message was too big */
			{                              /* complain and try again   */
				sprintf(Text,
					"Retrieved msg[%ld] (i%u m%u t%u) too big for ArcMsgBuf[%d]",
					recsize, reclogo.instid, reclogo.mod, reclogo.type,
					sizeof(ArcMsgBuf)-1 );
				arc2trig_status( TypeError, ERR_TOOBIG, Text );
				continue;
			}
			else if( res == GET_MISS )     /* got a msg, but missed some */
			{
				sprintf( Text,
					"Missed msg(s)  i%u m%u t%u  %s.",
					reclogo.instid, reclogo.mod, reclogo.type, InRingName );
				arc2trig_status( TypeError, ERR_MISSMSG, Text );
			}
			else if( res == GET_NOTRACK ) /* got a msg, but can't tell */
			{                             /* if any were missed        */
				sprintf( Text,
					"Msg received (i%u m%u t%u); transport.h NTRACK_GET exceeded",
					reclogo.instid, reclogo.mod, reclogo.type );
				arc2trig_status( TypeError, ERR_NOTRACK, Text );
			}
			
			/* Process the message
			*********************/
			ArcMsgBuf[recsize] = '\0';      /*null terminate the message*/
			
			if( reclogo.type == TypeHyp2000Arc )
			{
				int ret;
				/* Initialize things
				********************/
				for (i=0; i<MAX_STA; i++)
				{
					SelSta[i].selected=0;
					SelSta[i].startTime=0;
					SelSta[i].duration=0;
				}
				/*build the trigger message
				***************************/
				ret   = arc2trigII_hinvarc( ArcMsgBuf, TrigMsgBuf, reclogo );
				if(ret) 
				{
					arc2trig_status( TypeError, ERR_DECODEARC, "" );
					continue;
				}
				/* Write trigger message to output ring
				**************************************/
				if(DDebug) logit("e","strlen(TrigMsgBuf): %d\n",strlen(TrigMsgBuf));
				if( tport_putmsg( &OutRegion, &PutLogo, strlen(TrigMsgBuf), TrigMsgBuf ) != PUT_OK )
				{
					logit("et","arc2trig: Error writing trigger message to ring.\n" );
				}
			}
		} while( res != GET_NONE );  /*end of message-processing-loop */
		
		sleep_ew( 1000 );  /* no more messages; wait for new ones to arrive */
		
	}
	/*-----------------------------end of working loop-------------------------------*/
shutdown:
	/* Close down DB connection
	***************************/
	if(UseDB == 1) ret = ewdb_api_Shutdown();
	if( ret == EWDB_RETURN_FAILURE ) logit("","error from OraAPIShutdown\n");
	exit(0);

}
/************************ end of main *********************************************/

/*******************************************************************
*  arc2trigII_hinvarc( )  read a Hypoinverse archive message        *
*                       and build a trigger message               *
*******************************************************************/
int arc2trigII_hinvarc( char* arcmsg, char* trigMsg, MSG_LOGO incoming_logo )
{
	char     *in;             /* working pointer to archive message    */
	char      line[MAX_STR];  /* to store lines from msg               */
	char      shdw[MAX_STR];  /* to store shadow cards from msg        */
	int       msglen;         /* length of input archive message       */
	int       nline;          /* number of lines (not shadows) so far  */
	int		  ret;
	
	/* Initialize some stuff
	***********************/
	nline  = 0;
	msglen = strlen( arcmsg );
	in     = arcmsg;
	
	/* Read one data line and its shadow at a time from arcmsg; process them
	***********************************************************************/
	while( in < arcmsg+msglen )
	{
		if ( sscanf( in, "%[^\n]", line ) != 1 )  return( -1 );
		in += strlen( line ) + 1;
		if ( sscanf( in, "%[^\n]", shdw ) != 1 )  return( -1 );
		in += strlen( shdw ) + 1;
		nline++;
		/*logit( "e", "%s\n", line );*/  /*DEBUG*/
		/*logit( "e", "%s\n", shdw );*/  /*DEBUG*/
		
		/* Process the hypocenter card (1st line of msg) & its shadow
		************************************************************/
		if( nline == 1 ) {                /* hypocenter is 1st line in msg  */
			read_hyp( line, shdw, &Sum );
			/* write 1st part of trigger msg
			********************************/
			bldtrig_hyp( trigMsg, incoming_logo, PutLogo);        
			continue;
		}
		
		/* Process all the phase cards & their shadows
		*********************************************/
		/* We'll certainly include snippets for all stations in the arc message, 
		as was done in the original arc2trig */
		if( strlen(line) < (size_t) 75 )  /* found the terminator line      */
			break;
		read_phs( line, shdw, &Pick );    /* load info into Pick structure   */

		/* Enter snippet parameters for picked stations into Selected Stations array
		****************************************************************************/
		ret = phs2SelSta(Pick);

	} /*end while over reading message*/

	/* Collect "mandatory stations" 
	********************************/
	ret = MandSta2Selected();
	if(DDebug) logit("e","return from MandSta2Selected: %d\n",ret);
	if (ret <0) arc2trig_status( TypeError, ERR_TRIGMSG, "Error selecting mand. sta" );
	
	/* Collect all stations within proscribed epicentral distance 
	*************************************************************/
	ret = MagDist2SelSta( );
	if(DDebug) logit("e","%d stations within critical distance\n",ret);

	/* compute snippet start time and duration for selected stations
	****************************************************************/
	ret = SetTimesInSelSta();
	if(DDebug) logit("e","return from SetTimesInSelSta: %d\n",ret);

	/* Write selected stations to the trigger message
	*************************************************/
	ret = SelSta2TrigMsg( trigMsg );
	if(DDebug) logit("e","return from SelSta2TrigMsg: %d\n",ret);
	
	/* Write trigger message to trigger file
    ***************************************/
	if( writetrig("\n", TrigFileBase, OutputDir) != 0 )  /* add a blank line before trigger list */
	{
		arc2trig_status( TypeError, ERR_FILEIO, "Error opening trigger file" );
	}
	if( writetrig(trigMsg, TrigFileBase, OutputDir) != 0 )
	{
		arc2trig_status( TypeError, ERR_FILEIO, "Error opening trigger file" );
	}
	if( writetrig("\n", TrigFileBase, OutputDir) != 0 )  /* add a blank line after trigger list */
	{
		arc2trig_status( TypeError, ERR_FILEIO, "Error opening trigger file" );
	}
	
	return(0);
}

/**************************************************************
* bldtrig_hyp() builds the EVENT line of a trigger message   *
* Modified for author id by alex 7/10/98                     *
**************************************************************/
void bldtrig_hyp( char *trigmsg, MSG_LOGO incoming, MSG_LOGO outgoing)
{
	char datestr[DATESTR_LEN];
	
	/* Sample EVENT line for trigger message:
	EVENT DETECTED     970729 03:01:13.22 UTC EVENT ID:123456 AUTHOR: asdf:asdf\n
	0123456789 123456789 123456789 123456789 123456789 123456789
	************************************************************/
	make_datestr( Sum.ot, datestr );
	sprintf( trigmsg, "EVENT DETECTED     %s UTC EVENT ID: %u AUTHOR: %03d%03d%03d:%03d%03d%03d\n\n", 
		datestr, (int) Sum.qid,
		incoming.type, incoming.mod, incoming.instid,
		outgoing.type, outgoing.mod, outgoing.instid );
	strcat ( trigmsg, "Sta/Cmp/Net   Date   Time                       start save       duration in sec.\n" );
	strcat ( trigmsg, "-----------   ------ ---------------    ------------------------------------------\n");
	
	if(DDebug) printf( "\n%s", trigmsg ); /*DEBUG*/
	
	return ;
}
/*************************** phs2SelSta **********************************
* Given a Hpck structure, fill in an entry in the SelSta Structure:      *
* Station SCN name                                                       *
* epicentral distance,                                                   *
* snippet start and end time                                             *
* Squeeze duplicates                                                     *
**************************************************************************/
int phs2SelSta()
{
	int iSta;
	double event_duration;  /* coda duration from tau()      */
	long codalen;

	/* Loop over all known stations (groan!)
	****************************************/
	for ( iSta=0; iSta<NStationList; iSta++)
	{
		if ( match( Pick.site, Pick.comp, Pick.net, &(StationList[iSta]) )==1 )
		{
			/* mark this channel as selected
			********************************/
			SelSta[iSta].selected = 1;
			goto found;
		}
	}
	/* Didn't find the SCN name */
	logit("e","Warning: channel .%s.%s.%s. not found in station list\n",
		       Pick.site, Pick.comp, Pick.net);
	return(-1);

found:
	/* Set snippet start time
	*************************/
	SelSta[iSta].startTime = Pick.Pat-PreTime;  /*seconds since 1600 */

	/* Calculate how much to save based on the longer of
	calculated tau or picker-measured coda length
	***************************************************/
	event_duration = tau( Sum.Md );
	codalen = Pick.codalen;
	if( codalen < 0 ) codalen = -codalen;
	
	/* Include PreTime in duration time, since we are
	starting at time PickTime-PreTime 
	davidk 032400
	****************************************************/
	if( event_duration > codalen ) 
		SelSta[iSta].duration = event_duration + PostTime + PreTime;
	else 
		SelSta[iSta].duration = codalen + PostTime + PreTime;

	return(1);

}
/************************** end of phs2SelSta ****************************/
/*************************** MagDist2SelSta ******************************
* Given a magnitude, enter the stations within the proscribed distance   *
* into the  SelSta Structure:                                            *
* Just the SCN name. Someone will do the times later                     *
**************************************************************************/
int MagDist2SelSta( )
{
	int iSta;
	int i;
	int ifound = 0;
	double critDist = 0;
	double epiDist = 0;
	double azm;
	int   ret;

	/* Find critical distance for this event
	****************************************/
	for (i=0;i<NmagDist;i++)
	{
		if (Sum.Md < Mag[i]) 
		{
			critDist = Dist[i];
			break;
		}
	}
	if (critDist == 0) /* it was bigger than any specified mag */
		/* *** NOTE *** Bug fix: -1 added 2/1/2 Alex */
		critDist = Dist[NmagDist-1]; /* then use largest distance: */
	if(DDebug) logit("e","critDist: %f\n",critDist);

	/* Loop over all known stations (groan!)
	****************************************/
	for ( iSta=0; iSta<NStationList; iSta++)
	{
		/* compute epicentral distance
		******************************/
		ret = geo_to_km(Sum.lat,               Sum.lon,
						StationList[iSta].Lat, StationList[iSta].Lon,
						&epiDist, &azm);
		
		/* Mark it to be saved if its close enough
		******************************************/
		if(epiDist < critDist)
		{
			ifound++;
			SelSta[iSta].selected = 1;
			SelSta[iSta].startTime = 0; /* means: to be set by SetTimesInSelSta */
		}
	}
	return(ifound);
}
/************************* end of MagDist2SelSta ****************************/

/************************ SetTimesInSelSta **********************************
* run down the selected stations, and set snippet start times               *
* and duration for anyone who doesn't have their set as yet                 *
*****************************************************************************/
int SetTimesInSelSta()
{
	int iSta;
	double epiDist;
	double azm;
	int ret;
	int iComputed=0;

	/* run down the station list 
	****************************/
	for (iSta=0;iSta<NStationList;iSta++)
	{
		/* If this station is selected, but has no times:
		*************************************************/
		if ( SelSta[iSta].selected == 1 && SelSta[iSta].startTime == 0)
		{
			if(DDebug) 
				logit("","SetTimesInSelSta: %s %s %s --- ",
					StationList[iSta].Sta,StationList[iSta].Comp,StationList[iSta].Net);
			/* compute epicentral distance
			******************************/
			ret = geo_to_km(Sum.lat,               Sum.lon,
							StationList[iSta].Lat, StationList[iSta].Lon,
							&epiDist, &azm);
			if(DDebug) 
				logit("","distance: %0.2f ---", epiDist);
			
			/* Then compute the snippet times
			*********************************/
			if(epiDist <= SaveOriginDist)  /* its too close to worry about travel times */
			{
				SelSta[iSta].startTime = Sum.ot - PreTime;
				if(DDebug) 
					logit("","too close. ot: %d startTime: %d\n", 
						Sum.ot, SelSta[iSta].startTime );
			}
			else
			{
				SelSta[iSta].startTime = Ptime (Sum.ot, Sum.z, epiDist*(360./EARTH_CIRCUM)) - PreTime;
				if(DDebug) 
					logit("","from table. ot: %d startTime: %d\n", Sum.ot, SelSta[iSta].startTime );
			}


			SelSta[iSta].duration =  Stime (Sum.ot, Sum.z, epiDist*(360./EARTH_CIRCUM)) - 
									 Ptime (Sum.ot, Sum.z, epiDist*(360./EARTH_CIRCUM)) + PostTime;
			iComputed++;
		}
	}
	return(iComputed);
}

/************************* end of SetTimesInSelSta **************************/

/************************* SelSta2TrigMsg *******************************
* Write all selected stations to trigger message                        *
*************************************************************************/
int SelSta2TrigMsg( char* trigMsg)
{
	int iSta;
	char savet_str[DATESTR_LEN];
	char str[MEDIUM_STR];
	int iWrote;		/* bytes in outgoing message */

	iWrote = strlen(trigMsg);

	/* Run over the station list
	****************************/
	for (iSta=0; iSta<NStationList; iSta++)
	{
		if(SelSta[iSta].selected == 1)
		{
			/* Convert times in seconds since 1600 to character strings
			**********************************************************/
			make_datestr( SelSta[iSta].startTime, savet_str );

			/* Build the "phase" line:
			MCM VHZ NC N 19970729 03:01:13.34 UTC    save: yyyymmdd 03:00:12.34      120\n
			0123456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789
			********************************************************************************/
			sprintf( str, " %s %s %s %c %s UTC    save: %s %8ld\n",
				StationList[iSta].Sta, StationList[iSta].Comp, StationList[iSta].Net, 'x',
				savet_str, savet_str, (long)SelSta[iSta].duration );
			
			if(DDebug) printf( "%s",str ); /*DEBUG*/
			
			/* add to outgoing  message buffer
			**********************************/
			iWrote = iWrote + strlen(str);
			if(iWrote > MAX_TRIG_BYTES)
			{
				logit("e","Trigger message truncated! Exceeds limit: %d. \n",MAX_TRIG_BYTES);
				return(-1);
			}
			strcat( trigMsg, str );
		}
	}
	return(iWrote);
}


/******************* tau() ***************************
* Calculate tau (duration) from magnitude            *
******************************************************/
double tau( double xmag )
{
/* From Dave Oppenheimer:
*    TAU = 10.0**((XMAG - FMA)/FMB)
*  where TAU  is in seconds,
*        XMAG is the magnitude,
*        FMA  is the coda magnitude intercept coefficient
*        FMB  is the coda magnitude duration coefficient
	*/
	
	double fma   = (double) -0.87;  /* value from a paper by Bakun */
	double fmb   = (double)  2.0;   /* value from a paper by Bakun */
	
	return( pow( (double)10.0, (double) (xmag-fma)/fmb ) );
}
/************************end of tau ****************************/
/*************************** MandSta2Selected ***********************************
* Given an array of mandatory SCN names:                                        *
* Expand for wildcards.                                                         *
* Rummage through all stations known.                                           *
* Find all matching stations                                                    *
* Set pointers to them in SelSta.                                               *
********************************************************************************/
int MandSta2Selected( )
{
	int iSta;		/* running index over station list */
	int iMand;		/* running index over mandatory stations */
	int iSel = 0;		/* number of stations selected */
	
	/* Loop over mandatory stations
	*******************************/
	for ( iMand=0; iMand<NMand; iMand++)
	{
		/* Loop over all known stations (groan!)
		****************************************/
		for ( iSta=0; iSta<NStationList; iSta++)
		{
			if ( match( MandSta[iMand].sta, MandSta[iMand].comp, MandSta[iMand].net, 
				        &(StationList[iSta]) )==1 )
			{
				/* Enter this channel in selected list
				**************************************/
				SelSta[iSta].selected = 1;
				SelSta[iSta].startTime = 0; /* dont know snippet times yet */
				SelSta[iSta].duration = 0;
				iSel++;
			}
		}
	}
	return(iSel);
}
/*************************end of MandSta2Selected *****************************/
/****************************** match  *****************************************
*                                                                              *
*	helper routine for snippet2trReq above: See if the SCN names               *
*	match. Wildcards allowed				                                   *
*******************************************************************************/
int match( char* sta, char* comp, char* net, EWDB_StationStruct* staStruct)
{
	int staWild =0;
	int netWild =0;
	int chanWild=0;
	
	int staMatch =0;
	int netMatch =0;
	int chanMatch=0;
	
	if (strcmp( sta , "*") == 0)  staWild =1;
	if (strcmp( comp, "*") == 0)  chanWild=1;
	if (strcmp( net , "*") == 0)  netWild =1;

	if ( strcmp( sta, staStruct->Sta )==0 ) staMatch=1;
	if ( strcmp( net, staStruct->Net )==0 ) netMatch=1;
	if ( strcmp( comp, staStruct->Comp )==0 ) chanMatch=1;

	if ( staWild  ==1 ||  staMatch==1)  staMatch =1;
	if ( netWild  ==1 ||  netMatch==1)  netMatch =1;
	if ( chanWild ==1 || chanMatch==1) chanMatch =1;

	if (staMatch+netMatch+chanMatch == 3) 
		return(1);
	else
	   	return(0);
}

/******************************************************************************
*  arc2trig_config() processes command file(s) using kom.c functions;        *
*                    exits if any errors are encountered.                    *
******************************************************************************/
void arc2trig_config( char *configfile )
{
	int      ncommand;     /* # of required commands you expect to process   */
	char     init[50];     /* init flags, one byte for each required command */
	int      nmiss;        /* number of required commands that were missed   */
	char    *com;
	char    *str;
	int      nfiles;
	int      success;
	int      i;
	
	/* Set to zero one init flag for each required command
	*****************************************************/
	ncommand = 16;
	for( i=0; i<ncommand; i++ )  init[i] = 0;
	nLogo = 0;
	
	/* Open the main configuration file
	**********************************/
	nfiles = k_open( configfile );
	if ( nfiles == 0 ) {
        logit ("e",
			"arc2trig: Error opening command file <%s>; exiting!\n",
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
					logit ("e",
						"arc2trig: Error opening command file <%s>; exiting!\n",
						&com[1] );
					exit( -1 );
				}
				continue;
            }
			
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
/*2*/     else if( k_its("InRingName") ) {
			str = k_str();
			if(str) strcpy( InRingName, str );
			init[2] = 1;
            }
/*3*/     else if( k_its("OutRingName") ) {
			str = k_str();
			if(str) strcpy( OutRingName, str );
			init[3] = 1;
            }
/*4*/     else if( k_its("HeartBeatInterval") ) {
			HeartBeatInterval = k_long();
			init[4] = 1;
            }
			
			/* Get any DBMS parameters
			**************************/
			else if( k_its("DBservice") ) {
			str=k_str();
			if(str) strcpy(	DBservice, str);
			UseDB=1;
			}			
			else if( k_its("DBpassword") ) {
			str=k_str();
			if(str) strcpy(	DBpassword, str);
			UseDB=1;
			}			
			else if( k_its("DBuser") ) {
			str=k_str();
			if(str) strcpy(	DBuser, str);
			UseDB=1;
			}			
			else if( k_its("DBtimeoutSeconds") ) {
			DBtimeoutSeconds = k_int();
			}
			else if( k_its("DBLatRange") ) {
			LatMin = k_val(); LatMax = k_val();
			UseDB=1;
			}
			else if( k_its("DBLonRange") ) {
			LonMin = k_val(); LonMax = k_val();
			UseDB=1;
			}
			
			/* Is there a station list
			**************************/
			else if( k_its("StaListFileName") ) {
				str=k_str();
				if(str) strcpy(	StaListFileName, str);
				UseDB=0;
			}			
			
/*5*/		else if( k_its("SaveOriginDist") ) {
			SaveOriginDist = k_val();
			init[5] = 1;
			}
			/* Mandatory stations to include
			********************************/
			else if(k_its("Channel") ) {
				str=k_str(); if(str) strcpy( MandSta[NMand].sta,str);
				str=k_str(); if(str) strcpy( MandSta[NMand].comp,str);
				str=k_str(); if(str) strcpy( MandSta[NMand].net,str);
				NMand++;
				if(NMand == MAX_MAND_STA){
					logit ("e", "Error. More than %d mandatory channels specified. Exiting\n",MAX_MAGDIST);
					exit(-1);
				}
			}
			/* Enter installation & module to get event messages from
			********************************************************/
/*6*/     else if( k_its("GetEventsFrom") ) {
			if ( nLogo+1 >= MAXLOGO ) {
				logit ("e",
					"arc2trig: Too many <GetEventsFrom> commands in <%s>",
					configfile );
				logit ("e", "; max=%d; exiting!\n", (int) MAXLOGO/2 );
				exit( -1 );
			}
			if( ( str=k_str() ) ) {
				if( GetInst( str, &GetLogo[nLogo].instid ) != 0 ) {
					logit ("e",
						"arc2trig: Invalid installation name <%s>", str );
					logit ("e", " in <GetEventsFrom> cmd; exiting!\n" );
					exit( -1 );
				}
				GetLogo[nLogo+1].instid = GetLogo[nLogo].instid;
			}
			if( ( str=k_str() ) ) {
				if( GetModId( str, &GetLogo[nLogo].mod ) != 0 ) {
					logit ("e",
						"arc2trig: Invalid module name <%s>", str );
					logit ("e", " in <GetEventsFrom> cmd; exiting!\n" );
					exit( -1 );
				}
				GetLogo[nLogo+1].mod = GetLogo[nLogo].mod;
			}
			if( GetType( "TYPE_HYP2000ARC", &GetLogo[nLogo].type ) != 0 ) {
				logit ("e",
					"arc2trig: Invalid message type <TYPE_HYP2000ARC>" );
				logit ("e", "; exiting!\n" );
				exit( -1 );
			}
			if( GetType( "TYPE_H71SUM2K", &GetLogo[nLogo+1].type ) != 0 ) {
				logit ("e",
					"arc2trig: Invalid message type <TYPE_H71SUM2K>" );
				logit ("e", "; exiting!\n" );
				exit( -1 );
			}
			nLogo  += 2;
			init[6] = 1;
            /*    printf("GetLogo[%d] inst:%d module:%d type:%d\n",
			nLogo, (int) GetLogo[nLogo].instid,
			(int) GetLogo[nLogo].mod,
			(int) GetLogo[nLogo].type ); */  /*DEBUG*/
            /*    printf("GetLogo[%d] inst:%d module:%d type:%d\n",
			nLogo+1, (int) GetLogo[nLogo+1].instid,
			(int) GetLogo[nLogo+1].mod,
			(int) GetLogo[nLogo+1].type ); */  /*DEBUG*/
            }
/*7*/     else if( k_its("OutputDir") ) {
			str = k_str();
			if(str) strcpy( OutputDir, str );
			init[7] = 1;
            }
/*8*/     else if( k_its("BaseName") ) {
			str = k_str();
			if(str) strcpy( TrigFileBase, str );
			init[8] = 1;
            }
/*9*/		/* Magnitude/Distance table
			***************************/
			else if(k_its("MagDist") ) 
			{
				Mag[NmagDist]=k_val();
				Dist[NmagDist]=k_val();
				NmagDist++;
				if(NmagDist == MAX_MAGDIST)
				{
					logit ("e", "Error. More than %d MagDist specified. Exiting\n",MAX_MAGDIST);
					exit(-1);
				}
				init[9]=1;
			}
			/*optional*/
			else if( k_its("PreTime") ) {
                PreTime = k_val();
            }
			/*optional*/
			else if( k_its("PostTime") ) {
                PostTime = k_val();
            }
			/*optional*/
			else if( k_its("Debug") ) {
                DDebug = 1;
            }
			/* Read the travel time table
			*****************************/
/*10*/		else if( k_its("Nz") ) 
			{
				Nz = k_int();
				if(Nz > NZMAX) {
					logit ("e", "nsn_loc2trig: Too many depth values:%d. max=%d; exiting!\n", Nz,NZMAX);
					exit(-1);
				}
				init[10] = 1;
			}
/*11*/		else if( k_its("Nd") ) 
			{
				Nd = k_int();
				if(Nd > NDMAX) {
					logit ("e", "nsn_loc2trig: Too many distance values:%d. max=%d; exiting!\n", Nd,NDMAX);
					exit(-1);
				}
				init[11] = 1;
			}
/*12*/		else if( k_its("Ddepth") ) 
			{
				Ddepth = k_val();
				init[12] = 1;
			}
/*13*/		else if( k_its("Ddist") ) 
			{
				Ddist = k_val();
				init[13] = 1;
			}
/*14*/		else if( k_its("Pphase") ) 
			{
				int id,iz;
				int err=1;	/* assume all will go well */
				for(id=0;id<Nd;id++){
					for(iz=0;iz<Nz;iz++){
						if(readTTval( &(Pphase[id][iz]) )<0){
							err=0;
							break;
						}
					}
				if(err == 0) break;
				}
				init[14] = err;
			}
/*15*/		else if( k_its("Sphase") ) 
			{
				int id,iz;
				int err=1;	/* assume all will go well */
				for(id=0;id<Nd;id++){
					for(iz=0;iz<Nz;iz++){
						if(readTTval( &(Sphase[id][iz]) )<0){
							err=0;
							break;
						}
					}
				if(err == 0) break;
				}
				init[15] = err;
			}
			
			/* Unknown command
			*****************/
            else {
                logit ("e", "arc2trig: <%s> Unknown command in <%s>.\n",
					com, configfile );
                continue;
            }
			
			/* See if there were any errors processing the command
			*****************************************************/
            if( k_err() ) {
				logit ("e",
					"arc2trig: Bad <%s> command in <%s>; exiting!\n",
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
       logit ("e", "arc2trig: ERROR, no " );
       if ( !init[0] )  logit ("e", "<LogFile> "           );
       if ( !init[1] )  logit ("e", "<MyModuleId> "        );
       if ( !init[2] )  logit ("e", "<InRingName> "        );
       if ( !init[3] )  logit ("e", "<OutRingName> "       );
       if ( !init[4] )  logit ("e", "<HeartBeatInterval> " );
       if ( !init[5] )  logit ("e", "<SaveOriginDist> " );
       if ( !init[6] )  logit ("e", "<GetEventsFrom> " );
       if ( !init[7] )  logit ("e", "<OutputDir> " );
       if ( !init[8] )  logit ("e", "<BaseName> " );
       if ( !init[9] )  logit ("e", "<MagDist> " );
       if ( !init[10] ) logit ("e", "<Nz> " );
       if ( !init[11] ) logit ("e", "<Nd> " );
       if ( !init[12] ) logit ("e", "<Ddepth> " );
       if ( !init[13] ) logit ("e", "<Ddist> " );
       if ( !init[14] ) logit ("e", "<Pphase> " );
       if ( !init[15] ) logit ("e", "<Sphase> " );
       logit ("e", "command(s) in <%s>; exiting!\n", configfile );
       exit( -1 );
   }
   
   return;
}
/************************************************************************************************************/

double Ptime( double ot, double depth, double dist)
/* Real crude and conservative: We're only in the business of making 
sure we compute a large enough snippet. So for P, get the next smaller 
P value from the table for this depth and distance */
{ 
	int iz,id;
	iz= (int)(depth/Ddepth);
	id= (int)(dist/Ddist);
	if(iz>Nz) iz=Nz;
	if(id>Nd) id=Nd;
	if(id==0) id=1;
	if(iz==0) iz=1;
	return(ot + Pphase[id-1][iz-1]);
}

/*******************************************************************************************/

double Stime( double ot, double depth, double dist)
{ 
/* And by the same logic, get the next larger value for S */
	int iz,id;
	iz= (int)(depth/Ddepth);
	id= (int)(dist/Ddist);
	if(iz>=Nz) iz=Nz-1;
	if(id>=Nd) id=Nd-1;
	return(ot + Sphase[id+1][iz+1]);
}


/***********************************************************************************************
	To read the next travel time value. 
	Carl's k_val routine is a bit on the lame side.
	The idea is to find the next floating point value while
	stepping over comments and blank lines.
************************************************************************************************/
int readTTval(double* val)	
{
	char* nxt;

	/* get next token */
	nxt=k_str();

	/* or die trying */
	while(nxt==NULL || k_err() || nxt[0]=='#') {
		if( k_rd()==0 ) return(-1); /* eof found */
		nxt=k_str();
	}

	/* convert to floating point */
	*val= atof(nxt);
	return(1);
}	
/******************************************************************************
*  arc2trig_lookup( )   Look up important info from earthworm.h tables       *
******************************************************************************/
void arc2trig_lookup( void )
{
/* Look up keys to shared memory regions
	*************************************/
	if( ( InRingKey = GetKey(InRingName) ) == -1 ) {
        fprintf( stderr,
			"arc2trig:  Invalid ring name <%s>; exiting!\n", InRingName);
        exit( -1 );
	}
	
	/* Look up keys to shared memory regions
	*************************************/
	if( ( OutRingKey = GetKey(OutRingName) ) == -1 ) {
        fprintf( stderr,
			"arc2trig:  Invalid ring name <%s>; exiting!\n", OutRingName);
        exit( -1 );
	}
	
	/* Look up installations of interest
	*********************************/
	if ( GetLocalInst( &InstId ) != 0 ) {
		fprintf( stderr,
			"arc2trig: error getting local installation id; exiting!\n" );
		exit( -1 );
	}
	
	/* Look up modules of interest
	***************************/
	if ( GetModId( MyModName, &MyModId ) != 0 ) {
		fprintf( stderr,
			"arc2trig: Invalid module name <%s>; exiting!\n", MyModName );
		exit( -1 );
	}
	
	/* Look up message types of interest
	*********************************/
	if ( GetType( "TYPE_HEARTBEAT", &TypeHeartBeat ) != 0 ) {
		fprintf( stderr,
			"arc2trig: Invalid message type <TYPE_HEARTBEAT>; exiting!\n" );
		exit( -1 );
	}
	if ( GetType( "TYPE_ERROR", &TypeError ) != 0 ) {
		fprintf( stderr,
			"arc2trig: Invalid message type <TYPE_ERROR>; exiting!\n" );
		exit( -1 );
	}
	if ( GetType( "TYPE_HYP2000ARC", &TypeHyp2000Arc ) != 0 ) {
		fprintf( stderr,
			"arc2trig: Invalid message type <TYPE_HYP2000ARC>; exiting!\n" );
		exit( -1 );
	}
	if ( GetType( "TYPE_H71SUM2K", &TypeH71Sum2k ) != 0 ) {
		fprintf( stderr,
			"arc2trig: Invalid message type <TYPE_H71SUM2K>; exiting!\n" );
		exit( -1 );
	}
	if ( GetType( "TYPE_TRIGLIST2K", &TypeTrigList ) != 0 ) {
		fprintf( stderr,
			"arc2trig: Invalid message type <TYPE_TRIGLIST2K>; exiting!\n" );
		exit( -1 );
	}
	
	return;
}

/******************************************************************************
* arc2trig_status() builds a heartbeat or error message & puts it into       *
*                   shared memory.  Writes errors to log file & screen.      *
******************************************************************************/
void arc2trig_status( unsigned char type, short ierr, char *note )
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
	
	time( &t );
	
	if( type == TypeHeartBeat )
	{
        sprintf( msg, "%ld %ld\n\0", t, MyPid );
	}
	else if( type == TypeError )
	{
        sprintf( msg, "%ld %hd %s\n\0", t, ierr, note);
        logit( "et", "arc2trig: %s\n", note );
	}
	
	size = strlen( msg );   /* don't include the null byte in the message */
	
	/* Write the message to shared memory
	************************************/
	if( tport_putmsg( &OutRegion, &logo, size, msg ) != PUT_OK )
	{
        if( type == TypeHeartBeat ) {
			logit("et","arc2trig:  Error sending heartbeat.\n" );
        }
        else if( type == TypeError ) {
			logit("et","arc2trig:  Error sending error:%d.\n", ierr );
        }
	}
	
	return;
}

/************************* sonOfSiteRead.c ********************************
 * Derived from site_read, below. Modified to load DB station structures  *
 *  site_read(name)  Read in a HYPOINVERSE format, universal station      *
 *                   code file                                            *
 **************************************************************************/

/* Sample station line:
R8075 MN  BHZ  41 10.1000N121 10.1000E   01.0     0.00  0.00  0.00  0.00 1  0.00
0123456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789 
*/

int sonOfSiteRead(char *name)
{
	FILE  *stafile;
	char   line[256];
	int    dlat, dlon, elev;
	double  mlat, mlon;
	char   comp, ns, ew;
	int    n;
	
	/* open station file
	*****************/
	if( (stafile = fopen( name, "r" )) == (FILE *) NULL ) {
		logit("e","sonOfSiteRead: Cannot open station list <%s>; exiting!\n", name);
		return(-1);
	}
	
	/* read in one line of the site file at a time
	*******************************************/
	while( fgets( line, sizeof(line), stafile ) != (char *) NULL )
	{
        /* see if internal site table has room left */
		if( NStationList >= MAX_STA ) 
		{
			logit("e","sonOfSiteRead: StationList table full; cannot load entire file <%s>\n", name );
			return(-1);
		}
        /* decode each line of the file */
		sscanf( &line[0] , "%s", &StationList[NStationList].Sta);
		sscanf( &line[6] , "%s", &StationList[NStationList].Net);
		sscanf( &line[10], "%s", &StationList[NStationList].Comp);
		comp = line[9];
		
		line[42] = '\0';
		n = sscanf( &line[38], "%d", &elev );
		if( n < 1 ) 
		{
			logit("e","sonOfSiteRead: Error reading elevation from line in station file\n%s\n",line );
			continue;
		}
		
		ew       = line[37];
		line[37] = '\0';
		n = sscanf( &line[26], "%d %f", &dlon, &mlon );
		if( n < 2 ) 
		{
			logit("e","sonOfSiteRead: Error reading longitude from line in station file\n%s\n",line );
			continue;
		}
		
		ns       = line[25];
		line[25] = '\0';
		n = sscanf( &line[15], "%d %f", &dlat, &mlat );
		if ( n < 2 ) 
		{
			logit("e","sonOfSiteRead: Error reading latitude from line in station file\n%s\n",
				line );
			continue;
		}
		
        /*      printf( "%-5s %-2s %-3s %d %.4f%c%d %.4f%c%4d\n",
		StationList[NStationList].Sta, StationList[NStationList].Net, StationList[NStationList].Comp,
		dlat, mlat, ns,
		dlon, mlon, ew, elev ); */ /*DEBUG*/
		
        /* use one-letter component if there is no 3-letter component given */
		if ( !strcmp(StationList[NStationList].Comp, "   ") ) sprintf( StationList[NStationList].Comp, "%c  ", comp );
		
        /* convert to decimal degrees */
		if ( dlat < 0 ) dlat = -dlat;
		if ( dlon < 0 ) dlon = -dlon;
		StationList[NStationList].Lat = (float)((double) dlat + (mlat/60.0));
		StationList[NStationList].Lon = (float)((double) dlon + (mlon/60.0));
		
        /* make south-latitudes and west-longitudes negative */
		if ( ns=='s' || ns=='S' )
			StationList[NStationList].Lat = -StationList[NStationList].Lat;
		if ( ew=='w' || ew=='W' || ew==' ' )
			StationList[NStationList].Lon = -StationList[NStationList].Lon;
		StationList[NStationList].Elev = (float)((double) elev/1000.);
		
        /*      printf("%-5s %-2s %-3s %.4f %.4f %.0f\n\n",
		StationList[NStationList].Sta, StationList[NStationList].Net, StationList[NStationList].Comp,
		StationList[NStationList].Lat, StationList[NStationList].Lon, StationList[NStationList].Elev ); */ /*DEBUG*/
		
		/* update the total number of stations loaded */
		if(NStationList < MAX_STA) ++NStationList;
		
	} /*end while*/
	
	fclose( stafile );
	return(1);
}
