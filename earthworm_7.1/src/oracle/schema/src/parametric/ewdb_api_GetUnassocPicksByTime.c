/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_api_GetUnassocPicksByTime.c,v 1.4 2005/06/27 15:31:36 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_api_GetUnassocPicksByTime.c,v $
 *     Revision 1.4  2005/06/27 15:31:36  davidk
 *     Fixed bugs introduced during global fetch/replace during DB cleanup.
 *
 *     Revision 1.3  2005/06/15 19:03:12  davidk
 *     DB API Cleanup
 *
 *     Revision 1.2  2005/05/19 23:23:50  davidk
 *     Optimized query for better performance.
 *
 *     Revision 1.1  2005/03/29 17:32:20  davidk
 *     Added ewdb_api_GetUnassocPicksByTime
 *
 *
 */


#include <ewdb_ora_api.h>
#include <ewdb_cli_base.h>
#include <ewdb_ew_oci_base.h>


static char SQL_STRING[] =
  "select /*+ INDEX(pick pick_tphase) */ idPick, xidExternal, idChan, sPhase, tPhase, "
  "       cMotion, cOnset, dSigma                      "
  " from pick                                          "
  " where idpick in                                    "
  "   (                                                "
  "    select idpick from pick                         "
  "     where tPhase >= :IN_tStart                     "     
  "       and tPhase <= :IN_tEnd                       "    
  "    MINUS                                           "
  "    select  /*+ INDEX(o origin_torigin) */ pk.idpick"
  "     from origin o, prefer p, originpick op, event e, pick pk "
  "     where o.tOrigin > (:IN_tStart - 3600)          "
  "       and o.tOrigin < :IN_tEnd                     "
  "       and o.idorigin = p.idpreforigin              "
  "       and o.idOrigin = op.idOrigin                 "
  "       and  p.idevent  = e.idevent                  "
  "       and  e.iDubiocity = 0                        "
  "       and op.idpick = pk.idpick                    "
  "   )                                                "
  " order by idpick                                    "
  ;

static char SQL_STRING_CHAN[] =
  "select /*+ INDEX(pick pick_tphase) */ idPick, xidExternal, idChan, sPhase, tPhase, "
  "       cMotion, cOnset, dSigma                      "
  " from pick                                          "
  " where idpick in                                    "
  "   (                                                "
  "    select idpick from pick                         "
  "     where tPhase >= :IN_tStart                     "     
  "       and tPhase <= :IN_tEnd                       "    
  "       and Local_idChan  = :IN_idChan                     "    
  "    MINUS                                           "
  "    select  /*+ INDEX(o origin_torigin) */ pk.idpick"  
  "     from origin o, prefer p, originpick op, event e, pick pk "
  "     where o.tOrigin > (:IN_tStart - 3600)          "
  "       and o.tOrigin < :IN_tEnd                     "
  "       and o.idorigin = p.idpreforigin              "
  "       and o.idOrigin = op.idOrigin                 "
  "       and  p.idevent  = e.idevent                  "
  "       and  e.iDubiocity = 0                        "
  "       and op.idpick = pk.idpick                    "
  "   )                                                "
  " order by idpick                                    "
  ;


static EWDB_OCI_SFS SQLParamsBindArray[] =
{
  {0,1,0,0,0,OA_INT,"1idPick"},
  {0,1,16,0,0,OA_SZ,"2xidExternal"},
  {0,1,0,0,0,OA_INT,"3idChan"},
  {0,1,EWDB_PHASENAME_SIZE,
          0,0,OA_SZ,"4sPhase"},
  {0,1,20,0,0,OA_DOUBLE,"5tPhase"},
  {0,1,2,0,0,OA_SZ,"6cMotion"},
  {0,1,2,0,0,OA_SZ,"7cOnset"},
  {0,1,20,0,0,OA_DOUBLE,"8dSigma"},
  {0,1,0,0,0,OA_INT,":IN_tStart"},
  {0,1,0,0,0,OA_INT,":IN_tEnd"},
  {0,0,0,0,0,OA_EWDBID,":IN_idChan"}
};

/* define the total number of fields to be bound */
#define NUM_FIELDS 11

/* Insertion Struct for GetUnassocPicksByTime statement */
static EWDB_OCIStatementStruct SSStatement;

/* Temporary Storage Buffer params */
#define BUFFERSIZE 8192
static int    iRecordSize;
static int    iRecordsPerBuffer;
static char * pLocalBuffer=NULL;

static int    bInitialized=FALSE;

static EWDBid Local_idChan;
static time_t Local_tStart, Local_tEnd;


