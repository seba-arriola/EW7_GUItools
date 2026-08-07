
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: utah_polygon.c,v 1.3 2001/08/06 20:22:31 lucky Exp $
 *
 *    Revision history:
 *     $Log: utah_polygon.c,v $
 *     Revision 1.3  2001/08/06 20:22:31  lucky
 *     Added NT support
 *
 *     Revision 1.2  2001/07/01 21:55:07  davidk
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
 *     Revision 1.1  2001/05/18 19:08:03  lucky
 *     Initial revision
 *
 *     Revision 1.1  2001/05/15 02:14:59  davidk
 *     Initial revision
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
	if (DoCheck (&EvtInfo) != EW_SUCCESS)
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

int		DoCheck (EWEventInfoStruct *pEvt)
{

	int			NumSides, ret;
	float		*xRect;
	float		*yRect;
	float		x, y;

	if (pEvt == NULL)
		return EW_FAILURE;


	/* INST_UTAH polygon */
	NumSides = 4;

	if ((xRect = (float *) malloc (NumSides * sizeof (float))) == NULL)
		return EW_FAILURE;
	if ((yRect = (float *) malloc (NumSides * sizeof (float))) == NULL)
		return EW_FAILURE;

	xRect[0] = (float)36.75; yRect[0] = (float)-108.75;
	xRect[1] = (float)36.75; yRect[1] = (float)-114.25;
	xRect[2] = (float)42.50; yRect[2] = (float)-114.25;
	xRect[3] = (float)42.50; yRect[3] = (float)-108.75;
	xRect[4] = (float)36.75; yRect[4] = (float)-108.75;

	x = pEvt->PrefOrigin.dLat;
	y = pEvt->PrefOrigin.dLon;
	ret = PolygonArea(NumSides, xRect, yRect, &x, &y);

	free (xRect);
	free (yRect);

	if (ret == 1)
		return EW_SUCCESS;
	else
		return EW_FAILURE;
}  /* end DoCheck() */
