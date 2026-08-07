/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_api_GetArrivals.c,v 1.10 2005/06/27 15:31:36 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_api_GetArrivals.c,v $
 *     Revision 1.10  2005/06/27 15:31:36  davidk
 *     Fixed bugs introduced during global fetch/replace during DB cleanup.
 *
 *     Revision 1.9  2005/06/17 21:00:03  davidk
 *     Fixed problems in SQL code caused by errant global replacements during API cleanup.
 *
 *     Revision 1.8  2005/06/15 19:03:12  davidk
 *     DB API Cleanup
 *
 *     Revision 1.7  2005/01/28 19:29:06  mark
 *     Added sSource
 *
 *     Revision 1.6  2004/11/06 04:27:05  davidk
 *     Updated phasename field(s) in the SQL statement structs, to use the new
 *     EWDB_PHASENAME_SIZE length, as it is used in EWDB_ArrivalStructs,
 *     which is where the results are headed.
 *
 *     Revision 1.5  2003/09/16 16:55:36  davidk
 *      starting with sCalcPhase, they were off by two.
 *     General API cleanup to fix memory leaks.
 *
 *     Revision 1.4  2003/08/21 00:51:38  davidk
 *     Restructured API code that deals with dynamic allocation of memory.
 *
 *     Revision 1.3  2001/07/12 21:07:17  davidk
 *     cleaned up the function as part of the API cleanup.  Added improved
 *     error logging, handling, and reporting.
 *
 *     Revision 1.2  2001/05/15 02:16:20  davidk
 *     Moved functions around between the apps, DB API, and DB API INTERNAL
 *     levels.  Renamed functions and files.  Added support for amplitude
 *     magnitude types.  Reformatted makefiles.
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
        "select idPick, xidExternal, idChan, sPhase, tPhase, "
        " cMotion, cOnset, dSigma, "
        " idOriginPick, idOrigin, sCalcPhase, tCalcPhase, "
        " dWeight, dDist, dAzm, dTakeoff, tResPick, sSource "
        "from ALL_ARRIVAL_INFO "
        "where idOrigin = :IN_idOrigin";

static EWDB_OCI_SFS SQLParamsBindArray[] =

{
  {0,1,0,0,0,OA_INT,"1idPick"},
  {0,1,16,0,0,OA_SZ,"2xidExternal"},
  {0,1,0,0,0,OA_INT,"3idChan"},
  {0,1,EWDB_PHASENAME_SIZE,
         0,0,OA_SZ, "4sPhase"},
  {0,1,20,0,0,OA_DOUBLE,"5tPhase"},
  {0,1,2,0,0,OA_SZ,"6cMotion"},
  {0,1,2,0,0,OA_SZ,"7cOnset"},
  {0,1,20,0,0,OA_DOUBLE,"8dSigma"},
  {0,1,0,0,0,OA_INT,"9idOriginPick"},
  {0,1,0,0,0,OA_INT,"10idOrigin"},
  {0,1,EWDB_PHASENAME_SIZE,
         0,0,OA_SZ, "11sCalcPhase"},
  {0,1,20,0,0,OA_DOUBLE,"12tCalcPhase"},
  {0,1,20,0,0,OA_FLOAT,"13dWeight"},
  {0,1,20,0,0,OA_FLOAT,"14dDist"},
  {0,1,20,0,0,OA_FLOAT,"15dAzm"},
  {0,1,20,0,0,OA_FLOAT,"16dTakeoff"},
  {0,1,20,0,0,OA_DOUBLE,"17tResPick"},
  {0,1,16,0,0,OA_SZ,"18sSource"},
  {0,1,0,0,0,OA_INT,":IN_idOrigin"}
};

/* define the total number of fields to be bound */
#define NUM_FIELDS 19

/* Insertion Struct for GetStationList statement */
static EWDB_OCIStatementStruct SSStatement;

/* Temporary Storage Buffer params */
#define BUFFERSIZE 8192
static int    iRecordSize;
static int    iRecordsPerBuffer;
static char * pLocalBuffer=NULL;

