/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_api_CreateAlarmsCriteria.c,v 1.5 2005/06/03 22:32:16 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_api_CreateAlarmsCriteria.c,v $
 *     Revision 1.5  2005/06/03 22:32:16  davidk
 *     DB API Cleanup
 *
 *     Revision 1.4  2001/08/07 16:51:06  lucky
 *     Pre v6.0 checkin
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
#include <ewdb_ew_oci_base.h>

static char SQL_STRING[] =
  "Begin CreateOrUpdateAlarmsCriteria (OUT_idCritProgram => :OUT_idCritProgram,"
  "                                    IN_sProgName => :IN_sProgName, IN_sProgDir => :IN_sProgDir, "
  "                                    IN_sProgDescription => :IN_sProgDescription, "
  "                                    IN_idCritProgram => :IN_idCritProgram); End;";
  
static EWDB_OCI_SFS SQLParamsBindArray[] = 
{
  {0,1,0,0,0,OA_INT,    ":OUT_idCritProgram"},
  {0,1,256,0,0,OA_SZ,   ":IN_sProgName"},
  {0,1,256,0,0,OA_SZ,   ":IN_sProgDir"},
  {0,1,256,0,0,OA_SZ,   ":IN_sProgDescription"},
  {0,1,0,0,0,OA_INT,    ":IN_idCritProgram"},
};

#define  NUM_FIELDS  5

static  EWDB_OCIStatementStruct       SSStatement;
static  EWDB_AlarmsCritProgramStruct  Local_CritStruct;

static int PrepCreateCritExec (EWDB_AlarmsCritProgramStruct *, EWDB_Cursor *);
static int PostCreateCritExec (EWDB_AlarmsCritProgramStruct *);
static int InitCreateCritStatement (char *, EWDB_OCIStatementStruct *);


int ewdb_api_CreateAlarmsCriteria(EWDB_AlarmsCritProgramStruct *pCrit)
{

  EWDB_Cursor  pCursor;

  if (pCrit == NULL)
  {
    logit ("", "ewdb_api_CreateAlarmsCriteria(): Null parameters passed in!\n");
    return EWDB_RETURN_FAILURE;
  }

  ewdb_base_SetLastOraAPIActionTime ();

  if (ewdb_base_Reconnect () != EWDB_RETURN_SUCCESS)
  /* Establishes connection, and performs binding!?! */
  {
    logit ("", "ewdb_api_CreateAlarmsCriteria(): Could not reconnect to the database!\n");
    return (EWDB_RETURN_FAILURE);
  }

  if (PrepCreateCritExec (pCrit, &pCursor) != EWDB_RETURN_SUCCESS)
  {
    logit ("", "ewdb_api_CreateAlarmsCriteria(): PrepCreateCritExec failed.\n");
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  }

  if (ewdb_base_SQLExecute (pCursor))
  {
    ewdb_base_ErrorReport (hEWDBC, pCursor,"ewdb_api_CreateAlarmsCriteria(): ewdb_base_SQLExecute", 1);
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  } 
  
  /* Commit the transaction (all the previous inserts!)
     In Case there is any auditing or logging or debug
     changes made in the stored procedures.
  ****************************************************/
  if (ewdb_base_SQLCommit (hEWDBC))
  {
    ewdb_base_ErrorReport (hEWDBC, hEWDBC, "ewdb_api_CreateAlarmsCriteria(): ewdb_base_SQLCommit",2);
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  }

  if (PostCreateCritExec (pCrit) != EWDB_RETURN_SUCCESS)
  {
    logit("", "ewdb_api_CreateAlarmsCriteria(): PostCreateCritExec failed!\n");
    return(EWDB_RETURN_FAILURE);
  }

  return EWDB_RETURN_SUCCESS;
}  /* end ewdb_api_CreateAlarmsCriteria() */


/******************* InitCreateCritStatement *******************/
static int InitCreateCritStatement(char *szStatement, EWDB_OCIStatementStruct *pSS)
{

  if ((szStatement == NULL) ||  (pSS == NULL))
  {
    logit ("", "InitCreateCritStatement(): Null parameters passed in!\n");
    return EWDB_RETURN_FAILURE;
  }

  pSS->FieldArray[0].pVal = &(Local_CritStruct.idCritProgram);
  pSS->FieldArray[1].pVal = (Local_CritStruct.sProgName);
  pSS->FieldArray[2].pVal = (Local_CritStruct.sProgDir);
  pSS->FieldArray[3].pVal = (Local_CritStruct.sProgDescription);
  pSS->FieldArray[4].pVal =  &(Local_CritStruct.idCritProgram);

  if (ewdb_base_RequestCursor (szStatement, pSS, 0) != 0)
  {
    logit ("", "Call to ewdb_base_RequestCursor failed.\n");
    return EWDB_RETURN_FAILURE;
  }

  return EWDB_RETURN_SUCCESS;
}  


/******************* PrepCreateCritExec *******************/
static int PrepCreateCritExec (EWDB_AlarmsCritProgramStruct *pCrit, EWDB_Cursor *ppCursor)
{

  if ((pCrit == NULL) || (ppCursor == NULL))
  {
    logit ("", "Null parameters passed in!\n");
    return EWDB_RETURN_FAILURE;
  }

  memcpy (&Local_CritStruct, pCrit, sizeof (EWDB_AlarmsCritProgramStruct));

  SSStatement.NumOfFields = NUM_FIELDS;
  SSStatement.FieldArray = SQLParamsBindArray;
  SSStatement.RecordSize = 0;

  if (InitCreateCritStatement (SQL_STRING, &SSStatement)
                             != EWDB_RETURN_SUCCESS)
  {
    logit ("", "Call to InitCreateCritStatement failed!\n");
    return EWDB_RETURN_FAILURE;
  }

  *ppCursor = SSStatement.pCda;
  
  return EWDB_RETURN_SUCCESS;
}  


/******************* PostCreateCritExec *******************/
static int PostCreateCritExec (EWDB_AlarmsCritProgramStruct *pCrit)
{
  EWDB_Cursor pCursor;
  
  pCursor = SSStatement.pCda;
  
  ewdb_base_ReleaseCursor (pCursor);

  if (pCrit == NULL)
  {
    logit ("", "PostCreateCritExec(): Invalid arguments passed in.\n");
    return EWDB_RETURN_FAILURE;
  }
  
  pCrit->idCritProgram = Local_CritStruct.idCritProgram;
  
  if (Local_CritStruct.idCritProgram <= 0)
  {
    logit("", "PostCreateCritExec(): SQL Proc CreateOrUpdateAlarmsCriteria() returned error %d.\n", 
          Local_CritStruct.idCritProgram);
    return EWDB_RETURN_WARNING;
  }
  
  return EWDB_RETURN_SUCCESS;
}  /* end PostCreateCritExec() */
