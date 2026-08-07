/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_api_GetAlarmsGroupList.c,v 1.2 2005/06/03 22:32:16 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_api_GetAlarmsGroupList.c,v $
 *     Revision 1.2  2005/06/03 22:32:16  davidk
 *     DB API Cleanup
 *
 *     Revision 1.1  2005/02/03 20:41:57  mark
 *     Initial checkin
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <ewdb_cli_base.h>
#include <ewdb_ew_oci_base.h>
#include <ewdb_ora_api.h>
#include <ewdb_oci_base_internal.h>

/*
    We work in two ways: (1) get all groups, and
    (2) get a specific group
*/

static char SQL_STRING[256];

static char SQL_STRING_BASE[] =
  "select idGroup, sGroupName, bActive from ALL_AlarmGroup_INFO";

static char SQL_STRING_WHERE[] =
        " where idGroup = :IN_idGroup";


static EWDB_OCI_SFS SQLParamsBindArray[] =

{
  {0,1,0,0,0,OA_INT,   "1idGroup"},
  {0,1,256,0,0,OA_SZ,  "2sGroupName"},
  {0,1,0,0,0,OA_INT,   "3bActive"},
  {0,1,0,0,0,OA_INT,   ":IN_idGroup"},
};

/* define the total number of fields to be bound */
#define NUM_FIELDS    4 


/* (SQL) Statement Struct for our statement */
static EWDB_OCIStatementStruct SSStatement;

/* Temporary Storage Buffer params */
#define BUFFERSIZE 8192
static int    iRecordSize;
static int    iRecordsPerBuffer;
static char * pLocalBuffer=NULL;

static  int     Local_idGroup;

/********************************
      FUNCTION PROTOTYPES
********************************/
static int InitGetAlarmsGroupListStatement(char *, EWDB_OCIStatementStruct *);
static int PrepGetAlarmsGroupListExec(EWDBid IN_idGroup, EWDB_Cursor *ppCursor);
static int PostGetAlarmsGroupListExec(EWDB_AlarmsGroupStruct *, int);
/*******************************/


/* Returns ALL users in the AlarmsGroup table */

