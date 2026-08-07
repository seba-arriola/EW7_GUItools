/* THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_api_CreateStaMag.c,v 1.6 2005/06/27 15:30:46 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_api_CreateStaMag.c,v $
 *     Revision 1.6  2005/06/27 15:30:46  davidk
 *     Removed and error check that was no longer relevant.
 *
 *     Revision 1.5  2005/06/15 19:03:12  davidk
 *     DB API Cleanup
 *
 *     Revision 1.4  2004/08/03 18:49:02  davidk
 *     Changed SSStatement to static scope instead of global.
 *
 *     Revision 1.3  2004/04/03 23:32:08  lombard
 *     Repaired mangled top line.
 *
 *     Revision 1.2  2004/03/31 19:02:09  davidk
 *     Added validation/debugging code for dMag and dWeight values.
 *
 *     Revision 1.1  2003/09/16 16:56:55  davidk
 *     Initial revision
 *
 *
 *
 */


#include <ewdb_ora_api.h>
#include <ewdb_cli_base.h>
#include <ewdb_ew_oci_base.h>

static char SQL_STRING[] =
	"Begin Create_Sta_Mag(OUT_idMagLink => :OUT_idMagLink,"
	"IN_idMagnitude => :IN_idMagnitude,"
	"IN_idDatum => :IN_idDatum,"
	"IN_dMag => :IN_dMag,"
	"IN_dWeight => :IN_dWeight); End;";
	
static EWDB_OCI_SFS SQLParamsBindArray[] = 
{
  {0,1,0,0,0,OA_EWDBID, ":OUT_idMagLink"},
  {0,1,0,0,0,OA_EWDBID, ":IN_idMagnitude"},
  {0,1,0,0,0,OA_EWDBID, ":IN_idDatum"},
  {0,1,0,0,0,OA_FLOAT,  ":IN_dMag"},
  {0,1,0,0,0,OA_FLOAT,  ":IN_dWeight"},
};

#define	NUM_FIELDS	5

static char		Local_szdMag[15];
static char		Local_szdWeight[15];

static EWDBid Local_idMagLink, Local_idDatum, Local_idMag;

/* Insertion Struct for CreateStaMag szStatement */
static EWDB_OCIStatementStruct SSStatement;

static int PrepCreateStaMagExec (EWDB_StationMagStruct *pStaMag, EWDB_Cursor *ppCursor);
static int InitCreateStaMagStatement (char *szStatement,EWDB_OCIStatementStruct *pSS);
static int PostCreateStaMagExec (EWDBid * pidMagLink);


