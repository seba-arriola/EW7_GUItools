/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_api_GetChanParams.c,v 1.6 2005/06/27 15:30:01 davidk Exp $
 *
 *     $Log: ewdb_api_GetChanParams.c,v $
 *     Revision 1.6  2005/06/27 15:30:01  davidk
 *     Fixed bugs introduced during global fetch/replace during DB cleanup.
 *
 *     Revision 1.5  2005/06/10 16:27:59  davidk
 *     DB API Cleanup
 *
 *     Revision 1.4  2004/04/06 17:47:24  davidk
 *     Fixed a bug in Prep_XXX() where tOn and tOff, were not copied from the
 *     caller's structure to the internal variables, and thus were not used in the SQL
 *     query.
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
     "  Begin Get_Chan_Params(IN_idChanT => :IN_idChanT, "
           "IN_idChan => :IN_idChan, "
           "IN_tOn => :IN_tOn, "
           "IN_tOff => :IN_tOff, "
           "OUT_iRetCode => :OUT_iRetCode, "
           "OUT_idChanT => :OUT_idChanT, "
           "OUT_idChan => :OUT_idChan, "
           "OUT_idComp => :OUT_idComp, "
           "OUT_idCompT => :OUT_idCompT, "
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
           "OUT_sLoc => :OUT_sLoc); End;";

static EWDB_OCI_SFS SQLParamsBindArray[] = 
{
  {0,1,0,0,0,OA_EWDBID,":IN_idChanT"},
  {0,1,0,0,0,OA_EWDBID,":IN_idChan"},
  {0,1,0,0,0,OA_DOUBLE,":IN_tOn"},
  {0,1,0,0,0,OA_DOUBLE,":IN_tOff"},
  {0,1,0,0,0,OA_INT,":OUT_iRetCode"},
  {0,1,0,0,0,OA_EWDBID,":OUT_idChanT"},
  {0,1,0,0,0,OA_EWDBID,":OUT_idChan"},
  {0,1,0,0,0,OA_EWDBID,":OUT_idComp"},
  {0,1,0,0,0,OA_EWDBID,":OUT_idCompT"},
  {0,1,0,0,0,OA_DOUBLE,":OUT_dLat"},
  {0,1,0,0,0,OA_DOUBLE,":OUT_dLon"},
  {0,1,0,0,0,OA_DOUBLE,":OUT_dElev"},
  {0,1,0,0,0,OA_DOUBLE,":OUT_dAzm"},
  {0,1,0,0,0,OA_DOUBLE,":OUT_dDip"},
  {0,1,0,0,0,OA_DOUBLE,":OUT_tOn"},
  {0,1,0,0,0,OA_DOUBLE,":OUT_tOff"},
  {0,1,0,0,0,OA_SZ,":OUT_sSta"},
  {0,1,0,0,0,OA_SZ,":OUT_sComp"},
  {0,1,0,0,0,OA_SZ,":OUT_sNet"},
  {0,1,0,0,0,OA_SZ,":OUT_sLoc"}
};

#define  NUM_FIELDS 20

/* static variables */
static char Local_szdLat[20];
static char Local_szdLon[20];
static char Local_szdElev[20];
static char Local_szdAzm[20];
static char Local_szdDip[20];
static char Local_sztOn[20];
static char Local_sztOff[20];
static char Local_szINtOn[20];
static char Local_szINtOff[20];

static int  Local_iRetCode;

static EWDB_ChannelStruct Local_ChanStruct;

/* Statement Struct for GetChanParams szStatement */
static EWDB_OCIStatementStruct SSStatement;

/********************************
      FUNCTION PROTOTYPES
********************************/
static int PrepGetChanParamsExec(EWDB_ChannelStruct * IN_pChan, EWDB_Cursor * ppCursor);
static int PostGetChanParamsExec(EWDB_ChannelStruct * pChan);
static int InitGetChanParamsStatement(char *szStatement, 
                                 EWDB_OCIStatementStruct *pSS);


