/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_api_GetMergeList.c,v 1.3 2003/09/16 16:56:55 davidk Exp $
 *    Revision history:
 *
 *    $Log: ewdb_api_GetMergeList.c,v $
 *    Revision 1.3  2003/09/16 16:56:55  davidk
 *    General API Cleanup
 *
 *    Revision 1.2  2003/08/21 00:53:11  davidk
 *    Restructured API code that deals with dynamic allocation of memory.
 *
 *    Revision 1.1  2002/03/22 20:01:23  lucky
 *    Initial revision
 *
 * 
 */


#include <stdlib.h>
#include <stdio.h>
#include <ewdb_cli_base.h>
#include <ewdb_ora_api.h>
#include <ewdb_ew_oci_base.h>
  

static char SQL_STRING[500];
static char SQL_STRING_BASE[] =
 "select idEvent, idPh, idMerge, idPrefEvent, "
	"tOrigin, dLat, dLon, dDepth, dPrefMag, "
	"sSource, sHumanReadable "
 "from ALL_MERGE_INFO";

#define szWHERE " WHERE "
#define szAND   " AND "

#define szBASE 11
#define szSTTM 0
#define szENTM 1
#define szLATN 2
#define szLATX 3
#define szLONN 4
#define szLONX 5
#define szZMIN 6
#define szZMAX 7
#define szMGMN 8
#define szMGMX 9 
#define szIDPH 10 


/* Total number of fields, currently BASE + VLMD + 1 */
#define NUM_FIELDS 22


static char SQL_STRING_OPT[][30] = 
    {
      "tOrigin>=:starttime",
      "tOrigin<=:endtime",
      "dLat>=:minlat",
      "dLat<=:maxlat",
      "dLon>=:minlon",
      "dLon<=:maxlon",
      "dDepth>=:minz",
      "dDepth<=:maxz",
      "dPrefMag>=:minmag",
      "dPrefMag<=:maxmag",
      "idPh=:idph"
    };

static EWDB_OCI_SFS SQLParamsBindArray[] = 
{
  {0,1,0,0,0,OA_EWDBID,"1idEvent"},
  {0,1,0,0,0,OA_EWDBID,"2idPh"},
  {0,1,0,0,0,OA_EWDBID,"3idMerge"},
  {0,1,0,0,0,OA_EWDBID,"4idPrefEvent"},
  {0,1,20,0,0,OA_DOUBLE,"5tOrigin"},
  {0,1,20,0,0,OA_FLOAT,"6dLat"},
  {0,1,20,0,0,OA_FLOAT,"7dLon"},
  {0,1,20,0,0,OA_FLOAT,"8dDepth"},
  {0,1,20,0,0,OA_FLOAT,"9dPrefMag"},
  {0,1,52,0,0,OA_SZ,"10sSource"},
  {0,1,256,0,0,OA_SZ,"11sHumanReadable"},
  {0,0,0,0,0,OA_INT,":starttime"},
  {0,0,0,0,0,OA_INT,":endtime"},
  {0,0,0,0,0,OA_FLOAT,":minlat"},
  {0,0,0,0,0,OA_FLOAT,":maxlat"},
  {0,0,0,0,0,OA_FLOAT,":minlon"},
  {0,0,0,0,0,OA_FLOAT,":maxlon"},
  {0,0,0,0,0,OA_FLOAT,":minz"},
  {0,0,0,0,0,OA_FLOAT,":maxz"},
  {0,0,0,0,0,OA_FLOAT,":minmag"},
  {0,0,0,0,0,OA_FLOAT,":maxmag"},
  {0,0,0,0,0,OA_EWDBID,":idph"}
};

/* Insertion Struct for GetMergeList statement */
static EWDB_OCIStatementStruct SSStatement;

/* Temporary Storage Buffer params */
#define BUFFERSIZE 8192
static int    iRecordSize;
static int    iRecordsPerBuffer;
static char * pLocalBuffer=NULL;