static int PrepGetUnassocPicksByTimeExec(time_t IN_tStart, time_t IN_tEnd, EWDBid IN_idChan,
                                         EWDB_Cursor * ppCursor);
static int PostGetUnassocPicksByTimeExec(EWDB_ArrivalStruct * pBuffer,int BufferRecLen);
static int InitGetUnassocPicksByTimeStatement(char * szStatement, EWDB_OCIStatementStruct *pSS);



/***************************************************************************
  ewdb_api_GetUnassocPicksByTime() returns a list of picks from the DB based on
   time and optionally Local_idChan.
   Set IN_idChan <= 0 to query by time only.
   Set IN_idChan to a valid Local_idChan to query by time/chan.
   Used to get all of arrivals(phase arrivals at different stations)
   for a given time window and chan.

  pArrivals               Pointer to a buffer allocated by the caller
   where the function is to stick the arrivals.
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
int ewdb_api_GetUnassocPicksByTime(time_t IN_tStart, time_t IN_tEnd, EWDBid IN_idChan,
                                   EWDB_ArrivalStruct * pArrivals,
                                   int * pNumItemsFound, int * pNumItemsRetrieved,
                                   int iBufferLen)
{

  EWDB_Cursor  pCursor;
  ewdb_base_SetLastOraAPIActionTime();

  if( ewdb_base_Reconnect() != EWDB_RETURN_SUCCESS )
  {
    return( EWDB_RETURN_FAILURE );
  }

  if( PrepGetUnassocPicksByTimeExec(IN_tStart, IN_tEnd, IN_idChan, &pCursor)
     != EWDB_RETURN_SUCCESS)
  {
    logit("","ewdb_api_GetUnassocPicksByTime(): PrepGetUnassocPicksByTimeExec() failed!\n");
    return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }
  
  if( ewdb_base_SQLExecute(pCursor) )
  {
    ewdb_base_ErrorReport(hEWDBC, pCursor,"ewdb_api_GetUnassocPicksByTime(): ewdb_base_SQLExecute",1);
    return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  } 

 /* Commit the transaction (all the previous inserts!)
  In Case there is any auditing or logging or debug
  changes made in the stored procedures.
  ****************************************************/
  if (ewdb_base_SQLCommit (hEWDBC))
  {
    ewdb_base_ErrorReport (hEWDBC, hEWDBC,"DeletePolygon:ewdb_base_SQLCommit",2);
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  }
 
  if((*pNumItemsFound=PostGetUnassocPicksByTimeExec(pArrivals,iBufferLen)) == EWDB_RETURN_FAILURE)
  {
    logit("","ewdb_api_GetUnassocPicksByTime(): PostGetUnassocPicksByTimeExec() failed!\n");
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
}  /* end ewdb_api_GetUnassocPicksByTime() */


