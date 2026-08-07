/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_api_GetSelectResults.c,v 1.1 2005/06/21 20:11:54 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_api_GetSelectResults.c,v $
 *     Revision 1.1  2005/06/21 20:11:54  davidk
 *     function templates for use in creating new API functions.
 *
 *
 */


#include <ewdb_cli_base.h>
#include <ewdb_ora_api.h>
#include <ewdb_ew_oci_base.h>

 static char SQL_STRING[] =
       "select idRecord, dDouble, dFloat, iInt, cChar, sString "
       " from ALL_RECORD_INFO "
       " where idFK = :IN_idFK";

static EWDB_OCI_SFS SQLParamsBindArray[] = 
{
  {0,1,0,0,0,OA_EWDBID, "1idRecord"},
  {0,1,20,0,0,OA_DOUBLE,"2dDouble"},
  {0,1,20,0,0,OA_DOUBLE,"3dFloat"},
  {0,1,0,0,0,OA_INT,    "4iInt"},
  {0,1,2,0,0,OA_CHAR,   "5cChar"},
  {0,1,64,0,0,OA_SZ,    "6sString"},
  {0,1,0,0,0,OA_EWDBID, ":IN_idFK"}
};

/* define the total number of fields to be bound */
#define NUM_FIELDS 7

/* Statement Struct for SQL statement */
static EWDB_OCIStatementStruct SSStatement;

/* Temporary Storage Buffer params */
#define BUFFERSIZE 8192

static int    iRecordSize;
static int    iRecordsPerBuffer;
static char * pLocalBuffer=NULL;

static int    bInitialized=FALSE;

/* Local structures and buffers */
static  EWDBid  Local_idFK;

/****************************************************************/
/****************************************************************/
/********************************
      FUNCTION PROTOTYPES
********************************/
static int PrepGetSelectResultsExec(EWDBid IN_idFK, EWDB_Cursor * ppCursor);
static int PostGetSelectResultsExec(EWDB_RecordStruct * pRecords, int BufferRecLen);
static int InitGetSelectResultsStatement(char * szStatement, 
                                         EWDB_OCIStatementStruct *pSS);


int ewdb_api_GetSelectResults(EWDBid IN_idFK, EWDB_RecordStruct * pRecords, 
                              int * pNumItemsFound, int * pNumItemsRetrieved, int iBufferLen)
