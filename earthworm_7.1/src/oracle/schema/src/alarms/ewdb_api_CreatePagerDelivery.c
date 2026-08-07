/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_api_CreatePagerDelivery.c,v 1.7 2005/06/03 22:32:16 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_api_CreatePagerDelivery.c,v $
 *     Revision 1.7  2005/06/03 22:32:16  davidk
 *     DB API Cleanup
 *
 *     Revision 1.6  2004/12/07 23:28:23  mark
 *     Fixed updates to search based on idDelivery
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
 *     Revision 1.2  2001/05/21 18:44:21  lucky
 *     *** empty log message ***
 *
 *
 */


#include <ewdb_ora_api.h>
#include <ewdb_cli_base.h>
#include <ewdb_ew_oci_base.h>


static char SQL_STRING[] =
 "Begin CreatePagerDelivery (OUT_idDelivery => :OUT_idDelivery,"
 "OUT_idRecipientDelivery => :OUT_idRecipientDelivery, "
 "IN_idRecipient => :IN_idRecipient, "
 "IN_sNumber => :IN_sNumber, "
 "IN_sPagerCompany => :In_sPagerCompany, "
 "IN_bIsAudit => :IN_bIsAudit, "
 "IN_idDelivery => :IN_idDelivery); End;";
 
static EWDB_OCI_SFS SQLParamsBindArray[] = 
{
  {0,1,0,0,0,OA_INT,    ":OUT_idDelivery"},
  {0,1,0,0,0,OA_INT,    ":OUT_idRecipientDelivery"},
  {0,1,0,0,0,OA_INT,    ":IN_idRecipient"},
  {0,1,256,0,0,OA_SZ,   ":IN_sNumber"},
  {0,1,256,0,0,OA_SZ,   ":IN_sPagerCompany"},
  {0,1,0,0,0,OA_INT,    ":IN_bIsAudit"},
  {0,1,0,0,0,OA_INT,    ":IN_idDelivery"},
};

#define NUM_FIELDS 7


static  EWDB_OCIStatementStruct   SSStatement;
static  EWDB_PagerDeliveryStruct  Local_PagerStruct;
static int      Local_idRecipientDelivery;
static int      Local_idRecipient;
static int      Local_bIsAudit;
static int      Local_idDelivery;


static int PrepCreatePagerExec (EWDB_PagerDeliveryStruct *pPager, EWDBid IN_idRecipient, 
                                int IN_bIsAudit, EWDB_Cursor *ppCursor);
static int PostCreatePagerExec (int *, EWDB_PagerDeliveryStruct *);
static int InitCreatePagerStatement (char *, EWDB_OCIStatementStruct *);


int ewdb_api_CreatePagerDelivery (int *pidRecipientDelivery, 
                                  EWDBid IN_idRecipient, 
                                  EWDB_PagerDeliveryStruct *pPager, 
                                  int IN_bIsAudit)
{
  
  EWDB_Cursor pCursor;
  
  if ((pPager == NULL) || (pidRecipientDelivery == NULL))
  {
    logit ("", "ewdb_api_CreatePagerDelivery(): Null parameters passed in!\n");
    return EWDB_RETURN_FAILURE;
  }
  
  ewdb_base_SetLastOraAPIActionTime ();
  
  if (ewdb_base_Reconnect () != EWDB_RETURN_SUCCESS)
  {
    logit ("", "ewdb_api_CreatePagerDelivery(): Could not reconnect to the database!\n");
    return (EWDB_RETURN_FAILURE);
  }
  
  if (PrepCreatePagerExec (pPager, IN_idRecipient, IN_bIsAudit, &pCursor) != EWDB_RETURN_SUCCESS)
  {
    logit ("", "ewdb_api_CreatePagerDelivery(): PrepCreatePagerExec failed.\n");
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  }
  
  if (ewdb_base_SQLExecute (pCursor))
  {
    ewdb_base_ErrorReport (hEWDBC, pCursor,"ewdb_api_CreatePagerDelivery(): ewdb_base_SQLExecute", 1);
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  } 
  
  /* Commit the transaction (all the previous inserts!)
  In Case there is any auditing or logging or debug
  changes made in the stored procedures.
  ****************************************************/
  if (ewdb_base_SQLCommit (hEWDBC))
  {
    ewdb_base_ErrorReport (hEWDBC, hEWDBC, "ewdb_api_CreatePagerDelivery(): ewdb_base_SQLCommit",2);
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  }
  
  if (PostCreatePagerExec (pidRecipientDelivery, pPager) != EWDB_RETURN_SUCCESS)
  {
    logit ("", "ewdb_api_CreatePagerDelivery(): PostCreatePagerExec failed!\n");
    return (EWDB_RETURN_FAILURE);
  }
  
  return EWDB_RETURN_SUCCESS;
}  /* end ewdb_api_CreatePagerDelivery() */


