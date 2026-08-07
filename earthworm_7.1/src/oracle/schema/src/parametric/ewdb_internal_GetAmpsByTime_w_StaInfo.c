/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_internal_GetAmpsByTime_w_StaInfo.c,v 1.6 2005/06/15 19:03:12 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_internal_GetAmpsByTime_w_StaInfo.c,v $
 *     Revision 1.6  2005/06/15 19:03:12  davidk
 *     DB API Cleanup
 *
 *     Revision 1.5  2003/10/20 17:15:27  mark
 *     Changed DB calls to match correct Oracle views, elements
 *
 *     Revision 1.4  2003/09/16 16:56:55  davidk
 *     General API Cleanup
 *
 *     Revision 1.3  2003/08/25 18:08:04  lucky
 *     Added support for tStartInterval and tEndInterval in PeakAmpStruct
 *
 *     Revision 1.2  2003/08/21 00:58:48  davidk
 *     Restructured API code that deals with dynamic allocation of memory.
 *
 *     *** empty log message ***
 *
 *
 */


#include <ewdb_cli_base.h>
#include <ewdb_ora_api.h>
#include <ewdb_ew_oci_base.h>

static char SQL_STRING[] =
         /* To get all messages from a certain time */
         "select idPeakAmp, idChan, dPeakAmp1, tPeriod1, tAmp1, dPeakAmp2, tPeriod2, tAmp2, "
         "  idCompt, sSta, sComp, sNet, sLoc, dAzm, dDip, dLat, dLon, dElev, "
         "  tOn, tOff "
         "from ALL_PEAKAMPS_W_CHAN_INFO "
         "where tAmp1 >= :IN_tStart AND tAmp1 <= :IN_tEnd AND Local_iMagType = :IN_iMagType";

static EWDB_OCI_SFS SQLParamsBindArray[] = 
{

  {0,1,0,0,0,OA_EWDBID, "1idPeakAmp"},
  {0,1,0,0,0,OA_EWDBID, "2idChan"},
  {0,1,20,0,0,OA_FLOAT, "3dPeakAmp1"},
  {0,1,20,0,0,OA_FLOAT, "4tPeriod1"},
  {0,1,20,0,0,OA_DOUBLE,"5tAmp1"},
  {0,1,20,0,0,OA_FLOAT, "6dPeakAmp2"},
  {0,1,20,0,0,OA_FLOAT, "7tPeriod2"},
  {0,1,20,0,0,OA_DOUBLE,"8tAmp2"},
  {0,1,0,0,0, OA_EWDBID,"9idCompt"},
  {0,1,10,0,0,OA_SZ,    "10sSta"},
  {0,1,10,0,0,OA_SZ,    "11sComp"},
  {0,1,10,0,0,OA_SZ,    "12sNet"},
  {0,1,10,0,0,OA_SZ,    "13sLoc"},
  {0,1,20,0,0,OA_FLOAT, "14dAzm"},
  {0,1,20,0,0,OA_FLOAT, "15dDip"},
  {0,1,20,0,0,OA_DOUBLE,"16dLat"},
  {0,1,20,0,0,OA_DOUBLE,"17dLon"},
  {0,1,20,0,0,OA_DOUBLE,"18dElev"},
  {0,1,20,0,0,OA_DOUBLE,"19tOn"},
  {0,1,20,0,0,OA_DOUBLE,"20tOff"},
  {0,1,0,0,0,OA_INT,    ":IN_tStart"},
  {0,1,0,0,0,OA_INT,    ":IN_tEnd"},
  {0,1,0,0,0,OA_INT,    ":IN_iMagType"},

};

/* define the total number of fields to be bound */
#define NUM_FIELDS 23

/* Insertion Struct for GetAmpsByEvent statement */
static EWDB_OCIStatementStruct SSStatement;


/* Temporary Storage Buffer params */
#define BUFFERSIZE 8192
static int    iRecordSize;
static int    iRecordsPerBuffer;
static char * pLocalBuffer=NULL;

static int    bInitialized=FALSE;

static  int     Local_tStart, Local_tEnd, Local_iMagType;


/****************************************************************/
/****************************************************************/
/********************************
      FUNCTION PROTOTYPES
********************************/
static int PrepGetAmpsByTimeExec(time_t IN_tStart, time_t IN_tEnd, 
                          MAGNITUDE_TYPE IN_iMagType, 
                          EWDB_Cursor * ppCursor);
