
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: process_alarms.c,v 1.1 2001/05/15 02:15:42 davidk Exp $
 *
 *    Revision history:
 *     $Log: process_alarms.c,v $
 *     Revision 1.1  2001/05/15 02:15:42  davidk
 *     Initial revision
 *
 *
 *
 *************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

/* I forget what we grab out of unistd, but we grab something
   and to support multiple platforms this ugly ifdef is needed */
#ifndef _WINNT  /* DavidK 990119 */
# include <unistd.h> 
#endif

/* include earthworm headers */
#include <earthworm.h>
#include <kom.h>
#include <time_ew.h>  /* DavidK: done for perf. timing */
#include <ewdb_ora_api.h> 
#include <webparse.h>
#include <alarms.h>
#include <time_functions.h>
#include <map_display_structs.h>

#include "../include/review_function.h"


#define     AUTHOR_MSG_LEN      512
#define     MAX_ALARMS      	5000
#define     INIT_NUM_ALARMS     100

typedef struct _webstruct
{
	int		idEvent;
	int		NumAlarms;
	int		isInsert;
	int		UseFlag[MAX_ALARMS];
} WebStruct;


static		int		NumOutAlarms;
static		int		NumPhoneAlarms;

static		char 	PA_DBuser[EQP_MAXWORD];
static		char 	PA_DBpassword[EQP_MAXWORD];
static		char 	PA_DBservice[EQP_MAXWORD];
static		char 	PA_ReviewDir[MAXPATH];
static		char 	PA_ReviewTmpDir[MAXPATH];
static		char 	PA_envEW_LOG[MAXPATH];
static		char 	PA_envEW_PARAMS[MAXPATH];
static		char 	PA_WebDir[MAXPATH];
static		char 	PA_WebTmpDir[MAXPATH];
static		char 	PA_WebHost[MAXPATH];
static		char 	PA_AlarmRing[MAXPATH];
static		char 	PA_MyModuleID[MAXPATH];
static		char 	PA_MyInstID[MAXPATH];

