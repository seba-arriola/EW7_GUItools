/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_internal_GetReservedSnipReqs.c,v 1.3 2004/01/06 20:13:41 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_internal_GetReservedSnipReqs.c,v $
 *     Revision 1.3  2004/01/06 20:13:41  davidk
 *     Fixed buffer memset statements so that they work with the updated buffersize (BUFFERSIZE)
 *     instead of the previous buffersize (BUFFERSIZE*2).
 *
 *     Revision 1.2  2003/09/16 17:02:58  davidk
 *     General API Cleanup (internal data structure initialization).
 *
 *     Revision 1.1  2003/08/21 01:01:51  davidk
 *     Initial revision
 *
 *     Revision 1.2  2003/05/20 20:31:39  davidk
 *     *** empty log message ***
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
  
  EWDBid idSnipReq;
  EWDBid idChan;
  double tStart;
  double tEnd;
  EWDBid idEvent;
  int    iNumAttempts;
  time_t tNextAttempt;
  EWDBid idExistingWaveform;
  time_t tInitialRequest;
  int    iRequestGroup;
  int    iNumAlreadyAtmptd;
  int    iRetCode;
  int    iLockTime;
  char   szNote[100];
  EWDB_StationStruct ComponentInfo;

static char SQL_STRING[] =
 "select idSnipReq, idChan, tStart, tEnd, idEvent, "
 " iNumAttempts, tNextAttempt, idWaveform, tInitialRequest, "
 " iRequestGroup, iNumAlreadyAtmptd, iRetCode, sNote, "
 "       sSta, sComp, sNet, sLoc "
 " from ALL_SNIPPET_REQUESTS where iLockTime = :iReserveKey"
 " order by iRequestGroup DESC, tNextAttempt";

/* Total number of fields */
#define	NUM_FIELDS	18


static EWDB_OCI_SFS SQLParamsBindArray[] = 
{
  {0,1,0,0,0,OA_EWDBID,"1idSnipReq"},
  {0,1,0,0,0,OA_EWDBID,"2idChan"},
  {0,1,20,0,0,OA_DOUBLE,"3tStart"},
  {0,1,20,0,0,OA_DOUBLE,"4tEnd"},
  {0,1,0,0,0,OA_EWDBID, "5idEvent"},
  {0,1,0,0,0,OA_INT,    "6iNumAttempts"},
  {0,1,0,0,0,OA_INT,    "7tNextAttempt"},
  {0,1,0,0,0,OA_EWDBID, "8idWaveform"},
  {0,1,0,0,0,OA_INT,    "9tInitialRequest"},
  {0,1,0,0,0,OA_INT,    "10iRequestGroup"},
  {0,1,0,0,0,OA_INT,    "11iNumAlreadyAtmptd"},
  {0,1,0,0,0,OA_INT,    "12iRetCode"},
  {0,1,99,0,0,OA_SZ,    "13sNote"},
  {0,1,10,0,0,OA_SZ,    "14sSta"},
  {0,1,10,0,0,OA_SZ,    "15sComp"},
  {0,1,10,0,0,OA_SZ,    "16sNet"},
  {0,1,10,0,0,OA_SZ,    "17sLoc"},
  {0,1,0,0,0,OA_INT,    ":iReserveKey"}
};

/* Insertion Struct for GetReservedSnipReqs statement */
static EWDB_OCIStatementStruct SSStatement;

/* Temporary Storage Buffer params */
#define BUFFERSIZE 8192
static int    iRecordSize;
static int    iRecordsPerBuffer;
static char * pLocalBuffer=NULL;

static int    bInitialized=FALSE;

static  int    iReserveKey;


/********************************
      FUNCTION PROTOTYPES
********************************/
int PrepGetReservedSnipReqsExec(int IN_iReserveKey, EWDB_Cursor * ppCursor);
int PostGetReservedSnipReqsExec(EWDB_SnippetRequestStruct * pBuffer, 
                           int BufferRecLen);
int InitGetReservedSnipReqsStatement(char * szStatement, 
                              EWDB_OCIStatementStruct *pSS);



/* Used to get all of the snippet requests that should attempt to be
   filled after a given time.  
**********************************************************************/
int ewdb_internal_GetReservedSnipReqs(EWDB_SnippetRequestStruct * pBuffer,
                                             int iReserveKey,
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

  if(PrepGetReservedSnipReqsExec(iReserveKey, &pCursor) 
    != EWDB_RETURN_SUCCESS)
  {
    return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }

  
  if( ewdb_base_SQLExecute(pCursor) )
  {
    ewdb_base_ErrorReport(hEWDBC, pCursor,"EWDB_GetSnippetRequestList:ewdb_base_SQLExecute",1);
    return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  } 

  if((*pNumItemsFound=PostGetReservedSnipReqsExec(pBuffer, BufferLen)) == EWDB_RETURN_FAILURE)
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
} /* end ewdb_internal_GetReservedSnipReqs() */


