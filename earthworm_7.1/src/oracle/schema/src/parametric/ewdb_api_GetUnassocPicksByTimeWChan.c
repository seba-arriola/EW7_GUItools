/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_api_GetUnassocPicksByTimeWChan.c,v 1.3 2005/06/15 19:03:12 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_api_GetUnassocPicksByTimeWChan.c,v $
 *     Revision 1.3  2005/06/15 19:03:12  davidk
 *     DB API Cleanup
 *
 *     Revision 1.2  2005/04/14 20:04:56  davidk
 *     no message
 *
 *     Revision 1.1  2005/03/30 18:24:20  davidk
 *     Added ewdb_api_GetUnassocPicksByTimeWChan
 *
 *
 *
 */


#include <ewdb_ora_api.h>
#include <ewdb_cli_base.h>
#include <ewdb_ew_oci_base.h>


static char SQL_STRING[] =
         /* To get all messages from a certain time */
        "select AUPI.idPick, AUPI.xidExternal, AUPI.sPhase, AUPI.tPhase, "
        " AUPI.cMotion, AUPI.cOnset, AUPI.dSigma, AUPI.sSource, ASI.idComp, ASI.sSta, ASI.sComp, "
       "  ASI.sNet, ASI.sLoc, ASI.dAzm, ASI.dDip, ASI.dLat, ASI.dLon, ASI.dElev, "
       "  ASI.idChan, ASI.tOn, ASI.tOff, ASI.idChanT, "
       "  ASI.idCompT, ASI.tOnCompT, ASI.tOffCompT "
       " from ALL_UNASSOC_PICK_INFO AUPI, ALL_STATION_INFO ASI "
       " where AUPI.tPhase >= :IN_tStart"
       "   and AUPI.tPhase <= :IN_tEnd"
       "   and AUPI.idChan = ASI.idChan  "
       "   and AUPI.tPhase >= ASI.tOn    "
       "   and AUPI.tPhase < ASI.tOff   ";

static EWDB_OCI_SFS SQLParamsBindArray[] = 
{
  {0,1,0,0,0,OA_EWDBID, "1idPick"},
  {0,1,16,0,0,OA_SZ,    "2xidExternal"},
  {0,1,EWDB_PHASENAME_SIZE,
          0,0,OA_SZ,    "3sPhase"},
  {0,1,20,0,0,OA_DOUBLE,"4tPhase"},
  {0,1,2,0,0,OA_SZ,     "5cMotion"},
  {0,1,2,0,0,OA_SZ,     "6cOnset"},
  {0,1,20,0,0,OA_DOUBLE,"7dSigma"},
  {0,1,16,0,0,OA_SZ,    "8sSource"},
  {0,1,0,0,0, OA_EWDBID,"9idComp"},
  {0,1,10,0,0,OA_SZ,    "10sSta"},
  {0,1,10,0,0,OA_SZ,    "11sComp"},
  {0,1,10,0,0,OA_SZ,    "12sNet"},
  {0,1,10,0,0,OA_SZ,    "13sLoc"},
  {0,1,20,0,0,OA_FLOAT, "14dAzm"},
  {0,1,20,0,0,OA_FLOAT, "15dDip"},
  {0,1,20,0,0,OA_DOUBLE,"16dLat"},
  {0,1,20,0,0,OA_DOUBLE,"17dLon"},
  {0,1,20,0,0,OA_DOUBLE,"18dElev"},
  {0,1,0,0,0, OA_EWDBID,"19idChan"},
  {0,1,20,0,0,OA_DOUBLE,"20tOn"},
  {0,1,20,0,0,OA_DOUBLE,"21tOff"},
  {0,1,0,0,0, OA_EWDBID,"22idChanT"},
  {0,1,0,0,0, OA_EWDBID,"23idCompT"},
  {0,1,20,0,0,OA_DOUBLE,"24tOnCompT"},
  {0,1,20,0,0,OA_DOUBLE,"25tOffCompT"},
  {0,1,0,0,0,OA_INT,":IN_tStart"},
  {0,1,0,0,0,OA_INT,":IN_tEnd"}
};

