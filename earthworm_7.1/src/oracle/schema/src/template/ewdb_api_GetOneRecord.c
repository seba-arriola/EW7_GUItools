/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_api_GetOneRecord.c,v 1.1 2005/06/21 20:11:54 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_api_GetOneRecord.c,v $
 *     Revision 1.1  2005/06/21 20:11:54  davidk
 *     function templates for use in creating new API functions.
 *
 *
 */

#include <ewdb_cli_base.h>
#include <ewdb_ora_api.h>
#include <ewdb_ew_oci_base.h>

static char SQL_STRING[] =
        "Begin Get_One_Record(OUT_Retcode => :OUT_Retcode, IN_idRecord => :IN_idRecord, "
        " OUT_sString => :OUT_sString, OUT_dDouble => :OUT_dDouble, "
        " OUT_iInt => :OUT_iInt, OUT_dFloat => :OUT_dFloat, "
        " OUT_cChar => :OUT_cChar, OUT_idFK => :OUT_idFK); End;";


static EWDB_OCI_SFS SQLParamsBindArray[] =

{
  {0,1,0,0,0,OA_INT,":OUT_Retcode"},
  {0,1,0,0,0,OA_EWDBID,":IN_idRecord"},
  {0,1,0,0,0,OA_SZ,":OUT_sString"},
  {0,1,0,0,0,OA_DOUBLE,":OUT_dDouble"},
  {0,1,0,0,0,OA_INT,":OUT_iInt"},
  {0,1,0,0,0,OA_FLOAT,":OUT_dFloat"},
  {0,1,0,0,0,OA_CHAR,":OUT_cChar"},
  {0,1,0,0,0,OA_EWDBID,":OUT_idFK"}
};

/* define the total number of fields to be bound */
#define NUM_FIELDS 8

/* Insertion Struct for this statement */
static EWDB_OCIStatementStruct SSStatement;

/* Local structures and buffers */
static EWDB_RecordStruct Local_Record;

static char Local_szdDouble[20],Local_szdFloat[20], Local_szcChar[2];

static int Local_iRetCode;


/********************************
      FUNCTION PROTOTYPES
********************************/
static int PrepGetOneRecordExec(EWDBid IN_idRecord, EWDB_Cursor * ppCursor);
static int PostGetOneRecordExec(EWDB_RecordStruct * pRecord);
static int InitGetOneRecordStatement(char * szStatement, EWDB_OCIStatementStruct *pSS);


/**********************************************************************
**********************************************************************/
int ewdb_api_GetOneRecord(EWDBid IN_idRecord, EWDB_RecordStruct * pRecord)
{

  EWDB_Cursor  pCursor;
 
  /* validate input parameters */
  if(IN_idRecord <= 0 || !pRecord )
  {
    logit("","ewdb_api_GetOneRecord(): Invalid params passed in:\n"
             "IN_idRecord %d, pRecord %u, \n",
          IN_idRecord, pRecord);
    return(EWDB_RETURN_FAILURE);
  }

  /* reset the dbms connection timeout */
  ewdb_base_SetLastOraAPIActionTime();

  /* make sure we're connected to the database. */
  if( ewdb_base_Reconnect() != EWDB_RETURN_SUCCESS )
  {
    return( EWDB_RETURN_FAILURE );
  }

  /* Call Prep() to copy caller's data to internal buffers and setup sql interface */
  if( PrepGetOneRecordExec(IN_idMag,&pCursor) != EWDB_RETURN_SUCCESS)
  {
    logit("","ewdb_api_GetOneRecord(): PrepGetOneRecordExec() failed!\n");
    return(EWDB_RETURN_FAILURE);
  }

  /* execute the sql statement */
  if( ewdb_base_SQLExecute(pCursor) )
  {
    ewdb_base_ErrorReport(hEWDBC, pCursor,"ewdb_api_GetOneRecord(): ewdb_base_SQLExecute",1);
    return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  } 

 /* Commit the transaction (all the previous inserts!)
  In Case there is any auditing or logging or debug
  changes made in the stored procedure.
  ****************************************************/
  if (ewdb_base_SQLCommit (hEWDBC))
  {
    ewdb_base_ErrorReport (hEWDBC, hEWDBC,"ewdb_api_GetOneRecord(): ewdb_base_SQLCommit",2);
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  }
 
  /* call Post() to check return codes and copy new id into caller's buffer */
  if( PostGetOneRecordExec(pMag) == EWDB_RETURN_FAILURE)
  {
    logit("","ewdb_api_GetOneRecord(): PostGetOneRecordExec() failed!\n");
    return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }

  /* reset the dbms connection timeout */
  ewdb_base_SetLastOraAPIActionTime();

  return(EWDB_RETURN_SUCCESS);
}  /* end ewdb_api_GetOneRecord() */


