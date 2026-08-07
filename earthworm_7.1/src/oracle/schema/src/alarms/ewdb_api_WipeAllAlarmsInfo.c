/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_api_WipeAllAlarmsInfo.c,v 1.2 2005/06/03 22:32:16 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_api_WipeAllAlarmsInfo.c,v $
 *     Revision 1.2  2005/06/03 22:32:16  davidk
 *     DB API Cleanup
 *
 *     Revision 1.1  2005/02/17 23:03:02  mark
 *     Initial checkin
 *
 *     Revision 1.1  2005/02/03 20:42:27  mark
 *     Initial checkin
 *
 */


#include <ewdb_ora_api.h>
#include <ewdb_cli_base.h>
#include <ewdb_ew_oci_base.h>


static char SQL_STRING[] =
  "Begin WipeAlarmsInfo(OUT_Local_iRetCode => :OUT_Local_iRetCode); End;";

static EWDB_OCI_SFS WipeAlarmsBindArray[] = 
{
  {0,1,0,0,0,OA_INT,":OUT_Local_iRetCode"},
};

#define	NUM_FIELDS	1


/* Insertion Struct for WipeAlarms szStatement */
static EWDB_OCIStatementStruct SSStatement;

static int Local_iRetCode;

static int PrepWipeAlarmsExec(EWDB_Cursor *ppCursor);
static int PostWipeAlarmsExec();
static int InitWipeAlarmsStatement(char *szStatement, EWDB_OCIStatementStruct *pSS);

int ewdb_api_WipeAllAlarmsInfo()
{
	EWDB_Cursor pCursor;
	int rc;

	ewdb_base_SetLastOraAPIActionTime ();

	if (ewdb_base_Reconnect () != EWDB_RETURN_SUCCESS)
	/* Establishes connection, and performs binding!?! */
	{
		logit ("", "ewdb_api_WipeAllAlarmsInfo(): Could not reconnect to the database!\n");
		return (EWDB_RETURN_FAILURE);
	}

	if (PrepWipeAlarmsExec(&pCursor) != EWDB_RETURN_SUCCESS)
	{
		logit ("", "ewdb_api_WipeAllAlarmsInfo():PrepWipeAlarmsExec() failed.\n");
		return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
	}

	if (ewdb_base_SQLExecute (pCursor))
	{
		ewdb_base_ErrorReport (hEWDBC, pCursor,"ewdb_api_WipeAllAlarmsInfo(): ewdb_base_SQLExecute", 1);
		return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
	} 

	/* Commit the transaction (all the previous inserts!)
	   In Case there is any auditing or logging or debug
	   changes made in the stored procedures.
	****************************************************/
	if (ewdb_base_SQLCommit (hEWDBC))
	{
		ewdb_base_ErrorReport (hEWDBC, hEWDBC,"ewdb_api_WipeAllAlarmsInfo(): ewdb_base_SQLCommit",2);
		return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
	}
	
	rc = PostWipeAlarmsExec();
	if(rc == EWDB_RETURN_FAILURE)
	{
		logit("", "ewdb_api_WipeAllAlarmsInfo(): PostWipeAlarmsExec failed!\n");
		return(ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
	}
	else if(rc == EWDB_RETURN_WARNING)
	{
		return(EWDB_RETURN_FAILURE);
	}

	return(EWDB_RETURN_SUCCESS);
}  /* end ewdb_api_WipeAllAlarmsInfo() */


static int InitWipeAlarmsStatement (char *szStatement, EWDB_OCIStatementStruct *pSS)
{

	if ((szStatement == NULL) || (pSS == NULL))
	{
		logit ("", "InitWipeAlarmsStatement(): Null parameters passed in!\n");
		return EWDB_RETURN_FAILURE;
	}

	pSS->FieldArray[0].pVal = &Local_iRetCode;

	if (ewdb_base_RequestCursor (szStatement, pSS, 0) != 0)
	{
		logit ("", "InitWipeAlarmsStatement(): Call to ewdb_base_RequestCursor failed.\n");
		return EWDB_RETURN_FAILURE;
	}

	return EWDB_RETURN_SUCCESS;
}


static int PrepWipeAlarmsExec(EWDB_Cursor *ppCursor)
{
	if (ppCursor == NULL)
	{
		logit ("", "Null parameters passed in!\n");
		return EWDB_RETURN_FAILURE;
	}

	SSStatement.NumOfFields = NUM_FIELDS;
	SSStatement.FieldArray = WipeAlarmsBindArray;
	SSStatement.RecordSize = 0;

	if(InitWipeAlarmsStatement(SQL_STRING, &SSStatement) != EWDB_RETURN_SUCCESS)
	{
		logit ("", "Call to InitWipeAlarmsStatement failed!\n");
		return EWDB_RETURN_FAILURE;
	}

	*ppCursor = SSStatement.pCda;
	
	return EWDB_RETURN_SUCCESS;
} /* PrepWipeAlarmsExec() */


static int PostWipeAlarmsExec()
{
	EWDB_Cursor pCursor;

	pCursor = SSStatement.pCda;
	ewdb_base_ReleaseCursor (pCursor);

	if(Local_iRetCode != 0)
	{
		logit("","PostWipeAlarmsExec():  SQL Proc WipeAlarms() returned "
				"the following error(%d).  Please see that proc for details.\n",
				Local_iRetCode);
		return(EWDB_RETURN_WARNING);
	}

	return (EWDB_RETURN_SUCCESS);
}  /* end PostWipeAlarmsExec() */