static int    bInitialized=FALSE;

static  int     iStartTime;
static  int     iEndTime;
static  EWDBid  LocalidPh;

static  EWDB_MergeList     MinMergeStruct;
static  EWDB_MergeList     MaxMergeStruct;

static char szMinLat[15],szMaxLat[15],szMinLon[15],
            szMaxLon[15],szMinZ[15],szMaxZ[15],
			szMinMag[15],szMaxMag[15];



#define BUFFERSIZE 8192

/********************************
      FUNCTION PROTOTYPES
********************************/
int PrepGetMergeListExec(EWDB_MergeList *pMin,
                         EWDB_MergeList *pMax, EWDBid idPh, EWDB_Cursor * ppCursor);
int PostGetMergeListExec(EWDB_MergeList *pBuffer, int BufferRecLen);
static char * AddCriteria(char *MainString, int NumAdditions, char *pCriteria);
int InitGetMergeListStatement(char * szStatement, EWDB_OCIStatementStruct *pSS);


/*********************************************************************** 
***********************************************************************/
int ewdb_api_GetMergeList (EWDB_MergeList *pMerge, int BufferLen, EWDBid idPh,
                          EWDB_MergeList *pMin, EWDB_MergeList *pMax, 
							int *NumFound, int *NumRetr)
{

  EWDB_Cursor  pCursor;


	if ((pMerge == NULL) || (BufferLen <= 0))
	{
		logit ("", "Invalid merge buffer pointer; exitting\n");
		return( EWDB_RETURN_FAILURE );
	}

	if (idPh < 0)
	{
		if ((pMin == NULL) || (pMax == NULL))
		{
			logit ("", "Min and Max structures are null; exiting\n");
			return( EWDB_RETURN_FAILURE );
		}
	}

  ewdb_base_SetLastOraAPIActionTime();

  if( ewdb_base_Reconnect() != EWDB_RETURN_SUCCESS )
     return( EWDB_RETURN_FAILURE );


  if (PrepGetMergeListExec (pMin, pMax, idPh, &pCursor) != EWDB_RETURN_SUCCESS )
  {
    logit("","ORA_API:GetMergeList():PrepGetMergeListExec() failed.\n");
    return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }
  
  if( ewdb_base_SQLExecute(pCursor) )
  {
    ewdb_base_ErrorReport(hEWDBC, pCursor,"GetMergeList:ewdb_base_SQLExecute",1);
    return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  } 

  
  /* Commit the transaction (all the previous inserts!)
     In Case there is any auditing or logging or debug
     changes made in the stored procedures.
  ****************************************************/
  if (ewdb_base_SQLCommit(hEWDBC))
  {
    ewdb_base_ErrorReport(hEWDBC, hEWDBC,"GetMergeList:ewdb_base_SQLCommit",2);
    return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE) );
  }

  if((*NumFound = PostGetMergeListExec (pMerge, BufferLen)) == EWDB_RETURN_FAILURE )
  {
    return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }

  ewdb_base_SetLastOraAPIActionTime();
  if(*NumFound > BufferLen)
  {
    *NumRetr = BufferLen;
    return(EWDB_RETURN_SUCCESS);
  }
  else
  {
    *NumRetr = *NumFound;
    return(EWDB_RETURN_SUCCESS);
  }
} 



