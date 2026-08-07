/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_api_GetNextScheduledSnippetRetrievalAttempt.c,v 1.1 2003/09/16 17:02:58 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_api_GetNextScheduledSnippetRetrievalAttempt.c,v $
 *     Revision 1.1  2003/09/16 17:02:58  davidk
 *     Initial revision
 *
 *     Revision 1.3  2003/02/04 17:56:07  davidk
 *     Removed superfluous debug statement
 *
 *     Revision 1.2  2002/05/13 23:11:56  davidk
 *     Added additional constraints to the SQL statement as a result of the change from
 *     Mark1 to Mark2.
 *
 *     Revision 1.1  2002/04/16 20:09:59  davidk
 *     Initial revision
 *
 *     Revision 1.1  2002/03/05 23:50:18  davidk
 *     Initial revision
 *
 *     Revision 1.3  2001/07/23 17:09:11  davidk
 *     Added exception handler in SQL query, so that a return
 *     code is set, that can be checked by PostXXX().
 *
 *     Revision 1.2  2001/05/15 02:16:30  davidk
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
  " select min(tNextAttempt) into :OUT_tNextAttempt "
	"  from ALL_SNIPPET_REQUESTS "
	"  where iLockTime IS NULL or iLockTime=0; "
	" if :OUT_tNextAttempt IS NULL THEN "
	"  :OUT_RetCode :=1; "
	" else "
  "  :OUT_RetCode := 0; "
	" end if; "
  "EXCEPTION "
  "  WHEN NO_DATA_FOUND THEN "
  "    :OUT_RetCode := 1; "
  "  WHEN OTHERS THEN "
  "    :OUT_RetCode := -1; "
  "End;";

static EWDB_OCI_SFS SQLParamsBindArray[] = 
{
  {0,1,0,0,0,OA_INT,":OUT_RetCode"},
  {0,1,0,0,0,OA_INT,":OUT_tNextAttempt"}
};

#define  NUM_FIELDS 2


static int tNextAttempt, iRetCode;


/* Statement Struct for GetNextScheduledSnippetRetrievalAttempt statement */
static EWDB_OCIStatementStruct SSStatement;

/********************************
      FUNCTION PROTOTYPES
********************************/
int PrepGetNextScheduledSnippetRetrievalAttemptExec(EWDB_Cursor * ppCursor);
int PostGetNextScheduledSnippetRetrievalAttemptExec(time_t * ptNextAttempt);
int InitGetNextScheduledSnippetRetrievalAttemptStatement(char *statement, 
                            EWDB_OCIStatementStruct *pSS);




int ewdb_api_GetNextScheduledSnippetRetrievalAttempt(time_t * ptNextAttempt)
{

  EWDB_Cursor pCursor;

  if (ptNextAttempt== NULL)
  {
    logit ("", "ewdb_api_GetNextScheduledSnippetRetrievalAttempt():Null pointer passed in!\n");
    return EWDB_RETURN_FAILURE;
  }

  ewdb_base_SetLastOraAPIActionTime ();


  if (ewdb_base_Reconnect () != EWDB_RETURN_SUCCESS)
  {
    logit("", "ewdb_api_GetNextScheduledSnippetRetrievalAttempt(): "
          "Could not reconnect to the database!\n");
    return (EWDB_RETURN_FAILURE);
  }

  if (PrepGetNextScheduledSnippetRetrievalAttemptExec (&pCursor) != EWDB_RETURN_SUCCESS)
  {
    logit ("", "ORA_API:ewdb_api_GetNextScheduledSnippetRetrievalAttempt():PrepGetNextScheduledSnippetRetrievalAttemptExec() failed.\n");
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  }

  if (ewdb_base_SQLExecute (pCursor))
  {
    ewdb_base_ErrorReport (hEWDBC, pCursor,"ewdb_api_GetNextScheduledSnippetRetrievalAttempt():ewdb_base_SQLExecute", 1);
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  } 

  
  /* Commit the transaction (all the previous inserts!)
     In Case there is any auditing or logging or debug
     changes made in the stored procedures.
  ****************************************************/
  if (ewdb_base_SQLCommit (hEWDBC))
  {
    ewdb_base_ErrorReport (hEWDBC, hEWDBC,"ewdb_api_GetNextScheduledSnippetRetrievalAttempt():ewdb_base_SQLCommit",2);
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  }

  if (PostGetNextScheduledSnippetRetrievalAttemptExec (ptNextAttempt) 
      != EWDB_RETURN_SUCCESS)
  {
    logit ("", "ewdb_api_GetNextScheduledSnippetRetrievalAttempt(): "
           "Call to PostGetNextScheduledSnippetRetrievalAttemptExec failed!\n");
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  }

  ewdb_base_SetLastOraAPIActionTime ();

  return(EWDB_RETURN_SUCCESS);
}  

int InitGetNextScheduledSnippetRetrievalAttemptStatement(char *statement, 
                            EWDB_OCIStatementStruct *pSS)
{
  pSS->FieldArray[0].pVal = &iRetCode;
  pSS->FieldArray[1].pVal = &tNextAttempt;

  ewdb_base_RequestCursor (statement, pSS, 0);

  return(EWDB_RETURN_SUCCESS);
}  /* End InitGetNextScheduledSnippetRetrievalAttemptStatement() */


int PrepGetNextScheduledSnippetRetrievalAttemptExec(EWDB_Cursor * ppCursor)
{

  SSStatement.NumOfFields = NUM_FIELDS;
  SSStatement.FieldArray = SQLParamsBindArray;
  SSStatement.RecordSize = 0;

  if (InitGetNextScheduledSnippetRetrievalAttemptStatement (SQL_STRING,
                           &SSStatement) != EWDB_RETURN_SUCCESS)
  {
    logit ("", "Call to InitGetNextScheduledSnippetRetrievalAttemptStatement failed!\n");
    return EWDB_RETURN_FAILURE;
  }

  *ppCursor = SSStatement.pCda;
  
  return(EWDB_RETURN_SUCCESS);
}  /* End PrepGetNextScheduledSnippetRetrievalAttemptExec() */


int PostGetNextScheduledSnippetRetrievalAttemptExec(time_t * ptNextAttempt)
{
  EWDB_Cursor pCursor;
  
  pCursor = SSStatement.pCda;

  ewdb_base_ReleaseCursor(pCursor);

  *ptNextAttempt = tNextAttempt;

  if(iRetCode == 0)
  {
    return(EWDB_RETURN_SUCCESS);
  }
  else if(iRetCode == 1)
  {
    return(EWDB_RETURN_WARNING);
  }
  else
  {
    logit("","%s():ERROR! %d returned by SQL Query\n",
          "PostGetNextScheduledSnippetRetrievalAttemptExec",iRetCode);
    return(EWDB_RETURN_FAILURE);
  }

}  /* End PostGetNextScheduledSnippetRetrievalAttemptExec() */

