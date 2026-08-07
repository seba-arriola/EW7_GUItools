/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_api_UpdateSiteT.c,v 1.3 2005/06/10 16:27:59 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_api_UpdateSiteT.c,v $
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
     "  Begin Update_SiteT(OUT_iRetCode => :OUT_iRetCode, "
           "IN_idSiteT => :IN_idSiteT, "
           "IN_tOn => :IN_tOn, "
           "IN_tOff => :IN_tOff, "
           "IN_sComment => :IN_sComment); End;";

static EWDB_OCI_SFS SQLParamsBindArray[] = 
{
  {0,1,0,0,0,OA_INT,":OUT_iRetCode"},
  {0,1,0,0,0,OA_EWDBID,":IN_idSiteT"},
  {0,1,0,0,0,OA_DOUBLE,":IN_tOn"},
  {0,1,0,0,0,OA_DOUBLE,":IN_tOff"},
  {0,1,0,0,0,OA_SZ,":IN_sComment"}
};

#define  NUM_FIELDS 5

/* static variables */
static char   Local_sztOn[20];
static char   Local_sztOff[20];
static char   Local_szComment[1024];
static EWDBid Local_idSiteT;
static int    Local_iRetCode;

/* Statement Struct for UpdateSiteT statement */
static EWDB_OCIStatementStruct SSStatement;

/********************************
      FUNCTION PROTOTYPES
********************************/
static int PrepUpdateSiteTExec(EWDB_ChannelStruct * IN_pChan, char * IN_szComment,
                               EWDB_Cursor * ppCursor);
static int PostUpdateSiteTExec(int * pRetCode);
static int InitUpdateSiteTStatement(char *statement, 
                                    EWDB_OCIStatementStruct *pSS);


/* Describe Function ewdb_api_UpdateSiteT
*********************************************************************/
int ewdb_api_UpdateSiteT(int * OUT_pRetCode, EWDB_ChannelStruct * pChan, 
                         char * Local_szComment)
{

  EWDB_Cursor pCursor;
  int rc;

  if(pChan == NULL)
  {
    logit("","ewdb_api_UpdateSiteT(): Null pChan pointer passed in!\n");
    return(EWDB_RETURN_FAILURE);
  }

  ewdb_base_SetLastOraAPIActionTime();

  if(ewdb_base_Reconnect() != EWDB_RETURN_SUCCESS)
  {
    logit("","ewdb_api_UpdateSiteT(): Could not reconnect to the database!\n");
    return(EWDB_RETURN_FAILURE);
  }

  if(PrepUpdateSiteTExec(pChan, Local_szComment, &pCursor)
      != EWDB_RETURN_SUCCESS)
  {
    logit("","ewdb_api_UpdateSiteT():PrepUpdateSiteTExec() failed.\n");
    return(EWDB_RETURN_FAILURE);
  }

  if(ewdb_base_SQLExecute(pCursor))
  {
    ewdb_base_ErrorReport(hEWDBC, pCursor,"ewdb_api_UpdateSiteT():ewdb_base_SQLExecute", 1);
    return(ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  } 

  /* Commit the transaction(all the previous inserts!)
  ****************************************************/
  if(ewdb_base_SQLCommit(hEWDBC))
  {
    ewdb_base_ErrorReport(hEWDBC, hEWDBC,"ewdb_api_UpdateSiteT():ewdb_base_SQLCommit",2);
    return(ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }

  rc = PostUpdateSiteTExec(OUT_pRetCode);

  ewdb_base_SetLastOraAPIActionTime();

  if(rc == EWDB_RETURN_WARNING)
  {
    return(EWDB_RETURN_FAILURE);
  }
  else if(rc == EWDB_RETURN_FAILURE)
  {
    logit("", "ewdb_api_UpdateSiteT(): "
           "ERROR:  PostUpdateSiteTExec failed!!\n");
    return(ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }

  return(EWDB_RETURN_SUCCESS);
}  /* end ewdb_api_UpdateSiteT() */


static int InitUpdateSiteTStatement(char *statement,
EWDB_OCIStatementStruct *pSS)
{

  pSS->FieldArray[0].pVal = &Local_iRetCode;
  pSS->FieldArray[1].pVal = &Local_idSiteT;
  pSS->FieldArray[2].pVal = Local_sztOn;
  pSS->FieldArray[3].pVal = Local_sztOff;
  pSS->FieldArray[4].pVal = Local_szComment;

  return(ewdb_base_RequestCursor(statement, pSS, 0));
}  /* end InitUpdateSiteTStatement() */


static int PrepUpdateSiteTExec(EWDB_ChannelStruct * IN_pChan, char * IN_szComment,
                               EWDB_Cursor * ppCursor)
{

  SSStatement.NumOfFields = NUM_FIELDS;
  SSStatement.FieldArray = SQLParamsBindArray;
  SSStatement.RecordSize = 0;

  /* Copy user's vars to local vars here */
  sprintf(Local_sztOn, "%.2f", IN_pChan->tOn);
  sprintf(Local_sztOff, "%.2f", IN_pChan->tOff);

  Local_idSiteT = IN_pChan->idSiteT;

  if(IN_szComment)
  {
    memcpy(Local_szComment,IN_szComment,sizeof(Local_szComment));
    Local_szComment[sizeof(Local_szComment)-1] = 0x00;
  }
  else
    Local_szComment[0]=0x00;

  if(InitUpdateSiteTStatement(SQL_STRING,
                              &SSStatement) != EWDB_RETURN_SUCCESS)
  {
    logit("", "PrepUpdateSiteTExec(): InitUpdateSiteTStatement failed!\n");
    return(EWDB_RETURN_FAILURE);
  }

  *ppCursor = SSStatement.pCda;
  
  return(EWDB_RETURN_SUCCESS);

}  /* end PrepUpdateSiteTExec() */


static int PostUpdateSiteTExec(int * pRetCode)
{
  EWDB_Cursor pCursor;
  
  *pRetCode = Local_iRetCode;
  
  pCursor = SSStatement.pCda;
  
  ewdb_base_ReleaseCursor(pCursor);

  if(Local_iRetCode < 0)
  {
    logit("", "PostUpdateSiteTExec() reports SQL Proc Update_SiteT(%d) returned error(%d)!\n",
          Local_idSiteT, *pRetCode);
    return(EWDB_RETURN_WARNING);
  }

  return(EWDB_RETURN_SUCCESS);
}  /* end PostUpdateSiteTExec() */


