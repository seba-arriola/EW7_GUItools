
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: db_cleanup.c,v 1.6 2001/07/01 21:55:13 davidk Exp $
 *
 *    Revision history:
 *
 */



/*
 * This program is meant to be run out of cron (or some other 
 * periodic invocation method). Its function is to keep the 
 * number of events in the database manageable, by deleting 
 * the oldest events. 
 *
 * The configuration options StartDate and NumberOfDays determine
 * how many and which events are deleted. For example, StartDate
 * of 7 and NumberOfDays of 1 means that this program will delete
 * all events for one day starting 7 days ago. 
 *
 */
 

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <time.h>
#include <time_functions.h>
#include <ewdb_ora_api.h>
#include <ew_event_info.h>
#include <ewdb_apps_utils.h>
#include <sac_2_ewevent.h>

#include "db_cleanup.h"


extern int errno;   /* system error variable */

extern char DBuser[];
extern char DBservice[];
extern char DBpassword[];
extern char envEW_LOG[];
extern int  CleanDebug;
extern int  StartDate;
extern int  NumberOfDays;
extern int  DeleteTraceOnly;
extern int  Save;
extern char SACdatadir[];
extern long TraceBufferLen;
extern char OutputFormat[];


main (int argc, char **argv)
{

	EWEventInfoStruct		DBEvent;
	EWDB_WaveformStruct		*pWaveform;
	EWDB_EventListStruct 	EVMin,EVMax;
	EWDB_EventListStruct 	*pEventBuffer;
	int 					GetEventListBufferSize;
	int 					NumEventsRetrieved;
	int 					RetVal;
	time_t 					start, end, today;
	int					eventid;                /* current eventid we're processing  */
	int					iev;
	int					i, j;
	char				buf[256];


   /* Introduce ourselves
   **********************/
   if (argc != 2)
   {
		fprintf (stderr, "Usage: db_cleanup config_file.\n");
	   exit(0);
   }
   
   /* Read the configuration file (path hardcoded relative to executable)
   *********************************************************************/
	ReadConfig (argv[1]); /* it exits if it isn't happy */

   /* Start up Logging
   *******************/
	logit_init ("db_cleanup",1,1024,1);

	logit("","\n/*************************************\n"
	          "  Initializing ORA_API                 \n"
	          "*************************************/\n");

	/* Open connection to database
	*****************************/
	if( ewdb_api_Init (DBuser, DBpassword, DBservice) != 0 )
	{
		logit( "", "Trouble connecting to database; exiting!\n" );
		goto shutdown;
	}

	logit ("", "Connected!\n");

	/* Initialize EVMax and EVMin to blank */
	memset (&EVMax, 0, sizeof(EWDB_EventListStruct));
	memset (&EVMin, 0, sizeof(EWDB_EventListStruct));


	/* Get all events in the database, irrespective of where they are */
	EVMin.dLat = 0.00;  
	EVMin.dLon = 0.00;
	EVMax.dLat = 0.00;
	EVMax.dLon = 0.00;

	/* Get all events, regardless of their reviewed status */
	sprintf (EVMin.Event.szSource, "*");

	GetEventListBufferSize = 500;

	if ((pEventBuffer = (EWDB_EventListStruct *)
			malloc (GetEventListBufferSize * sizeof(EWDB_EventListStruct))) == NULL)
	{
		logit ("", "Unable to allocate space for retrieval of %d Events.\n",
															GetEventListBufferSize);
		goto shutdown;
	}


	/* Compute Start and End times */
	today = time (NULL);
	/* subtract the number of seconds since the StartDate (86400 sec/day) */
	start = today - (StartDate * 86400); 
	end = start + (NumberOfDays * 86400);

	EWDB_ttoa (&start, buf);
	logit ("", "Start Date: %s\n",  buf);
	EWDB_ttoa (&end, buf);
	logit ("", "End Date: %s\n",  buf);

	/* Figure out which events to delete
	 ****************************************/
	RetVal = ewdb_api_GetEventList (pEventBuffer, GetEventListBufferSize,
						start, end, &EVMin, &EVMax, &NumEventsRetrieved);

	if (RetVal == -1)
	{
		logit ("", "Call to ewdb_api_GetEventList failed -- see logfile.\n");
		goto shutdown;
	}

	if (DeleteTraceOnly == 1)
		logit ("", "Will clean up trace data from %d events.\n", NumEventsRetrieved);
	else 
		logit ("", "Will clean up all data from %d events.\n", NumEventsRetrieved);

	if (Save == 1)
		logit ("", "Trace data in SAC format saved in %s.\n", SACdatadir); 

					

	if (CleanDebug > 0)
	{
		for (iev = 0; iev < NumEventsRetrieved; iev++)
		{
			logit ("", "%d (%s): %0.2f ==>  %0.2f, %0.2f, %0.2f, %0.2f\n",
					pEventBuffer[iev].Event.idEvent, 
					pEventBuffer[iev].Event.szSourceName, 
					pEventBuffer[iev].dOT, 
					pEventBuffer[iev].dLat, 
					pEventBuffer[iev].dLon, 
					pEventBuffer[iev].dDepth, 
					pEventBuffer[iev].dPrefMag);
		}
	}


	/* Initialize SAC writing mechanism */
	if (Save == 1)
	{
		if (WriteSAC_init (SACdatadir, OutputFormat, CleanDebug) != EW_SUCCESS)
		{
			logit("", "db_cleanup: Call to WriteSAC_init failed!\n" );
			goto shutdown;
		}
	}


	/**************** LOOP OVER EVENTS ********************/
	for (iev = 0; iev < NumEventsRetrieved; iev++)
	{
		eventid = pEventBuffer[iev].Event.idEvent;

		if (Save == 1)
		{
			/* Get gory details for this event from the database 
			***************************************************/
			if (ewdb_apps_GetDBEventInfo (&DBEvent, eventid) != EWDB_RETURN_SUCCESS)
			{
				logit( "", 
					"Call to ewdb_apps_GetDBEventInfo failed for idEvent %d\n", eventid);
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
			if (Save == 1)
			{
				if (WriteSAC_StartEvent (&DBEvent) != EW_SUCCESS)
				{
					logit("","Call to WriteSAC_StartEvent failed for idEvent %d.\n", eventid);
					goto shutdown;
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
							logit("","Call to ProduceSAC_NextStationForEvent failed for Channel %d, idEvent %d.\n", i, eventid);
							goto shutdown;
						}
	
	
						if (WriteSAC_NextStationForEvent (&(DBEvent.pChanInfo[i])) != EW_SUCCESS)
						{
							logit("","Call to WriteSAC_NextStationForEvent failed for Channel %d, idEvent %d.\n", i, eventid);
							goto shutdown;
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
					goto shutdown;
				}

				/* free up channel space alloced in ewdb_apps_GetDBEventInfo
				********************************************************/
				for (i = 0; i < DBEvent.iNumAllocChans; i++)
				{
					free (&DBEvent.pChanInfo[i]);
				} 
	
				DBEvent.iNumAllocChans = 0;
			}
	
	
			/* delete section 
			******************/
			if (CleanDebug > 0)
			{
				if (DeleteTraceOnly == 0)
					logit ("", "Deleting all of event %d; \n",eventid);
				else if (DeleteTraceOnly == 1)
					logit ("", "Deleting trace only of event %d; \n",eventid);
			}
				
			if (ewdb_api_DeleteEvent (eventid, DeleteTraceOnly) != EWDB_RETURN_SUCCESS)
			{
				logit ("", "db_cleanup: Call to ewdb_api_DeleteEvent failed.\n");
				goto shutdown;
			}
			if (CleanDebug > 0)
				logit ("", "BACK from DeleteEvent\n");

		} /* if we got the correct event */
		else
		{
			logit ("", "Error retrieving event %d from the database.\n", eventid);
		}

	} /* end for each eventid from stdin */
   

shutdown:
	ewdb_api_Shutdown();

	logit("t","db_cleanup: terminating\n" );

	return( 0 );

}  /* end main */



