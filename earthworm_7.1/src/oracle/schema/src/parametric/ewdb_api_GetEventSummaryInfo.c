/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_api_GetEventSummaryInfo.c,v 1.3 2005/06/15 19:03:12 davidk Exp $
 *    Revision history:
 *
 *    $Log: ewdb_api_GetEventSummaryInfo.c,v $
 *    Revision 1.3  2005/06/15 19:03:12  davidk
 *    DB API Cleanup
 *
 *    Revision 1.2  2005/05/12 20:32:18  mark
 *    Added comments to event struct
 *
 *    Revision 1.1  2004/01/19 21:49:02  davidk
 *    Initial revision
 *
 *
 * 
 */


#include <ewdb_cli_base.h>
#include <ewdb_ora_api.h>
#include <ewdb_ew_oci_base.h>
  
/* ewdb_api_GetEventSummaryInfo.c */

static char SQL_STRING[] =
 "  select idEvent, tiEventType, iDubiocity, bArchived, sSource, sHumanReadable, sSourceEventID, "
 "         idOrigin, tOrigin, dLat, dLon, dDepth, dPrefMag, idComment, idInternalComment, idContribMagComment "
 "   from ALL_EVENTS_INFO where idEvent = :IN_idEvent";

/* Total number of fields, currently BASE + VLMD + 1 */
#define NUM_FIELDS 17

static EWDB_OCI_SFS SQLParamsBindArray[] = 
{
  {0,1,0,0,0,OA_EWDBID,"1idEvent"},
  {0,1,0,0,0,OA_INT,"2tiEventType"},
  {0,1,0,0,0,OA_INT,"3iDubiocity"},
  {0,1,0,0,0,OA_INT,"4bArchived"},
  {0,1,52,0,0,OA_SZ,"5sSource"},
  {0,1,256,0,0,OA_SZ,"6sHumanReadable"},
  {0,1,30,0,0,OA_SZ,"7sSourceEventID"},
  {0,1,0,0,0,OA_EWDBID,"8idOrigin"},
  {0,1,20,0,0,OA_DOUBLE,"9tOrigin"},
  {0,1,20,0,0,OA_FLOAT,"10dLat"},
  {0,1,20,0,0,OA_FLOAT,"11dLon"},
  {0,1,20,0,0,OA_FLOAT,"12dDepth"},
  {0,1,20,0,0,OA_FLOAT,"13dPrefMag"},
  {0,1,0,0,0,OA_EWDBID,"14idComment"},
  {0,1,0,0,0,OA_EWDBID,"15idInternalComment"},
  {0,1,0,0,0,OA_EWDBID,"16idContribMagComment"},
  {0,1,0,0,0,OA_EWDBID,":IN_idEvent"}
};

/* Insertion Struct for GetEventSummaryInfo statement */
static EWDB_OCIStatementStruct SSStatement;

/* Temporary Storage Buffer params */
#define BUFFERSIZE 8192
static int    iRecordSize;
static int    iRecordsPerBuffer;
static char * pLocalBuffer=NULL;

static int    bInitialized=FALSE;

static int    Local_iRetCode;

static  EWDB_EventListStruct     Local_EventStruct;


/********************************
      FUNCTION PROTOTYPES
********************************/
static int PrepGetEventSummaryInfoExec(EWDBid IN_idEvent, EWDB_Cursor * ppCursor);
static int PostGetEventSummaryInfoExec(EWDB_EventListStruct * pEvent);
static int InitGetEventSummaryInfoStatement(char * szStatement, 
                                EWDB_OCIStatementStruct *pSS);


