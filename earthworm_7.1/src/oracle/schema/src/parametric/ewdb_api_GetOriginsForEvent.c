/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_api_GetOriginsForEvent.c,v 1.8 2005/06/17 21:00:03 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_api_GetOriginsForEvent.c,v $
 *     Revision 1.8  2005/06/17 21:00:03  davidk
 *     Fixed problems in SQL code caused by errant global replacements during API cleanup.
 *
 *     Revision 1.7  2005/06/15 19:03:12  davidk
 *     DB API Cleanup
 *
 *     Revision 1.6  2004/11/10 18:37:45  davidk
 *     Fixed bug where the end of string fields in the return rows were being terminated
 *     beyond the end of the field, which was corrupting data in other fields.
 *
 *     Revision 1.5  2003/09/16 16:56:55  davidk
 *     General API Cleanup
 *
 *     Revision 1.4  2003/08/21 00:53:41  davidk
 *     Restructured API code that deals with dynamic allocation of memory.
 *
 *     Revision 1.3  2003/08/01 17:51:43  davidk
 *     Function never worked.  Fixed bugs in field sizes of double and int fields.
 *
 *     Revision 1.2  2002/05/28 17:22:35  lucky
 *     *** empty log message ***
 *
 *     Revision 1.1  2001/05/15 23:51:56  davidk
 *     Initial revision
 *
 *
 *
 */


#include <ewdb_cli_base.h>
#include <ewdb_ora_api.h>
#include <ewdb_ew_oci_base.h>


static char SQL_STRING[] =
        "select idOrigin, tOrigin, dLat, dLon, dDepth, iAssocPh, iUsedPh, iFixedDepth, "
        "  sSource, sHumanReadable "
        "from ALL_EVENT_ORIGIN_INFO "
        "where idEvent = :idEvent";



static EWDB_OCI_SFS SQLParamsBindArray[] =

{
  {0,1,0,0,0,OA_EWDBID, "1idOrigin"},
  {0,1,20,0,0,OA_DOUBLE,"2tOrigin"},
  {0,1,20,0,0,OA_DOUBLE,"3dLat"},
  {0,1,20,0,0,OA_DOUBLE,"4dLon"},
  {0,1,20,0,0,OA_DOUBLE,"5dDepth"},
  {0,1,0,0,0,OA_INT,    "6iAssocPh"},
  {0,1,0,0,0,OA_INT,    "7iUsedPh"},
  {0,1,0,0,0,OA_INT,    "8iFixedDepth"},
  {0,1,50,0,0,OA_SZ,    "9sSource"},
  {0,1,100,0,0,OA_SZ,   "10sHumanReadable"},
  {0,1,0,0,0,OA_EWDBID, ":idEvent"}
};

/* define the total number of fields to be bound */
#define NUM_FIELDS 11

/* Insertion Struct for  statement */
static EWDB_OCIStatementStruct SSStatement;

/* Temporary Storage Buffer params */
#define BUFFERSIZE 8192
static int    iRecordSize;
static int    iRecordsPerBuffer;
static char * pLocalBuffer=NULL;

static int    bInitialized=FALSE;

static  int     Local_idEvent;


static int PrepGetOriginsForEventExec(EWDBid IN_idEvent,EWDB_Cursor * ppCursor);
static int PostGetOriginsForEventExec(EWDB_OriginStruct * pBuffer,int BufferRecLen,
                               int * pNumFound, int * pNumRetrieved);
static int InitGetOriginsForEventStatement(char * szStatement, EWDB_OCIStatementStruct *pSS);



int ewdb_api_GetOriginsForEvent(EWDBid IN_idEvent, EWDB_OriginStruct * pOrigins,
                                int * pNumItemsFound, 
                                int * pNumItemsRetrieved,
                                int IN_BufferRecLen)
{
  EWDB_Cursor  pCursor;

  ewdb_base_SetLastOraAPIActionTime();

  if(IN_idEvent <= 0 || !pOrigins || !pNumItemsFound || !pNumItemsRetrieved)
  {
    logit("","ewdb_api_GetOriginsForEvent(): Invalid params passed in:\n"
             "IN_idEvent %d, pOrigins %u, pNumItemsFound %u, pNumItemsRetrieved %u\n",
          IN_idEvent, pOrigins, pNumItemsFound, pNumItemsRetrieved);
    return(EWDB_RETURN_FAILURE);
  }

  if( ewdb_base_Reconnect() != EWDB_RETURN_SUCCESS )
  {
    return( EWDB_RETURN_FAILURE );
  }

  if( PrepGetOriginsForEventExec(IN_idEvent,&pCursor)
     != EWDB_RETURN_SUCCESS)
  {
    logit("","ewdb_api_GetOriginsForEvent(): PrepGetOriginsForEventExec() failed!\n");
    return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }
  
  if( ewdb_base_SQLExecute(pCursor) )
  {
    ewdb_base_ErrorReport(hEWDBC, pCursor,"EWDB_GetOriginsForEvent:ewdb_base_SQLExecute",1);
    return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  } 

 /* Commit the transaction (all the previous inserts!)
  In Case there is any auditing or logging or debug
  changes made in the stored procedures.
  ****************************************************/
  if (ewdb_base_SQLCommit (hEWDBC))
  {
    ewdb_base_ErrorReport (hEWDBC, hEWDBC,"ewdb_api_GetOriginsForEvent(): ewdb_base_SQLCommit",2);
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  }
 
  if((PostGetOriginsForEventExec(pOrigins,IN_BufferRecLen,
                                 pNumItemsFound,pNumItemsRetrieved)) 
     == EWDB_RETURN_FAILURE)
  {
    logit("","ewdb_api_GetOriginsForEvent(): PostGetOriginsForEventExec() failed!\n");
    return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }

  ewdb_base_SetLastOraAPIActionTime();

  if(*pNumItemsFound == *pNumItemsRetrieved)
    return(EWDB_RETURN_SUCCESS);
  else
    return(EWDB_RETURN_WARNING);
}  /* end ewdb_api_GetOriginsForEvent() */


