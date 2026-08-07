
/*
 *   This file is under RCS - do not modify unless you have
 *   checked it out using the command checkout.
 *
 *    $Id: alarms_insert_prog.c,v 1.6 2002/05/28 19:15:59 lucky Exp $
 *
 *    Revision history:
 *    $Log: alarms_insert_prog.c,v $
 *    Revision 1.6  2002/05/28 19:15:59  lucky
 *    Added Footer and Header text tags
 *
 *    Revision 1.5  2002/02/20 20:03:31  lucky
 *    Added web apperance options (bkg color, header and footer logo)
 *
 *    Revision 1.4  2001/07/31 20:46:41  lucky
 *    Changed alarms nomenclature from User to Recipient
 *
 *    Revision 1.3  2001/07/28 00:49:59  lucky
 *    State of the code after debugging and testing prior to v6.0 release freeze.
 *
 *    Revision 1.2  2001/07/01 21:55:09  davidk
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
 *    Revision 1.1  2001/05/18 19:07:57  lucky
 *    Initial revision
 *
 *    Revision 1.1  2001/05/15 02:15:01  davidk
 *    Initial revision
 *
 *
 */


#include <alarms.h>
#include <ewdb_apps_utils.h>
#include <webparse.h>
#include <html_common.h>


static	int		DelFlag;


int main()
{

	int	 i, OrigId;
	AlarmsRecipientStructRuleDelivery	Options;
	char *configfile = "../params/alarms.d";


	/* Read the configuration file (path hardcoded relative to executable)
	 *********************************************************************/
	ReadConfig (configfile);

	logit_init ("alarms_insert_prog", 1, 1024, 1);
	logit ("", "alarms_insert_prog: starting.\n");

    /* Send header of reply back to web server */
    printf("Content-type: text/html\n\n");
    printf("<html>\n");

    printf ("<HEAD><TITLE>New Program Confirmation</TITLE></HEAD>\n");

	html_header (BackgroundColor, HeaderLogo, HeaderTag);

    printf("<center>\n");
    printf("<H2><FONT COLOR=blue>Earthworm Alarms: New Program Confirmation</FONT></H2>\n");
    printf("</center>\n");

    printf("<pre>\n");


	Options.NumRules = 0;
	Options.NumDeliveries = 0;
	Options.Recipient.idRecipient = NEW_ENTRY_FLAG;

	for (i = 0; i < EWDB_ALARMS_MAX_RULES_PER_RECIPIENT; i++)
	{
		Options.Rule[i].bCritInUse = FALSE;
		Options.Rule[i].Auto = 0;
	}

	DelFlag = FALSE;

	if (Webparse_GetAndProcessWebParams ((void *) (&Options)) == 1)
	{
		html_logit ("", "Call to GetAndProcessWebParams() failed\n");
		goto shutdown;
	}

	OrigId = Options.Rule[0].CritProg.idCritProgram;

    /* Connect to the DB */
    if (ewdb_api_Init (DBuser, DBpassword, DBservice) != 0)
    {
        html_logit( "", "Trouble connecting to database; exiting!\n" );
        goto shutdown;
    }


	if (DelFlag == TRUE)
	{
		if (ewdb_api_DeleteCritProgram (Options.Rule[0].CritProg.idCritProgram) 
															!= EWDB_RETURN_SUCCESS)
		{
			logit( "", "Call to ewdb_api_DeleteCritProgram failed.\n" );
			html_logit( "", "Could not delete program  with id=%d.\n",
									Options.Rule[0].CritProg.idCritProgram);
			printf("<CENTER>It is possible that this program is still in use.\n");
			printf("Check existing rules and modify or delete them first before \n");
			printf("deleting this criteria program.</CENTER><BR><HR><BR>\n");
			goto shutdown;
		}
	
		printf ("<CENTER><PRE>Criteria program with id=%d deleted!\n", 
								Options.Rule[0].CritProg.idCritProgram);

	}
	else
	{
		if (ewdb_api_CreateAlarmsCriteria (&Options.Rule[0].CritProg) 
															!= EWDB_RETURN_SUCCESS)
		{
			html_logit( "", "Call to ewdb_api_CreateAlarmsCriteria failed.\n" );
			goto shutdown;
		}
	
		if (Options.Rule[0].CritProg.idCritProgram < 0)
		{
			html_logit( "", "ewdb_api_CreateAlarmsCriteria returned bad idCritProg=%d.\n",
										Options.Rule[0].CritProg.idCritProgram);
			goto shutdown;
		}

		if (OrigId == NEW_ENTRY_FLAG)
		{
			printf ("<CENTER><PRE>New criteria program created!\n");
			printf ("Database ID=%d.\n", Options.Rule[0].CritProg.idCritProgram);
		}
		else if (OrigId == Options.Rule[0].CritProg.idCritProgram)
		{
			printf ("<CENTER><PRE>Criteria program with id=%d updated!\n", 
								Options.Rule[0].CritProg.idCritProgram);
		}
		else
		{
			html_logit ("", "idCritProgram doesn't match: Orig=%d, new=%d\n",
							OrigId, Options.Rule[0].CritProg.idCritProgram);
			goto shutdown;
		}
	}
	
	
shutdown:
	printf ("\n\n\n<CENTER><BR><BR><A HREF=\"alarms_manager\">");
	printf ("Click here to return to the Alarms Manager.</A></PRE></CENTER>\n"); 

	logit ("", "alarms_insert_prog: terminating.\n");
	ewdb_api_Shutdown();
	html_trailer (WebHost, FooterLogo, FooterTag);

	return EW_SUCCESS;

}  



/******************************************************
  Purpose:     Webparse_Client_SetVars() is a callout 
               function provided by 
               the web_common (webparse) routines that parse
               web parameters passed during a CGI-BIN GET or
               POST request.  Webparse_Client_SetVars() is 
               called for each parameter - value pair
 **********************************************************/
int Webparse_Client_SetVars(char * szVar, char * szVal, void * pUserParams)
{

/* User Callout function SetVars, passes the user a Variable, it's string value,
   and a pointer to User Defined Params, so that the user can take whatever
   desired action they want in the SetVars() function. */

	AlarmsRecipientStructRuleDelivery *pOptions = 
							(AlarmsRecipientStructRuleDelivery *) pUserParams;

    if (strcmp (szVar, "idprog") == 0)
	{
		pOptions->NumRules = 1;
		pOptions->Rule[0].CritProg.idCritProgram = atoi (szVal);
	}
    else if (strcmp (szVar, "pname") == 0)
	{
		strcpy (pOptions->Rule[0].CritProg.sProgName, szVal);
	}
    else if (strcmp (szVar, "pdir") == 0)
	{
		strcpy (pOptions->Rule[0].CritProg.sProgDir, szVal);
	}
    else if (strcmp (szVar, "pdesc") == 0)
	{
		strcpy (pOptions->Rule[0].CritProg.sProgDescription, szVal);
	}
    else if (strcmp (szVar, "delete") == 0)
	{
		DelFlag = TRUE;
	}
    else if (strcmp (szVar, "submit") == 0)
	{
		;		/* ignore */
	}
	else
	{
		logit ("", "Unrecognized Web Option : %s = %s\n",szVar,szVal);
	}

	return (0);
}


