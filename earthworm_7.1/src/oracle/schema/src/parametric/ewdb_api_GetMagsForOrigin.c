/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_api_GetMagsForOrigin.c,v 1.9 2005/06/17 21:00:03 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_api_GetMagsForOrigin.c,v $
 *     Revision 1.9  2005/06/17 21:00:03  davidk
 *     Fixed problems in SQL code caused by errant global replacements during API cleanup.
 *
 *     Revision 1.8  2005/06/15 19:03:12  davidk
 *     DB API Cleanup
 *
 *     Revision 1.7  2003/09/16 16:56:55  davidk
 *     General API Cleanup
 *
 *     Revision 1.6  2003/08/21 00:52:45  davidk
 *     Restructured API code that deals with dynamic allocation of memory.
 *
 *     Revision 1.5  2001/07/26 21:55:09  davidk
 *     removed bad #include, and reference to ub2.  Memset the column lenghts
 *     for the query when they are initialized.
 *
 *     Revision 1.4  2001/07/05 16:44:46  lucky
 *     Cleaned up debugging statements
 *
 *     Revision 1.3  2001/07/01 21:55:37  davidk
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
 *     Revision 1.2  2001/05/25 19:21:19  lucky
 *     Fixed a dumb syntax error
 *
 *     Revision 1.1  2001/05/25 18:54:21  lucky
 *     Initial revision
 *
 *
 */


#include <ewdb_ora_api.h>
#include <ewdb_cli_base.h>
#include <ewdb_ew_oci_base.h>

static char SQL_STRING[] =
        "select idMag, idOrigin, dMagAvg, iNumMags, dMagErr, iMagType "
        "from ALL_MAG_INFO "
        "where idOrigin = :IN_idOrigin";

static EWDB_OCI_SFS SQLParamsBindArray[] =

{
  {0,1,0,0,0,OA_EWDBID, "1idMag"},
  {0,1,0,0,0,OA_EWDBID,"2idOrigin"},
  {0,1,20,0,0,OA_DOUBLE,"3dMagAvg"},
  {0,1,0,0,0,OA_INT,   "4iNumMags"},
  {0,1,20,0,0,OA_DOUBLE,"5dMagErr"},
  {0,1,0,0,0,OA_INT,    "6iMagType"},
  {0,1,0,0,0,OA_EWDBID, ":IN_idOrigin"}
};

/* define the total number of fields to be bound */
#define NUM_FIELDS 7

/* Retrieval Struct for  statement */
static EWDB_OCIStatementStruct SSStatement;

/* Temporary Storage Buffer params */
#define BUFFERSIZE 8192
static int    iRecordSize;
static int    iRecordsPerBuffer;
static char * pLocalBuffer=NULL;

static int    bInitialized=FALSE;

static  EWDBid  Local_idOrigin;


static int PrepGetMagsForEventExec(EWDBid IN_idOrigin,EWDB_Cursor * ppCursor);
static int PostGetMagsForEventExec(EWDB_MagStruct * pBuffer,int BufferRecLen,
                               int * pNumFound, int * pNumRetrieved);
static int InitGetMagsForEventStatement(char * szStatement, EWDB_OCIStatementStruct *pSS);



