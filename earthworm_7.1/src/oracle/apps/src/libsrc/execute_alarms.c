/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: execute_alarms.c,v 1.8 2002/01/14 22:13:16 lucky Exp $
 *    Revision history:
 *
 *    $Log: execute_alarms.c,v $
 *    Revision 1.8  2002/01/14 22:13:16  lucky
 *    Changed format of email message to have flexible subject line.
 *
 *    Revision 1.7  2001/08/07 16:53:30  lucky
 *    Pre v6.0 checkin
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
 *    Revision 1.3  2001/06/26 17:37:03  lucky
 *    State of the code after all Utah specs have been met
 *
 *    Revision 1.2  2001/06/06 20:55:35  lucky
 *    Changes made to support multitude magnitudes, as well as amplitude picks. This
 *    is work in progress - checkin in for the sake of safety.
 *
 *    Revision 1.1  2001/05/15 02:15:25  davidk
 *    Initial revision
 *
 *
 *
 */



#include <alarms.h>
#include <ewdb_apps_utils.h>
#include <transport.h>

#define         INIT_BUF_SIZE       10

/* Transport global variables
 ****************************/
static  SHM_INFO  Region;      
static  MSG_LOGO  PutLogo;    

static      unsigned char   TypeEmail;
static      unsigned char   TypePager;
static      unsigned char   TypeQDDS;
static      unsigned char   TypeCustom;


