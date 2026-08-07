/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_api_GetAmpsByPick.c,v 1.4 2005/06/27 15:31:36 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_api_GetAmpsByPick.c,v $
 *     Revision 1.4  2005/06/27 15:31:36  davidk
 *     Fixed bugs introduced during global fetch/replace during DB cleanup.
 *
 *     Revision 1.3  2005/06/17 21:00:03  davidk
 *     Fixed problems in SQL code caused by errant global replacements during API cleanup.
 *
 *     Revision 1.2  2005/06/15 19:03:12  davidk
 *     DB API Cleanup
 *
 *     Revision 1.1  2003/09/16 16:56:55  davidk
 *     Initial revision
 *
 *
 */


#include <ewdb_cli_base.h>
#include <ewdb_ora_api.h>
#include <ewdb_ew_oci_base.h>

static char SQL_STRING[] =
         /* To get all amps with certain Local_idPick */
         "select idPeakAmp, idChan, idPick, dPeakAmp1, tPeriod1, tAmp1, "
		 " dPeakAmp2, tPeriod2, tAmp2, tStartInterval, tEndInterval "
         "from ALL_PEAKAMPS "
         "where idPick = :IN_idPick AND iMagType = :IN_iMagType";

static EWDB_OCI_SFS SQLParamsBindArray[] = 
{

  {0,1,0,0,0,OA_EWDBID,"1idPeakAmp"},
  {0,1,0,0,0,OA_EWDBID,"2idChan"},
  {0,1,0,0,0,OA_EWDBID,"3idPick"},
  {0,1,20,0,0,OA_FLOAT,"4dPeakAmp1"},
  {0,1,20,0,0,OA_FLOAT,"5tPeriod1"},
  {0,1,20,0,0,OA_DOUBLE,"6tAmp1"},
  {0,1,20,0,0,OA_FLOAT,"7dPeakAmp2"},
  {0,1,20,0,0,OA_FLOAT,"8tPeriod2"},
  {0,1,20,0,0,OA_DOUBLE,"9tAmp2"},
  {0,1,20,0,0,OA_DOUBLE,"10tStartInterval"},
  {0,1,20,0,0,OA_DOUBLE,"11tEndInterval"},
  {0,1,0,0,0,OA_EWDBID,":IN_idPick"},
  {0,1,0,0,0,OA_INT,   ":IN_iMagType"}
};

/* define the total number of fields to be bound */
#define NUM_FIELDS 13

/* Insertion Struct for GetAmpsByPick statement */
static EWDB_OCIStatementStruct SSStatement;

/* Temporary Storage Buffer params */
#define BUFFERSIZE 8192
static int    iRecordSize;
static int    iRecordsPerBuffer;
static char * pLocalBuffer=NULL;

static int    bInitialized=FALSE;

static  int       Local_iMagType;
static  EWDBid    Local_idPick;

/****************************************************************/
/****************************************************************/
/********************************
      FUNCTION PROTOTYPES
********************************/
static int PrepGetAmpsByPickExec(EWDBid IN_idPick, int IN_iMagType, EWDB_Cursor * ppCursor);
static int PostGetAmpsByPickExec(EWDB_PeakAmpStruct * pBuffer, int BufferRecLen);
static int InitGetAmpsByPickStatement(char * szStatement, 
                                EWDB_OCIStatementStruct *pSS);


