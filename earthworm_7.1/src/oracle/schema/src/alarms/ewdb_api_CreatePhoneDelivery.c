/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_api_CreatePhoneDelivery.c,v 1.6 2005/06/03 22:32:16 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_api_CreatePhoneDelivery.c,v $
 *     Revision 1.6  2005/06/03 22:32:16  davidk
 *     DB API Cleanup
 *
 *     Revision 1.5  2004/12/07 23:28:23  mark
 *     Fixed updates to search based on idDelivery
 *
 *     Revision 1.4  2001/08/07 16:51:06  lucky
 *     Pre v6.0 checkin
 *
 *     Revision 1.3  2001/07/31 20:44:40  lucky
 *     Changed from User to Recipient
 *
 *     Revision 1.2  2001/07/28 00:44:27  lucky
 *      State of the code after debugging and testing prior to v6.0 release freeze.
 *
 *     Revision 1.1  2001/05/15 02:16:13  davidk
 *     Initial revision
 *
 *
 */


#include <ewdb_ora_api.h>
#include <ewdb_cli_base.h>
#include <ewdb_ew_oci_base.h>


static char SQL_STRING[] =
  "Begin CreatePhoneDelivery (OUT_idDelivery => :OUT_idDelivery,"
  "                           OUT_idRecipientDelivery => :OUT_idRecipientDelivery, "
  "                           IN_idRecipient => :IN_idRecipient, "
  "                           IN_sPhoneNumber => :IN_sPhoneNumber, "
  "                           IN_bIsAudit => :IN_bIsAudit, "
  "                           IN_idDelivery => :IN_idDelivery); End;";
  
static EWDB_OCI_SFS SQLParamsBindArray[] = 
{
  {0,1,0,0,0,OA_INT,    ":OUT_idDelivery"},
  {0,1,0,0,0,OA_INT,    ":OUT_idRecipientDelivery"},
  {0,1,0,0,0,OA_INT,    ":IN_idRecipient"},
  {0,1,256,0,0,OA_SZ,   ":IN_sPhoneNumber"},
  {0,1,0,0,0,OA_INT,    ":IN_bIsAudit"},
  {0,1,0,0,0,OA_INT,    ":IN_idDelivery"},
};

#define  NUM_FIELDS  6


static  EWDB_OCIStatementStruct     SSStatement;
static  EWDB_PhoneDeliveryStruct    LocalPhoneStruct;
static  int                         Local_idRecipientDelivery;
static  int                         Local_idRecipient;
static  int                         Local_bIsAudit;
static  int                         Local_idDelivery;


static int PrepCreatePhoneExec (EWDB_PhoneDeliveryStruct *pPhone, 
                                EWDBid IN_idRecipient, int IN_bIsAudit,
                                EWDB_Cursor *ppCursor);
static int PostCreatePhoneExec (int *, EWDB_PhoneDeliveryStruct *);
static int InitCreatePhoneStatement (char *, EWDB_OCIStatementStruct *);


int ewdb_api_CreatePhoneDelivery(int *pidRecipientDelivery, 
                                 EWDBid IN_idRecipient, 
                                 EWDB_PhoneDeliveryStruct *pPhone, int IN_bIsAudit)
{

  EWDB_Cursor  pCursor;

  if ((pPhone == NULL) || (pidRecipientDelivery == NULL))
  {
    logit ("", "ewdb_api_CreatePhoneDelivery(): Null parameters passed in!\n");
    return EWDB_RETURN_FAILURE;
  }

  ewdb_base_SetLastOraAPIActionTime ();

  if (ewdb_base_Reconnect () != EWDB_RETURN_SUCCESS)
  {
    logit ("", "ewdb_api_CreatePhoneDelivery(): Could not reconnect to the database!\n");
    return (EWDB_RETURN_FAILURE);
  }

  if (PrepCreatePhoneExec (pPhone, IN_idRecipient, IN_bIsAudit, &pCursor) != EWDB_RETURN_SUCCESS)
  {
    logit ("", "ewdb_api_CreatePhoneDelivery(): PrepCreatePhoneExec failed.\n");
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  }

  if (ewdb_base_SQLExecute (pCursor))
  {
    ewdb_base_ErrorReport (hEWDBC, pCursor,"ewdb_api_CreatePhoneDelivery(): ewdb_base_SQLExecute", 1);
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  } 
  
  /* Commit the transaction (all the previous inserts!)
     In Case there is any auditing or logging or debug
     changes made in the stored procedures.
  ****************************************************/
  if (ewdb_base_SQLCommit (hEWDBC))
  {
    ewdb_base_ErrorReport (hEWDBC, hEWDBC, "ewdb_api_CreatePhoneDelivery(): ewdb_base_SQLCommit",2);
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  }

  if (PostCreatePhoneExec (pidRecipientDelivery, pPhone) != EWDB_RETURN_SUCCESS)
  {
    logit ("", "ewdb_api_CreatePhoneDelivery(): PostCreatePhoneExec failed!\n");
    return (EWDB_RETURN_FAILURE);
  }

  return EWDB_RETURN_SUCCESS;
} /* end ewdb_api_CreatePhoneDelivery() */


