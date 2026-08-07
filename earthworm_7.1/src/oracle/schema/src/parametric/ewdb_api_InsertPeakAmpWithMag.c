/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_api_InsertPeakAmpWithMag.c,v 1.3 2005/06/15 19:03:12 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_api_InsertPeakAmpWithMag.c,v $
 *     Revision 1.3  2005/06/15 19:03:12  davidk
 *     DB API Cleanup
 *
 *     Revision 1.2  2001/07/01 21:55:38  davidk
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
 *     Revision 1.1  2001/05/15 02:16:22  davidk
 *     Initial revision
 *
 *
 *
 */

#include <ewdb_ora_api.h>
#include <ewdb_ew_oci_base.h>
#include <ewdb_internal_functions.h>

/********************************************************************
********************************************************************/
/* ewdb_api_InsertPeakAmpWithMag() performs a full insertion of an amplitude,
   complete with how that amplitude contributed to an overall 
   magnitude.  This should be an API function because it is the
   official way of inserting a station magnitude based on an amplitude. */

   /* the use of ewdb_internal_CreatePeakAmp() and ewdb_internal_CreateStaMag() by functions
      outside the API should be highly discouraged.  Those functions are only 
      provided for applications with special needs, written by people that know
      exactly how to use them.  (I don't think I even qualify for that.) */
int ewdb_api_InsertPeakAmpWithMag(EWDB_StationMagStruct *pPeakAmp)

{

	if (pPeakAmp == NULL)
	{
		logit ("", "ewdb_api_InsertPeakAmpWithMag(): Null parameters passed in!\n");
		return EWDB_RETURN_FAILURE;
	}


	/************************* 
	 *   Create_Body_Amp     *
	 *************************/
	if (ewdb_internal_CreatePeakAmp (pPeakAmp) != EWDB_RETURN_SUCCESS)
	{
		logit ("", "Call to ewdb_internal_CreatePeakAmp failed!\n");
		return EWDB_RETURN_FAILURE;
	}

	if (pPeakAmp->StaMagUnion.PeakAmp.idPeakAmp < 0)
	{
		logit ("", "ewdb_api_InsertPeakAmpWithMag(): ewdb_internal_CreatePeakAmp():"
               " returned ERROR: idPeakAmp=%d!\n",
           pPeakAmp->StaMagUnion.PeakAmp.idPeakAmp);
		return EWDB_RETURN_FAILURE;
	}

	/************************* 
	 *   Create_Sta_Mag      *
	 *************************/
	pPeakAmp->idDatum = pPeakAmp->StaMagUnion.PeakAmp.idPeakAmp;
	if (ewdb_internal_CreateStaMag (pPeakAmp) != EWDB_RETURN_SUCCESS)
	{
		logit ("", "Call to ewdb_internal_CreateStaMag failed!\n");
		return EWDB_RETURN_FAILURE;
	}

	if (pPeakAmp->idMagLink < 0)
	{
		logit ("", "ewdb_api_InsertPeakAmpWithMag(): ewdb_internal_CreateStaMag"
               " returned ERROR: idMagLink=%d!\n",
           pPeakAmp->idMagLink);
		return EWDB_RETURN_FAILURE;
	}

	return EWDB_RETURN_SUCCESS;
}  /* ewdb_api_InsertPeakAmpWithMag() */
