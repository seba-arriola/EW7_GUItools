/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_api_UpdateRulePolygon.c,v 1.2 2005/06/03 22:32:16 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_api_UpdateRulePolygon.c,v $
 *     Revision 1.2  2005/06/03 22:32:16  davidk
 *     DB API Cleanup
 *
 *     Revision 1.1  2004/12/14 16:57:57  mark
 *     Added ewdb_api_UpdateRulePolygon
 *
 *
 */


#include <ewdb_ora_api.h>
#include <ewdb_cli_base.h>
#include <ewdb_ew_oci_base.h>


static char SQL_STRING[] =
  "Begin UpdateRulePolygons(IN_idOldPolygon => :IN_idOldPolygon,"
	"                         IN_idNewPolygon => :IN_idNewPolygon); End;";

static EWDB_OCI_SFS SQLParamsBindArray[] = 
{
  {0,1,0,0,0,OA_INT,":IN_idOldPolygon"},
  {0,1,0,0,0,OA_INT,":IN_idNewPolygon"}
};

#define	NUM_FIELDS	2


/* Insertion Struct for UpdateRulePolygons szStatement */
static EWDB_OCIStatementStruct SSStatement;

static	EWDBid	Local_idOldPolygon;
static	EWDBid	Local_idNewPolygon;

static int PrepUpdateRulePolygonsExec(EWDBid idOldPolygon, EWDBid idNewPolygon, EWDB_Cursor *ppCursor);
static int PostUpdateRulePolygonsExec();
static int InitUpdateRulePolygonsStatement(char *szStatement, EWDB_OCIStatementStruct *pSS);

int ewdb_api_UpdateRulePolygons(EWDBid idOldPolygon, EWDBid idNewPolygon)
{
	EWDB_Cursor pCursor;
	int rc;

	if (idOldPolygon <= 0 || idNewPolygon <= 0)
	{
		logit ("", "ewdb_api_UpdateRulePolygons(): Bad parameters passed in!\n");
		return EWDB_RETURN_FAILURE;
	}

	ewdb_base_SetLastOraAPIActionTime ();

	if (ewdb_base_Reconnect () != EWDB_RETURN_SUCCESS)
	/* Establishes connection, and performs binding!?! */
	{
		logit ("", "ewdb_api_UpdateRulePolygons(): Could not reconnect to the database!\n");
		return (EWDB_RETURN_FAILURE);
	}

	if (PrepUpdateRulePolygonsExec (idOldPolygon, idNewPolygon, &pCursor) != EWDB_RETURN_SUCCESS)
	{
		logit ("", "ewdb_api_UpdateRulePolygons(): PrepUpdateRulePolygonsExec() failed.\n");
		return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
	}

	if (ewdb_base_SQLExecute (pCursor))
	{
		ewdb_base_ErrorReport (hEWDBC, pCursor,"ewdb_api_UpdateRulePolygons(): ewdb_base_SQLExecute", 1);
		return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
	} 

	/* Commit the transaction (all the previous inserts!)
	   In Case there is any auditing or logging or debug
	   changes made in the stored procedures.
	****************************************************/
	if (ewdb_base_SQLCommit (hEWDBC))
	{
		ewdb_base_ErrorReport (hEWDBC, hEWDBC,"ewdb_api_UpdateRulePolygons(): ewdb_base_SQLCommit",2);
		return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
	}
	
	rc = PostUpdateRulePolygonsExec();
	if(rc == EWDB_RETURN_FAILURE || rc == EWDB_RETURN_WARNING)
	{
		logit("", "ewdb_api_UpdateRulePolygons(): PostUpdateRulePolygonsExec failed!\n");
		return(ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
	}

	return(EWDB_RETURN_SUCCESS);
}  /* end ewdb_api_UpdateRulePolygons() */


static int InitUpdateRulePolygonsStatement (char *szStatement, EWDB_OCIStatementStruct *pSS)
{

	if ((szStatement == NULL) || (pSS == NULL))
	{
		logit ("", "InitUpdateRulePolygonsStatement(): Null parameters passed in!\n");
		return EWDB_RETURN_FAILURE;
	}

	pSS->FieldArray[0].pVal = &(Local_idOldPolygon);
	pSS->FieldArray[1].pVal = &(Local_idNewPolygon);

	if (ewdb_base_RequestCursor (szStatement, pSS, 0) != 0)
	{
		logit ("", "InitUpdateRulePolygonsStatement(): Call to ewdb_base_RequestCursor failed.\n");
		return EWDB_RETURN_FAILURE;
	}

	return EWDB_RETURN_SUCCESS;
} /* end InitUpdateRulePolygonsStatement() */


static int PrepUpdateRulePolygonsExec(EWDBid idOldPolygon, EWDBid idNewPolygon, EWDB_Cursor *ppCursor)
{
	if (ppCursor == NULL)
	{
		logit ("", "PrepUpdateRulePolygonsExec(): Null parameters passed in!\n");
		return EWDB_RETURN_FAILURE;
	}

	SSStatement.NumOfFields = NUM_FIELDS;
	SSStatement.FieldArray = SQLParamsBindArray;
	SSStatement.RecordSize = 0;

	/* Copy the incoming struct into the local struct */
	Local_idOldPolygon = idOldPolygon;
	Local_idNewPolygon = idNewPolygon;

	if(InitUpdateRulePolygonsStatement (SQL_STRING, &SSStatement)
		 != EWDB_RETURN_SUCCESS)
	{
		logit ("", "Call to InitUpdateRulePolygonsStatement failed!\n");
		return EWDB_RETURN_FAILURE;
	}

	*ppCursor = SSStatement.pCda;
	
	return EWDB_RETURN_SUCCESS;
} /* end PrepUpdateRulePolygonsExec() */


static int PostUpdateRulePolygonsExec()
{
	EWDB_Cursor pCursor;

	pCursor = SSStatement.pCda;

	ewdb_base_ReleaseCursor (pCursor);

	return (EWDB_RETURN_SUCCESS);
}  /* end PostUpdateRulePolygonsExec() */
