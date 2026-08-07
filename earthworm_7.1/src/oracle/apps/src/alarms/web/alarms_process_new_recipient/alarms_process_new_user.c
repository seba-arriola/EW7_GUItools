
/*
 *   This file is under RCS - do not modify unless you have
 *   checked it out using the command checkout.
 *
 *    $Id: alarms_process_new_user.c,v 1.4 2001/07/28 00:49:59 lucky Exp $
 *
 *    Revision history:
 *    $Log: alarms_process_new_user.c,v $
 *    Revision 1.4  2001/07/28 00:49:59  lucky
 *    State of the code after debugging and testing prior to v6.0 release freeze.
 *
 *    Revision 1.3  2001/07/01 21:55:10  davidk
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
 *    Revision 1.1  2001/05/18 19:07:54  lucky
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



EWDB_AlarmsRuleStruct 	*pRule;

static int			NewPager;
static int			NewEmail;
static int			NewPhone;
static int			NewQdds;
static int			NewCustom;
static int			DelFlag;
static int			DelUser;
static int			NumRemove;


#define		NUM_INIT_CRITS 10

void			html_header ();


int main()
{

	int							i;
	EWDB_AlarmsUserStructRuleDelivery	Options;
	char						*configfile = "../params/alarms.d";
	char						filename[256];
	FILE						*fp;
    int     					crit_NumFound, crit_NumRetr;
    int     					mech_NumFound, mech_NumRetr;
    int     					tmp_NumFound, tmp_NumRetr;
    EWDB_AlarmsCritProgramStruct      *pCritProg;


	/* Read the configuration file (path hardcoded relative to executable)
	 *********************************************************************/
	ReadConfig (configfile);

	logit_init ("alarms_process_new_user", 1, 1024, 1);
	logit ("", "alarms_process_new_user: starting.\n");

	html_header (); 

	NewPager = FALSE;
	NewEmail = FALSE;
	NewPhone = FALSE;
	DelFlag = FALSE;
	DelUser = FALSE;
	Options.NumRules = 0;
	Options.NumDeliveries = 0;
	Options.User.idUser = NEW_ENTRY_FLAG;
	pRule = NULL;

	for (i = 0; i < EWDB_ALARMS_MAX_RULES_PER_USER; i++)
	{
		Options.Rule[i].bCritInUse = FALSE;
		Options.Rule[i].Auto = 0;
		Options.RuleUseFlag[i] = TRUE;
	}

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


    /* Retrieve list of criteria programs from the DB */
    if ((pCritProg = (EWDB_AlarmsCritProgramStruct *) malloc (NUM_INIT_CRITS *
                                sizeof (EWDB_AlarmsCritProgramStruct))) == NULL)
    {
        html_logit ("", "Can't malloc buffer for criteria.\n");
        goto shutdown;
    }

    if (ewdb_api_GetAlarmsCriteriaList (-1, pCritProg, &crit_NumFound, &crit_NumRetr,
                                NUM_INIT_CRITS) == EWDB_RETURN_FAILURE)
    {
        html_logit ("", "Call to ewdb_api_GetAlarmsCriteriaList failed.\n");
        goto shutdown;
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
            goto shutdown;
        }

        if (ewdb_api_GetAlarmsCriteriaList (-1, pCritProg, &crit_NumFound, &crit_NumRetr,
                                    NUM_INIT_CRITS) == EWDB_RETURN_FAILURE)
        {
            html_logit ("", "Call to ewdb_api_GetAlarmsCriteriaList failed.\n");
            goto shutdown;
        }
    }


	/* Retrieve known delivery information */
	for (i = 0; i < Options.NumDeliveries; i++)
	{
		if (Options.Delivery[i].DelMethodInd == EWDB_ALARMS_DELIVERY_IND_EMAIL)
		{
			if ((Options.Delivery[i].email.idDelivery != NEW_ENTRY_FLAG) &&
								(Options.Delivery[i].email.idDelivery > 0))
			{
				if (ewdb_api_GetEmailDeliveries 
						(0, Options.Delivery[i].email.idDelivery,
								&Options.Delivery[i].email, &mech_NumFound, 
											&mech_NumRetr, 1) == EWDB_RETURN_FAILURE)
				{
					html_logit ("", "Call to ewdb_api_GetEmailDeliveries failed\n");
					return EW_FAILURE;
				}
			}
		}
		else if (Options.Delivery[i].DelMethodInd == EWDB_ALARMS_DELIVERY_IND_PAGER)
		{
			if ((Options.Delivery[i].pager.idDelivery != NEW_ENTRY_FLAG) &&
								(Options.Delivery[i].pager.idDelivery > 0))
			{
				if (ewdb_api_GetPagerDeliveries 
						(0, Options.Delivery[i].pager.idDelivery,
								&Options.Delivery[i].pager, &mech_NumFound, 
											&mech_NumRetr, 1) == EWDB_RETURN_FAILURE)
				{
					html_logit ("", "Call to ewdb_api_GetPagerDeliveries failed\n");
					return EW_FAILURE;
				}
			}
		}
		else if (Options.Delivery[i].DelMethodInd == EWDB_ALARMS_DELIVERY_IND_PHONE)
		{
			if ((Options.Delivery[i].phone.idDelivery != NEW_ENTRY_FLAG) &&
								(Options.Delivery[i].phone.idDelivery > 0))
			{
				if (ewdb_api_GetPhoneDeliveries 
						(0, Options.Delivery[i].phone.idDelivery,
								&Options.Delivery[i].phone, &mech_NumFound, 
											&mech_NumRetr, 1) == EWDB_RETURN_FAILURE)
				{
					html_logit ("", "Call to ewdb_api_GetPhoneDeliveries failed\n");
					return EW_FAILURE;
				}
			}
		}
		else if (Options.Delivery[i].DelMethodInd == EWDB_ALARMS_DELIVERY_IND_QDDS)
		{
			if ((Options.Delivery[i].qdds.idDelivery != NEW_ENTRY_FLAG) &&
								(Options.Delivery[i].qdds.idDelivery > 0))
			{
				if (ewdb_api_GetQddsDeliveries 
						(0, Options.Delivery[i].qdds.idDelivery,
								&Options.Delivery[i].qdds, &mech_NumFound, 
											&mech_NumRetr, 1) == EWDB_RETURN_FAILURE)
				{
					html_logit ("", "Call to ewdb_api_GetQddsDeliveries failed\n");
					return EW_FAILURE;
				}
			}
		}
		else if (Options.Delivery[i].DelMethodInd == EWDB_ALARMS_DELIVERY_IND_CUSTOM)
		{
			if ((Options.Delivery[i].custom.idDelivery != NEW_ENTRY_FLAG) &&
								(Options.Delivery[i].custom.idDelivery > 0))
			{
				if (ewdb_api_GetCustomDeliveries 
						(0, Options.Delivery[i].custom.idDelivery,
								&Options.Delivery[i].custom, &mech_NumFound, 
											&mech_NumRetr, 1) == EWDB_RETURN_FAILURE)
				{
					html_logit ("", "Call to ewdb_api_GetCustomDeliveries failed\n");
					return EW_FAILURE;
				}
			}
		}
		else
		{
			html_logit ("", "Unknown delivery type: %d\n",
									Options.Delivery[i].DelMethodInd);
			return EW_FAILURE;
		}
	}

	/* Retrieve known criteria information */
	for (i = 0; i < Options.NumRules; i++)
	{
		if ((Options.Rule[i].bCritInUse == TRUE) &&
					(Options.Rule[i].CritProg.idCritProgram > 0))
		{

    		if (ewdb_api_GetAlarmsCriteriaList (
						Options.Rule[i].CritProg.idCritProgram, 
						&Options.Rule[i].CritProg, &tmp_NumFound, 
								&tmp_NumRetr, 1) == EWDB_RETURN_FAILURE)
    		{
		        html_logit ("", "Call to ewdb_api_GetAlarmsCriteriaList failed.\n");
       		 	goto shutdown;
		    }
		}
	}


	/* 
	 * Write out the structure to a binary file. After the user submits
	 * the form, the receiving cgi program will read this file and
	 * fill it in with information that we pass.
	 * 
	 * This is handy at this point so that we don't have to pass
	 * ALL of the information via the POST/GET method
	 */

	sprintf (filename, "%s/NEW_%d", AlarmDir, time(NULL)); 
	if ((fp = fopen (filename, "wb")) == NULL)
	{
		html_logit ("", "Could not open alarm file %s.\n", filename);
		goto shutdown;
	}

	fwrite (&Options, sizeof (EWDB_AlarmsUserStructRuleDelivery), 1, fp);
	fclose (fp);

	if (DelUser == TRUE)
	{
		printf ("<PRE><CENTER><STRONG>RECIPIENT FORM PROCESSED!</STRONG>\n");
		printf ("<BR><BR>\n");
		printf ("<FONT SIZE=+2 COLOR=\"red\"> "
				"Are you sure you want to delete this recipient: %s</FONT>\n",
													Options.User.sDescription);
		printf ("<BR><BR>\n");
		printf ("</COLOR>Press Continue to confirm deletion, or "
					"<A HREF=\"alarms_manager\">CLICK HERE</A> to return "
					"to the Earthworm Alarms Manager.\n");

		printf ("<BR><BR><HR><BR>\n");
		printf ("<FORM NAME=\"NewUserForm\" ACTION=\"alarms_insert_user\" METHOD=POST>\n");

		printf ("<INPUT TYPE=hidden NAME=\"file\" VALUE=\"%s\">\n", filename);
		printf ("<TABLE><TR>\n"); 
		
		printf ("<TD>");
		printf ("<INPUT TYPE=\"submit\" VALUE=\"CONTINUE\" NAME=\"deluser\"></TD>\n");
		printf ("</TR></TABLE></FORM>\n");

		goto shutdown;
	}


	/* 
	 * Ok, now we have parsed the new user form...Now, we need
	 * to examine the form and prompt the operator for information
	 * which we don't know about from the DB:
	 *
	 *   Email address
	 *   Pager Number
	 *   Phone Number
	 *   Qdds Info
	 *   Custom Info
	 *   Criteria Program names
	 */

	printf ("<PRE><CENTER><STRONG>RECIPIENT FORM PROCESSED!</STRONG>\n");
	printf ("<BR><BR>\n");
	printf ("<CENTER><FONT SIZE=+2 COLOR=\"red\"> Name: %s</FONT>\n", 
												Options.User.sDescription);
	printf ("<BR><BR>\n");
	printf ("Please provide the following information by filling out the form.<P><BR>");
	printf ("Press SUBMIT when done.</CENTER></PRE>\n");

	printf ("<BR><BR><HR><BR>\n");

	
    printf ("<FORM NAME=\"NewUserForm\" ACTION=\"alarms_insert_user\" METHOD=POST>\n");
    printf ("<TABLE BORDER=1 CELLPADDING=2>\n");

    /* First row: email (if necessary) */
	if (NewEmail == TRUE)
	{
		printf ("<TR VALIGN=center>\n");

		printf ("<TH ALIGN=center ROWSPAN=2><FONT COLOR=\"blue\">EMAIL INFO</FONT></TH>\n");

		printf ("<TH ALIGN=center HEIGHT=40>Email Address: </TH>\n");
		printf ("<TD ALIGN=left COLSPAN=3>\n");
		printf ("<INPUT TYPE=text NAME=emaddr SIZE=39 MAXLENGTH=256></TD>\n");

		printf ("</TR><TR ALIGN=center>\n");
		printf ("<TH ALIGN=center HEIGHT=40>Email Server: </TH>\n");
		printf ("<TD ALIGN=left COLSPAN=3>\n");
		printf ("<INPUT TYPE=text NAME=emsrvr SIZE=39 MAXLENGTH=256></TD>\n");

    	printf ("<INPUT TYPE=\"hidden\" NAME=\"update_email\" VALUE=\"%d\">\n", INFO_NEW);

		printf ("</TR>\n");
	}
	else
    	printf ("<INPUT TYPE=\"hidden\" NAME=\"update_email\" VALUE=\"%d\">\n", INFO_NONE);

    /* Second row: pager (if necessary) */
	if (NewPager == TRUE)
	{
		printf ("<TR VALIGN=center>\n");
		printf ("<TH ALIGN=center ROWSPAN=2><FONT COLOR=\"blue\">PAGER INFO</FONT></TH>\n");

		printf ("<TH ALIGN=center HEIGHT=40>Pager Number: </TH>\n");
		printf ("<TD ALIGN=left COLSPAN=3>\n");
		printf ("<INPUT TYPE=text NAME=pgrno SIZE=39 MAXLENGTH=256></TD>\n");

		printf ("</TR><TR ALIGN=center>\n");
		printf ("<TH ALIGN=center HEIGHT=40>Pager Provider: </TH>\n");
		printf ("<TD ALIGN=left COLSPAN=3>\n");
		printf ("<INPUT TYPE=text NAME=pgrco SIZE=39 MAXLENGTH=256></TD>\n");

    	printf ("<INPUT TYPE=\"hidden\" NAME=\"update_pager\" VALUE=\"%d\">\n", INFO_NEW);

		printf ("</TR>\n");
	}

    /* Third row: phone (if necessary) */
	if (NewPhone == TRUE)
	{
		printf ("<TR VALIGN=center>\n");
		printf ("<TH ALIGN=center ROWSPAN=1><FONT COLOR=\"blue\">PHONE INFO</FONT></TH>\n");

		printf ("<TH ALIGN=center HEIGHT=40>Phone Number: </TH>\n");
		printf ("<TD ALIGN=left COLSPAN=3>\n");
		printf ("<INPUT TYPE=text NAME=phoneno SIZE=39 MAXLENGTH=256></TD>\n");

    	printf ("<INPUT TYPE=\"hidden\" NAME=\"update_phone\" VALUE=\"%d\">\n", INFO_NEW);

		printf ("</TR>\n");
	}
	else
    	printf ("<INPUT TYPE=\"hidden\" NAME=\"update_phone\" VALUE=\"%d\">\n", INFO_NONE);

    /* Fourth row: qdds (if necessary) */
	if (NewQdds == TRUE)
	{
		printf ("<TR VALIGN=center>\n");
		printf ("<TH ALIGN=center ROWSPAN=1><FONT COLOR=\"blue\">QDDS INFO</FONT></TH>\n");

		printf ("<TH ALIGN=center HEIGHT=40>QDDS Directory: </TH>\n");
		printf ("<TD ALIGN=left COLSPAN=3>\n");
		printf ("<INPUT TYPE=text NAME=qddsdir SIZE=39 MAXLENGTH=256></TD>\n");

    	printf ("<INPUT TYPE=\"hidden\" NAME=\"update_qdds\" VALUE=\"%d\">\n", INFO_NEW);

		printf ("</TR>\n");
	}

    /* Fifth row: custom (if necessary) */
	if (NewCustom == TRUE)
	{
		printf ("<TR VALIGN=center>\n");
		printf ("<TH ALIGN=center ROWSPAN=1><FONT COLOR=\"blue\">CUSTOM INFO</FONT></TH>\n");

		printf ("<TH ALIGN=center HEIGHT=40>Custom Description: </TH>\n");
		printf ("<TD ALIGN=left COLSPAN=3>\n");
		printf ("<INPUT TYPE=text NAME=custdesc SIZE=39 MAXLENGTH=256></TD>\n");

    	printf ("<INPUT TYPE=\"hidden\" NAME=\"update_custom\" VALUE=\"%d\">\n", INFO_NEW);

		printf ("</TR>\n");
	}
	else
    	printf ("<INPUT TYPE=\"hidden\" NAME=\"update_cust\" VALUE=\"%d\">\n", INFO_NONE);

	/* Series of rows showing off the rules */ 

	/* Count the number of rules to remove */
	NumRemove = 0;
	for (i = 0; i < Options.NumRules; i++)
	{
		if (Options.RuleUseFlag[i] == FALSE)
			NumRemove = NumRemove + 1;
	}

	if (Options.NumRules > 0)
	{
		printf ("<TR VALIGN=center>\n");
		printf ("<TH ALIGN=center ROWSPAN=%d><FONT COLOR=\"blue\">"
					"RULES TO INSERT OR UPDATE</FONT></TH>\n", 
										(1+ Options.NumRules - NumRemove));

		/* header row */
		printf ("<TH ALIGN=center><FONT COLOR=\"olive\">Rule Description</FONT></TH>\n");
		printf ("<TD ALIGN=center><FONT COLOR=\"olive\">Prog. Name</FONT></TD>\n");
		printf ("<TD ALIGN=center><FONT COLOR=\"olive\">Prog. Dir</FONT></TD>\n");
		printf ("<TD ALIGN=center><FONT COLOR=\"olive\">Prog. Desc</FONT></TD>\n");
		printf ("</TR><TR ALIGN=center>\n");
	}
	for (i = 0; i < Options.NumRules; i++)
	{

		if (Options.RuleUseFlag[i] == TRUE)
		{
			printf ("<TH ALIGN=center HEIGHT=40>%s Rule %d, Mag=%0.1f, Auto=%d</TH>\n",
					DelMethodArray[Options.Delivery[Options.Rule[i].DeliveryIndex].DelMethodInd].label,
					i, Options.Rule[i].dMag, Options.Rule[i].Auto); 

	
			if (Options.Rule[i].bCritInUse != FALSE)
			{
				if (Options.Rule[i].CritProg.idCritProgram != NEW_ENTRY_FLAG)
				{
					/* Display what we know about this program */
					printf ("<TD ALIGN=center>%s</TD>\n", 
									Options.Rule[i].CritProg.sProgName); 
					printf ("<TD ALIGN=center>%s</TD>\n", 
									Options.Rule[i].CritProg.sProgDir); 
					printf ("<TD ALIGN=center>%s</TD>\n", 
									Options.Rule[i].CritProg.sProgDescription); 
				}
				else
				{
					/* Prompt for info on this new program */
					printf ("<TD><INPUT TYPE=text NAME=pnam SIZE=13 MAXLENGTH=256></TD>\n");
					printf ("<TD><INPUT TYPE=text NAME=pdir SIZE=13 MAXLENGTH=256></TD>\n");
					printf ("<TD><INPUT TYPE=text NAME=pdes SIZE=13 MAXLENGTH=256></TD>\n");
				}
			}
			else
			{
				/* Do nothing - no program to run */
				printf ("<TD ALIGN=center COLSPAN=3>No criteria program desired</TD>\n");
			}
	
			printf ("</TR><TR ALIGN=center>\n");

		} /* rule to be inserted */
	} /* loop over rules */


	if (NumRemove > 0)
	{
		printf ("<TR VALIGN=center>\n");
		printf ("<TH ALIGN=center ROWSPAN=%d><FONT COLOR=\"blue\">"
					"RULES TO DELETE</FONT></TH>\n", (1 + NumRemove));

		/* header row */
		printf ("<TH ALIGN=center><FONT COLOR=\"olive\">Rule Description</FONT></TH>\n");
		printf ("<TD ALIGN=center><FONT COLOR=\"olive\">Prog. Name</FONT></TD>\n");
		printf ("<TD ALIGN=center><FONT COLOR=\"olive\">Prog. Dir</FONT></TD>\n");
		printf ("<TD ALIGN=center><FONT COLOR=\"olive\">Prog. Desc</FONT></TD>\n");
		printf ("</TR><TR ALIGN=center>\n");
	}
	for (i = 0; i < Options.NumRules; i++)
	{

		if (Options.RuleUseFlag[i] == FALSE)
		{
			printf ("<TH ALIGN=center HEIGHT=40>%s Rule %d, Mag=%0.1f, Auto=%d</TH>\n",
					DelMethodArray[Options.Delivery[Options.Rule[i].DeliveryIndex].DelMethodInd].label,
					i, Options.Rule[i].dMag, Options.Rule[i].Auto); 

	
			if (Options.Rule[i].bCritInUse != FALSE)
			{
				if (Options.Rule[i].CritProg.idCritProgram != NEW_ENTRY_FLAG)
				{
					/* Display what we know about this program */
					printf ("<TD ALIGN=center>%s</TD>\n", 
									Options.Rule[i].CritProg.sProgName); 
					printf ("<TD ALIGN=center>%s</TD>\n", 
									Options.Rule[i].CritProg.sProgDir); 
					printf ("<TD ALIGN=center>%s</TD>\n", 
									Options.Rule[i].CritProg.sProgDescription); 
				}
				else
				{
					/* Prompt for info on this new program */
					printf ("<TD><INPUT TYPE=text NAME=pnam SIZE=13 MAXLENGTH=256></TD>\n");
					printf ("<TD><INPUT TYPE=text NAME=pdir SIZE=13 MAXLENGTH=256></TD>\n");
					printf ("<TD><INPUT TYPE=text NAME=pdes SIZE=13 MAXLENGTH=256></TD>\n");
				}
			}
			else
			{
				/* Do nothing - no program to run */
				printf ("<TD ALIGN=center COLSPAN=3>No criteria program desired</TD>\n");
			}
	
			printf ("</TR><TR ALIGN=center>\n");

		} /* rule to be deleted */
	} /* loop over rules */

	
    /* Last row: submit button*/
    printf ("<TR VALIGN=center>\n");
    printf ("<TD HEIGHT=100 ALIGN=center COLSPAN=5>");
    printf ("<INPUT TYPE=\"submit\" VALUE=\"SUBMIT\" NAME=\"submit\"></TD>\n");
    printf ("<INPUT TYPE=\"hidden\" NAME=\"file\" VALUE=\"%s\">\n", filename);
    printf ("</TR>\n");

    printf ("</PRE></CENTER></TABLE></FORM>\n");


