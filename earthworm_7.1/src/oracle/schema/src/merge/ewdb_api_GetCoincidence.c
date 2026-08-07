/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_api_GetCoincidence.c,v 1.3 2003/10/09 17:57:02 davidk Exp $
 *    Revision history:
 *
 *    $Log: ewdb_api_GetCoincidence.c,v $
 *    Revision 1.3  2003/10/09 17:57:02  davidk
 *    Cleanup of API call, to match standard list-retrieval format.
 *
 *    Revision 1.2  2002/05/28 17:22:16  lucky
 *    *** empty log message ***
 *
 *    Revision 1.1  2002/03/22 20:01:23  lucky
 *    Initial revision
 *
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
 "select unique idEvent, sSource, sHumanReadable, "
 "idCoincidence, tCoincidence "
 "from ALL_COINCIDENCE_EVENT_INFO";

#define szWHERE " WHERE "
#define szAND   " AND "

#define szBASE 5
#define szSTTM 0
#define szENTM 1
#define szIDEV 2
#define szEVTP 3

/* Total number of fields, currently BASE + VLMD + 1 */
#define NUM_FIELDS 9


static char SQL_STRING_OPT[][30] = 
    {
      "tCoincidence>=:starttime",
      "tCoincidence<=:endtime",
      "idEvent=:idevent",
      "tiEventType=:eventtype"
    };


static EWDB_OCI_SFS SQLParamsBindArray[] = 
{
  {0,1,0,0,0,OA_EWDBID,"1idEvent"},
  {0,1,256,0,0,OA_SZ,"2sSource"},
  {0,1,256,0,0,OA_SZ,"3sHumanReadable"},
  {0,1,0,0,0,OA_EWDBID,"4idCoincidence"},
  {0,1,20,0,0,OA_DOUBLE,"5tCoincidence"},
  {0,0,0,0,0,OA_INT,":starttime"},
  {0,0,0,0,0,OA_INT,":endtime"},
  {0,0,0,0,0,OA_EWDBID,":idevent"},
  {0,0,0,0,0,OA_INT,":eventtype"},
};

/* Insertion Struct for GetCoincList statement */
static EWDB_OCIStatementStruct SSStatement;

/* Temporary Storage Buffer params */
#define BUFFERSIZE 8192
static int    iRecordSize;
static int    iRecordsPerBuffer;
static char * pLocalBuffer=NULL;

static int    bInitialized=FALSE;

static  int     	iStartTime;
static  int     	iEndTime;
static  int     	iEventType;
static  EWDBid    idEventID;

/********************************
      FUNCTION PROTOTYPES
********************************/
int PrepGetCoincListExec(EWDBid idEvent, double StartTime, double EndTime, EWDB_Cursor * ppCursor);
int PostGetCoincListExec(EWDB_CoincEventStruct * pBuffer, int BufferRecLen);
static char *AddCriteria(char * MainString, int NumAdditions, 
                          char * pCriteria);
int InitGetCoincListStatement(char * szStatement, 
                              EWDB_OCIStatementStruct *pSS);


