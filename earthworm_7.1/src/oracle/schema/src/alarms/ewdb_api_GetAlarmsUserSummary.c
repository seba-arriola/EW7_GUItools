/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_api_GetAlarmsUserSummary.c,v 1.2 2001/07/28 00:44:27 lucky Exp $
 *
 *    Revision history:
 *     $Log: ewdb_api_GetAlarmsUserSummary.c,v $
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
#include <ewdb_ora_api.h>
#include <ewdb_oci_base_internal.h>


static char GET_SUMM_LIST_STRING[] =
        "select idRule, sTableName, idDelivery, idUserDelivery "
        "from RuleDeliveryUser where idUser = :IN_idUser";


EWDB_OCI_SFS GetSummLstBindArray[] =

{
  {0,1,0,0,0,OA_INT,    "1idRule"},
  {0,1,256,0,0,OA_SZ,   "2sTableName"},
  {0,1,0,0,0,OA_INT,   "3idDelivery"},
  {0,1,0,0,0,OA_INT,   "4idUserDelivery"},
  {0,1,0,0,0,OA_INT,    ":IN_idUser"},
};

/* define the total number of fields to be bound */
#define GS_TOTAL 5


/* (SQL) Statement Struct for our statement */
EWDB_OCIStatementStruct SSGetSummLst;


static  int     BASD;  /* Binary - ASCII size differential*/
/* Used to account for the difference in size between the
   Stationstruct record size and the size of the FormLst struct
   information that is returned from oracle, which uses
   ascii strings to represent floats and doubles.
*/

static  int     SummLstPerBuffer;
static  int     GS_idUser;

#define		GS_BUFSIZE		245760
static	char SummLstBuffer[2*GS_BUFSIZE];



/********************************
      FUNCTION PROTOTYPES
********************************/
int PrepGetSummLstExec (int, EWDB_Cursor *);
int InitGetSummLstStatement(int, char *, EWDB_OCIStatementStruct *);
int PostGetSummLstExec (EWDB_AlarmsUserStructSummary *, int);
/*******************************/


