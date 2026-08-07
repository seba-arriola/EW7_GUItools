
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_api_CreateArrival.c,v 1.6 2005/06/15 19:03:12 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_api_CreateArrival.c,v $
 *     Revision 1.6  2005/06/15 19:03:12  davidk
 *     DB API Cleanup
 *
 *     Revision 1.5  2005/01/28 19:08:03  mark
 *     Replaced szExtTableName with szSource
 *
 *     Revision 1.4  2002/05/28 17:21:55  lucky
 *     *** empty log message ***
 *
 *     Revision 1.3  2001/07/12 21:06:51  davidk
 *     rewrote the function as part of the API cleanup.
 *
 *     Revision 1.2  2001/05/15 02:16:18  davidk
 *     Moved functions around between the apps, DB API, and DB API INTERNAL
 *     levels.  Renamed functions and files.  Added support for amplitude
 *     magnitude types.  Reformatted makefiles.
 *
 *     Revision 1.1  2001/02/28 17:19:22  lucky
 *     Initial revision
 */


  
/* create_arrival.c */
#include <ewdb_ora_api.h>
#include <ewdb_cli_base.h>
#include <ewdb_ew_oci_base.h>


static char SQL_STRING[] =
	"Begin Create_Arrival(OUT_idOP => :OUT_idOP,"
	"OUT_idPick => :OUT_idPick,"
	"IN_idOrigin => :IN_idOrigin,"
	"IN_idPick => :IN_idPick,"
	"IN_sCalcPhase => :IN_sCalcPhase,"
	"IN_tCalcPhase => :IN_tCalcPhase,"
	"IN_dWeight => :IN_dWeight,"
	"IN_dDist => :IN_dDist,"
	"IN_dAzm => :IN_dAzm,"
	"IN_dTakeoff => :IN_dTakeoff,"
	"IN_sObsPhase => :IN_sObsPhase,"
	"IN_tObsPhase => :IN_tObsPhase,"
	"IN_sSource => :IN_sSource,"
	"IN_xidExternal => :IN_xidExternal,"
	"IN_idChan => :IN_idChan,"
	"IN_cMotion => :IN_cMotion,"
	"IN_cOnset => :IN_cOnset,"
	"IN_tResPick => :IN_tResPick,"
	"IN_dSigma => :IN_dSigma); End;";
	
static EWDB_OCI_SFS SQLParamsBindArray[] = 
{
  {0,1,0,0,0,OA_INT,    ":OUT_idOP"},
  {0,1,0,0,0,OA_INT,    ":OUT_idPick"},
  {0,1,0,0,0,OA_INT,    ":IN_idOrigin"},
  {0,1,0,0,0,OA_INT,    ":IN_idPick"},
  {0,1,0,0,0,OA_SZ,     ":IN_sCalcPhase"},
  {0,1,0,0,0,OA_DOUBLE, ":IN_tCalcPhase"},
  {0,1,0,0,0,OA_FLOAT,  ":IN_dWeight"},
  {0,1,0,0,0,OA_FLOAT,  ":IN_dDist"},
  {0,1,0,0,0,OA_FLOAT,  ":IN_dAzm"},
  {0,1,0,0,0,OA_FLOAT,  ":IN_dTakeoff"},
  {0,1,0,0,0,OA_SZ,     ":IN_sObsPhase"},
  {0,1,0,0,0,OA_DOUBLE, ":IN_tObsPhase"},
  {0,1,0,0,0,OA_SZ,     ":IN_sSource"},
  {0,1,0,0,0,OA_SZ,     ":IN_xidExternal"},
  {0,1,0,0,0,OA_INT,    ":IN_idChan"},
  {0,1,0,0,0,OA_SZ,     ":IN_cMotion"},
  {0,1,0,0,0,OA_SZ,     ":IN_cOnset"},
  {0,1,0,0,0,OA_DOUBLE, ":IN_tResPick"},
  {0,1,0,0,0,OA_FLOAT,  ":IN_dSigma"},
};