int ewdb_api_GetEventSummaryInfo(EWDBid IN_idEvent, EWDB_EventListStruct * pEvent)
{

  EWDB_Cursor  pCursor;
  
  ewdb_base_SetLastOraAPIActionTime();

  if(IN_idEvent <= 0 || !pEvent )
  {
    logit("","ewdb_api_GetEventSummaryInfo(): Invalid params passed in:\n"
             "IN_idEvent %d, pEvent %u\n",
          IN_idEvent, pEvent);
    return(EWDB_RETURN_FAILURE);
  }

  if( ewdb_base_Reconnect() != EWDB_RETURN_SUCCESS )
    /* Establishes connection, and performs binding!?! */
  {
     return( EWDB_RETURN_FAILURE );
  }

  if( PrepGetEventSummaryInfoExec(IN_idEvent, &pCursor)
      != EWDB_RETURN_SUCCESS )
  {
    logit("","ewdb_api_GetEventSummaryInfo(): ():PrepGetEventSummaryInfoExec() failed.\n");
    return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }
  
  if( ewdb_base_SQLExecute(pCursor) )
  {
    ewdb_base_ErrorReport(hEWDBC, pCursor,"ewdb_api_GetEventSummaryInfo(): ewdb_base_SQLExecute",1);
    return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  } 

  /* Commit the transaction (all the previous inserts!)
     In Case there is any auditing or logging or debug
     changes made in the stored procedures.
  ****************************************************/
  if (ewdb_base_SQLCommit(hEWDBC))
  {
    ewdb_base_ErrorReport(hEWDBC, hEWDBC,"ewdb_api_GetEventSummaryInfo(): ewdb_base_SQLCommit",2);
    return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE) );
  }

  if((Local_iRetCode=PostGetEventSummaryInfoExec(pEvent)) 
      == EWDB_RETURN_FAILURE )
  {
    return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }
  else
  {
    ewdb_base_SetLastOraAPIActionTime();
    return(Local_iRetCode);
  }
} /* end ewdb_api_GetEventSummaryInfo() */


