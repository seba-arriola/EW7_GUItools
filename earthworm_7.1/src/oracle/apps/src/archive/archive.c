
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: archive.c,v 1.24 2001/07/20 17:28:06 davidk Exp $
 *
 *    Revision history:
 *     $Log: archive.c,v $
 *     Revision 1.24  2001/07/20 17:28:06  davidk
 *     Reformatted the USAGE message that gets printed.
 *
 *     Revision 1.23  2001/05/15 02:15:05  davidk
 *     Moved functions around between the apps, DB API, and DB API INTERNAL
 *     levels.  Renamed functions and files.  Added support for amplitude
 *     magnitude types.  Reformatted makefiles.
 *
 *     Revision 1.22  2001/02/28 17:29:10  lucky
 *     Massive schema redesign and cleanup.
 *
 *     Revision 1.21  2001/02/13 18:24:34  lucky
 *     If a call to ewdb_api_GetWaveformSnippet does not return SUCCESS, free up
 *     malloc-ed space and set NumWaveforms to 0 so that that particular snippet
 *     is not archived.
 *
 *     Revision 1.20  2001/02/08 21:43:11  lucky
 *     Bug fixes including HACKS to make sure that no channels are alloc-ed
 *     before calling ewdb_apps_GetDBEventInfo.
 *
 *     Revision 1.19  2000/11/07 16:28:53  lucky
 *     Do not even enter the SAC writing portion if the event retrieved
 *     from the DB has 0 channels.
 *
 *     Revision 1.18  2000/11/07 16:22:38  lucky
 *     If we fail in SAC creation for a single channel, we don't want
 *     to exit.. Keep on plugging in hopes of saving at least some of
 *     the trace data.
 *
 *     Revision 1.17  2000/10/11 20:15:02  lucky
 *     Free all allocated channels, not just the ones that were filled in.
 *
 *     Revision 1.16  2000/10/11 18:02:25  lucky
 *     Removed InitEWEvent call -- it is called inside GetDBEvent. Also, moved the
 *     code to free up pChanInfo structs within the if-block where pChanInfos are alloced.
 *
 *     Revision 1.15  2000/10/11 17:52:00  lucky
 *     Added code to free up pChanInfo structs allocated inside the loop (either in
 *     InitDbEvent or GetEvent calls)
 *
 *     Revision 1.14  2000/10/10 22:03:53  lucky
 *     Added an if-statement to check whether we have any waveform snippets to
 *     save before calling SAC writing routines.
 *
 *     Revision 1.13  2000/10/10 20:50:59  lucky
 *     Rewrote the Sac writing part - we now get the Event info from GetDBEvent,
 *     but without snippets. Snippets are retrieved one by one, and the space
 *     is freed after the snippet is written out.
 *
 *     Revision 1.12  2000/08/30 16:07:38  lucky
 *     Fixed call to InitEWEvent
 *
 *     Revision 1.11  2000/08/29 20:22:43  lucky
 *     We now free snippet buffers before making calls to ewdb_apps_GetDBEventInfo.
 *
 *     Revision 1.10  2000/08/28 19:58:54  lucky
 *     Only call ewdb_apps_GetDBEventInfo if we need to, i.e. if we are going to
 *     save the trace data.
 *
 *     Revision 1.9  2000/08/25 18:01:38  lucky
 *     Cosmetic changes -- checking for return values.
 *
 *     Revision 1.8  2000/08/16 17:04:43  lucky
 *     Removed write_sac, added Sac2EWEvent
 *
 *     Revision 1.7  2000/08/09 16:18:58  lucky
 *     Lint cleanup
 *
 *     Revision 1.6  2000/08/04 20:24:31  lucky
 *     Added another command line option NOSAVE to allow quick deletion of trace
 *     from the DB without saving it. Also, made it so that diagnostic error
 *     is printed to the stderr as well as the log file when the Debug flag
 *     is set -- this should make the user interface better.
 *
 *     Revision 1.5  2000/04/13 17:09:56  lucky
 *     Added a check to make sure that ewdb_apps_GetDBEventInfo returns the event that we
 *     asked for. Without this check we would proceed into the bowels of the Sac
 *     putaway routines without being sure that the DBEvent structure was correct.
 *
 *     Revision 1.4  2000/03/24 18:34:42  lucky
 *     added WriteHtml argument to WriteSac
 *
 *     Revision 1.3  2000/03/14 17:25:14  lucky
 *     Simplified code to use the new calls to ewdb_apps_GetDBEventInfo to retrieve all info
 *     about an event from the database; and WriteSAC_Event to output the event
 *     info, including the parametric data, in SAC format.
 *
 *     Revision 1.2  2000/03/10 22:53:55  lucky
 *     Fixed the return value from ewdb_api_GetWaveformSnippet. Now we only
 *     continue with sac writing if we get SUCCESS. ewdb_api_GetWaveformSnippet
 *     returns WARNING if, in those rare and strange occassions, there
 *     exists a record for a snippet but no data.
 *
 *     Revision 1.1  2000/02/15 19:02:31  lucky
 *     Initial revision
 *
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


