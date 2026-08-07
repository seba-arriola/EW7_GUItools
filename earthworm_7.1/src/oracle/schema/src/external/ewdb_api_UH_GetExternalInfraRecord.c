/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_api_UH_GetExternalInfraRecord.c,v 1.6 2005/06/10 16:07:27 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_api_UH_GetExternalInfraRecord.c,v $
 *     Revision 1.6  2005/06/10 16:07:27  davidk
 *     DB Cleanup.  Fixed comments.
 *
 *     Revision 1.5  2005/06/06 16:18:08  davidk
 *     Fixed bug in PostXXX() where idChanT struct member had been
 *     changed to IN_idChanT.
 *
 *     Revision 1.4  2005/06/06 16:12:37  davidk
 *     DB API Cleanup
 *
 *     Revision 1.3  2003/10/09 17:57:01  davidk
 *     Cleanup of API call, to match standard list-retrieval format.
 *
 *     Revision 1.2  2002/03/22 20:48:38  lucky
 *     Pre v6.1 checkin
 *
 *     Revision 1.1  2001/09/26 21:47:39  lucky
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


static char SQL_STRING[] =
        "select idUHInfo, "
		"idChanT, "
		"dNaturalFrequency, "
		"dDamping, "
		"dFullscale, "
		"dSensitivity, "
		"dAzm, "
		"dDip, "
		"iGain, "
		"iSensorType "
        " from UH_Info_External "
        " where idChanT = :IN_idChanT ";

static EWDB_OCI_SFS SQLParamsBindArray[] =
{
  {0,1,0,0,0,   OA_INT,     "1idUHInfo"},
  {0,1,0,0,0,   OA_INT,     "2idChanT"},
  {0,1,20,0,0, 	OA_DOUBLE,  "3dNaturalFrequency"},
  {0,1,20,0,0, 	OA_DOUBLE,  "4dDamping"},
  {0,1,20,0,0, 	OA_DOUBLE,  "5dFullscale"},
  {0,1,20,0,0, 	OA_DOUBLE,  "6dSensitivity"},
  {0,1,20,0,0, 	OA_DOUBLE,  "7dAzm"},
  {0,1,20,0,0, 	OA_DOUBLE,  "8dDip"},
  {0,1,0,0,0,   OA_INT,     "9iGain"},
  {0,1,0,0,0,   OA_INT,     "10iSensorType"},
  {0,1,0,0,0,   OA_INT,     ":IN_idChanT"},
};


/* define the total number of fields to be bound */
#define NUM_FIELDS 	11

/* (SQL) Statement Struct for our statement */
static EWDB_OCIStatementStruct SSStatement;

/* Temporary Storage Buffer params */
#define BUFFERSIZE 8192
static int    iRecordSize;
static int    iRecordsPerBuffer;
static char * pLocalBuffer=NULL;

static int    bInitialized=FALSE;

static EWDBid	Local_idChanT;


/********************************
      FUNCTION PROTOTYPES
********************************/
static int PrepGetExtInfraExec (int, EWDB_Cursor *);
static int InitGetExtInfraStatement(char *, EWDB_OCIStatementStruct *);
static int PostGetExtInfraExec (EWDB_External_UH_InfraInfo *, int);
/*******************************/


