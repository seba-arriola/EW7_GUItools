/* ewdb_internal_GetSMMessages.c */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <ewdb_ora_api.h>
#include <earthworm_simple_funcs.h>
#include <ewdb_sm_internal.h>


#define MAX_SMMOTIONS_PER_MESSAGE (SM_MAX_RSA + 3)


int ewdb_internal_GetSMMessages(EWDB_CriteriaStruct * pcsCriteria,
                                char * szSta, char * szComp,
                                char * szNet, char * szLoc,
                                EWDBid idEvent, int iQueryType, char * szStructType,
                                void * pBuffer, int BufferRecLen,
                                int * pNumRecordsFound, int * pNumRecordsRetrieved)
{

  EWDB_SMChanAllStruct * pSMBuffer;
  EWDB_SMMotionStruct * pSMMotion;
  int bBufferMalloced = FALSE;
  int rc;
  int iRetCode = EWDB_RETURN_SUCCESS;
  int i, j;
  int NumMotionRecordsFound, NumMotionRecordsRetrieved;


  SM_INFO * pSMI = pBuffer;

  if(iQueryType == EWDB_SM_SELECT_SMMESSAGES_W_CHANNEL_INFO ||
     iQueryType == EWDB_SM_SELECT_SMMESSAGES_W_CHANNEL_INFO_BY_EVENT ||
     iQueryType == EWDB_SM_SELECT_SMMESSAGES_W_INFO ||
     iQueryType == EWDB_SM_SELECT_SMMESSAGES_W_CHANNEL_INFO_WITHOUT_EVENT)
  {
    if(strcmp(szStructType, "EWDB_SMChanAllStruct"))
    {
      /* we need to allocate our own internal buffer */
      pSMBuffer = malloc(BufferRecLen * sizeof(EWDB_SMChanAllStruct));
      if(!pSMBuffer)
      {
        logit("","ewdb_internal_GetSMMessages() ERROR 1: failed to malloc %d bytes!\n",
          BufferRecLen * sizeof(EWDB_SMChanAllStruct));
        return(EWDB_RETURN_FAILURE);
      }
      bBufferMalloced = TRUE;
    }
    else
    {
      pSMBuffer = pBuffer;
    }

    /* now initialize the whole buffer in one foul swoop. */
    memset(pSMBuffer,0,sizeof(EWDB_SMChanAllStruct) * BufferRecLen);

  }
  else if(iQueryType == EWDB_SM_SELECT_SMMESSAGES_W_ID_ONLY)
  {
    logit("","ewdb_internal_GetSMMessages() ERROR 2: QueryType %s not supported!\n",
          "SELECT_SMMESSAGES_W_ID_ONLY");
    return(EWDB_RETURN_FAILURE);
  }
  else
  {
    logit("","ewdb_internal_GetSMMessages() ERROR 3: Invalid iQueryType(%d) "
             "passed in.\n",
          iQueryType);
    return(EWDB_RETURN_FAILURE);
  }

  pSMMotion = malloc(sizeof(EWDB_SMMotionStruct) * MAX_SMMOTIONS_PER_MESSAGE);
  if(!pSMMotion)
  {
        logit("","ewdb_internal_GetSMMessages() ERROR 4: failed to malloc %d bytes!\n",
              SM_MAX_RSA * sizeof(EWDB_SMMotionStruct));
        iRetCode = EWDB_RETURN_FAILURE;
        goto cleanup_memory;
  }

  rc = ewdb_internal_GetAllSMMessages(szSta, szComp, szNet, szLoc,
                                      idEvent, iQueryType, pcsCriteria,
                                      pSMBuffer, BufferRecLen,
                                      pNumRecordsFound, 
                                      pNumRecordsRetrieved);

  if(rc == EWDB_RETURN_FAILURE)
  {
    logit("","%s: ERROR! ewdb_internal_GetAllSMMessages() failed!\n Returning!\n",
          "ewdb_internal_GetSMMessages()");
    return(EWDB_RETURN_FAILURE);
  }
  else if(rc == EWDB_RETURN_WARNING)
  {
    iRetCode = EWDB_RETURN_WARNING;
  }

  for(i=0; i < *pNumRecordsRetrieved; i++)
  {
    /* initialize pga, pgv, and pgd, in case those measurements don't exist */
    pSMBuffer[i].SMChan.pga = SM_NULL;
    pSMBuffer[i].SMChan.pgv = SM_NULL;
    pSMBuffer[i].SMChan.pgd = SM_NULL;

    rc = ewdb_internal_GetSMMotionsForMessage(pSMBuffer[i].idSMMessage, pSMMotion,
                                              MAX_SMMOTIONS_PER_MESSAGE, 
                                              &NumMotionRecordsFound,
                                              &NumMotionRecordsRetrieved);

    if(rc == EWDB_RETURN_FAILURE)
    {
      logit("t","%s: ewdb_internal_GetSMMessages() failed for idSMMessage(%d)\n",
            "ewdb_internal_GetSMMessages()", pSMBuffer[i].idSMMessage);
      (*pNumRecordsRetrieved)--;
      memcpy(&pSMBuffer[i], &pSMBuffer[*pNumRecordsRetrieved], 
             sizeof(EWDB_SMChanAllStruct));
      i--;
      continue;
    }

    for(j=0; j < NumMotionRecordsRetrieved; j++)
    {
      if(pSMMotion[j].dPeriod == -1)
      {
        switch(pSMMotion[j].iMotionType)
        {
        case EWDB_SM_MOTION_TYPE_ACCELERATION:
          pSMBuffer[i].SMChan.pga = pSMMotion[j].dMeasurement;
          break;

        case EWDB_SM_MOTION_TYPE_VELOCITY:
          pSMBuffer[i].SMChan.pgv = pSMMotion[j].dMeasurement;
          break;

        case EWDB_SM_MOTION_TYPE_DISPLACEMENT:
          pSMBuffer[i].SMChan.pgd = pSMMotion[j].dMeasurement;
          break;

        default:
          logit("","ewdb_internal_GetSMMessages(): ERROR undefined Period(%f) "
                   "for peak motion! Ignoring!\n", 
                pSMMotion[j].iMotionType);
        }  /* end switch */
      }  /* if dPeriod = -1 */
      else
      {
        if(pSMMotion[j].iMotionType != EWDB_SM_MOTION_TYPE_ACCELERATION)
        {
          logit("","%s ERROR!  Invalid MotionType(%d) for non "
                   "peak value. Ignoring\n",
                "ewdb_internal_GetSMMessages()", pSMMotion[j].iMotionType);
        }
        else
        {
          pSMBuffer[i].SMChan.pdrsa[pSMBuffer[i].SMChan.nrsa] = 
            pSMMotion[j].dPeriod;
          pSMBuffer[i].SMChan.rsa[pSMBuffer[i].SMChan.nrsa] = 
            pSMMotion[j].dMeasurement;
          pSMBuffer[i].SMChan.nrsa++;
          if(pSMBuffer[i].SMChan.nrsa >= SM_MAX_RSA)
            /* we've caught our limit, so quit.  We should have
               already processed all peak values due to an ordering
               clause, so there is no risk of missing one */
            break;
        }
      }  /* end else from if(pSMMotion[i].dPeriod == -1) */
    }  /* end for iNumMotionRecords retrieved (for this message) */
  }  /* end for number of SMMessages retrieved */

  if(bBufferMalloced)
  {
    /* we malloced a buffer to handle the records, so now we have
       to copy them from our buffer to the client buffer, and have
       to handle a type change along the way. */
    if(strcmp(szStructType, "SM_INFO"))
    {
      logit("","%s ERROR!  Unsupported szStructType(%s)\n",
            "ewdb_internal_GetSMMessages()", szStructType);
      iRetCode = EWDB_RETURN_FAILURE;
    }
    else
    {
      for(i=0; i < *pNumRecordsRetrieved; i++)
      {
        memcpy(&pSMI[i],&(pSMBuffer[i].SMChan), sizeof(SM_INFO));
      }
    }
  }  /* end if bBufferMalloced */
       
  cleanup_memory:
  if(pSMMotion) 
    free(pSMMotion);
  if(bBufferMalloced)
    free(pSMBuffer);

  return(iRetCode);
}  /* end ewdb_internal_GetSMMessages() */
