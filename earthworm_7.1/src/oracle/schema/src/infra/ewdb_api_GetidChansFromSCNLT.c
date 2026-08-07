/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_api_GetidChansFromSCNLT.c,v 1.10 2005/06/10 16:27:59 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_api_GetidChansFromSCNLT.c,v $
 *     Revision 1.10  2005/06/10 16:27:59  davidk
 *     DB API Cleanup
 *
 *     Revision 1.9  2003/09/16 17:00:14  davidk
 *     General API Cleanup of internal structures.
 *
 *     Revision 1.8  2003/08/21 00:37:22  davidk
 *     Restructured API code that deals with dynamic allocation of memory.
 *
 *     Revision 1.7  2001/07/26 21:50:26  davidk
 *     Removed NumIDChansReceived variable to get rid of compiler warnings on NT.
 *
 *     Revision 1.6  2001/07/16 20:31:17  davidk
 *     Fixed two bugs:
 *     1) In InitXXX() the order of the "pointer to value" assignments was
 *        screwed up so that the times and the Comp/Net values were swapped.
 *     2) In ewdb_api_GetidChansFromSCNLT(), the result of PostXXX(), which
 *        is the number of rows found, was written to the wrong variable, which
 *        was never read.  So even though the function was properly retrieving
 *        the data, it was always claiming that it didn't find anything to retrieve.
 *
 *     Revision 1.5  2001/07/16 18:23:14  davidk
 *     Made Sta a wildcardable parameter, so that you can now use wildcards for
 *     all four potential component codes.
 *     API cleanup.  Generally cleaned up the code (names, statics), and improved
 *     the logging.
 *
 *     Revision 1.4  2001/05/25 23:55:31  davidk
 *     Finally plugged the memory leak that was supposively fixed in 1.3.
 *     (We now free memory at the end of the call, that is allocated at
 *     the beginning to hold the return lenghts of the columns from the
 *     SQL query).
 *
 *     Revision 1.3  2001/05/16 23:37:52  davidk
 *     plugged a memory leak caused by temporary allocating storage for column return
 *     lengths for each call, and then never freeing them.
 *
 *     Revision 1.2  2001/05/15 02:16:30  davidk
 *     Moved functions around between the apps, DB API, and DB API INTERNAL
 *     levels.  Renamed functions and files.  Added support for amplitude
 *     magnitude types.  Reformatted makefiles.
 *
 *     Revision 1.1  2001/02/28 17:19:22  lucky
 *     Initial revision
 *
 *     Revision 1.10  2001/02/21 10:56:44  davidk
 *     Code was retrofitted to use the new db_cli_base API
 *     in lieu of the oci_base() and OCI7 function API's.layer, which
 *     provides abstraction from the OCI7 function calls to
 *     ora_api layer code.  See comments in
 *     schema/src/include/internal/ewdb_cli_base.h for more info
 *     on the details of the changes made.
 *
 *     Added code to set sLoc to a blank string if the Local_szLoc parameter is a NULL pointer,
 *     in function PrepGetidChansFromSCNLTExec().
 *
 *     Revision 1.9  2001/01/23 21:31:39  lucky
 *     Added a check for NULL Local_szLoc in PrepGetidChansFromSCNLTExec --
 *     otherwise this call fails if Loc is not specified.
 *
 *     Revision 1.8  2001/01/19 18:08:11  davidk
 *     added support for wildcards in Comp, Net, and Loc names.   Wildcards are
 *     not supported for station names.
 *
 *     Revision 1.7  2000/06/21 22:52:19  lucky
 *      Cleaned up logit calls to make log files more readable.
 *
 *     Revision 1.6  2000/05/15 22:31:06  davidk
 *     Added prototypes for the functions implemented in this file.
 *     Reformatted whitespace(put spaces in inplace of tabs).  Added
 *     some additional comments, and changed informational logits
 *     to only print when the EWDB_Debug flag is on.  Stopped all logits
 *     from printing to stderr, so as to be compatible with CGI-BIN web
 *     apps.
 *
 *     Revision 1.5  2000/02/10 21:22:19  davidk
 *     added code to the SQL query to handle NULL sLoc codes,
 *     changed the tOff and tOn values to have 4 digits of precision
 *     after the decimal point instead of two.
 *
 *     Revision 1.3  1999/11/09 18:46:12  lucky
 *     *** empty log message ***
 *
 *
 */


