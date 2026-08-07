
#include <ewdb_cli_base.h>
#include <ewdb_ora_api.h>

int ewdb_api_GetAmpsByOrigin(EWDBid IN_idOrigin, MAGNITUDE_TYPE iMagType,
                             EWDB_PeakAmpStruct * pAmpBuffer,
                             EWDB_StationStruct * pStationBuffer,
                             int * pNumAmpsFound, int * pNumAmpsRetrieved,
                             int iBufferLen)
{

  EWDBid idEvent;
  int rc;

  if(IN_idOrigin <= 0)
  {
    logit("","ewdb_api_GetAmpsByOrigin():  bad IN_idOrigin(%d). \n", IN_idOrigin);
    return(EWDB_RETURN_FAILURE);
  }

  rc = ewdb_api_GetEventForOrigin(IN_idOrigin, &idEvent);

  if(rc!= EWDB_RETURN_SUCCESS)
  {
    logit("","ewdb_api_GetAmpsByOrigin():  error(%d) in call to ewdb_api_GetEventForOrigin(%d). \n", IN_idOrigin);
    return(EWDB_RETURN_FAILURE);
  }

  return(ewdb_api_GetAmpsByEvent(idEvent, iMagType, pAmpBuffer, pStationBuffer, 
                                 pNumAmpsFound, pNumAmpsRetrieved, iBufferLen));


}  /* end ewdb_api_GetAmpsByOrigin() */



