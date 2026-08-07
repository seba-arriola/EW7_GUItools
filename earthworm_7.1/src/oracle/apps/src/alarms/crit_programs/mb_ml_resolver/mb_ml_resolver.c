
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: mb_ml_resolver.c,v 1.1 2004/03/17 18:57:34 davidk Exp $
 *
 *    Revision history:
 *     $Log: mb_ml_resolver.c,v $
 *     Revision 1.1  2004/03/17 18:57:34  davidk
 *     Initial revision
 *
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <earthworm.h>
#include <ew_event_info.h>
#include <PolygonArea.h>

/* function prototypes */
int	DoCheck(EWEventInfoStruct *pEvt);



int 	main (int argc, char **argv)
{

	char						filename[1024];
	FILE						*fp;
	EWEventInfoStruct			EvtInfo;


	/* 
	 * We expect two arguments: 
	 *  1 - working directory -- must be writeable to us
	 *  2 - name of the event file 
	 */
	if (argc < 3)
		exit (-1);

	/* Read in the Evt structure */
#ifdef _WINNT
	sprintf (filename, "%s\\%s", argv[1], argv[2]);
#else
	sprintf (filename, "%s/%s", argv[1], argv[2]);
#endif _WINNT

	if ((fp = fopen (filename, "rb")) == NULL)
	{
		exit (-1);
	}

	/* Read the contents of the evt struct file*/
	fread ((void *) &EvtInfo, sizeof (EWEventInfoStruct), 1, fp);

	/* Read the channel info structs */
	if ((EvtInfo.pChanInfo = (EWChannelDataStruct *) malloc
				(EvtInfo.iNumChans * sizeof (EWChannelDataStruct))) == NULL)
	{
		fclose (fp);
		exit (-1);
	}
	EvtInfo.iNumAllocChans = EvtInfo.iNumChans;

	fread ((void *) EvtInfo.pChanInfo, sizeof (EWChannelDataStruct), 
													EvtInfo.iNumChans, fp);
	fclose (fp);

	/* 
	 * DoCheck function performs specific criteria checks.
	 * If it returns SUCCESS, then we overwrite the Evt file
	 * with the idEvent being negative of what it originally
	 * was, to signal the caller that the rule fired.
	 * Otherwise, exit.
	 *
	 * To implement a different set of criteria, only the DoCheck
	 * function needs to be changed.
	 */ 
	if (DoCheck(&EvtInfo) != EW_SUCCESS)
	{
		exit (-1);
	}

	EvtInfo.Event.idEvent = -EvtInfo.Event.idEvent;
	if ((fp = fopen (filename, "wb")) == NULL)
	{
		exit (-1);
	}
	
	fwrite ((void *) &EvtInfo, sizeof (EWEventInfoStruct), 1, fp);

	/* append channel info structs */
	fwrite ((void *) EvtInfo.pChanInfo, sizeof (EWChannelDataStruct),
														EvtInfo.iNumChans, fp);
	fclose (fp);
}



/*********************************************************
*
* This function performs one or more criteria checks on 
* the event. This is used to raise alarms. It returns
* EW_SUCCESS if the events conforms to the criteria, 
* otherwise it returns EW_FAILURE
*
*********************************************************/

int	DoCheck(EWEventInfoStruct *pEvt)
{

	int			i, NumSides, ret;
	float		*xRect;
	float		*yRect;
	float		x, y;
	double		MB_ALARM, ML_ALARM;

	if (pEvt == NULL)
		return EW_FAILURE;

	MB_ALARM = 5.0;
	ML_ALARM = 4.0;

	/* 
	 * If the event was in the Western US, and we have an
	 * ML computed, alarm if the ML value is over ML_ALARM.
	 *
	 * Otherwise, alarm if MB value is over MB_ALARM.
	 */


	/* INST_MENLO polygon */
	NumSides = 9;
	if ((xRect = (float *) malloc (NumSides * sizeof (float))) == NULL)
		return EW_FAILURE;
	if ((yRect = (float *) malloc (NumSides * sizeof (float))) == NULL)
		return EW_FAILURE;

	xRect[0] = (float)36.68; yRect[0] = (float)-117.79;
	xRect[1] = (float)37.75; yRect[1] = (float)-118.25;
	xRect[2] = (float)37.75; yRect[2] = (float)-119.50;
	xRect[3] = (float)39.50; yRect[3] = (float)-120.75;
	xRect[4] = (float)42.00; yRect[4] = (float)-121.41;
	xRect[5] = (float)42.00; yRect[5] = (float)-122.70;
	xRect[6] = (float)43.02; yRect[6] = (float)-125.00;
	xRect[7] = (float)40.00; yRect[7] = (float)-125.50;
	xRect[8] = (float)34.69; yRect[8] = (float)-121.37;
	xRect[9] = (float)36.68; yRect[9] = (float)-117.79;

	x = pEvt->PrefOrigin.dLat;
	y = pEvt->PrefOrigin.dLon;
	ret = PolygonArea(NumSides, xRect, yRect, &x, &y);

	free (xRect);
	free (yRect);

	if (ret == 1)
	{
		/* Event was in the Menlo polygon, check for ML */
		for (i = 0; i < pEvt->iNumMags; i++)
		{
			if ((pEvt->Mags[i].iMagType == MAGTYPE_LOCAL_PEAK2PEAK) ||
				(pEvt->Mags[i].iMagType == MAGTYPE_LOCAL_ZERO2PEAK))
			{
				if (pEvt->Mags[i].dMagAvg >= ML_ALARM)
				{
					/* BINGO:  we are in the Western US, and we have
					 * an ML value over the alarm threshhold -- sound 
					 * the alarm.
					 */

					return EW_SUCCESS;
				}
			}
		}

		/* If we got this far, there was no ML magnitude value
		 * over the alarm threshhold -- no alarm
		 */

		return EW_FAILURE;
	}
	else
	{
		/* Event was outside of the Menlo polygon, check for MB */
		for (i = 0; i < pEvt->iNumMags; i++)
		{
			if (pEvt->Mags[i].iMagType == MAGTYPE_BODYWAVE)
			{
				if (pEvt->Mags[i].dMagAvg >= MB_ALARM)
				{
					/* BINGO:  we are outside the Western US, and we have
					 * an MB value over the alarm threshhold -- sound 
					 * the alarm.
					 */

					return EW_SUCCESS;
				}
			}
		}

		/* If we got this far, there was no ML magnitude value
		 * over the alarm threshhold -- no alarm
		 */

		return EW_FAILURE;
	}

}  /* end DoCheck() */
