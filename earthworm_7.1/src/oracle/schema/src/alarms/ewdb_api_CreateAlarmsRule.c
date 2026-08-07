/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_api_CreateAlarmsRule.c,v 1.10 2005/06/03 22:32:16 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_api_CreateAlarmsRule.c,v $
 *     Revision 1.10  2005/06/03 22:32:16  davidk
 *     DB API Cleanup
 *
 *     Revision 1.9  2005/03/07 18:50:42  mark
 *     Added bSecondary to rules
 *
 *     Revision 1.8  2005/02/28 17:30:32  mark
 *     Improved error catching
 *
 *     Revision 1.7  2005/02/03 20:36:50  mark
 *     Added groups
 *
 *     Revision 1.6  2005/01/19 21:34:02  mark
 *     Added iPhases and bUseMag to alarms rule struct
 *
 *     Revision 1.5  2004/12/08 22:26:02  mark
 *     Added idPolygon field
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
	"Begin CreateOrUpdateAlarmsRule "
	"(OUT_idRule => :OUT_idRule,"
	"IN_dMag => :IN_dMag, "
	"IN_bAuto => :IN_bAuto, "
	"IN_idFormat => :IN_idFormat, "
	"IN_idCritProgram => :IN_idCritProgram, "
	"IN_idRecipientDelivery => :IN_idRecipientDelivery, "
	"IN_idRule => :IN_idRule, "
	"IN_idPolygon => :IN_idPolygon, "
	"IN_bUseMag => :IN_bUseMag, "
	"IN_iPhases => :IN_iPhases, "
	"IN_idGroup => :IN_idGroup, "
	"IN_bSecondary => :IN_bSecondary); End;";
	
static EWDB_OCI_SFS SQLParamsBindArray[] = 
{
  {0,1,0,0,0,OA_INT,    ":OUT_idRule"},
  {0,1,0,0,0,OA_FLOAT,  ":IN_dMag"},
  {0,1,0,0,0,OA_INT,    ":IN_bAuto"},
  {0,1,0,0,0,OA_INT,    ":IN_idFormat"},
  {0,1,0,0,0,OA_INT,    ":IN_idCritProgram"},
  {0,1,0,0,0,OA_INT,    ":IN_idRecipientDelivery"},
  {0,1,0,0,0,OA_INT,    ":IN_idRule"},
  {0,1,0,0,0,OA_INT,    ":IN_idPolygon"},
  {0,1,0,0,0,OA_INT,    ":IN_bUseMag"},
  {0,1,0,0,0,OA_INT,    ":IN_iPhases"},
  {0,1,0,0,0,OA_INT,    ":IN_idGroup"},
  {0,1,0,0,0,OA_INT,    ":IN_bSecondary"},
};

#define	NUM_FIELDS		12


static EWDB_OCIStatementStruct 		SSStatement;

static  EWDB_AlarmsRuleStruct			Local_RuleStruct;
/*
static	EWDBid 	                  Local_IdRule;
static	EWDBid 	                  Local_IdRecipientDelivery;
static  EWDBid                    Local_IdGroup;
static	EWDBid 	                  Local_IdCritProg;
static  EWDBid                    Local_IdPolygon;
static  int                       Local_bUseMag;
static  int                       Local_NumPhases;
static	int		                    Local_bSecondary;
*/
static  char	                    Local_szdMag[15];


static int PrepCreateRuleExec(EWDB_AlarmsRuleStruct *pRule, 
                              EWDB_AlarmDeliveryUnionStruct *pDel,
                              EWDB_Cursor *ppCursor);
int PostCreateRuleExec (EWDB_AlarmsRuleStruct *);
int InitCreateRuleStatement (char *, EWDB_OCIStatementStruct *);


int ewdb_api_CreateAlarmsRule (EWDB_AlarmsRuleStruct *pRule, EWDB_AlarmDeliveryUnionStruct *pDel)
{

	EWDB_Cursor	pCursor;

	if (pRule == NULL)
	{
		logit ("", "ewdb_api_CreateAlarmsRule(): Null parameters passed in!\n");
		return EWDB_RETURN_FAILURE;
	}
	if (pRule->idRecipientDelivery > 0 && pRule->idGroup > 0)
	{
		logit ("", "ewdb_api_CreateAlarmsRule(): Error: Both idRecipientDelivery and idGroup are valid!");
		return EWDB_RETURN_FAILURE;
	}
	if (pRule->idRecipientDelivery <= 0 && pRule->idGroup <= 0)
	{
		logit ("", "ewdb_api_CreateAlarmsRule(): Error: Neither idRecipientDelivery nor idGroup are valid!");
		return EWDB_RETURN_FAILURE;
	}

	ewdb_base_SetLastOraAPIActionTime ();

	if (ewdb_base_Reconnect () != EWDB_RETURN_SUCCESS)
	{
		logit ("", "ewdb_api_CreateAlarmsRule(): Could not reconnect to the database!\n");
		return (EWDB_RETURN_FAILURE);
	}

	if (PrepCreateRuleExec (pRule, pDel, &pCursor) != EWDB_RETURN_SUCCESS)
	{
		logit ("", "ewdb_api_CreateAlarmsRule(): Call to PrepCreateRuleExec failed.\n");
		return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
	}

	if (ewdb_base_SQLExecute (pCursor))
	{
		ewdb_base_ErrorReport (hEWDBC, pCursor,"ewdb_api_CreateAlarmsRule(): ewdb_base_SQLExecute", 1);
		return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
	} 
  
	/* Commit the transaction (all the previous inserts!)
	   In Case there is any auditing or logging or debug
	   changes made in the stored procedures.
	****************************************************/
	if (ewdb_base_SQLCommit (hEWDBC))
	{
		ewdb_base_ErrorReport (hEWDBC, hEWDBC, "ewdb_api_CreateAlarmsRule(): ewdb_base_SQLCommit",2);
		return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
	}

	if (PostCreateRuleExec (pRule) != EWDB_RETURN_SUCCESS)
	{
		logit ("", "ewdb_api_CreateAlarmsRule(): PostCreateRuleExec failed!\n");
		return(EWDB_RETURN_FAILURE);
	}

	return EWDB_RETURN_SUCCESS;
}  /* end ewdb_api_CreateAlarmsRule() */


