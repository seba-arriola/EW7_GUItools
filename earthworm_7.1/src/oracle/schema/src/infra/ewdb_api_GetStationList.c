/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_api_GetStationList.c,v 1.8 2005/06/10 16:27:59 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_api_GetStationList.c,v $
 *     Revision 1.8  2005/06/10 16:27:59  davidk
 *     DB API Cleanup
 *
 *     Revision 1.7  2004/05/12 20:32:21  michelle
 *     removed from sql string select where clause the condition on lat and lon
 *     since in theory the system will no longer create stations at 0, 0 unless
 *     the meta data says so, thus all data is assumed "valid" so get all stations
 *     regardless of lat lon
 *
 *     Revision 1.6  2003/09/16 17:00:14  davidk
 *     General API Cleanup of internal structures.
 *
 *     Revision 1.5  2003/08/21 00:42:30  davidk
 *     Restructured API code that deals with dynamic allocation of memory.
 *
 *     Revision 1.4  2003/08/06 19:20:48  lucky
 *     Modified so that we return only those station with valid Lat and Lon fields (both must be nonzero)
 *
 *     Revision 1.3  2001/07/10 23:25:51  davidk
 *     rewrote the function as part of the EWDB API cleanup.
 *
 *     Revision 1.2  2001/05/15 02:16:30  davidk
 *     Moved functions around between the apps, DB API, and DB API INTERNAL
 *     levels.  Renamed functions and files.  Added support for amplitude
 *     magnitude types.  Reformatted makefiles.
 *
 *     Revision 1.1  2001/02/28 17:19:22  lucky
 *     Initial revision
 *
 *     Revision 1.7  2001/02/21 10:58:33  davidk
 *     Code was retrofitted to use the new db_cli_base API
 *     in lieu of the oci_base() and OCI7 function API's.layer, which
 *     provides abstraction from the OCI7 function calls to
 *     ora_api layer code.  See comments in
 *     schema/src/include/internal/ewdb_cli_base.h for more info
 *     on the details of the changes made.
 *
 *     Revision 1.6  2000/11/07 23:13:16  davidk
 *     changed the call to ewdb_base_RequestCursor(), in order to force
 *     the rebinding of the C to SQL variables before each execution.
 *     This is a temporary fix for a bug in the binding logic that causes
 *     successive calls using the same cursor to fail.
 *
 *     Revision 1.5  2000/06/21 22:52:19  lucky
 *      Cleaned up logit calls to make log files more readable.
 *
 *     Revision 1.4  2000/05/15 23:02:38  davidk
 *     Added prototypes for the functions implemented in this file.
 *     Reformatted whitespace(put spaces in inplace of tabs).  Added
 *     some additional comments, and changed informational logits
 *     to only print when the EWDB_Debug flag is on.  Stopped all logits
 *     from printing to stderr, so as to be compatible with CGI-BIN web
 *     apps.
 *
 *     Revision 1.3  1999/11/09 18:39:27  lucky
 *     *** empty log message ***
 *
 *
 */

#include <ewdb_ora_api.h>
#include <ewdb_cli_base.h>
#include <ewdb_ew_oci_base.h>

static char SQL_STRING[] =
         "select idChan, idComp, sSta, sComp, sNet, sLoc, dAzm, dDip, "
         " dLat, dLon, dElev "
         "from ALL_STATION_INFO "
         "where dLat >= :MinLat AND dLat <= :MaxLat "
         "  AND dLon >= :MinLon AND dLon <= :MaxLon " 
         "  AND tOn <= :ReqTime AND tOff >= :ReqTime";

         /** System used to insert unkown channels at zero, zero. this behavior removed 
          *  so remove this conditional check for 0 lat 0 lon  **********************/ 
         /********* "  AND dLat != 0 OR dLon != 0"; ***********/


static EWDB_OCI_SFS SQLParamsBindArray[] = 
{

  {0,1,0,0,0,OA_INT,"1idChan"},
  {0,1,0,0,0,OA_INT,"2idComp"},
  {0,1,10,0,0,OA_SZ,"3sSta"},
  {0,1,10,0,0,OA_SZ,"4sComp"},
  {0,1,10,0,0,OA_SZ,"5sNet"},
  {0,1,10,0,0,OA_SZ,"6sLoc"},
  {0,1,20,0,0,OA_FLOAT,"7dAzm"},
  {0,1,20,0,0,OA_FLOAT,"8dDip"},
  {0,1,20,0,0,OA_DOUBLE,"9dLat"},
  {0,1,20,0,0,OA_DOUBLE,"10dLon"},
  {0,1,20,0,0,OA_DOUBLE,"11dElev"},
  {0,1,0,0,0,OA_DOUBLE,":MinLat"},
  {0,1,0,0,0,OA_DOUBLE,":MaxLat"},
  {0,1,0,0,0,OA_DOUBLE,":MinLon"},
  {0,1,0,0,0,OA_DOUBLE,":MaxLon"},
  {0,1,0,0,0,OA_DOUBLE,":ReqTime"}
};