int	ewdb_api_UH_GetExternalInfraRecord(EWDBid IN_idChanT, 
                                       EWDB_External_UH_InfraInfo *pUHInfo, 
                                       int BufferLen, 
                                       int *pNumFound, int *pNumRetrieved)
{

	EWDB_Cursor pCursor;

	if ((pUHInfo == NULL) || (pNumFound == NULL) || (pNumRetrieved == NULL) ||
				(BufferLen <= 0) || (IN_idChanT < 0))
	{
		logit ("", "%s: ERROR Invalid arguments passed in.\n",
            "ewdb_api_UH_GetExternalInfraRecord()");
		return EWDB_RETURN_FAILURE;
	}

	ewdb_base_SetLastOraAPIActionTime ();

	if (ewdb_base_Reconnect () != EWDB_RETURN_SUCCESS)
	{
		logit ("", "%s: ERROR Call to ewdb_base_Reconnect failed.\n",
           "ewdb_api_UH_GetExternalInfraRecord()");
		return (EWDB_RETURN_FAILURE);
	}

	if (PrepGetExtInfraExec (IN_idChanT, &pCursor) != EWDB_RETURN_SUCCESS)
	{
		logit ("", "ewdb_api_UH_GetExternalInfraRecord(): PrepGetExtInfraExec failed.\n");
		return (ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
	}


	if (ewdb_base_SQLExecute (pCursor))
	{
		ewdb_base_ErrorReport (hEWDBC, 
				pCursor,"EWDB_GetExtInfra:ewdb_base_SQLExecute", 1);
		return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
	}
  
  /* Commit the transaction (all the previous inserts!)
  In Case there is any auditing or logging or debug
  changes made in the stored procedures.
  ****************************************************/
  if (ewdb_base_SQLCommit (hEWDBC))
  {
    ewdb_base_ErrorReport (hEWDBC, hEWDBC,"ewdb_api_UH_GetExternalInfraRecord(): ewdb_base_SQLCommit",2);
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  }
  
	if ((*pNumFound = PostGetExtInfraExec (pUHInfo, BufferLen)) 
											== EWDB_RETURN_FAILURE)
	{
		logit ("", "ewdb_api_UH_GetExternalInfraRecord(): PostGetExtInfraExec failed.\n");
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
}  /* end ewdb_api_UH_GetExternalInfraRecord() */


/******************* InitGetExtInfraStatement *******************/
static int	InitGetExtInfraStatement(char *szStatement, EWDB_OCIStatementStruct *pSS)
{

  int LastSize,i;  /* Size of last column array */


  if(!pLocalBuffer)
  {
    /*Allocate space for returned data - never freed */
    pLocalBuffer=malloc(BUFFERSIZE);

    iRecordSize=0;
    iRecordSize += sizeof(EWDBid); /*idUHInfo*/
    iRecordSize += sizeof(EWDBid); /* IN_idChanT */
    iRecordSize += pSS->FieldArray[2].Ind; /*dNaturalFrequency*/
    iRecordSize += pSS->FieldArray[3].Ind; /*dDamping*/
    iRecordSize += pSS->FieldArray[4].Ind; /*dFullscale*/
    iRecordSize += pSS->FieldArray[5].Ind; /*dSensitivity*/
    iRecordSize += pSS->FieldArray[6].Ind; /*dAzm*/
    iRecordSize += pSS->FieldArray[7].Ind; /*dDip*/
    iRecordSize += sizeof(int);            /*iGain*/
    iRecordSize += sizeof(int);            /*iSensorType*/

    /* Make the number of records a multiple of 8 to avoid alignment problems on UNIX */
    iRecordsPerBuffer= (BUFFERSIZE/iRecordSize) & 0xfffffff8;
    
    /*Allocate space for row/col ret lens. - never freed */
    for(i=0;i<pSS->NumOfFields;i++)
    {
      pSS->FieldArray[i].pRetLens=malloc(iRecordsPerBuffer*EWDB_FIELD_RET_LEN);
    }

    pSS->FieldArray[0].pVal = pLocalBuffer;
    LastSize = sizeof(EWDBid); /* idUHInfo */
    
    pSS->FieldArray[1].pVal= (void *) ((int)(pSS->FieldArray[0].pVal) + 
      (LastSize * iRecordsPerBuffer));
    LastSize = sizeof (EWDBid); /* IN_idChanT */
    
    pSS->FieldArray[2].pVal= (void *) ((int)(pSS->FieldArray[1].pVal) + 
      (LastSize * iRecordsPerBuffer));
    LastSize = pSS->FieldArray[2].Ind; /* dNaturalFrequency */
    
    pSS->FieldArray[3].pVal= (void *) ((int)(pSS->FieldArray[2].pVal) + 
      (LastSize * iRecordsPerBuffer));
    LastSize = pSS->FieldArray[3].Ind; /* dDamping*/
    
    pSS->FieldArray[4].pVal= (void *) ((int)(pSS->FieldArray[3].pVal) + 
      (LastSize * iRecordsPerBuffer));
    LastSize = pSS->FieldArray[4].Ind; /* dFullscale*/
    
    pSS->FieldArray[5].pVal= (void *) ((int)(pSS->FieldArray[4].pVal) + 
      (LastSize * iRecordsPerBuffer));
    LastSize = pSS->FieldArray[5].Ind; /* dSensitivity*/
    
    pSS->FieldArray[6].pVal= (void *) ((int)(pSS->FieldArray[5].pVal) + 
      (LastSize * iRecordsPerBuffer));
    LastSize = pSS->FieldArray[6].Ind; /* dAzm */
    
    pSS->FieldArray[7].pVal= (void *) ((int)(pSS->FieldArray[6].pVal) + 
      (LastSize * iRecordsPerBuffer));
    LastSize = pSS->FieldArray[7].Ind; /* dDip */
    
    pSS->FieldArray[8].pVal= (void *) ((int)(pSS->FieldArray[7].pVal) + 
      (LastSize * iRecordsPerBuffer));
    LastSize = sizeof (int); /* iGain */
    
    pSS->FieldArray[9].pVal= (void *) ((int)(pSS->FieldArray[8].pVal) + 
      (LastSize * iRecordsPerBuffer));
    LastSize = sizeof (int);  /* iSensorType */
    
    /* incoming IN_idChanT */
    pSS->FieldArray[10].pVal= &Local_idChanT;
    
  } /* end if(!pLocalBuffer) */
  
  if(!pLocalBuffer)
    logit("","InitGetExtInfraStatement(): malloc of pLocalBuffer "
          "failed! Returning.\n");


  return(ewdb_base_RequestCursor(szStatement, pSS,0/*don't force rebind*/));
}  /* End InitGetExtInfraStatement() */


/******************* PrepGetExtInfraExec *******************/
static int		PrepGetExtInfraExec (int IN_idChanT, EWDB_Cursor *ppCursor)
{

	if (ppCursor == NULL)
	{
		logit ("", "PrepGetExtInfraExec(): ERROR  Invalid arguments passed in.\n");
		return EWDB_RETURN_FAILURE;
	}

  if(!bInitialized)
  {
    bInitialized = TRUE;
    memset(&SSStatement,0,sizeof(SSStatement));
    SSStatement.NumOfFields=NUM_FIELDS;
    SSStatement.FieldArray=SQLParamsBindArray;
    SSStatement.RecordSize=0;
  }
	Local_idChanT = IN_idChanT;

  if(InitGetExtInfraStatement(SQL_STRING, &SSStatement) != EWDB_RETURN_SUCCESS)
  {
    *ppCursor = NULL;
    return(EWDB_RETURN_FAILURE);
  }

	*ppCursor = SSStatement.pCda;

	return (EWDB_RETURN_SUCCESS);
}


/******************* PostGetExtInfraExec *******************/
static int	PostGetExtInfraExec(EWDB_External_UH_InfraInfo *pBuffer, 
                        int BufferRecLen)
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
    memset(pLocalBuffer, 0, BUFFERSIZE);
    
    if (ewdb_base_SQLFetchRows (pCursor, iRecordsPerBuffer))
		{
			if (ewdb_base_GetCursorRetCode(pCursor) == EWDB_SQL_ERROR_NO_DATA) 
				done=1;
			else
			{
				ewdb_base_ErrorReport (hEWDBC, pCursor,
					"PostGetExtInfraExec:ewdb_base_SQLFetchRows", 1);
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

			/* field 1: idUHInfo */
			pBuffer[UCurr].idUHInfo = *(int *)((sizeof (int) * BCurr) + 
											(int)(pSS->FieldArray[0].pVal));

			/* field 2: IN_idChanT */
			pBuffer[UCurr].idChanT = *(int *)((sizeof (int) * BCurr) + 
											(int)(pSS->FieldArray[1].pVal));

			/* field 3: dNaturalFrequency */
            pTemp = (char *) ((pSS->FieldArray[2].Ind * BCurr) +
                                        (int)(pSS->FieldArray[2].pVal));
            pTemp[pSS->FieldArray[2].pRetLens[BCurr]] = 0;
            pBuffer[UCurr].dNaturalFrequency = atof (pTemp);

			/* field 4: dDamping */
            pTemp = (char *) ((pSS->FieldArray[3].Ind * BCurr) +
                                        (int)(pSS->FieldArray[3].pVal));
            pTemp[pSS->FieldArray[3].pRetLens[BCurr]] = 0;
            pBuffer[UCurr].dDamping = atof (pTemp);

			/* field 5: dFullscale */
            pTemp = (char *) ((pSS->FieldArray[4].Ind * BCurr) +
                                        (int)(pSS->FieldArray[4].pVal));
            pTemp[pSS->FieldArray[4].pRetLens[BCurr]] = 0;
            pBuffer[UCurr].dFullscale = atof (pTemp);

			/* field 6: dSensitivity */
            pTemp = (char *) ((pSS->FieldArray[5].Ind * BCurr) +
                                        (int)(pSS->FieldArray[5].pVal));
            pTemp[pSS->FieldArray[5].pRetLens[BCurr]] = 0;
            pBuffer[UCurr].dSensitivity = atof (pTemp);

			/* field 7: dAzm */
            pTemp = (char *) ((pSS->FieldArray[6].Ind * BCurr) +
                                        (int)(pSS->FieldArray[6].pVal));
            pTemp[pSS->FieldArray[6].pRetLens[BCurr]] = 0;
            pBuffer[UCurr].dAzm = atof (pTemp);

			/* field 8: dDip */
            pTemp = (char *) ((pSS->FieldArray[7].Ind * BCurr) +
                                        (int)(pSS->FieldArray[7].pVal));
            pTemp[pSS->FieldArray[7].pRetLens[BCurr]] = 0;
            pBuffer[UCurr].dDip = atof (pTemp);

			/* field 9: iGain */
			pBuffer[UCurr].iGain = *(int *)((sizeof (int) * BCurr) + 
											(int)(pSS->FieldArray[8].pVal));

			/* field 10: iSensorType */
			pBuffer[UCurr].iSensorType = *(int *)((sizeof (int) * BCurr) + 
											(int)(pSS->FieldArray[9].pVal));

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
                       "PostGetExtInfraExec:ewdb_base_SQLFetchRows",2);
      return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
    }
    /* else */
  }  /* End if(RowsRetrieved > BufferRecLen) */
  
  RowsProcessed = ewdb_base_GetCursorRowsProcessedCount(pCursor);

  ewdb_base_ReleaseCursor(pCursor);

  return(RowsProcessed);
}  /* End PostGetExtInfraExec() */

