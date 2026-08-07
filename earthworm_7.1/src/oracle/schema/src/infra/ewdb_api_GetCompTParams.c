/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_api_GetCompTParams.c,v 1.6 2005/06/10 16:27:59 davidk Exp $
 *
 *     $Log: ewdb_api_GetCompTParams.c,v $
 *     Revision 1.6  2005/06/10 16:27:59  davidk
 *     DB API Cleanup
 *
 *     Revision 1.5  2003/12/04 19:58:00  davidk
 *     changed _snprintf() to snprintf().  There is a #define in platform.h
 *     that does the reverse transformation for MS platforms.
 *
 *     Revision 1.4  2003/12/04 19:51:00  davidk
 *     Fixed a syntax error that was preventing compilation.
 *
 *     Revision 1.3  2003/12/04 19:33:34  davidk
 *     Improved handling of SQL warnings and errors, passing more information
 *     back to the caller.
 *
 *     Revision 1.2  2003/12/03 00:44:55  davidk
 *     Fixed a C++ style comment //
 *
 *     Revision 1.1  2003/12/03 00:23:29  davidk
 *     Initial revision
 *
 */


#include <ewdb_ora_api.h>
#include <ewdb_cli_base.h>
#include <ewdb_ew_oci_base.h>


static char SQL_STRING[] = 
     "  Begin Get_CompT_Params(OUT_Local_iRetCode => :OUT_Local_iRetCode, "
           "IN_idCompT => :IN_idCompT, "
           "IN_idComp => :IN_idComp, "
           "IN_tOn => :IN_tOn, "
           "IN_tOff => :IN_tOff, "
           "OUT_idCompT => :OUT_idCompT, "
           "OUT_idComp => :OUT_idComp, "
           "OUT_dLat => :OUT_dLat, "
           "OUT_dLon => :OUT_dLon, "
           "OUT_dElev => :OUT_dElev, "
           "OUT_dAzm => :OUT_dAzm, "
           "OUT_dDip => :OUT_dDip, "
           "OUT_tOn => :OUT_tOn, "
           "OUT_tOff => :OUT_tOff, "
           "OUT_sSta => :OUT_sSta, "
           "OUT_sComp => :OUT_sComp, "
           "OUT_sNet => :OUT_sNet, "
           "OUT_sLoc => :OUT_sLoc, "
           "OUT_sComment => :OUT_sComment); End;";

static EWDB_OCI_SFS SQLParamsBindArray[] = 
{
  {0,1,0,0,0,OA_INT,":OUT_Local_iRetCode"},
  {0,1,0,0,0,OA_EWDBID,":IN_idCompT"},
  {0,1,0,0,0,OA_EWDBID,":IN_idComp"},
  {0,1,0,0,0,OA_DOUBLE,":IN_tOn"},
  {0,1,0,0,0,OA_DOUBLE,":IN_toff"},
  {0,1,0,0,0,OA_EWDBID,":OUT_idCompT"},
  {0,1,0,0,0,OA_EWDBID,":OUT_idComp"},
  {0,1,20,0,0,OA_DOUBLE,":OUT_dLat"},
  {0,1,20,0,0,OA_DOUBLE,":OUT_dLon"},
  {0,1,20,0,0,OA_DOUBLE,":OUT_dElev"},
  {0,1,20,0,0,OA_DOUBLE,":OUT_dAzm"},
  {0,1,20,0,0,OA_DOUBLE,":OUT_dDip"},
  {0,1,20,0,0,OA_DOUBLE,":OUT_tOn"},
  {0,1,20,0,0,OA_DOUBLE,":OUT_tOff"},
  {0,1,20,0,0,OA_SZ,":OUT_sSta"},
  {0,1,20,0,0,OA_SZ,":OUT_sComp"},
  {0,1,20,0,0,OA_SZ,":OUT_sNet"},
  {0,1,20,0,0,OA_SZ,":OUT_sLoc"},
  {0,1,1024,0,0,OA_SZ,":OUT_sComment"}
};

#define  NUM_FIELDS 19

/* static variables */
static char Local_szdLat[20],Local_szdLon[20],Local_szdElev[20],Local_szdAzm[20],Local_szdDip[20],
            Local_sztOn[20],Local_sztOff[20], Local_szINtOn[20],Local_szINtOff[20];
