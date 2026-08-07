/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_internal_GetSMMotionsForMessage.c,v 1.4 2003/09/16 17:01:59 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_internal_GetSMMotionsForMessage.c,v $
 *     Revision 1.4  2003/09/16 17:01:59  davidk
 *     General API Cleanup of internal data structure.
 *
 *     Revision 1.3  2003/08/21 01:00:24  davidk
 *     Restructured API code that deals with dynamic allocation of memory.
 *
 *     Revision 1.2  2003/06/09 23:05:29  davidk
 *     Removed useless debugging statements.
 *
 *     Revision 1.1  2001/05/15 02:16:42  davidk
 *     Initial revision
 *
 *     Revision 1.2  2001/04/06 19:46:11  davidk
 *     Changed SQL_STRING to static so that it only has file scope, not global.
 *     This was a bug fix as SQL_STRING is supposed to be static.
 *
 *     Revision 1.1  2001/04/06 19:01:00  davidk
 *     Initial revision
 *
 *     Revision 1.1  2001/02/28 17:19:22  lucky
 *     Initial revision
 *
 *     Revision 1.5  2001/02/21 09:23:52  davidk
 *     Code was retrofitted to use the new db_cli_base API
 *     in lieu of the oci_base() and OCI7 function API's.layer, which
 *     provides abstraction from the OCI7 function calls to
 *     ora_api layer code.  See comments in
 *     schema/src/include/internal/ewdb_cli_base.h for more info
 *     on the details of the changes made.
 *
 *     Revision 1.4  2000/05/16 17:11:32  davidk
 *     Added prototypes for the functions implemented in this file.
 *
 *     Revision 1.3  1999/11/09 18:46:12  lucky
 *     *** empty log message ***
 *
 *
 */


#include <stdlib.h>
#include <stdio.h>
#include <ewdb_cli_base.h>
#include <ewdb_ora_api.h>
#include <ewdb_ew_oci_base.h>
#include <ewdb_sm_internal.h>


static char SQL_STRING[] =
/* To get all channels from a certain message */
/* To get measurements for a Message */
  "select iMotionType, dPeriod, dMeasurement "
  " from ALL_SMMOTION_INFO "
  " where idSMMessage = :IN_idSMMessage "
  " order by dPeriod";


static EWDB_OCI_SFS SQLParamsBindArray[] = 
{
  {0,1,0,0,0,OA_INT,"1iMotionType"},
  {0,1,20,0,0,OA_DOUBLE,"2dPeriod"},
  {0,1,20,0,0,OA_DOUBLE,"3dMeasurement"},
  {0,1,0,0,0,OA_INT,":IN_idSMMessage"}
};

/* define the total number of fields to be bound */
#define NUM_FIELDS 4

/* Insertion Struct for statement */
static EWDB_OCIStatementStruct SSStatement;

/* Temporary Storage pLocalBuffer params */
#define BUFFERSIZE 8192
static int    iRecordSize;
static int    iRecordsPerBuffer;
static char * pLocalBuffer=NULL;

static int    bInitialized=FALSE;

static int  idSMMessage;

int PrepGetSMMotionsForMessageExec(EWDBid IN_idSMMessage, EWDB_Cursor * ppCursor);
int PostGetSMMotionsForMessageExec(EWDB_SMMotionStruct * pMotion, int iBufferRecLen,
                      int * pNumRecordsFound, int * pNumRecordsRetrieved);
