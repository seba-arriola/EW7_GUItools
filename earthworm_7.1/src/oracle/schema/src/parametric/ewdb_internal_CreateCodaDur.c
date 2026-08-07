/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_internal_CreateCodaDur.c,v 1.3 2005/06/15 19:03:12 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_internal_CreateCodaDur.c,v $
 *     Revision 1.3  2005/06/15 19:03:12  davidk
 *     DB API Cleanup
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
 *     Revision 1.6  2000/05/12 18:00:16  davidk
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
 *     Revision 1.5  2000/05/12 17:36:07  davidk
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
	"Begin Create_CodaDur(OUT_idCodaDur => :OUT_idCodaDur,"
	"IN_idTCoda => :IN_idTCoda,"
	"IN_idPick => :IN_idPick); End;";
	
static EWDB_OCI_SFS SQLParamsBindArray[] = 
{
  {0,1,0,0,0,OA_INT,    ":OUT_idCodaDur"},
  {0,1,0,0,0,OA_INT,    ":IN_idTCoda"},
  {0,1,0,0,0,OA_INT,    ":IN_idPick"},
};

#define	NUM_FIELDS	3


/* Insertion Struct for CreateCodaDur szStatement */
static EWDB_OCIStatementStruct SSStatement;

static  EWDB_CodaDurationStruct     Local_PhaseStruct;


static int PrepCreateCodaDurExec(EWDB_CodaDurationStruct *pCodaDur, EWDB_Cursor *ppCursor);
static int PostCreateCodaDurExec(EWDB_CodaDurationStruct *pCodaDur);
static int InitCreateCodaDurStatement(char *szStatement, EWDB_OCIStatementStruct *pSS);

