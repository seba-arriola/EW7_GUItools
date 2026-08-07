/*
 *   THIS FILE IS UNDER CVS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_api_CreateAlarmsGroup.c,v 1.2 2005/06/03 22:32:16 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_api_CreateAlarmsGroup.c,v $
 *     Revision 1.2  2005/06/03 22:32:16  davidk
 *     DB API Cleanup
 *
 *     Revision 1.1  2005/02/03 20:43:00  mark
 *     Initial checkin
 *
 */


#include <ewdb_ora_api.h>
#include <ewdb_cli_base.h>
#include <ewdb_ew_oci_base.h>
#include <ewdb_oci_base_internal.h>


static char SQL_STRING[] =
	"Begin CreateAlarmsGroup (OUT_idGroup => :OUT_idGroup,"
	"IN_sName => :IN_sName, IN_bActive => :IN_bActive); End;";
	
static EWDB_OCI_SFS SQLParamsBindArray[] = 
{
  {0,1,0,0,0,OA_INT,    ":OUT_idGroup"},
  {0,1,256,0,0,OA_SZ,   ":IN_sName"},
  {0,1,0,0,0,OA_INT,    ":IN_bActive"},
};

#define	NUM_FIELDS	3

static  EWDB_OCIStatementStruct SSStatement;

static  EWDB_AlarmsGroupStruct	Local_GroupStruct;


static int PrepCreateGroupExec (EWDB_AlarmsGroupStruct *, EWDB_Cursor *);
static int PostCreateGroupExec (EWDB_AlarmsGroupStruct *);
static int InitCreateGroupStatement (char *, EWDB_OCIStatementStruct *);


int ewdb_api_CreateAlarmsGroup(EWDB_AlarmsGroupStruct *pGroup)
{

	EWDB_Cursor	pCursor;

	if (pGroup == NULL)
	{
		logit ("", "ewdb_api_CreateAlarmsGroup(): Null parameters passed in!\n");
		return EWDB_RETURN_FAILURE;
	}

	ewdb_base_SetLastOraAPIActionTime ();

	if (ewdb_base_Reconnect () != EWDB_RETURN_SUCCESS)
	/* Establishes connection, and performs binding!?! */
	{
		logit ("", "ewdb_api_CreateAlarmsGroup(): Could not reconnect to the database!\n");
		return (EWDB_RETURN_FAILURE);
	}

	if (PrepCreateGroupExec (pGroup, &pCursor) != EWDB_RETURN_SUCCESS)
	{
		logit ("", "ewdb_api_CreateAlarmsGroup(): PrepCreateGroupExec failed.\n");
		return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
	}

	if (ewdb_base_SQLExecute (pCursor))
	{
		ewdb_base_ErrorReport (hEWDBC, pCursor,"ewdb_api_CreateAlarmsGroup(): ewdb_base_SQLExecute", 1);
		return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
	} 
  
	/* Commit the transaction (all the previous inserts!)
	   In Case there is any auditing or logging or debug
	   changes made in the stored procedures.
	****************************************************/
	if (ewdb_base_SQLCommit (hEWDBC))
	{
		ewdb_base_ErrorReport (hEWDBC, hEWDBC, 
				"ewdb_api_CreateAlarmsGroup(): ewdb_base_SQLCommit",2);
		return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
	}

	if (PostCreateGroupExec (pGroup) != EWDB_RETURN_SUCCESS)
	{
		logit ("", "ewdb_api_CreateAlarmsGroup(): PostCreateGroupExec failed!\n");
		return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
	}

	return EWDB_RETURN_SUCCESS;
}  /* end ewdb_api_CreateAlarmsGroup() */


/******************* InitCreateGroupStatement *******************/
static int InitCreateGroupStatement(char *statement, EWDB_OCIStatementStruct *pSS)
{

	if ((statement == NULL) ||  (pSS == NULL))
	{
		logit ("", "InitCreateGroupStatement(): Null parameters passed in!\n");
		return EWDB_RETURN_FAILURE;
	}

	pSS->FieldArray[0].pVal = &(Local_GroupStruct.idGroup);
	pSS->FieldArray[1].pVal = (Local_GroupStruct.sName);
	pSS->FieldArray[2].pVal = &(Local_GroupStruct.bActive);

	if (ewdb_base_RequestCursor (statement, pSS, 0) != 0)
	{
		logit ("", "InitCreateGroupStatement(): ewdb_base_RequestCursor failed.\n");
		return EWDB_RETURN_FAILURE;
	}

	return EWDB_RETURN_SUCCESS;
}  /* end InitCreateGroupStatement() */


/******************* PrepCreateGroupExec *******************/
static int PrepCreateGroupExec(EWDB_AlarmsGroupStruct *pGroup, EWDB_Cursor *ppCursor)
{

	if ((pGroup == NULL) || (ppCursor == NULL))
	{
		logit ("", "PrepCreateGroupExec(): Null parameters passed in!\n");
		return EWDB_RETURN_FAILURE;
	}

	memcpy (&Local_GroupStruct, pGroup, sizeof (EWDB_AlarmsGroupStruct));

	SSStatement.NumOfFields = NUM_FIELDS;
	SSStatement.FieldArray = SQLParamsBindArray;
	SSStatement.RecordSize = 0;

	if (InitCreateGroupStatement (SQL_STRING, &SSStatement)
												     != EWDB_RETURN_SUCCESS)
	{
		logit ("", "PrepCreateGroupExec(): InitCreateGroupStatement failed!\n");
		return EWDB_RETURN_FAILURE;
	}

	*ppCursor = SSStatement.pCda;
	
	return EWDB_RETURN_SUCCESS;
}  /* end PrepCreateGroupExec() */


/******************* PostCreateGroupExec *******************/
static int PostCreateGroupExec (EWDB_AlarmsGroupStruct *pGroup)
{
	EWDB_Cursor pCursor;

	pCursor = SSStatement.pCda;
  
	ewdb_base_ReleaseCursor (pCursor);
  
	if (pGroup == NULL)
	{
		logit ("", "PostCreateGroupExec(): Invalid arguments passed in.\n");
		return EWDB_RETURN_FAILURE;
	}
  
	pGroup->idGroup = Local_GroupStruct.idGroup;
  if(Local_GroupStruct.idGroup <= 0)
  {
    logit("","PostCreateGroupExec(): ERROR SQL Proc CreateAlarmsGroup() returned error %d\n", 
          Local_GroupStruct.idGroup);
    return(EWDB_RETURN_WARNING);
  }
  
	return EWDB_RETURN_SUCCESS;
}  /* end PostCreateGroupExec() */
