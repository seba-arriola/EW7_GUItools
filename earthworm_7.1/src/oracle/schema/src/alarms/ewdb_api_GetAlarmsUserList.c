/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_api_GetAlarmsUserList.c,v 1.3 2001/07/28 00:44:27 lucky Exp $
 *
 *    Revision history:
 *     $Log: ewdb_api_GetAlarmsUserList.c,v $
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

#include <stdlib.h>
#include <stdio.h>
#include <ewdb_cli_base.h>
#include <ewdb_ora_api.h>
#include <ewdb_oci_base_internal.h>

/*
    We work in two ways: (1) get all users, and
    (2) get a specific user
*/

static char GET_USER_LIST_STRING1[] =
        "select idUser, sDescription, dPriority "
        " from AlarmsUser ";


EWDB_OCI_SFS GetUserLstBindArray1[] =

{
  {0,1,0,0,0,OA_INT,  "1idUser"},
  {0,1,256,0,0,OA_SZ, "2sDescription"},
  {0,1,0,0,0,OA_INT,  "3dPriority"},
};

/* define the total number of fields to be bound */
#define GU_TOTAL_1 		3


static char GET_USER_LIST_STRING2[] =
        "select idUser, sDescription, dPriority "
        " from AlarmsUser where idUser = :IN_idUser";


EWDB_OCI_SFS GetUserLstBindArray2[] =

{
  {0,1,0,0,0,OA_INT,   "1idUser"},
  {0,1,256,0,0,OA_SZ,  "2sDescription"},
  {0,1,0,0,0,OA_INT,   "3dPriority"},
  {0,1,0,0,0,OA_INT,   ":IN_idUser"},
};

/* define the total number of fields to be bound */
#define GU_TOTAL_2		4 


/* (SQL) Statement Struct for our statement */
EWDB_OCIStatementStruct SSGetUserLst;


static  int     BASD;  /* Binary - ASCII size differential*/
/* Used to account for the difference in size between the
   Stationstruct record size and the size of the UserLst struct
   information that is returned from oracle, which uses
   ascii strings to represent floats and doubles.
*/

static  int     UserLstPerBuffer;
static  int     GU_idUser;

#define		GU_BUFSIZE		245760
static	char UserLstBuffer[2*GU_BUFSIZE];



/********************************
      FUNCTION PROTOTYPES
********************************/
int PrepGetUserLstExec (int, EWDB_Cursor *);
int InitGetUserLstStatement (int, char *, EWDB_OCIStatementStruct *);
int PostGetUserLstExec (EWDB_AlarmsUserStruct *, int);
/*******************************/


/* Returns ALL users in the AlarmsUser table */

