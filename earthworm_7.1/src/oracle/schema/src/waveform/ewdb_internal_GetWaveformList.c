
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_internal_GetWaveformList.c,v 1.4 2003/09/16 17:02:58 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_internal_GetWaveformList.c,v $
 *     Revision 1.4  2003/09/16 17:02:58  davidk
 *     General API Cleanup (internal data structure initialization).
 *
 *     Revision 1.3  2003/08/21 01:02:10  davidk
 *     Restructured API code that deals with dynamic allocation of memory.
 *
 *     Revision 1.2  2001/07/23 17:27:07  davidk
 *     API Cleanup.
 *     Fixed memory leak (freed memory for column return lengths in PostXXX().
 *
 *     Revision 1.1  2001/05/15 02:16:43  davidk
 *     Initial revision
 *
 *     Revision 1.1  2001/02/28 17:19:22  lucky
 *     Initial revision
 *
 *
 */

#include <ewdb_ora_api.h>
#include <ewdb_cli_base.h>
#include <ewdb_ew_oci_base.h>


static char SQL_STRING[] =
        "select idWaveform, idChan, tStart, tEnd, iDataFormat, iByteLen "
        "from ALL_WAVES_W_COMPT_BY_EVENT "
        "where idEvent = :IN_idEvent";


static EWDB_OCI_SFS SQLParamsBindArray[] =

{
  {0,1, 0,0,0,OA_INT,"1idWaveform"},
  {0,1, 0,0,0,OA_INT,"2idChan"},
  {0,1,20,0,0,OA_DOUBLE,"3tStart"},
  {0,1,20,0,0,OA_DOUBLE,"4tEnd"},
  {0,1, 0,0,0,OA_INT,"5iDataFormat"},
  {0,1, 0,0,0,OA_INT,"6iByteLen"},
  {0,1, 0,0,0,OA_INT,":IN_idEvent"}
};

/* define the total number of fields to be bound */
#define NUM_FIELDS 7

/* Insertion Struct for GetStationList statement */
static EWDB_OCIStatementStruct SSStatement;

/* Temporary Storage Buffer params */
#define BUFFERSIZE 8192
static int    iRecordSize;
static int    iRecordsPerBuffer;
static char * pLocalBuffer=NULL;

static int    bInitialized=FALSE;

static  EWDBid  idEvent;


/********************************
      FUNCTION PROTOTYPES
********************************/
int PrepGetWaveformListExec(EWDBid IN_idEvent, EWDB_Cursor * ppCursor);
int PostGetWaveformListExec(EWDB_WaveformStruct * pWaveBuffer, 
                           int BufferRecLen);
int InitGetWaveformListStatement(char * szStatement, EWDB_OCIStatementStruct *pSS);
/*******************************/


int ewdb_internal_GetWaveformList(EWDBid IN_idEvent,  
                                  EWDB_WaveformStruct * IN_pWaveformBuffer,   
                                  int IN_BufferRecLen)
{

  EWDB_Cursor pCursor;
  int RecordsProcessed;

  if(ewdb_base_Reconnect() != EWDB_RETURN_SUCCESS)
  {
    logit("", "EWDB_GetWaveformList(): Could not reconnect to the database!\n");
    return(EWDB_RETURN_FAILURE);
  }

  if(PrepGetWaveformListExec(IN_idEvent,  &pCursor) != EWDB_RETURN_SUCCESS)
  {
    logit("", "ORA_API:EWDB_GetWaveformList():PrepGetWaveformListExec() failed.\n");
    return(ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }

  if(ewdb_base_SQLExecute(pCursor))
  {
    ewdb_base_ErrorReport(hEWDBC, pCursor,"EWDB_GetWaveformList:ewdb_base_SQLExecute", 1);
    return(ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  } 

  
  /* Commit the transaction(all the previous inserts!)
     In Case there is any auditing or logging or debug
     changes made in the stored procedures.
  ****************************************************/
  if(ewdb_base_SQLCommit(hEWDBC))
  {
    ewdb_base_ErrorReport(hEWDBC, hEWDBC,"EWDB_GetWaveformList:ewdb_base_SQLCommit",2);
    return(ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }

  if(( RecordsProcessed=
        PostGetWaveformListExec(IN_pWaveformBuffer,IN_BufferRecLen))
     == EWDB_RETURN_FAILURE)
  {
    logit("", "EWDB_GetWaveformList(): "
           "Call to PostGetWaveformListExec failed!\n");
    return(ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }

  if(RecordsProcessed > IN_BufferRecLen)
    RecordsProcessed=0-RecordsProcessed;

  return(RecordsProcessed);
}  


int InitGetWaveformListStatement(char * szStatement, 
                                 EWDB_OCIStatementStruct *pSS)
{

  int LastSize,i;  /* Size of last column array */


  if(!pLocalBuffer)
  {
    /*Allocate space for returned data - never freed */
    pLocalBuffer=malloc(BUFFERSIZE);


    iRecordSize=0;
    iRecordSize += sizeof(int); /*idWaveform*/
    iRecordSize += sizeof(int); /*idChan*/
    iRecordSize += pSS->FieldArray[2].Ind; /*tStart*/
    iRecordSize += pSS->FieldArray[3].Ind; /*tEnd*/
    iRecordSize += sizeof(int); /*iDataFormat*/
    iRecordSize += sizeof(int); /*iByteLen*/

    /* Make the number of records a multiple of 8 to avoid alignment problems on UNIX */
    iRecordsPerBuffer= (BUFFERSIZE/iRecordSize) & 0xfffffff8;
    
    /*Allocate space for row/col ret lens. - never freed */
    for(i=0;i<pSS->NumOfFields;i++)
    {
      pSS->FieldArray[i].pRetLens=malloc(iRecordsPerBuffer*EWDB_FIELD_RET_LEN);
    }

  pSS->FieldArray[0].pVal=&(pLocalBuffer[0]);
  LastSize=sizeof(int); /* 1idWaveform */
  pSS->FieldArray[1].pVal=(void *)(
   (int)(pSS->FieldArray[0].pVal)+(LastSize*iRecordsPerBuffer));
  LastSize=sizeof(int); /* 2idChan */
  pSS->FieldArray[2].pVal=(void *)(
   (int)(pSS->FieldArray[1].pVal)+(LastSize*iRecordsPerBuffer));
  LastSize=pSS->FieldArray[2].Ind; /* tStart */
  pSS->FieldArray[3].pVal=(void *)(
   (int)(pSS->FieldArray[2].pVal)+(LastSize*iRecordsPerBuffer));
  LastSize=pSS->FieldArray[3].Ind; /* tEnd */
  pSS->FieldArray[4].pVal=(void *)(
   (int)(pSS->FieldArray[3].pVal)+(LastSize*iRecordsPerBuffer));
  LastSize=sizeof(int); /* 5iDataFormat */
  pSS->FieldArray[5].pVal=(void *)(
   (int)(pSS->FieldArray[4].pVal)+(LastSize*iRecordsPerBuffer));
  LastSize=sizeof(int); /* 6iByteLen */
  pSS->FieldArray[6].pVal=&idEvent;

  } /* end if(!pLocalBuffer) */
  
  if(!pLocalBuffer)
    logit("","InitGetWaveformListStatement: malloc of pLocalBuffer "
          "failed! Returning.\n");


  ewdb_base_RequestCursor(szStatement, pSS,0/*don't force rebind*/);

  return(EWDB_RETURN_SUCCESS);
}  /* End InitGetWaveformListStatement() */


int PrepGetWaveformListExec(EWDBid IN_idEvent, EWDB_Cursor * ppCursor)
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

  idEvent = IN_idEvent;

  InitGetWaveformListStatement(SQL_STRING, &SSStatement);

  *ppCursor=SSStatement.pCda;

  return(EWDB_RETURN_SUCCESS);
}  /* End PrepGetWaveformListExec() */


int PostGetWaveformListExec(EWDB_WaveformStruct * pWaveBuffer, 
                           int BufferRecLen)
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
    if(ewdb_base_SQLFetchRows(pCursor, iRecordsPerBuffer))
    {
      if(ewdb_base_GetCursorRetCode(pCursor) == EWDB_SQL_ERROR_NO_DATA) 
      {
        done=1;
      }
      else
      {
        ewdb_base_ErrorReport(hEWDBC, pCursor,"PostGetWaveformListExec:ewdb_base_SQLFetchRows",1);
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


        pWaveBuffer[UCurr].idWaveform = *(EWDBid *)
                                        ((sizeof(EWDBid)*BCurr) +
                                         (int)(pSS->FieldArray[0].pVal));

        /* This looks complicated, but maybe it isn't.
           pTemp is a pointer that is set to a place in the retrieval buffer.
           It's location in the buffer is determined by adding the section-offset to the
           buffer-offset, where the section offset is the location of the data within this
           section of the buffer.  The buffer-offset is the location of this section within
           the entire buffer.  So the buffer-offset points to the beginning of this section,
           and the section-offset points to the desired location relative to this section.
           Thus when they are combined, they point to the desired location relative to
           the buffer. (And you thought this was complicated)
        */

        pWaveBuffer[UCurr].idChan=*(EWDBid *)((sizeof(EWDBid)*BCurr) +
                                              (int)(pSS->FieldArray[1].pVal));


        pTemp=(char *)((pSS->FieldArray[2].Ind*BCurr) + 
                       (int)(pSS->FieldArray[2].pVal) );
        pTemp[pSS->FieldArray[2].pRetLens[BCurr]]=0;
        pWaveBuffer[UCurr].tStart=atof(pTemp);

        pTemp=(char *)((pSS->FieldArray[3].Ind*BCurr) + 
                       (int)(pSS->FieldArray[3].pVal) );
        pTemp[pSS->FieldArray[3].pRetLens[BCurr]]=0;
        pWaveBuffer[UCurr].tEnd=atof(pTemp);

        pWaveBuffer[UCurr].iDataFormat = *(int *)
                  ((sizeof(int)*BCurr) +(int)(pSS->FieldArray[4].pVal));

        pWaveBuffer[UCurr].iByteLen    = *(int *)
                  ((sizeof(int)*BCurr) +(int)(pSS->FieldArray[5].pVal));
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
      ewdb_base_ErrorReport(hEWDBC, pCursor,"PostGetWaveformListExec:ewdb_base_SQLFetchRows",2);
      return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
    }
  }  /* End if(RowsRetrieved > BufferRecLen) */
  
  RecordsProcessed = ewdb_base_GetCursorRowsProcessedCount(pCursor);
  ewdb_base_ReleaseCursor(pCursor);

  return(RecordsProcessed);
}  /* End PostGetWaveformListExec() */