/* define the total number of fields to be bound */
#define NUM_FIELDS 16

/* Insertion Struct for GetStationList statement */
static EWDB_OCIStatementStruct SSStatement;


/* Temporary Storage Buffer params */
#define BUFFERSIZE 8192
static int    iRecordSize;
static int    iRecordsPerBuffer;
static char * pLocalBuffer=NULL;

static int    bInitialized=FALSE;

static char Local_szMinLat[15],Local_szMaxLat[15],
            Local_szMinLon[15],Local_szMaxLon[15],
            Local_szReqTime[15];


/********************************
      FUNCTION PROTOTYPES
********************************/
static int PrepGetStationListExec(double MinLat, double MaxLat, 
                           double MinLon, double MaxLon,
                           double ReqTime, EWDB_Cursor * ppCursor);
static int PostGetStationListExec(EWDB_StationStruct * pBuffer, int BufferRecLen);
static int InitGetStationListStatement(char * szStatement, 
                                EWDB_OCIStatementStruct *pSS);


/* 
   Retrieves a list of component/channels given a lat/lon box and time.
   Component attributes for the given time are included.(Lat,Lon,etc.)
   Used to get all of the stations from a given coordinate box.
 (MinLat,MinLon) and(MaxLat,MaxLon) define two vertices(?) of
  a box, for which to retrieve a list of stations within the box.
  Note*: Be careful of the effects of negative Lat/Lon on Min/Max

  pBuffer is a pointer to a buffer allocated by the caller
   where the function is to stick the stations.
  BufferLen is the length of the buffer in StationStructs allocated 
   by the caller.

    Return Value:   
     -1     when bad coordinates are passed by the caller
     -x     when the buffer passed by the caller is too
           small, where x is the neccessary size of BufferLen
           to get all of the picks for the given origin.
      0    on success.

    Other Details:  Caller is responsible for allocating
  space for the stations buffer.
*/
int ewdb_api_GetStationList(double MinLat, double MaxLat, double MinLon,
                            double MaxLon, double ReqTime, 
                            EWDB_StationStruct * pBuffer,
                            int * pNumStationsFound, 
                            int * pNumStationsRetrieved,
                            int BufferLen)
{

  EWDB_Cursor  pCursor;
 
  ewdb_base_SetLastOraAPIActionTime();

  if( ewdb_base_Reconnect() != EWDB_RETURN_SUCCESS )
  {
    return( EWDB_RETURN_FAILURE );
  }

  if( PrepGetStationListExec(MinLat,MaxLat,MinLon,MaxLon,ReqTime,&pCursor)
     != EWDB_RETURN_SUCCESS)
  {
    return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }

  if( ewdb_base_SQLExecute(pCursor) )
  {
    ewdb_base_ErrorReport(hEWDBC, pCursor,"ewdb_api_GetStationList(): ewdb_base_SQLExecute",1);
    return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  } 

/* Commit the transaction (all the previous inserts!)
  In Case there is any auditing or logging or debug
  changes made in the stored procedures.
  ****************************************************/
  if (ewdb_base_SQLCommit (hEWDBC))
  {
    ewdb_base_ErrorReport (hEWDBC, hEWDBC,"ewdb_api_GetStationList()::ewdb_base_SQLCommit",2);
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  }
  
  if((*pNumStationsFound = PostGetStationListExec(pBuffer,BufferLen)) 
     == EWDB_RETURN_FAILURE)
  {
    return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }

  ewdb_base_SetLastOraAPIActionTime();

  if(*pNumStationsFound <= BufferLen)
  {
    *pNumStationsRetrieved = *pNumStationsFound;
    return(EWDB_RETURN_SUCCESS);
  }
  else
  {
    *pNumStationsRetrieved = BufferLen;
    return(EWDB_RETURN_WARNING);
  }
}  /* end ewdb_api_GetStationList() */


