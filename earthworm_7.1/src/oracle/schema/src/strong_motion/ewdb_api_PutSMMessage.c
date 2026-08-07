/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_api_PutSMMessage.c,v 
 *
 *    Revision history:
 *     $Log: ewdb_api_PutSMMessage.c,v $
 *     Revision 1.4  2001/07/26 17:05:57  davidk
 *     Changed the behavior of ewdb_api_PutSMMessage() so that if it cannot get
 *     an idChan or else a lat/lon for the idChan for the SCNL from which the
 *     message came, then it will not stuff the message.  Instead, it logs the
 *     message to the logfile, and it logs a note saying that the message
 *     wasn't stuffed.  This was requested by Alex for Utah for EW v6.0
 *
 *     Revision 1.3  2001/07/23 17:21:00  davidk
 *     Modified code to handle changes in ewdb_api_GetidChansFromSCNLT().
 *     Cleanpu up CreateSMMessage() comment.
 *
 *     Revision 1.2  2001/05/25 23:58:55  davidk
 *     Added T to the SCNL that gets logged whenever GetCompTForSCNLT fails.
 *
 *     Revision 1.1  2001/05/15 02:16:42  davidk
 *     Initial revision
 *
 *     Revision 1.1  2001/04/06 19:01:00  davidk
 *     Initial revision
 *
 *
 *************************************************************/


#include <stdlib.h>
#include <stdio.h>
#include <ewdb_ora_api.h>
#include <ewdb_ew_oci_base.h>
#include <earthworm_simple_funcs.h> 
#include <ewdb_sm_internal.h>


