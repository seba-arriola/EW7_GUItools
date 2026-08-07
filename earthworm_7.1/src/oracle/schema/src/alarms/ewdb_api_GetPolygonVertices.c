/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_api_GetPolygonVertices.c,v 1.3 2005/06/03 22:32:16 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_api_GetPolygonVertices.c,v $
 *     Revision 1.3  2005/06/03 22:32:16  davidk
 *     DB API Cleanup
 *
 *     Revision 1.2  2004/12/01 20:37:34  mark
 *     Fixed retrieving floats (need to convert from strings...)
 *
 *     Revision 1.1  2004/11/23 17:26:26  mark
 *     Initial checkin
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <ewdb_cli_base.h>
#include <ewdb_ora_api.h>
#include <ewdb_ew_oci_base.h>


static char SQL_STRING[] =
         "SELECT idVertex, dLat, dLon, iOrder "
         " FROM ALL_Polygon_Vert_INFO "
         " WHERE idPolygon = :IN_idPolygon "
         " ORDER BY iOrder";



static EWDB_OCI_SFS SQLParamsBindArray[] = 
{
  {0,1,0,0,0,OA_EWDBID,"1idVertex"},
  {0,1,20,0,0,OA_DOUBLE,"2dLat"},
  {0,1,20,0,0,OA_DOUBLE,"3dLon"},
  {0,1,0,0,0,OA_INT,"4iOrder"},
  {0,1,0,0,0,OA_EWDBID,":IN_idPolygon"}
};

/* define the total number of fields to be bound */
#define NUM_FIELDS 5

/* Insertion Struct for GetTransFunc statement */
static EWDB_OCIStatementStruct SSStatement;

/* Temporary Storage Buffer params */
#define BUFFERSIZE 8192
static int    iRecordSize;
static int    iRecordsPerBuffer;
static char * pLocalBuffer=NULL;

static int    bInitialized=FALSE;

static  EWDBid  Local_idPolygon;

/********************************
      FUNCTION PROTOTYPES
********************************/
static int PrepGetPolygonList(EWDBid IN_idPolygon, EWDB_Cursor * ppCursor);
static int PostGetPolygonList(EWDB_PolygonVertexStruct * pBuffer, int BufferRecLen);
static int InitGetPolygonList(char * szStatement, EWDB_OCIStatementStruct *pSS);


int ewdb_api_GetPolygonVertices(EWDBid IN_idPolygon, EWDB_PolygonVertexStruct *pBuffer,
                                int *pNumFound, int *pNumRetrieved, int BufferLen)
{
  EWDB_Cursor  pCursor;
 
  ewdb_base_SetLastOraAPIActionTime();

  if( ewdb_base_Reconnect() != EWDB_RETURN_SUCCESS )
  {
    return( EWDB_RETURN_FAILURE );
  }

  if( PrepGetPolygonList(IN_idPolygon, &pCursor) != EWDB_RETURN_SUCCESS)
  {
    return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }
  
  if( ewdb_base_SQLExecute(pCursor) )
  {
    ewdb_base_ErrorReport(hEWDBC, pCursor,"ewdb_api_GetPolygonVertices:ewdb_base_SQLExecute",1);
    return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  } 

  /* Commit the transaction (all the previous inserts!)
     In Case there is any auditing or logging or debug
     changes made in the stored procedures.
  ****************************************************/
  if (ewdb_base_SQLCommit (hEWDBC))
  {
    ewdb_base_ErrorReport (hEWDBC, hEWDBC,"ewdb_api_GetPolygonVertices(): ewdb_base_SQLCommit",2);
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  }
  
  ewdb_base_SetLastOraAPIActionTime(); 

  if((*pNumFound = PostGetPolygonList(pBuffer,BufferLen)) == EWDB_RETURN_FAILURE)
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

}  /* end ewdb_api_GetPolygon() */


