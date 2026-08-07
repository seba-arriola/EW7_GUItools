/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_api_GetMwFilterByProperties.c,v 1.4 2005/06/15 19:03:12 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_api_GetMwFilterByProperties.c,v $
 *     Revision 1.4  2005/06/15 19:03:12  davidk
 *     DB API Cleanup
 *
 *     Revision 1.3  2005/03/24 00:16:03  davidk
 *     Fixed bugs during initial testing.
 *
 *     Revision 1.2  2005/03/23 18:55:42  davidk
 *     Added a SQL_Commit call to have the transaction commit.  Otherwise it rolls
 *     back when the cursor is closed.  Every statement should have a commit,
 *     even queries that don't appear to touch anything.
 *
 *     Revision 1.1  2005/03/19 01:57:02  davidk
 *     Added some functions for Mw.
 *
 *
 *
 */

#include <ewdb_cli_base.h>
#include <ewdb_ora_api.h>
#include <ewdb_ew_oci_base.h>

static char SQL_STRING[] =
        "Begin Get_MwFilter_By_Props(OUT_Retcode => :OUT_Retcode, OUT_idMwFilter => :OUT_idMwFilter, "
        " IN_dLowCutHz => :IN_dLowCutHz, IN_dLowTaperHz => :IN_dLowTaperHz, "
        " IN_dHighTaperHz => :IN_dHighTaperHz, IN_dHighCutHz => :IN_dHighCutHz); End;";



static EWDB_OCI_SFS SQLParamsBindArray[] =

{
  {0,1,0,0,0,OA_INT,":OUT_Retcode"},
  {0,1,0,0,0,OA_EWDBID,":OUT_idMwFilter"},
  {0,1,0,0,0,OA_FLOAT,":IN_dLowCutHz"},
  {0,1,0,0,0,OA_FLOAT,":IN_dLowTaperHz"},
  {0,1,0,0,0,OA_FLOAT,":IN_dHighTaperHz"},
  {0,1,0,0,0,OA_FLOAT,":IN_dHighCutHz"}
};

/* define the total number of fields to be bound */
#define NUM_FIELDS 6

/* Insertion Struct for GetStationList statement */
static EWDB_OCIStatementStruct SSStatement;

static char Local_szdLowCutHz[20],    Local_szdLowTaperHz[20], 
            Local_szdHighTaperHz[20], Local_szdHighCutHz[20];

static int Local_iRetcode;

static EWDBid Local_idMwFilter;

/********************************
      FUNCTION PROTOTYPES
********************************/
static int PrepGetMwFilterByPropertiesExec(EWDB_MwFilterStruct * pMwFilter, EWDB_Cursor * ppCursor);
static int PostGetMwFilterByPropertiesExec(EWDB_MwFilterStruct * pMwFilter);
static int InitGetMwFilterByPropertiesStatement(char * szStatement, EWDB_OCIStatementStruct *pSS);


/**********************************************************************
**********************************************************************/
int ewdb_api_GetMwFilterByProperties(EWDB_MwFilterStruct * pMwFilter)
{

  EWDB_Cursor  pCursor;
 
  if(!pMwFilter)
  {
    logit("et", "ERROR!  ewdb_api_GetMwFilterByProperties(): Null pointer passed as input! Returning!\n");
    return( EWDB_RETURN_FAILURE );
  }

  ewdb_base_SetLastOraAPIActionTime();

  if( ewdb_base_Reconnect() != EWDB_RETURN_SUCCESS )
  {
    return( EWDB_RETURN_FAILURE );
  }

  if( PrepGetMwFilterByPropertiesExec(pMwFilter,&pCursor) != EWDB_RETURN_SUCCESS)
  {
    return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }

  if( ewdb_base_SQLExecute(pCursor) )
  {
    ewdb_base_ErrorReport(hEWDBC, pCursor,"ewdb_api_GetMwFilterByProperties(): ewdb_base_SQLExecute",1);
    return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  } 

  if(ewdb_base_SQLCommit(hEWDBC))
  {
    ewdb_base_ErrorReport(hEWDBC, hEWDBC,
                      "ewdb_api_GetMwFilterByProperties(): ewdb_base_SQLCommit",2);
    return(ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }
  
  if( PostGetMwFilterByPropertiesExec(pMwFilter) == EWDB_RETURN_FAILURE)
  {
    return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }

  ewdb_base_SetLastOraAPIActionTime();

  return(EWDB_RETURN_SUCCESS);
}  /* end ewdb_api_GetMwFilterByProperties() */


static int InitGetMwFilterByPropertiesStatement(char * szStatement, EWDB_OCIStatementStruct *pSS)
{
	pSS->FieldArray[0].pVal = &Local_iRetcode;
	pSS->FieldArray[1].pVal = &Local_idMwFilter;
	pSS->FieldArray[2].pVal = Local_szdLowCutHz;
	pSS->FieldArray[3].pVal = Local_szdLowTaperHz;
	pSS->FieldArray[4].pVal = Local_szdHighTaperHz;
	pSS->FieldArray[5].pVal = Local_szdHighCutHz;

  return(ewdb_base_RequestCursor(szStatement, pSS,0/*don't force rebind*/));
}


static int PrepGetMwFilterByPropertiesExec(EWDB_MwFilterStruct * pMwFilter, EWDB_Cursor * ppCursor)
{

  SSStatement.NumOfFields=NUM_FIELDS;
  SSStatement.FieldArray=SQLParamsBindArray;
  SSStatement.RecordSize=0;


  sprintf(Local_szdLowCutHz,    "%.3f", pMwFilter->dLowCutHz);
  sprintf(Local_szdLowTaperHz,  "%.3f", pMwFilter->dLowTaperHz);
  sprintf(Local_szdHighTaperHz, "%.3f", pMwFilter->dHighTaperHz);
  sprintf(Local_szdHighCutHz,   "%.3f", pMwFilter->dHighCutHz);

  if(InitGetMwFilterByPropertiesStatement(SQL_STRING, &SSStatement) != EWDB_RETURN_SUCCESS)
  {
    *ppCursor = NULL;
    return(EWDB_RETURN_FAILURE);
  }

  *ppCursor=SSStatement.pCda;

  return(EWDB_RETURN_SUCCESS);
}  /* end PrepXXX() */


static int PostGetMwFilterByPropertiesExec(EWDB_MwFilterStruct * pMwFilter)
{

  ewdb_base_ReleaseCursor (SSStatement.pCda);

  if(Local_iRetcode >= 0)
  {
    pMwFilter->idMwFilter = Local_idMwFilter;

    if(Local_iRetcode > 0)
    {
      logit("","PostGetMwFilterByPropertiesExec(): SQL Proc Get_MwFilter_By_Props() procedure returned "
            "the following warning number(%d).\n",Local_iRetcode);  
    }

    return(EWDB_RETURN_SUCCESS);
  }
  else
  {
    logit("","PostGetMwFilterByPropertiesExec(): SQL Proc Get_MwFilter_By_Props() procedure returned "
             "the following ERROR(%d).\n",Local_iRetcode); 
    return(EWDB_RETURN_FAILURE);
  }
}   /* End PostXXX() */