static int InitGetStationListStatement(char * szStatement, 
                                EWDB_OCIStatementStruct *pSS)
{

  int LastSize,i;  /* Size of last column array */


  if(!pLocalBuffer)
  {
    /*Allocate space for returned data - never freed */
    pLocalBuffer=malloc(BUFFERSIZE);

    iRecordSize=0;
    iRecordSize += sizeof(EWDBid); /*idChan*/
    iRecordSize += sizeof(EWDBid); /*idComp*/
    iRecordSize += pSS->FieldArray[2].Ind; /*sSta*/
    iRecordSize += pSS->FieldArray[3].Ind; /*sComp*/
    iRecordSize += pSS->FieldArray[4].Ind; /*sNet*/
    iRecordSize += pSS->FieldArray[5].Ind; /*sLoc*/
    iRecordSize += pSS->FieldArray[6].Ind; /*dAzm*/
    iRecordSize += pSS->FieldArray[7].Ind; /*dDip*/
    iRecordSize += pSS->FieldArray[8].Ind; /*dLat*/
    iRecordSize += pSS->FieldArray[9].Ind; /*dLon*/
    iRecordSize += pSS->FieldArray[10].Ind;/*dElev*/

    /* Make the number of records a multiple of 8 to avoid alignment problems on UNIX */
    iRecordsPerBuffer= (BUFFERSIZE/iRecordSize) & 0xfffffff8;

    /*Allocate space for row/col ret lens. - never freed */
    for(i=0;i<pSS->NumOfFields;i++)
    {
      pSS->FieldArray[i].pRetLens=malloc(iRecordsPerBuffer*EWDB_FIELD_RET_LEN);
    }

    /* We have a SMALL problem.  For schema2 we are using 13-digit
       possible ID's.  Which fit our needs within the database, but
       don't quite fit into a 4-byte integer, when they come out.
       So, I think we are soon going to have to deal with the 64-bit
       integer problem.  Uh Oh!  Davidk 10/7/99 */

    pSS->FieldArray[0].pVal=&(pLocalBuffer[0]);
    LastSize=sizeof(int);
    pSS->FieldArray[1].pVal=(void *)(
     (int)(pSS->FieldArray[0].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=sizeof(int);
    pSS->FieldArray[2].pVal=(void *)(
     (int)(pSS->FieldArray[1].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[2].Ind; /* sta char[10] */
    pSS->FieldArray[3].pVal=(void *)(
     (int)(pSS->FieldArray[2].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[3].Ind; /* comp char[10] */
    pSS->FieldArray[4].pVal=(void *)(
     (int)(pSS->FieldArray[3].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[4].Ind; /* net char[10] */
    pSS->FieldArray[5].pVal=(void *)(
     (int)(pSS->FieldArray[4].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[5].Ind; /* loc char[10] */
    pSS->FieldArray[6].pVal=(void *)(
     (int)(pSS->FieldArray[5].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[6].Ind; /* dAzm float */
    pSS->FieldArray[7].pVal=(void *)(
     (int)(pSS->FieldArray[6].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[7].Ind; /* dDip float */
    pSS->FieldArray[8].pVal=(void *)(
     (int)(pSS->FieldArray[7].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[8].Ind; /* dLat double */
    pSS->FieldArray[9].pVal=(void *)(
     (int)(pSS->FieldArray[8].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[9].Ind; /* dLon double */
    pSS->FieldArray[10].pVal=(void *)(
     (int)(pSS->FieldArray[9].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[10].Ind; /* dElev double */
  
    pSS->FieldArray[11].pVal=Local_szMinLat;
    pSS->FieldArray[12].pVal=Local_szMaxLat;
    pSS->FieldArray[13].pVal=Local_szMinLon;
    pSS->FieldArray[14].pVal=Local_szMaxLon;
    pSS->FieldArray[15].pVal=Local_szReqTime;
  } /* end if(!pLocalBuffer) */
  
  if(!pLocalBuffer)
    logit("","InitGetStationListStatement: malloc of pLocalBuffer "
          "failed! Returning.\n");


  return(ewdb_base_RequestCursor(szStatement, pSS,0/*don't force rebind*/));
}  /* End InitGetStationListStatement() */


static int PrepGetStationListExec(double MinLat, double MaxLat, 
                                  double MinLon, double MaxLon,
                                  double ReqTime, EWDB_Cursor * ppCursor)
{

  if(!bInitialized)
  {
    bInitialized = TRUE;
    memset(&SSStatement,0,sizeof(SSStatement));
    SSStatement.NumOfFields=NUM_FIELDS;
    SSStatement.FieldArray=SQLParamsBindArray;
    SSStatement.RecordSize=0;
  }

  sprintf(Local_szMinLat,"%3.4f",MinLat);
  sprintf(Local_szMaxLat,"%3.4f",MaxLat);
  sprintf(Local_szMinLon,"%3.4f",MinLon);
  sprintf(Local_szMaxLon,"%3.4f",MaxLon);
  sprintf(Local_szReqTime,"%12.2f",ReqTime);

  if(InitGetStationListStatement(SQL_STRING, &SSStatement) != EWDB_RETURN_SUCCESS)
  {
    *ppCursor = NULL;
    return(EWDB_RETURN_FAILURE);
  }

  *ppCursor=SSStatement.pCda;

  return(EWDB_RETURN_SUCCESS);
}  /* End PrepGetStationListExec() */


static int PostGetStationListExec(EWDB_StationStruct * pBuffer, int BufferRecLen)
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
        ewdb_base_ErrorReport(hEWDBC, pCursor,"PostGetStationListExec:ewdb_base_SQLFetchRows",1);
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

        /*
        pTemp[pSS->FieldArray[2].pRetLens[BCurr]]=0;
        strcpy(pBuffer[UCurr].author,pTemp);

        pTemp=(char *)((pSS->FieldArray[3].Ind*BCurr) +(int)(pSS->FieldArray[3].pVal) );
        logit("","pRetLens for %d[3(ot)] is %d.\n",BCurr,pSS->FieldArray[3].pRetLens[BCurr]);
        pTemp[pSS->FieldArray[3].pRetLens[BCurr]]=0;
        pBuffer[UCurr].ot=atof(pTemp);
        */

        pBuffer[UCurr].idChan=*(int *)
           ((sizeof(int)*BCurr) +(int)(pSS->FieldArray[0].pVal));
        pBuffer[UCurr].idComp=*(int *)
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
        strcpy(pBuffer[UCurr].Sta,pTemp);

        pTemp=(char *) 
           ((pSS->FieldArray[3].Ind*BCurr) +(int)(pSS->FieldArray[3].pVal));
        pTemp[pSS->FieldArray[3].pRetLens[BCurr]]=0;
        strcpy(pBuffer[UCurr].Comp,pTemp);

        pTemp=(char *) 
         ((pSS->FieldArray[4].Ind*BCurr) +(int)(pSS->FieldArray[4].pVal));
        pTemp[pSS->FieldArray[4].pRetLens[BCurr]]=0;
        strcpy(pBuffer[UCurr].Net,pTemp);

        pTemp=(char *) 
           ((pSS->FieldArray[5].Ind*BCurr) +(int)(pSS->FieldArray[5].pVal));
        pTemp[pSS->FieldArray[5].pRetLens[BCurr]]=0;
        strcpy(pBuffer[UCurr].Loc,pTemp);

        pTemp=(char *) 
           ((pSS->FieldArray[6].Ind*BCurr) +(int)(pSS->FieldArray[6].pVal));
        pTemp[pSS->FieldArray[6].pRetLens[BCurr]]=0;
        pBuffer[UCurr].Azm=(float)atof(pTemp);

        pTemp=(char *) 
           ((pSS->FieldArray[7].Ind*BCurr) +(int)(pSS->FieldArray[7].pVal));
        pTemp[pSS->FieldArray[7].pRetLens[BCurr]]=0;
        pBuffer[UCurr].Dip=(float)atof(pTemp);

        pTemp=(char *) 
           ((pSS->FieldArray[8].Ind*BCurr) +(int)(pSS->FieldArray[8].pVal));
        pTemp[pSS->FieldArray[8].pRetLens[BCurr]]=0;
        pBuffer[UCurr].Lat=(float)atof(pTemp);

        pTemp=(char *) 
           ((pSS->FieldArray[9].Ind*BCurr) +(int)(pSS->FieldArray[9].pVal));
        pTemp[pSS->FieldArray[9].pRetLens[BCurr]]=0;
        pBuffer[UCurr].Lon=(float)atof(pTemp);

        pTemp=(char *) 
           ((pSS->FieldArray[10].Ind*BCurr) +(int)(pSS->FieldArray[10].pVal));
        pTemp[pSS->FieldArray[10].pRetLens[BCurr]]=0;
        pBuffer[UCurr].Elev=(float)atof(pTemp);
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
      ewdb_base_ErrorReport(hEWDBC, pCursor,"PostGetGetStationListExec:ewdb_base_SQLFetchRows",2);
      return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
    }
    /* else */
  }  /* End if(RowsRetrieved > BufferRecLen) */
  
  RowsProcessed = ewdb_base_GetCursorRowsProcessedCount(pCursor);

  ewdb_base_ReleaseCursor(pCursor);

  return(RowsProcessed);
}  /* End PostGetGetStationListExec() */


