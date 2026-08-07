/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_api_SetPrefer.c,v 1.2 2005/06/15 19:03:12 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_api_SetPrefer.c,v $
 *     Revision 1.2  2005/06/15 19:03:12  davidk
 *     DB API Cleanup
 *
 *     Revision 1.1  2002/06/18 16:10:24  lucky
 *     Initial revision
 *
 *
 *
 */


#include <ewdb_ora_api.h>
#include <ewdb_cli_base.h>
#include <ewdb_ew_oci_base.h>

static char SQL_STRING[] =
  "Begin Set_Prefer (OUT_idPrefer => :OUT_idPrefer,"
  "IN_idEvent => :IN_idEvent,"
  "IN_PreferType => :IN_PreferType,"
  "IN_idPrefered => :IN_idPrefered); End;";
  
static EWDB_OCI_SFS SQLParamsBindArray[] = 
{
  {0,1,0,0,0,OA_INT, ":OUT_idPrefer"},
  {0,1,0,0,0,OA_INT, ":IN_idEvent"},
  {0,1,0,0,0,OA_INT, ":IN_PreferType"},
  {0,1,0,0,0,OA_INT, ":IN_idPrefered"}
};

#define  NUM_FIELDS  4

static EWDB_OCIStatementStruct SSStatement;

static  EWDBid  Local_idPrefer;
static  EWDBid  Local_idEvent;
static  EWDBid  Local_idPreferred;
static  int     Local_PreferType;

static int PrepSetPreferExec(EWDBid IN_idEvent, int IN_tiCoreTable, 
                             EWDBid IN_idCore, EWDB_Cursor *ppCursor);
static int PostSetPreferExec(EWDBid * pidPrefer);
static int InitSetPreferStatement(char *szStatement, EWDB_OCIStatementStruct *pSS);


int ewdb_api_SetPrefer(EWDBid IN_idEvent, int IN_tiCoreTable, EWDBid IN_idCore, 
                       EWDBid *pidPrefer) 
{

  EWDB_Cursor pCursor;
  int rc;

  if ((IN_idEvent < 0) || (IN_idCore <= 0 || pidPrefer == NULL)) 
  {
    logit ("", "ewdb_api_SetPrefer(): Null parameters passed in!\n");
    return EWDB_RETURN_FAILURE;
  }

  ewdb_base_SetLastOraAPIActionTime ();

  if (ewdb_base_Reconnect () != EWDB_RETURN_SUCCESS)
  {
    logit ("", "ewdb_api_SetPrefer(): Could not reconnect to the database!\n");
    return (EWDB_RETURN_FAILURE);
  }

  if (PrepSetPreferExec (IN_idEvent, IN_tiCoreTable, IN_idCore, &pCursor) != EWDB_RETURN_SUCCESS)
  {
    logit ("", "ewdb_api_SetPrefer(): PrepSetPreferExec() failed.\n");
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  }

  if (ewdb_base_SQLExecute (pCursor))
  {
    ewdb_base_ErrorReport (hEWDBC, pCursor,"ewdb_api_SetPrefer(): ewdb_base_SQLExecute", 1);
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  } 

  /* Commit the transaction (all the previous inserts!)
     In Case there is any auditing or logging or debug
     changes made in the stored procedures.
  ****************************************************/
  if (ewdb_base_SQLCommit (hEWDBC))
  {
    ewdb_base_ErrorReport (hEWDBC, hEWDBC,"ewdb_api_SetPrefer(): ewdb_base_SQLCommit",2);
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  }

  rc = PostSetPreferExec(pidPrefer);
  if(rc == EWDB_RETURN_FAILURE)
  {
    logit("", "ewdb_api_SetPrefer(): PostSetPreferExec failed!\n");
    return(ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }

  return(rc);
}  /* end ewdb_api_SetPrefer() */


static int InitSetPreferStatement (char *szStatement, EWDB_OCIStatementStruct *pSS)
{

  SSStatement.FieldArray[0].pVal = &(Local_idPrefer);
  SSStatement.FieldArray[1].pVal = &(Local_idEvent);
  SSStatement.FieldArray[2].pVal = &(Local_PreferType);
  SSStatement.FieldArray[3].pVal = &(Local_idPrefer);

  if (ewdb_base_RequestCursor (szStatement, pSS, 0) != 0)
  {
    logit ("", "InitSetPreferStatement(): ewdb_base_RequestCursor failed.\n");
    return(EWDB_RETURN_FAILURE);
  }

	return(EWDB_RETURN_SUCCESS);
}


static int PrepSetPreferExec(EWDBid IN_idEvent, int IN_tiCoreTable, 
                             EWDBid IN_idCore, EWDB_Cursor *ppCursor)
{

	SSStatement.NumOfFields = NUM_FIELDS;
	SSStatement.FieldArray = SQLParamsBindArray;
	SSStatement.RecordSize = 0;

  Local_idEvent     = IN_idEvent;
  Local_idPreferred = IN_idCore;

  if (IN_tiCoreTable == CORE_TABLE_ORIGIN)
    Local_PreferType = 1;
  else if (IN_tiCoreTable == CORE_TABLE_MECHFM)
    Local_PreferType = 2;
  else if (IN_tiCoreTable == CORE_TABLE_MAGNITUDE)
    Local_PreferType = 3;
  else
  {
    logit ("", "PrepSetPreferExec(): Invalid IN_tiCoreTable designation <%d>\n", IN_tiCoreTable);
    return (EWDB_RETURN_FAILURE);
  }

	if (InitSetPreferStatement (SQL_STRING, &SSStatement)
      != EWDB_RETURN_SUCCESS)
	{
		logit ("", "PrepSetPreferExec(): InitSetPreferStatement failed!\n");
		return EWDB_RETURN_FAILURE;
	}

	*ppCursor = SSStatement.pCda;
	
	return(EWDB_RETURN_SUCCESS);
}  /* end PrepSetPreferExec() */


static int PostSetPreferExec(EWDBid * pidPrefer)
{
  EWDB_Cursor pCursor;
  
  pCursor = SSStatement.pCda;
  
  ewdb_base_ReleaseCursor(pCursor);

  if (pidPrefer == NULL)
  {
    logit ("", "PostSetPreferExec(): Invalid arguments passed in.\n");
    return EWDB_RETURN_FAILURE;
  }
  
  *pidPrefer = Local_idPrefer;

  if(Local_idPrefer <= 0)
  {
    if(Local_idPrefer == -1)
      logit("","PostSetPreferExec():  SQL Proc Create_PeakAmp() returned "
                "\"Unknown error\".  See SQL debug table for details.\n");
    else
      logit("","PostSetPreferExec():  SQL Proc Create_PeakAmp() returned "
               "the following error(%d).  Please see that proc for details.\n",
            Local_idPrefer);
    return(EWDB_RETURN_WARNING);
  }

	return(EWDB_RETURN_SUCCESS);

}  /* end PostSetPreferExec() */
