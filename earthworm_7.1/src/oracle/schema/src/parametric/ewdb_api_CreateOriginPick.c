
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_api_CreateOriginPick.c,v 1.4 2005/06/15 19:03:12 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_api_CreateOriginPick.c,v $
 *     Revision 1.4  2005/06/15 19:03:12  davidk
 *     DB API Cleanup
 *
 *     Revision 1.3  2003/09/10 16:30:54  lucky
 *     Added IN_idPick so that we now support two modes: one with and the other
 *     without a valid idPick being passed in.
 *
 *     Revision 1.2  2002/06/18 16:09:34  lucky
 *     Cleaned up code
 *
 *     Revision 1.1  2002/05/28 17:22:28  lucky
 *     Initial revision
 *
 *
 *
 */


/*
 *  STORY:  This function creates an OriginPick record which links an
 *   Origin with a Pick.  It works in two modes
 *
 *     - if DB idPick is known, it simply creates an OP record and returns 
 *         an idOriginPick
 *     - if DB idPick is not known (such as when pick comes from an earthworm 
 *         message, we first try to get the idPick from the pick's source and
 *         source id.
 *     
 *     The two modes are distinguished by the incomming idPick.  If it is      
 *     positive, we assume that it is a good DB idPick and will create     
 *     an OriginPick record directly.  Otherwise, if idPick is negative, we 
 *     get the good idPick from its source and source ID.
 *    
 *           Lucky Vidmar, September 2003    
 */

#include <ewdb_ora_api.h>
#include <ewdb_cli_base.h>
#include <ewdb_ew_oci_base.h>

static char SQL_STRING[] =
	"Begin Create_OriginPick("
	"OUT_idOP => :OUT_idOP,"
	"OUT_idPick => :OUT_idPick,"
	"IN_idOrigin => :IN_idOrigin,"
	"IN_sSource => :IN_sSource,"
	"IN_sSourcePickID => :IN_sSourcePickID,"
	"IN_sPhase => :IN_sPhase,"
	"IN_tPhase => :IN_tPhase,"
	"IN_dWeight => :IN_dWeight,"
	"IN_dDist => :IN_dDist,"
	"IN_dAzm => :IN_dAzm,"
	"IN_dTakeoff => :IN_dTakeoff,"
	"IN_tResPick => :IN_tResPick,"
	"IN_idPick => :IN_idPick); End;";
	
static EWDB_OCI_SFS SQLParamsBindArray[] = 
{
  {0,1,0,0,0,OA_EWDBID, ":OUT_idOP"},
  {0,1,0,0,0,OA_EWDBID, ":OUT_idPick"},
  {0,1,0,0,0,OA_EWDBID, ":IN_idOrigin"},
  {0,1,0,0,0,OA_SZ,     ":IN_sSource"},
  {0,1,0,0,0,OA_SZ,     ":IN_sSourcePickID"},
  {0,1,0,0,0,OA_SZ,     ":IN_sPhase"},
  {0,1,0,0,0,OA_DOUBLE, ":IN_tPhase"},
  {0,1,0,0,0,OA_DOUBLE, ":IN_dWeight"},
  {0,1,0,0,0,OA_DOUBLE, ":IN_dDist"},
  {0,1,0,0,0,OA_DOUBLE, ":IN_dAzm"},
  {0,1,0,0,0,OA_DOUBLE, ":IN_dTakeoff"},
  {0,1,0,0,0,OA_DOUBLE, ":IN_tResPick"},
  {0,1,0,0,0,OA_EWDBID, ":IN_idPick"},
};

#define	NUM_FIELDS	13

static char		Local_sztCalcPhase[15];
static char		Local_szdWeight[15];
static char		Local_szdDist[15];
static char		Local_szdAzm[15];
static char		Local_szdTOA[15];
static char		Local_sztResPick[15];

static	EWDB_ArrivalStruct	Local_ArrivalStruct;
static	EWDBid				      Local_idOrigin;

/* Insertion Struct for CreatePick szStatement */
static EWDB_OCIStatementStruct SSStatement;

static int PrepCreateOriginPickExec(EWDB_ArrivalStruct *pArrival, EWDB_Cursor *ppCursor);
static int PostCreateOriginPickExec(EWDB_ArrivalStruct * pArrival);
static int InitCreateOriginPickStatement(char *szStatement, EWDB_OCIStatementStruct *pSS);


