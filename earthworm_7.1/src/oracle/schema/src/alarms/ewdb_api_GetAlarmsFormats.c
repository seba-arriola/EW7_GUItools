/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_api_GetAlarmsFormats.c,v 1.8 2005/06/03 22:32:16 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_api_GetAlarmsFormats.c,v $
 *     Revision 1.8  2005/06/03 22:32:16  davidk
 *     DB API Cleanup
 *
 *     Revision 1.7  2003/10/22 15:50:17  davidk
 *     Enlarged the BUFFERSIZE of the internal buffer to incorporate the large record size > 8k.
 *
 *     Revision 1.6  2003/10/21 15:54:56  davidk
 *     Fixed a bug introduced in last "API Cleanup" change.
 *     Fixed code so that SSStatement params are correctly set in InitXXX().
 *
 *     Revision 1.5  2003/10/09 17:56:55  davidk
 *     Cleanup of API call, to match standard list-retrieval format.
 *
 *     Revision 1.4  2001/08/07 16:51:06  lucky
 *     Pre v6.0 checkin
 *
 *     Revision 1.3  2001/07/31 20:44:40  lucky
 *     Changed from User to Recipient
 *
 *     Revision 1.2  2001/07/28 00:44:27  lucky
 *      State of the code after debugging and testing prior to v6.0 release freeze.
 *
 *     Revision 1.1  2001/05/15 02:16:15  davidk
 *     Initial revision
 *
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <ewdb_cli_base.h>
#include <ewdb_ew_oci_base.h>
#include <ewdb_ora_api.h>
#include <ewdb_oci_base_internal.h>


/* 
  We work in two ways: (1) get all formats, and 
  (2) get a specific formats
*/

static char SQL_STRING[256];

static char SQL_STRING_BASE[] =
        "select idFormat, sDescription, sFmtInsert, sFmtDelete from ALL_AlarmsFormat_INFO ";

static char SQL_STRING_WHERE[] =
        " where idFormat = :IN_idFormat";


static EWDB_OCI_SFS SQLParamsBindArray[] =

{
  {0,1,0,0,0,OA_INT,    "1idFormat"},
  {0,1,256,0,0,OA_SZ,   "2sDescription"},
  {0,1,EWDB_ALARMS_MAX_FORMAT_LEN,0,0,OA_SZ,   "3sFmtInsert"},
  {0,1,EWDB_ALARMS_MAX_FORMAT_LEN,0,0,OA_SZ,   "4sFmtDelete"},
  {0,1,0,0,0,OA_INT,    ":IN_idFormat"},
};

/* define the total number of fields to be bound */
#define NUM_FIELDS 5

/* (SQL) Statement Struct for our statement */
static EWDB_OCIStatementStruct SSStatement;

/* Temporary Storage Buffer params */
#define BUFFERSIZE 245760
static int    iRecordSize;
static int    iRecordsPerBuffer;
static char * pLocalBuffer=NULL;
static int    bInitialized=FALSE;

static  int   Local_idFormat;

/********************************
      FUNCTION PROTOTYPES
********************************/
static int PrepGetAlarmsFormatsExec(int, EWDB_Cursor *);
static int InitGetAlarmsFormatsStatement(char *, EWDB_OCIStatementStruct *);
static int PostGetAlarmsFormatsExec(EWDB_AlarmsFormatStruct *, int);
/*******************************/


