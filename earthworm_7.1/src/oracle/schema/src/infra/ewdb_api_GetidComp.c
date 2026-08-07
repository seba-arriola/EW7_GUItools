/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_api_GetidComp.c,v 1.3 2005/06/27 15:30:01 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_api_GetidComp.c,v $
 *     Revision 1.3  2005/06/27 15:30:01  davidk
 *     Fixed bugs introduced during global fetch/replace during DB cleanup.
 *
 *     Revision 1.2  2005/06/10 16:27:59  davidk
 *     DB API Cleanup
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
  "Begin select idComp into :OUT_idComp from ALL_COMP_INFO "
  " where sSta  = :IN_sSta "
  "   and sComp = :IN_sComp "
  "   and sNet = :IN_sNet "
  "   and (sLoc = :IN_sLoc OR (sLoc IS NULL and :IN_sLoc IS NULL)); "
  "EXCEPTION "
  "  WHEN OTHERS THEN "
  "    :OUT_idComp := -1; "
  "End;";

static EWDB_OCI_SFS SQLParamsBindArray[] = 
{
  {0,1,0,0,0,OA_INT,":OUT_idComp"},
  {0,1,0,0,0,OA_SZ,":IN_sSta"},
  {0,1,0,0,0,OA_SZ,":IN_sComp"},
  {0,1,0,0,0,OA_SZ,":IN_sNet"},
  {0,1,0,0,0,OA_SZ,":IN_sLoc"}
};

#define  NUM_FIELDS 5


static EWDBid Local_idComp;
static char Local_szSta[10], Local_szComp[10], Local_szNet[10], Local_szLoc[10];

/* Statement Struct for GetidComp statement */
static EWDB_OCIStatementStruct SSStatement;

/********************************
      FUNCTION PROTOTYPES
********************************/
static int PrepGetidCompExec(EWDB_ChannelStruct * IN_pChan, EWDB_Cursor * ppCursor);
static int PostGetidCompExec(EWDBid * pidComp);
static int InitGetidCompStatement(char *statement, 
                            EWDB_OCIStatementStruct *pSS);




int ewdb_api_GetidComp(EWDB_ChannelStruct * pChan)
{
  int rc;

  EWDB_Cursor pCursor;

  if (pChan== NULL)
  {
    logit ("", "ewdb_api_GetidComp():Null pointer passed in!\n");
    return EWDB_RETURN_FAILURE;
  }

  ewdb_base_SetLastOraAPIActionTime ();

  if (ewdb_base_Reconnect () != EWDB_RETURN_SUCCESS)
  {
    logit("", "ewdb_api_GetidComp(): "
          "Could not reconnect to the database!\n");
    return (EWDB_RETURN_FAILURE);
  }

  if (PrepGetidCompExec (pChan, &pCursor) != EWDB_RETURN_SUCCESS)
  {
    logit ("", "ewdb_api_GetidComp():PrepGetidCompExec() failed.\n");
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  }

  if (ewdb_base_SQLExecute (pCursor))
  {
    ewdb_base_ErrorReport (hEWDBC, pCursor,"ewdb_api_GetidComp():ewdb_base_SQLExecute", 1);
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  } 
  
  /* Commit the transaction (all the previous inserts!)
     In Case there is any auditing or logging or debug
     changes made in the stored procedures.
  ****************************************************/
  if (ewdb_base_SQLCommit (hEWDBC))
  {
    ewdb_base_ErrorReport (hEWDBC, hEWDBC,"ewdb_api_GetidComp():ewdb_base_SQLCommit",2);
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  }

  rc = PostGetidCompExec(&pChan->Comp.idComp);
  if(rc == EWDB_RETURN_FAILURE)
  {
    logit ("", "ewdb_api_GetidComp(): PostGetidCompExec failed!\n");
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  }

  ewdb_base_SetLastOraAPIActionTime();

  return(rc);
}  /* end ewdb_api_GetidComp() */


static int InitGetidCompStatement(char *statement, 
                            EWDB_OCIStatementStruct *pSS)
{
  pSS->FieldArray[0].pVal = &Local_idComp;
  pSS->FieldArray[1].pVal = Local_szSta;
  pSS->FieldArray[2].pVal = Local_szComp;
  pSS->FieldArray[3].pVal = Local_szNet;
  pSS->FieldArray[4].pVal = Local_szLoc;

  return(ewdb_base_RequestCursor (statement, pSS, 0));
}  /* End InitGetidCompStatement() */


static int PrepGetidCompExec(EWDB_ChannelStruct * IN_pChan, EWDB_Cursor * ppCursor)
{

  SSStatement.NumOfFields = NUM_FIELDS;
  SSStatement.FieldArray = SQLParamsBindArray;
  SSStatement.RecordSize = 0;

  strncpy(Local_szSta, IN_pChan->Comp.Sta, sizeof(Local_szSta));
  Local_szSta[sizeof(Local_szSta) - 1] = 0x00;

  strncpy(Local_szComp, IN_pChan->Comp.Comp, sizeof(Local_szComp));
  Local_szComp[sizeof(Local_szComp) - 1] = 0x00;

  strncpy(Local_szNet, IN_pChan->Comp.Net, sizeof(Local_szNet));
  Local_szNet[sizeof(Local_szNet) - 1] = 0x00;

  strncpy(Local_szLoc, IN_pChan->Comp.Loc, sizeof(Local_szLoc));
  Local_szLoc[sizeof(Local_szLoc) - 1] = 0x00;

  if (InitGetidCompStatement (SQL_STRING, &SSStatement) != EWDB_RETURN_SUCCESS)
  {
    logit ("", "PrepGetidCompExec(): InitGetidCompStatement failed!\n");
    return EWDB_RETURN_FAILURE;
  }

  *ppCursor = SSStatement.pCda;
  
  return(EWDB_RETURN_SUCCESS);
}  /* End PrepGetidCompExec() */


static int PostGetidCompExec(EWDBid * pidComp)
{
  EWDB_Cursor pCursor;
  
  pCursor = SSStatement.pCda;

  ewdb_base_ReleaseCursor(pCursor);

  *pidComp = Local_idComp;

  if(Local_idComp <= 0)
  {
    return(EWDB_RETURN_WARNING);
  }

  return(EWDB_RETURN_SUCCESS);
}  /* End PostGetidCompExec() */

