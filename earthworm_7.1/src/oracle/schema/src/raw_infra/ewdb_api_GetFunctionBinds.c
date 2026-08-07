/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    Revision history:
 *     $Log: ewdb_api_GetFunctionBinds.c,v $
 *     Revision 1.4  2001/07/01 21:55:43  davidk
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
 *     Revision 1.3  2001/05/15 02:16:36  davidk
 *     Moved functions around between the apps, DB API, and DB API INTERNAL
 *     levels.  Renamed functions and files.  Added support for amplitude
 *     magnitude types.  Reformatted makefiles.
 *
 *     Revision 1.2  2001/04/06 19:44:27  davidk
 *     Changed a couple of variables to static so that they only
 *     had file scope instead of full global.
 *
 *     Revision 1.1  2001/02/28 17:19:22  lucky
 *     Initial revision
 *
 *     Revision 1.1  2001/02/21 10:05:04  davidk
 *     Initial revision
 *
 */


/* standard includes */
#include <stdlib.h>
#include <stdio.h>
#include <ewdb_cli_base.h>
#include <ewdb_ora_api.h>
#include <ewdb_ew_oci_base.h>

/* sql execution string */
static char SQL_STRING[] =
        "select  idFunctionBind, idDevice, sFunctionName, idFunction, "
        "        tOff, tOn, bOverridable"
        "from ALL_FUNCTIONBIND_INFO "
        "where idDevice = :IN_idDevice"
        "  and tOn     <= :IN_tTime"
        "  and tOff    >= :IN_tTime";

/* array of "bind params" structs for the above sql string */
static EWDB_OCI_SFS SQLParamsBindArray[] =
{
  {0,1,                        0,0,0,OA_EWDBID, "1idFunctionBind"},
  {0,1,                        0,0,0,OA_EWDBID, "2idDevice"},
  {0,1,EWDB_RAW_INFRA_NAME_LEN,0,0,OA_SZ,    "3sFunctionName"},
  {0,1,                        0,0,0,OA_EWDBID, "4idFunction"},
  {0,1,EWDB_DOUBLE_STRING_LEN, 0,0,OA_DOUBLE, "5tOff"},
  {0,1,EWDB_DOUBLE_STRING_LEN, 0,0,OA_DOUBLE, "6tOn"},
  {0,1,                        0,0,0,OA_INT,    "7bOverridable"},
  {0,1,                        0,0,0,OA_EWDBID, ":IN_idDevice"},
  {0,1,                        0,0,0,OA_DOUBLE, ":IN_tTime"}
};

/***************************/
/*   sample bind structs   */
/***************************/
/* 
 * input params or procedure params 
 * integer
  {0,1,  0,0,0,OA_INT,    ":OUT_RetCode"}
 * float
  {0,1,  0,0,0,OA_FLOAT,  ":OUT_dNumber"}
 * double
  {0,1,  0,0,0,OA_DOUBLE, ":OUT_dNumber"}
 * string
  {0,1,  0,0,0,OA_SZ,     ":OUT_sString"}
 * EWDBid
  {0,1,  0,0,0,OA_EWDBID, ":OUT_idRecord"}
 * char  (single)
  {0,1,  0,0,0,OA_CHAR,   "1cPZType"}


 * list retrieval
 * integer
  {0,1,  0,0,0,OA_INT,    "5iNumber"},
 * float
  {0,1, 20,0,0,OA_FLOAT,  "1dNumber"}
 * double
  {0,1, 20,0,0,OA_DOUBLE, "2dNumber"}
 * string
  {0,1, 10,0,0,OA_SZ,     "12sString"},
 * EWDBid
  {0,1,  0,0,0,OA_EWDBID, "5idRecord"},
 * char  (single)
  {0,1,  0,0,0,OA_CHAR,   "1cPZType"}
*/
/* end sample bind structs */
/***************************/




/* define the max number of usable fields in the SQLParamsBindArray */
static const int NumFieldsInBindArray = 9;

/* declare local copies of all the input/output params */
static int iRetCode;
static EWDBid idDevice;
static char sztTime[EWDB_DOUBLE_STRING_LEN];

/* declare our local buffer.  We should probably be using
   a more global one to save space, but for now... 
   Note:  the record buffer is made to be twice the size
   of the RecordBufferSize, because of an unresolved bug
   in the retrieval logic that causes more than just the
   buffer length to be used and thus strange things to
   happen.  This bug may now be fixed, but if so it wasn't
   documented, and all retrieval code has this hack in it.
   DK 02/01/2001 */

static char pRecordBuffer[8192 * 2];
static const int RecordBufferSize=8192;
static int iRecordSize;
static int iRecordsPerBuffer;

/* declare Statement Struct */
static EWDB_OCIStatementStruct SSStatement;


