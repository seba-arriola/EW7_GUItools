/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_api_CreateMagnitude.c,v 1.7 2005/06/15 19:03:12 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_api_CreateMagnitude.c,v $
 *     Revision 1.7  2005/06/15 19:03:12  davidk
 *     DB API Cleanup
 *
 *     Revision 1.6  2001/07/12 21:07:17  davidk
 *     cleaned up the function as part of the API cleanup.  Added improved
 *     error loggin, handling, and reporting.
 *
 *     Revision 1.4  2001/05/24 00:52:04  davidk
 *     fixed bugs in CreateMagnitude so that it now working on Solaris after the change in magnitude structure.
 *
 *     Revision 1.3  2001/05/21 16:50:54  davidk
 *     Fixing the CreateMagnitude C code to match the new sql proc.
 *
 *     Revision 1.2  2001/05/15 02:16:19  davidk
 *     Moved functions around between the apps, DB API, and DB API INTERNAL
 *     levels.  Renamed functions and files.  Added support for amplitude
 *     magnitude types.  Reformatted makefiles.
 *
 *     Revision 1.1  2001/02/28 17:19:22  lucky
 *     Initial revision
 *
 *     Revision 1.9  2001/02/21 08:39:22  davidk
 *     Code was retrofitted to use the new db_cli_base API
 *     in lieu of the oci_base() and OCI7 function API's.layer, which
 *     provides abstraction from the OCI7 function calls to
 *     ora_api layer code.  See comments in
 *     schema/src/include/internal/ewdb_cli_base.h for more info
 *     on the details of the changes made.
 *
 *     Revision 1.8  2000/06/21 22:51:35  lucky
 *     Cleaned up logit calls to make log files more readable.
 *
 *     Revision 1.7  2000/05/12 22:25:22  davidk
 *     fixed a bug where the idMag returned by the Create_Magnitude PL/SQL
 *     Procedure was not being written back into the caller's MagnitudeStruct.
 *
 *     Revision 1.6  2000/05/12 22:11:55  davidk
 *     Changed code to match the style of other API functions.  An API
 *     function calls a PrepXXX() and a PostXXX() function that copy
 *     variables in between the caller's struct and a local struct.
 *     The API function itself has no knowledge of the local struct.
 *     The Init_XXX() function only receives two parameters: a szStatement
 *     to be parsed, and a EWDB_OCIStatementStruct to hold binding
 *     information.  All binding is done to local variables, not user
 *     variable, so that a rebind does not need to occur for each new
 *     call.  EWDB_OCIStatementStruct.UseField assignments are set as
 *     constant in the struct declaration unless the SQL string is
 *     variable, meaning that the number of sql variables can change
 *     (example: when selecting an event list, sometimes you might use
 *     depth criteria and sometimes not).
 *
 *     Revision 1.5  2000/05/12 21:58:14  davidk
 *     added prototypes for the functions defined in this file.
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
	"Begin Create_Magnitude(OUT_idMag => :OUT_idMag,"
	"IN_sExternalTableName => '',"
	"IN_xidExternal => '',"
	"IN_sSource => :IN_sSource,"
	"IN_iMagType => :IN_iMagType,"
	"IN_dMagAvg => :IN_dMagAvg,"
  "IN_dMagErr => :IN_dMagErr,"
  "IN_iNumMags => :IN_iNumMags,"
	"IN_bBindToEvent => :IN_bBindToEvent,"
	"IN_bSetPreferred => :IN_bSetPreferred,"
	"IN_idEvent => :IN_idEvent,"
	"IN_idOrigin => :IN_idOrigin); End;";
	
static EWDB_OCI_SFS SQLParamsBindArray[] = 
{
  {0,1,0,0,0,OA_EWDBID,":OUT_idMag"},
  {0,1,0,0,0,OA_SZ,":IN_sSource"},
  {0,1,0,0,0,OA_INT,":IN_iMagType"},
  {0,1,0,0,0,OA_FLOAT,":IN_dMagAvg"},
  {0,1,0,0,0,OA_FLOAT,":IN_dMagErr"},
  {0,1,0,0,0,OA_INT,":IN_iNumMags"},
  {0,1,0,0,0,OA_INT,":IN_bBindToEvent"},
  {0,1,0,0,0,OA_INT,":IN_bSetPreferred"},
  {0,1,0,0,0,OA_EWDBID,":IN_idEvent"},
  {0,1,0,0,0,OA_EWDBID,":IN_idOrigin"}
};

#define	NUM_FIELDS	10

static char		Local_szdMag[15];
static char		Local_szdMagErr[15];
static	EWDB_MagStruct	Local_MagStruct;

/* Insertion Struct for CreateMagnitude szStatement */
EWDB_OCIStatementStruct SSStatement;

static int PrepCreateMagExec(EWDB_MagStruct *pMag, EWDB_Cursor *ppCursor);
static int PostCreateMagExec(EWDB_MagStruct *pMag);
static int InitCreateMagStatement(char *szStatement, EWDB_OCIStatementStruct *pSS);


