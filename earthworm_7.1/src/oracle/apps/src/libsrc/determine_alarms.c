/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: determine_alarms.c,v 1.24 2005/01/04 19:37:18 mark Exp $
 *    Revision history:
 *
 *    $Log: determine_alarms.c,v $
 *    Revision 1.24  2005/01/04 19:37:18  mark
 *    Fixed hypoTWC formatting
 *
 *    Revision 1.23  2004/12/28 17:43:49  mark
 *    Increased char arrays to prevent stack overwrite; fixed compile warnings
 *
 *    Revision 1.22  2004/12/15 18:27:29  mark
 *    Made building CUBE and hypoTWC messages available to external functions
 *
 *    Revision 1.21  2004/06/03 16:12:22  cjbryan
 *    changed MAGTYPE_SCALAR_MOMENT TO MAGTYPE_MWP
 *
 *    Revision 1.20  2003/07/01 19:31:58  lucky
 *    Fixed nasty bug which caused problems where origin without any magnitudes was being
 *    considered...
 *
 *    Revision 1.19  2003/01/30 23:12:57  lucky
 *    *** empty log message ***
 *
 *    Revision 1.18  2002/11/21 18:37:17  lucky
 *    Fixed bad azm check; also the annoying NT carriage returns, and C++ comments
 *
 *    Revision 1.17  2002/11/16 00:26:17  alex
 *    *** empty log message ***
 *
 *    Revision 1.15  2002/10/08 18:14:52  lucky
 *    Added nearest town option to alarm formats
 *
 *    Revision 1.14  2002/07/16 19:35:51  davidk
 *    Moved the platform ifdef for deleting a file, into the header area.
 *    Included a changed of checking for magtype = '?' that was on katherine,
 *    but done by I don't know whom.
 *    Added code that checks to make sure that the fullevtfile name was
 *    created (NOT NULL) before it tries to delete the file from the
 *    filesystem.  Programs using this code were generating strange system
 *    error messages when they tried to delete files without names. (Go figure).
 *
 *    Revision 1.13  2002/06/14 18:52:41  lucky
 *    Fixed creation of qdds message where it would barf on unkonwn mag
 *    type.. now we insert a blank space
 *
 *    Revision 1.12  2002/05/28 17:25:41  lucky
 *    *** empty log message ***
 *
 *    Revision 1.11  2001/08/07 16:53:30  lucky
 *    Pre v6.0 checkin
 *
 *    Revision 1.10  2001/07/31 20:45:44  lucky
 *    Changed alarms nomenclature from User to Recipient
 *
 *    Revision 1.9  2001/07/28 00:43:53  lucky
 *     State of the code after debugging and testing prior to v6.0 release freeze.
 *
 *    Revision 1.8  2001/07/20 17:33:45  lucky
 *    Added InvocationString
 *
 *    Revision 1.7  2001/07/01 21:55:20  davidk
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
 *    Revision 1.6  2001/06/26 18:07:23  lucky
 *    Network code is now a parameter to DetermineAlarms so that it
 *    can be passed to the CUBE msg builder.
 *
 *    Revision 1.5  2001/06/26 17:37:03  lucky
 *    State of the code after all Utah specs have been met
 *
 *    Revision 1.4  2001/06/06 20:54:46  lucky
 *    Changes made to support multitude magnitudes, as well as amplitude picks. This is w
 *    in progress - checkin in for the sake of safety.
 *
 *    Revision 1.3  2001/05/15 18:47:35  davidk
 *    Moved functions around between the apps, DB API, and DB API INTERNAL
 *    levels.  Renamed functions and files.  Added support for amplitude
 *    magnitude types.  Reformatted makefiles.
 *
 *
 *
 *
 */



#include <alarms.h>
#include <ewdb_apps_utils.h>
#include <ew_event_info.h>
#include <time_ew.h>
#include <review.h>
#include <review_globals.h>

#define         INIT_BUF_SIZE        100
#define         MAX_PLACE_ITER       10
#define         INIT_NUM_PLACES      10
#define         INIT_LAT_RANGE         1.0
#define         INIT_LON_RANGE         1.0
#define         KM_TO_MILE            1.6

#define PI 3.14159265358979323846


typedef struct _build_msg_struct
{

	char 					*AlarmMsg;
	EWDB_AlarmsFormatStruct	*pFormat;
	EWEventInfoStruct		*pEvent;
	int						isAuto;
	int						isInsert;
	char					*networkCode;
	char					*InvString;
	int						BigCityPop;		/* Population limit for big cities */
	int						ShowPopulation;		/* Population limit for big cities */

} BuildMsgStruct;


static int BuildAlarmMessage (BuildMsgStruct *);
static int BuildQDDSMessage (char *, EWEventInfoStruct *, int, int, char *);
static int CompareRecipientPriorities (const void *arg1, const void *arg2);
static int SortPlacesByDistance (const void *arg1, const void *arg2);
static int GetMenloCheckChar (char *);
static int QddsValidateEvent  (EWEventInfoStruct *pEvtInfo, int *result);
static  int     ComputeBearingFromAzm (double azm, char *bearing);


static	double	CompOriginLat, CompOriginLon;

#define FMT_CHAR        '~'

/* DK   This #ifdef should probably be done in platform.h, but atleast it's
   an improvement over the old way. */
#ifdef _WINNT
# define SYSTEM_DELETE_COMMAND "del"
#else
# define SYSTEM_DELETE_COMMAND "rm"
#endif


