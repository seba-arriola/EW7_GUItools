
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: review_finish.c,v 1.9 2002/05/28 19:34:48 lucky Exp $
 *
 *    Revision history:
 *    $Log: review_finish.c,v $
 *    Revision 1.9  2002/05/28 19:34:48  lucky
 *    Added header and footer tag text
 *
 *    Revision 1.8  2002/05/28 17:27:23  lucky
 *    *** empty log message ***
 *
 *    Revision 1.7  2002/03/22 20:03:33  lucky
 *    Pre v6.1 checkin
 *
 *    Revision 1.6  2001/08/10 21:04:51  lucky
 *    NT review and display partially implemented
 *
 *    Revision 1.5  2001/08/07 16:53:49  lucky
 *    Pre v6.0 checkin
 *
 *    Revision 1.4  2001/07/28 00:45:23  lucky
 *     State of the code after debugging and testing prior to v6.0 release freeze.
 *
 *    Revision 1.3  2001/07/01 21:55:32  davidk
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
 *    Revision 1.2  2001/06/26 17:12:17  lucky
 *    Removed debug stuff
 *
 *    Revision 1.1  2001/06/21 21:26:25  lucky
 *    Initial revision
 *
 *
 */
  

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

/* include earthworm headers */
#include <earthworm.h>
#include <time_ew.h>  /* DavidK: done for perf. timing */
#include <ewdb_ora_api.h> 
#include <webparse.h>
#include <alarms.h>
#include <time_functions.h>
#include <map_display_structs.h>
#include <review.h>

#define				MAX_PHONE_AUDITS		5000
#define				CONTINUE		100
#define				FINISH			200

typedef struct _webstruct
{
	int		NextAction;
	int		NumAudits;
	int		idAudit[MAX_PHONE_AUDITS];
	int		EventID;
	int		OriginID;
} WebStruct;


main ()
{
	WebStruct		 			WebParams;
	char						*configfile = "../params/review_event.d";
	int							i;
	EWDB_AlarmAuditStruct				Audit;
	char						cmd[1024];
	char						EvtDir[1024];

	/* Initialize some stuff
	 ***********************/
	DEBUG = 0;


	/* Read the configuration file (path hardcoded relative to executable)
	 *********************************************************************/
	ReadConfig (configfile);

	/* Cleanup, logfile should match config file name */
	logit_init ("review_finish",1,MAX_BYTES_PER_EQ,1);  


	/* initialize Web Parameters to all zeros */
	memset (&WebParams, 0, sizeof(WebStruct));

	if (Webparse_GetAndProcessWebParams((void *)(&WebParams)) != 0)
	{
		html_logit ("", "Call to Webparse_GetAndProcessWebParams failed.\n");
		html_break ();
		return EW_FAILURE;
	}

	sprintf (EvtDir, "%s%c%d", ReviewTmpDir, DIR_SLASH, WebParams.OriginID);



	/* Open connection to database
	 *****************************/
	if (ewdb_api_Init (DBuser, DBpassword, DBservice ) != 0 )
	{
		html_logit ("", "Trouble connecting to database; exiting!\n" );
		return EW_FAILURE;
	}

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

	ewdb_api_Shutdown ();

	if (WebParams.NextAction == CONTINUE)
	{
		ReviewFunction (WebParams.EventID, WebParams.OriginID, ACT_REFRESH, 0, 0);
		return EW_SUCCESS;
	}

	
	/* Send html header back to web server
	 *************************************/
	printf("Content-type: text/html\n\n");
	printf("<HTML>\n");
	printf("<HEAD><TITLE>Finish Review</TITLE></HEAD>\n");

	html_header (BackgroundColor, HeaderLogo, HeaderTag);

	printf ("<CENTER><PRE><BR>\n");

	if (WebParams.NumAudits > 0)
		printf ("UPDATED AUDITS FOR %d PHONE CALLS.\n", WebParams.NumAudits);


	printf ("\n\n<STRONG>REVIEW PROCESS COMPLETED FOR EVENT %d!</STRONG>\n\n\n",
																WebParams.EventID);
	printf ("Close this window and return to the Event list to review another event.\n");

	printf ("<BR></PRE></CENTER>\n");


	/* DELETE THE REVIEW DIRECTORY */
#ifdef _WINNT
	sprintf (cmd, "rmdir /Q /S %s", EvtDir);
#else
	sprintf (cmd, "rm -rf %s", EvtDir);
#endif _WINNT
	system (cmd);

	logit ("", "Review directory deleted: <%s>!\n", EvtDir);

	html_trailer (WebHost, FooterLogo, FooterTag);
	return EW_SUCCESS;

}  /* end main */


/*********************************************************************/
int Webparse_Client_SetVars(char * szVar, char * szVal, void * pUserParams)
{

	WebStruct *pOptions = (WebStruct *) pUserParams;
	

logit ("", "szVar=%s, szVal=%s\n", szVar, szVal);

    if(strncmp(szVar,"use", 3) == 0)
    {
		pOptions->idAudit[pOptions->NumAudits] = atoi (szVar + 3);
		pOptions->NumAudits = pOptions->NumAudits + 1;
    }
	else if(strcmp(szVar,"EventID") == 0)
    {
		pOptions->EventID = atoi (szVal);
	}
	else if(strcmp(szVar,"OriginID") == 0)
    {
		pOptions->OriginID = atoi (szVal);
	}
	else if(strcmp(szVar, "continue") == 0)
    {
		pOptions->NextAction = CONTINUE;
	}
	else if(strcmp(szVar, "finish") == 0)
    {
		pOptions->NextAction = FINISH;
	}
    else
    {
      html_logit ("" ,"Unrecognized Web Option : %s = %s\n",szVar,szVal);
    }
    return(0);
}  /* End of SetVars() */
