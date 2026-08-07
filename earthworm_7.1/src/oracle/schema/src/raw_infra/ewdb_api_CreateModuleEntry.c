/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */
/*                                                          */
/*    Revision history:                                     *
 *     $Log: ewdb_api_CreateModuleEntry.c,v $
 *     Revision 1.3  2001/07/01 21:55:41  davidk
 *     Cleanup of the Earthworm Database API and the applications that utilize it.
 *     The "ewdb_api" was cleanup in preparation for Earthworm v6.0, which is
 *     supposed to contain an API that will be backwards compatible in the
 *     future.  Functions were removed, parameters were changed, syntax was
 *     rewritten, and other stuff was done to try to get the code to follow a
 *     general format, so that it would be easier to read.
 *
 *     Applications were modified to handle changed API calls and structures.
 *     They were also modified to be compiler warning free on WinNT.
 *
 *     Revision 1.2  2001/05/15 02:16:35  davidk
 *     Moved functions around between the apps, DB API, and DB API INTERNAL
 *     levels.  Renamed functions and files.  Added support for amplitude
 *     magnitude types.  Reformatted makefiles.
 *
 *     Revision 1.1  2001/02/28 17:19:22  lucky
 *     Initial revision
 *
 *     Revision 1.1  2001/02/21 10:05:04  davidk
 *     Initial revision
 *
 *                                                          */

/* standard includes */
#include <stdlib.h>
#include <stdio.h>
#include <ewdb_ora_api.h>
#include <ewdb_cli_base.h>
#include <ewdb_ew_oci_base.h>


/* sql execution string */
static char SQL_STRING[] =
  "Begin Create_ModuleEntry(OUT_RetCode => :OUT_RetCode,"
  " OUT_idModuleEntry => :OUT_idModuleEntry, "
  " IN_idModule => :IN_idModule, IN_iPlexor => :IN_iPlexor,"
  " IN_idSubDevSlot => :IN_idSubDevSlot, "
  " IN_iSubPlexor => :IN_iSubPlexor); End;";

/* array of "bind params" structs for the above sql string */
static EWDB_OCI_SFS SQLParamsBindArray[] = 
{
  {0,1,  0,0,0,OA_INT,    ":OUT_RetCode"},
  {0,1,  0,0,0,OA_EWDBID, ":OUT_idModuleEntry"},
  {0,1,  0,0,0,OA_EWDBID, ":IN_idModule"},
  {0,1,  0,0,0,OA_INT,    ":IN_iPlexor"},
  {0,1,  0,0,0,OA_EWDBID, ":IN_idSubDevSlot"},
  {0,1,  0,0,0,OA_INT,    ":IN_iSubPlexor"}
};

/***************************/
/*   sample bind structs   */
/***************************/
/* 
 * input params or procedure params 
 * integer
  {0,1,  0,0,0,OA_INT,    ":OUT_RetCode"}
 * float
  {0,1,  0,0,0,OA_FLOAT,  ":OUT_dNumber"}
 * double
  {0,1,  0,0,0,OA_DOUBLE, ":OUT_dNumber"}
 * string
  {0,1,  0,0,0,OA_SZ,     ":OUT_sString"}
 * EWDBid
  {0,1,  0,0,0,OA_EWDBID, ":OUT_idRecord"}
 * char  (single)
  {0,1,  0,0,0,OA_CHAR,   "1cPZType"}


 * list retrieval
 * integer
  {0,1,  0,0,0,OA_INT,    "5iNumber"},
 * float
  {0,1, 20,0,0,OA_FLOAT,  "1dNumber"}
 * double
  {0,1, 20,0,0,OA_DOUBLE, "2dNumber"}
 * string
  {0,1, 10,0,0,OA_SZ,     "12sString"},
 * EWDBid
  {0,1,  0,0,0,OA_EWDBID, "5idRecord"},
 * char  (single)
  {0,1,  0,0,0,OA_CHAR,   "1cPZType"}
*/
/* end sample bind structs */
/***************************/

/* define the max number of usable fields in the SQLParamsBindArray */
static const int NumFieldsInBindArray = 6;

/* declare local copies of all the input/output params */
static int iRetCode;
static EWDBid idModuleEntry, idModule, idSubDevSlot;
static int  iPlexor, iSubPlexor;

/* declare Statement Struct */
static EWDB_OCIStatementStruct SSStatement;


/********************************
      FUNCTION PROTOTYPES
********************************/
int PrepCreateModuleEntryExec(EWDBid IN_idModule, int IN_iPlexor, 
                              EWDBid IN_idSubDevSlot, int IN_iSubPlexor, 
                              EWDB_Cursor * ppCursor);
int PostCreateModuleEntryExec(int * pSQLRetCode, EWDBid * pidModuleEntry);
int InitCreateModuleEntryStatement(char *statement, EWDB_OCIStatementStruct *pSS);
/*******************************/



