/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_api_GetSnippetRequestList_Mark1.c,v 1.1 2003/09/16 17:02:58 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_api_GetSnippetRequestList_Mark1.c,v $
 *     Revision 1.1  2003/09/16 17:02:58  davidk
 *     Initial revision
 *
 *     Revision 1.1  2002/04/16 20:09:59  davidk
 *     Initial revision
 *
 *     Revision 1.4  2002/02/12 04:22:36  davidk
 *     Fixed a couple of bugs.
 *       1)NumItemsFound was not being set in the main function.  It was changed to be the return value
 *          of PostXXX().
 *       2)There appeared to be a buffer clearing problem, where oracle was not writing NULLs to the
 *          memory area for idWaveform when executing a fetch.  This caused the leftover value from
 *          a previous query to rear its ugly head, and cause data from multiple stations to be
 *          merged into one snippet, and chaos to descend over the earth.
 *
 *     Revision 1.3  2001/07/23 16:50:45  davidk
 *     Cleanup as part of API clenup.
 *     Fixed memory leak, by freeing column return lengths in PostXXX().
 *
 *     Revision 1.2  2001/05/15 02:16:17  davidk
 *     Moved functions around between the apps, DB API, and DB API INTERNAL
 *     levels.  Renamed functions and files.  Added support for amplitude
 *     magnitude types.  Reformatted makefiles.
 *
 *     Revision 1.1  2001/02/28 17:19:22  lucky
 *     Initial revision
 *
 *     Revision 1.3  2001/02/21 09:48:38  davidk
 *     Added a proper RCS header comment for the file.
 *
 *
 */

#include <ewdb_cli_base.h>
#include <ewdb_ora_api.h>
#include <ewdb_ew_oci_base.h>
  
static char SQL_STRING[] =
 "select idSnipReq, idChan, tStart, tEnd, idEvent, idWaveform, "
 "       sSta, sComp, sNet, sLoc "
 " from ALL_SNIPPET_REQUESTS where tNextAttempt <= :tTime"
 " order by tNextAttempt";

/* Total number of fields */
#define	NUM_FIELDS	11


static EWDB_OCI_SFS SQLParamsBindArray[] = 
{
  {0,1,0,0,0,OA_EWDBID,"1idSnipReq"},
  {0,1,0,0,0,OA_EWDBID,"2idChan"},
  {0,1,20,0,0,OA_DOUBLE,"3tStart"},
  {0,1,20,0,0,OA_DOUBLE,"4tEnd"},
  {0,1,0,0,0,OA_EWDBID,"5idEvent"},
  {0,1,0,0,0,OA_EWDBID,"6idWaveform"},
  {0,1,10,0,0,OA_SZ,"7Sta"},
  {0,1,10,0,0,OA_SZ,"8Comp"},
  {0,1,10,0,0,OA_SZ,"9Net"},
  {0,1,10,0,0,OA_SZ,"10Loc"},
  {0,1,0,0,0,OA_DOUBLE,":tTime"}
};

/* Insertion Struct for GetSnipReqList statement */
static EWDB_OCIStatementStruct SSStatement;


static  int     SnipReqsPerBuffer;
static  char    sztCurrent[20];

#define BUFFERSIZE 8192

/********************************
      FUNCTION PROTOTYPES
********************************/
int PrepGetSnipReqListExec(time_t tThreshold, EWDB_Cursor * ppCursor);
int PostGetSnipReqListExec(EWDB_SnippetRequestStruct * pBuffer, 
                           int BufferRecLen);
int InitGetSnipReqListStatement(char * szStatement, 
                              EWDB_OCIStatementStruct *pSS);