/*************************** ExecuteAlarms ***************************/
int 	ExecuteAlarms (AlarmList *pAlarms, int NumAlarms, char
						*InvocationString, int RingKey, int InstId, int ModId)
{

	int					i, msgLen;
	char				*msg;

	if ((pAlarms == NULL) || (NumAlarms < 0) || (RingKey < 0) ||
				(InstId < 0) || (ModId < 0) || (InvocationString == NULL))
	{
		logit ("", "Invalid arguments passed in.\n");
		return EW_FAILURE;
	}

	if (NumAlarms == 0)
	{
		logit ("", "No Alarms to process - exiting.\n");
		return EW_SUCCESS;
	}

	if (GetType ("TYPE_EMAIL_MSG", &TypeEmail) != 0)
	{
		logit ("", " Invalid type TYPE_EMAIL_MSG!\n");
		return EW_FAILURE;
	}

	if (GetType ("TYPE_PAGER_MSG", &TypePager) != 0)
	{
		logit ("", " Invalid type TYPE_PAGER_MSG!\n");
		return EW_FAILURE;
	}

	if (GetType ("TYPE_QDDS_MSG", &TypeQDDS) != 0)
	{
		logit ("", " Invalid type TYPE_QDDS_MSG!\n");
		return EW_FAILURE;
	}

	if (GetType ("TYPE_CUSTOM_ALARM_MSG", &TypeCustom) != 0)
	{
		logit ("", " Invalid type TYPE_CUSTOM_ALARM_MSG!\n");
		return EW_FAILURE;
	}

	/* Attach to alarm ring
	 **********************************/
    tport_attach (&Region, RingKey);

	/* 
	 * Go through the alarms list, create appropriate message,
	 * and put it into the ring. Also, insert into the DB 
	 * an audit message to denote when this alarm was raised.
	 */
	for (i = 0; i < NumAlarms; i++)
	{
		if ((msg = malloc (2 * ALARM_MSG_SIZE * sizeof (char))) == NULL)
		{
			logit ("", "Could not malloc alarm msg\n");
		    tport_detach (&Region);
			return EW_FAILURE;
		}

		/* insert audit info into the database */
		pAlarms[i].Audit.idAudit = -1;
		pAlarms[i].Audit.tAlarmDeclared = (double) time (NULL);
		pAlarms[i].Audit.tAlarmExecuted = 0.0;
		strcpy (pAlarms[i].Audit.sInvocationString, InvocationString);

		if (EWDB_InsertAudit (&pAlarms[i].Audit, &pAlarms[i].Delivery) != EW_SUCCESS)
		{
			logit ("", "Call to EWDB_InsertAudit failed for alarm %d.\n", i);
		    tport_detach (&Region);
			return EW_FAILURE;
		}

		/* Create an appropriate message */
		if (pAlarms[i].Delivery.DelMethodInd == EWDB_ALARMS_DELIVERY_IND_EMAIL)
		{
			sprintf (msg,   "%s\n" 
							"%s\n"
							"%d\n"
							"%d\n"
							"%s", 
								pAlarms[i].Delivery.email.sAddress,
								pAlarms[i].Delivery.email.sMailServer,
								pAlarms[i].Audit.idEvent,
								pAlarms[i].Audit.idAudit,
								pAlarms[i].AlarmMsg);

			PutLogo.instid  = InstId;
			PutLogo.mod     = ModId;
			PutLogo.type    = TypeEmail;


			msgLen = strlen (msg);
			msg[msgLen] = '\0';
			
			if (tport_putmsg (&Region, &PutLogo, msgLen, msg) != PUT_OK)
			{
				logit ("t", "Error writing to ring -- ALARM NOT ISSUED!\n");

				pAlarms[i].Audit.tAlarmDeclared = -1.0;
				pAlarms[i].Audit.tAlarmExecuted = 0.0;
				if (ewdb_api_CreateAlarmAudit (&pAlarms[i].Audit) != EWDB_RETURN_SUCCESS)
				{
					logit ("", "Call to ewdb_api_CreateAlarmAudit failed.\n");
		    		tport_detach (&Region);
					return EW_FAILURE;
				}
			}
		}
		else if (pAlarms[i].Delivery.DelMethodInd == EWDB_ALARMS_DELIVERY_IND_PAGER)
		{
			sprintf (msg,   "%s\n" 
							"%s\n"
							"Earthworm Alarm: Event=%d\n"
							"%d\n"
							"%s", 
								pAlarms[i].Delivery.email.sAddress,
								pAlarms[i].Delivery.email.sMailServer,
								pAlarms[i].Audit.idEvent,
								pAlarms[i].Audit.idAudit,
								pAlarms[i].AlarmMsg);

			PutLogo.instid  = InstId;
			PutLogo.mod     = ModId;
			PutLogo.type    = TypePager;

			msgLen = strlen (msg);
			msg[msgLen] = '\0';
			
			if (tport_putmsg (&Region, &PutLogo, msgLen, msg) != PUT_OK)
			{
				logit ("t", "Error writing to ring -- ALARM NOT ISSUED!\n");

				pAlarms[i].Audit.tAlarmDeclared = -1.0;
				pAlarms[i].Audit.tAlarmExecuted = 0.0;
				if (ewdb_api_CreateAlarmAudit (&pAlarms[i].Audit) != EWDB_RETURN_SUCCESS)
				{
					logit ("", "Call to ewdb_api_CreateAlarmAudit failed.\n");
		    		tport_detach (&Region);
					return EW_FAILURE;
				}
			}
		}
		else if (pAlarms[i].Delivery.DelMethodInd == EWDB_ALARMS_DELIVERY_IND_PHONE)
		{
            /*
             * All we have to do is issue the declaration time audit.
             * It is up to the user to make the calls.
             */
		}
		else if (pAlarms[i].Delivery.DelMethodInd == EWDB_ALARMS_DELIVERY_IND_QDDS)
		{

			/* 
			 * Build a TYPE_QDDS message and put it on the ring. 
			 */

			sprintf (msg,   "%s\n"
							"%d\n"
							"%s", 
								pAlarms[i].Delivery.qdds.sQddsDirectory,
								pAlarms[i].Audit.idAudit,
								pAlarms[i].AlarmMsg);

			PutLogo.instid  = InstId;
			PutLogo.mod     = ModId;
			PutLogo.type    = TypeQDDS;


			msgLen = strlen (msg);
			msg[msgLen] = '\0';
			
			if (tport_putmsg (&Region, &PutLogo, msgLen, msg) != PUT_OK)
			{
				logit ("t", "Error writing to ring -- ALARM NOT ISSUED!\n");

				pAlarms[i].Audit.tAlarmDeclared = -1.0;
				pAlarms[i].Audit.tAlarmExecuted = 0.0;
				if (ewdb_api_CreateAlarmAudit (&pAlarms[i].Audit) != EWDB_RETURN_SUCCESS)
				{
					logit ("", "Call to ewdb_api_CreateAlarmAudit failed.\n");
		    		tport_detach (&Region);
					return EW_FAILURE;
				}
			}
		}
		else if (pAlarms[i].Delivery.DelMethodInd == EWDB_ALARMS_DELIVERY_IND_CUSTOM)
		{

			/* 
			 * Build a TYPE_CUSTOM message and put it on the ring. 
			 */

			sprintf (msg,   "%d\n"
							"%d\n"
							"%s\n"
							"%s", 
								pAlarms[i].Audit.idEvent,
								pAlarms[i].Audit.idAudit,
								pAlarms[i].Delivery.custom.sDescription,
								pAlarms[i].AlarmMsg);


			PutLogo.instid  = InstId;
			PutLogo.mod     = ModId;
			PutLogo.type    = TypeCustom;


			msgLen = strlen (msg);
			msg[msgLen] = '\0';
			
			if (tport_putmsg (&Region, &PutLogo, msgLen, msg) != PUT_OK)
			{
				logit ("t", "Error writing to ring -- ALARM NOT ISSUED!\n");

				pAlarms[i].Audit.tAlarmDeclared = -1.0;
				pAlarms[i].Audit.tAlarmExecuted = 0.0;
				if (ewdb_api_CreateAlarmAudit (&pAlarms[i].Audit) != EWDB_RETURN_SUCCESS)
				{
					logit ("", "Call to ewdb_api_CreateAlarmAudit failed.\n");
		    		tport_detach (&Region);
					return EW_FAILURE;
				}
			}
		}
		else
		{
			logit ("", "Alarm %d: Unknown delivery type: %d; skipping.\n", i,
										pAlarms[i].Delivery.DelMethodInd);
		}

		free (msg);
	}


	/* Detach from alarm ring
	 **********************************/
    tport_detach (&Region);

	return EW_SUCCESS;

}