/* 
   Retrieves a list of peak amplitudes for a given Local_idPick value 
				and amplitude type.

  pBuffer is a pointer to a buffer allocated by the caller
   where the function is to stick the stations.
  iBufferLen is the length of the buffer in EWDB_PeakAmpStruct allocated 
   by the caller.

    Return Value:   
     EWDB_RETURN_SUCCESS   Success.
     EWDB_RETURN_FAILURE   Error
     EWDB_RETURN_WARNING   WARNING: The buffer passed by the caller is too
                           small, where *pNumItemsFound is the neccessary 
                           size of iBufferLen to get all of the picks for the 
                           given origin.
                           Results that fit in the buffer are passed back.

    Other Details:  Caller is responsible for allocating
                    space for the amps buffer.
*/
int ewdb_api_GetAmpsByPick(EWDBid IN_idPick, MAGNITUDE_TYPE IN_iMagType, 
                           EWDB_PeakAmpStruct * pAmpBuffer,
                           int * pNumItemsFound, int * pNumItemsRetrieved, int iBufferLen)
{

  EWDB_Cursor  pCursor;
 
  ewdb_base_SetLastOraAPIActionTime();

  if(IN_idPick <= 0 || !pAmpBuffer || !pNumItemsFound || !pNumItemsRetrieved)
  {
    logit("","ewdb_api_GetAmpsByPick(): ERROR!  Invalid parameter(s) passed in: "
             "IN_idPick %d, pAmpBuffer %u, pNumItemsFound %u, pNumItemsRetrieved %u\n",
          IN_idPick, pAmpBuffer, pNumItemsFound, pNumItemsRetrieved);
    return(EWDB_RETURN_FAILURE);
  }

  if( ewdb_base_Reconnect() != EWDB_RETURN_SUCCESS )
  {
    return( EWDB_RETURN_FAILURE );
  }

  if( PrepGetAmpsByPickExec(IN_idPick, IN_iMagType, &pCursor) != EWDB_RETURN_SUCCESS)
  {
    logit("","ewdb_api_GetAmpsByPick(): PrepGetAmpsByPickExec() failed!\n");
    return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }

  if( ewdb_base_SQLExecute(pCursor) )
  {
    ewdb_base_ErrorReport(hEWDBC, pCursor,"ewdb_api_GetAmpsByPick(): ewdb_base_SQLExecute",1);
    return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  } 

 /* Commit the transaction (all the previous inserts!)
  In Case there is any auditing or logging or debug
  changes made in the stored procedures.
  ****************************************************/
  if (ewdb_base_SQLCommit (hEWDBC))
  {
    ewdb_base_ErrorReport (hEWDBC, hEWDBC,"ewdb_api_GetAmpsByPick(): ewdb_base_SQLCommit",2);
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  }
 
  ewdb_base_SetLastOraAPIActionTime();

  if((*pNumItemsFound = PostGetAmpsByPickExec(pAmpBuffer,iBufferLen)) 
     == EWDB_RETURN_FAILURE)
  {
    logit("","ewdb_api_GetAmpsByPick(): PostGetAmpsByPickExec() failed!\n");
    return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }

  ewdb_base_SetLastOraAPIActionTime();

  if(*pNumItemsFound <= iBufferLen)
  {
    *pNumItemsRetrieved = *pNumItemsFound;
    return(EWDB_RETURN_SUCCESS);
  }
  else
  {
    *pNumItemsRetrieved = iBufferLen;
    return(EWDB_RETURN_WARNING);
  }
}  /* end ewdb_internal_GetAmpsByPick() */


