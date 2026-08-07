/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_api_CreateOrUpdateComment.c,v 1.2 2005/06/15 19:03:12 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_api_CreateOrUpdateComment.c,v $
 *     Revision 1.2  2005/06/15 19:03:12  davidk
 *     DB API Cleanup
 *
 *     Revision 1.1  2005/05/11 22:30:04  mark
 *     Initial checkin
 *
 *
 */


#include <ewdb_ora_api.h>
#include <ewdb_cli_base.h>
#include <ewdb_ew_oci_base.h>
#include <ewdb_oci_base_internal.h>


static char SQL_STRING[] =
  "Begin Create_Or_Update_Comment "
  "(OUT_idComment => :OUT_idComment,"
  "IN_idComment => :IN_idComment, "
  "IN_sComment => :IN_sComment); End;";

static EWDB_OCI_SFS CreateCommentBindArray[] = 
{
  {0,1,0,0,0,OA_INT,  ":OUT_idComment"},
  {0,1,0,0,0,OA_INT,  ":IN_idComment"},
  {0,1,4000,0,0,OA_SZ,":IN_sComment"},
};

#define	NUM_FIELDS		3

static  EWDB_OCIStatementStruct	SSStatement;

static  int	    	Local_OUTidComment;
static	int	    	Local_INidComment;
static  char	  	Local_szComment[4000];


static int PrepCreateCommentExec (EWDBid IN_idComment, char * szComment, EWDB_Cursor *);
static int PostCreateCommentExec (EWDBid *pidComment);
static int InitCreateCommentStatement (char *, EWDB_OCIStatementStruct *);


int ewdb_api_CreateOrUpdateComment(EWDBid *pidComment, char *szComment)
{
	EWDB_Cursor	pCursor;

	if (*pidComment <= 0 && strlen(szComment) == 0)
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
  	logit ("", "ewdb_api_CreateOrUpdateComment(): Could not reconnect to the database!\n");
  	return (EWDB_RETURN_FAILURE);
  }

	if (PrepCreateCommentExec (*pidComment, szComment, &pCursor) != EWDB_RETURN_SUCCESS)
  {
  	logit ("", "ewdb_api_CreateOrUpdateComment(): PrepCreateCommentExec failed.\n");
  	return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  }

	if (ewdb_base_SQLExecute(pCursor))
  {
  	ewdb_base_ErrorReport (hEWDBC, pCursor,"ewdb_api_CreateOrUpdateComment(): ewdb_base_SQLExecute", 1);
  	return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  } 
  
  /* Commit the transaction (all the previous inserts!)
     In Case there is any auditing or logging or debug
     changes made in the stored procedures.
  ****************************************************/
	if (ewdb_base_SQLCommit(hEWDBC))
  {
  	ewdb_base_ErrorReport (hEWDBC, hEWDBC, "ewdb_api_CreateOrUpdateComment(): ewdb_base_SQLCommit",2);
  	return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  }

	if (PostCreateCommentExec(pidComment) != EWDB_RETURN_SUCCESS)
  {
  	return(EWDB_RETURN_FAILURE);
  }

	if (*pidComment == 0)
  	return EWDB_RETURN_WARNING;

	return EWDB_RETURN_SUCCESS;
}  /* end ewdb_api_CreateOrUpdateComment() */


/******************* InitCreateCommentStatement *******************/
int InitCreateCommentStatement(char *statement, EWDB_OCIStatementStruct *pSS)
{
	pSS->FieldArray[0].pVal = &Local_OUTidComment;
	pSS->FieldArray[1].pVal = &Local_INidComment;
	pSS->FieldArray[2].pVal = Local_szComment;

  return(ewdb_base_RequestCursor(szStatement, pSS,0/*don't force rebind*/));
}  


/******************* PrepCreateCommentExec *******************/
static int PrepCreateCommentExec (EWDBid IN_idComment, char * szComment, EWDB_Cursor *)
{
	strcpy (Local_szComment, szComment);
	Local_INidComment = IN_idComment;
  Local_OUTidComment = 0;

	SSStatement.NumOfFields = NUM_FIELDS;
	SSStatement.FieldArray = CreateCommentBindArray;
	SSStatement.RecordSize = 0;

	if (InitCreateCommentStatement (SQL_STRING, &SSStatement)
                             != EWDB_RETURN_SUCCESS)
  {
  	logit ("", "PrepCreateCommentExec(): InitCreateCommentStatement failed!\n");
  	return EWDB_RETURN_FAILURE;
  }

  *ppCursor = SSStatement.pCda;
  
	return EWDB_RETURN_SUCCESS;
}  /* end PrepCreateCommentExec() */


/******************* PostCreateCommentExec *******************/
int PostCreateCommentExec(EWDBid *pidComment)
{
	EWDB_Cursor pCursor;
  
  *pidComment = Local_OUTidComment;

	pCursor = SSStatement.pCda;
	ewdb_base_ReleaseCursor(pCursor);

	if (*pidComment < 0)
  {
  	logit("", "PostCreateCommentExec(): SQL Proc Create_Or_Update_Comment() failed with error %d!\n"
          Local_OUT_idComment);
  	return EWDB_RETURN_WARNING;
  }

	return EWDB_RETURN_SUCCESS;
}  /* end PostCreateCommentExec() */
