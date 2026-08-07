/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_api_GetAndIncrementCubeVersion.c,v 1.4 2005/06/03 22:32:16 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_api_GetAndIncrementCubeVersion.c,v $
 *     Revision 1.4  2005/06/03 22:32:16  davidk
 *     DB API Cleanup
 *
 *     Revision 1.3  2001/08/07 16:51:06  lucky
 *     Pre v6.0 checkin
 *
 *     Revision 1.2  2001/07/28 00:44:27  lucky
 *      State of the code after debugging and testing prior to v6.0 release freeze.
 *
 *     Revision 1.1  2001/06/26 17:24:13  lucky
 *     Initial revision
 *
 *
 */


#include <ewdb_ora_api.h>
#include <ewdb_cli_base.h>
#include <ewdb_ew_oci_base.h>
#include <ewdb_oci_base_internal.h>


static char SQL_STRING[] =
  "Begin GetAndIncrementCubeVersion(OUT_iVersionNumber => :OUT_iVersionNumber,"
  "                                 IN_idEvent => :IN_idEvent); End;";
  
static EWDB_OCI_SFS SQLParamsBindArray[] = 
{
  {0,1,0,0,0,OA_INT,    ":OUT_iVersionNumber"},
  {0,1,0,0,0,OA_EWDBID, ":IN_idEvent"},
};

#define  NUM_FIELDS  2


static EWDB_OCIStatementStruct     SSStatement;

static EWDBid   Local_idEvent;
static EWDBid   Local_iVersionNum;


static int PrepIncrementCubeExec (int, EWDB_Cursor *);
static int PostIncrementCubeExec (int *pVersionNumber);
static int InitIncrementCubeStatement (char *, EWDB_OCIStatementStruct *);


/* 
 * Returns the current value of the CUBE version number and then
 * increments the value.
 */
int ewdb_api_GetAndIncrementCubeVersion(EWDBid idEvent, int *pVersionNumber)
{

  EWDB_Cursor  pCursor;

  if (idEvent < 0)
  {
    logit ("", "ewdb_api_GetAndIncrementCubeVersion(): Null parameters passed in!\n");
    return EWDB_RETURN_FAILURE;
  }

  ewdb_base_SetLastOraAPIActionTime ();

  if (ewdb_base_Reconnect () != EWDB_RETURN_SUCCESS)
  {
    logit ("", "ewdb_api_GetAndIncrementCubeVersion(): Could not reconnect to the database!\n");
    return (EWDB_RETURN_FAILURE);
  }

  if (PrepIncrementCubeExec (idEvent, &pCursor) != EWDB_RETURN_SUCCESS)
  {
    logit ("", "ewdb_api_GetAndIncrementCubeVersion(): PrepIncrementCubeExec failed.\n");
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  }

  if (ewdb_base_SQLExecute (pCursor))
  {
    ewdb_base_ErrorReport (hEWDBC, pCursor,"ewdb_api_GetAndIncrementCubeVersion(): ewdb_base_SQLExecute", 1);
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  } 

  /* Commit the transaction (all the previous inserts!)
     In Case there is any auditing or logging or debug
     changes made in the stored procedures.
  ****************************************************/
  if (ewdb_base_SQLCommit (hEWDBC))
  {
    ewdb_base_ErrorReport (hEWDBC, hEWDBC, "ewdb_api_GetAndIncrementCubeVersion(): ewdb_base_SQLCommit",2);
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  }

  if (PostIncrementCubeExec (pVersionNumber) != EWDB_RETURN_SUCCESS)
  {
    logit ("", "ewdb_api_GetAndIncrementCubeVersion(): PostIncrementCubeExec failed!\n");
    return (EWDB_RETURN_FAILURE);
  }

  return EWDB_RETURN_SUCCESS;
}  /* end ewdb_api_GetAndIncrementCubeVersion() */


/******************* InitIncrementCubeStatement *******************/
static int InitIncrementCubeStatement(char *statement, EWDB_OCIStatementStruct *pSS)
{

  if ((statement == NULL) ||  (pSS == NULL))
  {
    logit ("", "InitIncrementCubeStatement(): Null parameters passed in!\n");
    return EWDB_RETURN_FAILURE;
  }

  pSS->FieldArray[0].pVal = &Local_iVersionNum;
  pSS->FieldArray[1].pVal = &Local_idEvent;


  if (ewdb_base_RequestCursor (statement, pSS, 0) != 0)
  {
    logit ("", "InitIncrementCubeStatement(): ewdb_base_RequestCursor failed.\n");
    return EWDB_RETURN_FAILURE;
  }

  return EWDB_RETURN_SUCCESS;
}  /* end InitIncrementCubeStatement() */


/******************* PrepIncrementCubeExec *******************/
static int PrepIncrementCubeExec (int idEvent, EWDB_Cursor *ppCursor)
{

  if (ppCursor == NULL)
  {
    logit ("", "PrepIncrementCubeExec(): Null parameters passed in!\n");
    return EWDB_RETURN_FAILURE;
  }

  SSStatement.NumOfFields = NUM_FIELDS;
  SSStatement.FieldArray = SQLParamsBindArray;
  SSStatement.RecordSize = 0;

  Local_idEvent = idEvent;
  Local_iVersionNum = 0;

  if (InitIncrementCubeStatement (SQL_STRING, &SSStatement)
                             != EWDB_RETURN_SUCCESS)
  {
    logit ("", "Call to InitIncrementCubeStatement failed!\n");
    return EWDB_RETURN_FAILURE;
  }

  *ppCursor = SSStatement.pCda;
  
  return EWDB_RETURN_SUCCESS;
}  


/******************* PostIncrementCubeExec *******************/
static int PostIncrementCubeExec (int *pVersionNumber)
{
  EWDB_Cursor pCursor;
   
  *pVersionNumber = Local_iVersionNum;

  pCursor = SSStatement.pCda;
  ewdb_base_ReleaseCursor (pCursor);

  if(Local_iVersionNum < 0)
  {
    logit("","SQL PROC GetAndIncrementCubeVersion returned error (%d)\n",
          Local_iVersionNum);
    return(EWDB_RETURN_FAILURE);
  }

  return EWDB_RETURN_SUCCESS;
} 
