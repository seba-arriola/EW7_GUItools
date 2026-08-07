/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_api_GetArrivalsWChanInfo.c,v 1.7 2005/06/15 19:03:12 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_api_GetArrivalsWChanInfo.c,v $
 *     Revision 1.7  2005/06/15 19:03:12  davidk
 *     DB API Cleanup
 *
 *     Revision 1.6  2005/01/28 19:29:06  mark
 *     Added sSource
 *
 *     Revision 1.5  2004/11/06 04:27:05  davidk
 *     Updated phasename field(s) in the SQL statement structs, to use the new
 *     EWDB_PHASENAME_SIZE length, as it is used in EWDB_ArrivalStructs,
 *     which is where the results are headed.
 *
 *     Revision 1.4  2004/05/05 21:59:32  davidk
 *     Ordered the results by epicentral distance.
 *
 *     Revision 1.3  2004/05/05 18:48:17  davidk
 *     Fixed where clause in SQL statement.  Original statement was missing join
 *     criteria for AAI - ASI join.  Was returning cartesian join.
 *
 *     Revision 1.2  2004/05/05 00:46:53  michelle
 *     there is no array position of 56 in FieldArray
 *     so on line 507 changed FieldArray[56] to FieldArray[5],
 *     just cleaning up a dangerous typo
 *
 *     Revision 1.1  2004/05/04 19:49:22  davidk
 *     New api function that retrieves arrivals for an origin along with associated
 *     channel information.
 *
 *     Revision 1.3  2003/09/16 16:56:55  davidk
 *     General API Cleanup
 *
 *     Revision 1.2  2003/08/21 00:57:41  davidk
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
       "select AAI.idPick, AAI.xidExternal, AAI.sPhase, AAI.tPhase, "
       "  AAI.cMotion, AAI.cOnset, AAI.dSigma, "
       "  AAI.idOriginPick, AAI.idOrigin, AAI.sCalcPhase, "
       "  AAI.tCalcPhase, AAI.dWeight, AAI.dDist, AAI.dAzm dArrivalAzm, "
       "  AAI.dTakeoff, AAI.tResPick, AAI.sSource, ASI.idComp, ASI.sSta, ASI.sComp, "
       "  ASI.sNet, ASI.sLoc, ASI.dAzm, ASI.dDip, ASI.dLat, ASI.dLon, ASI.dElev, "
       "  ASI.idChan, ASI.tOn, ASI.tOff, ASI.idChanT, "
       "  ASI.idCompT, ASI.tOnCompT, ASI.tOffCompT "
       " from ALL_ARRIVAL_INFO AAI, ALL_STATION_INFO ASI "
       " where AAI.idOrigin= :IN_idOrigin "
       "   and AAI.idChan = ASI.idChan  "
       "   and AAI.tPhase >= ASI.tOn    "
       "   and AAI.tPhase < ASI.tOff   "
       " order by dDist";




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
  {0,1,0,0,0,OA_EWDBID, "8idOriginPick"},
  {0,1,0,0,0,OA_EWDBID, "9idOrigin"},
  {0,1,EWDB_PHASENAME_SIZE,
         0,0,OA_SZ,     "10sCalcPhase"},
  {0,1,20,0,0,OA_DOUBLE,"11tCalcPhase"},
  {0,1,20,0,0,OA_FLOAT, "12dWeight"},
  {0,1,20,0,0,OA_FLOAT, "13dDist"},
  {0,1,20,0,0,OA_FLOAT, "14dAzm"},
  {0,1,20,0,0,OA_FLOAT, "15dTakeoff"},
  {0,1,20,0,0,OA_DOUBLE,"16tResPick"},
  {0,1,16,0,0,OA_SZ,    "17sSource"},
  {0,1,0,0,0, OA_EWDBID,"18idComp"},
  {0,1,10,0,0,OA_SZ,    "19sSta"},
  {0,1,10,0,0,OA_SZ,    "20sComp"},
  {0,1,10,0,0,OA_SZ,    "21sNet"},
  {0,1,10,0,0,OA_SZ,    "22sLoc"},
  {0,1,20,0,0,OA_FLOAT, "23dAzm"},
  {0,1,20,0,0,OA_FLOAT, "24dDip"},
  {0,1,20,0,0,OA_DOUBLE,"25dLat"},
  {0,1,20,0,0,OA_DOUBLE,"26dLon"},
  {0,1,20,0,0,OA_DOUBLE,"27dElev"},
  {0,1,0,0,0, OA_EWDBID,"28idChan"},
  {0,1,20,0,0,OA_DOUBLE,"29tOn"},
  {0,1,20,0,0,OA_DOUBLE,"30tOff"},
  {0,1,0,0,0, OA_EWDBID,"31idChanT"},
  {0,1,0,0,0, OA_EWDBID,"32idCompT"},
  {0,1,20,0,0,OA_DOUBLE,"33tOnCompT"},
  {0,1,20,0,0,OA_DOUBLE,"34tOffCompT"},
  {0,1,0,0,0,OA_EWDBID, ":IN_idOrigin"}
};

