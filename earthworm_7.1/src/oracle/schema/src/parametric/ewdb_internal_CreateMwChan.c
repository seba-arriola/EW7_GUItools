/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_internal_CreateMwChan.c,v 1.6 2005/06/15 19:03:12 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_internal_CreateMwChan.c,v $
 *     Revision 1.6  2005/06/15 19:03:12  davidk
 *     DB API Cleanup
 *
 *     Revision 1.5  2005/03/24 18:14:43  davidk
 *     Fixed bugs during initial testing of Mw API
 *
 *     Revision 1.4  2005/03/24 00:16:03  davidk
 *     Fixed bugs during initial testing.
 *
 *     Revision 1.3  2005/03/23 18:55:42  davidk
 *     Added a SQL_Commit call to have the transaction commit.  Otherwise it rolls
 *     back when the cursor is closed.  Every statement should have a commit,
 *     even queries that don't appear to touch anything.
 *
 *     Revision 1.2  2005/03/23 06:22:30  davidk
 *     Fixed bugs in SQL statements - prior to run-testing code.
 *
 *     Revision 1.1  2005/03/22 23:38:56  davidk
 *     Added API functions for Mw
 *
 *
 *
 *
 */

#include <ewdb_cli_base.h>
#include <ewdb_ora_api.h>
#include <ewdb_ew_oci_base.h>

static char SQL_STRING[] =
        "Begin Create_MwChan(OUT_Retcode => :OUT_Retcode, OUT_idMwChan => :OUT_idMwChan, "
        " IN_idMw => :IN_idMw,  IN_idPick => :IN_idPick, IN_idChan => :IN_idChan, "
        " IN_iOffset => :IN_iOffset, IN_dMisfit => :IN_dMisfit, "
        " IN_idSynthTS => :IN_idSynthTS, IN_idRealTS => :IN_idRealTS); End;";


static EWDB_OCI_SFS SQLParamsBindArray[] =

{
  {0,1,0,0,0,OA_INT,   ":OUT_Retcode"},
  {0,1,0,0,0,OA_EWDBID,":OUT_idMwChan"},
  {0,1,0,0,0,OA_EWDBID,":IN_idMw"},
  {0,1,0,0,0,OA_EWDBID,":IN_idPick"},
  {0,1,0,0,0,OA_EWDBID,":IN_idChan"},
  {0,1,0,0,0,OA_INT,   ":IN_iOffset"},
  {0,1,0,0,0,OA_DOUBLE,":IN_dMisfit"},
  {0,1,0,0,0,OA_EWDBID,":IN_idSynthTS"},
  {0,1,0,0,0,OA_EWDBID,":IN_idRealTS"}
};

/* define the total number of fields to be bound */
#define NUM_FIELDS 9

/* Insertion Struct for GetStationList statement */
static EWDB_OCIStatementStruct SSStatement;

static char Local_szdMisfit[20];

static char Local_szdDepth[20], Local_szdMisfit[20], Local_szdPercentDC[20], Local_szdPercentCVLD[20];

static EWDB_MwChanStruct Local_MwChan;
static int Local_iRetcode;

/********************************
      FUNCTION PROTOTYPES
********************************/
static int PrepCreateMwChanExec(EWDB_MwChanStruct * pMwChan, EWDB_Cursor * ppCursor);
static int PostCreateMwChanExec(EWDBid * pidMwChan);
static int InitCreateMwChanStatement(char * szStatement, EWDB_OCIStatementStruct *pSS);


