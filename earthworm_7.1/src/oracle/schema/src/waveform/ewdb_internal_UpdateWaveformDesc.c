
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_internal_UpdateWaveformDesc.c,v 1.1 2002/05/16 17:06:04 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_internal_UpdateWaveformDesc.c,v $
 *     Revision 1.1  2002/05/16 17:06:04  davidk
 *     Initial revision
 *
 *     Revision 1.2  2001/07/23 17:26:07  davidk
 *     API Cleanup.
 *     Improved error handling of return codes from SQL Proc.
 *
 *     Revision 1.1  2001/05/15 02:16:45  davidk
 *     Initial revision
 *
 *     Revision 1.1  2001/02/28 17:19:22  lucky
 *     Initial revision
 *
 *
 */



#include <ewdb_ora_api.h>
#include <ewdb_cli_base.h>
#include <ewdb_ew_oci_base.h>
#include <ewdb_internal_functions.h>


static char SQL_STRING[] =
  "Begin Update_Waveform_Desc(OUT_RetCode => :OUT_RetCode, "
  "IN_idWaveform => :IN_idWaveform, IN_tStart => :IN_tStart, "
  "IN_tEnd => :IN_tEnd, IN_iDataFormat => :IN_iDataFormat, "
  "IN_iByteLen => :IN_iByteLen); End;";

static EWDB_OCI_SFS SQLParamsBindArray[] = 
{
  {0,1,0,0,0,OA_INT,":OUT_RetCode"},
  {0,1,0,0,0,OA_EWDBID,":IN_idWaveform"},
  {0,1,0,0,0,OA_SZ,":IN_tStart"},
  {0,1,0,0,0,OA_SZ,":IN_tEnd"},
  {0,1,0,0,0,OA_INT,":IN_iDataFormat"},
  {0,1,0,0,0,OA_INT,":IN_iByteLen"}
};

#define  NUM_FIELDS  6



static char IN_tStart[20],IN_tEnd[20];
static  int RetCode;

static  EWDB_WaveformStruct  LocalWaveformStruct;

/* Insertion Struct for UpdateWaveformDesc statement */
static EWDB_OCIStatementStruct SSStatement;


/********************************
      FUNCTION PROTOTYPES
********************************/
int PrepUpdateWaveformDescExec(EWDB_WaveformStruct *pWaveform, 
                               EWDB_Cursor *ppCursor);
int PostUpdateWaveformDescExec(EWDB_WaveformStruct *pWaveform);
int InitUpdateWaveformDescStatement(char *statement, 
                                    EWDB_OCIStatementStruct *pSS);
/*******************************/


int ewdb_internal_UpdateWaveformDesc(EWDB_WaveformStruct *pWaveform)
{
  EWDB_Cursor pCursor;

  if(pWaveform == NULL)
  {
    logit("", "ewdb_internal_UpdateWaveformDesc(): Null parameters passed in!\n");
    return EWDB_RETURN_FAILURE;
  }

  if(ewdb_base_Reconnect() != EWDB_RETURN_SUCCESS)
  /* Re-establishes connection */
  {
    logit("", "ewdb_internal_UpdateWaveformDesc(): "
               "Could not reconnect to the database!\n");
    return(EWDB_RETURN_FAILURE);
  }

  if(PrepUpdateWaveformDescExec(pWaveform, &pCursor) 
      != EWDB_RETURN_SUCCESS)
  {
    logit("", "ORA_API:ewdb_internal_UpdateWaveformDesc(): "
               "PrepUpdateWaveformDescExec() failed.\n");
    /* DK 02/13/2001 removed call to disconnect, as that causes a
       commit to happen */
    return(EWDB_RETURN_FAILURE);
  }

  if(ewdb_base_SQLExecute(pCursor))
  {
    ewdb_base_ErrorReport(hEWDBC, pCursor,"ewdb_internal_UpdateWaveformDesc:ewdb_base_SQLExecute", 1);
    /* DK 02/13/2001 removed call to disconnect, as that causes a
       commit to happen */
    return(EWDB_RETURN_FAILURE);
  } 

  /* CHANGED: DavidK 05/08/2000 Removed commit.*/
  /* Do not commit the transaction yet.  This insertion is
     part of a larger transaction.  Someone above us in the
     food changed will do the commit, once they are sure that
     all of the req'd data is in. (This probably just means
     that the snippet that matches this waveform descriptor
     needs to also be successfully inserted into the DB.)
   **********************************************************/

  if(PostUpdateWaveformDescExec(pWaveform) == EWDB_RETURN_FAILURE)
  {
    logit("", "Call to PostUpdateWaveformDescExec failed!\n");
    /* DK 02/13/2001 removed call to disconnect, as that causes a
       commit to happen */
    return(EWDB_RETURN_FAILURE);
  }
  
  return(EWDB_RETURN_SUCCESS);
}  /* end ewdb_internal_UpdateWaveformDesc() */


