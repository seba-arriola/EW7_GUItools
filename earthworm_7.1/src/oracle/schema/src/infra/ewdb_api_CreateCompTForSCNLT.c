/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_api_CreateCompTForSCNLT.c,v 1.5 2005/06/10 16:27:58 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_api_CreateCompTForSCNLT.c,v $
 *     Revision 1.5  2005/06/10 16:27:58  davidk
 *     DB API Cleanup
 *
 *     Revision 1.4  2001/07/23 17:05:40  davidk
 *     API cleanup.
 *
 *     Revision 1.2  2001/05/15 02:16:29  davidk
 *     Moved functions around between the apps, DB API, and DB API INTERNAL
 *     levels.  Renamed functions and files.  Added support for amplitude
 *     magnitude types.  Reformatted makefiles.
 *
 *     Revision 1.1  2001/02/28 17:19:22  lucky
 *     Initial revision
 *
 *     Revision 1.6  2001/02/21 10:58:33  davidk
 *     Code was retrofitted to use the new db_cli_base API
 *     in lieu of the oci_base() and OCI7 function API's.layer, which
 *     provides abstraction from the OCI7 function calls to
 *     ora_api layer code.  See comments in
 *     schema/src/include/internal/ewdb_cli_base.h for more info
 *     on the details of the changes made.
 *
 *     Revision 1.5  2000/06/21 22:52:19  lucky
 *      Cleaned up logit calls to make log files more readable.
 *
 *     Revision 1.4  2000/05/15 20:54:09  davidk
 *     Added prototypes for the functions implemented in this file.
 *     Reformatted whitespace(put spaces in inplace of tabs).  Added
 *     some additional comments, and changed informational logits
 *     to only print when the EWDB_Debug flag is on.  Stopped all logits
 *     from printing to stderr, so as to be compatible with CGI-BIN web
 *     apps.
 *
 *     Revision 1.3  2000/02/10 21:24:22  davidk
 *     changed the tOn and tOff values to have 4 digits of precision after
 *     the decimal point instead of 2.  This is done because the DB now stores
 *     time in the 4 digit manner, and we do not want rounding to take place.
 *
 *     Revision 1.2  1999/11/09 18:46:12  lucky
 *     *** empty log message ***
 *
 *
 */


#include <ewdb_ora_api.h>
#include <ewdb_cli_base.h>
#include <ewdb_ew_oci_base.h>


static char SQL_STRING[] =
  "Begin Create_CompT_For_SCNLT(OUT_idChan => :OUT_idChan," 
  " IN_sSta => :IN_sSta, IN_sComp=>:IN_sComp,"
  " IN_sNet => :IN_sNet, IN_sLoc => :IN_sLoc," 
  " IN_tStart => :IN_tStart, IN_tEnd => :IN_tEnd); End;";
  
static EWDB_OCI_SFS SQLParamsBindArray[] = 
{
  {0,1,0,0,0,OA_EWDBID,":OUT_idChan"},
  {0,1,0,0,0,OA_SZ,":IN_sSta"},
  {0,1,0,0,0,OA_SZ,":IN_sComp"},
  {0,1,0,0,0,OA_SZ,":IN_sNet"},
  {0,1,0,0,0,OA_SZ,":IN_sLoc"},
  {0,1,0,0,0,OA_DOUBLE,":IN_tStart"},
  {0,1,0,0,0,OA_DOUBLE,":IN_tEnd"}
};

#define  NUM_FIELDS 7

static EWDB_StationStruct Local_StationStruct;
static char   Local_sztStart[20], Local_sztEnd[20];

/* Insertion Struct for SSStatement statement */
static EWDB_OCIStatementStruct SSStatement;


/********************************
      FUNCTION PROTOTYPES
********************************/
static int PrepCreateCompTForSCNLTExec(char * IN_szSta, char * IN_szComp,
                                char * IN_szNet, char * IN_szLoc, 
                                double IN_tStart, double IN_tEnd,
                                EWDB_Cursor *ppCursor);
static int PostCreateCompTForSCNLTExec(EWDBid * pidChan);
static int InitCreateCompTForSCNLTStatement(char *statement, 
                                     EWDB_OCIStatementStruct *pSS);
/*******************************/

