/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_api_CreateEmailDelivery.c,v 1.8 2005/06/03 22:32:16 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_api_CreateEmailDelivery.c,v $
 *     Revision 1.8  2005/06/03 22:32:16  davidk
 *     DB API Cleanup
 *
 *     Revision 1.7  2004/12/07 23:28:23  mark
 *     Fixed updates to search based on idDelivery
 *
 *     Revision 1.6  2004/05/11 21:00:23  michelle
 *     changed to use CreateOrUpdateEmailDelivery stored procedure
 *     so that you can update email servers
 *
 *     Revision 1.5  2001/08/07 16:51:06  lucky
 *     Pre v6.0 checkin
 *
 *     Revision 1.4  2001/07/31 20:44:40  lucky
 *     Changed from User to Recipient
 *
 *     Revision 1.3  2001/07/28 00:44:27  lucky
 *      State of the code after debugging and testing prior to v6.0 release freeze.
 *
 *     Revision 1.2  2001/05/15 18:47:41  davidk
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
  "Begin CreateOrUpdateEmailDelivery (OUT_idDelivery => :OUT_idDelivery,"
  "OUT_idRecipientDelivery => :OUT_idRecipientDelivery, "
  "IN_idRecipient => :IN_idRecipient, "
  "IN_sAddress => :IN_sAddress, "
  "IN_sMailServer => :In_sMailServer, "
  "IN_bisAudit => :IN_bisAudit, "
  "IN_idDelivery => :IN_idDelivery); End;";
  
static EWDB_OCI_SFS SQLParamsBindArray[] = 
{
  {0,1,0,0,0,OA_INT,    ":OUT_idDelivery"},
  {0,1,0,0,0,OA_INT,    ":OUT_idRecipientDelivery"},
  {0,1,0,0,0,OA_INT,    ":IN_idRecipient"},
  {0,1,256,0,0,OA_SZ,   ":IN_sAddress"},
  {0,1,256,0,0,OA_SZ,   ":IN_sMailServer"},
  {0,1,0,0,0,OA_INT,    ":IN_bisAudit"},
  {0,1,0,0,0,OA_INT,    ":IN_idDelivery"},
};

#define  NUM_FIELDS  7


static  EWDB_OCIStatementStruct     SSStatement
;
static  EWDB_EmailDeliveryStruct    Local_EmailStruct;
static  EWDBid                      Local_idRecipientDelivery;
static  EWDBid                      Local_idRecipient;
static  EWDBid                      Local_idDelivery;
static  EWDBid                      Local_bIsAudit;


static int PrepCreateEmailExec (EWDB_EmailDeliveryStruct *pEmail, EWDBid IN_idRecipient,
                                int IN_bIsAudit, EWDB_Cursor *ppCursor);
static int PostCreateEmailExec (EWDBid *pidRecipientDelivery, EWDB_EmailDeliveryStruct *pEmail);
static int InitCreateEmailStatement (char *, EWDB_OCIStatementStruct *);


int ewdb_api_CreateEmailDelivery (EWDBid *pidRecipientDelivery, 
                                  EWDBid IN_idRecipient, 
                                  EWDB_EmailDeliveryStruct *pEmail, int IN_bIsAudit)
{

  EWDB_Cursor  pCursor;

  if ((pEmail == NULL) || (pidRecipientDelivery == NULL))
  {
    logit ("", "ewdb_api_CreateEmailDelivery(): Null parameters passed in!\n");
    return EWDB_RETURN_FAILURE;
  }

  ewdb_base_SetLastOraAPIActionTime ();

  if (ewdb_base_Reconnect () != EWDB_RETURN_SUCCESS)
  {
    logit ("", "ewdb_api_CreateEmailDelivery(): Could not reconnect to the database!\n");
    return (EWDB_RETURN_FAILURE);
  }

  if (PrepCreateEmailExec (pEmail, IN_idRecipient, IN_bIsAudit, &pCursor) != EWDB_RETURN_SUCCESS)
  {
    logit ("", "ewdb_api_CreateEmailDelivery(): PrepCreateEmailExec failed.\n");
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  }

  if (ewdb_base_SQLExecute (pCursor))
  {
    ewdb_base_ErrorReport (hEWDBC, pCursor,"ewdb_api_CreateEmailDelivery(): ewdb_base_SQLExecute", 1);
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  } 

  /* Commit the transaction (all the previous inserts!)
     In Case there is any auditing or logging or debug
     changes made in the stored procedures.
  ****************************************************/
  if (ewdb_base_SQLCommit (hEWDBC))
  {
    ewdb_base_ErrorReport (hEWDBC, hEWDBC, "ewdb_api_CreateEmailDelivery(): ewdb_base_SQLCommit",2);
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  }

  if (PostCreateEmailExec (pidRecipientDelivery, pEmail) != EWDB_RETURN_SUCCESS)
  {
    logit("", "ewdb_api_CreateEmailDelivery(): PostCreateEmailExec failed!\n");
    return(EWDB_RETURN_FAILURE);
  }

  return EWDB_RETURN_SUCCESS;
}  /* end ewdb_api_CreateEmailDelivery(): () */