int InitGetMergeListStatement(char *szStatement, EWDB_OCIStatementStruct *pSS)
{

  int LastSize,i;  /* Size of last column array */


  if(!pLocalBuffer)
  {
    /*Allocate space for returned data - never freed */
    pLocalBuffer=malloc(BUFFERSIZE);


    iRecordSize=0;
    iRecordSize += sizeof(EWDBid); /*idEvent*/
    iRecordSize += sizeof(EWDBid); /*idPh*/
    iRecordSize += sizeof(EWDBid); /*idMerge*/
    iRecordSize += sizeof(EWDBid); /*idPrefEvent*/
    iRecordSize += pSS->FieldArray[4].Ind; /*tOrigin*/
    iRecordSize += pSS->FieldArray[5].Ind; /*dLat*/
    iRecordSize += pSS->FieldArray[6].Ind; /*dLon*/
    iRecordSize += pSS->FieldArray[7].Ind; /*dDepth*/
    iRecordSize += pSS->FieldArray[8].Ind; /*dPrefMag*/
    iRecordSize += pSS->FieldArray[9].Ind; /*sSource*/
    iRecordSize += pSS->FieldArray[10].Ind; /*sHumanReadable*/
    
    /* Make the number of records a multiple of 8 to avoid alignment problems on UNIX */
    iRecordsPerBuffer= (BUFFERSIZE/iRecordSize) & 0xfffffff8;
    
    /*Allocate space for row/col ret lens. - never freed */
    for(i=0;i<pSS->NumOfFields;i++)
    {
      pSS->FieldArray[i].pRetLens=malloc(iRecordsPerBuffer*EWDB_FIELD_RET_LEN);
    }


  pSS->FieldArray[0].pVal=&(pLocalBuffer[0]);
  LastSize=sizeof(int); /* idEvent */
  
  pSS->FieldArray[1].pVal= (void *) (
    (int)(pSS->FieldArray[0].pVal)+(LastSize*iRecordsPerBuffer));
  LastSize=sizeof(int); /* idPh */
  
  pSS->FieldArray[2].pVal= (void *) (
    (int)(pSS->FieldArray[1].pVal)+(LastSize*iRecordsPerBuffer));
  LastSize=sizeof(int); /* idMerge */
  
  pSS->FieldArray[3].pVal= (void *) (
    (int)(pSS->FieldArray[2].pVal)+(LastSize*iRecordsPerBuffer));
  LastSize=sizeof(int); /* idPrefEvent */
  
  pSS->FieldArray[4].pVal= (void *) (
    (int)(pSS->FieldArray[3].pVal)+(LastSize*iRecordsPerBuffer));
  LastSize=pSS->FieldArray[4].Ind;  /* double tOrigin */
  
  pSS->FieldArray[5].pVal= (void *) (
    (int)(pSS->FieldArray[4].pVal)+(LastSize*iRecordsPerBuffer));
  LastSize=pSS->FieldArray[5].Ind;  /* float dLat */
  
  pSS->FieldArray[6].pVal= (void *) (
    (int)(pSS->FieldArray[5].pVal)+(LastSize*iRecordsPerBuffer));
  LastSize=pSS->FieldArray[6].Ind;  /* float dLon */
  
  pSS->FieldArray[7].pVal= (void *) (
    (int)(pSS->FieldArray[6].pVal)+(LastSize*iRecordsPerBuffer));
  LastSize=pSS->FieldArray[7].Ind;  /* float dDepth */

  pSS->FieldArray[8].pVal= (void *) (
    (int)(pSS->FieldArray[7].pVal)+(LastSize*iRecordsPerBuffer));
  LastSize=pSS->FieldArray[8].Ind;  /* float dPrefMag */
  
  pSS->FieldArray[9].pVal= (void *) (
    (int)(pSS->FieldArray[8].pVal)+(LastSize*iRecordsPerBuffer));
  LastSize=pSS->FieldArray[9].Ind; /*char sSource[]*/

  pSS->FieldArray[10].pVal= (void *) (
    (int)(pSS->FieldArray[9].pVal)+(LastSize*iRecordsPerBuffer));
  LastSize=pSS->FieldArray[10].Ind; /* char sHumanReadable[]*/

  pSS->FieldArray[11].pVal=&(iStartTime);
  pSS->FieldArray[12].pVal=&(iEndTime);
  pSS->FieldArray[13].pVal=szMinLat;
  pSS->FieldArray[14].pVal=szMaxLat;
  pSS->FieldArray[15].pVal=szMinLon;
  pSS->FieldArray[16].pVal=szMaxLon;
  pSS->FieldArray[17].pVal=szMinZ;
  pSS->FieldArray[18].pVal=szMaxZ;
  pSS->FieldArray[19].pVal=szMinMag;
  pSS->FieldArray[20].pVal=szMaxMag;
  pSS->FieldArray[21].pVal=&(LocalidPh);

  } /* end if(!pLocalBuffer) */
  
  if(!pLocalBuffer)
    logit("","InitGetMergeListStatement: malloc of pLocalBuffer "
          "failed! Returning.\n");


  ewdb_base_RequestCursor(szStatement, pSS,0/*don't force rebind*/);

  return(EWDB_RETURN_SUCCESS);
}  /* end InitGetMergeListStatement() */

        

