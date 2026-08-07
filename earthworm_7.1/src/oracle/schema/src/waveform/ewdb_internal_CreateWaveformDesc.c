
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_internal_CreateWaveformDesc.c,v 1.3 2003/02/04 17:57:03 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_internal_CreateWaveformDesc.c,v $
 *     Revision 1.3  2003/02/04 17:57:03  davidk
 *     Added a call to release the cursor used by the function, when the function fails.
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


static char SQL_STRING[] =
  "Begin Create_Waveform_Desc(OUT_idWaveform => :OUT_idWaveform, "
  "IN_idChan => :IN_idChan, IN_tStart => :IN_tStart, "
  "IN_tEnd => :IN_tEnd, IN_iDataFormat => :IN_iDataFormat, "
  "IN_iByteLen => :IN_iByteLen, IN_bBindToEvent => :IN_bBindToEvent, "
  "IN_idEvent => :IN_idEvent); End;";

static EWDB_OCI_SFS SQLParamsBindArray[] = 
{
  {0,1,0,0,0,OA_INT,":OUT_idWaveform"},
  {0,1,0,0,0,OA_INT,":IN_idChan"},
  {0,1,0,0,0,OA_SZ,":IN_tStart"},
  {0,1,0,0,0,OA_SZ,":IN_tEnd"},
  {0,1,0,0,0,OA_INT,":IN_iDataFormat"},
  {0,1,0,0,0,OA_INT,":IN_iByteLen"},
  {0,1,0,0,0,OA_INT,":IN_bBindToEvent"},
  {0,1,0,0,0,OA_INT,":IN_idEvent"},
};

#define  NUM_FIELDS  8



static char IN_tStart[20],IN_tEnd[20];
static EWDBid IN_idEvent;
static int IN_bBindToEvent;


static  EWDB_WaveformStruct  LocalWaveformStruct;

/* Insertion Struct for CreateWaveformDesc statement */
static EWDB_OCIStatementStruct SSStatement;


/********************************
      FUNCTION PROTOTYPES
********************************/
int PrepCreateWaveformDescExec(EWDB_WaveformStruct *pWaveform, 
                               int bBindToEvent, EWDBid idEvent,
                               EWDB_Cursor *ppCursor);
int PostCreateWaveformDescExec(EWDB_WaveformStruct *pWaveform);
int InitCreateWaveformDescStatement(char *statement, 
                                    EWDB_OCIStatementStruct *pSS);
/*******************************/