static int  Local_iRetCode;
static EWDB_ChannelStruct Local_Chan;
static char Local_szComment[1024];

/* Statement Struct for GetCompTParams szStatement */
static EWDB_OCIStatementStruct SSStatement;

/********************************
      FUNCTION PROTOTYPES
********************************/
static int PrepGetCompTParamsExec(EWDB_ChannelStruct * IN_pChan,
                           EWDB_Cursor * ppCursor);
static int PostGetCompTParamsExec(EWDB_ChannelStruct * pChan);
static int InitGetCompTParamsStatement(char *szStatement,
                                EWDB_OCIStatementStruct *pSS);


/* Describe Function ewdb_api_GetCompTParams
*********************************************************************/
int ewdb_api_GetCompTParams(EWDB_ChannelStruct * pChan)
{

  EWDB_Cursor pCursor;
  int rc;

  if(pChan == NULL)
  {
    logit("","ewdb_api_GetCompTParams():Null pointer passed in!\n");
    return(EWDB_RETURN_FAILURE);
  }

  ewdb_base_SetLastOraAPIActionTime();

  if(ewdb_base_Reconnect() != EWDB_RETURN_SUCCESS)
  {
    logit("","ewdb_api_GetCompTParams(): Could not reconnect to the database!\n");
    return(EWDB_RETURN_FAILURE);
  }

  if(PrepGetCompTParamsExec(pChan, &pCursor)
      != EWDB_RETURN_SUCCESS)
  {
    logit("","ewdb_api_GetCompTParams():PrepGetCompTParamsExec() failed.\n");
    return(EWDB_RETURN_FAILURE);
  }

  if(ewdb_base_SQLExecute(pCursor))
  {
    ewdb_base_ErrorReport(hEWDBC, pCursor,"ewdb_api_GetCompTParams():ewdb_base_SQLExecute", 1);
    return(ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  } 

  /* Commit the transaction(all the previous inserts!)
  ****************************************************/
  if(ewdb_base_SQLCommit(hEWDBC))
  {
    ewdb_base_ErrorReport(hEWDBC, hEWDBC,"ewdb_api_GetCompTParams():ewdb_base_SQLCommit",2);
    return(ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }

  rc = PostGetCompTParamsExec(pChan);

  ewdb_base_SetLastOraAPIActionTime();

  if(rc == EWDB_RETURN_FAILURE)
  {
    logit("", "ewdb_api_GetCompTParams(): ERROR:  PostGetCompTParamsExec() failed!!\n");
  }

  return(rc);
}  /* end ewdb_api_GetCompTParams() */


static int InitGetCompTParamsStatement(char *szStatement, EWDB_OCIStatementStruct *pSS)
{

  pSS->FieldArray[0].pVal = &Local_iRetCode;
  pSS->FieldArray[1].pVal = &Local_Chan.idCompT;
  pSS->FieldArray[2].pVal = &Local_Chan.Comp.idComp;
  pSS->FieldArray[3].pVal = Local_szINtOn;
  pSS->FieldArray[4].pVal = Local_szINtOff;
  pSS->FieldArray[5].pVal = &Local_Chan.idCompT;
  pSS->FieldArray[6].pVal = &Local_Chan.Comp.idComp;
  pSS->FieldArray[7].pVal = Local_szdLat;
  pSS->FieldArray[8].pVal = Local_szdLon;
  pSS->FieldArray[9].pVal = Local_szdElev;
  pSS->FieldArray[10].pVal = Local_szdAzm;
  pSS->FieldArray[11].pVal = Local_szdDip;
  pSS->FieldArray[12].pVal = Local_sztOn;
  pSS->FieldArray[13].pVal = Local_sztOff;
  pSS->FieldArray[14].pVal = Local_Chan.Comp.Sta;
  pSS->FieldArray[15].pVal = Local_Chan.Comp.Comp;
  pSS->FieldArray[16].pVal = Local_Chan.Comp.Net;
  pSS->FieldArray[17].pVal = Local_Chan.Comp.Loc;
  pSS->FieldArray[18].pVal = Local_szComment;

  return(ewdb_base_RequestCursor(szStatement, pSS, 0));
}  /* end InitGetCompTParamsStatement() */


static int PrepGetCompTParamsExec(EWDB_ChannelStruct * IN_pChan, EWDB_Cursor * ppCursor)
{

  SSStatement.NumOfFields = NUM_FIELDS;
  SSStatement.FieldArray = SQLParamsBindArray;
  SSStatement.RecordSize = 0;

  memset(Local_szdLat, 0, sizeof(Local_szdLat));
  memset(Local_szdLon, 0, sizeof(Local_szdLon));
  memset(Local_szdElev, 0, sizeof(Local_szdElev));
  memset(Local_szdAzm, 0, sizeof(Local_szdAzm));
  memset(Local_szdDip, 0, sizeof(Local_szdDip));
  memset(Local_sztOn, 0, sizeof(Local_sztOn));
  memset(Local_sztOff, 0, sizeof(Local_sztOff));
  memset(Local_szComment, 0, sizeof(Local_szComment));

  memset(&Local_Chan, 0, sizeof(Local_Chan));

  /* Copy user's vars to local vars here */
  Local_Chan.idCompT = IN_pChan->idCompT;
  Local_Chan.Comp.idComp = IN_pChan->Comp.idComp;
  snprintf(Local_szINtOn,sizeof(Local_szINtOn)-1,"%.2f",IN_pChan->tOn);
  Local_szINtOn[sizeof(Local_szINtOn)-1] = 0x00;
  snprintf(Local_szINtOff,sizeof(Local_szINtOff)-1,"%.2f",IN_pChan->tOff);
  Local_szINtOff[sizeof(Local_szINtOff)-1] = 0x00;

  if(InitGetCompTParamsStatement(SQL_STRING,
                           &SSStatement) != EWDB_RETURN_SUCCESS)
  {
    logit("", "PrepGetCompTParamsExec(): InitGetCompTParamsStatement failed!\n");
    return(EWDB_RETURN_FAILURE);
  }

  *ppCursor = SSStatement.pCda;
  
  return(EWDB_RETURN_SUCCESS);
}  /* end PrepGetCompTParamsExec() */


static int PostGetCompTParamsExec(EWDB_ChannelStruct * pChan)
{
  EWDB_Cursor pCursor;
  int rc;
  
  /* Copy local vars to user's vars here */
  if(Local_iRetCode >= 0)
  {
    pChan->idCompT = Local_Chan.idCompT;
    pChan->Comp.idComp =  Local_Chan.Comp.idComp;
    pChan->tOn = atof(Local_sztOn);
    pChan->tOff = atof(Local_sztOff);
    pChan->Comp.Lat =  (float)atof(Local_szdLat);
    pChan->Comp.Lon = (float)atof(Local_szdLon);
    pChan->Comp.Elev = (float)atof(Local_szdElev);
    pChan->Comp.Azm = (float)atof(Local_szdAzm);
    pChan->Comp.Dip = (float)atof(Local_szdDip);
    memcpy(pChan->Comp.Sta,Local_Chan.Comp.Sta, sizeof(Local_Chan.Comp.Sta));
    memcpy(pChan->Comp.Comp,Local_Chan.Comp.Comp, sizeof(Local_Chan.Comp.Comp));
    memcpy(pChan->Comp.Net,Local_Chan.Comp.Net, sizeof(Local_Chan.Comp.Net));
    memcpy(pChan->Comp.Loc,Local_Chan.Comp.Loc, sizeof(Local_Chan.Comp.Loc));

    rc = EWDB_RETURN_SUCCESS;
  }
  else
  {
    logit("","PostGetCompTParamsExec():  ERROR:   SQL Proc Get_CompT_Params(%d/%d) returned %d.\n",
          Local_Chan.idCompT, Local_Chan.Comp.idComp, Local_iRetCode);
    rc=EWDB_RETURN_FAILURE;
  }

  if(Local_iRetCode > 0)
    rc = EWDB_RETURN_WARNING;
  
  pCursor = SSStatement.pCda;
  
  ewdb_base_ReleaseCursor(pCursor);

  return(rc);
}  /* end PostGetCompTParamsExec() */


