/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_api_GetRecipientDeliverySummary.c,v 1.3 2005/06/21 20:21:08 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_api_GetRecipientDeliverySummary.c,v $
 *     Revision 1.3  2005/06/21 20:21:08  davidk
 *     Fixed bugs in the SQL syntax, introduced during cleanup.
 *
 *     Revision 1.2  2005/06/03 22:32:16  davidk
 *     DB API Cleanup
 *
 *     Revision 1.1  2005/02/03 20:40:28  mark
 *     Initial checkin
 *
 *     Revision 1.1  2004/12/07 23:30:08  mark
 *     Initial checkin
 *
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <ewdb_cli_base.h>
#include <ewdb_ew_oci_base.h>
#include <ewdb_ora_api.h>
#include <ewdb_oci_base_internal.h>


static char SQL_STRING[] =
        "select sTableName, idDelivery, idRecipient "
        "from ALL_RecipientDelivery_INFO where idRecipientDelivery = :IN_idRecipientDelivery";


static EWDB_OCI_SFS SQLParamsBindArray[] =

{
  {0,1,64,0,0,OA_SZ,    "1sTableName"},
  {0,1,0,0,0,OA_EWDBID, "2idDelivery"},
  {0,1,0,0,0,OA_EWDBID, "3idRecipient"},
  {0,1,0,0,0,OA_EWDBID, ":IN_idRecipientDelivery"},
};

/* define the total number of fields to be bound */
#define NUM_FIELDS 4


/* (SQL) Statement Struct for our statement */
static EWDB_OCIStatementStruct SSStatement;

/* Temporary Storage Buffer params */
#define BUFFERSIZE 8192
static int    iRecordSize;
static int    iRecordsPerBuffer;
static char * pLocalBuffer=NULL;


static  int     Local_idRD;


/********************************
      FUNCTION PROTOTYPES
********************************/
static int PrepGetRecDelSummExec (int, EWDB_Cursor *);
static int InitGetRecDelSummStatement(char *, EWDB_OCIStatementStruct *);
static int PostGetRecDelSummExec (EWDB_AlarmsRecipientStructSummary *, int);
/*******************************/


