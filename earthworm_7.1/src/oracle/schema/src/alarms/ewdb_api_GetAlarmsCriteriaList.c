/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_api_GetAlarmsCriteriaList.c,v 1.9 2005/06/10 16:07:48 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_api_GetAlarmsCriteriaList.c,v $
 *     Revision 1.9  2005/06/10 16:07:48  davidk
 *     DB Cleanup.  Fixed comments.
 *
 *     Revision 1.8  2005/06/03 22:32:16  davidk
 *     DB API Cleanup
 *
 *     Revision 1.7  2003/10/21 15:56:28  davidk
 *     Finished API cleanup of function internals.
 *     Expanded buffer to 81920 because of recordsize.
 *
 *     Revision 1.6  2003/10/09 17:56:54  davidk
 *     Cleanup of API call, to match standard list-retrieval format.
 *
 *     Revision 1.5  2001/08/07 16:51:06  lucky
 *     Pre v6.0 checkin
 *
 *     Revision 1.4  2001/07/31 20:44:40  lucky
 *     Changed from User to Recipient
 *
 *     Revision 1.3  2001/07/28 00:44:27  lucky
 *      State of the code after debugging and testing prior to v6.0 release freeze.
 *
 *     Revision 1.2  2001/05/15 18:47:41  davidk
 *     Moved functions around between the apps, DB API, and DB API INTERNAL
 *     levels.  Renamed functions and files.  Added support for amplitude
 *     magnitude types.  Reformatted makefiles.
 *
 *     Revision 1.1  2001/02/28 17:24:59  lucky
 *     Initial revision
 *
 *     Revision 1.1  2001/02/28 17:19:22  lucky
 *     Initial revision
 *
 */

#include <ewdb_cli_base.h>
#include <ewdb_ew_oci_base.h>
#include <ewdb_ora_api.h>

/* 
  We work in two ways: (1) get all programs, and 
  (2) get a specific program
*/
static char SQL_STRING[256];

static char SQL_STRING_BASE[] =
        "select idCritProgram, sProgName, sProgDescription, sProgDir from ALL_CriteriaProgram_INFO ";

static char SQL_STRING_WHERE[] =
        " where idCritProgram = :IN_idCritProgram";

static EWDB_OCI_SFS SQLParamsBindArray[] =

{
  {0,1,0,0,0,OA_EWDBID,"1idCritProgram"},
  {0,1,256,0,0,OA_SZ,"2sProgName"},
  {0,1,256,0,0,OA_SZ,"3sProgDescription"},
  {0,1,256,0,0,OA_SZ,"4sProgDir"},
  {0,1,0,0,0,OA_INT,":IN_idCritProgram"}
};

/* define the total number of fields to be bound */
#define NUM_FIELDS 5


/* (SQL) Statement Struct for our statement */
EWDB_OCIStatementStruct SSStatement;

/* Temporary Storage Buffer params */
#define BUFFERSIZE 81920
static int    iRecordSize;
static int    iRecordsPerBuffer;
static char * pLocalBuffer=NULL;
static int    bInitialized=FALSE;

static  int     Local_idCritProgram;

/********************************
      FUNCTION PROTOTYPES
********************************/
static int PrepGetAlarmsCritListExec (EWDBid, EWDB_Cursor *);
static int InitGetAlarmsCritListStatement(char *, EWDB_OCIStatementStruct *);
static int PostGetAlarmsCritListExec (EWDB_AlarmsCritProgramStruct *, int);
/*******************************/


