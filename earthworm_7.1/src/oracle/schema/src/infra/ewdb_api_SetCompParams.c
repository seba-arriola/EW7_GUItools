
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_api_SetCompParams.c,v 1.5 2005/06/10 16:27:59 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_api_SetCompParams.c,v $
 *     Revision 1.5  2005/06/10 16:27:59  davidk
 *     DB API Cleanup
 *
 *     Revision 1.4  2003/12/03 00:23:29  davidk
 *     Modified the check of the return code for ewdb_api_GetTransformFunctionForChan().
 *     Now checks for !SUCCESS, instead of for FAILURE.
 *
 *     Revision 1.3  2001/07/23 17:10:29  davidk
 *     API Cleanup.
 *     Improved error handling of return codes from SQL Proc.
 *
 *     Revision 1.2  2001/05/15 02:16:31  davidk
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
  "Begin Set_Comp_Params(OUT_idCompT => :OUT_idCompT,"
  " IN_sSta => :IN_sSta, IN_sComp => :IN_sComp," 
  " IN_sNet => :IN_sNet, IN_sLoc => :IN_sLoc," 
  " IN_tOn => :IN_tOn, IN_tOff=>:IN_tOff,"
  " IN_dLat => :IN_dLat, IN_dLon => :IN_dLon," 
  " IN_dElev => :IN_dElev, IN_dAzm => :IN_dAzm," 
  " IN_dDip => :IN_dDip, IN_sComment => :IN_sComment); End;";

static EWDB_OCI_SFS SQLParamsBindArray[] = 
{
  {0,1,0,0,0,OA_INT,":OUT_idCompT"},
  {0,1,0,0,0,OA_SZ,":IN_sSta"},
  {0,1,0,0,0,OA_SZ,":IN_sComp"},
  {0,1,0,0,0,OA_SZ,":IN_sNet"},
  {0,1,0,0,0,OA_SZ,":IN_sLoc"},
  {0,1,0,0,0,OA_DOUBLE,":IN_tOn"},
  {0,1,0,0,0,OA_DOUBLE,":IN_tOff"},
  {0,1,0,0,0,OA_DOUBLE,":IN_dLat"},
  {0,1,0,0,0,OA_DOUBLE,":IN_dLon"},
  {0,1,0,0,0,OA_DOUBLE,":IN_dElev"},
  {0,1,0,0,0,OA_DOUBLE,":IN_dAzm"},
  {0,1,0,0,0,OA_DOUBLE,":IN_dDip"},
  {0,1,0,0,0,OA_SZ,":IN_sComment"}
};

#define  NUM_FIELDS 13

static EWDBid Local_idCompT;
static EWDB_StationStruct Local_Station;
static char Local_sztOn[20],Local_sztOff[20];
static char Local_szComment[4001];
static char Local_szdLat[20],Local_szdLon[20],Local_szdElev[20],
            Local_szdAzm[20],Local_szdDip[20];

/* Statement Struct for Set_Comp_Params szStatement */
static EWDB_OCIStatementStruct SSStatement;


/********************************
      FUNCTION PROTOTYPES
********************************/
static int PrepSetCompParamsExec(EWDB_StationStruct * IN_pStation,
                          double IN_tOn, double IN_tOff, 
                          char * IN_szComment, EWDB_Cursor * ppCursor);
static int PostSetCompParamsExec(EWDBid * pidCompT);
static int InitSetCompParamsStatement(char *szStatement, EWDB_OCIStatementStruct *pSS);