/*************************** DetermineAlarms ***************************/
/*
int 	DetermineAlarms (EWEventInfoStruct *pEvtInfo, int isAuto, int isInsert, 
				char *networkCode, char *InvocationString, AlarmList *pAlarms, 
				int AlarmListSize, int *NumAlarmsFound, int *NumAlarmsRetrieved, int *retVal)
*/
int     DetermineAlarms (EWEventInfoStruct *pEvtInfo, DetAlmsParams *pDAP, 
                AlarmList *pAlarms, int AlarmListSize, int *NumAlarmsFound, 
                int *NumAlarmsRetrieved, int *retVal)
{

	int									i, j, k, found, NumFound, NumRetr;
	int									RuleFired, tmpind, AutoMatch, result;
	char								evtfile[256];
	char								fullevtfile[256];
	char								cmd[1024];
	FILE								*fp;
	EWDB_AlarmsRecipientStruct         	*pRecipientList;
	AlarmsRecipientStructRuleDelivery   RecipientRules;
	EWDB_AlarmAuditStruct         		*pAudit;
	EWDB_AlarmDeliveryUnionStruct       *pDelivery;
	EWEventInfoStruct					OutEvtInfo;
	BuildMsgStruct						BMS;
	double								PrefMag;

	/* 
	 * Return values:  -101: fatal programming error;
	 *                 -102: database error;
   	 *				   -103: format error;
   	 *				   -100: success
     */


	*retVal = -100;

	if ((pEvtInfo == NULL) || (pAlarms == NULL) || (pDAP == NULL))
	{
		logit ("", "Invalid arguments passed in.\n");
		*retVal = -101;
		return EW_FAILURE;
	}

	/**********************************************************************
	*
	* Story: 
	*  First get the list of all recipients in the DB. Then, for each recipient, get
	*  all the Rules. Now, for each rule, check against the event's magnitude, 
	*  and auto/reviewed flag. If the rule has criteria program associated
	*  with it, run the program, retrieve the result. If the rule "fires" 
	*  (alarm should be issued), add it to the AlarmList. 
	*
	**********************************************************************/
	/* Get the list of recipients */
	if ((pRecipientList = (EWDB_AlarmsRecipientStruct *) malloc (INIT_BUF_SIZE * 
								sizeof (EWDB_AlarmsRecipientStruct))) == NULL)
	{
		logit ("", "Could not malloc recipient buffer.\n");
		*retVal = -101;
		return EW_FAILURE;
	}

	if (ewdb_api_GetAlarmsRecipientList (-1, pRecipientList, &NumFound, &NumRetr,
									INIT_BUF_SIZE) == EWDB_RETURN_FAILURE)
	{
		logit( "", "Call to ewdb_api_GetAlarmsRecipientList failed.\n");
		free (pRecipientList);
		*retVal = -102;
		return EW_FAILURE;
	}

	if (NumRetr < NumFound)
	{
		logit ("", "Buffer too small: Retr=%d, Found=%d; allocating more.\n",
													NumRetr, NumFound);

		free (pRecipientList);
		if ((pRecipientList = (EWDB_AlarmsRecipientStruct *) malloc (NumFound *
									sizeof (EWDB_AlarmsRecipientStruct))) == NULL)
		{
			logit( "", "Can't malloc Recipient buffer. \n" );
			*retVal = -101;
			return EW_FAILURE;
		}

		/* Retrieve Recipient List */
		if (ewdb_api_GetAlarmsRecipientList (-1, pRecipientList, &NumFound, &NumRetr,
												NumFound) == EWDB_RETURN_FAILURE)
		{
			logit( "", "Call to ewdb_api_GetAlarmsRecipientList failed.\n");
			*retVal = -102;
			free (pRecipientList);
			return EW_FAILURE;
		}
	}


	/* Sort the recipient list by priorities */
	qsort (pRecipientList, NumRetr, sizeof (EWDB_AlarmsRecipientStruct), CompareRecipientPriorities);


	*NumAlarmsFound = 0;
	*NumAlarmsRetrieved = 0;


	for (i = 0; i < NumFound; i++)
	{
		/* Get rule info for this recipient from the DB */

		/* Retrieve the information about rules for this recipient */
		if (GetRecipientAlarmRules (pRecipientList[i].idRecipient, &RecipientRules) != EW_SUCCESS)
		{
			logit ("", "Call to GetRecipientAlarmRules failed.\n");
			*retVal = -102;
			free (pRecipientList);
			return EW_FAILURE;
		}


		for (j = 0; j < RecipientRules.NumRules; j++)
		{
			RuleFired = FALSE;

			/* 
			 * Check auto/reviewed flag: If Auto flag is TRUE in the
		 	 * rule, this means that this recipient gets BOTH
			 * automatic and reviewed.
			 */

			if (RecipientRules.Rule[j].Auto == TRUE)
			{
				AutoMatch = TRUE;
			}
			else
			{
				if (pDAP->isAuto == FALSE)
					AutoMatch = TRUE;
				else
					AutoMatch = FALSE;
			}
					
			if (pEvtInfo->iPrefMag < 0)
				PrefMag = -99.0;
			else
				PrefMag = pEvtInfo->Mags[pEvtInfo->iPrefMag].dMagAvg;

			/* first Check mag and auto/reviewed flag */
			if ((RecipientRules.Rule[j].dMag <= PrefMag) && (AutoMatch == TRUE))
			{
				if (RecipientRules.Rule[j].bCritInUse == TRUE)
				{
					/* write the DB structure to file */
					sprintf (evtfile, "%d%d.bin", pEvtInfo->Event.idEvent, time(NULL));
					sprintf (fullevtfile, "%s%c%d%d.bin", RecipientRules.Rule[j].CritProg.sProgDir, 
										DIR_SLASH, pEvtInfo->Event.idEvent, time(NULL));

					if ((fp = fopen (fullevtfile, "wb")) == NULL)
					{
						logit ("", "Can't open %s\n", fullevtfile);
						*retVal = -101;
						free (pRecipientList);
						return EWDB_RETURN_FAILURE;
					}
	
					fwrite ((void *) pEvtInfo, sizeof (EWEventInfoStruct), 1, fp);
	
					/* append channel info structs */
					fwrite ((void *) pEvtInfo->pChanInfo, sizeof (EWChannelDataStruct),
																	pEvtInfo->iNumChans, fp);
					fclose (fp);
		
					/* Run the rule command */
					sprintf (cmd, "%s %s %s", RecipientRules.Rule[j].CritProg.sProgName,
									RecipientRules.Rule[j].CritProg.sProgDir, evtfile);
								
					system (cmd);

					/* Check the output file */
					if ((fp = fopen (fullevtfile, "rb")) == NULL)
					{
						logit ("", "Can't open %s\n", fullevtfile);
						*retVal = -101;
						free (pRecipientList);
						return EWDB_RETURN_FAILURE;
					}

					/* 
				     * Read the structure produced by the rule program 
					 *  if the eventid is negative of the one that was passed in,
					 *  then the rule program fired. Otherwise, the event did not
					 *  fit the criteria, or the program failed. 
					 */
					fread ((void *) &OutEvtInfo, sizeof (EWEventInfoStruct), 1, fp);
					fclose (fp);

					if (OutEvtInfo.Event.idEvent == -pEvtInfo->Event.idEvent)
					{
						RuleFired = TRUE;
					}
					else
					{
						RuleFired = FALSE;
					}

				} /** We have a criteria program to run */
				else
				{
					/* No command -- RULE FIRES */
					RuleFired = TRUE;
				}

			} /* we have matching magnitude and auto flag */
			else
			{
				RuleFired = FALSE;
			}

			if (RuleFired == TRUE)
			{

				/* 
				 * If the rule fired we add it to the alarms list. 
				 * However, we want to check for duplicates first.
				 */

				tmpind = RecipientRules.Rule[j].DeliveryIndex;

				/* See if this alarm is already in the list */
				found = FALSE;
				for (k = 0; ((k < *NumAlarmsRetrieved) && (found == FALSE)); k++)
				{

					if ((pAlarms[k].Audit.idEvent == pEvtInfo->Event.idEvent) && 
							(pAlarms[k].Audit.Format.idFormat == 
										RecipientRules.Rule[j].Format.idFormat) &&
							(pAlarms[k].Delivery.idRecipientDelivery == 
									RecipientRules.Delivery[tmpind].idRecipientDelivery))
					{
						found = TRUE;
					}
				}

				if ((*NumAlarmsFound < AlarmListSize) && (found == FALSE))
				{

					/*
					 * If this is a QDDS alarm we want to run some checks to 
					 * make sure that it is a "real" event suitable for 
					 * being posted to the Recent Quakes pages.
					 */

					result = TRUE;
					if (RecipientRules.Delivery[tmpind].DelMethodInd ==
													EWDB_ALARMS_DELIVERY_IND_QDDS)
					{
						if (ValidateQdds == 1)
						{

							if (QddsValidateEvent (pEvtInfo, &result) != EW_SUCCESS)
							{
								logit ("", "Call to QddsValidateEvent failed.\n");
								*retVal = -110;
								return EW_FAILURE;
							}
						}
					}

					if (result == TRUE)
					{

						pAlarms[*NumAlarmsFound].Audit.idEvent = pEvtInfo->Event.idEvent;
						pAlarms[*NumAlarmsFound].Audit.bAuto = pDAP->isAuto;

						memcpy (&pAlarms[*NumAlarmsFound].Audit.Recipient, &RecipientRules.Recipient,
									sizeof (EWDB_AlarmsRecipientStruct));
	
						memcpy (&pAlarms[*NumAlarmsFound].Audit.Format, 
									&RecipientRules.Rule[j].Format,
									sizeof (EWDB_AlarmsFormatStruct));
	
						memcpy (&pAlarms[*NumAlarmsFound].Delivery, 
													&RecipientRules.Delivery[tmpind],
									sizeof (EWDB_AlarmDeliveryUnionStruct));
	
						BMS.AlarmMsg = pAlarms[*NumAlarmsFound].AlarmMsg;
						BMS.networkCode = pDAP->networkCode;
						BMS.InvString = pDAP->InvocationString;
						BMS.pFormat =  &pAlarms[*NumAlarmsFound].Audit.Format;
						BMS.pEvent =  pEvtInfo;
						BMS.isAuto =  pDAP->isAuto;
						BMS.isInsert =  pDAP->isInsert;
						BMS.BigCityPop =  pDAP->minPopulation;
						BMS.ShowPopulation =  pDAP->showPopulation;

						if (BuildAlarmMessage (&BMS) != EW_SUCCESS)
						{
							logit ("", "Call to BuildAlarmMessage failed.\n");
							*retVal = -103;
							return EW_FAILURE;
						}
	
						*NumAlarmsRetrieved = *NumAlarmsRetrieved + 1;
					}
				}


				if (found == FALSE)
					*NumAlarmsFound = *NumAlarmsFound + 1;

			} /** rule fired -- move it to the AlarmsList **/

		} /** j-for loop over rules for one recipient **/

	} /** i-for loop over recipients **/

	/* Remove the event file */
  if(fullevtfile[0] != 0x00)
  {
	  sprintf (cmd, "%s %s", SYSTEM_DELETE_COMMAND, fullevtfile);
	  system (cmd);
  }


	/* 
	 * See if any alarms for this event have been issued previously.
	 * If so, add them to the list -- this is necessary for the 
	 * situation where an event's parameters are reviewed so that 
	 * they no longer fit a recipient's rules. But, the recipient should be 
	 * notified that this event has been reviewed
	 */

	if (*NumAlarmsRetrieved < AlarmListSize)
	{
		/* Allocate audit list buffer */
		if ((pAudit = (EWDB_AlarmAuditStruct *) malloc (INIT_BUF_SIZE *
									sizeof (EWDB_AlarmAuditStruct))) == NULL)
		{
			html_logit ("", "Could not malloc Audit list buffer.\n");
			*retVal = -101;
			free (pRecipientList);
			return EW_FAILURE;
		}

		/* Allocate delivery list buffer */
		if ((pDelivery = (EWDB_AlarmDeliveryUnionStruct *) malloc (INIT_BUF_SIZE *
									sizeof (EWDB_AlarmDeliveryUnionStruct))) == NULL)
		{
			html_logit ("", "Could not malloc Delivery list buffer.\n");
			*retVal = -101;
			free (pRecipientList);
			free (pAudit);
			return EW_FAILURE;
		}

		/* Retrieve list from the db */
		if (EWDB_GetAudits (pEvtInfo->Event.idEvent, pAudit, 1, pDelivery, &NumFound,
					&NumRetr, INIT_BUF_SIZE) != EWDB_RETURN_SUCCESS)
		{
			html_logit ("", "Call to EWDB_GetAudits failed.\n");
			*retVal = -102;
			free (pRecipientList);
			free (pAudit);
			free (pDelivery);
			return EW_FAILURE;
		}
	
		if (NumRetr < NumFound)
		{
			logit ("", "Buffers too small, found %d, retrieved %d\n",
							NumFound, NumRetr);
			free (pAudit);
			free (pDelivery);
	
			/* Allocated audit list buffer */
			if ((pAudit = (EWDB_AlarmAuditStruct *) malloc (NumFound *
										sizeof (EWDB_AlarmAuditStruct))) == NULL)
			{
				html_logit ("", "Could not malloc Audit list buffer.\n");
				*retVal = -101;
				free (pRecipientList);
				return EW_FAILURE;
			}

			/* Allocate delivery list buffer */
			if ((pDelivery = (EWDB_AlarmDeliveryUnionStruct *) malloc (NumFound *
										sizeof (EWDB_AlarmDeliveryUnionStruct))) == NULL)
			{
				html_logit ("", "Could not malloc Delivery list buffer.\n");
				*retVal = -101;
				free (pRecipientList);
				free (pAudit);
				return EW_FAILURE;
			}
		
			/* Retrieve list from the db */
			if (EWDB_GetAudits (pEvtInfo->Event.idEvent, pAudit, 1, pDelivery, &NumFound,
						&NumRetr, NumFound) != EWDB_RETURN_SUCCESS)
			{
				html_logit ("", "Call to EWDB_GetAudits failed.\n");
				*retVal = -102;
				free (pRecipientList);
				free (pAudit);
				free (pDelivery);
				return EW_FAILURE;
			}
		}

		for (i = 0; i < NumRetr; i++)
		{
			/* see if already have this alarm in the list */
			found = FALSE;
			for (j = 0; ((j < *NumAlarmsRetrieved) && (found == FALSE)); j++)
			{
				if ((pAudit[i].idEvent == pEvtInfo->Event.idEvent) && 
						(pAudit[i].Format.idFormat == 
									pAlarms[j].Audit.Format.idFormat) &&
						(pAudit[i].Recipient.idRecipient == pAlarms[j].Audit.Recipient.idRecipient))
				{
					/* Check delivery method */
					if (pAudit[i].DelMethodInd == EWDB_ALARMS_DELIVERY_IND_EMAIL)
					{
						if (strcmp (pDelivery[i].email.sAddress, 
								pAlarms[j].Delivery.email.sAddress) == 0)
						{
							found = TRUE;
						}
					}
					else if (pAudit[i].DelMethodInd == EWDB_ALARMS_DELIVERY_IND_PAGER)
					{
						if (strcmp (pDelivery[i].pager.sNumber, 
								pAlarms[j].Delivery.pager.sNumber) == 0)
						{
							found = TRUE;
						}
					}
					else if (pAudit[i].DelMethodInd == EWDB_ALARMS_DELIVERY_IND_PHONE)
					{
						if (strcmp (pDelivery[i].phone.sPhoneNumber, 
								pAlarms[j].Delivery.phone.sPhoneNumber) == 0)
						{
							found = TRUE;
						}
					}
					else if (pAudit[i].DelMethodInd == EWDB_ALARMS_DELIVERY_IND_QDDS)
					{
						if (strcmp (pDelivery[i].qdds.sQddsDirectory, 
								pAlarms[j].Delivery.qdds.sQddsDirectory) == 0)
						{
							found = TRUE;
						}
					}
					else if (pAudit[i].DelMethodInd == EWDB_ALARMS_DELIVERY_IND_CUSTOM)
					{
						if (strcmp (pDelivery[i].custom.sDescription, 
								pAlarms[j].Delivery.custom.sDescription) == 0)
						{
							found = TRUE;
						}
					}
					else
					{
						logit ("", "Unkown delivery method: %d. Continue.\n",
												pAudit[i].DelMethodInd);
					}
				}

			} /* looking for the alarm in the audit list */

			if ((*NumAlarmsRetrieved < AlarmListSize) && (found == FALSE))
			{

				/* Insert new */
				memcpy (&pAlarms[*NumAlarmsRetrieved].Audit, &pAudit[i],
							sizeof (EWDB_AlarmAuditStruct));

				memcpy (&pAlarms[*NumAlarmsRetrieved].Delivery, &pDelivery[i],
							sizeof (EWDB_AlarmDeliveryUnionStruct));

				pAlarms[*NumAlarmsRetrieved].Audit.bAuto = pDAP->isAuto;


				/* 
				 * set idAudit to NULL -- otherwise, an existing 
				 * audit will be updated.
				 */

				pAlarms[*NumAlarmsRetrieved].Audit.idAudit = NEW_ENTRY_FLAG;

				BMS.AlarmMsg = pAlarms[*NumAlarmsRetrieved].AlarmMsg;
				BMS.pFormat =  &pAlarms[*NumAlarmsRetrieved].Audit.Format;
				BMS.pEvent =  pEvtInfo;
				BMS.isAuto =  pDAP->isAuto;
				BMS.isInsert =  pDAP->isInsert;
				BMS.networkCode = pDAP->networkCode;
				BMS.InvString = pDAP->InvocationString;
				BMS.BigCityPop =  pDAP->minPopulation;
				BMS.ShowPopulation =  pDAP->showPopulation;

				if (BuildAlarmMessage (&BMS)  != EW_SUCCESS)
				{
					logit ("", "Call to BuildAlarmMessage failed.\n");
					*retVal = -103;
					free (pRecipientList);
					free (pAudit);
					free (pDelivery);
					return EW_FAILURE;
				}

				*NumAlarmsRetrieved = *NumAlarmsRetrieved + 1;
				*NumAlarmsFound = *NumAlarmsFound + 1;

			} /* Not found */

			else if (found == FALSE)
			{
				/* alarm found, but cannot be retrieved */
				*NumAlarmsFound = *NumAlarmsFound + 1;
			}
		}

	} /* If we have room for more alarms */

	free (pRecipientList);
	free (pAudit);
	free (pDelivery);


	return EW_SUCCESS;
}



