/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */
/*                                                          */
/*    Revision history:                                     *
 *     $Log: ewdb_api_GetPlexorForChanT.c,v $
 *     Revision 1.3  2001/07/01 21:55:44  davidk
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
 *     Revision 1.2  2001/05/15 02:16:37  davidk
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
  "Begin Get_Plexor_For_ChanT(OUT_RetCode => :OUT_RetCode,"
  " OUT_idDeviceSlot => :OUT_idDeviceSlot, OUT_iPlexor => :OUT_iPlexor,"
  " IN_idChanT => :IN_idChanT, IN_idChan => :IN_idChan,"
  " IN_tTime => :IN_tTime); End;";

/* array of "bind params" structs for the above sql string */
static EWDB_OCI_SFS SQLParamsBindArray[] = 
{
  {0,1,  0,0,0,OA_INT,    ":OUT_RetCode"},
  {0,1,  0,0,0,OA_EWDBID, ":OUT_idDeviceSlot"},
  {0,1,  0,0,0,OA_INT,    ":OUT_iPlexor"},
  {0,1,  0,0,0,OA_EWDBID, ":IN_idChanT"},
  {0,1,  0,0,0,OA_EWDBID, ":IN_idChan"},
  {0,1,  0,0,0,OA_DOUBLE, ":IN_tTime"}
};

/* define the max number of usable fields in the SQLParamsBindArray */
static const int NumFieldsInBindArray = 6;

/* declare local copies of all the input/output params */
static int iRetCode;
static EWDBid idDeviceSlot, idChanT;
static int iPlexor, idChan;
static char sztTime[20];

/* declare Statement Struct */
static EWDB_OCIStatementStruct SSStatement;


/********************************
      FUNCTION PROTOTYPES
********************************/
int PrepGetPlexorForChanTExec(EWDBid IN_idChanT, int IN_idChan, double IN_tTime,
                          EWDB_Cursor * ppCursor);
int PostGetPlexorForChanTExec(int * pSQLRetCode, EWDBid * pidDeviceSlot, 
                          EWDBid * piPlexor);
int InitGetPlexorForChanTStatement(char *statement, EWDB_OCIStatementStruct *pSS);
/*******************************/



int ewdb_api_GetPlexorForChanT (EWDBid IN_idChanT, int IN_idChan,  double IN_tTime,
                       EWDBid * pidDeviceSlot, EWDBid * piPlexor, 
                       int * pSQLRetCode)
{

	EWDB_Cursor pCursor;

  /* Note that we are executing an OraAPI function, in case someone
     is thinking about cutting a connection */
	ewdb_base_SetLastOraAPIActionTime ();

  /* Check for NULL pointers */
  if (!(pSQLRetCode && pidDeviceSlot && piPlexor))
	{
		logit ("", "EWDB_GetPlexorForChanT(): Null pointers passed in!\n");
		return EWDB_RETURN_FAILURE;
	}

  /* Check for blatantly invalid DB id's */
	if (IN_idChanT <= 0)
	{
		logit ("", "EWDB_GetPlexorForChanT(): Invalid DB id's passed in!\n");
		return EWDB_RETURN_FAILURE;
	}

  /* Do any neccessary initialization */
  *pSQLRetCode = 0;  /* init the SQLRetCode to 0, so that we don't get
                        any unintended SQL return codes */

	/* Ensure we are connected to the DB. */
	if (ewdb_base_Reconnect () != EWDB_RETURN_SUCCESS)
	{
		logit ("", "EWDB_GetPlexorForChanT(): Could not reconnect to the database!\n");
		return (EWDB_RETURN_FAILURE);
	}

  /* Call our Prep() function */
	if (PrepGetPlexorForChanTExec (IN_idChanT, IN_idChan, IN_tTime, &pCursor)
      != EWDB_RETURN_SUCCESS)
	{
		logit ("", "ORA_API:GetPlexorForChanT():PrepGetPlexorForChanTExec() failed.\n");
		return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
	}

	if (ewdb_base_SQLExecute(pCursor))
	{
		ewdb_base_ErrorReport (hEWDBC, pCursor,"GetPlexorForChanT:ewdb_base_SQLExecute", 1);
		return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
	} 
  
	/* Commit the transaction (all the previous inserts!)
	   In Case there is any auditing or logging or debug
	   changes made in the stored procedures.
	****************************************************/
	if (ewdb_base_SQLCommit(hEWDBC))
	{
		ewdb_base_ErrorReport (hEWDBC, hEWDBC,"GetPlexorForChanT:ewdb_base_SQLCommit",2);
		return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
	}

  if (PostGetPlexorForChanTExec(pSQLRetCode, pidDeviceSlot, piPlexor)
      != EWDB_RETURN_SUCCESS)
  {
    logit ("", "Call to PostGetPlexorForChanTExec failed!\n");
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  }

	/* reset the timeout */
	ewdb_base_SetLastOraAPIActionTime ();

  /* We successfully completed. Return Success */
	return(EWDB_RETURN_SUCCESS);

} 



int InitGetPlexorForChanTStatement(char *statement, EWDB_OCIStatementStruct *pSS)
{
 
  pSS->FieldArray[0].pVal = &iRetCode;
  pSS->FieldArray[1].pVal = &idDeviceSlot;
  pSS->FieldArray[2].pVal = &iPlexor;
  pSS->FieldArray[3].pVal = &idChanT;
  pSS->FieldArray[4].pVal = &idChan;
  pSS->FieldArray[5].pVal = sztTime;

  ewdb_base_RequestCursor (statement, pSS, 0);

  return(EWDB_RETURN_SUCCESS);
}  /* End InitGetPlexorForChanTStatement() */


int PrepGetPlexorForChanTExec(EWDBid IN_idChanT, int IN_idChan, double IN_tTime,
                          EWDB_Cursor * ppCursor)
{

  /* Copy misc. local variables to the statement struct */
  SSStatement.NumOfFields = NumFieldsInBindArray;
  SSStatement.FieldArray = SQLParamsBindArray;
  SSStatement.RecordSize = 0;

  /* Copy input params to local variables */
  idChanT = IN_idChanT;
  idChan  = IN_idChan;
  sprintf(sztTime, "%10.4f", IN_tTime);
  
  if (InitGetPlexorForChanTStatement (SQL_STRING,
                           &SSStatement) != EWDB_RETURN_SUCCESS)
  {
    logit ("", "Call to InitGetPlexorForChanTStatement failed!\n");
    return EWDB_RETURN_FAILURE;
  }

  *ppCursor = SSStatement.pCda;
  
  return(EWDB_RETURN_SUCCESS);
}  /* End PrepGetPlexorForChanTExec() */


int PostGetPlexorForChanTExec(int * pSQLRetCode, EWDBid * pidDeviceSlot, 
                          EWDBid * piPlexor)
{

  *pSQLRetCode    = iRetCode;
  *pidDeviceSlot  = idDeviceSlot;
  *piPlexor       = iPlexor;

  /* Release the cursor since we are done */
  ewdb_base_ReleaseCursor(SSStatement.pCda);

  return(EWDB_RETURN_SUCCESS);
}  /* End PostGetPlexorForChanTExec() */

