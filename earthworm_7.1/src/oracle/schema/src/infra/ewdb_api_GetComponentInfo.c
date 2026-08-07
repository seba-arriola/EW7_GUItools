/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_api_GetComponentInfo.c,v 1.5 2005/06/10 16:27:59 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_api_GetComponentInfo.c,v $
 *     Revision 1.5  2005/06/10 16:27:59  davidk
 *     DB API Cleanup
 *
 *     Revision 1.4  2001/07/26 21:48:38  davidk
 *     API Cleanup.  Changed two variables to static.
 *
 *     Revision 1.3  2001/07/23 17:08:32  davidk
 *     API Cleanup.
 *     Improved error handling of return codes from SQL Proc.
 *
 *     Revision 1.2  2001/05/15 02:16:29  davidk
 *     Moved functions around between the apps, DB API, and DB API INTERNAL
 *     levels.  Renamed functions and files.  Added support for amplitude
 *     magnitude types.  Reformatted makefiles.
 *
 *     Revision 1.1  2001/02/28 17:19:22  lucky
 *     Initial revision
 *
 *     Revision 1.8  2001/02/21 10:58:33  davidk
 *     Code was retrofitted to use the new db_cli_base API
 *     in lieu of the oci_base() and OCI7 function API's.layer, which
 *     provides abstraction from the OCI7 function calls to
 *     ora_api layer code.  See comments in
 *     schema/src/include/internal/ewdb_cli_base.h for more info
 *     on the details of the changes made.
 *
 *     Revision 1.7  2000/06/21 22:52:19  lucky
 *      Cleaned up logit calls to make log files more readable.
 *
 *     Revision 1.6  2000/05/15 22:39:33  davidk
 *     Added inclusion of <time_ew.h> for the prototypes of
 *     precision time functions used in this file for performance measurements.
 *     Added prototypes for the functions implemented in this file.
 *     Reformatted whitespace(put spaces in inplace of tabs).  Added
 *     some additional comments, and changed informational logits
 *     to only print when the EWDB_Debug flag is on.  Stopped all logits
 *     from printing to stderr, so as to be compatible with CGI-BIN web
 *     apps.
 *
 *     Revision 1.5  2000/03/30 18:39:52  davidk
 *     added performance measurements and log messages to show how long
 *     pieces of code are taking.
 *
 *     Revision 1.4  2000/03/24 22:48:52  davidk
 *     removed debugging statements that were no longer necessary and
 *     now generating VERY large logfiles.
 *
 *     Revision 1.3  1999/11/09 18:21:08  lucky
 *     *** empty log message ***
 *
 *
 */


#include <ewdb_ora_api.h>
#include <ewdb_cli_base.h>
#include <time_ew.h>
#include <ewdb_ew_oci_base.h>

static char SQL_STRING[] =
        "Begin Get_Comp_Params(OUT_idCompT => :OUT_idCompT, IN_idChan => :IN_idChan, "
        " IN_tTime => :IN_tTime, OUT_idComp => :OUT_idComp, "
        " OUT_dLat => :OUT_dLat, OUT_dLon => :OUT_dLon,  OUT_dElev => :OUT_dElev, "
        " OUT_dAzm => :OUT_dAzm, OUT_dDip => :OUT_dDip, "
        " OUT_sSta => :OUT_sSta, OUT_sComp => :OUT_sComp, "
        " OUT_sNet => :OUT_sNet, OUT_sLoc => :OUT_sLoc); End;";


static EWDB_OCI_SFS SQLParamsBindArray[] =

{
  {0,1,0,0,0,OA_INT,":OUT_idCompT"},
  {0,1,0,0,0,OA_INT,":IN_idChan"},
  {0,1,0,0,0,OA_INT,":IN_tTime"},
  {0,1,0,0,0,OA_INT,":OUT_idComp"},
  {0,1,0,0,0,OA_FLOAT,":OUT_dLat"},
  {0,1,0,0,0,OA_FLOAT,":OUT_dLon"},
  {0,1,0,0,0,OA_FLOAT,":OUT_dElev"},
  {0,1,0,0,0,OA_FLOAT,":OUT_dAzm"},
  {0,1,0,0,0,OA_FLOAT,":OUT_dDip"},
  {0,1,0,0,0,OA_SZ,":OUT_sSta"},
  {0,1,0,0,0,OA_SZ,":OUT_sComp"},
  {0,1,0,0,0,OA_SZ,":OUT_sNet"},
  {0,1,0,0,0,OA_SZ,":OUT_sLoc"}
};

/* define the total number of fields to be bound */
#define NUM_FIELDS 13

/* Insertion Struct for GetComponentInfo statement */
static EWDB_OCIStatementStruct SSStatement;

static EWDB_StationStruct Local_Station;

static char Local_szdLat[20],Local_szdLon[20],Local_szdElev[20],
            Local_szdAzm[20],Local_szdDip[20];

static EWDBid Local_idCompT;
static int Local_tTime;

/********************************
      FUNCTION PROTOTYPES
********************************/
static int PrepGetComponentInfoExec(EWDBid idChan, int tParamTime, 
                             EWDB_Cursor * ppCursor);
