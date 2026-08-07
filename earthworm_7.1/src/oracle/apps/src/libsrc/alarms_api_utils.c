/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: alarms_api_utils.c,v 1.11 2005/02/04 23:24:39 mark Exp $
 *    Revision history:
 *
 *    $Log: alarms_api_utils.c,v $
 *    Revision 1.11  2005/02/04 23:24:39  mark
 *    Added alarms delivery-only functions
 *
 *    Revision 1.10  2005/02/01 20:40:41  mark
 *    Added GetRecipientAlarmDelivery function
 *
 *    Revision 1.9  2004/12/08 22:24:33  mark
 *    Fixed get recipient and delete recipient functions
 *
 *    Revision 1.8  2002/06/28 21:06:22  lucky
 *    Lucky's pre-departure checkin. Most changes probably had to do with bug fixes
 *    in connection with the GIOC scaffold.
 *
 *    Revision 1.7  2002/05/28 17:25:41  lucky
 *    *** empty log message ***
 *
 *    Revision 1.6  2001/07/31 20:45:44  lucky
 *    Changed alarms nomenclature from User to Recipient
 *
 *    Revision 1.5  2001/07/28 00:43:53  lucky
 *     State of the code after debugging and testing prior to v6.0 release freeze.
 *
 *    Revision 1.4  2001/07/20 17:33:45  lucky
 *    Added InvocationString
 *
 *    Revision 1.3  2001/07/01 21:55:20  davidk
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
 *    Revision 1.2  2001/06/06 20:54:46  lucky
 *    Changes made to support multitude magnitudes, as well as amplitude picks. This is w
 *    in progress - checkin in for the sake of safety.
 *
 *    Revision 1.1  2001/05/15 02:15:22  davidk
 *    Initial revision
 *
 *
 *
 */



#include <alarms.h>
#include <ewdb_apps_utils.h>

#define         INIT_BUF_SIZE       10


char  envEW_LOG[MAXPATH];        /* where environment variable EW_LOG
                                        will be stored */
char  DBservice[EQP_MAXWORD];    /* DBMS instance to interact with */
char  DBuser[EQP_MAXWORD];       /* connect to database as */
char  DBpassword[EQP_MAXWORD];   /* Password to datasource */
char  WebHost[MAXPATH];          /* machine running the webserver */
char  AlarmDir[MAXPATH];         /* tmp directory */
char  BackgroundColor[MAXPATH];
char  HeaderLogo[MAXPATH];
char  FooterLogo[MAXPATH];
char  HeaderTag[MAXPATH];
char  FooterTag[MAXPATH];

int   DEBUG;

