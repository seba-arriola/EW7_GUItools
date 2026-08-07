/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_api_DeleteChanT.c,v 1.4 2005/06/10 16:27:58 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_api_DeleteChanT.c,v $
 *     Revision 1.4  2005/06/10 16:27:58  davidk
 *     DB API Cleanup
 *
 *     Revision 1.3  2003/12/04 19:33:34  davidk
 *     Improved handling of SQL warnings and errors, passing more information
 *     back to the caller.
 *
 *     Revision 1.2  2003/12/03 00:44:55  davidk
 *     Fixed a C++ style comment //
 *
 *     Revision 1.1  2003/12/03 00:23:29  davidk
 *     Initial revision
 *
 */


#include <ewdb_ora_api.h>
#include <ewdb_cli_base.h>
#include <ewdb_ew_oci_base.h>


static char SQL_STRING[] = 
     "  Begin Delete_ChanT(OUT_iRetCode => :OUT_iRetCode, IN_idChanT => :IN_idChanT); End;";

static EWDB_OCI_SFS SQLParamsBindArray[] = 
{
  {0,1,0,0,0,OA_INT,":OUT_iRetCode"},
  {0,1,0,0,0,OA_INT,":IN_idChanT"}
};

#define  NUM_FIELDS 2

/* static variables */
static EWDBid Local_idChanT;
static int    Local_iRetCode;

/* Statement Struct for DeleteChanT szStatement */
static EWDB_OCIStatementStruct SSStatement;

/********************************
      FUNCTION PROTOTYPES
********************************/
static int PrepDeleteChanTExec(EWDBid IN_idChanT,
                        EWDB_Cursor * ppCursor);
static int PostDeleteChanTExec();
static int InitDeleteChanTStatement(char *szStatement, 
                            EWDB_OCIStatementStruct *pSS);


/* Describe Function ewdb_api_DeleteChanT
*********************************************************************/
int ewdb_api_DeleteChanT(EWDBid IN_idChanT)
{

  EWDB_Cursor pCursor;
  int rc;

  ewdb_base_SetLastOraAPIActionTime();

  if(ewdb_base_Reconnect() != EWDB_RETURN_SUCCESS)
  {
    logit("","ewdb_api_DeleteChanT(): Could not reconnect to the database!\n");
    return(EWDB_RETURN_FAILURE);
  }

  if(PrepDeleteChanTExec(IN_idChanT, &pCursor)
      != EWDB_RETURN_SUCCESS)
  {
    logit("","ewdb_api_DeleteChanT():PrepDeleteChanTExec() failed.\n");
    return(EWDB_RETURN_FAILURE);
  }

  if(ewdb_base_SQLExecute(pCursor))
  {
    ewdb_base_ErrorReport(hEWDBC, pCursor,"ewdb_api_DeleteChanT():ewdb_base_SQLExecute", 1);
    return(ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  } 

  /* Commit the transaction(all the previous inserts!)
  ****************************************************/
  if(ewdb_base_SQLCommit(hEWDBC))
  {
    ewdb_base_ErrorReport(hEWDBC, hEWDBC,"ewdb_api_DeleteChanT():ewdb_base_SQLCommit",2);
    return(ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }

  rc = PostDeleteChanTExec();

  ewdb_base_SetLastOraAPIActionTime();

  if(rc == EWDB_RETURN_WARNING)
  {
    return(EWDB_RETURN_FAILURE);
  }
  else if(rc == EWDB_RETURN_FAILURE)
  {
    logit("", "ewdb_api_DeleteChanT(): "
           "ERROR:  PostDeleteChanTExec failed!!\n");
    return(ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }

  return(EWDB_RETURN_SUCCESS);
}  /* end ewdb_api_DeleteChanT() */


static int InitDeleteChanTStatement(char *szStatement, EWDB_OCIStatementStruct *pSS)
{
  pSS->FieldArray[0].pVal = &Local_iRetCode;
  pSS->FieldArray[1].pVal = &Local_idChanT;

  return(ewdb_base_RequestCursor(szStatement, pSS, 0));
}  /* end InitDeleteChanTStatement() */


static int PrepDeleteChanTExec(EWDBid IN_idChanT, EWDB_Cursor * ppCursor)
{

  SSStatement.NumOfFields = NUM_FIELDS;
  SSStatement.FieldArray = SQLParamsBindArray;
  SSStatement.RecordSize = 0;

  /* Copy user's vars to local vars here */
  Local_idChanT = IN_idChanT;

  if(InitDeleteChanTStatement(SQL_STRING,
                           &SSStatement) != EWDB_RETURN_SUCCESS)
  {
    logit("", "PrepDeleteChanTExec(): InitDeleteChanTStatement failed!\n");
    return(EWDB_RETURN_FAILURE);
  }

  *ppCursor = SSStatement.pCda;
  
  return(EWDB_RETURN_SUCCESS);

}  /* end PrepDeleteChanTExec() */


static int PostDeleteChanTExec(EWDBid * pid)
{
  EWDB_Cursor pCursor;
  
  pCursor = SSStatement.pCda;
  
  ewdb_base_ReleaseCursor(pCursor);

  if(Local_iRetCode > 0)
  {
    logit("", "ERROR:  PostDeleteChanTExec() reports SQL Proc Delete_ChanT(%d) returned warning(%d)!\n",
          Local_idChanT, Local_iRetCode);
    return(EWDB_RETURN_WARNING);
  }
  else if(Local_iRetCode < 0)
  {
    logit("", "ERROR:  PostDeleteChanTExec() reports SQL Proc Delete_ChanT(%d) returned error(%d)!\n",
          Local_idChanT, Local_iRetCode);
    return(EWDB_RETURN_WARNING);
  }
  else
  {
    return(EWDB_RETURN_SUCCESS);
  }
}  /* end PostDeleteChanTExec() */


