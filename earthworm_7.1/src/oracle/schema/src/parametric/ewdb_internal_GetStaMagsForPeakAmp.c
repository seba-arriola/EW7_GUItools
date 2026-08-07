/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_internal_GetStaMagsForPeakAmp.c,v 1.11 2005/06/15 19:03:13 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_internal_GetStaMagsForPeakAmp.c,v $
 *     Revision 1.11  2005/06/15 19:03:13  davidk
 *     DB API Cleanup
 *
 *     Revision 1.10  2003/10/31 20:35:36  davidk
 *     Changed the SSStatement to be static.  It was picking up another statement struct from another file,
 *     and then generating a strange SQL error as a result.
 *
 *     Revision 1.9  2003/10/30 22:25:33  davidk
 *     Fixed semantic error caused by last syntax error fix.  Removed references to idPeakAmp in SQL
 *     Statement.
 *
 *     Revision 1.8  2003/10/29 00:24:18  davidk
 *     added "," in SQL statement after idMagLink to fix syntax error.
 *
 *     Revision 1.7  2003/09/16 16:56:55  davidk
 *     General API Cleanup
 *
 *     Revision 1.6  2003/08/21 00:59:30  davidk
 *     Restructured API code that deals with dynamic allocation of memory.
 *
 *     Revision 1.5  2001/07/12 21:25:55  davidk
 *     changed code in InitXXX() function so that id didn't look like IN_idMag
 *     was part of the select list of the query, but was instead an input param.
 *
 *     Revision 1.4  2001/07/12 21:07:17  davidk
 *     Fixed memory leak from mallocing of column "return lengths".
 *
 *     Revision 1.2  2001/05/24 00:59:05  davidk
 *     Fixed a bug where one of the fields in the SQL query didn't match the view it was
 *     querying agains.
 *
 *     Revision 1.1  2001/05/15 02:16:24  davidk
 *     Initial revision
 *
 *     Revision 1.1  2001/02/28 17:19:22  lucky
 *     Initial revision
 *
 *     Revision 1.6  2001/02/21 08:39:22  davidk
 *     Code was retrofitted to use the new db_cli_base API
 *     in lieu of the oci_base() and OCI7 function API's.layer, which
 *     provides abstraction from the OCI7 function calls to
 *     ora_api layer code.  See comments in
 *     schema/src/include/internal/ewdb_cli_base.h for more info
 *     on the details of the changes made.
 *
 *     Revision 1.5  2000/06/21 22:51:35  lucky
 *     Cleaned up logit calls to make log files more readable.
 *
 *     Revision 1.4  2000/05/12 23:59:36  davidk
 *     Added prototypes for all of the functions implemented in this file.
 *
 *     Revision 1.3  1999/11/09 18:21:08  lucky
 *     *** empty log message ***
 *
 *
 */

#include <ewdb_cli_base.h>
#include <ewdb_ora_api.h>
#include <ewdb_ew_oci_base.h>

static char SQL_STRING[] =
        "select idMagLink, idMeasurement, dMag, dWeight, idChan, "
        " dPeakAmp1, tAmp1, tPeriod1, dPeakAmp2, tAmp2, tPeriod2, "
        " iMagType from StaMag_PeakAmp_Info where idMag=:IN_idMag";

static EWDB_OCI_SFS SQLParamsBindArray[] =

{
  {0,1,0,0,0,OA_EWDBID,"1idMagLink"},
  {0,1,0,0,0,OA_EWDBID,"2idMeasurement"},
  {0,1,20,0,0,OA_FLOAT,"3dMag"},
  {0,1,20,0,0,OA_FLOAT,"4dWeight"},
  {0,1,0,0,0,OA_EWDBID,"5idChan"},
  {0,1,20,0,0,OA_DOUBLE,"6dPeakAmp1"},
  {0,1,20,0,0,OA_DOUBLE,"7tAmp1"},
  {0,1,20,0,0,OA_FLOAT, "8tPeriod1"},
  {0,1,20,0,0,OA_DOUBLE,"9dPeakAmp2"},
  {0,1,20,0,0,OA_DOUBLE,"10tAmp2"},
  {0,1,20,0,0,OA_FLOAT, "11tPeriod2"},
  {0,1,0,0,0,OA_INT,    "12iMagType"},
  {0,1,0,0,0,OA_EWDBID,":IN_idMag"}
};

/* define the total number of fields to be bound */
#define NUM_FIELDS 13

/* Insertion Struct for GetStaMagsForPeakAmp statement */
static EWDB_OCIStatementStruct SSStatement;

/* Temporary Storage Buffer params */
#define BUFFERSIZE 8192
static int    iRecordSize;
static int    iRecordsPerBuffer;
static char * pLocalBuffer=NULL;