int ewdb_internal_CreateCodaDur(EWDB_CodaDurationStruct *pCodaDur)
{

	EWDB_Cursor pCursor;
  int rc;

	if (pCodaDur == NULL)
	{
		logit ("", "ewdb_internal_CreateCodaDur(): Null parameters passed in!\n");
		return EWDB_RETURN_FAILURE;
	}

	ewdb_base_SetLastOraAPIActionTime ();

	if (ewdb_base_Reconnect () != EWDB_RETURN_SUCCESS)
	/* Establishes connection, and performs binding!?! */
	{
		logit ("", "ewdb_internal_CreateCodaDur(): Could not reconnect to the database!\n");
		return (EWDB_RETURN_FAILURE);
	}

	if (PrepCreateCodaDurExec(pCodaDur, &pCursor) != EWDB_RETURN_SUCCESS)
	{
		logit ("", "ewdb_internal_CreateCodaDur(): PrepEWDB_CreateCodaDur() failed.\n");
		return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
	}

	if (ewdb_base_SQLExecute (pCursor))
	{
		ewdb_base_ErrorReport (hEWDBC, pCursor,"ewdb_internal_CreateCodaDur(): ewdb_base_SQLExecute", 1);
		return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
	} 

	/* Commit the transaction (all the previous inserts!)
	   In Case there is any auditing or logging or debug
	   changes made in the stored procedures.
	****************************************************/
	if (ewdb_base_SQLCommit (hEWDBC))
	{
		ewdb_base_ErrorReport (hEWDBC, hEWDBC,"ewdb_internal_CreateCodaDur(): ewdb_base_SQLCommit",2);
		return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
	}

  rc = PostCreateCodaDurExec(pCodaDur);
  if(rc == EWDB_RETURN_FAILURE)
  {
    logit("", "ewdb_internal_CreateCodaDur(): PostCreateCodaDurExec failed!\n");
    return(ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }
  else if(rc == EWDB_RETURN_WARNING)
  {
    return(EWDB_RETURN_FAILURE);
  }

	return(EWDB_RETURN_SUCCESS);
}  /* end ewdb_internal_CreateCodaDur() */


static int InitCreateCodaDurStatement(char *szStatement, EWDB_OCIStatementStruct *pSS)
{
	pSS->FieldArray[0].pVal = &(Local_PhaseStruct.idCodaDur);
	pSS->FieldArray[1].pVal = &(Local_PhaseStruct.idTCoda);
	pSS->FieldArray[2].pVal = &(Local_PhaseStruct.idPick);

  return(ewdb_base_RequestCursor(szStatement, pSS,0/*don't force rebind*/));
}  /* End InitCreateCodaDurStatement() */


static int PrepCreateCodaDurExec(EWDB_CodaDurationStruct *pCodaDur, EWDB_Cursor *ppCursor)
{

	if ((pCodaDur == NULL) || (ppCursor == NULL))
	{
		logit ("", "PrepCreateCodaDurExec(): Null parameters passed in!\n");
		return EWDB_RETURN_FAILURE;
	}

	memcpy (&Local_PhaseStruct, pCodaDur, sizeof (EWDB_CodaDurationStruct));

	SSStatement.NumOfFields = NUM_FIELDS;
	SSStatement.FieldArray = SQLParamsBindArray;
	SSStatement.RecordSize = 0;

	if(InitCreateCodaDurStatement(SQL_STRING, &SSStatement)
     != EWDB_RETURN_SUCCESS)
	{
		logit ("", "PrepCreateCodaDurExec(): InitCreateCodaDurStatement failed!\n");
		return EWDB_RETURN_FAILURE;
	}

	*ppCursor = SSStatement.pCda;
	
	return EWDB_RETURN_SUCCESS;
}  /* End PrepCreateCodaDurExec() */


static int PostCreateCodaDurExec(EWDB_CodaDurationStruct *pCodaDur)
{
  EWDB_Cursor pCursor;
  
  if (pCodaDur == NULL)
  {
    logit ("", "PostCreateCodaDurExec(): Invalid arguments passed in.\n");
    return EWDB_RETURN_FAILURE;
  }
  
  pCodaDur->idCodaDur = Local_PhaseStruct.idCodaDur;

  pCursor = SSStatement.pCda;

  ewdb_base_ReleaseCursor(pCursor);
  
  if(pCodaDur->idCodaDur <= 0)
  {
    if(pCodaDur->idCodaDur == -1)
      logit("","PostCreateCodaDurExec():  SQL Proc Create_CodaDur() returned "
                "\"Unknown error\".  See SQL debug table for details.\n");
    else if(pCodaDur->idCodaDur == -2)
      logit("","PostCreateCodaDurExec():  SQL Proc Create_CodaDur() returned "
                "\"Incorrect Phase Type\".  idPick(%d) must reference a valid "
                "P-Phase Pick.  Other types of phases may not be used for "
                "calculating duration mags.\n", 
            pCodaDur->idPick);
    else if(pCodaDur->idCodaDur <= -30 && pCodaDur->idCodaDur > -40)
      logit("","PostCreateCodaDurExec():  SQL Proc Create_CodaDur() returned "
                "\"Unable to Process Phase \".  idPick must reference a valid "
                "P-Phase Pick, already stored in the database.  The given "
                "idPick(%d) was invalid.\n", 
            pCodaDur->idPick);
    else if(pCodaDur->idCodaDur <= -40 && pCodaDur->idCodaDur > -50)
      logit("","PostCreateCodaDurExec():  SQL Proc Create_CodaDur() returned "
                "\"Unable to Process Coda Termination \".  idTCoda must "
                "reference a valid coda termination, already stored in the "
                " database.  The given idTCoda(%d) was invalid.\n", 
            pCodaDur->idTCoda);
    else
      logit("","PostCreateCodaDurExec():  SQL Proc Create_Phase() returned "
               "the following error(%d).  Please see that proc for details.\n",
            pCodaDur->idCodaDur);
    return(EWDB_RETURN_WARNING);
  }

  return(EWDB_RETURN_SUCCESS);
}  /* End PostCreateCodaDurExec() */