/* Used to get all of the snippet requests that should attempt to be
   filled after a given time.  
**********************************************************************/
int ewdb_api_GetSnippetRequestList(EWDB_SnippetRequestStruct * pBuffer,
                                   time_t tThreshold,
                                   int iRequestGroup,
                                   int * pNumItemsFound, 
                                   int * pNumItemsRetrieved,
                                   int BufferLen)
{

  EWDB_Cursor  pCursor;
 
  ewdb_base_SetLastOraAPIActionTime();

  if( ewdb_base_Reconnect() != EWDB_RETURN_SUCCESS )
  {
    return( EWDB_RETURN_FAILURE );
  }

  if(PrepGetSnipReqListExec(tThreshold, iRequestGroup, &pCursor) 
    != EWDB_RETURN_SUCCESS)
  {
    return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }

  
  if( ewdb_base_SQLExecute(pCursor) )
  {
    ewdb_base_ErrorReport(hEWDBC, pCursor,"EWDB_GetSnippetRequestList:ewdb_base_SQLExecute",1);
    return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  } 

  if((*pNumItemsFound=PostGetSnipReqListExec(pBuffer, BufferLen)) == EWDB_RETURN_FAILURE)
  {
    return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }

  ewdb_base_SetLastOraAPIActionTime();

  if(*pNumItemsFound <= BufferLen)
  {
    *pNumItemsRetrieved = *pNumItemsFound;
    return(EWDB_RETURN_SUCCESS);
  }
  else
  {
    *pNumItemsRetrieved = BufferLen;
    return(EWDB_RETURN_WARNING);
  }
} /* end ewdb_api_GetSnippetRequestList() */


int InitGetSnipReqListStatement(char * szStatement, EWDB_OCIStatementStruct *pSS)
{

  int LastSize,i;  /* Size of last column array */
  static char pSnipReqBuffer[2*BUFFERSIZE];
  int iSnipReqSize=0;  

  iSnipReqSize+= sizeof(EWDBid);  /* idSnipReq */
  iSnipReqSize+= sizeof(EWDBid);  /* idChan */
  iSnipReqSize+= 20;  /* tStart */
  iSnipReqSize+= 20;  /* tEnd */
  iSnipReqSize+= sizeof(EWDBid);  /* idEvent */
  iSnipReqSize+= sizeof(EWDBid);  /* idWaveform */
  iSnipReqSize+= 10;  /* Sta */
  iSnipReqSize+= 10;  /* Comp */
  iSnipReqSize+= 10;  /* Net */
  iSnipReqSize+= 10;  /* Loc */

  iSnipReqSize = ((iSnipReqSize/4) + 1) * 4;
  
  SnipReqsPerBuffer = BUFFERSIZE / iSnipReqSize;

  /* Allocate space for row/col ret lens.  Freed at end of PostXXX() */
  for(i=0;i<pSS->NumOfFields;i++)
  {
    pSS->FieldArray[i].pRetLens=malloc(SnipReqsPerBuffer*EWDB_FIELD_RET_LEN);
    memset(pSS->FieldArray[i].pRetLens,0,SnipReqsPerBuffer*EWDB_FIELD_RET_LEN);
  }

  pSS->FieldArray[0].pVal=&(pSnipReqBuffer[0]);
  LastSize=sizeof(EWDBid);
  
  pSS->FieldArray[1].pVal= (void *) (
    (int)(pSS->FieldArray[0].pVal)+(LastSize*SnipReqsPerBuffer));
  LastSize=sizeof(EWDBid);
  
  pSS->FieldArray[2].pVal= (void *) (
    (int)(pSS->FieldArray[1].pVal)+(LastSize*SnipReqsPerBuffer));
  LastSize=pSS->FieldArray[2].Ind; /*tStart*/
  
  pSS->FieldArray[3].pVal= (void *) (
    (int)(pSS->FieldArray[2].pVal)+(LastSize*SnipReqsPerBuffer));
  LastSize=pSS->FieldArray[3].Ind; /*tEnd*/
  
  pSS->FieldArray[4].pVal= (void *) (
    (int)(pSS->FieldArray[3].pVal)+(LastSize*SnipReqsPerBuffer));
  LastSize=sizeof(EWDBid);
  
  pSS->FieldArray[5].pVal= (void *) (
    (int)(pSS->FieldArray[4].pVal)+(LastSize*SnipReqsPerBuffer));
  LastSize=sizeof(EWDBid);
  
  pSS->FieldArray[6].pVal= (void *) (
    (int)(pSS->FieldArray[5].pVal)+(LastSize*SnipReqsPerBuffer));
  LastSize=pSS->FieldArray[6].Ind; /*szSta*/
  
  pSS->FieldArray[7].pVal= (void *) (
    (int)(pSS->FieldArray[6].pVal)+(LastSize*SnipReqsPerBuffer));
  LastSize=pSS->FieldArray[7].Ind; /*szComp*/
  
  pSS->FieldArray[8].pVal= (void *) (
    (int)(pSS->FieldArray[7].pVal)+(LastSize*SnipReqsPerBuffer));
  LastSize=pSS->FieldArray[8].Ind; /*szNet*/
  
  pSS->FieldArray[9].pVal= (void *) (
    (int)(pSS->FieldArray[8].pVal)+(LastSize*SnipReqsPerBuffer));
  LastSize=pSS->FieldArray[9].Ind; /*szLoc*/
  

  pSS->FieldArray[10].pVal=sztCurrent;

  ewdb_base_RequestCursor(szStatement, pSS,1);

  return(EWDB_RETURN_SUCCESS);
} /* end InitGetSnipReqListStatement() */