static int    bInitialized=FALSE;

static  int     Local_idOrigin;


static int PrepGetArrivalsExec(EWDBid IN_idOrigin,EWDB_Cursor * ppCursor);
static int PostGetArrivalsExec(EWDB_ArrivalStruct * pBuffer,int BufferRecLen);
static int InitGetArrivalsStatement(char * szStatement, EWDB_OCIStatementStruct *pSS);



/********************************************************************** 
   Used to get all of arrivals(phase arrivals at different stations)
  for a given origin.
  pArrivals               Pointer to a buffer allocated by the caller
	 where the function is to stick the arrivals.
  pNumItemsFound:      The number of Arrivals found(in the DB) for the Origin.
  pNumItemsRetrieved:  The number of Arrivals retrieved from the DB.
  BufferLen:              The length of the buffer in terms of ArrivalStructs.

    Return Value:   
     EWDB_RETURN_FAILURE     unknown error
     EWDB_RETURN_WARNING     success, but the buffer was not able to accommodate
                              all of the arrivals found.
     EWDB_RETURN_SUCCESS     success 

    Other Details:  Caller is responsible for allocating
	space for the arrivals buffer.
***********************************************************************/
int ewdb_api_GetArrivals(EWDBid IN_idOrigin, EWDB_ArrivalStruct * pArrivals,
                         int * pNumItemsFound, int * pNumItemsRetrieved,
                         int BufferLen)
{

  EWDB_Cursor  pCursor;

  ewdb_base_SetLastOraAPIActionTime();

  if(IN_idOrigin <= 0 || !pArrivals || !pNumItemsFound || !pNumItemsRetrieved)
  {
    logit("","ewdb_api_GetArrivals(): ERROR!  Invalid parameter(s) passed in: "
             "IN_idOrigin %d, pArrivals %u, pNumItemsFound %u, pNumItemsRetrieved %u\n",
          IN_idOrigin, pArrivals, pNumItemsFound, pNumItemsRetrieved);
    return(EWDB_RETURN_FAILURE);
  }

  if( ewdb_base_Reconnect() != EWDB_RETURN_SUCCESS )
  {
    return( EWDB_RETURN_FAILURE );
  }

  if( PrepGetArrivalsExec(IN_idOrigin,&pCursor) != EWDB_RETURN_SUCCESS)
  {
    logit("","ewdb_api_GetArrivals(): PrepGetArrivalsExec() failed!\n");
    return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }
  
  if( ewdb_base_SQLExecute(pCursor) )
  {
    ewdb_base_ErrorReport(hEWDBC, pCursor,"ewdb_api_GetArrivals(): ewdb_base_SQLExecute",1);
    return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  } 

 /* Commit the transaction (all the previous inserts!)
  In Case there is any auditing or logging or debug
  changes made in the stored procedures.
  ****************************************************/
  if (ewdb_base_SQLCommit (hEWDBC))
  {
    ewdb_base_ErrorReport (hEWDBC, hEWDBC,"ewdb_api_GetArrivals(): ewdb_base_SQLCommit",2);
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  }
 
  if((*pNumItemsFound=PostGetArrivalsExec(pArrivals,BufferLen)) == EWDB_RETURN_FAILURE)
  {
    logit("","ewdb_api_GetArrivals(): PostGetArrivalsExec() failed!\n");
    return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }

  ewdb_base_SetLastOraAPIActionTime();

  if(*pNumItemsFound <= BufferLen)
  {
    *pNumItemsRetrieved = *pNumItemsFound;
    return(EWDB_RETURN_SUCCESS);
  }
  else
  {
    *pNumItemsRetrieved = BufferLen;
    return(EWDB_RETURN_WARNING);
  }
}  /* end ewdb_api_GetArrivals() */