/*************************** BuildAlarmMessage ***************************/
static 	int		BuildAlarmMessage (BuildMsgStruct *pBMS)
{

	char					tmpstr[256];
	char					token[256];
	char					tzone[256];
	char					envcmd[256];
	char					placeName[256];
	char					bearing[256];
	char					townstr[256];
	int						done, iter, i, j, bigIndex, GotBig;
	char					*fmtptr; 
	char					*tmp;
	char					*tmpMsg;
	char					*tmp1;
	char					*tmp2;
	int						numTotal, numBig, numFound, numRetr; 
	int						PlBufSize, BigPlBufSize;
	double					milesDist;
	EWDB_AlarmsFormatStruct	*pFormat;
	EWEventInfoStruct		*pEvtInfo;
	char					*pMsg;
	EWDB_PlaceStruct		*pPlaces;
	EWDB_PlaceStruct		*pBigPlaces;
	EWDB_PlaceStruct		Min;
	EWDB_PlaceStruct		Max;
	


	if (pBMS == NULL)
	{
		logit ("", "Invalid arguments passed in.\n");
		return EW_FAILURE;
	}

	if ((pMsg = pBMS->AlarmMsg) == NULL)
	{
		logit ("", "Invalid alarm message passed in.\n");
		return EW_FAILURE;
	}

	if ((pFormat = pBMS->pFormat) == NULL)
	{
		logit ("", "Invalid format pointer passed in.\n");
		return EW_FAILURE;
	}

	if ((pEvtInfo = pBMS->pEvent) == NULL)
	{
		logit ("", "Invalid event pointer passed in.\n");
		return EW_FAILURE;
	}

	/* If format is CUBE invoke the automatic message builder */
	if (strcmp (pFormat->sDescription, "CUBE") == 0)
	{
		/* Ignore any user-defined format -- use CUBE */
		return (BuildCUBEMessage (pMsg, pEvtInfo, pBMS->isAuto, 
										pBMS->isInsert, pBMS->networkCode));
	}

	/* If format is hypoTWC invoke the automatic message builder. Alex 11/07/02 */
	if (strcmp (pFormat->sDescription, "hypoTWC") == 0)
	{
		/* Ignore any user-defined format -- use hypoTWC */
		return (BuildhypoTWCMessage (pMsg, pEvtInfo, pBMS->isAuto, 
										pBMS->isInsert, pBMS->networkCode));
	}

	if (pBMS->isInsert == TRUE)
	{
		fmtptr = &pFormat->sFmtInsert[0];
	}
	else	
	{
		fmtptr = &pFormat->sFmtDelete[0];
	}


	done = FALSE;
	tmpMsg = pMsg; 
	while (done == FALSE)
	{
		while (*fmtptr != FMT_CHAR)
		{
			*tmpMsg = *fmtptr;
			tmpMsg = tmpMsg + 1;
			fmtptr = fmtptr + 1;
		}

		/* Got a token -- see what it is */
		fmtptr = fmtptr + 1;
		if ((tmp = strchr (fmtptr, FMT_CHAR)) == NULL)
		{
			logit ("", "Invalid format.\n");
			return EW_FAILURE;
		}

		*tmp = '\0';
		strcpy (token, fmtptr);
		*tmp = FMT_CHAR;

		if (strcmp (token, "End") == 0)
		{
			done = TRUE;
			tmpMsg = tmpMsg + 1;
			*tmpMsg = '\0';
		}
		else
		{
			fmtptr = tmp + 1;

			if (strcmp (token, "EventID") == 0)
			{
				sprintf (tmpstr, "%d", pEvtInfo->Event.idEvent);
			}
			else if (strcmp (token, "RevAuto") == 0)
			{
				if (pBMS->isAuto == TRUE)
					sprintf (tmpstr, "Automatic");
				else
					sprintf (tmpstr, "Reviewed");
			}
			else if (strcmp (token, "PrefMag") == 0)
			{
				sprintf (tmpstr, "%0.1f", pEvtInfo->Mags[pEvtInfo->iPrefMag].dMagAvg);
			}
			else if (strcmp (token, "Md") == 0)
			{
				if (pEvtInfo->iMd >= 0)
					sprintf (tmpstr, "%0.1f", pEvtInfo->Mags[pEvtInfo->iMd].dMagAvg);
				else
					sprintf (tmpstr, "Not available");
			}
			else if (strcmp (token, "ML") == 0)
			{
				if (pEvtInfo->iML >= 0)
					sprintf (tmpstr, "%0.1f", pEvtInfo->Mags[pEvtInfo->iML].dMagAvg);
				else
					sprintf (tmpstr, "Not available");
			}
			else if (strncmp (token, "Otime", 5) == 0)
			{
				/* Extract the timezone string */
				if (token[5] == '@')
				{
					strcpy (tzone, (token + 6));
					sprintf (envcmd, "TZ=%s", tzone);
					putenv (envcmd);
				}

				datestr23_local (pEvtInfo->PrefOrigin.tOrigin, tmpstr, 256);

				strcat (tmpstr, " ");

				/* add string specifying timezone */
				if (token[5] == '@')
					strcat (tmpstr, tzone);
				else
					strcat (tmpstr, "Local Time");
			}
			else if (strcmp (token, "Lat") == 0)
			{
				sprintf (tmpstr, "%0.4f", pEvtInfo->PrefOrigin.dLat);
			}
			else if (strcmp (token, "Lon") == 0)
			{
				sprintf (tmpstr, "%0.4f", pEvtInfo->PrefOrigin.dLon);

			}
			else if (strcmp (token, "Depth") == 0)
			{
				sprintf (tmpstr, "%0.1f", pEvtInfo->PrefOrigin.dDepth);
			}
			else if (strcmp (token, "Nph") == 0)
			{
				sprintf (tmpstr, "%d", pEvtInfo->PrefOrigin.iUsedPh);
			}
			else if (strcmp (token, "Dmin") == 0)
			{
				sprintf (tmpstr, "%0.1f", pEvtInfo->PrefOrigin.dDmin);
			}
			else if (strcmp (token, "Rms") == 0)
			{
				sprintf (tmpstr, "%0.2f", pEvtInfo->PrefOrigin.dRms);
			}
			else if (strcmp (token, "ErrLat") == 0)
			{
				sprintf (tmpstr, "%0.1f", pEvtInfo->PrefOrigin.dErrLat);
			}
			else if (strcmp (token, "ErrLon") == 0)
			{
				sprintf (tmpstr, "%0.1f", pEvtInfo->PrefOrigin.dErrLon);
			}
			else if (strcmp (token, "ErrZ") == 0)
			{
				sprintf (tmpstr, "%0.1f", pEvtInfo->PrefOrigin.dErZ);
			}
			else if (strcmp (token, "Gap") == 0)
			{
				sprintf (tmpstr, "%d", pEvtInfo->PrefOrigin.iGap);
			}
			else if (strcmp (token, "InvokedBy") == 0)
			{
				strcpy (tmpstr, pBMS->InvString);
			}
			else if (strncmp (token, "Towns", 5) == 0)
			{
				/* 
				 * Token format:  Towns@4,1  means
				 * display four nearest towns with one of them
				 * being BIG (having population defined in the config file)
				 */
				
				/* Extract the options */
				if (token[5] != '@')
				{
					logit ("", "Token Towns requires options.\n");
					return EW_FAILURE;
				}

				tmp1 = token+6;
				if ((tmp2 = strchr (tmp1, ',')) == NULL)
				{
					logit ("", "Invalid token Towns format.\n");
					return EW_FAILURE;
				}
				*tmp2 = '\0';

				numTotal = atoi (tmp1);
				tmp1 = tmp2 + 1;
				numBig = atoi (tmp1);


				/* 
				 * Retrieve the list of PLACES from the database:
				 * 
				 *   Story:  We want to get numTotal number of towns,
				 *    of which numBig are "Big Places" having population 
				 *    over some limit set in the configuration file.
				 *
				 *   We start with the initial lat,lon box around the 
				 *   origin and continue retrieving places in an expanding
				 *   box until we have at numTotal places. 
				 * 
				 *   Then, we repeat the retrieval, but this time fetching
				 *   only "Big Places".  We add the nearest Big Place to the
				 *   list of places, sort everything by distance and print out. 
				 */

				CompOriginLat = pEvtInfo->PrefOrigin.dLat;
				CompOriginLon = pEvtInfo->PrefOrigin.dLon;

				PlBufSize = INIT_NUM_PLACES;
				if ((pPlaces = (EWDB_PlaceStruct *) malloc (PlBufSize *
						sizeof (EWDB_PlaceStruct))) == NULL)
				{
					logit ("", "Could not malloc Place array of %d elements\n", PlBufSize);
					return EW_FAILURE;
				}

				memset (&Min, 0, sizeof (EWDB_PlaceStruct));
				memset (&Max, 0, sizeof (EWDB_PlaceStruct));

				/* Ignore these criteria */
				Min.iPopulation = 1;
				Max.iPopulation = -1;
				Min.iPlaceMajorType = -1;
				Min.iPlaceMinorType = -1;


				numFound = numRetr = 0;
				iter = 1;

				while ((numRetr < numTotal) && (iter < MAX_PLACE_ITER))
				{
					/* location box */
					Min.dLat = pEvtInfo->PrefOrigin.dLat - (iter*INIT_LAT_RANGE);
					Max.dLat = pEvtInfo->PrefOrigin.dLat + (iter*INIT_LAT_RANGE);

					Min.dLon = pEvtInfo->PrefOrigin.dLon - (iter*INIT_LON_RANGE);
					Max.dLon = pEvtInfo->PrefOrigin.dLon + (iter*INIT_LON_RANGE);

					if (ewdb_api_GetPlaceList (pPlaces, PlBufSize, 
							&Min, &Max, &numRetr, &numFound) != EWDB_RETURN_SUCCESS)
					{
					    logit ("", "Call to ewdb_api_GetPlaceList failed.n");
					    return EW_FAILURE;
					}

					if (numRetr < numFound)
					{
						free (pPlaces);

						PlBufSize = numFound;
						if ((pPlaces = (EWDB_PlaceStruct *) malloc (PlBufSize *
										sizeof (EWDB_PlaceStruct))) == NULL)
						{
							logit ("", "Could not malloc Place array of %d elements\n", PlBufSize);
							return EW_FAILURE;
						}

						if (ewdb_api_GetPlaceList (pPlaces, PlBufSize, 
							&Min, &Max, &numRetr, &numFound) != EWDB_RETURN_SUCCESS)
						{
						    logit ("", "Call to ewdb_api_GetPlaceList failed.n");
						    return EW_FAILURE;
						}
					}

					iter = iter + 1;

				}


				/* Sort the list by distance */
				qsort (pPlaces, numRetr, sizeof (EWDB_PlaceStruct), 
													SortPlacesByDistance);

				if (numRetr < numTotal)
				{
					logit ("", "Insufficient number of places in the DB.\n");
					return EW_FAILURE;
				}
				
				/* Do we have a big place in the list already? */

				GotBig = 0;
				i = 0;
				while  ((i < numRetr) && (GotBig < numBig))
				{
					if (pPlaces[i].iPopulation >= pBMS->BigCityPop)
					{
						/* If big place is already among the first numTotal
						 * places, we are done;  Otherwise ,replace the last
						 * small place with the big place
						 */
	
						bigIndex = numTotal - numBig + GotBig;
						if (i > bigIndex)
						{
							memcpy (&pPlaces[bigIndex], &pPlaces[i],
									sizeof (EWDB_PlaceStruct)); 
						}
						else
						{
							/* bigplace already in place - do nothing */
						}

						GotBig = GotBig + 1;
					}

					i = i + 1;
				}

				if (GotBig < numBig)
				{
					/* No big places were in the list we got - let's get some */

					BigPlBufSize = INIT_NUM_PLACES;
					if ((pBigPlaces = (EWDB_PlaceStruct *) malloc (BigPlBufSize *
							sizeof (EWDB_PlaceStruct))) == NULL)
					{
						logit ("", "Could not malloc Place array of %d elements\n", 
														BigPlBufSize);
						return EW_FAILURE;
					}


					Min.iPopulation = pBMS->BigCityPop;

					numFound = numRetr = 0;
					iter = 1;

					while ((numRetr < numBig) && (iter < MAX_PLACE_ITER))
					{
						/* location box */
						Min.dLat = pEvtInfo->PrefOrigin.dLat - (iter*INIT_LAT_RANGE);
						Max.dLat = pEvtInfo->PrefOrigin.dLat + (iter*INIT_LAT_RANGE);
	
						Min.dLon = pEvtInfo->PrefOrigin.dLon - (iter*INIT_LON_RANGE);
						Max.dLon = pEvtInfo->PrefOrigin.dLon + (iter*INIT_LON_RANGE);

						if (ewdb_api_GetPlaceList (pBigPlaces, BigPlBufSize, 
								&Min, &Max, &numRetr, &numFound) != EWDB_RETURN_SUCCESS)
						{
						    logit ("", "Call to ewdb_api_GetPlaceList failed.n");
						    return EW_FAILURE;
						}
	
	
						if (numRetr < numFound)
						{
							free (pBigPlaces);
	
							BigPlBufSize = numFound;
							if ((pBigPlaces = (EWDB_PlaceStruct *) malloc (BigPlBufSize *
											sizeof (EWDB_PlaceStruct))) == NULL)
							{
								logit ("", "Could not malloc Place array of %d elements\n", BigPlBufSize);
								return EW_FAILURE;
							}
	
							if (ewdb_api_GetPlaceList (pBigPlaces, BigPlBufSize, 
								&Min, &Max, &numRetr, &numFound) != EWDB_RETURN_SUCCESS)
							{
							    logit ("", "Call to ewdb_api_GetPlaceList failed.n");
							    return EW_FAILURE;
							}
						}

						iter = iter + 1;

					} /* while not having enough big places */
			
					/* Sort the list by distance */
					qsort (pBigPlaces, numRetr, sizeof (EWDB_PlaceStruct), 
													SortPlacesByDistance);



					/* 
					 * if we have less than the desired number of big places, 
					 * we'll still return numTotal places, but there will be
					 * than many more small places in the list.
					 */
					if (numRetr < numBig)
						numBig = numRetr;

					/* Copy over the needed number of closest big places */
					i = GotBig;
					while (GotBig < numBig)
					{
						memcpy (&pPlaces[numTotal-(numBig-GotBig)], &pBigPlaces[i],
								sizeof (EWDB_PlaceStruct)); 


						GotBig = GotBig + 1;
						i = i + 1;

					}

					free (pBigPlaces);

				} /* if no big places retrieved */

				/* Sort it all one last time */
				qsort (pPlaces, numTotal, sizeof (EWDB_PlaceStruct), 
													SortPlacesByDistance);


				/* Build the alarm string */

				sprintf (tmpstr, "");
				for (i = 0; i < numTotal; i++)
				{
					if (pPlaces[i].iPopulation > pBMS->BigCityPop)
					{
						for (j = 0; j < (int)strlen (pPlaces[i].szPlaceName); j++)
							placeName[j] = toupper (pPlaces[i].szPlaceName[j]);

						placeName[j] = '\0';
					}
					else	
						strcpy (placeName, pPlaces[i].szPlaceName);


					/* figure out the azimuth */
					ComputeBearingFromAzm (pPlaces[i].dAzm, bearing);

					milesDist = pPlaces[i].dDist * KM_TO_MILE;

					sprintf (townstr, "%s, %s - %0.0f km (%0.0f miles) %s",
						placeName, pPlaces[i].szState, pPlaces[i].dDist, milesDist, bearing);

					strcat (tmpstr, townstr);

					if (pBMS->ShowPopulation != FALSE)
						sprintf (townstr, " (Pop: %d)\n", pPlaces[i].iPopulation);
					else
						sprintf (townstr, "\n");
						
					strcat (tmpstr, townstr);

				}
				free (pPlaces);
			}
			else
			{
				logit ("", "Unknown token: <%s>\n", token);
				return EW_FAILURE;
			}

			sprintf (tmpMsg, "%s", tmpstr);
			tmpMsg = tmpMsg + strlen (tmpstr);
		}
	}
		

	return EW_SUCCESS;

}

