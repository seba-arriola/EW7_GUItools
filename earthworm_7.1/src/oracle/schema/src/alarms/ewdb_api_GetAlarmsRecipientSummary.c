/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_api_GetAlarmsRecipientSummary.c,v 1.6 2005/06/10 16:07:48 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_api_GetAlarmsRecipientSummary.c,v $
 *     Revision 1.6  2005/06/10 16:07:48  davidk
 *     DB Cleanup.  Fixed comments.
 *
 *     Revision 1.5  2005/06/03 22:32:16  davidk
 *     DB API Cleanup
 *
 *     Revision 1.4  2005/02/03 20:38:24  mark
 *     Added idRecipient
 *
 *     Revision 1.3  2003/10/09 17:56:57  davidk
 *     Cleanup of API call, to match standard list-retrieval format.
 *
 *     Revision 1.2  2001/08/07 16:51:06  lucky
 *     Pre v6.0 checkin
 *
 *     Revision 1.1  2001/07/31 20:44:40  lucky
 *     Initial revision
 *
 *     Revision 1.2  2001/07/28 00:44:27  lucky
 *      State of the code after debugging and testing prior to v6.0 release freeze.
 *
 *     Revision 1.1  2001/05/15 02:16:15  davidk
 *     Initial revision
 *
 *
 */

#include <ewdb_cli_base.h>
#include <ewdb_ew_oci_base.h>
#include <ewdb_ora_api.h>

static char SQL_STRING[] =
        "select idRule, sTableName, idDelivery, idRecipientDelivery "
        "from ALL_RuleDeliveryRecipient_INFO where idRecipient = :IN_idRecipient";


static EWDB_OCI_SFS SQLParamsBindArray[] =

{
  {0,1,0,0,0,OA_EWDBID, "1idRule"},
  {0,1,256,0,0,OA_SZ,   "2sTableName"},
  {0,1,0,0,0,OA_EWDBID, "3idDelivery"},
  {0,1,0,0,0,OA_EWDBID, "4idRecipientDelivery"},
  {0,1,0,0,0,OA_EWDBID, ":IN_idRecipient"},
};

/* define the total number of fields to be bound */
#define NUM_FIELDS 5


/* (SQL) Statement Struct for our statement */
static EWDB_OCIStatementStruct SSStatement;

/* Temporary Storage Buffer params */
#define BUFFERSIZE 8192
static int    iRecordSize;
static int    iRecordsPerBuffer;
static char * pLocalBuffer=NULL;

static  int   Local_idRecipient;

/********************************
      FUNCTION PROTOTYPES
********************************/
static int PrepGetAlarmsRecSummExec (int, EWDB_Cursor *);
static int InitGetAlarmsRecSummStatement(char *, EWDB_OCIStatementStruct *);
static int PostGetAlarmsRecSummExec (EWDB_AlarmsRecipientStructSummary *, int);
/*******************************/


