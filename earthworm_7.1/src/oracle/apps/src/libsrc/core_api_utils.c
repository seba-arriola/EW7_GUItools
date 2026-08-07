/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: core_api_utils.c,v 1.3 2004/09/09 17:37:14 davidk Exp $
 *    Revision history:
 *
 *    $Log: core_api_utils.c,v $
 *    Revision 1.3  2004/09/09 17:37:14  davidk
 *    Removed reference to ewdb_ora_api_ioc.h.
 *    No hydra references belong in earthworm.
 *
 *    Revision 1.2  2003/10/20 17:06:17  mark
 *    #included ewdb_ora_api_ioc.h for compiling w/ Visual C++
 *
 *    Revision 1.1  2003/04/15 15:58:05  lucky
 *    Initial revision
 *
 */


#include <ewdb_apps_utils.h>

/*************************** ewdb_api_InsertPeakAmpWithMag*********************
*
*  This is a composite utility function which does a Texas-two step
*  necessary for insertion of a peak amplitude and its connection with
*  station magnitude.  We do the following:
*    a)  Insert the individual PeakAmp record
*    b)  Link the PeakAmp record with Station magnitude record
*************************** ewdb_api_InsertPeakAmpWithMag*********************/

int ewdb_apps_InsertPeakAmpWithMag (EWDB_StationMagStruct *pStaMag)
{
	if (pStaMag == NULL)
	{
		logit ("", "Null parameters passed in!\n");
		return EWDB_RETURN_FAILURE;
	}

	/************************* 
	 *   Create PeakAmp      *
	 *************************/
	pStaMag->StaMagUnion.PeakAmp.idChan = pStaMag->idChan;
	pStaMag->StaMagUnion.PeakAmp.iAmpType = pStaMag->iMagType;

	if (ewdb_api_CreatePeakAmp (&(pStaMag->StaMagUnion.PeakAmp)) != EWDB_RETURN_SUCCESS)
	{
		logit ("", "Call to ewdb_api_CreatePeakAmp failed!\n");
		return EWDB_RETURN_FAILURE;
	}

	if (pStaMag->StaMagUnion.PeakAmp.idPeakAmp < 0)
	{
		logit ("", "ewdb_api_CreatePeakAmp returned ERROR: idPeakAmp=%d!\n",
                                pStaMag->StaMagUnion.PeakAmp.idPeakAmp);
		return EWDB_RETURN_FAILURE;
	}


	/************************* 
	 *   Create_Sta_Mag      *
	 *************************/
	pStaMag->idDatum = pStaMag->StaMagUnion.PeakAmp.idPeakAmp;
	if (ewdb_api_CreateStaMag (pStaMag) != EWDB_RETURN_SUCCESS)
	{
		logit ("", "Call to ewdb_internal_CreateStaMag failed!\n");
		return EWDB_RETURN_FAILURE;
	}

	if (pStaMag->idMagLink < 0)
	{
		logit ("", "ewdb_api_CreateStaMag returned ERROR: idMagLink=%d!\n",
                                pStaMag->idMagLink);
		return EWDB_RETURN_FAILURE;
	}


	return EWDB_RETURN_SUCCESS;

}  /* ewdb_api_InsertPeakAmpWithMag() */
