/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_api_GetPagerDeliveries.c,v 1.5 2003/10/09 17:57:00 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_api_GetPagerDeliveries.c,v $
 *     Revision 1.5  2003/10/09 17:57:00  davidk
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
 *     Revision 1.1  2001/05/15 02:16:16  davidk
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
        "select idDelivery, sNumber, sPagerCompany";

static char SQL_STRING_WHERE[] =
        " where idDelivery = :IN_idDelivery";

static char SQL_REAL_TBL[] = "from PagerDelivery";
static char SQL_AUDIT_TBL[] = "from AuditPagerDelivery";





static EWDB_OCI_SFS SQLParamsBindArray[] =

{
  {0,1,0,0,0,OA_INT,    "1idDelivery"},
  {0,1,256,0,0,OA_SZ,   "2sNumber"},
  {0,1,256,0,0,OA_SZ,   "3sPagerCompany"},
  {0,1,0,0,0,OA_INT,    ":IN_idDelivery"},
};

/* define the total number of fields to be bound */
#define NUM_FIELDS 			4





/* (SQL) Statement Struct for our statement */
static EWDB_OCIStatementStruct SSStatement;

/* Temporary Storage Buffer params */
#define BUFFERSIZE 8192
static int    iRecordSize;
static int    iRecordsPerBuffer;
static char * pLocalBuffer=NULL;


static  int   Local_idDelivery;
static	char	TblName[256];

/********************************
      FUNCTION PROTOTYPES
********************************/
int	PrepGetPDelLstExec(int bIsAudit, int idDelivery, EWDB_Cursor *ppCursor);
int InitGetPDelLstStatement(char *, EWDB_OCIStatementStruct *);
int PostGetPDelLstExec (EWDB_PagerDeliveryStruct *, int);
/*******************************/


