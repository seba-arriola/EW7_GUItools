/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_internal_CreatePeakAmp.c,v 1.4 2005/06/15 19:03:12 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_internal_CreatePeakAmp.c,v $
 *     Revision 1.4  2005/06/15 19:03:12  davidk
 *     DB API Cleanup
 *
 *     Revision 1.3  2001/10/02 18:09:58  lucky
 *     Fixed sizes of numbers so that things are being inserted correctly.
 *
 *     Revision 1.2  2001/07/12 21:07:17  davidk
 *     Cleaned up the function as part of the API cleanup.  Added improved
 *     error logging, handling, and reporting.
 *
 *     Revision 1.1  2001/05/15 02:16:24  davidk
 *     Initial revision
 *
 *     Revision 1.1  2001/02/28 17:19:22  lucky
 *     Initial revision
 *
 *     Revision 1.8  2001/02/21 08:39:22  davidk
 *     Code was retrofitted to use the new db_cli_base API
 *     in lieu of the oci_base() and OCI7 function API's.layer, which
 *     provides abstraction from the OCI7 function calls to
 *     ora_api layer code.  See comments in
 *     schema/src/include/internal/ewdb_cli_base.h for more info
 *     on the details of the changes made.
 *
 *     Revision 1.7  2000/06/21 22:51:35  lucky
 *     Cleaned up logit calls to make log files more readable.
 *
 *     Revision 1.6  2000/05/12 17:35:00  davidk
 *     Changed code to match the style of other API functions.  An API
 *     function calls a PrepXXX() and a PostXXX() function that copy
 *     variables in between the caller's struct and a local struct.
 *     The API function itself has no knowledge of the local struct.
 *     The Init_XXX() function only receives two parameters: a statement
 *     to be parsed, and a EWDB_OCIStatementStruct to hold binding
 *     information.  All binding is done to local variables, not user
 *     variable, so that a rebind does not need to occur for each new
 *     call.  EWDB_OCIStatementStruct.UseField assignments are set as
 *     constant in the struct declaration unless the SQL string is
 *     variable, meaning that the number of sql variables can change
 *     (example: when selecting an event list, sometimes you might use
 *     depth criteria and sometimes not).
 *
 *     Revision 1.5  2000/05/12 17:19:47  davidk
 *     added prototypes for the functions defined in this file.
 *
 *     Revision 1.4  1999/11/09 18:21:08  lucky
 *     *** empty log message ***
 *
 *
 */


/* create_PeakAmp.c */
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
	"IN_iMagType => :IN_iMagType); End;";
	
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
  {0,1,0,0,0,OA_INT,     ":IN_iMagType"},
};

#define	NUM_FIELDS	9

static char		Local_szdPeakAmp1[20],Local_szdPeakAmp2[20];
static char		Local_sztAmp1[20],Local_sztAmp2[20];
static char		Local_sztPeriod1[20],Local_sztPeriod2[20];

static	int    Local_iMagType;
static  EWDBid Local_idChan;
static  EWDBid Local_idPeakAmp;

/* Insertion Struct for CreatePeakAmp statement */
static EWDB_OCIStatementStruct SSStatement;

static int PrepCreatePeakAmpExec(EWDB_StationMagStruct *pPeakAmp, EWDB_Cursor *ppCursor);
static int PostCreatePeakAmpExec(EWDB_StationMagStruct *pPeakAmp);
static int InitCreatePeakAmpStatement(char *statement, EWDB_OCIStatementStruct *pSS);