/******************* InitCreatePhoneStatement *******************/
static int InitCreatePhoneStatement(char *szStatement, EWDB_OCIStatementStruct *pSS)
{

  if ((szStatement == NULL) ||  (pSS == NULL))
  {
    logit ("", "InitCreatePhoneStatement(): Null parameters passed in!\n");
    return EWDB_RETURN_FAILURE;
  }

  pSS->FieldArray[0].pVal = &(LocalPhoneStruct.idDelivery);
  pSS->FieldArray[1].pVal = &Local_idRecipientDelivery;
  pSS->FieldArray[2].pVal = &Local_idRecipient;
  pSS->FieldArray[3].pVal = (LocalPhoneStruct.sPhoneNumber);
  pSS->FieldArray[4].pVal = &Local_bIsAudit;
  pSS->FieldArray[5].pVal = &Local_idDelivery;

  if (ewdb_base_RequestCursor (szStatement, pSS, 0) != 0)
  {
    logit ("", "InitCreatePhoneStatement(): Call to ewdb_base_RequestCursor failed.\n");
    return EWDB_RETURN_FAILURE;
  }

  return EWDB_RETURN_SUCCESS;
}  


/******************* PrepCreatePhoneExec *******************/
static int PrepCreatePhoneExec (EWDB_PhoneDeliveryStruct *pPhone, 
                                EWDBid IN_idRecipient, int IN_bIsAudit,
                                EWDB_Cursor *ppCursor)
{

  if ((pPhone == NULL) || (ppCursor == NULL) || (IN_idRecipient < 0))
  {
    logit ("", "Invalid parameters passed in!\n");
    return EWDB_RETURN_FAILURE;
  }

  memcpy (&LocalPhoneStruct, pPhone, sizeof (EWDB_PhoneDeliveryStruct));
  Local_idRecipientDelivery = 0;

  Local_idRecipient = IN_idRecipient;
  Local_bIsAudit    = IN_bIsAudit;
  Local_idDelivery  = pPhone->idDelivery;

  if (InitCreatePhoneStatement (SQL_STRING, &SSStatement)
                             != EWDB_RETURN_SUCCESS)
  {
    logit ("", "Call to InitCreatePhoneStatement failed!\n");
    return EWDB_RETURN_FAILURE;
  }

  *ppCursor = SSStatement.pCda;
  
  return EWDB_RETURN_SUCCESS;
}  


/******************* PostCreatePhoneExec *******************/
static int PostCreatePhoneExec (int *pidRecipientDelivery, EWDB_PhoneDeliveryStruct *pPhone)
{
  EWDB_Cursor pCursor;
  
  if (pPhone == NULL)
  {
    logit ("", "PostCreatePhoneExec(): Invalid arguments passed in.\n");
    return EWDB_RETURN_FAILURE;
  }
  
  pPhone->idDelivery    = LocalPhoneStruct.idDelivery;
  *pidRecipientDelivery = Local_idRecipientDelivery;
  pCursor = SSStatement.pCda;
  
  ewdb_base_ReleaseCursor (pCursor);

  if(Local_idRecipientDelivery <= 0 || LocalPhoneStruct.idDelivery <= 0)
    return(EWDB_RETURN_WARNING);
  
  return EWDB_RETURN_SUCCESS;
} 
