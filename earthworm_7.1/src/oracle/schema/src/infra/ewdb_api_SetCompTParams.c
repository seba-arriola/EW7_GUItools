
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_api_SetCompTParams.c,v 1.3 2005/06/10 16:27:59 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_api_SetCompTParams.c,v $
 *     Revision 1.3  2005/06/10 16:27:59  davidk
 *     DB API Cleanup
 *
 *     Revision 1.2  2004/07/19 17:44:42  davidk
 *     Fixed a bug in the code that used (Local_szComment) the uninitialized static variable
 *     for data input, instead of the user's (IN_szComment) variable.
 *     Comment field now gets properly passed into SQL proc.
 *
 *     Revision 1.1  2003/12/03 00:23:29  davidk
 *     Initial revision
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
  "Begin Set_CompT_Params(OUT_iRetCode => :OUT_iRetCode,"
  " IN_idCompT => :IN_idCompT, "
  " IN_dLat => :IN_dLat, IN_dLon => :IN_dLon," 
  " IN_dElev => :IN_dElev, IN_dAzm => :IN_dAzm," 
  " IN_dDip => :IN_dDip, IN_sComment => :IN_sComment); End;";

static EWDB_OCI_SFS SQLParamsBindArray[] = 
{
  {0,1,0,0,0,OA_INT,":OUT_iRetCode"},
  {0,1,0,0,0,OA_EWDBID,":IN_idCompT"},
  {0,1,0,0,0,OA_DOUBLE,":IN_dLat"},
  {0,1,0,0,0,OA_DOUBLE,":IN_dLon"},
  {0,1,0,0,0,OA_DOUBLE,":IN_dElev"},
  {0,1,0,0,0,OA_DOUBLE,":IN_dAzm"},
  {0,1,0,0,0,OA_DOUBLE,":IN_dDip"},
  {0,1,0,0,0,OA_SZ,":IN_sComment"}
};

#define  NUM_FIELDS 8

static int  Local_iRetCode;
static EWDBid Local_idCompT;
static EWDB_StationStruct Local_Station;
static char Local_szComment[4001];
static char Local_szdLat[20],Local_szdLon[20],Local_szdElev[20],
            Local_szdAzm[20],Local_szdDip[20];

/* Statement Struct for Set_CompT_Params szStatement */
static EWDB_OCIStatementStruct SSStatement;


/********************************
      FUNCTION PROTOTYPES
********************************/
static int PrepSetComptParamsExec(EWDB_ChannelStruct * IN_pChan,
                                  char * IN_szComment, EWDB_Cursor * ppCursor);
static int PostSetComptParamsExec();
static int InitSetComptParamsStatement(char *szStatement, EWDB_OCIStatementStruct *pSS);