int ewdb_api_GetAlarmsRecipientSummary(EWDBid IN_idRecipient,
                                       EWDB_AlarmsRecipientStructSummary *pSummary, 
                                       int *pNumFound, 
                                       int *pNumRetrieved, int BufferLen)
{

  EWDB_Cursor pCursor;

  if ((pSummary == NULL) || (pNumFound == NULL) || (pNumRetrieved == NULL) ||
        (BufferLen <= 0))
  {
    logit ("", "ewdb_api_GetAlarmsRecipientSummary(): Invalid arguments passed in.\n");
    return EWDB_RETURN_FAILURE;
  }

  ewdb_base_SetLastOraAPIActionTime ();

  if (ewdb_base_Reconnect () != EWDB_RETURN_SUCCESS)
  {
    logit ("", "ewdb_api_GetAlarmsRecipientSummary(): ewdb_base_Reconnect failed.\n");
    return (EWDB_RETURN_FAILURE);
  }

  if (PrepGetAlarmsRecSummExec (IN_idRecipient, &pCursor) != EWDB_RETURN_SUCCESS)
  {
    logit ("", "ewdb_api_GetAlarmsRecipientSummary(): PrepGetAlarmsRecSummExec failed.\n");
    return (ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }

  if (ewdb_base_SQLExecute (pCursor))
  {
    ewdb_base_ErrorReport (hEWDBC, pCursor,"ewdb_api_GetAlarmsRecipientSummary(): ewdb_base_SQLExecute", 1);
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  }

	/* Commit the transaction (all the previous inserts!)
	   In Case there is any auditing or logging or debug
	   changes made in the stored procedures.
	****************************************************/
	if (ewdb_base_SQLCommit (hEWDBC))
	{
		ewdb_base_ErrorReport (hEWDBC, hEWDBC,"ewdb_api_GetAlarmsRecipientSummary(): ewdb_base_SQLCommit",2);
		return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
	}
	
  if ((*pNumFound = PostGetAlarmsRecSummExec (pSummary, BufferLen)) == EWDB_RETURN_FAILURE)
  {
    logit ("", "ewdb_api_GetAlarmsRecipientSummary(): PostGetAlarmsRecSummExec failed.\n");
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
}  /* end ewdb_api_GetAlarmsRecipientSummary() */


/******************* InitGetAlarmsRecSummStatement *******************/
static int InitGetAlarmsRecSummStatement(char *szStatement,   
                            EWDB_OCIStatementStruct *pSS)
{

  int LastSize,i;  /* Size of last column array */

  if(!pLocalBuffer)
  {
    /*Allocate space for returned data - never freed */
    pLocalBuffer=malloc(BUFFERSIZE);

    iRecordSize=0;
    iRecordSize += sizeof(EWDBid);         /*idRule*/
    iRecordSize += pSS->FieldArray[1].Ind; /*sTableName*/
    iRecordSize += sizeof(EWDBid);         /*idDelivery*/
    iRecordSize += sizeof(EWDBid);         /*idRecipientDelivery*/

    /* Make the number of records a multiple of 8 to avoid alignment problems on UNIX */
    iRecordsPerBuffer= (BUFFERSIZE/iRecordSize) & 0xfffffff8;
    
    /*Allocate space for row/col ret lens. - never freed */
    for(i=0;i<pSS->NumOfFields;i++)
    {
      pSS->FieldArray[i].pRetLens=malloc(iRecordsPerBuffer*EWDB_FIELD_RET_LEN);
    }
  
    pSS->FieldArray[0].pVal = pLocalBuffer;
    LastSize = sizeof(int);  /* idRule */
    
    pSS->FieldArray[1].pVal= (void *) ((int)(pSS->FieldArray[0].pVal) + 
      (LastSize * iRecordsPerBuffer));
    LastSize = pSS->FieldArray[1].Ind; /* sTableName */
    
    pSS->FieldArray[2].pVal= (void *) ((int)(pSS->FieldArray[1].pVal) + 
      (LastSize * iRecordsPerBuffer));
    LastSize = sizeof (int); /* idDelivery */
    
    pSS->FieldArray[3].pVal= (void *) ((int)(pSS->FieldArray[2].pVal) + 
      (LastSize * iRecordsPerBuffer));
    LastSize = sizeof (int); /* idRecipientDelivery */
    
    /* incoming idRecipient */
    pSS->FieldArray[4].pVal= &Local_idRecipient;
    
  } /* end if(!pLocalBuffer) */
  
  if(!pLocalBuffer)
  {
    logit("","InitGetAlarmsRecSummStatement: malloc of pLocalBuffer "
          "failed! Returning.\n");
    return(EWDB_RETURN_FAILURE);
  }

  return(ewdb_base_RequestCursor(szStatement, pSS,0/*don't force rebind*/));
}  /* End InitGetAlarmsRecSummStatement() */


/******************* PrepGetAlarmsRecSummExec *******************/
static int PrepGetAlarmsRecSummExec (int IN_idRecipient, EWDB_Cursor *ppCursor)
{

  if (ppCursor == NULL)
  {
    logit ("", "PrepGetAlarmsRecSummExec(): Invalid arguments passed in.\n");
    return EWDB_RETURN_FAILURE;
  }

  Local_idRecipient = IN_idRecipient;

  SSStatement.NumOfFields = NUM_FIELDS;
  SSStatement.FieldArray = SQLParamsBindArray;
  SSStatement.RecordSize = 0;
  if(InitGetAlarmsRecSummStatement(SQL_STRING, &SSStatement) != EWDB_RETURN_SUCCESS)
  {
    *ppCursor = NULL;
    return(EWDB_RETURN_FAILURE);
  }

  *ppCursor = SSStatement.pCda;

  return (EWDB_RETURN_SUCCESS);
}


/******************* PostGetAlarmsRecSummExec *******************/
static int PostGetAlarmsRecSummExec(EWDB_AlarmsRecipientStructSummary *pBuffer, 
                                    int BufferRecLen)
{

  int           done = 0;
  int           RowsRetrieved = 0;
  int           RowsDone = 0;
  EWDB_Cursor   pCursor = SSStatement.pCda;
  char          *pTemp;
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
        ewdb_base_ErrorReport (hEWDBC, pCursor,"PostGetAlarmsRecSummExec:ewdb_base_SQLFetchRows", 1);
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

      /* indx 0: idRule */
      pBuffer[UCurr].idRule = *(int *)((sizeof (int) * BCurr) + 
                      (int)(pSS->FieldArray[0].pVal));

      /* indx 1: sTableName */
      pTemp = (char *) ((pSS->FieldArray[1].Ind * BCurr) + 
                    (int)(pSS->FieldArray[1].pVal));
      pTemp[pSS->FieldArray[1].pRetLens[BCurr]] = 0;
      strcpy (pBuffer[UCurr].sTableName, pTemp);

      /* indx 2: idDelivery */
      pBuffer[UCurr].idDelivery = *(int *)((sizeof (int) * BCurr) + 
                      (int)(pSS->FieldArray[2].pVal));

      /* indx 3: idRecipientDelivery */
      pBuffer[UCurr].idRecipientDelivery = *(int *)((sizeof (int) * BCurr) + 
                      (int)(pSS->FieldArray[3].pVal));

      pBuffer[UCurr].idRecipient = Local_idRecipient;

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
                       "PostGetAlarmsRecSummExec:ewdb_base_SQLFetchRows",2);
      return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
    }
    /* else */
  }  /* End if(RowsRetrieved > BufferRecLen) */
  
  RowsProcessed = ewdb_base_GetCursorRowsProcessedCount(pCursor);

  ewdb_base_ReleaseCursor(pCursor);

  return(RowsProcessed);
}  /* End PostGetAlarmsRecSummExec() */