#include <ewdb_ora_api.h>
#include <ewdb_cli_base.h>
#include <ewdb_ew_oci_base.h>

static char SQL_STRING_BASE[] =
/* To get all channels from a certain message */
  "select distinct idChan "
  " from ALL_STATION_INFO ";

static char SQL_STRING[500];

static char SQL_STRING_OPT[][80] = 
{
  " where sSta =   :IN_sSta ",
  " where sSta IS  NOT NULL ",
  "   and sComp =  :IN_sComp ",
  "   and sNet =   :IN_sNet ",
  "   and(sLoc =  :IN_sLoc or(sLoc IS NULL and :IN_sLoc IS NULL)) ",
  "   and tOn <=   :IN_tOff  and tOff >=  :IN_tOn "
};

static EWDB_OCI_SFS SQLParamsBindArray[] = 
{
  {0,1,0,0,0,OA_INT,"1idChan"},
  {0,0,0,0,0,OA_SZ,":IN_sSta"},
  {0,0,0,0,0,OA_SZ,":IN_sComp"},
  {0,0,0,0,0,OA_SZ,":IN_sNet"},
  {0,0,0,0,0,OA_SZ,":IN_sLoc"},
  {0,1,0,0,0,OA_DOUBLE,":IN_tOff"},
  {0,1,0,0,0,OA_DOUBLE,":IN_tOn"},

};

/* define the total number of fields to be bound */
#define NUM_BASE 1
#define NUM_FIELDS 7

/* define option field offsets */
#define OPT_STA 0
#define OPT_STA_WILD 1
#define OPT_COMP 2
#define OPT_NET  3
#define OPT_LOC  4
#define OPT_TIME 5

/* define BindArrayOffsets for optional params */
#define BAOFFSET_STA  1
#define BAOFFSET_COMP 2
#define BAOFFSET_NET  3
#define BAOFFSET_LOC  4

/* define the size of the local buffer */
#define STRSIZE 15
#define DOUBLESIZE 20


/* Insertion Struct for GetidChansFromSCNLT statement */
static EWDB_OCIStatementStruct SSStatement;

/* Temporary Storage Buffer params */
#define BUFFERSIZE 8192
static int    iRecordSize;
static int    iRecordsPerBuffer;
static char * pLocalBuffer=NULL;

static int    bInitialized=FALSE;


static char Local_szSta[STRSIZE], Local_szComp[STRSIZE],
            Local_szNet[STRSIZE], Local_szLoc[STRSIZE],
            Local_sztOff[DOUBLESIZE], Local_sztOn[DOUBLESIZE];

/********************************
      FUNCTION PROTOTYPES
********************************/
static int PrepGetidChansFromSCNLTExec(char * IN_szSta, char * IN_szComp, char * IN_szNet,
                                       char * IN_szLoc, double IN_tOn, double IN_tOff,
                                       EWDB_Cursor * ppCursor);
static int PostGetidChansFromSCNLTExec(EWDBid * pBuffer, int BufferRecLen);
static int InitGetidChansFromSCNLTStatement(char * szStatement, 
                                            EWDB_OCIStatementStruct *pSS);


