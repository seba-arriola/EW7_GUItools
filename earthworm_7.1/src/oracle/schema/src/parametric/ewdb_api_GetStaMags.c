/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_api_GetStaMags.c,v 1.7 2005/06/15 19:03:12 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_api_GetStaMags.c,v $
 *     Revision 1.7  2005/06/15 19:03:12  davidk
 *     DB API Cleanup
 *
 *     Revision 1.6  2001/07/01 21:55:37  davidk
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
 *     Revision 1.5  2001/06/26 17:30:03  lucky
 *     Added and removed debugging statements
 *
 *     Revision 1.4  2001/05/22 01:16:01  davidk
 *     oops, got ERROR and FAILURE confused again.
 *
 *     Revision 1.3  2001/05/22 01:14:35  davidk
 *     fixed a problem, where the code did not know which type of station magnitudes
 *     to retrieve (coda or peakamp).
 *
 *     Revision 1.2  2001/05/15 02:16:22  davidk
 *     Moved functions around between the apps, DB API, and DB API INTERNAL
 *     levels.  Renamed functions and files.  Added support for amplitude
 *     magnitude types.  Reformatted makefiles.
 *
 *
 */

#include <ewdb_ora_api.h>
#include <rw_mag.h>
#include <ewdb_internal_functions.h>

int ewdb_api_GetStaMags(EWDBid idMagnitude, 
                        EWDB_StationMagStruct * pStaMags,
                        int * pNumStaMagsFound, 
                        int * pNumStaMagsRetrieved,
                        int BufferLen)
{
  EWDB_MagStruct MagStruct;
  int			 rc;

  if(ewdb_api_GetMagnitude(idMagnitude, &MagStruct) != EWDB_RETURN_SUCCESS)
  {
    logit("","ewdb_api_GetStaMags(): ERROR ewdb_api_GetMagnitude() failed!\n");
    return(EWDB_RETURN_FAILURE);
  }

  if(MagStruct.iMagType == MAGTYPE_DURATION)
  {
     rc = ewdb_internal_GetStaMagsForCodaDur(idMagnitude, pStaMags,
                                               pNumStaMagsFound,
                                               pNumStaMagsRetrieved, 
                                               BufferLen);
  }
  else
  {
     rc = ewdb_internal_GetStaMagsForPeakAmp(idMagnitude, 
                                               pStaMags, 
                                               pNumStaMagsFound,
                                               pNumStaMagsRetrieved, 
                                               BufferLen);
  }

   return(rc);

}  /* end ewdb_api_GetStaMags() */
  


