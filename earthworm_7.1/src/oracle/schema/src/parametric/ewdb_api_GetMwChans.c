/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_api_GetMwChans.c,v 1.6 2005/06/17 21:00:03 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_api_GetMwChans.c,v $
 *     Revision 1.6  2005/06/17 21:00:03  davidk
 *     Fixed problems in SQL code caused by errant global replacements during API cleanup.
 *
 *     Revision 1.5  2005/06/15 19:03:12  davidk
 *     DB API Cleanup
 *
 *     Revision 1.4  2005/03/24 18:14:43  davidk
 *     Fixed bugs during initial testing of Mw API
 *
 *     Revision 1.3  2005/03/24 00:16:03  davidk
 *     Fixed bugs during initial testing.
 *
 *     Revision 1.2  2005/03/23 18:55:42  davidk
 *     Added a SQL_Commit call to have the transaction commit.  Otherwise it rolls
 *     back when the cursor is closed.  Every statement should have a commit,
 *     even queries that don't appear to touch anything.
 *
 *     Revision 1.1  2005/03/22 23:38:56  davidk
 *     Added API functions for Mw
 *
 */


#include <ewdb_cli_base.h>
#include <ewdb_ora_api.h>
#include <ewdb_ew_oci_base.h>
#include <ewdb_internal_functions.h>


static char SQL_STRING[] =
       "select idMwChan, IN_idMw, iOffset, idPick, idChan, dMisfit, "
       "       idSynthTS, Synth_dTSStart, Synth_dTSFreq, Synth_iTSLen, Synth_idMwFilter, "
       "       Synth_idChan, Synth_iType, Synth_sTS, Synth_sTS2, "
       "       idRealTS, Real_dTSStart, Real_dTSFreq, Real_iTSLen, Real_idMwFilter, "
       "       Real_idChan, Real_iType, Real_sTS, Real_sTS2 "
       " from ALL_MWCHAN_INFO_W_TS "
       " where idMw = :IN_idMw"
       " order by idChan";


static EWDB_OCI_SFS SQLParamsBindArray[] = 
{
  {0,1,0,0,0,OA_EWDBID, "1idMwChan"},
  {0,1,0,0,0,OA_EWDBID, "2idMw"},
  {0,1,0,0,0,OA_INT,    "3iOffset"},
  {0,1,0,0,0,OA_EWDBID, "4idPick"},
  {0,1,0,0,0,OA_EWDBID, "5idChan"},
  {0,1,20,0,0,OA_DOUBLE,"6dMisfit"},
  {0,1,0,0,0,OA_EWDBID, "7idSynthTS"},
  {0,1,20,0,0,OA_DOUBLE,"8Synth_dTSStart"},
  {0,1,20,0,0,OA_FLOAT, "9Synth_dTSFreq"},
  {0,1,0,0,0,OA_INT,    "10Synth_iTSLen"},
  {0,1,0,0,0,OA_EWDBID, "11Synth_idMwFilter"},
  {0,1,0,0,0,OA_EWDBID, "12Synth_idChan"},
  {0,1,0,0,0,OA_INT,    "13Synth_iType"},
  {0,1,2001,0,0,OA_SZ,  "14Synth_sTS"},
  {0,1,2001,0,0,OA_SZ,  "15Synth_sTS2"},
  {0,1,0,0,0,OA_EWDBID, "16idRealTS"},
  {0,1,20,0,0,OA_DOUBLE,"17Real_dTSStart"},
  {0,1,20,0,0,OA_FLOAT, "18Real_dTSFreq"},
  {0,1,0,0,0,OA_INT,    "19Real_iTSLen"},
  {0,1,0,0,0,OA_EWDBID, "20Real_idMwFilter"},
  {0,1,0,0,0,OA_EWDBID, "21Real_idChan"},
  {0,1,0,0,0,OA_INT,    "22Real_iType"},
  {0,1,4001,0,0,OA_SZ,  "23Real_sTS"},
  {0,1,4001,0,0,OA_SZ,  "24Real_sTS2"},
  {0,1,0,0,0,OA_EWDBID, ":IN_idMw"},
};

