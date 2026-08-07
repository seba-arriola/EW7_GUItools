/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_api_GetMwSolutionsForOrigin.c,v 1.6 2005/06/21 20:06:42 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_api_GetMwSolutionsForOrigin.c,v $
 *     Revision 1.6  2005/06/21 20:06:42  davidk
 *     Fixed a bug in the code.  should have no effect.
 *
 *     Revision 1.5  2005/06/15 19:03:12  davidk
 *     DB API Cleanup
 *
 *     Revision 1.4  2005/03/31 18:51:47  davidk
 *     Changed the EWDB_MwStruct:
 *       Added dM0 (doh!)
 *       Changed dPercentCVLD to dPercentCLVD
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
 *
 */


#include <ewdb_cli_base.h>
#include <ewdb_ora_api.h>
#include <ewdb_ew_oci_base.h>

 static char SQL_STRING[] =
       "select idMw, dMxx, dMxy, dMxz, dMyy, dMyz, dMzz, iScalarExp, "
       "  dPFPStrike, dPFPDip, dPFPRake, dAFPStrike, dAFPDip, dAFPRake, "
       "  dDepth, idOrigin, idMag, iNumStations, dMisfit, dPercentDC, dPercentCLVD, "
       "  idMwFilter, dLowCutHz, dLowTaperHz, dHighTaperHZ, dHighCutHz, dM0 "
       " from ALL_MW_INFO_W_FILTER"
       " where idOrigin = :IN_idOrigin "
       " order by dDepth";

static EWDB_OCI_SFS SQLParamsBindArray[] = 
{
  {0,1,0,0,0,OA_EWDBID, "1idMw"},
  {0,1,20,0,0,OA_DOUBLE,"2dMxx"},
  {0,1,20,0,0,OA_DOUBLE,"3dMxy"},
  {0,1,20,0,0,OA_DOUBLE,"4dMxz"},
  {0,1,20,0,0,OA_DOUBLE,"5dMyy"},
  {0,1,20,0,0,OA_DOUBLE,"6dMyz"},
  {0,1,20,0,0,OA_DOUBLE,"7dMzz"},
  {0,1,0,0,0,OA_INT,    "8iScalarExp"},
  {0,1,20,0,0,OA_DOUBLE,"9dPFPStrike"},
  {0,1,20,0,0,OA_DOUBLE,"10dPFPDip"},
  {0,1,20,0,0,OA_DOUBLE,"11dPFPRake"},
  {0,1,20,0,0,OA_DOUBLE,"12dAFPStrike"},
  {0,1,20,0,0,OA_DOUBLE,"13dAFPDip"},
  {0,1,20,0,0,OA_DOUBLE,"14dAFPRake"},
  {0,1,20,0,0,OA_DOUBLE,"15dDepth"},
  {0,1,0,0,0,OA_EWDBID, "16idOrigin"},
  {0,1,0,0,0,OA_EWDBID, "17idMag"},
  {0,1,0,0,0,OA_INT,    "18iNumStations"},
  {0,1,20,0,0,OA_DOUBLE,"19dMisfit"},
  {0,1,20,0,0,OA_DOUBLE,"20dPercentDC"},
  {0,1,20,0,0,OA_DOUBLE,"21dPercentCLVD"},
  {0,1,0,0,0,OA_EWDBID, "22idMwFilter"},
  {0,1,20,0,0,OA_FLOAT, "23dLowCutHz"},
  {0,1,20,0,0,OA_FLOAT, "24dLowTaperHz"},
  {0,1,20,0,0,OA_FLOAT, "25dHighTaperHZ"},
  {0,1,20,0,0,OA_FLOAT, "26dHighCutHz"},
  {0,1,20,0,0,OA_DOUBLE,"27dM0"},
  {0,1,0,0,0,OA_EWDBID, ":IN_idOrigin"}
};

/* define the total number of fields to be bound */
#define NUM_FIELDS 28

