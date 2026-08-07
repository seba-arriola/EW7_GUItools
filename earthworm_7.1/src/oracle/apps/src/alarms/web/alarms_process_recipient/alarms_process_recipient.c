
/*
 *   This file is under RCS - do not modify unless you have
 *   checked it out using the command checkout.
 *
 *    $Id: alarms_process_recipient.c,v 1.3 2002/05/28 19:15:59 lucky Exp $
 *
 *    Revision history:
 *    $Log: alarms_process_recipient.c,v $
 *    Revision 1.3  2002/05/28 19:15:59  lucky
 *    Added Footer and Header text tags
 *
 *    Revision 1.2  2002/02/20 20:03:31  lucky
 *    Added web apperance options (bkg color, header and footer logo)
 *
 *    Revision 1.1  2001/07/31 20:46:41  lucky
 *    Initial revision
 *
 *    Revision 1.4  2001/07/28 00:49:59  lucky
 *    State of the code after debugging and testing prior to v6.0 release freeze.
 *
 *    Revision 1.3  2001/07/01 21:55:11  davidk
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
 *    Revision 1.2  2001/06/21 21:30:25  lucky
 *    State of the code after the LocalMag review portion was completed.
 *
 *    Revision 1.1  2001/05/18 19:07:53  lucky
 *    Initial revision
 *
 *    Revision 1.1  2001/05/15 02:15:04  davidk
 *    Initial revision
 *
 *
 */


#include <alarms.h>
#include <ewdb_apps_utils.h>
#include <webparse.h>
#include <html_common.h>

#define			INIT_BUF_SIZE		10 
#define			NUM_INIT_RULES		5 



typedef struct _WebOptionsStructReview
{
	int 				idRecipient;
} WebOptionsStructAlarms;


/* function prototypes */
static int html_existing_recipient(int idRecipient);
static int html_new_recipient();