static int InitGetOriginsForEventStatement(char * szStatement, EWDB_OCIStatementStruct *pSS)
{

  int LastSize,i;  /* Size of last column array */


  if(!pLocalBuffer)
  {
    /*Allocate space for returned data - never freed */
    pLocalBuffer=malloc(BUFFERSIZE);

    iRecordSize = 0;
    iRecordSize += sizeof (EWDBid);
    iRecordSize += pSS->FieldArray[1].Ind; /*tOrigin*/
    iRecordSize += pSS->FieldArray[2].Ind; /*dLat*/
    iRecordSize += pSS->FieldArray[3].Ind; /*dLon*/
    iRecordSize += pSS->FieldArray[4].Ind; /*dDepth*/
    iRecordSize += sizeof(int);
    iRecordSize += sizeof(int);
    iRecordSize += sizeof(int);
    iRecordSize += pSS->FieldArray[8].Ind; /*sSource*/
    iRecordSize += pSS->FieldArray[9].Ind; /*sHumanReadable*/
    
    /* Make the number of records a multiple of 8 to avoid alignment problems on UNIX */
    iRecordsPerBuffer= (BUFFERSIZE/iRecordSize) & 0xfffffff8;
    
    /*Allocate space for row/col ret lens. - never freed */
    for(i=0;i<pSS->NumOfFields;i++)
    {
      pSS->FieldArray[i].pRetLens=malloc(iRecordsPerBuffer*EWDB_FIELD_RET_LEN);
    }

    pSS->FieldArray[0].pVal=&(pLocalBuffer[0]);
    LastSize=sizeof(EWDBid);

    pSS->FieldArray[1].pVal= (void *) (
      (int)(pSS->FieldArray[0].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[1].Ind; /* tOrigin */

    pSS->FieldArray[2].pVal= (void *) (
      (int)(pSS->FieldArray[1].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[2].Ind; /* dLat */

    pSS->FieldArray[3].pVal= (void *) (
      (int)(pSS->FieldArray[2].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[3].Ind; /* dLon */

    pSS->FieldArray[4].pVal= (void *) (
      (int)(pSS->FieldArray[3].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[4].Ind; /* dDepth */

    pSS->FieldArray[5].pVal= (void *) (
      (int)(pSS->FieldArray[4].pVal)+(LastSize*iRecordsPerBuffer));
      LastSize=sizeof(int); /* iAssocPh */

    pSS->FieldArray[6].pVal= (void *) (
      (int)(pSS->FieldArray[5].pVal)+(LastSize*iRecordsPerBuffer));
      LastSize=sizeof(int); /* iUsedPh */

    pSS->FieldArray[7].pVal= (void *) (
      (int)(pSS->FieldArray[6].pVal)+(LastSize*iRecordsPerBuffer));
      LastSize=sizeof(int); /* iFixedDepth */

    pSS->FieldArray[8].pVal= (void *) (
      (int)(pSS->FieldArray[7].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[8].Ind; /* sSource */

    pSS->FieldArray[9].pVal= (void *) (
      (int)(pSS->FieldArray[8].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[9].Ind; /* sHumanReadable */

    pSS->FieldArray[10].pVal=&Local_idEvent;

  } /* end if(!pLocalBuffer) */
  
  if(!pLocalBuffer)
    logit("","InitGetOriginsForEventStatement: malloc of pLocalBuffer "
          "failed! Returning.\n");

  return(ewdb_base_RequestCursor(szStatement, pSS,0/*don't force rebind*/));
}  /* end InitGetOriginsForEventStatement() */


static int PrepGetOriginsForEventExec(EWDBid IN_idEvent,EWDB_Cursor * ppCursor)
{

  Local_idEvent = IN_idEvent;

  /* Initialize the statement parameters */
  if(!bInitialized)
  {
    bInitialized = TRUE;
    memset(&SSStatement,0,sizeof(SSStatement));
    SSStatement.NumOfFields=NUM_FIELDS;
    SSStatement.FieldArray=SQLParamsBindArray;
    SSStatement.RecordSize=0;
  }

  if(InitGetOriginsForEventStatement(SQL_STRING, &SSStatement) != EWDB_RETURN_SUCCESS)
  {
    *ppCursor = NULL;
    return(EWDB_RETURN_FAILURE);
  }

  *ppCursor=SSStatement.pCda;

  return(EWDB_RETURN_SUCCESS);
}  /* end PrepGetOriginsForEventExec() */


static int PostGetOriginsForEventExec(EWDB_OriginStruct * pBuffer,int BufferRecLen,
                               int * pNumFound, int * pNumRetrieved)
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
  int RecordsProcessed;

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
        ewdb_base_ErrorReport(hEWDBC, pCursor,"PostGetOriginsForEventExec:ewdb_base_SQLFetchRows",1);
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


        pBuffer[UCurr].idOrigin=* (int *)((sizeof(int)*BCurr) + (int)(pSS->FieldArray[0].pVal));

        /* This looks complicated, but maybe it isn't.
           pTemp is a pointer that is set to a place in the retrieval buffer.
           It's location in the buffer is determined by adding the section-offset to the
           buffer-offset, where the section offset is the location of the data within this
           section of the buffer.  The buffer-offset is the location of this section within
           the entire buffer.  So the buffer-offset points to the beginning of this section,
           and the section-offset points to the desired location relative to this section.
           Thus when they are combined, they point to the desired location relative to
           the buffer.  (And you thought this was complicated)
        */

        pTemp=(char *) ((pSS->FieldArray[1].Ind*BCurr) + (int)(pSS->FieldArray[1].pVal) );
        pTemp[pSS->FieldArray[1].pRetLens[BCurr]]=0;
        pBuffer[UCurr].tOrigin=atof(pTemp);

        pTemp=(char *) ((pSS->FieldArray[2].Ind*BCurr) + (int)(pSS->FieldArray[2].pVal) );
        pTemp[pSS->FieldArray[2].pRetLens[BCurr]]=0;
        pBuffer[UCurr].dLat=(float)atof(pTemp);

        pTemp=(char *) ((pSS->FieldArray[3].Ind*BCurr) + (int)(pSS->FieldArray[3].pVal) );
        pTemp[pSS->FieldArray[3].pRetLens[BCurr]]=0;
        pBuffer[UCurr].dLon=(float)atof(pTemp);

        pTemp=(char *) ((pSS->FieldArray[4].Ind*BCurr) + (int)(pSS->FieldArray[4].pVal) );
        pTemp[pSS->FieldArray[4].pRetLens[BCurr]]=0;
        pBuffer[UCurr].dDepth=(float)atof(pTemp);

        pBuffer[UCurr].iAssocPh=* (int *)((sizeof(int)*BCurr) + (int)(pSS->FieldArray[5].pVal));

        pBuffer[UCurr].iUsedPh=* (int *)((sizeof(int)*BCurr) + (int)(pSS->FieldArray[6].pVal));

        pBuffer[UCurr].iFixedDepth=* (int *)((sizeof(int)*BCurr) + (int)(pSS->FieldArray[7].pVal));

        pTemp=(char *) ((pSS->FieldArray[8].Ind*BCurr) + (int)(pSS->FieldArray[8].pVal) );
        pTemp[pSS->FieldArray[8].pRetLens[BCurr]]=0;
        strncpy(pBuffer[UCurr].sRealSource,pTemp,sizeof(pBuffer[UCurr].sRealSource));
        pBuffer[UCurr].sRealSource[sizeof(pBuffer[UCurr].sRealSource)-1] = 0x00;

        pTemp=(char *) ((pSS->FieldArray[9].Ind*BCurr) + (int)(pSS->FieldArray[9].pVal) );
        pTemp[pSS->FieldArray[9].pRetLens[BCurr]]=0;
        strncpy(pBuffer[UCurr].sSource,pTemp,sizeof(pBuffer[UCurr].sSource));
        pBuffer[UCurr].sSource[sizeof(pBuffer[UCurr].sSource)-1] = 0x00;


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
      while(!ewdb_base_SQLFetchRows(pCursor, iRecordsPerBuffer))
      {
      }
    }

    if(ewdb_base_GetCursorRetCode(pCursor) != EWDB_SQL_ERROR_NO_DATA)
    {
      ewdb_base_ErrorReport(hEWDBC, pCursor,"PostGetOriginsForEventExec:ewdb_base_SQLFetchRows",2);
      return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
    }
    /* else */
  }  /* End if (RowsRetrieved > BufferRecLen) */
  
  RecordsProcessed = ewdb_base_GetCursorRowsProcessedCount(pCursor);
  ewdb_base_ReleaseCursor (pCursor);

  *pNumFound = RecordsProcessed;
  if(RecordsProcessed > BufferRecLen)
    *pNumRetrieved = BufferRecLen;
  else
    *pNumRetrieved = RecordsProcessed;

  return(EWDB_RETURN_SUCCESS);
}  /* end PostGetOriginsForEventExec() */

