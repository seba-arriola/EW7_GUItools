/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: hypo71_2_ewevent.c,v 1.1 2004/07/01 19:08:16 labcvs Exp $
 *
 *    Revision history:
 *     $Log: hypo71_2_ewevent.c,v $
 *     Revision 1.1  2004/07/01 19:08:16  labcvs
 *     Moved from src/data_sources to src/oracle/apps/src JMP
 *
 *     Revision 1.1  2002/03/22 20:14:07  lucky
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


/* Define max lengths of various lines in the Hypoinverse archive format
   (includes space for a newline & null-terminator on each line )
 ************************************************************************/
#define HYP71_SUM_LEN   500

int 	EWEvent2Hypo71Msg (EWEventInfoStruct *, char *, int);



/********************************************************
*
*  EWEvent2Hypo71Msg:
*     Creates a Hypo71 message, stored in 
*     string pHypo71, from the data passed in the 
*     pEWEvent structure and its substructures.
*
*
*      Returns EW_SUCCESS if everything went OK, 
*       EW_FAILURE otherwise.
*
*
*   Author: Lucky Vidmar    10/2001
*
********************************************************/
int 	EWEvent2Hypo71Msg (EWEventInfoStruct *pEWEvent, 
								char *pHypo71, int MsgLen)
{

	if ((pEWEvent == NULL) || (pHypo71 == NULL) || (MsgLen < 0))
	{
		logit ("", "Invalid arguments passed in.\n");
		return EW_FAILURE;
	}


	/* Don't need this yet -- will we ever?? */

	return EW_SUCCESS;
}


/********************************************************
*
*  Hypo71Msg2EWEvent:
*     Given a Hypoelipse message stored in 
*     string pHypo71, fill the pEWEvent structure
*     and its substructures.
*
*
*      Returns EW_SUCCESS if everything went OK, 
*       EW_FAILURE otherwise.
*
*
*   Author: Lucky Vidmar    10/2001
*
********************************************************/
int 	Hypo71Msg2EWEvent (EWEventInfoStruct *pEWEvent, 
								char *pHypo71, int MsgLen)
{

	char				line[HYP71_SUM_LEN];
	char				cdate[HYP71_SUM_LEN];
	int					len;
	float				deg, min, sign;

	if ((pEWEvent == NULL) || (pHypo71 == NULL) || (MsgLen <= 0))
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
	
	/* Copy incoming string to a local buffer so we can alter it.
	 ************************************************************/
	strncpy (line, pHypo71, 120); 
	len = strlen (pHypo71);       /* note length of original summary line */

	/* Read origin time
	 ******************/
	strncpy (cdate, line, 8); /* YYYYMMDD */
	strncpy ((cdate+8), line+9, 4);  /* HHMM */
	strncpy ((cdate+12), line+14, 5);  /* SS.SS*/
	cdate[17] = '\0';

	pEWEvent->PrefOrigin.tOrigin = julsec17 (cdate) - GSEC1970;
	if (pEWEvent->PrefOrigin.tOrigin == 0.0) 
	{
		logit( "", "Error decoding origin time <%s>, got %f\n",
									cdate, pEWEvent->PrefOrigin.tOrigin);
		return EW_FAILURE;
	}

	/* Read the rest of the line, back to front
	 ******************************************/
	if (len < 95)
	{
		logit ("", "Line too short, must have at least 95 characters.\n");
		return EW_FAILURE;
	}

	/* We're not reading fields beyond str[95] */

	/* Source Event Id; EventIDVersion */
	line[95] = '\0';
	pEWEvent->Event.idEvent= atoi (line+83);

	/* Vertical Error */
	line[79]  = '\0';  
	pEWEvent->PrefOrigin.dErZ = atof (line+74);

	/* Horizontal Error */
	line[74]  = '\0';  
	pEWEvent->PrefOrigin.dErrLat = atof (line+69);
	pEWEvent->PrefOrigin.dErrLon = pEWEvent->PrefOrigin.dErrLat;

	/* RMS Residual */
	line[69]  = '\0';  
	pEWEvent->PrefOrigin.dRms = atof (line+64);

	/* Distance to the nearest station */
	line[64]  = '\0';  
	pEWEvent->PrefOrigin.dDmin = atof (line+59);

	/* Gap */
	line[59]  = '\0';  
	pEWEvent->PrefOrigin.iGap = atoi (line+55);

	/* Phases used (P and S times with certain weights) */
	line[55]  = '\0';  
	pEWEvent->PrefOrigin.iUsedPh = atoi (line+52);

	/* Magnitude */
	line[52]  = '\0';  
	pEWEvent->Mags[0].dMagAvg = atof (line+47);

	/* Depth */
	line[45]  = '\0';  
	pEWEvent->PrefOrigin.dDepth = atof (line+38);
	pEWEvent->PrefOrigin.iFixedDepth = FALSE;

	/* Longitude minutes */
	line[38]  = '\0';  
	min = atof (line+33);

	/* Longitude sign */
	if ((line[32] == ' ') || (line[32] == 'W') || (line[32] == 'w')) 
		sign = -1.0;
	else
		sign = 1.0;

	/* Longitude degrees */
	line[32] = '\0';
	deg = atof (line+28);

	pEWEvent->PrefOrigin.dLon = (float) (sign * (deg + (min/60.0)));

	/* Latitude minutes */
	line[28]  = '\0';  
	min = atof (line+23);

	/* Latitude sign */
	if ((line[22] == 's') || (line[22] == 'S')) 
		sign = -1.0;
	else
		sign = 1.0;

	/* Latitude degrees */
	line[22] = '\0';
	deg = atof (line+19);

	pEWEvent->PrefOrigin.dLat = (float) (sign * (deg + (min/60.0)));


	/* HACK ALERT */
	/* 
	 * I set the magtype to duration for now, since we 
	 * don't know how to deal with other exotic types 
	 * of magnitudes yet.
	 * This should get fixed in the near future 
	 */
	pEWEvent->Mags[0].iMagType = MAGTYPE_DURATION;
	pEWEvent->iMd = 0;
	pEWEvent->Mags[0].idEvent = pEWEvent->Event.idEvent;
	pEWEvent->Mags[0].bBindToEvent = TRUE;
	pEWEvent->Mags[0].bSetPreferred = TRUE;
	pEWEvent->iNumMags = 1;
	pEWEvent->iPrefMag = 0;

	pEWEvent->Event.iEventType = EWDB_EVENT_TYPE_QUAKE;
	pEWEvent->PrefOrigin.idEvent = pEWEvent->Event.idEvent;
	pEWEvent->PrefOrigin.BindToEvent = TRUE;
	pEWEvent->PrefOrigin.SetPreferred = TRUE;


	return EW_SUCCESS;

}