/* 
     pStation:  pointer to caller-filled EWDB_StationStruct
                that contains the params for the component.
                pStation->idChan and pStation->idComp are ignored.
     pidCompT:  Local_idCompT of the component - time interval record
                that was created or updated by this function.
     tOn,tOff:  Start/End of time interval for which the params
                are correct for the component.
     Local_szComment: Comment regarding the component time interval.
     
     somehow this function will take a station struct
     or something comparable, and a time/interval or 
     Local_idCompT value.  It has to make sense and it has 
     to match up with the SQL function SetCompParams()
******************************************************************/
int ewdb_api_SetCompParams(EWDB_StationStruct * pStation, 
                           double tOn, double tOff,
                           EWDBid * pidCompT, char * Local_szComment)
{

  EWDB_Cursor pCursor;
  int rc;

  if(pStation == NULL || pidCompT== NULL || Local_szComment== NULL)
  {
    logit("", "ewdb_api_SetCompParams():Null pointer passed in!\n");
    return EWDB_RETURN_FAILURE;
  }

  ewdb_base_SetLastOraAPIActionTime();


  if(ewdb_base_Reconnect() != EWDB_RETURN_SUCCESS)
  {
    logit("", "ewdb_api_SetCompParams(): Could not reconnect to the database!\n");
    return(EWDB_RETURN_FAILURE);
  }

  if(PrepSetCompParamsExec(pStation, tOn, tOff, Local_szComment, &pCursor) 
      != EWDB_RETURN_SUCCESS)
  {
    logit("", "ewdb_api_SetCompParams(): PrepSetCompParamsExec() failed.\n");
    return(ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }

  if(ewdb_base_SQLExecute(pCursor))
  {
    ewdb_base_ErrorReport(hEWDBC, pCursor,"ewdb_api_SetCompParams(): ewdb_base_SQLExecute", 1);
    return(ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  } 

  
  /* Commit the transaction(all the previous inserts!)
     In Case there is any auditing or logging or debug
     changes made in the stored procedures.
  ****************************************************/
  if(ewdb_base_SQLCommit(hEWDBC))
  {
    ewdb_base_ErrorReport(hEWDBC, hEWDBC,"ewdb_api_SetCompParams(): ewdb_base_SQLCommit",2);
    return(ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }

  rc = PostSetCompParamsExec(pidCompT);
  if(rc != EWDB_RETURN_SUCCESS)
  {
    if(rc == EWDB_RETURN_FAILURE)
    {
      logit("", "ewdb_api_SetCompParams(): PostSetCompParamsExec failed!\n");
      return(ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
    }
    else if(rc == EWDB_RETURN_WARNING)
    {
      return(EWDB_RETURN_FAILURE);
    }
  }

  ewdb_base_SetLastOraAPIActionTime();

  return(EWDB_RETURN_SUCCESS);
}  /* end ewdb_api_SetCompParams() */

 
static int InitSetCompParamsStatement(char *szStatement, EWDB_OCIStatementStruct *pSS)
{
 
  pSS->FieldArray[0].pVal = &Local_idCompT;
  pSS->FieldArray[1].pVal = Local_Station.Sta;
  pSS->FieldArray[2].pVal = Local_Station.Comp;
  pSS->FieldArray[3].pVal = Local_Station.Net;
  pSS->FieldArray[4].pVal = Local_Station.Loc;
  pSS->FieldArray[5].pVal = Local_sztOn;
  pSS->FieldArray[6].pVal = Local_sztOff;
  pSS->FieldArray[7].pVal = Local_szdLat;
  pSS->FieldArray[8].pVal = Local_szdLon;
  pSS->FieldArray[9].pVal = Local_szdElev;
  pSS->FieldArray[10].pVal = Local_szdAzm;
  pSS->FieldArray[11].pVal = Local_szdDip;
  pSS->FieldArray[12].pVal = Local_szComment;

  return(ewdb_base_RequestCursor(szStatement, pSS, 0));
}  /* End InitSetCompParamsStatement() */


static int PrepSetCompParamsExec(EWDB_StationStruct * IN_pStation,
                                 double IN_tOn, double IN_tOff, 
                                 char * IN_szComment, EWDB_Cursor * ppCursor) 
{

  SSStatement.NumOfFields = NUM_FIELDS;
  SSStatement.FieldArray = SQLParamsBindArray;
  SSStatement.RecordSize = 0;

  memcpy(&Local_Station,IN_pStation,sizeof(Local_Station));
  sprintf(Local_sztOn,"%.2f",IN_tOn);
  sprintf(Local_sztOff,"%.2f",IN_tOff);

  sprintf(Local_szdLat, "%.4f", IN_pStation->Lat);
  sprintf(Local_szdLon, "%.4f", IN_pStation->Lon);
  sprintf(Local_szdElev, "%.4f", IN_pStation->Elev);
  sprintf(Local_szdAzm, "%.0f", IN_pStation->Azm);
  sprintf(Local_szdDip, "%.0f", IN_pStation->Dip);

  strncpy(Local_szComment,IN_szComment,sizeof(Local_szComment)-1);
  Local_szComment[sizeof(Local_szComment)-1]=0;

  if(InitSetCompParamsStatement(SQL_STRING,
                           &SSStatement) != EWDB_RETURN_SUCCESS)
  {
    logit("", "PrepSetCompParamsExec(): InitSetCompParamsStatement failed!\n");
    return EWDB_RETURN_FAILURE;
  }

  *ppCursor = SSStatement.pCda;
  
  return(EWDB_RETURN_SUCCESS);
}  /* End PrepSetCompParamsExec() */


static int PostSetCompParamsExec(EWDBid * pidCompT)
{
  EWDB_Cursor pCursor;
  
  *pidCompT=Local_idCompT;
  
  pCursor = SSStatement.pCda;
  
  ewdb_base_ReleaseCursor(pCursor);
  
  if(Local_idCompT <= 0)
  {
    logit("","PostSetCompParamsExec(): ERROR!  SQL PROC Set_Comp_Params() "
          "returned error(%d)!\n",
          Local_idCompT);
    return(EWDB_RETURN_WARNING);
  }
  
  return(EWDB_RETURN_SUCCESS);
}  /* End PostSetCompParamsExec() */