/**********************************************************************/
static int CompareRecipientPriorities (const void *arg1, const void *arg2)
{

    int retval;
    EWDB_AlarmsRecipientStruct  *par1 = (EWDB_AlarmsRecipientStruct *) arg1;
    EWDB_AlarmsRecipientStruct  *par2 = (EWDB_AlarmsRecipientStruct *) arg2;

    retval = par1->dPriority - par2->dPriority;

    if (retval < 0)
        return (-1);
    else if (retval == 0)
       return (0);
    else
        return (1);
}

/**********************************************************************/
static int SortPlacesByDistance (const void *arg1, const void *arg2)
{

    int 	retval;
    EWDB_PlaceStruct  *par1 = (EWDB_PlaceStruct *) arg1;
    EWDB_PlaceStruct  *par2 = (EWDB_PlaceStruct *) arg2;


	
	if (geo_to_km (CompOriginLat, CompOriginLon,
				par1->dLat, par1->dLon, &par1->dDist, &par1->dAzm) != 1)
	{
		logit ("", "Call to geo_to_km failed\n");
		return 0;
	}

	if (geo_to_km (CompOriginLat, CompOriginLon,
				par2->dLat, par2->dLon, &par2->dDist, &par2->dAzm) != 1)
	{
		logit ("", "Call to geo_to_km failed\n");
		return 0;
	}


    retval = (int)(par1->dDist - par2->dDist);

    if (retval < 0)
        return (-1);
    else if (retval == 0)
       return (0);
    else
        return (1);
}





