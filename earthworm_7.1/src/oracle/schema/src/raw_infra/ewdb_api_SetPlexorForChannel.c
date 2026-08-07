/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */
/*                                                          */
/*    Revision history:                                     *
 *     $Log: ewdb_api_SetPlexorForChannel.c,v $
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
 *     Revision 1.2  2001/05/15 02:16:38  davidk
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
  "Begin Set_Plexor_For_Channel(OUT_RetCode => :OUT_RetCode,"
  " OUT_idChanT => :OUT_idChanT, "
  " IN_idChan => :IN_idChan,"
  " IN_idDeviceSlot => :IN_idDeviceSlot," 
  " IN_iPlexor => :IN_iPlexor," 
  " IN_tOff => :IN_tOff, IN_tOn=>:IN_tOn); End;";

/* array of "bind params" structs for the above sql string */
static EWDB_OCI_SFS SQLParamsBindArray[] = 
{
  {0,1,  0,0,0,OA_INT,    ":OUT_RetCode"},
  {0,1,  0,0,0,OA_EWDBID, ":OUT_idChanT"},
  {0,1,  0,0,0,OA_EWDBID, ":IN_idChan"},
  {0,1,  0,0,0,OA_EWDBID, ":IN_idDeviceSlot"},
  {0,1,  0,0,0,OA_INT,    ":IN_iPlexor"},
  {0,1,  0,0,0,OA_DOUBLE, ":IN_tOff"},
  {0,1,  0,0,0,OA_DOUBLE, ":IN_tOn"}
};

/* define the max number of usable fields in the SQLParamsBindArray */
static const int NumFieldsInBindArray = 7;

/* declare local copies of all the input/output params */
static int iRetCode;
static EWDBid idChanT, idChan, idDeviceSlot;
static int  iPlexor;
static char szTOn[EWDB_DOUBLE_STRING_LEN],szTOff[EWDB_DOUBLE_STRING_LEN];

/* declare Statement Struct */
static EWDB_OCIStatementStruct SSStatement;


/********************************
      FUNCTION PROTOTYPES
********************************/
int PrepSetPlexorForChannelExec(EWDBid IN_idChan, EWDBid IN_idDeviceSlot, 
                             int IN_iPlexor, double IN_tOn, double IN_tOff,
                             EWDB_Cursor * ppCursor);
int PostSetPlexorForChannelExec(int * pSQLRetCode, EWDBid * pidChanT);
int InitSetPlexorForChannelStatement(char *statement, EWDB_OCIStatementStruct *pSS);
/*******************************/



int ewdb_api_SetPlexorForChannel(EWDBid IN_idChan, EWDBid IN_idDeviceSlot, 
                          int IN_iPlexor, double IN_tOn, double IN_tOff,
                          int * pSQLRetCode, EWDBid * pidChanT)
{

	EWDB_Cursor pCursor;

  /* Note that we are executing an OraAPI function, in case someone
     is thinking about cutting a connection */
	ewdb_base_SetLastOraAPIActionTime ();

  /* Check for NULL pointers */
  if (!(pSQLRetCode && pidChanT))
	{
		logit ("", "EWDB_SetPlexorForChannel(): Null pointers passed in!\n");
		return EWDB_RETURN_FAILURE;
	}

  /* Check for blatantly invalid DB id's */
	if (IN_idChan <= 0 || IN_idDeviceSlot <= 0 )
	{
		logit ("", "EWDB_SetPlexorForChannel(): Invalid DB id's passed in!\n");
		return EWDB_RETURN_FAILURE;
	}

  /* Do any neccessary initialization */
  *pSQLRetCode = 0;  /* init the SQLRetCode to 0, so that we don't get
                        any unintended SQL return codes */

	/* Ensure we are connected to the DB. */
	if (ewdb_base_Reconnect () != EWDB_RETURN_SUCCESS)
	{
		logit ("", "EWDB_SetPlexorForChannel(): Could not reconnect to the database!\n");
		return (EWDB_RETURN_FAILURE);
	}

  /* Call our Prep() function */
	if (PrepSetPlexorForChannelExec (IN_idChan, IN_idDeviceSlot, 
                                IN_iPlexor, IN_tOn, IN_tOff, &pCursor)
      != EWDB_RETURN_SUCCESS)
	{
		logit ("", "ORA_API:SetPlexorForChannel():PrepSetPlexorForChannelExec() failed.\n");
		return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
	}

	if (ewdb_base_SQLExecute(pCursor))
	{
		ewdb_base_ErrorReport (hEWDBC, pCursor,"SetPlexorForChannel:ewdb_base_SQLExecute", 1);
		return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
	} 
  
	/* Commit the transaction (all the previous inserts!)
	   In Case there is any auditing or logging or debug
	   changes made in the stored procedures.
	****************************************************/
	if (ewdb_base_SQLCommit(hEWDBC))
	{
		ewdb_base_ErrorReport (hEWDBC, hEWDBC,"SetPlexorForChannel:ewdb_base_SQLCommit",2);
		return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
	}


  if (PostSetPlexorForChannelExec (pSQLRetCode, pidChanT) != EWDB_RETURN_SUCCESS)
  {
    logit ("", "Call to PostSetPlexorForChannelExec failed!\n");
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  }

	/* reset the timeout */
	ewdb_base_SetLastOraAPIActionTime ();

  /* We successfully completed. Return Success */
	return(EWDB_RETURN_SUCCESS);

}  /* End EWDB_SetPlexorForChannel() */