int ewdb_api_GetMagsForOrigin(EWDBid IN_idOrigin, EWDB_MagStruct * pMags,
                                int * pNumItemsFound, 
                                int * pNumItemsRetrieved,
                                int IN_BufferRecLen)
{
  EWDB_Cursor  pCursor;

  ewdb_base_SetLastOraAPIActionTime();

  if(IN_idOrigin <= 0 || !pMags || !pNumItemsFound || !pNumItemsRetrieved)
  {
    logit("","ewdb_api_GetPicksByTime(): Invalid params passed in:\n"
             "IN_idOrigin %d, pMags %u, pNumItemsFound %u, pNumItemsRetrieved %u\n",
          IN_idOrigin, pMags, pNumItemsFound, pNumItemsRetrieved);
    return(EWDB_RETURN_FAILURE);
  }

  *pNumItemsFound = 0;
  *pNumItemsRetrieved = 0;

  if( ewdb_base_Reconnect() != EWDB_RETURN_SUCCESS )
  {
    return( EWDB_RETURN_FAILURE );
  }

  if( PrepGetMagsForEventExec(IN_idOrigin,&pCursor)
     != EWDB_RETURN_SUCCESS)
  {
    return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
    logit("","ewdb_api_GetMagsForOrigin(): PrepGetMagsForEventExec() failed!\n");
  }

  if( ewdb_base_SQLExecute(pCursor) )
  {
    ewdb_base_ErrorReport(hEWDBC, pCursor,"ewdb_api_GetMagsForOrigin(): ewdb_base_SQLExecute",1);
    return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  } 

 /* Commit the transaction (all the previous inserts!)
  In Case there is any auditing or logging or debug
  changes made in the stored procedures.
  ****************************************************/
  if (ewdb_base_SQLCommit (hEWDBC))
  {
    ewdb_base_ErrorReport (hEWDBC, hEWDBC,"ewdb_api_GetMagsForOrigin(): ewdb_base_SQLCommit",2);
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  }
 
  if((PostGetMagsForEventExec(pMags,IN_BufferRecLen,
                            pNumItemsFound,pNumItemsRetrieved)) == EWDB_RETURN_FAILURE)
  {
    logit("","ewdb_api_GetMagsForOrigin(): PrepGetMagsForEventExec() failed!\n");
    return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }

  ewdb_base_SetLastOraAPIActionTime();

  if(*pNumItemsFound == *pNumItemsRetrieved)
    return(EWDB_RETURN_SUCCESS);
  else
    return(EWDB_RETURN_WARNING);
}  /* end ewdb_api_GetMagsForOrigin() */


static int InitGetMagsForEventStatement(char * szStatement, EWDB_OCIStatementStruct *pSS)
{
  int LastSize,i;  /* Size of last column array */


  if(!pLocalBuffer)
  {
    /*Allocate space for returned data - never freed */
    pLocalBuffer=malloc(BUFFERSIZE);

    iRecordSize=0;
    iRecordSize += sizeof(EWDBid); /*idMag*/
    iRecordSize += sizeof(EWDBid); /*Local_idOrigin*/
    iRecordSize += pSS->FieldArray[2].Ind; /*dMagAvg*/
    iRecordSize += sizeof(int); /*iNumMags*/
    iRecordSize += pSS->FieldArray[4].Ind; /*dMagErr*/
    iRecordSize += sizeof(int); /*iMagType*/

    /* Make the number of records a multiple of 8 to avoid alignment problems on UNIX */
    iRecordsPerBuffer= (BUFFERSIZE/iRecordSize) & 0xfffffff8;
    
    /*Allocate space for row/col ret lens. - never freed */
    for(i=0;i<pSS->NumOfFields;i++)
    {
      pSS->FieldArray[i].pRetLens=malloc(iRecordsPerBuffer*EWDB_FIELD_RET_LEN);
    }

    /* idMag */
    pSS->FieldArray[0].pVal = &(pLocalBuffer[0]);
    LastSize = sizeof(EWDBid);

    /* Local_idOrigin */
    pSS->FieldArray[1].pVal=(void *)((int)(pSS->FieldArray[0].pVal) +
                                           (LastSize * iRecordsPerBuffer));
    LastSize = sizeof(EWDBid);

    /* dMagAvg */
    pSS->FieldArray[2].pVal=(void *)((int)(pSS->FieldArray[1].pVal) +
                                           (LastSize * iRecordsPerBuffer));
    LastSize = pSS->FieldArray[2].Ind;

    /* iNumMags */
    pSS->FieldArray[3].pVal=(void *)((int)(pSS->FieldArray[2].pVal) +
                                           (LastSize * iRecordsPerBuffer));
    LastSize = sizeof(EWDBid);

    /* dMagErr */
    pSS->FieldArray[4].pVal=(void *)((int)(pSS->FieldArray[3].pVal) +
                                           (LastSize * iRecordsPerBuffer));
    LastSize = pSS->FieldArray[4].Ind;

    /* iMagType */
    pSS->FieldArray[5].pVal=(void *)((int)(pSS->FieldArray[4].pVal) +
                                           (LastSize * iRecordsPerBuffer));
    LastSize = sizeof(EWDBid);

    pSS->FieldArray[6].pVal = &Local_idOrigin;
    LastSize = sizeof(EWDBid);

  } /* end if(!pLocalBuffer) */
  
  if(!pLocalBuffer)
    logit("","InitGetMagsForEventStatement: malloc of pLocalBuffer "
          "failed! Returning.\n");

  return(ewdb_base_RequestCursor(szStatement, pSS,0/*don't force rebind*/));
}  /* end InitGetMagsForEventStatement() */


