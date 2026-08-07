/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_internal_CreateStaMag.c,v 1.4 2005/06/15 19:03:12 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_internal_CreateStaMag.c,v $
 *     Revision 1.4  2005/06/15 19:03:12  davidk
 *     DB API Cleanup
 *
 *     Revision 1.3  2001/07/12 21:07:17  davidk
 *     Cleaned up the function as part of the API cleanup.  Added improved
 *     error logging, handling, and reporting.
 *
 *     Revision 1.1  2001/05/15 02:16:24  davidk
 *     Initial revision
 *
 *     Revision 1.1  2001/02/28 17:19:22  lucky
 *     Initial revision
 *
 *     Revision 1.7  2001/02/21 08:39:22  davidk
 *     Code was retrofitted to use the new db_cli_base API
 *     in lieu of the oci_base() and OCI7 function API's.layer, which
 *     provides abstraction from the OCI7 function calls to
 *     ora_api layer code.  See comments in
 *     schema/src/include/internal/ewdb_cli_base.h for more info
 *     on the details of the changes made.
 *
 *     Revision 1.6  2000/06/21 22:51:35  lucky
 *     Cleaned up logit calls to make log files more readable.
 *
 *     Revision 1.5  2000/03/30 18:50:13  davidk
 *     rewrote code to match the template used by other ewdb functions:
 *     main function does not know about internal vars, user data is
 *     copied to internal buffer by PrepXXX, InitXXX uses internal
 *     buffers, PostXXX is used to copy data from internal buffers back
 *     to user buffer, columns are declared as ToBeBound in struct
 *     array declaration instead of on the fly, unless they are optional
 *     SQL statement arguments.  (This is gibberish for I rewrote it
 *     to match the style of the others), so that it would be easier to
 *     debug.  I believe a bug was removed by the rewriting.
 *
 *     Revision 1.4  1999/11/09 18:21:08  lucky
 *     *** empty log message ***
 *
 *
 */


#include <ewdb_ora_api.h>
#include <ewdb_cli_base.h>
#include <ewdb_ew_oci_base.h>


static char SQL_STRING[] =
	"Begin Create_Sta_Mag(OUT_Local_idMagLink => :OUT_Local_idMagLink,"
	"IN_Local_idMag => :IN_Local_idMag,"
	"IN_Local_idDatum => :IN_Local_idDatum,"
	"IN_dMag => :IN_dMag,"
	"IN_dWeight => :IN_dWeight); End;";
	
static EWDB_OCI_SFS SQLParamsBindArray[] = 
{
  {0,1,0,0,0,OA_EWDBID, ":OUT_Local_idMagLink"},
  {0,1,0,0,0,OA_EWDBID, ":IN_Local_idMag"},
  {0,1,0,0,0,OA_EWDBID, ":IN_Local_idDatum"},
  {0,1,0,0,0,OA_FLOAT,  ":IN_dMag"},
  {0,1,0,0,0,OA_FLOAT,  ":IN_dWeight"},
};

#define	NUM_FIELDS	5

static char		Local_szdMag[15];
static char		Local_szdWeight[15];
static EWDBid Local_idMagLink, Local_idDatum, Local_idMag;

/* Insertion Struct for CreateStaMag statement */
EWDB_OCIStatementStruct SSStatement;

static int PrepCreateStaMagExec (EWDB_StationMagStruct *pStaMag, EWDB_Cursor *ppCursor);
static int InitCreateStaMagStatement (char *statement,EWDB_OCIStatementStruct *pSS);
static int PostCreateStaMagExec (EWDBid * pLocal_idMagLink);


