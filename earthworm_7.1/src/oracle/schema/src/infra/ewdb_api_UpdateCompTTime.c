/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_api_UpdateCompTTime.c,v 1.3 2005/06/10 16:27:59 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_api_UpdateCompTTime.c,v $
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
     "  Begin Update_CompT_Time(OUT_iRetCode => :OUT_iRetCode, "
           "IN_idCompT => :IN_idCompT, "
           "IN_tOn => :IN_tOn, "
           "IN_tOff => :IN_tOff); End;";

static EWDB_OCI_SFS SQLParamsBindArray[] = 
{
  {0,1,0,0,0,OA_INT,":OUT_iRetCode"},
  {0,1,0,0,0,OA_INT,":IN_idCompT"},
  {0,1,0,0,0,OA_DOUBLE,":IN_tOn"},
  {0,1,0,0,0,OA_DOUBLE,":IN_tOff"}
};

#define  NUM_FIELDS 4

/* static variables */
static char Local_sztOn[20];
static char Local_sztOff[20];
static EWDB_ChannelStruct Local_Chan;
static int Local_iRetCode;

/* Statement Struct for UpdateCompTTime szStatement */
static EWDB_OCIStatementStruct SSStatement;

/********************************
      FUNCTION PROTOTYPES
********************************/
static int PrepUpdateCompTTimeExec(EWDB_ChannelStruct * IN_pChan, EWDB_Cursor * ppCursor);
static int PostUpdateCompTTimeExec(int * pRetCode);
static int InitUpdateCompTTimeStatement(char *szStatement, 
                             EWDB_OCIStatementStruct *pSS);


/* Describe Function ewdb_api_UpdateCompTTime
*********************************************************************/
int ewdb_api_UpdateCompTTime(int * OUT_pRetCode, EWDB_ChannelStruct * pChan)
{

  EWDB_Cursor pCursor;
  int rc;

  if(pChan == NULL)
  {
    logit("","ewdb_api_UpdateCompTTime():Null pChan pointer passed in!\n");
    return(EWDB_RETURN_FAILURE);
  }

  ewdb_base_SetLastOraAPIActionTime();

  if(ewdb_base_Reconnect() != EWDB_RETURN_SUCCESS)
  {
    logit("","ewdb_api_UpdateCompTTime(): Could not reconnect to the database!\n");
    return(EWDB_RETURN_FAILURE);
  }

  if(PrepUpdateCompTTimeExec(pChan, &pCursor)
      != EWDB_RETURN_SUCCESS)
  {
    logit("","ewdb_api_UpdateCompTTime():PrepUpdateCompTTimeExec() failed.\n");
    return(EWDB_RETURN_FAILURE);
  }

  if(ewdb_base_SQLExecute(pCursor))
  {
    ewdb_base_ErrorReport(hEWDBC, pCursor,"ewdb_api_UpdateCompTTime():ewdb_base_SQLExecute", 1);
    return(ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  } 

  /* Commit the transaction(all the previous inserts!)
  ****************************************************/
  if(ewdb_base_SQLCommit(hEWDBC))
  {
    ewdb_base_ErrorReport(hEWDBC, hEWDBC,"ewdb_api_UpdateCompTTime():ewdb_base_SQLCommit",2);
    return(ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }

  rc = PostUpdateCompTTimeExec(OUT_pRetCode);

  ewdb_base_SetLastOraAPIActionTime();

  if(rc == EWDB_RETURN_WARNING)
  {
    return(EWDB_RETURN_FAILURE);
  }
  else if(rc == EWDB_RETURN_FAILURE)
  {
    logit("", "ewdb_api_UpdateCompTTime(): "
           "ERROR:  PostUpdateCompTTimeExec failed!!\n");
    return(ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }

  return(EWDB_RETURN_SUCCESS);
}  /* end ewdb_api_UpdateCompTTime() */


static int InitUpdateCompTTimeStatement(char *szStatement,
                                        EWDB_OCIStatementStruct *pSS)
{

  pSS->FieldArray[0].pVal = &Local_iRetCode;
  pSS->FieldArray[1].pVal = &Local_Chan.idCompT;
  pSS->FieldArray[2].pVal = Local_sztOn;
  pSS->FieldArray[3].pVal = Local_sztOff;

  return(ewdb_base_RequestCursor(szStatement, pSS, 0));
}  /* end InitUpdateCompTTimeStatement() */


static int PrepUpdateCompTTimeExec(EWDB_ChannelStruct * IN_pChan, EWDB_Cursor * ppCursor)
{

  SSStatement.NumOfFields = NUM_FIELDS;
  SSStatement.FieldArray = SQLParamsBindArray;
  SSStatement.RecordSize = 0;

  /* Copy user's vars to local vars here */
  sprintf(Local_sztOn, "%.2f", IN_pChan->tOn);
  sprintf(Local_sztOff, "%.2f", IN_pChan->tOff);
  Local_Chan.idCompT = IN_pChan->idCompT;

  if(InitUpdateCompTTimeStatement(SQL_STRING,
                              &SSStatement) != EWDB_RETURN_SUCCESS)
  {
    logit("", "Call to InitUpdateCompTTimeStatement failed!\n");
    return(EWDB_RETURN_FAILURE);
  }

  *ppCursor = SSStatement.pCda;
  
  return(EWDB_RETURN_SUCCESS);

}  /* end PrepUpdateCompTTimeExec() */


static int PostUpdateCompTTimeExec(int * pRetCode)
{
  EWDB_Cursor pCursor;
  
  *pRetCode = Local_iRetCode;
  
  pCursor = SSStatement.pCda;
  
  ewdb_base_ReleaseCursor(pCursor);

  if(Local_iRetCode < 0)
  {
    logit("", "ERROR:  PostUpdateCompTTimeExec() reports SQL Proc Update_CompT(%d) returned error(%d)!\n",
          Local_Chan.idCompT, *pRetCode);
    return(EWDB_RETURN_WARNING);
  }

  return(EWDB_RETURN_SUCCESS);
}  /* end PostUpdateCompTTimeExec() */