/* Describe Function ewdb_api_GetChanParams
*********************************************************************/
int ewdb_api_GetChanParams(EWDB_ChannelStruct * pChan)
{

  EWDB_Cursor pCursor;
  int rc;

  if(pChan == NULL)
  {
    logit("","ewdb_api_GetChanParams():Null pChan pointer passed in!\n");
    return(EWDB_RETURN_FAILURE);
  }

  ewdb_base_SetLastOraAPIActionTime();

  if(ewdb_base_Reconnect() != EWDB_RETURN_SUCCESS)
  {
    logit("","ewdb_api_GetChanParams(): Could not reconnect to the database!\n");
    return(EWDB_RETURN_FAILURE);
  }

  if(PrepGetChanParamsExec(pChan, &pCursor)
      != EWDB_RETURN_SUCCESS)
  {
    logit("","ewdb_api_GetChanParams():PrepGetChanParamsExec() failed.\n");
    return(EWDB_RETURN_FAILURE);
  }

  if(ewdb_base_SQLExecute(pCursor))
  {
    ewdb_base_ErrorReport(hEWDBC, pCursor,"ewdb_api_GetChanParams():ewdb_base_SQLExecute", 1);
    return(ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  } 

  /* Commit the transaction(all the previous inserts!)
  ****************************************************/
  if(ewdb_base_SQLCommit(hEWDBC))
  {
    ewdb_base_ErrorReport(hEWDBC, hEWDBC,"ewdb_api_GetChanParams():ewdb_base_SQLCommit",2);
    return(ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }

  rc = PostGetChanParamsExec(pChan);

  ewdb_base_SetLastOraAPIActionTime();

  if(rc == EWDB_RETURN_WARNING)
  {
    return(EWDB_RETURN_FAILURE);
  }
  else if(rc == EWDB_RETURN_FAILURE)
  {
    logit("", "ewdb_api_GetChanParams(): "
           "ERROR:  PostGetChanParamsExec failed!!\n");
    return(ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }

  return(EWDB_RETURN_SUCCESS);
}  /* end ewdb_api_GetChanParams() */


static int InitGetChanParamsStatement(char *szStatement,
EWDB_OCIStatementStruct *pSS)
{

  pSS->FieldArray[0].pVal = &Local_ChanStruct.idChanT;
  pSS->FieldArray[1].pVal = &Local_ChanStruct.Comp.idChan;
  pSS->FieldArray[2].pVal = Local_szINtOn;
  pSS->FieldArray[3].pVal = Local_szINtOff;
  pSS->FieldArray[4].pVal = &Local_iRetCode;
  pSS->FieldArray[5].pVal = &Local_ChanStruct.idChanT;
  pSS->FieldArray[6].pVal = &Local_ChanStruct.Comp.idChan;
  pSS->FieldArray[7].pVal = &Local_ChanStruct.Comp.idComp;
  pSS->FieldArray[8].pVal = &Local_ChanStruct.idCompT;
  pSS->FieldArray[9].pVal = Local_szdLat;
  pSS->FieldArray[10].pVal = Local_szdLon;
  pSS->FieldArray[11].pVal = Local_szdElev;
  pSS->FieldArray[12].pVal = Local_szdAzm;
  pSS->FieldArray[13].pVal = Local_szdDip;
  pSS->FieldArray[14].pVal = Local_sztOn;
  pSS->FieldArray[15].pVal = Local_sztOff;
  pSS->FieldArray[16].pVal = Local_ChanStruct.Comp.Sta;
  pSS->FieldArray[17].pVal = Local_ChanStruct.Comp.Comp;
  pSS->FieldArray[18].pVal = Local_ChanStruct.Comp.Net;
  pSS->FieldArray[19].pVal = Local_ChanStruct.Comp.Loc;

  return(ewdb_base_RequestCursor(szStatement, pSS, 0));
}  /* end InitGetChanParamsStatement() */


static int PrepGetChanParamsExec(EWDB_ChannelStruct * IN_pChan, EWDB_Cursor * ppCursor)
{

  SSStatement.NumOfFields = NUM_FIELDS;
  SSStatement.FieldArray = SQLParamsBindArray;
  SSStatement.RecordSize = 0;

  /* Initialize local struct and output strings */
  memset(&Local_ChanStruct, 0, sizeof(Local_ChanStruct));

  memset(Local_szdLat, 0, sizeof(Local_szdLat));
  memset(Local_szdLon, 0, sizeof(Local_szdLon));
  memset(Local_szdElev,0, sizeof(Local_szdElev));
  memset(Local_szdAzm, 0, sizeof(Local_szdAzm));
  memset(Local_szdDip, 0, sizeof(Local_szdDip));
  memset(Local_sztOn,  0, sizeof(Local_sztOn));
  memset(Local_sztOff, 0, sizeof(Local_sztOff));

  /* Copy user's vars to local vars here */
  Local_ChanStruct.tOn            = IN_pChan->tOn;
  Local_ChanStruct.tOff           = IN_pChan->tOff;
  Local_ChanStruct.idChanT        = IN_pChan->idChanT;
  Local_ChanStruct.Comp.idChan    = IN_pChan->Comp.idChan;
  Local_ChanStruct.pChanProps     = IN_pChan->pChanProps;

  sprintf(Local_szINtOn, "%.2f", Local_ChanStruct.tOn);
  sprintf(Local_szINtOff, "%.2f", Local_ChanStruct.tOff);

  if(InitGetChanParamsStatement(SQL_STRING,
                                &SSStatement) != EWDB_RETURN_SUCCESS)
  { 
    logit("", "PrepGetChanParamsExec(): InitGetChanParamsStatement failed!\n");
    return(EWDB_RETURN_FAILURE);
  }

  *ppCursor = SSStatement.pCda;
  
  return(EWDB_RETURN_SUCCESS);
}  /* end PrepGetChanParamsExec() */


static int PostGetChanParamsExec(EWDB_ChannelStruct * OUT_pChan)
{
  EWDB_Cursor pCursor;

  Local_ChanStruct.tOn  = atof(Local_sztOn);
  Local_ChanStruct.tOff = atof(Local_sztOff);
  Local_ChanStruct.Comp.Lat  = (float) atof(Local_szdLat);
  Local_ChanStruct.Comp.Lon  = (float) atof(Local_szdLon);
  Local_ChanStruct.Comp.Elev = (float) atof(Local_szdElev);
  Local_ChanStruct.Comp.Azm  = (float) atof(Local_szdAzm);
  Local_ChanStruct.Comp.Dip  = (float) atof(Local_szdDip);

  memcpy(OUT_pChan, &Local_ChanStruct, sizeof(Local_ChanStruct));

  pCursor = SSStatement.pCda;
  
  ewdb_base_ReleaseCursor(pCursor);

  if(Local_iRetCode != 0)
  {
      logit("", "ERROR:  PostGetChanParamsExec() reports SQL Proc Get_Chan_Params(%d/%d) returned error(%d)!\n",
            Local_ChanStruct.Comp.idChan, Local_ChanStruct.idChanT, Local_iRetCode);
    return(EWDB_RETURN_WARNING);
  }

  return(EWDB_RETURN_SUCCESS);
}  /* end PostGetChanParamsExec() */


