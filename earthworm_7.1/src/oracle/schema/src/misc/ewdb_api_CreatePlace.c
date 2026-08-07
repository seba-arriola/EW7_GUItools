/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_api_CreatePlace.c,v 1.3 2005/06/06 16:42:50 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_api_CreatePlace.c,v $
 *     Revision 1.3  2005/06/06 16:42:50  davidk
 *     DB API Cleanup
 *
 *     Revision 1.2  2003/01/30 23:13:30  lucky
 *     *** empty log message ***
 *
 *     Revision 1.1  2002/08/26 16:44:31  davidk
 *     Initial revision
 *
 */


#include <ewdb_ora_api.h>
#include <ewdb_cli_base.h>
#include <ewdb_ew_oci_base.h>


static char SQL_STRING[] =
  "Begin Create_Place(OUT_idPlace => :OUT_idPlace,"
  "IN_sState => :IN_sState,"
  "IN_sPlaceName => :IN_sPlaceName,"
  "IN_dLat => :IN_dLat,"
  "IN_dLon => :IN_dLon,"
  "IN_dElev => :IN_dElev,"
  "IN_iPopulation => :IN_iPopulation,"
  "IN_iPlaceMajorType => :IN_iPlaceMajorType,"
  "IN_iPlaceMinorType => :IN_iPlaceMinorType,"
  "IN_sCounty => :IN_sCounty,"
  "IN_sCountry => :IN_sCountry"
  "); End;";
  
static EWDB_OCI_SFS SQLParamsBindArray[] = 
{
  {0,1,0,0,0,OA_EWDBID,":OUT_idPlace"},
  {0,1,0,0,0,OA_SZ,    ":IN_sState"},
  {0,1,0,0,0,OA_SZ,    ":IN_sPlaceName"},
  {0,1,0,0,0,OA_DOUBLE,":IN_dLat"},
  {0,1,0,0,0,OA_DOUBLE,":IN_dLon"},
  {0,1,0,0,0,OA_DOUBLE,":IN_dElev"},
  {0,1,0,0,0,OA_INT,   ":IN_iPopulation"},
  {0,1,0,0,0,OA_INT,   ":IN_iPlaceMajorType"},
  {0,1,0,0,0,OA_INT,   ":IN_iPlaceMinorType"},
  {0,1,0,0,0,OA_SZ,    ":IN_sCounty"},
  {0,1,0,0,0,OA_SZ,    ":IN_sCountry"}
};

#define  NUM_FIELDS  11

static char    Local_szdLat[20], Local_szdLon[20], Local_szdElev[20];
static  EWDB_PlaceStruct Local_PlaceStruct;

/* Insertion Struct for CreatePlace szStatement */
EWDB_OCIStatementStruct SSStatement;

static int PrepCreatePlaceExec(EWDB_PlaceStruct *pPlace, EWDB_Cursor *ppCursor);
static int PostCreatePlaceExec(EWDB_PlaceStruct *pPlace);
static int InitCreatePlaceStatement(char *szStatement, EWDB_OCIStatementStruct *pSS);

