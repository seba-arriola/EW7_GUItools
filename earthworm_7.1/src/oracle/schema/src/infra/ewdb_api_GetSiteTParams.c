/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_api_GetSiteTParams.c,v 1.4 2005/06/10 16:27:59 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_api_GetSiteTParams.c,v $
 *     Revision 1.4  2005/06/10 16:27:59  davidk
 *     DB API Cleanup
 *
 *     Revision 1.3  2003/12/03 01:12:49  davidk
 *     Fixed bug in variable declarations.  Variables that were supposed to
 *     be static(file scope) were not.  Made 'em so they were.
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
     "  Begin Get_SiteT_Params(OUT_iRetCode => :OUT_iRetCode, "
           "IN_idSiteT => :IN_idSiteT, "
           "IN_idSite => :IN_idSite, "
           "IN_tOn => :IN_tOn, "
           "IN_tOff => :IN_tOff, "
           "OUT_idSiteT => :OUT_idSiteT, "
           "OUT_idSite => :OUT_idSite, "
           "OUT_dLat => :OUT_dLat, "
           "OUT_dLon => :OUT_dLon, "
           "OUT_dElev => :OUT_dElev, "
           "OUT_tOn => :OUT_tOn, "
           "OUT_tOff => :OUT_tOff, "
           "OUT_sSta => :OUT_sSta, "
           "OUT_sNet => :OUT_sNet, "
           "OUT_sComment => :OUT_sComment); End;";

static EWDB_OCI_SFS SQLParamsBindArray[] = 
{
  {0,1,0,0,0,OA_INT,":OUT_iRetCode"},
  {0,1,0,0,0,OA_INT,":IN_idSiteT"},
  {0,1,0,0,0,OA_INT,":IN_idSite"},
  {0,1,0,0,0,OA_DOUBLE,":IN_tOn"},
  {0,1,0,0,0,OA_DOUBLE,":IN_toff"},
  {0,1,0,0,0,OA_INT,":OUT_idSiteT"},
  {0,1,0,0,0,OA_INT,":OUT_idSite"},
  {0,1,20,0,0,OA_DOUBLE,":OUT_dLat"},
  {0,1,20,0,0,OA_DOUBLE,":OUT_dLon"},
  {0,1,20,0,0,OA_DOUBLE,":OUT_dElev"},
  {0,1,20,0,0,OA_DOUBLE,":OUT_tOn"},
  {0,1,20,0,0,OA_DOUBLE,":OUT_tOff"},
  {0,1,20,0,0,OA_SZ,":OUT_sSta"},
  {0,1,20,0,0,OA_SZ,":OUT_sNet"},
  {0,1,1024,0,0,OA_SZ,":OUT_sComment"}
};

#define  NUM_FIELDS 15

/* static variables */
static char Local_szdLat[20];
static char Local_szdLon[20];
static char Local_szdElev[20];
static char Local_sztOn[20];
static char Local_sztOff[20];
static char Local_szINtOn[20];
static char Local_szINtOff[20];
static int  Local_iRetCode;
static EWDB_ChannelStruct Local_Chan;
static char Local_szComment[1024];

/* Statement Struct for GetSiteTParams szStatement */
static EWDB_OCIStatementStruct SSStatement;

/********************************
      FUNCTION PROTOTYPES
********************************/
static int PrepGetSiteTParamsExec(EWDB_ChannelStruct * IN_pChan,
                           EWDB_Cursor * ppCursor);
static int PostGetSiteTParamsExec(EWDB_ChannelStruct * pChan);
static int InitGetSiteTParamsStatement(char *szStatement,
                                EWDB_OCIStatementStruct *pSS);