int InitGetSMMotionsForMessageStatement(char * szStatement, EWDB_OCIStatementStruct *pSS);
/****************************************************************/
/****************************************************************/
int ewdb_internal_GetSMMotionsForMessage(EWDBid idSMMessage, 
                                         EWDB_SMMotionStruct * pBuffer,
                                         int iBufferRecLen, 
                                         int * pNumRecordsFound,
                                         int * pNumRecordsRetrieved)
{

  EWDB_Cursor  pCursor;
 
  ewdb_base_SetLastOraAPIActionTime();

  if(!(pBuffer && pNumRecordsFound && pNumRecordsRetrieved))
  {
    logit("","ewdb_internal_GetSMMotionsForMessage(): ERROR  NULL pointers passed as params!\n");
    return(EWDB_RETURN_FAILURE);
  }

  if(ewdb_base_Reconnect() != EWDB_RETURN_SUCCESS)
    /* Establishes connection, and performs binding!?! */
  {
    return( EWDB_RETURN_FAILURE );
  }

  if( PrepGetSMMotionsForMessageExec(idSMMessage,&pCursor)
     != EWDB_RETURN_SUCCESS)
  {
    return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }

  
  if( ewdb_base_SQLExecute(pCursor) )
  {
    ewdb_base_ErrorReport(hEWDBC, pCursor,"ewdb_internal_GetSMMotionsForMessage:ewdb_base_SQLExecute",1);
    return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  } 

  if(PostGetSMMotionsForMessageExec(pBuffer, iBufferRecLen, 
                       pNumRecordsFound, pNumRecordsRetrieved)
     == EWDB_RETURN_FAILURE)
  {
    return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }

  ewdb_base_SetLastOraAPIActionTime();
  return(EWDB_RETURN_SUCCESS);
}  


