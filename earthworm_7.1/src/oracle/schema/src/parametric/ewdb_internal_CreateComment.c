/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_internal_CreateComment.c,v 1.3 2005/06/15 19:03:12 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_internal_CreateComment.c,v $
 *     Revision 1.3  2005/06/15 19:03:12  davidk
 *     DB API Cleanup
 *
 *     Revision 1.2  2005/05/16 17:05:29  mark
 *     Fixed SQL call
 *
 *     Revision 1.1  2005/05/12 20:33:14  mark
 *     Initial checkin
 *
 *
 */


#include <ewdb_ora_api.h>
#include <ewdb_cli_base.h>
#include <ewdb_ew_oci_base.h>
#include <ewdb_oci_base_internal.h>


static char SQL_STRING[] =
  "Begin Create_Comment "
  "(OUT_idComment => :OUT_idComment,"
  "IN_sComment => :IN_sComment); End;";

static EWDB_OCI_SFS SQLParamsBindArray[] = 
{
  {0,1,0,0,0,OA_INT,  ":OUT_idComment"},
  {0,1,4000,0,0,OA_SZ,  ":IN_sComment"},
};

#define  NUM_FIELDS    2


static EWDB_OCIStatementStruct  SSStatement;
static  int        Local_idComment;
static  char      Local_szComment[4000];


static int PrepCreateCommentExec(char * IN_szComment, EWDB_Cursor *ppCursor);
static int PostCreateCommentExec (EWDBid *pidComment);
static int InitCreateCommentStatement (char *, EWDB_OCIStatementStruct *);


int ewdb_internal_CreateComment(EWDBid *pidComment, char *IN_szComment)
{
  EWDB_Cursor  pCursor;

  if (IN_szComment == NULL)
  {
    logit ("", "ewdb_internal_CreateComment(): Null parameters passed in!\n");
    return EWDB_RETURN_FAILURE;
  }
  if (strlen(IN_szComment) == 0)
  {
    /* We're creating a comment, but there's nothing to create...
     * just return warning.
     */
    *pidComment = 0;
    return EWDB_RETURN_WARNING;
  }

  ewdb_base_SetLastOraAPIActionTime ();

  if (ewdb_base_Reconnect () != EWDB_RETURN_SUCCESS)
  {
    logit ("", "ewdb_internal_CreateComment(): Could not reconnect to the database!\n");
    return (EWDB_RETURN_FAILURE);
  }

  if (PrepCreateCommentExec (IN_szComment, &pCursor) != EWDB_RETURN_SUCCESS)
  {
    logit ("", "ewdb_internal_CreateComment(): PrepCreateCommentExec failed.\n");
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  }

  if (ewdb_base_SQLExecute(pCursor))
  {
    ewdb_base_ErrorReport (hEWDBC, pCursor,"ewdb_internal_CreateComment(): ewdb_base_SQLExecute", 1);
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  } 
  
  /* Commit the transaction (all the previous inserts!)
     In Case there is any auditing or logging or debug
     changes made in the stored procedures.
  ****************************************************/
  if (ewdb_base_SQLCommit(hEWDBC))
  {
    ewdb_base_ErrorReport (hEWDBC, hEWDBC, "ewdb_internal_CreateComment(): ewdb_base_SQLCommit",2);
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  }

  if (PostCreateCommentExec(pidComment) != EWDB_RETURN_SUCCESS)
  {
    logit ("", "ewdb_internal_CreateComment(): PostCreateCommentExec failed!\n");
    return (EWDB_RETURN_FAILURE);
  }

  return EWDB_RETURN_SUCCESS;
}  /* end ewdb_internal_CreateComment() */


/******************* InitCreateCommentStatement *******************/
static int InitCreateCommentStatement(char *szStatement, EWDB_OCIStatementStruct *pSS)
{

  pSS->FieldArray[0].pVal = &Local_idComment;
  pSS->FieldArray[1].pVal = Local_szComment;

  if (ewdb_base_RequestCursor (szStatement, pSS, 0) != 0)
  {
    logit ("", "InitCreateCommentStatement(): ewdb_base_RequestCursor failed.\n");
    return EWDB_RETURN_FAILURE;
  }

  return EWDB_RETURN_SUCCESS;
}  /* end InitCreateCommentStatement() */


/******************* PrepCreateCommentExec *******************/
static int PrepCreateCommentExec(char * IN_szComment, EWDB_Cursor *ppCursor)
{

  SSStatement.NumOfFields = NUM_FIELDS;
  SSStatement.FieldArray = SQLParamsBindArray;
  SSStatement.RecordSize = 0;

  strcpy (Local_szComment, IN_szComment);

  if (InitCreateCommentStatement(SQL_STRING, &SSStatement)
      != EWDB_RETURN_SUCCESS)
  {
    logit ("", "PrepCreateCommentExec(): InitCreateCommentStatement failed!\n");
    return EWDB_RETURN_FAILURE;
  }

  *ppCursor = SSStatement.pCda;
  
  return EWDB_RETURN_SUCCESS;
}  


/******************* PostCreateCommentExec *******************/
static int PostCreateCommentExec(EWDBid *pidComment)
{
  EWDB_Cursor pCursor;
  
  *pidComment = Local_idComment;

  pCursor = SSStatement.pCda;
  ewdb_base_ReleaseCursor(pCursor);

  if (*pidComment < 0)
  {
    logit("", "PostCreateCommentExec(): SQL Proc Create_Comment()  failed with error(%d).\n",
          Local_idComment);
    return EWDB_RETURN_WARNING;
  }

  return EWDB_RETURN_SUCCESS;
} 