static int PrepGetMagsForEventExec(EWDBid IN_idOrigin,EWDB_Cursor * ppCursor)
{

  Local_idOrigin = IN_idOrigin;

  /* Initialize the statement parameters */
  if(!bInitialized)
  {
    bInitialized = TRUE;
    memset(&SSStatement,0,sizeof(SSStatement));
    SSStatement.NumOfFields=NUM_FIELDS;
    SSStatement.FieldArray=SQLParamsBindArray;
    SSStatement.RecordSize=0;
  }

  if(InitGetMagsForEventStatement(SQL_STRING, &SSStatement) != EWDB_RETURN_SUCCESS)
  {
    *ppCursor = NULL;
    return(EWDB_RETURN_FAILURE);
  }

  *ppCursor=SSStatement.pCda;

  return(EWDB_RETURN_SUCCESS);
}  /* end PrepGetMagsForEventExec() */


static int PostGetMagsForEventExec(EWDB_MagStruct * pBuffer,int BufferRecLen,
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

    if(ewdb_base_SQLFetchRows(pCursor, iRecordsPerBuffer))
    {
      if(ewdb_base_GetCursorRetCode(pCursor) == EWDB_SQL_ERROR_NO_DATA) 
      {
        done=1;
      }
      else
      {
        ewdb_base_ErrorReport(hEWDBC, pCursor,"PostGetMagsForEventExec:ewdb_base_SQLFetchRows",1);
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


            /* field 1: idMag */
            pBuffer[UCurr].idMag = *(int *)((sizeof(int) * BCurr) +
                                           (int)(pSS->FieldArray[0].pVal));

            /* field 2: Local_idOrigin */
            pBuffer[UCurr].idOrigin = *(int *)((sizeof(int) * BCurr) +
                                           (int)(pSS->FieldArray[1].pVal));

            /* field 3: dMagAvg */
            pTemp =(char *)((pSS->FieldArray[2].Ind * BCurr) +
                                       (int)(pSS->FieldArray[2].pVal));
            pTemp[pSS->FieldArray[2].pRetLens[BCurr]] = 0;
            pBuffer[UCurr].dMagAvg = (float)atof(pTemp);


            /* field 4: iNumMags */
            pBuffer[UCurr].iNumMags = *(int *)((sizeof(int) * BCurr) +
                                           (int)(pSS->FieldArray[3].pVal));


            /* field 5: dMagErr */
            pTemp =(char *)((pSS->FieldArray[4].Ind * BCurr) +
                                       (int)(pSS->FieldArray[4].pVal));
            pTemp[pSS->FieldArray[4].pRetLens[BCurr]] = 0;
            pBuffer[UCurr].dMagErr = (float)atof(pTemp);


            /* field 6: iMagType */
            pBuffer[UCurr].iMagType = *(int *)((sizeof(int) * BCurr) +
                                           (int)(pSS->FieldArray[5].pVal));
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
      while(!ewdb_base_SQLFetchRows(pCursor, iRecordsPerBuffer))
      {
      }
    }

    if(ewdb_base_GetCursorRetCode(pCursor) != EWDB_SQL_ERROR_NO_DATA)
    {
      ewdb_base_ErrorReport(hEWDBC, pCursor,"PostGetMagsForEventExec:ewdb_base_SQLFetchRows",2);
      return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
    }
    /* else */
  }  /* End if(RowsRetrieved > BufferRecLen) */
  
  RecordsProcessed = ewdb_base_GetCursorRowsProcessedCount(pCursor);
  ewdb_base_ReleaseCursor(pCursor);

  *pNumFound = RecordsProcessed;
  if(RecordsProcessed > BufferRecLen)
    *pNumRetrieved = BufferRecLen;
  else
    *pNumRetrieved = RecordsProcessed;

  return(EWDB_RETURN_SUCCESS);
}  /* end PostGetMagsForEventExec() */