int InitSetPlexorForChannelStatement(char *statement, EWDB_OCIStatementStruct *pSS)
{
 
  pSS->FieldArray[0].pVal = &iRetCode;
  pSS->FieldArray[1].pVal = &idChanT;
  pSS->FieldArray[2].pVal = &idChan;
  pSS->FieldArray[3].pVal = &idDeviceSlot;
  pSS->FieldArray[4].pVal = &iPlexor;
  pSS->FieldArray[5].pVal = szTOn;
  pSS->FieldArray[6].pVal = szTOff;

  ewdb_base_RequestCursor (statement, pSS, 0);

  return(EWDB_RETURN_SUCCESS);
}  /* End InitSetPlexorForChannelStatement() */


int PrepSetPlexorForChannelExec(EWDBid IN_idChan, EWDBid IN_idDeviceSlot, 
                             int IN_iPlexor, double IN_tOn, double IN_tOff,
                             EWDB_Cursor * ppCursor)
{

  /* Copy misc. local variables to the statement struct */
  SSStatement.NumOfFields = NumFieldsInBindArray;
  SSStatement.FieldArray = SQLParamsBindArray;
  SSStatement.RecordSize = 0;

  /* Copy input params to local variables */
  idChan       = IN_idChan;
  idDeviceSlot = IN_idDeviceSlot;
  iPlexor      = IN_iPlexor;

  sprintf(szTOn,"%.2f",IN_tOn);
  sprintf(szTOff,"%.2f",IN_tOff);

  /* sample strncpy *
  strncpy(szSlotName, IN_szSlotName, sizeof(szSlotName) - 1);
  szSlotName[sizeof(szSlotName) - 1] = 0x00;
  *********************************/

  if (InitSetPlexorForChannelStatement (SQL_STRING,
                           &SSStatement) != EWDB_RETURN_SUCCESS)
  {
    logit ("", "Call to InitSetPlexorForChannelStatement failed!\n");
    return EWDB_RETURN_FAILURE;
  }

  *ppCursor = SSStatement.pCda;
  
  return(EWDB_RETURN_SUCCESS);
}  /* End PrepSetPlexorForChannelExec() */


int PostSetPlexorForChannelExec(int * pSQLRetCode, EWDBid * pidChanT)
{

  *pSQLRetCode = iRetCode;
  *pidChanT    = idChanT;
  
  /* Release the cursor since we are done */
  ewdb_base_ReleaseCursor(SSStatement.pCda);

  return(EWDB_RETURN_SUCCESS);
}  /* End PostSetPlexorForChannelExec() */

