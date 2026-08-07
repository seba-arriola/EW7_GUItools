/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_api_GetTransformFunctionForChan.c,v 1.5 2005/06/10 16:27:59 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_api_GetTransformFunctionForChan.c,v $
 *     Revision 1.5  2005/06/10 16:27:59  davidk
 *     DB API Cleanup
 *
 *     Revision 1.4  2003/11/11 17:51:37  davidk
 *     Added code to copy the idChanT and anything else in our copy of the ChanCTF struct back
 *     to the caller's, even when we don't find a Transform function.
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
#include <ewdb_ew_oci_base.h>
#include <internal/ewdb_internal_functions.h>
/********************************************************************
********************************************************************/
int ewdb_api_GetTransformFunctionForChan(EWDBid idChan, time_t tTime,
                                         EWDB_ChanTCTFStruct * pChanCTF)
{
  EWDBid idChanT;
  EWDB_ChanTCTFStruct ChanCTF;
  int RetCode;
  static double dTime;
  
  if (pChanCTF== NULL)
  {
    logit ("", "ewdb_api_GetTransformFunctionForChan():Null pointer passed in!\n");
    return EWDB_RETURN_FAILURE;
  }

  /* Get the idChanT for the idChan/Time  (idChanT)*/
  RetCode=ewdb_api_GetidChanT(idChan,(double)tTime, &idChanT);
  if(RetCode == EWDB_RETURN_FAILURE)
  {
    logit("","ewdb_api_GetTransformFunctionForChan(): Error in %s.\n",
          "ewdb_api_GetidChanT()");
    return(RetCode);
  }

  /* Initialize ChanCTF and set the idChanT using the result of ewdb_api_GetidChanT() */
  memset(&ChanCTF, 0, sizeof(ChanCTF));
  ChanCTF.idChanT = idChanT;

  /* Get the ChanCTF info for the idChanT (idCookedTF,dGain,dSampRate) */
  RetCode=ewdb_internal_GetChanCTFForChannel(&ChanCTF);
  if(RetCode == EWDB_RETURN_FAILURE)
  {
    logit("","ewdb_api_GetTransformFunctionForChan(): Error in %s.\n",
          "ewdb_internal_GetChanCTFForChannel()");
    return(RetCode);
  }
  else if(RetCode == EWDB_RETURN_WARNING)
  {
    /* There is no transfer function for this channel. 
       ABORT!!!!  But do it quietly, as this will happen
       frequently. 
     ****************************************************/
    /* copy our ChanCTF struct back to the caller's, so
       the idChanT and anything else we discovered is copied back */
    memcpy(pChanCTF,&ChanCTF,sizeof(ChanCTF));
    return(RetCode);
  }

  /* Get the CookedTF info for the idCookedTF (EWDB_TransformFunctionStruct)*/
  RetCode=ewdb_internal_GetTransformFunction(ChanCTF.tfsFunc.idCookedTF, &(ChanCTF.tfsFunc));
  if(RetCode == EWDB_RETURN_FAILURE)
  {
    logit("","GetTransformFunctionForChan(): Error in %s.\n",
          "ewdb_internal_GetTransformFunction()");
    return(RetCode);
  }

  /* Get the CookedTF Description for the Transform function */
  RetCode=ewdb_internal_GetTransformFunctionDesc(ChanCTF.tfsFunc.idCookedTF, ChanCTF.tfsFunc.szCookedTFDesc);
  if(RetCode == EWDB_RETURN_FAILURE)
  {
    logit("","ewdb_api_GetTransformFunctionForChan(%d/%d): Error in %s.\n",
          idChan, tTime, "ewdb_internal_GetTransformFunctionDesc()");
    return(RetCode);
  }
  
  /* copy retrieved data back to user's buffer */
  memcpy(pChanCTF,&ChanCTF,sizeof(ChanCTF));

  return(EWDB_RETURN_SUCCESS);
} /* end ewdb_api_GetTransformFunctionForChan() */