/*************************** GetRecipientAlarmRules ***************************/
int		GetRecipientAlarmRules (int idRecipient, 
						AlarmsRecipientStructRuleDelivery *pRecipientRules)
{
	EWDB_AlarmsRecipientStructSummary	*pRuleSummary;
	int									i, j;
	int									NumFound, NumRetr, alloc;

	/* Double-check our parameters are valid */
	if ((idRecipient < 0) || (pRecipientRules == NULL))
	{
		logit ("", "Invalid parameters passed in.\n");
		return EW_FAILURE;
	}

	/* Retrieve information about the recipient, if we don't already have it. */
	if (pRecipientRules->Recipient.idRecipient != idRecipient)
	{
		if (ewdb_api_GetAlarmsRecipientList (idRecipient, &(pRecipientRules->Recipient), &NumFound, 
											&NumRetr, 1) == EWDB_RETURN_FAILURE)
		{
			logit ("", "Call to ewdb_api_GetAlarmsRecipientList failed\n");
			return EW_FAILURE;
		}
	}

	/* Retrieve information about the delivery mechanisms associated with this recipient */
	alloc = INIT_BUF_SIZE;
	if ((pRuleSummary = (EWDB_AlarmsRecipientStructSummary *) malloc 
				(alloc * sizeof (EWDB_AlarmsRecipientStructSummary))) == NULL)
	{
		logit ("", "Could not malloc pRuleSummary buffer.\n");
		return EW_FAILURE;
	}
	if (ewdb_api_GetRecipientSummary (idRecipient, pRuleSummary, &NumFound, 
								&NumRetr, alloc) == EWDB_RETURN_FAILURE)
	{
		logit ("", "Call to ewdb_api_GetAlarmsRecipientSummary failed\n");
		free (pRuleSummary);
		return EW_FAILURE;
	}
	if (NumRetr < NumFound)
	{
		logit ("", "Summary buffer too small: retr=%d, found=%d\n",
							NumRetr, NumFound);

		free (pRuleSummary);
		alloc = NumFound + 1;
		if ((pRuleSummary = (EWDB_AlarmsRecipientStructSummary *) malloc 
					(alloc * sizeof (EWDB_AlarmsRecipientStructSummary))) == NULL)
		{
			logit ("", "Could not malloc pRuleSummary buffer.\n");
			return EW_FAILURE;
		}
	
		if (ewdb_api_GetRecipientSummary (idRecipient, pRuleSummary, &NumFound, 
									&NumRetr, alloc) == EWDB_RETURN_FAILURE)
		{
			logit ("", "Call to ewdb_api_GetAlarmsRecipientSummary failed\n");
			free (pRuleSummary);
			return EW_FAILURE;
		}
	}

	pRecipientRules->NumDeliveries = NumFound;

	/* Build the delivery array */
	for (i = 0; i < pRecipientRules->NumDeliveries; i++)
	{
		pRecipientRules->Delivery[i].idRecipientDelivery = pRuleSummary[i].idRecipientDelivery;
		if (strcmp (pRuleSummary[i].sTableName, "email") == 0)
		{
			pRecipientRules->Delivery[i].DelMethodInd = EWDB_ALARMS_DELIVERY_IND_EMAIL;
			pRecipientRules->Delivery[i].email.idDelivery = pRuleSummary[i].idDelivery;
			if (ewdb_api_GetEmailDeliveries (0, pRecipientRules->Delivery[i].email.idDelivery, 
									&pRecipientRules->Delivery[i].email, 
									&NumFound, &NumRetr, 1) == EWDB_RETURN_FAILURE)
			{
				logit ("", "Call to ewdb_api_GetEmailDeliveries failed\n");
				return EW_FAILURE;
			}
		}
		else if (strcmp (pRuleSummary[i].sTableName, "pager") == 0)
		{
			pRecipientRules->Delivery[i].DelMethodInd = EWDB_ALARMS_DELIVERY_IND_PAGER;
			pRecipientRules->Delivery[i].pager.idDelivery = pRuleSummary[i].idDelivery;
			if (ewdb_api_GetPagerDeliveries (0, pRecipientRules->Delivery[i].pager.idDelivery, 
									&pRecipientRules->Delivery[i].pager, 
									&NumFound, &NumRetr, 1) == EWDB_RETURN_FAILURE)
			{
				logit ("", "Call to ewdb_api_GetPagerDeliveries failed\n");
				return EW_FAILURE;
			}
		}
		else if (strcmp (pRuleSummary[i].sTableName, "phone") == 0)
		{
			pRecipientRules->Delivery[i].DelMethodInd = EWDB_ALARMS_DELIVERY_IND_PHONE;
			pRecipientRules->Delivery[i].phone.idDelivery = pRuleSummary[i].idDelivery;
			if (ewdb_api_GetPhoneDeliveries (0, pRecipientRules->Delivery[i].phone.idDelivery, 
									&pRecipientRules->Delivery[i].phone, 
									&NumFound, &NumRetr, 1) == EWDB_RETURN_FAILURE)
			{
				logit ("", "Call to ewdb_api_GetPhoneDeliveries failed\n");
				return EW_FAILURE;
			}
		}
		else if (strcmp (pRuleSummary[i].sTableName, "qdds") == 0)
		{
			pRecipientRules->Delivery[i].DelMethodInd = EWDB_ALARMS_DELIVERY_IND_QDDS;
			pRecipientRules->Delivery[i].qdds.idDelivery = pRuleSummary[i].idDelivery;
			if (ewdb_api_GetQddsDeliveries (0, pRecipientRules->Delivery[i].qdds.idDelivery, 
									&pRecipientRules->Delivery[i].qdds, 
									&NumFound, &NumRetr, 1) == EWDB_RETURN_FAILURE)
			{
				logit ("", "Call to ewdb_api_GetQddsDeliveries failed\n");
				return EW_FAILURE;
			}
		}
		else if (strcmp (pRuleSummary[i].sTableName, "custom") == 0)
		{
			pRecipientRules->Delivery[i].DelMethodInd = EWDB_ALARMS_DELIVERY_IND_CUSTOM;
			pRecipientRules->Delivery[i].custom.idDelivery = pRuleSummary[i].idDelivery;
			if (ewdb_api_GetCustomDeliveries (0, pRecipientRules->Delivery[i].custom.idDelivery, 
									&pRecipientRules->Delivery[i].custom, 
									&NumFound, &NumRetr, 1) == EWDB_RETURN_FAILURE)
			{
				logit ("", "Call to ewdb_api_GetCustomDeliveries failed\n");
				return EW_FAILURE;
			}
		}
		else
		{
			logit ("", "Unrecognized delivery mechanism: %s.\n",
											pRuleSummary[i].sTableName);
			return EW_FAILURE;
		}
	} /* building delivery array */

	/* Now get information on the rules associated with this recipient. */
	if (ewdb_api_GetAlarmsRecipientSummary (idRecipient, pRuleSummary, &NumFound, 
								&NumRetr, alloc) == EWDB_RETURN_FAILURE)
	{
		logit ("", "Call to ewdb_api_GetAlarmsRecipientSummary failed\n");
		free (pRuleSummary);
		return EW_FAILURE;
	}
	if (NumRetr < NumFound)
	{
		logit ("", "Summary buffer too small: retr=%d, found=%d\n",
							NumRetr, NumFound);

		free (pRuleSummary);
		alloc = NumFound + 1;
		if ((pRuleSummary = (EWDB_AlarmsRecipientStructSummary *) malloc 
					(alloc * sizeof (EWDB_AlarmsRecipientStructSummary))) == NULL)
		{
			logit ("", "Could not malloc pRuleSummary buffer.\n");
			return EW_FAILURE;
		}
	
		if (ewdb_api_GetAlarmsRecipientSummary (idRecipient, pRuleSummary, &NumFound, 
									&NumRetr, alloc) == EWDB_RETURN_FAILURE)
		{
			logit ("", "Call to ewdb_api_GetAlarmsRecipientSummary failed\n");
			free (pRuleSummary);
			return EW_FAILURE;
		}
	}
	pRecipientRules->NumRules = NumFound;

	/* Fill in information about rules */
	for (i = 0; i < pRecipientRules->NumRules; i++)
	{
		if (ewdb_api_GetAlarmsRules (pRuleSummary[i].idRule, &pRecipientRules->Rule[i], 
										&NumFound, &NumRetr, 1) == EWDB_RETURN_FAILURE)
		{
			logit ("", "Call to ewdb_api_GetAlarmsRules failed\n");
			return EW_FAILURE;
		}

		if (pRuleSummary[i].idRule != pRecipientRules->Rule[i].idRule)
		{
			logit ("", "Call to ewdb_api_GetAlarmsRules failed: idRule=%d\n",
									pRecipientRules->Rule[i].idRule);
			return EW_FAILURE;
		}

		if (pRecipientRules->Rule[i].CritProg.idCritProgram != 0)
		{
			/* Fill in the criteria program information */
			if (ewdb_api_GetAlarmsCriteriaList (pRecipientRules->Rule[i].CritProg.idCritProgram,
									&pRecipientRules->Rule[i].CritProg, &NumFound, 
													&NumRetr, 1) == EWDB_RETURN_FAILURE)
			{
				logit ("", "Call to ewdb_api_GetAlarmsCriteriaList failed\n");
				return EW_FAILURE;
			}

			pRecipientRules->Rule[i].bCritInUse = TRUE;
		}
		else
		{
			strcpy (pRecipientRules->Rule[i].CritProg.sProgName, "NULL");
			strcpy (pRecipientRules->Rule[i].CritProg.sProgDir, "NULL");
			strcpy (pRecipientRules->Rule[i].CritProg.sProgDescription, "NULL");
			pRecipientRules->Rule[i].bCritInUse = FALSE;
		}

		/* Set the delivery index to the correct delivery mechanism. */
		for (j = 0; j < pRecipientRules->NumDeliveries; j++)
		{
			if (pRecipientRules->Rule[i].idRecipientDelivery == pRecipientRules->Delivery[j].idRecipientDelivery)
			{
				pRecipientRules->Rule[i].DeliveryIndex = j;
				break;
			}
		}
	} /* loop retrieving rules */

	free (pRuleSummary);
	return EW_SUCCESS;
}

