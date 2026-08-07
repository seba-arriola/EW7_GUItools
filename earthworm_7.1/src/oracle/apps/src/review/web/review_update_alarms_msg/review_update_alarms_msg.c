
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: review_update_alarms_msg.c,v 1.7 2003/03/11 17:15:16 lucky Exp $
 *
 *    Revision history:
 *    $Log: review_update_alarms_msg.c,v $
 *    Revision 1.7  2003/03/11 17:15:16  lucky
 *    *** empty log message ***
 *
 *    Revision 1.6  2002/05/28 17:27:23  lucky
 *    *** empty log message ***
 *
 *    Revision 1.5  2002/03/22 20:03:33  lucky
 *    Pre v6.1 checkin
 *
 *    Revision 1.4  2001/08/07 16:53:49  lucky
 *    Pre v6.0 checkin
 *
 *    Revision 1.3  2001/07/28 00:45:23  lucky
 *     State of the code after debugging and testing prior to v6.0 release freeze.
 *
 *    Revision 1.2  2001/07/01 21:55:34  davidk
 *    Cleanup of the Earthworm Database API and the applications that utilize it.
 *    The "ewdb_api" was cleanup in preparation for Earthworm v6.0, which is
 *    supposed to contain an API that will be backwards compatible in the
 *    future.  Functions were removed, parameters were changed, syntax was
 *    rewritten, and other stuff was done to try to get the code to follow a
 *    general format, so that it would be easier to read.
 *
 *    Applications were modified to handle changed API calls and structures.
 *    They were also modified to be compiler warning free on WinNT.
 *
 *    Revision 1.1  2001/06/21 21:26:25  lucky
 *    Initial revision
 *
 *
 *
 */
  

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

#define		UNKNOWN	-1
#define		ALL		0
#define		CURRENT	1

#define     INIT_NUM_ALARMS     100


typedef struct _webstruct
{
	int		idEvent;
	int		idOrigin;
	int		AlarmNo;
	int		Mode;			/* All, or Current only */
	int		isInsert;
	char	AlarmMsg[ALARM_MSG_SIZE];
} WebStruct;