/* Added by Kevin for NT */
#ifdef _WINNT
#include <io.h>
#include <time.h>
#include <sys\stat.h>
#include <sys\types.h>
#include <dos.h>
#include <direct.h> 
#endif
/* End for added includes */

#include "archive.h"

#include <ewdb_ora_api.h>
#include <ewdb_apps_utils.h>
#include <earthworm.h>
#include <trace_buf.h>
#include <sac_2_ewevent.h>
#include <ws_clientII.h>


extern int errno;   /* system error variable */

#define		INIT_SNIPPET_SIZE 		10000


/* Function prototypes
 *********************/
static void PrintUsage ();

main (int argc, char **argv)
{

	EWEventInfoStruct	DBEvent;
	EWDB_WaveformStruct	*pWaveform;
	int					nevent;                 /* #eventids in list                 */
	int					eventid;                /* current eventid we're processing  */
	int					iev;
	int					i, j;
	int					eventlow;
	int					eventhigh;
	int					DELtoken;
	int					deleteEvent;
	int					deleteTraceOnly;
	int					saveTrace;


   /* Introduce ourselves
   **********************/
   if (argc != 7)
   {
	   PrintUsage();
	   exit(0);
   }
   
   /* Initialize some stuff
   ***********************/
	nQuickLook = NQUICKLOOK;
	deleteEvent = 0;
	deleteTraceOnly = 0;
	saveTrace = 1;


   /* Read the configuration file (path hardcoded relative to executable)
   *********************************************************************/
	ReadConfig (argv[1]); /* it exits if it isn't happy */

   /* Start up Logging
   *******************/
	logit_init ("archive",1,1024,1);
   
	logit ("","startup. ArchDebug=%d\n", ArchDebug);
  
  
	/* Pick up command line arguments
	*********************************/

	/* Events to archive */
	if (strcmp(argv[3],"-") == 0)
	{ 
		/* we've been given a range */
		eventlow = atoi(argv[2]);
		eventhigh = atoi(argv[4]);
		nevent = (eventhigh - eventlow) + 1;
		if (nevent < 1)
		{
			fprintf (stderr, "ERROR: Bad event range specified\n");
			PrintUsage();
			exit(0);
		}
		DELtoken = 5;
	} 
	else 
	{
		eventlow = eventhigh = atoi(argv[2]); /* its a single event */
		nevent = 1;
		DELtoken=3;
	}

	/* The DELETE option */
	if (strcmp(argv[DELtoken],"DELETE") != 0 && strcmp(argv[DELtoken],"delete") != 0)
	{
		fprintf (stderr, "DELETE keword not found\n");
		PrintUsage();
		exit(0);
	}
	if ((strcmp(argv[DELtoken+1],"SNIPPETS") == 0) || 
				(strcmp(argv[DELtoken+1],"snippets") == 0))
	{
		deleteEvent = 1;
		deleteTraceOnly = 1;
		saveTrace = 1;
	}
	else if ((strcmp(argv[DELtoken+1],"ALL") == 0) || 
					(strcmp(argv[DELtoken+1],"all")==0))
	{
		deleteEvent = 1;
		deleteTraceOnly = 0;
		saveTrace = 1;
	}
	else if ((strcmp(argv[DELtoken+1],"NO") == 0) || 
				(strcmp(argv[DELtoken+1],"no") == 0))
	{
		deleteEvent = 0;
		saveTrace = 1;
	}
	else if ((strcmp(argv[DELtoken+1],"NOSAVE") == 0) || 
				(strcmp(argv[DELtoken+1],"nosave") == 0))
	{
		deleteEvent = 1;
		deleteTraceOnly = 1;
		saveTrace = 0;
	}
	else 
	{
		fprintf (stderr, "Bad DELETE keyword: %s\n",argv[DELtoken+1]);
		PrintUsage();
		exit(0);
	}

	if (ArchDebug > 0)
	{
		logit ("e", "Starting Archiver for events: %d through %d\n",
							eventlow, eventhigh);
		logit ("e", "deleteEvent flag is %d\n", deleteEvent);
		logit ("e", "deleteTraceOnly flag is %d\n", deleteTraceOnly);
		logit ("e", "saveTrace flag is %d\n", saveTrace);
	}

	logit("","\n/*************************************\n"
	          "  Initializing ORA_API                 \n"
	          "*************************************/\n");

	/* Open connection to database
	*****************************/
	if( ewdb_api_Init (DBuser, DBpassword, DBservice) != 0 )
	{
		logit( "e", "Trouble connecting to database; exiting!\n" );
		goto shutdown;
	}

	logit ("", "Connected!\n");


	if (saveTrace == 1)
	{
		/* Initialize SAC writing mechanism */
		if (WriteSAC_init (SACdatadir, OutputFormat, ArchDebug) != EW_SUCCESS)
		{
			logit("e", "archive: Call to WriteSAC_init failed!\n" );
			return EW_FAILURE;
		}
	}




	/**************** LOOP OVER EVENTS ********************/

	for( iev = 0; iev < nevent; iev++ )
	{
		eventid = iev + eventlow;	/* vestigial from when we had an event list */

		if (saveTrace == 1)
		{
			/* Get gory details for this event from the database 
			***************************************************/
			memset (&DBEvent, 0, sizeof (EWEventInfoStruct));
			if (ewdb_apps_GetDBEventInfo (&DBEvent, eventid) != EWDB_RETURN_SUCCESS)
			{
				logit( "e", 
					"Call to ewdb_apps_GetDBEventInfo failed for idEvent %d\n", eventid);
				free (DBEvent.pChanInfo);
				continue;
			}
		}
		else
			DBEvent.Event.idEvent = eventid;

		/* Only proceed if the event that we got back from the DB
		   has the same ID as the one that we requested.
		 ************************************************************/
		if (DBEvent.Event.idEvent == eventid)
		{
			if ((saveTrace == 1) && (DBEvent.iNumChans > 0))
			{
				if (WriteSAC_StartEvent (&DBEvent) != EW_SUCCESS)
				{
					logit("","Call to WriteSAC_StartEvent failed for idEvent %d.\n", eventid);
					return EW_FAILURE;
				}

				for (i = 0; i < DBEvent.iNumChans; i++)
				{
					for (j = 0; j < DBEvent.pChanInfo[i].iNumWaveforms; j++)
					{
						/* Retrieve snippets for this channel */
						pWaveform = &(DBEvent.pChanInfo[i].Waveforms[j]);

						pWaveform->binSnippet = malloc (pWaveform->iByteLen);

						if (ewdb_api_GetWaveformSnippet (pWaveform->idWaveform, 
										pWaveform->binSnippet, pWaveform->iByteLen) 
																	!= EWDB_RETURN_SUCCESS)
						{
							logit ("", "Call to ewdb_api_GetWaveformSnippet failed for Chan %d, Waveform %d.\n", i, j);
							free (pWaveform->binSnippet);
							DBEvent.pChanInfo[i].iNumWaveforms = 0;
						}

					} /* for loop over waveforms */

					if (DBEvent.pChanInfo[i].iNumWaveforms > 0)
					{
						if (ProduceSAC_NextStationForEvent (&(DBEvent.pChanInfo[i])) != EW_SUCCESS)
						{
							logit("","Call to ProduceSAC_NextStationForEvent failed for Channel %d, idEvent %d. Continuing\n", i, eventid);
						}
						else
						{
							if (WriteSAC_NextStationForEvent (&(DBEvent.pChanInfo[i])) != EW_SUCCESS)
							{
								logit("","Call to WriteSAC_NextStationForEvent failed for Channel %d, idEvent %d. Continuing\n", i, eventid);
							}
						}
	
		
						/* Free up allocated snippet space */
						for (j = 0; j < DBEvent.pChanInfo[i].iNumWaveforms; j++)
						{
							pWaveform = &(DBEvent.pChanInfo[i].Waveforms[j]);
							free (pWaveform->binSnippet);
							pWaveform->binSnippet = NULL;
						}
				
					} /* Do we have any waveforms to save */

				} /* for loop over channels */

				if (WriteSAC_EndEvent () != EW_SUCCESS)
				{
					logit("","Call to WriteSAC_EndEvent failed for idEvent %d.\n", eventid);
					return EW_FAILURE;
				}
			}
	

			free (DBEvent.pChanInfo);
			DBEvent.iNumAllocChans = 0;

	
			/* delete section 
			******************/
			if (deleteEvent == 1)
			{
				if (ArchDebug > 0)
				{
					if (deleteTraceOnly == 0)
						logit ("e", "Deleting all of event %d; \n",eventid);
					else if (deleteTraceOnly == 1)
						logit ("e", "Deleting trace only of event %d; \n",eventid);
				}
					
				if (ewdb_api_DeleteEvent (eventid, deleteTraceOnly) != EWDB_RETURN_SUCCESS)
				{
					logit ("", "archive: Call to ewdb_api_DeleteEvent failed.\n");
					return FAILURE;
				}
				if (ArchDebug > 0)
					logit ("e", "BACK from DeleteEvent\n");
			}

		} /* if we got the correct event */
		else
		{
			logit ("e", "Error retrieving event %d from the database.\n", eventid);
			free (DBEvent.pChanInfo);
			DBEvent.iNumAllocChans = 0;
		}

	} /* end for each eventid from stdin */
   

shutdown:
	ewdb_api_Shutdown();

	logit("t","archive: terminating\n" );

	return( 0 );

}  /* end main */



/**************************************************
 *  PrintUsage() prints usage hints to the screen *
  **************************************************/
void PrintUsage (void)
{
	fprintf(stderr, "Archive writes events from the DBMS to SAC directories\n");
	fprintf(stderr, "Usage: archive <config file> A - B  "
                  "DELETE {ALL | SNIPPETS | NO | NOSAVE}\n");
	fprintf(stderr, "	where:\n ");
	fprintf(stderr, "		<config file> is the full path to the configuration file\n");
	fprintf(stderr, "		A and B are event id's, inclusive\n");
	fprintf(stderr, "		DELETE ALL will delete paramateric and trace data\n");
	fprintf(stderr, "		DELETE SNIPPETS will delete trace data only\n");
	fprintf(stderr, "		DELETE NO will delete nothing\n");
	fprintf(stderr, "		DELETE NOSAVE will delete trace data only without saving it\n");
	fprintf(stderr, "	All tokens must be separated by spaces.\n\n");
	fprintf(stderr, "	Log file will be written to the directory specified by the "
	                "EW_LOG environment variable.\n");
	return;
}