int GetRecipientAlarmDelivery(EWDBid idRecipient, AlarmsRecipientDeliveryStruct *pStruct)
{
	EWDB_AlarmsRecipientStructSummary	*pRuleSummary;
	int	i;
	int	NumFound, NumRetr, alloc;

	/* Double-check our parameters are valid */
	if ((idRecipient < 0) || (pStruct == NULL))
	{
		logit ("", "Invalid parameters passed in.\n");
		return EW_FAILURE;
	}

	/* Retrieve information about the recipient, if we don't already have it. */
	if (pStruct->Recipient.idRecipient != idRecipient)
	{
		if (ewdb_api_GetAlarmsRecipientList (idRecipient, &(pStruct->Recipient), &NumFound, 
											&NumRetr, 1) == EWDB_RETURN_FAILURE)
		{
			logit ("", "Call to ewdb_api_GetAlarmsRecipientList failed\n");
			return EW_FAILURE;
		}
	}

	/* Retrieve information about the delivery mechanisms associated with this recipient */
	alloc = INIT_BUF_SIZE;
	if ((pRuleSummary = (EWDB_AlarmsRecipientStructSummary *) malloc 
				(alloc * sizeof (EWDB_AlarmsRecipientStructSummary))) == NULL)
	{
		logit ("", "Could not malloc pRuleSummary buffer.\n");
		return EW_FAILURE;
	}
	if (ewdb_api_GetRecipientSummary (idRecipient, pRuleSummary, &NumFound, 
								&NumRetr, alloc) == EWDB_RETURN_FAILURE)
	{
		logit ("", "Call to ewdb_api_GetAlarmsRecipientSummary failed\n");
		free (pRuleSummary);
		return EW_FAILURE;
	}
	if (NumRetr < NumFound)
	{
		logit ("", "Summary buffer too small: retr=%d, found=%d\n",
							NumRetr, NumFound);

		free (pRuleSummary);
		alloc = NumFound + 1;
		if ((pRuleSummary = (EWDB_AlarmsRecipientStructSummary *) malloc 
					(alloc * sizeof (EWDB_AlarmsRecipientStructSummary))) == NULL)
		{
			logit ("", "Could not malloc pRuleSummary buffer.\n");
			return EW_FAILURE;
		}
	
		if (ewdb_api_GetRecipientSummary (idRecipient, pRuleSummary, &NumFound, 
									&NumRetr, alloc) == EWDB_RETURN_FAILURE)
		{
			logit ("", "Call to ewdb_api_GetAlarmsRecipientSummary failed\n");
			free (pRuleSummary);
			return EW_FAILURE;
		}
	}

	pStruct->NumDeliveries = NumFound;

	/* Build the delivery array */
	for (i = 0; i < pStruct->NumDeliveries; i++)
	{
		pStruct->Delivery[i].idRecipientDelivery = pRuleSummary[i].idRecipientDelivery;
		if (strcmp (pRuleSummary[i].sTableName, "email") == 0)
		{
			pStruct->Delivery[i].DelMethodInd = EWDB_ALARMS_DELIVERY_IND_EMAIL;
			pStruct->Delivery[i].email.idDelivery = pRuleSummary[i].idDelivery;
			if (ewdb_api_GetEmailDeliveries (0, pStruct->Delivery[i].email.idDelivery, 
									&pStruct->Delivery[i].email, 
									&NumFound, &NumRetr, 1) == EWDB_RETURN_FAILURE)
			{
				logit ("", "Call to ewdb_api_GetEmailDeliveries failed\n");
				return EW_FAILURE;
			}
		}
		else if (strcmp (pRuleSummary[i].sTableName, "pager") == 0)
		{
			pStruct->Delivery[i].DelMethodInd = EWDB_ALARMS_DELIVERY_IND_PAGER;
			pStruct->Delivery[i].pager.idDelivery = pRuleSummary[i].idDelivery;
			if (ewdb_api_GetPagerDeliveries (0, pStruct->Delivery[i].pager.idDelivery, 
									&pStruct->Delivery[i].pager, 
									&NumFound, &NumRetr, 1) == EWDB_RETURN_FAILURE)
			{
				logit ("", "Call to ewdb_api_GetPagerDeliveries failed\n");
				return EW_FAILURE;
			}
		}
		else if (strcmp (pRuleSummary[i].sTableName, "phone") == 0)
		{
			pStruct->Delivery[i].DelMethodInd = EWDB_ALARMS_DELIVERY_IND_PHONE;
			pStruct->Delivery[i].phone.idDelivery = pRuleSummary[i].idDelivery;
			if (ewdb_api_GetPhoneDeliveries (0, pStruct->Delivery[i].phone.idDelivery, 
									&pStruct->Delivery[i].phone, 
									&NumFound, &NumRetr, 1) == EWDB_RETURN_FAILURE)
			{
				logit ("", "Call to ewdb_api_GetPhoneDeliveries failed\n");
				return EW_FAILURE;
			}
		}
		else if (strcmp (pRuleSummary[i].sTableName, "qdds") == 0)
		{
			pStruct->Delivery[i].DelMethodInd = EWDB_ALARMS_DELIVERY_IND_QDDS;
			pStruct->Delivery[i].qdds.idDelivery = pRuleSummary[i].idDelivery;
			if (ewdb_api_GetQddsDeliveries (0, pStruct->Delivery[i].qdds.idDelivery, 
									&pStruct->Delivery[i].qdds, 
									&NumFound, &NumRetr, 1) == EWDB_RETURN_FAILURE)
			{
				logit ("", "Call to ewdb_api_GetQddsDeliveries failed\n");
				return EW_FAILURE;
			}
		}
		else if (strcmp (pRuleSummary[i].sTableName, "custom") == 0)
		{
			pStruct->Delivery[i].DelMethodInd = EWDB_ALARMS_DELIVERY_IND_CUSTOM;
			pStruct->Delivery[i].custom.idDelivery = pRuleSummary[i].idDelivery;
			if (ewdb_api_GetCustomDeliveries (0, pStruct->Delivery[i].custom.idDelivery, 
									&pStruct->Delivery[i].custom, 
									&NumFound, &NumRetr, 1) == EWDB_RETURN_FAILURE)
			{
				logit ("", "Call to ewdb_api_GetCustomDeliveries failed\n");
				return EW_FAILURE;
			}
		}
		else
		{
			logit ("", "Unrecognized delivery mechanism: %s.\n",
											pRuleSummary[i].sTableName);
			return EW_FAILURE;
		}
	} /* building delivery array */

	free (pRuleSummary);
	return EW_SUCCESS;
}


