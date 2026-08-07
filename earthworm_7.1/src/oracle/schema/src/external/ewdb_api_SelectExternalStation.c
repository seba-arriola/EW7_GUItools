/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */
/*    $Id: ewdb_api_SelectExternalStation.c,v 1.4 
       1999/11/09 18:51:00 lucky Exp $                      */
/*                                                          */
/*    Revision history:                                     */
/*
 *     $Log: ewdb_api_SelectExternalStation.c,v $
 *     Revision 1.3  2005/06/06 16:12:37  davidk
 *     DB API Cleanup
 *
 *     Revision 1.2  2004/04/08 23:43:31  davidk
 *     Completed and tested function.
 *
 *     Revision 1.1  2004/03/17 18:00:19  davidk
 *     Initial revision
 *
 *                                              */

#include <ewdb_ora_api.h>
#include <ewdb_cli_base.h>
#include <ewdb_ew_oci_base.h>


static char SQL_STRING[] =
    "Begin Select_Station_External(OUT_StationID => :OUT_StationID,"
    "IN_sta => :IN_sta,"
    "IN_chan => :IN_chan,"
    "IN_net => :IN_net,"
    "IN_loc => :IN_loc,"
    "IN_lat => :IN_lat,"
    "IN_lon => :IN_lon,"
    "IN_elev => :IN_elev,"
    "IN_description => :IN_comment); End;";

static EWDB_OCI_SFS SQLParamsBindArray[] = 
{
  {0,1,0,0,0,OA_INT,  ":OUT_StationID"},
  {0,1,0,0,0,OA_SZ,   ":IN_sta"},
  {0,1,0,0,0,OA_SZ,   ":IN_chan"},
  {0,1,0,0,0,OA_SZ,   ":IN_net"},
  {0,1,0,0,0,OA_SZ,   ":IN_loc"},
  {0,1,0,0,0,OA_FLOAT,":IN_lat"},
  {0,1,0,0,0,OA_FLOAT,":IN_lon"},
  {0,1,0,0,0,OA_FLOAT,":IN_elev"},
  {0,1,0,0,0,OA_SZ,   ":IN_comment"},
};

#define	NUM_FIELDS	9


static	EWDB_External_StationStruct 	Local_StationStruct;

static	char	Local_szdLat[15];
static	char	Local_szdLon[15];
static	char	Local_szdElev[15];

static EWDB_OCIStatementStruct SSStatement;

/*** 

  If station with the scnl is already in the table,
  its STATIONID will be set in the pStation struct, AND all
  non-zero, and non-empty character values will be updated.
  
    If this is a new record, all values from the pStation struct
    will be stored in the table.
    
***/


/* Prototypes of functions defined in this file */
static int InitSelectExternalStationStatement (char *statement, 
                                        EWDB_OCIStatementStruct *pSS);
static int PrepSelectExternalStationExec (EWDB_External_StationStruct *pStation, 
                                   EWDB_Cursor *ppCursor);
static int PostSelectExternalStationExec (int * pStationID);


int ewdb_api_SelectExternalStation (EWDB_External_StationStruct *pStation)
{
  
  EWDB_Cursor pCursor;
  int rc;
  
  
  if ((pStation == NULL))
  {
    logit ("", "ewdb_api_SelectExternalStation(): Null parameters passed in!\n");
    return EWDB_RETURN_FAILURE;
  }
  
  ewdb_base_SetLastOraAPIActionTime ();
  
  if (ewdb_base_Reconnect () != EWDB_RETURN_SUCCESS)
    /* Establishes connection, and performs binding!?! */
  {
    logit ("", "ewdb_api_SelectExternalStation(): Could not reconnect to the database!\n");
    return (EWDB_RETURN_FAILURE);
  }
  
  if (PrepSelectExternalStationExec (pStation, &pCursor) 
    != EWDB_RETURN_SUCCESS)
  {
    logit ("", "ewdb_api_SelectExternalStation(): "
           "PrepSelectExternalStationExec() failed.\n");
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  }
  
  if (ewdb_base_SQLExecute (pCursor))
  {
    ewdb_base_ErrorReport (hEWDBC, pCursor,
                      "ewdb_api_SelectExternalStation(): ewdb_base_SQLExecute", 1);
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  } 
    
  /* Commit the transaction (all the previous inserts!)
  In Case there is any auditing or logging or debug
  changes made in the stored procedures.
  ****************************************************/
  if (ewdb_base_SQLCommit (hEWDBC))
  {
    ewdb_base_ErrorReport (hEWDBC, hEWDBC,
                      "ewdb_api_SelectExternalStation(): ewdb_base_SQLCommit",2);
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  }
  
  if ((rc = PostSelectExternalStationExec (&(pStation->StationID))) 
      == EWDB_RETURN_FAILURE)
  {
    logit ("", "ewdb_api_SelectExternalStation(): PostSelectExternalStationExec failed!\n");
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  }
  else if (rc == EWDB_RETURN_WARNING)
  {
    return (EWDB_RETURN_WARNING);
  }
  
  return EWDB_RETURN_SUCCESS;
}  /* end ewdb_api_SelectExternalStation() */


