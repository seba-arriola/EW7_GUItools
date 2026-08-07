
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_api_CreateSite.c,v 1.3 2005/06/10 16:27:58 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_api_CreateSite.c,v $
 *     Revision 1.3  2005/06/10 16:27:58  davidk
 *     DB API Cleanup
 *
 *     Revision 1.2  2003/12/04 19:33:34  davidk
 *     Improved handling of SQL warnings and errors, passing more information
 *     back to the caller.
 *
 *     Revision 1.1  2003/12/03 00:19:45  davidk
 *     Initial revision
 *
 *
 */


#include <ewdb_ora_api.h>
#include <ewdb_cli_base.h>
#include <ewdb_ew_oci_base.h>

static char SQL_STRING[] =
  "Begin Create_Site(OUT_iRetCode => :OUT_iRetCode, "
  "                  OUT_idSite => :OUT_idSite," 
  "                  IN_sSta => :IN_sSta, IN_sNet => :IN_sNet,"
  "                  IN_sComment => :IN_sComment); End;";

static EWDB_OCI_SFS SQLParamsBindArray[] = 
{
  {0,1,0,0,0,OA_INT,":OUT_iRetCode"},
  {0,1,0,0,0,OA_EWDBID,":OUT_idSite"},
  {0,1,0,0,0,OA_SZ,":IN_sSta"},
  {0,1,0,0,0,OA_SZ,":IN_sNet"},
  {0,1,0,0,0,OA_SZ,":IN_sComment"}
};

#define  NUM_FIELDS 5

static EWDBid Local_idSite;
static char Local_szSta[10],Local_szNet[10],Local_szComment[4100];
static int Local_iRetCode;

/* Statement Struct for CREATE_COMP_FROM_SCNL statement */
static EWDB_OCIStatementStruct SSStatement;

/********************************
      FUNCTION PROTOTYPES
********************************/
static int PrepCreateSiteExec(char * IN_szSta, char * IN_szNet, 
                       char * IN_szComment, EWDB_Cursor * ppCursor);
static int PostCreateSiteExec(EWDBid * pidSite);
static int InitCreateSiteStatement(char *statement, 
                            EWDB_OCIStatementStruct *pSS);


/* Creates a component sensor record, and associates it with a Site.
   Site is(Sta,Net), and Component is Comp and optionally Loc.  
   Room for a comment is included.
*********************************************************************/
int ewdb_api_CreateSite(EWDB_ChannelStruct * pChan, char * IN_szComment)
{

  EWDB_Cursor pCursor;
  int rc;

  if(pChan == NULL || IN_szComment == NULL)
  {
    logit("", "ewdb_api_CreateSite(): Null pointer passed in!\n");
    return EWDB_RETURN_FAILURE;
  }

  if(pChan->Comp.Sta[0] == 0x00 || pChan->Comp.Net[0] == 0x00)
  {
    logit("", "ewdb_api_CreateSite(): Invalid Sta/Net (%s/%s)!\n",
          pChan->Comp.Sta, pChan->Comp.Net);
    return EWDB_RETURN_FAILURE;
  }

  ewdb_base_SetLastOraAPIActionTime();


  if(ewdb_base_Reconnect() != EWDB_RETURN_SUCCESS)
  {
    logit("", "ewdb_api_CreateSite():  Could not reconnect to the database!\n");
    return(EWDB_RETURN_FAILURE);
  }

  if(PrepCreateSiteExec(pChan->Comp.Sta, pChan->Comp.Net, IN_szComment, &pCursor) 
      != EWDB_RETURN_SUCCESS)
  {
    logit("", "ewdb_api_CreateSite(): PrepCreateSiteExec() failed.\n");
    return(ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }

  if(ewdb_base_SQLExecute(pCursor))
  {
    ewdb_base_ErrorReport(hEWDBC, pCursor,"ewdb_api_CreateSite(): ewdb_base_SQLExecute", 1);
    return(ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  } 

  
  /* Commit the transaction(all the previous inserts!)
     In Case there is any auditing or logging or debug
     changes made in the stored procedures.
  ****************************************************/
  if(ewdb_base_SQLCommit(hEWDBC))
  {
    ewdb_base_ErrorReport(hEWDBC, hEWDBC,"ewdb_api_CreateSite(): ewdb_base_SQLCommit",2);
    return(ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }

  rc = PostCreateSiteExec(&pChan->idSite);

  ewdb_base_SetLastOraAPIActionTime();

  if(rc == EWDB_RETURN_WARNING)
  {
    return(EWDB_RETURN_WARNING);
  }
  else if(rc == EWDB_RETURN_FAILURE)
  {
    logit("", "EWDB_CreateSite(): "
           "ERROR:  PostCreateSiteExec reports SQL Proc %s returned "
           "error(%d)!\n",
          "Create_Site()", Local_iRetCode);
    return(ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }

  return(EWDB_RETURN_SUCCESS);
}  /* end ewdb_api_CreateSite() */


static int InitCreateSiteStatement(char *statement, 
                                 EWDB_OCIStatementStruct *pSS)
{

  pSS->FieldArray[0].pVal = &Local_iRetCode;
  pSS->FieldArray[1].pVal = &Local_idSite;
  pSS->FieldArray[2].pVal = Local_szSta;
  pSS->FieldArray[3].pVal = Local_szNet;
  pSS->FieldArray[4].pVal = Local_szComment;

  return(ewdb_base_RequestCursor(statement, pSS, 0));
}  /* end InitCreateSiteStatement() */


static int PrepCreateSiteExec(char * IN_szSta, char * IN_szNet, 
                       char * IN_szComment, EWDB_Cursor * ppCursor)
{

  SSStatement.NumOfFields = NUM_FIELDS;
  SSStatement.FieldArray = SQLParamsBindArray;
  SSStatement.RecordSize = 0;

  strcpy(Local_szSta,IN_szSta);
  strcpy(Local_szNet,IN_szNet);

  strncpy(Local_szComment,IN_szComment,sizeof(Local_szComment));
  Local_szComment[sizeof(Local_szComment)-1]=0;

  if(InitCreateSiteStatement(SQL_STRING,
                             &SSStatement) != EWDB_RETURN_SUCCESS)
  {
    logit("", "PrepCreateSiteExec(): InitCreateSiteStatement failed!\n");
    return EWDB_RETURN_FAILURE;
  }

  *ppCursor = SSStatement.pCda;
  
  return(EWDB_RETURN_SUCCESS);

}  /* end PrepCreateSiteExec() */


static int PostCreateSiteExec(EWDBid * pidSite)
{
  EWDB_Cursor pCursor;
  
  *pidSite = Local_idSite;
  
  pCursor = SSStatement.pCda;
  
  ewdb_base_ReleaseCursor(pCursor);

  if(Local_iRetCode == 0)
  {
    return(EWDB_RETURN_SUCCESS);
  }
  else if(Local_iRetCode > 0)
  {
    logit("", "WARNING:  PostCreateSiteExec reports SQL Proc %s returned "
           "warning(%d)!\n",
          "Create_Site()", Local_iRetCode);
    return(EWDB_RETURN_WARNING);
  }
  else
  {
    *pidSite = Local_iRetCode;
    return(EWDB_RETURN_FAILURE);
  }

}  /* end PostCreateSiteExec() */

