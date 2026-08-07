/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_api_GetAmpsByTime.c,v 1.3 2005/06/15 19:03:12 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_api_GetAmpsByTime.c,v $
 *     Revision 1.3  2005/06/15 19:03:12  davidk
 *     DB API Cleanup
 *
 *     Revision 1.2  2003/12/09 23:44:54  michelle
 *     lucky changed amp type to mag type on lines 5, 23, 45 and 52
 *
 *     Revision 1.1  2003/08/25 18:00:42  lucky
 *     Initial revision
 *
 *
 */


#include <ewdb_cli_base.h>
#include <ewdb_ora_api.h>
#include <ewdb_internal_functions.h>

int ewdb_api_GetAmpsByTime(time_t tStart, time_t tEnd, MAGNITUDE_TYPE iMagType,
                           EWDB_PeakAmpStruct * pAmpBuffer,
                           EWDB_StationStruct * pStationBuffer,
                           int * pNumAmpsFound, int * pNumAmpsRetrieved,
                           int iBufferLen)
{
  if(tStart > tEnd)
  {
    logit("","ewdb_api_GetAmpsByTime():  bad tStart, tEnd (%.2f - %.2f). \n", 
          tStart, tEnd);
    return(EWDB_RETURN_FAILURE);
  }

  if(!pAmpBuffer || iBufferLen<=0)
  {
    logit("","ewdb_api_GetAmpsByTime():  bad pAmpBuffer or iBufferLen (%u, %d).\n", 
          pAmpBuffer, iBufferLen);
    return(EWDB_RETURN_FAILURE);
  }

  if(pStationBuffer)
  {
    return(ewdb_internal_GetAmpsByTime_w_StaInfo(tStart, tEnd, iMagType, 
                                                 pAmpBuffer, pStationBuffer, 
                                                 pNumAmpsFound, pNumAmpsRetrieved, 
                                                 iBufferLen));
  }
  else
  {
    return(ewdb_internal_GetAmpsByTime(tStart, tEnd, iMagType, 
                                       pAmpBuffer,
                                       pNumAmpsFound, pNumAmpsRetrieved, 
                                       iBufferLen));
  }

}  /* end ewdb_api_GetAmpsByTime() */