/******************* InitCreatePagerStatement *******************/
static int InitCreatePagerStatement(char *szStatement, EWDB_OCIStatementStruct *pSS)
{
  
  if ((szStatement == NULL) ||  (pSS == NULL))
  {
    logit ("", "InitCreatePagerStatement(): Null parameters passed in!\n");
    return EWDB_RETURN_FAILURE;
  }
  
  pSS->FieldArray[0].pVal = &(Local_PagerStruct.idDelivery);
  pSS->FieldArray[1].pVal = &Local_idRecipientDelivery;
  pSS->FieldArray[2].pVal = &Local_idRecipient;
  pSS->FieldArray[3].pVal = (Local_PagerStruct.sNumber);
  pSS->FieldArray[4].pVal = (Local_PagerStruct.sPagerCompany);
  pSS->FieldArray[5].pVal = &Local_bIsAudit;
  pSS->FieldArray[6].pVal = &(Local_PagerStruct.idDelivery);
  
  if (ewdb_base_RequestCursor (szStatement, pSS, 0) != 0)
  {
    logit ("", "InitCreatePagerStatement(): ewdb_base_RequestCursor failed.\n");
    return EWDB_RETURN_FAILURE;
  }
  
  return EWDB_RETURN_SUCCESS;
}  /* end InitCreatePagerStatement() */


/******************* PrepCreatePagerExec *******************/
static int PrepCreatePagerExec (EWDB_PagerDeliveryStruct *pPager, EWDBid IN_idRecipient, 
                                int IN_bIsAudit, EWDB_Cursor *ppCursor)
{

  if ((pPager == NULL) || (ppCursor == NULL) || (IN_idRecipient < 0))
  {
    logit ("", "PrepCreatePagerExec(): Invalid parameters passed in!\n");
    return EWDB_RETURN_FAILURE;
  }
  
  memcpy (&Local_PagerStruct, pPager, sizeof (EWDB_PagerDeliveryStruct));
  Local_idRecipient = IN_idRecipient;
  Local_idRecipientDelivery = 0;
  Local_idDelivery = pPager->idDelivery;
  Local_bIsAudit   = IN_bIsAudit;
  
  SSStatement.NumOfFields = NUM_FIELDS;
  SSStatement.FieldArray = SQLParamsBindArray;
  SSStatement.RecordSize = 0;
  
  if (InitCreatePagerStatement (SQL_STRING, &SSStatement)
    != EWDB_RETURN_SUCCESS)
  {
    logit ("", "PrepCreatePagerExec(): InitCreatePagerStatement failed!\n");
    return EWDB_RETURN_FAILURE;
  }
  
  *ppCursor = SSStatement.pCda;
  
  return EWDB_RETURN_SUCCESS;
}  /* end PrepCreatePagerExec() */


/******************* PostCreatePagerExec *******************/
static int PostCreatePagerExec (int *pidRecipientDelivery, EWDB_PagerDeliveryStruct *pPager)
{
  EWDB_Cursor pCursor;
  
  pCursor = SSStatement.pCda;

  if (pPager == NULL)
  {
    logit ("", "PostCreatePagerExec(): Invalid arguments passed in.\n");
    ewdb_base_ReleaseCursor (pCursor);
    return EWDB_RETURN_FAILURE;
  }
  
  pPager->idDelivery = Local_PagerStruct.idDelivery;
  *pidRecipientDelivery = Local_idRecipientDelivery;
  
  ewdb_base_ReleaseCursor (pCursor);
  
  if(Local_PagerStruct.idDelivery <= 0 || Local_idRecipientDelivery <= 0 )
    return(EWDB_RETURN_WARNING);
 
  return EWDB_RETURN_SUCCESS;
} 
