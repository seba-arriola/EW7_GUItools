/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_api_CreateAlarmAudit.c,v 1.9 2005/06/03 22:32:16 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_api_CreateAlarmAudit.c,v $
 *     Revision 1.9  2005/06/03 22:32:16  davidk
 *     DB API Cleanup
 *
 *     Revision 1.8  2005/02/03 20:36:24  mark
 *     Added groups and recipientdeliveries
 *
 *     Revision 1.7  2003/12/11 18:32:45  lucky
 *     Added iCore and iAlarmType fields
 *
 *     Revision 1.6  2001/08/07 16:51:06  lucky
 *     Pre v6.0 checkin
 *
 *     Revision 1.5  2001/07/31 20:44:40  lucky
 *     Changed from User to Recipient
 *
 *     Revision 1.4  2001/07/28 00:44:27  lucky
 *      State of the code after debugging and testing prior to v6.0 release freeze.
 *
 *     Revision 1.3  2001/07/20 17:33:23  lucky
 *     Added InvocationString
 *
 *     Revision 1.2  2001/06/26 17:24:13  lucky
 *     State of the code after Utah specs have been met.
 *
 *     Revision 1.1  2001/05/15 02:16:12  davidk
 *     Initial revision
 *
 *
 */


#include <ewdb_ora_api.h>
#include <ewdb_cli_base.h>
#include <ewdb_ew_oci_base.h>

static char SQL_STRING[] =
  "Begin CreateOrUpdateAlarmsAudit "
  "(OUT_idAudit => :OUT_idAudit,"
  "IN_idEvent => :IN_idEvent, "
  "IN_bAuto => :IN_bAuto, "
  "IN_idRecipient => :IN_idRecipient, "
  "IN_idFormat => :IN_idFormat, "
  "IN_idDelivery => :IN_idDelivery, "
  "IN_sTableName => :IN_sTableName, "
  "IN_tAlarmDeclared => :IN_tAlarmDeclared, "
  "IN_tAlarmExecuted => :IN_tAlarmExecuted,"
  "IN_sInvocationString => :IN_sInvocationString, "
  "IN_idAudit => :IN_idAudit, "
  "IN_idCore => :IN_idCore, "
  "IN_iAlarmType => :IN_iAlarmType, "
  "IN_idGroup => :IN_idGroup, "
  "IN_idRecipientDelivery => :IN_idRecipientDelivery); End;";
  
static EWDB_OCI_SFS SQLParamsBindArray[] = 
{
  {0,1,0,0,0,OA_INT,    ":OUT_idAudit"},
  {0,1,0,0,0,OA_INT,    ":IN_idEvent"},
  {0,1,0,0,0,OA_INT,    ":IN_bAuto"},
  {0,1,0,0,0,OA_INT,    ":IN_idRecipient"},
  {0,1,0,0,0,OA_INT,    ":IN_idFormat"},
  {0,1,0,0,0,OA_INT,    ":IN_idDelivery"},
  {0,256,0,0,0,OA_SZ,   ":IN_sTableName"},
  {0,1,0,0,0,OA_FLOAT,  ":IN_tAlarmDeclared"},
  {0,1,0,0,0,OA_FLOAT,  ":IN_tAlarmExecuted"},
  {0,256,0,0,0,OA_SZ,   ":IN_sInvocationString"},
  {0,1,0,0,0,OA_INT,    ":IN_idAudit"},
  {0,1,0,0,0,OA_INT,    ":IN_idCore"},
  {0,1,0,0,0,OA_INT,    ":IN_iAlarmType"},
  {0,1,0,0,0,OA_INT,    ":IN_idGroup"},
  {0,1,0,0,0,OA_INT,    ":IN_idRecipientDelivery"},
};

#define  NUM_FIELDS  15

static EWDB_OCIStatementStruct     SSStatement;

static  EWDB_AlarmAuditStruct      LocalAuditStruct;
static  char  Local_sztAlarmDeclared[20];
static  char  Local_sztAlarmExecuted[20];
static  char  Local_TableName[256];

static int PrepCreateAlarmAuditExec (EWDB_AlarmAuditStruct *, EWDB_Cursor *);
static int PostCreateAlarmAuditExec (EWDB_AlarmAuditStruct *);
static int InitCreateAlarmAuditStatement (char *, EWDB_OCIStatementStruct *);


int ewdb_api_CreateAlarmAudit(EWDB_AlarmAuditStruct *pAudit)
{

  EWDB_Cursor  pCursor;

  if (pAudit == NULL)
  {
    logit ("", "ewdb_api_CreateAlarmAudit(): Null parameters passed in!\n");
    return EWDB_RETURN_FAILURE;
  }

  ewdb_base_SetLastOraAPIActionTime ();

  if (ewdb_base_Reconnect () != EWDB_RETURN_SUCCESS)
  {
    logit ("", "ewdb_api_CreateAlarmAudit(): Could not reconnect to the database!\n");
    return (EWDB_RETURN_FAILURE);
  }

  if (PrepCreateAlarmAuditExec (pAudit, &pCursor) != EWDB_RETURN_SUCCESS)
  {
    logit ("", "ewdb_api_CreateAlarmAudit(): PrepCreateAlarmAuditExec failed.\n");
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  }

  if (ewdb_base_SQLExecute (pCursor))
  {
    ewdb_base_ErrorReport (hEWDBC, pCursor,"ewdb_api_CreateAlarmAudit(): ewdb_base_SQLExecute", 1);
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  } 
  
  /* Commit the transaction (all the previous inserts!)
     In Case there is any auditing or logging or debug
     changes made in the stored procedures.
  ****************************************************/
  if (ewdb_base_SQLCommit (hEWDBC))
  {
    ewdb_base_ErrorReport (hEWDBC, hEWDBC, "ewdb_api_CreateAlarmAudit(): ewdb_base_SQLCommit",2);
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  }

  if (PostCreateAlarmAuditExec (pAudit) != EWDB_RETURN_SUCCESS)
  {
    logit ("", "ewdb_api_CreateAlarmAudit(): PostCreateAlarmAuditExec failed!\n");
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  }

  return EWDB_RETURN_SUCCESS;
}  /* end ewdb_api_CreateAlarmAudit() */