int ewdb_internal_CreatePeakAmp(EWDB_StationMagStruct *pPeakAmp)
{

	EWDB_Cursor pCursor;
  int rc;

	if (pPeakAmp == NULL)
	{
		logit ("", "ewdb_internal_CreatePeakAmp(): Null parameters passed in!\n");
		return EWDB_RETURN_FAILURE;
	}

	ewdb_base_SetLastOraAPIActionTime();

	if (ewdb_base_Reconnect () != EWDB_RETURN_SUCCESS)
	/* Re-establishes connection if neccessary */
	{
		logit ("", "ewdb_internal_CreatePeakAmp(): Could not reconnect to the database!\n");
		return (EWDB_RETURN_FAILURE);
	}

	if (PrepCreatePeakAmpExec(pPeakAmp, &pCursor) != EWDB_RETURN_SUCCESS)
	{
		logit ("", "ewdb_internal_CreatePeakAmp(): PrepEWDB_CreatePeakAmp() failed.\n");
		return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
	}

	if (ewdb_base_SQLExecute (pCursor))
	{
		ewdb_base_ErrorReport (hEWDBC, pCursor,"ewdb_internal_CreatePeakAmp(): ewdb_base_SQLExecute", 1);
		return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
	} 

	/* Commit the transaction (all the previous inserts!)
	   In Case there is any auditing or logging or debug
	   changes made in the stored procedures.
	****************************************************/
	if (ewdb_base_SQLCommit (hEWDBC))
	{
		ewdb_base_ErrorReport (hEWDBC, hEWDBC,"ewdb_internal_CreatePeakAmp(): ewdb_base_SQLCommit",2);
		return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
	}

  rc = PostCreatePeakAmpExec(pPeakAmp);
  if(rc == EWDB_RETURN_FAILURE)
  {
    logit("", "ewdb_internal_CreatePeakAmp(): PostCreatePeakAmpExec failed!\n");
    return(ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }
  else if(rc == EWDB_RETURN_WARNING)
  {
    return(EWDB_RETURN_FAILURE);
  }
  
	return(EWDB_RETURN_SUCCESS);
}  /* end ewdb_internal_CreatePeakAmp() */


static int InitCreatePeakAmpStatement (char *statement, EWDB_OCIStatementStruct *pSS)
{

  pSS->FieldArray[0].pVal = &(Local_idPeakAmp);
  pSS->FieldArray[1].pVal = &(Local_idChan);
	pSS->FieldArray[2].pVal = Local_szdPeakAmp1;
	pSS->FieldArray[3].pVal = Local_sztAmp1;
	pSS->FieldArray[4].pVal = Local_sztPeriod1;
	pSS->FieldArray[5].pVal = Local_szdPeakAmp2;
	pSS->FieldArray[6].pVal = Local_sztAmp2;
	pSS->FieldArray[7].pVal = Local_sztPeriod2;
	pSS->FieldArray[8].pVal = &Local_iMagType;

  if (ewdb_base_RequestCursor (statement, pSS, 0) != 0)
  {
    logit ("", "InitCreatePeakAmpStatement(): ewdb_base_RequestCursor failed.\n");
    return(EWDB_RETURN_FAILURE);
  }

	return(EWDB_RETURN_SUCCESS);
}


static int PrepCreatePeakAmpExec (EWDB_StationMagStruct *pPeakAmp, EWDB_Cursor *ppCursor)
{

	if ((pPeakAmp == NULL) || (ppCursor == NULL))
	{
		logit ("", "PrepCreatePeakAmpExec(): Null parameters passed in!\n");
		return EWDB_RETURN_FAILURE;
	}


	SSStatement.NumOfFields = NUM_FIELDS;
	SSStatement.FieldArray = SQLParamsBindArray;
	SSStatement.RecordSize = 0;

  Local_idChan  = pPeakAmp->idChan;
  Local_iMagType = pPeakAmp->iMagType;

  sprintf (Local_szdPeakAmp1, "%.4f", pPeakAmp->StaMagUnion.PeakAmp.dAmp1);
  sprintf (Local_sztAmp1,     "%.4f", pPeakAmp->StaMagUnion.PeakAmp.tAmp1);
  sprintf (Local_sztPeriod1,  "%.4f", pPeakAmp->StaMagUnion.PeakAmp.dAmpPeriod1);
  sprintf (Local_szdPeakAmp2, "%.4f", pPeakAmp->StaMagUnion.PeakAmp.dAmp2);
  sprintf (Local_sztAmp2,     "%.4f", pPeakAmp->StaMagUnion.PeakAmp.tAmp2);
  sprintf (Local_sztPeriod2,  "%.4f", pPeakAmp->StaMagUnion.PeakAmp.dAmpPeriod2);


	if (InitCreatePeakAmpStatement (SQL_STRING, &SSStatement)
      != EWDB_RETURN_SUCCESS)
	{
		logit ("", "PrepCreatePeakAmpExec(): InitCreatePeakAmpStatement failed!\n");
		return EWDB_RETURN_FAILURE;
	}

	*ppCursor = SSStatement.pCda;
	
	return(EWDB_RETURN_SUCCESS);
}  /* end PrepCreatePeakAmpExec() */


static int PostCreatePeakAmpExec(EWDB_StationMagStruct *pPeakAmp)
{
  EWDB_Cursor pCursor;
  
  pCursor = SSStatement.pCda;
  
  ewdb_base_ReleaseCursor(pCursor);

  if (pPeakAmp == NULL)
  {
    logit ("", "PostCreatePeakAmpExec(): Invalid arguments passed in.\n");
    return EWDB_RETURN_FAILURE;
  }
  
  pPeakAmp->StaMagUnion.PeakAmp.idPeakAmp=Local_idPeakAmp;
  pPeakAmp->idDatum = Local_idPeakAmp;

  if(pPeakAmp->idDatum <= 0)
  {
    if(pPeakAmp->idDatum == -1)
      logit("","PostCreatePeakAmpExec():  SQL Proc Create_PeakAmp() returned "
                "\"Unknown error\".  See SQL debug table for details.\n");
    else
      logit("","PostCreatePeakAmpExec():  SQL Proc Create_PeakAmp() returned "
               "the following error(%d).  Please see that proc for details.\n",
            pPeakAmp->idDatum);
    return(EWDB_RETURN_WARNING);
  }

	return(EWDB_RETURN_SUCCESS);

  /* End PostCreatePeakAmpExec() */
}  /* end PostCreatePeakAmpExec() */
