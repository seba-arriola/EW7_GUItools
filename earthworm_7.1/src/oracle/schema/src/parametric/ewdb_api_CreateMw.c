/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_api_CreateMw.c,v 1.7 2005/06/15 19:03:12 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_api_CreateMw.c,v $
 *     Revision 1.7  2005/06/15 19:03:12  davidk
 *     DB API Cleanup
 *
 *     Revision 1.6  2005/06/14 16:58:29  davidk
 *     Changed Mij components of Mw to %.4f  format.
 *
 *     Revision 1.5  2005/03/31 18:51:47  davidk
 *     Changed the EWDB_MwStruct:
 *       Added dM0 (doh!)
 *       Changed dPercentCVLD to dPercentCLVD
 *
 *     Revision 1.4  2005/03/24 00:16:03  davidk
 *     Fixed bugs during initial testing.
 *
 *     Revision 1.3  2005/03/23 18:55:42  davidk
 *     Added a SQL_Commit call to have the transaction commit.  Otherwise it rolls
 *     back when the cursor is closed.  Every statement should have a commit,
 *     even queries that don't appear to touch anything.
 *
 *     Revision 1.2  2005/03/21 17:18:36  davidk
 *     Fixed a bug in SQL variable name.
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
        "Begin Create_Mw(OUT_Retcode => :OUT_Retcode, OUT_idMw => :OUT_idMw, "
        " IN_dMxx => :IN_dMxx, IN_dMxy => :IN_dMxy, IN_dMxz => :IN_dMxz, "
        " IN_dMyy => :IN_dMyy, IN_dMyz => :IN_dMyz, IN_dMzz => :IN_dMzz, "
        " IN_dM0 => :IN_dM0, IN_iScalarExp => :IN_iScalarExp, "
        " IN_dPFPStrike => :IN_dPFPStrike, IN_dPFPDip => :IN_dPFPDip, IN_dPFPRake => :IN_dPFPRake, "
        " IN_dAFPStrike => :IN_dAFPStrike, IN_dAFPDip => :IN_dAFPDip, IN_dAFPRake => :IN_dAFPRake, "
        " IN_dDepth => :IN_dDepth, IN_idOrigin => :IN_idOrigin, IN_idMag => :IN_idMag, "
        " IN_iNumStations => :IN_iNumStations, IN_dMisfit => :IN_dMisfit, IN_dPercentDC => :IN_dPercentDC, "
        " IN_dPercentCLVD => :IN_dPercentCLVD, IN_idMwFilter => :IN_idMwFilter); End;";

static EWDB_OCI_SFS SQLParamsBindArray[] =
{
  {0,1,0,0,0,OA_INT,":OUT_Retcode"},
  {0,1,0,0,0,OA_EWDBID,":OUT_idMw"},
  {0,1,0,0,0,OA_DOUBLE,":IN_dMxx"},
  {0,1,0,0,0,OA_DOUBLE,":IN_dMxy"},
  {0,1,0,0,0,OA_DOUBLE,":IN_dMxz"},
  {0,1,0,0,0,OA_DOUBLE,":IN_dMyy"},
  {0,1,0,0,0,OA_DOUBLE,":IN_dMyz"},
  {0,1,0,0,0,OA_DOUBLE,":IN_dMzz"},
  {0,1,0,0,0,OA_INT,":IN_iScalarExp"},
  {0,1,0,0,0,OA_DOUBLE,":IN_dPFPStrike"},
  {0,1,0,0,0,OA_DOUBLE,":IN_dPFPDip"},
  {0,1,0,0,0,OA_DOUBLE,":IN_dPFPRake"},
  {0,1,0,0,0,OA_DOUBLE,":IN_dAFPStrike"},
  {0,1,0,0,0,OA_DOUBLE,":IN_dAFPDip"},
  {0,1,0,0,0,OA_DOUBLE,":IN_dAFPRake"},
  {0,1,0,0,0,OA_DOUBLE,":IN_dDepth"},
  {0,1,0,0,0,OA_EWDBID,":IN_idOrigin"},
  {0,1,0,0,0,OA_EWDBID,":IN_idMag"},
  {0,1,0,0,0,OA_INT,":IN_iNumStations"},
  {0,1,0,0,0,OA_DOUBLE,":IN_dMisfit"},
  {0,1,0,0,0,OA_DOUBLE,":IN_dM0"},
  {0,1,0,0,0,OA_DOUBLE,":IN_dPercentDC"},
  {0,1,0,0,0,OA_DOUBLE,":IN_dPercentCLVD"},
  {0,1,0,0,0,OA_EWDBID,":IN_idMwFilter"}
};