int ewdb_internal_CreateStaMag(EWDB_StationMagStruct *pStaMag)
{

	EWDB_Cursor pCursor;
  int rc;

	if (pStaMag == NULL)
	{
		logit ("", "ewdb_internal_CreateStaMag(): Null parameters passed in!\n");
		return EWDB_RETURN_FAILURE;
	}

	ewdb_base_SetLastOraAPIActionTime ();
	
	if (ewdb_base_Reconnect () != EWDB_RETURN_SUCCESS)
	/* Establishes connection, and performs binding!?! */
	{
		logit ("", "ewdb_internal_CreateStaMag(): Could not reconnect to the database!\n");
		return (EWDB_RETURN_FAILURE);
	}

	if (PrepCreateStaMagExec (pStaMag, &pCursor) != EWDB_RETURN_SUCCESS)
	{
		logit ("", "ewdb_internal_CreateStaMag(): PrepCreateStaMagExec() failed.\n");
		return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
	}

	if (ewdb_base_SQLExecute (pCursor))
	{
		ewdb_base_ErrorReport (hEWDBC, pCursor,"ewdb_internal_CreateStaMag(): ewdb_base_SQLExecute", 1);
		return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
	} 

  
	/* Commit the transaction (all the previous inserts!)
	   In Case there is any auditing or logging or debug
	   changes made in the stored procedures.
	****************************************************/
	if (ewdb_base_SQLCommit (hEWDBC))
	{
		ewdb_base_ErrorReport (hEWDBC, hEWDBC,"ewdb_internal_CreateStaMag(): ewdb_base_SQLCommit",2);
		return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
	}

  rc = PostCreateStaMagExec(&(pStaMag->idMagLink));
  if(rc == EWDB_RETURN_FAILURE)
  {
    logit("", "ewdb_internal_CreateStaMag(): PostCreateStaMagExec failed!\n");
    return(ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }
  else if(rc == EWDB_RETURN_WARNING)
  {
    return(EWDB_RETURN_FAILURE);
  }
  
	return(EWDB_RETURN_SUCCESS);
} /* end ewdb_internal_CreateStaMag() */


static int InitCreateStaMagStatement (char *statement,EWDB_OCIStatementStruct *pSS)
{

	pSS->FieldArray[0].pVal = &(Local_idMagLink);
	pSS->FieldArray[1].pVal = &(Local_idMag);
	pSS->FieldArray[2].pVal = &(Local_idDatum);
	pSS->FieldArray[3].pVal = Local_szdMag;
	pSS->FieldArray[4].pVal = Local_szdWeight;

  if (ewdb_base_RequestCursor (statement, pSS, 0) != 0)
  {
    logit ("", "InitCreateStaMagStatement(): ewdb_base_RequestCursor failed.\n");
    return EWDB_RETURN_FAILURE;
  }
	
	return(EWDB_RETURN_SUCCESS);
}  /* end InitCreateStaMagStatement() */


static int PrepCreateStaMagExec (EWDB_StationMagStruct *pStaMag, EWDB_Cursor *ppCursor)
{

	if ((pStaMag == NULL) || (ppCursor == NULL))
	{
		logit ("", "PrepCreateStaMagExec(): Null parameters passed in!\n");
		return EWDB_RETURN_FAILURE;
	}

	SSStatement.NumOfFields = NUM_FIELDS;
	SSStatement.FieldArray = SQLParamsBindArray;
	SSStatement.RecordSize = 0;

  Local_idMag = pStaMag->idMag;
  Local_idDatum = pStaMag->idDatum;

  sprintf (Local_szdMag,   "%.2f", pStaMag->dMag);
  sprintf (Local_szdWeight,"%.1f", pStaMag->dWeight);

	if (InitCreateStaMagStatement (SQL_STRING,
				                   &SSStatement) != EWDB_RETURN_SUCCESS)
	{
		logit ("", "PrepCreateStaMagExec(): InitCreateStaMagStatement failed!\n");
		return EWDB_RETURN_FAILURE;
	}

	*ppCursor = SSStatement.pCda;
	
	return EWDB_RETURN_SUCCESS;
}  /* end PrepCreateStaMagExec() */


static int PostCreateStaMagExec(EWDBid * pLocal_idMagLink)
{
  EWDB_Cursor pCursor;
  
  pCursor = SSStatement.pCda;
  
  ewdb_base_ReleaseCursor (pCursor);
  
  if (pLocal_idMagLink == NULL)
  {
    logit ("", "PostCreateStaMagExec(): Invalid arguments passed in.\n");
    return(EWDB_RETURN_FAILURE);
  }
  
  *pLocal_idMagLink=Local_idMagLink;
  

  if(*pLocal_idMagLink <= 0)
  {
    if(*pLocal_idMagLink == -1)
      logit("","PostCreateStaMagExec():  SQL Proc Create_StaMag() returned "
                "\"Unknown error\".  See SQL debug table for details.\n");
    else if(*pLocal_idMagLink == -2)
      logit("","PostCreateStaMagExec():  SQL Proc Create_StaMag() returned "
                "\"Non-matching magnitude types\".  The magnitude types from "
                "Local_idMag(%d) and Local_idDatum(%d) do not match.  So Local_idDatum "
                "cannot be used to calculate Local_idMag.  Example, you are "
                "trying to link a SurfaceWave station magnitude with a "
                "Local Mag summary magnitude.\n",
            Local_idMag, Local_idDatum);
    else if(*pLocal_idMagLink == -6)
      logit("","PostCreateStaMagExec():  SQL Proc Create_StaMag() returned "
                "\"Duplicate Station Magnitude\".  There is an existing "
                " record that links the given amplitude Local_idDatum(%d) and "
                " summary magnitude Local_idMag(%d).  A duplicate record "
                " cannot be created.\n",
            Local_idMag, Local_idDatum);
    else
      logit("","PostCreateStaMagExec():  SQL Proc Create_StaMag() returned "
               "the following error(%d).  Please see that proc for details.\n",
            pLocal_idMagLink);
    return(EWDB_RETURN_WARNING);
  }

	return (EWDB_RETURN_SUCCESS);
} /* PostCreateStaMagExec() */
