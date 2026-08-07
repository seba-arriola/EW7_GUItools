/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_internal_CreateSMMotion.c,v 
 *
 *    Revision history:
 *     $Log: ewdb_internal_CreateSMMotion.c,v $
 *     Revision 1.2  2003/06/09 23:05:29  davidk
 *     Removed useless debugging statements.
 *
 *     Revision 1.1  2001/05/15 02:16:42  davidk
 *     Initial revision
 *
 *     Revision 1.1  2001/04/06 19:01:00  davidk
 *     Initial revision
 *
 *
 *************************************************************/



#include <stdlib.h>
#include <stdio.h>
#include <ewdb_ora_api.h>
#include <ewdb_cli_base.h>
#include <ewdb_ew_oci_base.h>


static char SQL_STRING[] =
  "Begin Create_SMMotion(OUT_RetCode => :OUT_RetCode,"
  "OUT_idSMMotion => :OUT_idSMMotion,"
  "IN_idSMMessage => :IN_idSMMessage,"
  "IN_iMotionType => :IN_iMotionType,"
  "IN_dPeriod => :IN_dPeriod,"
  "IN_dMeasurement => :IN_dMeasurement); End;";
  
static EWDB_OCI_SFS SQLParamsBindArray[] = 
{
  {0,1,0,0,0,OA_INT,   ":OUT_RetCode"},
  {0,1,0,0,0,OA_EWDBID,":OUT_idSMMotion"},
  {0,1,0,0,0,OA_EWDBID,":IN_idSMMessage"},
  {0,1,0,0,0,OA_INT,   ":IN_iMotionType"},
  {0,1,0,0,0,OA_DOUBLE,":IN_dPeriod"},
  {0,1,0,0,0,OA_DOUBLE,":IN_dMeasurement"}
};

#define  NUM_FIELDS  6

static int     iRetCode;
static EWDBid  idSMMotion, idSMMessage;
static int     iMotionType;
static char    szDPeriod[20];
static char    szDMeasurement[20];


EWDB_OCIStatementStruct SSStatement;

/********************************
      FUNCTION PROTOTYPES
********************************/
double EWDB_ConvertSMMeasurementFromDBtoEW(double dIN);
double EWDB_ConvertSMMeasurementFromEWtoDB(double dIN);

int InitCreateSMMotionStatement (char *statement, EWDB_OCIStatementStruct *pSS);
int PrepCreateSMMotionExec(EWDBid idSMMessage, int iMotionType, double dPeriod,
                           double dMeasurement, EWDB_Cursor *ppCursor);
int PostCreateSMMotionExec (int * pRetCode, EWDBid * pidSMMotion);
/*******************************/


/* Used to create a Strong Motion measurement in the DB */
int ewdb_internal_CreateSMMotion(EWDBid * pidSMMotion,
                                 EWDBid idSMMessage, int iMotionType,
                                 double dPeriod, double dMeasurement)
{

	EWDB_Cursor pCursor;
  int         Temp_RetCode;

  /* Note that we are executing an OraAPI function, in case someone
     is thinking about cutting a connection */
	ewdb_base_SetLastOraAPIActionTime ();

  /* Check for NULL pointers */
  if (!pidSMMotion)
	{
		logit ("", "ewdb_internal_CreateSMMotion(): Null pointers passed in!\n");
		return EWDB_RETURN_FAILURE;
	}

  /* Check for blatantly invalid DB id's */
	if (idSMMessage <= 0)
	{
		logit ("", "ewdb_internal_CreateSMMotion(): Invalid DB id's passed in!\n");
		return EWDB_RETURN_FAILURE;
	}

	/* Ensure we are connected to the DB. */
	if (ewdb_base_Reconnect () != EWDB_RETURN_SUCCESS)
	{
		logit ("", "ewdb_internal_CreateSMMotion(): Could not reconnect to the database!\n");
		return (EWDB_RETURN_FAILURE);
	}

  /* Call our Prep() function */
	if (PrepCreateSMMotionExec (idSMMessage, iMotionType, 
                                dPeriod, dMeasurement, &pCursor)
      != EWDB_RETURN_SUCCESS)
	{
		logit ("", "ORA_API:ewdb_internal_CreateSMMotion():PrepCreateSMMotionExec() failed.\n");
		return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
	}

	if (ewdb_base_SQLExecute(pCursor))
	{
		ewdb_base_ErrorReport (hEWDBC, pCursor,"ewdb_internal_CreateSMMotion:ewdb_base_SQLExecute", 1);
		return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
	} 
  
	/* Commit the transaction (all the previous inserts!)
	   In Case there is any auditing or logging or debug
	   changes made in the stored procedures.
	****************************************************/
	if (ewdb_base_SQLCommit(hEWDBC))
	{
		ewdb_base_ErrorReport (hEWDBC, hEWDBC,"ewdb_internal_CreateSMMotion:ewdb_base_SQLCommit",2);
		return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
	}


  if (PostCreateSMMotionExec (&Temp_RetCode, pidSMMotion) != EWDB_RETURN_SUCCESS)
  {
    logit ("", "Call to PostCreateSMMotionExec failed!\n");
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  }

	/* reset the timeout */
	ewdb_base_SetLastOraAPIActionTime ();

  if(Temp_RetCode)
    return(EWDB_RETURN_FAILURE);
  else
    /* We successfully completed. Return Success */
  	return(EWDB_RETURN_SUCCESS);

} /* end ewdb_internal_CreateSMMotion() */