/* CLEANUP  Why does this function return an idChan 
   when it is called CreateCompT, not CreateChan ?
   DavidK 2000/05/15
****************************************************/
int ewdb_api_CreateCompTForSCNLT(EWDBid * pidChan, 
                                 char * IN_szSta, char * IN_szComp,
                                 char * IN_szNet, char * IN_szLoc, 
                                 double IN_tStart, double IN_tEnd)
{
  /**************************
      Return Values:
                EWDB_RETURN_FAILURE: call failed
                EWDB_RETURN_SUCCESS: success
                others:              undefined
  **************************/

  EWDB_Cursor pCursor;

  ewdb_base_SetLastOraAPIActionTime();

  if(ewdb_base_Reconnect() != EWDB_RETURN_SUCCESS)
  {
    logit("", "ewdb_api_CreateCompTForSCNLT(): "
          "Could not reconnect to the database!\n");
    return(EWDB_RETURN_FAILURE);
  }

  if(PrepCreateCompTForSCNLTExec(IN_szSta,IN_szComp,IN_szNet,IN_szLoc,
                                 IN_tStart,IN_tEnd, &pCursor) != EWDB_RETURN_SUCCESS)
  {
    logit("", "ewdb_api_CreateCompTForSCNLT():PrepCreateCompTForSCNLT() failed.\n");
    return(ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }

  if(ewdb_base_SQLExecute(pCursor))
  {
    ewdb_base_ErrorReport(hEWDBC, pCursor,"ewdb_api_CreateCompTForSCNLT(): ewdb_base_SQLExecute", 1);
    return(ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  } 

  /* Commit the transaction(all the previous inserts!)
     In Case there is any auditing or logging or debug
     changes made in the stored procedures.
  ****************************************************/
  if(ewdb_base_SQLCommit(hEWDBC))
  {
    ewdb_base_ErrorReport(hEWDBC, hEWDBC,"ewdb_api_CreateCompTForSCNLT(): ewdb_base_SQLCommit",2);
    return(ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }

  if(PostCreateCompTForSCNLTExec(pidChan) != EWDB_RETURN_SUCCESS)
  {
    logit("", "ewdb_api_CreateCompTForSCNLT(): PostCreateCompTForSCNLTExec failed!\n");
    return(EWDB_RETURN_FAILURE);
  }

  return(EWDB_RETURN_SUCCESS);
}  /* end ewdb_api_CreateCompTForSCNLT() */


static int InitCreateCompTForSCNLTStatement(char *statement, 
                                     EWDB_OCIStatementStruct *pSS)
{

  pSS->FieldArray[0].pVal = &Local_StationStruct.idChan;
  pSS->FieldArray[1].pVal = Local_StationStruct.Sta;
  pSS->FieldArray[2].pVal = Local_StationStruct.Comp;
  pSS->FieldArray[3].pVal = Local_StationStruct.Net;
  pSS->FieldArray[4].pVal = Local_StationStruct.Loc;
  pSS->FieldArray[5].pVal = Local_sztStart;
  pSS->FieldArray[6].pVal = Local_sztEnd;

  return(ewdb_base_RequestCursor(statement, pSS, 0));
}  /* End InitCreateCompTForSCNLTStatement() */


static int PrepCreateCompTForSCNLTExec(char * IN_szSta, char * IN_szComp,
                                char * IN_szNet, char * IN_szLoc, 
                                double IN_tStart, double IN_tEnd,
                                EWDB_Cursor *ppCursor)
{

  SSStatement.NumOfFields = NUM_FIELDS;
  SSStatement.FieldArray = SQLParamsBindArray;
  SSStatement.RecordSize = 0;

  sprintf(Local_sztStart,"%.4f",IN_tStart);
  sprintf(Local_sztEnd,"%.4f",IN_tEnd);

  strcpy(Local_StationStruct.Sta,IN_szSta);
  strcpy(Local_StationStruct.Comp,IN_szComp);
  strcpy(Local_StationStruct.Net,IN_szNet);
  strcpy(Local_StationStruct.Loc,IN_szLoc);

  if(InitCreateCompTForSCNLTStatement(SQL_STRING,
                           &SSStatement) != EWDB_RETURN_SUCCESS)
  {
    logit("", "PrepCreateCompTForSCNLTExec(): InitCreateCompTForSCNLTStatement failed!\n");
    return EWDB_RETURN_FAILURE;
  }

  *ppCursor = SSStatement.pCda;
  
  return(EWDB_RETURN_SUCCESS);
}  /* End PrepCreateCompTForSCNLTExec() */


static int PostCreateCompTForSCNLTExec(EWDBid * pidChan)
{
  EWDB_Cursor pCursor;
  
  pCursor = SSStatement.pCda;
  ewdb_base_ReleaseCursor(pCursor);

  if(!pidChan)
  {
    logit("", "PostCreateChannelExec(): NULL Parameters passed in.\n");
    return(EWDB_RETURN_WARNING);
  }

  *pidChan = Local_StationStruct.idChan;
  
  
  if((*pidChan) <= 0)
  {
    logit("", "PostCreateCompTForSCNLTExec(): SQL Proc Create_CompT_For_SCNLT() returned error %d!\n", Local_StationStruct.idChan);
    return(EWDB_RETURN_FAILURE);
  }

  return(EWDB_RETURN_SUCCESS);
}  /* End PostCreateCompTForSCNLTExec() */
