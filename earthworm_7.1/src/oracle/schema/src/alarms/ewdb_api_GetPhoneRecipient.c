/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_api_GetPhoneRecipient.c,v 1.2 2005/06/03 22:32:16 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_api_GetPhoneRecipient.c,v $
 *     Revision 1.2  2005/06/03 22:32:16  davidk
 *     DB API Cleanup
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
        "select sPhoneNumber "
        " from ALL_PhoneDelivery_INFO "
        " where idDelivery = :IN_idDelivery";


static EWDB_OCI_SFS SQLParamsBindArray[] =

{
  {0,1,256,0,0,OA_SZ,   "sPhoneNumber"},
  {0,1,0,0,0,OA_INT,    ":IN_idDelivery"},
};

/* define the total number of fields to be bound */
#define NUM_FIELDS       2

/* (SQL) Statement Struct for our statement */
static EWDB_OCIStatementStruct SSStatement;

/* Temporary Storage Buffer params */
#define BUFFERSIZE 8192
static int    iRecordSize;
static int    iRecordsPerBuffer;
static char * pLocalBuffer=NULL;

static  int   Local_idDelivery;

/********************************
      FUNCTION PROTOTYPES
********************************/
static int PrepGetPhoneRecExec(EWDBid IN_idDelivery, EWDB_Cursor *ppCursor);
static int InitGetPhoneRecStatement(char *, EWDB_OCIStatementStruct *);
static int PostGetPhoneRecExec (EWDB_PhoneDeliveryStruct *, int);
/*******************************/

int ewdb_api_GetPhoneRecipient(EWDBid IN_idDelivery, 
                               EWDB_PhoneDeliveryStruct *pDelivery, 
                               int *pNumFound, int *pNumRetrieved, 
                               int BufferLen)
{

  EWDB_Cursor pCursor;

  if ((pDelivery == NULL) || (pNumFound == NULL) || (pNumRetrieved == NULL) ||
        (BufferLen <= 0))
  {
    logit ("", "ewdb_api_GetPhoneRecipient(): ERROR Invalid arguments passed in.\n");
    return EWDB_RETURN_FAILURE;
  }

  ewdb_base_SetLastOraAPIActionTime ();

  if (ewdb_base_Reconnect () != EWDB_RETURN_SUCCESS)
  {
    logit ("", "ewdb_api_GetPhoneRecipient(): ERROR Call to ewdb_base_Reconnect failed.\n");
    return (EWDB_RETURN_FAILURE);
  }

  if (PrepGetPhoneRecExec (IN_idDelivery, &pCursor) != EWDB_RETURN_SUCCESS)
  {
    logit ("", "ewdb_api_GetPhoneRecipient(): PrepGetPhoneRecExec failed.\n");
    return (ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }

  if (ewdb_base_SQLExecute (pCursor))
  {
    ewdb_base_ErrorReport (hEWDBC, pCursor,"ewdb_api_GetPhoneRecipient():ewdb_base_SQLExecute", 1);
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  }

	/* Commit the transaction (all the previous inserts!)
	   In Case there is any auditing or logging or debug
	   changes made in the stored procedures.
	****************************************************/
	if (ewdb_base_SQLCommit (hEWDBC))
	{
		ewdb_base_ErrorReport (hEWDBC, hEWDBC,"ewdb_api_GetPhoneRecipient():ewdb_base_SQLCommit",2);
		return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
	}
	
  if ((*pNumFound = PostGetPhoneRecExec (pDelivery, BufferLen)) == EWDB_RETURN_FAILURE)
  {
    logit ("", "ewdb_api_GetPhoneRecipient(): PostGetPhoneRecExec failed.\n");
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
}  /* end ewdb_api_GetPhoneRecipient() */


/******************* InitGetPhoneRecStatement *******************/
static int InitGetPhoneRecStatement(char *szStatement, EWDB_OCIStatementStruct *pSS)
{

  int LastSize,i;  /* Size of last column array */

  
  if(!pLocalBuffer)
  {
    /*Allocate space for returned data - never freed */
    pLocalBuffer=malloc(BUFFERSIZE);
    
    iRecordSize=0;
    iRecordSize += pSS->FieldArray[0].Ind; /*sPhoneNumber*/
    
    /* Make the number of records a multiple of 8 to avoid alignment problems on UNIX */
    iRecordsPerBuffer= (BUFFERSIZE/iRecordSize) & 0xfffffff8;
    
    /*Allocate space for row/col ret lens. - never freed */
    for(i=0;i<pSS->NumOfFields;i++)
    {
      pSS->FieldArray[i].pRetLens=malloc(iRecordsPerBuffer*EWDB_FIELD_RET_LEN);
    }
    
    pSS->FieldArray[0].pVal = pLocalBuffer;
    LastSize = pSS->FieldArray[0].Ind; /* sPhoneNumber */

    pSS->FieldArray[2].pVal= &Local_idDelivery; /* incoming IN_idDelivery */
  } /* end if(!pLocalBuffer) */

  if(!pLocalBuffer)
  {
    logit("","InitGetPhoneRecStatement(): malloc of pLocalBuffer "
          "failed! Returning.\n");
  return EWDB_RETURN_FAILURE;
  }

  return(ewdb_base_RequestCursor(szStatement, pSS,0/*don't force rebind*/));

}  /* End InitGetPhoneRecStatement() */


/******************* PrepGetPhoneRecExec *******************/
static int PrepGetPhoneRecExec(EWDBid IN_idDelivery, EWDB_Cursor *ppCursor)
{

  if (ppCursor == NULL)
  {
    logit ("", "PrepGetPhoneRecExec(): ERROR Invalid arguments passed in.\n");
    return EWDB_RETURN_FAILURE;
  }

  SSStatement.NumOfFields = NUM_FIELDS;
  SSStatement.FieldArray = SQLParamsBindArray;
  SSStatement.RecordSize = 0;

  Local_idDelivery = IN_idDelivery;

  if(InitGetPhoneRecStatement(SQL_STRING, &SSStatement) != EWDB_RETURN_SUCCESS)
  {
    *ppCursor = NULL;
    return(EWDB_RETURN_FAILURE);
  }

  *ppCursor = SSStatement.pCda;

  return (EWDB_RETURN_SUCCESS);
}  /* end PrepGetPhoneRecExec() */


/******************* PostGetPhoneRecExec *******************/
static int PostGetPhoneRecExec(EWDB_PhoneDeliveryStruct *pBuffer, int BufferRecLen)
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
        ewdb_base_ErrorReport (hEWDBC, pCursor,"PostGetPhoneRecExec:ewdb_base_SQLFetchRows", 1);
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

      pBuffer[UCurr].idDelivery = Local_idDelivery;

      /* indx 1: sPhoneNumber */
      pTemp = (char *) ((pSS->FieldArray[0].Ind * BCurr) + 
                    (int)(pSS->FieldArray[0].pVal));
      pTemp[pSS->FieldArray[0].pRetLens[BCurr]] = 0;
      strcpy (pBuffer[UCurr].sPhoneNumber, pTemp);

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
                       "PostGetPhoneRecExec:ewdb_base_SQLFetchRows",2);
      return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
    }
    /* else */
  }  /* End if(RowsRetrieved > BufferRecLen) */
  
  RowsProcessed = ewdb_base_GetCursorRowsProcessedCount(pCursor);

  ewdb_base_ReleaseCursor(pCursor);

  return(RowsProcessed);
}  /* End PostGetPhoneRecExec() */


