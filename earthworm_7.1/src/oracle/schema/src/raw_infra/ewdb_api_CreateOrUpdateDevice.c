/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */
/*                                                          */
/*    Revision history:                                     *
 *     $Log: ewdb_api_CreateOrUpdateDevice.c,v $
 *     Revision 1.3  2001/07/01 21:55:42  davidk
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
  "Begin Create_Or_Update_Device(OUT_RetCode => :OUT_RetCode,"
  " OUT_idDevice => :OUT_idDevice, "
  " IN_idDeviceModel => :IN_idDeviceModel, IN_sDeviceName => :IN_sDeviceName,"
  " IN_idDeviceType => :IN_idDeviceType,"
  " IN_bIsGeneric => :IN_bIsGeneric, IN_bIsRealizable => :IN_bIsRealizable"
  " ); End;";

/* array of "bind params" structs for the above sql string */
static EWDB_OCI_SFS SQLParamsBindArray[] = 
{
  {0,1,  0,0,0,OA_INT,    ":OUT_RetCode"},
  {0,1,  0,0,0,OA_EWDBID, ":OUT_idDevice"},
  {0,1,  0,0,0,OA_EWDBID, ":IN_idDeviceModel"},
  {0,1,  0,0,0,OA_SZ,     ":IN_sDeviceName"},
  {0,1,  0,0,0,OA_EWDBID, ":IN_idDeviceType"},
  {0,1,  0,0,0,OA_INT,    ":IN_bIsGeneric"},
  {0,1,  0,0,0,OA_INT,    ":IN_bIsRealizable"}
};

/* define the max number of usable fields in the SQLParamsBindArray */
static const int NumFieldsInBindArray = 7;

/* declare local copies of all the input/output params */
static int iRetCode;
static EWDBid idDevice, idDeviceModel, idDeviceType;
static int bIsGeneric, bIsRealizable;
static char szDeviceName[EWDB_RAW_INFRA_NAME_LEN + 1];

/* declare Statement Struct */
static EWDB_OCIStatementStruct SSStatement;


/********************************
      FUNCTION PROTOTYPES
********************************/
int PrepCreateOrUpdateDeviceExec(EWDBid IN_idDeviceModel, 
                                 char * IN_szDeviceName, EWDBid IN_idDeviceType, 
                                 int IN_bIsGeneric, int IN_bIsRealizable ,
                                 EWDBid IN_idDevice, EWDB_Cursor * ppCursor);
int PostCreateOrUpdateDeviceExec(int * pSQLRetCode, EWDBid * pidDevice);
int InitCreateOrUpdateDeviceStatement(char *statement, EWDB_OCIStatementStruct *pSS);
/*******************************/



int ewdb_api_CreateOrUpdateDevice (EWDBid IN_idDeviceModel, 
                              char * IN_szDeviceName, EWDBid IN_idDeviceType, 
                              int IN_bIsGeneric, int IN_bIsRealizable,
                              int * pSQLRetCode, EWDBid * pidDevice)
{

	EWDB_Cursor pCursor;

  /* Note that we are executing an OraAPI function, in case someone
     is thinking about cutting a connection */
	ewdb_base_SetLastOraAPIActionTime ();

  /* Check for NULL pointers */
  if (!(pSQLRetCode && pidDevice && IN_szDeviceName))
	{
		logit ("", "EWDB_CreateOrUpdateDevice(): Null pointers passed in!\n");
		return EWDB_RETURN_FAILURE;
	}

  /* Check for blatantly invalid DB id's */
	if (IN_idDeviceModel <= 0 || IN_idDeviceType <= 0)
	{
		logit ("", "EWDB_CreateOrUpdateDevice(): Invalid DB id's passed in!\n");
		return EWDB_RETURN_FAILURE;
	}

  /* Do any neccessary initialization */
  *pSQLRetCode = 0;  /* init the SQLRetCode to 0, so that we don't get
                        any unintended SQL return codes */

	/* Ensure we are connected to the DB. */
	if (ewdb_base_Reconnect () != EWDB_RETURN_SUCCESS)
	{
		logit ("", "EWDB_CreateOrUpdateDevice(): Could not reconnect to the database!\n");
		return (EWDB_RETURN_FAILURE);
	}

  /* Call our Prep() function */
	if (PrepCreateOrUpdateDeviceExec (IN_idDeviceModel, IN_szDeviceName, 
                                    IN_idDeviceType, 
                                    IN_bIsGeneric, IN_bIsRealizable, 
                                    *pidDevice, &pCursor)
      != EWDB_RETURN_SUCCESS)
	{
		logit ("", "ORA_API:CreateOrUpdateDevice():PrepCreateOrUpdateDeviceExec() failed.\n");
		return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
	}

	if (ewdb_base_SQLExecute(pCursor))
	{
		ewdb_base_ErrorReport (hEWDBC, pCursor,"CreateOrUpdateDevice:ewdb_base_SQLExecute", 1);
		return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
	} 
  
	/* Commit the transaction (all the previous inserts!)
	   In Case there is any auditing or logging or debug
	   changes made in the stored procedures.
	****************************************************/
	if (ewdb_base_SQLCommit(hEWDBC))
	{
		ewdb_base_ErrorReport (hEWDBC, hEWDBC,"CreateOrUpdateDevice:ewdb_base_SQLCommit",2);
		return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
	}


  if (PostCreateOrUpdateDeviceExec (pSQLRetCode, pidDevice) != EWDB_RETURN_SUCCESS)
  {
    logit ("", "Call to PostCreateOrUpdateDeviceExec failed!\n");
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  }

	/* reset the timeout */
	ewdb_base_SetLastOraAPIActionTime ();

  /* We successfully completed. Return Success */
	return(EWDB_RETURN_SUCCESS);

}  



