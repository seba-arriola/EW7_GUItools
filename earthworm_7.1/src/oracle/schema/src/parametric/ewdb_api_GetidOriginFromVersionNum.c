/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_api_GetidOriginFromVersionNum.c,v 1.4 2005/06/15 19:03:12 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_api_GetidOriginFromVersionNum.c,v $
 *     Revision 1.4  2005/06/15 19:03:12  davidk
 *     DB API Cleanup
 *
 *     Revision 1.3  2002/09/10 17:26:25  lucky
 *     Stable scaffold
 *
 *     Revision 1.2  2002/08/15 15:26:19  davidk
 *     Fixed compile errors by removing a chunk of data.
 *
 *     Revision 1.1  2002/08/14 14:57:25  davidk
 *     Initial revision
 *
 *
 *
 */


#include <ewdb_ora_api.h>
#include <ewdb_cli_base.h>
#include <ewdb_ew_oci_base.h>


static char SQL_STRING[] =
	"Begin :OUT_RetCode := Get_idOrigin_From_Source_Data(OUT_idOrigin => :OUT_idOrigin,"
	"OUT_idEvent => :OUT_idEvent,"
	"IN_sSource => :IN_sSource,"
	"IN_sSourceEventID => :IN_sSourceEventID,"
	"IN_iVersionNum => :IN_iVersionNum,"
	"IN_tiExternal => NULL,"  /* we are skipping tiExternal */
	"IN_xidExternal => :IN_xidExternal"
  "); End;";
	
static EWDB_OCI_SFS SQLParamsBindArray[] = 
{

  {0,1,0,0,0,OA_INT,":OUT_RetCode"},
  {0,1,0,0,0,OA_INT,":OUT_idOrigin"},
  {0,1,0,0,0,OA_INT,":OUT_idEvent"},
  {0,1,0,0,0,OA_SZ,":IN_sSource"},
  {0,1,0,0,0,OA_SZ,":IN_sSourceEventID"},
  {0,1,0,0,0,OA_INT,":IN_iVersionNum"},
  {0,1,0,0,0,OA_SZ,":IN_xidExternal"}
};
  /* {0,1,0,0,0,OA_SZ,":IN_sExternal_Table_Name"}, */


#define	NUM_FIELDS	7

/* Insertion Struct for GetidOriginFromVersionNum szStatement */
static EWDB_OCIStatementStruct SSStatement;

static EWDB_OriginStruct	Local_OriginStruct;

static int Local_iRetCode;
/*******************************
   FUNCTION PROTOTYPES
*******************************/
static int PrepGetidOriginFromVersionNumExec(EWDB_OriginStruct *pOrigin, EWDB_Cursor *ppCursor);
static int PostGetidOriginFromVersionNumExec(EWDB_OriginStruct *pOrigin);
static int InitGetidOriginFromVersionNumStatement(char *szStatement, EWDB_OCIStatementStruct *pSS);
/******************************/