int ewdb_api_CreateOriginPick(EWDB_ArrivalStruct *pArrival, EWDBid IN_idOrigin)
{

	EWDB_Cursor pCursor;
	int rc;

	if (pArrival == NULL  || IN_idOrigin <= 0)
	{
		logit ("", "ewdb_api_CreateOriginPick(): Invalid parameters passed in!\n");
		return EWDB_RETURN_FAILURE;
	}

	Local_idOrigin = IN_idOrigin;

	ewdb_base_SetLastOraAPIActionTime();

	if (ewdb_base_Reconnect() != EWDB_RETURN_SUCCESS)
	{
		logit("", "ewdb_api_CreateOriginPick(): Could not reconnect to the database!\n");
		return(EWDB_RETURN_FAILURE);
	}

	if (PrepCreateOriginPickExec(pArrival, &pCursor) != EWDB_RETURN_SUCCESS)
	{
		logit("", "ewdb_api_CreateOriginPick(): PrepCreateOriginPickExec() failed.\n");
		return(ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
	}

	if (ewdb_base_SQLExecute(pCursor))
	{
		ewdb_base_ErrorReport(hEWDBC, pCursor,"ewdb_api_CreateOriginPick(): ewdb_base_SQLExecute", 1);
		return(ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
	} 

	/* Commit the transaction (all the previous inserts!)
	   In Case there is any auditing or logging or debug
	   changes made in the stored procedures.
	****************************************************/
	if (ewdb_base_SQLCommit(hEWDBC))
	{
		ewdb_base_ErrorReport(hEWDBC, hEWDBC,"ewdb_api_CreateOriginPick(): ewdb_base_SQLCommit",2);
		return(ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
	}

  rc = PostCreateOriginPickExec(pArrival);
  if(rc == EWDB_RETURN_FAILURE)
  {
    logit("", "ewdb_api_CreateOriginPick(): PostCreateOriginPickExec failed!\n");
    return(ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }
  else if(rc == EWDB_RETURN_WARNING)
  {
    return(EWDB_RETURN_FAILURE);
  }

	return(EWDB_RETURN_SUCCESS);
}  /* end ewdb_api_CreateOriginPick() */


static int InitCreateOriginPickStatement(char *szStatement, EWDB_OCIStatementStruct *pSS)
{

  static int bInitialized = FALSE;

  if(!bInitialized)
  {
    bInitialized = TRUE;
	SSStatement.FieldArray[0].pVal = &(Local_ArrivalStruct.idOriginPick);
	SSStatement.FieldArray[1].pVal = &(Local_ArrivalStruct.idPick);
	SSStatement.FieldArray[2].pVal = &(Local_ArrivalStruct.idOrigin);
	SSStatement.FieldArray[3].pVal = Local_ArrivalStruct.szExtSource;
	SSStatement.FieldArray[4].pVal = Local_ArrivalStruct.szExternalPickID;
	SSStatement.FieldArray[5].pVal = Local_ArrivalStruct.szCalcPhase;
	SSStatement.FieldArray[6].pVal = Local_sztCalcPhase;
	SSStatement.FieldArray[7].pVal = Local_szdWeight;
	SSStatement.FieldArray[8].pVal = Local_szdDist;
	SSStatement.FieldArray[9].pVal = Local_szdAzm;
	SSStatement.FieldArray[10].pVal = Local_szdTOA;
	SSStatement.FieldArray[11].pVal = Local_sztResPick;
	SSStatement.FieldArray[12].pVal = &(Local_ArrivalStruct.idPick);
  }

  return(ewdb_base_RequestCursor(szStatement, pSS,0/*don't force rebind*/));
}  /* InitCreatePeakAmpStatement() */


static int PrepCreateOriginPickExec(EWDB_ArrivalStruct *pArrival, EWDB_Cursor *ppCursor)
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
	sprintf(Local_sztResPick,   "%0.2f", pArrival->tResPick);

  if(InitCreateOriginPickStatement(SQL_STRING, &SSStatement) != EWDB_RETURN_SUCCESS)
  {
    *ppCursor = NULL;
    return(EWDB_RETURN_FAILURE);
  }

	*ppCursor = SSStatement.pCda;

	return(EWDB_RETURN_SUCCESS);
}  /* end PrepCreateOriginPickExec() */


static int PostCreateOriginPickExec(EWDB_ArrivalStruct * pArrival)
{
  EWDB_Cursor pCursor;
  
  pArrival->idOriginPick = Local_ArrivalStruct.idOriginPick;
  pArrival->idPick       = Local_ArrivalStruct.idPick;

  pCursor = SSStatement.pCda;
  
  ewdb_base_ReleaseCursor(pCursor);
  
  if ((pArrival->idOriginPick <= 0) || (pArrival->idPick <= 0))
  {
      logit("","PostCreateOriginPickExec():  SQL Proc Create_OriginPick() returned "
               "bad value; idPick %d, idOP %d.  Please see that proc for details.\n", 
				pArrival->idPick, pArrival->idOriginPick);
    return(EWDB_RETURN_WARNING);
  }

  return(EWDB_RETURN_SUCCESS);
}  /* end PrepCreateOriginPickExec() */