int InitGetReservedSnipReqsStatement(char * szStatement, EWDB_OCIStatementStruct *pSS)
{

  int LastSize,i;  /* Size of last column array */


  if(!pLocalBuffer)
  {
    /*Allocate space for returned data - never freed */
    pLocalBuffer=malloc(BUFFERSIZE);


    iRecordSize=0;
    iRecordSize+= sizeof(EWDBid);  /* idSnipReq */
    iRecordSize+= sizeof(EWDBid);  /* idChan */
    iRecordSize += pSS->FieldArray[2].Ind; /*tStart*/
    iRecordSize += pSS->FieldArray[3].Ind; /*tEnd*/
    iRecordSize+= sizeof(EWDBid);  /* idEvent */
    iRecordSize+= sizeof(int);  /* iNumAttempts */
    iRecordSize+= sizeof(int);  /* tNextAttempt */
    iRecordSize+= sizeof(EWDBid);  /* idWaveform */
    iRecordSize+= sizeof(int);  /* tInitialRequest */
    iRecordSize+= sizeof(int);  /* iRequestGroup */
    iRecordSize+= sizeof(int);  /* iNumAlreadyAtmptd */
    iRecordSize+= sizeof(int);  /* iRetCode */
    iRecordSize += pSS->FieldArray[12].Ind; /*sNote*/
    iRecordSize += pSS->FieldArray[13].Ind; /*Sta*/
    iRecordSize += pSS->FieldArray[13].Ind; /*Comp*/
    iRecordSize += pSS->FieldArray[15].Ind; /*Net*/
    iRecordSize += pSS->FieldArray[16].Ind; /*Loc*/

    /* Make the number of records a multiple of 8 to avoid alignment problems on UNIX */
    iRecordsPerBuffer= (BUFFERSIZE/iRecordSize) & 0xfffffff8;
    
    /*Allocate space for row/col ret lens. - never freed */
    for(i=0;i<pSS->NumOfFields;i++)
    {
      pSS->FieldArray[i].pRetLens=malloc(iRecordsPerBuffer*EWDB_FIELD_RET_LEN);
    }

  pSS->FieldArray[0].pVal=&(pLocalBuffer[0]);
  LastSize=sizeof(EWDBid); /* idSnipReq */
  
  pSS->FieldArray[1].pVal= (void *) (
    (int)(pSS->FieldArray[0].pVal)+(LastSize*iRecordsPerBuffer));
  LastSize=sizeof(EWDBid); /* idChan */
  
  pSS->FieldArray[2].pVal= (void *) (
    (int)(pSS->FieldArray[1].pVal)+(LastSize*iRecordsPerBuffer));
  LastSize=pSS->FieldArray[2].Ind; /*tStart*/
  
  pSS->FieldArray[3].pVal= (void *) (
    (int)(pSS->FieldArray[2].pVal)+(LastSize*iRecordsPerBuffer));
  LastSize=pSS->FieldArray[3].Ind; /*tEnd*/
  
  pSS->FieldArray[4].pVal= (void *) (
    (int)(pSS->FieldArray[3].pVal)+(LastSize*iRecordsPerBuffer));
  LastSize=sizeof(EWDBid); /* idEvent */
  
  pSS->FieldArray[5].pVal= (void *) (
    (int)(pSS->FieldArray[4].pVal)+(LastSize*iRecordsPerBuffer));
  LastSize=sizeof(int);  /* iNumAttempts */
  
  pSS->FieldArray[6].pVal= (void *) (
    (int)(pSS->FieldArray[5].pVal)+(LastSize*iRecordsPerBuffer));
  LastSize=sizeof(int);  /* tNextAttempt */
  
  pSS->FieldArray[7].pVal= (void *) (
    (int)(pSS->FieldArray[6].pVal)+(LastSize*iRecordsPerBuffer));
  LastSize=sizeof(EWDBid); /* idWaveform */
  
  pSS->FieldArray[8].pVal= (void *) (
    (int)(pSS->FieldArray[7].pVal)+(LastSize*iRecordsPerBuffer));
  LastSize=sizeof(int);  /* tInitialRequest */
  
  pSS->FieldArray[9].pVal= (void *) (
    (int)(pSS->FieldArray[8].pVal)+(LastSize*iRecordsPerBuffer));
  LastSize=sizeof(int);  /* iRequestGroup */
  
  pSS->FieldArray[10].pVal= (void *) (
    (int)(pSS->FieldArray[9].pVal)+(LastSize*iRecordsPerBuffer));
  LastSize=sizeof(int);  /* iNumAlreadyAtmptd */
  
  pSS->FieldArray[11].pVal= (void *) (
    (int)(pSS->FieldArray[10].pVal)+(LastSize*iRecordsPerBuffer));
  LastSize=sizeof(int);  /* iRetCode */
  
  pSS->FieldArray[12].pVal= (void *) (
    (int)(pSS->FieldArray[11].pVal)+(LastSize*iRecordsPerBuffer));
  LastSize=pSS->FieldArray[12].Ind; /* sNote */
  
  pSS->FieldArray[13].pVal= (void *) (
    (int)(pSS->FieldArray[12].pVal)+(LastSize*iRecordsPerBuffer));
  LastSize=pSS->FieldArray[13].Ind; /*szSta*/
  
  pSS->FieldArray[14].pVal= (void *) (
    (int)(pSS->FieldArray[13].pVal)+(LastSize*iRecordsPerBuffer));
  LastSize=pSS->FieldArray[14].Ind; /*szComp*/
  
  pSS->FieldArray[15].pVal= (void *) (
    (int)(pSS->FieldArray[14].pVal)+(LastSize*iRecordsPerBuffer));
  LastSize=pSS->FieldArray[15].Ind; /*szNet*/
  
  pSS->FieldArray[16].pVal= (void *) (
    (int)(pSS->FieldArray[15].pVal)+(LastSize*iRecordsPerBuffer));
  LastSize=pSS->FieldArray[16].Ind; /*szLoc*/
  
  pSS->FieldArray[17].pVal = &iReserveKey;

  } /* end if(!pLocalBuffer) */
  
  if(!pLocalBuffer)
    logit("","InitGetReservedSnipReqsStatement: malloc of pLocalBuffer "
          "failed! Returning.\n");


  ewdb_base_RequestCursor(szStatement, pSS,0/*don't force rebind*/);

  return(EWDB_RETURN_SUCCESS);
} /* end InitGetReservedSnipReqsStatement() */