int		GetAlarmDelivery(int idRecipientDelivery, EWDB_AlarmsRecipientStructSummary *pSummary,
						 EWDB_AlarmDeliveryUnionStruct *pDelivery)
{
	int found, got;

	if (ewdb_api_GetRecipientDeliverySummary(idRecipientDelivery, pSummary,
				&found, &got, 1) != EWDB_RETURN_SUCCESS)
	{
		logit ("", "Error calling ewdb_api_GetRecipientDeliverySummary\n");
		return EWDB_RETURN_FAILURE;
	}

	pDelivery->idRecipientDelivery = idRecipientDelivery;
	if (strcmp(pSummary->sTableName, "email") == 0)
	{
		pDelivery->DelMethodInd = EWDB_ALARMS_DELIVERY_IND_EMAIL;
		if (ewdb_api_GetEmailDeliveries(0, pSummary->idDelivery, &(pDelivery->email),
				&found, &got, 1) != EWDB_RETURN_SUCCESS)
		{
			logit ("", "Error calling ewdb_api_GetEmailDeliveries\n");
			return EWDB_RETURN_FAILURE;
		}
	}
	else if (strcmp(pSummary->sTableName, "pager") == 0)
	{
		pDelivery->DelMethodInd = EWDB_ALARMS_DELIVERY_IND_PAGER;
		if (ewdb_api_GetPagerDeliveries(0, pSummary->idDelivery, &(pDelivery->pager),
				&found, &got, 1) != EWDB_RETURN_SUCCESS)
		{
			logit ("", "Error calling ewdb_api_GetEmailDeliveries\n");
			return EWDB_RETURN_FAILURE;
		}
	}
	else if (strcmp(pSummary->sTableName, "phone") == 0)
	{
		pDelivery->DelMethodInd = EWDB_ALARMS_DELIVERY_IND_PHONE;
		if (ewdb_api_GetPhoneDeliveries(0, pSummary->idDelivery, &(pDelivery->phone),
				&found, &got, 1) != EWDB_RETURN_SUCCESS)
		{
			logit ("", "Error calling ewdb_api_GetEmailDeliveries\n");
			return EWDB_RETURN_FAILURE;
		}
	}
	else if (strcmp(pSummary->sTableName, "qdds") == 0)
	{
		pDelivery->DelMethodInd = EWDB_ALARMS_DELIVERY_IND_QDDS;
		if (ewdb_api_GetQddsDeliveries(0, pSummary->idDelivery, &(pDelivery->qdds),
				&found, &got, 1) != EWDB_RETURN_SUCCESS)
		{
			logit ("", "Error calling ewdb_api_GetEmailDeliveries\n");
			return EWDB_RETURN_FAILURE;
		}
	}
	else if (strcmp(pSummary->sTableName, "custom") == 0)
	{
		pDelivery->DelMethodInd = EWDB_ALARMS_DELIVERY_IND_CUSTOM;
		if (ewdb_api_GetCustomDeliveries(0, pSummary->idDelivery, &(pDelivery->custom),
				&found, &got, 1) != EWDB_RETURN_SUCCESS)
		{
			logit ("", "Error calling ewdb_api_GetEmailDeliveries\n");
			return EWDB_RETURN_FAILURE;
		}
	}

	return EWDB_RETURN_SUCCESS;
}


