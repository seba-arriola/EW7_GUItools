/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_internal_CreateMwChanTS.c,v 1.6 2005/06/30 03:30:17 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_internal_CreateMwChanTS.c,v $
 *     Revision 1.6  2005/06/30 03:30:17  davidk
 *     Removed old logit() debugging statement.
 *
 *     Revision 1.5  2005/06/15 19:03:12  davidk
 *     DB API Cleanup
 *
 *     Revision 1.4  2005/03/24 18:14:43  davidk
 *     Fixed bugs during initial testing of Mw API
 *
 *     Revision 1.3  2005/03/24 00:16:03  davidk
 *     Fixed bugs during initial testing.
 *
 *     Revision 1.2  2005/03/23 18:55:42  davidk
 *     Added a SQL_Commit call to have the transaction commit.  Otherwise it rolls
 *     back when the cursor is closed.  Every statement should have a commit,
 *     even queries that don't appear to touch anything.
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
#include <ewdb_internal_functions.h>


static char SQL_STRING[] =
        "Begin Create_MwChanTS(OUT_Retcode => :OUT_Retcode, OUT_idMwCTS => :OUT_idMwCTS, "
        " IN_dTSStart => :IN_dTSStart, IN_dTSFreq => :IN_dTSFreq, IN_iTSLen => :IN_iTSLen, "
        " IN_iType => :IN_iType, IN_idMwFilter => :IN_idMwFilter, IN_idChan => :IN_idChan, "
        " IN_sTS1 => :IN_sTS1, IN_sTS2 => :IN_sTS2); End;";


static EWDB_OCI_SFS SQLParamsBindArray[] =

{
  {0,1,0,0,0,OA_INT,   ":OUT_Retcode"},
  {0,1,0,0,0,OA_EWDBID,":OUT_idMwCTS"},
  {0,1,0,0,0,OA_DOUBLE,":IN_dTSStart"},
  {0,1,0,0,0,OA_FLOAT, ":IN_dTSFreq"},
  {0,1,0,0,0,OA_INT,   ":IN_iTSLen"},
  {0,1,0,0,0,OA_INT,   ":IN_iType"},
  {0,1,0,0,0,OA_EWDBID,":IN_idMwFilter"},
  {0,1,0,0,0,OA_EWDBID,":IN_idChan"},
  {0,1,0,0,0,OA_SZ,    ":IN_sTS1"},
  {0,1,0,0,0,OA_SZ,    ":IN_sTS2"}
};

/* define the total number of fields to be bound */
#define NUM_FIELDS 10 

/* Insertion Struct for GetStationList statement */
static EWDB_OCIStatementStruct SSStatement;

static char Local_szdTSStart[20], Local_szdTSFreq[20], Local_szTS[EWDB_MAX_SQL_STRING_LEN+1];
static char Local_szTS2[EWDB_MAX_SQL_STRING_LEN+1];

static EWDB_MwTimeSeriesStruct Local_MwCTS;
static int Local_iRetcode;

/********************************
      FUNCTION PROTOTYPES
********************************/
static int PrepCreateMwChanTSExec(EWDB_MwTimeSeriesStruct * pMwCTS, EWDB_Cursor * ppCursor);
static int PostCreateMwChanTSExec(EWDBid * pidMwCTS);
static int InitCreateMwChanTSStatement(char * szStatement, EWDB_OCIStatementStruct *pSS);


