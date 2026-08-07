/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: aeic_2_ewevent.c,v 1.1 2004/07/01 18:58:51 labcvs Exp $
 *
 *    Revision history:
 *     $Log: aeic_2_ewevent.c,v $
 *     Revision 1.1  2004/07/01 18:58:51  labcvs
 *     Moved aeic2arc from src/data_sources to /src/oracle/apps/src JMP
 *
 *     Revision 1.1  2002/03/22 20:13:58  lucky
 *     Initial revision
 *
 *
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <earthworm.h>
#include <ws_clientII.h>
#include <chron3.h>
#include <ewdb_ora_api.h>
#include <ew_event_info.h>



int 	EWEvent2AeicMsg (EWEventInfoStruct *, char *, int);



/********************************************************
*
*  EWEvent2AeicMsg:
*     Creates an AEIC location message, stored in 
*     string pAeic, from the data passed in the 
*     pEWEvent structure and its substructures.
*
*
*      Returns EW_SUCCESS if everything went OK, 
*       EW_FAILURE otherwise.
*
*
*   Author: Lucky Vidmar    12/2001
*
********************************************************/
int 	EWEvent2AeicMsg (EWEventInfoStruct *pEWEvent, 
								char *pAeic, int MsgLen)
{

	if ((pEWEvent == NULL) || (pAeic == NULL) || (MsgLen < 0))
	{
		logit ("", "Invalid arguments passed in.\n");
		return EW_FAILURE;
	}


	/* Don't need this yet -- will we ever?? */

	return EW_SUCCESS;
}


