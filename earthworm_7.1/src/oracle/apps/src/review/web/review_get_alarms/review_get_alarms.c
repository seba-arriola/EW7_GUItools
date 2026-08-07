/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: review_get_alarms.c,v 1.8 2002/10/15 19:27:48 lucky Exp $
 *
 *    Revision history:
 *     $Log: review_get_alarms.c,v $
 *     Revision 1.8  2002/10/15 19:27:48  lucky
 *     *** empty log message ***
 *
 *     Revision 1.7  2002/05/28 19:34:48  lucky
 *     Added header and footer tag text
 *
 *     Revision 1.6  2002/03/22 20:03:33  lucky
 *     Pre v6.1 checkin
 *
 *     Revision 1.5  2001/07/31 20:47:20  lucky
 *     Changed alarms nomenclature from User to Recipient
 *
 *     Revision 1.4  2001/07/28 00:45:23  lucky
 *      State of the code after debugging and testing prior to v6.0 release freeze.
 *
 *     Revision 1.3  2001/07/20 17:33:02  lucky
 *     Added InvocationString to the display
 *
 *     Revision 1.2  2001/07/01 21:55:32  davidk
 *     Cleanup of the Earthworm Database API and the applications that utilize it.
 *     The "ewdb_api" was cleanup in preparation for Earthworm v6.0, which is
 *     supposed to contain an API that will be backwards compatible in the
 *     future.  Functions were removed, parameters were changed, syntax was
 *     rewritten, and other stuff was done to try to get the code to follow a
 *     general format, so that it would be easier to read.
 *
 *     Applications were modified to handle changed API calls and structures.
 *     They were also modified to be compiler warning free on WinNT.
 *
 *     Revision 1.1  2001/06/21 21:26:25  lucky
 *     Initial revision
 *
 *
 *
 *************************************************************/
  

/*** #includes ***/

/* include the normal stuff */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <time_ew.h>
#include <ew_event_info.h>
#include <ewdb_ora_api.h>
#include <ewdb_apps_utils.h>
#include <webparse.h>
#include <alarms.h>
#include <review.h>

typedef struct webopts
{
	int 	idEvent;
} WebOpts;

#define		NUM_INIT_AUDITS 	100