/* Statement Struct for GetMwSolutionsForOrigin statement */
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
static int PrepGetMwSolutionsForOriginExec(EWDBid idOrigin, EWDB_Cursor * ppCursor);
static int PostGetMwSolutionsForOriginExec(EWDB_MwStruct * pMwBuffer, int BufferRecLen);
static int InitGetMwSolutionsForOriginStatement(char * szStatement, 
                                              EWDB_OCIStatementStruct *pSS);


int ewdb_api_GetMwSolutionsForOrigin(EWDBid IN_idOrigin, EWDB_MwStruct * pMwBuffer, 
                                     int * pNumItemsFound, int * pNumItemsRetrieved, int iBufferLen)
/* retrieves all of the Mw records for a given Origin.  One of those Mw records
   should have a valid idMag value, which indicates it is the "final solution". 
   EWDB_RETURN_WARNING indicates the pMwBuffer wasn't large enough.
   EWDB_RETURN_FAILURE indicates function encountered unspecified error, including idOrigin was invalid.
 */
{
  EWDB_Cursor  pCursor;
 
  ewdb_base_SetLastOraAPIActionTime();

  if(!(IN_idOrigin && pMwBuffer && pNumItemsFound && pNumItemsRetrieved && iBufferLen))
  {
    logit("et", "ewdb_api_GetMwSolutionsForOrigin():  Error null parameter(s) "
                "passed in! (%u/%u/%u/%u/%u\n",
          IN_idOrigin, pMwBuffer, pNumItemsFound, 
          pNumItemsRetrieved, iBufferLen);
    return(EWDB_RETURN_FAILURE);
  }

  if( ewdb_base_Reconnect() != EWDB_RETURN_SUCCESS )
  {
    return( EWDB_RETURN_FAILURE );
  }

  if( PrepGetMwSolutionsForOriginExec(IN_idOrigin, &pCursor)
     != EWDB_RETURN_SUCCESS)
  {
    return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }

  if( ewdb_base_SQLExecute(pCursor) )
  {
    ewdb_base_ErrorReport(hEWDBC, pCursor,"ewdb_api_GetMwSolutionsForOrigin(): ewdb_base_SQLExecute",1);
    return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  } 

  if(ewdb_base_SQLCommit(hEWDBC))
  {
    ewdb_base_ErrorReport(hEWDBC, hEWDBC,
                      "ewdb_api_GetMwSolutionsForOrigin(): ewdb_base_SQLCommit",2);
    return(ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }
  
  if((*pNumItemsFound = PostGetMwSolutionsForOriginExec(pMwBuffer, iBufferLen)) 
     == EWDB_RETURN_FAILURE)
  {
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
}  /* end ewdb_internal_GetMwSolutionsForOrigin() */


static int InitGetMwSolutionsForOriginStatement(char * szStatement, 
                                                EWDB_OCIStatementStruct *pSS)
{
  int LastSize,i;  /* Size of last column array */

  if(!pLocalBuffer)
  {
    /*Allocate space for returned data - never freed */
    pLocalBuffer=malloc(BUFFERSIZE);

    iRecordSize=0;
    iRecordSize += sizeof(EWDBid);        /*1idMw*/
    iRecordSize += pSS->FieldArray[1].Ind;/*2dMxx*/
    iRecordSize += pSS->FieldArray[2].Ind;/*3dMxy*/
    iRecordSize += pSS->FieldArray[3].Ind;/*4dMxz*/
    iRecordSize += pSS->FieldArray[4].Ind;/*5dMyy*/
    iRecordSize += pSS->FieldArray[5].Ind;/*6dMyz*/
    iRecordSize += pSS->FieldArray[6].Ind;/*7dMzz*/
    iRecordSize += sizeof(int);           /*8iScalarExp*/
    iRecordSize += pSS->FieldArray[8].Ind;/*9dPFPStrike*/
    iRecordSize += pSS->FieldArray[9].Ind;/*10dPFPDip*/
    iRecordSize += pSS->FieldArray[10].Ind;/*11dPFPRake*/
    iRecordSize += pSS->FieldArray[11].Ind;/*12dAFPStrike*/
    iRecordSize += pSS->FieldArray[12].Ind;/*13dAFPDip*/
    iRecordSize += pSS->FieldArray[13].Ind;/*14dAFPRake*/
    iRecordSize += pSS->FieldArray[14].Ind;/*15dDepth*/
    iRecordSize += sizeof(EWDBid);        /*16idOrigin*/
    iRecordSize += sizeof(EWDBid);        /*17idMag*/
    iRecordSize += sizeof(int);           /*18iNumStations*/
    iRecordSize += pSS->FieldArray[18].Ind;/*19dMisfit*/
    iRecordSize += pSS->FieldArray[19].Ind;/*20dPercentDC*/
    iRecordSize += pSS->FieldArray[20].Ind;/*21dPercentCLVD*/
    iRecordSize += sizeof(EWDBid);        /*22idMwFilter*/
    iRecordSize += pSS->FieldArray[22].Ind;/*23dLowCutHz*/
    iRecordSize += pSS->FieldArray[23].Ind;/*24dLowTaperHz*/
    iRecordSize += pSS->FieldArray[24].Ind;/*25dHighTaperHZ*/
    iRecordSize += pSS->FieldArray[25].Ind;/*26dHighCutHz*/
    iRecordSize += pSS->FieldArray[26].Ind;/*26dM0*/

    /* Make the number of records a multiple of 8 to avoid alignment problems on UNIX */
    iRecordsPerBuffer= (BUFFERSIZE/iRecordSize) & 0xfffffff8;
    
    /*Allocate space for row/col ret lens. - never freed */
    for(i=0;i<pSS->NumOfFields;i++)
    {
      pSS->FieldArray[i].pRetLens=malloc(iRecordsPerBuffer*EWDB_FIELD_RET_LEN);
    }

    pSS->FieldArray[0].pVal=&(pLocalBuffer[0]);
    LastSize=sizeof(EWDBid);         /* 0idMw */
    pSS->FieldArray[1].pVal=(void *)(
      (int)(pSS->FieldArray[0].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[1].Ind; /* 2dMxx */
    pSS->FieldArray[2].pVal=(void *)(
      (int)(pSS->FieldArray[1].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[2].Ind; /* 3dMxy */
    pSS->FieldArray[3].pVal=(void *)(
      (int)(pSS->FieldArray[2].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[3].Ind; /* 4dMxz */
    pSS->FieldArray[4].pVal=(void *)(
      (int)(pSS->FieldArray[3].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[4].Ind; /* 5dMyy */
    pSS->FieldArray[5].pVal=(void *)(
      (int)(pSS->FieldArray[4].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[5].Ind; /* 6dMyz */
    pSS->FieldArray[6].pVal=(void *)(
      (int)(pSS->FieldArray[5].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[6].Ind; /* 7dMzz */
    pSS->FieldArray[7].pVal=(void *)(
      (int)(pSS->FieldArray[6].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=sizeof(int);            /* 8iScalarExp */
    pSS->FieldArray[8].pVal=(void *)(
      (int)(pSS->FieldArray[7].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[8].Ind; /* 9dPFPStrike */
    pSS->FieldArray[9].pVal=(void *)(
      (int)(pSS->FieldArray[8].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[9].Ind; /* 10dPFPDip */
    pSS->FieldArray[10].pVal=(void *)(
      (int)(pSS->FieldArray[9].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[10].Ind; /* 11dPFPRake */
    pSS->FieldArray[11].pVal=(void *)(
      (int)(pSS->FieldArray[10].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[11].Ind; /* 12dAFPStrike */
    pSS->FieldArray[12].pVal=(void *)(
      (int)(pSS->FieldArray[11].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[12].Ind; /* 13dAFPDip */
    pSS->FieldArray[13].pVal=(void *)(
      (int)(pSS->FieldArray[12].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[13].Ind; /* 14dAFPRake */
    pSS->FieldArray[14].pVal=(void *)(
      (int)(pSS->FieldArray[13].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[14].Ind; /* 15dDepth */
    pSS->FieldArray[15].pVal=(void *)(
      (int)(pSS->FieldArray[14].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=sizeof(EWDBid);          /* 16idOrigin */
    pSS->FieldArray[16].pVal=(void *)(
      (int)(pSS->FieldArray[15].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=sizeof(EWDBid);          /* 17idMag */
    pSS->FieldArray[17].pVal=(void *)(
      (int)(pSS->FieldArray[16].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=sizeof(int);             /* 18iNumStations */
    pSS->FieldArray[18].pVal=(void *)(
      (int)(pSS->FieldArray[17].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[18].Ind; /* 19dMisfit */
    pSS->FieldArray[19].pVal=(void *)(
      (int)(pSS->FieldArray[18].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[19].Ind; /* 20dPercentDC */
    pSS->FieldArray[20].pVal=(void *)(
      (int)(pSS->FieldArray[19].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[20].Ind; /* 21dPercentCLVD */
    pSS->FieldArray[21].pVal=(void *)(
      (int)(pSS->FieldArray[20].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=sizeof(EWDBid);          /* 22idMwFilter */
    pSS->FieldArray[22].pVal=(void *)(
      (int)(pSS->FieldArray[21].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[22].Ind; /* 23dLowCutHz */
    pSS->FieldArray[23].pVal=(void *)(
      (int)(pSS->FieldArray[22].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[23].Ind; /* 24dLowTaperHz */
    pSS->FieldArray[24].pVal=(void *)(
      (int)(pSS->FieldArray[23].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[24].Ind; /* 25dHighTaperHZ */
    pSS->FieldArray[25].pVal=(void *)(
      (int)(pSS->FieldArray[24].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[25].Ind; /* 26dHighCutHz */
    pSS->FieldArray[26].pVal=(void *)(
      (int)(pSS->FieldArray[25].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[26].Ind; /* 27dM0 */

    pSS->FieldArray[27].pVal=&Local_idOrigin;

  } /* end if(!pLocalBuffer) */
  
  if(!pLocalBuffer)
    logit("","InitGetMwSolutionsForOriginStatement: malloc of pLocalBuffer "
          "failed! Returning.\n");

  return(ewdb_base_RequestCursor(szStatement, pSS,0/*don't force rebind*/));
}  /* End InitGetMwSolutionsForOriginStatement() */


static int PrepGetMwSolutionsForOriginExec(EWDBid idOrigin, EWDB_Cursor * ppCursor)
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

  Local_idOrigin  = idOrigin;

  if(InitGetMwSolutionsForOriginStatement(SQL_STRING, &SSStatement) != EWDB_RETURN_SUCCESS)
  {
    *ppCursor = NULL;
    return(EWDB_RETURN_FAILURE);
  }

  *ppCursor=SSStatement.pCda;

  return(EWDB_RETURN_SUCCESS);
}  /* End PrepGetMwSolutionsForOriginExec() */


static int PostGetMwSolutionsForOriginExec(EWDB_MwStruct * pMwBuffer, int BufferRecLen)
{
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
    /* reinitialize the buffer before each fetch to ensure NULL columns end up
       NULL, and not whatever value was there before.  Oracle will not write a
       NULL value into the provided buffer, so the buffer retains whatever value
       it had prior to the Oracle fetch.
       DK 
     ****************************************************************************/
    memset(pLocalBuffer, 0, BUFFERSIZE);

    if(ewdb_base_SQLFetchRows(pCursor, iRecordsPerBuffer))
    {
      if(ewdb_base_GetCursorRetCode(pCursor) == EWDB_SQL_ERROR_NO_DATA) 
      {
        done=1;
      }
      else
      {
        ewdb_base_ErrorReport(hEWDBC, pCursor,"PostGetMwSolutionsForOriginExec:ewdb_base_SQLFetchRows",1);
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
        memset(&pMwBuffer[UCurr],0,sizeof(EWDB_ArrivalStruct));


        pMwBuffer[UCurr].idMw=*(EWDBid *)((sizeof(int)*BCurr) +(int)(pSS->FieldArray[0].pVal));

        pTemp=(char *)((pSS->FieldArray[1].Ind*BCurr) +(int)(pSS->FieldArray[1].pVal) );
        pTemp[pSS->FieldArray[1].pRetLens[BCurr]]=0;
        pMwBuffer[UCurr].dMxx = atof(pTemp);

        pTemp=(char *)((pSS->FieldArray[2].Ind*BCurr) +(int)(pSS->FieldArray[2].pVal) );
        pTemp[pSS->FieldArray[2].pRetLens[BCurr]]=0;
        pMwBuffer[UCurr].dMxy = atof(pTemp);

        pTemp=(char *)((pSS->FieldArray[3].Ind*BCurr) +(int)(pSS->FieldArray[3].pVal) );
        pTemp[pSS->FieldArray[3].pRetLens[BCurr]]=0;
        pMwBuffer[UCurr].dMxz = atof(pTemp);

        pTemp=(char *)((pSS->FieldArray[4].Ind*BCurr) +(int)(pSS->FieldArray[4].pVal) );
        pTemp[pSS->FieldArray[4].pRetLens[BCurr]]=0;
        pMwBuffer[UCurr].dMyy = atof(pTemp);

        pTemp=(char *)((pSS->FieldArray[5].Ind*BCurr) +(int)(pSS->FieldArray[5].pVal) );
        pTemp[pSS->FieldArray[5].pRetLens[BCurr]]=0;
        pMwBuffer[UCurr].dMyz = atof(pTemp);

        pTemp=(char *)((pSS->FieldArray[6].Ind*BCurr) +(int)(pSS->FieldArray[6].pVal) );
        pTemp[pSS->FieldArray[6].pRetLens[BCurr]]=0;
        pMwBuffer[UCurr].dMzz = atof(pTemp);

        pTemp=(char *)((pSS->FieldArray[26].Ind*BCurr) +(int)(pSS->FieldArray[26].pVal) );
        pTemp[pSS->FieldArray[26].pRetLens[BCurr]]=0;
        pMwBuffer[UCurr].dM0 = atof(pTemp);

        pMwBuffer[UCurr].iScalarExp=*(int *)((sizeof(int)*BCurr) +(int)(pSS->FieldArray[7].pVal));

        pTemp=(char *)((pSS->FieldArray[8].Ind*BCurr) +(int)(pSS->FieldArray[8].pVal) );
        pTemp[pSS->FieldArray[8].pRetLens[BCurr]]=0;
        pMwBuffer[UCurr].dPFPStrike = atof(pTemp);

        pTemp=(char *)((pSS->FieldArray[9].Ind*BCurr) +(int)(pSS->FieldArray[9].pVal) );
        pTemp[pSS->FieldArray[9].pRetLens[BCurr]]=0;
        pMwBuffer[UCurr].dPFPDip = atof(pTemp);

        pTemp=(char *)((pSS->FieldArray[10].Ind*BCurr) +(int)(pSS->FieldArray[10].pVal) );
        pTemp[pSS->FieldArray[10].pRetLens[BCurr]]=0;
        pMwBuffer[UCurr].dPFPRake = atof(pTemp);

        pTemp=(char *)((pSS->FieldArray[11].Ind*BCurr) +(int)(pSS->FieldArray[11].pVal) );
        pTemp[pSS->FieldArray[11].pRetLens[BCurr]]=0;
        pMwBuffer[UCurr].dAFPStrike = atof(pTemp);

        pTemp=(char *)((pSS->FieldArray[12].Ind*BCurr) +(int)(pSS->FieldArray[12].pVal) );
        pTemp[pSS->FieldArray[12].pRetLens[BCurr]]=0;
        pMwBuffer[UCurr].dAFPDip = atof(pTemp);

        pTemp=(char *)((pSS->FieldArray[13].Ind*BCurr) +(int)(pSS->FieldArray[13].pVal) );
        pTemp[pSS->FieldArray[13].pRetLens[BCurr]]=0;
        pMwBuffer[UCurr].dAFPRake = atof(pTemp);

        pTemp=(char *)((pSS->FieldArray[14].Ind*BCurr) +(int)(pSS->FieldArray[14].pVal) );
        pTemp[pSS->FieldArray[14].pRetLens[BCurr]]=0;
        pMwBuffer[UCurr].dDepth = atof(pTemp);

        pMwBuffer[UCurr].idOrigin=*(EWDBid *)((sizeof(int)*BCurr) +(int)(pSS->FieldArray[15].pVal));
        pMwBuffer[UCurr].idMag=*(EWDBid *)((sizeof(int)*BCurr) +(int)(pSS->FieldArray[16].pVal));
        pMwBuffer[UCurr].iNumStations=*(int *)((sizeof(int)*BCurr) +(int)(pSS->FieldArray[17].pVal));

        pTemp=(char *)((pSS->FieldArray[18].Ind*BCurr) +(int)(pSS->FieldArray[18].pVal) );
        pTemp[pSS->FieldArray[18].pRetLens[BCurr]]=0;
        pMwBuffer[UCurr].dMisfit = atof(pTemp);

        pTemp=(char *)((pSS->FieldArray[19].Ind*BCurr) +(int)(pSS->FieldArray[19].pVal) );
        pTemp[pSS->FieldArray[19].pRetLens[BCurr]]=0;
        pMwBuffer[UCurr].dPercentDC = atof(pTemp);

        pTemp=(char *)((pSS->FieldArray[20].Ind*BCurr) +(int)(pSS->FieldArray[20].pVal) );
        pTemp[pSS->FieldArray[20].pRetLens[BCurr]]=0;
        pMwBuffer[UCurr].dPercentCLVD = atof(pTemp);

        pMwBuffer[UCurr].Filter.idMwFilter=*(EWDBid *)((sizeof(int)*BCurr) +(int)(pSS->FieldArray[21].pVal));

        pTemp=(char *)((pSS->FieldArray[22].Ind*BCurr) +(int)(pSS->FieldArray[22].pVal) );
        pTemp[pSS->FieldArray[22].pRetLens[BCurr]]=0;
        pMwBuffer[UCurr].Filter.dLowCutHz = (float)atof(pTemp);

        pTemp=(char *)((pSS->FieldArray[23].Ind*BCurr) +(int)(pSS->FieldArray[23].pVal) );
        pTemp[pSS->FieldArray[23].pRetLens[BCurr]]=0;
        pMwBuffer[UCurr].Filter.dLowTaperHz = (float)atof(pTemp);

        pTemp=(char *)((pSS->FieldArray[24].Ind*BCurr) +(int)(pSS->FieldArray[24].pVal) );
        pTemp[pSS->FieldArray[24].pRetLens[BCurr]]=0;
        pMwBuffer[UCurr].Filter.dHighTaperHz = (float)atof(pTemp);

        pTemp=(char *)((pSS->FieldArray[25].Ind*BCurr) +(int)(pSS->FieldArray[25].pVal) );
        pTemp[pSS->FieldArray[25].pRetLens[BCurr]]=0;
        pMwBuffer[UCurr].Filter.dHighCutHz = (float)atof(pTemp);

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
}  /* End PostGetGetMwSolutionsForOriginExec() */


