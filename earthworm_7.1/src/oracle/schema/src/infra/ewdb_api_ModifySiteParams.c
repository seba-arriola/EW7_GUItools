/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_api_ModifySiteParams.c,v 1.3 2005/06/10 16:27:59 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_api_ModifySiteParams.c,v $
 *     Revision 1.3  2005/06/10 16:27:59  davidk
 *     DB API Cleanup
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
     "  Begin Modify_Site_Params(OUT_iRetCode => :OUT_iRetCode, "
           "IN_idSiteT => :IN_idSiteT, "
           "IN_dLat => :IN_dLat, "
           "IN_dLon => :IN_dLon, "
           "IN_dElev => :IN_dElev, "
           "IN_sComment => :IN_sComment); End;";

static EWDB_OCI_SFS SQLParamsBindArray[] = 
{
  {0,1,0,0,0,OA_INT,":OUT_iRetCode"},
  {0,1,0,0,0,OA_EWDBID,":IN_idSiteT"},
  {0,1,0,0,0,OA_DOUBLE,":IN_dLat"},
  {0,1,0,0,0,OA_DOUBLE,":IN_dLon"},
  {0,1,0,0,0,OA_DOUBLE,":IN_dElev"},
  {0,1,0,0,0,OA_SZ,":IN_sComment"}
};

#define  NUM_FIELDS 6

/* static variables */
static char   Local_szdLat[20];
static char   Local_szdLon[20];
static char   Local_szdElev[20];
static char   Local_szComment[1024];
static EWDBid Local_idSiteT;
static int    Local_iRetCode;

/* Statement Struct for ModifySiteParams szStatement */
static EWDB_OCIStatementStruct SSStatement;

/********************************
      FUNCTION PROTOTYPES
********************************/
static int PrepModifySiteParamsExec(EWDB_ChannelStruct * IN_pChan, char * IN_szComment,
                        EWDB_Cursor * ppCursor);
static int PostModifySiteParamsExec(int * pRetCode);
static int InitModifySiteParamsStatement(char *szStatement, 
                             EWDB_OCIStatementStruct *pSS);


/* Describe Function ewdb_api_ModifySiteParams
*********************************************************************/
int ewdb_api_ModifySiteParams(int * OUT_pRetCode, EWDB_ChannelStruct * pChan, 
                              char * Local_szComment)
{

  EWDB_Cursor pCursor;
  int rc;

  if(pChan == NULL)
  {
    logit("","ewdb_api_ModifySiteParams():Null pChan pointer passed in!\n");
    return(EWDB_RETURN_FAILURE);
  }

  ewdb_base_SetLastOraAPIActionTime();

  if(ewdb_base_Reconnect() != EWDB_RETURN_SUCCESS)
  {
    logit("","ewdb_api_ModifySiteParams(): Could not reconnect to the database!\n");
    return(EWDB_RETURN_FAILURE);
  }

  if(PrepModifySiteParamsExec(pChan, Local_szComment, &pCursor)
      != EWDB_RETURN_SUCCESS)
  {
    logit("","ewdb_api_ModifySiteParams():PrepModifySiteParamsExec() failed.\n");
    return(EWDB_RETURN_FAILURE);
  }

  if(ewdb_base_SQLExecute(pCursor))
  {
    ewdb_base_ErrorReport(hEWDBC, pCursor,"ewdb_api_ModifySiteParams():ewdb_base_SQLExecute", 1);
    return(ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  } 

  /* Commit the transaction(all the previous inserts!)
  ****************************************************/
  if(ewdb_base_SQLCommit(hEWDBC))
  {
    ewdb_base_ErrorReport(hEWDBC, hEWDBC,"ewdb_api_ModifySiteParams():ewdb_base_SQLCommit",2);
    return(ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }

  rc = PostModifySiteParamsExec(OUT_pRetCode);

  ewdb_base_SetLastOraAPIActionTime();

  if(rc == EWDB_RETURN_WARNING)
  {
    return(EWDB_RETURN_FAILURE);
  }
  else if(rc == EWDB_RETURN_FAILURE)
  {
    logit("", "ewdb_api_ModifySiteParams(): "
           "ERROR:  PostModifySiteParamsExec failed!!\n");
    return(ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }

  return(EWDB_RETURN_SUCCESS);
}  /* end ewdb_api_ModifySiteParams() */


static int InitModifySiteParamsStatement(char *szStatement,
                                         EWDB_OCIStatementStruct *pSS)
{

  pSS->FieldArray[0].pVal = &Local_iRetCode;
  pSS->FieldArray[1].pVal = &Local_idSiteT;
  pSS->FieldArray[2].pVal = Local_szdLat;
  pSS->FieldArray[3].pVal = Local_szdLon;
  pSS->FieldArray[4].pVal = Local_szdElev;
  pSS->FieldArray[5].pVal = Local_szComment;

  return(ewdb_base_RequestCursor(szStatement, pSS, 0));
}  /* end InitModifySiteParamsStatement() */


static int PrepModifySiteParamsExec(EWDB_ChannelStruct * IN_pChan, char * IN_szComment,
                        EWDB_Cursor * ppCursor)
{

  SSStatement.NumOfFields = NUM_FIELDS;
  SSStatement.FieldArray = SQLParamsBindArray;
  SSStatement.RecordSize = 0;

  /* Copy user's vars to local vars here */
  sprintf(Local_szdLat, "%.4f", IN_pChan->Comp.Lat);
  sprintf(Local_szdLon, "%.4f", IN_pChan->Comp.Lon);
  sprintf(Local_szdElev, "%.2f", IN_pChan->Comp.Elev);
  Local_idSiteT = IN_pChan->idSiteT;

  if(IN_szComment)
  {
    memcpy(Local_szComment,IN_szComment,sizeof(Local_szComment));
    Local_szComment[sizeof(Local_szComment)-1] = 0x00;
  }
  else
    Local_szComment[0]=0x00;

  if(InitModifySiteParamsStatement(SQL_STRING,
                              &SSStatement) != EWDB_RETURN_SUCCESS)
  {
    logit("", "Call to InitModifySiteParamsStatement failed!\n");
    return(EWDB_RETURN_FAILURE);
  }

  *ppCursor = SSStatement.pCda;
  
  return(EWDB_RETURN_SUCCESS);

}  /* end PrepModifySiteParamsExec() */


static int PostModifySiteParamsExec(int * pRetCode)
{
  EWDB_Cursor pCursor;
  
  *pRetCode = Local_iRetCode;
  
  pCursor = SSStatement.pCda;
  
  ewdb_base_ReleaseCursor(pCursor);

  if(Local_iRetCode < 0)
  {
    logit("", "ERROR:  PostModifySiteParamsExec() reports SQL Proc "
              "Modify_Site_Params(%d) returned error(%d)!\n",
          Local_idSiteT, *pRetCode);
    return(EWDB_RETURN_WARNING);
  }

  return(EWDB_RETURN_SUCCESS);
}  /* end PostModifySiteParamsExec() */