/******************* InitCreateAlarmAuditStatement *******************/
static int InitCreateAlarmAuditStatement(char *szStatement, EWDB_OCIStatementStruct *pSS)
{

  if ((szStatement == NULL) ||  (pSS == NULL))
  {
    logit ("", "InitCreateAlarmAuditStatement(): Null parameters passed in!\n");
    return EWDB_RETURN_FAILURE;
  }

  pSS->FieldArray[0].pVal = &(LocalAuditStruct.idAudit);
  pSS->FieldArray[1].pVal = &(LocalAuditStruct.idEvent);
  pSS->FieldArray[2].pVal = &(LocalAuditStruct.bAuto);
  pSS->FieldArray[3].pVal = &(LocalAuditStruct.Recipient.idRecipient);
  pSS->FieldArray[4].pVal = &(LocalAuditStruct.Format.idFormat);
  pSS->FieldArray[5].pVal = &(LocalAuditStruct.idDelivery);
  pSS->FieldArray[6].pVal = Local_TableName;
  pSS->FieldArray[7].pVal = Local_sztAlarmDeclared;
  pSS->FieldArray[8].pVal = Local_sztAlarmExecuted;
  pSS->FieldArray[9].pVal = LocalAuditStruct.sInvocationString;
  pSS->FieldArray[10].pVal = &(LocalAuditStruct.idAudit);
  pSS->FieldArray[11].pVal = &(LocalAuditStruct.idCore);
  pSS->FieldArray[12].pVal = &(LocalAuditStruct.AlarmType);
  pSS->FieldArray[13].pVal = &(LocalAuditStruct.idGroup);
  pSS->FieldArray[14].pVal = &(LocalAuditStruct.idRecipientDelivery);

  if (ewdb_base_RequestCursor (szStatement, pSS, 0) != 0)
  {
    logit ("", "Call to ewdb_base_RequestCursor failed.\n");
    return EWDB_RETURN_FAILURE;
  }

  return EWDB_RETURN_SUCCESS;
}  


/******************* PrepCreateAlarmAuditExec *******************/
static int PrepCreateAlarmAuditExec (EWDB_AlarmAuditStruct *pAudit, EWDB_Cursor *ppCursor)
{

  if ((pAudit == NULL) || (ppCursor == NULL))
  {
    logit ("", "Null parameters passed in!\n");
    return EWDB_RETURN_FAILURE;
  }


  memcpy (&LocalAuditStruct, pAudit, sizeof (EWDB_AlarmAuditStruct));

  sprintf (Local_sztAlarmDeclared, "%f", pAudit->tAlarmDeclared);
  sprintf (Local_sztAlarmExecuted, "%f", pAudit->tAlarmExecuted);

  if (pAudit->DelMethodInd == EWDB_ALARMS_DELIVERY_IND_EMAIL)
    sprintf (Local_TableName, "email");
  else if (pAudit->DelMethodInd == EWDB_ALARMS_DELIVERY_IND_PAGER)
    sprintf (Local_TableName, "pager");
  else if (pAudit->DelMethodInd == EWDB_ALARMS_DELIVERY_IND_PHONE)
    sprintf (Local_TableName, "phone");
  else if (pAudit->DelMethodInd == EWDB_ALARMS_DELIVERY_IND_QDDS)
    sprintf (Local_TableName, "qdds");
  else if (pAudit->DelMethodInd == EWDB_ALARMS_DELIVERY_IND_CUSTOM)
    sprintf (Local_TableName, "custom");
  else
  {
    logit ("", "PrepCreateAlarmAuditExec(): ERROR: Unknown delivery method: %d.\n");
    return EWDB_RETURN_FAILURE;
  }

  SSStatement.NumOfFields = NUM_FIELDS;
  SSStatement.FieldArray = SQLParamsBindArray;
  SSStatement.RecordSize = 0;

  if (InitCreateAlarmAuditStatement(SQL_STRING, &SSStatement)
      != EWDB_RETURN_SUCCESS)
  {
    logit ("", "PrepCreateAlarmAuditExec(): InitCreateAlarmAuditStatement failed!\n");
    return EWDB_RETURN_FAILURE;
  }

  *ppCursor = SSStatement.pCda;
  
  return EWDB_RETURN_SUCCESS;
}  


/******************* PostCreateAlarmAuditExec *******************/
static int PostCreateAlarmAuditExec (EWDB_AlarmAuditStruct *pAudit)
{
  EWDB_Cursor pCursor;
  
  pCursor = SSStatement.pCda;
  
  ewdb_base_ReleaseCursor (pCursor);

  if (pAudit == NULL)
  {
    logit ("", "PostCreateAlarmAuditExec(): Invalid arguments passed in.\n");
    return EWDB_RETURN_FAILURE;
  }
  
  pAudit->idAudit = LocalAuditStruct.idAudit;
  
  if (LocalAuditStruct.idAudit <= 0)
  {
    logit("", "PostCreateAlarmAuditExec(): SQL Proc CreateOrUpdateAlarmsAudit() returned error %d.\n", 
          LocalAuditStruct.idAudit);
    return EWDB_RETURN_FAILURE;
  }
  
  return EWDB_RETURN_SUCCESS;
}  /* end PostCreateAlarmAuditExec() */
