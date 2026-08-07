/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_api_GetPlaceList.c,v 1.4 2005/06/06 16:42:50 davidk Exp $
 *    Revision history:
 *
 *    $Log: ewdb_api_GetPlaceList.c,v $
 *    Revision 1.4  2005/06/06 16:42:50  davidk
 *    DB API Cleanup
 *
 *    Revision 1.3  2004/03/17 20:34:35  davidk
 *    Cleaned up syntax.
 *
 *    Revision 1.2  2003/08/21 00:47:16  davidk
 *    Restructured API code that deals with dynamic allocation of memory.
 *
 *    Revision 1.1  2003/01/30 23:13:30  lucky
 *    Initial revision
 *
 * 
 */


#include <ewdb_cli_base.h>
#include <ewdb_ora_api.h>
#include <ewdb_ew_oci_base.h>
  

static char SQL_STRING[500];
static char SQL_STRING_BASE[] =
 "select idPlace, iPlaceMajorType, iPlaceMinorType, dLat, dLon, dElev, "
 "iPopulation, sName, sState, sCounty, sCountry "
 "from Place";

#define szWHERE " WHERE "
#define szAND   " AND "

#define szBASE 11
#define Local_szMATY 0
#define Local_szMITY 1
#define szLATN 2
#define szLATX 3
#define szLONN 4
#define szLONX 5
#define szELEN 6
#define szELEX 7
#define szPOPN 8
#define szPOPX 9
#define szSTAT 10
#define szCTRY 11
#define szCOTY 12
#define szNAME 13

/* Total number of fields, currently BASE + VLMD + 1 */
#define NUM_FIELDS 25


static char SQL_STRING_OPT[][30] = 
    {
      "iPlaceMajorType=:majortype",
      "iPlaceMinorType=:minortype",
      "dLat>=:minlat",
      "dLat<=:maxlat",
      "dLon>=:minlon",
      "dLon<=:maxlon",
      "dElev>=:minelev",
      "dElev<=:maxelev",
      "iPopulation>=:minpop",
      "iPopulation<=:maxpop",
      "sState=:state",
      "sCountry=:country",
      "sCounty=:county",
      "sName=:name"
    };

static EWDB_OCI_SFS SQLParamsBindArray[] = 
{
  {0,1,0,0,0,OA_EWDBID,	"1idPlace"},
  {0,1,0,0,0,OA_INT,	"2iPlaceMajorType"},
  {0,1,0,0,0,OA_INT,	"3iPlaceMinorType"},
  {0,1,20,0,0,OA_FLOAT,	"4dLat"},
  {0,1,20,0,0,OA_FLOAT,	"5dLon"},
  {0,1,20,0,0,OA_FLOAT,	"6dElev"},
  {0,1,0,0,0,OA_INT,	"7iPopulation"},
  {0,1,70,0,0,OA_SZ,	"8sName"},
  {0,1,5,0,0,OA_SZ,		"9sState"},
  {0,1,30,0,0,OA_SZ,	"10sCounty"},
  {0,1,30,0,0,OA_SZ,	"11sCountry"},
  {0,0,0,0,0,OA_INT,	":majortype"},
  {0,0,0,0,0,OA_INT,	":minortype"},
  {0,0,0,0,0,OA_FLOAT,	":minlat"},
  {0,0,0,0,0,OA_FLOAT,	":maxlat"},
  {0,0,0,0,0,OA_FLOAT,	":minlon"},
  {0,0,0,0,0,OA_FLOAT,	":maxlon"},
  {0,0,0,0,0,OA_FLOAT,	":minelev"},
  {0,0,0,0,0,OA_FLOAT,	":maxelev"},
  {0,0,0,0,0,OA_INT,	":minpop"},
  {0,0,0,0,0,OA_INT,	":maxpop"},
  {0,0,0,0,0,OA_SZ,		":state"},
  {0,0,0,0,0,OA_SZ,		":country"},
  {0,0,0,0,0,OA_SZ,		":county"},
  {0,0,0,0,0,OA_SZ,		":name"}

};