/*********************************************************************** 
   Used to get the list of coincidence events from the database. 
     
	idEvent:if > 0, retrieve the information for this particular event; 
            otherwise get a list between StartTime and EndTime.
	EvtType:if > 0, retrieve coincidences associated with this
            event type only, otherwise get all coincidences
	pCoinc: pointer to a buffer allocated by the caller and used
            to store the array of events.
	NumAlloc: Allocated length of pCoinc
	StartTime: Start search time. To use unlimited start time, set to -1.
	EndTime: End search time. To use unlimited end time, set to -1.
	pNumFound: This many events are in the database.
	pNumRetr: This many events fit in the pCoinc buffer.

***********************************************************************/
int ewdb_api_GetCoincidence (EWDBid idEvent, int EvtType, EWDB_CoincEventStruct *pCoinc, 
							int NumAlloc, double StartTime, double EndTime, 
							int *pNumFound, int *pNumRetr)
{

  EWDB_Cursor  pCursor;


	if (pCoinc == NULL)
	{
		logit ("", "Coincidence buffer must be pre-allocated.\n");
     	return( EWDB_RETURN_FAILURE );
	}
		

	iEventType = EvtType;
  
  ewdb_base_SetLastOraAPIActionTime();

  if( ewdb_base_Reconnect() != EWDB_RETURN_SUCCESS )
  {
		logit ("", "ewdb_api_GetCoincidence(): ERROR Call to ewdb_base_Reconnect failed.\n");
     	return( EWDB_RETURN_FAILURE );
  }

  if( PrepGetCoincListExec (idEvent, StartTime, EndTime, &pCursor)
      != EWDB_RETURN_SUCCESS )
  {
    logit("","ORA_API:GetCoincList():PrepGetCoincListExec() failed.\n");
    return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }
  
  if( ewdb_base_SQLExecute(pCursor) )
  {
    ewdb_base_ErrorReport(hEWDBC, pCursor,"GetCoincList:ewdb_base_SQLExecute",1);
    return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  } 

  
  /* Commit the transaction (all the previous inserts!)
     In Case there is any auditing or logging or debug
     changes made in the stored procedures.
  ****************************************************/
  if (ewdb_base_SQLCommit(hEWDBC))
  {
    ewdb_base_ErrorReport(hEWDBC, hEWDBC,"GetCoincList:ewdb_base_SQLCommit",2);
    return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE) );
  }

  if ((*pNumFound = PostGetCoincListExec ((EWDB_CoincEventStruct *) pCoinc, NumAlloc)) 
      == EWDB_RETURN_FAILURE )
  {
    return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }

  ewdb_base_SetLastOraAPIActionTime();
  if (*pNumFound <= NumAlloc)
  {
    *pNumRetr = *pNumFound;
    return (EWDB_RETURN_SUCCESS);
  }
  else
  {
    *pNumRetr = NumAlloc;
    return (EWDB_RETURN_WARNING);
  }
} 