static int InitGetOneRecordStatement(char * szStatement, EWDB_OCIStatementStruct *pSS)
{
	pSS->FieldArray[0].pVal = &Local_iRetCode;
	pSS->FieldArray[1].pVal = &(Local_Record.idRecord);
	pSS->FieldArray[2].pVal = Local_Record.szString;
	pSS->FieldArray[3].pVal = Local_szdDouble;
	pSS->FieldArray[4].pVal = &(Local_Record.iInt);
	pSS->FieldArray[5].pVal = Local_szdFloat;
	pSS->FieldArray[6].pVal = Local_szcChar;
	pSS->FieldArray[7].pVal = &(Local_Record.idFK);

  return(ewdb_base_RequestCursor(szStatement, pSS,0/*don't force rebind*/));
}  /* end InitGetOneRecordStatement() */


static int PrepGetOneRecordExec(EWDBid IN_idRecord, EWDB_Cursor * ppCursor)
{

  /* initialize internal output buffers */
  memset(Local_szdDouble, 0, sizeof(Local_szdDouble));
  memset(Local_szdFloat,  0, sizeof(Local_szdFloat));
  memset(Local_szcChar,   0, sizeof(Local_szcChar));

  memset(&Local_Record, 0, sizeof(Local_Record));

  /* copy user's data over to local buffers */
  Local_Record.idRecord=IN_idRecord;

  SSStatement.NumOfFields=NUM_FIELDS;
  SSStatement.FieldArray=SQLParamsBindArray;
  SSStatement.RecordSize=0;

  /* call Init() function to prep the dbms client environment */
  if(InitGetOneRecordStatement(SQL_STRING, &SSStatement) != EWDB_RETURN_SUCCESS)
  {
    *ppCursor = NULL;
    return(EWDB_RETURN_FAILURE);
  }

  *ppCursor=SSStatement.pCda;

  return(EWDB_RETURN_SUCCESS);
}  /* end PrepGetOneRecordExec() */


static int PostGetOneRecordExec(EWDB_RecordStruct * pRecord)
{

  /* release the cursor so it can be used by other calls or this one again */
  ewdb_base_ReleaseCursor (SSStatement.pCda);

  /* check the return code */
  if(Local_iRetCode >= 0)
  {
    /* copy data from local buffers to local struct */
    Local_Record.dDouble = atof(Local_szdDouble);
    Local_Record.dFloat  = (float) atof(Local_szdFloat);
    Local_Recrod.cChar   = Local_szcChar[0];

    /* Copy the results to the user's struct */
    memcpy(pRecord, &Local_Record, sizeof(EWDB_RecordStruct));

    if(Local_iRetCode > 0)
    {
      logit("","PostGetOneRecordExec(): SQL Proc Get_One_Record() returned warning(%d).\n",Local_iRetCode);  
    }
    return(EWDB_RETURN_SUCCESS);
  }
  else
  {
      logit("","PostGetOneRecordExec(): SQL Proc Get_One_Record() returned error(%d).\n",Local_iRetCode);  
    return(EWDB_RETURN_WARNING);
  }
}   /* End PostGetOneRecordExec() */

