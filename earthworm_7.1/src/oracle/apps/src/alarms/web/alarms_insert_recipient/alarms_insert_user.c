
/*
 *   This file is under RCS - do not modify unless you have
 *   checked it out using the command checkout.
 *
 *    $Id: alarms_insert_user.c,v 1.4 2001/07/28 00:49:59 lucky Exp $
 *
 *    Revision history:
 *    $Log: alarms_insert_user.c,v $
 *    Revision 1.4  2001/07/28 00:49:59  lucky
 *    State of the code after debugging and testing prior to v6.0 release freeze.
 *
 *    Revision 1.3  2001/07/01 21:55:09  davidk
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
 *    Revision 1.1  2001/05/18 19:07:46  lucky
 *    Initial revision
 *
 *    Revision 1.1  2001/05/15 02:15:02  davidk
 *    Initial revision
 *
 *
 */


#include <alarms.h>
#include <ewdb_apps_utils.h>
#include <webparse.h>
#include <html_common.h>

#define		NUM_INIT_CRITS 10

typedef struct _crit_option
{
	char	Name[256];
	char	Dir[256];
	char	Desc[256];
} CritOpt;

typedef struct _web_options
{
	int			EmailInfoFlag;	
	int			PagerInfoFlag;	
	int			PhoneInfoFlag;	
	int			QddsInfoFlag;	
	int			CustomInfoFlag;	
	char		AlarmsFile[256];
	char		EmailAddress[256];
	char		EmailServer[256];
	char		PagerNumber[256];
	char		PagerCompany[256];
	char		PhoneNumber[256];
	char		QddsDirectory[256];
	char		CustomDescription[256];
	CritOpt		Criteria[EWDB_ALARMS_MAX_RULES_PER_USER];
} WebOptions;


static	int		NumCrit;
static	int		DelUser;

void			html_header ();