/* define the total number of fields to be bound */
#define NUM_FIELDS 35

/* Statement Struct for GetArrivalsWChanInfo statement */
static EWDB_OCIStatementStruct SSStatement;

/* Temporary Storage Buffer params */
#define BUFFERSIZE 8192
static int    iRecordSize;
static int    iRecordsPerBuffer;
static char * pLocalBuffer=NULL;

static int    bInitialized=FALSE;


static  EWDBid  Local_idOrigin;

/****************************************************************/
/****************************************************************/
/********************************
      FUNCTION PROTOTYPES
********************************/
static int PrepGetArrivalsWChanInfoExec(EWDBid IN_idOrigin,
                                        EWDB_Cursor * ppCursor);
static int PostGetArrivalsWChanInfoExec(EWDB_ArrivalStruct * pArrivals, 
                                        EWDB_ChannelStruct * pChannelBuffer, 
                                        int BufferRecLen);
static int InitGetArrivalsWChanInfoStatement(char * szStatement, 
                                             EWDB_OCIStatementStruct *pSS);


/* 
   Retrieves a list of arrivals for a given origin.

  pArrivalBuffer is a pointer to a buffer allocated by the caller
   where the function is to stick the arrivals.
  pChannelBuffer is a pointer to a buffer allocated by the caller
   where the function is to stick the channel info.
   pArrivalBuffer[i].pStation = &pChannelBuffer[i].Comp
  iBufferLen is the length of the buffer in structs 
   (allocated by the caller).

    Return Value:   
     EWDB_RETURN_SUCCESS   Success.
     EWDB_RETURN_FAILURE   Error
     EWDB_RETURN_WARNING   WARNING: The buffer passed by the caller is too
                           small, where *pNumArrivalsFound is the neccessary 
                           size of iBufferLen to get all of the picks for the 
                           given origin.
                           Results that fit in the buffer are passed back.

    Other Details:  Caller is responsible for allocating
                    space for the struct buffers.
*/
int ewdb_api_GetArrivalsWChanInfo(EWDBid IN_idOrigin,
                                  EWDB_ArrivalStruct * pArrivals,
                                  EWDB_ChannelStruct * pChannelBuffer,
                                  int * pNumArrivalsFound, int * pNumArrivalsRetrieved,
                                  int iBufferLen)
{

  EWDB_Cursor  pCursor;
 
  ewdb_base_SetLastOraAPIActionTime();

  if(!(pArrivals && pChannelBuffer && IN_idOrigin && 
       pNumArrivalsFound && pNumArrivalsRetrieved))
  {
    logit("et", "ewdb_api_GetArrivalsWChanInfo():  Error null parameter(s) "
                "passed in! (%u/%u/%u/%u/%u\n",
          pArrivals, pChannelBuffer, IN_idOrigin, 
          pNumArrivalsFound, pNumArrivalsRetrieved);
    return(EWDB_RETURN_FAILURE);
  }

  if( ewdb_base_Reconnect() != EWDB_RETURN_SUCCESS )
  {
    return( EWDB_RETURN_FAILURE );
  }

  if( PrepGetArrivalsWChanInfoExec(IN_idOrigin, &pCursor)
     != EWDB_RETURN_SUCCESS)
  {
    return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }

  if( ewdb_base_SQLExecute(pCursor) )
  {
    ewdb_base_ErrorReport(hEWDBC, pCursor,"ewdb_api_GetArrivalsWChanInfo(): ewdb_base_SQLExecute",1);
    return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  } 

 /* Commit the transaction (all the previous inserts!)
  In Case there is any auditing or logging or debug
  changes made in the stored procedures.
  ****************************************************/
  if (ewdb_base_SQLCommit (hEWDBC))
  {
    ewdb_base_ErrorReport (hEWDBC, hEWDBC,"ewdb_api_GetArrivalsWChanInfo(): ewdb_base_SQLCommit",2);
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  }
 
  if((*pNumArrivalsFound = PostGetArrivalsWChanInfoExec(pArrivals, pChannelBuffer,iBufferLen)) 
     == EWDB_RETURN_FAILURE)
  {
    return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }

  ewdb_base_SetLastOraAPIActionTime();

  if(*pNumArrivalsFound <= iBufferLen)
  {
    *pNumArrivalsRetrieved = *pNumArrivalsFound;
    return(EWDB_RETURN_SUCCESS);
  }
  else
  {
    *pNumArrivalsRetrieved = iBufferLen;
    return(EWDB_RETURN_WARNING);
  }
}  /* end ewdb_api_GetArrivalsWChanInfo() */