int PrepGetFunctionBindsExec(EWDBid IN_idDevice, double IN_tTime, 
                             EWDB_Cursor * ppCursor);
int PostGetFunctionBindsExec(EWDB_FunctionBindStruct * pBuffer,int BufferRecLen);
int InitGetFunctionBindsStatement(char * szStatement, EWDB_OCIStatementStruct *pSS);


int InitGetFunctionBindsStatement(char * szStatement, EWDB_OCIStatementStruct *pSS)
{
  int LastSize,i;  /* Size of last column array */

  iRecordSize = 0;
  iRecordSize += sizeof(EWDBid); /*idFunctionBind*/
  iRecordSize += sizeof(EWDBid); /*idDevice*/
  iRecordSize += EWDB_RAW_INFRA_NAME_LEN+1; /*sFunctionName*/
  iRecordSize += sizeof(EWDBid); /*idFunction*/
  iRecordSize += EWDB_DOUBLE_STRING_LEN; /*tOff*/
  iRecordSize += EWDB_DOUBLE_STRING_LEN; /*tOn*/
  iRecordSize += sizeof(int); /*bOverridable*/

  iRecordsPerBuffer=RecordBufferSize/iRecordSize/sizeof(int)*sizeof(int); 
  /* Make iRecordsPerBuffer a multiple of sizeof(int) to avoid
     pointer arithmetic problems on UNIX Davidk 6/24/98 
  **********************************************************/

  /*Allocate (free'd in PostXXX()) space for row/col ret lens.*/
  for(i=0;i<pSS->NumOfFields;i++)
  {
    pSS->FieldArray[i].pRetLens=malloc(iRecordsPerBuffer*EWDB_FIELD_RET_LEN);
  }

  /* set the pointer for each columns data.  Instead of writing
     data 1 record at a time, as if you were writing an array of
     structures in C, we get the data back as column 1 data for
     all rows, then column 2 data for all rows, etc.  So we have
     to segment the buffer, and set the pointers appropriately.
     This is pretty straight forward, the only difference on a
     per column basis is the LastSize variable.  If the column
     is of String type, then LastSize should be set to the buffer
     size that is stored in the Ind(indicator) field of the 
     EWDB_OCI_SFS struct for that column.  If instead the column,
     is an integral number type, then use sizeof(column type) 
     for the value of LastSize.  Note: we convert all non integral
     numbers (floats & doubles) to strings when passing to/from
     DB.  DK 02/01/2001
  ***************************************************************/
  pSS->FieldArray[0].pVal= pRecordBuffer;
  LastSize=sizeof(EWDBid);
  pSS->FieldArray[1].pVal= (void *) (
    (int)(pSS->FieldArray[0].pVal)+(LastSize*iRecordsPerBuffer));
  LastSize=sizeof(EWDBid);
  pSS->FieldArray[2].pVal= (void *) (
    (int)(pSS->FieldArray[1].pVal)+(LastSize*iRecordsPerBuffer));
  LastSize=pSS->FieldArray[2].Ind; /* sFunctionName */
  pSS->FieldArray[3].pVal= (void *) (
    (int)(pSS->FieldArray[2].pVal)+(LastSize*iRecordsPerBuffer));
  LastSize=sizeof(EWDBid);
  pSS->FieldArray[4].pVal= (void *) (
    (int)(pSS->FieldArray[3].pVal)+(LastSize*iRecordsPerBuffer));
  LastSize=pSS->FieldArray[4].Ind; /* tOff */
  pSS->FieldArray[5].pVal= (void *) (
    (int)(pSS->FieldArray[4].pVal)+(LastSize*iRecordsPerBuffer));
  LastSize=pSS->FieldArray[5].Ind; /* tOn */
  pSS->FieldArray[6].pVal= (void *) (
    (int)(pSS->FieldArray[5].pVal)+(LastSize*iRecordsPerBuffer));
  LastSize=sizeof(int);

  pSS->FieldArray[7].pVal = &idDevice;
  pSS->FieldArray[8].pVal = sztTime;

  ewdb_base_RequestCursor(szStatement, pSS,1);

  return(EWDB_RETURN_SUCCESS);
}  /* end InitXXX() */

