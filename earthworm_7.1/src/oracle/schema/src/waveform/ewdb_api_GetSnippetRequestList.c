#include <ewdb_cli_base.h>
#include <ewdb_ora_api.h>


int ewdb_internal_ReserveSnippetRequests(EWDBid idSnipReq,
                                         time_t tThreshold, 
                                         int iRequestGroup,
                                         int iPID);

int ewdb_internal_GetReservedSnipReqs(EWDB_SnippetRequestStruct * pBuffer,
                                             int iReserveKey,
                                             int * pNumItemsFound, 
                                             int * pNumItemsRetrieved,
                                             int BufferLen);

int ewdb_internal_LockReservedSnipReqs(int iReserveKey, int * piLockTime);

int ewdb_internal_LockSnipReq(EWDB_SnippetRequestStruct * pSnipReq,
                              int IN_iReserveKey, int * piLockTime);

int ewdb_internal_ReleaseReservedSnipReqs(int iReserveKey);

int ewdb_api_GetSnippetRequestList(EWDB_SnippetRequestStruct * pBuffer,
                                   time_t tThreshold,
                                   int iRequestGroup,
                                   int * pNumItemsFound, 
                                   int * pNumItemsRetrieved,
                                   int BufferLen)
{

  int iPID;
  int iRetCode;
  int i;
	int iLockTime;

  /* Get the list of snippets that we are interested in. 

     This ends up being a 3 step procedure:

     1) Figure out what snippet requests we are interested in.
     2) Reserve those requests, so that we can lock them for
        processing.
     3) Lock the requests for processing.
  ************************************************************/

  /* steps 1 & 2, find and reserve our desired requests */

  iPID = getpid();
  iRetCode = ewdb_internal_ReserveSnippetRequests(pBuffer[0].idSnipReq,
                                                  tThreshold, 
                                                  iRequestGroup,
                                                  iPID);


  if(iRetCode == EWDB_RETURN_FAILURE)
  {
    logit("t","%s:%s failed while while getting request list.  Returning!\n",
          "ewdb_api_GetSnippetRequestList()",
          "ewdb_internal_ReserveSnippetRequests()");
    return(iRetCode);
  }
  else if(iRetCode == EWDB_RETURN_WARNING)
  {
    /* oops, no requests found.  return! */
    return(iRetCode);
  }

  /* now that we've reserved the requests, retrieve them */
  iRetCode = ewdb_internal_GetReservedSnipReqs(pBuffer, iPID,
                                               pNumItemsFound, 
                                               pNumItemsRetrieved,
                                               BufferLen);

  if(iRetCode == EWDB_RETURN_FAILURE)
  {
    logit("t","%s:%s failed.  Returning!\n",
          "ewdb_api_GetSnippetRequestList()",
          "ewdb_internal_GetReservedSnippetRequests()");
    return(iRetCode);
  }

  if(iRetCode == EWDB_RETURN_SUCCESS)
  {

  /* now that we've got the requests, lock them.  I know this
     sounds a little backwards, but we end up with a two step
     locking mechanism with the retrieval sandwiched in between.
     First we reserve them (step 1), 
      then we fetch them (meat of the sandwich),
      then we officially lock them(step 2),
      and THEN we COMMMIT.
  *********************************************************/
    iRetCode = ewdb_internal_LockReservedSnipReqs(iPID, &iLockTime);
    if(iRetCode != EWDB_RETURN_SUCCESS)
    {
      logit("t","%s:%s failed with code(%d).  Returning!\n",
            "ewdb_api_GetSnippetRequestList()",
            "ewdb_internal_LockReservedSnipReqs()");
      return(EWDB_RETURN_FAILURE);
    }
    for(i=0; i < *pNumItemsRetrieved; i++)
    {
      pBuffer[i].iLockTime = iLockTime;
		}
  }
  else
  {
    /* ewdb_internal_GetReservedSnippetRequests() return WARNING */
    if(EWDB_Debug)
    {
			  logit("t","%s:%s returned warning.  Num snippets(%d,%d)!\n",
			  			"ewdb_api_GetSnippetRequestList()",
			  			"ewdb_internal_GetReservedSnippetRequests()",
			  			*pNumItemsRetrieved, *pNumItemsFound);
    }
    /* crudknuckers, we only got part of the list,  now
       we have to lock only the ones we retrieved, and then
       release the rest.  oh brother.....
     *******************************************************/
    for(i=0; i < *pNumItemsRetrieved; i++)
    {
      iRetCode = ewdb_internal_LockSnipReq(&(pBuffer[i]), iPID, &iLockTime);

      if(iRetCode != EWDB_RETURN_SUCCESS)
      {
        logit("t","ewdb_api_GetSnippetRequestList(): Error Locking SnipReq(%d) "
                  "(%s,%s,%s,%s)\n",
              pBuffer[i].idSnipReq, pBuffer[i].ComponentInfo.Sta,
              pBuffer[i].ComponentInfo.Comp,
              pBuffer[i].ComponentInfo.Net,
              pBuffer[i].ComponentInfo.Loc);
      }
			pBuffer[i].iLockTime = iLockTime;
    }
    iRetCode = ewdb_internal_ReleaseReservedSnipReqs(iPID);
    if(iRetCode != EWDB_RETURN_SUCCESS)
    {
      logit("t","ewdb_api_GetSnippetRequestList(): Error %s returned %d.\n"
            "ewdb_internal_ReleaseReservedSnipReqs()", iRetCode);
    }
  }  /* end if iRetCode == EWDB_RETURN_SUCCESS


  /* Commit the transaction(all the previous inserts!)
  ****************************************************/
  if(ewdb_base_SQLCommit(hEWDBC))
  {
    ewdb_base_ErrorReport(hEWDBC, hEWDBC,"ewdb_api_GetSnippetRequestList():ewdb_base_SQLCommit",2);
    return(ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }

  return(iRetCode);
}  /* end ewdb_api_GetSnippetRequestList() */