static char * AddCriteria(char * MainString,int NumAdditions, char * pCriteria)
{
  if(NumAdditions == 0)
    strcat(MainString,szWHERE);
  else
    strcat(MainString,szAND);
  return(strcat(MainString,pCriteria));
}

int PrepGetMergeListExec(EWDB_MergeList *pMin, EWDB_MergeList *pMax, 
                         EWDBid idPh, EWDB_Cursor * ppCursor)
    /*
    1.  check criteria to see which statement configuration to execute.
    2.  check to see if the desired configuration is the same as the last.
    3.  if yes, then done.  if no, then we need to re prep the statement, go to 4.
    4.  Build the desired statement.
    5.  Parse the statement.
    6.  Autobind the statement.
    7.  Manual bind any remaining variables.
    8.  Done.
    */
{

  int StatementAdditions=0;

  szMinLat[0]=szMaxLat[0]=szMinLon[0]=szMaxLon[0]= 
		szMinZ[0]=szMaxZ[0]=szMinMag[0]=szMaxMag[0]=0;


  /* Initialize the statement parameters */
  if(!bInitialized)
  {
    bInitialized = TRUE;
    memset(&SSStatement,0,sizeof(SSStatement));
    SSStatement.NumOfFields=NUM_FIELDS;
    SSStatement.FieldArray=SQLParamsBindArray;
    SSStatement.RecordSize=0;
  }


  /* reset all of the optional fields */
  SQLParamsBindArray[szBASE+szSTTM].UseField = FALSE;
  SQLParamsBindArray[szBASE+szENTM].UseField = FALSE;
  SQLParamsBindArray[szBASE+szLATN].UseField = FALSE;
  SQLParamsBindArray[szBASE+szLATX].UseField = FALSE;
  SQLParamsBindArray[szBASE+szLONN].UseField = FALSE;
  SQLParamsBindArray[szBASE+szLONX].UseField = FALSE;
  SQLParamsBindArray[szBASE+szZMIN].UseField = FALSE;
  SQLParamsBindArray[szBASE+szZMAX].UseField = FALSE;
  SQLParamsBindArray[szBASE+szMGMN].UseField = FALSE;
  SQLParamsBindArray[szBASE+szMGMX].UseField = FALSE;
  SQLParamsBindArray[szBASE+szIDPH].UseField = FALSE;


  /* Create the base string */
  sprintf(SQL_STRING,"%s",SQL_STRING_BASE);


  /* 
     If we are passed a "valid" idPh, we only retrieve its info;
     otherwise, search given the parameters.
   */

  if (idPh > 0)
  {
    LocalidPh = idPh;

    AddCriteria (SQL_STRING, StatementAdditions, SQL_STRING_OPT[szIDPH]);
    SQLParamsBindArray[szBASE+szIDPH].UseField = 1;
    StatementAdditions++;
  }
  else
  {

	  memcpy (&MinMergeStruct, pMin, sizeof(EWDB_MergeList));
	  memcpy (&MaxMergeStruct, pMax, sizeof(EWDB_MergeList));
	  iStartTime = (int) pMin->tOrigin;
	  iEndTime = (int) pMax->tOrigin;

	  if(iStartTime)
  	  {
	    AddCriteria (SQL_STRING, StatementAdditions, SQL_STRING_OPT[szSTTM]);
   	    SQLParamsBindArray[szBASE+szSTTM].UseField = 1;
        StatementAdditions++;
      }

      if(iEndTime)
      {
        AddCriteria (SQL_STRING, StatementAdditions, SQL_STRING_OPT[szENTM]);
        SQLParamsBindArray[szBASE+szENTM].UseField = 1;
        StatementAdditions++;
      }

      if(MinMergeStruct.dLat)
      {
        sprintf(szMinLat, "%f", MinMergeStruct.dLat);
        AddCriteria (SQL_STRING, StatementAdditions, SQL_STRING_OPT[szLATN]);
        SQLParamsBindArray[szBASE+szLATN].UseField = 1;
        StatementAdditions++;
      }

      if(MaxMergeStruct.dLat)
      {
        sprintf (szMaxLat, "%f", MaxMergeStruct.dLat);
        AddCriteria(SQL_STRING,StatementAdditions,SQL_STRING_OPT[szLATX]);
        SQLParamsBindArray[szBASE+szLATX].UseField=1;
        StatementAdditions++;
      }

      if(MinMergeStruct.dLon)
      {
        sprintf(szMinLon,"%f",MinMergeStruct.dLon);
        AddCriteria(SQL_STRING,StatementAdditions,SQL_STRING_OPT[szLONN]);
        SQLParamsBindArray[szBASE+szLONN].UseField=1;
        StatementAdditions++;
      }

      if(MaxMergeStruct.dLon)
      {
        sprintf(szMaxLon,"%f",MaxMergeStruct.dLon);
        AddCriteria(SQL_STRING,StatementAdditions,SQL_STRING_OPT[szLONX]);
        SQLParamsBindArray[szBASE+szLONX].UseField=1;
        StatementAdditions++;
      }

      if(MinMergeStruct.dDepth)
      {
        sprintf(szMinZ,"%f",MinMergeStruct.dDepth);
        AddCriteria(SQL_STRING,StatementAdditions,SQL_STRING_OPT[szZMIN]);
        SQLParamsBindArray[szBASE+szZMIN].UseField=1;
        StatementAdditions++;
      }

      if(MaxMergeStruct.dDepth)
      {
        sprintf(szMaxZ,"%f",MaxMergeStruct.dDepth);
        AddCriteria(SQL_STRING,StatementAdditions,SQL_STRING_OPT[szZMAX]);
        SQLParamsBindArray[szBASE+szZMAX].UseField=1;
        StatementAdditions++;
      }

      if(MinMergeStruct.dPrefMag)
      {
        sprintf(szMinMag,"%f",MinMergeStruct.dPrefMag);
        AddCriteria(SQL_STRING,StatementAdditions,SQL_STRING_OPT[szMGMN]);
        SQLParamsBindArray[szBASE+szMGMN].UseField=1;
        StatementAdditions++;
      }

      if(MaxMergeStruct.dPrefMag)
      {
        sprintf(szMaxMag,"%f",MaxMergeStruct.dPrefMag);
        AddCriteria(SQL_STRING,StatementAdditions,SQL_STRING_OPT[szMGMX]);
        SQLParamsBindArray[szBASE+szMGMX].UseField=1;
        StatementAdditions++;
      }
   }


  /* Set the query to order by Origin Time */
  strcat(SQL_STRING," ORDER BY tOrigin DESC");

  InitGetMergeListStatement(SQL_STRING, &SSStatement);

  *ppCursor=SSStatement.pCda;

  return(EWDB_RETURN_SUCCESS);
}  /* end PrepGetMergeListExec() */