/* define the total number of fields to be bound */
#define NUM_FIELDS 25

/* Statement Struct for GetMwChans statement */
static EWDB_OCIStatementStruct SSStatement;

/* Temporary Storage Buffer params */
#define BUFFERSIZE 327680

static int    iRecordSize;
static int    iRecordsPerBuffer;
static char * pLocalBuffer=NULL;

static int    bInitialized=FALSE;


static  EWDBid  Local_idMw;

/****************************************************************/
/****************************************************************/
/********************************
      FUNCTION PROTOTYPES
********************************/
static int PrepGetMwChansExec(EWDBid IN_idMw, EWDB_Cursor * ppCursor);
static int PostGetMwChansExec(EWDB_MwChanStruct * pMwChanBuffer, int BufferRecLen);
static int InitGetMwChansStatement(char * szStatement, 
                                              EWDB_OCIStatementStruct *pSS);


int ewdb_api_GetMwChans(EWDBid IN_idMw, EWDB_MwChanStruct * pMwChanBuffer, 
                        int * pNumItemsFound, int * pNumItemsRetrieved, int iBufferLen)
/* retrieves all of the Mw Channel entires for a given Mw.  Each MwChan returned includes
   both sets of time series data associated with the MwChan computation. 
   EWDB_RETURN_WARNING indicates the pMwChanBuffer wasn't large enough.
   EWDB_RETURN_FAILURE indicates function encountered unspecified error, including IN_idMw was invalid.
*/
{
  EWDB_Cursor  pCursor;
 
  ewdb_base_SetLastOraAPIActionTime();

  if(!(IN_idMw && pMwChanBuffer && pNumItemsFound && pNumItemsRetrieved && iBufferLen))
  {
    logit("et", "ewdb_api_GetMwChans():  Error null parameter(s) "
                "passed in! (%u/%u/%u/%u/%u\n",
          IN_idMw, pMwChanBuffer, pNumItemsFound, 
          pNumItemsRetrieved, iBufferLen);
    return(EWDB_RETURN_FAILURE);
  }

  if( ewdb_base_Reconnect() != EWDB_RETURN_SUCCESS )
  {
    return( EWDB_RETURN_FAILURE );
  }

  if( PrepGetMwChansExec(IN_idMw, &pCursor)
     != EWDB_RETURN_SUCCESS)
  {
    logit("","ewdb_api_GetMwChans(): PrepGetMwChansExec() failed!\n");
    return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }

  if( ewdb_base_SQLExecute(pCursor) )
  {
    ewdb_base_ErrorReport(hEWDBC, pCursor,"ewdb_api_GetMwChans(): ewdb_base_SQLExecute",1);
    return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  } 

  if(ewdb_base_SQLCommit(hEWDBC))
  {
    ewdb_base_ErrorReport(hEWDBC, hEWDBC,
                      "ewdb_api_GetMwChans(): ewdb_base_SQLCommit",2);
    return(ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }
  
  if((*pNumItemsFound = PostGetMwChansExec(pMwChanBuffer, iBufferLen)) 
     == EWDB_RETURN_FAILURE)
  {
    logit("","ewdb_api_GetMwChans(): PostGetMwChansExec() failed!\n");
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
}  /* end ewdb_internal_GetMwChans() */


static int InitGetMwChansStatement(char * szStatement, 
                                              EWDB_OCIStatementStruct *pSS)
{
  int LastSize,i;  /* Size of last column array */

  if(!pLocalBuffer)
  {
    /*Allocate space for returned data - never freed */
    pLocalBuffer=malloc(BUFFERSIZE);

    iRecordSize=0;
    iRecordSize += sizeof(EWDBid);        /*1idMwChan*/
    iRecordSize += sizeof(EWDBid);        /*2idMw*/
    iRecordSize += sizeof(int);           /*3iOffset*/
    iRecordSize += sizeof(EWDBid);        /*4idPick*/
    iRecordSize += sizeof(EWDBid);        /*5idChan*/
    iRecordSize += pSS->FieldArray[5].Ind;/*6dMisfit*/
    iRecordSize += sizeof(EWDBid);        /*7idSynthTS*/
    iRecordSize += pSS->FieldArray[7].Ind;/*8Synth_dTSStart*/
    iRecordSize += pSS->FieldArray[8].Ind;/*9Synth_dTSFreq*/
    iRecordSize += sizeof(int);           /*10Synth_iTSLen*/
    iRecordSize += sizeof(EWDBid);        /*11Synth_idMwFilter*/
    iRecordSize += sizeof(EWDBid);        /*12Synth_idChan*/
    iRecordSize += sizeof(int);           /*13Synth_iType*/
    iRecordSize += pSS->FieldArray[13].Ind;/*14Synth_sTS*/
    iRecordSize += pSS->FieldArray[14].Ind;/*15Synth_sTS2*/
    iRecordSize += sizeof(EWDBid);        /*16idRealTS*/
    iRecordSize += pSS->FieldArray[16].Ind;/*17Real_dTSStart*/
    iRecordSize += pSS->FieldArray[17].Ind;/*18Real_dTSFreq*/
    iRecordSize += sizeof(int);           /*19Real_iTSLen*/
    iRecordSize += sizeof(EWDBid);        /*20Real_idMwFilter*/
    iRecordSize += sizeof(EWDBid);        /*21Real_idChan*/
    iRecordSize += sizeof(int);           /*22Real_iType*/
    iRecordSize += pSS->FieldArray[22].Ind;/*23Real_sTS*/
    iRecordSize += pSS->FieldArray[23].Ind;/*24Real_sTS2*/


    /* Make the number of records a multiple of 8 to avoid alignment problems on UNIX */
    iRecordsPerBuffer= (BUFFERSIZE/iRecordSize) & 0xfffffff8;
    if(iRecordsPerBuffer <= 0)
    {
      logit("et","ERROR! InitGetMwChansStatement(): not enough bs(%d): %d needed!\n",
            BUFFERSIZE, iRecordSize * 8);
      return(EWDB_RETURN_FAILURE);
    }

    
    /*Allocate space for row/col ret lens. - never freed */
    for(i=0;i<pSS->NumOfFields;i++)
    {
      pSS->FieldArray[i].pRetLens=malloc(iRecordsPerBuffer*EWDB_FIELD_RET_LEN);
    }

    pSS->FieldArray[0].pVal=&(pLocalBuffer[0]);
    LastSize=sizeof(EWDBid);         /* 1idMwChan */
    pSS->FieldArray[1].pVal=(void *)(
      (int)(pSS->FieldArray[0].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=sizeof(EWDBid);         /* 2idMw */
    pSS->FieldArray[2].pVal=(void *)(
      (int)(pSS->FieldArray[1].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=sizeof(int);            /* 3iOffset */
    pSS->FieldArray[3].pVal=(void *)(
      (int)(pSS->FieldArray[2].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=sizeof(EWDBid);         /* 4idPick */
    pSS->FieldArray[4].pVal=(void *)(
      (int)(pSS->FieldArray[3].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=sizeof(EWDBid);         /* 5idChan */
    pSS->FieldArray[5].pVal=(void *)(
      (int)(pSS->FieldArray[4].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[5].Ind; /* 6dMisfit */
    pSS->FieldArray[6].pVal=(void *)(
      (int)(pSS->FieldArray[5].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=sizeof(EWDBid);         /* 7idSynthTS */
    pSS->FieldArray[7].pVal=(void *)(
      (int)(pSS->FieldArray[6].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[7].Ind; /* 8Synth_dTSStart */
    pSS->FieldArray[8].pVal=(void *)(
      (int)(pSS->FieldArray[7].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[8].Ind; /* 9Synth_dTSFreq */
    pSS->FieldArray[9].pVal=(void *)(
      (int)(pSS->FieldArray[8].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=sizeof(int);            /* 10Synth_iTSLen */
    pSS->FieldArray[10].pVal=(void *)(
      (int)(pSS->FieldArray[9].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=sizeof(EWDBid);         /* 11Synth_idMwFilter */
    pSS->FieldArray[11].pVal=(void *)(
      (int)(pSS->FieldArray[10].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=sizeof(EWDBid);         /* 12Synth_idChan */
    pSS->FieldArray[12].pVal=(void *)(
      (int)(pSS->FieldArray[11].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=sizeof(int);            /* 13Synth_iType */
    pSS->FieldArray[13].pVal=(void *)(
      (int)(pSS->FieldArray[12].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[13].Ind; /* 14Synth_sTS */
    pSS->FieldArray[14].pVal=(void *)(
      (int)(pSS->FieldArray[13].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[14].Ind; /* 14Synth_sTS2 */
    pSS->FieldArray[15].pVal=(void *)(
      (int)(pSS->FieldArray[14].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=sizeof(EWDBid);         /* 15idRealTS */
    pSS->FieldArray[16].pVal=(void *)(
      (int)(pSS->FieldArray[15].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[16].Ind; /* 16Real_dTSStart */
    pSS->FieldArray[17].pVal=(void *)(
      (int)(pSS->FieldArray[16].pVal)+(LastSize*iRecordsPerBuffer));
     LastSize=pSS->FieldArray[17].Ind; /* 17Real_dTSFreq */
   pSS->FieldArray[18].pVal=(void *)(
      (int)(pSS->FieldArray[17].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=sizeof(int);            /* 18Real_iTSLen */
    pSS->FieldArray[19].pVal=(void *)(
      (int)(pSS->FieldArray[18].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=sizeof(EWDBid);         /* 19Real_idMwFilter */
    pSS->FieldArray[20].pVal=(void *)(
      (int)(pSS->FieldArray[19].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=sizeof(EWDBid);         /* 20Real_idChan */
    pSS->FieldArray[21].pVal=(void *)(
      (int)(pSS->FieldArray[20].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=sizeof(int);            /* 21Real_iType */
    pSS->FieldArray[22].pVal=(void *)(
      (int)(pSS->FieldArray[21].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[22].Ind; /* 22Real_sTS */
    pSS->FieldArray[23].pVal=(void *)(
      (int)(pSS->FieldArray[22].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[23].Ind; /* 22Real_sTS2 */

    pSS->FieldArray[24].pVal=&Local_idMw;

  } /* end if(!pLocalBuffer) */
  
  if(!pLocalBuffer)
  {
    logit("","InitGetMwChansStatement: malloc of pLocalBuffer "
          "failed! Returning.\n");
    return(EWDB_RETURN_FAILURE);
  }

  return(ewdb_base_RequestCursor(szStatement, pSS,0/*don't force rebind*/));
}  /* End () */


static int PrepGetMwChansExec(EWDBid IN_idMw, EWDB_Cursor * ppCursor)
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

  Local_idMw  = IN_idMw;

  if(InitGetMwChansStatement(SQL_STRING, &SSStatement) != EWDB_RETURN_SUCCESS)
  {
    *ppCursor = NULL;
    return(EWDB_RETURN_FAILURE);
  }

  *ppCursor=SSStatement.pCda;

  return(EWDB_RETURN_SUCCESS);
}  /* End PrepGetMwChansExec() */


static int PostGetMwChansExec(EWDB_MwChanStruct * pMwChanBuffer, int BufferRecLen)
{
  int done=0;
  int RowsRetrieved=0;
  int RowsDone=0;
  EWDB_Cursor  pCursor=SSStatement.pCda;
  char * pTemp;
  char * pTemp2;
  int BCurr,UCurr;
  EWDB_OCIStatementStruct * pSS=&SSStatement;
  int RowsProcessed;
  int iTempLen;
  int rc;

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
        ewdb_base_ErrorReport(hEWDBC, pCursor,"PostGetMwChansExec:ewdb_base_SQLFetchRows",1);
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
        memset(&pMwChanBuffer[UCurr],0,sizeof(EWDB_ArrivalStruct));

        pMwChanBuffer[UCurr].idMwChan=*(EWDBid *)((sizeof(EWDBid)*BCurr) +(int)(pSS->FieldArray[0].pVal));
        pMwChanBuffer[UCurr].idMw=*(EWDBid *)((sizeof(EWDBid)*BCurr) +(int)(pSS->FieldArray[1].pVal));
        pMwChanBuffer[UCurr].iOffset=*(int *)((sizeof(int)*BCurr) +(int)(pSS->FieldArray[2].pVal));
        pMwChanBuffer[UCurr].idPick=*(EWDBid *)((sizeof(EWDBid)*BCurr) +(int)(pSS->FieldArray[3].pVal));
        pMwChanBuffer[UCurr].idChan=*(EWDBid *)((sizeof(EWDBid)*BCurr) +(int)(pSS->FieldArray[4].pVal));

        pTemp=(char *)((pSS->FieldArray[5].Ind*BCurr) +(int)(pSS->FieldArray[5].pVal) );
        pTemp[pSS->FieldArray[5].pRetLens[BCurr]]=0;
        pMwChanBuffer[UCurr].dMisfit = atof(pTemp);

        pMwChanBuffer[UCurr].SynthTS.idMwCTS=*(EWDBid *)((sizeof(EWDBid)*BCurr) +(int)(pSS->FieldArray[6].pVal));

        pTemp=(char *)((pSS->FieldArray[7].Ind*BCurr) +(int)(pSS->FieldArray[7].pVal) );
        pTemp[pSS->FieldArray[7].pRetLens[BCurr]]=0;
        pMwChanBuffer[UCurr].SynthTS.dStart = atof(pTemp);

        pTemp=(char *)((pSS->FieldArray[8].Ind*BCurr) +(int)(pSS->FieldArray[8].pVal) );
        pTemp[pSS->FieldArray[8].pRetLens[BCurr]]=0;
        pMwChanBuffer[UCurr].SynthTS.dFreq = (float)atof(pTemp);

        pMwChanBuffer[UCurr].SynthTS.iLen=*(int *)((sizeof(int)*BCurr) +(int)(pSS->FieldArray[9].pVal));
        pMwChanBuffer[UCurr].SynthTS.idMwFilter=*(EWDBid *)((sizeof(EWDBid)*BCurr) +(int)(pSS->FieldArray[10].pVal));
        pMwChanBuffer[UCurr].SynthTS.idChan=*(EWDBid *)((sizeof(EWDBid)*BCurr) +(int)(pSS->FieldArray[11].pVal));
        pMwChanBuffer[UCurr].SynthTS.iTSType=*(int *)((sizeof(int)*BCurr) +(int)(pSS->FieldArray[12].pVal));


        pTemp=(char *)((pSS->FieldArray[13].Ind*BCurr) +(int)(pSS->FieldArray[13].pVal) );
        pTemp[pSS->FieldArray[13].pRetLens[BCurr]]=0;
        pTemp2=(char *)((pSS->FieldArray[14].Ind*BCurr) +(int)(pSS->FieldArray[14].pVal) );
        pTemp2[pSS->FieldArray[14].pRetLens[BCurr]]=0;
        rc = ewdb_internal_TS_SQLtoC(pTemp,pTemp2, pMwChanBuffer[UCurr].SynthTS.TS, &iTempLen);
        if(rc == EWDB_RETURN_FAILURE)
        {
          logit("et","ERROR: PostGetMwChansExec(%d) Failure in ewdb_internal_TS_SQLtoC()!  Aborting\n",
                pMwChanBuffer[UCurr].SynthTS.idMwCTS);
          ewdb_base_ReleaseCursor(pCursor);
          return(rc);
        }
        if(iTempLen != pMwChanBuffer[UCurr].SynthTS.iLen)
        {
          logit("et","ERROR: PostGetMwChansExec(%d) Synth TimeSeries lengths do not match. Given(%d) Parsed(%d)!  Aborting\n",
                pMwChanBuffer[UCurr].SynthTS.idMwCTS);
          ewdb_base_ReleaseCursor(pCursor);
          return(EWDB_RETURN_FAILURE);
        }

        pMwChanBuffer[UCurr].RealTS.idMwCTS =*(EWDBid *)((sizeof(EWDBid)*BCurr) +(int)(pSS->FieldArray[15].pVal));

        pTemp=(char *)((pSS->FieldArray[16].Ind*BCurr) +(int)(pSS->FieldArray[16].pVal) );
        pTemp[pSS->FieldArray[16].pRetLens[BCurr]]=0;
        pMwChanBuffer[UCurr].RealTS.dStart = atof(pTemp);

        pTemp=(char *)((pSS->FieldArray[17].Ind*BCurr) +(int)(pSS->FieldArray[17].pVal) );
        pTemp[pSS->FieldArray[17].pRetLens[BCurr]]=0;
        pMwChanBuffer[UCurr].RealTS.dFreq = (float)atof(pTemp);

        pMwChanBuffer[UCurr].RealTS.iLen=*(int *)((sizeof(int)*BCurr) +(int)(pSS->FieldArray[18].pVal));
        pMwChanBuffer[UCurr].RealTS.idMwFilter=*(EWDBid *)((sizeof(EWDBid)*BCurr) +(int)(pSS->FieldArray[19].pVal));
        pMwChanBuffer[UCurr].RealTS.idChan=*(EWDBid *)((sizeof(EWDBid)*BCurr) +(int)(pSS->FieldArray[20].pVal));
        pMwChanBuffer[UCurr].RealTS.iTSType=*(int *)((sizeof(int)*BCurr) +(int)(pSS->FieldArray[21].pVal));

        pTemp=(char *)((pSS->FieldArray[22].Ind*BCurr) +(int)(pSS->FieldArray[22].pVal) );
        pTemp[pSS->FieldArray[22].pRetLens[BCurr]]=0;
        pTemp2=(char *)((pSS->FieldArray[23].Ind*BCurr) +(int)(pSS->FieldArray[23].pVal) );
        pTemp2[pSS->FieldArray[23].pRetLens[BCurr]]=0;
        rc = ewdb_internal_TS_SQLtoC(pTemp,pTemp2,pMwChanBuffer[UCurr].RealTS.TS, &iTempLen);
        if(rc == EWDB_RETURN_FAILURE)
        {
          logit("et","ERROR: PostGetMwChansExec(%d) Failure in ewdb_internal_TS_SQLtoC()!  Aborting\n",
                pMwChanBuffer[UCurr].RealTS.idMwCTS);
          ewdb_base_ReleaseCursor(pCursor);
          return(rc);
        }
        if(iTempLen != pMwChanBuffer[UCurr].RealTS.iLen)
        {
          logit("et","ERROR: PostGetMwChansExec(%d) Real TimeSeries lengths do not match. Given(%d) Parsed(%d)!  Aborting\n",
                pMwChanBuffer[UCurr].RealTS.idMwCTS);
          ewdb_base_ReleaseCursor(pCursor);
          return(EWDB_RETURN_FAILURE);
        }

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
}  /* End PostGetGetMwChansExec() */


