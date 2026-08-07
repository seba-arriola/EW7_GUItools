/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_api_GetIdSource.c,v 1.3 2005/06/15 19:03:12 davidk Exp $
 *
 *
 *
 */


#include <ewdb_ora_api.h>
#include <ewdb_cli_base.h>
#include <ewdb_ew_oci_base.h>


static char SQL_STRING[] =
  "Begin Get_idSource(OUT_idSource => :OUT_idSource,IN_sSource => :IN_sSource,"
  "             IN_iEWInstID => :IN_iEWInstID, IN_idInst => :IN_idInst); End;";

static EWDB_OCI_SFS SQLParamsBindArray[] = 
{
  {0,1,0,0,0,OA_INT,":OUT_idSource"},
  {0,1,0,0,0,OA_SZ, ":IN_sSource"},
  {0,1,0,0,0,OA_INT,":IN_iEWInstID"},
  {0,1,0,0,0,OA_INT,":IN_idInst"}
};

#define	NUM_FIELDS	4


/* Insertion Struct for GetIDSource szStatement */
static EWDB_OCIStatementStruct SSStatement;

static EWDBid Local_idSource, Local_idInst;
static int Local_iEWInstID;
static char Local_szSource[256];

static int PrepGetIDSourceExec (char * IN_szSource, int IN_iEWInstID, EWDBid Local_idInst, EWDB_Cursor *ppCursor);
static int PostGetIDSourceExec (EWDBid * pidSource);
static int InitGetIDSourceStatement (char *szStatement, EWDB_OCIStatementStruct *pSS);


int ewdb_api_GetIDSource(EWDBid *pidSource, char * IN_szSource, int IN_iEWInstID, EWDBid IN_idInst)
{

	EWDB_Cursor pCursor;
  int rc;

	if (IN_szSource == NULL || pidSource == NULL)
	{
		logit ("", "ewdb_api_GetIDSource(): Null parameters passed in!\n");
		return EWDB_RETURN_FAILURE;
	}

	ewdb_base_SetLastOraAPIActionTime ();

  if (ewdb_base_Reconnect () != EWDB_RETURN_SUCCESS)
	/* Establishes connection, and performs binding!?! */
	{
		logit ("", "ewdb_api_GetIDSource(): Could not reconnect to the database!\n");
		return (EWDB_RETURN_FAILURE);
	}

	if (PrepGetIDSourceExec (IN_szSource, IN_iEWInstID, IN_idInst, &pCursor) != EWDB_RETURN_SUCCESS)
	{
		logit ("", "ewdb_api_GetIDSource(): PrepGetIDSourceExec() failed.\n");
		return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
	}

	if (ewdb_base_SQLExecute (pCursor))
	{
		ewdb_base_ErrorReport (hEWDBC, pCursor,"ewdb_api_GetIDSource(): ewdb_base_SQLExecute", 1);
		return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
	} 
  
	/* Commit the transaction (all the previous inserts!)
	   In Case there is any auditing or logging or debug
	   changes made in the stored procedures.
	****************************************************/
	if (ewdb_base_SQLCommit (hEWDBC))
	{
		ewdb_base_ErrorReport (hEWDBC, hEWDBC,"ewdb_api_GetIDSource(): ewdb_base_SQLCommit",2);
		return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
	}
	
  rc = PostGetIDSourceExec(pidSource);
  if(rc == EWDB_RETURN_FAILURE)
  {
    logit("", "ewdb_api_GetIDSource(): PostGetIDSourceExec failed!\n");
    return(ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }
  else if(rc == EWDB_RETURN_WARNING)
  {
    return(EWDB_RETURN_FAILURE);
  }

	return(EWDB_RETURN_SUCCESS);
}  /* end ewdb_internal_CreatePeakAmp() */


static int InitGetIDSourceStatement (char *szStatement, EWDB_OCIStatementStruct *pSS)
{

  pSS->FieldArray[0].pVal = &Local_idSource;
	pSS->FieldArray[1].pVal = Local_szSource;
	pSS->FieldArray[2].pVal = &Local_iEWInstID;
	pSS->FieldArray[3].pVal = &Local_idInst;

  return(ewdb_base_RequestCursor(szStatement, pSS,0/*don't force rebind*/));
}


static int PrepGetIDSourceExec(char * IN_szSource, int IN_iEWInstID, 
                         EWDBid IN_idInst, EWDB_Cursor *ppCursor)
{
	SSStatement.NumOfFields = NUM_FIELDS;
	SSStatement.FieldArray = SQLParamsBindArray;
	SSStatement.RecordSize = 0;

	/* Copy the incoming struct into the local struct */
  strncpy(Local_szSource, IN_szSource, sizeof(Local_szSource));
  Local_szSource[sizeof(Local_szSource)-1] = 0x00;
  Local_iEWInstID = IN_iEWInstID;
  Local_idInst    = IN_idInst;

  if(InitGetIDSourceStatement(SQL_STRING, &SSStatement)
     != EWDB_RETURN_SUCCESS)
	{
		logit ("", "PrepGetIDSourceExec(): InitGetIDSourceStatement failed!\n");
		return EWDB_RETURN_FAILURE;
	}

	*ppCursor = SSStatement.pCda;
	
	return EWDB_RETURN_SUCCESS;
}


static int PostGetIDSourceExec (EWDBid * pidSource)
{
  EWDB_Cursor pCursor;
  
  *pidSource = Local_idSource;
  
  pCursor = SSStatement.pCda;
  
  ewdb_base_ReleaseCursor (pCursor);

  if(Local_idSource <= 0)
  {
    logit("","PostGetIDSourceExec():  SQL Proc Get_idSource() returned %d.\n", Local_idSource);
    return(EWDB_RETURN_WARNING);
  }
  
	return (EWDB_RETURN_SUCCESS);
}  /* end PostGetIDSourceExec() */
