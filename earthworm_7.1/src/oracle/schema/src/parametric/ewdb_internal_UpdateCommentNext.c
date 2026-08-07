/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_internal_UpdateCommentNext.c,v 1.2 2005/06/15 19:03:13 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_internal_UpdateCommentNext.c,v $
 *     Revision 1.2  2005/06/15 19:03:13  davidk
 *     DB API Cleanup
 *
 *     Revision 1.1  2005/05/12 20:34:02  mark
 *     Initial checkin
 *
 *
 */


#include <ewdb_ora_api.h>
#include <ewdb_cli_base.h>
#include <ewdb_ew_oci_base.h>
#include <ewdb_oci_base_internal.h>

static char SQL_STRING[] =
  "Begin Update_Comment_Next "
  "(OUT_iErrCode => :OUT_iErrCode,"
  "IN_idComment => :IN_idComment,"
  "IN_idNextComment => :IN_idNextComment); End;";

static EWDB_OCI_SFS SQLParamsBindArray[] = 
{
  {0,1,0,0,0,OA_INT,  ":OUT_iErrCode"},
  {0,1,0,0,0,OA_INT,  ":IN_idComment"},
  {0,1,0,0,0,OA_INT,  ":IN_idNextComment"},
};

#define  NUM_FIELDS    3


static  EWDB_OCIStatementStruct    SSStatement;

static  EWDBid        Local_idComment, Local_idNextComment;
static  int           Local_iRetCode;

static int PrepUpdateCommentNextExec(EWDBid IN_idComment, EWDBid IN_idNextComment, EWDB_Cursor *ppCursor);
static int PostUpdateCommentNextExec();
static int InitUpdateCommentNextStatement (char *, EWDB_OCIStatementStruct *);


int ewdb_internal_UpdateCommentNext(EWDBid IN_idComment, EWDBid IN_idNextComment)
{
  EWDB_Cursor  pCursor;

  ewdb_base_SetLastOraAPIActionTime ();

  if (ewdb_base_Reconnect () != EWDB_RETURN_SUCCESS)
  {
    logit ("", "ewdb_internal_UpdateCommentNext(): Could not reconnect to the database!\n");
    return (EWDB_RETURN_FAILURE);
  }

  if (PrepUpdateCommentNextExec (IN_idComment, IN_idNextComment, &pCursor) != EWDB_RETURN_SUCCESS)
  {
    logit ("", "ewdb_internal_UpdateCommentNext(): PrepUpdateCommentNextExec failed.\n");
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  }

  if (ewdb_base_SQLExecute(pCursor))
  {
    ewdb_base_ErrorReport (hEWDBC, pCursor,"ewdb_internal_UpdateCommentNext(): ewdb_base_SQLExecute", 1);
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  } 

 /* Commit the transaction (all the previous inserts!)
     In Case there is any auditing or logging or debug
     changes made in the stored procedures.
  ****************************************************/
  if (ewdb_base_SQLCommit(hEWDBC))
  {
    ewdb_base_ErrorReport (hEWDBC, hEWDBC, "ewdb_internal_UpdateCommentNext(): :ewdb_base_SQLCommit",2);
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  }

  if (PostUpdateCommentNextExec() != EWDB_RETURN_SUCCESS)
  {
    logit ("", "ewdb_internal_UpdateCommentNext(): PostUpdateCommentNextExec failed!\n");
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  }

  return EWDB_RETURN_SUCCESS;
}  /* end ewdb_internal_UpdateCommentNext() */


/******************* InitUpdateCommentNextStatement *******************/
static int InitUpdateCommentNextStatement(char *szStatement, EWDB_OCIStatementStruct *pSS)
{

  pSS->FieldArray[0].pVal = &Local_iRetCode;
  pSS->FieldArray[1].pVal = &Local_idComment;
  pSS->FieldArray[2].pVal = &Local_idNextComment;

  return(ewdb_base_RequestCursor (szStatement, pSS, 0));
}  


/******************* PrepUpdateCommentNextExec *******************/
static int PrepUpdateCommentNextExec(EWDBid IN_idComment, EWDBid IN_idNextComment, EWDB_Cursor *ppCursor)
{
  if (ppCursor == NULL)
  {
    logit ("", "PrepUpdateCommentNextExec(): Null parameters passed in!\n");
    return EWDB_RETURN_FAILURE;
  }

  Local_idComment = IN_idComment;
  Local_idNextComment = IN_idNextComment;

  SSStatement.NumOfFields = NUM_FIELDS;
  SSStatement.FieldArray = SQLParamsBindArray;
  SSStatement.RecordSize = 0;

  if (InitUpdateCommentNextStatement (SQL_STRING, &SSStatement)
                             != EWDB_RETURN_SUCCESS)
  {
    logit ("", "PrepUpdateCommentNextExec(): InitUpdateCommentNextStatement failed!\n");
    return EWDB_RETURN_FAILURE;
  }

  *ppCursor = SSStatement.pCda;
  
  return EWDB_RETURN_SUCCESS;
}  /* end PrepUpdateCommentNextExec() */


/******************* PostUpdateCommentNextExec *******************/
static int PostUpdateCommentNextExec()
{
  EWDB_Cursor pCursor;
  
  pCursor = SSStatement.pCda;
  ewdb_base_ReleaseCursor(pCursor);

  if (Local_iRetCode == -2)
  {
    logit("", "PostUpdateCommentNextExec(): SQL Proc Update_Comment_Next(%d/%d): Can't find comment ID %d in DB\n", 
          Local_idComment, Local_idNextComment);
    return EWDB_RETURN_FAILURE;
  }
  else if (Local_iRetCode != 0)
  {
    logit("", "PostUpdateCommentNextExec(): SQL Proc  to Update_Comment_Next(%d/%d) failed(%d); see debug table\n",
          Local_idComment, Local_idNextComment, Local_iRetCode);
    return EWDB_RETURN_FAILURE;
  }

  return EWDB_RETURN_SUCCESS;
} 