int PostGetMergeListExec(EWDB_MergeList *pBuffer, int BufferRecLen)
{
  int done=0;
  int RowsRetrieved=0;
  int RowsDone=0;
  EWDB_Cursor  pCursor=SSStatement.pCda;
  int BCurr,UCurr;
  EWDB_OCIStatementStruct * pSS=&SSStatement;
  char * pTemp;
  int RowsProcessed;


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
        ewdb_base_ErrorReport(hEWDBC, pCursor,"PostGetMergeListExec:ewdb_base_SQLFetchRows",1);
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
        pBuffer[UCurr].idEvent=* (int *)((sizeof(int)*BCurr) + 
									(int)(pSS->FieldArray[0].pVal));

        pBuffer[UCurr].idPh=* (int *)((sizeof(int)*BCurr) + 
									(int)(pSS->FieldArray[1].pVal));
        
        pBuffer[UCurr].idMerge=* (int *)((sizeof(int)*BCurr) + 
									(int)(pSS->FieldArray[2].pVal));
        
        pBuffer[UCurr].idPrefEvent=* (int *)((sizeof(int)*BCurr) + 
									(int)(pSS->FieldArray[3].pVal));


        pTemp=(char *) ((pSS->FieldArray[4].Ind*BCurr) + 
									(int)(pSS->FieldArray[4].pVal) );
        pTemp[pSS->FieldArray[4].pRetLens[BCurr]]=0;
        pBuffer[UCurr].tOrigin = atof(pTemp);


        pTemp=(char *) ((pSS->FieldArray[5].Ind*BCurr) + 
									(int)(pSS->FieldArray[5].pVal) );
        pTemp[pSS->FieldArray[5].pRetLens[BCurr]]=0;
        pBuffer[UCurr].dLat = (float)atof(pTemp);


        pTemp=(char *) ((pSS->FieldArray[6].Ind*BCurr) + 
									(int)(pSS->FieldArray[6].pVal) );
        pTemp[pSS->FieldArray[6].pRetLens[BCurr]]=0;
        pBuffer[UCurr].dLon = (float)atof(pTemp);


        pTemp=(char *) ((pSS->FieldArray[7].Ind*BCurr) + 
									(int)(pSS->FieldArray[7].pVal) );
        pTemp[pSS->FieldArray[7].pRetLens[BCurr]]=0;
        pBuffer[UCurr].dDepth = (float)atof(pTemp);


        pTemp=(char *) ((pSS->FieldArray[8].Ind*BCurr) + 
									(int)(pSS->FieldArray[8].pVal) );
        pTemp[pSS->FieldArray[8].pRetLens[BCurr]]=0;
        pBuffer[UCurr].dPrefMag = (float)atof(pTemp);


        pTemp=(char *) ((pSS->FieldArray[9].Ind*BCurr) +
									(int)(pSS->FieldArray[9].pVal));
        pTemp[pSS->FieldArray[9].pRetLens[BCurr]]=0;
        strcpy(pBuffer[UCurr].szSource,pTemp);


        pTemp=(char *) ((pSS->FieldArray[10].Ind*BCurr) +
									(int)(pSS->FieldArray[10].pVal));
        pTemp[pSS->FieldArray[10].pRetLens[BCurr]]=0;
        strcpy(pBuffer[UCurr].szHumanReadable,pTemp);


      }
    } /* End for RowsDone < RowsRetrieved */

  }  /* End while !done */
  

  if (RowsRetrieved > BufferRecLen)
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
      ewdb_base_ErrorReport(hEWDBC, pCursor,"PostGetMergeListExec:ewdb_base_SQLFetchRows",2);
      return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
    }
    /* else */
  }  /* End if (RowsRetrieved > BufferRecLen) */
  

  RowsProcessed = ewdb_base_GetCursorRowsProcessedCount(pCursor);

  ewdb_base_ReleaseCursor(pCursor);

  return(RowsProcessed);
}  /* end PostGetMergeListExec() */
