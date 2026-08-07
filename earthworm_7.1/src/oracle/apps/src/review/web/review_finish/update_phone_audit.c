
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: update_phone_audit.c,v 1.1 2001/05/15 02:15:44 davidk Exp $
 *
 *    Revision history:
 *    $Log: update_phone_audit.c,v $
 *    Revision 1.1  2001/05/15 02:15:44  davidk
 *    Initial revision
 *
 *
 *
 */
  

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

#define				MAX_PHONE_AUDITS		5000

typedef struct _webstruct
{
	int		NumAudits;
	int		idAudit[MAX_PHONE_AUDITS];
	int		EventID;
} WebStruct;


main ()
{
	WebStruct		 			WebParams;
	char						*configfile = "../params/eqreview.d";
	int							i;
	ewdb_AlarmAudit				Audit;
	char						cmd[1024];

	/* Initialize some stuff
	 ***********************/
	DEBUG = 0;

	/* Send html header back to web server
	 *************************************/
	printf("Content-type: text/html\n\n");
	printf("<html>\n");
	printf("<HEAD><TITLE>Phone List Update Confirmation</TITLE></HEAD>\n" 
				"<BODY BGCOLOR=#EEEEEE TEXT=#333333>\n" );


	/* Read the configuration file (path hardcoded relative to executable)
	 *********************************************************************/
	ReadConfig (configfile);

	/* Cleanup, logfile should match config file name */
	logit_init ("update_phone_audit",1,10240,1);  


	/* initialize Web Parameters to all zeros */
	memset (&WebParams, 0, sizeof(WebOptionsStruct));

	if (Webparse_GetAndProcessWebParams((void *)(&WebParams)) != 0)
	{
		html_logit ("", "Call to Webparse_GetAndProcessWebParams failed.\n");
		html_break ();
		return EW_FAILURE;
	}

	/* Open connection to database
	 *****************************/
	if (ewdb_api_Init (DBuser, DBpassword, DBservice ) != 0 )
	{
		html_logit ("", "Trouble connecting to database; exiting!\n" );
		return EW_FAILURE;
	}


	printf ("<CENTER><PRE><HR><BR>\n");

	for  (i = 0; i < WebParams.NumAudits; i++)
	{
		Audit.idAudit = WebParams.idAudit[i];
		Audit.tAlarmDeclared = 0.0;
		Audit.tAlarmExecuted = (double) time (NULL);

		if (ewdb_api_CreateAlarmAudit (&Audit) != EWDB_RETURN_SUCCESS)
		{
			logit ("", "Call to ewdb_api_CreateAlarmAudit failed for idAudit = %d.\n",
										Audit.idAudit);
			return EW_FAILURE;
		}
	}

	printf ("UPDATED AUDITS FOR %d PHONE CALLS.\n", WebParams.NumAudits);

	printf ("\n\n<STRONG>REVIEW PROCESS COMPLETED FOR EVENT %d!</STRONG>\n\n\n",
																WebParams.EventID);
	printf ("Close this window and return to the Event list to review another event.\n");

	printf ("<BR><HR></PRE></CENTER>\n");

	/* DELETE THE REVIEW DIRECTORY */
	sprintf (cmd, "rm -rf %s/%d", ReviewTmpDir, WebParams.EventID);
	system (cmd);
	logit ("", "Review directory deleted: <%s/%d>!\n", ReviewTmpDir, 
												WebParams.EventID);

	html_trailer (WebHost);
	ewdb_api_Shutdown();

	return EW_SUCCESS;


}  /* end main */


/*********************************************************************/
int Webparse_Client_SetVars(char * szVar, char * szVal, void * pUserParams)
{

	int			index;
	WebStruct *pOptions = (WebStruct *) pUserParams;
	

/*
logit ("", "szVar=%s, szVal=%s\n", szVar, szVal);
*/

    if(strncmp(szVar,"use", 3) == 0)
    {
		pOptions->idAudit[pOptions->NumAudits] = atoi (szVar + 3);
		pOptions->NumAudits = pOptions->NumAudits + 1;
    }
	else if(strcmp(szVar,"EventID") == 0)
    {
		pOptions->EventID = atoi (szVal);
	}
	else if(strcmp(szVar,"submit") == 0)
    {
		/* ignore */
	}
    else
    {
      html_logit ("" ,"Unrecognized Web Option : %s = %s\n",szVar,szVal);
    }
    return(0);
}  /* End of SetVars() */
