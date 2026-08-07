/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_api_GetQddsDeliveries.c,v 1.6 2005/06/03 22:32:16 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_api_GetQddsDeliveries.c,v $
 *     Revision 1.6  2005/06/03 22:32:16  davidk
 *     DB API Cleanup
 *
 *     Revision 1.5  2003/10/09 17:57:01  davidk
 *     Cleanup of API call, to match standard list-retrieval format.
 *
 *     Revision 1.3  2001/08/07 16:51:06  lucky
 *     Pre v6.0 checkin
 *
 *     Revision 1.2  2001/07/31 20:44:40  lucky
 *     Changed from User to Recipient
 *
 *     Revision 1.1  2001/07/28 00:44:27  lucky
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
  We work in four ways: 
     (1) Get all deliveries
         (a) audit
       (b) real
     (2) get a specific delivery
         (a) audit
       (b) real

    We'll try to build the string on the fly.
*/

static char SQL_STRING[512];

static char SQL_STRING_BASE[] =
        "select IN_idDelivery, sQddsDirectory";

static char SQL_STRING_WHERE[] =
        " where idDelivery = :IN_idDelivery";

static char SQL_REAL_TBL[] = "from ALL_QddsDelivery_INFO";
static char SQL_AUDIT_TBL[] = "from ALL_AuditQddsDelivery_INFO";



static EWDB_OCI_SFS SQLParamsBindArray[] =

{
  {0,1,0,0,0,OA_INT,    "1idDelivery"},
  {0,1,256,0,0,OA_SZ,   "2sQddsDirectory"},
  {0,1,0,0,0,OA_INT,    ":IN_idDelivery"},
};

/* define the total number of fields to be bound */
#define NUM_FIELDS       3


/* (SQL) Statement Struct for our statement */
static EWDB_OCIStatementStruct SSStatement;

/* Temporary Storage Buffer params */
#define BUFFERSIZE 8192
static int    iRecordSize;
static int    iRecordsPerBuffer;
static char * pLocalBuffer=NULL;

static  int   Local_idDelivery;
static  char  TblName[256];

/********************************
      FUNCTION PROTOTYPES
********************************/
static int PrepGetQddsDeliveriesExec(int bIsAudit, int IN_idDelivery, EWDB_Cursor *ppCursor);
static int InitGetQddsDeliveriesStatement(char *, EWDB_OCIStatementStruct *);
static int PostGetQddsDeliveriesExec(EWDB_QddsDeliveryStruct *, int);
/*******************************/


