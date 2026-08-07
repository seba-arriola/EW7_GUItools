/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_api_DeletePolygon.c,v 1.2 2005/06/03 22:32:16 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_api_DeletePolygon.c,v $
 *     Revision 1.2  2005/06/03 22:32:16  davidk
 *     DB API Cleanup
 *
 *     Revision 1.1  2004/12/01 20:46:09  mark
 *     Initial checkin
 *
 *     Revision 1.1  2004/11/23 16:57:49  mark
 *     Initial checkin
 *
 */


#include <ewdb_ora_api.h>
#include <ewdb_cli_base.h>
#include <ewdb_ew_oci_base.h>


static char SQL_STRING[] =
  "Begin DeletePolygon(OUT_RetCode => :OUT_RetCode,"
  "                    IN_idPolygon => :IN_idPolygon); End;";

static EWDB_OCI_SFS SQLParamsBindArray[] = 
{
  {0,1,0,0,0,OA_INT,":OUT_RetCode"},
  {0,1,0,0,0,OA_INT,":IN_idPolygon"}
};

#define	NUM_FIELDS	2


/* Insertion Struct for DeletePolygon szStatement */
static EWDB_OCIStatementStruct SSStatement;

static int Local_iRetCode;
static int Local_idPolygon;

static int PrepDeletePolygonExec(EWDBid IN_idPolygon, EWDB_Cursor *ppCursor);
static int PostDeletePolygonExec();
static int InitDeletePolygonStatement(char *szStatement, EWDB_OCIStatementStruct *pSS);

int ewdb_api_DeletePolygon(EWDBid IN_idPolygon)
{
	EWDB_Cursor pCursor;
	int rc;

	if (IN_idPolygon <= 0)
  {
  	logit ("", "ewdb_api_DeletePolygon(): Invalid parameters passed in!\n");
  	return EWDB_RETURN_FAILURE;
  }

	ewdb_base_SetLastOraAPIActionTime ();

	if (ewdb_base_Reconnect () != EWDB_RETURN_SUCCESS)
  /* Establishes connection, and performs binding!?! */
  {
  	logit ("", "ewdb_api_DeletePolygon(): Could not reconnect to the database!\n");
  	return (EWDB_RETURN_FAILURE);
  }

	if (PrepDeletePolygonExec (IN_idPolygon, &pCursor) != EWDB_RETURN_SUCCESS)
  {
  	logit ("", "ewdb_api_DeletePolygon():PrepDeletePolygonExec() failed.\n");
  	return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  }

	if (ewdb_base_SQLExecute (pCursor))
  {
  	ewdb_base_ErrorReport (hEWDBC, pCursor,"ewdb_api_DeletePolygon(): ewdb_base_SQLExecute", 1);
  	return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  } 

  /* Commit the transaction (all the previous inserts!)
     In Case there is any auditing or logging or debug
     changes made in the stored procedures.
  ****************************************************/
	if (ewdb_base_SQLCommit (hEWDBC))
  {
  	ewdb_base_ErrorReport (hEWDBC, hEWDBC,"ewdb_api_DeletePolygon(): ewdb_base_SQLCommit",2);
  	return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  }
  
	rc = PostDeletePolygonExec();
	if(rc == EWDB_RETURN_FAILURE)
  {
  	logit("", "ewdb_api_DeletePolygon(): PostDeletePolygonExec failed!\n");
  	return(ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }
	else if(rc == EWDB_RETURN_WARNING)
  {
  	return(EWDB_RETURN_FAILURE);
  }

	return(EWDB_RETURN_SUCCESS);
}  /* end ewdb_api_DeletePolygon() */


static int InitDeletePolygonStatement (char *szStatement, EWDB_OCIStatementStruct *pSS)
{

	if ((szStatement == NULL) || (pSS == NULL))
  {
  	logit ("", "Null parameters passed in!\n");
  	return EWDB_RETURN_FAILURE;
  }

	pSS->FieldArray[0].pVal = &Local_iRetCode;
	pSS->FieldArray[1].pVal = &Local_idPolygon;

	if (ewdb_base_RequestCursor (szStatement, pSS, 0) != 0)
  {
  	logit ("", "InitDeletePolygonStatement(): Call to ewdb_base_RequestCursor failed.\n");
  	return EWDB_RETURN_FAILURE;
  }

	return EWDB_RETURN_SUCCESS;
}


static int PrepDeletePolygonExec(EWDBid IN_idPolygon, EWDB_Cursor *ppCursor)
{
	if (ppCursor == NULL)
  {
  	logit ("", "Null parameters passed in!\n");
  	return EWDB_RETURN_FAILURE;
  }

	SSStatement.NumOfFields = NUM_FIELDS;
	SSStatement.FieldArray = SQLParamsBindArray;
	SSStatement.RecordSize = 0;

  /* Copy the incoming struct into the local struct */
	Local_idPolygon = IN_idPolygon;

	if(InitDeletePolygonStatement (SQL_STRING, &SSStatement)
     != EWDB_RETURN_SUCCESS)
  {
  	logit ("", "Call to InitDeletePolygonStatement failed!\n");
  	return EWDB_RETURN_FAILURE;
  }

  *ppCursor = SSStatement.pCda;
  
	return EWDB_RETURN_SUCCESS;
} /* end PrepDeletePolygonExec() */


static int PostDeletePolygonExec()
{
	EWDB_Cursor pCursor;

	pCursor = SSStatement.pCda;
	ewdb_base_ReleaseCursor (pCursor);

	if(Local_iRetCode != 0)
  {
  	logit("","PostDeletePolygonExec():  SQL Proc DeletePolygon() returned "
        "the following error(%d).  Please see that proc for details.\n",
      	Local_iRetCode);
  	return(EWDB_RETURN_WARNING);
  }

	return (EWDB_RETURN_SUCCESS);
}  /* end PostDeletePolygonExec() */