int  ewdb_api_GetAlarmsFormats(int IN_idFormat,EWDB_AlarmsFormatStruct *pFormat,
                              int *pNumFound, int *pNumRetrieved, int BufferLen)
{

  EWDB_Cursor pCursor;

  if ((pFormat == NULL) || (pNumFound == NULL) || (pNumRetrieved == NULL) ||
        (BufferLen <= 0))
  {
    logit ("", "ewdb_api_GetAlarmsFormats(): Invalid arguments passed in.\n");
    return EWDB_RETURN_FAILURE;
  }

  ewdb_base_SetLastOraAPIActionTime ();

  if (ewdb_base_Reconnect () != EWDB_RETURN_SUCCESS)
  {
    logit ("", "ewdb_api_GetAlarmsFormats(): ewdb_base_Reconnect failed.\n");
    return (EWDB_RETURN_FAILURE);
  }

  if (PrepGetAlarmsFormatsExec (IN_idFormat, &pCursor) != EWDB_RETURN_SUCCESS)
  {
    logit ("", "ewdb_api_GetAlarmsFormats(): PrepGetAlarmsFormatsExec failed.\n");
    return (ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }

  if (ewdb_base_SQLExecute (pCursor))
  {
    ewdb_base_ErrorReport (hEWDBC, pCursor,"ewdb_api_GetAlarmsFormats(): ewdb_base_SQLExecute", 1);
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  }

  /* Commit the transaction (all the previous inserts!)
     In Case there is any auditing or logging or debug
     changes made in the stored procedures.
  ****************************************************/
  if (ewdb_base_SQLCommit (hEWDBC))
  {
    ewdb_base_ErrorReport (hEWDBC, hEWDBC,"ewdb_api_GetAlarmsFormats():ewdb_base_SQLCommit",2);
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  }

  if ((*pNumFound = PostGetAlarmsFormatsExec (pFormat, BufferLen)) == EWDB_RETURN_FAILURE)
  {
    logit ("", "ewdb_api_GetAlarmsFormats(): PostGetAlarmsFormatsExec failed.\n");
    return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }

  ewdb_base_SetLastOraAPIActionTime ();

  if (*pNumFound <= BufferLen)
  {
    *pNumRetrieved = *pNumFound;
    return (EWDB_RETURN_SUCCESS);
  }
  else
  {
    *pNumRetrieved = BufferLen;
    return (EWDB_RETURN_WARNING);
  }
}  /* end ewdb_api_GetAlarmsFormats() */

/******************* InitGetAlarmsFormatsStatement *******************/
static int InitGetAlarmsFormatsStatement(char *szStatement, EWDB_OCIStatementStruct *pSS)
{

  int LastSize,i;  /* Size of last column array */


  if(!pLocalBuffer)
  {
    /*Allocate space for returned data - never freed */
    pLocalBuffer=malloc(BUFFERSIZE);

    iRecordSize=0;
    iRecordSize += sizeof(EWDBid); /*IN_idFormat*/
    iRecordSize += pSS->FieldArray[1].Ind; /*sDescription*/
    iRecordSize += pSS->FieldArray[2].Ind; /*sFmtInsert*/
    iRecordSize += pSS->FieldArray[3].Ind; /*sFmtDelete*/

    /* Make the number of records a multiple of 8 to avoid alignment problems on UNIX */
    iRecordsPerBuffer= (BUFFERSIZE/iRecordSize) & 0xfffffff8;
    
    /*Allocate space for row/col ret lens. - never freed */
    for(i=0;i<pSS->NumOfFields;i++)
    {
      pSS->FieldArray[i].pRetLens=malloc(iRecordsPerBuffer*EWDB_FIELD_RET_LEN);
    }
    
    pSS->FieldArray[0].pVal = &(pLocalBuffer[0]);
    LastSize = sizeof(int); /* IN_idFormat */
    
    pSS->FieldArray[1].pVal= (void *) ((int)(pSS->FieldArray[0].pVal) + 
      (LastSize * iRecordsPerBuffer));
    LastSize = pSS->FieldArray[1].Ind; /* sDescription */
    
    pSS->FieldArray[2].pVal= (void *) ((int)(pSS->FieldArray[1].pVal) + 
      (LastSize * iRecordsPerBuffer));
    LastSize = pSS->FieldArray[2].Ind; /* sFmtInsert */
    
    pSS->FieldArray[3].pVal= (void *) ((int)(pSS->FieldArray[2].pVal) + 
      (LastSize * iRecordsPerBuffer));
    LastSize = pSS->FieldArray[3].Ind; /* sFmtDelete */
    
    pSS->FieldArray[4].pVal= &Local_idFormat;

  } /* end if(!pLocalBuffer) */
  
  if(!pLocalBuffer)
  {
    logit("","InitGetAlarmsFormatsStatement: malloc of pLocalBuffer "
          "failed! Returning.\n");
    return(EWDB_RETURN_FAILURE);
  }

  return(ewdb_base_RequestCursor(szStatement, pSS,0/*don't force rebind*/));
}  /* End InitGetAlarmsFormatsStatement() */


/******************* PrepGetAlarmsFormatsExec *******************/
static int PrepGetAlarmsFormatsExec (int IN_idFormat, EWDB_Cursor *ppCursor)
{

  if (ppCursor == NULL)
  {
    logit ("", "PrepGetAlarmsFormatsExec(): Invalid arguments passed in.\n");
    return EWDB_RETURN_FAILURE;
  }

  SSStatement.NumOfFields = NUM_FIELDS;
  SSStatement.FieldArray = SQLParamsBindArray;
  SSStatement.RecordSize = 0;

  if (IN_idFormat < 0)
  {
    SQLParamsBindArray[4].UseField = FALSE;
    sprintf(SQL_STRING, "%s", SQL_STRING_BASE);
  }
  else
  {
    Local_idFormat = IN_idFormat;

    SQLParamsBindArray[4].UseField = TRUE;
    sprintf(SQL_STRING, "%s%s", SQL_STRING_BASE, SQL_STRING_WHERE);
  }

  if(InitGetAlarmsFormatsStatement(SQL_STRING, &SSStatement) != EWDB_RETURN_SUCCESS)
  {
    *ppCursor = NULL;
    return(EWDB_RETURN_FAILURE);
  }

  *ppCursor = SSStatement.pCda;

  return (EWDB_RETURN_SUCCESS);
} /* end PrepGetAlarmsFormatsExec() */


/******************* PostGetAlarmsFormatsExec *******************/
static int PostGetAlarmsFormatsExec(EWDB_AlarmsFormatStruct *pBuffer, 
                                    int BufferRecLen)
{
  int           done = 0;
  int           RowsRetrieved = 0;
  int           RowsDone = 0;
  EWDB_Cursor       pCursor = SSStatement.pCda;
  char           *pTemp;
  int           BCurr,UCurr;
  int           RowsProcessed;
  EWDB_OCIStatementStruct  *pSS=&SSStatement;

  while (!done)
  {
    memset(pLocalBuffer, 0, BUFFERSIZE);

    if (ewdb_base_SQLFetchRows (pCursor, iRecordsPerBuffer))
    {
      if (ewdb_base_GetCursorRetCode(pCursor) == EWDB_SQL_ERROR_NO_DATA) 
        done=1;
      else
      {
        ewdb_base_ErrorReport (hEWDBC, pCursor,"PostGetAlarmsFormatsExec:ewdb_base_SQLFetchRows", 1);
        return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
      }
    }

    RowsRetrieved=ewdb_base_GetCursorRowsProcessedCount(pCursor);

    for(; RowsDone < RowsRetrieved; RowsDone++)
    {
      if(RowsDone >= BufferRecLen)
      {
        done=1;
        break;
      }

      /* Copy from DB rows into the output buffer */
      BCurr = RowsDone % iRecordsPerBuffer;
      UCurr = RowsDone;

      /* indx 0: IN_idFormat */
      pBuffer[UCurr].idFormat = *(int *)((sizeof (int) * BCurr) + 
                      (int)(pSS->FieldArray[0].pVal));

      /* indx 1: sDescription */
      pTemp = (char *) ((pSS->FieldArray[1].Ind * BCurr) + 
                    (int)(pSS->FieldArray[1].pVal));
      pTemp[pSS->FieldArray[1].pRetLens[BCurr]] = 0;
      strcpy (pBuffer[UCurr].sDescription, pTemp);

      /* indx 2: sFmtInsert */
      pTemp = (char *) ((pSS->FieldArray[2].Ind * BCurr) + 
                    (int)(pSS->FieldArray[2].pVal));
      pTemp[pSS->FieldArray[2].pRetLens[BCurr]] = 0;
      strcpy (pBuffer[UCurr].sFmtInsert, pTemp);

      /* indx 3: sFmtDelete */
      pTemp = (char *) ((pSS->FieldArray[3].Ind * BCurr) + 
                    (int)(pSS->FieldArray[3].pVal));
      pTemp[pSS->FieldArray[3].pRetLens[BCurr]] = 0;
      strcpy (pBuffer[UCurr].sFmtDelete, pTemp);

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
                       "PostGetAlarmsFormatsExec:ewdb_base_SQLFetchRows",2);
      return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
    }
    /* else */
  }  /* End if(RowsRetrieved > BufferRecLen) */
  
  RowsProcessed = ewdb_base_GetCursorRowsProcessedCount(pCursor);

  ewdb_base_ReleaseCursor(pCursor);

  return(RowsProcessed);
}  /* End PostGetAlarmsFormatsExec() */