int ewdb_api_GetidChansFromSCNLT(EWDBid * pBuffer,  
                                 char * IN_szSta, char * IN_szComp, 
                                 char * IN_szNet, char * IN_szLoc,
                                 double IN_tOff, double IN_tOn, 
                                 int * pNumChansFound, 
                                 int * pNumChansRetrieved,
                                 int BufferLen)
{

  EWDB_Cursor  pCursor;
 
  ewdb_base_SetLastOraAPIActionTime();

  if(ewdb_base_Reconnect() != EWDB_RETURN_SUCCESS)
  /* Re-establishes connection. */
  {
    logit("", "ewdb_api_GetidChansFromSCNLT(): "
          "Could not reconnect to the database!\n");
    return(EWDB_RETURN_FAILURE);
  }

  if(PrepGetidChansFromSCNLTExec(IN_szSta, IN_szComp, IN_szNet, IN_szLoc,
                                 IN_tOn, IN_tOff, &pCursor)
     != EWDB_RETURN_SUCCESS)
  {
    return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }

  if( ewdb_base_SQLExecute(pCursor) )
  {
    ewdb_base_ErrorReport(hEWDBC, pCursor,
                          "ewdb_api_GetidChansFromSCNLT(): ewdb_base_SQLExecute",1);
    return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  } 

  /* Commit the transaction (all the previous inserts!)
  In Case there is any auditing or logging or debug
  changes made in the stored procedures.
  ****************************************************/
  if (ewdb_base_SQLCommit (hEWDBC))
  {
    ewdb_base_ErrorReport (hEWDBC, hEWDBC,"ewdb_api_GetidChansFromSCNLT(): ewdb_base_SQLCommit",2);
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  }
  
  if((*pNumChansFound=PostGetidChansFromSCNLTExec(pBuffer,BufferLen))
     == EWDB_RETURN_FAILURE)
  {
    return(ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }

  ewdb_base_SetLastOraAPIActionTime();

  if(*pNumChansFound <= BufferLen)
  {
    *pNumChansRetrieved = *pNumChansFound;
    return(EWDB_RETURN_SUCCESS);
  }
  else
  {
    *pNumChansRetrieved = BufferLen;
    return(EWDB_RETURN_WARNING);
  }
}  /* end ewdb_api_GetidChansFromSCNLT() */


static int InitGetidChansFromSCNLTStatement(char * szStatement, 
                                            EWDB_OCIStatementStruct *pSS)
{
  int LastSize,i;  /* Size of last column array */

  if(!pLocalBuffer)
  {
    /*Allocate space for returned data - never freed */
    pLocalBuffer=malloc(BUFFERSIZE);

    iRecordSize=0;
    iRecordSize += sizeof(EWDBid); /*idChan*/

    /* Make the number of records a multiple of 8 to avoid alignment problems on UNIX */
    iRecordsPerBuffer= (BUFFERSIZE/iRecordSize) & 0xfffffff8;
    
    /*Allocate space for row/col ret lens. - never freed */
    for(i=0;i<pSS->NumOfFields;i++)
    {
      pSS->FieldArray[i].pRetLens=malloc(iRecordsPerBuffer*EWDB_FIELD_RET_LEN);
    }

    pSS->FieldArray[0].pVal=&(pLocalBuffer[0]);
    LastSize=sizeof(int);
    
    pSS->FieldArray[1].pVal = Local_szSta;
    pSS->FieldArray[2].pVal = Local_szComp;
    pSS->FieldArray[3].pVal = Local_szNet;
    pSS->FieldArray[4].pVal = Local_szLoc;
    pSS->FieldArray[5].pVal = Local_sztOff;
    pSS->FieldArray[6].pVal = Local_sztOn;

  } /* end if(!pLocalBuffer) */
  
  if(!pLocalBuffer)
    logit("","InitGetidChansFromSCNLTStatement: malloc of pLocalBuffer "
          "failed! Returning.\n");

  return(ewdb_base_RequestCursor(szStatement, pSS,0/*don't force rebind*/));
}  /* End InitGetidChansFromSCNLTStatement() */


static int PrepGetidChansFromSCNLTExec(char * IN_szSta, char * IN_szComp, char * IN_szNet,
                                       char * IN_szLoc, double IN_tOn, double IN_tOff,
                                       EWDB_Cursor * ppCursor)
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

  sprintf(Local_sztOff,"%.4f",IN_tOff);
  sprintf(Local_sztOn,"%.4f",IN_tOn);

  strncpy(Local_szSta,IN_szSta,STRSIZE);
  Local_szSta[STRSIZE-1] = 0;

  strncpy(Local_szComp,IN_szComp,STRSIZE);
  Local_szComp[STRSIZE-1] = 0;

  strncpy(Local_szNet,IN_szNet,STRSIZE);
  Local_szNet[STRSIZE-1] = 0;

  if(IN_szLoc != NULL)
  {
    strncpy(Local_szLoc,IN_szLoc,STRSIZE);
    Local_szLoc[STRSIZE-1] = 0;
  }
  else  /* added by DK 02/21/01 */
  {
    /* set to blank string if pointer was NULL */
    Local_szLoc[0] = 0;
  }

  /* Initialize the SQL string, and number of SQL vars to the base values */
  strcpy(SQL_STRING, SQL_STRING_BASE);

    /* Add Where and station criteria, check for wildcard */
  if(strcmp(Local_szSta,"*"))
  {
    SQLParamsBindArray[BAOFFSET_STA].UseField = TRUE;
    strcat(SQL_STRING, SQL_STRING_OPT[OPT_STA]);
  }
  else
  {
    SQLParamsBindArray[BAOFFSET_STA].UseField = FALSE;
    strcat(SQL_STRING, SQL_STRING_OPT[OPT_STA_WILD]);
  }

  /* Add Comp if not wildcarded */
  if(strcmp(Local_szComp,"*"))
  {
    SQLParamsBindArray[BAOFFSET_COMP].UseField = TRUE;
    strcat(SQL_STRING, SQL_STRING_OPT[OPT_COMP]);
  }
  else
  {
    SQLParamsBindArray[BAOFFSET_COMP].UseField = FALSE;
  }

  /* Add Net if not wildcarded */
  if(strcmp(Local_szNet,"*"))
  {
    SQLParamsBindArray[BAOFFSET_NET].UseField = TRUE;
    strcat(SQL_STRING, SQL_STRING_OPT[OPT_NET]);
  }
  else
  {
    SQLParamsBindArray[BAOFFSET_NET].UseField = FALSE;
  }

  /* Add Loc if not wildcarded */
  if(strcmp(Local_szLoc,"*"))
  {
    SQLParamsBindArray[BAOFFSET_LOC].UseField = TRUE;
    strcat(SQL_STRING, SQL_STRING_OPT[OPT_LOC]);
  }
  else
  {
    SQLParamsBindArray[BAOFFSET_LOC].UseField = FALSE;
  }

  /* Always add time constraints at the end */
  strcat(SQL_STRING, SQL_STRING_OPT[OPT_TIME]);

  /* Call InitXXX() to bind the params, get a cursor, and prep */
  if(InitGetidChansFromSCNLTStatement(SQL_STRING, &SSStatement) != EWDB_RETURN_SUCCESS)
  {
    *ppCursor = NULL;
    return(EWDB_RETURN_FAILURE);
  }

  *ppCursor=SSStatement.pCda;

  return(EWDB_RETURN_SUCCESS);
}  /* End PrepGetidChansFromSCNLTExec() */


static int PostGetidChansFromSCNLTExec(EWDBid * pBuffer, int BufferRecLen)
{
    /* 
    The statement has been executed.  we need to do the following:
    1.  While we haven't fetched all of the records or reached capacity,
        fetch a new bunch of records into the buffer.
    2.  For each bunch:  Format each row into SnipDescStruct format, 
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
  int RowsProcessed;


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
        ewdb_base_ErrorReport(hEWDBC, pCursor,
                              "PostGetidChansFromSCNLTExec:ewdb_base_SQLFetchRows",1);
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

        pBuffer[UCurr] =  *(int *)
         ((sizeof(int)*BCurr) +(int)(pSS->FieldArray[0].pVal));
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
                            "PostGetGetidChansFromSCNLTExec:ewdb_base_SQLFetchRows",2);
      return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
    }
    /* else */
  }  /* End if(RowsRetrieved > BufferRecLen) */
  
  RowsProcessed = ewdb_base_GetCursorRowsProcessedCount(pCursor);

  ewdb_base_ReleaseCursor(pCursor);

  return(RowsProcessed);
}  /* End PostGetGetidChansFromSCNLTExec() */
