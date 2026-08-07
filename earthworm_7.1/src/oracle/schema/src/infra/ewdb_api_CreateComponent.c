
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_api_CreateComponent.c,v 1.4 2005/06/10 16:27:58 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_api_CreateComponent.c,v $
 *     Revision 1.4  2005/06/10 16:27:58  davidk
 *     DB API Cleanup
 *
 *     Revision 1.3  2001/07/23 17:06:25  davidk
 *     API Cleanup.
 *     Improved handling of error codes from SQL Proc.
 *
 *     Revision 1.2  2001/05/15 02:16:28  davidk
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
  "Begin Create_Comp_From_SCNL(OUT_idComp => :OUT_idComp,"
  " OUT_idSite => :OUT_idSite," 
  " IN_sSta => :IN_sSta, IN_sComp=>:IN_sComp,"
  " IN_sNet => :IN_sNet, IN_sLoc => :IN_sLoc," 
  " IN_sComment => :IN_sComment); End;";

static EWDB_OCI_SFS SQLParamsBindArray[] = 
{
  {0,1,0,0,0,OA_INT,":OUT_idComp"},
  {0,1,0,0,0,OA_INT,":OUT_idSite"},
  {0,1,0,0,0,OA_SZ,":IN_sSta"},
  {0,1,0,0,0,OA_SZ,":IN_sComp"},
  {0,1,0,0,0,OA_SZ,":IN_sNet"},
  {0,1,0,0,0,OA_SZ,":IN_sLoc"},
  {0,1,0,0,0,OA_SZ,":IN_sComment"}
};

#define  CCFSCNL_NUM_FIELDS 7

static EWDBid Local_idSite;
static EWDB_StationStruct Local_StationStruct;
static char Local_szComment[4100];

/* Statement Struct for CREATE_COMP_FROM_SCNL statement */
static EWDB_OCIStatementStruct SSStatement;

/********************************
      FUNCTION PROTOTYPES
********************************/
static int PrepCreateComponentExec(char * IN_szSta, char * IN_szComp, char * IN_szNet, 
                            char * IN_szLoc, char * IN_szComment, 
                            EWDB_Cursor * ppCursor);
static int PostCreateComponentExec(EWDBid * pidComp);
static int InitCreateComponentStatement(char *statement, 
                                 EWDB_OCIStatementStruct *pSS);