static int InitSelectExternalStationStatement (char *statement, 
                                        EWDB_OCIStatementStruct *pSS)
{
  
  if ((statement == NULL) || (pSS == NULL))
  {
    logit ("", "InitSelectExternalStationStatement(): Null parameters passed in!\n");
    return EWDB_RETURN_FAILURE;
  }
  
  pSS->FieldArray[0].pVal = &(Local_StationStruct.StationID);
  pSS->FieldArray[1].pVal = Local_StationStruct.Station.Sta;
  pSS->FieldArray[2].pVal = Local_StationStruct.Station.Comp;
  pSS->FieldArray[3].pVal = Local_StationStruct.Station.Net;
  pSS->FieldArray[4].pVal = Local_StationStruct.Station.Loc;
  pSS->FieldArray[5].pVal = Local_szdLat;
  pSS->FieldArray[6].pVal = Local_szdLon;
  pSS->FieldArray[7].pVal = Local_szdElev;
  pSS->FieldArray[8].pVal = Local_StationStruct.Description;
  
  if (ewdb_base_RequestCursor (statement, pSS, 0) != 0)
  {
    logit ("", "InitSelectExternalStationStatement(): ewdb_base_RequestCursor failed.\n");
    return EWDB_RETURN_FAILURE;
  }
  
  return EWDB_RETURN_SUCCESS;
}  /* end InitSelectExternalStationStatement() */


static int PrepSelectExternalStationExec (EWDB_External_StationStruct *pStation,
                                   EWDB_Cursor *ppCursor)
{
  
  if ((pStation == NULL) ||  (ppCursor == NULL))
  {
    logit ("", "PrepSelectExternalStationExec(): Null parameters passed in!\n");
    return EWDB_RETURN_FAILURE;
  }
  
  
  memcpy (&Local_StationStruct, pStation, 
          sizeof (EWDB_External_StationStruct));
  
  SSStatement.NumOfFields = NUM_FIELDS;
  SSStatement.FieldArray = SQLParamsBindArray;
  SSStatement.RecordSize = 0;
  
  sprintf (Local_szdLat,  "%0.6f", pStation->Station.Lat);
  sprintf (Local_szdLon,  "%0.6f", pStation->Station.Lon);
  sprintf (Local_szdElev, "%0.2f", pStation->Station.Elev);
  
  if (InitSelectExternalStationStatement (SQL_STRING,
                                          &SSStatement) 
      != EWDB_RETURN_SUCCESS)
  {
    logit ("", "PrepSelectExternalStationExec(): InitSelectExternalStationStatement failed!\n");
    return EWDB_RETURN_FAILURE;
  }
  
  *ppCursor = SSStatement.pCda;
  
  return EWDB_RETURN_SUCCESS;
  
} /* end PrepSelectExternalStationExec() */


static int PostSelectExternalStationExec (int * pStationID)
{
  EWDB_Cursor pCursor;
  
  pCursor = SSStatement.pCda;
  
  ewdb_base_ReleaseCursor (pCursor);
  
  if (pStationID == NULL)
  {
    return EWDB_RETURN_FAILURE;
  }
  
  *pStationID=Local_StationStruct.StationID;
  
  if(*pStationID <= 0)
    return(EWDB_RETURN_WARNING);
  else
    return(EWDB_RETURN_SUCCESS);
  
}  /* end PostSelectExternalStationExec() */