main ()
{
	WebStruct		 			WebParams;
	EWEventInfoStruct			EventInfo;
	char						*configfile = "../params/process_alarms.d"; 
	int							retsize, i, j;
	char						cmd[256];
	char						filename[256];
	char						tmpstr[1000];
	char						*AuthorMsg;
	FILE						*fp;
	ewdb_AlarmList				*pAlarms;
	ewdb_AlarmList				*pAlarmsOut;
	int							AlarmsBufSize, NumRetr, NumFound;
	

	/* Initialize some stuff
	 ***********************/
	DEBUG = 0;

	/* Send html header back to web server
	 *************************************/
	html_header();

	/* Read the configuration file (path hardcoded relative to executable)
	 *********************************************************************/
	ReadConfig (configfile);

	/* Cleanup, logfile should match config file name */
	logit_init ("process_alarms",1,MAX_FORMAT_LEN,1);  


	/* initialize Web Parameters to all zeros */
	memset (&WebParams, 0, sizeof(WebOptionsStruct));

	for (i = 0; i < MAX_ALARMS; i++)
		WebParams.UseFlag[i] = FALSE;

	NumOutAlarms = 0;

	if (Webparse_GetAndProcessWebParams((void *)(&WebParams)) != 0)
	{
		html_logit ("", "Call to Webparse_GetAndProcessWebParams failed.\n");
		html_break ();
		goto shutdown;
	}

	/* Open connection to database
	 *****************************/
	if (ewdb_api_Init (PA_DBuser, PA_DBpassword, PA_DBservice ) != 0 )
	{
		html_logit ("", "Trouble connecting to database; exiting!\n" );
		html_break();
		goto shutdown;
	}


	/* Read in the EVENT file */
	sprintf (filename, "%s/%d/EvtStruct.bin", PA_ReviewTmpDir, WebParams.idEvent);

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


	EventInfo.PrefOrigin.idEvent = WebParams.idEvent;
	EventInfo.PrefOrigin.BindToEvent = TRUE;
	EventInfo.PrefOrigin.SetPreferred = TRUE;

	EventInfo.PrefMag.idEvent = WebParams.idEvent;
	EventInfo.PrefMag.bBindToEvent = TRUE;
	EventInfo.PrefMag.bSetPreferred = TRUE;

	/* HACK HACK HACK alert */
	/*
	 * It appears that ewdb_apps_GetDBEventInfo does not fill in
	 * the PhaseAmp structures. But, they are used 
	 * in ewdb_apps_PutDBEventInfo to build the DurCoda type of 
	 * tables. So, we need to fill PhaseAmp information 
	 * from the info available elsewhere
	 */

	for (i = 0; i < EventInfo.iNumChans; i++)
	{
    for (j = 0; j < EventInfo.pChanInfo[i].iNumStaMags; j++)
		{
			if(EventInfo.pChanInfo[i].Stamags[j].iMagType == 0 /* DK CLEANUP */)
			{
				EventInfo.pChanInfo[i].Stamags[j].StaMagUnion.CodaDur.idPick =
					EventInfo.pChanInfo[i].Arrivals[j].idPick;
				EventInfo.pChanInfo[i].Stamags[j].StaMagUnion.CodaDur.tCodaTermObs =
					EventInfo.pChanInfo[i].Arrivals[j].tObsPhase +
					EventInfo.pChanInfo[i].Stamags[j].StaMagUnion.CodaDur.tCodaDurObs;
				EventInfo.pChanInfo[i].Stamags[j].StaMagUnion.CodaDur.tCodaTermXtp =
					EventInfo.pChanInfo[i].Arrivals[j].tObsPhase +
					EventInfo.pChanInfo[i].Stamags[j].StaMagUnion.CodaDur.tCodaDurXtp;
			}
			EventInfo.pChanInfo[i].Stamags[j].idChan =
				EventInfo.pChanInfo[i].Arrivals[j].idChan;
			EventInfo.pChanInfo[i].Stamags[j].idMagnitude =
				EventInfo.PrefMag.idMag;
		}
	}


	/* Get the original Author Logo from file */
	if ((AuthorMsg = malloc (AUTHOR_MSG_LEN * sizeof (char))) == NULL)
	{
		html_logit ("", "Can't malloc AuthorMsg.\n");
		html_break ();
		goto shutdown;	
	}

	/* build the file name */
	sprintf (filename, "%s/%d/Author", PA_ReviewTmpDir, WebParams.idEvent);

	if ((fp = fopen (filename, "rt")) == NULL)
	{
		html_logit ("", "Can't open %s\n", filename);
		html_break ();
		goto shutdown;	
	}


	retsize = fread ((void *) AuthorMsg, sizeof (char), AUTHOR_MSG_LEN, fp);

	fclose (fp);
	
	AuthorMsg[retsize] = '\0';


	/*  Build a new author string */

	/* Is this a previously reviewed event? */
	if (strncmp (AuthorMsg, "REV_", 4) != 0)
		sprintf (tmpstr, "REV_%s", AuthorMsg);
	else
		strcpy (tmpstr, AuthorMsg);

	free (AuthorMsg);


	if (WebParams.isInsert == TRUE)
	{
		if (ewdb_apps_PutDBEventInfo (&EventInfo, tmpstr, NULL, 0) != EWDB_RETURN_SUCCESS)
		{
			html_logit ("", "Call to ewdb_apps_PutDBEventInfo failed.\n");
			return EW_FAILURE;
		}
	}
	else
	{
		if (ewdb_api_DeleteEvent (EventInfo.Event.idEvent, 0) != EWDB_RETURN_SUCCESS)
		{
			html_logit ("", "Call to ewdb_api_DeleteEvent failed.\n");
			return EW_FAILURE;
		}
	}

	/** EXECUTE ALARMS **/

	/* Retrieve the list of alarms from the DB */

	if ((pAlarms = (ewdb_AlarmList *) malloc (INIT_NUM_ALARMS * 
					sizeof (ewdb_AlarmList))) == NULL)
	{
		html_logit ("", "Could not malloc alarms buffer \n");
		return EW_FAILURE;
	}

	if (DetermineAlarms (&EventInfo, 0, WebParams.isInsert, pAlarms, 
						INIT_NUM_ALARMS, &NumFound, &NumRetr) != EW_SUCCESS)
   	{
		html_logit ("", "Call to DetermineAlarms failed.\n");
		return EW_FAILURE;
	}

	if (NumRetr < NumFound)
	{
		logit ("", "Alarm list too small - got %d alarms, allocating.\n", NumRetr);

		free (pAlarms);

		if ((pAlarms = (ewdb_AlarmList *) malloc (NumFound * 
								sizeof (ewdb_AlarmList))) == NULL)
		{
			html_logit ("", "Could not malloc alarms buffer \n");
			return EW_FAILURE;
		}

		if (DetermineAlarms (&EventInfo, 0, WebParams.isInsert, pAlarms, 
								NumFound, &NumFound, &NumRetr) != EW_SUCCESS)
   		{
			html_logit ("", "Call to DetermineAlarms failed.\n");
			return EW_FAILURE;
		}
	}


	/* Reconcile with the options retrieved by the web server */
	if ((pAlarmsOut = (ewdb_AlarmList *) malloc (NumOutAlarms * 
								sizeof (ewdb_AlarmList))) == NULL)
	{
		html_logit ("", "Could not malloc alarms buffer \n");
		return EW_FAILURE;
	}

	NumOutAlarms = 0;
	NumPhoneAlarms = 0;
	for (i = 0; i < NumRetr; i++)
	{
		if (WebParams.UseFlag[i] == TRUE)
		{
			memcpy (&pAlarmsOut[NumOutAlarms], &pAlarms[i], 
										sizeof (ewdb_AlarmList));

			/* Read in the potentially modified text messages */
			sprintf (filename, "%s/%d/AlarmsMsg_%d.bin",
						PA_ReviewTmpDir, EventInfo.Event.idEvent, i);

			if ((fp = fopen (filename, "rb")) == NULL)
			{
				html_logit ("", "Can't open %s.\n", filename);
				return EW_FAILURE;
			}

			fread (pAlarmsOut[NumOutAlarms].AlarmMsg, 
									sizeof (char), ALARM_MSG_SIZE, fp);
			fclose (fp);

			if (pAlarmsOut[NumOutAlarms].Delivery.DelMethodInd == IND_PHONE)
				NumPhoneAlarms = NumPhoneAlarms + 1;

			NumOutAlarms = NumOutAlarms + 1;

		}
	}


	if (NumOutAlarms != 0)
	{
		/* Execute the alarms */

		if (ExecuteAlarms (pAlarmsOut, NumOutAlarms, PA_AlarmRing, 
							PA_MyInstID, PA_MyModuleID) != EW_SUCCESS)
		{
			html_logit ("", "Call to ExecuteAlarms failed.\n");
			return EW_FAILURE;
		}
	}

	printf ("<CENTER><PRE>\n");
	printf ("<hr>\n");
	printf ("<br><br>\n");

	if (WebParams.isInsert == TRUE)
		printf ("REVIEWED EVENT %u SUCCESSFULLY INSERTED INTO THE DATABASE.\n\n",
										EventInfo.Event.idEvent);
	else
		printf ("REVIEWED EVENT %u SUCCESSFULLY DELETED FROM THE DATABASE.\n\n",
										EventInfo.Event.idEvent);

	printf ("<P>%d ALARMS ISSUED.\n", NumOutAlarms);
			

	printf ("<BR><HR><BR><BR>\n");


	/* Now, put up the list of phone numbers to call */
	if (NumPhoneAlarms > 0)
	{
		printf ("<STRONG>Alarms Phone List</STRONG><BR>\n");
		printf ("The following users should be telephoned in the order listed.\n"
				"Clicking on the Message link will open a new window containing \n"
				"the message to be relayed to the user.\n\n"
				"Check the Done? checkbox after the user has been contacted.\n\n"
				"Click FINISH to send the updated information to the database\n"
				"and complete the review process for this event.\n");

		printf ("</PRE>\n");

		printf ("<FORM NAME=\"PhoneListForm\" ACTION=\"update_phone_audit\" METHOD=POST>\n");
		printf ("<TABLE BORDER=1 CELLPADDING=2>\n");

		printf ("<TR VALIGN=center>\n");
		printf ("<TD COLSPAN=2 ALIGN=center>Done?</TD>\n");
		printf ("<TD COLSPAN=4 ALIGN=center>User</TD>\n");
		printf ("<TD COLSPAN=3 ALIGN=center>Number</TD>\n");
		printf ("<TD COLSPAN=3 ALIGN=center>Message</TD>\n");
		printf ("</TR>\n");

		for (i = 0; i < NumOutAlarms; i++)
		{
			if (pAlarmsOut[i].Delivery.DelMethodInd == IND_PHONE)
			{
				printf ("<TR VALIGN=center>\n");

				/* use checkbox */
				printf ("<TD ALIGN=center COLSPAN=2>");
				printf ("<INPUT TYPE=checkbox NAME=use%d></TD>\n", 
											pAlarmsOut[i].Audit.idAudit);

				/* user description */
				if (pAlarmsOut[i].Audit.User.sDescription != NULL)
					printf ("<TD ALIGN=center COLSPAN=4>%s</TD>", 
								pAlarmsOut[i].Audit.User.sDescription);
				else
					printf ("<TD ALIGN=center COLSPAN=5>Unknown User</TD>");

				/* phone number */
				printf ("<TD ALIGN=center COLSPAN=3>%s</TD>", 
							pAlarmsOut[i].Delivery.phone.sPhoneNumber);


				/* Link to the text of the message */
				printf ("<TD ALIGN=center COLSPAN=3>"
						"<A HREF=\"review_alarm_msg?EventID=%u?AlarmNo=%u\" "
						" TARGET=\"Review Message\">%s</A></TD>\n",
									EventInfo.Event.idEvent, i,
									pAlarmsOut[i].Audit.Format.sDescription);

				printf ("</TR>\n");
			}
		}	

		/* pass on the event ID so that the review directory can be deleted */
		printf ("<INPUT TYPE=hidden NAME=\"EventID\" VALUE=\"%d\">\n",
													WebParams.idEvent);

		printf ("<TR VALIGN=center>\n");
		printf ("<TD HEIGHT=100 ALIGN=center COLSPAN=12>");
		printf ("<INPUT TYPE=\"submit\" VALUE=\"FINISH\" NAME=\"submit\"></TD></TR>\n");

		printf ("</TABLE></FORM>\n");
	}


shutdown:
	ewdb_api_Shutdown();

	/* 
	 * Delete the directory only if there are no phone alarms;
	 * Otherwise, the directory will be deleted by update_phone_audit.
	 */
	if (NumPhoneAlarms <= 0)
	{
		/* DELETE THE REVIEW DIRECTORY */
		sprintf (cmd, "rm -rf %s/%d\n", PA_ReviewTmpDir, WebParams.idEvent);
		system (cmd);
	
		logit ("", "Review directory deleted!\n");
	}

	printf ("<BR><HR><BR>\n");
	html_trailer(WebHost);
	logit("","process: terminating\n" );

	ewdb_api_Shutdown();
	return (0);

}  /* end main */



