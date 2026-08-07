/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_api_GetMwTimeSeriesByChan.c,v 1.6 2005/06/15 19:03:12 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_api_GetMwTimeSeriesByChan.c,v $
 *     Revision 1.6  2005/06/15 19:03:12  davidk
 *     DB API Cleanup
 *
 *     Revision 1.5  2005/03/24 18:14:43  davidk
 *     Fixed bugs during initial testing of Mw API
 *
 *     Revision 1.4  2005/03/24 00:16:03  davidk
 *     Fixed bugs during initial testing.
 *
 *     Revision 1.3  2005/03/23 18:55:42  davidk
 *     Added a SQL_Commit call to have the transaction commit.  Otherwise it rolls
 *     back when the cursor is closed.  Every statement should have a commit,
 *     even queries that don't appear to touch anything.
 *
 *     Revision 1.2  2005/03/23 06:22:30  davidk
 *     Fixed bugs in SQL statements - prior to run-testing code.
 *
 *     Revision 1.1  2005/03/22 23:38:56  davidk
 *     Added API functions for Mw
 *
 *
 */


#include <ewdb_cli_base.h>
#include <ewdb_ora_api.h>
#include <ewdb_ew_oci_base.h>
#include <ewdb_internal_functions.h>


static char SQL_STRING[] =
       "select MWCTS.idMwCTS, MWCTS.dTSStart, MWCTS.dTSFreq, MWCTS.iTSLen, MWCTS.idMwFilter, MWCTS.idChan, MWCTS.iType, MWCTS.sTS1, MWCTS.sTS2 "
       " from ALL_MWCTS_INFO MWCTS"
       " where MWCTS.iType = :IN_iType "
       "   and MWCTS.idMwFilter = :IN_idMwFilter "
       "   and MWCTS.dTSStart <= :IN_tReqStart "
       "   and (MWCTS.dTSStart + MWCTS.iTSLen / MWCTS.dTSFreq) >= :IN_tReqEnd "
       " order by dTSStart";

static EWDB_OCI_SFS SQLParamsBindArray[] = 
{
  {0,1,0,0,0,OA_EWDBID, "1idMwCTS"},
  {0,1,20,0,0,OA_DOUBLE,"2dTSStart"},
  {0,1,20,0,0,OA_DOUBLE,"3dTSFreq"},
  {0,1,0,0,0,OA_INT,    "4iTSLen"},
  {0,1,0,0,0,OA_EWDBID, "5idMwFilter"},
  {0,1,0,0,0,OA_EWDBID, "6idChan"},
  {0,1,0,0,0,OA_EWDBID, "7iType"},
  {0,1,2001,0,0,OA_SZ,  "8sTS1"},
  {0,1,2001,0,0,OA_SZ,  "9sTS2"},
  {0,1,0,0,0,OA_INT,    ":IN_iType"},
  {0,1,0,0,0,OA_EWDBID, ":IN_idMwFilter"},
  {0,1,0,0,0,OA_DOUBLE, ":IN_tReqStart"},
  {0,1,0,0,0,OA_DOUBLE, ":IN_tReqEnd"}
};

/* define the total number of fields to be bound */
#define NUM_FIELDS 13

/* Statement Struct for GetMwTimeSeriesByChan statement */
static EWDB_OCIStatementStruct SSStatement;

/* Temporary Storage Buffer params */
#define BUFFERSIZE 32768

static int    iRecordSize;
static int    iRecordsPerBuffer;
static char * pLocalBuffer=NULL;

static int    bInitialized=FALSE;


static  EWDBid  Local_idMwFilter;
static  char    Local_sztReqStart[20], Local_sztReqEnd[20];
static  int     Local_iType;

/****************************************************************/
/****************************************************************/
/********************************
      FUNCTION PROTOTYPES
********************************/
static int PrepGetMwTimeSeriesByChanExec(double IN_tReqStart, double IN_tReqEnd, EWDB_MwTimeSeriesStruct * pMwCTS,
                                         EWDB_Cursor * ppCursor);
