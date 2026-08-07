
/*
 *   This file is under RCS - do not modify unless you have
 *   checked it out using the command checkout.
 *
 *    $Id: coinclist.c,v 1.4 2004/07/01 20:50:02 labcvs Exp $
 *    Revision history:
 *
 *    $Log: coinclist.c,v $
 *    Revision 1.4  2004/07/01 20:50:02  labcvs
 *    Removed Old logging statement
 *
 *    Revision 1.3  2002/05/28 19:34:19  lucky
 *    Added header and footer tag text
 *
 *    Revision 1.2  2002/05/28 17:26:59  lucky
 *    *** empty log message ***
 *
 *    Revision 1.1  2002/03/22 20:13:23  lucky
 *    Initial revision
 *
 *
 *
 */
  
/*****************************************************************

*****************************************************************/

/*** #includes ***/

/* include the normal stuff */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <earthworm.h>
#include <time_ew.h>
#include <ew_event_info.h>
#include <ewdb_ora_api.h>
#include <ewdb_apps_utils.h>
#include <webparse.h>
#include <html_common.h>

#define		INIT_NUM_COINC		50
#ifdef _WINNT
# define EXE_EXT ".exe"
#else
# define EXE_EXT
#endif


/* This is where we are going to look for the config file */
# ifdef _WINNT
#define GL_CONFIG_DIR "..\\params\\"
#else
#define GL_CONFIG_DIR "../params/"
#endif _WINNT

/*** GLOBAL VARIABLES ***/

/* Debug definition */
int DEBUG=0; /* Change to 1 to enable debugging messages */

/* Generic DB Configuration Parameters */
char DBuser[50]="main";
char DBpassword[50]="main";
char DBservice[50]="eqsm.usgs";
char WebHost[512];
char envEW_LOG[512];
int  MyModID=1, LogSwitch=1;
char  BackgroundColor[512];
char  HeaderLogo[512];
char  FooterLogo[512];
char  HeaderTag[512];
char  FooterTag[512];


/* Web params structure */
typedef struct webopts
{
	float 	StartTime;
	float 	EndTime;
} WebOptionsStruct;

