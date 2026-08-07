/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_api_DeleteAlarmsUser.c,v 1.2 2005/06/03 22:32:16 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_api_DeleteAlarmsUser.c,v $
 *     Revision 1.2  2005/06/03 22:32:16  davidk
 *     DB API Cleanup
 *
 *     Revision 1.1  2001/05/15 02:16:14  davidk
 *     Initial revision
 *
 *
 */


#include <ewdb_ora_api.h>
#include <ewdb_cli_base.h>


static char SQL_STRING[] =
  "Begin DeleteAlarmsUser (OUT_RetCode => :OUT_RetCode, "
  "                        IN_idUser => :IN_idUser); End;";

static EWDB_OCI_SFS SQLParamsBindArray[] = 
{
  {0,1,0,0,0,OA_INT,":OUT_RetCode"},
  {0,1,0,0,0,OA_INT,":IN_idUser"},
};

#define	NUM_FIELDS	2

/* Insertion Struct for DeleteAlUsrrogram szStatement */
static EWDB_OCIStatementStruct SSStatement;

static	int	    Local_RetCode;
static	EWDBid  Local_idUser;


/********************************
      FUNCTION PROTOTYPES
********************************/
static int PrepDeleteAlUsrExec (EWDBid IN_idUser, EWDB_Cursor *ppCursor);
static int PostDeleteAlUsrExec ();
static int InitDeleteAlUsrStatement (char *szStatement, EWDB_OCIStatementStruct *pSS);
/*******************************/


int ewdb_api_DeleteAlarmsUser (EWDBid IN_idUser)
{
	EWDB_Cursor pCursor;

	if (IN_idUser <= 0)
  {
  	logit ("", "ewdb_api_DeleteAlarmsUser(): Invalid parameters passed in!\n");
  	return EWDB_RETURN_FAILURE;
  }

  /* Set longer timeout -- need this for large events */
	ewdb_base_SetOraConnectionTimeout(15*60);

	if (ewdb_base_Reconnect () != EWDB_RETURN_SUCCESS)
  {
  	logit ("", "ewdb_api_DeleteAlarmsUser(): Could not reconnect to the database!\n");
  	return (EWDB_RETURN_FAILURE);
  }

	if (PrepDeleteAlUsrExec (IN_idUser, &pCursor) != EWDB_RETURN_SUCCESS)
  {
  	logit ("", "ewdb_api_DeleteAlarmsUser(): PrepDeleteAlUsrExec() failed.\n");
  	return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  }

	if (ewdb_base_SQLExecute (pCursor))
  {
  	ewdb_base_ErrorReport (hEWDBC, pCursor,"ewdb_api_DeleteAlarmsUser(): ewdb_base_SQLExecute", 1);
  	return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  } 
  
  /* Commit the transaction (all the previous inserts!)
     In Case there is any auditing or logging or debug
     changes made in the stored procedures.
  ****************************************************/
	if (ewdb_base_SQLCommit (hEWDBC))
  {
  	ewdb_base_ErrorReport (hEWDBC, hEWDBC,"ewdb_api_DeleteAlarmsUser(): ewdb_base_SQLCommit",2);
  	return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  }

	if (PostDeleteAlUsrExec() != EWDB_RETURN_SUCCESS)
  {
  	logit ("", "ewdb_api_DeleteAlarmsUser(): PostDeleteAlUsrExec failed!\n");
  	return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  }

  /* reset the timeout */
	ewdb_base_SetLastOraAPIActionTime ();

	return EWDB_RETURN_SUCCESS;
}  


static int InitDeleteAlUsrStatement (char *szStatement, EWDB_OCIStatementStruct *pSS)
{

	if ((szStatement == NULL) || (pSS == NULL))
  {
  	logit ("", "InitDeleteAlUsrStatement(): Invalid parameters passed in!\n");
  	return EWDB_RETURN_FAILURE;
  }

	pSS->FieldArray[0].pVal = &Local_RetCode;
	pSS->FieldArray[1].pVal = &Local_idUser;

  if (ewdb_base_RequestCursor (szStatement, pSS, 0) != 0)
  {
    logit ("", "InitDeleteAlUsrStatement(): ewdb_base_RequestCursor failed.\n");
    return EWDB_RETURN_FAILURE;
  }
	return EWDB_RETURN_SUCCESS;
}


static int PrepDeleteAlUsrExec (EWDBid IN_idUser, EWDB_Cursor *ppCursor)
{

	if (ppCursor == NULL)
  {
  	logit ("", "PrepDeleteAlUsrExec(): Invalid parameters passed in!\n");
  	return EWDB_RETURN_FAILURE;
  }

	SSStatement.NumOfFields = NUM_FIELDS;
	SSStatement.FieldArray = SQLParamsBindArray;
	SSStatement.RecordSize = 0;

	Local_RetCode = 0;
	Local_idUser = IN_idUser;

	if (InitDeleteAlUsrStatement (SQL_STRING, &SSStatement) 
                                != EWDB_RETURN_SUCCESS)
  {
  	logit ("", "PrepDeleteAlUsrExec(): InitDeleteAlUsrStatement failed!\n");
  	return EWDB_RETURN_FAILURE;
  }

  *ppCursor = SSStatement.pCda;
  
	return EWDB_RETURN_SUCCESS;
}


static int PostDeleteAlUsrExec()
{
	EWDB_Cursor pCursor;
  
	pCursor = SSStatement.pCda;
  
	ewdb_base_ReleaseCursor (pCursor);
  
	if (Local_RetCode != 0)
  {
  	logit ("", "SQL call to DeleteAlarmsUser returned error: %d\n", Local_RetCode);
      return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  }

	return (EWDB_RETURN_SUCCESS);
}