int		ewdb_api_GetAlarmsUserSummary (int idUser,
				EWDB_AlarmsUserStructSummary *pSummary, int *NumFound, 
								int *NumRetrieved, int BufferLen)
{

	EWDB_Cursor pCursor;

	if ((pSummary == NULL) || (NumFound == NULL) || (NumRetrieved == NULL) ||
				(BufferLen <= 0))
	{
		logit ("", "Invalid arguments passed in.\n");
		return EWDB_RETURN_FAILURE;
	}

	ewdb_base_SetLastOraAPIActionTime ();

	if (ewdb_base_Reconnect () != EWDB_RETURN_SUCCESS)
	{
		logit ("", "Call to ewdb_base_Reconnect failed.\n");
		return (EWDB_RETURN_FAILURE);
	}

	if (PrepGetSummLstExec (idUser, &pCursor) != EWDB_RETURN_SUCCESS)
	{
		logit ("", "Call to PrepGetSummLstExec failed.\n");
		return (ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
	}


	if (ewdb_base_SQLExecute (pCursor))
	{
		ewdb_base_ErrorReport (hEWDBC, pCursor,"EWDB_GetSummLst:ewdb_base_SQLExecute", 1);
		return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
	}

	if ((*NumFound = PostGetSummLstExec (pSummary, BufferLen)) == EWDB_RETURN_FAILURE)
	{
		logit ("", "Call to PostGetSummLstExec failed.\n");
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


/******************* PrepGetSummLstExec *******************/
int		PrepGetSummLstExec (int idUser, EWDB_Cursor *ppCursor)
{

	if (ppCursor == NULL)
	{
		logit ("", "Invalid arguments passed in.\n");
		return EWDB_RETURN_FAILURE;
	}

	GS_idUser = idUser;

	SSGetSummLst.NumOfFields = GS_TOTAL;
	SSGetSummLst.FieldArray = GetSummLstBindArray;
	SSGetSummLst.RecordSize = 0;
	InitGetSummLstStatement (idUser, GET_SUMM_LIST_STRING, &SSGetSummLst);

	*ppCursor = SSGetSummLst.pCda;

	return (EWDB_RETURN_SUCCESS);
}


/******************* InitGetSummLstStatement *******************/
int		InitGetSummLstStatement(int idUser, char *szStatement, 	
							EWDB_OCIStatementStruct *pSS)
{

	int		LastSize, i; 

	BASD = sizeof (EWDB_AlarmsUserStructSummary);

	SummLstPerBuffer = GS_BUFSIZE/BASD/sizeof(int)*sizeof(int); 

	/*Allocate space for row/col ret lens.*/
	for (i = 0; i < pSS->NumOfFields; i++)
		pSS->FieldArray[i].pRetLens = malloc (SummLstPerBuffer * sizeof (ub2));


	/* idRule */
	pSS->FieldArray[0].pVal = &(SummLstBuffer[0]);
	LastSize = sizeof(int);

	/* sTableName */
	pSS->FieldArray[1].pVal= (void *) ((int)(pSS->FieldArray[0].pVal) + 
											(LastSize * SummLstPerBuffer));
	LastSize = pSS->FieldArray[1].Ind;

	/* idDelivery */
	pSS->FieldArray[2].pVal= (void *) ((int)(pSS->FieldArray[1].pVal) + 
											(LastSize * SummLstPerBuffer));
	LastSize = sizeof (int);

	/* idUserDelivery */
	pSS->FieldArray[3].pVal= (void *) ((int)(pSS->FieldArray[2].pVal) + 
											(LastSize * SummLstPerBuffer));
	LastSize = sizeof (int);

	/* incoming idUser */
	pSS->FieldArray[4].pVal= &GS_idUser;
	LastSize = sizeof (int);


	if (ewdb_base_RequestCursor (szStatement, pSS, 1) != 0)
	{
		logit ("", "Call to ewdb_base_RequestCursor failed.\n");
		return EWDB_RETURN_FAILURE;
	}

	return (EWDB_RETURN_SUCCESS);

}


/******************* PostGetSummLstExec *******************/
int		PostGetSummLstExec (EWDB_AlarmsUserStructSummary *pBuffer, 
												int BufferRecLen)
{

	int 					i;
	int 					done = 0;
	int 					RowsRetrieved = 0;
	int 					RowsDone = 0;
	EWDB_Cursor 			pCursor = SSGetSummLst.pCda;
	char 					*pTemp;
	int 					BCurr,UCurr;
	int 					RecordsProcessed;
	EWDB_OCIStatementStruct	*pSS=&SSGetSummLst;

	while (!done)
	{
		if (ewdb_base_SQLFetchRows (pCursor, SummLstPerBuffer))
		{
			if (ewdb_base_GetCursorRetCode(pCursor) == EWDB_SQL_ERROR_NO_DATA) 
				done=1;
			else
			{
				ewdb_base_ErrorReport (hEWDBC, pCursor,"PostGetSummLstExec:ewdb_base_SQLFetchRows", 1);
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
			BCurr = RowsDone % SummLstPerBuffer;
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

			/* indx 3: idUserDelivery */
			pBuffer[UCurr].idUserDelivery = *(int *)((sizeof (int) * BCurr) + 
											(int)(pSS->FieldArray[3].pVal));

		} /* End for RowsDone < RowsRetrieved */
	}  /* End while !done */
  

	/* 
       keep going till we get all of the rows,
	   but ignore the contents, since we've
   	   filled up the user's buffer.
	*/
	if (RowsRetrieved > BufferRecLen)
	{
		logit ("","Retrieving unhandled rows  rowsretrieved %d, BufRL %d.\n",
													RowsRetrieved,BufferRecLen);
		if (ewdb_base_GetCursorRetCode(pCursor) != EWDB_SQL_ERROR_NO_DATA)
		{
			while (!ewdb_base_SQLFetchRows (pCursor, SummLstPerBuffer))
			{
				logit ("", "Rows Processed %d, Return Code %d\n", 
								ewdb_base_GetCursorRowsProcessedCount(pCursor), 
								ewdb_base_GetCursorRetCode(pCursor));
			}
		}

		if (ewdb_base_GetCursorRetCode(pCursor) != EWDB_SQL_ERROR_NO_DATA)
		{
			ewdb_base_ErrorReport (hEWDBC, pCursor,"PostGetSummLstExec:ewdb_base_SQLFetchRows", 2);
			return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
		}
	}  /* End if (RowsRetrieved > BufferRecLen) */
  
	RecordsProcessed = ewdb_base_GetCursorRowsProcessedCount(pCursor);
	ewdb_base_ReleaseCursor (pCursor);

    for (i = 0; i < pSS->NumOfFields; i++)
    {
        if (pSS->FieldArray[i].pRetLens)
            free (pSS->FieldArray[i].pRetLens);
    }

	return (RecordsProcessed);

}  


