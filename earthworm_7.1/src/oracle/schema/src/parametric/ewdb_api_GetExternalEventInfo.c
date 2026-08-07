/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_api_GetExternalEventInfo.c,v 1.3 2005/06/15 19:03:12 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_api_GetExternalEventInfo.c,v $
 *     Revision 1.3  2005/06/15 19:03:12  davidk
 *     DB API Cleanup
 *
 *     Revision 1.2  2004/11/10 18:36:47  davidk
 *     Fixed minor bug where the string fields in the return rows were not neccessarily
 *     being properly terminated.
 *     Problems not observed, just fixed because code was similar to
 *     problem in ewdb_api_GetOriginsForEvent().
 *
 *     Revision 1.1  2004/01/20 22:07:00  davidk
 *     Initial revision
 *
 *
 *
 */


#include <ewdb_ora_api.h>
#include <ewdb_cli_base.h>
#include <ewdb_ew_oci_base.h>

static char SQL_STRING[] = 
    "select IN_idEvent, tiEventType, iDubiocity, bArchived, "
    "       sSourceEventID, sSource, sHumanReadable,  "
    "       sComment "
    " from ALL_EXTERNAL_EVENT_INFO";



static EWDB_OCI_SFS SQLParamsBindArray[] = 
{
  {0,1,0,  0,0,OA_INT,"1idEvent"},
  {0,1,0,  0,0,OA_INT,"2tiEventType"},
  {0,1,0,  0,0,OA_INT,"3iDubiocity"},
  {0,1,0,  0,0,OA_INT,"4bArchived"},
  {0,1,256,0,0,OA_SZ,"5sSourceEventID"},
  {0,1,256,0,0,OA_SZ,"6sSource"},
  {0,1,256,0,0,OA_SZ,"7sHumanReadable"},
  {0,1,512,0,0,OA_SZ,"8sComment"},
  {0,1,0,  0,0,OA_EWDBID,":IN_idEvent"}
};

/* define the total number of fields to be bound */
#define NUM_FIELDS 9

/* define the size of the local buffer */
#define STRSIZE 15


/* Insertion Struct for GetExternalEventInfo statement */
static EWDB_OCIStatementStruct SSStatement;

/* Temporary Storage Buffer params */
#define BUFFERSIZE 16384
static int    iRecordSize;
static int    iRecordsPerBuffer;
static char * pLocalBuffer=NULL;

static int    bInitialized=FALSE;

static EWDBid Local_idEvent;
/********************************
      FUNCTION PROTOTYPES
********************************/
static int PrepGetExternalEventInfoExec(EWDBid IN_idEvent, EWDB_Cursor * ppCursor);
static int PostGetExternalEventInfoExec(EWDB_EventStruct * pBuffer, int BufferRecLen);
static int InitGetExternalEventInfoStatement(char * szStatement, 
                                             EWDB_OCIStatementStruct *pSS);