/* Describe Function ewdb_api_GetSiteTParams
*********************************************************************/
int ewdb_api_GetSiteTParams(EWDB_ChannelStruct * pChan)
{

  EWDB_Cursor pCursor;
  int rc;

  if(pChan == NULL)
  {
    logit("","ewdb_api_GetSiteTParams():Null pointer passed in!\n");
    return(EWDB_RETURN_FAILURE);
  }

  ewdb_base_SetLastOraAPIActionTime();

  if(ewdb_base_Reconnect() != EWDB_RETURN_SUCCESS)
  {
    logit("","ewdb_api_GetSiteTParams(): Could not reconnect to the database!\n");
    return(EWDB_RETURN_FAILURE);
  }

  if(PrepGetSiteTParamsExec(pChan, &pCursor)
      != EWDB_RETURN_SUCCESS)
  {
    logit("","ewdb_api_GetSiteTParams():PrepGetSiteTParamsExec() failed.\n");
    return(EWDB_RETURN_FAILURE);
  }

  if(ewdb_base_SQLExecute(pCursor))
  {
    ewdb_base_ErrorReport(hEWDBC, pCursor,"ewdb_api_GetSiteTParams():ewdb_base_SQLExecute", 1);
    return(ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  } 

  /* Commit the transaction(all the previous inserts!)
  ****************************************************/
  if(ewdb_base_SQLCommit(hEWDBC))
  {
    ewdb_base_ErrorReport(hEWDBC, hEWDBC,"ewdb_api_GetSiteTParams():ewdb_base_SQLCommit",2);
    return(ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }

  rc = PostGetSiteTParamsExec(pChan);

  ewdb_base_SetLastOraAPIActionTime();

  if(rc == EWDB_RETURN_WARNING)
  {
    return(EWDB_RETURN_FAILURE);
  }
  else if(rc == EWDB_RETURN_FAILURE)
  {
    return(ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }

  return(EWDB_RETURN_SUCCESS);
}  /* end ewdb_api_GetSiteTParams() */


static int InitGetSiteTParamsStatement(char *szStatement, EWDB_OCIStatementStruct *pSS)
{

  pSS->FieldArray[0].pVal = &Local_iRetCode;
  pSS->FieldArray[1].pVal = &Local_Chan.idSiteT;
  pSS->FieldArray[2].pVal = &Local_Chan.idSite;
  pSS->FieldArray[3].pVal = Local_szINtOn;
  pSS->FieldArray[4].pVal = Local_szINtOff;
  pSS->FieldArray[5].pVal = &Local_Chan.idSiteT;
  pSS->FieldArray[6].pVal = &Local_Chan.idSite;
  pSS->FieldArray[7].pVal = Local_szdLat;
  pSS->FieldArray[8].pVal = Local_szdLon;
  pSS->FieldArray[9].pVal = Local_szdElev;
  pSS->FieldArray[10].pVal = Local_sztOn;
  pSS->FieldArray[11].pVal = Local_sztOff;
  pSS->FieldArray[12].pVal = Local_Chan.Comp.Sta;
  pSS->FieldArray[13].pVal = Local_Chan.Comp.Net;
  pSS->FieldArray[14].pVal = Local_szComment;

  return(ewdb_base_RequestCursor(szStatement, pSS, 0));
}  /* end InitGetSiteTParamsStatement() */


static int PrepGetSiteTParamsExec(EWDB_ChannelStruct * IN_pChan, EWDB_Cursor * ppCursor)
{

  SSStatement.NumOfFields = NUM_FIELDS;
  SSStatement.FieldArray = SQLParamsBindArray;
  SSStatement.RecordSize = 0;

  /* initialize all output buffers */
  memset(&Local_Chan, 0, sizeof(Local_Chan));
  memset(Local_szdLat, 0, sizeof(Local_szdLat));
  memset(Local_szdLon, 0, sizeof(Local_szdLon));
  memset(Local_szdElev, 0, sizeof(Local_szdElev));
  memset(Local_sztOn, 0, sizeof(Local_sztOn));
  memset(Local_sztOff, 0, sizeof(Local_sztOff));

  /* Copy user's vars to local vars here */
  Local_Chan.idSiteT = IN_pChan->idSiteT;
  Local_Chan.idSite = IN_pChan->idSite;
  sprintf(Local_szINtOn,"%.2f",IN_pChan->tOn);
  sprintf(Local_szINtOff,"%.2f",IN_pChan->tOff);
 
  if(InitGetSiteTParamsStatement(SQL_STRING,
                           &SSStatement) != EWDB_RETURN_SUCCESS)
  {
    logit("", "PrepGetSiteTParamsExec(): InitGetSiteTParamsStatement failed!\n");
    return(EWDB_RETURN_FAILURE);
  }

  *ppCursor = SSStatement.pCda;
  
  return(EWDB_RETURN_SUCCESS);

}  /* end PrepGetSiteTParamsExec() */


static int PostGetSiteTParamsExec(EWDB_ChannelStruct * pChan)
{
  EWDB_Cursor pCursor;
  int rc;
  
  /* Copy user's vars to local vars here */
  if(Local_iRetCode = 0)
  {
    pChan->idSiteT = Local_Chan.idSiteT;
    pChan->idSite =  Local_Chan.idSite;
    pChan->tOn = atof(Local_sztOn);
    pChan->tOff = atof(Local_sztOff);
    pChan->Comp.Lat =  (float)atof(Local_szdLat);
    pChan->Comp.Lon = (float)atof(Local_szdLon);
    pChan->Comp.Elev = (float)atof(Local_szdElev);
    memcpy(pChan->Comp.Sta,Local_Chan.Comp.Sta, sizeof(Local_Chan.Comp.Sta));
    memcpy(pChan->Comp.Net,Local_Chan.Comp.Net, sizeof(Local_Chan.Comp.Net));

    rc = EWDB_RETURN_SUCCESS;
  }
  else
  {
    logit("", "PostGetSiteTParamsExec() reports SQL Proc Get_SiteT_Params(%d/%d) returned error(%d)!\n",
          Local_Chan.idSite, Local_Chan.idSiteT, Local_iRetCode);
    rc=EWDB_RETURN_WARNING;
  }
  
  pCursor = SSStatement.pCda;
  
  ewdb_base_ReleaseCursor(pCursor);

  return(rc);
}  /* end PostGetSiteTParamsExec() */