int PrepGetFunctionBindsExec(EWDBid IN_idDevice, double IN_tTime, 
                             EWDB_Cursor * ppCursor)
{

  /* Copy misc. local variables to the statement struct */
  SSStatement.NumOfFields = NumFieldsInBindArray;
  SSStatement.FieldArray = SQLParamsBindArray;
  SSStatement.RecordSize = 0;

  /* Copy input params to local variables */
  idDevice       = IN_idDevice;

  sprintf(sztTime,"%.4f",IN_tTime);

  /* sample strncpy *
  strncpy(szSlotName, IN_szSlotName, sizeof(szSlotName) - 1);
  szSlotName[sizeof(szSlotName) - 1] = 0x00;
  *********************************/

  if (InitGetFunctionBindsStatement (SQL_STRING,
                           &SSStatement) != EWDB_RETURN_SUCCESS)
  {
    logit ("", "Call to InitGetFunctionBindsStatement failed!\n");
    return EWDB_RETURN_FAILURE;
  }

  *ppCursor = SSStatement.pCda;
  
  return(EWDB_RETURN_SUCCESS);
}  /* End PrepGetFunctionBindsExec() */


int PostGetFunctionBindsExec(EWDB_FunctionBindStruct * pBuffer,int BufferRecLen)
{
    /* 
    The statement has been executed.  we need to do the following:
    1.  While we haven't fetched all of the records or reached capacity,
        fetch a new bunch of records into the buffer.
    2.  For each bunch:  Format each row into EWDB_XXXStruct format, 
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
  int RecordsProcessed=0;
  int i;

  int iRetCode = EWDB_RETURN_SUCCESS;

  while(!done)
  {
    if (ewdb_base_SQLFetchRows(pCursor, iRecordsPerBuffer))
    {
      if (ewdb_base_GetCursorRetCode(pCursor) == EWDB_SQL_ERROR_NO_DATA) 
      {
        done=1;
      }
      else
      {
        ewdb_base_ErrorReport(hEWDBC, pCursor,
                         "PostGetFunctionBindsExec:ewdb_base_SQLFetchRows",1);
        iRetCode = EWDB_RETURN_FAILURE;
        goto Finish;
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

        /* initialize the current record portion of the buffer */
        memset(&(pBuffer[UCurr]), 0, sizeof(EWDB_FunctionBindStruct));

        /* idFunctionBind */
        pBuffer[UCurr].idFunctionBind =
          * (int *)((sizeof(EWDBid)*BCurr) + (int)(pSS->FieldArray[0].pVal));

        /* idDevice */
        pBuffer[UCurr].idDevice =
          * (int *)((sizeof(EWDBid)*BCurr) + (int)(pSS->FieldArray[1].pVal));

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
        /* szFunctionName */
        pTemp = (char *) ((pSS->FieldArray[2].Ind*BCurr) +
                          (int)(pSS->FieldArray[2].pVal) );
        pTemp[pSS->FieldArray[1].pRetLens[BCurr]]=0;
        strcpy(pBuffer[UCurr].szFunctionName,pTemp);

        /* idFunction */
        pBuffer[UCurr].idFunction =
          * (int *)((sizeof(EWDBid)*BCurr) + (int)(pSS->FieldArray[3].pVal));

        /* tOff */
        pTemp=(char *) ((pSS->FieldArray[4].Ind*BCurr) + (int)(pSS->FieldArray[4].pVal) );
        pTemp[pSS->FieldArray[4].pRetLens[BCurr]]=0;
        pBuffer[UCurr].tOff = atof(pTemp);

        /* tOn */
        pTemp=(char *) ((pSS->FieldArray[5].Ind*BCurr) + (int)(pSS->FieldArray[5].pVal) );
        pTemp[pSS->FieldArray[5].pRetLens[BCurr]]=0;
        pBuffer[UCurr].tOn = atof(pTemp);

        /* bOverridable */
        pBuffer[UCurr].bOverridable =
          * (int *)((sizeof(int)*BCurr) + (int)(pSS->FieldArray[6].pVal));

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
        /* add debug information here if desired */
      }
    }

    if(ewdb_base_GetCursorRetCode(pCursor) != EWDB_SQL_ERROR_NO_DATA)
    {
      ewdb_base_ErrorReport(hEWDBC, pCursor,
                       "PostGetFunctionBindsExec:ewdb_base_SQLFetchRows",2);
        iRetCode = EWDB_RETURN_FAILURE;
        goto Finish;
    }
    /* else */
  }  /* End if (RowsRetrieved > BufferRecLen) */
  
  RecordsProcessed = ewdb_base_GetCursorRowsProcessedCount(pCursor);
  ewdb_base_ReleaseCursor (pCursor);


Finish:
  /* free data allocated in InitXXX() */
  for(i=0;i<pSS->NumOfFields;i++)
  {
    pSS->FieldArray[i].pRetLens=malloc(iRecordsPerBuffer*EWDB_FIELD_RET_LEN);
  }

  /* return proper code */
  if(iRetCode == EWDB_RETURN_SUCCESS)
    return(RecordsProcessed);
  else if(iRetCode == EWDB_RETURN_SUCCESS)
    return(ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  else
    return(iRetCode);

}  /* end of PostXXX() */