/* Insertion Struct for GetPlaceList statement */
static EWDB_OCIStatementStruct SSStatement;

/* Temporary Storage Buffer params */
#define BUFFERSIZE 81920
static int    iRecordSize;
static int    iRecordsPerBuffer;
static char * pLocalBuffer=NULL;

static int    bInitialized=FALSE;

static  int     minPopulation, maxPopulation;
static  EWDB_PlaceStruct     LocalPlaceStruct;

static char Local_szMinLat[15],Local_szMaxLat[15],Local_szMinLon[15],
            Local_szMaxLon[15],Local_szMinElev[15],Local_szMaxElev[15];


/********************************
      FUNCTION PROTOTYPES
********************************/
static int PrepGetPlaceListExec (EWDB_PlaceStruct *pMinPlace,
                         EWDB_PlaceStruct *pMaxPlace, EWDB_Cursor * ppCursor);
static int PostGetPlaceListExec (EWDB_PlaceStruct *pBuffer, int BufferRecLen);
static char * AddCriteria(char *MainString, int NumAdditions, char *pCriteria);
static int InitGetPlaceListStatement(char *szStatement, EWDB_OCIStatementStruct *pSS);


/*********************************************************************** 
   Used to get the list of all Places in the database that meet certain criteria.

	pBuffer is a pointer to a buffer allocated by the caller and used
	 by GetPlaceList to store the array of places.

	BufferlLen is the length of the Place array (pBuffer)  allocated by the caller.

 	The search criteria are determined by the two PlaceStructs that are passed in 
	as pointers pMinStruct and pMaxStruct.  Wherever a range is appropriate (i.e.,
	min and max population) the two structures are used.  Where only a single
	criterion is needed (i.e., get all places in Utah) only the pMinPlace is used.

	pNumRet is the number of Place structures returned by GetPlaceList.
	pNumFound is the number of Place found in the DB that meet the criteria 

      Return Value: -1 when an error occurs
					-x when the buffer passed by the caller is too
					 small, where x is the neccessary size of len
					 in PlaceStructs to get all of the places for the given criteria.
					0 on success.

    Other Details:  Caller is responsible for allocating space for the place list buffer.

***********************************************************************/
int ewdb_api_GetPlaceList (EWDB_PlaceStruct *pBuffer, int BufferLen, EWDB_PlaceStruct *pMinPlace, 
			EWDB_PlaceStruct *pMaxPlace, int *pNumRet, int *pNumFound)
{

  EWDB_Cursor  pCursor;


	if ((pBuffer == NULL) || (BufferLen <= 0) || (pMinPlace == NULL) ||
			(pMaxPlace == NULL) || (pNumRet == NULL) || (pNumFound == NULL))
	{
		logit ("", "ewdb_api_GetPlaceList(): Invalid arguments passed in.\n");
     	return( EWDB_RETURN_FAILURE );
	}
  
  ewdb_base_SetLastOraAPIActionTime();

  if( ewdb_base_Reconnect() != EWDB_RETURN_SUCCESS )
  {
     return( EWDB_RETURN_FAILURE );
  }

  if( PrepGetPlaceListExec(pMinPlace,pMaxPlace,&pCursor) != EWDB_RETURN_SUCCESS )
  {
    logit("","ewdb_api_GetPlaceList(): PrepGetPlaceListExec() failed.\n");
    return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }
  
  if( ewdb_base_SQLExecute(pCursor) )
  {
    ewdb_base_ErrorReport(hEWDBC, pCursor,"ewdb_api_GetPlaceList(): ewdb_base_SQLExecute",1);
    return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  } 

  /* Commit the transaction (all the previous inserts!)
     In Case there is any auditing or logging or debug
     changes made in the stored procedures.
  ****************************************************/
  if (ewdb_base_SQLCommit(hEWDBC))
  {
    ewdb_base_ErrorReport(hEWDBC, hEWDBC,"ewdb_api_GetPlaceList(): ewdb_base_SQLCommit",2);
    return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE) );
  }

  if((*pNumFound = PostGetPlaceListExec((EWDB_PlaceStruct *)pBuffer,BufferLen)) 
      == EWDB_RETURN_FAILURE )
  {
    return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }

  ewdb_base_SetLastOraAPIActionTime();

  if (*pNumFound > BufferLen)
    *pNumRet = BufferLen;
  else
    *pNumRet = *pNumFound;

   return(EWDB_RETURN_SUCCESS);
} /* end ewdb_api_GetPlaceList() */