static int    bInitialized=FALSE;

static  int     Local_idMag;

/********************************
      FUNCTION PROTOTYPES
********************************/
static int PrepGetStaMagsForPeakAmpExec(EWDBid IN_idMag, EWDB_Cursor * ppCursor);
static int PostGetStaMagsForPeakAmpExec(EWDB_StationMagStruct * pBuffer,int BufferRecLen);
static int InitGetStaMagsForPeakAmpStatement(char * szStatement, EWDB_OCIStatementStruct *pSS);



/***********************************************************************
  Used to get all of station magnitudes (magnitude measurements at 
      different stations) for a given summary magnitude.

  pStaMags                Pointer to a buffer allocated by the caller
	                         where the function is to stick the stamags.
  pNumStaMagsFound:       The number of StaMags found (in the DB) for 
                           the Magnitude(summary).
  pNumArrivalsRetrieved:  The number of StaMags retrieved from the DB.
  BufferLen:              The length of the buffer in terms of StaMagStructs.

    Return Value:   
     EWDB_RETURN_FAILURE     unknown error
     EWDB_RETURN_WARNING     success, but the buffer was not able to 
                              accommodate all of the StaMags found.
     EWDB_RETURN_SUCCESS     success 

    Other Details:  Caller is responsible for allocating
	space for the arrivals buffer.
***********************************************************************/
int ewdb_internal_GetStaMagsForPeakAmp(EWDBid IN_idMag, 
                                       EWDB_StationMagStruct * pStaMags,
                                       int * pNumStaMagsFound, 
                                       int * pNumStaMagsRetrieved,
                                       int BufferLen)
{

  EWDB_Cursor  pCursor;
 
  ewdb_base_SetLastOraAPIActionTime();

  if( ewdb_base_Reconnect() != EWDB_RETURN_SUCCESS )
  {
    return( EWDB_RETURN_FAILURE );
  }

  if(PrepGetStaMagsForPeakAmpExec(IN_idMag,&pCursor) != EWDB_RETURN_SUCCESS)
  {
    return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }
  
  if( ewdb_base_SQLExecute(pCursor) )
  {
    ewdb_base_ErrorReport(hEWDBC, pCursor,"ewdb_internal_GetStaMagsForPeakAmp(): ewdb_base_SQLExecute",1);
    return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  } 

  if( (*pNumStaMagsFound=PostGetStaMagsForPeakAmpExec(pStaMags,BufferLen)) 
      == EWDB_RETURN_FAILURE)
  {
    return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }

  ewdb_base_SetLastOraAPIActionTime();

  if(*pNumStaMagsFound <= BufferLen)
  {
    *pNumStaMagsRetrieved = *pNumStaMagsFound;
    return(EWDB_RETURN_SUCCESS);
  }
  else
  {
    *pNumStaMagsRetrieved = BufferLen;
    return(EWDB_RETURN_WARNING);
  }
}  /* End ewdb_internal_GetStaMagsForPeakAmp() */