static int InitGetArrivalsWChanInfoStatement(char * szStatement, 
                                             EWDB_OCIStatementStruct *pSS)
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
    iRecordSize += sizeof(EWDBid);        /*idOriginPick*/
    iRecordSize += sizeof(EWDBid);        /*Local_idOrigin*/
    iRecordSize += pSS->FieldArray[9].Ind;/*sCalcPhase*/
    iRecordSize += pSS->FieldArray[10].Ind;/*tCalcPhase*/
    iRecordSize += pSS->FieldArray[11].Ind;/*dWeight*/
    iRecordSize += pSS->FieldArray[12].Ind;/*dDist*/
    iRecordSize += pSS->FieldArray[13].Ind;/*dAzm*/
    iRecordSize += pSS->FieldArray[14].Ind;/*dTakeOff*/
    iRecordSize += pSS->FieldArray[15].Ind;/*tResPick*/
    iRecordSize += pSS->FieldArray[16].Ind;/*sSource*/
    iRecordSize += sizeof(EWDBid); /*idComp*/
    iRecordSize += pSS->FieldArray[18].Ind; /*sSta*/
    iRecordSize += pSS->FieldArray[19].Ind; /*sComp*/
    iRecordSize += pSS->FieldArray[20].Ind; /*sNet*/
    iRecordSize += pSS->FieldArray[21].Ind; /*sLoc*/
    iRecordSize += pSS->FieldArray[22].Ind; /*dAzm*/
    iRecordSize += pSS->FieldArray[23].Ind; /*dDip*/
    iRecordSize += pSS->FieldArray[24].Ind; /*dLat*/
    iRecordSize += pSS->FieldArray[25].Ind; /*dLon*/
    iRecordSize += pSS->FieldArray[26].Ind; /*dElev*/
    iRecordSize += sizeof(EWDBid); /*idChan*/
    iRecordSize += pSS->FieldArray[28].Ind; /*tOn*/
    iRecordSize += pSS->FieldArray[29].Ind; /*tOff*/
    iRecordSize += sizeof(EWDBid); /*idChanT*/
    iRecordSize += sizeof(EWDBid); /*idCompT*/
    iRecordSize += pSS->FieldArray[32].Ind; /*tOnCompT*/
    iRecordSize += pSS->FieldArray[33].Ind; /*tOffCompT*/
    
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
    LastSize=sizeof(EWDBid);
    pSS->FieldArray[8].pVal=(void *)(
      (int)(pSS->FieldArray[7].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=sizeof(EWDBid);
    pSS->FieldArray[9].pVal=(void *)(
      (int)(pSS->FieldArray[8].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[9].Ind; /* sCalcPhase */
    pSS->FieldArray[10].pVal=(void *)(
      (int)(pSS->FieldArray[9].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[10].Ind; /* tCalcPhase */
    pSS->FieldArray[11].pVal=(void *)(
      (int)(pSS->FieldArray[10].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[11].Ind; /* dWeight */
    pSS->FieldArray[12].pVal=(void *)(
      (int)(pSS->FieldArray[11].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[12].Ind; /* dDist */
    pSS->FieldArray[13].pVal=(void *)(
      (int)(pSS->FieldArray[12].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[13].Ind; /* dAzm */
    pSS->FieldArray[14].pVal=(void *)(
      (int)(pSS->FieldArray[13].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[14].Ind; /* dTakeoff */
    pSS->FieldArray[15].pVal=(void *)(
      (int)(pSS->FieldArray[14].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[15].Ind; /* tResPick */
    pSS->FieldArray[16].pVal=(void *)(
      (int)(pSS->FieldArray[15].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[16].Ind; /* sSource */
    pSS->FieldArray[17].pVal=(void *)(
      (int)(pSS->FieldArray[16].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=sizeof(EWDBid); /* idComp */
    pSS->FieldArray[18].pVal=(void *)(
      (int)(pSS->FieldArray[17].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[18].Ind; /* sSta */
    pSS->FieldArray[19].pVal=(void *)(
      (int)(pSS->FieldArray[18].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[19].Ind; /* sComp */
    pSS->FieldArray[20].pVal=(void *)(
      (int)(pSS->FieldArray[19].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[20].Ind; /* sNet */
    pSS->FieldArray[21].pVal=(void *)(
      (int)(pSS->FieldArray[20].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[21].Ind; /* sLoc */
    pSS->FieldArray[22].pVal=(void *)(
      (int)(pSS->FieldArray[21].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[22].Ind; /* dAzm */
    pSS->FieldArray[23].pVal=(void *)(
      (int)(pSS->FieldArray[22].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[23].Ind; /* dDip */
    pSS->FieldArray[24].pVal=(void *)(
      (int)(pSS->FieldArray[23].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[24].Ind; /* dLat */
    pSS->FieldArray[25].pVal=(void *)(
      (int)(pSS->FieldArray[24].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[25].Ind; /* dLon */
    pSS->FieldArray[26].pVal=(void *)(
      (int)(pSS->FieldArray[25].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[26].Ind; /* dElev */
    pSS->FieldArray[27].pVal=(void *)(
      (int)(pSS->FieldArray[26].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=sizeof(EWDBid); /* idChan */
    pSS->FieldArray[28].pVal=(void *)(
      (int)(pSS->FieldArray[27].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[28].Ind; /* tOn */
    pSS->FieldArray[29].pVal=(void *)(
      (int)(pSS->FieldArray[28].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[29].Ind; /* tOff */
    pSS->FieldArray[30].pVal=(void *)(
      (int)(pSS->FieldArray[29].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=sizeof(EWDBid); /* idChanT */
    pSS->FieldArray[31].pVal=(void *)(
      (int)(pSS->FieldArray[30].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=sizeof(EWDBid); /* idCompT */
    pSS->FieldArray[32].pVal=(void *)(
      (int)(pSS->FieldArray[31].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[32].Ind; /* tOnCompT */
    pSS->FieldArray[33].pVal=(void *)(
      (int)(pSS->FieldArray[32].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[33].Ind; /* tOffCompT */
    
    pSS->FieldArray[34].pVal=&Local_idOrigin;

  } /* end if(!pLocalBuffer) */
  
  if(!pLocalBuffer)
    logit("","InitGetArrivalsWChanInfoStatement: malloc of pLocalBuffer "
          "failed! Returning.\n");

  return(ewdb_base_RequestCursor(szStatement, pSS,0/*don't force rebind*/));

}  /* End InitGetArrivalsWChanInfoStatement() */


static int PrepGetArrivalsWChanInfoExec(EWDBid IN_idOrigin,
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

  Local_idOrigin  = IN_idOrigin;

  if(InitGetArrivalsWChanInfoStatement(SQL_STRING, &SSStatement) != EWDB_RETURN_SUCCESS)
  {
    *ppCursor = NULL;
    return(EWDB_RETURN_FAILURE);
  }

  *ppCursor=SSStatement.pCda;

  return(EWDB_RETURN_SUCCESS);
}  /* End PrepGetArrivalsWChanInfoExec() */


static int PostGetArrivalsWChanInfoExec(EWDB_ArrivalStruct * pArrivals, 
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
        ewdb_base_ErrorReport(hEWDBC, pCursor,"PostGetArrivalsWChanInfoExec:ewdb_base_SQLFetchRows",1);
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
        memset(&pArrivals[UCurr],0,sizeof(EWDB_ArrivalStruct));
        memset(&pChannelBuffer[UCurr],0,sizeof(EWDB_ChannelStruct));

        pArrivals[UCurr].pStation = &pChannelBuffer[UCurr].Comp;

        pArrivals[UCurr].idPick=*(int *)((sizeof(int)*BCurr) +(int)(pSS->FieldArray[0].pVal));

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
        strcpy(pArrivals[UCurr].szExternalPickID,pTemp);

        pTemp=(char *)((pSS->FieldArray[2].Ind*BCurr) +(int)(pSS->FieldArray[2].pVal) );
        pTemp[pSS->FieldArray[2].pRetLens[BCurr]]=0;
        strcpy(pArrivals[UCurr].szObsPhase,pTemp);

        pTemp=(char *)((pSS->FieldArray[3].Ind*BCurr) +(int)(pSS->FieldArray[3].pVal) );
        pTemp[pSS->FieldArray[3].pRetLens[BCurr]]=0;
        pArrivals[UCurr].tObsPhase=atof(pTemp);

        pTemp=(char *)((pSS->FieldArray[4].Ind*BCurr) +(int)(pSS->FieldArray[4].pVal) );
        pArrivals[UCurr].cMotion = pTemp[0];  /* we are looking for 1 character */

        pTemp=(char *)((pSS->FieldArray[5].Ind*BCurr) +(int)(pSS->FieldArray[5].pVal) );
        pArrivals[UCurr].cOnset = pTemp[0];  /* we are looking for 1 character */

        pTemp=(char *)((pSS->FieldArray[6].Ind*BCurr) +(int)(pSS->FieldArray[6].pVal) );
        pTemp[pSS->FieldArray[6].pRetLens[BCurr]]=0;
        pArrivals[UCurr].dSigma=atof(pTemp);

        pArrivals[UCurr].idOriginPick=*(int *)((sizeof(int)*BCurr) +(int)(pSS->FieldArray[7].pVal));

        pArrivals[UCurr].idOrigin=*(int *)((sizeof(int)*BCurr) +(int)(pSS->FieldArray[8].pVal));

        pTemp=(char *)((pSS->FieldArray[9].Ind*BCurr) +(int)(pSS->FieldArray[9].pVal) );
        pTemp[pSS->FieldArray[9].pRetLens[BCurr]]=0;
        strcpy(pArrivals[UCurr].szCalcPhase,pTemp);

        pTemp=(char *)((pSS->FieldArray[10].Ind*BCurr) +(int)(pSS->FieldArray[10].pVal) );
        pTemp[pSS->FieldArray[10].pRetLens[BCurr]]=0;
        pArrivals[UCurr].tCalcPhase=atof(pTemp);

        pTemp=(char *)((pSS->FieldArray[11].Ind*BCurr) +(int)(pSS->FieldArray[11].pVal) );
        pTemp[pSS->FieldArray[11].pRetLens[BCurr]]=0;
        pArrivals[UCurr].dWeight=(float)atof(pTemp);

        pTemp=(char *)((pSS->FieldArray[12].Ind*BCurr) +(int)(pSS->FieldArray[12].pVal) );
        pTemp[pSS->FieldArray[12].pRetLens[BCurr]]=0;
        pArrivals[UCurr].dDist=(float)atof(pTemp);

        pTemp=(char *)((pSS->FieldArray[13].Ind*BCurr) +(int)(pSS->FieldArray[13].pVal) );
        pTemp[pSS->FieldArray[13].pRetLens[BCurr]]=0;
        pArrivals[UCurr].dAzm=(float)atof(pTemp);

        pTemp=(char *)((pSS->FieldArray[14].Ind*BCurr) +(int)(pSS->FieldArray[14].pVal) );
        pTemp[pSS->FieldArray[14].pRetLens[BCurr]]=0;
        pArrivals[UCurr].dTakeoff=(float)atof(pTemp);

        pTemp=(char *)((pSS->FieldArray[15].Ind*BCurr) +(int)(pSS->FieldArray[15].pVal) );
        pTemp[pSS->FieldArray[15].pRetLens[BCurr]]=0;
        pArrivals[UCurr].tResPick=atof(pTemp);

        pTemp=(char *)((pSS->FieldArray[16].Ind*BCurr) +(int)(pSS->FieldArray[16].pVal) );
        pTemp[pSS->FieldArray[16].pRetLens[BCurr]]=0;
        strcpy(pArrivals[UCurr].szExtSource,pTemp);

        pChannelBuffer[UCurr].Comp.idComp=*(EWDBid *)((sizeof(int)*BCurr) +(int)(pSS->FieldArray[17].pVal));

        pTemp=(char *) 
           ((pSS->FieldArray[18].Ind*BCurr) +(int)(pSS->FieldArray[18].pVal));
        pTemp[pSS->FieldArray[18].pRetLens[BCurr]]=0;
        strncpy(pChannelBuffer[UCurr].Comp.Sta, pTemp, sizeof(pChannelBuffer[UCurr].Comp.Sta));

        pTemp=(char *) 
           ((pSS->FieldArray[19].Ind*BCurr) +(int)(pSS->FieldArray[19].pVal));
        pTemp[pSS->FieldArray[19].pRetLens[BCurr]]=0;
        strncpy(pChannelBuffer[UCurr].Comp.Comp, pTemp, sizeof(pChannelBuffer[UCurr].Comp.Comp));

        pTemp=(char *) 
           ((pSS->FieldArray[20].Ind*BCurr) +(int)(pSS->FieldArray[20].pVal));
        pTemp[pSS->FieldArray[20].pRetLens[BCurr]]=0;
        strncpy(pChannelBuffer[UCurr].Comp.Net, pTemp, sizeof(pChannelBuffer[UCurr].Comp.Net));

        pTemp=(char *) 
           ((pSS->FieldArray[21].Ind*BCurr) +(int)(pSS->FieldArray[21].pVal));
        pTemp[pSS->FieldArray[21].pRetLens[BCurr]]=0;
        strncpy(pChannelBuffer[UCurr].Comp.Loc, pTemp, sizeof(pChannelBuffer[UCurr].Comp.Loc));
        pChannelBuffer[UCurr].Comp.Loc[sizeof(pChannelBuffer[UCurr].Comp.Loc) - 1] = 0x00;
      
        pTemp=(char *) 
           ((pSS->FieldArray[22].Ind*BCurr) +(int)(pSS->FieldArray[22].pVal));
        pTemp[pSS->FieldArray[22].pRetLens[BCurr]]=0;
        pChannelBuffer[UCurr].Comp.Azm=(float)atof(pTemp);

        pTemp=(char *) 
           ((pSS->FieldArray[23].Ind*BCurr) +(int)(pSS->FieldArray[23].pVal));
        pTemp[pSS->FieldArray[23].pRetLens[BCurr]]=0;
        pChannelBuffer[UCurr].Comp.Dip=(float)atof(pTemp);

        pTemp=(char *) 
           ((pSS->FieldArray[24].Ind*BCurr) +(int)(pSS->FieldArray[24].pVal));
        pTemp[pSS->FieldArray[24].pRetLens[BCurr]]=0;
        pChannelBuffer[UCurr].Comp.Lat=(float)atof(pTemp);

        pTemp=(char *) 
           ((pSS->FieldArray[25].Ind*BCurr) +(int)(pSS->FieldArray[25].pVal));
        pTemp[pSS->FieldArray[25].pRetLens[BCurr]]=0;
        pChannelBuffer[UCurr].Comp.Lon=(float)atof(pTemp);

        pTemp=(char *) 
           ((pSS->FieldArray[26].Ind*BCurr) +(int)(pSS->FieldArray[26].pVal));
        pTemp[pSS->FieldArray[26].pRetLens[BCurr]]=0;
        pChannelBuffer[UCurr].Comp.Elev=(float)atof(pTemp);

        pArrivals[UCurr].idChan=*(EWDBid *)((sizeof(int)*BCurr) +(int)(pSS->FieldArray[27].pVal));
        pChannelBuffer[UCurr].Comp.idChan = pArrivals[UCurr].idChan;

        pTemp=(char *) 
           ((pSS->FieldArray[28].Ind*BCurr) +(int)(pSS->FieldArray[28].pVal));
        pTemp[pSS->FieldArray[28].pRetLens[BCurr]]=0;
        pChannelBuffer[UCurr].tOn = atof(pTemp);

        pTemp=(char *) 
           ((pSS->FieldArray[29].Ind*BCurr) +(int)(pSS->FieldArray[29].pVal));
        pTemp[pSS->FieldArray[29].pRetLens[BCurr]]=0;
        pChannelBuffer[UCurr].tOff = atof(pTemp);

        pChannelBuffer[UCurr].idChanT=*(EWDBid *)((sizeof(int)*BCurr) +(int)(pSS->FieldArray[30].pVal));
        pChannelBuffer[UCurr].idCompT=*(EWDBid *)((sizeof(int)*BCurr) +(int)(pSS->FieldArray[31].pVal));

        pTemp=(char *) 
           ((pSS->FieldArray[32].Ind*BCurr) +(int)(pSS->FieldArray[32].pVal));
        pTemp[pSS->FieldArray[32].pRetLens[BCurr]]=0;
        pChannelBuffer[UCurr].tOnCompT = atof(pTemp);

        pTemp=(char *) 
           ((pSS->FieldArray[33].Ind*BCurr) +(int)(pSS->FieldArray[33].pVal));
        pTemp[pSS->FieldArray[33].pRetLens[BCurr]]=0;
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
      while(!ewdb_base_SQLFetchRows(pCursor, iRecordsPerBuffer));
    }

    if(ewdb_base_GetCursorRetCode(pCursor) != EWDB_SQL_ERROR_NO_DATA)
    {
      ewdb_base_ErrorReport(hEWDBC, pCursor,"PostGetGetArrivalsWChanInfoExec:ewdb_base_SQLFetchRows",2);
      return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
    }
    /* else */
  }  /* End if(RowsRetrieved > BufferRecLen) */
  
  RowsProcessed = ewdb_base_GetCursorRowsProcessedCount(pCursor);

  ewdb_base_ReleaseCursor(pCursor);

  return(RowsProcessed);
}  /* End PostGetGetArrivalsWChanInfoExec() */