int ewdb_api_GetidOriginFromVersionNum(EWDB_OriginStruct *pOrigin)
{
	EWDB_Cursor pCursor;
  int rc;

	if (pOrigin == NULL)
	{
		logit ("", "ewdb_api_GetidOriginFromVersionNum(): Null parameters passed in!\n");
		return EWDB_RETURN_FAILURE;
	}

	ewdb_base_SetLastOraAPIActionTime ();

	if (ewdb_base_Reconnect () != EWDB_RETURN_SUCCESS)
	/* Establishes connection, and performs binding!?! */
	{
		logit ("", "ewdb_api_GetidOriginFromVersionNum(): Could not reconnect to the database!\n");
		return (EWDB_RETURN_FAILURE);
	}

	if (PrepGetidOriginFromVersionNumExec (pOrigin, &pCursor) != EWDB_RETURN_SUCCESS)
	{
		logit ("", "ewdb_api_GetidOriginFromVersionNum(): PrepGetidOriginFromVersionNumExec() failed.\n");
		return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
	}

	if (ewdb_base_SQLExecute (pCursor))
	{
		ewdb_base_ErrorReport (hEWDBC, pCursor,"ewdb_api_GetidOriginFromVersionNum(): ewdb_base_SQLExecute", 1);
		return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
	} 

	/* Commit the transaction (all the previous inserts!)
	   In Case there is any auditing or logging or debug
	   changes made in the stored procedures.
	****************************************************/
	if (ewdb_base_SQLCommit (hEWDBC))
	{
		ewdb_base_ErrorReport (hEWDBC, hEWDBC,"ewdb_api_GetidOriginFromVersionNum(): ewdb_base_SQLCommit",2);
		return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
	}

  rc = PostGetidOriginFromVersionNumExec(pOrigin);
  if(rc == EWDB_RETURN_FAILURE)
  {
    logit("", "ewdb_api_GetidOriginFromVersionNum(): PostGetidOriginFromVersionNumExec failed!\n");
    return(ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }
  else if(rc == EWDB_RETURN_WARNING)
  {
    return(EWDB_RETURN_FAILURE);
  }
  
	return(EWDB_RETURN_SUCCESS);
}  /* end ewdb_api_GetidOriginFromVersionNum() */


static int InitGetidOriginFromVersionNumStatement (char *szStatement, EWDB_OCIStatementStruct *pSS)
{

  pSS->FieldArray[0].pVal = &(Local_iRetCode);
	pSS->FieldArray[1].pVal = &(Local_OriginStruct.idOrigin);
	pSS->FieldArray[2].pVal = &(Local_OriginStruct.idEvent);
	pSS->FieldArray[3].pVal = Local_OriginStruct.sSource;
	pSS->FieldArray[4].pVal = Local_OriginStruct.szSourceEventID;
	pSS->FieldArray[5].pVal = &(Local_OriginStruct.iVersionNum);
	pSS->FieldArray[6].pVal = Local_OriginStruct.xidExternal;

  return(ewdb_base_RequestCursor(szStatement, pSS,0/*don't force rebind*/));
}


static int PrepGetidOriginFromVersionNumExec (EWDB_OriginStruct *pOrigin, EWDB_Cursor *ppCursor)
{

	SSStatement.NumOfFields = NUM_FIELDS;
	SSStatement.FieldArray = SQLParamsBindArray;
	SSStatement.RecordSize = 0;

	/* Copy the incomming struct into the local struct */
	memcpy (&Local_OriginStruct, pOrigin, sizeof (EWDB_OriginStruct));

  Local_iRetCode = 0;
	if (InitGetidOriginFromVersionNumStatement (SQL_STRING, &SSStatement)
      != EWDB_RETURN_SUCCESS)
	{
		logit ("", "PrepGetidOriginFromVersionNumExec():  InitGetidOriginFromVersionNumStatement failed!\n");
		return EWDB_RETURN_FAILURE;
	}

	*ppCursor = SSStatement.pCda;
	
	return EWDB_RETURN_SUCCESS;
} /* End PrepGetidOriginFromVersionNumExec() */


static int PostGetidOriginFromVersionNumExec (EWDB_OriginStruct *pOrigin)
{
  EWDB_Cursor pCursor;
  
  pOrigin->idOrigin = Local_OriginStruct.idOrigin;
  pOrigin->idEvent = Local_OriginStruct.idEvent;

  pCursor = SSStatement.pCda;
  
  ewdb_base_ReleaseCursor (pCursor);

  if(Local_iRetCode < 0)
  {
    if(Local_iRetCode == -1)
      logit("","PostGetidOriginFromVersionNumExec():  SQL Proc Create_Origin() returned "
                "\"Unknown error\".  See SQL debug table for details.\n");
    else if(Local_iRetCode == -3)
      logit("","PostGetidOriginFromVersionNumExec():  SQL Proc Create_Origin() returned "
                "\"Unable to Create Origin -- iVersionNum(%d) was null\".  \n",
            pOrigin->iVersionNum);
    else if(Local_iRetCode == -4)
      logit("","PostGetidOriginFromVersionNumExec():  SQL Proc Create_Origin() returned "
                "\"Unable to Create Origin -- iVersionNum(%d) sSourceEventID(%s)\".  \n",
            pOrigin->iVersionNum, pOrigin->szSourceEventID);
    else
      logit("","PostGetidOriginFromVersionNumExec():  SQL Proc Create_Origin() returned "
               "the following error(%d).  Please see that proc for details.\n",
            Local_iRetCode);
    return(EWDB_RETURN_WARNING);
  }
  
  return (EWDB_RETURN_SUCCESS);
} /* End PostGetidOriginFromVersionNumExec() */