static int PostGetMwTimeSeriesByChanExec(EWDB_MwTimeSeriesStruct * pMwCTS);
static int InitGetMwTimeSeriesByChanStatement(char * szStatement, 
                                              EWDB_OCIStatementStruct *pSS);


int ewdb_api_GetMwTimeSeriesByChan(double IN_tReqStart, double IN_tReqEnd, EWDB_MwTimeSeriesStruct * pMwCTS)
/* fill in pMwCTS->idChan, pMwCTS->idMwFilter, IN_tReqStart, and IN_tReqEnd, and get 
   back the earliest time series that matches the given channel, filter, and time-criteria window.
   If no records exist that start prior to IN_tReqStart and end after IN_tReqEnd then
   a warning will be returned.  Be sure to fill in pMwCTS->cTSType in order to get the
   appropriate type(synthetic/real) data.
   EWDB_RETURN_WARNING indicates function was unable to find a time series record that matched
   the criteria.
   EWDB_RETURN_FAILURE indicates function encountered unspecified error
  */
{
  EWDB_Cursor  pCursor;
  int rc;
 
  ewdb_base_SetLastOraAPIActionTime();

  if(!(IN_tReqStart && IN_tReqEnd && pMwCTS))
  {
    logit("et", "ewdb_api_GetMwTimeSeriesByChan():  Error null parameter(s) "
                "passed in! (%u/%.2f/%.2f\n",
          pMwCTS, IN_tReqStart, IN_tReqEnd);
    return(EWDB_RETURN_FAILURE);
  }

  if(!(pMwCTS->idMwFilter && pMwCTS->idChan))
  {
    logit("et", "ewdb_api_GetMwTimeSeriesByChan():  Error null filter/chan "
                "parameters passed in! (%u/%u)\n",
          pMwCTS->idMwFilter, pMwCTS->idChan);
    return(EWDB_RETURN_FAILURE);
  }

  if( ewdb_base_Reconnect() != EWDB_RETURN_SUCCESS )
  {
    return( EWDB_RETURN_FAILURE );
  }

  if( PrepGetMwTimeSeriesByChanExec(IN_tReqStart, IN_tReqEnd, pMwCTS, &pCursor)
     != EWDB_RETURN_SUCCESS)
  {
    return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }

  if( ewdb_base_SQLExecute(pCursor) )
  {
    ewdb_base_ErrorReport(hEWDBC, pCursor,"GetMwTimeSeriesByChan:ewdb_base_SQLExecute",1);
    return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  } 

  if(ewdb_base_SQLCommit(hEWDBC))
  {
    ewdb_base_ErrorReport(hEWDBC, hEWDBC,
                      "ewdb_internal_GetMwTimeSeriesByChan(): ewdb_base_SQLCommit",2);
    return(ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }
  
  if((rc = PostGetMwTimeSeriesByChanExec(pMwCTS)) 
     == EWDB_RETURN_FAILURE)
  {
    return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }

  ewdb_base_SetLastOraAPIActionTime();

  return(rc);

}  /* end ewdb_internal_GetMwTimeSeriesByChan() */


static int InitGetMwTimeSeriesByChanStatement(char * szStatement, 
                                              EWDB_OCIStatementStruct *pSS)
{
  int LastSize,i;  /* Size of last column array */

  if(!pLocalBuffer)
  {
    /*Allocate space for returned data - never freed */
    pLocalBuffer=malloc(BUFFERSIZE);

    iRecordSize=0;
    iRecordSize += sizeof(EWDBid);        /*0idMwCTS*/
    iRecordSize += pSS->FieldArray[1].Ind;/*1dTSStart*/
    iRecordSize += pSS->FieldArray[2].Ind;/*2dTSFreq*/
    iRecordSize += sizeof(int);           /*3iTSLen*/
    iRecordSize += sizeof(EWDBid);        /*4idMwFilter*/
    iRecordSize += sizeof(EWDBid);        /*5idChan*/
    iRecordSize += sizeof(int);           /*6iType*/
    iRecordSize += pSS->FieldArray[7].Ind;/*7sTS1*/
    iRecordSize += pSS->FieldArray[8].Ind;/*8sTS2*/
    
    /* Make the number of records a multiple of 8 to avoid alignment problems on UNIX */
    iRecordsPerBuffer= (BUFFERSIZE/iRecordSize) & 0xfffffff8;
    
    if(iRecordsPerBuffer <= 0)
    {
      logit("et","ERROR! InitGetMwTimeSeriesByChanStatement(): not enough bs(%d): %d needed!\n",
            BUFFERSIZE, iRecordSize * 8);
      return(EWDB_RETURN_FAILURE);
    }

    /*Allocate space for row/col ret lens. - never freed */
    for(i=0;i<pSS->NumOfFields;i++)
    {
      pSS->FieldArray[i].pRetLens=malloc(iRecordsPerBuffer*EWDB_FIELD_RET_LEN);
    }

    pSS->FieldArray[0].pVal=&(pLocalBuffer[0]);
    LastSize=sizeof(EWDBid);         /* 0idMwCTS */
    pSS->FieldArray[1].pVal=(void *)(
      (int)(pSS->FieldArray[0].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[1].Ind; /* 1dTSStart */
    pSS->FieldArray[2].pVal=(void *)(
      (int)(pSS->FieldArray[1].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[2].Ind; /* 2dTSFreq */
    pSS->FieldArray[3].pVal=(void *)(
      (int)(pSS->FieldArray[2].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=sizeof(EWDBid);         /* 3iTSLen */
    pSS->FieldArray[4].pVal=(void *)(
      (int)(pSS->FieldArray[3].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=sizeof(EWDBid);         /* 4idMwFilter */
    pSS->FieldArray[5].pVal=(void *)(
      (int)(pSS->FieldArray[4].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=sizeof(EWDBid);         /* 5idChan */
    pSS->FieldArray[6].pVal=(void *)(
      (int)(pSS->FieldArray[5].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=sizeof(EWDBid);         /* 6iType */
    pSS->FieldArray[7].pVal=(void *)(
      (int)(pSS->FieldArray[6].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[7].Ind; /* 7sTS1 */
    pSS->FieldArray[8].pVal=(void *)(
      (int)(pSS->FieldArray[7].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[8].Ind; /* 8sTS2 */

    pSS->FieldArray[9].pVal=&Local_iType;
    pSS->FieldArray[10].pVal=&Local_idMwFilter;
    pSS->FieldArray[11].pVal=Local_sztReqStart;
    pSS->FieldArray[12].pVal=Local_sztReqEnd;

  } /* end if(!pLocalBuffer) */
  
  if(!pLocalBuffer)
    logit("","InitGetMwTimeSeriesByChanStatement: malloc of pLocalBuffer "
          "failed! Returning.\n");

  return(ewdb_base_RequestCursor(szStatement, pSS,0/*don't force rebind*/));
}  /* End InitGetMwTimeSeriesByChanStatement() */


static int PrepGetMwTimeSeriesByChanExec(double IN_tReqStart, double IN_tReqEnd, EWDB_MwTimeSeriesStruct * pMwCTS,
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

  Local_iType  = pMwCTS->iTSType;
  Local_idMwFilter = pMwCTS->idMwFilter;

  sprintf(Local_sztReqStart, "%.4f", IN_tReqStart);
  sprintf(Local_sztReqEnd, "%.4f", IN_tReqEnd);

  if(InitGetMwTimeSeriesByChanStatement(SQL_STRING, &SSStatement) != EWDB_RETURN_SUCCESS)
  {
    *ppCursor = NULL;
    return(EWDB_RETURN_FAILURE);
  }

  *ppCursor=SSStatement.pCda;

  return(EWDB_RETURN_SUCCESS);
}  /* End PrepGetMwTimeSeriesByChanExec() */


static int PostGetMwTimeSeriesByChanExec(EWDB_MwTimeSeriesStruct * pMwCTS)
{

  int done=0;
  int RowsRetrieved=0;
  int RowsDone=0;
  EWDB_Cursor  pCursor=SSStatement.pCda;
  char * pTemp;
  char * pTemp2;
  int BCurr,UCurr;
  EWDB_OCIStatementStruct * pSS=&SSStatement;
  int BufferRecLen = 1;
  int rc;
  int iTempLen;

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
        ewdb_base_ErrorReport(hEWDBC, pCursor,"PostGetMwTimeSeriesByChanExec:ewdb_base_SQLFetchRows",1);
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
        memset(&pMwCTS[UCurr],0,sizeof(EWDB_ArrivalStruct));


        pMwCTS[UCurr].idMwCTS=*(EWDBid *)((sizeof(int)*BCurr) +(int)(pSS->FieldArray[0].pVal));

        pTemp=(char *)((pSS->FieldArray[1].Ind*BCurr) +(int)(pSS->FieldArray[1].pVal) );
        pTemp[pSS->FieldArray[1].pRetLens[BCurr]]=0;
        pMwCTS[UCurr].dStart = atof(pTemp);

        pTemp=(char *)((pSS->FieldArray[2].Ind*BCurr) +(int)(pSS->FieldArray[2].pVal) );
        pTemp[pSS->FieldArray[2].pRetLens[BCurr]]=0;
        pMwCTS[UCurr].dFreq = (float) atof(pTemp);

        pMwCTS[UCurr].iLen=*(int *)((sizeof(int)*BCurr) +(int)(pSS->FieldArray[3].pVal));

        pMwCTS[UCurr].idMwFilter=*(EWDBid *)((sizeof(int)*BCurr) +(int)(pSS->FieldArray[4].pVal));

        pMwCTS[UCurr].idChan=*(EWDBid *)((sizeof(int)*BCurr) +(int)(pSS->FieldArray[5].pVal));

        pMwCTS[UCurr].iTSType=(MwTimeSeriesDataType)*(int *)((sizeof(int)*BCurr) +(int)(pSS->FieldArray[6].pVal));

        pTemp=(char *)((pSS->FieldArray[7].Ind*BCurr) +(int)(pSS->FieldArray[7].pVal) );
        pTemp[pSS->FieldArray[7].pRetLens[BCurr]]=0;
        pTemp2=(char *)((pSS->FieldArray[8].Ind*BCurr) +(int)(pSS->FieldArray[8].pVal) );
        pTemp2[pSS->FieldArray[8].pRetLens[BCurr]]=0;
        rc = ewdb_internal_TS_SQLtoC(pTemp, pTemp2, pMwCTS->TS, &iTempLen);
        if(rc == EWDB_RETURN_FAILURE)
        {
          logit("et","ERROR: PostGetMwTimeSeriesByChanExec(%d) Failure in ewdb_internal_TS_SQLtoC()!  Aborting\n",
                pMwCTS[UCurr].idMwCTS);
          return(rc);
        }
        if(iTempLen != pMwCTS[UCurr].iLen)
        {
          logit("et","ERROR: PostGetMwTimeSeriesByChanExec(%d) TimeSeries lengths do not match. Given(%d) Parsed(%d)!  Aborting\n",
                pMwCTS[UCurr].idMwCTS);
          ewdb_base_ReleaseCursor(pCursor);
          return(EWDB_RETURN_FAILURE);
        }

      }
    } /* End for RowsDone < RowsRetrieved */
  }  /* End while !done */

  /* don't care about extras */
  ewdb_base_ReleaseCursor(pCursor);

  if(RowsDone)
    return(EWDB_RETURN_SUCCESS);
  else
    return(EWDB_RETURN_WARNING);
}  /* End PostGetGetMwTimeSeriesByChanExec() */


