/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_api_CreateAlarmsUser.c,v 1.4 2005/06/03 22:32:16 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_api_CreateAlarmsUser.c,v $
 *     Revision 1.4  2005/06/03 22:32:16  davidk
 *     DB API Cleanup
 *
 *     Revision 1.3  2001/07/28 00:44:27  lucky
 *      State of the code after debugging and testing prior to v6.0 release freeze.
 *
 *     Revision 1.2  2001/05/15 18:47:40  davidk
 *     Moved functions around between the apps, DB API, and DB API INTERNAL
 *     levels.  Renamed functions and files.  Added support for amplitude
 *     magnitude types.  Reformatted makefiles.
 *
 *     Revision 1.1  2001/02/28 17:24:59  lucky
 *     Initial revision
 *
 *     Revision 1.1  2001/02/28 17:19:22  lucky
 *     Initial revision
 *
 *
 */


#include <ewdb_ora_api.h>
#include <ewdb_cli_base.h>
#include <ewdb_oci_base_internal.h>


static char SQL_STRING[] =
  "Begin CreateAlarmsUser (OUT_idUser => :OUT_idUser,"
  "                        IN_dPriority => :IN_dPriority, "
  "                        IN_sDescription => :IN_sDescription); End;";
  
static EWDB_OCI_SFS SQLParamsBindArray[] = 
{
  {0,1,0,0,0,OA_INT,    ":OUT_idUser"},
  {0,1,0,0,0,OA_INT,    ":IN_dPriority"},
  {0,1,256,0,0,OA_SZ,   ":IN_sDescription"},
};

#define  NUM_FIELDS  3

static EWDB_OCIStatementStruct SSStatement;
static EWDB_AlarmsUserStruct   Local_UserStruct;

static int PrepCreateUserExec (EWDB_AlarmsUserStruct *, EWDB_Cursor *);
static int PostCreateUserExec (EWDB_AlarmsUserStruct *);
static int InitCreateUserStatement (char *, EWDB_OCIStatementStruct *);


int ewdb_api_CreateAlarmsUser(EWDB_AlarmsUserStruct *pUser)
{

  EWDB_Cursor  pCursor;

  if (pUser == NULL)
  {
    logit ("", "ewdb_api_CreateAlarmsUser(): Null parameters passed in!\n");
    return EWDB_RETURN_FAILURE;
  }

  ewdb_base_SetLastOraAPIActionTime ();

  if (ewdb_base_Reconnect () != EWDB_RETURN_SUCCESS)
  /* Establishes connection, and performs binding!?! */
  {
    logit ("", "ewdb_api_CreateAlarmsUser(): Could not reconnect to the database!\n");
    return (EWDB_RETURN_FAILURE);
  }

  if (PrepCreateUserExec (pUser, &pCursor) != EWDB_RETURN_SUCCESS)
  {
    logit ("", "ewdb_api_CreateAlarmsUser(): PrepCreateUserExec failed.\n");
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  }

  if (ewdb_base_SQLExecute (pCursor))
  {
    ewdb_base_ErrorReport (hEWDBC, pCursor,"ewdb_api_CreateAlarmsUser(): ewdb_base_SQLExecute", 1);
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  } 
  
  /* Commit the transaction (all the previous inserts!)
     In Case there is any auditing or logging or debug
     changes made in the stored procedures.
  ****************************************************/
  if (ewdb_base_SQLCommit (hEWDBC))
  {
    ewdb_base_ErrorReport (hEWDBC, hEWDBC, "ewdb_api_CreateAlarmsUser(): ewdb_base_SQLCommit",2);
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  }

  if (PostCreateUserExec (pUser) != EWDB_RETURN_SUCCESS)
  {
    logit ("", "ewdb_api_CreateAlarmsUser(): PostCreateUserExec failed!\n");
    return(EWDB_RETURN_FAILURE);
  }

  return EWDB_RETURN_SUCCESS;
}  /* end ewdb_api_CreateAlarmsUser() */


/******************* InitCreateUserStatement *******************/
static int InitCreateUserStatement(char *szStatement, EWDB_OCIStatementStruct *pSS)
{

  if ((szStatement == NULL) ||  (pSS == NULL))
  {
    logit ("", "InitCreateUserStatement(): Null parameters passed in!\n");
    return EWDB_RETURN_FAILURE;
  }

  pSS->FieldArray[0].pVal = &(Local_UserStruct.idUser);
  pSS->FieldArray[1].pVal = &(Local_UserStruct.dPriority);
  pSS->FieldArray[2].pVal = Local_UserStruct.sDescription;

  if (ewdb_base_RequestCursor (szStatement, pSS, 0) != 0)
  {
    logit ("", "InitCreateUserStatement(): ewdb_base_RequestCursor failed.\n");
    return EWDB_RETURN_FAILURE;
  }

  return EWDB_RETURN_SUCCESS;
}  /* end InitCreateUserStatement() */


/******************* PrepCreateUserExec *******************/
static int PrepCreateUserExec (EWDB_AlarmsUserStruct *pUser, EWDB_Cursor *ppCursor)
{

  if ((pUser == NULL) || (ppCursor == NULL))
  {
    logit ("", "PrepCreateUserExec(): Null parameters passed in!\n");
    return EWDB_RETURN_FAILURE;
  }

  memcpy (&Local_UserStruct, pUser, sizeof (EWDB_AlarmsUserStruct));

  SSStatement.NumOfFields = NUM_FIELDS;
  SSStatement.FieldArray = SQLParamsBindArray;
  SSStatement.RecordSize = 0;

  if (InitCreateUserStatement (SQL_STRING, &SSStatement)
                             != EWDB_RETURN_SUCCESS)
  {
    logit ("", "PrepCreateUserExec(): InitCreateUserStatement failed!\n");
    return EWDB_RETURN_FAILURE;
  }

  *ppCursor = SSStatement.pCda;
  
  return EWDB_RETURN_SUCCESS;
}  


/******************* PostCreateUserExec *******************/
static int PostCreateUserExec (EWDB_AlarmsUserStruct *pUser)
{
  EWDB_Cursor pCursor;
  
  pCursor = SSStatement.pCda;

  if (pUser == NULL)
  {
    logit ("", "PostCreateUserExec(): Invalid arguments passed in.\n");
    ewdb_base_ReleaseCursor (pCursor);
    return(EWDB_RETURN_WARNING);
  }
  
  pUser->idUser = Local_UserStruct.idUser;
  
  ewdb_base_ReleaseCursor (pCursor);

  if(Local_UserStruct.idUser <= 0)
    return(EWDB_RETURN_WARNING);
  
  return EWDB_RETURN_SUCCESS;
}  /* end PostCreateUserExec() */
