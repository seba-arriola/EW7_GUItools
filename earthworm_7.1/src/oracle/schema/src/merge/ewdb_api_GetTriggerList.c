/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_api_GetTriggerList.c,v 1.3 2003/10/09 17:57:03 davidk Exp $
 *    Revision history:
 *
 *    $Log: ewdb_api_GetTriggerList.c,v $
 *    Revision 1.3  2003/10/09 17:57:03  davidk
 *    Cleanup of API call, to match standard list-retrieval format.
 *
 *    Revision 1.2  2002/05/28 17:22:42  lucky
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
 "select unique idTrigger, idCoincidence, idChan, tTrigger "
 "from ChannelTrigger";

#define szWHERE " WHERE "
#define szAND   " AND "

#define szBASE 4
#define szSTTM 0
#define szENTM 1
#define szIDCO 2

/* Total number of fields, currently BASE + VLMD + 1 */
#define NUM_FIELDS 7


static char SQL_STRING_OPT[][30] = 
    {
      "tTrigger>=:starttime",
      "tTrigger<=:endtime",
      "idCoincidence=:idcoincidence"
    };


static EWDB_OCI_SFS SQLParamsBindArray[] = 
{
  {0,1,0,0,0,OA_EWDBID,"1idTrigger"},
  {0,1,0,0,0,OA_EWDBID,"2idCoincidence"},
  {0,1,0,0,0,OA_EWDBID,"3idChan"},
  {0,1,20,0,0,OA_DOUBLE,"4tTrigger"},
  {0,0,0,0,0,OA_INT,":starttime"},
  {0,0,0,0,0,OA_INT,":endtime"},
  {0,0,0,0,0,OA_EWDBID,":idcoincidence"},
};

/* Insertion Struct for GetTriggList statement */
static EWDB_OCIStatementStruct SSStatement;

/* Temporary Storage Buffer params */
#define BUFFERSIZE 8192
static int    iRecordSize;
static int    iRecordsPerBuffer;
static char * pLocalBuffer=NULL;

static int    bInitialized=FALSE;


static  int     iStartTime;
static  int     iEndTime;
static  EWDBid  iCoincID;


/********************************
      FUNCTION PROTOTYPES
********************************/
int PrepGetTriggerExec(EWDBid idCoincidence, double StartTime, 
						double EndTime, EWDB_Cursor * ppCursor);
int PostGetTriggerExec(EWDB_TriggerStruct * pBuffer, int BufferRecLen);
static char *AddCriteria(char * MainString, int NumAdditions, 
                          char * pCriteria);
int InitGetTriggerStatement(char * szStatement, 
                              EWDB_OCIStatementStruct *pSS);