/*************************** PrintRecipientAlarmRules ***************************/
int		PrintRecipientAlarmRules (AlarmsRecipientStructRuleDelivery *pRecipientRules)
{

	int 		i;

	if (pRecipientRules == NULL)
	{
		logit ("", "Invalid arguments passed in.\n");
		return EW_FAILURE;
	}

	logit ("", "BEGIN Printing Rule information for recipient ID=%d.\n", 
									pRecipientRules->Recipient.idRecipient);
	logit ("", "Recipient: %s, Priority=%d\n", 
					pRecipientRules->Recipient.sDescription, 
					pRecipientRules->Recipient.dPriority);
	logit ("", "Got %d Rules, %d Deliveries\n", 
						pRecipientRules->NumRules, pRecipientRules->NumDeliveries);
	for (i = 0; i < pRecipientRules->NumDeliveries; i++)
	{
	    if (pRecipientRules->Delivery[i].DelMethodInd == EWDB_ALARMS_DELIVERY_IND_EMAIL)
			logit ("", "Del %d: Email id=%d addr=%s\n", i,
						pRecipientRules->Delivery[i].email.idDelivery,
						pRecipientRules->Delivery[i].email.sAddress);
		else if (pRecipientRules->Delivery[i].DelMethodInd == EWDB_ALARMS_DELIVERY_IND_PAGER)
			logit ("", "Del %d: Pager id=%d number=%s\n", i,
					pRecipientRules->Delivery[i].pager.idDelivery,
					pRecipientRules->Delivery[i].pager.sNumber);
		else if (pRecipientRules->Delivery[i].DelMethodInd == EWDB_ALARMS_DELIVERY_IND_PHONE)
			logit ("", "Del %d: Phone id=%d number=%s\n", i,
					pRecipientRules->Delivery[i].phone.idDelivery,
					pRecipientRules->Delivery[i].phone.sPhoneNumber);
		else if (pRecipientRules->Delivery[i].DelMethodInd == EWDB_ALARMS_DELIVERY_IND_QDDS)
			logit ("", "Del %d: QDDS id=%d directory=%s\n", i,
					pRecipientRules->Delivery[i].qdds.idDelivery,
					pRecipientRules->Delivery[i].qdds.sQddsDirectory);
		else if (pRecipientRules->Delivery[i].DelMethodInd == EWDB_ALARMS_DELIVERY_IND_CUSTOM)
			logit ("", "Del %d: Custom id=%d description=%s\n", i,
					pRecipientRules->Delivery[i].custom.idDelivery,
					pRecipientRules->Delivery[i].custom.sDescription);
		else
			logit ("", "Del %d: Invalid delivery index %d\n", i,
					pRecipientRules->Delivery[i].DelMethodInd);
	}

	for (i = 0; i < pRecipientRules->NumRules; i++)
	{
		logit ("", "Rule %d (id=%d): Del=%d Format=%d, Mag=%0.1f, Auto=%d\n", i,
					pRecipientRules->Rule[i].idRule,
					pRecipientRules->Rule[i].DeliveryIndex,
					pRecipientRules->Rule[i].Format.idFormat,
					pRecipientRules->Rule[i].dMag,
					pRecipientRules->Rule[i].Auto);

		if (pRecipientRules->Rule[i].bCritInUse != FALSE)
			logit ("", "\t CritProgram DEFINED: id=%d, pname=%s, dir=%s, desc=%s\n",
						pRecipientRules->Rule[i].CritProg.idCritProgram,
						pRecipientRules->Rule[i].CritProg.sProgName,
						pRecipientRules->Rule[i].CritProg.sProgDir,
						pRecipientRules->Rule[i].CritProg.sProgDescription);
		else
			logit ("", "\t CritProgram NULL \n");
	}

	logit ("", "DONE printing rule info for recipient %d\n", 
									pRecipientRules->Recipient.idRecipient);

	return EW_SUCCESS;
}



/*************************** EWDB_InsertAudit ***************************/
int EWDB_InsertAudit (EWDB_AlarmAuditStruct *pAudit, EWDB_AlarmDeliveryUnionStruct *pDelivery)
{
	int tmp_recipientdel;

	if ((pAudit == NULL) || (pDelivery == NULL)) 
	{
		logit ("", "Invalid arguments passed in.\n");
		return EW_FAILURE;
	}

	/* 
	 * First insert the delivery mechanism into the appropriate 
	 * AuditDelivery table, and then make an entry into the Audit 
	 * table itself.
	 */

	if (pDelivery->DelMethodInd == EWDB_ALARMS_DELIVERY_IND_EMAIL)
	{
		if (ewdb_api_CreateEmailDelivery (&tmp_recipientdel, pAudit->Recipient.idRecipient, 
								&pDelivery->email, 1) != EWDB_RETURN_SUCCESS)
		{
			logit ("", "Call to ewdb_api_CreateEmailDelivery failed for audit delivery.\n");
			return EW_FAILURE;
		}

		pAudit->idDelivery = pDelivery->email.idDelivery;
		pAudit->DelMethodInd = EWDB_ALARMS_DELIVERY_IND_EMAIL;
	}
	else if (pDelivery->DelMethodInd == EWDB_ALARMS_DELIVERY_IND_PAGER)
	{
		if (ewdb_api_CreatePagerDelivery (&tmp_recipientdel, pAudit->Recipient.idRecipient, 
								&pDelivery->pager, 1) != EWDB_RETURN_SUCCESS)
		{
			logit ("", "Call to ewdb_api_CreatePagerDelivery failed for audit delivery.\n");
			return EW_FAILURE;
		}

		pAudit->idDelivery = pDelivery->pager.idDelivery;
		pAudit->DelMethodInd = EWDB_ALARMS_DELIVERY_IND_PAGER;
	}
	else if (pDelivery->DelMethodInd == EWDB_ALARMS_DELIVERY_IND_PHONE)
	{
		if (ewdb_api_CreatePhoneDelivery (&tmp_recipientdel, pAudit->Recipient.idRecipient, 
								&pDelivery->phone, 1) != EWDB_RETURN_SUCCESS)
		{
			logit ("", "Call to ewdb_api_CreatePhoneDelivery failed for audit delivery.\n");
			return EW_FAILURE;
		}

		pAudit->idDelivery = pDelivery->phone.idDelivery;
		pAudit->DelMethodInd = EWDB_ALARMS_DELIVERY_IND_PHONE;
	}
	else if (pDelivery->DelMethodInd == EWDB_ALARMS_DELIVERY_IND_QDDS)
	{
		if (ewdb_api_CreateQddsDelivery (&tmp_recipientdel, pAudit->Recipient.idRecipient, 
								&pDelivery->qdds, 1) != EWDB_RETURN_SUCCESS)
		{
			logit ("", "Call to ewdb_api_CreateQddsDelivery failed for audit delivery.\n");
			return EW_FAILURE;
		}

		pAudit->idDelivery = pDelivery->qdds.idDelivery;
		pAudit->DelMethodInd = EWDB_ALARMS_DELIVERY_IND_QDDS;
	}
	else if (pDelivery->DelMethodInd == EWDB_ALARMS_DELIVERY_IND_CUSTOM)
	{
		if (ewdb_api_CreateCustomDelivery (&tmp_recipientdel, pAudit->Recipient.idRecipient, 
								&pDelivery->custom, 1) != EWDB_RETURN_SUCCESS)
		{
			logit ("", "Call to ewdb_api_CreateCustomDelivery failed for audit delivery.\n");
			return EW_FAILURE;
		}

		pAudit->idDelivery = pDelivery->custom.idDelivery;
		pAudit->DelMethodInd = EWDB_ALARMS_DELIVERY_IND_CUSTOM;
	}
	else
	{
		logit ("", "Unknown delivery method: %d.\n",  pDelivery->DelMethodInd);
		return EW_FAILURE;
	}

	if (ewdb_api_CreateAlarmAudit (pAudit) != EWDB_RETURN_SUCCESS)
	{
		logit ("", "Call to ewdb_api_CreateAlarmAudit failed.\n");
		return EW_FAILURE;
	}

	return EW_SUCCESS;
}