int ewdb_api_GetAlarmsCriteriaList(int IN_idCritProgram,
                                   EWDB_AlarmsCritProgramStruct *pCrit, 
                                   int *pNumFound, int *pNumRetrieved, 
                                   int BufferLen)
{

  EWDB_Cursor pCursor;

  if ((pCrit == NULL) || (pNumFound == NULL) || (pNumRetrieved == NULL) ||
        (BufferLen <= 0))
  {
    logit ("", "%s: ERROR: Invalid arguments passed in.\n",
           "ewdb_api_GetAlarmsCriteriaList()");
    return EWDB_RETURN_FAILURE;
  }

  ewdb_base_SetLastOraAPIActionTime ();

  if (ewdb_base_Reconnect () != EWDB_RETURN_SUCCESS)
  {
    logit ("", "%s: ERROR Call to ewdb_base_Reconnect failed.\n",
           "ewdb_api_GetAlarmsCriteriaList()");
    return (EWDB_RETURN_FAILURE);
  }

  if (PrepGetAlarmsCritListExec (IN_idCritProgram, &pCursor) != EWDB_RETURN_SUCCESS)
  {
    logit ("", "%s: ERROR: Call to PrepGetAlarmsCritListExec failed.\n",
           "ewdb_api_GetAlarmsCriteriaList()");
    return (ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }

  if (ewdb_base_SQLExecute (pCursor))
  {
    ewdb_base_ErrorReport (hEWDBC, pCursor,"ewdb_api_GetAlarmsCriteriaList():ewdb_base_SQLExecute", 1);
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  }

	/* Commit the transaction (all the previous inserts!)
	   In Case there is any auditing or logging or debug
	   changes made in the stored procedures.
	****************************************************/
	if (ewdb_base_SQLCommit (hEWDBC))
	{
		ewdb_base_ErrorReport (hEWDBC, hEWDBC,"ewdb_api_GetAlarmsCriteriaList(): ewdb_base_SQLCommit",2);
		return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
	}
	
  if ((*pNumFound = PostGetAlarmsCritListExec (pCrit, BufferLen)) == EWDB_RETURN_FAILURE)
  {
    logit ("", "ewdb_api_GetAlarmsCriteriaList(): PostGetAlarmsCritListExec failed.\n");
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
}  /* end ewdb_api_GetAlarmsCriteriaList() */


/******************* InitGetAlarmsCritListStatement *******************/
static int InitGetAlarmsCritListStatement(char *szStatement, EWDB_OCIStatementStruct *pSS)
{

  int LastSize,i;  /* Size of last column array */


  if(!pLocalBuffer)
  {
    /*Allocate space for returned data - never freed */
    pLocalBuffer=malloc(BUFFERSIZE);

    iRecordSize=0;
    iRecordSize += sizeof(EWDBid); /*IN_idCritProgram*/
    iRecordSize += pSS->FieldArray[1].Ind; /*sProgName*/
    iRecordSize += pSS->FieldArray[2].Ind; /*sProgDescription*/
    iRecordSize += pSS->FieldArray[3].Ind; /*sProgDir*/

    /* Make the number of records a multiple of 8 to avoid alignment problems on UNIX */
    iRecordsPerBuffer= (BUFFERSIZE/iRecordSize) & 0xfffffff8;
    
    /*Allocate space for row/col ret lens. - never freed */
    for(i=0;i<pSS->NumOfFields;i++)
    {
      pSS->FieldArray[i].pRetLens=malloc(iRecordsPerBuffer*EWDB_FIELD_RET_LEN);
    }

  
    pSS->FieldArray[0].pVal = &(pLocalBuffer[0]);
    LastSize = sizeof(EWDBid); /* IN_idCritProgram */
    pSS->FieldArray[1].pVal= (void *) ((int)(pSS->FieldArray[0].pVal) + 
      (LastSize * iRecordsPerBuffer));
    LastSize = pSS->FieldArray[1].Ind;  /* sProgName */
    
    pSS->FieldArray[2].pVal= (void *) ((int)(pSS->FieldArray[1].pVal) + 
      (LastSize * iRecordsPerBuffer));
    LastSize = pSS->FieldArray[2].Ind;  /* sProgDescription */
    
    pSS->FieldArray[3].pVal= (void *) ((int)(pSS->FieldArray[2].pVal) + 
      (LastSize * iRecordsPerBuffer));
    LastSize = pSS->FieldArray[3].Ind;  /* sProgDir */
    
    /* incoming IN_idCritProgram */
    pSS->FieldArray[4].pVal= &Local_idCritProgram;


  } /* end if(!pLocalBuffer) */
  
  if(!pLocalBuffer)
  {
    logit("","InitGetAlarmsCritListStatement: malloc of pLocalBuffer "
          "failed! Returning.\n");
    return(EWDB_RETURN_FAILURE);
  }

  return(ewdb_base_RequestCursor(szStatement, pSS,0/*don't force rebind*/));
}  /* End InitGetAlarmsCritListStatement() */


/******************* PrepGetAlarmsCritListExec *******************/
static int PrepGetAlarmsCritListExec (int IN_idCritProgram, EWDB_Cursor *ppCursor)
{

  if (ppCursor == NULL)
  {
    logit ("", "Invalid arguments passed in.\n");
    return EWDB_RETURN_FAILURE;
  }

  SSStatement.NumOfFields = NUM_FIELDS;
  SSStatement.FieldArray = SQLParamsBindArray;
  SSStatement.RecordSize = 0;

  if (IN_idCritProgram < 0)
  {
    SQLParamsBindArray[4].UseField = FALSE;
    sprintf(SQL_STRING, "%s", SQL_STRING_BASE);
  }
  else
  {
    Local_idCritProgram = IN_idCritProgram;

    SQLParamsBindArray[4].UseField = TRUE;
    sprintf(SQL_STRING, "%s%s", SQL_STRING_BASE, SQL_STRING_WHERE);
  }

  if(InitGetAlarmsCritListStatement(SQL_STRING, &SSStatement) != EWDB_RETURN_SUCCESS)
  {
    *ppCursor = NULL;
    return(EWDB_RETURN_FAILURE);
  }

  *ppCursor = SSStatement.pCda;

  return (EWDB_RETURN_SUCCESS);
}  /* end PrepGetAlarmsCritListExec() */


/******************* PostGetAlarmsCritListExec *******************/
static int PostGetAlarmsCritListExec (EWDB_AlarmsCritProgramStruct *pBuffer, 
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
        ewdb_base_ErrorReport (hEWDBC, pCursor,"PostGetAlarmsCritListExec:ewdb_base_SQLFetchRows", 1);
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

      /* indx 0: IN_idCritProgram */
      pBuffer[UCurr].idCritProgram = *(int *)((sizeof (int) * BCurr) + 
                      (int)(pSS->FieldArray[0].pVal));

      /* indx 1: sProgName */
      pTemp = (char *) ((pSS->FieldArray[1].Ind * BCurr) + 
                    (int)(pSS->FieldArray[1].pVal));
      pTemp[pSS->FieldArray[1].pRetLens[BCurr]] = 0;
      strcpy (pBuffer[UCurr].sProgName, pTemp);

      /* indx 2: sProgDescription */
      pTemp = (char *) ((pSS->FieldArray[2].Ind * BCurr) + 
                    (int)(pSS->FieldArray[2].pVal));
      pTemp[pSS->FieldArray[2].pRetLens[BCurr]] = 0;
      strcpy (pBuffer[UCurr].sProgDescription, pTemp);

      /* indx 3: sProgDir */
      pTemp = (char *) ((pSS->FieldArray[3].Ind * BCurr) + 
                    (int)(pSS->FieldArray[3].pVal));
      pTemp[pSS->FieldArray[3].pRetLens[BCurr]] = 0;
      strcpy (pBuffer[UCurr].sProgDir, pTemp);

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
                       "PostGetAlarmsCritListExec:ewdb_base_SQLFetchRows",2);
      return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
    }
    /* else */
  }  /* End if(RowsRetrieved > BufferRecLen) */
  
  RowsProcessed = ewdb_base_GetCursorRowsProcessedCount(pCursor);

  ewdb_base_ReleaseCursor(pCursor);

  return(RowsProcessed);
}  /* End PostGetAlarmsCritListExec() */


