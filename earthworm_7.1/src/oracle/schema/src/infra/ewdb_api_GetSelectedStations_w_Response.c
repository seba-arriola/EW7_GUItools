/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_api_GetSelectedStations_w_Response.c,v 1.3 2005/06/10 16:27:59 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_api_GetSelectedStations_w_Response.c,v $
 *     Revision 1.3  2005/06/10 16:27:59  davidk
 *     DB API Cleanup
 *
 *     Revision 1.2  2003/12/03 00:23:29  davidk
 *     Modified the check of the return code for ewdb_api_GetTransformFunctionForChan().
 *     Now checks for !SUCCESS, instead of for FAILURE.
 *
 *     Revision 1.1  2003/03/27 17:53:02  davidk
 *     Initial revision
 *
 *     Revision 1.3  2001/07/14 07:53:03  davidk
 *     Added code for handling a WARNING return code from the GetTransformFunction call,
 *     so that the function does not log lots of error messages each time a channel
 *     doesn't have a transfer function (quite common).
 *
 *     Revision 1.2  2001/07/01 21:55:40  davidk
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
 *     Revision 1.1  2001/05/15 02:16:31  davidk
 *     Initial revision
 *
 *     Revision 1.1  2001/02/28 17:29:10  lucky
 *     Initial revision
 *
 *
 */

#include <ewdb_ora_api.h>
#include <internal/ewdb_internal_functions.h>
/********************************************************************
********************************************************************/
int ewdb_api_GetSelectedStations_w_Response(EWDB_ChannelStruct * pBuffer, int iBufferLen,
                                            EWDB_ChannelStruct * pCSCriteria, 
                                            EWDB_ChannelStruct * pCSMaxCriteria, 
                                            int iCriteria,
                                            int * pNumStationsFound, 
                                            int * pNumStationsRetrieved)

{
  int iRetCode;
  int OUT_iRetCode;
  int i;

  if (!(pBuffer && pCSCriteria &&  pCSMaxCriteria &&  pNumStationsFound &&  pNumStationsRetrieved))
  {
    logit ("", "ewdb_api_GetSelectedStations_w_Response():Null pointer passed in!\n");
    return EWDB_RETURN_FAILURE;
  }

  if (iBufferLen <= 0)
  {
    logit ("", "ewdb_api_GetSelectedStations_w_Response():Bad iBufferLen passed in!\n");
    return EWDB_RETURN_FAILURE;
  }

  
  /* Get the list of stations that meet the caller's criteria */
  iRetCode = ewdb_api_GetSelectedStations(pBuffer, iBufferLen, pCSCriteria, pCSMaxCriteria,
                                          iCriteria, pNumStationsFound, pNumStationsRetrieved);

  if(iRetCode == EWDB_RETURN_FAILURE)
  {
    logit("","ewdb_api_GetSelectedStations_w_Response():  ERROR!! Call to "
             "ewdb_api_GetSelectedStations() failed!\n");
    return(EWDB_RETURN_FAILURE);
  }
  else
  {
    OUT_iRetCode = iRetCode;
  }

  for(i=0; i < *pNumStationsFound; i++)
  {
    pBuffer[i].pChanProps = (EWDB_ChanTCTFStruct*) calloc(1,sizeof(EWDB_ChanTCTFStruct));
    if(pBuffer[i].pChanProps == NULL)
    {
      logit("","ewdb_api_GetSelectedStations_w_Response():  ERROR!! Failed on %dth malloc of %d bytes!",
            i+1, sizeof(EWDB_ChanTCTFStruct));
      return(EWDB_RETURN_FAILURE);
    }
    iRetCode = ewdb_api_GetTransformFunctionForChan(pBuffer[i].Comp.idChan, 
                                                    (time_t)(pBuffer[i].tOn) + 1, 
                                                    pBuffer[i].pChanProps);
    if(iRetCode != EWDB_RETURN_SUCCESS)
    {
      free(pBuffer[i].pChanProps);
      pBuffer[i].pChanProps = NULL;
      OUT_iRetCode = EWDB_RETURN_WARNING;
    }
  }

/*
  /* Get the CookedTF info for the idCookedTF (EWDB_TransformFunctionStruct)* /
  RetCode=ewdb_internal_GetTransformFunction(ChanCTF.tfsFunc.idCookedTF, &(ChanCTF.tfsFunc));
  if(RetCode == EWDB_RETURN_FAILURE)
  {
    logit("","GetTransformFunctionForChan(): Error in %s.\n",
          "ewdb_internal_GetTransformFunction()");
    return(RetCode);
  }
*/
  return(OUT_iRetCode);
}   /* end ewdb_api_GetSelectedStations_w_Response() */



