/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_internal_CreateTCoda.c,v 1.3 2005/06/15 19:03:12 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_internal_CreateTCoda.c,v $
 *     Revision 1.3  2005/06/15 19:03:12  davidk
 *     DB API Cleanup
 *
 *     Revision 1.2  2001/07/12 21:07:17  davidk
 *     Cleaned up the function as part of the API cleanup.  Added improved
 *     error logging, handling, and reporting.
 *
 *     Revision 1.1  2001/05/15 02:16:19  davidk
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
 *     Revision 1.6  2000/05/12 23:02:50  davidk
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
 *     Revision 1.5  2000/05/12 22:44:15  davidk
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
	"Begin Create_TCoda(OUT_idTCoda => :OUT_idTCoda,"
	"IN_sExternalTableName => :IN_sExternalTableName,"
	"IN_xidExternal => :IN_xidExternal,"
	"IN_idChan => :IN_idChan,"
	"IN_tCodaTermObs => :IN_tCodaTermObs,"
	"IN_tCodaTermXtp => :IN_tCodaTermXtp); End;";
	
static EWDB_OCI_SFS SQLParamsBindArray[] = 
{
  {0,1,0,0,0,OA_INT,    ":OUT_idTCoda"},
  {0,1,0,0,0,OA_SZ,     ":IN_sExternalTableName"},
  {0,1,0,0,0,OA_SZ,     ":IN_xidExternal"},
  {0,1,0,0,0,OA_INT,    ":IN_idChan"},
  {0,1,0,0,0,OA_DOUBLE, ":IN_tCodaTermObs"},
  {0,1,0,0,0,OA_DOUBLE, ":IN_tCodaTermXtp"},
};

#define	NUM_FIELDS	6

static char		Local_sztCodaTermObs[15];
static char		Local_sztCodaTermXtp[15];
static char   Local_szExternalTableName[1];
static char   Local_szxidExternal[1];

static	EWDB_CodaDurationStruct  Local_PhaseStruct;

/* Insertion Struct for CreateTCoda szStatement */
static EWDB_OCIStatementStruct SSStatement;

/********************************
      FUNCTION PROTOTYPES
********************************/
static int PrepCreateTCodaExec (EWDB_CodaDurationStruct *pTCoda, EWDB_Cursor *ppCursor);
static int PostCreateTCodaExec (EWDB_CodaDurationStruct *pTCoda);
static int InitCreateTCodaStatement (char *szStatement, EWDB_OCIStatementStruct *pSS);
/*******************************/


