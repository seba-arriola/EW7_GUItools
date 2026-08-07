/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_api_ProcessSnippetReqs.c,v 1.1 2003/09/16 17:02:58 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_api_ProcessSnippetReqs.c,v $
 *     Revision 1.1  2003/09/16 17:02:58  davidk
 *     Initial revision
 *
 *     Revision 1.2  2002/08/26 16:44:31  davidk
 *     Added/modified code for initial jiggle implementation for NC.
 *
 *     Revision 1.1  2002/04/16 20:09:59  davidk
 *     Initial revision
 *
 *     Revision 1.4  2002/02/20 22:57:28  davidk
 *     Changed the number of attempts to 11 from 7.
 *
 *     Revision 1.3  2002/02/12 04:27:00  davidk
 *     Added code to add a channel to the database when the database has no existing knowledge
 *     of the station.
 *     Changed the parameters for scheduling snippet retrieval.
 *
 *     Revision 1.2  2001/07/23 16:52:18  davidk
 *     API cleanup.  Changed handling of return code from ewdb_api_GetidChansFromSCNLT().
 *
 *     Revision 1.1  2001/05/15 02:16:17  davidk
 *     Initial revision
 *
 *     Revision 1.1  2001/02/28 17:29:10  lucky
 *     Initial revision
 *
 *
 */

#include <ewdb_ora_api.h>
#include <ewdb_ew_oci_base.h>
#include <string.h>
#include <internal/ewdb_internal_functions.h>

# define IDCHAN_BUFFER_LEN 50

/********************************************************************
********************************************************************/

int ewdb_api_ProcessSnippetReqs(char * szSta,char * szComp, 
                                char * szNet, char * szLoc,
                                double tStart, double tEnd, EWDBid idEvent,
                                int iNumAttempts, int iRequestGroup)
{
  EWDBid pidChanBuffer[IDCHAN_BUFFER_LEN];
  int i, rc;
  int iNumidChansRetrieved, iNumidChansFound;
  EWDB_SnippetRequestStruct SnipReq;
  int bReturnFailure = FALSE;
  
  ewdb_base_SetLastOraAPIActionTime();

  rc=ewdb_api_GetidChansFromSCNLT(pidChanBuffer, szSta, szComp, szNet, szLoc,
                              tEnd, tStart, &iNumidChansFound,
                              &iNumidChansRetrieved, IDCHAN_BUFFER_LEN);

  if(rc != EWDB_RETURN_SUCCESS)
  {
    logit("t","Call to ewdb_api_GetidChansFromSCNLT returned code %d.\n");
  }
  else
  {
    if(iNumidChansRetrieved != 1)
    {
      logit("t","%s returned %d chans for station %s,%s,%s\n",
          "ewdb_api_GetidChansFromSCNLT()",iNumidChansRetrieved,szSta,szComp,szNet);
      /* Begin Change DK 02/06/20002 */
      if(iNumidChansRetrieved == 0)
      {
        rc = ewdb_api_CreateCompTForSCNLT(pidChanBuffer, szSta, szComp, szNet, szLoc,
                                          0.0, 9999999999.99);
        if(rc == EWDB_RETURN_SUCCESS)
        {
           logit("t","WARNING:  ewdb_api_ProcessSnippetReqs():  Created new channel(%d) "
                     " for (%s,%s,%s,%s), since one was not found!\n",
                 *pidChanBuffer, szSta, szComp, szNet, szLoc);
           iNumidChansRetrieved = 1;  /* dk 082402  need to set numidchansretrieved, so
                                         that we know we have 1 chan. */
        }
        else
        {
           logit("t","ERROR:  ewdb_api_ProcessSnippetReqs():  Failed to create new channel "
                     " for (%s,%s,%s,%s), when none was not found!\n",
                 szSta, szComp, szNet, szLoc);
           return(EWDB_RETURN_FAILURE);
        }
      }
    }
  }


  /* check for valid tStart/tEnd */
  if(tStart < 0 || tEnd < 0 || tStart >= tEnd )
  {
    logit("","ewdb_api_ProcessSnippetReqs() illegal tStart/tEnd(%.2f/%.2f) combination!  Returning!!\n",
          tStart,tEnd);
    return(EWDB_RETURN_FAILURE);
  }

  /* check for valid iNumAttempts */
  if(iNumAttempts < 0)
  {
    logit("","ewdb_api_ProcessSnippetReqs() illegal iNumAttempts value(%d)!  Returning!!\n",
          iNumAttempts);
    return(EWDB_RETURN_FAILURE);
  }
 
  /* check for valid iRequestGroup */
  if(iRequestGroup < 0)
  {
    logit("","ewdb_api_ProcessSnippetReqs() illegal iRequestGroup value(%d)!  Returning!!\n",
          iRequestGroup);
    return(EWDB_RETURN_FAILURE);
  }
 
  memset(&SnipReq,0,sizeof(EWDB_SnippetRequestStruct));
  SnipReq.tStart = tStart;
  SnipReq.tEnd = tEnd;
  SnipReq.idEvent = idEvent;
  SnipReq.tInitialRequest = time(NULL);
  SnipReq.iNumAttempts = iNumAttempts;
  SnipReq.iRequestGroup = iRequestGroup;
    

  /* There may be wildcards in the SCNL */
  for(i=0; i<iNumidChansRetrieved; i++)
  {
    /* Reset the idSnipReq */
    SnipReq.idSnipReq = 0;

    /* set the SnipReq idChan equal to the current idChan */
    SnipReq.idChan = pidChanBuffer[i];

    /* create a new request */
    rc= ewdb_internal_CreateSnippetRequest(&SnipReq);
    if(rc != EWDB_RETURN_SUCCESS)
    {
      logit("t","Call to ewdb_internal_CreateSnippetRequest failed with rc %d,\n"
            "for idChan = %d, tStart = %.4f, and tEnd = %.4f\n",
            rc, SnipReq.idChan,
            SnipReq.tStart, SnipReq.tEnd);
      bReturnFailure=TRUE;
    }
  }  /* end for i < iNumidChansReceived */

  if(bReturnFailure)
    return(EWDB_RETURN_FAILURE);
  else
    return(EWDB_RETURN_SUCCESS);
}  /* end ewdb_api_ProcessSnippetReqs() */

