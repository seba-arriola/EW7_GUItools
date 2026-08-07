/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_api_GetPolygons.c,v 1.3 2005/06/10 16:07:48 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_api_GetPolygons.c,v $
 *     Revision 1.3  2005/06/10 16:07:48  davidk
 *     DB Cleanup.  Fixed comments.
 *
 *     Revision 1.2  2005/06/03 22:32:16  davidk
 *     DB API Cleanup
 *
 *     Revision 1.1  2004/12/01 20:46:25  mark
 *     Initial checkin
 *
 *     Revision 1.3  2004/09/20 20:37:44  davidk
 *     Added an ORDER BY clause that orders the retrieved data by time/idEP
 *
 *     Revision 1.2  2004/07/21 17:01:36  mark
 *     Added tEntryCreated column to EventPassport table
 *
 *     Revision 1.1.1.1  2004/03/31 18:43:19  michelle
 *     New Hydra Import
 *
 *     Revision 1.5  2004/03/09 06:15:44  davidk
 *     Reformatted NEIC DB API routines, including:
 *         fixing bugs
 *         reformatting structures, routines, and tables
 *          rewriting routines.
 *
 *     Revision 1.4  2003/10/22 17:18:01  mark
 *     Fixed initialization errors
 *
 *     Revision 1.2  2003/10/09 17:56:13  davidk
 *     Cleanup of API call, to match standard list-retrieval format.
 *
 *     Revision 1.1  2003/08/05 21:26:42  lucky
 *     Initial revision
 *
 *     Revision 1.5  2003/08/05 21:25:54  lucky
 *     *** empty log message ***
 *
 *     Revision 1.4  2003/07/01 16:34:59  michelle
 *     changed line 220 from idEventPassport to idPassport
 *
 *     Revision 1.3  2003/05/27 21:17:09  michelle
 *     made fixes after testing, fixes include NUM_FIELDS, numRetreived and numReturned params
 *
 *
 */

#include <ewdb_ora_api.h>
#include <ewdb_cli_base.h>
#include <ewdb_ew_oci_base.h>


static char SQL_STRING[] =
         "SELECT idPolygon, sPolygonName "
         " FROM ALL_POLYGON_INFO "
         " ORDER BY sPolygonName";


static EWDB_OCI_SFS SQLParamsBindArray[] = 
{
  {0,1,0,0,0,OA_EWDBID,"1idPolygon"},
  {0,1,1024,0,0,OA_SZ,"2sPolygonName"}
};

/* define the total number of fields to be bound */
#define NUM_FIELDS 2

/* Insertion Struct for GetTransFunc statement */
static EWDB_OCIStatementStruct SSStatement;

/* Temporary Storage Buffer params */
#define BUFFERSIZE 8192
static int    iRecordSize;
static int    iRecordsPerBuffer;
static char * pLocalBuffer=NULL;

static int    bInitialized=FALSE;


/********************************
      FUNCTION PROTOTYPES
********************************/
static int PrepGetPolygons(EWDB_Cursor * ppCursor);
static int PostGetPolygons(EWDB_PolygonStruct * pBuffer, int BufferRecLen);
static int InitGetPolygons(char * szStatement, EWDB_OCIStatementStruct *pSS);


int ewdb_api_GetPolygons(EWDB_PolygonStruct *pBuffer,
                         int *pNumFound, int *pNumRetrieved, int BufferLen)
{

  EWDB_Cursor  pCursor;
 
  ewdb_base_SetLastOraAPIActionTime();

  if( ewdb_base_Reconnect() != EWDB_RETURN_SUCCESS )
  {
    return( EWDB_RETURN_FAILURE );
  }

  if( PrepGetPolygons(&pCursor) != EWDB_RETURN_SUCCESS)
  {
    return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }
  
  if( ewdb_base_SQLExecute(pCursor) )
  {
    ewdb_base_ErrorReport(hEWDBC, pCursor,"ewdb_api_GetPolygons(): ewdb_base_SQLExecute",1);
    return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  } 

  /* Commit the transaction (all the previous inserts!)
     In Case there is any auditing or logging or debug
     changes made in the stored procedures.
  ****************************************************/
  if (ewdb_base_SQLCommit (hEWDBC))
  {
    ewdb_base_ErrorReport (hEWDBC, hEWDBC,"ewdb_api_GetPolygons():ewdb_base_SQLCommit",2);
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  }

  ewdb_base_SetLastOraAPIActionTime(); 

  if((*pNumFound = PostGetPolygons(pBuffer,BufferLen)) == EWDB_RETURN_FAILURE)
  {
    return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }

  if(*pNumFound <= BufferLen)
  {
    *pNumRetrieved = *pNumFound;
    return(EWDB_RETURN_SUCCESS);
  }
  else
  {
    *pNumRetrieved = BufferLen;
    return(EWDB_RETURN_WARNING);
  }
}  /* end ewdb_api_GetEventPassport() */