int InitGetSMMotionsForMessageStatement(char * szStatement, EWDB_OCIStatementStruct *pSS)
{

  int LastSize,i;  /* Size of last column array */


  if(!pLocalBuffer)
  {
    /*Allocate space for returned data - never freed */
    pLocalBuffer=malloc(BUFFERSIZE);


    iRecordSize=0;
    iRecordSize += sizeof(int);
    iRecordSize += pSS->FieldArray[1].Ind; /*dPeriod*/
    iRecordSize += pSS->FieldArray[2].Ind; /*dMeasurement*/

    /* Make the number of records a multiple of 8 to avoid alignment problems on UNIX */
    iRecordsPerBuffer= (BUFFERSIZE/iRecordSize) & 0xfffffff8;
    
    /*Allocate space for row/col ret lens. - never freed */
    for(i=0;i<pSS->NumOfFields;i++)
    {
      pSS->FieldArray[i].pRetLens=malloc(iRecordsPerBuffer*EWDB_FIELD_RET_LEN);
    }



    pSS->FieldArray[0].pVal=&(pLocalBuffer[0]);
    LastSize=sizeof(int);
    pSS->FieldArray[1].pVal= (void *) (
      (int)(pSS->FieldArray[0].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[1].Ind;
    pSS->FieldArray[2].pVal= (void *) (
      (int)(pSS->FieldArray[1].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[2].Ind;
    
    pSS->FieldArray[3].pVal=&idSMMessage;

  } /* end if(!pLocalBuffer) */
  
  if(!pLocalBuffer)
    logit("","InitGetSMMotionsForMessageStatement: malloc of pLocalBuffer "
          "failed! Returning.\n");


  ewdb_base_RequestCursor(szStatement, pSS,0/*don't force rebind*/);

  return(EWDB_RETURN_SUCCESS);
}  /* end InitGetSMMotionsForMessageStatement() */


int PrepGetSMMotionsForMessageExec(EWDBid IN_idSMMessage, EWDB_Cursor * ppCursor)
{
  idSMMessage = IN_idSMMessage;

  /* Initialize the statement parameters */
  if(!bInitialized)
  {
    bInitialized = TRUE;
    memset(&SSStatement,0,sizeof(SSStatement));
    SSStatement.NumOfFields=NUM_FIELDS;
    SSStatement.FieldArray=SQLParamsBindArray;
    SSStatement.RecordSize=0;
  }

  InitGetSMMotionsForMessageStatement(SQL_STRING, &SSStatement);

  *ppCursor = SSStatement.pCda;

  return(EWDB_RETURN_SUCCESS);
}

int PostGetSMMotionsForMessageExec(EWDB_SMMotionStruct * pMotion, int iBufferRecLen,
                      int * pNumRecordsFound, int * pNumRecordsRetrieved)
{
    /* 
    The statement has been executed.  we need to do the following:
    1.  While we haven't fetched all of the records or reached capacity,
        fetch a new bunch of records into the buffer.
    2.  For each bunch:  Format each row into SnipDescStruct format, 
        and copy it into the UserBuffer.
    3.  Repeat step 2. until we have reached capacity in the 
        UserBuffer or have copied all of the records from the 
        bunch.  Then we are done for the bunch.
    4.  If there are more records available, and we haven't reached
        capacity, go back to step 1.  Otherwise go to step 5.
    5.  Figure out why we stopped processing records.  If we reached
        capacity, then find out how many records were actually available
        and return that amount.  If we processed all records, return the
        amount that we processed.
    6.  Done.
    */

  int done=0;
  int RowsRetrieved=0;
  int RowsDone=0;
  EWDB_Cursor  pCursor=SSStatement.pCda;
  char * pTemp;
  int BCurr,UCurr;
  EWDB_OCIStatementStruct * pSS=&SSStatement;
  int RowsProcessed;

  while(!done)
  {
    if (ewdb_base_SQLFetchRows(pCursor, iRecordsPerBuffer))
    {
      if (ewdb_base_GetCursorRetCode(pCursor) == EWDB_SQL_ERROR_NO_DATA) 
      {
        done=1;
      }
      else
      {
        ewdb_base_ErrorReport(hEWDBC, pCursor,"PostGetSMMotionsForMessageExec:ewdb_base_SQLFetchRows",1);
        return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
      }
    }
    RowsRetrieved = ewdb_base_GetCursorRowsProcessedCount(pCursor);
    
    for(; RowsDone < RowsRetrieved; RowsDone++)
    {
      /* CopyRowFromBuffertoUserBuffer */
      BCurr=RowsDone % iRecordsPerBuffer;
      UCurr=RowsDone;

      pMotion[UCurr].iMotionType = * (int *)((sizeof(int)*BCurr) + (int)(pSS->FieldArray[0].pVal));

      pTemp=(char *) ((pSS->FieldArray[1].Ind*BCurr) + (int)(pSS->FieldArray[1].pVal) );
      pTemp[pSS->FieldArray[1].pRetLens[BCurr]]=0;
      pMotion[UCurr].dPeriod = atof(pTemp);

      pTemp=(char *) ((pSS->FieldArray[2].Ind*BCurr) + (int)(pSS->FieldArray[2].pVal) );
      pTemp[pSS->FieldArray[2].pRetLens[BCurr]]=0;
      pMotion[UCurr].dMeasurement = atof(pTemp);

    } /* End for RowsDone < RowsRetrieved */
  }  /* End while !done */
  
  if (RowsRetrieved > iBufferRecLen)
  {
    /* keep going till we get all of the rows,
       but ignore the contents, since we've
       filled up the user's buffer.
    */
    if(ewdb_base_GetCursorRetCode(pCursor) != EWDB_SQL_ERROR_NO_DATA)
    {
      while(!ewdb_base_SQLFetchRows(pCursor, iRecordsPerBuffer));
    }

    if(ewdb_base_GetCursorRetCode(pCursor) != EWDB_SQL_ERROR_NO_DATA)
    {
      ewdb_base_ErrorReport(hEWDBC, pCursor,"PostGetSMMotionsForMessageExec:ewdb_base_SQLFetchRows",2);
      return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
    }
    /* else */
  }  /* End if (RowsRetrieved > BufferRecLen) */
  
  RowsProcessed = ewdb_base_GetCursorRowsProcessedCount(pCursor);

  *pNumRecordsRetrieved = RowsDone;
  *pNumRecordsFound = RowsProcessed;

  ewdb_base_ReleaseCursor(pCursor);

  return(EWDB_RETURN_SUCCESS);
}  /* end PostGetSMMotionsForMessageExec() */
