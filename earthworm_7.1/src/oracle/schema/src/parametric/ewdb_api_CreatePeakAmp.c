/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_api_CreatePeakAmp.c,v 1.6 2005/06/15 19:03:12 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_api_CreatePeakAmp.c,v $
 *     Revision 1.6  2005/06/15 19:03:12  davidk
 *     DB API Cleanup
 *
 *     Revision 1.5  2004/08/03 22:34:26  davidk
 *     Fixed bugs in code:
 *     1)  Set bInitialized=TRUE in init code.
 *     2)  Cleaned up PrepXXX() function that was in a limbo screwed up
 *     stage.
 *     3)  Added call from PrepXXX() to InitXXX() that was missing from
 *     previous call.
 *
 *     Revision 1.4  2004/08/03 18:51:07  davidk
 *     Rewrote function, since it was not written in standard DB insert format, and function had irreparable
 *     logic errors.
 *
 *     Revision 1.3  2003/08/25 18:08:49  lucky
 *     Added support for idPick, tStart and tEnd in PeakAmpStruct
 *
 *     Revision 1.2  2003/08/13 20:57:13  lucky
 *     idCHan was not being properly set when multiple calls were made
 *     from the same executable
 *
 *     Revision 1.1  2003/04/15 15:59:24  lucky
 *     Initial revision
 *
 *
 *
 */


/* ewdb_api_create_PeakAmp.c */
#include <ewdb_ora_api.h>
#include <ewdb_cli_base.h>
#include <ewdb_ew_oci_base.h>

static char SQL_STRING[] =
	"Begin Create_PeakAmp(OUT_idPeakAmp => :OUT_idPeakAmp,"
	"IN_idChan => :IN_idChan,"
	"IN_dPeakAmp1 => :IN_dPeakAmp1,"
	"IN_tAmp1 => :IN_tAmp1,"
	"IN_tPeriod1 => :IN_tPeriod1,"
	"IN_dPeakAmp2 => :IN_dPeakAmp2,"
	"IN_tAmp2 => :IN_tAmp2,"
	"IN_tPeriod2 => :IN_tPeriod2,"
	"IN_iMagType => :IN_iMagType,"
	"IN_sSource => :IN_sSource,"
	"IN_sSourceAmpID => :IN_sSourceAmpID,"
	"IN_idPick => :IN_idPick,"
	"IN_tStartInterval => :IN_tStartInterval,"
	"IN_tEndInterval => :IN_tEndInterval); End;";
	
static EWDB_OCI_SFS SQLParamsBindArray[] = 
{
  {0,1,0,0,0,OA_EWDBID, ":OUT_idPeakAmp"},
  {0,1,0,0,0,OA_EWDBID, ":IN_idChan"},
  {0,1,0,0,0,OA_FLOAT,  ":IN_dPeakAmp1"},
  {0,1,0,0,0,OA_DOUBLE, ":IN_tAmp1"},
  {0,1,0,0,0,OA_FLOAT,  ":IN_tPeriod1"},
  {0,1,0,0,0,OA_FLOAT,  ":IN_dPeakAmp2"},
  {0,1,0,0,0,OA_DOUBLE, ":IN_tAmp2"},
  {0,1,0,0,0,OA_FLOAT,  ":IN_tPeriod2"},
  {0,1,0,0,0,OA_INT,    ":IN_iMagType"},
  {0,1,0,0,0,OA_SZ,     ":IN_sSource"},
  {0,1,0,0,0,OA_SZ,     ":IN_sSourceAmpID"},
  {0,1,0,0,0,OA_EWDBID, ":IN_idPick"},
  {0,1,0,0,0,OA_DOUBLE, ":IN_tStartInterval"},
  {0,1,0,0,0,OA_DOUBLE, ":IN_tEndInterval"}
};

#define	NUM_FIELDS	14


static char		Local_szdPeakAmp1[30],Local_szdPeakAmp2[30];
static char		Local_sztAmp1[30],Local_sztAmp2[30];
static char		Local_sztPeriod1[30],Local_sztPeriod2[30];
static char		Local_sztStartInt[30],Local_sztEndInt[30];
static EWDB_PeakAmpStruct Local_PeakAmp;

/* Insertion Struct for CreatePeakAmp szStatement */
static EWDB_OCIStatementStruct SSStatement;

static int PrepCreatePeakAmpExec(EWDB_PeakAmpStruct *pPeakAmp, EWDB_Cursor *ppCursor);
static int PostCreatePeakAmpExec(EWDBid *pidPeakAmp);
static int InitCreatePeakAmpStatement(char *szStatement, EWDB_OCIStatementStruct *pSS);