/*************************** BuildCUBEMessage ***************************/
int		BuildCUBEMessage (char *pMsg, EWEventInfoStruct *pEvtInfo, 
						int isAuto, int isInsert, char *networkCode)
{
	struct tm 	*tstruct;
	double	sec;
	char	CubeMsg[128];	/* Make this big; we often get a large message before we truncate
							 * back to 80 chars.
							 */
	char	tmp[64];
	time_t	otime;
	int		version;		/* Get from database */
	int		len;
	char	evtID[12];
	char	magtype, LocMethod;

	if ((pMsg == NULL) || (pEvtInfo == NULL))
	{
		logit ("", "Invalid arguments passed in.\n");
		return EW_FAILURE;
	}

	/* Manufacture an 8 character EventID from our 10 chars - take last 8 */
	sprintf (tmp, "%d", pEvtInfo->Event.idEvent);

	len = strlen (tmp);
	if (len <= 8)
		strcpy (evtID, tmp);
	else
	{
		strncpy (evtID, (tmp+(len-8)), 8);
		evtID[8] = '\0';
	}


	if (isInsert == FALSE)
	{
		/* Do deletion message first -- it is easier */

		sprintf (pMsg, "DE%8s%2s  EVENT DELETED", evtID, networkCode);

	}
	else
	{

		/* 
		 * Retrieve the version number --
		 *  Story: we keep the mapping between version numbers 
		 *  and event IDs in the database.  Get the current
		 *  value and increment it.
		 */
		if (ewdb_api_GetAndIncrementCubeVersion (pEvtInfo->Event.idEvent, &version) 
											!= EWDB_RETURN_SUCCESS)
		{
			logit ("", "Call to ewdb_api_GetAndIncrementCubeVersion failed.\n");
			return EW_FAILURE;
		}

		if (version > 9)
		{
			logit ("", "Next CUBE version is %d, setting to 9\n", version);
			version = 9;
		}

		/* Start us off on the right track */
		sprintf (CubeMsg, "E %8s%2s%d", evtID, networkCode, version);


		/* format event time */
		otime = (time_t) pEvtInfo->PrefOrigin.tOrigin;
		tstruct = gmtime (&otime);

		/* seconds * 10 */
		sec = 10.0 * ((double) pEvtInfo->PrefOrigin.tOrigin - 
								(int) pEvtInfo->PrefOrigin.tOrigin);
		len = 10 * tstruct->tm_sec + (int) sec;

		sprintf (tmp, "%04d%02d%02d%02d%02d%03d", 
							tstruct->tm_year + 1900,
							tstruct->tm_mon + 1,
							tstruct->tm_mday,
							tstruct->tm_hour,
							tstruct->tm_min, len);

		strcat (CubeMsg, tmp);


		/* lat, lon, depth */
		sprintf (tmp, "%7d%8d%4d", 
					(int) (10000 * pEvtInfo->PrefOrigin.dLat),
					(int) (10000 * pEvtInfo->PrefOrigin.dLon),
					(int) (10 * pEvtInfo->PrefOrigin.dDepth));
		

		strcat (CubeMsg, tmp);


		/* mag, number of stations and phases, distance */
		sprintf (tmp, "%2d%3d%3d%4d", 
					(int) (10 * pEvtInfo->Mags[pEvtInfo->iPrefMag].dMagAvg),
					pEvtInfo->PrefOrigin.iUsedPh, 
					pEvtInfo->PrefOrigin.iUsedPh,
					(int) (10 * pEvtInfo->PrefOrigin.dDmin));

		strcat (CubeMsg, tmp);


		/* rmss, horiz and vert error, gap*/
		sprintf (tmp, "%4d%4d%4d%2d", 
					(int) (100 * pEvtInfo->PrefOrigin.dRms),
					(int) (10 * pEvtInfo->PrefOrigin.dErrLat), 
					(int) (10 * pEvtInfo->PrefOrigin.dErZ), 
					(int) ((double)pEvtInfo->PrefOrigin.iGap/3.6));

		strcat (CubeMsg, tmp);


		/* magnitude information */

		if ((pEvtInfo->Mags[pEvtInfo->iPrefMag].iMagType == MAGTYPE_LOCAL_PEAK2PEAK) || 
		    (pEvtInfo->Mags[pEvtInfo->iPrefMag].iMagType == MAGTYPE_LOCAL_ZERO2PEAK))
			magtype = 'L';
		else if ((pEvtInfo->Mags[pEvtInfo->iPrefMag].iMagType == MAGTYPE_MOMENT) ||
				 (pEvtInfo->Mags[pEvtInfo->iPrefMag].iMagType == MAGTYPE_MWP))
			magtype = 'O';
		else if ((pEvtInfo->Mags[pEvtInfo->iPrefMag].iMagType == MAGTYPE_BODYWAVE) ||
				(pEvtInfo->Mags[pEvtInfo->iPrefMag].iMagType == MAGTYPE_MBLG))
			magtype = 'B';
		else if (pEvtInfo->Mags[pEvtInfo->iPrefMag].iMagType == MAGTYPE_SURFACEWAVE)
			magtype = 'S';
		else if (pEvtInfo->Mags[pEvtInfo->iPrefMag].iMagType == MAGTYPE_DURATION)
			magtype = 'D';
		else
			magtype = '?';


		/* 
	  	 * Location method:  this is somewhat of a cludge.
		 * For now, we hardcode 'L' for automatic event and
		 * 'l' for reviewed.  
		 *   There actually exists a blessed table for this field
		 * so it should be configurable.  Perhaps later...
		 *     LV 1/8/2003
	  	 */

		if (isAuto == TRUE)
			LocMethod = 'L';
		else
			LocMethod = 'l';
		
		sprintf (tmp, "%c%2d%2d%c", 
					magtype,
					pEvtInfo->Mags[pEvtInfo->iPrefMag].iNumMags,
					(int) (10 * pEvtInfo->Mags[pEvtInfo->iPrefMag].dMagErr),
					LocMethod);

		strcat (CubeMsg, tmp);

		/* Compute MenloPark check character */
		CubeMsg[79] = '\0';
		len = GetMenloCheckChar (CubeMsg);

		CubeMsg[79] = (char) len;
		CubeMsg[80] = '\0';

		strcpy (pMsg, CubeMsg);
	}


	return EW_SUCCESS;

}