int InitGetCoincListStatement(char * szStatement, EWDB_OCIStatementStruct *pSS)
{

  int LastSize,i;  /* Size of last column array */


  if(!pLocalBuffer)
  {
    /*Allocate space for returned data - never freed */
    pLocalBuffer=malloc(BUFFERSIZE);


    iRecordSize=0;
    iRecordSize += sizeof(EWDBid);         /*idEvent*/
    iRecordSize += pSS->FieldArray[1].Ind; /*sSource*/
    iRecordSize += pSS->FieldArray[2].Ind; /*sHumanReadable*/
    iRecordSize += sizeof(EWDBid);         /*idCoincidence*/
    iRecordSize += pSS->FieldArray[4].Ind; /*tCoincidence*/

    /* Make the number of records a multiple of 8 to avoid alignment problems on UNIX */
    iRecordsPerBuffer= (BUFFERSIZE/iRecordSize) & 0xfffffff8;
    
    /*Allocate space for row/col ret lens. - never freed */
    for(i=0;i<pSS->NumOfFields;i++)
    {
      pSS->FieldArray[i].pRetLens=malloc(iRecordsPerBuffer*EWDB_FIELD_RET_LEN);
    }


    pSS->FieldArray[0].pVal=pLocalBuffer;
    LastSize=sizeof(EWDBid); /* idEvent */
    
    pSS->FieldArray[1].pVal= (void *) ( 
      (int)(pSS->FieldArray[0].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[1].Ind; /* char sSource[]*/
    
    pSS->FieldArray[2].pVal= (void *) (
      (int)(pSS->FieldArray[1].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[2].Ind; /* char sHumanReadable[]*/
    
    pSS->FieldArray[3].pVal= (void *) (
      (int)(pSS->FieldArray[2].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=sizeof(int); /* idCoincidence */
    
    pSS->FieldArray[4].pVal= (void *) (
      (int)(pSS->FieldArray[3].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[4].Ind;  /* double tCoincidence */
    
    pSS->FieldArray[5].pVal = &(iStartTime);
    pSS->FieldArray[6].pVal = &(iEndTime);
    pSS->FieldArray[7].pVal = &(idEventID);
    pSS->FieldArray[8].pVal = &(iEventType);

  } /* end if(!pLocalBuffer) */
  
  if(!pLocalBuffer)
    logit("","InitGetCoincListStatement: malloc of pLocalBuffer "
          "failed! Returning.\n");


  ewdb_base_RequestCursor(szStatement, pSS,0/*don't force rebind*/);

  return(EWDB_RETURN_SUCCESS);
}  /* end InitGetCoincListStatement() */

        

static char * AddCriteria (char *MainString, int NumAdditions, char *pCriteria)
{
  if(NumAdditions == 0)
    strcat(MainString,szWHERE);
  else
    strcat(MainString,szAND);
  return (strcat (MainString, pCriteria));
}


int PrepGetCoincListExec(EWDBid idEvent, double StartTime, double EndTime, 
                         EWDB_Cursor * ppCursor)
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
  SQLParamsBindArray[szBASE+szIDEV].UseField = FALSE;
  SQLParamsBindArray[szBASE+szEVTP].UseField = FALSE;


  iStartTime = (time_t) StartTime;
  iEndTime = (time_t) EndTime;
  idEventID = idEvent;

  /* Create the base string */
  sprintf(SQL_STRING,"%s",SQL_STRING_BASE);


  if (iEventType > 0)
  {
    AddCriteria (SQL_STRING, StatementAdditions, SQL_STRING_OPT[szEVTP]);
    SQLParamsBindArray[szBASE+szEVTP].UseField = 1;
    StatementAdditions++;
  }


  if (idEvent > 0)
  {
    AddCriteria (SQL_STRING, StatementAdditions, SQL_STRING_OPT[szIDEV]);
    SQLParamsBindArray[szBASE+szIDEV].UseField = 1;
    StatementAdditions++;
  }
  else
  {
     if (iStartTime >= 0)
     {
       AddCriteria (SQL_STRING,StatementAdditions, SQL_STRING_OPT[szSTTM]);
       SQLParamsBindArray[szBASE+szSTTM].UseField = 1;
       StatementAdditions++;
     }

     if (iEndTime >= 0)
     {
       AddCriteria (SQL_STRING,StatementAdditions, SQL_STRING_OPT[szENTM]);
       SQLParamsBindArray[szBASE+szENTM].UseField = 1;
       StatementAdditions++;
     }
     /* Set the query to order by Coincidence Time */
     strcat (SQL_STRING," ORDER BY tCoincidence DESC");
  }

  InitGetCoincListStatement (SQL_STRING, &SSStatement);

  *ppCursor=SSStatement.pCda;

  return (EWDB_RETURN_SUCCESS);

}  

int PostGetCoincListExec(EWDB_CoincEventStruct *pBuffer, int BufferRecLen)
{
    /* 
    The statement has been executed.  we need to do the following:
    1.  While we haven't fetched all of the records or reached capacity,
        fetch a new bunch of records into the buffer.
    2.  For each bunch:  Format each row into EWDB_CoincEventStruct format, 
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
    if (ewdb_base_SQLFetchRows(pCursor, iRecordsPerBuffer))
    {
      if (ewdb_base_GetCursorRetCode(pCursor) == EWDB_SQL_ERROR_NO_DATA) 
      {
        done=1;
      }
      else
      {
        ewdb_base_ErrorReport(hEWDBC, pCursor,"PostGetCoincListExec:ewdb_base_SQLFetchRows",1);
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

        pTemp=(char *) ((pSS->FieldArray[1].Ind*BCurr) + 
										(int)(pSS->FieldArray[1].pVal));
        pTemp[pSS->FieldArray[1].pRetLens[BCurr]]=0;
        strcpy(pBuffer[UCurr].szSource,pTemp);

        pTemp=(char *) ((pSS->FieldArray[2].Ind*BCurr) + 	
										(int)(pSS->FieldArray[2].pVal));
        pTemp[pSS->FieldArray[2].pRetLens[BCurr]]=0;
        strcpy(pBuffer[UCurr].szHumanReadable,pTemp);
        
        pBuffer[UCurr].idCoincidence=* (int *)((sizeof(int)*BCurr) + 
										(int)(pSS->FieldArray[3].pVal));

        pTemp=(char *) ((pSS->FieldArray[4].Ind*BCurr) + (int)(pSS->FieldArray[4].pVal) );
        pTemp[pSS->FieldArray[4].pRetLens[BCurr]]=0;
        pBuffer[UCurr].tCoincidence = atof(pTemp);

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
      ewdb_base_ErrorReport(hEWDBC, pCursor,
                       "PostGetCoincListExec:ewdb_base_SQLFetchRows",2);
      return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
    }
    /* else */
  }  /* End if(RowsRetrieved > BufferRecLen) */
  
  RowsProcessed = ewdb_base_GetCursorRowsProcessedCount(pCursor);

  ewdb_base_ReleaseCursor(pCursor);

  return(RowsProcessed);
}  /* end PostGetCoincListExec() */
