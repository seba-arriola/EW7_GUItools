
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_api_GetWaveformDesc.c,v 1.4 2002/05/16 17:04:44 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_api_GetWaveformDesc.c,v $
 *     Revision 1.4  2002/05/16 17:04:44  davidk
 *     Performed some minor cleanup.
 *     Copied in the idWaveform key to use for the query (VERY IMPORTANT).
 *
 *     Revision 1.3  2001/07/23 17:24:46  davidk
 *     API cleanup.
 *
 *     Revision 1.2  2001/05/15 02:16:43  davidk
 *     Moved functions around between the apps, DB API, and DB API INTERNAL
 *     levels.  Renamed functions and files.  Added support for amplitude
 *     magnitude types.  Reformatted makefiles.
 *
 *     Revision 1.1  2001/02/28 17:19:22  lucky
 *     Initial revision
 *
 *
 */


#include <ewdb_ora_api.h>
#include <ewdb_cli_base.h>
#include <ewdb_ew_oci_base.h>


static char SQL_STRING[] =
  "Begin "
  "Get_WaveformDesc(OUT_RetCode => :OUT_RetCode,"
  " IN_idWaveform => :IN_idWaveform, OUT_idChan => :OUT_idChan,"
  " OUT_tStart => :OUT_tStart,  OUT_tEnd => :OUT_tEnd,"
  " OUT_iDataFormat => :OUT_iDataFormat,  OUT_iByteLen => :OUT_iByteLen); "
  "End;";

static EWDB_OCI_SFS GetWaveformDescBindArray[] = 
{
  {0,1,0,0,0,OA_INT,":OUT_RetCode"},
  {0,1,0,0,0,OA_INT,":IN_idWaveform"},
  {0,1,0,0,0,OA_INT,":OUT_idChan"},
  {0,1,20,0,0,OA_DOUBLE,":OUT_tStart"},
  {0,1,20,0,0,OA_DOUBLE,":OUT_tEnd"},
  {0,1,0,0,0,OA_INT,":OUT_iDataFormat"},
  {0,1,0,0,0,OA_INT,":OUT_iByteLen"},
};

#define	NUM_FIELDS	7

/* Insertion Struct for GetWaveformDesc statement */
static EWDB_OCIStatementStruct SSStatement;

static	int		  iRetCode;
static  char szStart[20], szEnd[20];
static  EWDB_WaveformStruct WaveformDesc;
/********************************
      FUNCTION PROTOTYPES
********************************/
int PrepGetWaveformDescExec(EWDB_WaveformStruct * pWaveform, EWDB_Cursor *ppCursor);
int PostGetWaveformDescExec(EWDB_WaveformStruct * pWaveform, int * pRetCode);
int InitGetWaveformDescStatement(char *statement, EWDB_OCIStatementStruct *pSS);



