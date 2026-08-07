/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_api_CreateCustomDelivery.c,v 1.4 2005/06/03 22:32:16 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_api_CreateCustomDelivery.c,v $
 *     Revision 1.4  2005/06/03 22:32:16  davidk
 *     DB API Cleanup
 *
 *     Revision 1.3  2001/08/07 16:51:06  lucky
 *     Pre v6.0 checkin
 *
 *     Revision 1.2  2001/07/31 20:44:40  lucky
 *     Changed from User to Recipient
 *
 *     Revision 1.1  2001/07/28 00:44:27  lucky
 *     Initial revision
 *
 *
 */


#include <ewdb_ora_api.h>
#include <ewdb_cli_base.h>
#include <ewdb_ew_oci_base.h>


static char SQL_STRING[] =
  "Begin CreateCustomDelivery (OUT_idDelivery => :OUT_idDelivery,"
  "OUT_idRecipientDelivery => :OUT_idRecipientDelivery, "
  "IN_idRecipient => :IN_idRecipient, "
  "IN_sDescription => :IN_sDescription, "
  "IN_bisAudit => :IN_bisAudit); End;";
  
static EWDB_OCI_SFS SQLParamsBindArray[] = 
{
  {0,1,0,0,0,OA_INT,    ":OUT_idDelivery"},
  {0,1,0,0,0,OA_INT,    ":OUT_idRecipientDelivery"},
  {0,1,0,0,0,OA_INT,    ":IN_idRecipient"},
  {0,1,256,0,0,OA_SZ,   ":IN_sDescription"},
  {0,1,0,0,0,OA_INT,    ":IN_bisAudit"},
};

#define  NUM_FIELDS  5


static  EWDB_OCIStatementStruct      SSStatement;
static  EWDB_CustomDeliveryStruct    Local_CustomDStruct;
static  int                          Local_idRecipientDelivery;
static  int                          Local_idRecipient;
static  int                          Local_bisAudit;


static int PrepCreateCustomExec(EWDB_CustomDeliveryStruct *pCustom, 
                                EWDBid IN_idRecipient, int IN_bIsAudit, 
                                EWDB_Cursor *ppCursor);
static int PostCreateCustomExec (int *, EWDB_CustomDeliveryStruct *);
static int InitCreateCustomStatement (char *, EWDB_OCIStatementStruct *);


int ewdb_api_CreateCustomDelivery (EWDBid *pidRecipientDelivery, 
                                   EWDBid IN_idRecipient, 
                                   EWDB_CustomDeliveryStruct *pCustom, 
                                   int IN_bIsAudit)
{

  EWDB_Cursor  pCursor;

  if ((pCustom == NULL) || (pidRecipientDelivery == NULL))
  {
    logit ("", "ewdb_api_CreateCustomDelivery(): Null parameters passed in!\n");
    return EWDB_RETURN_FAILURE;
  }

  ewdb_base_SetLastOraAPIActionTime ();

  if (ewdb_base_Reconnect () != EWDB_RETURN_SUCCESS)
  {
    logit ("", "ewdb_api_CreateCustomDelivery():Could not reconnect to the database!\n");
    return (EWDB_RETURN_FAILURE);
  }

  if (PrepCreateCustomExec (pCustom, IN_idRecipient, IN_bIsAudit, &pCursor) != EWDB_RETURN_SUCCESS)
  {
    logit ("", "ewdb_api_CreateCustomDelivery(): PrepCreateCustomExec failed.\n");
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  }

  if (ewdb_base_SQLExecute (pCursor))
  {
    ewdb_base_ErrorReport (hEWDBC, pCursor,"ewdb_api_CreateCustomDelivery(): ewdb_base_SQLExecute", 1);
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  } 

    /* Commit the transaction (all the previous inserts!)
     In Case there is any auditing or logging or debug
     changes made in the stored procedures.
  ****************************************************/
  if (ewdb_base_SQLCommit (hEWDBC))
  {
    ewdb_base_ErrorReport (hEWDBC, hEWDBC, "ewdb_api_CreateCustomDelivery(): ewdb_base_SQLCommit",2);
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  }

  if (PostCreateCustomExec (pidRecipientDelivery, pCustom) != EWDB_RETURN_SUCCESS)
  {
    logit ("", "ewdb_api_CreateCustomDelivery(): PostCreateCustomExec failed!\n");
    return(EWDB_RETURN_FAILURE);
  }

  return EWDB_RETURN_SUCCESS;
}  /* end ewdb_api_CreateCustomDelivery() */


