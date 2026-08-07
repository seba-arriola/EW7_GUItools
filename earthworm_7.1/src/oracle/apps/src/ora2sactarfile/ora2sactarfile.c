/*
 *   This file is under RCS - do not modify unless you have
 *   checked it out using the command checkout.
 *
 *    $Id: ora2sactarfile.c,v 1.10 2002/05/28 19:34:19 lucky Exp $
 *    Revision history:
 *    $Log: ora2sactarfile.c,v $
 *    Revision 1.10  2002/05/28 19:34:19  lucky
 *    Added header and footer tag text
 *
 *    Revision 1.9  2002/05/28 17:25:23  lucky
 *    *** empty log message ***
 *
 *    Revision 1.8  2001/10/18 19:13:54  lucky
 *    Added support for NT.
 *
 *    Revision 1.7  2001/09/10 20:45:08  lucky
 *    Added support for download in ZIP format.
 *
 *    Revision 1.6  2001/07/01 21:55:26  davidk
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
 *    Revision 1.5  2001/06/07 17:16:25  lucky
 *    Removed ora2sac -- we now only carry ora2sactarfile
 *
 *    Revision 1.11  2001/05/15 02:15:30  davidk
 *    Moved functions around between the apps, DB API, and DB API INTERNAL
 *    levels.  Renamed functions and files.  Added support for amplitude
 *    magnitude types.  Reformatted makefiles.
 *
 *    Revision 1.10  2001/02/28 17:29:10  lucky
 *    Massive schema redesign and cleanup.
 *
 *    Revision 1.9  2001/02/13 18:26:09  lucky
 *    If a call to ewdb_api_GetWaveformSnippet does not return SUCCESS, free up
 *    malloc-ed space and set NumWaveforms to 0 so that that particular snippet
 *    is not archived.
 *
 *    Revision 1.8  2000/10/11 20:18:02  lucky
 *    Free up all allocated channels after we are done producing sac files
 *
 *    Revision 1.7  2000/10/10 22:02:55  lucky
 *    Added an if-statement to check if we have any waveforms to save before
 *    invoking SAC writing routines on a channel
 *
 *    Revision 1.6  2000/10/10 21:04:18  lucky
 *    Rewrote Sac writing mechanism so that snippet space is allocated one by one
 *    for each channel and then freed up as soon as it is written out.
 *
 *    Revision 1.5  2000/08/16 17:12:20  lucky
 *    Removed write_sac, added Sac2EWEvent
 *
 *    Revision 1.4  2000/08/09 16:32:37  lucky
 *    Lint cleanup
 *
 *    Revision 1.3  2000/08/07 19:40:21  lucky
 *    Using html_trailer from libsrc
 *
 *    Revision 1.2  2000/03/24 19:23:25  lucky
 *    *** empty log message ***
 *
 *    Revision 1.1  2000/03/24 18:25:43  lucky
 *    Initial revision
 *
 *
 *
 */
 

/*****************************************************************

   filename:       ora2sactarfile.c 

   description:   
     Given the list of events, retrieve data from the database
     and write it out in Sac Format. 

*****************************************************************/
 

#include <earthworm.h>
#include <ewdb_ora_api.h>
#include <ewdb_apps_utils.h>
#include <html_common.h>

#include <ora2sac.h>

#define   EVENT_LEN 5  /* length of string "EVENT" */


/* Configuration parameters for ora2sac */
char DBuser[50];
char DBservice[50];
char DBpassword[50];
char envEW_LOG[512];
char SACDataDir[APP_MAXWORD+1];
char BackgroundColor[512];
char HeaderLogo[512];
char FooterLogo[512];
char HeaderTag[512];
char FooterTag[512];
long TraceBufferLen;   /* bytes of largest snippet we're up for -
                                       from configuration file */
long MaxTraces;        /* max traces per message we'll ever deal with */
int  OraDebug;
char OutputFormat[APP_MAXWORD];

/* Configuration parameters for ora2sactarfile */
time_t              tOrigin;
struct tm          *ptmOrigin;
char 	           EventDate[256];
char                EventTime[256];
char                yrmondir[7];
char                eventdir[APP_MAXWORD];
char                BinDir[256];
char                EventID[APP_MAXWORD];

 /* FTP connection parameters */