/*************************** EWDB_GetAudits ***************************
 *
 * Story: We first retrieve the list of Audit entries into the pAudit
 *  buffer. Then, if GetDeliveryInfo is TRUE, then we will retrieve
 *  the gory details about each delivery. 
 *
 *  Note that if GetDeliveryInfo is set, we expect the pDelivery buffer
 *   to be pre-allocated to BufferLen. Otherwise, it can be NULL.
 *
 *   Just like API calls, we return WARNING if ewdb_api_GetAlarmsAudit
 *   signals that there are more Audit entries in the DB than we 
 *   have room for in the buffer.
 *
 **********************************************************************/
int		EWDB_GetAudits (int idEvent, EWDB_AlarmAuditStruct *pAudit, 
							int GetDeliveryInfo, EWDB_AlarmDeliveryUnionStruct *pDelivery, 
                            int *NumFound, int *NumRetrieved, int BufferLen)
{

	int		retval, i, fnd, retr;

	if ((idEvent < 0) || (pAudit == NULL) || (BufferLen < 0))
	{
		logit ("", "Invalid arguments passed in.\n");
		return EWDB_RETURN_FAILURE;
	}

	/* 
	 * First, get the list of Audit structures, then, if
	 * GetDeliveryInfo is set, get the details about each
	 * delivery method.
	 */

	retval = ewdb_api_GetAlarmsAudit (idEvent, pAudit, NumFound, 
												NumRetrieved, BufferLen); 
	if (retval == EWDB_RETURN_FAILURE)
	{
		logit ("", "Call to ewdb_api_GetAlarmsAudit failed.\n");
		return EW_FAILURE;
	}

	/* Fill in delivery info, if desired */
	if (GetDeliveryInfo == TRUE) 
	{
		for (i = 0; i < *NumRetrieved; i++)
		{
			if (pAudit[i].DelMethodInd == EWDB_ALARMS_DELIVERY_IND_EMAIL)
			{
				if (ewdb_api_GetEmailDeliveries (1, pAudit[i].idDelivery,
							&pDelivery[i].email, &fnd, &retr, 1) == EWDB_RETURN_FAILURE) 
				{
					logit ("", "Call to ewdb_api_GetEmailDeliveries failed.\n");
					return EWDB_RETURN_FAILURE;
				}

				if (retr < 1)
				{
					logit ("", "Email delivery for id=%d could not be retrieved.\n",
										pAudit[i].idDelivery);
				}
				else
				{
					pDelivery[i].DelMethodInd = EWDB_ALARMS_DELIVERY_IND_EMAIL;
					pDelivery[i].idRecipientDelivery = -200;
				}
			}
			else if (pAudit[i].DelMethodInd == EWDB_ALARMS_DELIVERY_IND_PAGER)
			{
				if (ewdb_api_GetPagerDeliveries (1, pAudit[i].idDelivery,
							&pDelivery[i].pager, &fnd, &retr, 1) == EWDB_RETURN_FAILURE) 
				{
					logit ("", "Call to ewdb_api_GetPagerDeliveries failed.\n");
					return EWDB_RETURN_FAILURE;
				}

				if (retr < 1)
				{
					logit ("", "Pager delivery for id=%d could not be retrieved.\n",
										pAudit[i].idDelivery);
				}
				else
				{
					pDelivery[i].DelMethodInd = EWDB_ALARMS_DELIVERY_IND_PAGER;
					pDelivery[i].idRecipientDelivery = -200;
				}
			}
			else if (pAudit[i].DelMethodInd == EWDB_ALARMS_DELIVERY_IND_PHONE)
			{
				if (ewdb_api_GetPhoneDeliveries (1, pAudit[i].idDelivery,
							&pDelivery[i].phone, &fnd, &retr, 1) == EWDB_RETURN_FAILURE) 
				{
					logit ("", "Call to ewdb_api_GetPhoneDeliveries failed.\n");
					return EWDB_RETURN_FAILURE;
				}

				if (retr < 1)
				{
					logit ("", "Phone delivery for id=%d could not be retrieved.\n",
										pAudit[i].idDelivery);
				}
				else
				{
					pDelivery[i].DelMethodInd = EWDB_ALARMS_DELIVERY_IND_PHONE;
					pDelivery[i].idRecipientDelivery = -200;
				}
			}
			else if (pAudit[i].DelMethodInd == EWDB_ALARMS_DELIVERY_IND_QDDS)
			{
				if (ewdb_api_GetQddsDeliveries (1, pAudit[i].idDelivery,
							&pDelivery[i].qdds, &fnd, &retr, 1) == EWDB_RETURN_FAILURE) 
				{
					logit ("", "Call to ewdb_api_GetQddsDeliveries failed.\n");
					return EWDB_RETURN_FAILURE;
				}

				if (retr < 1)
				{
					logit ("", "QDDS delivery for id=%d could not be retrieved.\n",
										pAudit[i].idDelivery);
				}
				else
				{
					pDelivery[i].DelMethodInd = EWDB_ALARMS_DELIVERY_IND_QDDS;
					pDelivery[i].idRecipientDelivery = -200;
				}
			}
			else if (pAudit[i].DelMethodInd == EWDB_ALARMS_DELIVERY_IND_CUSTOM)
			{
				if (ewdb_api_GetCustomDeliveries (1, pAudit[i].idDelivery,
							&pDelivery[i].custom, &fnd, &retr, 1) == EWDB_RETURN_FAILURE) 
				{
					logit ("", "Call to ewdb_api_GetCustomDeliveries failed.\n");
					return EWDB_RETURN_FAILURE;
				}

				if (retr < 1)
				{
					logit ("", "Custom delivery for id=%d could not be retrieved.\n",
										pAudit[i].idDelivery);
				}
				else
				{
					pDelivery[i].DelMethodInd = EWDB_ALARMS_DELIVERY_IND_CUSTOM;
					pDelivery[i].idRecipientDelivery = -200;
				}
			}
			else
			{
				html_logit ("", "Unknown delivery type: %d.\n", pAudit[i].DelMethodInd);
				return EW_FAILURE;
			}
		}
	}

	return retval;
}