int ewdb_api_CreateStaMag (EWDB_StationMagStruct *pStaMag)
{

	EWDB_Cursor pCursor;
  int rc;

	if (pStaMag == NULL)
	{
		logit ("", "ewdb_api_CreateStaMag(): Null parameters passed in!\n");
		return EWDB_RETURN_FAILURE;
	}

	ewdb_base_SetLastOraAPIActionTime ();
	
	if (ewdb_base_Reconnect () != EWDB_RETURN_SUCCESS)
	/* Establishes connection, and performs binding!?! */
	{
		logit ("", "ewdb_api_CreateStaMag(): Could not reconnect to the database!\n");
		return (EWDB_RETURN_FAILURE);
	}

	if (PrepCreateStaMagExec (pStaMag, &pCursor) != EWDB_RETURN_SUCCESS)
	{
		logit ("", "ewdb_api_CreateStaMag(): PrepCreateStaMagExec() failed.\n");
		return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
	}

	if (ewdb_base_SQLExecute (pCursor))
	{
		ewdb_base_ErrorReport (hEWDBC, pCursor,"ewdb_api_CreateStaMag(): ewdb_base_SQLExecute", 1);
		return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
	} 
  
	/* Commit the transaction (all the previous inserts!)
	   In Case there is any auditing or logging or debug
	   changes made in the stored procedures.
	****************************************************/
	if (ewdb_base_SQLCommit (hEWDBC))
	{
		ewdb_base_ErrorReport (hEWDBC, hEWDBC,"ewdb_api_CreateStaMag(): ewdb_base_SQLCommit",2);
		return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
	}

  rc = PostCreateStaMagExec(&(pStaMag->idMagLink));
  if(rc == EWDB_RETURN_FAILURE)
  {
    logit("", "ewdb_api_CreateStaMag(): PostCreateStaMagExec failed!\n");
    return(ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }
  else if(rc == EWDB_RETURN_WARNING)
  {
    return(EWDB_RETURN_FAILURE);
  }
  
	return(EWDB_RETURN_SUCCESS);
} /* end ewdb_internal_CreateStaMag() */


static int InitCreateStaMagStatement (char *szStatement,EWDB_OCIStatementStruct *pSS)
{
	pSS->FieldArray[0].pVal = &(Local_idMagLink);
	pSS->FieldArray[1].pVal = &(Local_idMag);
	pSS->FieldArray[2].pVal = &(Local_idDatum);
	pSS->FieldArray[3].pVal = Local_szdMag;
	pSS->FieldArray[4].pVal = Local_szdWeight;

  return(ewdb_base_RequestCursor(szStatement, pSS,0/*don't force rebind*/));
}  /* end InitCreateStaMagStatement() */


static int PrepCreateStaMagExec (EWDB_StationMagStruct *pStaMag, EWDB_Cursor *ppCursor)
{
	SSStatement.NumOfFields = NUM_FIELDS;
	SSStatement.FieldArray = SQLParamsBindArray;
	SSStatement.RecordSize = 0;

  Local_idMag   = pStaMag->idMagnitude;
  Local_idDatum = pStaMag->idDatum;

  if(pStaMag->dMag > 9.99)
    logit("","ewdb_api_CreateStaMag() ERROR! dMag = %.2f. Too Large!!\n", pStaMag->dMag);

  sprintf (Local_szdMag,   "%.2f", pStaMag->dMag);
  sprintf (Local_szdWeight,"%.1f", pStaMag->dWeight);

	if (InitCreateStaMagStatement (SQL_STRING,
				                   &SSStatement) != EWDB_RETURN_SUCCESS)
	{
		logit ("", "Call to InitCreateStaMagStatement failed!\n");
		return EWDB_RETURN_FAILURE;
	}

	*ppCursor = SSStatement.pCda;
	
	return EWDB_RETURN_SUCCESS;
}  /* end PrepCreateStaMagExec() */


static int PostCreateStaMagExec(EWDBid * pidMagLink)
{
    EWDB_Cursor pCursor;

    if (pidMagLink == NULL)
    {
        logit ("", "PostCreateStaMagExec(): Invalid arguments passed in.\n");
        return(EWDB_RETURN_FAILURE);
    }

    *pidMagLink=Local_idMagLink;

    pCursor = SSStatement.pCda;

    ewdb_base_ReleaseCursor (pCursor);

  if(*pidMagLink <= 0)
  {
    if(*pidMagLink == -1)
      logit("","PostCreateStaMagExec():  SQL Proc Create_StaMag() returned "
                "\"Unknown error\".  See SQL debug table for details.\n");
    else if(*pidMagLink == -2)
      logit("","PostCreateStaMagExec():  SQL Proc Create_StaMag() returned "
                "\"Non-matching magnitude types\".  The magnitude types from "
                "Local_idMag(%d) and Local_idDatum(%d) do not match.  So Local_idDatum "
                "cannot be used to calculate Local_idMag.  Example, you are "
                "trying to link a SurfaceWave station magnitude with a "
                "Local Mag summary magnitude.\n",
            Local_idMag, Local_idDatum);
    else if(*pidMagLink == -6)
      logit("","PostCreateStaMagExec():  SQL Proc Create_StaMag() returned "
                "\"Duplicate Station Magnitude\".  There is an existing "
                " record that links the given amplitude Local_idDatum(%d) and "
                " summary magnitude Local_idMag(%d).  A duplicate record "
                " cannot be created.\n",
            Local_idMag, Local_idDatum);
    else
      logit("","PostCreateStaMagExec():  SQL Proc Create_StaMag() returned "
               "the following error(%d).  Please see that proc for details.\n",
            pidMagLink);
    return(EWDB_RETURN_WARNING);
  }

	return (EWDB_RETURN_SUCCESS);
} /* PostCreateStaMagExec() */