static int InitGetUnassocPicksByTimeStatement(char * szStatement, EWDB_OCIStatementStruct *pSS)
{

  int LastSize,i;  /* Size of last column array */


  if(!pLocalBuffer)
  {
    /*Allocate space for returned data - never freed */
    pLocalBuffer=malloc(BUFFERSIZE);


    iRecordSize=0;
    iRecordSize += sizeof(EWDBid);        /*idPick*/
    iRecordSize += pSS->FieldArray[1].Ind;/*xidExternal*/
    iRecordSize += sizeof(EWDBid);        /*Local_idChan*/
    iRecordSize += pSS->FieldArray[3].Ind;/*sPhase*/
    iRecordSize += pSS->FieldArray[4].Ind;/*tPhase*/
    iRecordSize += pSS->FieldArray[5].Ind;/*cMotion*/
    iRecordSize += pSS->FieldArray[6].Ind;/*cOnset*/
    iRecordSize += pSS->FieldArray[7].Ind;/*dSigma*/

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
    LastSize=sizeof(int);
    pSS->FieldArray[1].pVal=(void *)(
     (int)(pSS->FieldArray[0].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[1].Ind; /* xidExternal */
    pSS->FieldArray[2].pVal=(void *)(
     (int)(pSS->FieldArray[1].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=sizeof(int);
    pSS->FieldArray[3].pVal=(void *)(
     (int)(pSS->FieldArray[2].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[3].Ind; /* sPhase */
    pSS->FieldArray[4].pVal=(void *)(
     (int)(pSS->FieldArray[3].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[4].Ind; /* tPhase */
    pSS->FieldArray[5].pVal=(void *)(
     (int)(pSS->FieldArray[4].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[5].Ind; /* cMotion */
    pSS->FieldArray[6].pVal=(void *)(
     (int)(pSS->FieldArray[5].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[6].Ind; /* cOnset */
    pSS->FieldArray[7].pVal=(void *)(
     (int)(pSS->FieldArray[6].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[7].Ind; /* dSigma */
    pSS->FieldArray[8].pVal=&Local_tStart;
    pSS->FieldArray[9].pVal=&Local_tEnd;
    pSS->FieldArray[10].pVal=&Local_idChan;
  } /* end if(!pLocalBuffer) */
  
  if(!pLocalBuffer)
    logit("","InitGetUnassocPicksByTimeStatement: malloc of pLocalBuffer "
          "failed! Returning.\n");


  return(ewdb_base_RequestCursor(szStatement, pSS,0/*don't force rebind*/));
}  /* end InitGetUnassocPicksByTimeStatement() */


static int PrepGetUnassocPicksByTimeExec(time_t IN_tStart, time_t IN_tEnd, EWDBid IN_idChan,
               EWDB_Cursor * ppCursor)
{
  char * pSQL_STRING;

  /* Initialize the statement parameters */
  if(!bInitialized)
  {
    bInitialized = TRUE;
    memset(&SSStatement,0,sizeof(SSStatement));
    SSStatement.NumOfFields=NUM_FIELDS;
    SSStatement.FieldArray=SQLParamsBindArray;
    SSStatement.RecordSize=0;
  }
  
  Local_idChan = IN_idChan;
  Local_tStart = IN_tStart;
  Local_tEnd   = IN_tEnd;

  if(Local_idChan > 0)
  {
    pSQL_STRING = SQL_STRING_CHAN;
    SSStatement.FieldArray[10].UseField = TRUE;
  }
  else
  {
    pSQL_STRING = SQL_STRING;
    SSStatement.FieldArray[10].UseField = FALSE;
  }


  if(InitGetUnassocPicksByTimeStatement(pSQL_STRING, &SSStatement) == EWDB_RETURN_FAILURE)
    return(EWDB_RETURN_FAILURE);

  *ppCursor=SSStatement.pCda;

  return(EWDB_RETURN_SUCCESS);
}  /* end PrepGetUnassocPicksByTimeExec() */


static int PostGetUnassocPicksByTimeExec(EWDB_ArrivalStruct * pBuffer,int BufferRecLen)
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
        ewdb_base_ErrorReport(hEWDBC, pCursor,"PostGetUnassocPicksByTimeExec:ewdb_base_SQLFetchRows",1);
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

        pBuffer[UCurr].idChan=*(int *)((sizeof(int)*BCurr) +(int)(pSS->FieldArray[2].pVal));

        pTemp=(char *)((pSS->FieldArray[3].Ind*BCurr) +(int)(pSS->FieldArray[3].pVal) );
        pTemp[pSS->FieldArray[3].pRetLens[BCurr]]=0;
        strcpy(pBuffer[UCurr].szObsPhase,pTemp);

        pTemp=(char *)((pSS->FieldArray[4].Ind*BCurr) +(int)(pSS->FieldArray[4].pVal) );
        pTemp[pSS->FieldArray[4].pRetLens[BCurr]]=0;
        pBuffer[UCurr].tObsPhase=atof(pTemp);

        pTemp=(char *)((pSS->FieldArray[5].Ind*BCurr) +(int)(pSS->FieldArray[5].pVal) );
        pBuffer[UCurr].cMotion = pTemp[0];  /* we are looking for 1 character */

        pTemp=(char *)((pSS->FieldArray[6].Ind*BCurr) +(int)(pSS->FieldArray[6].pVal) );
        pBuffer[UCurr].cOnset = pTemp[0];  /* we are looking for 1 character */

        pTemp=(char *)((pSS->FieldArray[7].Ind*BCurr) +(int)(pSS->FieldArray[7].pVal) );
        pTemp[pSS->FieldArray[7].pRetLens[BCurr]]=0;
        pBuffer[UCurr].dSigma=atof(pTemp);

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
      ewdb_base_ErrorReport(hEWDBC, pCursor,"PostGetUnassocPicksByTimeExec:ewdb_base_SQLFetchRows",2);
      return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
    }
    /* else */
  }  /* End if(RowsRetrieved > BufferRecLen) */
  
  RecordsProcessed = ewdb_base_GetCursorRowsProcessedCount(pCursor);
  ewdb_base_ReleaseCursor(pCursor);

  return(RecordsProcessed);
}  /* end PostGetUnassocPicksByTimeExec() */