static int		GetMenloCheckChar (char *pch)
{
	unsigned short sum;

	for (sum = 0; *pch; pch++)
		sum = ((sum&01)?0x8000:0) + (sum>>1) + *pch;

	return (int) (36 + sum%91);
}
/*************************************************************************/

/*************************** BuildhypoTWCMessage ***************************/
/* We got the following from \loc_wcatwc\locate.c:
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

HYPOTWC SUMMARY FORMAT.
Type     Description
----     -----------
int      Quake ID
int      Version
double   Origin time (1/1/70 seconds)
double   Latitude (geocentric)
double   Longitude (geocentric)
int      Depth (km)
int      # P times used in location
int      Station Azimuthal coverage around epicenter
double   RMS travel time residual (absolute average)
int      Quality: 0=poor, 1=fair, 2=good, 3=very good (based on residual)
double   Preferred magnitude (modified average)
char     Type of preferred magnitude (b, l, S, w)
int      # stations used in preferred magnitude
double   MS magnitude (modified average)
int      # stations used in Ms
double   Mwp magnitude (modified average)
int      # stations used in Mwp
double   Mb magnitude (modified average)
int      # stations used in Mb
double   Ml magnitude (modified average)
int      # stations used in Ml


/* Fill message */
/*
   sprintf( pszMsg, "%ld %ld %lf %lf %lf %lf %ld %ld %lf %ld %lf %s %ld %lf "
                    "%ld %lf %ld %lf %ld %lf %ld   ",
    pHypo->iQuakeID, pHypo->iVersion, 
    pHypo->dOriginTime, pHypo->dLat, pHypo->dLon,
    pHypo->dDepth, pHypo->iNumPs, pHypo->iAzm,
    pHypo->dAvgRes, pHypo->iGoodSoln, 
	pHypo->dPreferredMag, pHypo->szPMagType, pHypo->iNumPMags,
    pHypo->dMSAvg, pHypo->iNumMS, pHypo->dMwpAvg, pHypo->iNumMwp, 
    pHypo->dMbAvg, pHypo->iNumMb, pHypo->dMlAvg, pHypo->iNumMl );
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
*/   
int		BuildhypoTWCMessage (char *pMsg, EWEventInfoStruct *pEvtInfo,  
									int isAuto, int isInsert, char *networkCode)
{

	char	hypoTWCMessage[384];
	char	tmp[64];
	char	magtype;

	if ((pMsg == NULL) || (pEvtInfo == NULL))
	{
		logit ("", "Invalid arguments passed in.\n");
		return EW_FAILURE;
	}


	if (isInsert == FALSE)
	{
		/* hypoTWC doesn't believe in deletions as of 11/08/02 */
		return EW_SUCCESS;
	}
	else
	{

		hypoTWCMessage[0]='0'; /* clear the assembly area */

		/* Use the glass event id */
		sprintf (tmp, "%d ", pEvtInfo->Event.idEvent);
		/* Use a strcpy for the first one, or else Weirdness Ensues... */
		strcpy(hypoTWCMessage,tmp);
		logit("","1BuildhypoTWCMessage: .%s%\n",hypoTWCMessage);

		/* Use the glass version number */
		sprintf(tmp, "%d ", pEvtInfo->PrefOrigin.iVersionNum);
		strcat(hypoTWCMessage,tmp);

		/* Event time */
		sprintf(tmp, "%lf ", pEvtInfo->PrefOrigin.tOrigin);
		strcat(hypoTWCMessage,tmp);


		/* lat, lon, depth */
		sprintf (tmp, "%lf %lf %4d ", 
					   pEvtInfo->PrefOrigin.dLat,
					       pEvtInfo->PrefOrigin.dLon,
					           (int) (pEvtInfo->PrefOrigin.dDepth));
		strcat (hypoTWCMessage, tmp);

		/* int   # P times used in location */
		sprintf(tmp, "%4d ", pEvtInfo->PrefOrigin.iUsedPh);
		strcat (hypoTWCMessage, tmp);

		/* int   Station Azimuthal coverage around epicenter */
		sprintf(tmp, "%4d ",360 - pEvtInfo->PrefOrigin.iGap);
		strcat (hypoTWCMessage, tmp);

		/* double   RMS travel time residual (absolute average) */
		sprintf(tmp, "%lf ",360 - pEvtInfo->PrefOrigin.dRms);
		strcat (hypoTWCMessage, tmp);

		/* int   Quality: 0=poor, 1=fair, 2=good, 3=very good (based on residual) */
		sprintf(tmp, "%2d ", 3); /* kludge kludge kludge */
		strcat (hypoTWCMessage, tmp);

		/* char  Type of preferred magnitude (b, l, S, w) */
		if ((pEvtInfo->Mags[pEvtInfo->iPrefMag].iMagType == MAGTYPE_LOCAL_PEAK2PEAK) || 
		    (pEvtInfo->Mags[pEvtInfo->iPrefMag].iMagType == MAGTYPE_LOCAL_ZERO2PEAK))
			magtype = 'l';
		else if ((pEvtInfo->Mags[pEvtInfo->iPrefMag].iMagType == MAGTYPE_MOMENT) ||
				 (pEvtInfo->Mags[pEvtInfo->iPrefMag].iMagType == MAGTYPE_MWP))
			magtype = 'w';
		else if ((pEvtInfo->Mags[pEvtInfo->iPrefMag].iMagType == MAGTYPE_BODYWAVE) ||
				(pEvtInfo->Mags[pEvtInfo->iPrefMag].iMagType == MAGTYPE_MBLG))
			magtype = 'b';
		else if (pEvtInfo->Mags[pEvtInfo->iPrefMag].iMagType == MAGTYPE_SURFACEWAVE)
			magtype = 'S';
		else if (pEvtInfo->Mags[pEvtInfo->iPrefMag].iMagType == MAGTYPE_DURATION)
			magtype = 'D';
		else
			magtype = '?';
		sprintf(tmp, "%c ",magtype);
		strcat (hypoTWCMessage, tmp);
		logit("","2BuildhypoTWCMessage: .%s%\n",hypoTWCMessage);
		/* int  # stations used in preferred magnitude */
		sprintf (tmp, "%d ", pEvtInfo->Mags[pEvtInfo->iPrefMag].iNumMags);
		strcat (hypoTWCMessage, tmp);

		/* see if we have any of the sought-after magnitudes */
		{
			int		i;
			double	ms=0,  mwp=0,  mb=0,  ml=0;	/* mag values */
			int		ims=0, imwp=0, imb=0, iml=0;/* stations used in mag */

			for (i=0; i<pEvtInfo->iNumMags; i++)
			{
				if (pEvtInfo->Mags[i].iMagType == MAGTYPE_SURFACEWAVE)
				{
					ms = pEvtInfo->Mags[i].dMagAvg; 
					ims = pEvtInfo->Mags[i].iNumMags;
				}
				if (pEvtInfo->Mags[i].iMagType == MAGTYPE_MOMENT ||
					pEvtInfo->Mags[i].iMagType == MAGTYPE_MWP    )
				{
					mwp = pEvtInfo->Mags[i].dMagAvg; 
					imwp = pEvtInfo->Mags[i].iNumMags;
				}
				if (pEvtInfo->Mags[i].iMagType == MAGTYPE_BODYWAVE ||
					pEvtInfo->Mags[i].iMagType == MAGTYPE_MBLG    )
				{
					mb = pEvtInfo->Mags[i].dMagAvg; 
					imb = pEvtInfo->Mags[i].iNumMags;
				}
				if (pEvtInfo->Mags[i].iMagType == MAGTYPE_LOCAL_PEAK2PEAK ||
					pEvtInfo->Mags[i].iMagType == MAGTYPE_LOCAL_ZERO2PEAK    )
				{
					ml = pEvtInfo->Mags[i].dMagAvg; 
					iml = pEvtInfo->Mags[i].iNumMags;
				}
			}
			sprintf(tmp, "%lf %d %lf %d %lf %d %lf %d   ",ms,ims, mwp,imwp, mb,imb, ml,iml);
			strcat (hypoTWCMessage, tmp);
			logit("","3BuildhypoTWCMessage: .%s%\n",hypoTWCMessage);
		}

		strcpy (pMsg, hypoTWCMessage);
	}
	return EW_SUCCESS;
}
/*************************** End of BuildhypoTWCMessage ***************************/