int ewdb_api_CreatePeakAmp(EWDB_PeakAmpStruct *pPeakAmp)
{

	EWDB_Cursor pCursor;
	int rc;

	if (pPeakAmp == NULL)
	{
		logit ("", "ewdb_api_CreatePeakAmp(): Null parameters passed in!\n");
		return EWDB_RETURN_FAILURE;
	}

	ewdb_base_SetLastOraAPIActionTime();

	if (ewdb_base_Reconnect () != EWDB_RETURN_SUCCESS)
	{
		logit ("", "ewdb_api_CreatePeakAmp(): Could not reconnect to the database!\n");
		return (EWDB_RETURN_FAILURE);
	}

	if (PrepCreatePeakAmpExec(pPeakAmp, &pCursor) != EWDB_RETURN_SUCCESS)
	{
		logit ("", "ewdb_api_CreatePeakAmp(): PrepCreatePeakAmpExec() failed.\n");
		return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
	}

	if (ewdb_base_SQLExecute (pCursor))
	{
		ewdb_base_ErrorReport (hEWDBC, pCursor,"ewdb_api_CreatePeakAmp(): ewdb_base_SQLExecute", 1);
		return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
	} 
  
	/* Commit the transaction (all the previous inserts!)
	   In Case there is any auditing or logging or debug
	   changes made in the stored procedures.
	****************************************************/
	if (ewdb_base_SQLCommit (hEWDBC))
	{
		ewdb_base_ErrorReport (hEWDBC, hEWDBC,"ewdb_api_CreatePeakAmp(): ewdb_base_SQLCommit",2);
		return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
	}

  rc = PostCreatePeakAmpExec(&pPeakAmp->idPeakAmp);
  if(rc == EWDB_RETURN_FAILURE)
  {
    logit("", "ewdb_api_CreatePeakAmp(): PostCreatePeakAmpExec failed!\n");
    return(ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }
  else if(rc == EWDB_RETURN_WARNING)
  {
    return(EWDB_RETURN_FAILURE);
  }
  
	return(EWDB_RETURN_SUCCESS);
}  /* end ewdb_api_CreatePeakAmp() */


static int InitCreatePeakAmpStatement(char *szStatement, EWDB_OCIStatementStruct *pSS)
{

  static int bInitialized = FALSE;

  if(!bInitialized)
  {
    bInitialized = TRUE;
    pSS->FieldArray[0].pVal = &Local_PeakAmp.idPeakAmp;
    pSS->FieldArray[1].pVal = &Local_PeakAmp.idChan;
    pSS->FieldArray[2].pVal = Local_szdPeakAmp1;
    pSS->FieldArray[3].pVal = Local_sztAmp1;
    pSS->FieldArray[4].pVal = Local_sztPeriod1;
    pSS->FieldArray[5].pVal = Local_szdPeakAmp2;
    pSS->FieldArray[6].pVal = Local_sztAmp2;
    pSS->FieldArray[7].pVal = Local_sztPeriod2;
    pSS->FieldArray[8].pVal = &Local_PeakAmp.iAmpType;
    pSS->FieldArray[9].pVal = Local_PeakAmp.szExtSource;
    pSS->FieldArray[10].pVal = Local_PeakAmp.szExternalAmpID;
    pSS->FieldArray[11].pVal = &Local_PeakAmp.idPick;
    pSS->FieldArray[12].pVal = Local_sztStartInt;
    pSS->FieldArray[13].pVal = Local_sztEndInt;
  }

  return(ewdb_base_RequestCursor(szStatement, pSS,0/*don't force rebind*/));
}  /* InitCreatePeakAmpStatement() */


static int PrepCreatePeakAmpExec(EWDB_PeakAmpStruct *pPeakAmp, EWDB_Cursor *ppCursor)
{
	SSStatement.NumOfFields = NUM_FIELDS;
	SSStatement.FieldArray = SQLParamsBindArray;
	SSStatement.RecordSize = 0;

  memcpy(&Local_PeakAmp, pPeakAmp, sizeof(Local_PeakAmp));
  /* set pick to null if invalid */
  if(Local_PeakAmp.idPick < 0)
    Local_PeakAmp.idPick = 0;

	sprintf (Local_szdPeakAmp1, "%.4f", pPeakAmp->dAmp1);
	sprintf (Local_sztAmp1,     "%.4f", pPeakAmp->tAmp1);
	sprintf (Local_sztPeriod1,  "%.4f", pPeakAmp->dAmpPeriod1);
	sprintf (Local_szdPeakAmp2, "%.4f", pPeakAmp->dAmp2);
	sprintf (Local_sztAmp2,     "%.4f", pPeakAmp->tAmp2);
	sprintf (Local_sztPeriod2,  "%.4f", pPeakAmp->dAmpPeriod2);
	sprintf (Local_sztStartInt, "%.4f", pPeakAmp->tStartInterval);
	sprintf (Local_sztEndInt,   "%.4f", pPeakAmp->tEndInterval);

	if(InitCreatePeakAmpStatement(SQL_STRING, &SSStatement) 
      != EWDB_RETURN_SUCCESS)
	{
		logit("", "PrepCreatePeakAmpExec(): InitCreatePeakAmpStatement failed!\n");
		return(EWDB_RETURN_FAILURE);
	}

  *ppCursor = SSStatement.pCda;
	
	return(EWDB_RETURN_SUCCESS);
}  /* end PrepCreatePeakAmpExec() */


static int PostCreatePeakAmpExec(EWDBid *pidPeakAmp)
{
  EWDB_Cursor pCursor;
  
  pCursor = SSStatement.pCda;
  
  ewdb_base_ReleaseCursor(pCursor);

  if (pidPeakAmp == NULL)
  {
    logit ("", "PostCreatePeakAmpExec(): Invalid arguments passed in.\n");
    return EWDB_RETURN_FAILURE;
  }
  
  *pidPeakAmp = Local_PeakAmp.idPeakAmp;

  if(Local_PeakAmp.idPeakAmp <= 0)
  {
    logit("", "PostCreateChannelExec(): SQL Proc Create_PeakAmp() failed with error %d\n",
          Local_PeakAmp.idPeakAmp);
    return(EWDB_RETURN_WARNING);
  }
  else
    return(EWDB_RETURN_SUCCESS);

  /* End PostCreatePeakAmpExec() */
}