int PrepGetSnipReqListExec(time_t tThreshhold, EWDB_Cursor * ppCursor)
{

  sprintf(sztCurrent,"%d",(int)tThreshhold);


  SSStatement.NumOfFields=NUM_FIELDS;
  SSStatement.FieldArray=SQLParamsBindArray;
  SSStatement.RecordSize=0;

  InitGetSnipReqListStatement(SQL_STRING, &SSStatement);

  *ppCursor=SSStatement.pCda;

  return(EWDB_RETURN_SUCCESS);
}  /* end PrepGetSnipReqListExec() */


int PostGetSnipReqListExec(EWDB_SnippetRequestStruct * pBuffer, int BufferRecLen)
{
    /* 
    The statement has been executed.  we need to do the following:
    1.  While we haven't fetched all of the records or reached capacity,
        fetch a new bunch of records into the buffer.
    2.  For each bunch:  Format each row into EWDB_SnippetRequestStruct format, 
        and copy it into the UserBuffer.
    3.  Repeat step 2. until we have reached capacity in the 
        UserBuffer or have copied all of the records from the 
        bunch.  Then we are done for the bunch.
    4.  If there are more records available, and we haven't reached
        capacity, go back to step 1.  Otherwise go to step 5.
    5.  Figure out why we stopped processing records.  If we reached
        capacity, then find out how many records were actually available
        and return that amount.  If we processed all records, return the
        amount that we processed.
    6.  Done.
    */

  int done=0;
  int RowsRetrieved=0;
  int RowsDone=0;
  EWDB_Cursor  pCursor=SSStatement.pCda;
  int BCurr,UCurr;
  EWDB_OCIStatementStruct * pSS=&SSStatement;
  char * pTemp;
  int RowsProcessed;
  int i;


  while(!done)
  {
    /* DK 021102  added while trying to fix problem
       with idExistingWaveform values seeming to be
       copied from one record to another 
       This appears to fix a bug, where records that did not 
       have existing waveforms were having them added.  There
       appeared to be a buffer cleanup problem, where data
       within the pSS->FieldArray buffer, was not neccessarily
       getting written when a value was NULL.  This allowed an
       old value to exist when the buffer was refilled by another
       call.  This seems like a bug, as I haven't seen this behavior
       before and have writtent lots of code that assumes that 
       Oracle OCI fills in C memory areas with 0s when a value is NULL.
       Maybe I'm missing something...
     ******************************************************************/

    memset(pSS->FieldArray[0].pVal,0,2*BUFFERSIZE);

    /* end DK 021102 */

    if (ewdb_base_SQLFetchRows(pCursor, SnipReqsPerBuffer))
    {
      if (ewdb_base_GetCursorRetCode(pCursor) == EWDB_SQL_ERROR_NO_DATA) 
      {
        done=1;
      }
      else
      {
        ewdb_base_ErrorReport(hEWDBC, pCursor,"PostGetSnipReqListExec:ewdb_base_SQLFetchRows",1);
        return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
      }
    }
    RowsRetrieved = ewdb_base_GetCursorRowsProcessedCount(pCursor);

    for(; RowsDone < RowsRetrieved; RowsDone++)
    {
      if(RowsDone >= BufferRecLen)
      {
        done=1;
        break;
      }
      
      /* CopyRowFromBuffertoUserBuffer */
      {
        BCurr=RowsDone % SnipReqsPerBuffer;
        UCurr=RowsDone;

        /* initialize the record to empty */
        memset(&(pBuffer[UCurr]), 0, sizeof(EWDB_SnippetRequestStruct));

        pBuffer[UCurr].idSnipReq=* (int *)((sizeof(int)*BCurr) + (int)(pSS->FieldArray[0].pVal));

        pBuffer[UCurr].idChan=* (int *)((sizeof(int)*BCurr) + (int)(pSS->FieldArray[1].pVal));
        
        pTemp=(char *) ((pSS->FieldArray[2].Ind*BCurr) + (int)(pSS->FieldArray[2].pVal));
        pTemp[pSS->FieldArray[2].pRetLens[BCurr]]=0;
        pBuffer[UCurr].tStart=atof(pTemp);


        pTemp=(char *) ((pSS->FieldArray[3].Ind*BCurr) + (int)(pSS->FieldArray[3].pVal));
        pTemp[pSS->FieldArray[3].pRetLens[BCurr]]=0;
        pBuffer[UCurr].tEnd=atof(pTemp);
        
        pBuffer[UCurr].idEvent=* (int *)((sizeof(int)*BCurr) + (int)(pSS->FieldArray[4].pVal));

        /* DK 021102 debug */
        if(pBuffer[UCurr].idExistingWaveform != 0)
        {
          logit("t","PostGetSnipReqListExec():  Non Null buffer for %d(%d - %d) = %d\n",
                BCurr, pBuffer[UCurr].idSnipReq, pBuffer[UCurr].idChan,
                pBuffer[UCurr].idExistingWaveform);
        }
        /* end DK 021102 debug */

        pBuffer[UCurr].idExistingWaveform=* (int *)((sizeof(int)*BCurr) + (int)(pSS->FieldArray[5].pVal));
        
        pTemp=(char *) ((pSS->FieldArray[6].Ind*BCurr) + (int)(pSS->FieldArray[6].pVal));
        pTemp[pSS->FieldArray[6].pRetLens[BCurr]]=0;
        strcpy(pBuffer[UCurr].ComponentInfo.Sta, pTemp);
        
        pTemp=(char *) ((pSS->FieldArray[7].Ind*BCurr) + (int)(pSS->FieldArray[7].pVal));
        pTemp[pSS->FieldArray[7].pRetLens[BCurr]]=0;
        strcpy(pBuffer[UCurr].ComponentInfo.Comp, pTemp);
        
        pTemp=(char *) ((pSS->FieldArray[8].Ind*BCurr) + (int)(pSS->FieldArray[8].pVal));
        pTemp[pSS->FieldArray[8].pRetLens[BCurr]]=0;
        strcpy(pBuffer[UCurr].ComponentInfo.Net, pTemp);
        
        pTemp=(char *) ((pSS->FieldArray[9].Ind*BCurr) + (int)(pSS->FieldArray[9].pVal));
        pTemp[pSS->FieldArray[9].pRetLens[BCurr]]=0;
        strcpy(pBuffer[UCurr].ComponentInfo.Loc, pTemp);
        
        if(pBuffer[UCurr].idExistingWaveform != 0)
        {
          logit("","PostGetSnippetRequestList():   (%s,%s,%s,%s)-%d/%d has idExistingWaveform(%d)\n",
                pBuffer[UCurr].ComponentInfo.Sta, pBuffer[UCurr].ComponentInfo.Comp, 
                pBuffer[UCurr].ComponentInfo.Net, pBuffer[UCurr].ComponentInfo.Loc,
                UCurr, BCurr, pBuffer[UCurr].idExistingWaveform);
        }

      }
    } /* End for RowsDone < RowsRetrieved */

  }  /* End while !done */
  

  if (RowsRetrieved > BufferRecLen)
  {
    /* keep going till we get all of the rows,
       but ignore the contents, since we've
       filled up the user's buffer.
    */

    if(ewdb_base_GetCursorRetCode(pCursor) != EWDB_SQL_ERROR_NO_DATA)
    {
      while(!ewdb_base_SQLFetchRows(pCursor, SnipReqsPerBuffer));
    }


    if(ewdb_base_GetCursorRetCode(pCursor) != EWDB_SQL_ERROR_NO_DATA)
    {
      ewdb_base_ErrorReport(hEWDBC, pCursor,"PostGetSnipReqListExec:ewdb_base_SQLFetchRows",2);
      return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
    }
    /* else */
  }  /* End if (RowsRetrieved > BufferRecLen) */
  

  RowsProcessed = ewdb_base_GetCursorRowsProcessedCount(pCursor);

  ewdb_base_ReleaseCursor(pCursor);

  /* Free space for row/col ret lens.*/
  for(i=0;i<pSS->NumOfFields;i++)
  {
    if(pSS->FieldArray[i].pRetLens)
      free(pSS->FieldArray[i].pRetLens);
  }

  return(RowsProcessed);
}  /* end PostGetSnipReqListExec() */

