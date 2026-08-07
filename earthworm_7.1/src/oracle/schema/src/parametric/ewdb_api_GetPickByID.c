/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_api_GetPickByID.c,v 1.2 2005/06/15 19:03:12 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_api_GetPickByID.c,v $
 *     Revision 1.2  2005/06/15 19:03:12  davidk
 *     DB API Cleanup
 *
 *     Revision 1.1  2003/08/20 23:13:37  michelle
 *     Initial revision
 *
 *     Revision 1.5  2003/06/10 14:38:55  lucky
 *     *** empty log message ***
 *
 *
 */

#include <ewdb_cli_base.h>
#include <ewdb_ora_api.h>
#include <ewdb_ew_oci_base.h>

static char SQL_STRING[] =
        "Begin Get_PickById(OUT_Local_iRetCode => :OUT_Local_iRetCode, IN_idPick => :IN_idPick, "
        " OUT_xidExternal => :OUT_xidExternal, "
        " OUT_idChan => :OUT_idChan, "
        " OUT_sPhase => :OUT_sPhase, "
        " OUT_tPhase => :OUT_tPhase, "
        " OUT_cMotion => :OUT_cMotion, "
        " OUT_cOnset => :OUT_cOnset, "
        " OUT_dSigma => :OUT_dSigma, "
        " OUT_sSource => :OUT_sSource); End;";


static EWDB_OCI_SFS SQLParamsBindArray[] =
{
  {0,1,0,0,0,OA_INT,":OUT_Local_iRetCode"},
  {0,1,0,0,0,OA_EWDBID,":IN_idPick"},
  {0,1,0,0,0,OA_SZ,":OUT_xidExternal"},
  {0,1,0,0,0,OA_EWDBID,":OUT_idChan"},
  {0,1,0,0,0,OA_SZ,":OUT_sPhase"},
  {0,1,0,0,0,OA_DOUBLE,":OUT_tPhase"},
  {0,1,0,0,0,OA_SZ,":OUT_cMotion"},
  {0,1,0,0,0,OA_SZ,":OUT_cOnset"},
  {0,1,0,0,0,OA_FLOAT,":OUT_dSigma"},
  {0,1,0,0,0,OA_SZ,":OUT_sSource"}
};

/* define the total number of fields to be bound */
#define NUM_FIELDS 10

/* Insertion Struct for GetPick statement */
EWDB_OCIStatementStruct SSStatement;

static EWDB_ArrivalStruct Local_Pick;

static char Local_sztPhase[20],Local_szdSigma[20], Local_szcOnset[10]; Local_szcMotion[10];

static int Local_iRetCode;


/********************************
      FUNCTION PROTOTYPES
********************************/
static int PostGetPickExec(EWDB_ArrivalStruct * pPick);
static int PrepGetPickExec(EWDBid IN_idPick, EWDB_Cursor * ppCursor);
static int InitGetPickStatement(char * szStatement, EWDB_OCIStatementStruct *pSS);


/**********************************************************************
**********************************************************************/
int ewdb_api_GetPickByID(EWDBid IN_idPick, EWDB_ArrivalStruct * pPick)
{

  EWDB_Cursor  pCursor;
 
  ewdb_base_SetLastOraAPIActionTime();

  if(IN_idPick <= 0 || !pPick )
  {
    logit("","ewdb_api_GetPickByID(): Invalid params passed in:\n"
             "IN_idPick %d, pPick %u\n",
          IN_idPick, pPick);
    return(EWDB_RETURN_FAILURE);
  }

  if( ewdb_base_Reconnect() != EWDB_RETURN_SUCCESS )
  {
    return( EWDB_RETURN_FAILURE );
  }

  if( PrepGetPickExec(IN_idPick,&pCursor) != EWDB_RETURN_SUCCESS)
  {
    logit("","ewdb_api_GetPickByID(): PrepGetPickExec() failed!\n");
    return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }

  if( ewdb_base_SQLExecute(pCursor) )
  {
    ewdb_base_ErrorReport(hEWDBC, pCursor,"ewdb_api_GetPickByID(): ewdb_base_SQLExecute",1);
    return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  } 

 /* Commit the transaction (all the previous inserts!)
  In Case there is any auditing or logging or debug
  changes made in the stored procedures.
  ****************************************************/
  if (ewdb_base_SQLCommit (hEWDBC))
  {
    ewdb_base_ErrorReport (hEWDBC, hEWDBC,"DeletePolygon:ewdb_base_SQLCommit",2);
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  }
 
  if( PostGetPickExec(pPick) == EWDB_RETURN_FAILURE)
  {
    logit("","ewdb_api_GetPickByID(): PostGetPickExec() failed!\n");
    return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }

  ewdb_base_SetLastOraAPIActionTime();

  return(EWDB_RETURN_SUCCESS);
}  /* end ewdb_api_GetPick() */


static int InitGetPickStatement(char * szStatement, EWDB_OCIStatementStruct *pSS)
{
	pSS->FieldArray[0].pVal = &Local_iRetCode;
	pSS->FieldArray[1].pVal = &(Local_Pick.idPick);
	pSS->FieldArray[2].pVal = Local_Pick.szExternalPickID;
	pSS->FieldArray[3].pVal = &(Local_Pick.idChan);
	pSS->FieldArray[4].pVal = Local_Pick.szObsPhase;
	pSS->FieldArray[5].pVal = Local_sztPhase;
	pSS->FieldArray[6].pVal = Local_szcMotion;
	pSS->FieldArray[7].pVal = Local_szcOnset;
	pSS->FieldArray[8].pVal = Local_szdSigma;
	pSS->FieldArray[9].pVal = Local_Pick.szExtSource;

  return(ewdb_base_RequestCursor(szStatement, pSS,0));
}  /* end InitGetPickStatement() */


static int PrepGetPickExec(EWDBid IN_idPick, EWDB_Cursor * ppCursor)
{

  Local_Pick.idPick=IN_idPick;

  SSStatement.NumOfFields=NUM_FIELDS;
  SSStatement.FieldArray=SQLParamsBindArray;
  SSStatement.RecordSize=0;

  if(InitGetPickStatement(SQL_STRING, &SSStatement) != EWDB_RETURN_SUCCESS)
  {
    *ppCursor = NULL;
    return(EWDB_RETURN_FAILURE);
  }

  *ppCursor=SSStatement.pCda;

  return(EWDB_RETURN_SUCCESS);
}  /* end PrepXXX() */


static int PostGetPickExec(EWDB_ArrivalStruct * pPick)
{

  ewdb_base_ReleaseCursor (SSStatement.pCda);

  if(Local_iRetCode >= 0)
  {
    Local_Pick.tObsPhase=atof(Local_sztPhase);
    Local_Pick.dSigma=(float)atof(Local_szdSigma);
    Local_Pick.cMotion=Local_szcMotion[0];
    Local_Pick.cOnset=Local_szcOnset[0];

    /* Copy the results to the users struct */
    memcpy(pPick,&Local_Pick,sizeof(EWDB_ArrivalStruct));

    if(Local_iRetCode > 0)
    {
      logit("","PostGetPickExec(): SQL Proc Get_PickById() procedure returned "
            "the following warning number(%d).\n",Local_iRetCode);  
    }

    return(EWDB_RETURN_WARNING);
  }
  else
  {
    logit("","PL/SQL Get_PickById() procedure returned "
          "the following error(%d).\n",Local_iRetCode);  
    return(EWDB_RETURN_FAILURE);
  }
}  /* end PostXXX() */