static int InitGetPlaceListStatement(char * szStatement, EWDB_OCIStatementStruct *pSS)
{

  int LastSize,i;  /* Size of last column array */


  if(!pLocalBuffer)
  {
    /*Allocate space for returned data - never freed */
    pLocalBuffer=malloc(BUFFERSIZE);

    iRecordSize=0;
    iRecordSize += sizeof(EWDBid); /*idPlace*/
    iRecordSize += sizeof(int); /*iPlaceMajorType*/
    iRecordSize += sizeof(int); /*iPlaceMinorType*/
    iRecordSize += pSS->FieldArray[3].Ind; /*dLat*/
    iRecordSize += pSS->FieldArray[4].Ind; /*dLon*/
    iRecordSize += pSS->FieldArray[5].Ind; /*dElev*/
    iRecordSize += sizeof(int); /*iPopulation*/
    iRecordSize += pSS->FieldArray[7].Ind; /*sName*/
    iRecordSize += pSS->FieldArray[8].Ind; /*sState*/
    iRecordSize += pSS->FieldArray[9].Ind; /*sCounty*/
    iRecordSize += pSS->FieldArray[10].Ind; /*sCountry*/

    /* Make the number of records a multiple of 8 to avoid alignment problems on UNIX */
    iRecordsPerBuffer= (BUFFERSIZE/iRecordSize) & 0xfffffff8;
    
    /*Allocate space for row/col ret lens. - never freed */
    for(i=0;i<pSS->NumOfFields;i++)
    {
      pSS->FieldArray[i].pRetLens=malloc(iRecordsPerBuffer*EWDB_FIELD_RET_LEN);
    }

    pSS->FieldArray[0].pVal=&(pLocalBuffer[0]);
    LastSize=sizeof(int); 	/* idPlace */
  
    pSS->FieldArray[1].pVal= (void *) (
      (int)(pSS->FieldArray[0].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=sizeof(int);		/* iPlaceMajorType */
  
    pSS->FieldArray[2].pVal= (void *) (
      (int)(pSS->FieldArray[1].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=sizeof(int);		/* iPlaceMinorType */
  
    pSS->FieldArray[3].pVal= (void *) (
      (int)(pSS->FieldArray[2].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[3].Ind;  /* float dLat */

    pSS->FieldArray[4].pVal= (void *) (
      (int)(pSS->FieldArray[3].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[4].Ind;  /* float dLon */

    pSS->FieldArray[5].pVal= (void *) (
      (int)(pSS->FieldArray[4].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[5].Ind;  /* float dElev */

    pSS->FieldArray[6].pVal= (void *) (
      (int)(pSS->FieldArray[5].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=sizeof (int);  	/* iPopulation */
  
    pSS->FieldArray[7].pVal= (void *) (
      (int)(pSS->FieldArray[6].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[7].Ind; /*char szPlaceName[]*/

    pSS->FieldArray[8].pVal= (void *) (
      (int)(pSS->FieldArray[7].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[8].Ind; /*char szState[]*/

    pSS->FieldArray[9].pVal= (void *) (
      (int)(pSS->FieldArray[8].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[9].Ind; /*char szCountry[]*/

    pSS->FieldArray[10].pVal= (void *) (
      (int)(pSS->FieldArray[9].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[10].Ind; /*char szCounty[]*/

    pSS->FieldArray[11].pVal=&(LocalPlaceStruct.iPlaceMajorType);
    pSS->FieldArray[12].pVal=&(LocalPlaceStruct.iPlaceMinorType);
    pSS->FieldArray[13].pVal=Local_szMinLat;
    pSS->FieldArray[14].pVal=Local_szMaxLat;
    pSS->FieldArray[15].pVal=Local_szMinLon;
    pSS->FieldArray[16].pVal=Local_szMaxLon;
    pSS->FieldArray[17].pVal=Local_szMinElev;
    pSS->FieldArray[18].pVal=Local_szMaxElev;
    pSS->FieldArray[19].pVal=&minPopulation;
    pSS->FieldArray[20].pVal=&maxPopulation;
    pSS->FieldArray[21].pVal=LocalPlaceStruct.szState;
    pSS->FieldArray[22].pVal=LocalPlaceStruct.szCounty;
    pSS->FieldArray[23].pVal=LocalPlaceStruct.szCountry;
    pSS->FieldArray[24].pVal=LocalPlaceStruct.szPlaceName;

  } /* end if(!pLocalBuffer) */
  
  if(!pLocalBuffer)
    logit("","InitGetPlaceListStatement: malloc of pLocalBuffer "
          "failed! Returning.\n");

  return(ewdb_base_RequestCursor(szStatement, pSS,0/*don't force rebind*/));
}  /* end InitGetPlaceListStatement() */


static char * AddCriteria(char * MainString,int NumAdditions, char * pCriteria)
{
  if(NumAdditions == 0)
    strcat(MainString,szWHERE);
  else
    strcat(MainString,szAND);
  return(strcat(MainString,pCriteria));
}


static int PrepGetPlaceListExec (EWDB_PlaceStruct *pMinPlace,
                         EWDB_PlaceStruct *pMaxPlace, EWDB_Cursor * ppCursor)
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

  Local_szMinLat[0]=Local_szMaxLat[0]=Local_szMinLon[0]=Local_szMaxLon[0]=Local_szMinElev[0]=Local_szMaxElev[0];
  minPopulation = maxPopulation = 0;


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
  SQLParamsBindArray[szBASE+Local_szMATY].UseField = FALSE;
  SQLParamsBindArray[szBASE+Local_szMITY].UseField = FALSE;
  SQLParamsBindArray[szBASE+szLATN].UseField = FALSE;
  SQLParamsBindArray[szBASE+szLATX].UseField = FALSE;
  SQLParamsBindArray[szBASE+szLONN].UseField = FALSE;
  SQLParamsBindArray[szBASE+szLONX].UseField = FALSE;
  SQLParamsBindArray[szBASE+szELEN].UseField = FALSE;
  SQLParamsBindArray[szBASE+szELEX].UseField = FALSE;
  SQLParamsBindArray[szBASE+szPOPN].UseField = FALSE;
  SQLParamsBindArray[szBASE+szPOPX].UseField = FALSE;
  SQLParamsBindArray[szBASE+szSTAT].UseField = FALSE;
  SQLParamsBindArray[szBASE+szCTRY].UseField = FALSE;
  SQLParamsBindArray[szBASE+szCOTY].UseField = FALSE;
  SQLParamsBindArray[szBASE+szNAME].UseField = FALSE;


  memcpy(&LocalPlaceStruct,pMinPlace,sizeof(EWDB_PlaceStruct));

  /* Create the base string */
  sprintf(SQL_STRING,"%s",SQL_STRING_BASE);

  /* set to -1 to avoid searching based on this */
  if (pMinPlace->iPlaceMajorType >= 0)
  {
    AddCriteria(SQL_STRING,StatementAdditions,SQL_STRING_OPT[Local_szMATY]);
    SQLParamsBindArray[szBASE+Local_szMATY].UseField=1;
    StatementAdditions++;
  }

  /* set to -1 to avoid searching based on this */
  if (pMinPlace->iPlaceMinorType >= 0)
  {
    AddCriteria(SQL_STRING,StatementAdditions,SQL_STRING_OPT[Local_szMITY]);
    SQLParamsBindArray[szBASE+Local_szMITY].UseField=1;
    StatementAdditions++;
  }


  if (pMinPlace->dLat != 0.0)
  {
    sprintf(Local_szMinLat,"%0.2f", pMinPlace->dLat);
    AddCriteria(SQL_STRING,StatementAdditions,SQL_STRING_OPT[szLATN]);
    SQLParamsBindArray[szBASE+szLATN].UseField=1;
    StatementAdditions++;
  }

  if(pMaxPlace->dLat != 0.0)
  {
    sprintf(Local_szMaxLat,"%0.2f",pMaxPlace->dLat);
    AddCriteria(SQL_STRING,StatementAdditions,SQL_STRING_OPT[szLATX]);
    SQLParamsBindArray[szBASE+szLATX].UseField=1;
    StatementAdditions++;
  }

  if(pMinPlace->dLon != 0.0)
  {
    sprintf(Local_szMinLon,"%0.2f",pMinPlace->dLon);
    AddCriteria(SQL_STRING,StatementAdditions,SQL_STRING_OPT[szLONN]);
    SQLParamsBindArray[szBASE+szLONN].UseField=1;
    StatementAdditions++;
  }

  if(pMaxPlace->dLon != 0.0)
  {
    sprintf(Local_szMaxLon,"%0.2f",pMaxPlace->dLon);
    AddCriteria(SQL_STRING,StatementAdditions,SQL_STRING_OPT[szLONX]);
    SQLParamsBindArray[szBASE+szLONX].UseField=1;
    StatementAdditions++;
  }

  if(pMinPlace->dElev != 0.0)
  {
    sprintf(Local_szMinElev,"%0.2f",pMinPlace->dElev);
    AddCriteria(SQL_STRING,StatementAdditions,SQL_STRING_OPT[szELEN]);
    SQLParamsBindArray[szBASE+szELEN].UseField=1;
    StatementAdditions++;
  }

  if(pMaxPlace->dElev != 0.0)
  {
    sprintf(Local_szMaxElev,"%0.2f",pMaxPlace->dElev);
    AddCriteria(SQL_STRING,StatementAdditions,SQL_STRING_OPT[szELEX]);
    SQLParamsBindArray[szBASE+szELEX].UseField=1;
    StatementAdditions++;
  }

  /* Set to -1 to avoid searching according to this */
  if (pMinPlace->iPopulation >= 0)
  {
    minPopulation = pMinPlace->iPopulation;
    AddCriteria(SQL_STRING,StatementAdditions,SQL_STRING_OPT[szPOPN]);
    SQLParamsBindArray[szBASE+szPOPN].UseField=1;
    StatementAdditions++;
  }

  /* Set to -1 to avoid searching according to this */
  if(pMaxPlace->iPopulation >= 0)
  {
    maxPopulation = pMaxPlace->iPopulation;
    AddCriteria(SQL_STRING,StatementAdditions,SQL_STRING_OPT[szPOPX]);
    SQLParamsBindArray[szBASE+szPOPX].UseField=1;
    StatementAdditions++;
  }

  if(strlen (pMinPlace->szState) > 0)
  {
    AddCriteria(SQL_STRING,StatementAdditions,SQL_STRING_OPT[szSTAT]);
    SQLParamsBindArray[szBASE+szSTAT].UseField=1;
    StatementAdditions++;
  }

  if(strlen (pMinPlace->szCountry) > 0)
  {
    AddCriteria(SQL_STRING,StatementAdditions,SQL_STRING_OPT[szCTRY]);
    SQLParamsBindArray[szBASE+szCTRY].UseField=1;
    StatementAdditions++;
  }

  if(strlen (pMinPlace->szCounty) > 0)
  {
    AddCriteria(SQL_STRING,StatementAdditions,SQL_STRING_OPT[szCOTY]);
    SQLParamsBindArray[szBASE+szCOTY].UseField=1;
    StatementAdditions++;
  }

  if(strlen (pMinPlace->szPlaceName) > 0)
  {
    AddCriteria(SQL_STRING,StatementAdditions,SQL_STRING_OPT[szNAME]);
    SQLParamsBindArray[szBASE+szNAME].UseField=1;
    StatementAdditions++;
  }

  /* Set the query to order by Origin Time */
  strcat(SQL_STRING," ORDER BY iPopulation DESC");

  if(InitGetPlaceListStatement(SQL_STRING, &SSStatement) != EWDB_RETURN_SUCCESS)
  {
    *ppCursor = NULL;
    return(EWDB_RETURN_FAILURE);
  }

  *ppCursor=SSStatement.pCda;

  return(EWDB_RETURN_SUCCESS);
}  /* end PrepGetPlaceListExec() */


static int PostGetPlaceListExec(EWDB_PlaceStruct * pBuffer, int BufferRecLen)
{
    /* 
    The statement has been executed.  we need to do the following:
    1.  While we haven't fetched all of the records or reached capacity,
        fetch a new bunch of records into the buffer.
    2.  For each bunch:  Format each row into EWDB_EventListStruct format, 
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
  int BCurr,UCurr;
  EWDB_OCIStatementStruct * pSS=&SSStatement;
  char * pTemp;
  int RowsProcessed;


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
        ewdb_base_ErrorReport(hEWDBC, pCursor,"PostGetPlaceListExec:ewdb_base_SQLFetchRows",1);
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

        pBuffer[UCurr].idPlace=* (int *)((sizeof(int)*BCurr) + (int)(pSS->FieldArray[0].pVal));

        pBuffer[UCurr].iPlaceMajorType=* (int *)((sizeof(int)*BCurr) + (int)(pSS->FieldArray[1].pVal));
        
        pBuffer[UCurr].iPlaceMinorType=* (int *)((sizeof(int)*BCurr) + (int)(pSS->FieldArray[2].pVal));

        pTemp=(char *) ((pSS->FieldArray[3].Ind*BCurr) + (int)(pSS->FieldArray[3].pVal) );
        pTemp[pSS->FieldArray[3].pRetLens[BCurr]]=0;
        pBuffer[UCurr].dLat=(float)atof(pTemp);

        pTemp=(char *) ((pSS->FieldArray[4].Ind*BCurr) + (int)(pSS->FieldArray[4].pVal) );
        pTemp[pSS->FieldArray[4].pRetLens[BCurr]]=0;
        pBuffer[UCurr].dLon=(float)atof(pTemp);

        pTemp=(char *) ((pSS->FieldArray[5].Ind*BCurr) + (int)(pSS->FieldArray[5].pVal) );
        pTemp[pSS->FieldArray[5].pRetLens[BCurr]]=0;
        pBuffer[UCurr].dElev=(float)atof(pTemp);

        pBuffer[UCurr].iPopulation=* (int *)((sizeof(int)*BCurr) + (int)(pSS->FieldArray[6].pVal));

        pTemp=(char *) ((pSS->FieldArray[7].Ind*BCurr) + (int)(pSS->FieldArray[7].pVal));
        pTemp[pSS->FieldArray[7].pRetLens[BCurr]]=0;
        strcpy(pBuffer[UCurr].szPlaceName,pTemp);

        pTemp=(char *) ((pSS->FieldArray[8].Ind*BCurr) + (int)(pSS->FieldArray[8].pVal));
        pTemp[pSS->FieldArray[8].pRetLens[BCurr]]=0;
        strcpy(pBuffer[UCurr].szState,pTemp);

        pTemp=(char *) ((pSS->FieldArray[9].Ind*BCurr) + (int)(pSS->FieldArray[9].pVal));
        pTemp[pSS->FieldArray[9].pRetLens[BCurr]]=0;
        strcpy(pBuffer[UCurr].szCounty,pTemp);

        pTemp=(char *) ((pSS->FieldArray[10].Ind*BCurr) + (int)(pSS->FieldArray[10].pVal));
        pTemp[pSS->FieldArray[10].pRetLens[BCurr]]=0;
        strcpy(pBuffer[UCurr].szCountry,pTemp);

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
      ewdb_base_ErrorReport(hEWDBC, pCursor,"PostGetPlaceListExec:ewdb_base_SQLFetchRows",2);
      return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
    }
    /* else */
  }  /* End if (RowsRetrieved > BufferRecLen) */
  

  RowsProcessed = ewdb_base_GetCursorRowsProcessedCount(pCursor);

  ewdb_base_ReleaseCursor(pCursor);

  return(RowsProcessed);
}  /* end PostGetPlaceListExec() */