/**********************************************************************
**********************************************************************/
int ewdb_api_GetWaveformDesc(EWDB_WaveformStruct * pWaveformDesc)
{

  int RetCode;

	EWDB_Cursor pCursor;

	if(!pWaveformDesc)
	{
		logit("", "EWDB_GetWaveformDesc(): Invalid parameters passed in!\n");
		return EWDB_RETURN_FAILURE;
	}

	ewdb_base_SetLastOraAPIActionTime();

	if(ewdb_base_Reconnect() != EWDB_RETURN_SUCCESS)
	{
		logit("", "EWDB_GetWaveformDesc(): Could not reconnect to the database!\n");
		return(EWDB_RETURN_FAILURE);
	}

	if(PrepGetWaveformDescExec(pWaveformDesc, &pCursor) != EWDB_RETURN_SUCCESS)
	{
		logit("", "ORA_API:EWDB_GetWaveformDesc():PrepGetWaveformDescExec() failed.\n");
		return(ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
	}

	if(ewdb_base_SQLExecute(pCursor))
	{
		ewdb_base_ErrorReport(hEWDBC, pCursor,"EWDB_GetWaveformDesc:ewdb_base_SQLExecute", 1);
		return(ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
	} 
  
	/* Commit the transaction(all the previous inserts!)
	   In Case there is any auditing or logging or debug
	   changes made in the stored procedures.
	****************************************************/
	if(ewdb_base_SQLCommit(hEWDBC))
	{
		ewdb_base_ErrorReport(hEWDBC, hEWDBC,"EWDB_GetWaveformDesc:ewdb_base_SQLCommit",2);
		return(ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
	}


  if(PostGetWaveformDescExec(pWaveformDesc, &RetCode) != EWDB_RETURN_SUCCESS)
  {
    logit("", "Call to PostGetWaveformDescExec failed!\n");
    return(ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }

	if(RetCode != 0)
	{
		logit("", "SQL call to GetWaveformDesc(%d) returned error: %d\n", 
          pWaveformDesc->idWaveform, RetCode);
    return(EWDB_RETURN_FAILURE);
	}

	return(EWDB_RETURN_SUCCESS);
}  /* end ewdb_api_GetWaveformDesc() */  


int InitGetWaveformDescStatement(char *statement, EWDB_OCIStatementStruct *pSS)
{

	if((statement == NULL) ||(pSS == NULL))
	{
		logit("", "Invalid parameters passed in!\n");
		return EWDB_RETURN_FAILURE;
	}

	pSS->FieldArray[0].pVal = &iRetCode;
	pSS->FieldArray[1].pVal = &(WaveformDesc.idWaveform);
	pSS->FieldArray[2].pVal = &(WaveformDesc.idChan);
	pSS->FieldArray[3].pVal = szStart;
	pSS->FieldArray[4].pVal = szEnd;
	pSS->FieldArray[5].pVal = &(WaveformDesc.iDataFormat);
	pSS->FieldArray[6].pVal = &(WaveformDesc.iByteLen);

  if(ewdb_base_RequestCursor(statement, pSS, 0) != 0)
  {
    logit("", "Call to ewdb_base_RequestCursor failed.\n");
    return EWDB_RETURN_FAILURE;
  }
	return EWDB_RETURN_SUCCESS;
}  /* InitGetWaveformDescStatement() */


int PrepGetWaveformDescExec(EWDB_WaveformStruct * pWaveform, EWDB_Cursor *ppCursor)
{

	if(pWaveform == NULL || ppCursor == NULL)
	{
		logit("", "PrepGetWaveformDescExec(): Invalid parameters passed in!\n");
		return EWDB_RETURN_FAILURE;
	}


	SSStatement.NumOfFields = NUM_FIELDS;
	SSStatement.FieldArray = GetWaveformDescBindArray;
	SSStatement.RecordSize = 0;

	iRetCode = 0;

  WaveformDesc.idWaveform = pWaveform->idWaveform;

	if(InitGetWaveformDescStatement(SQL_STRING, &SSStatement) 
      != EWDB_RETURN_SUCCESS)
	{
		logit("", "Call to InitGetWaveformDescStatement failed!\n");
		return EWDB_RETURN_FAILURE;
	}

	*ppCursor = SSStatement.pCda;
	
	return EWDB_RETURN_SUCCESS;
}  /* PrepGetWaveformDescExec() */


int PostGetWaveformDescExec(EWDB_WaveformStruct * pWaveform, int * pRetCode)
{
  EWDB_Cursor pCursor;
  
  pCursor = SSStatement.pCda;

  /* convert the string number forms back to doubles */
  WaveformDesc.tStart = atof(szStart);
  WaveformDesc.tEnd = atof(szEnd);

  /* copy the contents of the internal struct out to the caller's struct */
  memcpy(pWaveform, &WaveformDesc, sizeof(EWDB_WaveformStruct));

  /* copy the return code */
  *pRetCode=iRetCode;
  
  ewdb_base_ReleaseCursor(pCursor);
  
  return(EWDB_RETURN_SUCCESS);
}  /* end PostGetWaveformDescExec() */