int PrepGetReservedSnipReqsExec(int IN_iReserveKey, EWDB_Cursor * ppCursor)
{


  iReserveKey = IN_iReserveKey;


  /* Initialize the statement parameters */
  if(!bInitialized)
  {
    bInitialized = TRUE;
    memset(&SSStatement,0,sizeof(SSStatement));
    SSStatement.NumOfFields=NUM_FIELDS;
    SSStatement.FieldArray=SQLParamsBindArray;
    SSStatement.RecordSize=0;
  }

  InitGetReservedSnipReqsStatement(SQL_STRING, &SSStatement);

  *ppCursor=SSStatement.pCda;

  return(EWDB_RETURN_SUCCESS);
}  /* end PrepGetReservedSnipReqsExec() */


int PostGetReservedSnipReqsExec(EWDB_SnippetRequestStruct * pBuffer, int BufferRecLen)
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

    memset(pSS->FieldArray[0].pVal,0,BUFFERSIZE);

    /* end DK 021102 */

    if (ewdb_base_SQLFetchRows(pCursor, iRecordsPerBuffer))
    {
      if (ewdb_base_GetCursorRetCode(pCursor) == EWDB_SQL_ERROR_NO_DATA) 
      {
        done=1;
      }
      else
      {
        ewdb_base_ErrorReport(hEWDBC, pCursor,"PostGetReservedSnipReqsExec:ewdb_base_SQLFetchRows",1);
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
        BCurr=RowsDone % iRecordsPerBuffer;
        UCurr=RowsDone;

        /* initialize the record to empty */
        memset(&(pBuffer[UCurr]), 0, sizeof(EWDB_SnippetRequestStruct));

        pBuffer[UCurr].idSnipReq=* (EWDBid *)((sizeof(EWDBid)*BCurr) + (int)(pSS->FieldArray[0].pVal));

        pBuffer[UCurr].idChan=* (EWDBid *)((sizeof(EWDBid)*BCurr) + (int)(pSS->FieldArray[1].pVal));
        
        pTemp=(char *) ((pSS->FieldArray[2].Ind*BCurr) + (int)(pSS->FieldArray[2].pVal));
        pTemp[pSS->FieldArray[2].pRetLens[BCurr]]=0;
        pBuffer[UCurr].tStart=atof(pTemp);


        pTemp=(char *) ((pSS->FieldArray[3].Ind*BCurr) + (int)(pSS->FieldArray[3].pVal));
        pTemp[pSS->FieldArray[3].pRetLens[BCurr]]=0;
        pBuffer[UCurr].tEnd=atof(pTemp);
        
        pBuffer[UCurr].idEvent=* (EWDBid *)((sizeof(EWDBid)*BCurr) + (int)(pSS->FieldArray[4].pVal));

        pBuffer[UCurr].iNumAttempts=* (int *)((sizeof(int)*BCurr) + (int)(pSS->FieldArray[5].pVal));

        pBuffer[UCurr].tNextAttempt=* (int *)((sizeof(int)*BCurr) + (int)(pSS->FieldArray[6].pVal));

        pBuffer[UCurr].idExistingWaveform=* (EWDBid *)((sizeof(EWDBid)*BCurr) + (int)(pSS->FieldArray[7].pVal));

        pBuffer[UCurr].tInitialRequest=* (int *)((sizeof(int)*BCurr) + (int)(pSS->FieldArray[8].pVal));

        pBuffer[UCurr].iRequestGroup=* (int *)((sizeof(int)*BCurr) + (int)(pSS->FieldArray[9].pVal));

        pBuffer[UCurr].iNumAlreadyAtmptd=* (int *)((sizeof(int)*BCurr) + (int)(pSS->FieldArray[10].pVal));

        pBuffer[UCurr].iRetCode=* (int *)((sizeof(int)*BCurr) + (int)(pSS->FieldArray[11].pVal));

        pTemp=(char *) ((pSS->FieldArray[12].Ind*BCurr) + (int)(pSS->FieldArray[12].pVal));
        pTemp[pSS->FieldArray[12].pRetLens[BCurr]]=0;
        strcpy(pBuffer[UCurr].szNote, pTemp);
        
        pTemp=(char *) ((pSS->FieldArray[13].Ind*BCurr) + (int)(pSS->FieldArray[13].pVal));
        pTemp[pSS->FieldArray[13].pRetLens[BCurr]]=0;
        strcpy(pBuffer[UCurr].ComponentInfo.Sta, pTemp);
        
        pTemp=(char *) ((pSS->FieldArray[14].Ind*BCurr) + (int)(pSS->FieldArray[14].pVal));
        pTemp[pSS->FieldArray[14].pRetLens[BCurr]]=0;
        strcpy(pBuffer[UCurr].ComponentInfo.Comp, pTemp);
        
        pTemp=(char *) ((pSS->FieldArray[15].Ind*BCurr) + (int)(pSS->FieldArray[15].pVal));
        pTemp[pSS->FieldArray[15].pRetLens[BCurr]]=0;
        strcpy(pBuffer[UCurr].ComponentInfo.Net, pTemp);
        
        pTemp=(char *) ((pSS->FieldArray[16].Ind*BCurr) + (int)(pSS->FieldArray[16].pVal));
        pTemp[pSS->FieldArray[16].pRetLens[BCurr]]=0;
        strcpy(pBuffer[UCurr].ComponentInfo.Loc, pTemp);
        
        /*
        if(pBuffer[UCurr].idExistingWaveform != 0)
        {
          logit("","PostGetSnippetRequestList():   (%s,%s,%s,%s)-%d/%d has idExistingWaveform(%d)\n",
                pBuffer[UCurr].ComponentInfo.Sta, pBuffer[UCurr].ComponentInfo.Comp, 
                pBuffer[UCurr].ComponentInfo.Net, pBuffer[UCurr].ComponentInfo.Loc,
                UCurr, BCurr, pBuffer[UCurr].idExistingWaveform);
        }
        */

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
      while(!ewdb_base_SQLFetchRows(pCursor, iRecordsPerBuffer));
    }


    if(ewdb_base_GetCursorRetCode(pCursor) != EWDB_SQL_ERROR_NO_DATA)
    {
      ewdb_base_ErrorReport(hEWDBC, pCursor,"PostGetReservedSnipReqsExec:ewdb_base_SQLFetchRows",2);
      return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
    }
    /* else */
  }  /* End if (RowsRetrieved > BufferRecLen) */
  

  RowsProcessed = ewdb_base_GetCursorRowsProcessedCount(pCursor);

  ewdb_base_ReleaseCursor(pCursor);

  return(RowsProcessed);
}  /* end PostGetReservedSnipReqsExec() */

