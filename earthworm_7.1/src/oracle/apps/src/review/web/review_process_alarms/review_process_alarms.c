
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: review_process_alarms.c,v 1.10 2003/03/11 17:15:15 lucky Exp $
 *
 *    Revision history:
 *     $Log: review_process_alarms.c,v $
 *     Revision 1.10  2003/03/11 17:15:15  lucky
 *     *** empty log message ***
 *
 *     Revision 1.9  2002/05/28 19:34:48  lucky
 *     Added header and footer tag text
 *
 *     Revision 1.8  2002/05/28 17:27:23  lucky
 *     *** empty log message ***
 *
 *     Revision 1.7  2002/03/22 20:03:33  lucky
 *     Pre v6.1 checkin
 *
 *     Revision 1.6  2001/08/07 16:53:49  lucky
 *     Pre v6.0 checkin
 *
 *     Revision 1.5  2001/07/31 20:47:20  lucky
 *     Changed alarms nomenclature from User to Recipient
 *
 *     Revision 1.4  2001/07/28 00:45:23  lucky
 *      State of the code after debugging and testing prior to v6.0 release freeze.
 *
 *     Revision 1.3  2001/07/01 21:55:33  davidk
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
 *     Revision 1.2  2001/06/26 18:09:50  lucky
 *     Added NetowrkCode as a parameter to DetermineAlarms
 *
 *     Revision 1.1  2001/06/21 21:26:25  lucky
 *     Initial revision
 *
 *
 *************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

/* include earthworm headers */
#include <time_ew.h>  /* DavidK: done for perf. timing */
#include <ewdb_ora_api.h> 
#include <alarms.h>
#include <ewdb_apps_utils.h>
#include <webparse.h>
#include <time_functions.h>
#include <review.h>
#include <review_globals.h>


#define     INIT_NUM_ALARMS     100

