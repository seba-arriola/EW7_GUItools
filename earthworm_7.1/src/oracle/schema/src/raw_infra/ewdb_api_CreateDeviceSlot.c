/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */
/*                                                          */

/*    Revision history:                                 
 *     $Log: ewdb_api_CreateDeviceSlot.c,v $
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
 *     Revision 1.2  2001/05/15 02:16:34  davidk
 *     Moved functions around between the apps, DB API, and DB API INTERNAL
 *     levels.  Renamed functions and files.  Added support for amplitude
 *     magnitude types.  Reformatted makefiles.
 *
 *     Revision 1.1  2001/02/28 17:19:22  lucky
 *     Initial revision
 *
 *     Revision 1.1  2001/02/21 09:59:01  davidk
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
  "Begin Create_DeviceSlot(OUT_RetCode => :OUT_RetCode,"
  " OUT_idDeviceSlot => :OUT_idDeviceSlot, "
  " IN_idSlotType => :IN_idSlotType,"
  " IN_sSlotName => :IN_sSlotName); End;";

/* array of "bind params" structs for the above sql string */
static EWDB_OCI_SFS SQLParamsBindArray[] = 
{
  {0,1,  0,0,0,OA_INT,    ":OUT_RetCode"},
  {0,1,  0,0,0,OA_EWDBID, ":OUT_idDeviceSlot"},
  {0,1,  0,0,0,OA_EWDBID, ":IN_idSlotType"},
  {0,1,  0,0,0,OA_SZ,     ":IN_sSlotName"}
};

/* define the max number of usable fields in the SQLParamsBindArray */
static const int NumFieldsInBindArray = 4;

/* declare local copies of all the input/output params */
static int iRetCode;
static EWDBid idDeviceSlot, idSlotType;
static char szSlotName[EWDB_RAW_INFRA_NAME_LEN + 1];

/* declare Statement Struct */
static EWDB_OCIStatementStruct SSStatement;


/********************************
      FUNCTION PROTOTYPES
********************************/
int PrepCreateDeviceSlotExec(EWDBid IN_idSlotType, char * IN_szSlotName, 
                             EWDB_Cursor * ppCursor);
int PostCreateDeviceSlotExec(int * pSQLRetCode, EWDBid * pidDeviceSlot);
int InitCreateDeviceSlotStatement(char *statement, EWDB_OCIStatementStruct *pSS);
/*******************************/



int ewdb_api_CreateDeviceSlot (EWDBid IN_idSlotType, char * IN_szSlotName, 
                          int * pSQLRetCode, EWDBid * pidDeviceSlot)
{

	EWDB_Cursor pCursor;

  /* Note that we are executing an OraAPI function, in case someone
     is thinking about cutting a connection */
	ewdb_base_SetLastOraAPIActionTime ();

  /* Check for NULL pointers */
  if (!(pSQLRetCode && pidDeviceSlot))
	{
		logit ("", "EWDB_CreateDeviceSlot(): Null pointers passed in!\n");
		return EWDB_RETURN_FAILURE;
	}

  /* Check for blatantly invalid DB id's */
	if (IN_idSlotType <= 0)
	{
		logit ("", "EWDB_CreateDeviceSlot(): Invalid DB id's passed in!\n");
		return EWDB_RETURN_FAILURE;
	}

  /* Do any neccessary initialization */
  *pSQLRetCode = 0;  /* init the SQLRetCode to 0, so that we don't get
                        any unintended SQL return codes */

	/* Ensure we are connected to the DB. */
	if (ewdb_base_Reconnect () != EWDB_RETURN_SUCCESS)
	{
		logit ("", "EWDB_CreateDeviceSlot(): Could not reconnect to the database!\n");
		return (EWDB_RETURN_FAILURE);
	}

  /* Call our Prep() function */
	if (PrepCreateDeviceSlotExec (IN_idSlotType, IN_szSlotName, &pCursor)
      != EWDB_RETURN_SUCCESS)
	{
		logit ("", "ORA_API:CreateDeviceSlot():PrepCreateDeviceSlotExec() failed.\n");
		return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
	}

	if (ewdb_base_SQLExecute(pCursor))
	{
		ewdb_base_ErrorReport (hEWDBC, pCursor,"CreateDeviceSlot:ewdb_base_SQLExecute", 1);
		return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
	} 
  
	/* Commit the transaction (all the previous inserts!)
	   In Case there is any auditing or logging or debug
	   changes made in the stored procedures.
	****************************************************/
	if (ewdb_base_SQLCommit(hEWDBC))
	{
		ewdb_base_ErrorReport (hEWDBC, hEWDBC,"CreateDeviceSlot:ewdb_base_SQLCommit",2);
		return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
	}


  if (PostCreateDeviceSlotExec (pSQLRetCode, pidDeviceSlot) != EWDB_RETURN_SUCCESS)
  {
    logit ("", "Call to PostCreateDeviceSlotExec failed!\n");
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  }

	/* reset the timeout */
	ewdb_base_SetLastOraAPIActionTime ();

  /* We successfully completed. Return Success */
	return(EWDB_RETURN_SUCCESS);

}  



int InitCreateDeviceSlotStatement(char *statement, EWDB_OCIStatementStruct *pSS)
{
 
  pSS->FieldArray[0].pVal = &iRetCode;
  pSS->FieldArray[1].pVal = &idDeviceSlot;
  pSS->FieldArray[2].pVal = &idSlotType;
  pSS->FieldArray[3].pVal = szSlotName;

  ewdb_base_RequestCursor (statement, pSS, 0);

  return(EWDB_RETURN_SUCCESS);
}  /* End InitCreateDeviceSlotStatement() */


int PrepCreateDeviceSlotExec(EWDBid IN_idSlotType, char * IN_szSlotName, 
                             EWDB_Cursor * ppCursor)
{

  /* Copy misc. local variables to the statement struct */
  SSStatement.NumOfFields = NumFieldsInBindArray;
  SSStatement.FieldArray = SQLParamsBindArray;
  SSStatement.RecordSize = 0;

  /* Copy input params to local variables */
  idSlotType  = IN_idSlotType;

  strncpy(szSlotName, IN_szSlotName, sizeof(szSlotName) - 1);
  szSlotName[sizeof(szSlotName) - 1] = 0x00;

  if (InitCreateDeviceSlotStatement (SQL_STRING,
                           &SSStatement) != EWDB_RETURN_SUCCESS)
  {
    logit ("", "Call to InitCreateDeviceSlotStatement failed!\n");
    return EWDB_RETURN_FAILURE;
  }

  *ppCursor = SSStatement.pCda;
  
  return(EWDB_RETURN_SUCCESS);
}  /* End PrepCreateDeviceSlotExec() */


int PostCreateDeviceSlotExec(int * pSQLRetCode, EWDBid * pidDeviceSlot)
{

  *pSQLRetCode = iRetCode;
  *pidDeviceSlot = idDeviceSlot;
  
  /* Release the cursor since we are done */
  ewdb_base_ReleaseCursor(SSStatement.pCda);

  return(EWDB_RETURN_SUCCESS);
}  /* End PostCreateDeviceSlotExec() */

