/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_internal_DeleteComment.c,v 1.2 2005/06/15 19:03:12 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_internal_DeleteComment.c,v $
 *     Revision 1.2  2005/06/15 19:03:12  davidk
 *     DB API Cleanup
 *
 *     Revision 1.1  2005/05/12 20:33:40  mark
 *     Initial checkin
 *
 *
 */


#include <ewdb_ora_api.h>
#include <ewdb_cli_base.h>
#include <ewdb_ew_oci_base.h>
#include <ewdb_oci_base_internal.h>


static char SQL_STRING[] =
	"Begin DeleteComment "
	"(OUT_iRetCode => :OUT_iRetCode,"
	"IN_idComment => :IN_idComment); End;";

static EWDB_OCI_SFS SQLParamsBindArray[] = 
{
  {0,1,0,0,0,OA_INT,	":OUT_iError"},
  {0,1,0,0,0,OA_INT,	":IN_idComment"},
};

#define	NUM_FIELDS		2


static EWDB_OCIStatementStruct	SSStatement;
static  int				Local_idComment;
static	int				Local_iRetCode;


static int PrepDeleteCommentExec(EWDBid IN_idComment, EWDB_Cursor *ppCursor);
static int PostDeleteCommentExec ();
static int InitDeleteCommentStatement (char *, EWDB_OCIStatementStruct *);


int ewdb_internal_DeleteComment(EWDBid IN_idComment)
{
	EWDB_Cursor	pCursor;

	ewdb_base_SetLastOraAPIActionTime ();

	if (ewdb_base_Reconnect () != EWDB_RETURN_SUCCESS)
	{
		logit ("", "ewdb_internal_DeleteComment(): Could not reconnect to the database!\n");
		return (EWDB_RETURN_FAILURE);
	}


	if (PrepDeleteCommentExec (IN_idComment, &pCursor) != EWDB_RETURN_SUCCESS)
	{
		logit ("", "ewdb_internal_DeleteComment(): PrepDeleteCommentExec failed.\n");
		return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
	}

	if (ewdb_base_SQLExecute(pCursor))
	{
		ewdb_base_ErrorReport (hEWDBC, pCursor,"ewdb_internal_DeleteComment(): ewdb_base_SQLExecute", 1);
		return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
	} 

  /* Commit the transaction (all the previous inserts!)
	   In Case there is any auditing or logging or debug
	   changes made in the stored procedures.
	****************************************************/
	if (ewdb_base_SQLCommit(hEWDBC))
	{
		ewdb_base_ErrorReport (hEWDBC, hEWDBC, "ewdb_internal_DeleteComment(): ewdb_base_SQLCommit",2);
		return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
	}

	if (PostDeleteCommentExec() != EWDB_RETURN_SUCCESS)
	{
		logit ("", "Call to PostDeleteCommentExec failed!\n");
		return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
	}

	return EWDB_RETURN_SUCCESS;
}  /* end ewdb_internal_DeleteComment() */


/******************* InitDeleteCommentStatement *******************/
static int InitDeleteCommentStatement(char *statement, EWDB_OCIStatementStruct *pSS)
{

	pSS->FieldArray[0].pVal = &Local_iRetCode;
	pSS->FieldArray[1].pVal = &Local_idComment;

	if (ewdb_base_RequestCursor (statement, pSS, 0) != 0)
	{
		logit ("", "InitDeleteCommentStatement(): ewdb_base_RequestCursor failed.\n");
		return EWDB_RETURN_FAILURE;
	}

	return EWDB_RETURN_SUCCESS;
}  


/******************* PrepDeleteCommentExec *******************/
static int PrepDeleteCommentExec(EWDBid IN_idComment, EWDB_Cursor *ppCursor)
{
  Local_idComment = IN_idComment;

	SSStatement.NumOfFields = NUM_FIELDS;
	SSStatement.FieldArray = SQLParamsBindArray;
	SSStatement.RecordSize = 0;

	if (InitDeleteCommentStatement (SQL_STRING, &SSStatement)
												     != EWDB_RETURN_SUCCESS)
	{
		logit ("", "PrepDeleteCommentExec(): InitDeleteCommentStatement failed!\n");
		return EWDB_RETURN_FAILURE;
	}

	*ppCursor = SSStatement.pCda;
	
	return EWDB_RETURN_SUCCESS;
}  


/******************* PostDeleteCommentExec *******************/
static int PostDeleteCommentExec(EWDBid *pidComment)
{
	EWDB_Cursor pCursor;
  
	pCursor = SSStatement.pCda;
	ewdb_base_ReleaseCursor(pCursor);

	if (Local_iRetCode != 0)
	{
		logit("", "PostDeleteCommentExec():  SQL PROC DeleteComment(%d) failed(%d)\n", 
          Local_idComment, Local_iRetCode);
		return EWDB_RETURN_FAILURE;
	}

	return EWDB_RETURN_SUCCESS;
} 