main ()
{
	WebOptionsStruct 			WebOpts;
	EWEventInfoStruct			EventInfo;
	char						*configfile = "../params/review_event.d"; 
	AlarmList					*pAlarms;
	int							i, j, del, isInsert, NumRetr, NumFound, ret;
	char						EvtDir[512];
	char						filename[512];
	char						InvString[512];
	FILE						*fp;
	DetAlmsParams				DAP;
	

	/* Initialize some stuff
	 ***********************/
	DEBUG = 0;

	/* Read the configuration file (path hardcoded relative to executable)
	 *********************************************************************/
	ReadConfig (configfile);

	/* Cleanup, logfile should match config file name */
	logit_init ("review_process_alarms",1,MAX_BYTES_PER_EQ,1);  

	/* Send html header back to web server
	 *************************************/
   printf("Content-type: text/html\n\n");
   printf("<html>\n");
   printf("<HEAD><TITLE>Alarms Confirmation</TITLE></HEAD>\n");

	html_header (BackgroundColor, HeaderLogo, HeaderTag);

    printf("<H2><CENTER><FONT COLOR=red>Alarms Confirmation Page</FONT></H2>\n");
    printf("</center>\n");



	/* initialize Web Parameters to all zeros */
	memset (&WebOpts, 0, sizeof(WebOptionsStruct));
	if (Webparse_GetAndProcessWebParams((void *)(&WebOpts)) != 0)
	{
		html_logit ("", "Call to Webparse_GetAndProcessWebParams failed.\n");
		html_break ();
		goto shutdown;
	}


	/* Open connection to database
	 *****************************/
	if (ewdb_api_Init (DBuser, DBpassword, DBservice ) != 0 )
	{
		html_logit ("", "Trouble connecting to database; exiting!\n" );
		html_break();
		goto shutdown;
	}


	/* Read in the EVENT file */
	sprintf (EvtDir, "%s%c%d", ReviewTmpDir, DIR_SLASH, WebOpts.idOrigin);

	sprintf (filename, "%s%cEvtStruct.bin", EvtDir, DIR_SLASH);

	if ((fp = fopen (filename, "rb")) == NULL)
	{
		html_logit ("", "Can't open %s\n", filename);
		html_break ();
		goto shutdown;	
	}

	fread ((void *) &EventInfo, sizeof (EWEventInfoStruct), 1, fp);

    /* Read the channel info structs */
    if ((EventInfo.pChanInfo = (EWChannelDataStruct *) malloc
                   (EventInfo.iNumChans * sizeof (EWChannelDataStruct))) == NULL)
    {
        html_logit ("", "Can't malloc pChanInfo.\n");
        html_break ();
        goto shutdown;
    }
    EventInfo.iNumAllocChans = EventInfo.iNumChans;

    fread ((void *) EventInfo.pChanInfo, sizeof (EWChannelDataStruct),
                                                        EventInfo.iNumChans, fp);

	fclose (fp);


	EventInfo.PrefOrigin.idEvent = WebOpts.idEvent;
	EventInfo.PrefOrigin.BindToEvent = TRUE;
	EventInfo.PrefOrigin.SetPreferred = TRUE;

	EventInfo.Mags[EventInfo.iPrefMag].idEvent = WebOpts.idEvent;
	EventInfo.Mags[EventInfo.iPrefMag].bBindToEvent = TRUE;
	EventInfo.Mags[EventInfo.iPrefMag].bSetPreferred = TRUE;


	for (i = 0; i < EventInfo.iNumChans; i++)
	{
	    for (j = 0; j < EventInfo.pChanInfo[i].iNumStaMags; j++)
		{
			EventInfo.pChanInfo[i].Stamags[j].idChan =
				EventInfo.pChanInfo[i].Arrivals[j].idChan;
			EventInfo.pChanInfo[i].Stamags[j].idMagnitude =
				EventInfo.Mags[EventInfo.iPrefMag].idMag;
		}
	}


	/* Retrieve the list of alarms from the DB */

	if ((pAlarms = (AlarmList *) malloc (INIT_NUM_ALARMS * 
						sizeof (AlarmList))) == NULL)
	{
		html_logit ("", "Could not malloc alarms buffer \n");
		html_break ();
		goto shutdown;
	}

	if (WebOpts.Type == ACT_INSERT)
	{
		isInsert = TRUE;

		/*
		 * Build the invocation string which may be included
		 * in the alarm messages to indicate where the alarm
		 * came from
		 */
		strcpy (InvString, "Event inserted into the database by a reviewer.");

	}
	else
	{
		isInsert = FALSE;
		strcpy (InvString, "Event deleted from the database by a revewier.");
	}


	/* Fill in the Parameter struct to be passed into DetermineAlarms */
	DAP.isAuto = 0;
	DAP.isInsert = isInsert;
	DAP.minPopulation = Alarms_minPopulation;
	DAP.showPopulation = Alarms_showPopulation;
	DAP.networkCode = NetworkCode;
	DAP.InvocationString = InvString;

    if (DetermineAlarms (&EventInfo, &DAP, pAlarms, INIT_NUM_ALARMS, 
								&NumFound, &NumRetr, &ret) != EW_SUCCESS)
   	{
		if (ret == -103)
			html_logit ("", "Could not process one or more messages. "
				"Check alarm formats for errors!\n"); 
		else if (ret = -102)
			html_logit ("", "Could not process one or more messages. "
				"Database access error!\n"); 
		else
			html_logit ("", "Could not process one or more messages. Fatal error!\n"); 

		logit ("", "Call to DetermineAlarms failed with ret=%d\n", ret);

        html_break ();
        goto shutdown;
	}

	if (NumRetr < NumFound)
	{
		logit ("", "Alarm list too small - got %d alarms, allocating.\n", NumRetr);

		free (pAlarms);

		if ((pAlarms = (AlarmList *) malloc (NumRetr * 
							sizeof (AlarmList))) == NULL)
		{
			html_logit ("", "Could not malloc alarms buffer \n");
   		    html_break ();
        	goto shutdown;
		}
	
		if (DetermineAlarms (&EventInfo, &DAP, pAlarms, NumRetr, &NumFound, 
												&NumRetr, &ret) != EW_SUCCESS)
		{
			if (ret == -103)
				html_logit ("", "Could not process one or more messages. "
					"Check alarm formats for errors!\n"); 
			else if (ret = -102)
				html_logit ("", "Could not process one or more messages. "
					"Database access error!\n"); 
			else
				html_logit ("", "Could not process one or more messages. Fatal error!\n"); 
	
			logit ("", "Call to DetermineAlarms failed with ret=%d\n", ret);
        	html_break ();
        	goto shutdown;
		}
	}


	for (i = 0; i < NumRetr; i++)
	{

		sprintf (filename, "%s%cAlarmsMsg_%d.bin", EvtDir, DIR_SLASH, i);

		if ((fp = fopen (filename, "wb")) == NULL)
		{
			html_logit ("", "Can't open %s\n", filename);
        	html_break ();
        	goto shutdown;
		}

		fwrite (pAlarms[i].AlarmMsg, sizeof (char), ALARM_MSG_SIZE,  fp);
		fclose (fp);
	}

		

	printf ("<HR><BR><CENTER><PRE><STRONG>\n");
	printf ("REVIEW ALARMS for event %d</STRONG>\n\n"
			"The following %d alarms will be issued. \n"
			"Click the Send? checkbox to cancel a particular alarm.\n\n"
			"Click CONTINUE to insert or delete the event and issue the alarms. \n"
			"The list of Phone alarms will be presented again on the next page.\n", 
							EventInfo.Event.idEvent, NumRetr);


	printf ("<BR><BR><P>\n");

	printf ("</PRE>\n");

	printf ("<FORM NAME=\"AlarmForm\" ACTION=\"review_process_event\" METHOD=POST>\n");


	if (NumRetr != 0)
	{
		/* 
		 * Write the text of alarm message out to files, so 
		 * that the user can review and modify it
		 */
	
		sprintf (filename, "%s%cAlarmsStruct.bin", EvtDir, DIR_SLASH);
	
		if ((fp = fopen (filename, "wb")) == NULL)
		{
			html_logit ("", "Could not open alarm file %s.\n", filename);
			html_break ();
			goto shutdown;
		}
	
		fwrite (pAlarms, sizeof (AlarmList), NumRetr, fp);
		fclose (fp);

		printf ("<INPUT TYPE=hidden NAME=\"AlarmsFile\" VALUE=%s>\n", filename);
		printf ("<INPUT TYPE=hidden NAME=\"Type\" VALUE=%d>\n", WebOpts.Type);
		printf ("<INPUT TYPE=hidden NAME=\"NumAlarms\" VALUE=%d>\n", NumRetr);
		printf ("<INPUT TYPE=hidden NAME=\"Action\" VALUE=%d>\n", ACT_ALARMS_YES);
		printf ("<INPUT TYPE=hidden NAME=\"EventID\" VALUE=%d>\n", 
									EventInfo.Event.idEvent);
		printf ("<INPUT TYPE=hidden NAME=\"OriginID\" VALUE=%d>\n", 
									EventInfo.PrefOrigin.idOrigin);

		printf ("<TABLE border=2 cellspacing=2>");

		printf ("<TR align=center>\n");
		printf ("<TD ALIGN=center COLSPAN=2>Send?</TD>\n");
		printf ("<TD ALIGN=center COLSPAN=5>Recipient</TD>\n");
		printf ("<TD ALIGN=center COLSPAN=3>Method</TD>\n");
		printf ("<TD ALIGN=center COLSPAN=5>Delivery Point</TD>\n");
		printf ("<TD ALIGN=center COLSPAN=3>Format</TD>\n");
		printf ("</TR>\n");

		/* 
		 * Story: we want to display alarms grouped by the 
		 * delivery mechanism (emails, pagers, etc) -- this makes
		 * it easier to see which alarms should or should not be
		 * sent. Unfortunately, we don't want to re-sort the alarm
	  	 * list because it is already sorted in the priority order.
		 *
		 * So, the solution is to go through the list several times, 
		 * each time putting up only the particular delivery types.
	 	 */

		for (del = 0; del < EWDB_ALARMS_DELIVERY_NUM_MECHANISMS; del++)
		{
			for (i = 0; i < NumRetr; i++)
			{
				if (pAlarms[i].Delivery.DelMethodInd == DelMethodArray[del].idMethod)
				{

					printf ("<TR align=center>\n");
	
					/* Put up the select checkbox */
					printf ("<TD ALIGN=center COLSPAN=2> "
								"<INPUT NAME=\"Use_%d\" TYPE=CHECKBOX CHECKED></TD>\n", i); 
	
					/* recipient */
					if (pAlarms[i].Audit.Recipient.sDescription != NULL)
						printf ("<TD COLSPAN=5>%s</TD>\n", 
										pAlarms[i].Audit.Recipient.sDescription);
					else
						printf ("<TD COLSPAN=5>  </TD>\n");


					/* Show alarm info */
					/* delivery mechanism */
					if (pAlarms[i].Delivery.DelMethodInd == EWDB_ALARMS_DELIVERY_IND_EMAIL)
					{
						printf ("<TD COLSPAN=3>Email</TD>\n");
						printf ("<TD COLSPAN=5>%s</TD>\n", pAlarms[i].Delivery.email.sAddress);
					}
					else if (pAlarms[i].Delivery.DelMethodInd == EWDB_ALARMS_DELIVERY_IND_PAGER)
					{
						printf ("<TD COLSPAN=3>Pager</TD>\n");
						printf ("<TD COLSPAN=5>%s</TD>\n", pAlarms[i].Delivery.pager.sNumber);
					}
					else if (pAlarms[i].Delivery.DelMethodInd == EWDB_ALARMS_DELIVERY_IND_PHONE)
					{
						printf ("<TD COLSPAN=3>Phone</TD>\n");
						printf ("<TD COLSPAN=5>%s</TD>\n", pAlarms[i].Delivery.phone.sPhoneNumber);
					}
					else if (pAlarms[i].Delivery.DelMethodInd == EWDB_ALARMS_DELIVERY_IND_QDDS)
					{
						printf ("<TD COLSPAN=3>QDDS</TD>\n");
						printf ("<TD COLSPAN=5>%s</TD>\n", 
									pAlarms[i].Delivery.qdds.sQddsDirectory);
					}
					else if (pAlarms[i].Delivery.DelMethodInd == EWDB_ALARMS_DELIVERY_IND_CUSTOM)
					{
						printf ("<TD COLSPAN=3>Custom</TD>\n");
						printf ("<TD COLSPAN=5>%s</TD>\n", 
									pAlarms[i].Delivery.custom.sDescription);
					}
					else
					{
						html_logit ("", "Unknown delivery method: %d.\n", 
												pAlarms[i].Delivery.DelMethodInd);
						return EW_FAILURE;
					}
			
					/* format */
					printf ("<TD COLSPAN=3><A HREF=\"review_alarm_msg?EventID=%u?"
							"OriginID=%u?AlarmNo=%u?isInsert=%d\" "
									" TARGET=\"Review Message\">%s</A></TD>\n", 
											EventInfo.Event.idEvent, 
											EventInfo.PrefOrigin.idOrigin, i, isInsert, 
											pAlarms[i].Audit.Format.sDescription);

					printf ("</TR>\n");

				} /* if this is the delivery method of interest */

			} /* loop over alarms */

		} /* loop over delivery methods */

		printf ("<TR><TD COLSPAN=18 ALIGN=center>\n");
		printf ("<INPUT TYPE=\"submit\" VALUE=\"CONTINUE\" NAME=\"Continue\">\n");

		printf("</TABLE></FORM>\n");

		printf ("<PRE>\n "
			"NOTE: The list of phone alarms will be presented "
			"after clicking CONTINUE.\n");

	} /* if we got any alarms */
	else
	{
		printf ("<CENTER>\n");
		printf ("<INPUT TYPE=\"submit\" VALUE=\"CONTINUE\" NAME=\"Continue\">\n");
		printf ("<INPUT TYPE=hidden NAME=\"Type\" VALUE=%d>\n", WebOpts.Type);
		printf ("<INPUT TYPE=hidden NAME=\"NumAlarms\" VALUE=0>\n");
	
		printf ("<INPUT TYPE=hidden NAME=\"EventID\" VALUE=%d>\n", 
									EventInfo.Event.idEvent);
		printf ("<INPUT TYPE=hidden NAME=\"OriginID\" VALUE=%d>\n", 
									EventInfo.PrefOrigin.idOrigin);

		printf("<P></PRE>\n");
		printf("</CENTER></TABLE></FORM>\n");
	}


shutdown:

	printf ("<BR>\n");
	html_trailer (WebHost, FooterLogo, FooterTag);
	logit("","review_process_alarms: terminating\n" );

	ewdb_api_Shutdown();
	return (0);

}  /* end main */