int main()
{

	WebOptionsStructAlarms	Options;
	char				*configfile = "../params/alarms.d";


	/* Read the configuration file (path hardcoded relative to executable)
	 *********************************************************************/
	ReadConfig (configfile);

	logit_init ("alarms_process_recipient", 1, EWDB_ALARMS_MAX_FORMAT_LEN*2, 1);
	logit ("", "alarms_process_recipient: starting.\n");

	/* Send header of reply back to web server */
	printf("Content-type: text/html\n\n");
	printf("<html>\n");

	printf ("<HEAD><TITLE>Recipient Manager</TITLE></HEAD>\n");

	html_header (BackgroundColor, HeaderLogo, HeaderTag);

	printf("<center>\n");
	printf("<H2><FONT COLOR=blue>Earthworm Alarms: Recipient Manager</FONT></H2>\n");
	printf("</center>\n");

	printf("<pre>\n");

	if (Webparse_GetAndProcessWebParams ((void *) (&Options)) == 1)
	{
		html_logit ("", "Call to GetAndProcessWebParams() failed\n");
		goto shutdown;
	}

    /* Connect to the DB */
    if (ewdb_api_Init (DBuser, DBpassword, DBservice) != 0)
    {
        html_logit( "", "Trouble connecting to database; exiting!\n" );
    	goto shutdown;
    }


	if (Options.idRecipient == NEW_ENTRY_FLAG)
	{
		if (html_new_recipient () != EW_SUCCESS)
		{
			html_logit ("", "Call to html_new_recipient failed.\n");
			goto shutdown;
		}
	}
	else
	{
		if (html_existing_recipient (Options.idRecipient) != EW_SUCCESS)
		{
			html_logit ("", "Call to html_existing_recipient failed.\n");
			goto shutdown;
		}
	}
	
shutdown:
	logit ("", "alarms_process_recipient: terminating.\n");
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

	WebOptionsStructAlarms *pOptions = (WebOptionsStructAlarms *) pUserParams;


    if (strcmp (szVar, "recipient") == 0)
	{
		pOptions->idRecipient = atoi (szVal);
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



/*************************** html_new_recipient ***************************/
static	int	html_new_recipient ()
{

	int		i, j;
	int							crit_NumFound, crit_NumRetr;
	int							form_NumFound, form_NumRetr;
	EWDB_AlarmsCritProgramStruct		*pCritProg;
	EWDB_AlarmsFormatStruct			*pFormat;


	/* Retrieve list of criteria programs from the DB */
	if ((pCritProg = (EWDB_AlarmsCritProgramStruct *) malloc (INIT_BUF_SIZE *
								sizeof (EWDB_AlarmsCritProgramStruct))) == NULL)
	{
		html_logit ("", "Can't malloc buffer for criteria.\n");
		return EW_FAILURE;
	}

	if (ewdb_api_GetAlarmsCriteriaList (-1, pCritProg, &crit_NumFound, 
							&crit_NumRetr, INIT_BUF_SIZE) == EWDB_RETURN_FAILURE)
	{
		html_logit ("", "Call to ewdb_api_GetAlarmsCriteriaList failed.\n");
		return EW_FAILURE;
	}

	if (crit_NumRetr < crit_NumFound)
	{
		logit ("", "Criteria buffer too small: Retr=%d, Found=%d. allocating.\n",
						crit_NumRetr, crit_NumFound);

		free (pCritProg);

		if ((pCritProg = (EWDB_AlarmsCritProgramStruct *) malloc (crit_NumFound *
								sizeof (EWDB_AlarmsCritProgramStruct))) == NULL)
		{
			html_logit ("", "Can't malloc buffer for criteria.\n");
			return EW_FAILURE;
		}

		if (ewdb_api_GetAlarmsCriteriaList (-1, pCritProg, &crit_NumFound, 
							&crit_NumRetr, INIT_BUF_SIZE) == EWDB_RETURN_FAILURE)
		{
			html_logit ("", "Call to ewdb_api_GetAlarmsCriteriaList failed.\n");
			return EW_FAILURE;
		}
	}

	/* Retrieve list of delivery formats from the DB */
	if ((pFormat = (EWDB_AlarmsFormatStruct *) malloc (INIT_BUF_SIZE *
								sizeof (EWDB_AlarmsFormatStruct))) == NULL)
	{
		html_logit ("", "Can't malloc buffer for formats.\n");
		return EW_FAILURE;
	}

	if (ewdb_api_GetAlarmsFormats (-1, pFormat, &form_NumFound, 
							&form_NumRetr, INIT_BUF_SIZE) == EWDB_RETURN_FAILURE)
	{
		html_logit ("", "Call to ewdb_api_GetAlarmsFormats failed.\n");
		return EW_FAILURE;
	}

	if (form_NumRetr < form_NumFound)
	{
		logit ("", "Format buffer too small: Retr=%d, Found=%d. allocating.\n",
						form_NumRetr, form_NumFound);

		free (pFormat);

		if ((pFormat = (EWDB_AlarmsFormatStruct *) malloc (form_NumFound *
								sizeof (EWDB_AlarmsFormatStruct))) == NULL)
		{
			html_logit ("", "Can't malloc buffer for formats.\n");
			return EW_FAILURE;
		}

		if (ewdb_api_GetAlarmsFormats (-1, pFormat, &form_NumFound, 
						&form_NumRetr, INIT_BUF_SIZE) == EWDB_RETURN_FAILURE)
		{
			html_logit ("", "Call to ewdb_api_GetAlarmsFormats failed.\n");
			return EW_FAILURE;
		}
	}

	printf ("<PRE><BR><BR><CENTER>\n");

	printf ("<STRONG>Fill out the information for the new recipient:</STRONG>\n"
				"Fill in the name of the recipient and the recipient's priority;\n\n"
				"For each rule, specify the lower magnitude limit and select \n"
				"delivery and criteria information from pull-down lists.\n\n"
				"By default the recipient will receive REVIEWED SOLUTIONS ONLY.\n"
				"Click the Automatic checkbox to request automatic solutions also.\n\n"); 
				
	printf ("<STRONG>Press Submit when done.</STRONG>\n");

	printf ("\n\n\n<CENTER></PRE>");

    printf ("<FORM NAME=\"NewRecipientForm\" ACTION=\"alarms_process_new_recipient\" METHOD=POST>\n");
	printf ("<TABLE BORDER=0>\n");

	/* First row: name */
	printf ("<TR VALIGN=center>");
	printf ("<TD COLSPAN=2 ALIGN=center>Name:</TD>"); 
	printf ("<TD COLSPAN=6 ALIGN=center><INPUT TYPE=text NAME=recipientname SIZE=64 MAXLENGTH=256></TD>"); 
	printf ("<INPUT TYPE=hidden NAME=recipientid VALUE=%d>", NEW_ENTRY_FLAG);
	printf ("</TR>\n");

	/* Second row: priority */
	printf ("<TR VALIGN=center>");
	printf ("<TD HEIGHT=100 COLSPAN=2 ALIGN=center>Priority:</TD>"); 
	printf ("<TD COLSPAN=6 ALIGN=center><INPUT TYPE=text NAME=recipientprio SIZE=3 MAXLENGTH=3></TD>"); 
	printf ("<INPUT TYPE=hidden NAME=recipientid VALUE=%d>", NEW_ENTRY_FLAG);
	printf ("</TR>\n");


    /* Put up a series of rows allowing new rule definitions */
	printf ("<TR VALIGN=center>\n");
	printf ("<TD COLSPAN=1 ALIGN=center>Mechanism</TD>\n");
	printf ("<TD COLSPAN=1 ALIGN=center>Format</TD>\n");
	printf ("<TD COLSPAN=2 ALIGN=center>Mag</TD>\n");
	printf ("<TD COLSPAN=3 ALIGN=center>Automatic Also?</TD>\n");
	printf ("<TD COLSPAN=1 ALIGN=center>Criteria Program</TD>\n");

	for (i = 0; i < NUM_INIT_RULES; i++)
	{
		printf ("<TR VALIGN=center>\n");

		/* Pulldown menu for Delivery Mechanism */
		printf ("<TD COLSPAN=1 ALIGN=center><SELECT NAME=mech SIZE=1>\n");
		printf ("<OPTION VALUE=%d_-1>Not Selected\n", NONE_ENTRY_FLAG);
		for (j = 0; j < EWDB_ALARMS_DELIVERY_NUM_MECHANISMS; j++)
			printf ("<OPTION VALUE=%d_%d>%s\n", 
				DelMethodArray[j].idMethod, NEW_ENTRY_FLAG, DelMethodArray[j].label);
		printf ("</SELECT></TD>\n");

		/* Pulldown menu for Delivery Format */
		printf ("<TD COLSPAN=1 ALIGN=center><SELECT NAME=form SIZE=1>\n");
		printf ("<OPTION VALUE=%d>Not Selected\n", NONE_ENTRY_FLAG);
		for (j = 0; j < form_NumRetr; j++)
			printf ("<OPTION VALUE=%d>%s\n", 
				pFormat[j].idFormat, pFormat[j].sDescription);
		printf ("</SELECT></TD>\n");

		/* Text field for Min magnitude */
		printf ("<TD COLSPAN=2 ALIGN=center>\n");
		printf ("<INPUT TYPE=text NAME=mag SIZE=4 MAXLENGTH=4 VALUE=\" 0.0\"></TD>\n"); 

		/* Checkbox for Automatic */
		printf ("<TD COLSPAN=3 ALIGN=center>\n");
		printf ("<INPUT TYPE=checkbox NAME=auto></TD>\n"); 

		/* Pulldown menu for criteria programs */
		printf ("<TD COLSPAN=1 ALIGN=center><SELECT NAME=crit SIZE=1>\n");
		printf ("<OPTION VALUE=%d>NO Criteria Program\n", NONE_ENTRY_FLAG);
		printf ("<OPTION VALUE=%d>NEW Criteria Program\n", NEW_ENTRY_FLAG);
		for (j = 0; j < crit_NumRetr; j++)
			printf ("<OPTION VALUE=%d>%s\n", pCritProg[j].idCritProgram, 
									pCritProg[j].sProgDescription);
		printf ("</SELECT></TD>\n");

        printf ("<INPUT TYPE=hidden NAME=rule VALUE=%d>\n", NEW_ENTRY_FLAG);
	}


    /* Last row: submit button*/
    printf ("<TR VALIGN=center>\n");
    printf ("<TD HEIGHT=100 ALIGN=center COLSPAN=8>");
	printf ("<INPUT TYPE=\"submit\" VALUE=\"SUBMIT\" NAME=\"submit\"></TD>\n");
	printf ("</TR>\n");

	printf ("</PRE></CENTER></TABLE></FORM>\n");


	return EW_SUCCESS;
}


/*************************** html_existing_recipient ***************************/
static	int	html_existing_recipient (int idRecipient)
{

	int										i, j;
	int										crit_NumFound, crit_NumRetr;
	int										form_NumFound, form_NumRetr;
	char									sel_string[32];
	EWDB_AlarmsCritProgramStruct			*pCritProg;
	EWDB_AlarmsFormatStruct					*pFormat;
	AlarmsRecipientStructRuleDelivery		RecipientRules;


	if (idRecipient < 0)
	{
		html_logit ("", "Invalid idRecipient passed in: %d\n", idRecipient);
		return EW_FAILURE;
	}


	/* Retrieve list of criteria programs from the DB */
	if ((pCritProg = (EWDB_AlarmsCritProgramStruct *) malloc (INIT_BUF_SIZE *
								sizeof (EWDB_AlarmsCritProgramStruct))) == NULL)
	{
		logit ("", "Can't malloc buffer for criteria.\n");
		return EW_FAILURE;
	}

	if (ewdb_api_GetAlarmsCriteriaList (-1, pCritProg, &crit_NumFound, 
							&crit_NumRetr, INIT_BUF_SIZE) == EWDB_RETURN_FAILURE)
	{
		logit ("", "Call to ewdb_api_GetAlarmsCriteriaList failed.\n");
		return EW_FAILURE;
	}

	if (crit_NumRetr < crit_NumFound)
	{
		logit ("", "Criteria buffer too small: Retr=%d, Found=%d. allocating.\n",
						crit_NumRetr, crit_NumFound);

		free (pCritProg);

		if ((pCritProg = (EWDB_AlarmsCritProgramStruct *) malloc (crit_NumFound *
								sizeof (EWDB_AlarmsCritProgramStruct))) == NULL)
		{
			logit ("", "Can't malloc buffer for criteria.\n");
			return EW_FAILURE;
		}

		if (ewdb_api_GetAlarmsCriteriaList (-1, pCritProg, &crit_NumFound, 
							&crit_NumRetr, INIT_BUF_SIZE) == EWDB_RETURN_FAILURE)
		{
			logit ("", "Call to ewdb_api_GetAlarmsCriteriaList failed.\n");
			return EW_FAILURE;
		}
	}
logit ("", "After GetCrit, retr=%d, found=%d\n", crit_NumRetr, crit_NumFound);


	/* Retrieve list of delivery formats from the DB */
	if ((pFormat = (EWDB_AlarmsFormatStruct *) malloc (INIT_BUF_SIZE *
								sizeof (EWDB_AlarmsFormatStruct))) == NULL)
	{
		logit ("", "Can't malloc buffer for formats.\n");
		return EW_FAILURE;
	}

	if (ewdb_api_GetAlarmsFormats (-1, pFormat, &form_NumFound, 
							&form_NumRetr, INIT_BUF_SIZE) == EWDB_RETURN_FAILURE)
	{
		logit ("", "Call to ewdb_api_GetAlarmsFormats failed.\n");
		return EW_FAILURE;
	}

	if (form_NumRetr < form_NumFound)
	{
		logit ("", "Format buffer too small: Retr=%d, Found=%d. allocating.\n",
						form_NumRetr, form_NumFound);

		free (pFormat);

		if ((pFormat = (EWDB_AlarmsFormatStruct *) malloc (form_NumFound *
								sizeof (EWDB_AlarmsFormatStruct))) == NULL)
		{
			logit ("", "Can't malloc buffer for formats.\n");
			return EW_FAILURE;
		}

		if (ewdb_api_GetAlarmsFormats (-1, pFormat, &form_NumFound, 
						&form_NumRetr, INIT_BUF_SIZE) == EWDB_RETURN_FAILURE)
		{
			logit ("", "Call to ewdb_api_GetAlarmsFormats failed.\n");
			return EW_FAILURE;
		}
	}

	/* Retrieve the information about rules for this recipient */
	if (GetRecipientAlarmRules (idRecipient, &RecipientRules) != EW_SUCCESS)
	{
		html_logit ("", "Call to GetRecipientAlarmRules failed.\n");
		return EW_FAILURE;
	}

	printf ("<PRE><BR><BR><CENTER>\n");

	printf ("<STRONG>INFORMATION ABOUT AN EXISTING RECIPIENT\n");
	printf ("Recipient Name: %s </STRONG>\n\n", RecipientRules.Recipient.sDescription);


	printf ("<HR><BR>Delivery Information</CENTER>\n\n");

	for (j = 0; j < RecipientRules.NumDeliveries; j++)
	{
		if (RecipientRules.Delivery[j].DelMethodInd == EWDB_ALARMS_DELIVERY_IND_EMAIL)
			printf ("<STRONG>Email:</STRONG>\t%s\n", RecipientRules.Delivery[j].email.sAddress);
		else if (RecipientRules.Delivery[j].DelMethodInd == EWDB_ALARMS_DELIVERY_IND_PAGER)
			printf ("<STRONG>Pager:</STRONG>\t%s\n", RecipientRules.Delivery[j].pager.sNumber);
		else if (RecipientRules.Delivery[j].DelMethodInd == EWDB_ALARMS_DELIVERY_IND_PHONE)
			printf ("<STRONG>Phone:</STRONG>\t%s\n", 
						RecipientRules.Delivery[j].phone.sPhoneNumber);
		else if (RecipientRules.Delivery[j].DelMethodInd == EWDB_ALARMS_DELIVERY_IND_QDDS)
			printf ("<STRONG>QDDS:</STRONG>\t%s\n", 
					RecipientRules.Delivery[j].qdds.sQddsDirectory);
		else if (RecipientRules.Delivery[j].DelMethodInd == EWDB_ALARMS_DELIVERY_IND_CUSTOM)
			printf ("<STRONG>Custom:</STRONG>\t%s\n", 
					RecipientRules.Delivery[j].custom.sDescription);
		else
		{
			html_logit ("", "Invalid delivery method: %d\n", 
							RecipientRules.Delivery[j].DelMethodInd);
			return EW_FAILURE;
		}
	}


	printf ("\n<CENTER><HR>\n"
			"CLICK DELETE_RECIPIENT TO COMPLETELY REMOVE THIS RECIPIENT AND ALL ASSOCIATED RULES.\n\n"
				"Otherwise, Check the Delete checkbox to delete a rule.\n\n"
				"Update any of the fields, or define new rules.\n\n"
				"By default the recipient will receive REVIEWED SOLUTIONS ONLY.\n"
				"Click the Automatic checkbox to request automatic solutions also.\n\n"); 
				
	printf ("<STRONG>Press SUBMIT to process the updated rules for this recipient.</STRONG>");

	printf ("<CENTER></PRE>");

    printf ("<FORM NAME=\"OldRecipientForm\" ACTION=\"alarms_process_new_recipient\" METHOD=POST>\n");

	printf ("<INPUT TYPE=hidden NAME=recipientname VALUE=\"%s\">\n", RecipientRules.Recipient.sDescription);
	printf ("<INPUT TYPE=hidden NAME=recipientid VALUE=\"%d\">\n", RecipientRules.Recipient.idRecipient);

	printf ("<TABLE BORDER=0>\n");

	/* first row: priority */
	printf ("<TR VALIGN=center>\n");
	printf ("<TD HEIGHT=80 COLSPAN=3 ALIGN=center>Priority:</TD>\n"); 
	printf ("<TD COLSPAN=7 ALIGN=center><INPUT TYPE=text NAME=recipientprio SIZE=3 VALUE=%d></TD>\n", RecipientRules.Recipient.dPriority); 
	printf ("</TR>\n");


    /* Put up a series of rows allowing new rule definitions */
	printf ("<TR VALIGN=center>\n");

	printf ("<TR VALIGN=center>\n");
	printf ("<TD COLSPAN=2 ALIGN=center>Delete Rule?</TD>\n");
	printf ("<TD COLSPAN=1 ALIGN=center>Mechanism</TD>\n");
	printf ("<TD COLSPAN=1 ALIGN=center>Format</TD>\n");
	printf ("<TD COLSPAN=2 ALIGN=center>Mag</TD>\n");
	printf ("<TD COLSPAN=3 ALIGN=center>Automatic</TD>\n");
	printf ("<TD COLSPAN=1 ALIGN=center>Criteria Program</TD>\n");

    /* Put up a series of rows showing existing rule definitions */
	for (i = 0; i < RecipientRules.NumRules; i++)
	{
		printf ("<TR VALIGN=center>\n");

		/* Delete Checkbox */
		printf ("<TD COLSPAN=2 ALIGN=center>\n");
		printf ("<INPUT TYPE=checkbox NAME=delete></TD>\n"); 

		/* Pulldown menu for Delivery Mechanism */
		printf ("<TD COLSPAN=1 ALIGN=center><SELECT NAME=mech SIZE=1>\n");
		printf ("<OPTION VALUE=%d_-1>Not Selected\n", NONE_ENTRY_FLAG);
		for (j = 0; j < RecipientRules.NumDeliveries; j++)
		{
			if (RecipientRules.Rule[i].DeliveryIndex == j)
				sprintf (sel_string, "SELECTED");
			else
				sprintf (sel_string, " ");

			if (RecipientRules.Delivery[j].DelMethodInd == EWDB_ALARMS_DELIVERY_IND_EMAIL)
			{
				printf ("<OPTION VALUE=%d_%d %s>Email: %s\n", 
					EWDB_ALARMS_DELIVERY_IND_EMAIL, RecipientRules.Delivery[j].email.idDelivery, 
					sel_string, RecipientRules.Delivery[j].email.sAddress);
			}
			else if (RecipientRules.Delivery[j].DelMethodInd == EWDB_ALARMS_DELIVERY_IND_PAGER)
			{
				printf ("<OPTION VALUE=%d_%d %s>Pager: %s\n", 
					EWDB_ALARMS_DELIVERY_IND_PAGER, RecipientRules.Delivery[j].pager.idDelivery,
					sel_string, RecipientRules.Delivery[j].pager.sNumber);
			}
			else if (RecipientRules.Delivery[j].DelMethodInd == EWDB_ALARMS_DELIVERY_IND_PHONE)
			{
				printf ("<OPTION VALUE=%d_%d %s>Phone: %s\n", 
					EWDB_ALARMS_DELIVERY_IND_PHONE, RecipientRules.Delivery[j].phone.idDelivery,
					sel_string, RecipientRules.Delivery[j].phone.sPhoneNumber);
			}
			else if (RecipientRules.Delivery[j].DelMethodInd == EWDB_ALARMS_DELIVERY_IND_QDDS)
			{
				printf ("<OPTION VALUE=%d_%d %s>QDDS: %s\n", 
					EWDB_ALARMS_DELIVERY_IND_QDDS, RecipientRules.Delivery[j].qdds.idDelivery,
					sel_string, RecipientRules.Delivery[j].qdds.sQddsDirectory);
			}
			else if (RecipientRules.Delivery[j].DelMethodInd == EWDB_ALARMS_DELIVERY_IND_CUSTOM)
			{
				printf ("<OPTION VALUE=%d_%d %s>Custom: %s\n", 
					EWDB_ALARMS_DELIVERY_IND_CUSTOM, 
					RecipientRules.Delivery[j].custom.idDelivery,
					sel_string, RecipientRules.Delivery[j].custom.sDescription);
			}
			else
			{
				html_logit ("", "Invalid delivery method: %d\n", 
								RecipientRules.Delivery[j].DelMethodInd);
				return EW_FAILURE;
			}
		}

		for (j = 0; j < EWDB_ALARMS_DELIVERY_NUM_MECHANISMS; j++)
		{
			printf ("<OPTION VALUE=%d_%d>New %s\n", 
				DelMethodArray[j].idMethod, NEW_ENTRY_FLAG, DelMethodArray[j].label);
		}
		printf ("</SELECT></TD>\n");

		/* Pulldown menu for Delivery Format */
		printf ("<TD COLSPAN=1 ALIGN=center><SELECT NAME=form SIZE=1>\n");
		printf ("<OPTION VALUE=%d>Not Selected\n", NONE_ENTRY_FLAG);
		for (j = 0; j < form_NumRetr; j++)
		{
			if (pFormat[j].idFormat == RecipientRules.Rule[i].Format.idFormat)
				printf ("<OPTION VALUE=%d SELECTED>%s\n", 
					pFormat[j].idFormat, pFormat[j].sDescription);
			else
				printf ("<OPTION VALUE=%d>%s\n", 
					pFormat[j].idFormat, pFormat[j].sDescription);
		}
		printf ("</SELECT></TD>\n");

		/* Text field for Min magnitude */
		printf ("<TD COLSPAN=2 ALIGN=center>\n");
		printf ("<INPUT TYPE=text NAME=mag SIZE=4 MAXLENGTH=4 VALUE=\" %0.1f\"></TD>\n",
								RecipientRules.Rule[i].dMag); 

		/* Checkbox for Automatic */
		printf ("<TD COLSPAN=3 ALIGN=center>\n");
		if (RecipientRules.Rule[i].Auto == 0)
			printf ("<INPUT TYPE=checkbox NAME=auto></TD>\n"); 
		else 
			printf ("<INPUT TYPE=checkbox checked NAME=auto></TD>\n"); 

		/* Pulldown menu for criteria programs */
		printf ("<TD COLSPAN=1 ALIGN=center><SELECT NAME=crit SIZE=1>\n");
		printf ("<OPTION VALUE=%d>NO Criteria Program\n", NONE_ENTRY_FLAG);
		printf ("<OPTION VALUE=%d>NEW Criteria Program\n", NEW_ENTRY_FLAG);
		for (j = 0; j < crit_NumRetr; j++)
		{
			if (pCritProg[j].idCritProgram == RecipientRules.Rule[i].CritProg.idCritProgram)
				printf ("<OPTION VALUE=%d SELECTED>%s\n", pCritProg[j].idCritProgram, 
										pCritProg[j].sProgDescription);
				printf ("<OPTION VALUE=%d>%s\n", pCritProg[j].idCritProgram, 
										pCritProg[j].sProgDescription);
		}
		printf ("</SELECT></TD>\n");

		printf ("<INPUT TYPE=hidden NAME=rule VALUE=%d>\n", RecipientRules.Rule[i].idRule);

	}


    /* Put up a series of rows allowing new rule definitions */
	for (i = 0; i < NUM_INIT_RULES; i++)
	{
		printf ("<TR VALIGN=center>\n");
		/* Delete Checkbox */
		printf ("<TD COLSPAN=2 ALIGN=center>\n");
		printf ("<INPUT TYPE=checkbox NAME=delete></TD>\n"); 

		/* Pulldown menu for Delivery Mechanism */
		printf ("<TD COLSPAN=1 ALIGN=center><SELECT NAME=mech SIZE=1>\n");
		printf ("<OPTION VALUE=%d_-1 SELECTED>Not Selected\n", NONE_ENTRY_FLAG);
		for (j = 0; j < RecipientRules.NumDeliveries; j++)
		{
			sprintf (sel_string, " ");

			if (RecipientRules.Delivery[j].DelMethodInd == EWDB_ALARMS_DELIVERY_IND_EMAIL)
			{
				printf ("<OPTION VALUE=%d_%d %s>Email: %s\n", 
					EWDB_ALARMS_DELIVERY_IND_EMAIL, RecipientRules.Delivery[j].email.idDelivery, 
					sel_string, RecipientRules.Delivery[j].email.sAddress);
			}
			else if (RecipientRules.Delivery[j].DelMethodInd == EWDB_ALARMS_DELIVERY_IND_PAGER)
			{
				printf ("<OPTION VALUE=%d_%d %s>Pager: %s\n", 
					EWDB_ALARMS_DELIVERY_IND_PAGER, RecipientRules.Delivery[j].pager.idDelivery,
					sel_string, RecipientRules.Delivery[j].pager.sNumber);
			}
			else if (RecipientRules.Delivery[j].DelMethodInd == EWDB_ALARMS_DELIVERY_IND_PHONE)
			{
				printf ("<OPTION VALUE=%d_%d %s>Phone: %s\n", 
					EWDB_ALARMS_DELIVERY_IND_PHONE, RecipientRules.Delivery[j].phone.idDelivery,
					sel_string, RecipientRules.Delivery[j].phone.sPhoneNumber);
			}
			else if (RecipientRules.Delivery[j].DelMethodInd == EWDB_ALARMS_DELIVERY_IND_QDDS)
			{
				printf ("<OPTION VALUE=%d_%d %s>QDDS: %s\n", 
					EWDB_ALARMS_DELIVERY_IND_QDDS, RecipientRules.Delivery[j].qdds.idDelivery,
					sel_string, RecipientRules.Delivery[j].qdds.sQddsDirectory);
			}
			else if (RecipientRules.Delivery[j].DelMethodInd == EWDB_ALARMS_DELIVERY_IND_CUSTOM)
			{
				printf ("<OPTION VALUE=%d_%d %s>Custom: %s\n", 
					EWDB_ALARMS_DELIVERY_IND_CUSTOM, 
					RecipientRules.Delivery[j].custom.idDelivery,
					sel_string, RecipientRules.Delivery[j].custom.sDescription);
			}
			else
			{
				html_logit ("", "Invalid delivery method: %d\n", 
								RecipientRules.Delivery[j].DelMethodInd);
				return EW_FAILURE;
			}
		}
		for (j = 0; j < EWDB_ALARMS_DELIVERY_NUM_MECHANISMS; j++)
		{
			printf ("<OPTION VALUE=%d_%d>New %s\n", 
				DelMethodArray[j].idMethod, NEW_ENTRY_FLAG, DelMethodArray[j].label);
		}
		printf ("</SELECT></TD>\n");

		/* Pulldown menu for Delivery Format */
		printf ("<TD COLSPAN=1 ALIGN=center><SELECT NAME=form SIZE=1>\n");
		printf ("<OPTION VALUE=%d>Not Selected\n", NONE_ENTRY_FLAG);
		for (j = 0; j < form_NumRetr; j++)
			printf ("<OPTION VALUE=%d>%s\n", 
				pFormat[j].idFormat, pFormat[j].sDescription);
		printf ("</SELECT></TD>\n");

		/* Text field for Min magnitude */
		printf ("<TD COLSPAN=2 ALIGN=center>\n");
		printf ("<INPUT TYPE=text NAME=mag SIZE=4 MAXLENGTH=4 VALUE=\" 0.0\"></TD>\n"); 

		/* Checkbox for Automatic */
		printf ("<TD COLSPAN=3 ALIGN=center>\n");
		printf ("<INPUT TYPE=checkbox NAME=auto></TD>\n"); 

		/* Pulldown menu for criteria programs */
		printf ("<TD COLSPAN=1 ALIGN=center><SELECT NAME=crit SIZE=1>\n");
		printf ("<OPTION VALUE=%d>NO Criteria Program\n", NONE_ENTRY_FLAG);
		printf ("<OPTION VALUE=%d>NEW Criteria Program\n", NEW_ENTRY_FLAG);
		for (j = 0; j < crit_NumRetr; j++)
			printf ("<OPTION VALUE=%d>%s\n", pCritProg[j].idCritProgram, 
									pCritProg[j].sProgDescription);
		printf ("</SELECT></TD>\n");

		printf ("<INPUT TYPE=hidden NAME=rule VALUE=%d>\n", NEW_ENTRY_FLAG);
	}


    /* Last two rows: submit and delete buttons*/
    printf ("<TR VALIGN=center>\n");
    printf ("<TD HEIGHT=100 ALIGN=center COLSPAN=10>");
	printf ("<INPUT TYPE=\"submit\" VALUE=\"SUBMIT\" NAME=\"submit\"></TD></TR>\n");

    printf ("<TR VALIGN=center>\n");
    printf ("<TD ALIGN=center COLSPAN=10>");
	printf ("<INPUT TYPE=\"submit\" VALUE=\"DELETE RECIPIENT\" NAME=\"delrecipient\"></TD></TR>\n");

	printf ("</PRE></CENTER></TABLE></FORM>\n");



	return EW_SUCCESS;

}