int InitCreateSMMotionStatement (char *statement, EWDB_OCIStatementStruct *pSS)
{

  pSS->FieldArray[0].pVal = &iRetCode;
  pSS->FieldArray[1].pVal = &idSMMotion;
  pSS->FieldArray[2].pVal = &idSMMessage;
  pSS->FieldArray[3].pVal = &iMotionType;
  pSS->FieldArray[4].pVal = szDPeriod;
  pSS->FieldArray[5].pVal = szDMeasurement;

  ewdb_base_RequestCursor (statement, pSS, 0);

  return(EWDB_RETURN_SUCCESS);
}  /* End InitCreateSMMotionStatement() */


double EWDB_ConvertSMMeasurementFromDBtoEW(double dIN)
{
  return(dIN);
}  /* End EWDB_ConvertSMMeasurementFromDBtoEW() */

double EWDB_ConvertSMMeasurementFromEWtoDB(double dIN)
{
  return(dIN);
}  /* End EWDB_ConvertSMMeasurementFromEWtoDB() */


int PrepCreateSMMotionExec(EWDBid IN_idSMMessage, int IN_iMotionType, 
                           double IN_dPeriod, double IN_dMeasurement, 
                           EWDB_Cursor *ppCursor)
{

  double dTempMeasurement;

  SSStatement.NumOfFields = NUM_FIELDS;
  SSStatement.FieldArray = SQLParamsBindArray;
  SSStatement.RecordSize = 0;

  dTempMeasurement = EWDB_ConvertSMMeasurementFromEWtoDB(IN_dMeasurement);

  sprintf (szDPeriod, "%.3f", IN_dPeriod);
  sprintf (szDMeasurement, "%.6f", dTempMeasurement);
  idSMMessage = IN_idSMMessage;
  iMotionType = IN_iMotionType;

  if (InitCreateSMMotionStatement (SQL_STRING,
                           &SSStatement) != EWDB_RETURN_SUCCESS)
  {
    logit ("", "Call to InitCreateSMMotionStatement failed!\n");
    return EWDB_RETURN_FAILURE;
  }

  *ppCursor = SSStatement.pCda;
  
  return(EWDB_RETURN_SUCCESS);
}  /* End PrepCreateSMMotionExec() */


int PostCreateSMMotionExec (int * pRetCode, EWDBid * pidSMMotion)
{
  EWDB_Cursor pCursor;
  
  if (pidSMMotion == NULL)
  {
    logit ("", "Invalid arguments passed in.\n");
    return EWDB_RETURN_FAILURE;
  }
  
  *pRetCode = iRetCode;
  *pidSMMotion = idSMMotion;
  pCursor = SSStatement.pCda;
  
  ewdb_base_ReleaseCursor (pCursor);
  
  return (EWDB_RETURN_SUCCESS);
}  /* End PostCreateSMMotionExec() */
