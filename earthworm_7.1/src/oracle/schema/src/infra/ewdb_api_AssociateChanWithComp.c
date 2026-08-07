
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_api_AssociateChanWithComp.c,v 1.4 2005/06/10 16:27:58 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_api_AssociateChanWithComp.c,v $
 *     Revision 1.4  2005/06/10 16:27:58  davidk
 *     DB API Cleanup
 *
 *     Revision 1.3  2001/07/23 17:04:12  davidk
 *     API cleanup.
 *     Removed pidExistingComp argument from PostXXX() param list, as it
 *     was not used or supported.
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
  "Begin Assoc_Comp_w_Chan(OUT_Local_iRetCode => :OUT_Local_iRetCode,"
  " OUT_idCompT => :OUT_idCompT," 
  " OUT_idChanT => :OUT_idChanT, IN_idComp=>:IN_idComp,"
  " IN_idChan => :IN_idChan, IN_tStart => :IN_tStart," 
  " IN_tEnd => :IN_tEnd); End;";

static EWDB_OCI_SFS SQLParamsBindArray[] = 
{
  {0,1,0,0,0,OA_INT,":OUT_Local_iRetCode"},
  {0,1,0,0,0,OA_INT,":OUT_idCompT"},
  {0,1,0,0,0,OA_INT,":OUT_idChanT"},
  {0,1,0,0,0,OA_INT,":IN_idComp"},
  {0,1,0,0,0,OA_INT,":IN_idChan"},
  {0,1,0,0,0,OA_DOUBLE,":IN_tStart"},
  {0,1,0,0,0,OA_DOUBLE,":IN_tEnd"}
};

#define  NUM_FIELDS 7

static EWDBid Local_idCompT, Local_idChanT,
              Local_idComp,  Local_idChan;

static int  Local_iRetCode;
static char Local_sztStart[20],Local_sztEnd[20];

/* Statement Struct for Assoc_Comp_w_Chan statement */
static EWDB_OCIStatementStruct SSStatement;


/********************************
      FUNCTION PROTOTYPES
********************************/
static int PrepAssocChanWCompExec(EWDBid IN_idComp, EWDBid IN_idChan, 
                            double IN_tStart, double IN_tEnd, 
                            EWDB_Cursor * ppCursor);
static int PostAssocChanWCompExec(EWDBid * pLocal_idCompT, EWDBid * pLocal_idChanT);
static int InitAssocChanWCompStatement(char *statement, EWDB_OCIStatementStruct *pSS);



/* A channel can only be associated with at most one component at a give time.  
   If the channel that the caller is trying to bind a component to is already
   bound to another component during an overlapping time interval, then the
   call will fail, and the Local_idComp of the first component found that has an
   overlapping association with that channel will be returned.
******************************************************************/
int ewdb_api_AssociateChanWithComp(EWDBid Local_idComp, EWDBid Local_idChan, 
                              double tStart, double tEnd,
                              EWDBid * pLocal_idCompT, EWDBid * pLocal_idChanT)
{

  EWDB_Cursor pCursor;

  if(pLocal_idCompT== NULL || pLocal_idChanT== NULL)
  {
    logit("", "ewdb_api_AssociateChanWithComp(): Null pointer passed in!\n");
    return EWDB_RETURN_FAILURE;
  }

  ewdb_base_SetLastOraAPIActionTime();

  if(ewdb_base_Reconnect() != EWDB_RETURN_SUCCESS)
  {
    logit("", "ewdb_api_AssociateChanWithComp(): "
          "Could not reconnect to the database!\n");
    return(EWDB_RETURN_FAILURE);
  }

  if(PrepAssocChanWCompExec(Local_idComp, Local_idChan, tStart, tEnd, &pCursor) 
      != EWDB_RETURN_SUCCESS)
  {
    logit("", "ewdb_api_AssociateChanWithComp(): PrepAssocChanWCompExec() failed.\n");
    return(ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }

  if(ewdb_base_SQLExecute(pCursor))
  {
    ewdb_base_ErrorReport(hEWDBC, pCursor,"ewdb_api_AssociateChanWithComp(): ewdb_base_SQLExecute", 1);
    return(ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  } 

  
  /* Commit the transaction(all the previous inserts!)
     In Case there is any auditing or logging or debug
     changes made in the stored procedures.
  ****************************************************/
  if(ewdb_base_SQLCommit(hEWDBC))
  {
    ewdb_base_ErrorReport(hEWDBC, hEWDBC,"ewdb_api_AssociateChanWithComp(): ewdb_base_SQLCommit",2);
    return(ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }

  if(PostAssocChanWCompExec(pLocal_idCompT,pLocal_idChanT) 
      != EWDB_RETURN_SUCCESS)
  {
    logit("", "ewdb_api_AssociateChanWithComp(): PostAssocChanWCompExec failed!\n");
    return(EWDB_RETURN_FAILURE);
  }

  ewdb_base_SetLastOraAPIActionTime();

  return(EWDB_RETURN_SUCCESS);
}  /* end ewdb_api_AssociateChanWithComp() */


static int InitAssocChanWCompStatement(char *statement, EWDB_OCIStatementStruct *pSS)
{

  pSS->FieldArray[0].pVal = &Local_iRetCode;
  pSS->FieldArray[1].pVal = &Local_idCompT;
  pSS->FieldArray[2].pVal = &Local_idChanT;
  pSS->FieldArray[3].pVal = &Local_idComp;
  pSS->FieldArray[4].pVal = &Local_idChan;
  pSS->FieldArray[5].pVal = Local_sztStart;
  pSS->FieldArray[6].pVal = Local_sztEnd;

  return(ewdb_base_RequestCursor(statement, pSS, 0));
}  /* End InitAssocChanWCompStatement() */


static int PrepAssocChanWCompExec(EWDBid IN_idComp, EWDBid IN_idChan, 
                           double IN_tStart, double IN_tEnd, 
                           EWDB_Cursor * ppCursor) 
{

  SSStatement.NumOfFields = NUM_FIELDS;
  SSStatement.FieldArray = SQLParamsBindArray;
  SSStatement.RecordSize = 0;

  sprintf(Local_sztStart,"%.2f",IN_tStart);
  sprintf(Local_sztEnd,"%.2f",IN_tEnd);

  Local_idComp = IN_idComp;
  Local_idChan = IN_idChan;

  if(InitAssocChanWCompStatement(SQL_STRING,
                           &SSStatement) != EWDB_RETURN_SUCCESS)
  {
    logit("", "PrepAssocChanWCompExec(): Call to InitAssocChanWCompStatement failed!\n");
    return EWDB_RETURN_FAILURE;
  }

  *ppCursor = SSStatement.pCda;
  
  return(EWDB_RETURN_SUCCESS);
}  /* End PrepAssocChanWCompExec() */


static int PostAssocChanWCompExec(EWDBid * pLocal_idCompT, EWDBid * pLocal_idChanT)
{
  EWDB_Cursor pCursor;
  
  *pLocal_idCompT=Local_idCompT;
  *pLocal_idChanT=Local_idChanT;
  
  pCursor = SSStatement.pCda;
  
  ewdb_base_ReleaseCursor(pCursor);
  
  if(Local_iRetCode==0)
    return(EWDB_RETURN_SUCCESS);
  else
  {
    logit("","PostAssocChanWCompExec():  SQL Proc Assoc_Comp_w_Chan() failed with error: %d\n",Local_iRetCode);
    return(EWDB_RETURN_FAILURE);
  }
}  /* End PostAssocChanWCompExec() */