/********************************************************
*
*  AeicMsg2EWEvent:
*     Given a reformatted AIEC location message stored in 
*     string pAeic, fill the pEWEvent structure
*     and its substructures.
*
*
*      Returns EW_SUCCESS if everything went OK, 
*       EW_FAILURE otherwise.
*
*
*   Author: Lucky Vidmar    12/2001
*
********************************************************/
int 	AeicMsg2EWEvent (EWEventInfoStruct *pEWEvent, 
								char *pAeic, int MsgLen)
{

	char	*token;
	char	tmpstr[128];
	float	ml, mb, ms;
	int		ndef, nm;

	if ((pEWEvent == NULL) || (pAeic == NULL) || (MsgLen <= 0))
	{
		logit ("", "Invalid arguments passed in.\n");
		return EW_FAILURE;
	}


	/* Initialize the Event structure */
	if (InitEWEvent (pEWEvent) != EW_SUCCESS)
	{
		logit ("", "Call to InitEWEvent failed.\n");
		return EW_FAILURE;
	}
	
	/* Parse the location line
	 *****************************/
/*
	sscanf (pAeic, "%9.4lf %9.4lf %9.4lf %17.5lf %7.2lf  %7.2lf %7.2lf %d %s",
				&pEWEvent->PrefOrigin.dLat,
				&pEWEvent->PrefOrigin.dLon,
				&pEWEvent->PrefOrigin.dDepth,
				&pEWEvent->PrefOrigin.tOrigin,
				&ml, &mb, &ms, &ndef, 
				pEWEvent->Event.szSource);
*/


	/* Lat */
	strncpy (tmpstr, pAeic, 9);
	tmpstr[9] = '\0';
	pEWEvent->PrefOrigin.dLat = atof (tmpstr);
	token = pAeic + 10;

	/* Lon */
	strncpy (tmpstr, token, 9);
	tmpstr[9] = '\0';
	pEWEvent->PrefOrigin.dLon = atof (tmpstr);
	token = token + 10;

	/* Depth */
	strncpy (tmpstr, token, 9);
	tmpstr[9] = '\0';
	pEWEvent->PrefOrigin.dDepth = atof (tmpstr);
	token = token + 10;

	/* Origin Time */
	strncpy (tmpstr, token, 17);
	tmpstr[17] = '\0';
	pEWEvent->PrefOrigin.tOrigin = atof (tmpstr);
	token = token + 18;

	/* ML */
	strncpy (tmpstr, token, 7);
	tmpstr[7] = '\0';
	ml = atof (tmpstr);
	token = token + 9;

	/* MB */
	strncpy (tmpstr, token, 7);
	tmpstr[7] = '\0';
	mb = atof (tmpstr);
	token = token + 8;

	/* MS */
	strncpy (tmpstr, token, 7);
	tmpstr[7] = '\0';
	ms = atof (tmpstr);
	token = token + 8;


	/* Unknowns */
	sscanf (token, "%d %s", &ndef, pEWEvent->Event.szSource);

logit ("e", "Read <%9.4lf %9.4lf %9.4lf %17.5lf %7.2lf  %7.2lf %7.2lf %d %s>\n",
				pEWEvent->PrefOrigin.dLat,
				pEWEvent->PrefOrigin.dLon,
				pEWEvent->PrefOrigin.dDepth,
				pEWEvent->PrefOrigin.tOrigin,
				ml, mb, ms, ndef, 
				pEWEvent->Event.szSource);

	/* Set other unknown fields
	 ****************************/

	pEWEvent->iNumChans = 0;

	/* This is a HACK -- we should ask for a real event ID from AEIC */
	pEWEvent->Event.idEvent = (EWDBid) pEWEvent->PrefOrigin.tOrigin;

	pEWEvent->Event.iEventType = EWDB_EVENT_TYPE_QUAKE;

	pEWEvent->PrefOrigin.iFixedDepth = FALSE;
	pEWEvent->PrefOrigin.dErrLat = 0.0;
	pEWEvent->PrefOrigin.dErrLon = 0.0;
	pEWEvent->PrefOrigin.dErZ = 0.0;
	pEWEvent->PrefOrigin.dRms = 0.0;
	pEWEvent->PrefOrigin.dDmin = 0.0;
	pEWEvent->PrefOrigin.iGap = 0;
	pEWEvent->PrefOrigin.iUsedPh = 0;
	pEWEvent->PrefOrigin.idEvent = pEWEvent->Event.idEvent;
	pEWEvent->PrefOrigin.BindToEvent = TRUE;
	pEWEvent->PrefOrigin.SetPreferred = TRUE;


	/* Magnitude 
	 ****************/
	if (ml != -999.0)
	{
		pEWEvent->Mags[0].dMagAvg = ml;
		pEWEvent->Mags[0].iMagType = MAGTYPE_LOCAL_PEAK2PEAK;
		pEWEvent->Mags[0].idEvent = pEWEvent->Event.idEvent;
		pEWEvent->Mags[0].bBindToEvent = TRUE;
		pEWEvent->Mags[0].bSetPreferred = TRUE;
		pEWEvent->iML = 0;
		pEWEvent->iPrefMag = 0;
		pEWEvent->iNumMags = 1;
	}
		
	if (mb != -999.0)
	{
		nm = pEWEvent->iNumMags;
		pEWEvent->Mags[nm].dMagAvg = mb;
		pEWEvent->Mags[nm].iMagType = MAGTYPE_BODYWAVE;
		pEWEvent->Mags[nm].idEvent = pEWEvent->Event.idEvent;
		pEWEvent->Mags[nm].bBindToEvent = TRUE;

		if (nm == 0)
		{
			/* No ML */
			pEWEvent->Mags[0].bSetPreferred = TRUE;
			pEWEvent->iPrefMag = 0;
		}

		pEWEvent->iNumMags = pEWEvent->iNumMags + 1;
	}

	if (ms != -999.0)
	{
		nm = pEWEvent->iNumMags;
		pEWEvent->Mags[nm].dMagAvg = ms;
		pEWEvent->Mags[nm].iMagType = MAGTYPE_SURFACEWAVE;
		pEWEvent->Mags[nm].idEvent = pEWEvent->Event.idEvent;
		pEWEvent->Mags[nm].bBindToEvent = TRUE;

		if (nm == 0)
		{
			/* No ML or Mb */
			pEWEvent->Mags[0].bSetPreferred = TRUE;
			pEWEvent->iPrefMag = 0;
		}

		pEWEvent->iNumMags = pEWEvent->iNumMags + 1;
	}

	/* What to do if we have no valid magnitudes? Lie and cheat? */
	if (pEWEvent->iNumMags == 0)
	{
		pEWEvent->Mags[0].dMagAvg = 0.0;
		pEWEvent->Mags[0].iMagType = MAGTYPE_DURATION;
		pEWEvent->Mags[0].idEvent = pEWEvent->Event.idEvent;
		pEWEvent->Mags[0].bBindToEvent = TRUE;
		pEWEvent->Mags[0].bSetPreferred = TRUE;
		pEWEvent->iMd = 0;
		pEWEvent->iPrefMag = 0;
		pEWEvent->iNumMags = 1;
	}
		


	return EW_SUCCESS;

}