shutdown:
	logit ("", "alarms_process_new_user: terminating.\n");
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

	EWDB_AlarmsUserStructRuleDelivery *pOptions = 
							(EWDB_AlarmsUserStructRuleDelivery *) pUserParams;
	int				i;
	int				mech_type, mech_id, crit;
	char			tmp[64];

/*
logit ("", "%s=%s\n", szVar, szVal);
*/

    if (strcmp (szVar, "username") == 0)
	{
		strcpy (pOptions->User.sDescription, szVal);
	}
    else if (strcmp (szVar, "userid") == 0)
	{
		pOptions->User.idUser = atoi (szVal);
	}
    else if (strcmp (szVar, "userprio") == 0)
	{
		pOptions->User.dPriority = atoi (szVal);
	}
    else if (strcmp (szVar, "rule") == 0)
	{
		if (pRule != NULL)
		{
			pRule->idRule = atoi (szVal);
		}	
	}
    else if (strcmp (szVar, "delete") == 0)
	{
		DelFlag = TRUE;
	}
    else if (strcmp (szVar, "mech") == 0)
	{
		/* 
		 * This is a signal that we have a new rule.
		 */

		/* mech value consists of two parts TYPE_ID */

		i = 0;
		while ((szVal[i] != '_') && (i < (int)strlen(szVal)))
		{
			tmp[i] = szVal[i];
			i++;
		}
		tmp[i] = '\0';
		mech_type = atoi (tmp);
		mech_id = atoi (szVal + i + 1);

		if (mech_type != NONE_ENTRY_FLAG)
		{
			if (DelFlag == TRUE)
			{
				pOptions->RuleUseFlag[pOptions->NumRules] = FALSE;
				DelFlag = FALSE;
			}
			else
			{
				pOptions->RuleUseFlag[pOptions->NumRules] = TRUE;
				DelFlag = FALSE;
			}

			/* email */
			if (mech_type == EWDB_ALARMS_DELIVERY_IND_EMAIL)
			{
				pRule = &pOptions->Rule[pOptions->NumRules];
				pOptions->NumRules = pOptions->NumRules + 1;

				if (mech_id == NEW_ENTRY_FLAG)
				{
					/* insert new email delivery */

					NewEmail = TRUE;

					pOptions->Delivery[pOptions->NumDeliveries].DelMethodInd = 
													EWDB_ALARMS_DELIVERY_IND_EMAIL;
					pOptions->Delivery[pOptions->NumDeliveries].email.idDelivery = 
													mech_id;

					pRule->DeliveryIndex = pOptions->NumDeliveries;
					pOptions->NumDeliveries = pOptions->NumDeliveries + 1;
				}
				else
				{
					/* find this email delivery in the list */
					pRule->DeliveryIndex = -1;
					for (i = 0; i < pOptions->NumDeliveries; i++)
					{
						if (pOptions->Delivery[i].email.idDelivery == mech_id)
							pRule->DeliveryIndex = i;
					}
					/* if not found, insert it into the list */
					if (pRule->DeliveryIndex == -1)
					{
						pOptions->Delivery[pOptions->NumDeliveries].DelMethodInd = 
													EWDB_ALARMS_DELIVERY_IND_EMAIL;
						pOptions->Delivery[pOptions->NumDeliveries].email.idDelivery = 
													mech_id;

						pRule->DeliveryIndex = pOptions->NumDeliveries;
						pOptions->NumDeliveries = pOptions->NumDeliveries + 1;
					}
				}
			} /* email */

			/* pager */
			else if (mech_type == EWDB_ALARMS_DELIVERY_IND_PAGER)
			{
				pRule = &pOptions->Rule[pOptions->NumRules];
				pOptions->NumRules = pOptions->NumRules + 1;

				if (mech_id == NEW_ENTRY_FLAG)
				{
					/* insert new pager delivery */

					NewPager = TRUE;

					pOptions->Delivery[pOptions->NumDeliveries].DelMethodInd = 
												EWDB_ALARMS_DELIVERY_IND_PAGER;
					pOptions->Delivery[pOptions->NumDeliveries].pager.idDelivery = 
												mech_id;

					pRule->DeliveryIndex = pOptions->NumDeliveries;
					pOptions->NumDeliveries = pOptions->NumDeliveries + 1;
				}
				else
				{
					/* find this pager delivery in the list */
					pRule->DeliveryIndex = -1;
					for (i = 0; i < pOptions->NumDeliveries; i++)
					{
						if (pOptions->Delivery[i].pager.idDelivery == mech_id)
							pRule->DeliveryIndex = i;
					}
					/* if not found, insert it into the list */
					if (pRule->DeliveryIndex == -1)
					{
						pOptions->Delivery[pOptions->NumDeliveries].DelMethodInd = 
												EWDB_ALARMS_DELIVERY_IND_PAGER;
						pOptions->Delivery[pOptions->NumDeliveries].pager.idDelivery =
												mech_id;

						pRule->DeliveryIndex = pOptions->NumDeliveries;
						pOptions->NumDeliveries = pOptions->NumDeliveries + 1;
					}
				}
			} /* pager */

			/* phone */
			else if (mech_type == EWDB_ALARMS_DELIVERY_IND_PHONE)
			{
				pRule = &pOptions->Rule[pOptions->NumRules];
				pOptions->NumRules = pOptions->NumRules + 1;

				if (mech_id == NEW_ENTRY_FLAG)
				{
					/* insert new phone delivery */

					NewPhone = TRUE;

					pOptions->Delivery[pOptions->NumDeliveries].DelMethodInd = 
													EWDB_ALARMS_DELIVERY_IND_PHONE;
					pOptions->Delivery[pOptions->NumDeliveries].phone.idDelivery = 
													mech_id;

					pRule->DeliveryIndex = pOptions->NumDeliveries;
					pOptions->NumDeliveries = pOptions->NumDeliveries + 1;
				}
				else
				{
					/* find this phone delivery in the list */
					pRule->DeliveryIndex = -1;
					for (i = 0; i < pOptions->NumDeliveries; i++)
					{
						if (pOptions->Delivery[i].phone.idDelivery == mech_id)
							pRule->DeliveryIndex = i;
					}
					/* if not found, insert it into the list */
					if (pRule->DeliveryIndex == -1)
					{
						pOptions->Delivery[pOptions->NumDeliveries].DelMethodInd = 
													EWDB_ALARMS_DELIVERY_IND_PHONE;
						pOptions->Delivery[pOptions->NumDeliveries].phone.idDelivery = 
													mech_id;

						pRule->DeliveryIndex = pOptions->NumDeliveries;
						pOptions->NumDeliveries = pOptions->NumDeliveries + 1;
					}
				}
			} /* phone */

			/* qdds */
			else if (mech_type == EWDB_ALARMS_DELIVERY_IND_QDDS)
			{
				pRule = &pOptions->Rule[pOptions->NumRules];
				pOptions->NumRules = pOptions->NumRules + 1;

				if (mech_id == NEW_ENTRY_FLAG)
				{
					/* insert new qdds delivery */

					NewQdds = TRUE;

					pOptions->Delivery[pOptions->NumDeliveries].DelMethodInd = 
													EWDB_ALARMS_DELIVERY_IND_QDDS;
					pOptions->Delivery[pOptions->NumDeliveries].qdds.idDelivery = 
													mech_id;

					pRule->DeliveryIndex = pOptions->NumDeliveries;
					pOptions->NumDeliveries = pOptions->NumDeliveries + 1;
				}
				else
				{
					/* find this qdds delivery in the list */
					pRule->DeliveryIndex = -1;
					for (i = 0; i < pOptions->NumDeliveries; i++)
					{
						if (pOptions->Delivery[i].qdds.idDelivery == mech_id)
							pRule->DeliveryIndex = i;
					}
					/* if not found, insert it into the list */
					if (pRule->DeliveryIndex == -1)
					{
						pOptions->Delivery[pOptions->NumDeliveries].DelMethodInd = 
														EWDB_ALARMS_DELIVERY_IND_QDDS;
						pOptions->Delivery[pOptions->NumDeliveries].qdds.idDelivery = 
														mech_id;

						pRule->DeliveryIndex = pOptions->NumDeliveries;
						pOptions->NumDeliveries = pOptions->NumDeliveries + 1;
					}
				}
			} /* qdds */
			else if (mech_type == EWDB_ALARMS_DELIVERY_IND_CUSTOM)
			{
				pRule = &pOptions->Rule[pOptions->NumRules];
				pOptions->NumRules = pOptions->NumRules + 1;

				if (mech_id == NEW_ENTRY_FLAG)
				{
					/* insert new custom delivery */

					NewCustom = TRUE;

					pOptions->Delivery[pOptions->NumDeliveries].DelMethodInd = 
												EWDB_ALARMS_DELIVERY_IND_CUSTOM;
					pOptions->Delivery[pOptions->NumDeliveries].custom.idDelivery = 
																mech_id;

					pRule->DeliveryIndex = pOptions->NumDeliveries;
					pOptions->NumDeliveries = pOptions->NumDeliveries + 1;
				}
				else
				{
					/* find this qdds delivery in the list */
					pRule->DeliveryIndex = -1;
					for (i = 0; i < pOptions->NumDeliveries; i++)
					{
						if (pOptions->Delivery[i].custom.idDelivery == mech_id)
							pRule->DeliveryIndex = i;
					}
					/* if not found, insert it into the list */
					if (pRule->DeliveryIndex == -1)
					{
						pOptions->Delivery[pOptions->NumDeliveries].DelMethodInd = 
													EWDB_ALARMS_DELIVERY_IND_CUSTOM;
						pOptions->Delivery[pOptions->NumDeliveries].custom.idDelivery = 
																	mech_id;

						pRule->DeliveryIndex = pOptions->NumDeliveries;
						pOptions->NumDeliveries = pOptions->NumDeliveries + 1;
					}
				}
			} /* custom */
			else
			{
				html_logit ("", "Unknown delivery mechanism: %d.\n", mech_type);
			}

		} /* Got a new rule */
		else
			pRule = NULL;
	}
    else if (strcmp (szVar, "form") == 0)
	{
		if (pRule != NULL)
		{
			pRule->Format.idFormat = atoi (szVal);
		}
	}
    else if (strcmp (szVar, "auto") == 0)
	{
		if (pRule != NULL)
		{
			pRule->Auto = 1;
		}
	}
    else if (strcmp (szVar, "mag") == 0)
	{
		if (pRule != NULL)
		{
			pRule->dMag = atof (szVal);
		}
	}
    else if (strcmp (szVar, "crit") == 0)
	{
		if (pRule != NULL)
		{
			crit = atoi (szVal);
			if (crit != NONE_ENTRY_FLAG)
			{
				pRule->bCritInUse = TRUE;
				pRule->CritProg.idCritProgram = crit;
			}
			else
				pRule->bCritInUse = FALSE;
		}
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

    printf ("<HEAD><TITLE>New Recipient Manager</TITLE></HEAD>\n"
                            "<BODY BGCOLOR=#EEEEEE TEXT=#333333>\n" );
    printf("<center>\n");
    printf("<H2><FONT COLOR=blue>Earthworm Alarms: New Recipient Manager</FONT></H2>\n");
    printf("</center>\n");

    printf("<pre>\n");
}