#define	NUM_FIELDS	19

static char		Local_sztCalcPhase[15];
static char		Local_szdWeight[15];
static char		Local_szdDist[15];
static char		Local_szdAzm[15];
static char		Local_szdTOA[15];
static char		Local_sztObsPhase[15];
static char		Local_szdResidual[15];
static char		Local_szdSigma[15];
static char		Local_szcOnset[2];
static char		Local_szcMotion[2];

static	EWDB_ArrivalStruct	Local_ArrivalStruct;


/* Insertion Struct for CreateArrival szStatement */
static EWDB_OCIStatementStruct SSStatement;

static int PrepCreateArrivalExec(EWDB_ArrivalStruct *pArrival, EWDB_Cursor *ppCursor);
static int PostCreateArrivalExec(EWDB_ArrivalStruct * pArrival);
static int InitCreateArrivalStatement(char *szStatement, EWDB_OCIStatementStruct *pSS);


int ewdb_api_CreateArrival(EWDB_ArrivalStruct *pArrival)
{

	EWDB_Cursor pCursor;
  int rc;

	if (pArrival == NULL)
	{
		logit ("", "ewdb_api_CreateArrival(): Null parameters passed in!\n");
		return EWDB_RETURN_FAILURE;
	}

	ewdb_base_SetLastOraAPIActionTime();

	if (ewdb_base_Reconnect() != EWDB_RETURN_SUCCESS)
	/* Establishes connection, and performs binding!?! */
	{
		logit("", "ewdb_api_CreateArrival(): Could not reconnect to the database!\n");
		return(EWDB_RETURN_FAILURE);
	}

	if (PrepCreateArrivalExec(pArrival, &pCursor) != EWDB_RETURN_SUCCESS)
	{
		logit("", "ewdb_api_CreateArrival(): PrepCreateArrivalExec() failed.\n");
		return(ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
	}

	if (ewdb_base_SQLExecute(pCursor))
	{
		ewdb_base_ErrorReport(hEWDBC, pCursor,"ewdb_api_CreateArrival(): ewdb_base_SQLExecute", 1);
		return(ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
	} 

	/* Commit the transaction (all the previous inserts!)
	   In Case there is any auditing or logging or debug
	   changes made in the stored procedures.
	****************************************************/
	if (ewdb_base_SQLCommit(hEWDBC))
	{
		ewdb_base_ErrorReport(hEWDBC, hEWDBC,"ewdb_api_CreateArrival(): ewdb_base_SQLCommit",2);
		return(ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
	}

  rc = PostCreateArrivalExec(pArrival);
  if(rc == EWDB_RETURN_FAILURE)
  {
    logit("", "Call to PostCreateArrivalExec failed!\n");
    return(ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }
  else if(rc == EWDB_RETURN_WARNING)
  {
    return(EWDB_RETURN_FAILURE);
  }

	return(EWDB_RETURN_SUCCESS);
}  /* end ewdb_api_CreateArrival() */


static int InitCreateArrivalStatement(char *szStatement, EWDB_OCIStatementStruct *pSS)
{
	pSS->FieldArray[0].pVal = &(Local_ArrivalStruct.idOriginPick);
	pSS->FieldArray[1].pVal = &(Local_ArrivalStruct.idPick);
	pSS->FieldArray[2].pVal = &(Local_ArrivalStruct.idOrigin);
	pSS->FieldArray[3].pVal = &(Local_ArrivalStruct.idPick);
	pSS->FieldArray[4].pVal = Local_ArrivalStruct.szCalcPhase;
	pSS->FieldArray[5].pVal = Local_sztCalcPhase;
	pSS->FieldArray[6].pVal = Local_szdWeight;
	pSS->FieldArray[7].pVal = Local_szdDist;
	pSS->FieldArray[8].pVal = Local_szdAzm;
	pSS->FieldArray[9].pVal = Local_szdTOA;
	pSS->FieldArray[10].pVal = Local_ArrivalStruct.szObsPhase;
	pSS->FieldArray[11].pVal = Local_sztObsPhase;
	pSS->FieldArray[12].pVal = Local_ArrivalStruct.szExtSource;
	pSS->FieldArray[13].pVal = Local_ArrivalStruct.szExternalPickID;
	pSS->FieldArray[14].pVal = &(Local_ArrivalStruct.idChan);
	pSS->FieldArray[15].pVal = Local_szcMotion;
	pSS->FieldArray[16].pVal = Local_szcOnset;
	pSS->FieldArray[17].pVal = Local_szdResidual;
	pSS->FieldArray[18].pVal = Local_szdSigma;

  return(ewdb_base_RequestCursor(szStatement, pSS,0/*don't force rebind*/));
}  /* end InitCreateArrivalStatement() */


static int PrepCreateArrivalExec(EWDB_ArrivalStruct *pArrival, EWDB_Cursor *ppCursor)
{
	SSStatement.NumOfFields = NUM_FIELDS;
	SSStatement.FieldArray = SQLParamsBindArray;
	SSStatement.RecordSize = 0;

	memcpy(&Local_ArrivalStruct, pArrival, sizeof (EWDB_ArrivalStruct));

	sprintf(Local_sztCalcPhase, "%0.2f", pArrival->tCalcPhase);
	sprintf(Local_szdWeight,        "%0.2f", pArrival->dWeight);
	sprintf(Local_szdDist,          "%0.2f", pArrival->dDist);
	sprintf(Local_szdAzm,           "%0.2f", pArrival->dAzm);
	sprintf(Local_szdTOA,       "%0.2f", pArrival->dTakeoff);
	sprintf(Local_sztObsPhase,  "%0.2f", pArrival->tObsPhase);
	sprintf(Local_szdResidual,           "%0.2f", pArrival->tResPick);
	sprintf(Local_szdSigma,         "%0.2f", pArrival->dSigma);
	Local_szcMotion[0] = pArrival->cMotion;
	Local_szcMotion[1] = '\0';
	Local_szcOnset[0] = pArrival->cOnset;
	Local_szcOnset[1] = '\0';

	if(InitCreateArrivalStatement(SQL_STRING, &SSStatement) 
      != EWDB_RETURN_SUCCESS)
	{
		logit("", "PrepCreateArrivalExec(): InitCreateArrivalStatement failed!\n");
		return(EWDB_RETURN_FAILURE);
	}

	*ppCursor = SSStatement.pCda;
	
	return(EWDB_RETURN_SUCCESS);
}  /* end PrepCreateArrivalExec() */


static int PostCreateArrivalExec(EWDB_ArrivalStruct * pArrival)
{
  EWDB_Cursor pCursor;
  
  pArrival->idOriginPick = Local_ArrivalStruct.idOriginPick;
  pArrival->idPick       = Local_ArrivalStruct.idPick;

  pCursor = SSStatement.pCda;
  
  ewdb_base_ReleaseCursor(pCursor);
  
  if(pArrival->idOriginPick <= 0)
  {
    if(pArrival->idOriginPick == -1)
      logit("","PostCreateArrivalExec():  SQL Proc Create_Phase() returned "
                "\"Unknown error\".  See SQL debug table for details.\n");
    else if(pArrival->idOriginPick <= -100)
      logit("","PostCreateArrivalExec():  SQL Proc Create_Phase() returned "
                "\"Error in Create_Pick\"(%d).  See Create_Pick() SQL Proc "
                "for details on the return code.\n",
            pArrival->idOriginPick);
    else
      logit("","PostCreateArrivalExec():  SQL Proc Create_Phase() returned "
               "the following error(%d).  Please see that proc for details.\n",
            pArrival->idOriginPick);
    return(EWDB_RETURN_WARNING);
  }

  return(EWDB_RETURN_SUCCESS);
}  /* end PrepCreateArrivalExec() */