static int PostGetAmpsByTimeExec(EWDB_PeakAmpStruct * pAmpBuffer, 
                           EWDB_StationStruct * pStationBuffer, int BufferRecLen);
static int InitGetAmpsByTimeStatement(char * szStatement, 
                                EWDB_OCIStatementStruct *pSS);


/* 
   Retrieves a list of peak amplitudes for a given time window and amplitude
   type.

  pAmpBuffer is a pointer to a buffer allocated by the caller
   where the function is to stick the stations.
  iBufferLen is the length of the buffer in EWDB_PeakAmpStruct allocated 
   by the caller.

    Return Value:   
     EWDB_RETURN_SUCCESS   Success.
     EWDB_RETURN_FAILURE   Error
     EWDB_RETURN_WARNING   WARNING: The buffer passed by the caller is too
                           small, where *pNumAmpsFound is the neccessary 
                           size of iBufferLen to get all of the picks for the 
                           given origin.
                           Results that fit in the buffer are passed back.

    Other Details:  Caller is responsible for allocating
                    space for the amps buffer.
*/
int ewdb_internal_GetAmpsByTime_w_StaInfo(time_t IN_tStart, time_t IN_tEnd, 
                                          MAGNITUDE_TYPE IN_iMagType,
                                          EWDB_PeakAmpStruct * pAmpBuffer,
                                          EWDB_StationStruct * pStationBuffer,
                                          int * pNumAmpsFound, int * pNumAmpsRetrieved,
                                          int iBufferLen)
{

  EWDB_Cursor  pCursor;
 
  ewdb_base_SetLastOraAPIActionTime();

  if( ewdb_base_Reconnect() != EWDB_RETURN_SUCCESS )
  {
    return( EWDB_RETURN_FAILURE );
  }

  if( PrepGetAmpsByTimeExec(IN_tStart, IN_tEnd, IN_iMagType, &pCursor)
     != EWDB_RETURN_SUCCESS)
  {
    return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }

  if( ewdb_base_SQLExecute(pCursor) )
  {
    ewdb_base_ErrorReport(hEWDBC, pCursor,"ewdb_internal_GetAmpsByTime_w_StaInfo(): ewdb_base_SQLExecute",1);
    return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  } 

 /* Commit the transaction (all the previous inserts!)
  In Case there is any auditing or logging or debug
  changes made in the stored procedures.
  ****************************************************/
  if (ewdb_base_SQLCommit (hEWDBC))
  {
    ewdb_base_ErrorReport (hEWDBC, hEWDBC,"ewdb_internal_GetAmpsByTime_w_StaInfo(): ewdb_base_SQLCommit",2);
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  }
 
  if((*pNumAmpsFound = PostGetAmpsByTimeExec(pAmpBuffer,pStationBuffer,iBufferLen)) 
     == EWDB_RETURN_FAILURE)
  {
    return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }

  ewdb_base_SetLastOraAPIActionTime();

  if(*pNumAmpsFound <= iBufferLen)
  {
    *pNumAmpsRetrieved = *pNumAmpsFound;
    return(EWDB_RETURN_SUCCESS);
  }
  else
  {
    *pNumAmpsRetrieved = iBufferLen;
    return(EWDB_RETURN_WARNING);
  }
}  /* end ewdb_internal_GetAmpsByTime_w_StaInfo():() */


