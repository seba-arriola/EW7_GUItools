
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_api_UpdateComment.c,v 1.3 2005/06/15 19:03:12 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_api_UpdateComment.c,v $
 *     Revision 1.3  2005/06/15 19:03:12  davidk
 *     DB API Cleanup
 *
 *     Revision 1.2  2001/05/15 02:16:22  davidk
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
  "Begin Update_Comment(OUT_RetCode => :OUT_RetCode,"
  " IN_idCore => :IN_idCore, IN_sCoreTableName => :IN_sCoreTableName," 
  " IN_sComment => :IN_sComment); End;";
  

static EWDB_OCI_SFS SQLParamsBindArray[] = 
{
  {0,1,0,0,0,OA_INT,":OUT_RetCode"},
  {0,1,0,0,0,OA_INT,":IN_idCore"},
  {0,1,0,0,0,OA_SZ,":IN_sCoreTableName"},
  {0,1,0,0,0,OA_SZ,":IN_sComment"}
};

#define	NUM_FIELDS 4

static  EWDBid    Local_idCore;
static  int       Local_iRetCode;
static  char      Local_szComment[4001];
static  char      Local_szTableName[100];

/* Statement Struct for UpdateComment szStatement */
static EWDB_OCIStatementStruct SSStatement;


/********************************
      FUNCTION PROTOTYPES
********************************/
static int PrepUpdateCommentExec(EWDBid IN_idCore, char * IN_szTableName, 
                          char * IN_szComment, EWDB_Cursor * ppCursor);
static int PostUpdateCommentExec(void);
static int InitUpdateCommentStatement(char *szStatement, 
                               EWDB_OCIStatementStruct *pSS);


/* Updates a comment for any record in the DB.  Enter the tablename 
   and the record ID for the table that the comment belongs to, and
   the function will update the comment for that record.
   BAD THINGS:  This requires that the caller know DB layout, which
   is a bad thing.  What we need is a lookup table for the comment
   types, and we can perform a conversion from the lookup table to
   the actual DB table type withing the DB.  (I even confuse myself
   trying to explain that one, but it is simple.(REALLY))
*********************************************************************/
int ewdb_api_UpdateComment(char * szFieldTypeName, EWDBid IN_idCore, 
                           char * szUpdatedComment)
{

  EWDB_Cursor pCursor;

  if (szFieldTypeName == NULL || szUpdatedComment== NULL)
  {
    logit ("", "ewdb_api_UpdateComment(): Null pointer passed in!\n");
    return EWDB_RETURN_FAILURE;
  }

  ewdb_base_SetLastOraAPIActionTime ();


  if (ewdb_base_Reconnect () != EWDB_RETURN_SUCCESS)
  {
    logit("", "ewdb_api_UpdateComment(): Could not reconnect to the database!\n");
    return (EWDB_RETURN_FAILURE);
  }

  if (PrepUpdateCommentExec (IN_idCore, szFieldTypeName, 
                      szUpdatedComment, &pCursor) != EWDB_RETURN_SUCCESS)
  {
    logit ("", "ewdb_api_UpdateComment(): PrepUpdateCommentExec() failed.\n");
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  }

  if (ewdb_base_SQLExecute (pCursor))
  {
    ewdb_base_ErrorReport (hEWDBC, pCursor,"ewdb_api_UpdateComment(): ewdb_base_SQLExecute", 1);
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  } 
  
  /* Commit the transaction (all the previous inserts!)
     In Case there is any auditing or logging or debug
     changes made in the stored procedures.
  ****************************************************/
  if (ewdb_base_SQLCommit (hEWDBC))
  {
    ewdb_base_ErrorReport (hEWDBC, hEWDBC,"ewdb_api_UpdateComment(): ewdb_base_SQLCommit",2);
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  }

  if (PostUpdateCommentExec () != EWDB_RETURN_SUCCESS)
  {
    logit ("", "ewdb_api_UpdateComment(): PostUpdateCommentExec failed!\n");
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  }

  ewdb_base_SetLastOraAPIActionTime ();

  return(EWDB_RETURN_SUCCESS);
}  /* end ewdb_api_UpdateComment() */


static int InitUpdateCommentStatement(char *szStatement, 
                                      EWDB_OCIStatementStruct *pSS)
{
	pSS->FieldArray[0].pVal = &Local_iRetCode;
	pSS->FieldArray[1].pVal = &Local_idCore;
	pSS->FieldArray[2].pVal = Local_szTableName;
	pSS->FieldArray[3].pVal = Local_szComment;

	return(ewdb_base_RequestCursor (szStatement, pSS, 0));
}


static int PrepUpdateCommentExec(EWDBid IN_idCore, char * IN_szTableName, 
                                 char * IN_szComment, EWDB_Cursor * ppCursor) 
{


	SSStatement.NumOfFields = NUM_FIELDS;
	SSStatement.FieldArray = SQLParamsBindArray;
	SSStatement.RecordSize = 0;

  Local_idCore=IN_idCore;
  strcpy(Local_szTableName,IN_szTableName);
 
  strncpy(Local_szComment,IN_szComment,sizeof(Local_szComment)-1);
  Local_szComment[sizeof(Local_szComment)-1]=0;


	if (InitUpdateCommentStatement (SQL_STRING, &SSStatement) != EWDB_RETURN_SUCCESS)
	{
		logit ("", "PrepUpdateCommentExec(): InitUpdateCommentStatement failed!\n");
		return EWDB_RETURN_FAILURE;
	}

	*ppCursor = SSStatement.pCda;
	
	return(EWDB_RETURN_SUCCESS);
}  /* end PrepUpdateCommentExec() */


static int PostUpdateCommentExec(void)
{
  EWDB_Cursor pCursor;
  
  pCursor = SSStatement.pCda;

  ewdb_base_ReleaseCursor(pCursor);

  if(Local_iRetCode < 0)
  {
    logit("","PostUpdateCommentExec():ERROR! SQL Proc Update_Comment(%d/%s) returned %d.\n",
          Local_idCore, Local_szTableName, Local_iRetCode);
    return(EWDB_RETURN_WARNING);
  }

  return(EWDB_RETURN_SUCCESS);
}  /* end PostUpdateCommentExec() */