/**********************************************************************
**********************************************************************/
int ewdb_internal_CreateMwChan(EWDB_MwChanStruct * pMwChan)
{
  int rc;

  EWDB_Cursor  pCursor;
 
  if(!pMwChan)
  {
    logit("et", "ERROR!  ewdb_internal_CreateMwChan(): Null pointer passed as input! Returning!\n");
    return( EWDB_RETURN_FAILURE );
  }

  ewdb_base_SetLastOraAPIActionTime();

  if( ewdb_base_Reconnect() != EWDB_RETURN_SUCCESS )
  {
    return( EWDB_RETURN_FAILURE );
  }

  if( PrepCreateMwChanExec(pMwChan,&pCursor) != EWDB_RETURN_SUCCESS)
  {
    return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }

  if( ewdb_base_SQLExecute(pCursor) )
  {
    ewdb_base_ErrorReport(hEWDBC, pCursor,"ewdb_internal_CreateMwChan(): ewdb_base_SQLExecute",1);
    return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  } 

  if(ewdb_base_SQLCommit(hEWDBC))
  {
    ewdb_base_ErrorReport(hEWDBC, hEWDBC,
                      "ewdb_internal_CreateMwChan(): ewdb_base_SQLCommit",2);
    return(ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }
  
  if((rc = PostCreateMwChanExec(&pMwChan->idMwChan)) == EWDB_RETURN_FAILURE)
  {
    return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }

  ewdb_base_SetLastOraAPIActionTime();

  return(rc);
}  /* end ewdb_internal_CreateMwChan() */


static int InitCreateMwChanStatement(char * szStatement, EWDB_OCIStatementStruct *pSS)
{
	pSS->FieldArray[0].pVal = &Local_iRetcode;
	pSS->FieldArray[1].pVal = &Local_MwChan.idMwChan;
	pSS->FieldArray[2].pVal = &Local_MwChan.idMw;
	pSS->FieldArray[3].pVal = &Local_MwChan.idPick;
	pSS->FieldArray[4].pVal = &Local_MwChan.idChan;
	pSS->FieldArray[5].pVal = &Local_MwChan.iOffset;
	pSS->FieldArray[6].pVal = Local_szdMisfit;
	pSS->FieldArray[7].pVal = &Local_MwChan.SynthTS.idMwCTS;
	pSS->FieldArray[8].pVal = &Local_MwChan.RealTS.idMwCTS;

  return(ewdb_base_RequestCursor(szStatement, pSS,0));
}


static int PrepCreateMwChanExec(EWDB_MwChanStruct * pMwChan, EWDB_Cursor * ppCursor)
{

  SSStatement.NumOfFields=NUM_FIELDS;
  SSStatement.FieldArray=SQLParamsBindArray;
  SSStatement.RecordSize=0;


  memcpy(&Local_MwChan, pMwChan, sizeof(EWDB_MwChanStruct));

  sprintf(Local_szdMisfit,    "%.2f", Local_MwChan.dMisfit);

  if(InitCreateMwChanStatement(SQL_STRING, &SSStatement) != EWDB_RETURN_SUCCESS)
  {
    *ppCursor = NULL;
    return(EWDB_RETURN_FAILURE);
  }

  *ppCursor=SSStatement.pCda;

  return(EWDB_RETURN_SUCCESS);
}  /* end PrepXXX() */


static int PostCreateMwChanExec(EWDBid * pidMwChan)
{

  ewdb_base_ReleaseCursor (SSStatement.pCda);

  if(Local_iRetcode >= 0)
  {
    *pidMwChan = Local_MwChan.idMwChan;

    if(Local_iRetcode > 0)
    {
      logit("","PL/SQL Create_MwChan() procedure returned "
            "the following warning number(%d).\n",Local_iRetcode);  
    }

    return(EWDB_RETURN_SUCCESS);
  }
  else
  {
    if(Local_iRetcode == -1)
    {
      logit("","PL/SQL Create_MwChan() procedure returned UNKNOWN ERROR.  See debug table for details.\n");
      return(EWDB_RETURN_FAILURE);
    }
    else if(Local_iRetcode == -2)
    {
      logit("","PL/SQL Create_MwChan() procedure returned INVALID Mw(%d)\n",
            Local_MwChan.idMw);  
    }
    else if(Local_iRetcode == -3)
    {
      logit("","PL/SQL Create_MwChan() procedure returned INVALID Synthetic TS(%d)\n",
            Local_MwChan.SynthTS.idMwCTS);  
    }
    else if(Local_iRetcode == -4)
    {
      logit("","PL/SQL Create_MwChan() procedure returned INVALID Real TS(%d)\n",
            Local_MwChan.RealTS.idMwCTS);  
    }
    else if(Local_iRetcode == -5)
    {
      logit("","PL/SQL Create_MwChan() procedure returned INVALID Pick or Chan(%d/%d)\n",
            Local_MwChan.idPick, Local_MwChan.idChan);  
    }
    else
    {
      logit("","PL/SQL Create_MwChan() procedure returned "
            "the following ERROR(%d).\n",Local_iRetcode);  
      return(EWDB_RETURN_FAILURE);
    }
    return(EWDB_RETURN_WARNING);
  }
}   /* End PostXXX() */

