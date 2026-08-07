/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_api_SplitSiteT.c,v 1.3 2005/06/10 16:27:59 davidk Exp $
 *
 *     $Log: ewdb_api_SplitSiteT.c,v $
 *     Revision 1.3  2005/06/10 16:27:59  davidk
 *     DB API Cleanup
 *
 *     Revision 1.2  2003/12/04 19:33:34  davidk
 *     Improved handling of SQL warnings and errors, passing more information
 *     back to the caller.
 *
 *     Revision 1.1  2003/12/03 00:23:29  davidk
 *     Initial revision
 *
 */


#include <ewdb_ora_api.h>
#include <ewdb_cli_base.h>
#include <ewdb_ew_oci_base.h>


static char SQL_STRING[] = 
     "  Begin Split_SiteT(OUT_iRetCode => :OUT_iRetCode, "
           "OUT_idSiteT => :OUT_idSiteT, "
           "IN_idSiteT => :IN_idSiteT, "
           "IN_tSplit => :IN_tSplit); End;";

static EWDB_OCI_SFS SQLParamsBindArray[] = 
{
  {0,1,0,0,0,OA_INT,":OUT_iRetCode"},
  {0,1,0,0,0,OA_EWDBID,":OUT_idSiteT"},
  {0,1,0,0,0,OA_EWDBID,":IN_idSiteT"},
  {0,1,0,0,0,OA_DOUBLE,":IN_tSplit"}
};

#define  NUM_FIELDS 4

/* static variables */
static char Local_sztSplit[20];
static int  Local_iRetCode;
static EWDBid Local_idSiteT_Orig, Local_idSiteT_New;

/* Statement Struct for SplitSiteT statement */
static EWDB_OCIStatementStruct SSStatement;

/********************************
      FUNCTION PROTOTYPES
********************************/
static int PrepSplitSiteTExec(EWDB_ChannelStruct * IN_pChan,
                           EWDB_Cursor * ppCursor);
static int PostSplitSiteTExec(EWDBid * pid);
static int InitSplitSiteTStatement(char *statement, 
                                   EWDB_OCIStatementStruct *pSS);


/* use pChan->tOn for the split value
*********************************************************************/
int ewdb_api_SplitSiteT(EWDB_ChannelStruct * pChan, EWDBid * pidNewSiteT)
{

  EWDB_Cursor pCursor;
  int rc;

  if(pChan == NULL)
  {
    logit("","ewdb_api_SplitSiteT():Null pointer passed in!\n");
    return(EWDB_RETURN_FAILURE);
  }

  ewdb_base_SetLastOraAPIActionTime();

  if(ewdb_base_Reconnect() != EWDB_RETURN_SUCCESS)
  {
    logit("","ewdb_api_SplitSiteT(): Could not reconnect to the database!\n");
    return(EWDB_RETURN_FAILURE);
  }

  if(PrepSplitSiteTExec(pChan, &pCursor)
      != EWDB_RETURN_SUCCESS)
  {
    logit("","ewdb_api_SplitSiteT():PrepSplitSiteTExec() failed.\n");
    return(EWDB_RETURN_FAILURE);
  }

  if(ewdb_base_SQLExecute(pCursor))
  {
    ewdb_base_ErrorReport(hEWDBC, pCursor,"ewdb_api_SplitSiteT():ewdb_base_SQLExecute", 1);
    return(ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  } 

  /* Commit the transaction(all the previous inserts!)
  ****************************************************/
  if(ewdb_base_SQLCommit(hEWDBC))
  {
    ewdb_base_ErrorReport(hEWDBC, hEWDBC,"ewdb_api_SplitSiteT():ewdb_base_SQLCommit",2);
    return(ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }

  rc = PostSplitSiteTExec(pidNewSiteT);

  ewdb_base_SetLastOraAPIActionTime();

  if(rc == EWDB_RETURN_WARNING)
  {
    return(EWDB_RETURN_FAILURE);
  }
  else if(rc == EWDB_RETURN_FAILURE)
  {
    logit("", "ewdb_api_SplitSiteT(): "
           "ERROR:  PostSplitSiteTExec failed!!\n");
    return(ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }

  return(EWDB_RETURN_SUCCESS);
}  /* end ewdb_api_SplitSiteT() */


static int InitSplitSiteTStatement(char *statement,
EWDB_OCIStatementStruct *pSS)
{

  pSS->FieldArray[0].pVal = &Local_iRetCode;
  pSS->FieldArray[1].pVal = &Local_idSiteT_New;
  pSS->FieldArray[2].pVal = &Local_idSiteT_Orig;
  pSS->FieldArray[3].pVal = Local_sztSplit;

  return(ewdb_base_RequestCursor(statement, pSS, 0));
}  /* end InitSplitSiteTStatement() */


static int PrepSplitSiteTExec(EWDB_ChannelStruct * IN_pChan,
                              EWDB_Cursor * ppCursor)
{

  SSStatement.NumOfFields = NUM_FIELDS;
  SSStatement.FieldArray = SQLParamsBindArray;
  SSStatement.RecordSize = 0;

  /* initialize output buffers */
  Local_iRetCode = 0;
  Local_idSiteT_New = 0;

  /* copy user values to local vars */
  sprintf(Local_sztSplit, "%.0f", IN_pChan->tOn);
  Local_idSiteT_Orig = IN_pChan->idSiteT;

  if(InitSplitSiteTStatement(SQL_STRING,
                             &SSStatement) != EWDB_RETURN_SUCCESS)
  {
    logit("", "PrepSplitSiteTExec(): InitSplitSiteTStatement failed!\n");
    return(EWDB_RETURN_FAILURE);
  }

  *ppCursor = SSStatement.pCda;
  
  return(EWDB_RETURN_SUCCESS);
}  /* end PrepSplitSiteTExec() */


static int PostSplitSiteTExec(EWDBid * pid)
{
  EWDB_Cursor pCursor;
  
  *pid=Local_idSiteT_New;
  
  pCursor = SSStatement.pCda;
  
  ewdb_base_ReleaseCursor(pCursor);

  if(Local_iRetCode < 0)
  {
   logit("","PostSplitSiteTExec():  ERROR SQL Proc Split_SiteT(%d/%s) returned error %d!\n",
         Local_idSiteT_Orig, Local_sztSplit, Local_iRetCode);
    *pid = Local_iRetCode;
    return(EWDB_RETURN_WARNING);
  }

  return(EWDB_RETURN_SUCCESS);
}  /* end PostSplitSiteTExec() */