/*********************************************************************** 
   Used to get the list of coincidence events from the database. 
     
	idCoincidence:if > 0, retrieve the information for this particular event; 
            otherwise get a list between StartTime and EndTime.
	pTrigs: pointer to a buffer allocated by the caller and used
            to store the array of triggers.
	NumAlloc: Allocated length of pTrigs
	StartTime: Start search time. To use unlimited start time, set to -1.
	EndTime: End search time. To use unlimited end time, set to -1.
	pNumFound: This many triggers are in the database.
	pNumRetr: This many triggers fit in the pTrigs buffer.

***********************************************************************/
int ewdb_api_GetTriggerList (EWDBid idCoincidence, EWDB_TriggerStruct *pTrigs, 
							int NumAlloc, double StartTime, double EndTime, 
							int *pNumFound, int *pNumRetr)
{

  EWDB_Cursor  pCursor;


	if (pTrigs == NULL)
	{
		logit ("", "ewdb_api_GetTriggerList(): ERROR Coincidence buffer must be pre-allocated.\n");
     	return( EWDB_RETURN_FAILURE );
	}
		
  /*logit ("", "GetTrigList, idCoinc is %d\n", idCoincidence);*/
  
  ewdb_base_SetLastOraAPIActionTime();

  if( ewdb_base_Reconnect() != EWDB_RETURN_SUCCESS )
  {
		logit ("", "ewdb_api_GetTriggerList(): ERROR Call to ewdb_base_Reconnect failed.\n");
     	return( EWDB_RETURN_FAILURE );
  }

  if( PrepGetTriggerExec (idCoincidence, StartTime, EndTime, &pCursor)
      != EWDB_RETURN_SUCCESS )
  {
    logit("","ewdb_api_GetTriggerList():PrepGetTriggerExec() failed.\n");
    return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }
  
  if( ewdb_base_SQLExecute(pCursor) )
  {
    ewdb_base_ErrorReport(hEWDBC, pCursor,"GetTriggsist:ewdb_base_SQLExecute",1);
    return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  } 

  
  /* Commit the transaction (all the previous inserts!)
     In Case there is any auditing or logging or debug
     changes made in the stored procedures.
  ****************************************************/
  if (ewdb_base_SQLCommit(hEWDBC))
  {
    ewdb_base_ErrorReport(hEWDBC, hEWDBC,"GetTriggsist:ewdb_base_SQLCommit",2);
    return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE) );
  }

  if ((*pNumFound = PostGetTriggerExec ((EWDB_TriggerStruct *) pTrigs, NumAlloc)) 
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


int InitGetTriggerStatement(char * szStatement, EWDB_OCIStatementStruct *pSS)
{

  int LastSize,i;  /* Size of last column array */


  if(!pLocalBuffer)
  {
    /*Allocate space for returned data - never freed */
    pLocalBuffer=malloc(BUFFERSIZE);


    iRecordSize=0;
    iRecordSize += sizeof(EWDBid);         /*idTrigger*/
    iRecordSize += sizeof(EWDBid);         /*idCoincidence*/
    iRecordSize += sizeof(EWDBid);         /*idChan*/
    iRecordSize += pSS->FieldArray[3].Ind; /*tTrigger*/

    /* Make the number of records a multiple of 8 to avoid alignment problems on UNIX */
    iRecordsPerBuffer= (BUFFERSIZE/iRecordSize) & 0xfffffff8;
    
    /*Allocate space for row/col ret lens. - never freed */
    for(i=0;i<pSS->NumOfFields;i++)
    {
      pSS->FieldArray[i].pRetLens=malloc(iRecordsPerBuffer*EWDB_FIELD_RET_LEN);
    }


    pSS->FieldArray[0].pVal=pLocalBuffer;
    LastSize=sizeof(int); /* idTrigger */
    
    pSS->FieldArray[1].pVal= (void *) ( 
      (int)(pSS->FieldArray[0].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=sizeof(int); /* idCoincidence */
    
    pSS->FieldArray[2].pVal= (void *) (
      (int)(pSS->FieldArray[1].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=sizeof(int); /* idChan */
    
    pSS->FieldArray[3].pVal= (void *) (
      (int)(pSS->FieldArray[2].pVal)+(LastSize*iRecordsPerBuffer));
    LastSize=pSS->FieldArray[3].Ind;  /* double tTrigger */
    
    pSS->FieldArray[4].pVal = &(iStartTime);
    pSS->FieldArray[5].pVal = &(iEndTime);
    pSS->FieldArray[6].pVal = &(iCoincID);

  } /* end if(!pLocalBuffer) */
  
  if(!pLocalBuffer)
    logit("","InitGetTriggerStatement(): malloc of pLocalBuffer "
          "failed! Returning.\n");


  ewdb_base_RequestCursor(szStatement, pSS,0/*don't force rebind*/);

  return(EWDB_RETURN_SUCCESS);
}  /* end InitGetTriggerStatement() */

        

static char * AddCriteria (char *MainString, int NumAdditions, char *pCriteria)
{
  if(NumAdditions == 0)
    strcat(MainString,szWHERE);
  else
    strcat(MainString,szAND);
  return (strcat (MainString, pCriteria));
}


int PrepGetTriggerExec(EWDBid idCoincidence, double StartTime, double EndTime, 
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
  SQLParamsBindArray[szBASE+szIDCO].UseField = FALSE;


  iStartTime = (time_t) StartTime;
  iEndTime   =   (time_t) EndTime;
  iCoincID   = idCoincidence;

  /* Create the base string */
  sprintf(SQL_STRING,"%s",SQL_STRING_BASE);


  if (idCoincidence > 0)
  {
    AddCriteria (SQL_STRING, StatementAdditions, SQL_STRING_OPT[szIDCO]);
    SQLParamsBindArray[szBASE+szIDCO].UseField = 1;
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
     strcat (SQL_STRING," ORDER BY tTrigger DESC");
  }

  InitGetTriggerStatement (SQL_STRING, &SSStatement);

  *ppCursor=SSStatement.pCda;

  return (EWDB_RETURN_SUCCESS);

}  

int PostGetTriggerExec(EWDB_TriggerStruct *pBuffer, int BufferRecLen)
{
    /* 
    The statement has been executed.  we need to do the following:
    1.  While we haven't fetched all of the records or reached capacity,
        fetch a new bunch of records into the buffer.
    2.  For each bunch:  Format each row into EWDB_TriggerStruct format, 
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
        ewdb_base_ErrorReport(hEWDBC, pCursor,"PostGetTriggerExec:ewdb_base_SQLFetchRows",1);
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
        pBuffer[UCurr].idTrigger=* (int *)((sizeof(int)*BCurr) + 
										(int)(pSS->FieldArray[0].pVal));

        pBuffer[UCurr].idCoincidence=* (int *)((sizeof(int)*BCurr) + 
										(int)(pSS->FieldArray[1].pVal));

        pBuffer[UCurr].idChan=* (int *)((sizeof(int)*BCurr) + 
										(int)(pSS->FieldArray[2].pVal));

        pTemp=(char *) ((pSS->FieldArray[3].Ind*BCurr) + 
									(int)(pSS->FieldArray[3].pVal) );
        pTemp[pSS->FieldArray[3].pRetLens[BCurr]]=0;
        pBuffer[UCurr].tTrigger = atof(pTemp);

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
                       "PostGetTriggerExec():ewdb_base_SQLFetchRows",2);
      return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
    }
    /* else */
  }  /* End if(RowsRetrieved > BufferRecLen) */
  
  RowsProcessed = ewdb_base_GetCursorRowsProcessedCount(pCursor);

  ewdb_base_ReleaseCursor(pCursor);

  return(RowsProcessed);
}  /* end PostGetTriggerExec() */