/**********************************************************************
**********************************************************************/
int ewdb_internal_CreateMwChanTS(EWDB_MwTimeSeriesStruct * pMwCTS)
{
  int rc;

  EWDB_Cursor  pCursor;
 
  if(!pMwCTS)
  {
    logit("et", "ERROR!  ewdb_internal_CreateMwChanTS(): Null pointer passed as input! Returning!\n");
    return( EWDB_RETURN_FAILURE );
  }

  ewdb_base_SetLastOraAPIActionTime();

  if( ewdb_base_Reconnect() != EWDB_RETURN_SUCCESS )
  {
    return( EWDB_RETURN_FAILURE );
  }

  if( PrepCreateMwChanTSExec(pMwCTS,&pCursor) != EWDB_RETURN_SUCCESS)
  {
    return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }
  
  if( ewdb_base_SQLExecute(pCursor) )
  {
    ewdb_base_ErrorReport(hEWDBC, pCursor,"ewdb_internal_CreateMwChanTS(): ewdb_base_SQLExecute",1);
    return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  } 

  if(ewdb_base_SQLCommit(hEWDBC))
  {
    ewdb_base_ErrorReport(hEWDBC, hEWDBC,
                      "ewdb_internal_CreateMwChanTS(): ewdb_base_SQLCommit",2);
    return(ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }
  
  if((rc = PostCreateMwChanTSExec(&pMwCTS->idMwCTS)) == EWDB_RETURN_FAILURE)
  {
    return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }

  ewdb_base_SetLastOraAPIActionTime();

  return(rc);
}  /* end ewdb_internal_CreateMwChanTS() */


static int InitCreateMwChanTSStatement(char * szStatement, EWDB_OCIStatementStruct *pSS)
{
	pSS->FieldArray[0].pVal = &Local_iRetcode;
	pSS->FieldArray[1].pVal = &Local_MwCTS.idMwCTS;
	pSS->FieldArray[2].pVal = Local_szdTSStart;
	pSS->FieldArray[3].pVal = Local_szdTSFreq;
	pSS->FieldArray[4].pVal = &Local_MwCTS.iLen;
	pSS->FieldArray[5].pVal = &Local_MwCTS.iTSType;
	pSS->FieldArray[6].pVal = &Local_MwCTS.idMwFilter;
	pSS->FieldArray[7].pVal = &Local_MwCTS.idChan;
	pSS->FieldArray[8].pVal = Local_szTS;
	pSS->FieldArray[9].pVal = Local_szTS2;

  return(ewdb_base_RequestCursor(szStatement, pSS,0));
}


static int PrepCreateMwChanTSExec(EWDB_MwTimeSeriesStruct * pMwCTS, EWDB_Cursor * ppCursor)
{
  int rc;

  SSStatement.NumOfFields=NUM_FIELDS;
  SSStatement.FieldArray=SQLParamsBindArray;
  SSStatement.RecordSize=0;


  memcpy(&Local_MwCTS, pMwCTS, sizeof(EWDB_MwTimeSeriesStruct));

  sprintf(Local_szdTSStart,   "%.4f", Local_MwCTS.dStart);
  sprintf(Local_szdTSFreq,    "%.2f", Local_MwCTS.dFreq);

  rc = ewdb_internal_TS_CtoSQL(Local_MwCTS.TS, Local_MwCTS.iLen, Local_szTS, Local_szTS2);
  if(rc != EWDB_RETURN_SUCCESS)
  {
    logit("et","PrepCreateMwChanTSExec(%d %.4f %d): error(%d) during ewdb_internal_TS_CtoSQL()!  Returning!\n",
           Local_MwCTS.idChan, Local_MwCTS.dStart, Local_MwCTS.iTSType, rc);
    return(EWDB_RETURN_FAILURE);
  }

  if(InitCreateMwChanTSStatement(SQL_STRING, &SSStatement) != EWDB_RETURN_SUCCESS)
  {
    *ppCursor = NULL;
    return(EWDB_RETURN_FAILURE);
  }

  *ppCursor=SSStatement.pCda;

  return(EWDB_RETURN_SUCCESS);
}  /* end PrepXXX() */


static int PostCreateMwChanTSExec(EWDBid * pidMwCTS)
{
  ewdb_base_ReleaseCursor (SSStatement.pCda);

  if(Local_iRetcode >= 0)
  {
    *pidMwCTS = Local_MwCTS.idMwCTS;

    if(Local_iRetcode > 0)
    {
      logit("","PL/SQL Create_MwCTS() procedure returned "
            "the following warning number(%d).\n",Local_iRetcode);  
    }

    return(EWDB_RETURN_SUCCESS);
  }
  else
  {
    if(Local_iRetcode == -1)
    {
      logit("","PL/SQL Create_MwCTS() procedure returned UNKNOWN ERROR.  See debug table for details.\n");
      return(EWDB_RETURN_FAILURE);
    }
    else if(Local_iRetcode == -2)
    {
      logit("","PL/SQL Create_MwCTS() procedure returned INVALID CHANNEL(%d)\n",
            Local_MwCTS.idChan);  
    }
    else if(Local_iRetcode == -3)
    {
      logit("","PL/SQL Create_MwCTS() procedure returned INVALID FILTER(%d)\n",
            Local_MwCTS.idMwFilter);  
    }
    else
    {
      logit("","PL/SQL Create_MwCTS() procedure returned "
            "the following ERROR(%d).\n",Local_iRetcode);  
      return(EWDB_RETURN_FAILURE);
    }
    return(EWDB_RETURN_WARNING);
  }
}   /* End PostXXX() */

