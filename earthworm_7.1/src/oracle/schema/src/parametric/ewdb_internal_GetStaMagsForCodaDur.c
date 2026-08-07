/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_internal_GetStaMagsForCodaDur.c,v 1.6 2005/06/15 19:03:13 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_internal_GetStaMagsForCodaDur.c,v $
 *     Revision 1.6  2005/06/15 19:03:13  davidk
 *     DB API Cleanup
 *
 *     Revision 1.5  2003/09/16 16:56:55  davidk
 *     General API Cleanup
 *
 *     Revision 1.4  2003/08/21 00:59:11  davidk
 *     Restructured API code that deals with dynamic allocation of memory.
 *
 *     Revision 1.3  2001/07/12 21:07:17  davidk
 *     Fixed memory leak from mallocing of column "return lengths".
 *
 *     Revision 1.2  2001/05/24 00:57:41  davidk
 *     fixed a bunch of little bugs.  ewdb_internal_GetStaMagsForCodaDur.c has been tested on solaris.
 *
 *     Revision 1.1  2001/05/15 02:16:19  davidk
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
        "select idMagLink, idMeasurement idCodaDur, dMag, dWeight, idChan, "
        "  tCodaDurObs, tCodaDurXtp, tCodaTermObs, tCodaTermXtp, "
        "  idTCoda, iMagType "
        " from StaMag_CodaDur_Info where idMag=:idMagnitude";

static EWDB_OCI_SFS SQLParamsBindArray[] =

{
  {0,1,0,0,0,OA_EWDBID, "1idMagLink"},
  {0,1,0,0,0,OA_EWDBID, "2idCodaDur"},
  {0,1,20,0,0,OA_FLOAT, "3dMag"},
  {0,1,20,0,0,OA_FLOAT, "4dWeight"},
  {0,1,0,0,0,OA_EWDBID, "5idChan"},
  {0,1,20,0,0,OA_DOUBLE,"6tCodaDurObs"},
  {0,1,20,0,0,OA_DOUBLE,"7tCodaDurXtp"},
  {0,1,20,0,0,OA_DOUBLE,"8tCodaTermObs"},
  {0,1,20,0,0,OA_DOUBLE,"9tCodaTermXtp"},
  {0,1,0,0,0,OA_EWDBID, "10idTCoda"},
  {0,1,0,0,0,OA_INT,    "11iMagType"},
  {0,1,0,0,0,OA_EWDBID,":idMagnitude"}
};

/* define the total number of fields to be bound */
#define NUM_FIELDS 12

/* Insertion Struct for GetStaMagsForCodaDur statement */
EWDB_OCIStatementStruct SSStatement;


static  int     iRecordsPerBuffer;
static  EWDBid  Local_idMag;

/* Temporary Storage Buffer params */
#define BUFFERSIZE 8192
static int    iRecordSize;
static int    iRecordsPerBuffer;
static char * pLocalBuffer=NULL;

static int    bInitialized=FALSE;

