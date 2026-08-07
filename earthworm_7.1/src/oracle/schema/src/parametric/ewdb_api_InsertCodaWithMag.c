/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_api_InsertCodaWithMag.c,v 1.5 2005/06/15 19:03:12 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_api_InsertCodaWithMag.c,v $
 *     Revision 1.5  2005/06/15 19:03:12  davidk
 *     DB API Cleanup
 *
 *     Revision 1.4  2003/03/13 19:31:16  lucky
 *     changed CreateStaMag from ewdb_internal to ewdb_api
 *
 *     Revision 1.3  2001/07/01 21:55:37  davidk
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
 *     Revision 1.2  2001/05/24 00:58:23  davidk
 *     Added code and a comment to copy the idChan from the main StaMag struct to
 *     the CodaDur substruct.
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
/* ewdb_api_InsertCodaWithMag() performs a full insertion of a coda,
   complete with how that coda (duration) contributed to an overall 
   duration magnitude.  This should be an API function because it is the
   official way of inserting a station magnitude based on a coda duration. */

   /* the use of ewdb_internal_CreateTCoda() and ewdb_internal_CreateCodaDur(), and
      ewdb_internal_CreateStaMag() by functions outside the API should be 
      highly discouraged.  Those functions are only provided for applications 
      with special needs, written by people that know
      exactly how to use them.  (I don't think I even qualify for that.) */
int ewdb_api_InsertCodaWithMag(EWDB_StationMagStruct *pCodaDur)
{

	if (pCodaDur == NULL)
	{
		logit ("", "ewdb_api_InsertCodaWithMag(): Null parameters passed in!\n");
		return EWDB_RETURN_FAILURE;
	}

  /* Make sure that the idChan in the CodaDur struct matches the one
     in the main stationmag struct */
  pCodaDur->StaMagUnion.CodaDur.idChan = pCodaDur->idChan;

	/************************* 
	 *   Create_TCoda        *
	 *************************/

	if (ewdb_internal_CreateTCoda (&(pCodaDur->StaMagUnion.CodaDur)) != EWDB_RETURN_SUCCESS)
	{
		logit ("", "ewdb_api_InsertCodaWithMag(): ewdb_internal_CreateTCoda failed!\n");
		return EWDB_RETURN_FAILURE;
	}
	if (pCodaDur->StaMagUnion.CodaDur.idTCoda < 0)
	{
		logit("", "ewdb_api_InsertCodaWithMag(): ewdb_internal_CreateTCoda returned ERROR: idTCoda=%d!\n",
          pCodaDur->StaMagUnion.CodaDur.idTCoda);
		return EWDB_RETURN_FAILURE;
	}



	/************************* 
	 *   Create_Dur_Coda     *
	 *************************/
	if (ewdb_internal_CreateCodaDur (&(pCodaDur->StaMagUnion.CodaDur)) != EWDB_RETURN_SUCCESS)
	{
		logit ("", "ewdb_api_InsertCodaWithMag(): ewdb_internal_CreateCodaDur failed!\n");
		return EWDB_RETURN_FAILURE;
	}
	if (pCodaDur->StaMagUnion.CodaDur.idCodaDur < 0)
	{
		logit("", "ewdb_api_InsertCodaWithMag(): ewdb_internal_CreateCodaDur()"
              " returned ERROR: idCodaDur=%d!\n",
          pCodaDur->StaMagUnion.CodaDur.idCodaDur);
		return EWDB_RETURN_FAILURE;
	}

	/************************* 
	 *    Create_Sta_Mag     *
	 *************************/
	pCodaDur->idDatum = pCodaDur->StaMagUnion.CodaDur.idCodaDur;

	if (ewdb_api_CreateStaMag (pCodaDur) != EWDB_RETURN_SUCCESS)
	{
		logit ("", "ewdb_api_InsertCodaWithMag(): ewdb_api_CreateStaMag() failed!\n");
		return EWDB_RETURN_FAILURE;
	}
	if (pCodaDur->idMagLink < 0)
	{
		logit ("", "ewdb_api_InsertCodaWithMag(): ewdb_api_CreateStaMag returned ERROR: idMagLink=%d!\n",
           pCodaDur->idMagLink);
		return EWDB_RETURN_FAILURE;
	}

	return EWDB_RETURN_SUCCESS;

}  /* ewdb_api_InsertCodaWithMag() */