char                FTPDir[256];
char                FTPHost[256];
int                 numWaveRetrieved;
char                cmd[512];




/**************************************************************

**************************************************************/

int		main (int argc, char **argv)
{

    EWEventInfoStruct   DBEvent;
    EWDB_WaveformStruct *pWaveform;
	int					*eventlist;             /* eventids to save                  */
	int					nevent;                 /* #eventids in list                 */
	int					eventid;                /* current eventid we're processing  */
	int					i,j, iev;
	char				frontdir[1024];
	char				*MY_PROG_NAME = "ora2sactarfile";
	char				*CONFIG_FILE = "../params/ora2sactarfile.d";


	/* Read the configuration file (path hardcoded relative to executable)
	*********************************************************************/
	ReadConfig (CONFIG_FILE); /* it exits if it isn't happy */

	/* Start up Logging
	*******************/
	logit_init (MY_PROG_NAME, 1, 1024, 1);

	logit ("", "startup. OraDebug=%d\n", OraDebug);

	/* Send html header back to web server
	*************************************/
   printf("Content-type: text/html\n\n");
   printf("<html>\n");
   printf("<HEAD><TITLE>ora2sactarfile: Download SAC files</TITLE></HEAD>\n");

	html_header (BackgroundColor, HeaderLogo, HeaderTag);
	html_instructions ();


	/* Get list of eventids from stdin
	 *********************************/
	if (InputEventIds (&eventlist, &nevent))
	{
		logit("o", "InputEventIds failed; exiting!\n" );
		html_break ();
		goto shutdown;
	}


	if (OraDebug > 0)
	{
		logit ("", "Starting %s for the following events: \n", MY_PROG_NAME);
		for (i = 0; i < nevent; i++)
			logit ("", "%d ", eventlist[i]);
		logit ("", "\n");
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

	if (WriteSAC_init (SACDataDir, OutputFormat,OraDebug) != EW_SUCCESS)
	{
		logit("e", "%s: Call to SACPA_init failed!\n", MY_PROG_NAME);
		return EW_FAILURE;
	}


	html_tableheader();

	/**************** LOOP OVER EVENTS ********************/

	for( iev = 0; iev < nevent; iev++ )
	{
		eventid = eventlist[iev];
    	memset(&DBEvent,0,sizeof(EWEventInfoStruct));

		if (ewdb_apps_GetDBEventInfo(&DBEvent,eventid) != EWDB_RETURN_SUCCESS)
		{
			logit("","%s:ewdb_apps_GetDBEventInfo() failed for idEvent %d\n",
												MY_PROG_NAME, eventid);
			continue;
		}

		/**** Figure out the event directory names for tarring *****/
		/* We need these for the tar file creation script to work */
		tOrigin = (time_t) DBEvent.PrefOrigin.tOrigin;
		ptmOrigin = gmtime (&tOrigin);
		sprintf (EventDate, "%04d%02d%02d",
				ptmOrigin->tm_year+1900, ptmOrigin->tm_mon+1, ptmOrigin->tm_mday);
		sprintf (EventTime, "%02d%02d", 
				ptmOrigin->tm_hour, ptmOrigin->tm_min);
		sprintf (EventID, "%d", eventid);
    
		sprintf( yrmondir, "%04d%02d", ptmOrigin->tm_year+1900, ptmOrigin->tm_mon+1 );
		sprintf( eventdir, "%04d%02d%02d_%02d%02d%02d_%02d",
				ptmOrigin->tm_year+1900, ptmOrigin->tm_mon+1, ptmOrigin->tm_mday, 
				ptmOrigin->tm_hour, ptmOrigin->tm_min, ptmOrigin->tm_sec,eventid );


		/* Loop through the Channels and write out snippets in SAC */
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
					logit("","Call to ProduceSAC_NextStationForEvent failed for Channel %d, idEvent %d.\n", i, eventid);
					return EW_FAILURE;
				}

	
				if (WriteSAC_NextStationForEvent (&(DBEvent.pChanInfo[i])) != EW_SUCCESS)
				{
					logit("","Call to WriteSAC_NextStationForEvent failed for Channel %d, idEvent %d.\n", i, eventid);
					return EW_FAILURE;
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
		
		numWaveRetrieved=0;
		for(i=0;i<DBEvent.iNumChans;i++)
		{
			/* we assume that we are using at most 1 waveform snippet per channel */
			if (DBEvent.pChanInfo[i].iNumWaveforms > 0)
				numWaveRetrieved++;
		}

		html_tablerow( eventid, FTPHost, FTPDir, yrmondir, eventdir, numWaveRetrieved);


		/* Call a script to create the tar file, compress it 
		and remove the actual SAC directory
		******************************************************/
#ifdef _WINNT
		/* 
		 * Solaris, being a real operating system, supports 
		 * easy use of scripts. NT, alas, does not. So, we have
		 * to call each step in the sequence separately.
		 */

		sprintf (frontdir, "%s/%s/%s", SACDataDir, yrmondir, eventdir);

		/* Remove any old stuff */
		sprintf (cmd, "\\cygwin\\bin\\rm -f c:%s*", frontdir);
		system (cmd);

		/* Create a tar file of the SAC directory */
		sprintf (cmd, "\\cygwin\\bin\\bash -c 'cd c:%s/%s; /cygdrive/c/cygwin/bin/tar -cf %s.tar %s'", 
			SACDataDir, yrmondir, eventdir, eventdir);
		system (cmd);

		
		/*
		 * Now, let's do some more huffing and puffing to create a zip 
		 * file. Since zip sounds more NT-ish, maybe it won't be as hard.
		 */
		sprintf (cmd, "\\cygwin\\bin\\bash -c 'cd c:%s/%s; /cygdrive/c/cygwin/bin/zip -1qr %s.zip %s", 
				SACDataDir, yrmondir, eventdir, eventdir);
		system (cmd);

		/* Now, let's clean up */
		sprintf (cmd, "\\cygwin\\bin\\rm -rf c:%s", frontdir);
		system (cmd);
#else
	
		sprintf (cmd, "%s/SAC_create_tarfile %d %s/%s %s",
				BinDir, eventid, SACDataDir, yrmondir, eventdir);
		system (cmd);
#endif _WINNT
		

	} /* end for each eventid from stdin */
   

shutdown:
  free(eventlist);
  ewdb_api_Shutdown();

  printf ("</table>\n");;

  html_trailer (FTPHost, FooterLogo, FooterTag);
	logit ("t","%s: terminating\n", MY_PROG_NAME);

	return(0);

}  /* end main() */



static	int InputEventIds(int ** pEventList, int * pNumEventsToRetrieve)
{
  Ora2SAC_EventListStruct Options;

  /* initialized Options to all zeros */
  memset( &Options, 0, sizeof(Ora2SAC_EventListStruct) );
  Webparse_GetAndProcessWebParams((void *) &Options);
  *pEventList=Options.pEvents;
  *pNumEventsToRetrieve=Options.LastEvent;

  return(SUCCESS);
}  /* End of InputEventIds() */
 


int 	Webparse_Client_SetVars(char * szVar, char * szVal, void * pUserParams)
{
  Ora2SAC_EventListStruct * pOptions = (Ora2SAC_EventListStruct *)pUserParams;
  logit("","Webparse_Client_SetVars():Expression: %s = %s\n",szVar,szVal);
  if(strcmp(szVar,"NUMEVENTS") == 0)
  {
    if((pOptions->NumEvents=atoi(szVal)) <= 0)
      return(-1);
    else
    {
      pOptions->pEvents=(int *)malloc(pOptions->NumEvents * sizeof(int));
      pOptions->LastEvent=0;
    }
  }
  else if(strncmp(szVar,"EVENT",EVENT_LEN) == 0)
  {
    if( pOptions->NumEvents <= 0 )
    {
      pOptions->NumEvents=1;
      pOptions->pEvents=(int *)malloc(pOptions->NumEvents * sizeof(int));
      pOptions->LastEvent=0;
    }
    pOptions->pEvents[pOptions->LastEvent++]=atoi(&(szVar[EVENT_LEN]));
  }
  else
  {
    logit("","Unrecognized Web Option : %s = %s\n",szVar,szVal);
    printf("Unrecognized Web Option : %s = %s\n<br>\n",szVar,szVal);
  }
  return(SUCCESS);
}  /* End of Webparse_Client_SetVars */