main ()
{
	WebStruct		 			WebParams;
	char						*configfile = "../params/review_event.d";
	char						filename[512];
	FILE						*fp;
	AlarmList					*pAlarms;
	EWEventInfoStruct			EventInfo;
	int							NumRetr, NumFound, i, j, ret, idFormat;
	char						EvtDir[512];
	char						tmpstr[512];
	DetAlmsParams				DAP;

	/* Initialize some stuff
	 ***********************/
	DEBUG = 0;
	sprintf (tmpstr, "");

	/* Send html header back to web server
	 *************************************/
	printf("Content-type: text/html\n\n");
	printf("<html>\n");
	printf("<HEAD><TITLE>Message Review Confirmation</TITLE></HEAD>\n" 
				"<BODY BGCOLOR=\"lemonchiffon\" TEXT=#333333>\n" );


	/* Read the configuration file (path hardcoded relative to executable)
	 *********************************************************************/
	ReadConfig (configfile);

	/* Cleanup, logfile should match config file name */
	logit_init ("review_update_alarms_msg",1,MAX_BYTES_PER_EQ,1);  


	/* initialize Web Parameters to all zeros */
	memset (&WebParams, 0, sizeof(WebOptionsStruct));

	WebParams.Mode = UNKNOWN;

	if (Webparse_GetAndProcessWebParams((void *)(&WebParams)) != 0)
	{
		html_logit ("", "Call to Webparse_GetAndProcessWebParams failed.\n");
		html_break ();
		return EW_FAILURE;
	}


	if (WebParams.Mode == CURRENT)
	{
		/* Write the message out to the file */

		sprintf (filename, "%s%c%d%cAlarmsMsg_%d.bin",
					ReviewTmpDir, DIR_SLASH,
					WebParams.idOrigin, DIR_SLASH, WebParams.AlarmNo);

		if ((fp = fopen (filename, "wb")) == NULL)
		{
			html_logit ("", "Can't open %s.\n", filename);
			return EW_FAILURE;
		}
	
		fwrite (WebParams.AlarmMsg, sizeof (char), ALARM_MSG_SIZE, fp);
		fclose (fp);
	}
	else if (WebParams.Mode == ALL)
	{

		/* Open connection to database
		 *****************************/
		if (ewdb_api_Init (DBuser, DBpassword, DBservice ) != 0 )
		{
			html_logit ("", "Trouble connecting to database; exiting!\n" );
			return EW_FAILURE;
		}


		/* Read in the EVENT file */
		sprintf (EvtDir, "%s%c%d", ReviewTmpDir, DIR_SLASH, WebParams.idOrigin);

		sprintf (filename, "%s%cEvtStruct.bin", EvtDir, DIR_SLASH);

		if ((fp = fopen (filename, "rb")) == NULL)
		{
			html_logit ("", "Can't open %s\n", filename);
			return EW_FAILURE;
		}
	
		fread ((void *) &EventInfo, sizeof (EWEventInfoStruct), 1, fp);
	
		/* Read the channel info structs */
		if ((EventInfo.pChanInfo = (EWChannelDataStruct *) malloc
					(EventInfo.iNumChans * sizeof (EWChannelDataStruct))) == NULL)
		{
			html_logit ("", "Can't malloc pChanInfo.\n");
			return EW_FAILURE;
		}
		EventInfo.iNumAllocChans = EventInfo.iNumChans;

	    fread ((void *) EventInfo.pChanInfo, sizeof (EWChannelDataStruct),
                                                        EventInfo.iNumChans, fp);

		fclose (fp);


		EventInfo.PrefOrigin.idEvent = EventInfo.Event.idEvent;
		EventInfo.PrefOrigin.BindToEvent = TRUE;
		EventInfo.PrefOrigin.SetPreferred = TRUE;
	
		EventInfo.Mags[EventInfo.iPrefMag].idEvent = EventInfo.Event.idEvent;
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
			return EW_FAILURE;
		}
	

		/* Fill in the Parameter struct to be passed into DetermineAlarms */
		DAP.isAuto = 0;
		DAP.isInsert = WebParams.isInsert;;
		DAP.minPopulation = Alarms_minPopulation;
		DAP.showPopulation = Alarms_showPopulation;
		DAP.networkCode = tmpstr;
		DAP.InvocationString = tmpstr;

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
			return EW_FAILURE;
		}

		if (NumRetr < NumFound)
		{
			logit ("", "Alarm list too small - got %d alarms, allocating.\n", NumRetr);

			free (pAlarms);
	
			if ((pAlarms = (AlarmList *) malloc (NumRetr * 
								sizeof (AlarmList))) == NULL)
			{
				html_logit ("", "Could not malloc alarms buffer \n");
				return EW_FAILURE;
			}
		
			/* Fill in the Parameter struct to be passed into DetermineAlarms */
			DAP.isAuto = 0;
			DAP.isInsert = WebParams.isInsert;;
			DAP.minPopulation = Alarms_minPopulation;
			DAP.showPopulation = Alarms_showPopulation;
			DAP.networkCode = tmpstr;
			DAP.InvocationString = tmpstr;

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
				return EW_FAILURE;
			}
		}


		/* Figure out what format the original message was in */
		idFormat = pAlarms[WebParams.AlarmNo].Audit.Format.idFormat;

		for (i = 0; i < NumRetr; i++)
		{

			/* Check format */

			if (pAlarms[i].Audit.Format.idFormat == idFormat)
			{
				/* Write the message out to the file */

				sprintf (filename, "%s%c%d%cAlarmsMsg_%d.bin",
					ReviewTmpDir, DIR_SLASH, WebParams.idOrigin, DIR_SLASH, i);

				if ((fp = fopen (filename, "wb")) == NULL)
				{
					html_logit ("", "Can't open %s.\n", filename);
					return EW_FAILURE;
				}
	
				fwrite (WebParams.AlarmMsg, sizeof (char), ALARM_MSG_SIZE, fp);
				fclose (fp);

			}

		} /* For loop over alarms */

	} /* Update ALL  of this format */
	else
	{
		html_logit ("", "Update mode must be specified: ALL or CURRENT.\n");
		return EW_FAILURE;
	}

	

	printf ("<STRONG><CENTER><PRE>Alarm Message updated.</STRONG>\n\n");
	printf ("Close this window to continue with the review process.\n");

  return(0);
}  /* end main */


/*********************************************************************/
int Webparse_Client_SetVars(char * szVar, char * szVal, void * pUserParams)
{

	WebStruct *pOptions = (WebStruct *) pUserParams;
	

/*
logit ("", "szVar=%s, szVal=%s\n", szVar, szVal);
*/

    if(strcmp(szVar,"EventID") == 0)
    {
		pOptions->idEvent = atoi (szVal);
    }
    else if(strcmp(szVar,"OriginID") == 0)
    {
		pOptions->idOrigin = atoi (szVal);
    }
    else if(strcmp(szVar,"AlarmNo") == 0)
    {
		pOptions->AlarmNo = atoi (szVal);
    }
    else if(strcmp(szVar,"alarmmsg") == 0)
    {
		strcpy (pOptions->AlarmMsg, szVal);
    }
	else if(strcmp(szVar,"All") == 0)
    {
		pOptions->Mode = ALL;
	}
	else if(strcmp(szVar,"Current") == 0)
    {
		pOptions->Mode = CURRENT;
	}
	else if(strcmp(szVar,"isInsert") == 0)
    {
		pOptions->isInsert = atoi (szVal);
	}
    else
    {
      html_logit ("" ,"Unrecognized Web Option : %s = %s\n",szVar,szVal);
    }
    return(0);
}  /* End of SetVars() */
