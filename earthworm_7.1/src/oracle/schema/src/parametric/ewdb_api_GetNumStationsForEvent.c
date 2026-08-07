/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_api_GetNumStationsForEvent.c,v 1.3 2005/06/15 19:03:12 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_api_GetNumStationsForEvent.c,v $
 *     Revision 1.3  2005/06/15 19:03:12  davidk
 *     DB API Cleanup
 *
 *     Revision 1.2  2001/09/28 15:35:27  lucky
 *     Fixed the name of the SQL routine
 *
 *     Revision 1.1  2001/09/28 00:09:30  davidk
 *     Initial revision
 *
 *
 */

#include <ewdb_cli_base.h>
#include <ewdb_ora_api.h>
#include <ewdb_ew_oci_base.h>

static char SQL_STRING[] =
 "Begin Get_Num_Wavestations_For_Event (OUT_iNumStations => :OUT_iNumStations, "
    " IN_idEvent => :IN_idEvent); End;";



static EWDB_OCI_SFS SQLParamsBindArray[] =

{
  {0,1,0,0,0,OA_INT,":OUT_iNumStations"},
  {0,1,0,0,0,OA_EWDBID,":IN_idEvent"}
};

/* define the total number of fields to be bound */
#define NUM_FIELDS 2

/* Insertion Struct for GetStationList statement */
static EWDB_OCIStatementStruct SSStatement;

static int Local_iNumStations;
static EWDBid Local_idEvent;


/********************************
      FUNCTION PROTOTYPES
********************************/
static int PrepGetNumStationsForEventExec(EWDBid IN_idEvent, EWDB_Cursor * ppCursor);
static int PostGetNumStationsForEventExec(int * pNumStations);
static int InitGetNumStationsForEventStatement(char * szStatement, EWDB_OCIStatementStruct *pSS);


/**********************************************************************
**********************************************************************/
int ewdb_api_GetNumStationsForEvent(EWDBid IN_idEvent, int * pNumStations)
{

  EWDB_Cursor  pCursor;
 
  ewdb_base_SetLastOraAPIActionTime();

  if(IN_idEvent <= 0 || !pNumStations)
  {
    logit("","ewdb_api_GetPicksByTime(): Invalid params passed in:\n"
             "IN_idEvent %d, pNumStations %u\n",
          IN_idEvent, pNumStations);
    return(EWDB_RETURN_FAILURE);
  }

  if( ewdb_base_Reconnect() != EWDB_RETURN_SUCCESS )
  {
    return( EWDB_RETURN_FAILURE );
  }

  if( PrepGetNumStationsForEventExec(IN_idEvent,&pCursor) != EWDB_RETURN_SUCCESS)
  {
    logit("","ewdb_api_GetNumStationsForEvent(): PrepGetNumStationsForEventExec() failed!\n");
    return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }
  
  if( ewdb_base_SQLExecute(pCursor) )
  {
    ewdb_base_ErrorReport(hEWDBC, pCursor,"EWDB_GetNumStationsForEvent:ewdb_base_SQLExecute",1);
    return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  } 

 /* Commit the transaction (all the previous inserts!)
  In Case there is any auditing or logging or debug
  changes made in the stored procedures.
  ****************************************************/
  if (ewdb_base_SQLCommit (hEWDBC))
  {
    ewdb_base_ErrorReport (hEWDBC, hEWDBC,"DeletePolygon:ewdb_base_SQLCommit",2);
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  }
 
  if( PostGetNumStationsForEventExec(pNumStations) == EWDB_RETURN_FAILURE)
  {
    logit("","ewdb_api_GetNumStationsForEvent(): PostGetNumStationsForEventExec() failed!\n");
    return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }

  ewdb_base_SetLastOraAPIActionTime();

  return(EWDB_RETURN_SUCCESS);
}  /* end ewdb_api_GetNumStationsForEvent() */


static int InitGetNumStationsForEventStatement(char * szStatement, EWDB_OCIStatementStruct *pSS)
{
	pSS->FieldArray[0].pVal = &Local_iNumStations;
	pSS->FieldArray[1].pVal = &Local_idEvent;

  return(ewdb_base_RequestCursor(szStatement, pSS,0/*don't force rebind*/));
}


static int PrepGetNumStationsForEventExec(EWDBid IN_idEvent, EWDB_Cursor * ppCursor)
{

  Local_idEvent      = IN_idEvent;
  Local_iNumStations = 0;

  SSStatement.NumOfFields=NUM_FIELDS;
  SSStatement.FieldArray=SQLParamsBindArray;
  SSStatement.RecordSize=0;

  if(InitGetNumStationsForEventStatement(SQL_STRING, &SSStatement) != EWDB_RETURN_SUCCESS)
  {
    *ppCursor = NULL;
    return(EWDB_RETURN_FAILURE);
  }

  *ppCursor=SSStatement.pCda;

  return(EWDB_RETURN_SUCCESS);
}  /* end PrepXXX() */


static int PostGetNumStationsForEventExec(int * pNumStations)
{
  
  *pNumStations = Local_iNumStations;
  ewdb_base_ReleaseCursor (SSStatement.pCda);
  
  return(EWDB_RETURN_SUCCESS);
}   /* End PostXXX() */