int ewdb_internal_CreateWaveformDesc(EWDB_WaveformStruct *pWaveform, 
                                     int bBindToEvent, EWDBid idEvent)
{
  EWDB_Cursor pCursor;

  if(pWaveform == NULL)
  {
    logit("", "ewdb_internal_CreateWaveformDesc(): Null parameters passed in!\n");
    return EWDB_RETURN_FAILURE;
  }

  if(ewdb_base_Reconnect() != EWDB_RETURN_SUCCESS)
  /* Re-establishes connection */
  {
    logit("", "ewdb_internal_CreateWaveformDesc(): "
               "Could not reconnect to the database!\n");
    return(EWDB_RETURN_FAILURE);
  }

  if(PrepCreateWaveformDescExec(pWaveform, bBindToEvent,  
                                  idEvent, &pCursor) 
      != EWDB_RETURN_SUCCESS)
  {
    logit("", "ORA_API:ewdb_internal_CreateWaveformDesc(): "
               "PrepCreateWaveformDescExec() failed.\n");
    /* DK 02/13/2001 removed call to disconnect, as that causes a
       commit to happen */
    return(EWDB_RETURN_FAILURE);
  }

  if(ewdb_base_SQLExecute(pCursor))
  {
    ewdb_base_ErrorReport(hEWDBC, pCursor,"ewdb_internal_CreateWaveformDesc:ewdb_base_SQLExecute", 1);
    /* DK 02/13/2001 removed call to disconnect, as that causes a
       commit to happen */
    /* DK 02/03/2003 added a call to release the cursor.  Normally we would
       do a disconnect at this point, but since that forces a commit, and we
       want a chance to do the rollback in the ewdb_api_CreateWaveform()
       function, we cannot do the disconnect here.  
       We could do the disconnect in ewdb_api_CreateWaveform(), but unfortunately
       we don't get enough information back in ewdb_api_CreateWaveform() to figure
       out if the error is bad enough to disconnect over.
       One of the things the disconnect does is clear out the cursors.  By not 
       disconnecting and not closing the cursor, we cause a cursor leak.  
       So, we atleast need to release the cursor here, even if we don't do anything
       else.
    */
    ewdb_base_ReleaseCursor(pCursor);
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

  if(PostCreateWaveformDescExec(pWaveform) != EWDB_RETURN_SUCCESS)
  {
    logit("", "Call to PostCreateWaveformDescExec failed!\n");
    /* DK 02/13/2001 removed call to disconnect, as that causes a
       commit to happen */
    return(EWDB_RETURN_FAILURE);
  }
  
  return(EWDB_RETURN_SUCCESS);
}  


int InitCreateWaveformDescStatement(char *statement, 
                                     EWDB_OCIStatementStruct *pSS)
{

  if((statement == NULL) ||(pSS == NULL))
  {
    logit("", "InitCreateWaveformDescStatement(): "
           "Null parameters passed in!\n");
    return EWDB_RETURN_FAILURE;
  }

  pSS->FieldArray[0].pVal = &(LocalWaveformStruct.idWaveform);
  pSS->FieldArray[1].pVal = &(LocalWaveformStruct.idChan);
  pSS->FieldArray[2].pVal = IN_tStart;
  pSS->FieldArray[3].pVal = IN_tEnd;
  pSS->FieldArray[4].pVal = &(LocalWaveformStruct.iDataFormat);
  pSS->FieldArray[5].pVal = &(LocalWaveformStruct.iByteLen);
  pSS->FieldArray[6].pVal = &IN_bBindToEvent;
  pSS->FieldArray[7].pVal = &IN_idEvent;

  if(ewdb_base_RequestCursor(statement, pSS, 0) != EWDB_RETURN_SUCCESS)
    {
        logit("", "InitCreateWaveformDescStatement(): "
               "Call to ewdb_base_RequestCursor failed.\n");
        return EWDB_RETURN_FAILURE;
    }

  return(EWDB_RETURN_SUCCESS);
}  /* End of InitCreateWaveformDescStatement() */


int PrepCreateWaveformDescExec(EWDB_WaveformStruct *pWaveform, 
                               int bBindToEvent, EWDBid idEvent,
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

  IN_bBindToEvent=bBindToEvent;
  IN_idEvent=idEvent;
  sprintf(IN_tStart,"%.4f",pWaveform->tStart);
  sprintf(IN_tEnd,"%.4f",pWaveform->tEnd);

  if(InitCreateWaveformDescStatement(SQL_STRING,
                                       &SSStatement) 
      != EWDB_RETURN_SUCCESS)
  {
    logit("", "Call to InitCreateWaveformDescStatement failed!\n");
    return EWDB_RETURN_FAILURE;
  }

  *ppCursor = SSStatement.pCda;
  
  return(EWDB_RETURN_SUCCESS);
}  /* End PrepCreateWaveformDescExec() */


int PostCreateWaveformDescExec(EWDB_WaveformStruct *pWaveform)
{
  EWDB_Cursor pCursor;
  
  pCursor = SSStatement.pCda;
  
  /* Copy the values back to the caller's structure */
  pWaveform->idWaveform=LocalWaveformStruct.idWaveform;
  
  ewdb_base_ReleaseCursor(pCursor);
  
  if(LocalWaveformStruct.idWaveform <= 0)
  {
    if(LocalWaveformStruct.idWaveform == -1)
      logit("","PostCreateWaveformDescExec():  SQL Proc Create_Waveform_Desc() "
               "returned \"Unknown Error\"!\n");
    else if(LocalWaveformStruct.idWaveform == -8)
      logit("","PostCreateWaveformDescExec():  SQL Proc Create_Waveform_Desc() "
               "returned \"Invalid Event\"!  bBindToEvent was set to TRUE, "
               "but idEvent did not match an existing Event in the DB.\n");
    else
      logit("","PostCreateWaveformDescExec():  SQL Proc Create_Waveform_Desc() "
               "returned error(%d)!\n",
            LocalWaveformStruct.idWaveform);
    return(EWDB_RETURN_WARNING);
  }

  return(EWDB_RETURN_SUCCESS);
}  /* End PostCreateWaveformDescExec() */