static int InitGetAmpsByPickStatement(char * szStatement, 
                               EWDB_OCIStatementStruct *pSS)
{
  int LastSize,i;  /* Size of last column array */

  if(!pLocalBuffer)
  {
    /*Allocate space for returned data - never freed */
    pLocalBuffer=malloc(BUFFERSIZE);

    iRecordSize=0;
    iRecordSize += sizeof(EWDBid); /*idPeakAmp*/
    iRecordSize += sizeof(EWDBid); /*idChan*/
    iRecordSize += sizeof(EWDBid); /*Local_idPick*/
    iRecordSize += pSS->FieldArray[2].Ind; /*dAmp1*/
    iRecordSize += pSS->FieldArray[3].Ind; /*tPeriod1*/
    iRecordSize += pSS->FieldArray[4].Ind; /*tAmp1*/
    iRecordSize += pSS->FieldArray[5].Ind; /*dAmp2*/
    iRecordSize += pSS->FieldArray[6].Ind; /*tPeriod2*/
    iRecordSize += pSS->FieldArray[7].Ind; /*tAmp2*/
    iRecordSize += pSS->FieldArray[8].Ind; /*tStartInterval*/
    iRecordSize += pSS->FieldArray[9].Ind; /*tEndInterval*/
    
    /* Make the number of records a multiple of 8 to avoid alignment problems on UNIX */
    iRecordsPerBuffer= (BUFFERSIZE/iRecordSize) & 0xfffffff8;
    
    /*Allocate space for row/col ret lens. - never freed */
    for(i=0;i<pSS->NumOfFields;i++)
    {
      pSS->FieldArray[i].pRetLens=malloc(iRecordsPerBuffer*EWDB_FIELD_RET_LEN);
    }

    pSS->FieldArray[0].pVal=&(pLocalBuffer[0]);
    LastSize=sizeof(EWDBid);	/* idPeakAmp */
    pSS->FieldArray[1].pVal=(void *)(
     (int)(pSS->FieldArray[0].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=sizeof(EWDBid);	/* idChan */
    pSS->FieldArray[2].pVal=(void *)(
     (int)(pSS->FieldArray[1].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=sizeof(EWDBid);	/* Local_idPick */
    pSS->FieldArray[3].pVal=(void *)(
     (int)(pSS->FieldArray[2].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[3].Ind; /* dPeakAmp1 */
    pSS->FieldArray[4].pVal=(void *)(
     (int)(pSS->FieldArray[3].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[4].Ind; /* tPeriod1 */
    pSS->FieldArray[5].pVal=(void *)(
     (int)(pSS->FieldArray[4].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[5].Ind; /* tAmp1 */
    pSS->FieldArray[6].pVal=(void *)(
     (int)(pSS->FieldArray[5].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[6].Ind; /* dPeakAmp2 */
    pSS->FieldArray[7].pVal=(void *)(
     (int)(pSS->FieldArray[6].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[7].Ind; /* tPeriod2 */
    pSS->FieldArray[8].pVal=(void *)(
     (int)(pSS->FieldArray[7].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[8].Ind; /* tAmp2 */
    pSS->FieldArray[9].pVal=(void *)(
     (int)(pSS->FieldArray[8].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[9].Ind; /* tStartInterval */
    pSS->FieldArray[10].pVal=(void *)(
     (int)(pSS->FieldArray[9].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[10].Ind; /* tEndInterval */

    pSS->FieldArray[11].pVal=&Local_idPick;
    pSS->FieldArray[12].pVal=&Local_iMagType;

  } /* end if(!pLocalBuffer) */
  
  if(!pLocalBuffer)
    logit("","InitGetAmpsByPickStatement(): malloc of pLocalBuffer "
          "failed! Returning.\n");

  return(ewdb_base_RequestCursor(szStatement, pSS,0/*don't force rebind*/));
}  /* End InitGetAmpsByPickStatement() */


static int PrepGetAmpsByPickExec(EWDBid IN_idPick, int IN_iMagType, EWDB_Cursor * ppCursor)
{
  Local_idPick     = IN_idPick;
  Local_iMagType   = IN_iMagType;

  /* Initialize the statement parameters */
  if(!bInitialized)
  {
    bInitialized = TRUE;
    memset(&SSStatement,0,sizeof(SSStatement));
    SSStatement.NumOfFields=NUM_FIELDS;
    SSStatement.FieldArray=SQLParamsBindArray;
    SSStatement.RecordSize=0;
  }

  if(InitGetAmpsByPickStatement(SQL_STRING, &SSStatement) != EWDB_RETURN_SUCCESS)
  {
    *ppCursor = NULL;
    return(EWDB_RETURN_FAILURE);
  }

  *ppCursor=SSStatement.pCda;

  return(EWDB_RETURN_SUCCESS);
}  /* End PrepGetAmpsByPickExec() */


static int PostGetAmpsByPickExec(EWDB_PeakAmpStruct * pBuffer, int BufferRecLen)
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
    memset(pLocalBuffer, 0, BUFFERSIZE);

    if(ewdb_base_SQLFetchRows(pCursor, iRecordsPerBuffer))
    {
      if(ewdb_base_GetCursorRetCode(pCursor) == EWDB_SQL_ERROR_NO_DATA) 
      {
        done=1;
      }
      else
      {
        ewdb_base_ErrorReport(hEWDBC, pCursor,"PostGetAmpsByPickExec:ewdb_base_SQLFetchRows",1);
        return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
      }
    }
    RowsRetrieved = ewdb_base_GetCursorRowsProcessedCount(pCursor);

    for(; RowsDone < RowsRetrieved; RowsDone++)
    {
      if(RowsDone >= BufferRecLen)
      {
        done=1;
        break;
      }

      /* CopyRowFromBuffertoUserBuffer */
      {
        BCurr=RowsDone % iRecordsPerBuffer;
        UCurr=RowsDone;


        pBuffer[UCurr].iAmpType=Local_iMagType;
        pBuffer[UCurr].idPeakAmp=*(EWDBid *)
           ((sizeof(int)*BCurr) +(int)(pSS->FieldArray[0].pVal));
        pBuffer[UCurr].idChan=*(EWDBid *)
           ((sizeof(int)*BCurr) +(int)(pSS->FieldArray[1].pVal));
        pBuffer[UCurr].idPick=*(EWDBid *)
           ((sizeof(int)*BCurr) +(int)(pSS->FieldArray[2].pVal));

        /* This looks complicated, but maybe it isn't.
           pTemp is a pointer that is set to a place in the retrieval buffer.
           It's location in the buffer is determined by adding the 
           section-offset to the buffer-offset, where the section offset is 
           the location of the data within this section of the buffer.  The 
           buffer-offset is the location of this section within the entire 
           buffer.  So the buffer-offset points to the beginning of this 
           section, and the section-offset points to the desired location 
           relative to this section.  Thus when they are combined, they 
           point to the desired location relative to the buffer.  
          (And you thought this was complicated)
        ******************************************************************/
        pTemp=(char *) 
           ((pSS->FieldArray[3].Ind*BCurr) +(int)(pSS->FieldArray[3].pVal));
        pTemp[pSS->FieldArray[3].pRetLens[BCurr]]=0;
        pBuffer[UCurr].dAmp1=(float)atof(pTemp);

        pTemp=(char *) 
           ((pSS->FieldArray[4].Ind*BCurr) +(int)(pSS->FieldArray[4].pVal));
        pTemp[pSS->FieldArray[4].pRetLens[BCurr]]=0;
        pBuffer[UCurr].dAmpPeriod1=(float)atof(pTemp);

        pTemp=(char *) 
           ((pSS->FieldArray[5].Ind*BCurr) +(int)(pSS->FieldArray[5].pVal));
        pTemp[pSS->FieldArray[5].pRetLens[BCurr]]=0;
        pBuffer[UCurr].tAmp1=(double)atof(pTemp);

        pTemp=(char *) 
           ((pSS->FieldArray[6].Ind*BCurr) +(int)(pSS->FieldArray[6].pVal));
        pTemp[pSS->FieldArray[6].pRetLens[BCurr]]=0;
        pBuffer[UCurr].dAmp2=(float)atof(pTemp);

        pTemp=(char *) 
           ((pSS->FieldArray[7].Ind*BCurr) +(int)(pSS->FieldArray[7].pVal));
        pTemp[pSS->FieldArray[7].pRetLens[BCurr]]=0;
        pBuffer[UCurr].dAmpPeriod2=(float)atof(pTemp);

        pTemp=(char *) 
           ((pSS->FieldArray[8].Ind*BCurr) +(int)(pSS->FieldArray[8].pVal));
        pTemp[pSS->FieldArray[8].pRetLens[BCurr]]=0;
        pBuffer[UCurr].tAmp2=(double)atof(pTemp);

        pTemp=(char *) 
           ((pSS->FieldArray[9].Ind*BCurr) +(int)(pSS->FieldArray[9].pVal));
        pTemp[pSS->FieldArray[9].pRetLens[BCurr]]=0;
        pBuffer[UCurr].tStartInterval=(double)atof(pTemp);

        pTemp=(char *) 
           ((pSS->FieldArray[10].Ind*BCurr) +(int)(pSS->FieldArray[10].pVal));
        pTemp[pSS->FieldArray[10].pRetLens[BCurr]]=0;
        pBuffer[UCurr].tEndInterval=(double)atof(pTemp);


      }
    } /* End for RowsDone < RowsRetrieved */
  }  /* End while !done */
  
  if(RowsRetrieved > BufferRecLen)
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
      ewdb_base_ErrorReport(hEWDBC, pCursor,"PostGetGetAmpsByPickExec:ewdb_base_SQLFetchRows",2);
      return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
    }
    /* else */
  }  /* End if(RowsRetrieved > BufferRecLen) */
  
  RowsProcessed = ewdb_base_GetCursorRowsProcessedCount(pCursor);

  ewdb_base_ReleaseCursor(pCursor);

  return(RowsProcessed);
}  /* End PostGetGetAmpsByPickExec() */


