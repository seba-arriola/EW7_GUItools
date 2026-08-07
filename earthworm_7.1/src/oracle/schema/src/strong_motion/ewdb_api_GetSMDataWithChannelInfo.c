/* ewdb_api_GetSMDataWithChannelInfo.c */

#include <stdlib.h>
#include <stdio.h>
#include <ewdb_ora_api.h>
#include <ewdb_sm_internal.h>


/* Used to retrieve Strong Motion messages, along with 
   associated channel information for each message from
   the DB.  All structs within the EWDB_SMChanAllStruct 
   will be completely filled in.  */
int ewdb_api_GetSMDataWithChannelInfo(EWDB_CriteriaStruct * pcsCriteria,
                                      char * szSta, char * szComp,
                                      char * szNet, char * szLoc,
                                      EWDBid idEvent, int iEventAssocFlag,
                                      EWDB_SMChanAllStruct * pSMCAS, 
                                      int BufferRecLen,
                                      int * pNumRecordsFound, 
                                      int * pNumRecordsRetrieved)
{
  int iQueryType;
  char szFunctionName[] = "ewdb_api_GetSMDataWithChannelInfo()";

  if(pcsCriteria->Criteria & EWDB_CRITERIA_USE_IDEVENT  && idEvent <= 0)
  {
    logit("","%s: ERROR: EWDB_CRITERIA_USE_IDEVENT is set to on, but idEvent is "
          "invalid(%d)!  Returning.\n",
          szFunctionName, idEvent);
    return(EWDB_RETURN_FAILURE);
  }

  if(pcsCriteria->Criteria & EWDB_CRITERIA_USE_IDEVENT)
  {
    iQueryType = EWDB_SM_SELECT_SMMESSAGES_W_CHANNEL_INFO_BY_EVENT;
  }
  else if(iEventAssocFlag == EWDB_SM_SEARCH_FOR_ALL_SMMESSAGES)
    iQueryType = EWDB_SM_SELECT_SMMESSAGES_W_CHANNEL_INFO;
  else if(iEventAssocFlag == EWDB_SM_SEARCH_FOR_ALL_UNASSOCIATED_MESSAGES)
    iQueryType = EWDB_SM_SELECT_SMMESSAGES_W_CHANNEL_INFO_WITHOUT_EVENT;
  else
  {
    logit("","%s: ERROR: iEventAssocFlag is set to an unsupported value(%d)\n",
          szFunctionName, iEventAssocFlag);
    return(EWDB_RETURN_FAILURE);
  }

  return(ewdb_internal_GetSMMessages(pcsCriteria, szSta, szComp, szNet, szLoc,
                                     idEvent, iQueryType,                                 
                                     "EWDB_SMChanAllStruct", 
                                     pSMCAS, BufferRecLen, 
                                     pNumRecordsFound, pNumRecordsRetrieved));
}  /* end ewdb_api_GetSMDataWithChannelInfo() */