static int InitGetEventSummaryInfoStatement(char * szStatement, EWDB_OCIStatementStruct *pSS)
{

  int LastSize,i;  /* Size of last column array */

  if(!pLocalBuffer)
  {
    /*Allocate space for returned data - never freed */
    pLocalBuffer=malloc(BUFFERSIZE);

    iRecordSize=0;
    iRecordSize += sizeof(EWDBid); /*idEvent*/
    iRecordSize += sizeof(int); /*tiEventType*/
    iRecordSize += sizeof(int); /*iDubiocity*/
    iRecordSize += sizeof(int); /*bArchived*/
    iRecordSize += pSS->FieldArray[5].Ind; /*sSource*/
    iRecordSize += pSS->FieldArray[6].Ind; /*sHumanReadable*/
    iRecordSize += pSS->FieldArray[7].Ind; /*sSourceEventID*/
    iRecordSize += sizeof(EWDBid); /*idOrigin*/
    iRecordSize += pSS->FieldArray[8].Ind; /*tOrigin*/
    iRecordSize += pSS->FieldArray[9].Ind; /*dLat*/
    iRecordSize += pSS->FieldArray[10].Ind; /*dLon*/
    iRecordSize += pSS->FieldArray[11].Ind; /*dDepth*/
    iRecordSize += pSS->FieldArray[12].Ind; /*dPrefMag*/
    iRecordSize += sizeof(EWDBid); /*idComment*/
    iRecordSize += sizeof(EWDBid); /*idInternalComment*/
    iRecordSize += sizeof(EWDBid); /*idContribMagComment*/

    /* Make the number of records a multiple of 8 to avoid alignment problems on UNIX */
    iRecordsPerBuffer= (BUFFERSIZE/iRecordSize) & 0xfffffff8;
    
    /*Allocate space for row/col ret lens. - never freed */
    for(i=0;i<pSS->NumOfFields;i++)
    {
      pSS->FieldArray[i].pRetLens=malloc(iRecordsPerBuffer*EWDB_FIELD_RET_LEN);
    }

    pSS->FieldArray[0].pVal=&(pLocalBuffer[0]);
    LastSize=sizeof(int);
    
    pSS->FieldArray[1].pVal= (void *) (
      (int)(pSS->FieldArray[0].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=sizeof(int);
    
    pSS->FieldArray[2].pVal= (void *) (
      (int)(pSS->FieldArray[1].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=sizeof(int);
    
    pSS->FieldArray[3].pVal= (void *) (
      (int)(pSS->FieldArray[2].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=sizeof(int);
    
    pSS->FieldArray[4].pVal= (void *) (
      (int)(pSS->FieldArray[3].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[4].Ind; /*char sSource[]*/
    
    pSS->FieldArray[5].pVal= (void *) (
      (int)(pSS->FieldArray[4].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[5].Ind; /* char sHumanReadable[]*/
    
    pSS->FieldArray[6].pVal= (void *) (
      (int)(pSS->FieldArray[5].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[6].Ind; /* char sSourceEventID[]*/
    
    pSS->FieldArray[7].pVal= (void *) (
      (int)(pSS->FieldArray[6].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=sizeof (EWDBid); /* EWDBid idOrigin*/
    
    pSS->FieldArray[8].pVal= (void *) (
      (int)(pSS->FieldArray[7].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[8].Ind;  /* double tOrigin */
    
    pSS->FieldArray[9].pVal= (void *) (
      (int)(pSS->FieldArray[8].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[9].Ind;  /* float dLat */
    
    pSS->FieldArray[10].pVal= (void *) (
      (int)(pSS->FieldArray[9].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[10].Ind;  /* float dLon */
    
    pSS->FieldArray[11].pVal= (void *) (
      (int)(pSS->FieldArray[10].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[11].Ind;  /* float dDepth */
    
    pSS->FieldArray[12].pVal= (void *) (
      (int)(pSS->FieldArray[11].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[12].Ind;  /* float dMag */
    
    pSS->FieldArray[13].pVal= (void *) (
      (int)(pSS->FieldArray[12].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=sizeof (EWDBid); /* EWDBid idComment*/
    
    pSS->FieldArray[14].pVal= (void *) (
      (int)(pSS->FieldArray[13].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=sizeof (EWDBid); /* EWDBid idInternalComment*/
    
    pSS->FieldArray[15].pVal= (void *) (
      (int)(pSS->FieldArray[14].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=sizeof (EWDBid); /* EWDBid idContribMagComment*/
    
    pSS->FieldArray[16].pVal=&(Local_EventStruct.Event.idEvent);
  } /* end if(!pLocalBuffer) */
  
  if(!pLocalBuffer)
  {
    logit("","InitGetEventSummaryInfoStatement(): malloc of pLocalBuffer "
          "failed! Returning.\n");
    return(EWDB_RETURN_FAILURE);
  }

  return(ewdb_base_RequestCursor(szStatement, pSS,0/*don't force rebind*/));

}  /* end InitGetEventSummaryInfoStatement() */

       
static int PrepGetEventSummaryInfoExec(EWDBid IN_idEvent, EWDB_Cursor * ppCursor)
{

  /* Initialize the statement parameters */
  if(!bInitialized)
  {
    bInitialized = TRUE;
    memset(&SSStatement,0,sizeof(SSStatement));
    SSStatement.NumOfFields=NUM_FIELDS;
    SSStatement.FieldArray=SQLParamsBindArray;
    SSStatement.RecordSize=0;
  }

  Local_EventStruct.Event.idEvent = IN_idEvent;

  if(InitGetEventSummaryInfoStatement(SQL_STRING, &SSStatement) != EWDB_RETURN_SUCCESS)
  {
    *ppCursor = NULL;
    return(EWDB_RETURN_FAILURE);
  }

  *ppCursor=SSStatement.pCda;

  return(EWDB_RETURN_SUCCESS);
}  /* end PrepGetEventSummaryInfoExec() */


static int PostGetEventSummaryInfoExec(EWDB_EventListStruct * pEvent)
{
    /* 
    The statement has been executed.  we need to do the following:
    1.  While we haven't fetched all of the records or reached capacity,
        fetch a new bunch of records into the buffer.  (LIMIT CAPACITY 
        to 1 RECORD!)
    2.  For each bunch:  Format each row into EWDB_EventListStruct format, 
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
  int BufferRecLen = 1;  /* Only do one row in this call */


  while(!done)
  {
    memset(pLocalBuffer, 0, BUFFERSIZE);

    if (ewdb_base_SQLFetchRows(pCursor, iRecordsPerBuffer))
    {
      if (ewdb_base_GetCursorRetCode(pCursor) == EWDB_SQL_ERROR_NO_DATA) 
      {
        done=1;
      }
      else
      {
        ewdb_base_ErrorReport(hEWDBC, pCursor,"PostGetEventSummaryInfoExec:ewdb_base_SQLFetchRows",1);
        return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
      }
    }
    if(RowsRetrieved = ewdb_base_GetCursorRowsProcessedCount(pCursor))
    {
      /* CopyRowFromBuffertoUserBuffer */
      BCurr=RowsDone % iRecordsPerBuffer;
      UCurr=RowsDone;
      pEvent->Event.idEvent=* (int *)((sizeof(int)*BCurr) + (int)(pSS->FieldArray[0].pVal));
      
      pEvent->Event.iEventType=* (int *)((sizeof(int)*BCurr) + (int)(pSS->FieldArray[1].pVal));
      
      pEvent->Event.iDubiocity=* (int *)((sizeof(int)*BCurr) + (int)(pSS->FieldArray[2].pVal));
      
      pEvent->Event.bArchived=* (int *)((sizeof(int)*BCurr) + (int)(pSS->FieldArray[3].pVal));
      
      pTemp=(char *) ((pSS->FieldArray[4].Ind*BCurr) + (int)(pSS->FieldArray[4].pVal));
      pTemp[pSS->FieldArray[4].pRetLens[BCurr]]=0;
      strcpy(pEvent->Event.szSource,pTemp);
      
      pTemp=(char *) ((pSS->FieldArray[5].Ind*BCurr) + (int)(pSS->FieldArray[5].pVal));
      pTemp[pSS->FieldArray[5].pRetLens[BCurr]]=0;
      strcpy(pEvent->Event.szSourceName,pTemp);
      
      pTemp=(char *) ((pSS->FieldArray[6].Ind*BCurr) + (int)(pSS->FieldArray[6].pVal));
      pTemp[pSS->FieldArray[6].pRetLens[BCurr]]=0;
      strcpy(pEvent->Event.szSourceEventID,pTemp);
      
      pEvent->idOrigin=* (int *)((sizeof(int)*BCurr) + (int)(pSS->FieldArray[7].pVal));
      
      pTemp=(char *) ((pSS->FieldArray[8].Ind*BCurr) + (int)(pSS->FieldArray[8].pVal) );
      pTemp[pSS->FieldArray[8].pRetLens[BCurr]]=0;
      pEvent->dOT=atof(pTemp);
      
      pTemp=(char *) ((pSS->FieldArray[9].Ind*BCurr) + (int)(pSS->FieldArray[9].pVal) );
      pTemp[pSS->FieldArray[9].pRetLens[BCurr]]=0;
      pEvent->dLat=(float)atof(pTemp);
      
      pTemp=(char *) ((pSS->FieldArray[10].Ind*BCurr) + (int)(pSS->FieldArray[10].pVal) );
      pTemp[pSS->FieldArray[10].pRetLens[BCurr]]=0;
      pEvent->dLon=(float)atof(pTemp);
      
      pTemp=(char *) ((pSS->FieldArray[11].Ind*BCurr) + (int)(pSS->FieldArray[11].pVal) );
      pTemp[pSS->FieldArray[11].pRetLens[BCurr]]=0;
      pEvent->dDepth=(float)atof(pTemp);
      
      pTemp=(char *) ((pSS->FieldArray[12].Ind*BCurr) + (int)(pSS->FieldArray[12].pVal) );
      pTemp[pSS->FieldArray[12].pRetLens[BCurr]]=0;
      pEvent->dPrefMag=(float)atof(pTemp);
      
      pEvent->Event.idPublicComment =* (int *)((sizeof(int)*BCurr) + (int)(pSS->FieldArray[13].pVal));
      pEvent->Event.idInternalComment =* (int *)((sizeof(int)*BCurr) + (int)(pSS->FieldArray[14].pVal));
      pEvent->Event.idContribMagComment =* (int *)((sizeof(int)*BCurr) + (int)(pSS->FieldArray[15].pVal));
    }

  }  /* End while !done */
  

  if (RowsRetrieved > BufferRecLen)
  {
    /* keep going till we get all of the rows,
       but ignore the contents, since we've
       filled up the user's buffer.
    */

    if(ewdb_base_GetCursorRetCode(pCursor) != EWDB_SQL_ERROR_NO_DATA)
    {
      while(!ewdb_base_SQLFetchRows(pCursor, iRecordsPerBuffer))
      {
      }
    }


    if(ewdb_base_GetCursorRetCode(pCursor) != EWDB_SQL_ERROR_NO_DATA)
    {
      ewdb_base_ErrorReport(hEWDBC, pCursor,"PostGetEventSummaryInfoExec:ewdb_base_SQLFetchRows",2);
      return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
    }
    /* else */
  }  /* End if (RowsRetrieved > BufferRecLen) */
  

  RowsRetrieved = ewdb_base_GetCursorRowsProcessedCount(pCursor);

  ewdb_base_ReleaseCursor(pCursor);

  if(RowsRetrieved != 1)
    return(EWDB_RETURN_WARNING);
  else
    return(EWDB_RETURN_SUCCESS);
}  /* end PostGetEventSummaryInfoExec() */