/* define the total number of fields to be bound */
#define NUM_FIELDS 24

/* Insertion Struct for GetStationList statement */
static EWDB_OCIStatementStruct SSStatement;

static char Local_szdMxx[20], Local_szdMxy[20], Local_szdMxz[20], 
            Local_szdMyy[20], Local_szdMyz[20], Local_szdMzz[20];
static char Local_szdPFPStrike[20], Local_szdPFPDip[20], Local_szdPFPRake[20], 
            Local_szdAFPStrike[20], Local_szdAFPDip[20], Local_szdAFPRake[20];

static char Local_szdM0[20], Local_szdDepth[20], Local_szdMisfit[20], 
            Local_szdPercentDC[20], Local_szdPercentCLVD[20];

static EWDB_MwStruct Local_Mw;
static int Local_iRetcode;

/********************************
      FUNCTION PROTOTYPES
********************************/
static int PrepCreateMwExec(EWDB_MwStruct * pMw, EWDB_Cursor * ppCursor);
static int PostCreateMwExec(EWDBid * pidMw);
static int InitCreateMwStatement(char * szStatement, EWDB_OCIStatementStruct *pSS);


/**********************************************************************
**********************************************************************/
int ewdb_api_CreateMw(EWDB_MwStruct * pMw)
{
  int rc;

  EWDB_Cursor  pCursor;
 
  if(!pMw)
  {
    logit("et", "ERROR!  ewdb_api_CreateMw(): Null pointer passed as input! Returning!\n");
    return( EWDB_RETURN_FAILURE );
  }

  ewdb_base_SetLastOraAPIActionTime();

  if( ewdb_base_Reconnect() != EWDB_RETURN_SUCCESS )
  {
    return( EWDB_RETURN_FAILURE );
  }

  if( PrepCreateMwExec(pMw,&pCursor) != EWDB_RETURN_SUCCESS)
  {
    logit("","ewdb_api_CreateMw(): PrepCreateMwExec() failed!\n");
    return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }
  
  if( ewdb_base_SQLExecute(pCursor) )
  {
    ewdb_base_ErrorReport(hEWDBC, pCursor,"ewdb_api_CreateMw(): ewdb_base_SQLExecute",1);
    return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  } 

  if(ewdb_base_SQLCommit(hEWDBC))
  {
    ewdb_base_ErrorReport(hEWDBC, hEWDBC,
                      "ewdb_api_CreateMw(): ewdb_base_SQLCommit",2);
    return(ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }
  
  if((rc = PostCreateMwExec(&pMw->idMw)) == EWDB_RETURN_FAILURE)
  {
    logit("","ewdb_api_CreateMw(): PostCreateMwExec() failed!\n");
    return(EWDB_RETURN_FAILURE);
  }

  ewdb_base_SetLastOraAPIActionTime();

  return(rc);
}  /* end ewdb_api_CreateMw() */


static int InitCreateMwStatement(char * szStatement, EWDB_OCIStatementStruct *pSS)
{
	pSS->FieldArray[0].pVal = &Local_iRetcode;
	pSS->FieldArray[1].pVal = &Local_Mw.idMw;
	pSS->FieldArray[2].pVal = Local_szdMxx;
	pSS->FieldArray[3].pVal = Local_szdMxy;
	pSS->FieldArray[4].pVal = Local_szdMxz;
	pSS->FieldArray[5].pVal = Local_szdMyy;
	pSS->FieldArray[6].pVal = Local_szdMyz;
	pSS->FieldArray[7].pVal = Local_szdMzz;
	pSS->FieldArray[8].pVal = &Local_Mw.iScalarExp;
	pSS->FieldArray[9].pVal = Local_szdPFPStrike;
	pSS->FieldArray[10].pVal = Local_szdPFPDip;
	pSS->FieldArray[11].pVal = Local_szdPFPRake;
	pSS->FieldArray[12].pVal = Local_szdAFPStrike;
	pSS->FieldArray[13].pVal = Local_szdAFPDip;
	pSS->FieldArray[14].pVal = Local_szdAFPRake;
	pSS->FieldArray[15].pVal = Local_szdDepth;
	pSS->FieldArray[16].pVal = &Local_Mw.idOrigin;
	pSS->FieldArray[17].pVal = &Local_Mw.idMag;
	pSS->FieldArray[18].pVal = &Local_Mw.iNumStations;
	pSS->FieldArray[19].pVal = Local_szdMisfit;
	pSS->FieldArray[20].pVal = Local_szdM0;
	pSS->FieldArray[21].pVal = Local_szdPercentDC;
	pSS->FieldArray[22].pVal = Local_szdPercentCLVD;
	pSS->FieldArray[23].pVal = &Local_Mw.Filter.idMwFilter;

  return(ewdb_base_RequestCursor(szStatement, pSS,0/*don't force rebind*/));
}


static int PrepCreateMwExec(EWDB_MwStruct * pMw, EWDB_Cursor * ppCursor)
{

  SSStatement.NumOfFields=NUM_FIELDS;
  SSStatement.FieldArray=SQLParamsBindArray;
  SSStatement.RecordSize=0;

  memcpy(&Local_Mw, pMw, sizeof(EWDB_MwStruct));

  sprintf(Local_szdMxx,    "%.4f", Local_Mw.dMxx);
  sprintf(Local_szdMxy,    "%.4f", Local_Mw.dMxy);
  sprintf(Local_szdMxz,    "%.4f", Local_Mw.dMxz);
  sprintf(Local_szdMyy,    "%.4f", Local_Mw.dMyy);
  sprintf(Local_szdMyz,    "%.4f", Local_Mw.dMyz);
  sprintf(Local_szdMzz,    "%.4f", Local_Mw.dMzz);
 
  sprintf(Local_szdM0,          "%.2f", Local_Mw.dM0);

  sprintf(Local_szdPFPStrike,   "%.2f", Local_Mw.dPFPStrike);
  sprintf(Local_szdPFPDip,      "%.2f", Local_Mw.dPFPDip);
  sprintf(Local_szdPFPRake,     "%.2f", Local_Mw.dPFPRake);
  sprintf(Local_szdAFPStrike,   "%.2f", Local_Mw.dAFPStrike);
  sprintf(Local_szdAFPDip,      "%.2f", Local_Mw.dAFPDip);
  sprintf(Local_szdAFPRake,     "%.2f", Local_Mw.dAFPRake);

  sprintf(Local_szdDepth,       "%.1f", Local_Mw.dDepth);
  sprintf(Local_szdMisfit,      "%.2f", Local_Mw.dMisfit);
  sprintf(Local_szdPercentDC,   "%.2f", Local_Mw.dPercentDC);
  sprintf(Local_szdPercentCLVD, "%.2f", Local_Mw.dPercentCLVD);

  if(InitCreateMwStatement(SQL_STRING, &SSStatement) != EWDB_RETURN_SUCCESS)
  {
    *ppCursor = NULL;
    return(EWDB_RETURN_FAILURE);
  }

  *ppCursor=SSStatement.pCda;

  return(EWDB_RETURN_SUCCESS);
}  /* end PrepXXX() */


static int PostCreateMwExec(EWDBid * pidMw)
{

  ewdb_base_ReleaseCursor (SSStatement.pCda);

  if(Local_iRetcode >= 0)
  {
    *pidMw = Local_Mw.idMw;

    if(Local_iRetcode > 0)
    {
      logit("","PostCreateMwExec(): SQL Proc Create_Mw() returned warning(%d).\n",
           Local_iRetcode);  
    }

    return(EWDB_RETURN_SUCCESS);
  }
  else
  {
    if(Local_iRetcode == -1)
    {
      logit("","PostCreateMwExec(): SQL Proc Create_Mw() returned UNKNOWN ERROR.  See debug table for details.\n");
      return(EWDB_RETURN_FAILURE);
    }
    else if(Local_iRetcode == -2)
    {
      logit("","PostCreateMwExec(): SQL Proc Create_Mw() returned INVALID ORIGIN(%d)\n",
            Local_Mw.idOrigin);  
    }
    else if(Local_iRetcode == -3)
    {
      logit("","PostCreateMwExec(): SQL Proc Create_Mw() returned INVALID MAGNITUDE(%d)\n",
            Local_Mw.idMag);  
    }
    else if(Local_iRetcode == -4)
    {
      logit("","PostCreateMwExec(): SQL Proc Create_Mw() returned INVALID FILTER(%d)\n",
            Local_Mw.Filter.idMwFilter);  
    }
    else if(Local_iRetcode == -5)
    {
      logit("","PostCreateMwExec(): SQL Proc Create_Mw() returned INVALID M0(%.2f)\n",
            Local_szdM0);  
    }
    else
    {
      logit("","PostCreateMwExec(): SQL Proc Create_Mw() sreturned "
            "the following ERROR(%d).\n",Local_iRetcode);  
      return(EWDB_RETURN_FAILURE);
    }
    return(EWDB_RETURN_WARNING);
  }
}   /* End PostXXX() */