int ewdb_api_GetAlarmsGroupList(EWDBid IN_idGroup, 
                                EWDB_AlarmsGroupStruct *pGroups, 
                                int *pNumFound, int *pNumRetrieved, 
                                int BufferLen)
{
  EWDB_Cursor pCursor;

  if ((pGroups == NULL) || (pNumFound == NULL) || (pNumRetrieved == NULL) ||
      (BufferLen <= 0))
  {
    logit ("", "ewdb_api_GetAlarmsGroupList(): Invalid arguments passed in.\n");
    return EWDB_RETURN_FAILURE;
  }
  
  ewdb_base_SetLastOraAPIActionTime ();

  if (ewdb_base_Reconnect () != EWDB_RETURN_SUCCESS)
  {
    logit ("", "ewdb_api_GetAlarmsGroupList(): ewdb_base_Reconnect failed.\n");
    return (EWDB_RETURN_FAILURE);
  }

  if (PrepGetAlarmsGroupListExec (IN_idGroup, &pCursor) != EWDB_RETURN_SUCCESS)
  {
    logit ("", "ewdb_api_GetAlarmsGroupList(): PrepGetAlarmsGroupListExec failed.\n");
    return (ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }

  if (ewdb_base_SQLExecute (pCursor))
  {
    ewdb_base_ErrorReport (hEWDBC, pCursor,"EWDB_GetAlarmsGroupList:ewdb_base_SQLExecute", 1);
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  }

	/* Commit the transaction (all the previous inserts!)
	   In Case there is any auditing or logging or debug
	   changes made in the stored procedures.
	****************************************************/
	if (ewdb_base_SQLCommit (hEWDBC))
	{
		ewdb_base_ErrorReport (hEWDBC, hEWDBC,"ewdb_api_GetAlarmsGroupList(): ewdb_base_SQLCommit",2);
		return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
	}

  if ((*pNumFound = PostGetAlarmsGroupListExec (pGroups, BufferLen)) == EWDB_RETURN_FAILURE)
  {
    logit ("", "ewdb_api_GetAlarmsGroupList(): PostGetAlarmsGroupListExec failed.\n");
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
}  /* end ewdb_api_GetAlarmsGroupList() */


/******************* InitGetAlarmsGroupListStatement *******************/
static int InitGetAlarmsGroupListStatement(char *szStatement, 
                                           EWDB_OCIStatementStruct *pSS)
{
  int LastSize,i;  /* Size of last column array */

  if(!pLocalBuffer)
  {
    /*Allocate space for returned data - never freed */
    pLocalBuffer=malloc(BUFFERSIZE);

    iRecordSize=0;
    iRecordSize += sizeof(EWDBid); /*IN_idGroup*/
    iRecordSize += pSS->FieldArray[1].Ind; /*sName*/
    iRecordSize += sizeof(int); /*bActive*/

    /* Make the number of records a multiple of 8 to avoid alignment problems on UNIX */
    iRecordsPerBuffer= (BUFFERSIZE/iRecordSize) & 0xfffffff8;
    
    /*Allocate space for row/col ret lens. - never freed */
    for(i=0;i<pSS->NumOfFields;i++)
    {
      pSS->FieldArray[i].pRetLens=malloc(iRecordsPerBuffer*EWDB_FIELD_RET_LEN);
    }

    pSS->FieldArray[0].pVal = &(pLocalBuffer[0]);
    LastSize = sizeof(int);  /* IN_idGroup */
    
    pSS->FieldArray[1].pVal= (void *) ((int)(pSS->FieldArray[0].pVal) + 
      (LastSize * iRecordsPerBuffer));
    LastSize = pSS->FieldArray[1].Ind; /* sName */
    
    pSS->FieldArray[2].pVal= (void *) ((int)(pSS->FieldArray[1].pVal) + 
      (LastSize * iRecordsPerBuffer));
    LastSize = sizeof(int); /* bActive */
    
    pSS->FieldArray[3].pVal = &Local_idGroup;
    
  } /* end if(!pLocalBuffer) */
  
  if(!pLocalBuffer)
  {
    logit("","InitGetAlarmsGroupListStatement: malloc of pLocalBuffer "
          "failed! Returning.\n");
    return(EWDB_RETURN_FAILURE);
  }

  return(ewdb_base_RequestCursor(szStatement, pSS,0/*don't force rebind*/));
}  /* End InitGetAlarmsGroupListStatement() */


/******************* PrepGetAlarmsGroupListExec *******************/
static int PrepGetAlarmsGroupListExec (EWDBid IN_idGroup, EWDB_Cursor *ppCursor)
{

  if (ppCursor == NULL)
  {
    logit ("", "Invalid arguments passed in.\n");
    return EWDB_RETURN_FAILURE;
  }

  SSStatement.NumOfFields = NUM_FIELDS;
  SSStatement.FieldArray = SQLParamsBindArray;
  SSStatement.RecordSize = 0;

  if (IN_idGroup < 0)
  {
    SQLParamsBindArray[3].UseField = FALSE;
    sprintf(SQL_STRING, "%s", SQL_STRING_BASE);
  }
  else
  {
    Local_idGroup = IN_idGroup;

    SQLParamsBindArray[3].UseField = TRUE;
    sprintf(SQL_STRING, "%s%s", SQL_STRING_BASE, SQL_STRING_WHERE);
  }

  if(InitGetAlarmsGroupListStatement(SQL_STRING, &SSStatement) != EWDB_RETURN_SUCCESS)
  {
    *ppCursor = NULL;
    return(EWDB_RETURN_FAILURE);
  }

  *ppCursor = SSStatement.pCda;

  return (EWDB_RETURN_SUCCESS);
}


/******************* PostGetAlarmsGroupListExec *******************/
static int PostGetAlarmsGroupListExec(EWDB_AlarmsGroupStruct *pBuffer, 
                                      int BufferRecLen)
{
  int           done = 0;
  int           RowsRetrieved = 0;
  int           RowsDone = 0;
  EWDB_Cursor   pCursor = SSStatement.pCda;
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
        ewdb_base_ErrorReport (hEWDBC, pCursor,"PostGetAlarmsGroupListExec:ewdb_base_SQLFetchRows", 1);
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


      /* Copy from DB rows into the user buffer */
      BCurr = RowsDone % iRecordsPerBuffer;
      UCurr = RowsDone;

      /* indx 0: idGroup */
      pBuffer[UCurr].idGroup = *(int *)((sizeof (int) * BCurr) + 
                      (int)(pSS->FieldArray[0].pVal));

      /* indx 1: sDescription */
      pTemp = (char *) ((pSS->FieldArray[1].Ind * BCurr) + 
                    (int)(pSS->FieldArray[1].pVal));
      pTemp[pSS->FieldArray[1].pRetLens[BCurr]] = 0;
      strcpy (pBuffer[UCurr].sName, pTemp);

      /* indx 2: dPriority */
      pBuffer[UCurr].bActive = *(int *)((sizeof (int) * BCurr) + 
                      (int)(pSS->FieldArray[2].pVal));

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
                       "PostGetAlarmsGroupListExec:ewdb_base_SQLFetchRows",2);
      return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
    }
    /* else */
  }  /* End if(RowsRetrieved > BufferRecLen) */
  
  RowsProcessed = ewdb_base_GetCursorRowsProcessedCount(pCursor);

  ewdb_base_ReleaseCursor(pCursor);

  return(RowsProcessed);
}  /* End PostGetAlarmsGroupListExec() */

