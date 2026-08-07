/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_api_DeleteEmailDelivery.c,v 1.3 2005/06/03 22:32:16 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_api_DeleteEmailDelivery.c,v $
 *     Revision 1.3  2005/06/03 22:32:16  davidk
 *     DB API Cleanup
 *
 *     Revision 1.2  2001/08/07 16:51:06  lucky
 *     Pre v6.0 checkin
 *
 *     Revision 1.1  2001/05/15 02:16:14  davidk
 *     Initial revision
 *
 *
 */

#include <ewdb_ora_api.h>
#include <ewdb_cli_base.h>
#include <ewdb_ew_oci_base.h>


static char SQL_STRING[] =
  "Begin DeleteEmailDelivery (OUT_RetCode => :OUT_RetCode, "
	"                           IN_idDelivery => :IN_idDelivery); End;";

static EWDB_OCI_SFS SQLParamsBindArray[] = 
{
  {0,1,0,0,0,OA_INT,":OUT_RetCode"},
  {0,1,0,0,0,OA_INT,":IN_idDelivery"},
};

#define	NUM_FIELDS	2

/* Insertion Struct for DeleteEmDelrogram statement */
EWDB_OCIStatementStruct SSStatement;

static	int		  Local_RetCode;
static	EWDBid	Local_idDelivery;


/********************************
      FUNCTION PROTOTYPES
********************************/
static int PrepDeleteEmDelExec (EWDBid IN_idDelivery, EWDB_Cursor *ppCursor);
static int PostDeleteEmDelExec ();
static int InitDeleteEmDelStatement (char *statement, EWDB_OCIStatementStruct *pSS);
/*******************************/


int ewdb_api_DeleteEmailDelivery (int IN_idDelivery)
{

	EWDB_Cursor pCursor;

	if (IN_idDelivery <= 0)
	{
		logit ("", "ewdb_api_DeleteEmailDelivery(): Invalid parameters passed in!\n");
		return EWDB_RETURN_FAILURE;
	}

	/* Set longer timeout -- need this for large events */
	ewdb_base_SetOraConnectionTimeout(15*60);


	if (ewdb_base_Reconnect () != EWDB_RETURN_SUCCESS)
	{
		logit ("", "ewdb_api_DeleteEmailDelivery(): Could not reconnect to the database!\n");
		return (EWDB_RETURN_FAILURE);
	}

	if (PrepDeleteEmDelExec (IN_idDelivery, &pCursor) != EWDB_RETURN_SUCCESS)
	{
		logit ("", "ewdb_api_DeleteEmailDelivery(): PrepDeleteEmDelExec() failed.\n");
		return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
	}

	if (ewdb_base_SQLExecute (pCursor))
	{
		ewdb_base_ErrorReport (hEWDBC, pCursor,"ewdb_api_DeleteEmailDelivery(): ewdb_base_SQLExecute", 1);
		return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
	} 
  
	/* Commit the transaction (all the previous inserts!)
	   In Case there is any auditing or logging or debug
	   changes made in the stored procedures.
	****************************************************/
	if (ewdb_base_SQLCommit (hEWDBC))
	{
		ewdb_base_ErrorReport (hEWDBC, hEWDBC,"ewdb_api_DeleteEmailDelivery(): ewdb_base_SQLCommit",2);
		return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
	}

	if (PostDeleteEmDelExec () != EWDB_RETURN_SUCCESS)
	{
		logit ("", "ewdb_api_DeleteEmailDelivery(): PostDeleteEmDelExec failed!\n");
		return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
	}

	/* reset the timeout */
	ewdb_base_SetLastOraAPIActionTime ();

	return EWDB_RETURN_SUCCESS;
}  


static int InitDeleteEmDelStatement (char *statement, EWDB_OCIStatementStruct *pSS)
{

	if ((statement == NULL) || (pSS == NULL))
	{
		logit ("", "InitDeleteEmDelStatement(): Invalid parameters passed in!\n");
		return EWDB_RETURN_FAILURE;
	}


	pSS->FieldArray[0].pVal = &Local_RetCode;
	pSS->FieldArray[1].pVal = &Local_idDelivery;

	if (ewdb_base_RequestCursor (statement, pSS, 0) != 0)
	{
		logit ("", "InitDeleteEmDelStatement(): ewdb_base_RequestCursor failed.\n");
		return EWDB_RETURN_FAILURE;
	}
	return EWDB_RETURN_SUCCESS;
}


static int PrepDeleteEmDelExec (EWDBid IN_idDelivery, EWDB_Cursor *ppCursor)
{

	if (ppCursor == NULL)
	{
		logit ("", "PrepDeleteEmDelExec(): Invalid parameters passed in!\n");
		return EWDB_RETURN_FAILURE;
	}


	SSStatement.NumOfFields = NUM_FIELDS;
	SSStatement.FieldArray = SQLParamsBindArray;
	SSStatement.RecordSize = 0;

	Local_RetCode = 0;
	Local_idDelivery = IN_idDelivery;

	if (InitDeleteEmDelStatement (SQL_STRING, &SSStatement) 
    													  != EWDB_RETURN_SUCCESS)
	{
		logit ("", "PrepDeleteEmDelExec(): InitDeleteEmDelStatement failed!\n");
		return EWDB_RETURN_FAILURE;
	}

	*ppCursor = SSStatement.pCda;
	
	return EWDB_RETURN_SUCCESS;
}


static int PostDeleteEmDelExec()
{
	EWDB_Cursor pCursor;
  
	pCursor = SSStatement.pCda;

  ewdb_base_ReleaseCursor (pCursor);
  
	if (Local_RetCode != 0)
	{
		logit ("", "SQL call to DeleteEmailDelivery returned error: %d\n", Local_RetCode);
	  return(EWDB_RETURN_FAILURE);
	}

	return (EWDB_RETURN_SUCCESS);
}
