/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_api_CreateQddsDelivery.c,v 1.5 2005/06/03 22:32:16 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_api_CreateQddsDelivery.c,v $
 *     Revision 1.5  2005/06/03 22:32:16  davidk
 *     DB API Cleanup
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
 *     Revision 1.1  2001/06/26 17:24:13  lucky
 *     Initial revision
 *
 *
 */


#include <ewdb_ora_api.h>
#include <ewdb_cli_base.h>
#include <ewdb_ew_oci_base.h>


static char SQL_STRING[] =
  "Begin CreateQddsDelivery(OUT_idDelivery => :OUT_idDelivery,"
  "                         OUT_idRecipientDelivery => :OUT_idRecipientDelivery, "
  "                         IN_idRecipient => :IN_idRecipient, "
  "                         IN_sQddsDirectory => :IN_sQddsDirectory, "
  "                         IN_bisAudit => :IN_bisAudit); End;";
  
static EWDB_OCI_SFS SQLParamsBindArray[] = 
{
  {0,1,0,0,0,OA_INT,    ":OUT_idDelivery"},
  {0,1,0,0,0,OA_INT,    ":OUT_idRecipientDelivery"},
  {0,1,0,0,0,OA_INT,    ":IN_idRecipient"},
  {0,1,256,0,0,OA_SZ,   ":IN_sQddsDirectory"},
  {0,1,0,0,0,OA_INT,    ":IN_bisAudit"},
};

#define	NUM_FIELDS	5


static  EWDB_OCIStatementStruct   	SSStatement;

static  EWDB_QddsDeliveryStruct		Local_QddsStruct;
static	int	                      Local_idRecipientDelivery;
static	int	                      Local_idRecipient;
static	int	                      Local_bIsAudit;


static int PrepCreateQddsExec(EWDB_QddsDeliveryStruct *pQdds, 
                              EWDBid IN_idRecipient, int IN_bIsAudit,
                              EWDB_Cursor *ppCursor);
static int PostCreateQddsExec (int *, EWDB_QddsDeliveryStruct *);
static int InitCreateQddsStatement (char *, EWDB_OCIStatementStruct *);


int ewdb_api_CreateQddsDelivery(EWDBid *pidRecipientDelivery, 
                                EWDBid IN_idRecipient, 
                                EWDB_QddsDeliveryStruct *pQdds, int IN_bIsAudit)
{

	EWDB_Cursor	pCursor;

	if ((pQdds == NULL) || (pidRecipientDelivery == NULL))
  {
  	logit ("", "ewdb_api_CreateQddsDelivery(): Null parameters passed in!\n");
  	return EWDB_RETURN_FAILURE;
  }

	ewdb_base_SetLastOraAPIActionTime ();

	if (ewdb_base_Reconnect () != EWDB_RETURN_SUCCESS)
  {
  	logit ("", "ewdb_api_CreateQddsDelivery(): Could not reconnect to the database!\n");
  	return (EWDB_RETURN_FAILURE);
  }

	if (PrepCreateQddsExec (pQdds, IN_idRecipient, IN_bIsAudit, &pCursor) != EWDB_RETURN_SUCCESS)
  {
  	logit ("", "ewdb_api_CreateQddsDelivery(): PrepCreateQddsExec failed.\n");
  	return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  }

	if (ewdb_base_SQLExecute (pCursor))
  {
  	ewdb_base_ErrorReport (hEWDBC, pCursor,"ewdb_api_CreateQddsDelivery(): ewdb_base_SQLExecute", 1);
  	return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  } 
  
  /* Commit the transaction (all the previous inserts!)
     In Case there is any auditing or logging or debug
     changes made in the stored procedures.
  ****************************************************/
	if (ewdb_base_SQLCommit (hEWDBC))
  {
  	ewdb_base_ErrorReport (hEWDBC, hEWDBC, "ewdb_api_CreateQddsDelivery(): ewdb_base_SQLCommit",2);
  	return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  }

	if (PostCreateQddsExec (pidRecipientDelivery, pQdds) != EWDB_RETURN_SUCCESS)
  {
  	logit ("", "ewdb_api_CreateQddsDelivery(): PostCreateQddsExec failed!\n");
  	return (EWDB_RETURN_FAILURE);
  }

	return EWDB_RETURN_SUCCESS;
}  /* end ewdb_api_CreateQddsDelivery() */


/******************* InitCreateQddsStatement *******************/
static int InitCreateQddsStatement(char *szStatement, EWDB_OCIStatementStruct *pSS)
{

	if ((szStatement == NULL) ||  (pSS == NULL))
  {
  	logit ("", "InitCreateQddsStatement(): Null parameters passed in!\n");
  	return EWDB_RETURN_FAILURE;
  }

	pSS->FieldArray[0].pVal = &(Local_QddsStruct.idDelivery);
	pSS->FieldArray[1].pVal = &Local_idRecipientDelivery;
	pSS->FieldArray[2].pVal = &Local_idRecipient;
	pSS->FieldArray[3].pVal = (Local_QddsStruct.sQddsDirectory);
	pSS->FieldArray[4].pVal = &Local_bIsAudit;

	if (ewdb_base_RequestCursor (szStatement, pSS, 0) != 0)
  {
  	logit ("", "InitCreateQddsStatement(): ewdb_base_RequestCursor failed.\n");
  	return EWDB_RETURN_FAILURE;
  }

	return EWDB_RETURN_SUCCESS;
}  


/******************* PrepCreateQddsExec *******************/
static int PrepCreateQddsExec(EWDB_QddsDeliveryStruct *pQdds, 
                              EWDBid IN_idRecipient, int IN_bIsAudit,
                              EWDB_Cursor *ppCursor)
{

	if ((pQdds == NULL) || (ppCursor == NULL) || (IN_idRecipient < 0))
  {
  	logit ("", "PrepCreateQddsExec(): Invalid parameters passed in!\n");
  	return EWDB_RETURN_FAILURE;
  }

	memcpy (&Local_QddsStruct, pQdds, sizeof (EWDB_QddsDeliveryStruct));
	Local_idRecipientDelivery   = 0;

  Local_idRecipient = IN_idRecipient;
  Local_bIsAudit    = IN_bIsAudit;

	SSStatement.NumOfFields = NUM_FIELDS;
	SSStatement.FieldArray = SQLParamsBindArray;
	SSStatement.RecordSize = 0;

	if (InitCreateQddsStatement (SQL_STRING, &SSStatement)
                             != EWDB_RETURN_SUCCESS)
  {
  	logit ("", "Call to InitCreateStatement failed!\n");
  	return EWDB_RETURN_FAILURE;
  }

  *ppCursor = SSStatement.pCda;
  
	return EWDB_RETURN_SUCCESS;
}  /* end PrepCreateQddsExec() */


/******************* PostCreateQddsExec *******************/
static int PostCreateQddsExec (int *pidRecipientDelivery, EWDB_QddsDeliveryStruct *pQdds)
{
	EWDB_Cursor pCursor;
  
	if (pQdds == NULL)
  {
  	logit ("", "Invalid arguments passed in.\n");
  	return EWDB_RETURN_FAILURE;
  }
  
	pQdds->idDelivery = Local_QddsStruct.idDelivery;
  *pidRecipientDelivery = Local_idRecipientDelivery;

	pCursor = SSStatement.pCda;
  
	ewdb_base_ReleaseCursor (pCursor);

  if(Local_QddsStruct.idDelivery < 0 || 
     Local_idRecipientDelivery < 0)
     return(EWDB_RETURN_WARNING);
  
	return EWDB_RETURN_SUCCESS;
}  /* end PostCreateQddsExec() */
