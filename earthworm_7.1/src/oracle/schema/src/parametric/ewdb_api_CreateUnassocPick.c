
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_api_CreateUnassocPick.c,v 1.2 2005/06/15 19:03:12 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_api_CreateUnassocPick.c,v $
 *     Revision 1.2  2005/06/15 19:03:12  davidk
 *     DB API Cleanup
 *
 *     Revision 1.1  2002/06/18 16:09:56  lucky
 *     Initial revision
 *
 *
 *
 *
 */

#include <ewdb_ora_api.h>
#include <ewdb_cli_base.h>
#include <ewdb_ew_oci_base.h>

static char SQL_STRING[] =
	"Begin Create_Unassoc_Pick("
	"OUT_idPick => :OUT_idPick,"
	"IN_idChan => :IN_idChan,"
	"IN_sSource => :IN_sSource,"
	"IN_sSourcePickID => :IN_sSourcePickID,"
	"IN_sPhase => :IN_sPhase,"
	"IN_tPhase => :IN_tPhase,"
	"IN_cMotion => :IN_cMotion,"
	"IN_cOnset => :IN_cOnset,"
	"IN_dSigma => :IN_dSigma); End;";
	
static EWDB_OCI_SFS SQLParamsBindArray[] = 
{
  {0,1,0,0,0,OA_EWDBID, ":OUT_idPick"},
  {0,1,0,0,0,OA_EWDBID, ":IN_idChan"},
  {0,1,0,0,0,OA_SZ,     ":IN_sSource"},
  {0,1,0,0,0,OA_SZ,     ":IN_sSourcePickID"},
  {0,1,0,0,0,OA_SZ,     ":IN_sPhase"},
  {0,1,0,0,0,OA_DOUBLE, ":IN_tPhase"},
  {0,1,0,0,0,OA_SZ,     ":IN_cMotion"},
  {0,1,0,0,0,OA_SZ,     ":IN_cOnset"},
  {0,1,0,0,0,OA_DOUBLE, ":IN_dSigma"},
};

#define	NUM_FIELDS	9

static char		Local_sztTimeObsPhase[15];
static char		Local_szdSigma[15];
static char		Local_szcOnset[2];
static char		Local_szcMotion[2];

static	EWDB_ArrivalStruct	Local_ArrivalStruct;

/* Insertion Struct for CreatePick statement */
static EWDB_OCIStatementStruct SSStatement;

static int PrepCreatePickExec(EWDB_ArrivalStruct *pArrival, EWDB_Cursor *ppCursor);
static int PostCreatePickExec(EWDB_ArrivalStruct * pArrival);
static int InitCreatePickStatement (char *szStatement, EWDB_OCIStatementStruct *pSS);