static int InitGetPolygonList(char * szStatement, EWDB_OCIStatementStruct *pSS)
{
  int LastSize,i;  /* Size of last column array */

  if(!pLocalBuffer)
  {
    /* Allocate space for returned data - never freed */
    pLocalBuffer = (char *)malloc(BUFFERSIZE * 2);

    iRecordSize = 0;
    iRecordSize += sizeof(EWDBid);         /*idVertex*/
    iRecordSize += pSS->FieldArray[1].Ind; /*dLat*/
    iRecordSize += pSS->FieldArray[2].Ind; /*dLon*/
    iRecordSize += sizeof(int);            /*iOrder*/

    /* Make the number of records a multiple of 8 to avoid alignment problems on UNIX;
   * round up so we actually have records to use.
   */
    iRecordsPerBuffer = ((BUFFERSIZE/iRecordSize) & 0xfffffff8) + 8;
    
    /* Allocate space for row/col ret lens. - never freed */
    for(i = 0; i < pSS->NumOfFields; i++)
      pSS->FieldArray[i].pRetLens = malloc(iRecordsPerBuffer*EWDB_FIELD_RET_LEN);

    pSS->FieldArray[0].pVal=pLocalBuffer;
    LastSize = sizeof(EWDBid);  /* idVertex */
        
    pSS->FieldArray[1].pVal=(void *)(
      (int)(pSS->FieldArray[0].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize = pSS->FieldArray[1].Ind;  /* dLat */
    
    pSS->FieldArray[2].pVal=(void *)(
      (int)(pSS->FieldArray[1].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize = pSS->FieldArray[2].Ind;  /* dLon */

    pSS->FieldArray[3].pVal=(void *)(
      (int)(pSS->FieldArray[2].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize = sizeof(int);  /* iOrder */

    /* incoming IN_idPolygon */
    pSS->FieldArray[4].pVal = &Local_idPolygon; 
    LastSize = sizeof(EWDBid); 
  } /* end if(!pLocalBuffer) */
  
  if(!pLocalBuffer)
  {
    logit("","InitGetPolygonList: malloc of pLocalBuffer failed! Returning.\n");
    return (EWDB_RETURN_FAILURE);
  }

  return(ewdb_base_RequestCursor(szStatement, pSS,0/*don't force rebind*/));
}  /* End InitGetPolygonList() */


static int PrepGetPolygonList(EWDBid IN_idPolygon, EWDB_Cursor * ppCursor)
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

  Local_idPolygon = IN_idPolygon;

  if(InitGetPolygonList(SQL_STRING, &SSStatement) != EWDB_RETURN_SUCCESS)
  {
    *ppCursor = NULL;
    return(EWDB_RETURN_FAILURE);
  }

  *ppCursor=SSStatement.pCda;

  return(EWDB_RETURN_SUCCESS);
}  /* End PrepGetPolygonList() */


static int PostGetPolygonList(EWDB_PolygonVertexStruct * pBuffer, int BufferRecLen)
{
  EWDB_OCIStatementStruct * pSS=&SSStatement;
  char * pTemp;
  int done=0;
  int RowsRetrieved=0;
  int RowsDone=0;
  EWDB_Cursor  pCursor=SSStatement.pCda;
  int BCurr,UCurr;
  int RowsProcessed;

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
                         "PostGetPolygonList:ewdb_base_SQLFetchRows",1);
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
        
        /* idVertex */
        pBuffer[UCurr].idVertex = *(EWDBid *)
        ((sizeof(EWDBid)*BCurr) + (int)(pSS->FieldArray[0].pVal));
                
        /* dLat */
        pTemp = (char *)((pSS->FieldArray[1].Ind*BCurr) + (int)(pSS->FieldArray[1].pVal));
        pBuffer[UCurr].dLat = (double)atof(pTemp);
        
        /* dLon */
        pTemp = (char *)((pSS->FieldArray[2].Ind*BCurr) + (int)(pSS->FieldArray[2].pVal));
        pBuffer[UCurr].dLon = (double)atof(pTemp);

    /* iOrder */
        pBuffer[UCurr].iOrder = *(int *)
        ((sizeof(int)*BCurr) + (int)(pSS->FieldArray[3].pVal));
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
      ewdb_base_ErrorReport(hEWDBC, pCursor,"PostGetPolygonList:ewdb_base_SQLFetchRows",2);
      return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
    }
  }  /* End if(RowsRetrieved > BufferRecLen) */
  
  RowsProcessed = ewdb_base_GetCursorRowsProcessedCount(pCursor);

  ewdb_base_ReleaseCursor(pCursor);

  return(RowsProcessed);
}  /* End PostGetGetDataChangeListExec() */