static int InitGetArrivalsStatement(char * szStatement, EWDB_OCIStatementStruct *pSS)
{
  int LastSize,i;  /* Size of last column array */

  if(!pLocalBuffer)
  {
    /*Allocate space for returned data - never freed */
    pLocalBuffer=malloc(BUFFERSIZE);

    iRecordSize=0;
    iRecordSize += sizeof(EWDBid);        /*idPick*/
    iRecordSize += pSS->FieldArray[1].Ind;/*xidExternal*/
    iRecordSize += sizeof(EWDBid);        /*idChan*/
    iRecordSize += pSS->FieldArray[3].Ind;/*sPhase*/
    iRecordSize += pSS->FieldArray[4].Ind;/*tPhase*/
    iRecordSize += pSS->FieldArray[5].Ind;/*cMotion*/
    iRecordSize += pSS->FieldArray[6].Ind;/*cOnset*/
    iRecordSize += pSS->FieldArray[7].Ind;/*dSigma*/
    iRecordSize += sizeof(EWDBid);        /*idOriginPick*/
    iRecordSize += sizeof(EWDBid);        /*Local_idOrigin*/
    iRecordSize += pSS->FieldArray[10].Ind;/*sCalcPhase*/
    iRecordSize += pSS->FieldArray[11].Ind;/*tCalcPhase*/
    iRecordSize += pSS->FieldArray[12].Ind;/*dWeight*/
    iRecordSize += pSS->FieldArray[13].Ind;/*dDist*/
    iRecordSize += pSS->FieldArray[14].Ind;/*dAzm*/
    iRecordSize += pSS->FieldArray[15].Ind;/*dTakeOff*/
    iRecordSize += pSS->FieldArray[16].Ind;/*tResPick*/
    iRecordSize += pSS->FieldArray[17].Ind;/*sSource*/

    /* Make the number of records a multiple of 8 to avoid alignment problems on UNIX */
    iRecordsPerBuffer= (BUFFERSIZE/iRecordSize) & 0xfffffff8;
    
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
    pSS->FieldArray[8].pVal=(void *)(
      (int)(pSS->FieldArray[7].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=sizeof(int);
    pSS->FieldArray[9].pVal=(void *)(
      (int)(pSS->FieldArray[8].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=sizeof(int);
    pSS->FieldArray[10].pVal=(void *)(
      (int)(pSS->FieldArray[9].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[10].Ind; /* sCalcPhase */
    pSS->FieldArray[11].pVal=(void *)(
      (int)(pSS->FieldArray[10].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[11].Ind; /* tCalcPhase */
    pSS->FieldArray[12].pVal=(void *)(
      (int)(pSS->FieldArray[11].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[12].Ind; /* dWeight */
    pSS->FieldArray[13].pVal=(void *)(
      (int)(pSS->FieldArray[12].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[13].Ind; /* dDist */
    pSS->FieldArray[14].pVal=(void *)(
      (int)(pSS->FieldArray[13].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[14].Ind; /* dAzm */
    pSS->FieldArray[15].pVal=(void *)(
      (int)(pSS->FieldArray[14].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[15].Ind; /* dTakeoff */
    pSS->FieldArray[16].pVal=(void *)(
      (int)(pSS->FieldArray[15].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[16].Ind; /* tResPick */
    pSS->FieldArray[17].pVal=(void *)(
      (int)(pSS->FieldArray[16].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[17].Ind; /* sSource */
    pSS->FieldArray[18].pVal=&Local_idOrigin;
  } /* end if(!pLocalBuffer) */

  if(!pLocalBuffer)
  {
    logit("","InitGetArrivalsStatement(): malloc of pLocalBuffer "
          "failed! Returning.\n");
    return(EWDB_RETURN_FAILURE);
  }

  return(ewdb_base_RequestCursor(szStatement, pSS,0/*don't force rebind*/));
}  /* end InitGetArrivalsStatement() */


static int PrepGetArrivalsExec(EWDBid IN_idOrigin, EWDB_Cursor * ppCursor)
{
  Local_idOrigin = IN_idOrigin;

  /* Initialize the statement parameters */
  if(!bInitialized)
  {
    bInitialized = TRUE;
    memset(&SSStatement,0,sizeof(SSStatement));
    SSStatement.NumOfFields=NUM_FIELDS;
    SSStatement.FieldArray=SQLParamsBindArray;
    SSStatement.RecordSize=0;
  }

  if(InitGetArrivalsStatement(SQL_STRING, &SSStatement) != EWDB_RETURN_SUCCESS)
  {
    *ppCursor = NULL;
    return(EWDB_RETURN_FAILURE);
  }

  *ppCursor=SSStatement.pCda;

  return(EWDB_RETURN_SUCCESS);
}  /* end PrepGetArrivalsExec() */


static int PostGetArrivalsExec(EWDB_ArrivalStruct * pBuffer,int BufferRecLen)
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
        ewdb_base_ErrorReport(hEWDBC, pCursor,"PostGetArrivalsExec:ewdb_base_SQLFetchRows",1);
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

        pBuffer[UCurr].idOriginPick=*(int *)((sizeof(int)*BCurr) +(int)(pSS->FieldArray[8].pVal));

        pBuffer[UCurr].idOrigin=*(int *)((sizeof(int)*BCurr) +(int)(pSS->FieldArray[9].pVal));

        pTemp=(char *)((pSS->FieldArray[10].Ind*BCurr) +(int)(pSS->FieldArray[10].pVal) );
        pTemp[pSS->FieldArray[10].pRetLens[BCurr]]=0;
        strcpy(pBuffer[UCurr].szCalcPhase,pTemp);

        pTemp=(char *)((pSS->FieldArray[11].Ind*BCurr) +(int)(pSS->FieldArray[11].pVal) );
        pTemp[pSS->FieldArray[11].pRetLens[BCurr]]=0;
        pBuffer[UCurr].tCalcPhase=atof(pTemp);

        pTemp=(char *)((pSS->FieldArray[12].Ind*BCurr) +(int)(pSS->FieldArray[12].pVal) );
        pTemp[pSS->FieldArray[12].pRetLens[BCurr]]=0;
        pBuffer[UCurr].dWeight=(float)atof(pTemp);

        pTemp=(char *)((pSS->FieldArray[13].Ind*BCurr) +(int)(pSS->FieldArray[13].pVal) );
        pTemp[pSS->FieldArray[13].pRetLens[BCurr]]=0;
        pBuffer[UCurr].dDist=(float)atof(pTemp);

        pTemp=(char *)((pSS->FieldArray[14].Ind*BCurr) +(int)(pSS->FieldArray[14].pVal) );
        pTemp[pSS->FieldArray[14].pRetLens[BCurr]]=0;
        pBuffer[UCurr].dAzm=(float)atof(pTemp);

        pTemp=(char *)((pSS->FieldArray[15].Ind*BCurr) +(int)(pSS->FieldArray[15].pVal) );
        pTemp[pSS->FieldArray[15].pRetLens[BCurr]]=0;
        pBuffer[UCurr].dTakeoff=(float)atof(pTemp);

        pTemp=(char *)((pSS->FieldArray[16].Ind*BCurr) +(int)(pSS->FieldArray[16].pVal) );
        pTemp[pSS->FieldArray[16].pRetLens[BCurr]]=0;
        pBuffer[UCurr].tResPick=atof(pTemp);

        pTemp=(char *)((pSS->FieldArray[17].Ind*BCurr) +(int)(pSS->FieldArray[17].pVal) );
        pTemp[pSS->FieldArray[17].pRetLens[BCurr]]=0;
        strcpy(pBuffer[UCurr].szExtSource,pTemp);
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
      ewdb_base_ErrorReport(hEWDBC, pCursor,"PostGetArrivalsExec:ewdb_base_SQLFetchRows",2);
      return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
    }
    /* else */
  }  /* End if(RowsRetrieved > BufferRecLen) */
  
  RecordsProcessed = ewdb_base_GetCursorRowsProcessedCount(pCursor);
  ewdb_base_ReleaseCursor(pCursor);

  return(RecordsProcessed);
}  /* end PostGetArrivalsExec() */