int ewdb_internal_CreateTCoda(EWDB_CodaDurationStruct *pTCoda)
{

	EWDB_Cursor pCursor;
  int rc;


	if (pTCoda == NULL)
	{
		logit ("", "ewdb_internal_CreateTCoda(): Null parameters passed in!\n");
		return EWDB_RETURN_FAILURE;
	}

	ewdb_base_SetLastOraAPIActionTime ();


	if(ewdb_base_Reconnect() != EWDB_RETURN_SUCCESS)
	/* Establishes connection, and performs binding!?! */
	{
		logit ("", "ewdb_internal_CreateTCoda(): Could not reconnect to the database!\n");
		return (EWDB_RETURN_FAILURE);
	}

	if(PrepCreateTCodaExec(pTCoda, &pCursor) != EWDB_RETURN_SUCCESS)
	{
		logit ("", "ewdb_internal_CreateTCoda(): PrepEWDB_CreateTCoda() failed.\n");
		return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
	}

	if(ewdb_base_SQLExecute(pCursor))
	{
		ewdb_base_ErrorReport(hEWDBC, pCursor,"ewdb_internal_CreateTCoda(): ewdb_base_SQLExecute", 1);
		return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
	} 

	/* Commit the transaction (all the previous inserts!)
	   In Case there is any auditing or logging or debug
	   changes made in the stored procedures.
	****************************************************/
	if (ewdb_base_SQLCommit (hEWDBC))
	{
		ewdb_base_ErrorReport (hEWDBC, hEWDBC,"ewdb_internal_CreateTCoda(): ewdb_base_SQLCommit",2);
		return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
	}

  rc = PostCreateTCodaExec(pTCoda);
  if(rc == EWDB_RETURN_FAILURE)
  {
    logit("", "ewdb_internal_CreateTCoda(): PostCreateTCodaExec failed!\n");
    return(ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }
  else if(rc == EWDB_RETURN_WARNING)
  {
    return(EWDB_RETURN_FAILURE);
  }
  
  return(EWDB_RETURN_SUCCESS);
}  /* end ewdb_internal_CreateTCoda() */


static int InitCreateTCodaStatement (char *szStatement, EWDB_OCIStatementStruct *pSS)
{

	pSS->FieldArray[0].pVal = &(Local_PhaseStruct.idTCoda);
	pSS->FieldArray[1].pVal = Local_szExternalTableName;
	pSS->FieldArray[2].pVal = Local_szxidExternal;
	pSS->FieldArray[3].pVal = &(Local_PhaseStruct.idChan);
	pSS->FieldArray[4].pVal = Local_sztCodaTermObs;
	pSS->FieldArray[5].pVal = Local_sztCodaTermXtp;

  if (ewdb_base_RequestCursor (szStatement, pSS, 0) != 0)
  {
    logit ("", "InitCreateTCodaStatement(): "
           "Call to ewdb_base_RequestCursor failed.\n");
    return EWDB_RETURN_FAILURE;
  }

	return EWDB_RETURN_SUCCESS;
}


static int PrepCreateTCodaExec (EWDB_CodaDurationStruct *pTCoda, EWDB_Cursor *ppCursor)
{

	if ((pTCoda == NULL) || (ppCursor == NULL))
	{
		logit ("", "PrepCreateTCodaExec(): Null parameters passed in!\n");
		return EWDB_RETURN_FAILURE;
	}

	SSStatement.NumOfFields = NUM_FIELDS;
	SSStatement.FieldArray = SQLParamsBindArray;
	SSStatement.RecordSize = 0;

	memcpy (&Local_PhaseStruct, pTCoda, sizeof (EWDB_CodaDurationStruct));

  Local_szExternalTableName[0] = 0x00;
  Local_szxidExternal[0] = 0x00;

  sprintf (Local_sztCodaTermObs, "%0.2f", pTCoda->tCodaTermObs);
	sprintf (Local_sztCodaTermXtp, "%0.2f", pTCoda->tCodaTermXtp);

	if(InitCreateTCodaStatement(SQL_STRING,&SSStatement) 
     != EWDB_RETURN_SUCCESS)
	{
		logit ("", "PrepCreateTCodaExec(): InitCreateTCodaStatement failed!\n");
		return EWDB_RETURN_FAILURE;
	}

	*ppCursor = SSStatement.pCda;
	
	return EWDB_RETURN_SUCCESS;
} /* end PrepCreateTCodaExec() */


static int PostCreateTCodaExec(EWDB_CodaDurationStruct *pTCoda)
{
  EWDB_Cursor pCursor;
  
  if (pTCoda == NULL)
  {
    logit ("", "PostCreateTCodaExec(): Invalid arguments passed in.\n");
    return EWDB_RETURN_FAILURE;
  }
  
  pTCoda->idTCoda=Local_PhaseStruct.idTCoda;
  
  pCursor = SSStatement.pCda;
  
  ewdb_base_ReleaseCursor (pCursor);
  
  if(pTCoda->idTCoda <= 0)
  {
    if(pTCoda->idTCoda == -1)
      logit("","PostCreateTCodaExec():  SQL Proc Create_TCoda() returned "
                "\"Unknown error\".  See SQL debug table for details.\n");
    else
      logit("","PostCreateTCodaExec():  SQL Proc Create_TCoda() returned "
               "the following error(%d).  Please see that proc for details.\n",
            pTCoda->idTCoda);
    return(EWDB_RETURN_WARNING);
  }

  return(EWDB_RETURN_SUCCESS);
}  /* end PostCreateTCodaExec() */
