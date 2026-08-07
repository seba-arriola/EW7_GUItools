/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_api_GetidSite.c,v 1.4 2005/06/27 15:30:01 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_api_GetidSite.c,v $
 *     Revision 1.4  2005/06/27 15:30:01  davidk
 *     Fixed bugs introduced during global fetch/replace during DB cleanup.
 *
 *     Revision 1.3  2005/06/10 16:27:59  davidk
 *     DB API Cleanup
 *
 *     Revision 1.2  2003/12/04 19:51:00  davidk
 *     Fixed a syntax error that was preventing compilation.
 *
 *     Revision 1.1  2003/12/03 00:23:29  davidk
 *     Initial revision
 *
 *
 *
 */


#include <ewdb_ora_api.h>
#include <ewdb_cli_base.h>
#include <ewdb_ew_oci_base.h>

static char SQL_STRING[] =
  "Begin select idSite into :OUT_idSite from ALL_SITE_INFO "
  " where sSta = :IN_sSta "
  "   and sNet = :IN_sNet; "
  "EXCEPTION "
  "  WHEN OTHERS THEN "
  "    :OUT_idSite := -1; "
  "End;";

static EWDB_OCI_SFS SQLParamsBindArray[] = 
{
  {0,1,0,0,0,OA_INT,":OUT_idSite"},
  {0,1,0,0,0,OA_SZ,":IN_sSta"},
  {0,1,0,0,0,OA_SZ,":IN_sNet"}
};

#define  NUM_FIELDS 3


static EWDBid Local_idSite;
static char Local_szSta[10], Local_szNet[10];

/* Statement Struct for GetidSite statement */
static EWDB_OCIStatementStruct SSStatement;

/********************************
      FUNCTION PROTOTYPES
********************************/
static int PrepGetidSiteExec(char * IN_szSta, char * IN_szNet, EWDB_Cursor * ppCursor);
static int PostGetidSiteExec(EWDBid * pidSite);
static int InitGetidSiteStatement(char *statement, 
                            EWDB_OCIStatementStruct *pSS);


int ewdb_api_GetidSite(EWDB_ChannelStruct * pChan)
{
  int rc;

  EWDB_Cursor pCursor;

  if (pChan== NULL)
  {
    logit ("", "ewdb_api_GetidSite():Null pointer passed in!\n");
    return EWDB_RETURN_FAILURE;
  }

  ewdb_base_SetLastOraAPIActionTime ();


  if (ewdb_base_Reconnect () != EWDB_RETURN_SUCCESS)
  {
    logit("", "ewdb_api_GetidSite(): "
          "Could not reconnect to the database!\n");
    return (EWDB_RETURN_FAILURE);
  }

  if (PrepGetidSiteExec (pChan->Comp.Sta, pChan->Comp.Net, &pCursor) != EWDB_RETURN_SUCCESS)
  {
    logit ("", "ewdb_api_GetidSite():PrepGetidSiteExec() failed.\n");
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  }

  if (ewdb_base_SQLExecute (pCursor))
  {
    ewdb_base_ErrorReport (hEWDBC, pCursor,"ewdb_api_GetidSite():ewdb_base_SQLExecute", 1);
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  } 

  
  /* Commit the transaction (all the previous inserts!)
     In Case there is any auditing or logging or debug
     changes made in the stored procedures.
  ****************************************************/
  if (ewdb_base_SQLCommit (hEWDBC))
  {
    ewdb_base_ErrorReport (hEWDBC, hEWDBC,"ewdb_api_GetidSite():ewdb_base_SQLCommit",2);
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  }

  rc = PostGetidSiteExec(&pChan->idSite);
  if(rc == EWDB_RETURN_FAILURE)
  {
    logit ("", "ewdb_api_GetidSite(): PostGetidSiteExec failed!\n");
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  }

  ewdb_base_SetLastOraAPIActionTime();

  return(rc);
}  /* end ewdb_api_GetidSite() */


static int InitGetidSiteStatement(char *statement, 
                            EWDB_OCIStatementStruct *pSS)
{
  pSS->FieldArray[0].pVal = &Local_idSite;
  pSS->FieldArray[1].pVal = Local_szSta;
  pSS->FieldArray[2].pVal = Local_szNet;

  return(ewdb_base_RequestCursor (statement, pSS, 0));
}  /* End InitGetidSiteStatement() */


static int PrepGetidSiteExec(char * IN_szSta, char * IN_szNet, EWDB_Cursor * ppCursor)
{

  SSStatement.NumOfFields = NUM_FIELDS;
  SSStatement.FieldArray = SQLParamsBindArray;
  SSStatement.RecordSize = 0;

  strncpy(Local_szSta, IN_szSta, sizeof(Local_szSta));
  Local_szSta[sizeof(Local_szSta) - 1] = 0x00;

  strncpy(Local_szNet, IN_szNet, sizeof(Local_szNet));
  Local_szNet[sizeof(Local_szNet) - 1] = 0x00;

  if (InitGetidSiteStatement (SQL_STRING, &SSStatement) != EWDB_RETURN_SUCCESS)
  {
    logit ("", "PrepGetidSiteExec(): InitGetidSiteStatement() failed!\n");
    return EWDB_RETURN_FAILURE;
  }

  *ppCursor = SSStatement.pCda;
  
  return(EWDB_RETURN_SUCCESS);
}  /* End PrepGetidSiteExec() */


static int PostGetidSiteExec(EWDBid * pidSite)
{
  EWDB_Cursor pCursor;
  
  pCursor = SSStatement.pCda;

  ewdb_base_ReleaseCursor(pCursor);

  *pidSite = Local_idSite;

  if(Local_idSite <= 0)
  {
    return(EWDB_RETURN_WARNING);
  }

  return(EWDB_RETURN_SUCCESS);
}  /* End PostGetidSiteExec() */