int ewdb_api_CreateMagnitude(EWDB_MagStruct *pMag)
{

	EWDB_Cursor pCursor;
  int rc;

	if (pMag == NULL)
	{
		logit ("", "ewdb_api_CreateMagnitude(): Null parameters passed in!\n");
		return EWDB_RETURN_FAILURE;
	}

	ewdb_base_SetLastOraAPIActionTime();
	

	if (ewdb_base_Reconnect() != EWDB_RETURN_SUCCESS)
	/* Establishes connection, and performs binding!?! */
	{
		logit("","ewdb_api_CreateMagnitude():  Could not reconnect to the database!\n");
		return (EWDB_RETURN_FAILURE);
	}

	if (PrepCreateMagExec(pMag, &pCursor) != EWDB_RETURN_SUCCESS)
	{
		logit ("", "ewdb_api_CreateMagnitude(): PrepCreateMagExec() failed.\n");
		return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
	}

	if (ewdb_base_SQLExecute(pCursor))
	{
		logit ("", "ewdb_api_CreateMagnitude(): ewdb_base_SQLExecute() failed.\n");
		ewdb_base_ErrorReport (hEWDBC, pCursor,"CreateMagnitude:ewdb_base_SQLExecute", 1);
		return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
	} 

  
	/* Commit the transaction (all the previous inserts!)
	   In Case there is any auditing or logging or debug
	   changes made in the stored procedures.
	****************************************************/
	if (ewdb_base_SQLCommit(hEWDBC))
	{
		logit ("", "ewdb_api_CreateMagnitude(): ewdb_base_SQLCommit() failed.\n");
		ewdb_base_ErrorReport (hEWDBC, hEWDBC,"CreateMagnitude:ewdb_base_SQLCommit",2);
		return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
	}

  rc = PostCreateMagExec(pMag);
  if(rc == EWDB_RETURN_FAILURE)
  {
    logit("", "ewdb_api_CreateMagnitude(): PostCreateMagExec failed!\n");
    return(ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }
  else if(rc == EWDB_RETURN_WARNING)
  {
    return(EWDB_RETURN_FAILURE);
  }
  
  return(EWDB_RETURN_SUCCESS);
}  /* ewdb_api_CreateMagnitude() */


static int InitCreateMagStatement (char *szStatement, EWDB_OCIStatementStruct *pSS)
{
	pSS->FieldArray[0].pVal = &(Local_MagStruct.idMag);
	pSS->FieldArray[1].pVal = Local_MagStruct.szSource;
	pSS->FieldArray[2].pVal = &(Local_MagStruct.iMagType);
	pSS->FieldArray[3].pVal = Local_szdMag;
	pSS->FieldArray[4].pVal = Local_szdMagErr;
	pSS->FieldArray[5].pVal = &(Local_MagStruct.iNumMags);
	pSS->FieldArray[6].pVal = &(Local_MagStruct.bBindToEvent);
	pSS->FieldArray[7].pVal = &(Local_MagStruct.bSetPreferred);
	pSS->FieldArray[8].pVal = &(Local_MagStruct.idEvent);
	pSS->FieldArray[9].pVal = &(Local_MagStruct.idOrigin);

  return(ewdb_base_RequestCursor(szStatement, pSS,0/*don't force rebind*/));
} /* end InitCreateMagStatement() */


static int PrepCreateMagExec(EWDB_MagStruct *pMag, EWDB_Cursor *ppCursor)
{
	SSStatement.NumOfFields = NUM_FIELDS;
	SSStatement.FieldArray = SQLParamsBindArray;
	SSStatement.RecordSize = 0;

	memcpy (&Local_MagStruct, pMag, sizeof (EWDB_MagStruct));

  sprintf (Local_szdMag,    "%.3f", pMag->dMagAvg);
	sprintf (Local_szdMagErr, "%.3f", pMag->dMagErr);

	if (InitCreateMagStatement(SQL_STRING, &SSStatement)
      != EWDB_RETURN_SUCCESS)
	{
		logit ("", "PrepCreateMagExec(): InitCreateMagStatement failed!\n");
		return EWDB_RETURN_FAILURE;
	}

	*ppCursor = SSStatement.pCda;
	return(EWDB_RETURN_SUCCESS);
}  /* end PrepCreateMagExec() */


static int PostCreateMagExec (EWDB_MagStruct *pMag)
{
  EWDB_Cursor pCursor;
  
  pMag->idMag=Local_MagStruct.idMag;

  pCursor = SSStatement.pCda;
  
  ewdb_base_ReleaseCursor (pCursor);
  
  if(pMag->idMag <= 0)
  {
    if(pMag->idMag == -1)
      logit("","PostCreateMagExec():  SQL Proc Create_Magnitude() returned "
                "\"Unknown error\".  See SQL debug table for details.\n");
    else if(pMag->idMag == -2)
      logit("","PostCreateMagExec():  SQL Proc Create_Magnitude() returned "
                "\"Bad Magnitude Type\".  iMagType(%d) must be set to a valid "
                "Magnitude type from rw_mag.h.  If you are sure that your "
                "magnitude type is correct, check the MagType table from the DB"
                "to be sure it matches the magtypes from rw_mag.h\n",
            pMag->iMagType);
    else if(pMag->idMag == -8)
      logit("","PostCreateMagExec():  SQL Proc Create_Magnitude() returned "
                "\"Bad idEvent\".  If bBindToEvent is set to a non-zero value "
                "then idEvent must be a valid existing idEvent(%d) from the DB.\n",
            pMag->idEvent);
    else if(pMag->idMag == -7)
      logit("","PostCreateMagExec():  SQL Proc Create_Magnitude() returned "
                "\"Bad idOrigin\".  If idOrigin(%d) is set to a non-zero value.\n",
                "then it must be a valid existing idOrigin from the DB.\n",
            pMag->idOrigin);
    else
      logit("","PostCreateMagExec():  SQL Proc Create_Magnitude() returned "
               "the following error(%d).  Please see that proc for details.\n",
            pMag->idMag);
    return(EWDB_RETURN_WARNING);
  }

  return(EWDB_RETURN_SUCCESS);
}  /* end PostCreateMagExec() */



