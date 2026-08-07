/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */
/*    $Id: ewdb_api_CreateOrAlterExternalStation.c,v 1.4 
       1999/11/09 18:51:00 lucky Exp $                      */
/*                                                          */
/*    Revision history:                                     */
/*
 *     $Log: ewdb_api_CreateOrAlterExternalStation.c,v $
 *     Revision 1.5  2005/06/06 16:12:37  davidk
 *     DB API Cleanup
 *
 *     Revision 1.4  2001/07/10 23:24:55  davidk
 *     Changed the code to reflect the change in the internal structure of
 *     EWDB_ExternalStationStruct.
 *
 *     Revision 1.3  2001/05/15 02:16:24  davidk
 *     Moved functions around between the apps, DB API, and DB API INTERNAL
 *     levels.  Renamed functions and files.  Added support for amplitude
 *     magnitude types.  Reformatted makefiles.
 *
 *     Revision 1.2  2001/03/19 22:55:22  dietz
 *     Print lat,lon as %.6f before sending to DBMS
 *
 *     Revision 1.1  2001/02/28 17:19:22  lucky
 *     Initial revision
 *
 *     Revision 1.4  2001/02/21 09:30:26  davidk
 *     Code was retrofitted to use the new db_cli_base API
 *     in lieu of the oci_base() and OCI7 function API's.layer, which
 *     provides abstraction from the OCI7 function calls to
 *     ora_api layer code.  See comments in
 *     schema/src/include/internal/ewdb_cli_base.h for more info
 *     on the details of the changes made.
 *
 *     Revision 1.3  2000/06/21 22:52:13  lucky
 *      Cleaned up logit calls to make log files more readable.
 *
 *     Revision 1.2  2000/03/30 20:29:17  davidk
 *     fixed $Log syntax in the RCS header
 *
 *     Revision 1.1  2000/03/30 20:26:54  davidk
 *     Initial revision
 *
*     Revision 1.4  1999/11/09 18:51:00  lucky
*     *** empty log message ***
*                                              */
/*                                                          */

#include <ewdb_ora_api.h>
#include <ewdb_cli_base.h>
#include <ewdb_ew_oci_base.h>


static char SQL_STRING[] =
    "Begin Create_Or_Alter_Station_Ext(OUT_StationID => :OUT_StationID,"
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

EWDB_OCIStatementStruct SSStatement;

/*** 

  If station with the scnl is already in the table,
  its STATIONID will be set in the pStation struct, AND all
  non-zero, and non-empty character values will be updated.
  
    If this is a new record, all values from the pStation struct
    will be stored in the table.
    
***/


/* Prototypes of functions defined in this file */
int InitCreateExternalStationStatement (char *szStatement, 
                                        EWDB_OCIStatementStruct *pSS);
int PrepCreateExternalStationExec (EWDB_External_StationStruct *pStation, 
                                   EWDB_Cursor *ppCursor);
int PostCreateExternalStationExec (int * pStationID);


int ewdb_api_CreateOrAlterExternalStation (EWDB_External_StationStruct *pStation)
{
  EWDB_Cursor pCursor;
  
  if ((pStation == NULL))
  {
    logit ("", "ewdb_api_CreateOrAlterExternalStation(): Null parameters passed in!\n");
    return EWDB_RETURN_FAILURE;
  }
  
  ewdb_base_SetLastOraAPIActionTime ();
  
  if (ewdb_base_Reconnect () != EWDB_RETURN_SUCCESS)
    /* Establishes connection, and performs binding!?! */
  {
    logit ("", "ewdb_api_CreateOrAlterExternalStation(): Could not reconnect to the database!\n");
    return (EWDB_RETURN_FAILURE);
  }
  
  if (PrepCreateExternalStationExec (pStation, &pCursor) 
    != EWDB_RETURN_SUCCESS)
  {
    logit ("", "ewdb_api_CreateOrAlterExternalStation(): "
           "PrepCreateExternalStationExec() failed.\n");
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  }
  
  if (ewdb_base_SQLExecute (pCursor))
  {
    ewdb_base_ErrorReport (hEWDBC, pCursor,
                      "ewdb_api_CreateOrAlterExternalStation(): ewdb_base_SQLExecute", 1);
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  } 
  
    /* Commit the transaction (all the previous inserts!)
  In Case there is any auditing or logging or debug
  changes made in the stored procedures.
  ****************************************************/
  if (ewdb_base_SQLCommit (hEWDBC))
  {
    ewdb_base_ErrorReport (hEWDBC, hEWDBC,
                      "ewdb_api_CreateOrAlterExternalStation(): ewdb_base_SQLCommit",2);
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  }
  
  if (PostCreateExternalStationExec (&(pStation->StationID)) 
      != EWDB_RETURN_SUCCESS)
  {
    logit ("", "ewdb_api_CreateOrAlterExternalStation(): PostCreateExternalStationExec failed!\n");
    return (EWDB_RETURN_FAILURE);
  }
  
  return EWDB_RETURN_SUCCESS;
}


int InitCreateExternalStationStatement (char *szStatement, 
                                        EWDB_OCIStatementStruct *pSS)
{
  
  if ((szStatement == NULL) || (pSS == NULL))
  {
    logit ("", "InitCreateExternalStationStatement(): Null parameters passed in!\n");
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
  
  if (ewdb_base_RequestCursor (szStatement, pSS, 0) != 0)
  {
    logit ("", "InitCreateExternalStationStatement(): Call to ewdb_base_RequestCursor failed.\n");
    return EWDB_RETURN_FAILURE;
  }
  
  return EWDB_RETURN_SUCCESS;
}


int PrepCreateExternalStationExec (EWDB_External_StationStruct *pStation,
                                   EWDB_Cursor *ppCursor)
{
  
  if ((pStation == NULL) ||  (ppCursor == NULL))
  {
    logit ("", "PrepCreateExternalStationExec(): Null parameters passed in!\n");
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
  
  if (InitCreateExternalStationStatement (SQL_STRING,
                                          &SSStatement) 
      != EWDB_RETURN_SUCCESS)
  {
    logit ("", "PrepCreateExternalStationExec(): InitCreateExternalStationStatement failed!\n");
    return EWDB_RETURN_FAILURE;
  }
  
  *ppCursor = SSStatement.pCda;
  
  return EWDB_RETURN_SUCCESS;
}  /* end PrepCreateExternalStationExec() */


int PostCreateExternalStationExec (int * pStationID)
{
  EWDB_Cursor pCursor;
  
  pCursor = SSStatement.pCda;
  ewdb_base_ReleaseCursor (pCursor);

  if (pStationID == NULL)
  {
    return EWDB_RETURN_FAILURE;
  }
  
  *pStationID=Local_StationStruct.StationID;
  
  if (Local_StationStruct.StationID < 0)
  {
    logit(""," PostCreateExternalStationExec(): SQL Proc Create_Or_Alter_Station_Ext() failed"
             " for (%s %s %s %s) with code(%d)\n", 
          Local_StationStruct.Station.Sta, Local_StationStruct.Station.Comp,
          Local_StationStruct.Station.Net, Local_StationStruct.Station.Loc,
          Local_StationStruct.StationID);
    return EWDB_RETURN_FAILURE;
  }

  return (EWDB_RETURN_SUCCESS);
}