ewdb_api_GetExternalEventInfo(EWDBid IN_idEvent, EWDB_EventStruct * pBuffer,
                             int * pNumItemsFound,
                             int * pNumItemsRetrieved,
                             int BufferLen)
{

  EWDB_Cursor  pCursor;
 
  ewdb_base_SetLastOraAPIActionTime();

  if(IN_idEvent <= 0 || !pBuffer || !pNumItemsFound || !pNumItemsRetrieved)
  {
    logit("","ewdb_api_GetExternalEventInfo(): Invalid params passed in:\n"
             "IN_idEvent %d, pBuffer %u, pNumItemsFound %u, pNumItemsRetrieved %u\n",
          IN_idEvent, pBuffer, pNumItemsFound, pNumItemsRetrieved);
    return(EWDB_RETURN_FAILURE);
  }

  if(ewdb_base_Reconnect() != EWDB_RETURN_SUCCESS)
  /* Re-establishes connection. */
  {
    logit("", "ewdb_api_GetExternalEventInfo(): Could not reconnect to the database!\n");
    return(EWDB_RETURN_FAILURE);
  }

  if(PrepGetExternalEventInfoExec(IN_idEvent, &pCursor)
     != EWDB_RETURN_SUCCESS)
  {
    logit("","ewdb_api_GetExternalEventInfo(): PrepGetExternalEventInfoExec() failed!\n");
    return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }

  if( ewdb_base_SQLExecute(pCursor) )
  {
    ewdb_base_ErrorReport(hEWDBC, pCursor,
                          "ewdb_api_GetExternalEventInfo(): ewdb_base_SQLExecute",1);
    return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  } 

 /* Commit the transaction (all the previous inserts!)
  In Case there is any auditing or logging or debug
  changes made in the stored procedures.
  ****************************************************/
  if (ewdb_base_SQLCommit (hEWDBC))
  {
    ewdb_base_ErrorReport (hEWDBC, hEWDBC,"DeletePolygon:ewdb_base_SQLCommit",2);
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  }
 
  if((*pNumItemsFound=PostGetExternalEventInfoExec(pBuffer,BufferLen))
     == EWDB_RETURN_FAILURE)
  {
    logit("","ewdb_api_GetExternalEventInfo(): PostGetExternalEventInfoExec() failed!\n");
    return(ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
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
}  /* end ewdb_api_GetExternalEventInfo() */


static int InitGetExternalEventInfoStatement(char * szStatement, 
                                     EWDB_OCIStatementStruct *pSS)
{

  int LastSize,i;  /* Size of last column array */


  if(!pLocalBuffer)
  {
    /*Allocate space for returned data - never freed */
    pLocalBuffer=malloc(BUFFERSIZE);


    iRecordSize=0;
    iRecordSize += sizeof(EWDBid); /*1idEvent*/
    iRecordSize += sizeof(int); /*2tiEventType*/
    iRecordSize += sizeof(int); /*3iDubiocity*/
    iRecordSize += sizeof(int); /*4bArchived*/
    iRecordSize += pSS->FieldArray[4].Ind; /*5sSourceEventID*/
    iRecordSize += pSS->FieldArray[5].Ind; /*6sSource*/
    iRecordSize += pSS->FieldArray[6].Ind; /*7sHumanReadable*/
    iRecordSize += pSS->FieldArray[7].Ind; /*8sComment*/

    /* Make the number of records a multiple of 8 to avoid alignment problems on UNIX */
    iRecordsPerBuffer= (BUFFERSIZE/iRecordSize) & 0xfffffff8;
    
    /*Allocate space for row/col ret lens. - never freed */
    for(i=0;i<pSS->NumOfFields;i++)
    {
      pSS->FieldArray[i].pRetLens=malloc(iRecordsPerBuffer*EWDB_FIELD_RET_LEN);
    }

    pSS->FieldArray[0].pVal=&(pLocalBuffer[0]);
    LastSize=sizeof(EWDBid);  /* IN_idEvent */
    pSS->FieldArray[1].pVal=(void *)(
     (int)(pSS->FieldArray[0].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=sizeof(int);   /* tiEventType */
    pSS->FieldArray[2].pVal=(void *)(
     (int)(pSS->FieldArray[1].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=sizeof(int);   /* iDubiocity */
    pSS->FieldArray[3].pVal=(void *)(
     (int)(pSS->FieldArray[2].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=sizeof(int);   /* bArchived */
    pSS->FieldArray[4].pVal=(void *)(
     (int)(pSS->FieldArray[3].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[4].Ind; /* sSourceEventID */
    pSS->FieldArray[5].pVal=(void *)(
     (int)(pSS->FieldArray[4].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[5].Ind; /* sSource */
    pSS->FieldArray[6].pVal=(void *)(
     (int)(pSS->FieldArray[5].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[6].Ind; /* sHumanReadable */
    pSS->FieldArray[7].pVal=(void *)(
     (int)(pSS->FieldArray[6].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[7].Ind; /* sComment */

    pSS->FieldArray[8].pVal=&Local_idEvent;

  } /* end if(!pLocalBuffer) */
  
  if(!pLocalBuffer)
  {
    logit("","InitGetExternalEventInfoStatement(): malloc of pLocalBuffer "
          "failed! Returning.\n");
    return(EWDB_RETURN_FAILURE);
  }

  return(ewdb_base_RequestCursor(szStatement, pSS,0/*don't force rebind*/));
}  /* End InitGetExternalEventInfoStatement() */


static int PrepGetExternalEventInfoExec(EWDBid IN_idEvent, EWDB_Cursor * ppCursor)
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

  Local_idEvent = IN_idEvent;

  /* Call InitXXX() to bind the params, get a cursor, and let 
     Oracle do its preparation. */
  if(InitGetExternalEventInfoStatement(SQL_STRING, &SSStatement) != EWDB_RETURN_SUCCESS)
  {
    *ppCursor = NULL;
    return(EWDB_RETURN_FAILURE);
  }

  *ppCursor=SSStatement.pCda;

  return(EWDB_RETURN_SUCCESS);
}  /* End PrepGetExternalEventInfoExec() */


static int PostGetExternalEventInfoExec(EWDB_EventStruct * pBuffer, int BufferRecLen)
{
    /* 
    The statement has been executed.  we need to do the following:
    1.  While we haven't fetched all of the records or reached capacity,
        fetch a new bunch of records into the buffer.
    2.  For each bunch:  Format each row into SnipDescStruct format, 
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
  char * pTemp;
  int BCurr,UCurr;
  EWDB_OCIStatementStruct * pSS=&SSStatement;
  int RowsProcessed;


  while(!done)
  {
    memset(pLocalBuffer, 0, BUFFERSIZE);

    if(ewdb_base_SQLFetchRows(pCursor, iRecordsPerBuffer))
    {
      if(ewdb_base_GetCursorRetCode(pCursor) == EWDB_SQL_ERROR_NO_DATA) 
      {
        done=1;
      }
      else
      {
        ewdb_base_ErrorReport(hEWDBC, pCursor,
                              "PostGetExternalEventInfoExec:ewdb_base_SQLFetchRows",1);
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

        /* initialize the user buffer record */
        memset(&pBuffer[UCurr], 0, sizeof(pBuffer[UCurr]));

        pBuffer[UCurr].idEvent=*(EWDBid *)
           ((sizeof(int)*BCurr) +(int)(pSS->FieldArray[0].pVal));
        pBuffer[UCurr].iEventType=*(int *)
           ((sizeof(int)*BCurr) +(int)(pSS->FieldArray[1].pVal));

        pBuffer[UCurr].iDubiocity=*(int *)
           ((sizeof(int)*BCurr) +(int)(pSS->FieldArray[2].pVal));

        pBuffer[UCurr].bArchived=*(int *)
           ((sizeof(int)*BCurr) +(int)(pSS->FieldArray[3].pVal));

        pTemp=(char *) 
           ((pSS->FieldArray[4].Ind*BCurr) +(int)(pSS->FieldArray[4].pVal));
        pTemp[pSS->FieldArray[4].pRetLens[BCurr]]=0;
        strncpy(pBuffer[UCurr].szSourceEventID, pTemp, sizeof(pBuffer[UCurr].szSourceEventID)-1);

        pTemp=(char *) 
           ((pSS->FieldArray[5].Ind*BCurr) +(int)(pSS->FieldArray[5].pVal));
        pTemp[pSS->FieldArray[5].pRetLens[BCurr]]=0;
        strncpy(pBuffer[UCurr].szSource, pTemp, sizeof(pBuffer[UCurr].szSource)-1);

        pTemp=(char *) 
           ((pSS->FieldArray[6].Ind*BCurr) +(int)(pSS->FieldArray[6].pVal));
        pTemp[pSS->FieldArray[6].pRetLens[BCurr]]=0;
        strncpy(pBuffer[UCurr].szSourceName, pTemp, sizeof(pBuffer[UCurr].szSourceName)-1);

        pTemp=(char *) 
           ((pSS->FieldArray[7].Ind*BCurr) +(int)(pSS->FieldArray[7].pVal));
        pTemp[pSS->FieldArray[7].pRetLens[BCurr]]=0;
        strncpy(pBuffer[UCurr].szComment, pTemp, sizeof(pBuffer[UCurr].szComment)-1);
      }
    } /* End for RowsDone < RowsRetrieved */
  }  /* End while !done */
  
  if(RowsRetrieved > BufferRecLen)
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
      ewdb_base_ErrorReport(hEWDBC, pCursor,
                            "PostGetGetExternalEventInfoExec:ewdb_base_SQLFetchRows",2);
      return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
    }
    /* else */
  }  /* End if(RowsRetrieved > BufferRecLen) */
  
  RowsProcessed = ewdb_base_GetCursorRowsProcessedCount(pCursor);

  ewdb_base_ReleaseCursor(pCursor);

  return(RowsProcessed);
}  /* End PostGetGetExternalEventInfoExec() */