/*************************** QddsValidateEvent ***************************/
static 	int		QddsValidateEvent  (EWEventInfoStruct *pEvtInfo, int *result)
{

	int		i, j, nCoda, nCodaNeeded, nPh;
	double	MagVal;

	if ((pEvtInfo == NULL) || (result == NULL))
	{
		logit ("", "Invalid arguments passed in.\n");
		return EW_FAILURE;
	}


	*result = FALSE;


	/*
	 * STORY: We don't want to issue QDDS notifications to the public web pages
	 *  unless the event passes some checks.  The verification checks were
	 *  stolen from Mitch Withers' eqfilter program
	 */


	/* 
	 * Preliminary calculations:
	 *      count number of phases with weight > 0.1
	 *      count number of codas for this mag level  
	 */

	nPh = 0;
	nCoda = 0;
	nCodaNeeded = 0;

	if (pEvtInfo->iPrefMag < 0)
	{
		logit ("", "No preferred magnitude -- exit\n");
		return EW_FAILURE;
	}

	MagVal = pEvtInfo->Mags[pEvtInfo->iPrefMag].dMagAvg;

	/* first, figure out which mag level we are in */
	for (i = 0; i < NumMCTest; i++)
	{
		if (MagVal > MC_Test[i].Mag)
			nCodaNeeded = MC_Test[i].MinCodas;
	}

	for (i = 0; i < pEvtInfo->iNumChans; i++)
	{
		if (pEvtInfo->pChanInfo[i].iNumArrivals > 0)
		{
			for (j = 0; j < pEvtInfo->pChanInfo[i].iNumArrivals; j++)
			{
				/* check phase weight */
				if (pEvtInfo->pChanInfo[i].Arrivals[j].dSigma < 0.1)
					nPh = nPh + 1;


				/* check coda */
				if (pEvtInfo->pChanInfo[i].Stamags[j].StaMagUnion.CodaDur.tCodaTermXtp > 0.0)
					nCoda = nCoda + 1;
			}
		}
	}


	/* Depth tests */
	if (pEvtInfo->PrefOrigin.dDepth < MinDepth)
	{
		logit ("", "ValidateQdds: fail MinDepthTest (%0.2f < %0.2f)\n", 
							pEvtInfo->PrefOrigin.dDepth, MinDepth);
		return EW_SUCCESS;
	}
			

	if (pEvtInfo->PrefOrigin.dDepth > MaxDepth)
	{
		logit ("", "ValidateQdds: fail MaxDepthTest (%0.2f > %0.2f)\n", 
							pEvtInfo->PrefOrigin.dDepth, MaxDepth);
		return EW_SUCCESS;
	}
			
	
	if (NumPhases > nPh)
	{
		logit ("", "ValidateQdds: fail NphTest (%d > %d)\n", NumPhases, nPh);
		return EW_SUCCESS;
	}

	if (pEvtInfo->PrefOrigin.iGap > MaxGap)
	{
		logit ("", "ValidateQdds: fail GapTest (%0.2f > %0.2f)\n", 
							pEvtInfo->PrefOrigin.iGap, MaxGap);
		return EW_SUCCESS;
	}

	if (pEvtInfo->PrefOrigin.dDmin > MaxDist)
	{
		logit ("", "ValidateQdds: fail DminTest (%0.2f > %0.2f)\n", 
							pEvtInfo->PrefOrigin.dDmin, MaxDist);
		return EW_SUCCESS;
	}
			
	if (pEvtInfo->PrefOrigin.dRms > MaxRMS)
	{
		logit ("", "ValidateQdds: fail RmsTest (%0.2f > %0.2f)\n", 
							pEvtInfo->PrefOrigin.dRms, MaxRMS);
		return EW_SUCCESS;
	}
			
	if (pEvtInfo->PrefOrigin.dRms > MaxRMS)
	{
		logit ("", "ValidateQdds: fail RmsTest (%0.2f > %0.2f)\n", 
							pEvtInfo->PrefOrigin.dRms, MaxRMS);
		return EW_SUCCESS;
	}
			
	if (pEvtInfo->PrefOrigin.dE0 > MaxER0)
	{
		logit ("", "ValidateQdds: fail MaxE0Test (%0.2f > %0.2f)\n", 
							pEvtInfo->PrefOrigin.dE0, MaxER0);
		return EW_SUCCESS;
	}
			
	if (pEvtInfo->PrefOrigin.dE1 > MaxERZ)
	{
		logit ("", "ValidateQdds: fail MaxERZTest (%0.2f > %0.2f)\n", 
							pEvtInfo->PrefOrigin.dE1, MaxERZ);
		return EW_SUCCESS;
	}

	if (pEvtInfo->PrefOrigin.dE2 > MaxERH)
	{
		logit ("", "ValidateQdds: fail MaxERHTest (%0.2f > %0.2f)\n", 
							pEvtInfo->PrefOrigin.dE2, MaxERH);
		return EW_SUCCESS;
	}
			

	if (nCodaNeeded > nCoda)
	{
		logit ("", "ValidateQdds: fail NcodaTest (%d > %d)\n", nCodaNeeded, nCoda);
		return EW_SUCCESS;
	}
			
			
	*result = TRUE;
	return EW_SUCCESS;

}



static	int		ComputeBearingFromAzm (double azm, char *bearing)
{

	int		index;
	char    *bnames[] = {   "N", "NNE", "NE", "ENE", "E", "ESE", "SE", "SSE",
                "S", "SSW", "SW", "WSW", "W", "WNW", "NW", "NNW", "N" };

	if (bearing == NULL)
	{
		logit ("", "Invalid arguments passed in.\n");
		return EW_FAILURE;
	}

	index =  (int) (azm/22.5);
	strcpy (bearing, bnames[index]);

	return EW_SUCCESS;
}