int ewdb_api_CreateModuleEntry (EWDBid IN_idModule, int IN_iPlexor, 
                           EWDBid IN_idSubDevSlot, int IN_iSubPlexor, 
                           int * pSQLRetCode, EWDBid * pidModuleEntry)
{

	EWDB_Cursor pCursor;

  /* Note that we are executing an OraAPI function, in case someone
     is thinking about cutting a connection */
	ewdb_base_SetLastOraAPIActionTime ();

  /* Check for NULL pointers */
  if (!(pSQLRetCode && pidModuleEntry))
	{
		logit ("", "EWDB_CreateModuleEntry(): Null pointers passed in!\n");
		return EWDB_RETURN_FAILURE;
	}

  /* Check for blatantly invalid DB id's */
	if (IN_idModule <= 0 || IN_iPlexor <= 0 )
	{
		logit ("", "EWDB_CreateModuleEntry(): Invalid DB id/plexor's passed in!\n");
		return EWDB_RETURN_FAILURE;
	}

  /* Do any neccessary initialization */
  *pSQLRetCode = 0;  /* init the SQLRetCode to 0, so that we don't get
                        any unintended SQL return codes */

	/* Ensure we are connected to the DB. */
	if (ewdb_base_Reconnect () != EWDB_RETURN_SUCCESS)
	{
		logit ("", "EWDB_CreateModuleEntry(): Could not reconnect to the database!\n");
		return (EWDB_RETURN_FAILURE);
	}

  /* Call our Prep() function */
	if (PrepCreateModuleEntryExec (IN_idModule, IN_iPlexor,
                                 IN_idSubDevSlot, IN_iSubPlexor,
                                 &pCursor)
      != EWDB_RETURN_SUCCESS)
	{
		logit ("", "ORA_API:CreateModuleEntry():PrepCreateModuleEntryExec() failed.\n");
		return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
	}

	if (ewdb_base_SQLExecute(pCursor))
	{
		ewdb_base_ErrorReport (hEWDBC, pCursor,"CreateModuleEntry:ewdb_base_SQLExecute", 1);
		return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
	} 
  
	/* Commit the transaction (all the previous inserts!)
	   In Case there is any auditing or logging or debug
	   changes made in the stored procedures.
	****************************************************/
	if (ewdb_base_SQLCommit(hEWDBC))
	{
		ewdb_base_ErrorReport (hEWDBC, hEWDBC,"CreateModuleEntry:ewdb_base_SQLCommit",2);
		return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
	}


  if (PostCreateModuleEntryExec (pSQLRetCode, pidModuleEntry) != EWDB_RETURN_SUCCESS)
  {
    logit ("", "Call to PostCreateModuleEntryExec failed!\n");
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  }

	/* reset the timeout */
	ewdb_base_SetLastOraAPIActionTime ();

  /* We successfully completed. Return Success */
	return(EWDB_RETURN_SUCCESS);

} 



int InitCreateModuleEntryStatement(char *statement, EWDB_OCIStatementStruct *pSS)
{
 
  pSS->FieldArray[0].pVal = &iRetCode;
  pSS->FieldArray[1].pVal = &idModuleEntry;
  pSS->FieldArray[2].pVal = &idModule;
  pSS->FieldArray[3].pVal = &iPlexor;
  pSS->FieldArray[4].pVal = &idSubDevSlot;
  pSS->FieldArray[5].pVal = &iSubPlexor;

  ewdb_base_RequestCursor (statement, pSS, 0);

  return(EWDB_RETURN_SUCCESS);
}  /* End InitCreateModuleEntryStatement() */


int PrepCreateModuleEntryExec(EWDBid IN_idModule, int IN_iPlexor, 
                              EWDBid IN_idSubDevSlot, int IN_iSubPlexor, 
                              EWDB_Cursor * ppCursor)
{

  /* Copy misc. local variables to the statement struct */
  SSStatement.NumOfFields = NumFieldsInBindArray;
  SSStatement.FieldArray = SQLParamsBindArray;
  SSStatement.RecordSize = 0;

  /* Copy input params to local variables */
  idModule        = IN_idModule;
  iPlexor         = IN_iPlexor;
  idSubDevSlot    = IN_idSubDevSlot;
  iSubPlexor      = IN_iSubPlexor;

  if (InitCreateModuleEntryStatement (SQL_STRING,
                           &SSStatement) != EWDB_RETURN_SUCCESS)
  {
    logit ("", "Call to InitCreateModuleEntryStatement failed!\n");
    return EWDB_RETURN_FAILURE;
  }

  *ppCursor = SSStatement.pCda;
  
  return(EWDB_RETURN_SUCCESS);
}  /* End PrepCreateModuleEntryExec() */


int PostCreateModuleEntryExec(int * pSQLRetCode, EWDBid * pidModuleEntry)
{

  *pSQLRetCode    = iRetCode;
  *pidModuleEntry = idModuleEntry;
  
  /* Release the cursor since we are done */
  ewdb_base_ReleaseCursor(SSStatement.pCda);

  return(EWDB_RETURN_SUCCESS);
}  /* End PostCreateModuleEntryExec() */

