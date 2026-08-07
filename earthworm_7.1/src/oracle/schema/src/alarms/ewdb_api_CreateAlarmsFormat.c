/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_api_CreateAlarmsFormat.c,v 1.4 2005/06/03 22:32:16 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_api_CreateAlarmsFormat.c,v $
 *     Revision 1.4  2005/06/03 22:32:16  davidk
 *     DB API Cleanup
 *
 *     Revision 1.3  2001/08/07 16:51:06  lucky
 *     Pre v6.0 checkin
 *
 *     Revision 1.2  2001/07/28 00:44:27  lucky
 *      State of the code after debugging and testing prior to v6.0 release freeze.
 *
 *     Revision 1.1  2001/05/15 02:16:13  davidk
 *     Initial revision
 *
 */

#include <ewdb_ora_api.h>
#include <ewdb_cli_base.h>
#include <ewdb_ew_oci_base.h>

static char SQL_STRING[] =
  "Begin CreateOrUpdateAlarmsFormat (OUT_idFormat => :OUT_idFormat,"
  "                                  IN_sDescription => :IN_sDescription, "
  "                                  IN_sFmtInsert => :IN_sFmtInsert, "
  "                                  IN_sFmtDelete => :IN_sFmtDelete, "
  "                                  IN_idFormat => :IN_idFormat); End;";
  
static EWDB_OCI_SFS SQLParamsBindArray[] = 
{
  {0,1,0,0,0,OA_INT,    ":OUT_idFormat"},
  {0,1,256,0,0,OA_SZ,   ":IN_sDescription"},
  {0,1,EWDB_ALARMS_MAX_FORMAT_LEN,
           0,0,OA_SZ,   ":IN_sFmtInsert"},
  {0,1,EWDB_ALARMS_MAX_FORMAT_LEN,
           0,0,OA_SZ,   ":IN_sFmtDelete"},
  {0,1,0,0,0,OA_INT,    ":IN_idFormat"},
};

#define  NUM_FIELDS  5

static EWDB_OCIStatementStruct   SSStatement;

static EWDB_AlarmsFormatStruct  Local_FormStruct;

static int PrepCreateFormExec (EWDB_AlarmsFormatStruct *, EWDB_Cursor *);
static int PostCreateFormExec (EWDB_AlarmsFormatStruct *);
static int InitCreateFormStatement (char *, EWDB_OCIStatementStruct *);

int ewdb_api_CreateAlarmsFormat(EWDB_AlarmsFormatStruct *pForm)
{

  EWDB_Cursor  pCursor;

  if (pForm == NULL)
  {
    logit ("", "ewdb_api_CreateAlarmsFormat(): Null parameters passed in!\n");
    return EWDB_RETURN_FAILURE;
  }

  ewdb_base_SetLastOraAPIActionTime ();

  if (ewdb_base_Reconnect () != EWDB_RETURN_SUCCESS)
  {
    logit ("", "ewdb_api_CreateAlarmsFormat(): Could not reconnect to the database!\n");
    return (EWDB_RETURN_FAILURE);
  }

  if (PrepCreateFormExec (pForm, &pCursor) != EWDB_RETURN_SUCCESS)
  {
    logit ("", "ewdb_api_CreateAlarmsFormat(): PrepCreateFormExec failed.\n");
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  }

  if (ewdb_base_SQLExecute (pCursor))
  {
    ewdb_base_ErrorReport (hEWDBC, pCursor,"ewdb_api_CreateAlarmsFormat(): ewdb_base_SQLExecute", 1);
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  } 

  /* Commit the transaction (all the previous inserts!)
     In Case there is any auditing or logging or debug
     changes made in the stored procedures.
  ****************************************************/
  if (ewdb_base_SQLCommit (hEWDBC))
  {
    ewdb_base_ErrorReport (hEWDBC, hEWDBC, "ewdb_api_CreateAlarmsFormat(): ewdb_base_SQLCommit",2);
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  }

  if (PostCreateFormExec (pForm) != EWDB_RETURN_SUCCESS)
  {
    logit ("", "ewdb_api_CreateAlarmsFormat(): PostCreateFormExec failed!\n");
    return(EWDB_RETURN_FAILURE);
  }

  return EWDB_RETURN_SUCCESS;
}  /* end ewdb_api_CreateAlarmsFormat() */


/******************* InitCreateFormStatement *******************/
static int InitCreateFormStatement(char *statement, EWDB_OCIStatementStruct *pSS)
{

  if ((statement == NULL) ||  (pSS == NULL))
  {
    logit ("", "InitCreateFormStatement(): Null parameters passed in!\n");
    return EWDB_RETURN_FAILURE;
  }

  pSS->FieldArray[0].pVal = &(Local_FormStruct.idFormat);
  pSS->FieldArray[1].pVal = (Local_FormStruct.sDescription);
  pSS->FieldArray[2].pVal = (Local_FormStruct.sFmtInsert);
  pSS->FieldArray[3].pVal = (Local_FormStruct.sFmtDelete);
  pSS->FieldArray[4].pVal = &(Local_FormStruct.idFormat);

  if (ewdb_base_RequestCursor (statement, pSS, 0) != 0)
  {
    logit ("", "Call to ewdb_base_RequestCursor failed.\n");
    return EWDB_RETURN_FAILURE;
  }

  return EWDB_RETURN_SUCCESS;
}  


/******************* PrepCreateFormExec *******************/
static int PrepCreateFormExec (EWDB_AlarmsFormatStruct *pForm, EWDB_Cursor *ppCursor)
{

  if ((pForm == NULL) || (ppCursor == NULL))
  {
    logit ("", "PrepCreateFormExec(): Null parameters passed in!\n");
    return EWDB_RETURN_FAILURE;
  }

  memcpy (&Local_FormStruct, pForm, sizeof (EWDB_AlarmsFormatStruct));

  SSStatement.NumOfFields = NUM_FIELDS;
  SSStatement.FieldArray = SQLParamsBindArray;
  SSStatement.RecordSize = 0;

  if (InitCreateFormStatement (SQL_STRING, &SSStatement)
                             != EWDB_RETURN_SUCCESS)
  {
    logit ("", "PrepCreateFormExec(): InitCreateFormStatement failed!\n");
    return EWDB_RETURN_FAILURE;
  }

  *ppCursor = SSStatement.pCda;
  
  return EWDB_RETURN_SUCCESS;
}  


/******************* PostCreateFormExec *******************/
static int PostCreateFormExec (EWDB_AlarmsFormatStruct *pForm)
{
  EWDB_Cursor pCursor;
  
  pCursor = SSStatement.pCda;
  
  ewdb_base_ReleaseCursor (pCursor);

  if (pForm == NULL)
  {
    logit ("", "PostCreateFormExec(): Invalid arguments passed in.\n");
    return EWDB_RETURN_FAILURE;
  }
  
  pForm->idFormat = Local_FormStruct.idFormat;
  if (Local_FormStruct.idFormat <= 0)
  {
    logit("", "PostCreateFormExec(): SQL Proc CreateOrUpdateAlarmsFormat() returned error %d..\n",
          Local_FormStruct.idFormat);
    return EWDB_RETURN_FAILURE;
  }
  
  return EWDB_RETURN_SUCCESS;
}  /* end PostCreateFormExec() */
