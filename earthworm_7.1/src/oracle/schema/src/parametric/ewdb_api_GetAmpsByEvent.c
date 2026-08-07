
#include <ewdb_cli_base.h>
#include <ewdb_ora_api.h>
#include <ewdb_internal_functions.h>


int ewdb_api_GetAmpsByEvent(EWDBid IN_idEvent, MAGNITUDE_TYPE IN_iMagType,
                            EWDB_PeakAmpStruct * pAmpBuffer,
                            EWDB_StationStruct * pStationBuffer,
                            int * pNumAmpsFound, int * pNumAmpsRetrieved,
                            int iBufferLen)
{

  if(IN_idEvent <= 0)
  {
    logit("","ewdb_api_GetAmpsByEvent():  bad IN_idEvent(%d). \n", IN_idEvent);
    return(EWDB_RETURN_FAILURE);
  }

  if(!pAmpBuffer || iBufferLen<=0)
  {
    logit("","ewdb_api_GetAmpsByEvent():  bad pAmpBuffer or iBufferLen (%u, %d).\n", 
          pAmpBuffer, iBufferLen);
    return(EWDB_RETURN_FAILURE);
  }

  if(pStationBuffer)
  {
    return(ewdb_internal_GetAmpsByEvent_w_StaInfo(IN_idEvent, IN_iMagType, 
                                                 pAmpBuffer, pStationBuffer, 
                                                 pNumAmpsFound, pNumAmpsRetrieved, 
                                                 iBufferLen));
  }
  else
  {
    return(ewdb_internal_GetAmpsByEvent(IN_idEvent, IN_iMagType, pAmpBuffer,
                                        pNumAmpsFound, pNumAmpsRetrieved, 
                                        iBufferLen));
  }

}  /* end ewdb_api_GetAmpsByEvent() */