int ewdb_api_CreatePlace(EWDB_PlaceStruct *pPlace)
{

  EWDB_Cursor pCursor;
  int rc;

  if (pPlace == NULL)
  {
    logit ("", "ewdb_api_CreatePlace(): Null parameters passed in!\n");
    return EWDB_RETURN_FAILURE;
  }

  ewdb_base_SetLastOraAPIActionTime();
  
  if (ewdb_base_Reconnect() != EWDB_RETURN_SUCCESS)
  {
    logit("","ewdb_api_CreatePlace(): Could not reconnect to the database!\n");
    return (EWDB_RETURN_FAILURE);
  }

  if (PrepCreatePlaceExec(pPlace, &pCursor) != EWDB_RETURN_SUCCESS)
  {
    logit ("", "ewdb_api_CreatePlace(): PrepCreatePlaceExec() failed.\n");
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  }

  if (ewdb_base_SQLExecute(pCursor))
  {
    logit ("", "ewdb_api_CreatePlace(): ewdb_base_SQLExecute() failed.\n");
    ewdb_base_ErrorReport (hEWDBC, pCursor,"CreatePlace:ewdb_base_SQLExecute", 1);
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  } 

  
  /* Commit the transaction (all the previous inserts!)
     In Case there is any auditing or logging or debug
     changes made in the stored procedures.
  ****************************************************/
  if (ewdb_base_SQLCommit(hEWDBC))
  {
    logit ("", "ewdb_api_CreatePlace(): ewdb_base_SQLCommit() failed.\n");
    ewdb_base_ErrorReport (hEWDBC, hEWDBC,"CreatePlace:ewdb_base_SQLCommit",2);
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  }

  rc = PostCreatePlaceExec(pPlace);
  if(rc == EWDB_RETURN_FAILURE)
  {
    logit("", "ewdb_api_CreatePlace(): PostCreatePlaceExec failed!\n");
    return(ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }
  else if(rc == EWDB_RETURN_WARNING)
  {
    return(EWDB_RETURN_FAILURE);
  }
  
  return(EWDB_RETURN_SUCCESS);
}  /* ewdb_api_CreatePlace() */


static int InitCreatePlaceStatement(char *szStatement, EWDB_OCIStatementStruct *pSS)
{

  pSS->FieldArray[0].pVal = &(Local_PlaceStruct.idPlace);
  pSS->FieldArray[1].pVal = Local_PlaceStruct.szState;
  pSS->FieldArray[2].pVal = Local_PlaceStruct.szPlaceName;
  pSS->FieldArray[3].pVal = Local_szdLat;
  pSS->FieldArray[4].pVal = Local_szdLon;
  pSS->FieldArray[5].pVal = Local_szdElev;
  pSS->FieldArray[6].pVal = &(Local_PlaceStruct.iPopulation);
  pSS->FieldArray[7].pVal = &(Local_PlaceStruct.iPlaceMajorType);
  pSS->FieldArray[8].pVal = &(Local_PlaceStruct.iPlaceMinorType);
  pSS->FieldArray[9].pVal = Local_PlaceStruct.szCounty;
  pSS->FieldArray[10].pVal = Local_PlaceStruct.szCountry;

  return(ewdb_base_RequestCursor (szStatement, pSS, 0));
}  /* End InitSetChanParamsStatement() */


static int PrepCreatePlaceExec (EWDB_PlaceStruct *pPlace, EWDB_Cursor *ppCursor)
{

  if ((pPlace == NULL) || (ppCursor == NULL))
  {
    logit ("", "PrepCreatePlaceExec(): Null parameters passed in!\n");
    return EWDB_RETURN_FAILURE;
  }

  SSStatement.NumOfFields = NUM_FIELDS;
  SSStatement.FieldArray = SQLParamsBindArray;
  SSStatement.RecordSize = 0;

  memcpy (&Local_PlaceStruct, pPlace, sizeof (EWDB_PlaceStruct));

  sprintf (Local_szdLat, "%.3f", pPlace->dLat);
  sprintf (Local_szdLon, "%.3f", pPlace->dLon);
  sprintf (Local_szdElev, "%.2f", pPlace->dElev);


  if (ewdb_base_RequestCursor (SQL_STRING, &SSStatement, 0) != 0)
  {
    logit ("", "PrepCreatePlaceExec(): Call to ewdb_base_RequestCursor failed.\n");
    return EWDB_RETURN_FAILURE;
  }

  *ppCursor = SSStatement.pCda;
  return(EWDB_RETURN_SUCCESS);
}


static int PostCreatePlaceExec (EWDB_PlaceStruct *pPlace)
{
  EWDB_Cursor pCursor;
  
  pCursor = SSStatement.pCda;
  
  ewdb_base_ReleaseCursor (pCursor);
  
  if (pPlace == NULL)
  {
    logit ("", "PostCreatePlaceExec(): Invalid arguments passed in.\n");
    return EWDB_RETURN_FAILURE;
  }
  
  pPlace->idPlace = Local_PlaceStruct.idPlace;

  if(pPlace->idPlace <= 0)
  {
    if(pPlace->idPlace == -1)
      logit("","PostCreatePlaceExec():  SQL Proc Create_Place() returned "
                "\"Unknown error\".  See SQL debug table for details.\n");
    else
      logit("","PostCreatePlaceExec():  SQL Proc Create_Place() returned "
               "the following error(%d).  Please see that proc for details.\n",
            pPlace->idPlace);
    return(EWDB_RETURN_WARNING);
  }

  return(EWDB_RETURN_SUCCESS);
}  /* end PostCreatePlaceExec() */



