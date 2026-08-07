/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_api_CreateRecord.c,v 1.1 2005/06/21 20:11:54 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_api_CreateRecord.c,v $
 *     Revision 1.1  2005/06/21 20:11:54  davidk
 *     function templates for use in creating new API functions.
 *
 *
 */

#include <ewdb_cli_base.h>
#include <ewdb_ora_api.h>
#include <ewdb_ew_oci_base.h>

/* SQL String to be executed */
static char SQL_STRING[] =
        "Begin Create_Record(OUT_Retcode => :OUT_Retcode, OUT_idRecord => :OUT_idRecord, "
        " IN_dDouble => :IN_dDouble, IN_dFloat => :IN_dFloat, IN_idFK => :IN_idFK, "
        " IN_sString => :IN_sString, IN_cChar => :IN_cChar); End;";

/* Array of SQL bind parameters */
static EWDB_OCI_SFS SQLParamsBindArray[] =
{
  {0,1,0,0,0,OA_INT,":OUT_Retcode"},
  {0,1,0,0,0,OA_EWDBID,":OUT_idRecord"},
  {0,1,0,0,0,OA_DOUBLE,":IN_dDouble"},
  {0,1,0,0,0,OA_FLOAT, ":IN_dFloat"},
  {0,1,0,0,0,OA_EWDBID,":IN_idFK"},
  {0,1,0,0,0,OA_SZ,    ":IN_sString"},
  {0,1,0,0,0,OA_CHAR,  ":IN_cChar"},
};

/* define the total number of fields to be bound */
#define NUM_FIELDS 7

/* Insertion Struct for this statement */
static EWDB_OCIStatementStruct SSStatement;

/* LOCAL BUFFERS */
static EWDB_RecordStruct Local_RecordStruct;

static char Local_szdDouble[20], Local_szdFloat[20], Local_szcChar[2];

static int Local_iRetcode;

/********************************
      FUNCTION PROTOTYPES
********************************/
static int PrepCreateRecordExec(EWDB_RecordStruct * pRecord, EWDB_Cursor * ppCursor);
static int PostCreateRecordExec(EWDBid * pidRecord);
static int InitCreateRecordStatement(char * szStatement, EWDB_OCIStatementStruct *pSS);


/**********************************************************************
**********************************************************************/
int ewdb_api_CreateRecord(EWDB_RecordStruct * pRecord)
{
  int rc;

  EWDB_Cursor  pCursor;
 
  /* validate input parameters */
  if(!pRecord)
  {
    logit("et", "ERROR!  ewdb_api_CreateRecord(): Null pointer passed as input! Returning!\n");
    return( EWDB_RETURN_FAILURE );
  }

  /* reset the dbms connection timeout */
  ewdb_base_SetLastOraAPIActionTime();

  /* make sure we're connected to the database. */
  if( ewdb_base_Reconnect() != EWDB_RETURN_SUCCESS )
  {
    return( EWDB_RETURN_FAILURE );
  }

  /* Call Prep() to copy caller's data to internal buffers and setup sql interface */
  if( PrepCreateRecordExec(pRecord,&pCursor) != EWDB_RETURN_SUCCESS)
  {
    logit("","ewdb_api_CreateRecord(): PrepCreateRecordExec() failed!\n");
    return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }
  
  /* execute the sql statement */
  if( ewdb_base_SQLExecute(pCursor) )
  {
    ewdb_base_ErrorReport(hEWDBC, pCursor,"ewdb_api_CreateRecord(): ewdb_base_SQLExecute",1);
    return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  } 

  /* commit the sql statement */
  if(ewdb_base_SQLCommit(hEWDBC))
  {
    ewdb_base_ErrorReport(hEWDBC, hEWDBC,
                      "ewdb_api_CreateRecord(): ewdb_base_SQLCommit",2);
    return(ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }
  
  /* call Post() to check return codes and copy new id into caller's buffer */
  if((rc = PostCreateRecordExec(&pRecord->idRecord)) == EWDB_RETURN_FAILURE)
  {
    logit("","ewdb_api_CreateRecord(): PostCreateRecordExec() failed!\n");
    return(EWDB_RETURN_FAILURE);
  }

  /* reset the dbms connection timeout */
  ewdb_base_SetLastOraAPIActionTime();

  return(rc);
}  /* end ewdb_api_CreateRecord() */


static int InitCreateRecordStatement(char * szStatement, EWDB_OCIStatementStruct *pSS)
{
	pSS->FieldArray[0].pVal = &Local_iRetcode;
	pSS->FieldArray[1].pVal = &Local_Record.idRecord;
	pSS->FieldArray[2].pVal = Local_szdDouble;
	pSS->FieldArray[3].pVal = Local_szdFloat;
	pSS->FieldArray[4].pVal = &Local_Record.idFK;
	pSS->FieldArray[5].pVal = Local_Record.szString;
	pSS->FieldArray[6].pVal = Local_szcChar;

  return(ewdb_base_RequestCursor(szStatement, pSS,0/*don't force rebind*/));
}  /* end InitCreateRecordStatement() */


static int PrepCreateRecordExec(EWDB_RecordStruct * pRecord, EWDB_Cursor * ppCursor)
{
  SSStatement.NumOfFields=NUM_FIELDS;
  SSStatement.FieldArray=SQLParamsBindArray;
  SSStatement.RecordSize=0;

  /* initialize internal buffers */
  Local_iRetCode = 0;

  /* copy caller's data to local buffers */
  memcpy(&Local_Record, pRecord, sizeof(EWDB_RecordStruct));

  sprintf(Local_szdDouble, "%.4f", Local_Record.dDouble);
  sprintf(Local_szdFloat,  "%.2f", Local_Record.dFloat);

  Local_szcChar[0] = Local_Record.cChar;
  Local_szcChar[1] = 0x00;

  /* call Init() function to prep the dbms client environment */
  if(InitCreateRecordStatement(SQL_STRING, &SSStatement) != EWDB_RETURN_SUCCESS)
  {
    *ppCursor = NULL;
    return(EWDB_RETURN_FAILURE);
  }

  *ppCursor=SSStatement.pCda;

  return(EWDB_RETURN_SUCCESS);
}  /* end PrepCreateRecordExec() */


static int PostCreateRecordExec(EWDBid * pidRecord)
{
  /* release the cursor so it can be used by other calls or this one again */
  ewdb_base_ReleaseCursor (SSStatement.pCda);

  /* check the return code */
  if(Local_iRetcode >= 0)
  {
    *pidRecord = Local_Record.idRecord;

    if(Local_iRetcode > 0)
    {
      logit("","PostCreateRecordExec(): SQL Proc Create_Record() returned warning(%d).\n",
           Local_iRetcode);  
    }

    return(EWDB_RETURN_SUCCESS);
  }
  else
  {
    if(Local_iRetcode == -1)
    {
      logit("","PostCreateRecordExec(): SQL Proc Create_Record() returned UNKNOWN ERROR.  See debug table for details.\n");
      return(EWDB_RETURN_FAILURE);
    }
    else 
    {
      logit("","PostCreateRecordExec(): SQL Proc Create_Record() returned "
            "the following ERROR(%d).\n",Local_iRetcode);  
      return(EWDB_RETURN_FAILURE);
    }
    return(EWDB_RETURN_WARNING);
  }
}   /* End PostXXX() */