int InitCreateOrUpdateDeviceStatement(char *statement, EWDB_OCIStatementStruct *pSS)
{
 
  pSS->FieldArray[0].pVal = &iRetCode;
  pSS->FieldArray[1].pVal = &idDevice;
  pSS->FieldArray[2].pVal = &idDeviceModel;
  pSS->FieldArray[3].pVal = szDeviceName;
  pSS->FieldArray[4].pVal = &idDeviceType;
  pSS->FieldArray[5].pVal = &bIsGeneric;
  pSS->FieldArray[6].pVal = &bIsRealizable;

  ewdb_base_RequestCursor (statement, pSS, 0);

  return(EWDB_RETURN_SUCCESS);
}  /* End InitCreateOrUpdateDeviceStatement() */


int PrepCreateOrUpdateDeviceExec(EWDBid IN_idDeviceModel, 
                                 char * IN_szDeviceName, EWDBid IN_idDeviceType, 
                                 int IN_bIsGeneric, int IN_bIsRealizable, 
                                 EWDBid IN_idDevice, EWDB_Cursor * ppCursor)
{

  /* Copy misc. local variables to the statement struct */
  SSStatement.NumOfFields = NumFieldsInBindArray;
  SSStatement.FieldArray = SQLParamsBindArray;
  SSStatement.RecordSize = 0;

  /* Copy input params to local variables */
  idDevice       = IN_idDevice;
  idDeviceModel  = IN_idDeviceModel;
  idDeviceType   = IN_idDeviceType;
  bIsGeneric     = IN_bIsGeneric;
  bIsRealizable  = IN_bIsRealizable;

  strncpy(szDeviceName, IN_szDeviceName, sizeof(szDeviceName) - 1);
  szDeviceName[sizeof(szDeviceName) - 1] = 0x00;

  if (InitCreateOrUpdateDeviceStatement (SQL_STRING,
                           &SSStatement) != EWDB_RETURN_SUCCESS)
  {
    logit ("", "Call to InitCreateOrUpdateDeviceStatement failed!\n");
    return EWDB_RETURN_FAILURE;
  }

  *ppCursor = SSStatement.pCda;
  
  return(EWDB_RETURN_SUCCESS);
}  /* End PrepCreateOrUpdateDeviceExec() */


int PostCreateOrUpdateDeviceExec(int * pSQLRetCode, EWDBid * pidDevice)
{

  *pSQLRetCode = iRetCode;
  *pidDevice   = idDevice;
  
  /* Release the cursor since we are done */
  ewdb_base_ReleaseCursor(SSStatement.pCda);

  return(EWDB_RETURN_SUCCESS);
}  /* End PostCreateOrUpdateDeviceExec() */