/* define the total number of fields to be bound */
#define NUM_FIELDS 27

/* Insertion Struct for GetUnassocPicksByTimeWChan statement */
static EWDB_OCIStatementStruct SSStatement;

/* Temporary Storage Buffer params */
#define BUFFERSIZE 8192
static int    iRecordSize;
static int    iRecordsPerBuffer;
static char * pLocalBuffer=NULL;

static int    bInitialized=FALSE;

static time_t Local_tStart, Local_tEnd;


static int PrepGetUnassocPicksByTimeWChanExec(time_t IN_tStart, time_t IN_tEnd, EWDB_Cursor * ppCursor);
static int PostGetUnassocPicksByTimeWChanExec(EWDB_ArrivalStruct * pBuffer,
                                  EWDB_ChannelStruct * pChannelBuffer,
                                  int BufferRecLen);
static int InitGetUnassocPicksByTimeWChanStatement(char * szStatement, EWDB_OCIStatementStruct *pSS);



/***************************************************************************
  ewdb_api_GetUnassocPicksByTimeWChan() returns a list of picks from the DB based on
   time.

  pBuffer               Pointer to a buffer allocated by the caller
   where the function is to stick the arrivals.
  pChannelBuffer          Pointer to a buffer allocated by the caller
   where the function is to stick the channels associated with the arrivals.
  pNumItemsFound:         The number of Arrivals found(in the DB) for the Origin.
  pNumItemsRetrieved:     The number of Arrivals retrieved from the DB.
  iBufferLen:              The length of the buffer in terms of ArrivalStructs.

    Return Value:   
     EWDB_RETURN_FAILURE     unknown error
     EWDB_RETURN_WARNING     success, but the buffer was not able to accommodate
                              all of the arrivals found.
     EWDB_RETURN_SUCCESS     success 

    Other Details:  Caller is responsible for allocating
  space for the arrivals buffer.
***********************************************************************/
int ewdb_api_GetUnassocPicksByTimeWChan(time_t IN_tStart, time_t IN_tEnd,
                                        EWDB_ArrivalStruct * pBuffer,
                                        EWDB_ChannelStruct * pChannelBuffer,
                                        int * pNumItemsFound, int * pNumItemsRetrieved,
                                        int iBufferLen)
{

  EWDB_Cursor  pCursor;

  ewdb_base_SetLastOraAPIActionTime();

  if( ewdb_base_Reconnect() != EWDB_RETURN_SUCCESS )
  {
    return( EWDB_RETURN_FAILURE );
  }

  if( PrepGetUnassocPicksByTimeWChanExec(IN_tStart, IN_tEnd, &pCursor)
     != EWDB_RETURN_SUCCESS)
  {
    logit("","ewdb_api_GetUnassocPicksByTimeWChan(): PrepGetUnassocPicksByTimeWChanExec() failed!\n");
    return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }
  
  if( ewdb_base_SQLExecute(pCursor) )
  {
    ewdb_base_ErrorReport(hEWDBC, pCursor,"ewdb_api_GetUnassocPicksByTimeWChan(): ewdb_base_SQLExecute",1);
    return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  } 

 /* Commit the transaction (all the previous inserts!)
  In Case there is any auditing or logging or debug
  changes made in the stored procedures.
  ****************************************************/
  if (ewdb_base_SQLCommit (hEWDBC))
  {
    ewdb_base_ErrorReport (hEWDBC, hEWDBC,"ewdb_api_GetUnassocPicksByTimeWChan(): ewdb_base_SQLCommit",2);
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  }
 
  if((*pNumItemsFound=PostGetUnassocPicksByTimeWChanExec(pBuffer,pChannelBuffer,iBufferLen)) == EWDB_RETURN_FAILURE)
  {
    logit("","ewdb_api_GetUnassocPicksByTimeWChan(): PostGetUnassocPicksByTimeWChanExec() failed!\n");
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
}  /* end ewdb_api_GetUnassocPicksByTimeWChan() */


static int InitGetUnassocPicksByTimeWChanStatement(char * szStatement, EWDB_OCIStatementStruct *pSS)
{

  int LastSize,i;  /* Size of last column array */


  if(!pLocalBuffer)
  {
    /*Allocate space for returned data - never freed */
    pLocalBuffer=malloc(BUFFERSIZE);

    iRecordSize=0;
    iRecordSize += sizeof(EWDBid);        /*idPick*/
    iRecordSize += pSS->FieldArray[1].Ind;/*xidExternal*/
    iRecordSize += pSS->FieldArray[2].Ind;/*sPhase*/
    iRecordSize += pSS->FieldArray[3].Ind;/*tPhase*/
    iRecordSize += pSS->FieldArray[4].Ind;/*cMotion*/
    iRecordSize += pSS->FieldArray[5].Ind;/*cOnset*/
    iRecordSize += pSS->FieldArray[6].Ind;/*dSigma*/
    iRecordSize += pSS->FieldArray[7].Ind;/*sSource*/
    iRecordSize += sizeof(EWDBid); /*idComp*/
    iRecordSize += pSS->FieldArray[9].Ind; /*sSta*/
    iRecordSize += pSS->FieldArray[10].Ind; /*sComp*/
    iRecordSize += pSS->FieldArray[11].Ind; /*sNet*/
    iRecordSize += pSS->FieldArray[12].Ind; /*sLoc*/
    iRecordSize += pSS->FieldArray[13].Ind; /*dAzm*/
    iRecordSize += pSS->FieldArray[14].Ind; /*dDip*/
    iRecordSize += pSS->FieldArray[15].Ind; /*dLat*/
    iRecordSize += pSS->FieldArray[16].Ind; /*dLon*/
    iRecordSize += pSS->FieldArray[17].Ind; /*dElev*/
    iRecordSize += sizeof(EWDBid); /*idChan*/
    iRecordSize += pSS->FieldArray[19].Ind; /*tOn*/
    iRecordSize += pSS->FieldArray[20].Ind; /*tOff*/
    iRecordSize += sizeof(EWDBid); /*idChanT*/
    iRecordSize += sizeof(EWDBid); /*idCompT*/
    iRecordSize += pSS->FieldArray[23].Ind; /*tOnCompT*/
    iRecordSize += pSS->FieldArray[24].Ind; /*tOffCompT*/

        /* Make the number of records a multiple of 8 to avoid alignment problems on UNIX */
    iRecordsPerBuffer= (BUFFERSIZE/iRecordSize) & 0xfffffff8;
    if(iRecordsPerBuffer <= 0)
    {
      logit("et","%s: INTERNAL ERROR:  BUFFERSIZE(%d) not large enough. %d bytes required.\n",
            BUFFERSIZE, iRecordSize * 8);
      return(EWDB_RETURN_FAILURE);
    }
    
    /*Allocate space for row/col ret lens. - never freed */
    for(i=0;i<pSS->NumOfFields;i++)
    {
      pSS->FieldArray[i].pRetLens=malloc(iRecordsPerBuffer*EWDB_FIELD_RET_LEN);
    }

    pSS->FieldArray[0].pVal=&(pLocalBuffer[0]);
    LastSize=sizeof(EWDBid);
    pSS->FieldArray[1].pVal=(void *)(
      (int)(pSS->FieldArray[0].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[1].Ind; /* xidExternal */
    pSS->FieldArray[2].pVal=(void *)(
      (int)(pSS->FieldArray[1].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[2].Ind; /* sPhase */
    pSS->FieldArray[3].pVal=(void *)(
      (int)(pSS->FieldArray[2].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[3].Ind; /* tPhase */
    pSS->FieldArray[4].pVal=(void *)(
      (int)(pSS->FieldArray[3].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[4].Ind; /* cMotion */
    pSS->FieldArray[5].pVal=(void *)(
      (int)(pSS->FieldArray[4].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[5].Ind; /* cOnset */
    pSS->FieldArray[6].pVal=(void *)(
      (int)(pSS->FieldArray[5].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[6].Ind; /* dSigma */
    pSS->FieldArray[7].pVal=(void *)(
      (int)(pSS->FieldArray[6].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[7].Ind; /* sSource */
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
    LastSize=sizeof(EWDBid); /* idChan */
    pSS->FieldArray[19].pVal=(void *)(
      (int)(pSS->FieldArray[18].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[19].Ind; /* tOn */
    pSS->FieldArray[20].pVal=(void *)(
      (int)(pSS->FieldArray[19].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[20].Ind; /* tOff */
    pSS->FieldArray[21].pVal=(void *)(
      (int)(pSS->FieldArray[20].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=sizeof(EWDBid); /* idChanT */
    pSS->FieldArray[22].pVal=(void *)(
      (int)(pSS->FieldArray[21].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=sizeof(EWDBid); /* idCompT */
    pSS->FieldArray[23].pVal=(void *)(
      (int)(pSS->FieldArray[22].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[23].Ind; /* tOnCompT */
    pSS->FieldArray[24].pVal=(void *)(
      (int)(pSS->FieldArray[23].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[24].Ind; /* tOffCompT */
    
    pSS->FieldArray[25].pVal=&Local_tStart;
    pSS->FieldArray[26].pVal=&Local_tEnd;

  } /* end if(!pLocalBuffer) */
  
  if(!pLocalBuffer)
    logit("","InitGetUnassocPicksByTimeWChanStatement(): malloc of pLocalBuffer "
          "failed! Returning.\n");

  return(ewdb_base_RequestCursor(szStatement, pSS,0/*don't force rebind*/));
}  /* end InitGetUnassocPicksByTimeWChanStatement() */


static int PrepGetUnassocPicksByTimeWChanExec(time_t IN_tStart, time_t IN_tEnd,
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

  Local_tStart = IN_tStart;
  Local_tEnd   = IN_tEnd;

  if(InitGetUnassocPicksByTimeWChanStatement(SQL_STRING, &SSStatement) == EWDB_RETURN_FAILURE)
    return(EWDB_RETURN_FAILURE);

  *ppCursor=SSStatement.pCda;

  return(EWDB_RETURN_SUCCESS);
}  /* end PrepGetUnassocPicksByTimeWChanExec() */


static int PostGetUnassocPicksByTimeWChanExec(EWDB_ArrivalStruct * pBuffer,
                                       EWDB_ChannelStruct * pChannelBuffer,
                                       int BufferRecLen)
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
  int RecordsProcessed;

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
        ewdb_base_ErrorReport(hEWDBC, pCursor,"PostGetUnassocPicksByTimeWChanExec:ewdb_base_SQLFetchRows",1);
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


        /* Initialize the current struct to all 0's. */
        memset(&pBuffer[UCurr],0,sizeof(EWDB_ArrivalStruct));
        memset(&pChannelBuffer[UCurr],0,sizeof(EWDB_ChannelStruct));

        pBuffer[UCurr].pStation = &pChannelBuffer[UCurr].Comp;

        pBuffer[UCurr].idPick=*(int *)((sizeof(int)*BCurr) +(int)(pSS->FieldArray[0].pVal));

        /* This looks complicated, but maybe it isn't.
           pTemp is a pointer that is set to a place in the retrieval buffer.
           It's location in the buffer is determined by adding the section-offset to the
           buffer-offset, where the section offset is the location of the data within this
           section of the buffer.  The buffer-offset is the location of this section within
           the entire buffer.  So the buffer-offset points to the beginning of this section,
           and the section-offset points to the desired location relative to this section.
           Thus when they are combined, they point to the desired location relative to
           the buffer. (And you thought this was complicated)
        */
        pTemp=(char *)((pSS->FieldArray[1].Ind*BCurr) +(int)(pSS->FieldArray[1].pVal) );
        pTemp[pSS->FieldArray[1].pRetLens[BCurr]]=0;
        strcpy(pBuffer[UCurr].szExternalPickID,pTemp);

        pTemp=(char *)((pSS->FieldArray[2].Ind*BCurr) +(int)(pSS->FieldArray[2].pVal) );
        pTemp[pSS->FieldArray[2].pRetLens[BCurr]]=0;
        strcpy(pBuffer[UCurr].szObsPhase,pTemp);

        pTemp=(char *)((pSS->FieldArray[3].Ind*BCurr) +(int)(pSS->FieldArray[3].pVal) );
        pTemp[pSS->FieldArray[3].pRetLens[BCurr]]=0;
        pBuffer[UCurr].tObsPhase=atof(pTemp);

        pTemp=(char *)((pSS->FieldArray[4].Ind*BCurr) +(int)(pSS->FieldArray[4].pVal) );
        pBuffer[UCurr].cMotion = pTemp[0];  /* we are looking for 1 character */

        pTemp=(char *)((pSS->FieldArray[5].Ind*BCurr) +(int)(pSS->FieldArray[5].pVal) );
        pBuffer[UCurr].cOnset = pTemp[0];  /* we are looking for 1 character */

        pTemp=(char *)((pSS->FieldArray[6].Ind*BCurr) +(int)(pSS->FieldArray[6].pVal) );
        pTemp[pSS->FieldArray[6].pRetLens[BCurr]]=0;
        pBuffer[UCurr].dSigma=atof(pTemp);

        pTemp=(char *)((pSS->FieldArray[7].Ind*BCurr) +(int)(pSS->FieldArray[7].pVal) );
        pTemp[pSS->FieldArray[7].pRetLens[BCurr]]=0;
        strcpy(pBuffer[UCurr].szExtSource,pTemp);

        pChannelBuffer[UCurr].Comp.idComp=*(EWDBid *)((sizeof(int)*BCurr) +(int)(pSS->FieldArray[8].pVal));

        pTemp=(char *) 
           ((pSS->FieldArray[9].Ind*BCurr) +(int)(pSS->FieldArray[9].pVal));
        pTemp[pSS->FieldArray[9].pRetLens[BCurr]]=0;
        strncpy(pChannelBuffer[UCurr].Comp.Sta, pTemp, sizeof(pChannelBuffer[UCurr].Comp.Sta));

        pTemp=(char *) 
           ((pSS->FieldArray[10].Ind*BCurr) +(int)(pSS->FieldArray[10].pVal));
        pTemp[pSS->FieldArray[10].pRetLens[BCurr]]=0;
        strncpy(pChannelBuffer[UCurr].Comp.Comp, pTemp, sizeof(pChannelBuffer[UCurr].Comp.Comp));

        pTemp=(char *) 
           ((pSS->FieldArray[11].Ind*BCurr) +(int)(pSS->FieldArray[11].pVal));
        pTemp[pSS->FieldArray[11].pRetLens[BCurr]]=0;
        strncpy(pChannelBuffer[UCurr].Comp.Net, pTemp, sizeof(pChannelBuffer[UCurr].Comp.Net));

        pTemp=(char *) 
           ((pSS->FieldArray[12].Ind*BCurr) +(int)(pSS->FieldArray[12].pVal));
        pTemp[pSS->FieldArray[12].pRetLens[BCurr]]=0;
        strncpy(pChannelBuffer[UCurr].Comp.Loc, pTemp, sizeof(pChannelBuffer[UCurr].Comp.Loc));
        pChannelBuffer[UCurr].Comp.Loc[sizeof(pChannelBuffer[UCurr].Comp.Loc) - 1] = 0x00;
      
        pTemp=(char *) 
           ((pSS->FieldArray[13].Ind*BCurr) +(int)(pSS->FieldArray[13].pVal));
        pTemp[pSS->FieldArray[13].pRetLens[BCurr]]=0;
        pChannelBuffer[UCurr].Comp.Azm=(float)atof(pTemp);

        pTemp=(char *) 
           ((pSS->FieldArray[14].Ind*BCurr) +(int)(pSS->FieldArray[14].pVal));
        pTemp[pSS->FieldArray[14].pRetLens[BCurr]]=0;
        pChannelBuffer[UCurr].Comp.Dip=(float)atof(pTemp);

        pTemp=(char *) 
           ((pSS->FieldArray[15].Ind*BCurr) +(int)(pSS->FieldArray[15].pVal));
        pTemp[pSS->FieldArray[15].pRetLens[BCurr]]=0;
        pChannelBuffer[UCurr].Comp.Lat=(float)atof(pTemp);

        pTemp=(char *) 
           ((pSS->FieldArray[16].Ind*BCurr) +(int)(pSS->FieldArray[16].pVal));
        pTemp[pSS->FieldArray[16].pRetLens[BCurr]]=0;
        pChannelBuffer[UCurr].Comp.Lon=(float)atof(pTemp);

        pTemp=(char *) 
           ((pSS->FieldArray[17].Ind*BCurr) +(int)(pSS->FieldArray[17].pVal));
        pTemp[pSS->FieldArray[17].pRetLens[BCurr]]=0;
        pChannelBuffer[UCurr].Comp.Elev=(float)atof(pTemp);

        pBuffer[UCurr].idChan=*(EWDBid *)((sizeof(int)*BCurr) +(int)(pSS->FieldArray[18].pVal));
        pChannelBuffer[UCurr].Comp.idChan = pBuffer[UCurr].idChan;

        pTemp=(char *) 
           ((pSS->FieldArray[19].Ind*BCurr) +(int)(pSS->FieldArray[19].pVal));
        pTemp[pSS->FieldArray[19].pRetLens[BCurr]]=0;
        pChannelBuffer[UCurr].tOn = atof(pTemp);

        pTemp=(char *) 
           ((pSS->FieldArray[20].Ind*BCurr) +(int)(pSS->FieldArray[20].pVal));
        pTemp[pSS->FieldArray[20].pRetLens[BCurr]]=0;
        pChannelBuffer[UCurr].tOff = atof(pTemp);

        pChannelBuffer[UCurr].idChanT=*(EWDBid *)((sizeof(int)*BCurr) +(int)(pSS->FieldArray[21].pVal));
        pChannelBuffer[UCurr].idCompT=*(EWDBid *)((sizeof(int)*BCurr) +(int)(pSS->FieldArray[22].pVal));

        pTemp=(char *) 
           ((pSS->FieldArray[23].Ind*BCurr) +(int)(pSS->FieldArray[23].pVal));
        pTemp[pSS->FieldArray[23].pRetLens[BCurr]]=0;
        pChannelBuffer[UCurr].tOnCompT = atof(pTemp);

        pTemp=(char *) 
           ((pSS->FieldArray[24].Ind*BCurr) +(int)(pSS->FieldArray[24].pVal));
        pTemp[pSS->FieldArray[24].pRetLens[BCurr]]=0;
        pChannelBuffer[UCurr].tOffCompT = atof(pTemp);


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
      while(!ewdb_base_SQLFetchRows(pCursor, iRecordsPerBuffer))
      {
      }
    }

    if(ewdb_base_GetCursorRetCode(pCursor) != EWDB_SQL_ERROR_NO_DATA)
    {
      ewdb_base_ErrorReport(hEWDBC, pCursor,"PostGetUnassocPicksByTimeWChanExec:ewdb_base_SQLFetchRows",2);
      return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
    }
    /* else */
  }  /* End if(RowsRetrieved > BufferRecLen) */
  
  RecordsProcessed = ewdb_base_GetCursorRowsProcessedCount(pCursor);
  ewdb_base_ReleaseCursor(pCursor);

  return(RecordsProcessed);
}  /* end PostGetUnassocPicksByTimeWChanExec() */