/******************* InitCreateEmailStatement *******************/
static int InitCreateEmailStatement(char *szStatement, EWDB_OCIStatementStruct *pSS)
{

  if ((szStatement == NULL) ||  (pSS == NULL))
  {
    logit ("", "InitCreateEmailStatement(): Null parameters passed in!\n");
    return EWDB_RETURN_FAILURE;
  }

  pSS->FieldArray[0].pVal = &(Local_EmailStruct.idDelivery);
  pSS->FieldArray[1].pVal = &Local_idRecipientDelivery;
  pSS->FieldArray[2].pVal = &Local_idRecipient;
  pSS->FieldArray[3].pVal = (Local_EmailStruct.sAddress);
  pSS->FieldArray[4].pVal = (Local_EmailStruct.sMailServer);
  pSS->FieldArray[5].pVal = &Local_bIsAudit;
  pSS->FieldArray[6].pVal = &Local_idDelivery;

  if (ewdb_base_RequestCursor (szStatement, pSS, 0) != 0)
  {
    logit ("", "InitCreateEmailStatement(): ewdb_base_RequestCursor failed.\n");
    return EWDB_RETURN_FAILURE;
  }

  return EWDB_RETURN_SUCCESS;
}  /* end InitCreateEmailStatement() */


/******************* PrepCreateEmailExec *******************/
static int PrepCreateEmailExec (EWDB_EmailDeliveryStruct *pEmail, EWDBid IN_idRecipient,
                                int IN_bIsAudit, EWDB_Cursor *ppCursor)
{

  if ((pEmail == NULL) || (ppCursor == NULL) || (IN_idRecipient < 0))
  {
    logit ("", "PrepCreateEmailExec(): Invalid parameters passed in!\n");
    return EWDB_RETURN_FAILURE;
  }

  memcpy (&Local_EmailStruct, pEmail, sizeof (EWDB_EmailDeliveryStruct));
  Local_idRecipient = IN_idRecipient;
  Local_idDelivery  = pEmail->idDelivery;
  Local_bIsAudit    = IN_bIsAudit;

  Local_idRecipientDelivery = 0;

  SSStatement.NumOfFields = NUM_FIELDS;
  SSStatement.FieldArray = SQLParamsBindArray;
  SSStatement.RecordSize = 0;

  if (InitCreateEmailStatement (SQL_STRING, &SSStatement)
                             != EWDB_RETURN_SUCCESS)
  {
    logit ("", "PrepCreateEmailExec(): InitCreateEmailStatement failed!\n");
    return EWDB_RETURN_FAILURE;
  }

  *ppCursor = SSStatement.pCda;
  
  return EWDB_RETURN_SUCCESS;
}  /* end PrepCreateEmailExec() */


/******************* PostCreateEmailExec *******************/
static int PostCreateEmailExec (EWDBid *pidRecipientDelivery, EWDB_EmailDeliveryStruct *pEmail)
{
  EWDB_Cursor pCursor;
  
  pCursor = SSStatement.pCda;
  
  ewdb_base_ReleaseCursor (pCursor);

  if (pEmail == NULL)
  {
    logit ("", "PostCreateEmailExec(): Invalid arguments passed in.\n");
    return EWDB_RETURN_FAILURE;
  }
  
  pEmail->idDelivery = Local_EmailStruct.idDelivery;
  *pidRecipientDelivery = Local_idRecipientDelivery;

  if(Local_EmailStruct.idDelivery <= 0 || Local_idRecipientDelivery <= 0)
    return(EWDB_RETURN_WARNING);
  
  return EWDB_RETURN_SUCCESS;
}  /* end PostCreateEmailExec() */