static int InitGetStaMagsForPeakAmpStatement(char * szStatement, EWDB_OCIStatementStruct *pSS)
{
  int LastSize,i;  /* Size of last column array */


  if(!pLocalBuffer)
  {
    /*Allocate space for returned data - never freed */
    pLocalBuffer=malloc(BUFFERSIZE);

    iRecordSize=0;
    iRecordSize += sizeof(EWDBid); /* idMagLink */
    iRecordSize += sizeof(EWDBid); /* idMeasurement */
    iRecordSize += pSS->FieldArray[2].Ind; /*dMag*/
    iRecordSize += pSS->FieldArray[3].Ind; /*dWeight*/
    iRecordSize += sizeof(EWDBid); /* idChan */
    iRecordSize += pSS->FieldArray[5].Ind; /*dPeakAmp1*/
    iRecordSize += pSS->FieldArray[6].Ind; /*tAmp1*/
    iRecordSize += pSS->FieldArray[7].Ind; /*tPeriod1*/
    iRecordSize += pSS->FieldArray[8].Ind; /*dPeakAmp2*/
    iRecordSize += pSS->FieldArray[9].Ind; /*tAmp2*/
    iRecordSize += pSS->FieldArray[10].Ind; /*tPeriod2*/
    iRecordSize += sizeof(int);    /* iMagType */

    /* Make the number of records a multiple of 8 to avoid alignment problems on UNIX */
    iRecordsPerBuffer= (BUFFERSIZE/iRecordSize) & 0xfffffff8;
    
    /*Allocate space for row/col ret lens. - never freed */
    for(i=0;i<pSS->NumOfFields;i++)
    {
      pSS->FieldArray[i].pRetLens=malloc(iRecordsPerBuffer*EWDB_FIELD_RET_LEN);
    }

    /* idMagLink */
    pSS->FieldArray[0].pVal=&(pLocalBuffer[0]);
    LastSize=sizeof(EWDBid);

    /* idMeasurement */
    pSS->FieldArray[1].pVal= (void *) (
      (int)(pSS->FieldArray[0].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=sizeof(EWDBid);

    /* dMag */
    pSS->FieldArray[2].pVal= (void *) (
      (int)(pSS->FieldArray[1].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[2].Ind; 

    /* dWeight */
    pSS->FieldArray[3].pVal= (void *) (
      (int)(pSS->FieldArray[2].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[3].Ind; 

    /* idChan */
    pSS->FieldArray[4].pVal= (void *) (
      (int)(pSS->FieldArray[3].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=sizeof(EWDBid);

    /* dPeakAmp1 */
    pSS->FieldArray[5].pVal= (void *) (
      (int)(pSS->FieldArray[4].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[5].Ind;

    /* tAmp1 */
    pSS->FieldArray[6].pVal= (void *) (
      (int)(pSS->FieldArray[5].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[6].Ind;

    /* tPeriod1 */
    pSS->FieldArray[7].pVal= (void *) (
      (int)(pSS->FieldArray[6].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[7].Ind; 

    /* dPeakAmp2 */
    pSS->FieldArray[8].pVal= (void *) (
      (int)(pSS->FieldArray[7].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[8].Ind; 

    /* tAmp2 */
    pSS->FieldArray[9].pVal= (void *) (
      (int)(pSS->FieldArray[8].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[9].Ind; 

    /* tPeriod2 */
    pSS->FieldArray[10].pVal= (void *) (
      (int)(pSS->FieldArray[9].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[10].Ind;

    /* iMagType */
    pSS->FieldArray[11].pVal= (void *) (
      (int)(pSS->FieldArray[10].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=sizeof(int);


    /* IN_idMag */
    pSS->FieldArray[12].pVal=&Local_idMag;

  } /* end if(!pLocalBuffer) */
  
  if(!pLocalBuffer)
    logit("","InitGetStaMagsForPeakAmpStatement(): malloc of pLocalBuffer "
          "failed! Returning.\n");


  return(ewdb_base_RequestCursor(szStatement, pSS,0));
}  /* end InitGetStaMagsForPeakAmpStatement() */


static int PrepGetStaMagsForPeakAmpExec(EWDBid IN_idMag, EWDB_Cursor * ppCursor)
{
  Local_idMag=IN_idMag;

  /* Initialize the statement parameters */
  if(!bInitialized)
  {
    bInitialized = TRUE;
    memset(&SSStatement,0,sizeof(SSStatement));
    SSStatement.NumOfFields=NUM_FIELDS;
    SSStatement.FieldArray=SQLParamsBindArray;
    SSStatement.RecordSize=0;
  }

  if(InitGetStaMagsForPeakAmpStatement(SQL_STRING, &SSStatement) != EWDB_RETURN_SUCCESS)
  {
    *ppCursor = NULL;
    return(EWDB_RETURN_FAILURE);
  }

  *ppCursor=SSStatement.pCda;

  return(EWDB_RETURN_SUCCESS);
} /* end PrepGetStaMagsForPeakAmpExec() */


static int PostGetStaMagsForPeakAmpExec(EWDB_StationMagStruct * pBuffer,int BufferRecLen)
{

  int done=0;
  int RowsRetrieved=0;
  int RowsDone=0;
  EWDB_Cursor  pCursor=SSStatement.pCda;
  char * pTemp;
  int BCurr,UCurr;
  EWDB_OCIStatementStruct * pSS=&SSStatement;
  int RecordsProcessed;
  EWDB_PeakAmpStruct * pPeakAmp;

  while(!done)
  {
    memset(pLocalBuffer, 0, BUFFERSIZE);

    if (ewdb_base_SQLFetchRows(pCursor, iRecordsPerBuffer))
    {
      if (ewdb_base_GetCursorRetCode(pCursor) == EWDB_SQL_ERROR_NO_DATA) 
      {
        done=1;
      }
      else
      {
        ewdb_base_ErrorReport(hEWDBC, pCursor,"PostGetStaMagsForPeakAmpExec:ewdb_base_SQLFetchRows",1);
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

        /* set pPeakAmp to be a shortcut to the current PeakAmp struct */
        pPeakAmp = &(pBuffer[UCurr].StaMagUnion.PeakAmp);

        /* idMagLink */
        pBuffer[UCurr].idMagLink=
				* (EWDBid *)((sizeof(int)*BCurr) + (int)(pSS->FieldArray[0].pVal));

        /* idMeasurement */
        pBuffer[UCurr].idDatum=
				* (EWDBid *)((sizeof(int)*BCurr) + (int)(pSS->FieldArray[1].pVal));
        pPeakAmp->idPeakAmp = pBuffer[UCurr].idDatum;

        /* dMag */
        pTemp=(char *) ((pSS->FieldArray[2].Ind*BCurr) + (int)(pSS->FieldArray[2].pVal) );
        pTemp[pSS->FieldArray[2].pRetLens[BCurr]]=0;
        pBuffer[UCurr].dMag=(float)atof(pTemp);

        /* dWeight */
        pTemp=(char *) ((pSS->FieldArray[3].Ind*BCurr) + (int)(pSS->FieldArray[3].pVal) );
        pTemp[pSS->FieldArray[3].pRetLens[BCurr]]=0;
        pBuffer[UCurr].dWeight=(float)atof(pTemp);

        /* idChan */
        pBuffer[UCurr].idChan=
				* (EWDBid *)((sizeof(int)*BCurr) + (int)(pSS->FieldArray[4].pVal));

        /* dPeakAmp1 */
        pTemp=(char *) ((pSS->FieldArray[5].Ind*BCurr) + (int)(pSS->FieldArray[5].pVal) );
        pTemp[pSS->FieldArray[5].pRetLens[BCurr]]=0;
        pPeakAmp->dAmp1= (float)atof(pTemp);

        /* tAmp1 */
        pTemp=(char *) ((pSS->FieldArray[6].Ind*BCurr) + (int)(pSS->FieldArray[6].pVal) );
        pTemp[pSS->FieldArray[6].pRetLens[BCurr]]=0;
        pPeakAmp->tAmp1=atof(pTemp);

        /* tPeriod1 */
        pTemp=(char *) ((pSS->FieldArray[7].Ind*BCurr) + (int)(pSS->FieldArray[7].pVal) );
        pTemp[pSS->FieldArray[7].pRetLens[BCurr]]=0;
        pPeakAmp->dAmpPeriod1=(float)atof(pTemp);

        /* dPeakAmp2 */
        pTemp=(char *) ((pSS->FieldArray[8].Ind*BCurr) + (int)(pSS->FieldArray[8].pVal) );
        pTemp[pSS->FieldArray[8].pRetLens[BCurr]]=0;
        pPeakAmp->dAmp2= (float)atof(pTemp);

        /* tAmp2 */
        pTemp=(char *) ((pSS->FieldArray[9].Ind*BCurr) + (int)(pSS->FieldArray[9].pVal) );
        pTemp[pSS->FieldArray[9].pRetLens[BCurr]]=0;
        pPeakAmp->tAmp2=atof(pTemp);

        /* tPeriod2 */
        pTemp=(char *) ((pSS->FieldArray[10].Ind*BCurr) + (int)(pSS->FieldArray[10].pVal) );
        pTemp[pSS->FieldArray[10].pRetLens[BCurr]]=0;
        pPeakAmp->dAmpPeriod2=(float)atof(pTemp);

        /* iMagType */
        pBuffer[UCurr].iMagType=
				* (int *)((sizeof(int)*BCurr) + (int)(pSS->FieldArray[11].pVal));
      }
    } /* End for RowsDone < RowsRetrieved */
  }  /* End while !done */
  
  if (RowsRetrieved > BufferRecLen)
  {
    /* keep going till we get all of the rows,
       but ignore the contents, since we've
       filled up the user's buffer.
    */
    logit("","Retrieving unhandled rows  rowsretrieved %d, BufRL %d.\n",RowsRetrieved,BufferRecLen);
    if(ewdb_base_GetCursorRetCode(pCursor) != EWDB_SQL_ERROR_NO_DATA)
    {
      while(!ewdb_base_SQLFetchRows(pCursor, iRecordsPerBuffer))
      {
        /*logit("et","Rows Processed %d, Return Code %d\n",
              ewdb_base_GetCursorRowsProcessedCount(pCursor),
              ewdb_base_GetCursorRetCode(pCursor));
        */
      }
    }

    if(ewdb_base_GetCursorRetCode(pCursor) != EWDB_SQL_ERROR_NO_DATA)
    {
      ewdb_base_ErrorReport(hEWDBC, pCursor,"PostGetStaMagsForPeakAmpExec:ewdb_base_SQLFetchRows",2);
      return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
    }
    /* else */
  }  /* End if (RowsRetrieved > BufferRecLen) */
  
  RecordsProcessed = ewdb_base_GetCursorRowsProcessedCount(pCursor);
  ewdb_base_ReleaseCursor (pCursor);

  return(RecordsProcessed);

}  /* end of PostXXX() */