static int InitGetPolygons(char * szStatement, EWDB_OCIStatementStruct *pSS)
{
  int LastSize,i;  /* Size of last column array */

  if(!pLocalBuffer)
  {
    /* Allocate space for returned data - never freed */
    pLocalBuffer = malloc(BUFFERSIZE * 2);

    iRecordSize=0;
    iRecordSize += sizeof(EWDBid);          /*idPolygon*/
    iRecordSize += pSS->FieldArray[1].Ind;  /*sPolygonName*/

    /* Make the number of records a multiple of 8 to avoid alignment problems on UNIX;
     * round up so we actually have records to use.
     */
    iRecordsPerBuffer = ((BUFFERSIZE/iRecordSize) & 0xfffffff8) + 8;
    
    /* Allocate space for row/col ret lens. - never freed */
    for(i=0;i<pSS->NumOfFields;i++)
    {
      pSS->FieldArray[i].pRetLens=malloc(iRecordsPerBuffer*EWDB_FIELD_RET_LEN);
    }

    pSS->FieldArray[0].pVal=pLocalBuffer;
    LastSize=sizeof(EWDBid);  /* idPolygon */
    
    pSS->FieldArray[1].pVal=(void *)(
      (int)(pSS->FieldArray[0].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize = pSS->FieldArray[1].Ind;  /* sPolygonName */
        
  } /* end if(!pLocalBuffer) */
  
  if(!pLocalBuffer)
  {
    logit("","InitGetEventPassportList: malloc of pLocalBuffer "
          "failed! Returning.\n");
  }

  return(ewdb_base_RequestCursor(szStatement, pSS,0/*don't force rebind*/));

}  /* End InitGetEventPassportList() */


static int PrepGetPolygons(EWDB_Cursor * ppCursor)
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


  if(InitGetPolygons(SQL_STRING, &SSStatement) != EWDB_RETURN_SUCCESS)
  {
    *ppCursor = NULL;
    return(EWDB_RETURN_FAILURE);
  }

  *ppCursor=SSStatement.pCda;

  return(EWDB_RETURN_SUCCESS);
}  /* End PrepGetEventPassportList() */


static int PostGetPolygons(EWDB_PolygonStruct * pBuffer, int BufferRecLen)
{
  int done=0;
  int RowsRetrieved=0;
  int RowsDone=0;
  EWDB_Cursor  pCursor=SSStatement.pCda;
  int BCurr,UCurr;
  EWDB_OCIStatementStruct * pSS=&SSStatement;
  int RowsProcessed;
  char *pTemp;

  while(!done)
  {
    memset(pLocalBuffer, 0, BUFFERSIZE*2);

    if(ewdb_base_SQLFetchRows(pCursor, iRecordsPerBuffer))
    {
      if(ewdb_base_GetCursorRetCode(pCursor) == EWDB_SQL_ERROR_NO_DATA) 
      {
        done=1;
      }
      else
      {
        ewdb_base_ErrorReport(hEWDBC, pCursor,
                         "PostGetEventPassportList:ewdb_base_SQLFetchRows",1);
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
        
        /* idPolygon */
        pBuffer[UCurr].idPolygon=*(EWDBid *) ((sizeof(EWDBid)*BCurr) +
          (int)(pSS->FieldArray[0].pVal));
        
        /* sPolygonName */
        pTemp = (char *) ((pSS->FieldArray[1].Ind * BCurr) +
          (int)(pSS->FieldArray[1].pVal));
        pTemp[pSS->FieldArray[1].pRetLens[BCurr]] = 0;
        strcpy (pBuffer[UCurr].szPolygonName, pTemp);
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
      ewdb_base_ErrorReport(hEWDBC, pCursor,"PostGetGetDataChangeListExec:ewdb_base_SQLFetchRows",2);
      return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
    }
    /* else */
  }  /* End if(RowsRetrieved > BufferRecLen) */
  
  RowsProcessed = ewdb_base_GetCursorRowsProcessedCount(pCursor);

  ewdb_base_ReleaseCursor(pCursor);

  return(RowsProcessed);
}  /* End PostGetGetDataChangeListExec() */



