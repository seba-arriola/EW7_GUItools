/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_api_DeleteSMMessage.c,v 
 *
 *    Revision history:
 *     $Log: ewdb_api_DeleteSMMessage.c,v $
 *     Revision 1.1  2001/05/15 02:16:41  davidk
 *     Initial revision
 *
 *     Revision 1.1  2001/04/06 19:01:00  davidk
 *     Initial revision
 *
 *
 *************************************************************/


#include <stdlib.h>
#include <stdio.h>
#include <ewdb_ora_api.h>
#include <ewdb_cli_base.h>
#include <ewdb_ew_oci_base.h>

static char SQL_STRING[] =
  "Begin :OUT_RetCode = Delete_SMMessage("
  " IN_idSMMessage => :IN_idSMMessage); End;";
  
static EWDB_OCI_SFS SQLParamsBindArray[] = 
{
  {0,1,0,0,0,OA_INT,   ":OUT_RetCode"},
  {0,1,0,0,0,OA_EWDBID,":IN_idSMMessage"}
};

#define  NUM_FIELDS  2

static int    iRetCode;
static EWDBid idSMMessage;

/* Insertion Struct for statement */
static EWDB_OCIStatementStruct SSStatement;


/********************************
      FUNCTION PROTOTYPES
********************************/
int PrepDeleteSMMessageExec(EWDBid IN_idSMMessage, EWDB_Cursor *ppCursor);
int PostDeleteSMMessageExec(int * pRetCode);
int InitDeleteSMMessageStatement (char *statement, 
                                  EWDB_OCIStatementStruct *pSS);
/*******************************/

int ewdb_api_DeleteSMMessage(EWDBid IN_idSMMessage)
{
  /**************************
      Return Values:
          EWDB_RETURN_FAILURE: call failed
          EWDB_RETURN_SUCCESS: call succeeded
          others             : undefined
  **************************/

  EWDB_Cursor pCursor;
  int RetCode;

  ewdb_base_SetLastOraAPIActionTime ();

  if (ewdb_base_Reconnect () != EWDB_RETURN_SUCCESS)
  /* Re-establishes connection. */
  {
    logit("", "EWDB_DeleteSMMessage(): "
          "Could not reconnect to the database!\n");
    return (EWDB_RETURN_FAILURE);
  }

  if (PrepDeleteSMMessageExec(IN_idSMMessage, &pCursor)
      != EWDB_RETURN_SUCCESS)
  {
    logit ("", "ORA_API:DeleteSMMessage():PrepDeleteSMMessage() failed.\n");
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  }

  if (ewdb_base_SQLExecute (pCursor))
  {
    ewdb_base_ErrorReport (hEWDBC, pCursor,"DeleteSMMessage:ewdb_base_SQLExecute", 1);
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  } 
  
  /* Commit the transaction (all the previous inserts!)
     In Case there is any auditing or logging or debug
     changes made in the stored procedures.
  ****************************************************/
  if (ewdb_base_SQLCommit (hEWDBC))
  {
    ewdb_base_ErrorReport (hEWDBC, hEWDBC,"DeleteSMMessage:ewdb_base_SQLCommit",2);
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  }

  if (PostDeleteSMMessageExec(&RetCode) != EWDB_RETURN_SUCCESS)
  {
    logit ("", "Call to PostDeleteSMMessageExec failed!\n");
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  }

  if(RetCode < 0)
    return(EWDB_RETURN_FAILURE);
  else
    return(EWDB_RETURN_SUCCESS);
}  /* end ewdb_api_DeleteSMMessage() */


int InitDeleteSMMessageStatement (char *statement, EWDB_OCIStatementStruct *pSS)
{

  pSS->FieldArray[0].pVal = &iRetCode;
  pSS->FieldArray[1].pVal = &idSMMessage;

  if(EWDB_Debug)
  {
    logit ("", "InitDeleteSMMessageStatement(): Requesting Cursor\n");
  }

  ewdb_base_RequestCursor (statement, pSS, 0);

  if(EWDB_Debug)
  {
    logit ("", "InitDeleteSMMessageStatement(): szStatement:\n%s\n",
           statement);
  }
  
  return(EWDB_RETURN_SUCCESS);
}  /* End InitDeleteSMMessageStatement() */


int PrepDeleteSMMessageExec(EWDBid IN_idSMMessage, EWDB_Cursor *ppCursor)
{

  SSStatement.NumOfFields = NUM_FIELDS;
  SSStatement.FieldArray = SQLParamsBindArray;
  SSStatement.RecordSize = 0;


  /* copy the input params over to the local copies */
  idSMMessage  = IN_idSMMessage;

  /* call InitXXX() to prep a cursor */
  if (InitDeleteSMMessageStatement (SQL_STRING,
                           &SSStatement) != EWDB_RETURN_SUCCESS)
  {
    logit ("", "Call to InitDeleteSMMessageStatement failed!\n");
    return EWDB_RETURN_FAILURE;
  }

  /* record the cursor we got */
  *ppCursor = SSStatement.pCda;
  
  /* go home, happy! */
  return(EWDB_RETURN_SUCCESS);
}  /* End PrepDeleteSMMessageExec() */


int PostDeleteSMMessageExec(int * pRetCode)
{
  EWDB_Cursor pCursor;
  
  if (pRetCode == NULL)
  {
    logit ("", "%s(): Invalid arguments passed in.\n",
      "PostDeleteSMMessageExec");
    return EWDB_RETURN_FAILURE;
  }
  
  if(iRetCode)
  {
    logit("t","SQL Proc Delete_SMMessage() failed with return code %d\n",
      iRetCode);
  }
  
  *pRetCode = iRetCode;
  
  pCursor = SSStatement.pCda;
  
  ewdb_base_ReleaseCursor (pCursor);
  
  return (EWDB_RETURN_SUCCESS);
}  /* End PostDeleteSMMessageExec() */