int ewdb_api_CreateUnassocPick(EWDB_ArrivalStruct *pArrival)
{

	EWDB_Cursor pCursor;
	int rc;

	if (pArrival == NULL)
	{
		logit ("", "ewdb_api_CreateUnassocPick(): Null parameters passed in!\n");
		return EWDB_RETURN_FAILURE;
	}

	ewdb_base_SetLastOraAPIActionTime();

	if (ewdb_base_Reconnect() != EWDB_RETURN_SUCCESS)
	{
		logit("", "ewdb_api_CreateUnassocPick(): Could not reconnect to the database!\n");
		return(EWDB_RETURN_FAILURE);
	}

	if (PrepCreatePickExec(pArrival, &pCursor) != EWDB_RETURN_SUCCESS)
	{
		logit("", "ewdb_api_CreateUnassocPick(): PrepCreatePickExec() failed.\n");
		return(ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
	}

	if (ewdb_base_SQLExecute(pCursor))
	{
		ewdb_base_ErrorReport(hEWDBC, pCursor,"ewdb_api_CreateUnassocPick(): ewdb_base_SQLExecute", 1);
		return(ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
	} 

	/* Commit the transaction (all the previous inserts!)
	   In Case there is any auditing or logging or debug
	   changes made in the stored procedures.
	****************************************************/
	if (ewdb_base_SQLCommit(hEWDBC))
	{
		ewdb_base_ErrorReport(hEWDBC, hEWDBC,"ewdb_api_CreateUnassocPick(): ewdb_base_SQLCommit",2);
		return(ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
	}

  rc = PostCreatePickExec(pArrival);
  if(rc == EWDB_RETURN_FAILURE)
  {
    logit("", "ewdb_api_CreateUnassocPick(): PostCreatePickExec failed!\n");
    return(ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }
  else if(rc == EWDB_RETURN_WARNING)
  {
    return(EWDB_RETURN_FAILURE);
  }

	return(EWDB_RETURN_SUCCESS);
}  /* end ewdb_api_CreatePick() */


static int InitCreatePickStatement (char *szStatement, EWDB_OCIStatementStruct *pSS)
{
	SSStatement.FieldArray[0].pVal = &(Local_ArrivalStruct.idPick);
	SSStatement.FieldArray[1].pVal = &(Local_ArrivalStruct.idChan);
	SSStatement.FieldArray[2].pVal = Local_ArrivalStruct.szExtSource;
	SSStatement.FieldArray[3].pVal = Local_ArrivalStruct.szExternalPickID;
	SSStatement.FieldArray[4].pVal = Local_ArrivalStruct.szObsPhase;
	SSStatement.FieldArray[5].pVal = Local_sztTimeObsPhase;
	SSStatement.FieldArray[6].pVal = Local_szcMotion;
	SSStatement.FieldArray[7].pVal = Local_szcOnset;
	SSStatement.FieldArray[8].pVal = Local_szdSigma;

  return(ewdb_base_RequestCursor(szStatement, pSS,0/*don't force rebind*/));
}


static int PrepCreatePickExec(EWDB_ArrivalStruct *pArrival, EWDB_Cursor *ppCursor)
{
	SSStatement.NumOfFields = NUM_FIELDS;
	SSStatement.FieldArray = SQLParamsBindArray;
	SSStatement.RecordSize = 0;

	memcpy(&Local_ArrivalStruct, pArrival, sizeof (EWDB_ArrivalStruct));

	sprintf(Local_sztTimeObsPhase,  "%0.2f", pArrival->tObsPhase);
	sprintf(Local_szdSigma,         "%0.2f", pArrival->dSigma);
	Local_szcMotion[0] = pArrival->cMotion;
	Local_szcMotion[1] = '\0';
	Local_szcOnset[0] = pArrival->cOnset;
	Local_szcOnset[1] = '\0';

  if(InitCreatePickStatement(SQL_STRING, &SSStatement) != EWDB_RETURN_SUCCESS)
  {
    *ppCursor = NULL;
    return(EWDB_RETURN_FAILURE);
  }

  *ppCursor = SSStatement.pCda;

	return(EWDB_RETURN_SUCCESS);
}  /* end PrepCreatePickExec() */


static int PostCreatePickExec(EWDB_ArrivalStruct * pArrival)
{
  EWDB_Cursor pCursor;
  
  pArrival->idPick = Local_ArrivalStruct.idPick;

  pCursor = SSStatement.pCda;
  
  ewdb_base_ReleaseCursor(pCursor);
  
  if(pArrival->idPick <= 0)
  {
    if(pArrival->idPick == -1)
      logit("","PostCreatePickExec():  SQL Proc Create_Unassoc_Pick() returned "
                "\"Unknown error\".  See SQL debug table for details.\n");
    else
      logit("","PostCreatePickExec():  SQL Proc Create_Unassoc_Pick() returned "
               "the following error(%d).  Please see that proc for details.\n",
            pArrival->idPick);
    return(EWDB_RETURN_WARNING);
  }

  return(EWDB_RETURN_SUCCESS);
}  /* end PrepCreatePickExec() */
