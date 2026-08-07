/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_api_GetWaveformListByEvent.c,v 1.3 2001/07/01 22:21:19 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_api_GetWaveformListByEvent.c,v $
 *     Revision 1.3  2001/07/01 22:21:19  davidk
 *     Removed a semicolon from the beginning of the definition of ewdb_api_GetWaveformListByEvent(),
 *     so that it would be recognized as the function body and not as a prototype.
 *
 *     Revision 1.2  2001/07/01 21:55:44  davidk
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
 *     Revision 1.1  2001/05/15 02:16:44  davidk
 *     Initial revision
 *
 *     Revision 1.1  2001/02/28 17:29:10  lucky
 *     Initial revision
 *
 *
 */

#include <stdio.h>
#include <string.h>
#include <ewdb_ora_api.h>
#include <ewdb_ew_oci_base.h>
#include <internal/ewdb_internal_functions.h>
/********************************************************************
********************************************************************/
int ewdb_api_GetWaveformListByEvent(EWDBid idEvent, 
                                    EWDB_WaveformStruct * pWaveformBuffer,
                                    EWDB_StationStruct * pStationBuffer,
                                    int bIncludeStationInfo,
                                    int * pWaveformDescsFound,
                                    int * pWaveformDescsRetrieved,
                                    int BufferRecLen)

{
  int RetCode;

  ewdb_base_SetLastOraAPIActionTime();

  if(pWaveformBuffer == NULL)
  {
    logit("","pWaveformBuffer is NULL.");
    return(EWDB_RETURN_FAILURE);
  }

  if(bIncludeStationInfo && pStationBuffer == NULL)
  {
    logit("","pStationBuffer is NULL, and caller requested station info!");
    return(EWDB_RETURN_FAILURE);
  }

  if(bIncludeStationInfo)
  {
    RetCode=ewdb_internal_GetWaveformListWithCompT(idEvent,pWaveformBuffer,
                                         pStationBuffer,BufferRecLen);
    if(RetCode == EWDB_RETURN_FAILURE)
    {
      logit("","Call to ewdb_internal_GetWaveformListWithCompT() failed!");
      return(RetCode);
    }
    else if(RetCode < 0) 
    {
      *pWaveformDescsFound=0-RetCode;
      *pWaveformDescsRetrieved=BufferRecLen;
    }
    else
    {
      *pWaveformDescsFound=*pWaveformDescsRetrieved=RetCode;
    }
  }
  else /* !bIncludeStationInfo */
  {
    RetCode=ewdb_internal_GetWaveformList(idEvent,pWaveformBuffer,
                                 BufferRecLen);
    if(RetCode == EWDB_RETURN_FAILURE)
    {
      logit("","Call to ewdb_internal_GetWaveformList() failed!");
      return(RetCode);
    }
    else if(RetCode < 0) 
    {
      *pWaveformDescsFound=0-RetCode;
      *pWaveformDescsRetrieved=BufferRecLen;
    }
    else
    {
      *pWaveformDescsFound=*pWaveformDescsRetrieved=RetCode;
    }
  }

  ewdb_base_SetLastOraAPIActionTime();

  return(EWDB_RETURN_SUCCESS);
}  /* end ewdb_api_GetWaveformListByEvent() */