/******************* InitCreateRuleStatement *******************/
static int InitCreateRuleStatement(char *szStatement, EWDB_OCIStatementStruct *pSS)
{

	if ((szStatement == NULL) ||  (pSS == NULL))
	{
		logit ("", "InitCreateRuleStatement(): Null parameters passed in!\n");
		return EWDB_RETURN_FAILURE;
	}

	pSS->FieldArray[0].pVal = &(Local_RuleStruct.idRule);
	pSS->FieldArray[1].pVal = Local_szdMag;
	pSS->FieldArray[2].pVal = &(Local_RuleStruct.Auto);
	pSS->FieldArray[3].pVal = &(Local_RuleStruct.Format.idFormat);
	pSS->FieldArray[4].pVal = &Local_RuleStruct.CritProg.idCritProgram;
	pSS->FieldArray[5].pVal = &Local_RuleStruct.idRecipientDelivery;
	pSS->FieldArray[6].pVal = &(Local_RuleStruct.idRule);
	pSS->FieldArray[7].pVal = &Local_RuleStruct.idPolygon;
	pSS->FieldArray[8].pVal = &Local_RuleStruct.bUseMag;
	pSS->FieldArray[9].pVal = &Local_RuleStruct.iPhases;
	pSS->FieldArray[10].pVal = &Local_RuleStruct.idGroup;
	pSS->FieldArray[11].pVal = &Local_RuleStruct.bSecondary;

	if (ewdb_base_RequestCursor (szStatement, pSS, 0) != 0)
	{
		logit ("", "InitCreateRuleStatement(): Call to ewdb_base_RequestCursor failed.\n");
		return EWDB_RETURN_FAILURE;
	}

	return EWDB_RETURN_SUCCESS;
}  


/******************* PrepCreateRuleExec *******************/
static int PrepCreateRuleExec(EWDB_AlarmsRuleStruct *pRule, 
                              EWDB_AlarmDeliveryUnionStruct *pDel,
                              EWDB_Cursor *ppCursor)
{
	if ((pRule == NULL) || (ppCursor == NULL))
	{
		logit ("", "PrepCreateRuleExec(): Null parameters passed in!\n");
		return EWDB_RETURN_FAILURE;
	}

	memcpy (&Local_RuleStruct, pRule, sizeof (EWDB_AlarmsRuleStruct));
	sprintf (Local_szdMag, "%.4f", pRule->dMag);

	if (pDel == NULL)
	{
		Local_RuleStruct.idRecipientDelivery = pRule->idRecipientDelivery;
		Local_RuleStruct.idGroup             = pRule->idGroup;
	}
	else
	{
		Local_RuleStruct.idRecipientDelivery = pDel->idRecipientDelivery;
		Local_RuleStruct.idGroup             = 0;
	}

	if (pRule->bCritInUse == TRUE)
		Local_RuleStruct.CritProg.idCritProgram = pRule->CritProg.idCritProgram;
	else
		Local_RuleStruct.CritProg.idCritProgram = 0;

	SSStatement.NumOfFields = NUM_FIELDS;
	SSStatement.FieldArray = SQLParamsBindArray;
	SSStatement.RecordSize = 0;

	if (InitCreateRuleStatement (SQL_STRING, &SSStatement)
												     != EWDB_RETURN_SUCCESS)
	{
		logit ("", "PrepCreateRuleExec(): InitCreateRuleStatement failed!\n");
		return EWDB_RETURN_FAILURE;
	}

	*ppCursor = SSStatement.pCda;
	
	return EWDB_RETURN_SUCCESS;
}  /* end PrepCreateRuleExec() */


/******************* PostCreateRuleExec *******************/
static int PostCreateRuleExec (EWDB_AlarmsRuleStruct *pRule)
{
	EWDB_Cursor pCursor;
  
	pCursor = SSStatement.pCda;
  
	ewdb_base_ReleaseCursor (pCursor);

  if (pRule == NULL)
	{
		logit ("", "PostCreateRuleExec(): Invalid arguments passed in.\n");
		return EWDB_RETURN_FAILURE;
	}
  
	pRule->idRule = Local_RuleStruct.idRule;

  if(Local_RuleStruct.idRule <= 0)
  {
    logit("", "PostCreateRuleExec(): ERROR SQL Proc CreateOrUpdateAlarmsRule() returned %d.\n", 
          Local_RuleStruct.idRule);
    return(EWDB_RETURN_WARNING);
  }
  
	return EWDB_RETURN_SUCCESS;
} 