int ewdb_api_GetRecipientDeliverySummary(int IN_idRecipientDelivery,
                                         EWDB_AlarmsRecipientStructSummary *pSummary, 
                                         int *pNumFound, 
                                         int *pNumRetrieved, int BufferLen)
{

  EWDB_Cursor pCursor;

  if ((pSummary == NULL) || (pNumFound == NULL) || (pNumRetrieved == NULL) ||
        (BufferLen <= 0))
  {
    logit ("", "ewdb_api_GetRecipientDeliverySummary(): Invalid arguments passed in.\n");
    return EWDB_RETURN_FAILURE;
  }

  ewdb_base_SetLastOraAPIActionTime ();

  if (ewdb_base_Reconnect () != EWDB_RETURN_SUCCESS)
  {
    logit ("", "ewdb_api_GetRecipientDeliverySummary(): Call to ewdb_base_Reconnect failed.\n");
    return (EWDB_RETURN_FAILURE);
  }

  if (PrepGetRecDelSummExec (IN_idRecipientDelivery, &pCursor) != EWDB_RETURN_SUCCESS)
  {
    logit ("", "ewdb_api_GetRecipientDeliverySummary(): PrepGetRecSummExec failed.\n");
    return (ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }


  if (ewdb_base_SQLExecute (pCursor))
  {
    ewdb_base_ErrorReport (hEWDBC, pCursor,"ewdb_api_GetRecipientDeliverySummary()::ewdb_base_SQLExecute", 1);
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  }

	/* Commit the transaction (all the previous inserts!)
	   In Case there is any auditing or logging or debug
	   changes made in the stored procedures.
	****************************************************/
	if (ewdb_base_SQLCommit (hEWDBC))
	{
		ewdb_base_ErrorReport (hEWDBC, hEWDBC,"ewdb_api_GetRecipientDeliverySummary():ewdb_base_SQLCommit",2);
		return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
	}
	
  if ((*pNumFound = PostGetRecDelSummExec (pSummary, BufferLen)) == EWDB_RETURN_FAILURE)
  {
    logit ("", "ewdb_api_GetRecipientDeliverySummary(): PostGetRecSummExec failed.\n");
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
}  /* end ewdb_api_GetRecipientDeliverySummary() */


/******************* InitGetRecSummStatement *******************/
static int InitGetRecDelSummStatement(char *szStatement,   
                                      EWDB_OCIStatementStruct *pSS)
{

  int LastSize,i;  /* Size of last column array */

  if(!pLocalBuffer)
  {
    /*Allocate space for returned data - never freed */
    pLocalBuffer=malloc(BUFFERSIZE);

    iRecordSize=0;
    iRecordSize += pSS->FieldArray[0].Ind; /*sTableName*/
    iRecordSize += sizeof(EWDBid);         /*idDelivery*/
    iRecordSize += sizeof(EWDBid);         /*idRecipient*/

    /* Make the number of records a multiple of 8 to avoid alignment problems on UNIX */
    iRecordsPerBuffer= (BUFFERSIZE/iRecordSize) & 0xfffffff8;
    
    /*Allocate space for row/col ret lens. - never freed */
    for(i=0;i<pSS->NumOfFields;i++)
    {
      pSS->FieldArray[i].pRetLens=malloc(iRecordsPerBuffer*EWDB_FIELD_RET_LEN);
    }
  
    pSS->FieldArray[0].pVal = pLocalBuffer;
    LastSize = pSS->FieldArray[0].Ind; /* sTableName */
        
    pSS->FieldArray[1].pVal= (void *) ((int)(pSS->FieldArray[0].pVal) + 
      (LastSize * iRecordsPerBuffer));
    LastSize = sizeof (EWDBid); /* idDelivery */
        
    pSS->FieldArray[2].pVal= (void *) ((int)(pSS->FieldArray[1].pVal) + 
      (LastSize * iRecordsPerBuffer));
    LastSize = sizeof (EWDBid); /* idRecipient */

    /* incoming IN_idRecipientDelivery */
    pSS->FieldArray[3].pVal= &Local_idRD;
    
  } /* end if(!pLocalBuffer) */
  
  if(!pLocalBuffer)
  {
    logit("","InitGetRecSummStatement: malloc of pLocalBuffer "
             "failed! Returning.\n");
    return(EWDB_RETURN_FAILURE);
  }

  return(ewdb_base_RequestCursor(szStatement, pSS,0/*don't force rebind*/));
}  /* End InitGetRecSummStatement() */


/******************* PrepGetRecSummExec *******************/
static int PrepGetRecDelSummExec(int IN_idRecipientDelivery, EWDB_Cursor *ppCursor)
{

  if (ppCursor == NULL)
  {
    logit ("", "PrepGetRecDelSummExec(): Invalid arguments passed in.\n");
    return EWDB_RETURN_FAILURE;
  }

  Local_idRD = IN_idRecipientDelivery;

  SSStatement.NumOfFields = NUM_FIELDS;
  SSStatement.FieldArray = SQLParamsBindArray;
  SSStatement.RecordSize = 0;
  
  if(InitGetRecDelSummStatement(SQL_STRING, &SSStatement) != EWDB_RETURN_SUCCESS)
  {
    *ppCursor = NULL;
    return(EWDB_RETURN_FAILURE);
  }

  *ppCursor = SSStatement.pCda;

  return (EWDB_RETURN_SUCCESS);
}  /* end PrepGetRecDelSummExec() */


/******************* PostGetRecSummExec *******************/
static int PostGetRecDelSummExec(EWDB_AlarmsRecipientStructSummary *pBuffer, 
                                 int BufferRecLen)
{

  int           done = 0;
  int           RowsRetrieved = 0;
  int           RowsDone = 0;
  EWDB_Cursor   pCursor = SSStatement.pCda;
  char         *pTemp;
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
        ewdb_base_ErrorReport (hEWDBC, pCursor,"PostGetRecSummExec:ewdb_base_SQLFetchRows", 1);
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
      pBuffer[UCurr].idRule = 0;

      /* indx 1: sTableName */
      pTemp = (char *) ((pSS->FieldArray[0].Ind * BCurr) + 
                    (int)(pSS->FieldArray[0].pVal));
      pTemp[pSS->FieldArray[0].pRetLens[BCurr]] = 0;
      strcpy (pBuffer[UCurr].sTableName, pTemp);

      /* indx 2: idDelivery */
      pBuffer[UCurr].idDelivery = *(int *)((sizeof (EWDBid) * BCurr) + 
                      (int)(pSS->FieldArray[1].pVal));

      pBuffer[UCurr].idRecipient = *(int *)((sizeof (EWDBid) * BCurr) + 
                      (int)(pSS->FieldArray[2].pVal));

      pBuffer[UCurr].idRecipientDelivery = Local_idRD;

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
                       "PostGetRecSummExec:ewdb_base_SQLFetchRows",2);
      return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
    }
    /* else */
  }  /* End if(RowsRetrieved > BufferRecLen) */
  
  RowsProcessed = ewdb_base_GetCursorRowsProcessedCount(pCursor);

  ewdb_base_ReleaseCursor(pCursor);

  return(RowsProcessed);
}  /* End PostGetRecSummExec() */