/*************************** DeleteAlarmsRecipient ***************************/
int		DeleteAlarmsRecipient (AlarmsRecipientStructRuleDelivery *pRecipientRules)
{

	int	i;

	if (pRecipientRules == NULL)
	{
		logit ("", "Invalid parameters passed in.\n");
		return EW_FAILURE;
	}

	/* First delete the rules.  We have to delete these first, or else Oracle can complain
	 * about dependencies...
	 */
	for (i = 0; i < pRecipientRules->NumRules; i++)
	{
		if (ewdb_api_DeleteAlarmsRule (pRecipientRules->Rule[i].idRule) != EWDB_RETURN_SUCCESS)
		{
			html_logit ("", "Call to ewdb_api_DeleteAlarmsRule failed for %d.\n",
									pRecipientRules->Rule[i].idRule);
			return EW_FAILURE;
		}
	}


	/* Next delete all delivery mechanisms for this recipient */
	for (i = 0; i < pRecipientRules->NumDeliveries; i++)
	{
		if (pRecipientRules->Delivery[i].DelMethodInd == EWDB_ALARMS_DELIVERY_IND_EMAIL)
		{
			if (ewdb_api_DeleteEmailDelivery 
					(pRecipientRules->Delivery[i].email.idDelivery) != EWDB_RETURN_SUCCESS)
			{
				html_logit ("", "Call to ewdb_api_DeleteEmailDelivery failed.\n");
				return EW_FAILURE;
			}
		}
		else if (pRecipientRules->Delivery[i].DelMethodInd == EWDB_ALARMS_DELIVERY_IND_PAGER)
		{
			if (ewdb_api_DeletePagerDelivery 
					(pRecipientRules->Delivery[i].pager.idDelivery) != EWDB_RETURN_SUCCESS)
			{
				html_logit ("", "Call to ewdb_api_DeletePagerDelivery failed.\n");
				return EW_FAILURE;
			}
		}
		else if (pRecipientRules->Delivery[i].DelMethodInd == EWDB_ALARMS_DELIVERY_IND_PHONE)
		{
			if (ewdb_api_DeletePhoneDelivery 
					(pRecipientRules->Delivery[i].phone.idDelivery) != EWDB_RETURN_SUCCESS)
			{
				html_logit ("", "Call to ewdb_api_DeletePhoneDelivery failed.\n");
				return EW_FAILURE;
			}
		}
		else if (pRecipientRules->Delivery[i].DelMethodInd == EWDB_ALARMS_DELIVERY_IND_QDDS)
		{
			if (ewdb_api_DeleteQddsDelivery 
					(pRecipientRules->Delivery[i].qdds.idDelivery) != EWDB_RETURN_SUCCESS)
			{
				html_logit ("", "Call to ewdb_api_DeleteQddsDelivery failed.\n");
				return EW_FAILURE;
			}
		}
		else if (pRecipientRules->Delivery[i].DelMethodInd == EWDB_ALARMS_DELIVERY_IND_CUSTOM)
		{
			if (ewdb_api_DeleteCustomDelivery 
					(pRecipientRules->Delivery[i].custom.idDelivery) != EWDB_RETURN_SUCCESS)
			{
				html_logit ("", "Call to ewdb_api_DeleteCustomDelivery failed.\n");
				return EW_FAILURE;
			}
		}
		else
		{
			html_logit ("", "Unknown delivery method: %d.\n", 
								pRecipientRules->Delivery[i].DelMethodInd);
			return EW_FAILURE;
		}
	}


	/* And finally, delete the recipient entry */
	if (ewdb_api_DeleteAlarmsRecipient 
				(pRecipientRules->Recipient.idRecipient) != EWDB_RETURN_SUCCESS)
	{
		html_logit ("", "Call to ewdb_api_DeleteAlarmsRecipient failed for %d.\n",
								pRecipientRules->Recipient.idRecipient);
		return EW_FAILURE;
	}

	return EW_SUCCESS;
}