/* retrieves all of the Mw records for a given Origin.  One of those Mw records
   should have a valid idMag value, which indicates it is the "final solution". 
   EWDB_RETURN_WARNING indicates the pBuffer wasn't large enough.
   EWDB_RETURN_FAILURE indicates function encountered unspecified error, including idFK was invalid.
 */
{
  EWDB_Cursor  pCursor;
 
  /* validate input parameters */
  if(!(IN_idFK && pRecords && pNumItemsFound && pNumItemsRetrieved && iBufferLen))
  {
    logit("et", "ewdb_api_GetSelectResults():  Error null parameter(s) "
                "passed in! (%u/%u/%u/%u/%u\n",
          IN_idFK, pRecords, pNumItemsFound, 
          pNumItemsRetrieved, iBufferLen);
    return(EWDB_RETURN_FAILURE);
  }

  /* reset the dbms connection timeout */
  ewdb_base_SetLastOraAPIActionTime();

  /* make sure we're connected to the database. */
  if( ewdb_base_Reconnect() != EWDB_RETURN_SUCCESS )
  {
    return( EWDB_RETURN_FAILURE );
  }

  /* Call Prep() to copy caller's data to internal buffers and setup sql interface */
  if( PrepGetSelectResultsExec(IN_idFK, &pCursor)
     != EWDB_RETURN_SUCCESS)
  {
    return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }

  /* execute the sql statement */
  if( ewdb_base_SQLExecute(pCursor) )
  {
    ewdb_base_ErrorReport(hEWDBC, pCursor,"ewdb_api_GetSelectResults(): ewdb_base_SQLExecute",1);
    return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  } 

 /* Commit the SQL transaction in case there is any auditing or logging 
  ****************************************************/
  if(ewdb_base_SQLCommit(hEWDBC))
  {
    ewdb_base_ErrorReport(hEWDBC, hEWDBC,
                      "ewdb_api_GetSelectResults(): ewdb_base_SQLCommit",2);
    return(ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }
  
  /* call Post() to fetch data, check return codes, and copy data into caller's buffer */
  if((*pNumItemsFound = PostGetSelectResultsExec(pBuffer, iBufferLen)) 
     == EWDB_RETURN_FAILURE)
  {
    return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }

  /* reset the dbms connection timeout */
  ewdb_base_SetLastOraAPIActionTime();

  if(*pNumItemsFound <= iBufferLen)
  {
    *pNumItemsRetrieved = *pNumItemsFound;
    return(EWDB_RETURN_SUCCESS);
  }
  else
  {
    *pNumItemsRetrieved = iBufferLen;
    return(EWDB_RETURN_WARNING);
  }
}  /* end ewdb_internal_GetSelectResults() */


static int InitGetSelectResultsStatement(char * szStatement, 
                                         EWDB_OCIStatementStruct *pSS)
{
  int LastSize,i;  /* Size of last column array */

  if(!pLocalBuffer)
  {
    /*Allocate space for returned data - never freed */
    pLocalBuffer=malloc(BUFFERSIZE);


    /* determine the record size for this result set */
    iRecordSize=0;
    iRecordSize += sizeof(EWDBid);        /*1idRecord*/
    iRecordSize += pSS->FieldArray[1].Ind;/*2dDouble*/
    iRecordSize += pSS->FieldArray[2].Ind;/*3dFloat*/
    iRecordSize += sizeof(int);           /**/
    iRecordSize += pSS->FieldArray[4].Ind;/*5cChar*/
    iRecordSize += pSS->FieldArray[5].Ind;/*6sString*/

    /* Make the number of records a multiple of 8 to avoid alignment problems on UNIX */
    iRecordsPerBuffer= (BUFFERSIZE/iRecordSize) & 0xfffffff8;
    
    /*Allocate space for row/col ret lens. - never freed */
    for(i=0;i<pSS->NumOfFields;i++)
    {
      pSS->FieldArray[i].pRetLens=malloc(iRecordsPerBuffer*EWDB_FIELD_RET_LEN);
    }

    /* divide the buffer up into blocks based on the number of records and 
       size of each field 
     ***********************************************************************/
    pSS->FieldArray[0].pVal=&(pLocalBuffer[0]);
    LastSize=sizeof(EWDBid);         /* 0idMw */
    pSS->FieldArray[1].pVal=(void *)(
      (int)(pSS->FieldArray[0].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[1].Ind; /* 2dDouble */
    pSS->FieldArray[2].pVal=(void *)(
      (int)(pSS->FieldArray[1].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[2].Ind; /* 3dFloat */
    pSS->FieldArray[3].pVal=(void *)(
      (int)(pSS->FieldArray[2].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=sizeof(int);            /* 4iInt */
    pSS->FieldArray[4].pVal=(void *)(
      (int)(pSS->FieldArray[3].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[4].Ind; /* 5cChar */
    pSS->FieldArray[5].pVal=(void *)(
      (int)(pSS->FieldArray[4].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[5].Ind; /* 6sString */

    pSS->FieldArray[6].pVal=&Local_idFK;

  } /* end if(!pLocalBuffer) */

  /* ensure that someone hasn't come along and screwed up our buffer pointer */
  if(!pLocalBuffer)
    logit("","InitGetSelectResultsStatement: malloc of pLocalBuffer "
          "failed! Returning.\n");

  /* get a cursor and parse the sql statement */
  return(ewdb_base_RequestCursor(szStatement, pSS,0/*don't force rebind*/));
}  /* End InitGetSelectResultsStatement() */


static int PrepGetSelectResultsExec(EWDBid IN_idFK, EWDB_Cursor * ppCursor)
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

  /* copy user's data over to local buffers */
  Local_idFK  = idFK;

  /* call Init() function to prep the dbms client environment */
  if(InitGetSelectResultsStatement(SQL_STRING, &SSStatement) != EWDB_RETURN_SUCCESS)
  {
    *ppCursor = NULL;
    return(EWDB_RETURN_FAILURE);
  }

  *ppCursor=SSStatement.pCda;

  return(EWDB_RETURN_SUCCESS);
}  /* End PrepGetSelectResultsExec() */


static int PostGetSelectResultsExec(EWDB_RecordStruct * pRecords, int BufferRecLen)
{
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
    /* reinitialize the buffer before each fetch to ensure NULL columns end up
       NULL, and not whatever value was there before.  Oracle will not write a
       NULL value into the provided buffer, so the buffer retains whatever value
       it had prior to the Oracle fetch.
       DK 
     ****************************************************************************/
    memset(pLocalBuffer, 0, BUFFERSIZE);

    if(ewdb_base_SQLFetchRows(pCursor, iRecordsPerBuffer))
    {
      if(ewdb_base_GetCursorRetCode(pCursor) == EWDB_SQL_ERROR_NO_DATA) 
      {
        done=1;
      }
      else
      {
        ewdb_base_ErrorReport(hEWDBC, pCursor,"PostGetSelectResultsExec:ewdb_base_SQLFetchRows",1);
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


        /* Initialize the current caller's struct to all 0's. */
        memset(&pBuffer[UCurr],0,sizeof(EWDB_ArrivalStruct));

        pBuffer[UCurr].idRecord=*(EWDBid *)((sizeof(int)*BCurr) +(int)(pSS->FieldArray[0].pVal));

        pTemp=(char *)((pSS->FieldArray[1].Ind*BCurr) +(int)(pSS->FieldArray[1].pVal) );
        pTemp[pSS->FieldArray[1].pRetLens[BCurr]]=0;
        pBuffer[UCurr].dDouble = atof(pTemp);

        pTemp=(char *)((pSS->FieldArray[2].Ind*BCurr) +(int)(pSS->FieldArray[2].pVal) );
        pTemp[pSS->FieldArray[2].pRetLens[BCurr]]=0;
        pBuffer[UCurr].dFloat = (float) atof(pTemp);

        pBuffer[UCurr].iInt =*(int *)((sizeof(int)*BCurr) +(int)(pSS->FieldArray[3].pVal));

        pTemp=(char *)((pSS->FieldArray[4].Ind*BCurr) +(int)(pSS->FieldArray[4].pVal) );
        pTemp[pSS->FieldArray[4].pRetLens[BCurr]]=0;
        pBuffer[UCurr].cChar = pTemp[0];

        pTemp=(char *)((pSS->FieldArray[5].Ind*BCurr) +(int)(pSS->FieldArray[5].pVal) );
        pTemp[pSS->FieldArray[5].pRetLens[BCurr]]=0;
        strncpy(pBuffer[UCurr].szString, pTemp, sizeof(pBuffer[UCurr].szString)-1);

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
      ewdb_base_ErrorReport(hEWDBC, pCursor,"PostGetSelectResultsExec:ewdb_base_SQLFetchRows",2);
      return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
    }
    /* else */
  }  /* End if(RowsRetrieved > BufferRecLen) */
  
  RowsProcessed = ewdb_base_GetCursorRowsProcessedCount(pCursor);

  ewdb_base_ReleaseCursor(pCursor);

  return(RowsProcessed);
}  /* End PostGetGetSelectResultsExec() */