/********************************
      FUNCTION PROTOTYPES
********************************/
int PrepGetStaMagsForCodaDurExec(EWDBid idMagnitude, EWDB_Cursor * ppCursor);
int PostGetStaMagsForCodaDurExec(EWDB_StationMagStruct * pBuffer,int BufferRecLen);
int InitGetStaMagsForCodaDurStatement(char * szStatement, EWDB_OCIStatementStruct *pSS);



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
int ewdb_internal_GetStaMagsForCodaDur(EWDBid idMagnitude, 
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

  if(PrepGetStaMagsForCodaDurExec(idMagnitude,&pCursor) != EWDB_RETURN_SUCCESS)
  {
    return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }

  
  if( ewdb_base_SQLExecute(pCursor) )
  {
    ewdb_base_ErrorReport(hEWDBC, pCursor,"EWDB_GetStaMagsForCodaDur:ewdb_base_SQLExecute",1);
    return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  } 
  

  if( (*pNumStaMagsFound=PostGetStaMagsForCodaDurExec(pStaMags,BufferLen)) 
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
}  /* End ewdb_internal_GetStaMagsForCodaDur() */



int InitGetStaMagsForCodaDurStatement(char * szStatement, EWDB_OCIStatementStruct *pSS)
{

  int LastSize,i;  /* Size of last column array */


  if(!pLocalBuffer)
  {
    /*Allocate space for returned data - never freed */
    pLocalBuffer=malloc(BUFFERSIZE);


    iRecordSize=0;
    iRecordSize += sizeof(EWDBid); /* idMagLink */
    iRecordSize += sizeof(EWDBid); /* idDurCoda */
    iRecordSize += pSS->FieldArray[2].Ind; /*dMag*/
    iRecordSize += pSS->FieldArray[3].Ind; /*dWeight*/
    iRecordSize += sizeof(EWDBid); /* idChan */
    iRecordSize += pSS->FieldArray[5].Ind; /*tCodaDurObs*/
    iRecordSize += pSS->FieldArray[6].Ind; /*tCodaDurXtp*/
    iRecordSize += pSS->FieldArray[7].Ind; /*tCodaTermObs*/
    iRecordSize += pSS->FieldArray[8].Ind; /*tCodaTermXtp*/
    iRecordSize += sizeof(EWDBid); /* idTCoda */
    iRecordSize += sizeof(int);    /* iMagType */
    
    /* Make the number of records a multiple of 8 to avoid alignment problems on UNIX */
    iRecordsPerBuffer= (BUFFERSIZE/iRecordSize) & 0xfffffff8;
    
    /*Allocate space for row/col ret lens. - never freed */
    for(i=0;i<pSS->NumOfFields;i++)
    {
      pSS->FieldArray[i].pRetLens=malloc(iRecordsPerBuffer*EWDB_FIELD_RET_LEN);
    }

  pSS->FieldArray[0].pVal=&(pLocalBuffer[0]);
  LastSize=sizeof(EWDBid);
  pSS->FieldArray[1].pVal= (void *) (
    (int)(pSS->FieldArray[0].pVal)+(LastSize*iRecordsPerBuffer));
  LastSize=sizeof(EWDBid);
  pSS->FieldArray[2].pVal= (void *) (
    (int)(pSS->FieldArray[1].pVal)+(LastSize*iRecordsPerBuffer));
  LastSize=pSS->FieldArray[2].Ind; /* dMag */
  pSS->FieldArray[3].pVal= (void *) (
    (int)(pSS->FieldArray[2].pVal)+(LastSize*iRecordsPerBuffer));
  LastSize=pSS->FieldArray[3].Ind; /* dWeight */
  pSS->FieldArray[4].pVal= (void *) (
    (int)(pSS->FieldArray[3].pVal)+(LastSize*iRecordsPerBuffer));
  LastSize=sizeof(EWDBid);  /* idchan */
  pSS->FieldArray[5].pVal= (void *) (
    (int)(pSS->FieldArray[4].pVal)+(LastSize*iRecordsPerBuffer));
  LastSize=pSS->FieldArray[5].Ind; /* tCodaDurObs */
  pSS->FieldArray[6].pVal= (void *) (
    (int)(pSS->FieldArray[5].pVal)+(LastSize*iRecordsPerBuffer));
  LastSize=pSS->FieldArray[6].Ind; /* tCodaDurXtp */
  pSS->FieldArray[7].pVal= (void *) (
    (int)(pSS->FieldArray[6].pVal)+(LastSize*iRecordsPerBuffer));
  LastSize=pSS->FieldArray[7].Ind; /* tCodaTermObs */
  pSS->FieldArray[8].pVal= (void *) (
    (int)(pSS->FieldArray[7].pVal)+(LastSize*iRecordsPerBuffer));
  LastSize=pSS->FieldArray[8].Ind; /* tCodaTermXtp */
  pSS->FieldArray[9].pVal= (void *) (
    (int)(pSS->FieldArray[8].pVal)+(LastSize*iRecordsPerBuffer));
  LastSize=sizeof(EWDBid);            /* idTCoda */
  pSS->FieldArray[10].pVal= (void *) (
    (int)(pSS->FieldArray[9].pVal)+(LastSize*iRecordsPerBuffer));
  LastSize=sizeof(int);            /* iMagType */

  pSS->FieldArray[11].pVal=&Local_idMag;

  } /* end if(!pLocalBuffer) */
  
  if(!pLocalBuffer)
    logit("","InitGetSelectedChannelsStatement: malloc of pLocalBuffer "
          "failed! Returning.\n");


  ewdb_base_RequestCursor(szStatement, pSS,0/*don't force rebind*/);

  return(EWDB_RETURN_SUCCESS);
}  /* end InitGetStaMagsForCodaDurStatement() */


int PrepGetStaMagsForCodaDurExec(EWDBid idMagnitude, EWDB_Cursor * ppCursor)
{
  Local_idMag=idMagnitude;

  /* Initialize the statement parameters */
  if(!bInitialized)
  {
    bInitialized = TRUE;
    memset(&SSStatement,0,sizeof(SSStatement));
    SSStatement.NumOfFields=NUM_FIELDS;
    SSStatement.FieldArray=SQLParamsBindArray;
    SSStatement.RecordSize=0;
  }

  InitGetStaMagsForCodaDurStatement(SQL_STRING, &SSStatement);

  *ppCursor=SSStatement.pCda;

  return(EWDB_RETURN_SUCCESS);
} /* end PrepGetStaMagsForCodaDurExec() */


int PostGetStaMagsForCodaDurExec(EWDB_StationMagStruct * pBuffer,int BufferRecLen)
{

  int done=0;
  int RowsRetrieved=0;
  int RowsDone=0;
  EWDB_Cursor  pCursor=SSStatement.pCda;
  char * pTemp;
  int BCurr,UCurr;
  EWDB_OCIStatementStruct * pSS=&SSStatement;
  int RecordsProcessed;
  EWDB_CodaDurationStruct * pCodaDur;


  while(!done)
  {

    if (ewdb_base_SQLFetchRows(pCursor, iRecordsPerBuffer))
    {
      if (ewdb_base_GetCursorRetCode(pCursor) == EWDB_SQL_ERROR_NO_DATA) 
      {
        done=1;
      }
      else
      {
        ewdb_base_ErrorReport(hEWDBC, pCursor,"PostGetStaMagsForCodaDurExec:ewdb_base_SQLFetchRows",1);
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

        /* set pCodaDur to be a shortcut to the current CodaDur struct */
        pCodaDur = &(pBuffer[UCurr].StaMagUnion.CodaDur);

        pBuffer[UCurr].idMagLink=* (EWDBid *)((sizeof(int)*BCurr) + (int)(pSS->FieldArray[0].pVal));

        pBuffer[UCurr].idDatum=* (EWDBid *)((sizeof(int)*BCurr) + (int)(pSS->FieldArray[1].pVal));
        pCodaDur->idCodaDur = pBuffer[UCurr].idDatum;

        pTemp=(char *) ((pSS->FieldArray[2].Ind*BCurr) + (int)(pSS->FieldArray[2].pVal) );
        pTemp[pSS->FieldArray[2].pRetLens[BCurr]]=0;
        pBuffer[UCurr].dMag=(float)atof(pTemp);

        pTemp=(char *) ((pSS->FieldArray[3].Ind*BCurr) + (int)(pSS->FieldArray[3].pVal) );
        pTemp[pSS->FieldArray[3].pRetLens[BCurr]]=0;
        pBuffer[UCurr].dWeight=(float)atof(pTemp);

        pBuffer[UCurr].idChan=* (EWDBid *)((sizeof(int)*BCurr) + (int)(pSS->FieldArray[4].pVal));

        pTemp=(char *) ((pSS->FieldArray[5].Ind*BCurr) + (int)(pSS->FieldArray[5].pVal) );
        pTemp[pSS->FieldArray[5].pRetLens[BCurr]]=0;
        pCodaDur->tCodaDurObs=atof(pTemp);

        pTemp=(char *) ((pSS->FieldArray[6].Ind*BCurr) + (int)(pSS->FieldArray[6].pVal) );
        pTemp[pSS->FieldArray[6].pRetLens[BCurr]]=0;
        pCodaDur->tCodaDurXtp=atof(pTemp);

        pTemp=(char *) ((pSS->FieldArray[7].Ind*BCurr) + (int)(pSS->FieldArray[7].pVal) );
        pTemp[pSS->FieldArray[7].pRetLens[BCurr]]=0;
        pCodaDur->tCodaTermObs=atof(pTemp);

        pTemp=(char *) ((pSS->FieldArray[8].Ind*BCurr) + (int)(pSS->FieldArray[8].pVal) );
        pTemp[pSS->FieldArray[8].pRetLens[BCurr]]=0;
        pCodaDur->tCodaTermXtp=atof(pTemp);

        pCodaDur->idTCoda=* (EWDBid *)((sizeof(int)*BCurr) + (int)(pSS->FieldArray[9].pVal));

        pBuffer[UCurr].iMagType=* (int *)((sizeof(int)*BCurr) + (int)(pSS->FieldArray[10].pVal));
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
      ewdb_base_ErrorReport(hEWDBC, pCursor,"PostGetStaMagsForCodaDurExec:ewdb_base_SQLFetchRows",2);
      return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
    }
    /* else */
  }  /* End if (RowsRetrieved > BufferRecLen) */
  
  RecordsProcessed = ewdb_base_GetCursorRowsProcessedCount(pCursor);
  ewdb_base_ReleaseCursor (pCursor);

  return(RecordsProcessed);

}  /* end of PostXXX() */