int ewdb_api_GetQddsDeliveries(int bIsAudit, EWDBid IN_idDelivery, 
                               EWDB_QddsDeliveryStruct *pDelivery, 
                               int *pNumFound, int *pNumRetrieved, 
                               int BufferLen)
{

  EWDB_Cursor pCursor;

  if ((pDelivery == NULL) || (pNumFound == NULL) || (pNumRetrieved == NULL) ||
        (BufferLen <= 0))
  {
    logit ("", "ewdb_api_GetQddsDeliveries(): ERROR Invalid arguments passed in.\n");
    return EWDB_RETURN_FAILURE;
  }

  ewdb_base_SetLastOraAPIActionTime ();

  if(ewdb_base_Reconnect () != EWDB_RETURN_SUCCESS)
  {
    logit ("", "ewdb_api_GetQddsDeliveries(): ewdb_base_Reconnect failed.\n");
    return (EWDB_RETURN_FAILURE);
  }


  if(PrepGetQddsDeliveriesExec(bIsAudit, IN_idDelivery, &pCursor) != EWDB_RETURN_SUCCESS)
  {
    logit ("", "ewdb_api_GetQddsDeliveries(): PrepGetQddsDeliveriesExec failed.\n");
    return (ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }

  if(ewdb_base_SQLExecute (pCursor))
  {
    ewdb_base_ErrorReport (hEWDBC, pCursor,"ewdb_api_GetQddsDeliveries():ewdb_base_SQLExecute", 1);
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  }

	/* Commit the transaction (all the previous inserts!)
	   In Case there is any auditing or logging or debug
	   changes made in the stored procedures.
	****************************************************/
	if (ewdb_base_SQLCommit (hEWDBC))
	{
		ewdb_base_ErrorReport (hEWDBC, hEWDBC,"ewdb_api_GetQddsDeliveries()::ewdb_base_SQLCommit",2);
		return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
	}
	
  if((*pNumFound = PostGetQddsDeliveriesExec (pDelivery, BufferLen)) == EWDB_RETURN_FAILURE)
  {
    logit ("", "ewdb_api_GetQddsDeliveries(): PostGetQddsDeliveriesExec failed.\n");
    return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }

  ewdb_base_SetLastOraAPIActionTime ();

  if(*pNumFound <= BufferLen)
  {
    *pNumRetrieved = *pNumFound;
    return (EWDB_RETURN_SUCCESS);
  }
  else
  {
    *pNumRetrieved = BufferLen;
    return (EWDB_RETURN_WARNING);
  }
}  /*  end ewdb_api_GetQddsDeliveries() */


/******************* InitGetQddsDeliveriesStatement *******************/
static int InitGetQddsDeliveriesStatement(char *szStatement, EWDB_OCIStatementStruct *pSS)
{

  int LastSize,i;  /* Size of last column array */

  
  if(!pLocalBuffer)
  {
    /*Allocate space for returned data - never freed */
    pLocalBuffer=malloc(BUFFERSIZE);
    
    iRecordSize=0;
    iRecordSize += sizeof(EWDBid); /*IN_idDelivery*/
    iRecordSize += pSS->FieldArray[1].Ind; /*sQddsDirectory*/
    
    /* Make the number of records a multiple of 8 to avoid alignment problems on UNIX */
    iRecordsPerBuffer= (BUFFERSIZE/iRecordSize) & 0xfffffff8;
    
    /*Allocate space for row/col ret lens. - never freed */
    for(i=0;i<pSS->NumOfFields;i++)
    {
      pSS->FieldArray[i].pRetLens=malloc(iRecordsPerBuffer*EWDB_FIELD_RET_LEN);
    }
    
    pSS->FieldArray[0].pVal = pLocalBuffer;
    LastSize = sizeof(int);  /* IN_idDelivery */
    
    pSS->FieldArray[1].pVal= (void *) ((int)(pSS->FieldArray[0].pVal) + 
      (LastSize * iRecordsPerBuffer));
    LastSize = pSS->FieldArray[1].Ind; /* sQddsDirectory */
    
    pSS->FieldArray[2].pVal= &Local_idDelivery; /* incoming IN_idDelivery */
  } /* end if(!pLocalBuffer) */
  
  if(!pLocalBuffer)
  {
    logit("","InitGetQddsDeliveriesStatement(): malloc of pLocalBuffer failed! Returning.\n");
    return(EWDB_RETURN_FAILURE);
  }

  return(ewdb_base_RequestCursor(szStatement, pSS,0/*don't force rebind*/));
}  /* End InitGetQddsDeliveriesStatement() */


/******************* PrepGetQddsDeliveriesExec *******************/
static int PrepGetQddsDeliveriesExec(int bIsAudit, int IN_idDelivery, EWDB_Cursor *ppCursor)
{
  int rc;

  if (ppCursor == NULL)
  {
    logit ("", "PrepGetQddsDeliveriesExec(): ERROR Invalid arguments passed in.\n");
    return EWDB_RETURN_FAILURE;
  }
  
  if (bIsAudit == 0)
    strcpy (TblName, SQL_REAL_TBL);
  else
    strcpy (TblName, SQL_AUDIT_TBL);
  
    SSStatement.NumOfFields = NUM_FIELDS;
    SSStatement.FieldArray = SQLParamsBindArray;
    SSStatement.RecordSize = 0;

  if (IN_idDelivery < 0)
  {
    SQLParamsBindArray[2].UseField = FALSE;
    sprintf (SQL_STRING, "%s %s", SQL_STRING_BASE, TblName);
    rc = InitGetQddsDeliveriesStatement(SQL_STRING, &SSStatement);
  }
  else
  {
    Local_idDelivery = IN_idDelivery;

    SQLParamsBindArray[2].UseField = TRUE;
    sprintf(SQL_STRING, "%s %s %s", 
            SQL_STRING_BASE, TblName, SQL_STRING_WHERE);
    rc = InitGetQddsDeliveriesStatement(SQL_STRING, &SSStatement);
  }

  *ppCursor = SSStatement.pCda;

  return (rc);
}  /* end PrepGetQddsDeliveriesExec() */


/******************* PostGetQddsDeliveriesExec *******************/
static int PostGetQddsDeliveriesExec(EWDB_QddsDeliveryStruct *pBuffer, int BufferRecLen)
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
        ewdb_base_ErrorReport (hEWDBC, pCursor,"PostGetQddsDeliveriesExec:ewdb_base_SQLFetchRows", 1);
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

      /* indx 0: IN_idDelivery */
      pBuffer[UCurr].idDelivery = *(int *)((sizeof (int) * BCurr) + 
                      (int)(pSS->FieldArray[0].pVal));

      /* indx 1: sQddsDirectory */
      pTemp = (char *) ((pSS->FieldArray[1].Ind * BCurr) + 
                    (int)(pSS->FieldArray[1].pVal));
      pTemp[pSS->FieldArray[1].pRetLens[BCurr]] = 0;
      strcpy (pBuffer[UCurr].sQddsDirectory, pTemp);

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
                       "PostGetQddsDeliveriesExec:ewdb_base_SQLFetchRows",2);
      return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
    }
    /* else */
  }  /* End if(RowsRetrieved > BufferRecLen) */
  
  RowsProcessed = ewdb_base_GetCursorRowsProcessedCount(pCursor);

  ewdb_base_ReleaseCursor(pCursor);

  return(RowsProcessed);
}  /* End PostGetQddsDeliveriesExec() */