/* 
     IN_pChan:  pointer to caller-filled EWDB_ChannelStruct
                only Local_idCompT, lat, lon, elev, azm, and dip
                are read..
     Local_szComment: Comment regarding the component time interval.
     
     this function updates the params of the given CompT.
     It uses the SQL function SetComptParams() to accomplish this.
******************************************************************/
int ewdb_api_SetComptParams(EWDB_ChannelStruct * IN_pChan,
                            char * IN_szComment)
{

  EWDB_Cursor pCursor;
  int rc;

  if(IN_pChan == NULL  || IN_szComment== NULL)
  {
    logit("", "ewdb_api_SetComptParams(): Null pointer passed in!\n");
    return EWDB_RETURN_FAILURE;
  }

  ewdb_base_SetLastOraAPIActionTime();


  if(ewdb_base_Reconnect() != EWDB_RETURN_SUCCESS)
  {
    logit("", "ewdb_api_SetComptParams(): Could not reconnect to the database!\n");
    return(EWDB_RETURN_FAILURE);
  }

  if(PrepSetComptParamsExec(IN_pChan, IN_szComment, &pCursor) 
      != EWDB_RETURN_SUCCESS)
  {
    logit("", "ewdb_api_SetComptParams(): PrepSetComptParamsExec() failed.\n");
    return(ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }

  if(ewdb_base_SQLExecute(pCursor))
  {
    ewdb_base_ErrorReport(hEWDBC, pCursor,"ewdb_api_SetComptParams(): ewdb_base_SQLExecute", 1);
    return(ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  } 

  
  /* Commit the transaction(all the previous inserts!)
     In Case there is any auditing or logging or debug
     changes made in the stored procedures.
  ****************************************************/
  if(ewdb_base_SQLCommit(hEWDBC))
  {
    ewdb_base_ErrorReport(hEWDBC, hEWDBC,"ewdb_api_SetComptParams(): ewdb_base_SQLCommit",2);
    return(ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }

  rc = PostSetComptParamsExec();
  if(rc != EWDB_RETURN_SUCCESS)
  {
    if(rc == EWDB_RETURN_FAILURE)
    {
      logit("", "ewdb_api_SetComptParams(): Call to PostSetComptParamsExec failed!\n");
      return(ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
    }
    else if(rc == EWDB_RETURN_WARNING)
    {
      return(EWDB_RETURN_FAILURE);
    }
    else
    {
      logit("", "ewdb_api_SetComptParams(): Call to PostSetComptParamsExec returned "
                "unknown return code(%d)!\n", rc);
      return(ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
    }
  }

  ewdb_base_SetLastOraAPIActionTime();

  return(EWDB_RETURN_SUCCESS);
}  /* end ewdb_api_SetComptParams() */

 
static int InitSetComptParamsStatement(char *szStatement, EWDB_OCIStatementStruct *pSS)
{
  pSS->FieldArray[0].pVal = &Local_iRetCode;
  pSS->FieldArray[1].pVal = &Local_idCompT;
  pSS->FieldArray[2].pVal = Local_szdLat;
  pSS->FieldArray[3].pVal = Local_szdLon;
  pSS->FieldArray[4].pVal = Local_szdElev;
  pSS->FieldArray[5].pVal = Local_szdAzm;
  pSS->FieldArray[6].pVal = Local_szdDip;
  pSS->FieldArray[7].pVal = Local_szComment;

  return(ewdb_base_RequestCursor(szStatement, pSS, 0));
}  /* End InitSetComptParamsStatement() */


static int PrepSetComptParamsExec(EWDB_ChannelStruct * IN_pChan,
                                  char * IN_szComment, EWDB_Cursor * ppCursor) 
{

  SSStatement.NumOfFields = NUM_FIELDS;
  SSStatement.FieldArray = SQLParamsBindArray;
  SSStatement.RecordSize = 0;

  Local_idCompT = IN_pChan->idCompT;
  sprintf(Local_szdLat, "%.4f", IN_pChan->Comp.Lat);
  sprintf(Local_szdLon, "%.4f", IN_pChan->Comp.Lon);
  sprintf(Local_szdElev, "%.4f", IN_pChan->Comp.Elev);
  sprintf(Local_szdAzm, "%.0f", IN_pChan->Comp.Azm);
  sprintf(Local_szdDip, "%.0f", IN_pChan->Comp.Dip);

  if(IN_szComment)
  {
    strncpy(Local_szComment,IN_szComment,sizeof(Local_szComment)-1);
    Local_szComment[sizeof(Local_szComment)-1]=0;
  }
  else
  {
    Local_szComment[0] = 0x00;
  }

  if(InitSetComptParamsStatement(SQL_STRING, &SSStatement) 
     != EWDB_RETURN_SUCCESS)
  {
    logit("", "PrepSetComptParamsExec(): InitSetComptParamsStatement failed!\n");
    return EWDB_RETURN_FAILURE;
  }

  *ppCursor = SSStatement.pCda;
  
  return(EWDB_RETURN_SUCCESS);
}  /* End PrepSetComptParamsExec() */


static int PostSetComptParamsExec(EWDBid * pidCompT)
{
  EWDB_Cursor pCursor;
  
  pCursor = SSStatement.pCda;
  
  ewdb_base_ReleaseCursor(pCursor);
  
  if(Local_iRetCode < 0)
  {
    logit("","PostSetComptParamsExec(): ERROR!  SQL PROC Set_CompT_Params(%d) "
          "returned error(%d)!\n",
          Local_idCompT, Local_iRetCode);
    return(EWDB_RETURN_WARNING);
  }
  
  return(EWDB_RETURN_SUCCESS);
}  /* End PostSetComptParamsExec() */