int main(int argc, char * argv[])
{

	WebOptionsStruct				WebParams;
	EWEventInfoStruct   			EventInfo;
	EWDB_CoincEventStruct			*pCoinc;
	int 							i, j, indx, DBFlags;
	char 							ConfigFile[128];
	char 							TempFileNameStr[128];
	char 							* pTemp1, *pTemp2;
	int								NumFound, NumRetr, NumAlloc, NumTrigs, NumWaves;
	char  							datestr[DATESTR23];
	time_t 							TempTime;






  /* Read parameters from the config file. */
# ifdef _WINNT
  strncpy(TempFileNameStr, argv[0], sizeof(TempFileNameStr));

  pTemp1 = strrchr (TempFileNameStr, '\\');
  
  if (pTemp1 != NULL)
	  pTemp1 = pTemp1 + 1;
  else
	  pTemp1 = TempFileNameStr;

  pTemp2 = strchr (TempFileNameStr, '.');
  if (pTemp2 != NULL)
	  *pTemp2 = '\0';
  
  sprintf(ConfigFile,"%s%s.d",GL_CONFIG_DIR, pTemp1);

# else
  sprintf(ConfigFile,"%s%s.d",GL_CONFIG_DIR,argv[0]);
# endif _WINNT

  /* Go get them config file params */
  if(ReadConfig(ConfigFile))
  {
    fprintf(stdout,"Read of config file failed. Filename: %s\n",ConfigFile);
    return(-1);
  }


  /* Initialize logging */
#ifdef _WINNT
  logit_init (pTemp1, (short)MyModID, 1024, LogSwitch);
#else
  logit_init (argv[0], (short)MyModID, 1024, LogSwitch);
#endif _WINNT


  /* Send header of reply back to web server */
   printf("Content-type: text/html\n\n");
   printf("<html>\n");
   printf("<HEAD><TITLE>Coincidence Events</TITLE></HEAD>\n");

	if (strcmp (BackgroundColor, "notset") == 0)
		printf("<BODY bgcolor=#FFFFFF>\n");
	else
        printf("<BODY BGCOLOR=%s>\n", BackgroundColor);

  /* Display the header gif, if specified */
  printf ("<BR>\n");

  if (strcmp (HeaderLogo, "notset") != 0)
      printf ("<CENTER><IMG SRC=\"%s\" align=CENTER></CENTER><BR>\n", HeaderLogo);


   printf("<center>\n");
   printf("<H1><FONT COLOR=blue>Earthworm DBMS<br></FONT></H1>\n" );
   printf("<H2><FONT COLOR=red>Coincidence Events Retrieved From DBMS</FONT></H2>\n");
   printf("</center><HR><BR>\n");
 

  /* Get the web params from the web client */
    /* initialize Web Parameters to all zeros */
    memset (&WebParams, 0, sizeof(WebOptionsStruct));
    if (Webparse_GetAndProcessWebParams((void *)(&WebParams)) != 0)
    {
        logit ("", "Call to Webparse_GetAndProcessWebParams failed.\n");
        return (-1);
    }

	/* Prepare the DB, cause we are a comin'! */
	if (ewdb_api_Init (DBuser, DBpassword, DBservice))
	{
		html_logit ("","DB Initialization failed! Exiting.\n");
		return(-1);
	}


	NumAlloc = INIT_NUM_COINC;
	if ((pCoinc = (EWDB_CoincEventStruct *) malloc (NumAlloc * 
							sizeof (EWDB_CoincEventStruct))) == NULL)
	{
		html_logit ("", "Could not malloc Coincidence buffer of length %d.\n",  
												NumAlloc);
		return -1;
	}

	/* Get the list of events in thsi time frame */
	if (ewdb_api_GetCoincidence (-1, EWDB_EVENT_TYPE_COINCIDENCE, pCoinc, NumAlloc,  
			WebParams.StartTime, WebParams.EndTime,
			&NumFound, &NumRetr) == EWDB_RETURN_FAILURE)
	{
		html_logit ("", "Call to ewdb_api_GetCoincidence failed.\n");
		return (-1);
	}

	if (NumRetr < NumFound)
	{
		/* Reallocate and try again */
		free (pCoinc);

		NumAlloc = NumFound;
		if ((pCoinc = (EWDB_CoincEventStruct *) malloc (NumAlloc * 
								sizeof (EWDB_CoincEventStruct))) == NULL)
		{
			html_logit ("", "Could not malloc Coincidence buffer of length %d.\n",  
											NumAlloc);
			return -1;
		}

		/* Get the gory details about the event */
		if (ewdb_api_GetCoincidence (-1, EWDB_EVENT_TYPE_COINCIDENCE, pCoinc, NumAlloc, 
				WebParams.StartTime, WebParams.EndTime,
				&NumFound, &NumRetr) == EWDB_RETURN_FAILURE)
		{
			html_logit ("", "Call to ewdb_api_GetCoincidence failed.\n");
			return (-1);
		}
	}

	printf ("<PRE>\n");

	DBFlags = GETEWEVENTINFO_PICKS | GETEWEVENTINFO_COMPINFO | 
						GETEWEVENTINFO_WAVEFORM_DESCS | GETEWEVENTINFO_WAVEFORMS;

	datestr23 (WebParams.StartTime, datestr, DATESTR23);
	printf ("Start Time:     %s\n",  datestr);

	datestr23 (WebParams.EndTime, datestr, DATESTR23);
	printf ("  End Time:     %s\n\n",  datestr);

	printf ("Retrieved %d Coincidence Events.\n\n", NumRetr);
	printf ("<HR>\n");

	if (NumRetr <= 0)
	{
		ewdb_api_Shutdown();

		/* We are done! */
		return(0);
	}

    /* Start ora2sac postable form. */
    printf("</PRE>\n");
    printf("<FORM ACTION=\"ora2sactarfile\"" EXE_EXT
           "METHOD=POST TARGET=\"Download Seismograms\">\n");
    printf("<INPUT NAME=NUMEVENTS TYPE=HIDDEN VALUE=\"%d\">\n", NumRetr);

    printf("<CENTER><PRE>\n\n");

    printf("To retrieve seismograms for one or more events, select the events'\n"
            "checkbox, then press 'Retrieve Seismograms'.  NOTE: This operation\n"
            "may take some time for a large number of channels to be saved.\n\n");
    printf("To enter quick review for the event, click on the EventID.\n\n");
    printf("To see a snapshot of the waveforms, click on the link in the 'Traces' column.\n\n");

    printf("</PRE>\n");

    /* Write the submit button at the top of the table */
    printf("<INPUT TYPE=\"SUBMIT\" VALUE=\"Retrieve Seismograms\">\n<br>\n");
    printf("<br>\n");

    /*  Write the table header */
    printf("<table border=1> <tr bgcolor=#2222AA VALIGN=TOP align=center>\n");
    printf("<td colspan=2 valign=center>"
				"<strong><font size=\"+1\" color=#FFFFFF>EventID</td>\n");
    printf("<td valign=center>"
				"<strong><font size=\"+1\" color=#FFFFFF>Coincidence Time</td>\n");
    printf("<td valign=center>"
				"<strong><font size=\"+1\" color=#FFFFFF>Source</td>\n");
    printf("<td valign=center>"
				"<strong><font size=\"+1\" color=#FFFFFF>Triggers</td>\n");
    printf("<td valign=center>"
				"<strong><font size=\"+1\" color=#FFFFFF>Traces</td>\n");

    printf ("</TR>\n");

	for (i = 0; i < NumRetr; i++)
	{

		/* Get the gory details */
		if (ewdb_apps_GetDBEventInfoII (&EventInfo, pCoinc[i].idEvent, 
					-1, DBFlags) != EWDB_RETURN_SUCCESS)
		{
			html_logit ("", "Call to ewdb_apps_GetDBEventInfo failed for %d\n", 
															pCoinc[i].idEvent);
			html_break ();
			continue;
		}

		NumTrigs = 0;
		NumWaves = 0;
		for (j = 0; j < EventInfo.iNumChans; j++)
		{
			if (EventInfo.pChanInfo[j].iNumTriggers > 0)
				NumTrigs = NumTrigs + 1;
			if (EventInfo.pChanInfo[j].iNumWaveforms > 0)
				NumWaves = NumWaves + 1;
		}


      /* write the checkbox for the event */
      printf("<tr align=center>\n");
      printf("<td> <INPUT NAME=\"EVENT%u\" TYPE=CHECKBOX> </td>\n",
								             EventInfo.Event.idEvent);

	  /* Review link */
      printf("<td> <A HREF=\"eqparam2html"EXE_EXT"?EVENT%u=On\" "
			                "TARGET=\"Event Info\">%u</A> </td>\n",
					             EventInfo.Event.idEvent, EventInfo.Event.idEvent);

      /* Write the coincidence time in the form YYYY/MM/DD  HH:MM:SS */
      TempTime = (time_t) (EventInfo.CoincEvt.tCoincidence);
      printf ("<td>%s</td>\n", EWDB_ttoa (&TempTime, datestr));

      /* event source */
      printf("<td> %s </td>\n", EventInfo.CoincEvt.szHumanReadable);


      /* Number of channel triggers */
      printf ("<TD bgcolor=#FFFF00><A HREF=\"review_event?EventID=%u?OriginID=%u?Action=%d\" "
				"target=\"Event Review\">%d</A></TD>\n", 
						EventInfo.Event.idEvent, EventInfo.CoincEvt.idCoincidence, 
						ACT_GETFROMDB, NumTrigs);

      /* Number of waveforms */
		if (NumWaves > 0)
              printf ("<TD bgcolor=#00FF00> <A HREF=\"ora2snippet_gif?EventID=%u\""
                  "target=\"All Snippets\">%d</A></TD>\n",
                  EventInfo.Event.idEvent, NumWaves);
		else
	      printf ("<td>0</td>\n");

      /* Write the close of the row */
      printf("</tr>\n");

		if (EventInfo.pChanInfo != NULL);
			free (EventInfo.pChanInfo);
		EventInfo.iNumAllocChans = 0;
		EventInfo.iNumChans = 0;

	} /* for loop over retrieved coincidence events */

	printf ("</TABLE><BR><BR>\n");

    /* Write the submit button at the bottom of the table */
    printf("<INPUT TYPE=\"SUBMIT\" VALUE=\"Retrieve Seismograms\">\n<br>\n");

    /* Close the form and the table*/
    printf("</form>\n");

	html_trailer (WebHost, FooterLogo, FooterTag);


	ewdb_api_Shutdown();

	/* We are done! */
	return(0);

}  /* End of main() */



int Webparse_Client_SetVars(char * szVar, char * szVal, void * pUserParams)
/******************************************************
  Function:    Webparse_Client_SetVars()
  Purpose:     Webparse_Client_SetVars() is a callout 
               function provided by 
               the web_common (webparse) routines that parse
               web parameters passed during a CGI-BIN GET or
               POST request.  Webparse_Client_SetVars() is 
               called for each parameter - value pair
  **********************************************************/
{
  WebOptionsStruct * pOptions= (WebOptionsStruct *) pUserParams;


    if (strcmp (szVar, "StartTime") == 0)
    {
      pOptions->StartTime = atof (szVal);
    }
    else if (strcmp (szVar, "EndTime") == 0)
    {
      pOptions->EndTime = atof (szVal);
    }
	else
    {
      html_logit ("" ,"Unrecognized Web Option : %s = %s\n", szVar, szVal);
    }
    return(0);

}  /* End of SetVars() */