int main()
{

	int							i;
	int							NextCrit;
	EWDB_AlarmsUserStructRuleDelivery	Options;
	WebOptions					WebOpts;
	char						*configfile = "../params/alarms.d";
	FILE						*fp;
	char						cmd[4*256];

	/* Read the configuration file (path hardcoded relative to executable)
	 *********************************************************************/
	ReadConfig (configfile);

	logit_init ("alarms_insert_user", 1, 1024, 1);
	logit ("", "alarms_insert_user: starting.\n");

	html_header (); 

	NumCrit = 0;
	DelUser = FALSE;
	WebOpts.EmailInfoFlag = INFO_NONE;
	WebOpts.PagerInfoFlag = INFO_NONE;
	WebOpts.PhoneInfoFlag = INFO_NONE;
	if (Webparse_GetAndProcessWebParams ((void *) (&WebOpts)) == 1)
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


	/* Read the Alarms Structure */
	if ((fp = fopen (WebOpts.AlarmsFile, "rb")) == NULL)
	{
		html_logit ("", "Could not open alarm file %s.\n", WebOpts.AlarmsFile);
		goto shutdown;
	}

	fread (&Options, sizeof (EWDB_AlarmsUserStructRuleDelivery), 1, fp);
	fclose (fp);

	if (DelUser == TRUE)
	{
		if (DeleteAlarmsUser (&Options) != EW_SUCCESS)
		{
			html_logit ("", "Call to DeleteAlarmsUser failed.\n");
			goto shutdown;
		}

		printf ("<CENTER><PRE>\n\n\n<BR><BR>Deleted recipient: %s!\n",
													Options.User.sDescription);

		goto shutdown;
	}

	/* Reconcile the web parameters with the Alarms structure */
	NextCrit = 0;
	for (i = 0; i < Options.NumRules; i++)
	{
		if (Options.Rule[i].CritProg.idCritProgram == NEW_ENTRY_FLAG)
		{
			strcpy (Options.Rule[i].CritProg.sProgName, 
										WebOpts.Criteria[NextCrit].Name);
			strcpy (Options.Rule[i].CritProg.sProgDir, 
										WebOpts.Criteria[NextCrit].Dir);
			strcpy (Options.Rule[i].CritProg.sProgDescription, 
										WebOpts.Criteria[NextCrit].Desc);

			NextCrit = NextCrit + 1;
			if (NextCrit > NumCrit)
			{
				/* This should never happen */
				html_logit ("", "Number of criteria exceeded - BAD NEWS.\n");
				goto shutdown;
			}
		}
	}

	/* Update delivery methods */
	for (i = 0; i < Options.NumDeliveries; i++)
	{
		if (Options.Delivery[i].DelMethodInd == EWDB_ALARMS_DELIVERY_IND_EMAIL)
		{
			if (Options.Delivery[i].email.idDelivery == NEW_ENTRY_FLAG)
			{
				/* Did we get brand new email info, or is this an update? */ 
				if (WebOpts.EmailInfoFlag != INFO_NONE)
				{
					strcpy (Options.Delivery[i].email.sAddress, WebOpts.EmailAddress);
					strcpy (Options.Delivery[i].email.sMailServer, WebOpts.EmailServer);
				}
				else
				{
					html_logit ("", "No Email info available for new delivery %d\n", i);
					goto shutdown;
				}
			}
		}
		else if (Options.Delivery[i].DelMethodInd == EWDB_ALARMS_DELIVERY_IND_PAGER)
		{
			if (Options.Delivery[i].pager.idDelivery == NEW_ENTRY_FLAG)
			{
				if (WebOpts.PagerInfoFlag != INFO_NONE)
				{
					strcpy (Options.Delivery[i].pager.sNumber, WebOpts.PagerNumber);
					strcpy (Options.Delivery[i].pager.sPagerCompany, WebOpts.PagerCompany);
				}
				else
				{
					html_logit ("", "No Pager info available for new delivery %d\n", i);
					goto shutdown;
				}
			}
		}
		else if (Options.Delivery[i].DelMethodInd == EWDB_ALARMS_DELIVERY_IND_PHONE)
		{
			if (Options.Delivery[i].phone.idDelivery == NEW_ENTRY_FLAG)
			{
				if (WebOpts.PhoneInfoFlag != INFO_NONE)
				{
					strcpy (Options.Delivery[i].phone.sPhoneNumber, 
													WebOpts.PhoneNumber);
				}
				else
				{
					html_logit ("", "No Phone info available for new delivery %d\n", i);
					goto shutdown;
				}
			}
		}
		else if (Options.Delivery[i].DelMethodInd == EWDB_ALARMS_DELIVERY_IND_QDDS)
		{
			if (Options.Delivery[i].qdds.idDelivery == NEW_ENTRY_FLAG)
			{
				if (WebOpts.QddsInfoFlag != INFO_NONE)
				{
					strcpy (Options.Delivery[i].qdds.sQddsDirectory, 
													WebOpts.QddsDirectory);
				}
				else
				{
					html_logit ("", "No Qdds info available for new delivery %d\n", i);
					goto shutdown;
				}
			}
		}
		else if (Options.Delivery[i].DelMethodInd == EWDB_ALARMS_DELIVERY_IND_CUSTOM)
		{
			if (Options.Delivery[i].custom.idDelivery == NEW_ENTRY_FLAG)
			{
				if (WebOpts.CustomInfoFlag != INFO_NONE)
				{
					strcpy (Options.Delivery[i].custom.sDescription, 
													WebOpts.CustomDescription);
				}
				else
				{
					html_logit ("", "No Custom info available for new delivery %d\n", i);
					goto shutdown;
				}
			}
		}
		else
		{
			html_logit ("", "Unknown delivery type: %d.\n",
								Options.Delivery[i].DelMethodInd);
			goto shutdown;
		}
	}



	/**** Insert the user ****/
	if (ewdb_api_CreateAlarmsUser (&Options.User) != EWDB_RETURN_SUCCESS)
	{
		html_logit ("", "Call to ewdb_api_CreateAlarmsUser failed.\n");
		goto shutdown;
	}

	if (Options.User.idUser < 0)
	{
		html_logit ("", "Call to ewdb_api_CreateAlarmsUser return bad idUser=%d\n", 
						Options.User.idUser);
		goto shutdown;
	}

	/**** Insert the criteria programs ****/
	for (i = 0; i < Options.NumRules; i++)
	{
		if ((Options.Rule[i].bCritInUse == TRUE) &&
				(Options.Rule[i].CritProg.idCritProgram == NEW_ENTRY_FLAG))
		{
			if (ewdb_api_CreateAlarmsCriteria (&Options.Rule[i].CritProg) 
															!= EWDB_RETURN_SUCCESS)
			{
				html_logit ("", "Call to ewdb_api_CreateAlarmsCriteria failed for rule %d.\n", i);
				goto shutdown;
			}
	

			if (Options.Rule[i].CritProg.idCritProgram < 0)
			{
				html_logit ("", "Rule %d: Call to ewdb_api_CreateAlarmsCriteria "
								"returned bad idCritProgram=%d\n", 
										i, Options.Rule[i].CritProg.idCritProgram);
				goto shutdown;
			}
		}
	}


	/**** Insert the delivery mechanisms ****/
	for (i = 0; i < Options.NumDeliveries; i++)
	{
		if (Options.Delivery[i].DelMethodInd == EWDB_ALARMS_DELIVERY_IND_EMAIL)
		{
			if (ewdb_api_CreateEmailDelivery (&Options.Delivery[i].idUserDelivery,
							Options.User.idUser,
							&Options.Delivery[i].email, 0) != EWDB_RETURN_SUCCESS)
			{
				html_logit ("", "Call to ewdb_api_CreateEmailDelivery failed "
									"for delivery %d.\n", i);
				goto shutdown;
			}

			if (Options.Delivery[i].idUserDelivery < 0)
			{
				html_logit ("", "Delivery %d: Call to ewdb_api_CreateEmailDelivery "
									"returned bad idUserDelivery=%d\n", 
										i, Options.Delivery[i].idUserDelivery);
				goto shutdown;
			}
		}
		else if (Options.Delivery[i].DelMethodInd == EWDB_ALARMS_DELIVERY_IND_PAGER)
		{
			if (ewdb_api_CreatePagerDelivery (&Options.Delivery[i].idUserDelivery,
								Options.User.idUser,
								&Options.Delivery[i].pager, 0) != EWDB_RETURN_SUCCESS)
			{
				html_logit ("", "Call to ewdb_api_CreatePagerDelivery failed "
										"for delivery %d.\n", i);
				goto shutdown;
			}
	
			if (Options.Delivery[i].idUserDelivery < 0)
			{
				html_logit ("", "Delivery %d: Call to ewdb_api_CreatePagerDelivery "
									"returned bad idUserDelivery=%d\n", 
											i, Options.Delivery[i].idUserDelivery);
				goto shutdown;
			}
		}
		else if (Options.Delivery[i].DelMethodInd == EWDB_ALARMS_DELIVERY_IND_PHONE)
		{
			if (ewdb_api_CreatePhoneDelivery (&Options.Delivery[i].idUserDelivery,
								Options.User.idUser,
								&Options.Delivery[i].phone, 0) != EWDB_RETURN_SUCCESS)
			{
				html_logit ("", "Call to ewdb_api_CreatePhoneDelivery failed for "
											"delivery %d.\n", i);
				goto shutdown;
			}
	
			if (Options.Delivery[i].idUserDelivery < 0)
			{
				html_logit ("", "Delivery %d: Call to ewdb_api_CreatePhoneDelivery "
									"returned bad idUserDelivery=%d\n", 
											i, Options.Delivery[i].idUserDelivery);
				goto shutdown;
			}
		}
		else if (Options.Delivery[i].DelMethodInd == EWDB_ALARMS_DELIVERY_IND_QDDS)
		{
			if (ewdb_api_CreateQddsDelivery (&Options.Delivery[i].idUserDelivery,
								Options.User.idUser,
								&Options.Delivery[i].qdds, 0) != EWDB_RETURN_SUCCESS)
			{
				html_logit ("", "Call to ewdb_api_CreateQddsDelivery failed for "
											"delivery %d.\n", i);
				goto shutdown;
			}
	
			if (Options.Delivery[i].idUserDelivery < 0)
			{
				html_logit ("", "Delivery %d: Call to ewdb_api_CreateQddsDelivery "
									"returned bad idUserDelivery=%d\n", 
											i, Options.Delivery[i].idUserDelivery);
				goto shutdown;
			}
		}
		else if (Options.Delivery[i].DelMethodInd == EWDB_ALARMS_DELIVERY_IND_CUSTOM)
		{
			if (ewdb_api_CreateCustomDelivery (&Options.Delivery[i].idUserDelivery,
								Options.User.idUser,
								&Options.Delivery[i].custom, 0) != EWDB_RETURN_SUCCESS)
			{
				html_logit ("", "Call to ewdb_api_CreateCustomDelivery failed for "
											"delivery %d.\n", i);
				goto shutdown;
			}
	
			if (Options.Delivery[i].idUserDelivery < 0)
			{
				html_logit ("", "Delivery %d: Call to ewdb_api_CreateCustomDelivery "
									"returned bad idUserDelivery=%d\n", 
											i, Options.Delivery[i].idUserDelivery);
				goto shutdown;
			}
		}
		else
		{
			html_logit ("", "Unknown delivery type: %d.\n",
									Options.Delivery[i].DelMethodInd);
			goto shutdown;
		}
	}



	/**** process the rules ****/
	for (i = 0; i < Options.NumRules; i++)
	{

		if (Options.RuleUseFlag[i] == TRUE)
		{
			int tmpind;

			tmpind = Options.Rule[i].DeliveryIndex;
	
			if (ewdb_api_CreateAlarmsRule (&Options.Rule[i], 
								&(Options.Delivery[tmpind])) != EWDB_RETURN_SUCCESS)
			{
				html_logit ("", "Call to ewdb_api_CreateAlarmsRule failed for rule %d.\n", i);
				goto shutdown;
			}
	
			if (Options.Rule[i].idRule < 0)
			{
					html_logit ("", "Rule %d: Call to ewdb_api_CreateAlarmsRule "
										"returned bad idRule=%d\n", i, Options.Rule[i].idRule);
					goto shutdown;
			}

		} /* insert the rule */
		else
		{
			if (ewdb_api_DeleteAlarmsRule (Options.Rule[i].idRule) != EWDB_RETURN_SUCCESS)
			{
				html_logit ("", "Call to ewdb_api_DeleteAlarmsRule failed for rule %d.\n", i);
				goto shutdown;
			}
		} /* delete the rule */
	}

	printf ("<CENTER><PRE>\n\n\n<BR><BR>Recipient created or updated!\n");
	printf ("Database ID=%d.\n", Options.User.idUser);



shutdown:
	printf ("\n\n\n<CENTER><BR><BR><A HREF=\"alarms_manager\">");
	printf ("Click here to return to the Alarms Manager.</A></PRE></CENTER>\n");


	/* Remove the temporary file */
	sprintf (cmd, "/bin/rm %s", WebOpts.AlarmsFile);
	system (cmd);

	logit ("", "alarms_insert_user: terminating; deleted tmp file: %s.\n", 
         WebOpts.AlarmsFile);

	ewdb_api_Shutdown();
	html_trailer (WebHost);

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

	WebOptions *pOptions = (WebOptions *) pUserParams;


/*
logit ("", "szVar=%s, szVal=%s\n", szVar, szVal);
*/

    if (strcmp (szVar, "file") == 0)
	{
		strcpy (pOptions->AlarmsFile, szVal);
	}
    else if (strcmp (szVar, "emaddr") == 0)
	{
		strcpy (pOptions->EmailAddress, szVal);
	}
    else if (strcmp (szVar, "emsrvr") == 0)
	{
		strcpy (pOptions->EmailServer, szVal);
	}
    else if (strcmp (szVar, "pgrno") == 0)
	{
		strcpy (pOptions->PagerNumber, szVal);
	}
    else if (strcmp (szVar, "pgrco") == 0)
	{
		strcpy (pOptions->PagerCompany, szVal);
	}
    else if (strcmp (szVar, "phoneno") == 0)
	{
		strcpy (pOptions->PhoneNumber, szVal);
	}
    else if (strcmp (szVar, "qddsdir") == 0)
	{
		strcpy (pOptions->QddsDirectory, szVal);
	}
    else if (strcmp (szVar, "custdesc") == 0)
	{
		strcpy (pOptions->CustomDescription, szVal);
	}
    else if (strcmp (szVar, "update_email") == 0)
	{
		pOptions->EmailInfoFlag = atoi (szVal);
	}
    else if (strcmp (szVar, "update_pager") == 0)
	{
		pOptions->PagerInfoFlag = atoi (szVal);
	}
    else if (strcmp (szVar, "update_phone") == 0)
	{
		pOptions->PhoneInfoFlag = atoi (szVal);
	}
    else if (strcmp (szVar, "update_qdds") == 0)
	{
		pOptions->QddsInfoFlag = atoi (szVal);
	}
    else if (strcmp (szVar, "update_cust") == 0)
	{
		pOptions->CustomInfoFlag = atoi (szVal);
	}

	/* Get criteria fields */
    else if (strcmp (szVar, "pnam") == 0)
	{
		strcpy (pOptions->Criteria[NumCrit].Name, szVal);
	}
	else if (strcmp (szVar, "pdir") == 0)
	{
		strcpy (pOptions->Criteria[NumCrit].Dir, szVal);
	}
	else if (strcmp (szVar, "pdes") == 0)
	{
		strcpy (pOptions->Criteria[NumCrit].Desc, szVal);
		NumCrit = NumCrit + 1;
	}
    else if (strcmp (szVar, "deluser") == 0)
	{
		DelUser = TRUE;
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


void    html_header ()
{
    /* Send header of reply back to web server */
    printf("Content-type: text/html\n\n");
    printf("<html>\n");

    printf ("<HEAD><TITLE>New Recipient Confirmation</TITLE></HEAD>\n"
                            "<BODY BGCOLOR=#EEEEEE TEXT=#333333>\n" );
    printf("<center>\n");
    printf("<H2><FONT COLOR=blue>Earthworm Alarms: New Recipient Confirmation</FONT></H2>\n");
    printf("</center>\n");

    printf("<pre>\n");
}