int	ewdb_api_GetPagerDeliveries(int isAudit, int idDelivery, 
                                EWDB_PagerDeliveryStruct *pDelivery, 
                                int *NumFound, int *NumRetrieved, 
                                int BufferLen)
{

	EWDB_Cursor pCursor;

	if ((pDelivery == NULL) || (NumFound == NULL) || (NumRetrieved == NULL) ||
				(BufferLen <= 0))
	{
		logit ("", "ewdb_api_GetPagerDeliveries(): ERROR Invalid arguments passed in.\n");
		return EWDB_RETURN_FAILURE;
	}

	ewdb_base_SetLastOraAPIActionTime ();

	if (ewdb_base_Reconnect () != EWDB_RETURN_SUCCESS)
	{
		logit ("", "ewdb_api_GetPagerDeliveries(): ERROR Call to ewdb_base_Reconnect failed.\n");
		return (EWDB_RETURN_FAILURE);
	}

	if (PrepGetPDelLstExec (isAudit, idDelivery, &pCursor) != EWDB_RETURN_SUCCESS)
	{
		logit ("", "Call to PrepGetPDelLstExec failed.\n");
		return (ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
	}

	if (ewdb_base_SQLExecute (pCursor))
	{
		ewdb_base_ErrorReport (hEWDBC, pCursor,"EWDB_GetPDelLst:ewdb_base_SQLExecute", 1);
		return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
	}

	if ((*NumFound = PostGetPDelLstExec (pDelivery, BufferLen)) == EWDB_RETURN_FAILURE)
	{
		logit ("", "Call to PostGetPDelLstExec failed.\n");
		return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
	}

	ewdb_base_SetLastOraAPIActionTime ();

	if (*NumFound <= BufferLen)
	{
		*NumRetrieved = *NumFound;
		return (EWDB_RETURN_SUCCESS);
	}
	else
	{
		*NumRetrieved = BufferLen;
		return (EWDB_RETURN_WARNING);
	}
}  


/******************* InitGetPDelLstStatement *******************/
int	InitGetPDelLstStatement(char *szStatement, EWDB_OCIStatementStruct *pSS)
{

  int LastSize,i;  /* Size of last column array */

  
  if(!pLocalBuffer)
  {
    /*Allocate space for returned data - never freed */
    pLocalBuffer=malloc(BUFFERSIZE);
    
    
    iRecordSize=0;
    iRecordSize += sizeof(EWDBid); /*idDelivery*/
    iRecordSize += pSS->FieldArray[1].Ind; /*sDescription*/
    iRecordSize += pSS->FieldArray[2].Ind; /*sPagerCompany*/
    
    /* Make the number of records a multiple of 8 to avoid alignment problems on UNIX */
    iRecordsPerBuffer= (BUFFERSIZE/iRecordSize) & 0xfffffff8;
    
    /*Allocate space for row/col ret lens. - never freed */
    for(i=0;i<pSS->NumOfFields;i++)
    {
      pSS->FieldArray[i].pRetLens=malloc(iRecordsPerBuffer*EWDB_FIELD_RET_LEN);
    }
    
    pSS->FieldArray[0].pVal = pLocalBuffer;
    LastSize = sizeof(int);  /* idDelivery */
    
    pSS->FieldArray[1].pVal= (void *) ((int)(pSS->FieldArray[0].pVal) + 
      (LastSize * iRecordsPerBuffer));
    LastSize = pSS->FieldArray[1].Ind; /* sDescription */
    
    pSS->FieldArray[2].pVal= (void *) ((int)(pSS->FieldArray[1].pVal) + 
      (LastSize * iRecordsPerBuffer));
    LastSize = pSS->FieldArray[2].Ind; /* sPagerCompany */
    
    pSS->FieldArray[3].pVal= &Local_idDelivery; /* incoming idDelivery */
  } /* end if(!pLocalBuffer) */
  
  if(!pLocalBuffer)
    logit("","InitGetPDelLstStatement(): malloc of pLocalBuffer "
          "failed! Returning.\n");


  ewdb_base_RequestCursor(szStatement, pSS,0/*don't force rebind*/);

  return(EWDB_RETURN_SUCCESS);
}  /* End InitGetPDelLstStatement() */


/******************* PrepGetPDelLstExec *******************/
int	PrepGetPDelLstExec(int bIsAudit, int idDelivery, EWDB_Cursor *ppCursor)
{

  if (ppCursor == NULL)
  {
    logit ("", "PrepGetPDelLstExec(): ERROR Invalid arguments passed in.\n");
    return EWDB_RETURN_FAILURE;
  }
  
  if (bIsAudit == 0)
    strcpy (TblName, SQL_REAL_TBL);
  else
    strcpy (TblName, SQL_AUDIT_TBL);
  
		SSStatement.NumOfFields = NUM_FIELDS;
		SSStatement.FieldArray = SQLParamsBindArray;
		SSStatement.RecordSize = 0;

  if (idDelivery < 0)
	{
    SQLParamsBindArray[3].UseField = FALSE;
		sprintf (SQL_STRING, "%s %s", SQL_STRING_BASE, TblName);
		InitGetPDelLstStatement(SQL_STRING, &SSStatement);
	}
	else
	{
		Local_idDelivery = idDelivery;

    SQLParamsBindArray[3].UseField = TRUE;
		sprintf(SQL_STRING, "%s %s %s", 
 					  SQL_STRING_BASE, TblName, SQL_STRING_WHERE);
		InitGetPDelLstStatement(SQL_STRING, &SSStatement);
	}

	*ppCursor = SSStatement.pCda;

	return (EWDB_RETURN_SUCCESS);
}  /* end PrepGetPDelLstExec() */


/******************* PostGetPDelLstExec *******************/
int		PostGetPDelLstExec (EWDB_PagerDeliveryStruct *pBuffer, int BufferRecLen)
{

	int 					done = 0;
	int 					RowsRetrieved = 0;
	int 					RowsDone = 0;
	EWDB_Cursor 			pCursor = SSStatement.pCda;
	char 					*pTemp;
	int 					BCurr,UCurr;
	int 					RowsProcessed;
	EWDB_OCIStatementStruct	*pSS=&SSStatement;

	while (!done)
	{
		if (ewdb_base_SQLFetchRows (pCursor, iRecordsPerBuffer))
		{
			if (ewdb_base_GetCursorRetCode(pCursor) == EWDB_SQL_ERROR_NO_DATA) 
				done=1;
			else
			{
				ewdb_base_ErrorReport (hEWDBC, pCursor,"PostGetPDelLstExec:ewdb_base_SQLFetchRows", 1);
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

			/* indx 0: idDelivery */
			pBuffer[UCurr].idDelivery = *(int *)((sizeof (int) * BCurr) + 
											(int)(pSS->FieldArray[0].pVal));

			/* indx 1: sNumber */
			pTemp = (char *) ((pSS->FieldArray[1].Ind * BCurr) + 
										(int)(pSS->FieldArray[1].pVal));
			pTemp[pSS->FieldArray[1].pRetLens[BCurr]] = 0;
			strcpy (pBuffer[UCurr].sNumber, pTemp);

			/* indx 2: sPagerCompany */
			pTemp = (char *) ((pSS->FieldArray[2].Ind * BCurr) + 
										(int)(pSS->FieldArray[2].pVal));
			pTemp[pSS->FieldArray[2].pRetLens[BCurr]] = 0;
			strcpy (pBuffer[UCurr].sPagerCompany, pTemp);

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
                       "PostGetPDelLstExec:ewdb_base_SQLFetchRows",2);
      return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
    }
    /* else */
  }  /* End if(RowsRetrieved > BufferRecLen) */
  
  RowsProcessed = ewdb_base_GetCursorRowsProcessedCount(pCursor);

  ewdb_base_ReleaseCursor(pCursor);

  return(RowsProcessed);
}  /* End PostGetPDelLstExec() */