/* Creates a component sensor record, and associates it with a Site.
   Site is(Sta,Net), and Component is Comp and optionally Loc.  
   Room for a comment is included.
*********************************************************************/
int ewdb_api_CreateComponent(EWDBid *pidComp, char * IN_szSta, char * IN_szComp, 
                             char * IN_szNet, char * IN_szLoc, char * IN_szComment)
{

  EWDB_Cursor pCursor;
  int rc;

  if(pidComp == NULL || IN_szSta == NULL)
  {
    logit("", "EWDB_CreateComponent:Null pointer passed in!\n");
    return EWDB_RETURN_FAILURE;
  }

  ewdb_base_SetLastOraAPIActionTime();


  if(ewdb_base_Reconnect() != EWDB_RETURN_SUCCESS)
  {
    logit("", "EWDB_CreateComponent(): Could not reconnect to the database!\n");
    return(EWDB_RETURN_FAILURE);
  }

  if(PrepCreateComponentExec(IN_szSta, IN_szComp, IN_szNet, IN_szLoc, IN_szComment, &pCursor) 
      != EWDB_RETURN_SUCCESS)
  {
    logit("", "ORA_API:EWDB_CreateComponent():PrepCreateComponentExec() failed.\n");
    return(ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }

  if(ewdb_base_SQLExecute(pCursor))
  {
    ewdb_base_ErrorReport(hEWDBC, pCursor,"EWDB_CreateComponent:ewdb_base_SQLExecute", 1);
    return(ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  } 

  
  /* Commit the transaction(all the previous inserts!)
     In Case there is any auditing or logging or debug
     changes made in the stored procedures.
  ****************************************************/
  if(ewdb_base_SQLCommit(hEWDBC))
  {
    ewdb_base_ErrorReport(hEWDBC, hEWDBC,"EWDB_CreateComponent:ewdb_base_SQLCommit",2);
    return(ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }

  rc = PostCreateComponentExec(pidComp);

  ewdb_base_SetLastOraAPIActionTime();

  if(rc == EWDB_RETURN_WARNING)
  {
    logit("", "EWDB_CreateComponent(): "
           "ERROR:  PostCreateComponentExec reports SQL Proc %s returned "
           "error(%d)!\n"
          "Create_Comp_From_SCNL()", *pidComp);
    return(EWDB_RETURN_FAILURE);
  }
  else if(rc == EWDB_RETURN_FAILURE)
  {
    logit("", "EWDB_CreateComponent(): "
           "ERROR:  PostCreateComponentExec failed!!\n");
    return(ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }

  return(EWDB_RETURN_SUCCESS);
}  /* end ewdb_api_CreateComponent() */


static int InitCreateComponentStatement(char *statement, 
                                 EWDB_OCIStatementStruct *pSS)
{

  pSS->FieldArray[0].pVal = &Local_StationStruct.idComp;
  pSS->FieldArray[1].pVal = &Local_idSite;
  pSS->FieldArray[2].pVal = Local_StationStruct.Sta;
  pSS->FieldArray[3].pVal = Local_StationStruct.Comp;
  pSS->FieldArray[4].pVal = Local_StationStruct.Net;
  pSS->FieldArray[5].pVal = Local_StationStruct.Loc;
  pSS->FieldArray[6].pVal = Local_szComment;

  ewdb_base_RequestCursor(statement, pSS, 0);

  
  return(EWDB_RETURN_SUCCESS);
}  /* end InitCreateComponentStatement() */


static int PrepCreateComponentExec(char * IN_szSta, char * IN_szComp, char * IN_szNet, 
                            char * IN_szLoc, char * IN_szComment, 
                            EWDB_Cursor * ppCursor)
{

  SSStatement.NumOfFields = CCFSCNL_NUM_FIELDS;
  SSStatement.FieldArray = SQLParamsBindArray;
  SSStatement.RecordSize = 0;

  strcpy(Local_StationStruct.Sta, IN_szSta);
  strcpy(Local_StationStruct.Comp, IN_szComp);
  strcpy(Local_StationStruct.Net, IN_szNet);
  strcpy(Local_StationStruct.Loc, IN_szLoc);

  strncpy(Local_szComment, IN_szComment, sizeof(Local_szComment));
  Local_szComment[sizeof(Local_szComment)-1]=0;

  if(InitCreateComponentStatement(SQL_STRING,
                           &SSStatement) != EWDB_RETURN_SUCCESS)
  {
    logit("", "PrepCreateComponentExec(): InitCreateComponentStatement failed!\n");
    return EWDB_RETURN_FAILURE;
  }

  *ppCursor = SSStatement.pCda;
  
  return(EWDB_RETURN_SUCCESS);
}  /* end PrepCreateComponentExec() */


static int PostCreateComponentExec(EWDBid * pidComp)
{
  EWDB_Cursor pCursor;

  pCursor = SSStatement.pCda;
  
  ewdb_base_ReleaseCursor(pCursor);

  if(!pidComp)
  {
    logit("", "PostCreateComponentExec(): Null parameters passed in!\n");
    return(EWDB_RETURN_FAILURE);
  }

  *pidComp=Local_StationStruct.idComp;
  
  if(Local_StationStruct.idComp <= 0)
  {
    logit("", "PostCreateComponentExec(): SQL Proc Create_Comp_From_SCNL() failed with error %d/%d\n", 
          Local_StationStruct.idComp, Local_idSite);
    return(EWDB_RETURN_WARNING);
  }

  return(EWDB_RETURN_SUCCESS);
}  /* end PostCreateComponentExec() */

