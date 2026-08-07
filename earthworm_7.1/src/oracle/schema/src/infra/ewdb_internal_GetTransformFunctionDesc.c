
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_internal_GetTransformFunctionDesc.c,v 1.2 2005/06/10 16:27:59 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_internal_GetTransformFunctionDesc.c,v $
 *     Revision 1.2  2005/06/10 16:27:59  davidk
 *     DB API Cleanup
 *
 *     Revision 1.1  2001/07/23 17:10:39  davidk
 *     Initial revision
 *
 *     Revision 1.2  2001/05/15 02:16:31  davidk
 *     Moved functions around between the apps, DB API, and DB API INTERNAL
 *     levels.  Renamed functions and files.  Added support for amplitude
 *     magnitude types.  Reformatted makefiles.
 *
 *     Revision 1.1  2001/02/28 17:19:22  lucky
 *     Initial revision
 *
 *
 */


#include <ewdb_ora_api.h>
#include <ewdb_cli_base.h>
#include <ewdb_ew_oci_base.h>

static char SQL_STRING[] =
  "Begin "
  "  select sFuncDesc into :OUT_sFuncDesc "
  "   from ALL_CTF "
  "   where idCTF = :IN_idCookedTF; "
  "  :OUT_iRetCode := 0; "
  "EXCEPTION   "
  "  WHEN OTHERS THEN "
  "   :OUT_iRetCode := -1; "
  "End;";


static EWDB_OCI_SFS SQLParamsBindArray[] = 
{
  {0,1,0,0,0,OA_INT,":OUT_iRetCode"},
  {0,1,0,0,0,OA_SZ,":OUT_sFuncDesc"},
  {0,1,0,0,0,OA_EWDBID,":IN_idCookedTF"}
};

#define  NUM_FIELDS 3

static  EWDBid    Local_idCookedTF;
static  char      Local_szFuncDesc[100];
static  int       Local_iRetCode;

/* Statement Struct for GetTransFuncDesc statement */
static EWDB_OCIStatementStruct SSStatement;


/********************************
      FUNCTION PROTOTYPES
********************************/
static int PrepGetTransFuncDescExec(EWDBid IN_idCookedTF, EWDB_Cursor * ppCursor);
static int PostGetTransFuncDescExec(char * OUT_szFuncDesc);
static int InitGetTransFuncDescStatement(char *statement, 
                                  EWDB_OCIStatementStruct *pSS);



/* Retrieves the description for a Transform function from the DB, 
   using the CookedTF record identifier(Local_idCookedTF).
*********************************************************************/
int ewdb_internal_GetTransformFunctionDesc(EWDBid Local_idCookedTF, char * szCookedTFDesc)
{

  EWDB_Cursor pCursor;

  if(szCookedTFDesc== NULL)
  {
    logit("", "ewdb_internal_GetTransformFunctionDesc():Null pointer passed in!\n");
    return EWDB_RETURN_FAILURE;
  }

  ewdb_base_SetLastOraAPIActionTime();

  if(ewdb_base_Reconnect() != EWDB_RETURN_SUCCESS)
  {
    logit("", "EWDB_GetTransformFunctionDesc(): "
          "Could not reconnect to the database!\n");
    return(EWDB_RETURN_FAILURE);
  }

  if(PrepGetTransFuncDescExec(Local_idCookedTF, &pCursor) != EWDB_RETURN_SUCCESS)
  {
    logit("", "ewdb_internal_GetTransformFunctionDesc():PrepGetTransFuncDescExec() failed.\n");
    return(ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }

  if(ewdb_base_SQLExecute(pCursor))
  {
    ewdb_base_ErrorReport(hEWDBC, pCursor,"ewdb_internal_GetTransformFunctionDesc():ewdb_base_SQLExecute", 1);
    return(ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  } 
  
  /* Commit the transaction(all the previous inserts!)
     In Case there is any auditing or logging or debug
     changes made in the stored procedures.
  ****************************************************/
  if(ewdb_base_SQLCommit(hEWDBC))
  {
    ewdb_base_ErrorReport(hEWDBC, hEWDBC,"ewdb_internal_GetTransformFunctionDesc():ewdb_base_SQLCommit",2);
    return(ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }

  if(PostGetTransFuncDescExec(szCookedTFDesc) 
      != EWDB_RETURN_SUCCESS)
  {
    logit("", "ewdb_internal_GetTransformFunctionDesc(): PostGetTransFuncDescExec failed!\n");
    return(EWDB_RETURN_FAILURE);
  }

  ewdb_base_SetLastOraAPIActionTime();

  return(EWDB_RETURN_SUCCESS);
}  /* end ewdb_internal_GetTransformFunctionDesc() */


static int InitGetTransFuncDescStatement(char *statement, 
                                     EWDB_OCIStatementStruct *pSS)
{
  pSS->FieldArray[0].pVal = &Local_iRetCode;
  pSS->FieldArray[1].pVal = Local_szFuncDesc;
  pSS->FieldArray[2].pVal = &Local_idCookedTF;


  return(ewdb_base_RequestCursor(statement, pSS, 0));
}  /* End InitGetTransFuncDescStatement() */


static int PrepGetTransFuncDescExec(EWDBid IN_idCookedTF, EWDB_Cursor * ppCursor) 
{


  SSStatement.NumOfFields = NUM_FIELDS;
  SSStatement.FieldArray = SQLParamsBindArray;
  SSStatement.RecordSize = 0;

  Local_idCookedTF=IN_idCookedTF;
  memset(Local_szFuncDesc, 0, sizeof(Local_szFuncDesc));

  if(InitGetTransFuncDescStatement(SQL_STRING,
                           &SSStatement) != EWDB_RETURN_SUCCESS)
  {
    logit("", "Call to InitGetTransFuncDescStatement failed!\n");
    return EWDB_RETURN_FAILURE;
  }

  *ppCursor = SSStatement.pCda;
  
  return(EWDB_RETURN_SUCCESS);

}  /* End PrepGetTransFuncDescExec() */


static int PostGetTransFuncDescExec(char * OUT_szFuncDesc)
{
  EWDB_Cursor pCursor;
  
  pCursor = SSStatement.pCda;

  strcpy(OUT_szFuncDesc,Local_szFuncDesc);

  ewdb_base_ReleaseCursor(pCursor);

  if(Local_iRetCode != 0)
    return(EWDB_RETURN_WARNING);

  return(EWDB_RETURN_SUCCESS);
}  /* End PostGetTransFuncDescExec() */
