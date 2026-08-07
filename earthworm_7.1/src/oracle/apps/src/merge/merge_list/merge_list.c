/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: merge_list.c,v 1.1 2004/03/17 18:38:03 davidk Exp $
 *
 *    Revision history:
 *     $Log: merge_list.c,v $
 *     Revision 1.1  2004/03/17 18:38:03  davidk
 *     Initial revision
 *
 *
 *************************************************************/
  

/*** #includes ***/

/* include the normal stuff */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <time_ew.h>
#include <ew_event_info.h>
#include <ewdb_ora_api.h>
#include <ewdb_apps_utils.h>
#include <webparse.h>
#include <merge.h>

#include <review.h>

typedef struct webopts
{
	int 	idPh;
} WebOpts;

int main(int argc, char * argv[])
{

	WebOpts				WebParams;
  	char				*configfile = "../params/review_event.d";
	int					i, j, NumTraces, NumFound, NumRetr, EventsShown;
	EWDBid				idPrefEvent;
	EWDB_MergeList		*pMergeBuffer;
	EWDB_MergeList		EVMin,EVMax;
	double				minTime, maxTime;
	EWDB_PhenomenaMergeList	*pConsList;
	EWEventInfoStruct	EventInfo;
	EWDB_CoincEventStruct	*pCoinc;
	int					DBFlags, NumPhenomena;
	time_t				TempTime;
	char				pTimeBuffer[256];


    DEBUG = 0;

   /* Read the configuration file (path hardcoded relative to executable)
     *********************************************************************/
    ReadConfig (configfile);

    /* Cleanup, logfile should match config file name */
    logit_init ("merge_list",1,MAX_BYTES_PER_EQ,1);

    if (Webparse_GetAndProcessWebParams((void *)(&WebParams)) != 0)
    {
        html_logit ("", "Call to Webparse_GetAndProcessWebParams failed.\n");
        html_break ();
        goto shutdown;
    }


    /* Send html header back to web server
     *************************************/
    printf("Content-type: text/html\n\n");
    printf("<html>\n");

    printf ("<HEAD><TITLE>Event Merges for %d</TITLE></HEAD>\n", 
											WebParams.idPh );

	html_header (BackgroundColor, HeaderLogo);

    printf("<center>\n");
    printf("<H2><FONT COLOR=red>Event Merge Candidates</FONT></H2>\n");
    printf("</center>\n");

    printf("<pre>\n");

	/* Connect to the DB */
    if (ewdb_api_Init (DBuser, DBpassword, DBservice ) != 0 )
    {
        html_logit ("", "Trouble connecting to database; exiting!\n" );
        html_break();
        goto shutdown;
    }


	if ((pMergeBuffer = (EWDB_MergeList *) malloc (MAX_MERGES_PER_EVENT * 
					sizeof (EWDB_MergeList))) == NULL)
    {
        html_logit ("", "Could not malloc MergeBuffer\n");
        html_break();
        goto shutdown;
    }


	/* Retrieve list of merges from the db */
	NumFound = NumRetr = 0;
	if (ewdb_api_GetMergeList (pMergeBuffer, MAX_MERGES_PER_EVENT, 
				WebParams.idPh, (EWDB_MergeList *) NULL, (EWDB_MergeList *) NULL,
				&NumFound, &NumRetr) != EWDB_RETURN_SUCCESS)
	{
		html_logit ("", "Call to ewdb_api_GetMergeList failed.\n");
        html_break();
        goto shutdown;
	}

	if (NumRetr < NumFound)
	{
		logit ("", "WARNING: merge array to small; reallocating to %d.\n", NumFound);

		free (pMergeBuffer);

		/* Allocated audit list buffer */
		if ((pMergeBuffer = (EWDB_MergeList *) malloc (NumFound *
									sizeof (EWDB_MergeList))) == NULL)
		{
			html_logit ("", "Could not malloc MergeBuffer.\n");
	        html_break();
			goto shutdown;
		}

		if (ewdb_api_GetMergeList (pMergeBuffer, NumFound, 
				WebParams.idPh, (EWDB_MergeList *) NULL, (EWDB_MergeList *) NULL,
				&NumFound, &NumRetr) != EWDB_RETURN_SUCCESS)
		{
			html_logit ("", "Call to ewdb_api_GetMergeList failed.\n");
	        html_break();
			goto shutdown;
		}
	}

	if (NumRetr < 1)
	{
		html_logit ("", "Could not retrieve at least one merge.\n");
        html_break();
		goto shutdown;
	}

logit ("", "Ph %d, merge with %d evts\n", WebParams.idPh, NumRetr);
for (i = 0; i < NumRetr; i++)
{
 logit ("", "Merge %d: %d %d %d, %0.2f %0.2f %0.2f %0.2f\n", i,
   pMergeBuffer[i].idEvent,
   pMergeBuffer[i].idPrefEvent,
   pMergeBuffer[i].idPh,
   pMergeBuffer[i].dLat,
   pMergeBuffer[i].dLon,
   pMergeBuffer[i].dDepth,
   pMergeBuffer[i].dPrefMag);
}


	/* Retrieve info on the initial (triggering) event */
	if (InitEWEvent (&EventInfo) != EW_SUCCESS)
	{
		logit ("", "Call to InitEWEvent failed.\n");
        html_break();
		goto shutdown;
	}


    DBFlags = GETEWEVENTINFO_SUMMARYINFO | GETEWEVENTINFO_PICKS
                    | GETEWEVENTINFO_STAMAGS | GETEWEVENTINFO_COMPINFO |
                    GETEWEVENTINFO_WAVEFORM_DESCS;

	if (ewdb_apps_GetDBEventInfoII (&EventInfo, pMergeBuffer[0].idPrefEvent, 
												-1, DBFlags) != EWDB_RETURN_SUCCESS)
	{
		logit ("", "Call to ewdb_apps_GetDBEventInfoII failed.\n");
        html_break();
		goto shutdown;
	}

	if ((idPrefEvent = EventInfo.Event.idEvent) <= 0)
	{
		logit ("", "Call to ewdb_apps_GetDBEventInfoII returned bad idEvent %d.\n",
											idPrefEvent);
        html_break();
		goto shutdown;
	}


	minTime = EventInfo.PrefOrigin.tOrigin - 3600.0;
	maxTime = EventInfo.PrefOrigin.tOrigin + 3600.0;

	
	printf ("<PRE><HR><P>\n");

	printf ("<CENTER><STRONG>Preferred Solution -- Event %d</CENTER></STRONG>\n", 
											EventInfo.Event.idEvent);

	/* Put up principal event parameters for the initial phenomenon */
	html_principal_summary (&EventInfo, FALSE);


	/* 
	 * just to be sure, count the number of events different from the 
	 * one that triggered this phenomenon 
	 */

	/* Start the form */
	printf ("<FORM ACTION=\"merge_process\"" EXE_EXT "METHOD-POST>\n"); 
	printf ("<INPUT NAME=idPh TYPE=HIDDEN VALUE=\"%d\">\n", WebParams.idPh);
	printf ("<INPUT NAME=idPrefEvent TYPE=HIDDEN VALUE=\"%d\">\n", 
										EventInfo.Event.idEvent);


	EventsShown = 0;
	if (NumRetr == 0)
	{
		printf ("<CENTER><BR><BR><HR><BR><BR>\n");
		printf ("<STRONG>There are no automatic merge candidates for event %d</STRONG>.\n<BR><BR>", idPrefEvent);
	}
	else
	{
		/* Now, list info for all potential merges */

	
		printf("<CENTER><HR><PRE>\n\n");
	
		printf ("<STRONG>Below is the list of %d automatic merge candidates.</STRONG>\n\n", NumRetr);
	
		printf("Highlighted event is the preferred one\n\n");
		printf("To see more information about an event, click on the EventID.\n");
	
		printf ("To see a snapshot of the waveforms for each event, click on \n"
					"the link in the 'Traces' column.\n\n"
					"To approve a merge candidate and add it to the merged event,\n"
					"make sure that the Merge? button for that event is checked.\n\n"
				"When review of all events is completed, click Process Merges button.\n\n");
 

		printf ("</PRE>\n");
		/*  Write the table header */
		printf ("<TABLE BORDER=1> <TR BGCOLOR=#2222AA VALIGN=TOP ALIGN=CENTER>\n");

		printf ("<TD VALIGN=CENTER><STRONG><FONT SIZE=\"+1\" COLOR=#FFFFFF>Merge?</TD>\n");
		printf ("<TD VALIGN=CENTER><STRONG><FONT SIZE=\"+1\" COLOR=#FFFFFF>EventID</TD>\n");
		printf ("<TD VALIGN=CENTER><STRONG><FONT SIZE=\"+1\" COLOR=#FFFFFF>Origin Time</TD>\n");
		printf ("<TD VALIGN=CENTER><STRONG><FONT SIZE=\"+1\" COLOR=#FFFFFF>Lat</TD>\n");
		printf ("<TD VALIGN=CENTER><STRONG><FONT SIZE=\"+1\" COLOR=#FFFFFF>Lon</TD>\n");
		printf ("<TD VALIGN=CENTER><STRONG><FONT SIZE=\"+1\" COLOR=#FFFFFF>Depth</TD>\n");
		printf ("<TD VALIGN=CENTER><STRONG><FONT SIZE=\"+1\" COLOR=#FFFFFF>Md</TD>\n");
		printf ("<TD VALIGN=CENTER><STRONG><FONT SIZE=\"+1\" COLOR=#FFFFFF>ML</TD>\n");
		printf ("<TD VALIGN=CENTER><STRONG><FONT SIZE=\"+1\" COLOR=#FFFFFF>Source</TD>\n");
		printf ("<TD VALIGN=CENTER><STRONG><FONT SIZE=\"+1\" COLOR=#FFFFFF>Traces</TD>\n");

	    printf("</TR>\n");
	
		for (i = 0; i < NumRetr; i++)
		{
			free (EventInfo.pChanInfo);
	
			/* Retrieve info on the initial (triggering) event */
			if (InitEWEvent (&EventInfo) != EW_SUCCESS)
			{
				logit ("", "Call to InitEWEvent failed.\n");
		        html_break();
				goto shutdown;
			}
	
			if (ewdb_apps_GetDBEventInfoII (&EventInfo, pMergeBuffer[i].idEvent, 
													-1, DBFlags) != EWDB_RETURN_SUCCESS)
			{
				logit ("", "Call to ewdb_apps_GetDBEventInfoII failed.\n");
   	    		html_break();
				goto shutdown;
			}

			printf ("<INPUT NAME=Evt%d TYPE=HIDDEN VALUE=\"%d\">\n", EventsShown, 
													pMergeBuffer[i].idEvent);
			printf ("<INPUT NAME=Ph%d TYPE=HIDDEN VALUE=\"%d\">\n", EventsShown, 
													pMergeBuffer[i].idPh);
			printf ("<INPUT NAME=Auto%d TYPE=HIDDEN VALUE=\"%d\">\n", EventsShown, 1);
													
	

			/* Count the number of traces */
			NumTraces = 0;
			for (j = 0; j < EventInfo.iNumChans; j++)
			{
				if (EventInfo.pChanInfo[j].iNumWaveforms > 0)
					NumTraces = NumTraces + 1;
			}
	
			if (pMergeBuffer[i].idEvent != idPrefEvent)
				printf("<TR ALIGN=CENTER>\n");
			else
			{
				printf("<TR BGCOLOR=\"silver\" ALIGN=CENTER>\n");
				printf ("<INPUT NAME=PrefEvt TYPE=HIDDEN VALUE=\"%d\">\n", EventsShown);
			}

			/* write the checkbox for the event */
			printf("<TD> <INPUT NAME=\"Merge%d\" TYPE=CHECKBOX CHECKED> </TD>\n", EventsShown);

		
			/* event id */
			printf("<TD> <A HREF=\"eqparam2html" EXE_EXT "?EVENT%u=On\" "
						"TARGET=\"Event Info\">%u</A> </TD>\n",
							pMergeBuffer[i].idEvent, pMergeBuffer[i].idEvent);
	
		
			/*Write the origin time in the form YYYY/MM/DD  HH:MM:SS */
			TempTime = (time_t) (pMergeBuffer[i].tOrigin);
			printf ("<TD>%s</TD>\n", EWDB_ttoa (&TempTime, pTimeBuffer));
	
		
			/* Write the location fields */
			printf ("<TD> %2.2f </TD>\n",pMergeBuffer[i].dLat);
			printf ("<TD> %3.2f </TD>\n",pMergeBuffer[i].dLon);
			printf ("<TD> %1.1f </TD>\n",pMergeBuffer[i].dDepth);
	
			/* Write the magnitude fields */
			if (EventInfo.iMd >= 0)
				printf ("<TD>%1.1f</TD>\n", EventInfo.Mags[EventInfo.iMd].dMagAvg);
			else
				printf ("<TD>---</TD>\n");
	
			if (EventInfo.iML >= 0)
				printf ("<TD>%1.1f</TD>\n", EventInfo.Mags[EventInfo.iML].dMagAvg);
			else
				printf ("<TD>---</TD>\n");
	
			/* write the source */
			printf ("<TD> %s </TD>\n",pMergeBuffer[i].szHumanReadable);
	
			/* Put up the traces link */
			printf ("<TD bgcolor=#00FF00> <A HREF=\"ora2snippet_gif?EventID=%u\" "
						"TARGET=\"Trace for %u\">%d</A></TD>\n",
							pMergeBuffer[i].idEvent, pMergeBuffer[i].idEvent, NumTraces);
	
	
			/* close the row */
			printf ("</TR>\n");

			EventsShown = EventsShown + 1;

		}
	
		/* close the table */
		printf ("</TABLE>\n");

	} /* if we have at least one automatic merge candidate */


	printf ("<CENTER><BR><BR><HR><BR><BR>\n");

	/* Now, display the located events within the desired timeframe */

	/* Initialize EVMax and EVMin to blank */
	memset(&EVMax,0,sizeof(EWDB_MergeList));
	memset(&EVMin,0,sizeof(EWDB_MergeList));

	EVMax.dLat=0.0;
	EVMin.dLat=0.0;
	EVMax.dLon=0.0;
	EVMin.dLon=0.0;

	EVMin.tOrigin = minTime;
	EVMax.tOrigin = maxTime;


	if (ewdb_api_GetMergeList (pMergeBuffer, MAX_MERGES_PER_EVENT, -1, &EVMin, 
			&EVMax, &NumFound, &NumRetr) != EWDB_RETURN_SUCCESS)
	{
		html_logit ("", "Call to ewdb_api_GetMergeList failed.\n");
		return EW_FAILURE;
	} 

logit("","GetMergeList done, found=%d, retr=%d.\n", NumFound, NumRetr);

	/* Consolidate the list of merges according to phenomena */

logit ("", "Before Consolidating %d merges\n", NumRetr);
for (i = 0; i < NumRetr; i++)
 logit ("", "%d (%d): %d %d   %f, %f, %f  <%s>\n", i,
   pMergeBuffer[i].idPh,
   pMergeBuffer[i].idPrefEvent,
   pMergeBuffer[i].idEvent,
   pMergeBuffer[i].dLat,
   pMergeBuffer[i].dLon,
   pMergeBuffer[i].dPrefMag,
   pMergeBuffer[i].szHumanReadable);


	if ((pConsList = (EWDB_PhenomenaMergeList *) malloc 
			(NumRetr * sizeof (EWDB_PhenomenaMergeList))) == NULL)
	{
		html_logit ("", "Could not malloc space for consolidate merge list.\n");
		return EW_FAILURE;
	}

	if (EWDB_ConsolidateMergeList (pMergeBuffer, 
			NumRetr, pConsList, &NumPhenomena) != EW_SUCCESS)
	{
		html_logit ("", "Call to ConsolidateMergeList failed.\n");
		return EW_FAILURE;
	}

logit ("", "Consolidated into %d phenomena\n", NumPhenomena);
for (i = 0; i < NumPhenomena; i++)
 logit ("", "%d (%d): %d ==> %d  %f, %f, %f  <%s>\n", i,
   pConsList[i].NumMerges,
   pConsList[i].idPh,
   pConsList[i].PrefEvt.Event.idEvent,
   pConsList[i].PrefEvt.dLat,
   pConsList[i].PrefEvt.dLon,
   pConsList[i].PrefEvt.dPrefMag,
   pConsList[i].szHumanReadable);


	if (NumPhenomena < 2)
	{
		printf ("<CENTER><BR><BR><HR><BR><BR>\n");
		printf ("<STRONG>There are no other potential merge candidates</STRONG>.\n<BR><BR>");
	}
	else
	{
		/* Now, list info for all potential merges */

		printf("<CENTER><PRE>\n");
	
		printf ("<STRONG>Below is the list of %d nearby located events.</STRONG>\n\n", (NumPhenomena-1));
	
		printf("To see more information about an event, click on the EventID.\n");
	
		printf ("To see a snapshot of the waveforms for each event, click on \n"
					"the link in the 'Traces' column.\n\n"
					"To merge the event with the current event,\n"
					"make sure that the Merge? button for that event is checked.\n\n"
				"When review of all events is completed, click Process Merges button.\n\n");
 

		printf ("</PRE>\n");
		/*  Write the table header */
		printf ("<TABLE BORDER=1> <TR BGCOLOR=#2222AA VALIGN=TOP ALIGN=CENTER>\n");

		printf ("<TD VALIGN=CENTER><STRONG><FONT SIZE=\"+1\" COLOR=#FFFFFF>Merge?</TD>\n");
		printf ("<TD VALIGN=CENTER><STRONG><FONT SIZE=\"+1\" COLOR=#FFFFFF>EventID</TD>\n");
		printf ("<TD VALIGN=CENTER><STRONG><FONT SIZE=\"+1\" COLOR=#FFFFFF>Origin Time</TD>\n");
		printf ("<TD VALIGN=CENTER><STRONG><FONT SIZE=\"+1\" COLOR=#FFFFFF>Lat</TD>\n");
		printf ("<TD VALIGN=CENTER><STRONG><FONT SIZE=\"+1\" COLOR=#FFFFFF>Lon</TD>\n");
		printf ("<TD VALIGN=CENTER><STRONG><FONT SIZE=\"+1\" COLOR=#FFFFFF>Depth</TD>\n");
		printf ("<TD VALIGN=CENTER><STRONG><FONT SIZE=\"+1\" COLOR=#FFFFFF>Md</TD>\n");
		printf ("<TD VALIGN=CENTER><STRONG><FONT SIZE=\"+1\" COLOR=#FFFFFF>ML</TD>\n");
		printf ("<TD VALIGN=CENTER><STRONG><FONT SIZE=\"+1\" COLOR=#FFFFFF>Source</TD>\n");
		printf ("<TD VALIGN=CENTER><STRONG><FONT SIZE=\"+1\" COLOR=#FFFFFF>Traces</TD>\n");

	    printf("</TR>\n");
	
		for (i = 0; i < NumPhenomena; i++)
		{
			if (pConsList[i].PrefEvt.Event.idEvent != idPrefEvent)
			{
				printf ("<INPUT NAME=Evt%d TYPE=HIDDEN VALUE=\"%d\">\n", EventsShown, 
													pConsList[i].PrefEvt.Event.idEvent);
				printf ("<INPUT NAME=Ph%d TYPE=HIDDEN VALUE=\"%d\">\n", EventsShown, 
													pConsList[i].idPh);
				free (EventInfo.pChanInfo);
	
				/* Retrieve info on the initial (triggering) event */
				if (InitEWEvent (&EventInfo) != EW_SUCCESS)
				{
					logit ("", "Call to InitEWEvent failed.\n");
			        html_break();
					goto shutdown;
				}
		
				if (ewdb_apps_GetDBEventInfoII (&EventInfo, pConsList[i].PrefEvt.Event.idEvent,
														-1, DBFlags) != EWDB_RETURN_SUCCESS)
				{
					logit ("", "Call to ewdb_apps_GetDBEventInfoII failed.\n");
   		    		html_break();
					goto shutdown;
				}
		
				/* Count the number of traces */
				NumTraces = 0;
				for (j = 0; j < EventInfo.iNumChans; j++)
				{
					if (EventInfo.pChanInfo[j].iNumWaveforms > 0)
						NumTraces = NumTraces + 1;
				}
		
				printf("<TR ALIGN=CENTER>\n");
		
				/* write the checkbox for the event */
				printf("<TD> <INPUT NAME=\"Merge%d\" TYPE=CHECKBOX> </TD>\n", EventsShown);
			
				/* event id */
				printf("<TD> <A HREF=\"eqparam2html" EXE_EXT "?EVENT%u=On\" "
							"TARGET=\"Event Info\">%u</A> </TD>\n",
								pConsList[i].PrefEvt.Event.idEvent,
								pConsList[i].PrefEvt.Event.idEvent);
		
			
				/*Write the origin time in the form YYYY/MM/DD  HH:MM:SS */
				TempTime = (time_t) (EventInfo.PrefOrigin.tOrigin);
				printf ("<TD>%s</TD>\n", EWDB_ttoa (&TempTime, pTimeBuffer));
		
			
				/* Write the location fields */
				printf ("<TD> %2.2f </TD>\n",EventInfo.PrefOrigin.dLat);
				printf ("<TD> %3.2f </TD>\n",EventInfo.PrefOrigin.dLon);
				printf ("<TD> %1.1f </TD>\n",EventInfo.PrefOrigin.dDepth);
		
				/* Write the magnitude fields */
				if (EventInfo.iMd >= 0)
					printf ("<TD>%1.1f</TD>\n", EventInfo.Mags[EventInfo.iMd].dMagAvg);
				else
					printf ("<TD>---</TD>\n");
		
				if (EventInfo.iML >= 0)
					printf ("<TD>%1.1f</TD>\n", EventInfo.Mags[EventInfo.iML].dMagAvg);
				else
					printf ("<TD>---</TD>\n");
		
				/* write the source */
				printf ("<TD> %s </TD>\n",EventInfo.PrefOrigin.sSource);
		
				/* Put up the traces link */
				printf ("<TD bgcolor=#00FF00> <A HREF=\"ora2snippet_gif?EventID=%u\" "
							"TARGET=\"Trace for %u\">%d</A></TD>\n",
								pConsList[i].PrefEvt.Event.idEvent,
								pConsList[i].PrefEvt.Event.idEvent, NumTraces);
		
		
				/* close the row */
				printf ("</TR>\n");

				EventsShown = EventsShown + 1;

			} /* if this is not the preferred event */
	
		} /* Loop over phenomena */

		/* close the table */
		printf ("</TABLE>\n");

	} /* if we have at least one nearby phenomenon */

	printf ("<CENTER><BR><BR><HR><BR><BR>\n");



	/* Now, display the coincidence events within the desired timeframe */

	NumFound = NumRetr = 0;


	if ((pCoinc = (EWDB_CoincEventStruct *) malloc 
			(MAX_MERGES_PER_EVENT * sizeof (EWDB_CoincEventStruct))) == NULL)
	{
		html_logit ("", "Could not malloc space for coincidence list.\n");
		return EW_FAILURE;
	}


	if (ewdb_api_GetCoincidence (-1, EWDB_EVENT_TYPE_COINCIDENCE, pCoinc, MAX_MERGES_PER_EVENT, 
			minTime, maxTime, &NumFound, &NumRetr) != EWDB_RETURN_SUCCESS)
	{
		html_logit ("", "Call to ewdb_api_GetCoincidence failed.\n");
		return EW_FAILURE;
	} 

logit("","ewdb_api_GetCoincidence done, found=%d, retr=%d.\n", NumFound, NumRetr);

	if (NumRetr < 1)
	{
		printf ("<CENTER><BR><BR><HR><BR><BR>\n");
		printf ("<STRONG>There are no other potential coincidence merge candidates</STRONG>.\n<BR><BR>");
	}
	else
	{
		/* Now, list info for all potential merges */

		printf("<CENTER><PRE>\n");
	
		printf ("<STRONG>Below is the list of %d nearby coincidence events.</STRONG>\n\n", NumRetr);
	
		printf("To see more information about an event, click on the EventID.\n");
	
		printf ("To see a snapshot of the waveforms for each event, click on \n"
					"the link in the 'Traces' column.\n\n"
					"To merge the event with the current event,\n"
					"make sure that the Merge? button for that event is checked.\n\n"
				"When review of all events is completed, click Process Merges button.\n\n");
 

		printf ("</PRE>\n");
		/*  Write the table header */
		printf ("<TABLE BORDER=1> <TR BGCOLOR=#2222AA VALIGN=TOP ALIGN=CENTER>\n");

		printf ("<TD VALIGN=CENTER><STRONG><FONT SIZE=\"+1\" COLOR=#FFFFFF>Merge?</TD>\n");
		printf ("<TD VALIGN=CENTER><STRONG><FONT SIZE=\"+1\" COLOR=#FFFFFF>EventID</TD>\n");
		printf ("<TD VALIGN=CENTER><STRONG><FONT SIZE=\"+1\" COLOR=#FFFFFF>Coincidence Time</TD>\n");
		printf ("<TD VALIGN=CENTER><STRONG><FONT SIZE=\"+1\" COLOR=#FFFFFF>Source</TD>\n");
		printf ("<TD VALIGN=CENTER><STRONG><FONT SIZE=\"+1\" COLOR=#FFFFFF>Traces</TD>\n");

	    printf("</TR>\n");
	
		for (i = 0; i < NumRetr; i++)
		{

			printf ("<INPUT NAME=Evt%d TYPE=HIDDEN VALUE=\"%d\">\n", EventsShown, 
															pCoinc[i].idEvent);
			printf ("<INPUT NAME=Ph%d TYPE=HIDDEN VALUE=\"%d\">\n", EventsShown, -100);
	
			free (EventInfo.pChanInfo);

			/* Retrieve info on the initial (triggering) event */
			if (InitEWEvent (&EventInfo) != EW_SUCCESS)
			{
				logit ("", "Call to InitEWEvent failed.\n");
		        html_break();
				goto shutdown;
			}
	
			if (ewdb_apps_GetDBEventInfoII (&EventInfo, pCoinc[i].idEvent,
													-1, DBFlags) != EWDB_RETURN_SUCCESS)
			{
				logit ("", "Call to ewdb_apps_GetDBEventInfoII failed.\n");
   	    		html_break();
				goto shutdown;
			}

		
			/* Count the number of traces */
			NumTraces = 0;
			for (j = 0; j < EventInfo.iNumChans; j++)
			{
				if (EventInfo.pChanInfo[j].iNumWaveforms > 0)
					NumTraces = NumTraces + 1;
			}
	
			printf("<TR ALIGN=CENTER>\n");
		
			/* write the checkbox for the event */
			printf("<TD> <INPUT NAME=\"Merge%d\" TYPE=CHECKBOX> </TD>\n", EventsShown);
			
			/* event id */
			printf("<TD> <A HREF=\"eqparam2html" EXE_EXT "?EVENT%u=On\" "
						"TARGET=\"Event Info\">%u</A> </TD>\n",
							pCoinc[i].idEvent,
							pCoinc[i].idEvent);
	
			
			/*Write the origin time in the form YYYY/MM/DD  HH:MM:SS */
			TempTime = (time_t) (EventInfo.CoincEvt.tCoincidence);
			printf ("<TD>%s</TD>\n", EWDB_ttoa (&TempTime, pTimeBuffer));
		
			/* write the source */
			printf ("<TD> %s </TD>\n",EventInfo.CoincEvt.szHumanReadable);
		
			/* Put up the traces link */
			printf ("<TD bgcolor=#00FF00> <A HREF=\"ora2snippet_gif?EventID=%u\" "
						"TARGET=\"Trace for %u\">%d</A></TD>\n",
							pCoinc[i].idEvent,
							pCoinc[i].idEvent, NumTraces);
	
		
			/* close the row */
			printf ("</TR>\n");

			EventsShown = EventsShown + 1;

		} /* Loop over coincidences */

		/* close the table */
		printf ("</TABLE>\n");

	} /* if we have at least one nearby coincidence */

	printf ("<CENTER><BR><BR><HR><BR><BR>\n");
	printf ("<INPUT NAME=NumEvents TYPE=HIDDEN VALUE=\"%d\">\n", EventsShown);

	
	/* Write the submit button at the bottom of the table */
	printf("<INPUT TYPE=\"SUBMIT\" VALUE=\"Process Merges\">\n<br>\n");

	printf ("<BR><BR>\n");
	
	/* Close the form */
	printf ("</FORM>\n");



shutdown:
    ewdb_api_Shutdown();
    html_trailer (WebHost, FooterLogo);
    logit("","merge_list: terminating\n" );

    return (0);
}


int Webparse_Client_SetVars(char * szVar, char * szVal, void * pUserParams)
{

	WebOpts * pOptions= (WebOpts *) pUserParams;

/*
logit ("", "szVar=<%s>, szVal=<%s>\n", szVar, szVal);
*/


	if(strcmp(szVar,"idPh") == 0)
	{
		pOptions->idPh = atoi(szVal);
	}
	else
	{
		html_logit ("" ,"Unrecognized Web Option : %s = %s\n",szVar,szVal);
	}
	return(0);

}  /* End of SetVars() */