/********************************************************************
********************************************************************/
int ewdb_api_PutSMMessage(SM_INFO * pMessage, EWDBid idEvent)
{
  EWDBid idSMMessage;
  time_t    tNow;
  int j;
  int NumOfChans,NumOfChansFound;
  EWDBid  idChan;
  int RetCode;
  EWDBid idSMMotion;
  int rc;  /* return code */
  EWDB_StationStruct Station;  /* station struct for varifying lat/lon of
                                  channel before stuffing data */


  time(&tNow);

  if(pMessage->tload <= 0.0)
  {
	  /* a DB Load date/time has not yet been set.  Set it! */
	  pMessage->tload = tNow;
  }
  if(pMessage->altcode == SM_ALTCODE_NONE)
  {
	  pMessage->talt = tNow;
	  pMessage->altcode = SM_ALTCODE_DATABASE;
  }

  RetCode=ewdb_api_GetidChansFromSCNLT(&idChan,  pMessage->sta, pMessage->comp, 
                                       pMessage->net, pMessage->loc, pMessage->t,
                                       pMessage->t, &NumOfChansFound, &NumOfChans,
                                       1);


  /****************** begin changes by DK 2001/07/26 ******************/
  if(RetCode == EWDB_RETURN_FAILURE || NumOfChans < 1)
  {
    /* log SCNL, log Message, log non-insert! */
    logit("","ewdb_api_PutSMMessage():  No record of component from "
             "SCNL (%s,%s,%s,%s) in the EWDB.  Not inserting message "
             "from time %.2f!\n",
          pMessage->sta, pMessage->comp, pMessage->net, pMessage->loc, 
          pMessage->t
         );
    log_strongmotionII(pMessage);
    return(EWDB_RETURN_WARNING);

    /********************************************************************************
         COMMENT OUT OLD BEHAVIOR    DK 2001/07/26
    / * try to create one that starts at time pMessage->t and doesn't end * /
     RetCode=ewdb_api_CreateCompTForSCNLT(&idChan, pMessage->sta,
                                         pMessage->comp,
                                         pMessage->net,
                                         pMessage->loc,
                                        pMessage->t,EWDB_MAX_TIME
                                        );
    if(RetCode != EWDB_RETURN_SUCCESS)
    {
      logit("","%s: ERROR! Could not get idChan for (%s,%s,%s,%s:%.2f-MAX_TIME)! "
               "returning!\n",
            "ewdb_api_PutSMMessage()", 
            pMessage->sta, pMessage->comp, pMessage->net, pMessage->loc,pMessage->t);
      return(EWDB_RETURN_FAILURE);
    }
    ********************************************************************************/

  }  /* end if no chans back from ewdb_api_GetidChansFromSCNLT() */

  /* Make sure we have lat/lon for the channel */
  rc = ewdb_api_GetComponentInfo(idChan, (time_t)(pMessage->t), &Station);
  if((rc != EWDB_RETURN_SUCCESS) || (Station.Lat == 0 && Station.Lon == 0))
  {
    if(rc != EWDB_RETURN_SUCCESS)
    {
      logit("t","ewdb_api_PutSMMessage(): ERROR retrieving Component Info "
                "for idchan %d\n", 
            idChan);
    }

    /* log SCNL, log Message, log non-insert! */
    logit("","ewdb_api_PutSMMessage():  No lat/lon for component from "
             "SCNL (%s,%s,%s,%s) in the EWDB.  Not inserting message "
             "from time %.2f!\n",
          pMessage->sta, pMessage->comp, pMessage->net, pMessage->loc, 
          pMessage->t
         );
    log_strongmotionII(pMessage);
    return(EWDB_RETURN_WARNING);
  } /* end we do not have lat/lon for the channel */
  /****************** end  changes by DK 2001/07/26 ******************/


  /* make sure the peak times are set properly if the peak motions
     are NULL */
  if(pMessage->pga == SM_NULL)
    pMessage->tpga = 0;
  if(pMessage->pgv == SM_NULL)
    pMessage->tpgv = 0;
  if(pMessage->pgd == SM_NULL)
    pMessage->tpgd = 0;

  RetCode=ewdb_internal_CreateSMMessage(&idSMMessage, pMessage->t, 
                                        pMessage->tload,
	                                      pMessage->talt, pMessage->altcode, 
                                        pMessage->tpga, pMessage->tpgv, 
                                        pMessage->tpgd, idChan, idEvent);
  if(RetCode == EWDB_RETURN_FAILURE)
  {
    logit("","Call to ewdb_internal_CreateSMMessage() failed"
          "with sql retcode=%d.\n", idSMMessage);
    return(EWDB_RETURN_FAILURE);
  }

  /* insert pga if applicable */
  if(pMessage->pga != SM_NULL)
  {
    RetCode = 
      ewdb_internal_CreateSMMotion(&idSMMotion, idSMMessage, 
                                   EWDB_SM_MOTION_TYPE_ACCELERATION,
                                   EWDB_SM_PERIOD_PEAK_VALUE_CODE/*dPeriod*/,
                                   pMessage->pga);
    if(RetCode == EWDB_RETURN_FAILURE)
    {
      logit("","ewdb_internal_CreateSMMotion() failed for peak accel(idchan=%d) "
               "for idSMMessage=%d. Returning!!\n", 
            idChan,idSMMessage);
      return(EWDB_RETURN_FAILURE);
    }
  }  /* end if pga */
  
  /* insert pgv if applicable */
  if(pMessage->pgv != SM_NULL)
  {
    RetCode = 
      ewdb_internal_CreateSMMotion(&idSMMotion, idSMMessage, 
                                   EWDB_SM_MOTION_TYPE_VELOCITY,
                                   EWDB_SM_PERIOD_PEAK_VALUE_CODE/*dPeriod*/,
                                   pMessage->pgv);
    if(RetCode == EWDB_RETURN_FAILURE)
    {
      logit("","ewdb_internal_CreateSMMotion() failed for peak velocity(idchan=%d) "
               "for idSMMessage=%d. Returning!!\n", 
            idChan,idSMMessage);
      return(EWDB_RETURN_FAILURE);
    }
  }  /* end if pgv */

  /* insert pgd if applicable */
  if(pMessage->pgd != SM_NULL)
  {
    RetCode = 
      ewdb_internal_CreateSMMotion(&idSMMotion, idSMMessage, 
                                   EWDB_SM_MOTION_TYPE_DISPLACEMENT,
                                   EWDB_SM_PERIOD_PEAK_VALUE_CODE/*dPeriod*/,
                                   pMessage->pgd);
    if(RetCode == EWDB_RETURN_FAILURE)
    {
      logit("","ewdb_internal_CreateSMMotion() failed for peak displacement "
               "(idchan=%d), for idSMMessage=%d. Returning!!\n", 
            idChan,idSMMessage);
      return(EWDB_RETURN_FAILURE);
    }
  }  /* end if pgd */


  /* insert spectral values */
  for(j=0; j < pMessage->nrsa; j++)
  {
    RetCode = 
      ewdb_internal_CreateSMMotion(&idSMMotion, idSMMessage, 
                                   EWDB_SM_MOTION_TYPE_ACCELERATION,
                                   pMessage->pdrsa[j],
                                   pMessage->rsa[j]);
    if(RetCode == EWDB_RETURN_FAILURE)
    {
      logit("","ewdb_internal_CreateSMMotion() failed for rsa period(%d, %f) "
               "(idchan=%d), for idSMMessage=%d. Returning!!\n", 
            j, pMessage->rsa[j],idChan,idSMMessage);
      return(EWDB_RETURN_FAILURE);
    }
  }  /* end for RSA Period's */

  return(EWDB_RETURN_SUCCESS);

}