int DeleteAlarmsRecipientAndRules(AlarmsRecipientDeliveryStruct *pRecipient)
{
	EWDB_AlarmsRecipientStructSummary	*pRuleSummary;
	EWDBid                              idRecipient;
	int									i;
	int									NumFound, NumRetr, alloc;

	if (pRecipient == NULL)
	{
		logit ("", "Invalid parameters passed in.\n");
		return EW_FAILURE;
	}
	idRecipient = pRecipient->Recipient.idRecipient;

	/* First delete the rules.  We have to delete these first, or else Oracle can complain
	 * about dependencies...
	 */

	/* Since we don't have the rules offhand, we've gotta go get 'em. */
	alloc = 30;
	if ((pRuleSummary = (EWDB_AlarmsRecipientStructSummary *) malloc 
				(alloc * sizeof (EWDB_AlarmsRecipientStructSummary))) == NULL)
	{
		logit ("", "Could not malloc pRuleSummary buffer.\n");
		return EW_FAILURE;
	}
	if (ewdb_api_GetAlarmsRecipientSummary (idRecipient, pRuleSummary, &NumFound, 
								&NumRetr, alloc) == EWDB_RETURN_FAILURE)
	{
		logit ("", "Call to ewdb_api_GetAlarmsRecipientSummary failed\n");
		free (pRuleSummary);
		return EW_FAILURE;
	}
	if (NumRetr < NumFound)
	{
		logit ("", "Summary buffer too small: retr=%d, found=%d\n",
							NumRetr, NumFound);

		free (pRuleSummary);
		alloc = NumFound + 1;
		if ((pRuleSummary = (EWDB_AlarmsRecipientStructSummary *) malloc 
					(alloc * sizeof (EWDB_AlarmsRecipientStructSummary))) == NULL)
		{
			logit ("", "Could not malloc pRuleSummary buffer.\n");
			return EW_FAILURE;
		}
	
		if (ewdb_api_GetAlarmsRecipientSummary (idRecipient, pRuleSummary, &NumFound, 
									&NumRetr, alloc) == EWDB_RETURN_FAILURE)
		{
			logit ("", "Call to ewdb_api_GetAlarmsRecipientSummary failed\n");
			free (pRuleSummary);
			return EW_FAILURE;
		}
	}

	/* Now, delete all the rules we found. */
	for (i = 0; i < NumRetr; i++)
	{
		if (ewdb_api_DeleteAlarmsRule(pRuleSummary->idRule) != EWDB_RETURN_SUCCESS)
		{
			html_logit ("", "Call to ewdb_api_DeleteAlarmsRule failed for %d.\n",
									pRuleSummary->idRule);
			return EW_FAILURE;
		}
	}

	free (pRuleSummary);

	/* Next delete all delivery mechanisms for this recipient */
	for (i = 0; i < pRecipient->NumDeliveries; i++)
	{
		if (pRecipient->Delivery[i].DelMethodInd == EWDB_ALARMS_DELIVERY_IND_EMAIL)
		{
			if (ewdb_api_DeleteEmailDelivery 
					(pRecipient->Delivery[i].email.idDelivery) != EWDB_RETURN_SUCCESS)
			{
				html_logit ("", "Call to ewdb_api_DeleteEmailDelivery failed.\n");
				return EW_FAILURE;
			}
		}
		else if (pRecipient->Delivery[i].DelMethodInd == EWDB_ALARMS_DELIVERY_IND_PAGER)
		{
			if (ewdb_api_DeletePagerDelivery 
					(pRecipient->Delivery[i].pager.idDelivery) != EWDB_RETURN_SUCCESS)
			{
				html_logit ("", "Call to ewdb_api_DeletePagerDelivery failed.\n");
				return EW_FAILURE;
			}
		}
		else if (pRecipient->Delivery[i].DelMethodInd == EWDB_ALARMS_DELIVERY_IND_PHONE)
		{
			if (ewdb_api_DeletePhoneDelivery 
					(pRecipient->Delivery[i].phone.idDelivery) != EWDB_RETURN_SUCCESS)
			{
				html_logit ("", "Call to ewdb_api_DeletePhoneDelivery failed.\n");
				return EW_FAILURE;
			}
		}
		else if (pRecipient->Delivery[i].DelMethodInd == EWDB_ALARMS_DELIVERY_IND_QDDS)
		{
			if (ewdb_api_DeleteQddsDelivery 
					(pRecipient->Delivery[i].qdds.idDelivery) != EWDB_RETURN_SUCCESS)
			{
				html_logit ("", "Call to ewdb_api_DeleteQddsDelivery failed.\n");
				return EW_FAILURE;
			}
		}
		else if (pRecipient->Delivery[i].DelMethodInd == EWDB_ALARMS_DELIVERY_IND_CUSTOM)
		{
			if (ewdb_api_DeleteCustomDelivery 
					(pRecipient->Delivery[i].custom.idDelivery) != EWDB_RETURN_SUCCESS)
			{
				html_logit ("", "Call to ewdb_api_DeleteCustomDelivery failed.\n");
				return EW_FAILURE;
			}
		}
		else
		{
			html_logit ("", "Unknown delivery method: %d.\n", 
								pRecipient->Delivery[i].DelMethodInd);
			return EW_FAILURE;
		}
	}

	/* And finally, delete the recipient entry */
	if (ewdb_api_DeleteAlarmsRecipient(idRecipient) != EWDB_RETURN_SUCCESS)
	{
		html_logit ("", "Call to ewdb_api_DeleteAlarmsRecipient failed for %d.\n",
								pRecipient->Recipient.idRecipient);
		return EW_FAILURE;
	}

	return EW_SUCCESS;
}