/******************************************************
     Dumps HTML header, title, and beginning page
 **********************************************************/
void	html_header( void )
{

   printf("Content-type: text/html\n\n");
   printf("<html>\n");
   printf("<HEAD><TITLE>Event Review Confirmation</TITLE></HEAD>\n" 
          "<BODY BGCOLOR=#EEEEEE TEXT=#333333>\n" );
   printf("<center>\n");
   printf("</center>\n");

}  /* End html_header() */



/*****************************************************************************
 *  ReadConfig() processes command file(s) using kom.c functions;            *
 *                  Exits if any errors are encountered 	             *
 *****************************************************************************/
#define ncommand  11          /* # of required commands you expect to process */

int ReadConfig( char *configfile )
{
   char     init[ncommand]; /* init flags, one byte for each required command */
   int      nmiss;          /* number of required commands that were missed   */
   char    *com;
   int      nfiles;
   int      success;
   int      i;	
   char*    str;

/* Set to zero one init flag for each required command 
 *****************************************************/   
   for( i=0; i<ncommand; i++ )  init[i] = 0;


	NumRevSrcs = 0;
	iNumSPDs = 0;
	DisplayChannels = DISPLAY_CHANNELS;

/* Open the main configuration file 
 **********************************/
   nfiles = k_open( configfile ); 
   if ( nfiles == 0 ) {
	printf("ReadConfig: Error opening command file %s; exiting!\n<br>\n", 
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
                  printf("ReadConfig: Error opening command file %s; exiting!\n<br>\n",
                           &com[1] );
                  exit( -1 );
               }
               continue;
            }

        /* Process anything else as a command 
         ************************************/
  /*0*/     if( k_its("Logfiledir") ) {
                str = k_str();
                if( strlen(str) < MAXPATH ) 
                {
                    sprintf( PA_envEW_LOG, "EW_LOG=%s", str );  
                    if( putenv( PA_envEW_LOG ) != 0 )  /*set environment variable for logit*/
                    {
                       printf("putenv: unable to set "
                              "EW_LOG environment variable; exiting!\n<br>\n" );
                       exit( -1 );
                    }
                } else {
                    printf("Bad Logfiledir command in %s:\n"
                             "pathname \"%s\" too long; maxchar=%d; exiting!\n<br>\n",
                             configfile, str, MAXPATH );
                    exit( -1 );
                }
                init[0] = 1;
            }

  /*1*/     else if( k_its("DBservice") ) {
                str = k_str();
                if( strlen(str) < EQP_MAXWORD ) {
                    strcpy( PA_DBservice, str );
                } else {
                    printf("Bad DBservice command in %s:\n"
                           "string \"%s\" too long; maxchar=%d; exiting!\n<br>\n",
                             configfile, str, EQP_MAXWORD );
                    exit( -1 );
                }
                init[1] = 1;
            }

  /*2*/     else if( k_its("DBuser") ) {
                str = k_str();
                if( strlen(str) < EQP_MAXWORD ) {
                    strcpy( PA_DBuser, str );
                } else {
                    printf("Bad DBuser command in %s:\n"
                           "username \"%s\" too long; maxchar=%d; exiting!\n<br>\n",
                             configfile, str, EQP_MAXWORD );
                    exit( -1 );
                }
                init[2] = 1;
            }

  /*3*/     else if( k_its("DBpassword") ) {
                str = k_str();
                if( strlen(str) < EQP_MAXWORD ) {
                    strcpy( PA_DBpassword, str );
                } else {
                    printf("Bad DBpassword command in %s:\n"
                           "passwd \"%s\" too long; maxchar=%d; exiting!\n<br>\n",
                             configfile, str, EQP_MAXWORD );
                    exit( -1 );
                }
                init[3] = 1;
            }
  /*4*/     else if( k_its("ReviewDir") ) {
                str = k_str();
                if( strlen(str) < MAXPATH ) {
                    strcpy( PA_ReviewDir, str );
                    sprintf( PA_ReviewTmpDir, "%s/tmp", PA_ReviewDir );
                } else {
                    printf("Bad ReviewDir command in %s:\n"
                           "name \"%s\" too long; maxchar=%d; exiting!\n<br>\n",
                             configfile, str, MAXPATH );
                    exit( -1 );
                }
                init[4] = 1;
            }
  /*5*/     else if( k_its("WebDir") ) {
                str = k_str();
                if( strlen(str) < MAXPATH ) {
                    strcpy( PA_WebDir, str );
                    sprintf( PA_WebTmpDir,"%s/tmp", PA_WebDir );
                } else {
                    printf("Bad WebDir command in %s:\n"
                           "name \"%s\" too long; maxchar=%d; exiting!\n<br>\n",
                             configfile, str, MAXPATH );
                    exit( -1 );
                }
                init[5] = 1;
            }
  /*6*/     else if( k_its("WebHost") ) {
                str = k_str();
                if( strlen(str) < MAXPATH ) {
                    strcpy( PA_WebHost, str );
                } else {
                    printf("Bad WebHost command in %s:\n"
                           "name \"%s\" too long; maxchar=%d; exiting!\n<br>\n",
                             configfile, str, MAXPATH );
                    exit( -1 );
                }
                init[6] = 1;
            }
  /*7*/     else if( k_its("AlarmRing") ) {
                str = k_str();
                if( strlen(str) < MAXPATH ) {
                    strcpy( PA_AlarmRing, str );
                } else {
                    printf("Bad AlarmRing command in %s:\n"
                           "name \"%s\" too long; maxchar=%d; exiting!\n<br>\n",
                             configfile, str, MAXPATH );
                    exit( -1 );
                }
                init[7] = 1;
            }
  /*8*/     else if( k_its("MyModuleID") ) {
                str = k_str();
                if( strlen(str) < MAXPATH ) {
                    strcpy( PA_MyModuleID, str );
                } else {
                    printf("Bad MyModuleID command in %s:\n"
                           "name \"%s\" too long; maxchar=%d; exiting!\n<br>\n",
                             configfile, str, MAXPATH );
                    exit( -1 );
                }
                init[8] = 1;
            }
  /*9*/     else if( k_its("MyInstID") ) {
                str = k_str();
                if( strlen(str) < MAXPATH ) {
                    strcpy( PA_MyInstID, str );
                } else {
                    printf("Bad MyInstID command in %s:\n"
                           "name \"%s\" too long; maxchar=%d; exiting!\n<br>\n",
                             configfile, str, MAXPATH );
                    exit( -1 );
                }
                init[9] = 1;
            }

  /*10*/    else if( k_its("EwParams") ) {
                str = k_str();
                if( strlen(str) < MAXPATH )
                {
                    sprintf( PA_envEW_PARAMS, "EW_PARAMS=%s", str );
                    if( putenv( PA_envEW_PARAMS ) != 0 )  
                    {
                       printf("putenv: unable to set "
                              "EW_PARAMS environment variable; exiting!\n<br>\n" );
                       exit( -1 );
                    }
                } else {
                    printf("Bad EwParams command in %s:\n"
                             "pathname \"%s\" too long; maxchar=%d; exiting!\n<br>\n",
                             configfile, str, MAXPATH );
                    exit( -1 );
                }
                init[10] = 1;
            }
            else if( k_its("Debug") ) {
                DEBUG=1;
            }

         /* Unknown command
          *****************/ 
	    else {
                printf( "ReadConfig: \"%s\" Unknown command in %s.\n<br>\n", 
                         com, configfile );
                continue;
            }

        /* See if there were any errors processing the command 
         *****************************************************/
            if( k_err() ) {
               printf("ReadConfig: Bad %s command in %s; exiting!\n<br>\n",
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
       printf( "ReadConfig: ERROR, no " );
       if ( !init[0] )  printf( "Logfiledir "     );
       if ( !init[1] )  printf( "DBservice "      );
       if ( !init[2] )  printf( "DBuser "         );
       if ( !init[3] )  printf( "DBpassword "     );
       if ( !init[4] )  printf( "ReviewDir "    );
       if ( !init[5] )  printf( "WebDir "    );
       if ( !init[6] )  printf( "WebHost "    );
       if ( !init[7] )  printf( "AlarmRing "    );
       if ( !init[8] )  printf( "MyModuleID "    );
       if ( !init[9] )  printf( "MyInstID "    );
       if ( !init[10] )  printf( "EwParams "     );
       printf( "command(s) in %s; exiting!\n<br>\n", configfile );
       exit( -1 );
   }

   return( 0 );
}



/*********************************************************************/
int Webparse_Client_SetVars(char * szVar, char * szVal, void * pUserParams)
{

	int			index;
	WebStruct *pOptions = (WebStruct *) pUserParams;
	

/*
	logit ("", "szVar=%s, szVal=%s\n", szVar, szVal);
*/

    if(strcmp(szVar,"EventID") == 0)
    {
		pOptions->idEvent = atoi (szVal);
    }
	else
    if(strncmp(szVar,"Use", 3) == 0)
    {
		/* extract index */
		index = atoi (szVar + 4);
		pOptions->UseFlag[index] = TRUE;
		NumOutAlarms = NumOutAlarms + 1;
	}
	else
    if(strcmp(szVar,"NumAlarms") == 0)
    {
		pOptions->NumAlarms = atoi (szVal);
	}
	else
    if(strcmp(szVar,"isInsert") == 0)
    {
		pOptions->isInsert = atoi (szVal);
	}
	else
    if(strcmp(szVar,"Finish") == 0)
    {
		/* ignore */
	}
    else
    {
      html_logit ("" ,"Unrecognized Web Option : %s = %s\n",szVar,szVal);
    }
    return(0);
}  /* End of SetVars() */