/******************* InitCreateCustomStatement *******************/
static int InitCreateCustomStatement(char *szStatement, EWDB_OCIStatementStruct *pSS)
{

  if ((szStatement == NULL) ||  (pSS == NULL))
  {
    logit ("", "InitCreateCustomStatement(): Null parameters passed in!\n");
    return EWDB_RETURN_FAILURE;
  }

  pSS->FieldArray[0].pVal = &(Local_CustomDStruct.idDelivery);
  pSS->FieldArray[1].pVal = &Local_idRecipientDelivery;
  pSS->FieldArray[2].pVal = &Local_idRecipient;
  pSS->FieldArray[3].pVal = (Local_CustomDStruct.sDescription);
  pSS->FieldArray[4].pVal = &Local_bisAudit;

  if (ewdb_base_RequestCursor (szStatement, pSS, 0) != 0)
  {
    logit ("", "InitCreateCustomStatement(): ewdb_base_RequestCursor failed.\n");
    return EWDB_RETURN_FAILURE;
  }

  return EWDB_RETURN_SUCCESS;
}  /* end InitCreateCustomStatement() */


/******************* PrepCreateCustomExec *******************/
static int PrepCreateCustomExec(EWDB_CustomDeliveryStruct *pCustom, 
                                EWDBid IN_idRecipient, int IN_bIsAudit, 
                                EWDB_Cursor *ppCursor)
{

  if ((pCustom == NULL) || (ppCursor == NULL) || (IN_idRecipient < 0))
  {
    logit ("", "PrepCreateCustomExec(): Invalid parameters passed in!\n");
    return EWDB_RETURN_FAILURE;
  }

  memcpy (&Local_CustomDStruct, pCustom, sizeof (EWDB_CustomDeliveryStruct));
  Local_idRecipient = IN_idRecipient;
  Local_bisAudit    = IN_bIsAudit;

  Local_idRecipientDelivery = 0;

  SSStatement.NumOfFields = NUM_FIELDS;
  SSStatement.FieldArray = SQLParamsBindArray;
  SSStatement.RecordSize = 0;

  if (InitCreateCustomStatement (SQL_STRING, &SSStatement)
                             != EWDB_RETURN_SUCCESS)
  {
    logit ("", "PrepCreateCustomExec(): Call to InitCreateStatement failed!\n");
    return EWDB_RETURN_FAILURE;
  }

  *ppCursor = SSStatement.pCda;
  
  return EWDB_RETURN_SUCCESS;
}  /* end PrepCreateCustomExec() */


/******************* PostCreateCustomExec *******************/
static int PostCreateCustomExec (int *pidRecipientDelivery, EWDB_CustomDeliveryStruct *pCustom)
{
  EWDB_Cursor pCursor;
  
  pCursor = SSStatement.pCda;

  if (pCustom == NULL)
  {
    logit ("", "PostCreateCustomExec(): Invalid arguments passed in.\n");
    ewdb_base_ReleaseCursor (pCursor);
    return EWDB_RETURN_FAILURE;
  }
  
  pCustom->idDelivery = Local_CustomDStruct.idDelivery;
  *pidRecipientDelivery = Local_idRecipientDelivery;
  
  ewdb_base_ReleaseCursor (pCursor);
  
  if(Local_CustomDStruct.idDelivery <= 0 || Local_idRecipientDelivery <= 0)
    return(EWDB_RETURN_WARNING);

  return EWDB_RETURN_SUCCESS;
}  /* end PostCreateCustomExec() */