int InitUpdateWaveformDescStatement(char *statement, 
                                     EWDB_OCIStatementStruct *pSS)
{

  if((statement == NULL) ||(pSS == NULL))
  {
    logit("", "InitUpdateWaveformDescStatement(): "
           "Null parameters passed in!\n");
    return EWDB_RETURN_FAILURE;
  }

  pSS->FieldArray[0].pVal = &(RetCode);
  pSS->FieldArray[1].pVal = &(LocalWaveformStruct.idWaveform);
  pSS->FieldArray[2].pVal = IN_tStart;
  pSS->FieldArray[3].pVal = IN_tEnd;
  pSS->FieldArray[4].pVal = &(LocalWaveformStruct.iDataFormat);
  pSS->FieldArray[5].pVal = &(LocalWaveformStruct.iByteLen);

  if(ewdb_base_RequestCursor(statement, pSS, 0) != EWDB_RETURN_SUCCESS)
    {
        logit("", "InitUpdateWaveformDescStatement(): "
               "Call to ewdb_base_RequestCursor failed.\n");
        return EWDB_RETURN_FAILURE;
    }

  return(EWDB_RETURN_SUCCESS);
}  /* End of InitUpdateWaveformDescStatement() */


int PrepUpdateWaveformDescExec(EWDB_WaveformStruct *pWaveform, 
                               EWDB_Cursor *ppCursor)
{

  if((pWaveform == NULL) ||(ppCursor == NULL))
  {
    logit("", "Null parameters passed in!\n");
    return EWDB_RETURN_FAILURE;
  }


  SSStatement.NumOfFields = NUM_FIELDS;
  SSStatement.FieldArray = SQLParamsBindArray;
  SSStatement.RecordSize = 0;

  /* Copy the incomming struct into the local struct */
  memcpy(&LocalWaveformStruct, pWaveform, sizeof(EWDB_WaveformStruct));

  sprintf(IN_tStart,"%.4f",pWaveform->tStart);
  sprintf(IN_tEnd,"%.4f",pWaveform->tEnd);

  if(InitUpdateWaveformDescStatement(SQL_STRING, &SSStatement) 
      != EWDB_RETURN_SUCCESS)
  {
    logit("", "Call to InitUpdateWaveformDescStatement failed!\n");
    return EWDB_RETURN_FAILURE;
  }

  *ppCursor = SSStatement.pCda;
  
  return(EWDB_RETURN_SUCCESS);
}  /* End PrepUpdateWaveformDescExec() */


int PostUpdateWaveformDescExec(EWDB_WaveformStruct *pWaveform)
{
  EWDB_Cursor pCursor;
  
  pCursor = SSStatement.pCda;
  
  
  ewdb_base_ReleaseCursor(pCursor);
  
  if(RetCode < 0)
  {
    if(RetCode == -1)
      logit("","PostUpdateWaveformDescExec():  SQL Proc Update_Waveform_Desc() "
               "returned \"Unknown Error\"!\n");
/*    
    else if(LocalWaveformStruct.idWaveform == -8)
      logit("","PostUpdateWaveformDescExec():  SQL Proc Update_Waveform_Desc() "
               "returned \"Invalid Event\"!  bBindToEvent was set to TRUE, "
               "but idEvent did not match an existing Event in the DB.\n");
*/
    else
      logit("","PostUpdateWaveformDescExec():  SQL Proc Update_Waveform_Desc() "
               "returned error(%d)!\n",
            RetCode);
    return(EWDB_RETURN_FAILURE);
  }
  else if(RetCode > 0)
  {
      logit("","PostUpdateWaveformDescExec():  SQL Proc Update_Waveform_Desc() "
               "returned warning(%d)!\n",
            RetCode);
    return(EWDB_RETURN_WARNING);
  }
  else
  {
    return(EWDB_RETURN_SUCCESS);
  }
}  /* End PostUpdateWaveformDescExec() */