static int InitGetAmpsByTimeStatement(char * szStatement, 
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
    iRecordSize += pSS->FieldArray[2].Ind; /*dPeakAmp1*/
    iRecordSize += pSS->FieldArray[3].Ind; /*tPeriod1*/
    iRecordSize += pSS->FieldArray[4].Ind; /*tAmp1*/
    iRecordSize += pSS->FieldArray[5].Ind; /*dPeakAmp2*/
    iRecordSize += pSS->FieldArray[6].Ind; /*tPeriod2*/
    iRecordSize += pSS->FieldArray[7].Ind; /*tAmp2*/
    iRecordSize += sizeof(EWDBid); /*idCompt*/
    iRecordSize += pSS->FieldArray[9].Ind; /*sSta*/
    iRecordSize += pSS->FieldArray[10].Ind; /*sComp*/
    iRecordSize += pSS->FieldArray[11].Ind; /*sNet*/
    iRecordSize += pSS->FieldArray[12].Ind; /*sLoc*/
    iRecordSize += pSS->FieldArray[13].Ind; /*dAzm*/
    iRecordSize += pSS->FieldArray[14].Ind; /*dDip*/
    iRecordSize += pSS->FieldArray[15].Ind; /*dLat*/
    iRecordSize += pSS->FieldArray[16].Ind; /*dLon*/
    iRecordSize += pSS->FieldArray[17].Ind; /*dElev*/
    iRecordSize += pSS->FieldArray[18].Ind; /*tOn*/
    iRecordSize += pSS->FieldArray[19].Ind; /*tOff*/
    
    /* Make the number of records a multiple of 8 to avoid alignment problems on UNIX */
    iRecordsPerBuffer= (BUFFERSIZE/iRecordSize) & 0xfffffff8;
    
    /*Allocate space for row/col ret lens. - never freed */
    for(i=0;i<pSS->NumOfFields;i++)
    {
      pSS->FieldArray[i].pRetLens=malloc(iRecordsPerBuffer*EWDB_FIELD_RET_LEN);
    }


    pSS->FieldArray[0].pVal=&(pLocalBuffer[0]);
    LastSize=sizeof(EWDBid);
    pSS->FieldArray[1].pVal=(void *)(
     (int)(pSS->FieldArray[0].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=sizeof(EWDBid);
    pSS->FieldArray[2].pVal=(void *)(
     (int)(pSS->FieldArray[1].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[2].Ind; /* dAmp1 */
    pSS->FieldArray[3].pVal=(void *)(
     (int)(pSS->FieldArray[2].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[3].Ind; /* tPeriod1 */
    pSS->FieldArray[4].pVal=(void *)(
     (int)(pSS->FieldArray[3].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[4].Ind; /* tAmp1 */
    pSS->FieldArray[5].pVal=(void *)(
     (int)(pSS->FieldArray[4].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[5].Ind; /* dAmp2 */
    pSS->FieldArray[6].pVal=(void *)(
     (int)(pSS->FieldArray[5].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[6].Ind; /* tPeriod2 */
    pSS->FieldArray[7].pVal=(void *)(
     (int)(pSS->FieldArray[6].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[7].Ind; /* tAmp2 */
    pSS->FieldArray[8].pVal=(void *)(
     (int)(pSS->FieldArray[7].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=sizeof(EWDBid); /* idComp */
    pSS->FieldArray[9].pVal=(void *)(
     (int)(pSS->FieldArray[8].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[9].Ind; /* sSta */
    pSS->FieldArray[10].pVal=(void *)(
     (int)(pSS->FieldArray[9].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[10].Ind; /* sComp */
    pSS->FieldArray[11].pVal=(void *)(
     (int)(pSS->FieldArray[10].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[11].Ind; /* sNet */
    pSS->FieldArray[12].pVal=(void *)(
     (int)(pSS->FieldArray[11].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[12].Ind; /* sLoc */
    pSS->FieldArray[13].pVal=(void *)(
     (int)(pSS->FieldArray[12].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[13].Ind; /* dAzm */
    pSS->FieldArray[14].pVal=(void *)(
     (int)(pSS->FieldArray[13].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[14].Ind; /* dDip */
    pSS->FieldArray[15].pVal=(void *)(
     (int)(pSS->FieldArray[14].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[15].Ind; /* dLat */
    pSS->FieldArray[16].pVal=(void *)(
     (int)(pSS->FieldArray[15].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[16].Ind; /* dLon */
    pSS->FieldArray[17].pVal=(void *)(
     (int)(pSS->FieldArray[16].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[17].Ind; /* dElev */
    pSS->FieldArray[18].pVal=(void *)(
     (int)(pSS->FieldArray[17].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[18].Ind; /* tStartInterval */
    pSS->FieldArray[19].pVal=(void *)(
     (int)(pSS->FieldArray[18].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[19].Ind; /* tEndInterval */

    pSS->FieldArray[20].pVal=&Local_tStart;
    pSS->FieldArray[21].pVal=&Local_tEnd;
    pSS->FieldArray[22].pVal=&Local_iMagType;

  } /* end if(!pLocalBuffer) */
  
  if(!pLocalBuffer)
  {
    logit("","InitGetAmpsByTime_w_StaInfoStatement(): malloc of pLocalBuffer "
          "failed! Returning.\n");
    return(EWDB_RETURN_FAILURE);
  }

  return(ewdb_base_RequestCursor(szStatement, pSS,0/*don't force rebind*/));

}  /* End InitGetAmpsByTime_w_StaInfoStatement() */


static int PrepGetAmpsByTimeExec(time_t IN_tStart, time_t IN_tEnd, 
                                 MAGNITUDE_TYPE IN_iMagType, 
                                 EWDB_Cursor * ppCursor)
{

  /* Initialize the statement parameters */
  if(!bInitialized)
  {
    bInitialized = TRUE;
    memset(&SSStatement,0,sizeof(SSStatement));
    SSStatement.NumOfFields=NUM_FIELDS;
    SSStatement.FieldArray=SQLParamsBindArray;
    SSStatement.RecordSize=0;
  }

  Local_tStart   = IN_tStart;
  Local_tEnd     = IN_tEnd;
  Local_iMagType = IN_iMagType;

  if(InitGetAmpsByTimeStatement(SQL_STRING, &SSStatement) != EWDB_RETURN_SUCCESS)
  {
    *ppCursor = NULL;
    return(EWDB_RETURN_FAILURE);
  }
  *ppCursor=SSStatement.pCda;

  return(EWDB_RETURN_SUCCESS);
}  /* End PrepGetAmpsByTimeExec() */


static int PostGetAmpsByTimeExec(EWDB_PeakAmpStruct * pAmpBuffer, 
                                 EWDB_StationStruct * pStationBuffer, int BufferRecLen)
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
        ewdb_base_ErrorReport(hEWDBC, pCursor,"PostGetAmpsByTimeExec:ewdb_base_SQLFetchRows",1);
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


        pAmpBuffer[UCurr].iAmpType = Local_iMagType;
        pAmpBuffer[UCurr].idPeakAmp=*(EWDBid *)
           ((sizeof(int)*BCurr) +(int)(pSS->FieldArray[0].pVal));
        pAmpBuffer[UCurr].idChan=*(EWDBid *)
           ((sizeof(int)*BCurr) +(int)(pSS->FieldArray[1].pVal));

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
           ((pSS->FieldArray[2].Ind*BCurr) +(int)(pSS->FieldArray[2].pVal));
        pTemp[pSS->FieldArray[2].pRetLens[BCurr]]=0;
        pAmpBuffer[UCurr].dAmp1=(float)atof(pTemp);

        pTemp=(char *) 
           ((pSS->FieldArray[3].Ind*BCurr) +(int)(pSS->FieldArray[3].pVal));
        pTemp[pSS->FieldArray[3].pRetLens[BCurr]]=0;
        pAmpBuffer[UCurr].dAmpPeriod1=(float)atof(pTemp);

        pTemp=(char *) 
           ((pSS->FieldArray[4].Ind*BCurr) +(int)(pSS->FieldArray[4].pVal));
        pTemp[pSS->FieldArray[4].pRetLens[BCurr]]=0;
        pAmpBuffer[UCurr].tAmp1=(double)atof(pTemp);

        pTemp=(char *) 
           ((pSS->FieldArray[5].Ind*BCurr) +(int)(pSS->FieldArray[5].pVal));
        pTemp[pSS->FieldArray[5].pRetLens[BCurr]]=0;
        pAmpBuffer[UCurr].dAmp2=(float)atof(pTemp);

        pTemp=(char *) 
           ((pSS->FieldArray[6].Ind*BCurr) +(int)(pSS->FieldArray[6].pVal));
        pTemp[pSS->FieldArray[6].pRetLens[BCurr]]=0;
        pAmpBuffer[UCurr].dAmpPeriod2=(float)atof(pTemp);

        pTemp=(char *) 
           ((pSS->FieldArray[7].Ind*BCurr) +(int)(pSS->FieldArray[7].pVal));
        pTemp[pSS->FieldArray[7].pRetLens[BCurr]]=0;
        pAmpBuffer[UCurr].tAmp2=(double)atof(pTemp);


        pStationBuffer[UCurr].idChan=*(EWDBid *)
           ((sizeof(int)*BCurr) +(int)(pSS->FieldArray[1].pVal));
        
        pStationBuffer[UCurr].idComp=*(EWDBid *)
           ((sizeof(int)*BCurr) +(int)(pSS->FieldArray[8].pVal));
      
        pTemp=(char *) 
           ((pSS->FieldArray[9].Ind*BCurr) +(int)(pSS->FieldArray[9].pVal));
        pTemp[pSS->FieldArray[9].pRetLens[BCurr]]=0;
        strncpy(pStationBuffer[UCurr].Sta, pTemp, sizeof(pStationBuffer[UCurr].Sta));

        pTemp=(char *) 
           ((pSS->FieldArray[10].Ind*BCurr) +(int)(pSS->FieldArray[10].pVal));
        pTemp[pSS->FieldArray[10].pRetLens[BCurr]]=0;
        strncpy(pStationBuffer[UCurr].Comp, pTemp, sizeof(pStationBuffer[UCurr].Comp));

        pTemp=(char *) 
           ((pSS->FieldArray[11].Ind*BCurr) +(int)(pSS->FieldArray[11].pVal));
        pTemp[pSS->FieldArray[11].pRetLens[BCurr]]=0;
        strncpy(pStationBuffer[UCurr].Net, pTemp, sizeof(pStationBuffer[UCurr].Net));

        pTemp=(char *) 
           ((pSS->FieldArray[12].Ind*BCurr) +(int)(pSS->FieldArray[12].pVal));
        pTemp[pSS->FieldArray[12].pRetLens[BCurr]]=0;
        strncpy(pStationBuffer[UCurr].Loc, pTemp, sizeof(pStationBuffer[UCurr].Loc));
        pStationBuffer[UCurr].Loc[sizeof(pStationBuffer[UCurr].Loc) - 1] = 0x00;
      
        pTemp=(char *) 
           ((pSS->FieldArray[13].Ind*BCurr) +(int)(pSS->FieldArray[13].pVal));
        pTemp[pSS->FieldArray[13].pRetLens[BCurr]]=0;
        pStationBuffer[UCurr].Azm=(float)atof(pTemp);

        pTemp=(char *) 
           ((pSS->FieldArray[14].Ind*BCurr) +(int)(pSS->FieldArray[14].pVal));
        pTemp[pSS->FieldArray[14].pRetLens[BCurr]]=0;
        pStationBuffer[UCurr].Dip=(float)atof(pTemp);

        pTemp=(char *) 
           ((pSS->FieldArray[15].Ind*BCurr) +(int)(pSS->FieldArray[15].pVal));
        pTemp[pSS->FieldArray[15].pRetLens[BCurr]]=0;
        pStationBuffer[UCurr].Lat=(float)atof(pTemp);

        pTemp=(char *) 
           ((pSS->FieldArray[16].Ind*BCurr) +(int)(pSS->FieldArray[16].pVal));
        pTemp[pSS->FieldArray[16].pRetLens[BCurr]]=0;
        pStationBuffer[UCurr].Lon=(float)atof(pTemp);

        pTemp=(char *) 
           ((pSS->FieldArray[17].Ind*BCurr) +(int)(pSS->FieldArray[17].pVal));
        pTemp[pSS->FieldArray[17].pRetLens[BCurr]]=0;
        pStationBuffer[UCurr].Elev=(float)atof(pTemp);

        pTemp=(char *) 
           ((pSS->FieldArray[18].Ind*BCurr) +(int)(pSS->FieldArray[18].pVal));
        pTemp[pSS->FieldArray[18].pRetLens[BCurr]]=0;
        pAmpBuffer[UCurr].tStartInterval=(double)atof(pTemp);

        pTemp=(char *) 
           ((pSS->FieldArray[19].Ind*BCurr) +(int)(pSS->FieldArray[19].pVal));
        pTemp[pSS->FieldArray[19].pRetLens[BCurr]]=0;
        pAmpBuffer[UCurr].tEndInterval=(double)atof(pTemp);

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
      ewdb_base_ErrorReport(hEWDBC, pCursor,"PostGetAmpsByTimeExec:ewdb_base_SQLFetchRows",2);
      return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
    }
    /* else */
  }  /* End if(RowsRetrieved > BufferRecLen) */
  
  RowsProcessed = ewdb_base_GetCursorRowsProcessedCount(pCursor);

  ewdb_base_ReleaseCursor(pCursor);

  return(RowsProcessed);
}  /* End PostGetAmpsByTimeExec() */