int main(int argc, char * argv[])
{

	WebOpts			WebParams;
  	char			*configfile = "../params/review_event.d";
	EWDB_AlarmAuditStruct	*pAudit;
	EWDB_AlarmDeliveryUnionStruct	*pDelivery;
	int				i, NumFound, NumRetr;
	char			tBuf[30];
	char			tmpstr[256];

    DEBUG = 0;

   /* Read the configuration file (path hardcoded relative to executable)
     *********************************************************************/
    ReadConfig (configfile);

    /* Cleanup, logfile should match config file name */
    logit_init ("review_get_alarms",1,MAX_BYTES_PER_EQ,1);

    if (Webparse_GetAndProcessWebParams((void *)(&WebParams)) != 0)
    {
        html_logit ("", "Call to Webparse_GetAndProcessWebParams failed.\n");
        html_break ();
        goto shutdown;
    }


    /* Send html header back to web server
     *************************************/
    printf("Content-type: text/html\n\n");
    printf("<html>\n");

    printf ("<HEAD><TITLE>Alarms for %d</TITLE></HEAD>\n", WebParams.idEvent );

	html_header (BackgroundColor, HeaderLogo, HeaderTag);

    printf("<center>\n");
    printf("<H2><FONT COLOR=red>Alarms Listing Page</FONT></H2>\n");
    printf("</center>\n");

    printf("<pre>\n");

	/* Connect to the DB */
    if (ewdb_api_Init (DBuser, DBpassword, DBservice ) != 0 )
    {
        html_logit ("", "Trouble connecting to database; exiting!\n" );
        html_break();
        goto shutdown;
    }

	/* Allocated audit list buffer */
	if ((pAudit = (EWDB_AlarmAuditStruct *) malloc (NUM_INIT_AUDITS *
								sizeof (EWDB_AlarmAuditStruct))) == NULL)
	{
		html_logit ("", "Could not malloc Audit list buffer.\n");
		return EW_FAILURE;
	}

	/* Allocated delivery list buffer */
	if ((pDelivery = (EWDB_AlarmDeliveryUnionStruct *) malloc (NUM_INIT_AUDITS *
								sizeof (EWDB_AlarmDeliveryUnionStruct))) == NULL)
	{
		html_logit ("", "Could not malloc Delivery list buffer.\n");
		return EW_FAILURE;
	}

	/* Retrieve list from the db */
	if (EWDB_GetAudits (WebParams.idEvent, pAudit, 1, pDelivery,  &NumFound,
				&NumRetr, NUM_INIT_AUDITS) != EWDB_RETURN_SUCCESS)
	{
		html_logit ("", "Call to EWDB_GetAudits failed.\n");
		return EW_FAILURE;
	}

	if (NumRetr < NumFound)
	{
		logit ("", "buffers too small, found %d, retrieved %d\n",
						NumFound, NumRetr);
		free (pAudit);
		free (pDelivery);

		/* Allocated audit list buffer */
		if ((pAudit = (EWDB_AlarmAuditStruct *) malloc (NumFound *
									sizeof (EWDB_AlarmAuditStruct))) == NULL)
		{
			html_logit ("", "Could not malloc Audit list buffer.\n");
			return EW_FAILURE;
		}

		/* Allocated delivery list buffer */
		if ((pDelivery = (EWDB_AlarmDeliveryUnionStruct *) malloc (NumFound *
									sizeof (EWDB_AlarmDeliveryUnionStruct))) == NULL)
		{
			html_logit ("", "Could not malloc Delivery list buffer.\n");
			return EW_FAILURE;
		}
		
		/* Retrieve list from the db */
		if (EWDB_GetAudits (WebParams.idEvent, pAudit, 1, pDelivery, &NumFound,
					&NumRetr, NumFound) != EWDB_RETURN_SUCCESS)
		{
			html_logit ("", "Call to EWDB_GetAudits failed.\n");
			return EW_FAILURE;
		}
	}

	printf ("<PRE><HR><P>\n");
	printf ("<STRONG><CENTER>Event %d: Issued %d alarms.</CENTER></STRONG>\n",
									  WebParams.idEvent, NumRetr);

	for (i = 0; i < NumRetr; i++)
	{
		printf ("<HR>\n");

		if (pAudit[i].bAuto == TRUE)
			strcpy (tmpstr, "Automatic");
		else 
			strcpy (tmpstr, "Reviewed");

		printf ("%s solution alarm declared for user %s\n", 
							tmpstr, pAudit[i].Recipient.sDescription);

		printf ("Alarm triggered by:  %s\n", 
						pAudit[i].sInvocationString);

		printf ("Delivery by ");
		if (pDelivery[i].DelMethodInd == EWDB_ALARMS_DELIVERY_IND_EMAIL)
			printf ("email to %s\n", 
					pDelivery[i].email.sAddress);

		else if (pDelivery[i].DelMethodInd == EWDB_ALARMS_DELIVERY_IND_PAGER)
			printf ("pager to %s\n", 
					pDelivery[i].pager.sNumber);

		else if (pDelivery[i].DelMethodInd == EWDB_ALARMS_DELIVERY_IND_PHONE)
			printf ("phone to %s\n", 
					pDelivery[i].phone.sPhoneNumber);
		else if (pDelivery[i].DelMethodInd == EWDB_ALARMS_DELIVERY_IND_QDDS)
			printf ("QDDS to %s\n", 
					pDelivery[i].qdds.sQddsDirectory);
		else if (pDelivery[i].DelMethodInd == EWDB_ALARMS_DELIVERY_IND_CUSTOM)
			printf ("Custom delivery labelled with (%s)\n", 
					pDelivery[i].custom.sDescription);

		else
			printf ("ERROR: UNKNOWN DELIVERY TYPE: %d\n",
						pDelivery[i].DelMethodInd);

 		printf ("Message Format:   %s\n", pAudit[i].Format.sDescription);

		if (pAudit[i].tAlarmDeclared > 0.0) 
			datestr23 (pAudit[i].tAlarmDeclared, tBuf, 30);
		else
			sprintf (tBuf, "PENDING");
		printf ("\nDeclaration Time: %s\n", tBuf);  

		if (pAudit[i].tAlarmExecuted > 0.0) 
			datestr23 (pAudit[i].tAlarmExecuted, tBuf, 30);
		else
			sprintf (tBuf, "PENDING");
		printf ("Execution Time:   %s\n", tBuf);

	}

shutdown:
    ewdb_api_Shutdown();
    html_trailer (WebHost, FooterLogo, FooterTag);
    logit("","get_alarms: terminating\n" );

    return (0);
}


int Webparse_Client_SetVars(char * szVar, char * szVal, void * pUserParams)
{

	WebOpts * pOptions= (WebOpts *) pUserParams;

	if(strcmp(szVar,"EventID") == 0)
	{
		pOptions->idEvent = atoi(szVal);
	}
	else
	{
		html_logit ("" ,"Unrecognized Web Option : %s = %s\n",szVar,szVal);
	}
	return(0);

}  /* End of SetVars() */


