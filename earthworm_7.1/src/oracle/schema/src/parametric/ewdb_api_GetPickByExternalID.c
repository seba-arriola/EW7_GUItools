/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */
/*    $Id: ewdb_api_GetPickByExternalID.c,v 1.2 2005/06/15 19:03:12 davidk Exp $                                                */
/*                                                          */
/*    Revision history:                                     */
/*
 *     $Log: ewdb_api_GetPickByExternalID.c,v $
 *     Revision 1.2  2005/06/15 19:03:12  davidk
 *     DB API Cleanup
 *
 *     Revision 1.1  2003/08/25 18:08:28  lucky
 *     Initial revision
 *
/*                                                          */

#include <ewdb_ora_api.h>
#include <ewdb_cli_base.h>
#include <ewdb_ew_oci_base.h>

static char SQL_STRING[] =
    "Begin Get_PickId_ByExtId (OUT_idPick => :OUT_idPick,"
    "IN_xidExternal => :IN_xidExternal); End;";


static EWDB_OCI_SFS SQLParamsBindArray[] = 
{
  {0,1,0,0,0,OA_EWDBID,  ":OUT_idPick"},
  {0,1,0,0,0,OA_SZ,      ":IN_xidExternal"}
};

#define	NUM_FIELDS	2

static	EWDBid Local_idPick;
static  char   Local_szExtPickID[64];
/* Insertion Struct for Get_idChan_From_Station_External statement */
static EWDB_OCIStatementStruct SSStatement;


/* Prototypes of functions defined in this file */
static int InitGetPickByExternalIDStatement(char *statement, EWDB_OCIStatementStruct *pSS);
static int PrepGetPickByExternalIDExec (char *IN_szExtPickID, EWDB_Cursor *ppCursor);
static int PostGetPickByExternalIDExec (EWDBid *pidPick);


int ewdb_api_GetPickByExternalID(char *IN_szExtPickID, EWDBid *pidPick)
{
  
	EWDB_Cursor pCursor;
	int rc;
  
	if ((IN_szExtPickID == NULL) || (pidPick == NULL))
	{
		logit("", "ewdb_api_GetPickByExternalID(): Invalid arguments passed in!\n");
		return EWDB_RETURN_FAILURE;
	}

  ewdb_base_SetLastOraAPIActionTime();
  
  if(ewdb_base_Reconnect() != EWDB_RETURN_SUCCESS)
  {
    logit("", "ewdb_api_GetPickByExternalID(): Could not reconnect to the database!\n");
    return(EWDB_RETURN_FAILURE);
  }
  
  if(PrepGetPickByExternalIDExec(IN_szExtPickID, &pCursor) != EWDB_RETURN_SUCCESS)
  {
    logit("", "ewdb_api_GetPickByExternalID(): PrepGetPickByExternalIDExec() failed.\n");
    return(ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }
  
  if(ewdb_base_SQLExecute(pCursor))
  {
    ewdb_base_ErrorReport(hEWDBC, pCursor, 
                          "ewdb_api_GetPickByExternalID(): ewdb_base_SQLExecute", 1);
    return(ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  } 
  
  /* Commit the transaction(all the previous inserts!)
  In Case there is any auditing or logging or debug
  changes made in the stored procedures.
  ****************************************************/
  if(ewdb_base_SQLCommit(hEWDBC))
  {
    ewdb_base_ErrorReport(hEWDBC, hEWDBC, "ewdb_api_GetPickByExternalID(): ewdb_base_SQLCommit",2);
    return(ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }
  
  rc = PostGetPickByExternalIDExec (pidPick);
  if(rc == EWDB_RETURN_FAILURE)
  {
    logit("", "ewdb_api_GetPickByExternalID(): PostGetPickByExternalIDExec failed!\n");
    return(ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }
  else if(rc == EWDB_RETURN_WARNING)
  {
    return(EWDB_RETURN_FAILURE);
  }
  
  return(EWDB_RETURN_SUCCESS);
}  /* end ewdb_api_GetIdChanFromStationExternal() */


static int InitGetPickByExternalIDStatement(char * szStatement, EWDB_OCIStatementStruct *pSS)
{
  SSStatement.FieldArray[0].pVal = &Local_idPick;
  SSStatement.FieldArray[1].pVal = Local_szExtPickID;

  return(ewdb_base_RequestCursor(szStatement, pSS,0));
}


static int PrepGetPickByExternalIDExec (char *IN_szExtPickID, EWDB_Cursor *ppCursor)
{
  
  SSStatement.NumOfFields = NUM_FIELDS;
  SSStatement.FieldArray = SQLParamsBindArray;
  SSStatement.RecordSize = 0;
  
  strncpy(Local_szExtPickID, IN_szExtPickID, sizeof(Local_szExtPickID));
  Local_szExtPickID[sizeof(Local_szExtPickID)-1] = 0x00;

  if(InitGetPickByExternalIDStatement(SQL_STRING, &SSStatement) != EWDB_RETURN_SUCCESS)
  {
    *ppCursor = NULL;
    return(EWDB_RETURN_FAILURE);
  }

  *ppCursor=SSStatement.pCda;

  return(EWDB_RETURN_SUCCESS);
}  /* end PrepGetPickByExternalIDExec() */


int PostGetPickByExternalIDExec (EWDBid *pidPick)
{
  EWDB_Cursor pCursor;
  
  *pidPick = Local_idPick;
  
  pCursor = SSStatement.pCda;
  ewdb_base_ReleaseCursor(pCursor);
  
  if(Local_idPick <= 0)
  {
    logit("","%s: ERROR!  SQL Proc Get_PickId_ByExtId(%s) returned error(%d)!\n", 
          "PostGetPickByExternalIDExec()", Local_idPick);
    return(EWDB_RETURN_WARNING);
  }

  return(EWDB_RETURN_SUCCESS);
}  /* end PostGetPickByExternalIDExec() */