static int PostGetComponentInfoExec(EWDB_StationStruct * pStation);
static int InitGetComponentInfoStatement(char * szStatement, 
                                  EWDB_OCIStatementStruct *pSS);




/* Get a station struct full of parameters for a component.
   An idChan is entered, but the attributes returned belong to 
   the component that that channel is associated with
   at time tParamsTime.
*********************************************************************/
int ewdb_api_GetComponentInfo(EWDBid idChan, int tParamsTime, 
                              EWDB_StationStruct * pStation)
{

  EWDB_Cursor  pCursor;
 
  ewdb_base_SetLastOraAPIActionTime();

  if( ewdb_base_Reconnect() != EWDB_RETURN_SUCCESS )
  {
    return( EWDB_RETURN_FAILURE );
  }

  if( PrepGetComponentInfoExec(idChan,tParamsTime,&pCursor) != EWDB_RETURN_SUCCESS)
  {
    return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }


  if( ewdb_base_SQLExecute(pCursor) )
  {
    ewdb_base_ErrorReport(hEWDBC, pCursor,"ewdb_api_GetComponentInfo(): ewdb_base_SQLExecute",1);
    return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  } 

  /* Commit the transaction (all the previous inserts!)
  In Case there is any auditing or logging or debug
  changes made in the stored procedures.
  ****************************************************/
  if (ewdb_base_SQLCommit (hEWDBC))
  {
    ewdb_base_ErrorReport (hEWDBC, hEWDBC,"ewdb_api_GetComponentInfo(): :ewdb_base_SQLCommit",2);
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  }
  
  if( PostGetComponentInfoExec(pStation) != EWDB_RETURN_SUCCESS)
  {
    return(EWDB_RETURN_FAILURE);
  }

  ewdb_base_SetLastOraAPIActionTime();

  return(EWDB_RETURN_SUCCESS);
} /* end ewdb_api_GetComponentInfo() */ 


static int InitGetComponentInfoStatement(char * szStatement, 
                                         EWDB_OCIStatementStruct *pSS)
{

  pSS->FieldArray[0].pVal = &Local_idCompT;
  pSS->FieldArray[1].pVal = &(Local_Station.idChan);
  pSS->FieldArray[2].pVal = &Local_tTime;
  pSS->FieldArray[3].pVal = &(Local_Station.idComp);
  pSS->FieldArray[4].pVal = Local_szdLat;
  pSS->FieldArray[5].pVal = Local_szdLon;
  pSS->FieldArray[6].pVal = Local_szdElev;
  pSS->FieldArray[7].pVal = Local_szdAzm;
  pSS->FieldArray[8].pVal = Local_szdDip;
  pSS->FieldArray[9].pVal = Local_Station.Sta;
  pSS->FieldArray[10].pVal = Local_Station.Comp;
  pSS->FieldArray[11].pVal = Local_Station.Net;
  pSS->FieldArray[12].pVal = Local_Station.Loc;

  return(ewdb_base_RequestCursor(szStatement, pSS,0));
}  /* End of InitGetComponentInfoStatement() */


static int PrepGetComponentInfoExec(EWDBid idChan, int tParamTime, 
                             EWDB_Cursor * ppCursor)
{

  SSStatement.NumOfFields=NUM_FIELDS;
  SSStatement.FieldArray=SQLParamsBindArray;
  SSStatement.RecordSize=0;

  /* initialize output buffers */
  memset(Local_szdLat, 0, sizeof(Local_szdLat));
  memset(Local_szdLon, 0, sizeof(Local_szdLon));
  memset(Local_szdElev, 0, sizeof(Local_szdElev));
  memset(Local_szdAzm, 0, sizeof(Local_szdAzm));
  memset(Local_szdDip, 0, sizeof(Local_szdDip));
  memset(&Local_Station, 0, sizeof(Local_Station));

  /* Copy caller's params to local static copies */
  Local_Station.idChan=idChan;
  Local_tTime=tParamTime;

  if(InitGetComponentInfoStatement(SQL_STRING, &SSStatement) != EWDB_RETURN_SUCCESS)
  {
    *ppCursor = NULL;
    return(EWDB_RETURN_FAILURE);
  }

  *ppCursor=SSStatement.pCda;

  return(EWDB_RETURN_SUCCESS);
}  /* end PrepGetComponentInfoExec() */


static int PostGetComponentInfoExec(EWDB_StationStruct * pStation)
{

  ewdb_base_ReleaseCursor(SSStatement.pCda);

  if(Local_idCompT >= 0)
  {
    Local_Station.Lat=(float)atof(Local_szdLat);
    Local_Station.Lon=(float)atof(Local_szdLon);
    Local_Station.Elev=(float)atof(Local_szdElev);
    Local_Station.Azm=(float)atof(Local_szdAzm);
    Local_Station.Dip=(float)atof(Local_szdDip);

    /* Copy the results to the users struct */
    memcpy(pStation,&Local_Station,sizeof(EWDB_StationStruct));

    return(EWDB_RETURN_SUCCESS);
  }
  else
  {
    logit("","ERROR! PostGetComponentInfoExec(): SQL Proc Get_Comp_Params() "
          "failed with error(%d)!\n",
          Local_idCompT);
    return(EWDB_RETURN_WARNING);
  }
}   /* End PostGetComponentInfoExec() */