int		ewdb_api_GetAlarmsUserList (int idUser, EWDB_AlarmsUserStruct *pUsers, int *NumFound, 
									int *NumRetrieved, int BufferLen)
{

	EWDB_Cursor pCursor;

    if ((pUsers == NULL) || (NumFound == NULL) || (NumRetrieved == NULL) ||
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

	if (PrepGetUserLstExec (idUser, &pCursor) != EWDB_RETURN_SUCCESS)
	{
		logit ("", "Call to PrepGetUserLstExec failed.\n");
		return (ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
	}

	if (ewdb_base_SQLExecute (pCursor))
	{
		ewdb_base_ErrorReport (hEWDBC, pCursor,"EWDB_GetUserLst:ewdb_base_SQLExecute", 1);
		return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
	}

	if ((*NumFound = PostGetUserLstExec (pUsers, BufferLen)) == EWDB_RETURN_FAILURE)
	{
		logit ("", "Call to PostGetUserLstExec failed.\n");
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


/******************* PrepGetUserLstExec *******************/
int		PrepGetUserLstExec (int idUser, EWDB_Cursor *ppCursor)
{

	if (ppCursor == NULL)
	{
		logit ("", "Invalid arguments passed in.\n");
		return EWDB_RETURN_FAILURE;
	}

	if (idUser < 0)
	{
		SSGetUserLst.NumOfFields = GU_TOTAL_1;
		SSGetUserLst.FieldArray = GetUserLstBindArray1;
		SSGetUserLst.RecordSize = 0;
		InitGetUserLstStatement (idUser, GET_USER_LIST_STRING1, &SSGetUserLst);
	}
	else
	{
		GU_idUser = idUser;
		
		SSGetUserLst.NumOfFields = GU_TOTAL_2;
		SSGetUserLst.FieldArray = GetUserLstBindArray2;
		SSGetUserLst.RecordSize = 0;
		InitGetUserLstStatement (idUser, GET_USER_LIST_STRING2, &SSGetUserLst);
	}

	*ppCursor = SSGetUserLst.pCda;

	return (EWDB_RETURN_SUCCESS);
}


/******************* InitGetUserLstStatement *******************/
int		InitGetUserLstStatement(int idUser, char *szStatement, 	
							EWDB_OCIStatementStruct *pSS)
{

	int		LastSize, i; 

	BASD = sizeof (EWDB_AlarmsUserStruct);

	UserLstPerBuffer = GU_BUFSIZE/BASD/sizeof(int)*sizeof(int); 

	/*Allocate space for row/col ret lens.*/
	for (i = 0; i < pSS->NumOfFields; i++)
		pSS->FieldArray[i].pRetLens = malloc (UserLstPerBuffer * sizeof (ub2));


	/* idUser */
	pSS->FieldArray[0].pVal = &(UserLstBuffer[0]);
	LastSize = sizeof(int);

	/* sDescription */
	pSS->FieldArray[1].pVal= (void *) ((int)(pSS->FieldArray[0].pVal) + 
											(LastSize * UserLstPerBuffer));
	LastSize = pSS->FieldArray[1].Ind;

	/* dPriority */
	pSS->FieldArray[2].pVal= (void *) ((int)(pSS->FieldArray[1].pVal) + 
											(LastSize * UserLstPerBuffer));
	LastSize = sizeof(int);

	if (idUser > 0)
	{
		/* Incoming idUser */
		pSS->FieldArray[3].pVal = &GU_idUser;
		LastSize = sizeof(int);
	}
		

	ewdb_base_RequestCursor (szStatement, pSS,1);

	return (EWDB_RETURN_SUCCESS);

}


/******************* PostGetUserLstExec *******************/
int		PostGetUserLstExec (EWDB_AlarmsUserStruct *pBuffer, 
												int BufferRecLen)
{

	int 					i;
	int 					done = 0;
	int 					RowsRetrieved = 0;
	int 					RowsDone = 0;
	EWDB_Cursor 			pCursor = SSGetUserLst.pCda;
	char 					*pTemp;
	int 					BCurr,UCurr;
	int 					RecordsProcessed;
	EWDB_OCIStatementStruct	*pSS=&SSGetUserLst;

	while (!done)
	{
		if (ewdb_base_SQLFetchRows (pCursor, UserLstPerBuffer))
		{
			if (ewdb_base_GetCursorRetCode(pCursor) == EWDB_SQL_ERROR_NO_DATA) 
				done=1;
			else
			{
				ewdb_base_ErrorReport (hEWDBC, pCursor,"PostGetUserLstExec:ewdb_base_SQLFetchRows", 1);
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
			BCurr = RowsDone % UserLstPerBuffer;
			UCurr = RowsDone;

			/* indx 0: idUser */
			pBuffer[UCurr].idUser = *(int *)((sizeof (int) * BCurr) + 
											(int)(pSS->FieldArray[0].pVal));

			/* indx 1: sDescription */
			pTemp = (char *) ((pSS->FieldArray[1].Ind * BCurr) + 
										(int)(pSS->FieldArray[1].pVal));
			pTemp[pSS->FieldArray[1].pRetLens[BCurr]] = 0;
			strcpy (pBuffer[UCurr].sDescription, pTemp);

			/* indx 2: dPriority */
			pBuffer[UCurr].dPriority = *(int *)((sizeof (int) * BCurr) + 
											(int)(pSS->FieldArray[2].pVal));

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
			while (!ewdb_base_SQLFetchRows (pCursor, UserLstPerBuffer))
			{
				logit ("", "Rows Processed %d, Return Code %d\n", 
									ewdb_base_GetCursorRowsProcessedCount(pCursor), 
									ewdb_base_GetCursorRetCode(pCursor));
			}
		}

		if (ewdb_base_GetCursorRetCode(pCursor) != EWDB_SQL_ERROR_NO_DATA)
		{
			ewdb_base_ErrorReport (hEWDBC, pCursor,"PostGetUserLstExec:ewdb_base_SQLFetchRows", 2);
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


