/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_api_GetPreferredSummaryInfo.c,v 1.7 2005/06/15 19:03:12 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_api_GetPreferredSummaryInfo.c,v $
 *     Revision 1.7  2005/06/15 19:03:12  davidk
 *     DB API Cleanup
 *
 *     Revision 1.6  2004/09/03 18:41:00  davidk
 *     Modified function behavior when  NO Preferred record is found:
 *     1) WARNING is now returned instead of ERROR
 *     2) No information is logged.
 *
 *     Revision 1.5  2004/09/01 17:59:45  davidk
 *     Added code to PrepXXX() to initialize the Local_idMag and Local_idOrigin return parameters
 *     before each call, so that NULL values returned by the call end up as 0, instead of
 *     whatever value they were set to before.
 *
 *     Revision 1.4  2002/06/11 14:21:51  davidk
 *     Changed several SQL idXXX parameters to EWDBID from int.
 *
 *     Revision 1.3  2001/07/12 21:07:17  davidk
 *     Cleaned up the function as part of the API cleanup.  Added improved
 *     error logging, handling, and reporting.
 *
 *     Revision 1.2  2001/05/15 02:16:21  davidk
 *     Moved functions around between the apps, DB API, and DB API INTERNAL
 *     levels.  Renamed functions and files.  Added support for amplitude
 *     magnitude types.  Reformatted makefiles.
 *
 *     Revision 1.1  2001/02/28 17:19:22  lucky
 *     Initial revision
 *
 *
 */

#include <ewdb_ora_api.h>
#include <ewdb_cli_base.h>
#include <ewdb_ew_oci_base.h>

static char SQL_STRING[] =
        "Begin Get_Preferred_Summary_Info(OUT_RetCode => :OUT_RetCode, IN_idEvent => :IN_idEvent, "
        " OUT_idOrigin => :OUT_idOrigin, OUT_idMag => :OUT_idMag, "
        " OUT_idMech => :OUT_idMech); End;";

static EWDB_OCI_SFS GetPSIBindArray[] =

{
  {0,1,0,0,0,OA_INT,":OUT_Retcode"},
  {0,1,0,0,0,OA_EWDBID,":IN_idEvent"},
  {0,1,0,0,0,OA_EWDBID,":OUT_idOrigin"},
  {0,1,0,0,0,OA_EWDBID,":OUT_idMag"},
  {0,1,0,0,0,OA_EWDBID,":OUT_idMech"},
};

/* define the total number of fields to be bound */
#define NUM_FIELDS 5

/* Insertion Struct for GetPreferInfo statement */
static EWDB_OCIStatementStruct SSStatement;

static int    Local_iRetCode;
static EWDBid Local_idEvent, Local_idOrigin, Local_idMag, Local_idMechFM;


/********************************
      FUNCTION PROTOTYPES
********************************/
static int PrepGetPSIExec(EWDBid IN_idEvent, EWDB_Cursor * ppCursor);
static int PostGetPSIExec(EWDBid *pidOrigin, EWDBid *pidMagnitude, EWDBid *pidMech);
static int InitGetPSIStatement(char * szStatement, EWDB_OCIStatementStruct *pSS);


/**********************************************************************
**********************************************************************/
int ewdb_api_GetPreferredSummaryInfo(EWDBid IN_idEvent, EWDBid *pidOrigin, 
                                     EWDBid *pidMagnitude, EWDBid *pidMech)
{

  EWDB_Cursor  pCursor;
  int          rc;
 
  ewdb_base_SetLastOraAPIActionTime();

  if(IN_idEvent <= 0 || !pidOrigin || !pidMagnitude || !pidMech)
  {
    logit("","ewdb_api_GetPreferredSummaryInfo(): Invalid params passed in:\n"
             "idEvent %d, pidOrigin %u, pidMagnitude %u, pidMech %u\n",
          IN_idEvent, pidOrigin, pidMagnitude, pidMech);
    return(EWDB_RETURN_FAILURE);
  }

  if( ewdb_base_Reconnect() != EWDB_RETURN_SUCCESS )
  {
    return( EWDB_RETURN_FAILURE );
  }

  if( PrepGetPSIExec(Local_idEvent,&pCursor) != EWDB_RETURN_SUCCESS)
  {
    logit("t", "ewdb_api_GetPreferredSummaryInfo(): %s failed for idEvent %u!\n",
          "PrepGetPSIExec()", IN_idEvent);
    return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }
  
  if( ewdb_base_SQLExecute(pCursor) )
  {
    ewdb_base_ErrorReport(hEWDBC, pCursor,"ewdb_api_GetPreferredSummaryInfo(): ewdb_base_SQLExecute",1);
    return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  } 

 /* Commit the transaction (all the previous inserts!)
  In Case there is any auditing or logging or debug
  changes made in the stored procedures.
  ****************************************************/
  if (ewdb_base_SQLCommit (hEWDBC))
  {
    ewdb_base_ErrorReport (hEWDBC, hEWDBC,"ewdb_api_GetPreferredSummaryInfo(): ewdb_base_SQLCommit",2);
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  }
 
  if((rc = PostGetPSIExec(pidOrigin,pidMagnitude,pidMech)) != EWDB_RETURN_SUCCESS)
  {
    logit("t", "ewdb_api_GetPreferredSummaryInfo(): %s failed for idEvent %u!\n",
          "PostGetPSIExec()", IN_idEvent);
    return(EWDB_RETURN_FAILURE);
  }

  ewdb_base_SetLastOraAPIActionTime();

  return(EWDB_RETURN_SUCCESS);
}  /* end ewdb_api_GetPreferredSummaryInfo() */


static int InitGetPSIStatement(char * szStatement, EWDB_OCIStatementStruct *pSS)
{
	pSS->FieldArray[0].pVal = &Local_iRetCode;
	pSS->FieldArray[1].pVal = &Local_idEvent;
	pSS->FieldArray[2].pVal = &Local_idOrigin;
	pSS->FieldArray[3].pVal = &Local_idMag;
	pSS->FieldArray[4].pVal = &Local_idMechFM;

  return(ewdb_base_RequestCursor(szStatement, pSS,0));
}


int PrepGetPSIExec(EWDBid IN_idEvent, EWDB_Cursor * ppCursor)
{

  Local_idEvent = IN_idEvent;
  Local_idOrigin=Local_idMag=Local_idMechFM=0;

  SSStatement.NumOfFields=NUM_FIELDS;
  SSStatement.FieldArray=GetPSIBindArray;
  SSStatement.RecordSize=0;

  if(InitGetPSIStatement(SQL_STRING, &SSStatement) != EWDB_RETURN_SUCCESS)
  {
    *ppCursor = NULL;
    return(EWDB_RETURN_FAILURE);
  }

  *ppCursor=SSStatement.pCda;

  return(EWDB_RETURN_SUCCESS);
}  /* end PrepXXX() */


int PostGetPSIExec(EWDBid *pidOrigin,EWDBid *pidMagnitude, EWDBid *pidMech)
{

  ewdb_base_ReleaseCursor (SSStatement.pCda);

  if(!Local_iRetCode)
  {
    *pidOrigin=Local_idOrigin;
    *pidMagnitude=Local_idMag;
    *pidMech=Local_idMechFM;
    return(EWDB_RETURN_SUCCESS);
  }
  else if(Local_iRetCode < 0)
  {
    logit("","PostGetPSIExec() SQL Proc %s returned error %d\n",
          "Get_Preferred_Summary_Info()", Local_iRetCode);
    return(EWDB_RETURN_WARNING);
  }
  else
  {
    return(EWDB_RETURN_WARNING);
  }
}  /* end PostXXX() */


