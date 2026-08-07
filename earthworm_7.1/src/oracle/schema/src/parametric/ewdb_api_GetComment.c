/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_api_GetComment.c,v 1.4 2005/06/15 19:03:12 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_api_GetComment.c,v $
 *     Revision 1.4  2005/06/15 19:03:12  davidk
 *     DB API Cleanup
 *
 *     Revision 1.3  2005/05/16 17:06:09  mark
 *     Fixed function to actually work
 *
 *     Revision 1.1  2004/12/07 23:30:08  mark
 *     Initial checkin
 *
 *
 */

#include <ewdb_cli_base.h>
#include <ewdb_ew_oci_base.h>
#include <ewdb_ora_api.h>
#include <ewdb_oci_base_internal.h>


static char SQL_STRING[] =
        "select sComment, idNextComment "
		    " from ALL_COMMENT_INFO "
        " where idComment = :IN_idComment";

static EWDB_OCI_SFS SQLParamsBindArray[] =

{
  {0,1,4000,0,0,OA_SZ,   "1sComment"},
  {0,1,0,0,0,OA_INT,     "2idNextComment"},
  {0,1,0,0,0,OA_INT,     ":IN_idComment"},
};

/* define the total number of fields to be bound */
#define NUM_FIELDS 			3


/* (SQL) Statement Struct for our statement */
static EWDB_OCIStatementStruct SSStatement;

/* Temporary Storage Buffer params */
#define BUFFERSIZE 32768
static int    iRecordSize;
static int    iRecordsPerBuffer;
static char * pLocalBuffer=NULL;

static  int   Local_idComment;

/********************************
      FUNCTION PROTOTYPES
********************************/
static int PrepGetCommentRecExec(EWDB_Cursor *ppCursor);
static int InitGetCommentRecStatement(char *, EWDB_OCIStatementStruct *);
static int PostGetCommentRecExec (char *, EWDBid *);
/*******************************/

int ewdb_api_GetComment(EWDBid idComment, char *szComment, EWDBid *idNextComment)
{
	EWDB_Cursor pCursor;
	int retval;

	if ((idComment <= 0) || (szComment == NULL) || (idNextComment == NULL))
	{
		logit ("", "ewdb_api_GetComment(): ERROR Invalid arguments passed in.\n");
		return EWDB_RETURN_FAILURE;
	}

	ewdb_base_SetLastOraAPIActionTime ();

	if (ewdb_base_Reconnect () != EWDB_RETURN_SUCCESS)
	{
		logit ("", "ewdb_api_GetComment(): ERROR Call to ewdb_base_Reconnect failed.\n");
		return (EWDB_RETURN_FAILURE);
	}

	Local_idComment = idComment;

	if (PrepGetCommentRecExec (&pCursor) != EWDB_RETURN_SUCCESS)
	{
		logit ("", "Call to PrepGetCommentRecExec failed.\n");
		return (ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
	}

	if (ewdb_base_SQLExecute (pCursor))
	{
		ewdb_base_ErrorReport(hEWDBC, pCursor,"ewdb_api_GetComment:ewdb_base_SQLExecute", 1);
		return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
	}

	retval = PostGetCommentRecExec (szComment, idNextComment);
	if (retval == EWDB_RETURN_FAILURE)
	{
		logit ("", "Call to PostGetCommentRecExec failed.\n");
		return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
	}

	ewdb_base_SetLastOraAPIActionTime ();
	return retval;
}  


/******************* InitGetCommentRecStatement *******************/
int	InitGetCommentRecStatement(char *szStatement, EWDB_OCIStatementStruct *pSS)
{

  int LastSize,i;  /* Size of last column array */

  
  if(!pLocalBuffer)
  {
    /*Allocate space for returned data - never freed */
    pLocalBuffer=malloc(BUFFERSIZE);
    
    iRecordSize=0;
    iRecordSize += pSS->FieldArray[0].Ind;	/*sComment*/
    iRecordSize += sizeof(EWDBid);			/*idNextComment*/
    
    /* Make the number of records a multiple of 8 to avoid alignment problems on UNIX */
    iRecordsPerBuffer= (BUFFERSIZE/iRecordSize) & 0xfffffff8;
    
    /*Allocate space for row/col ret lens. - never freed */
    for(i=0;i<pSS->NumOfFields;i++)
    {
      pSS->FieldArray[i].pRetLens=malloc(iRecordsPerBuffer*EWDB_FIELD_RET_LEN);
    }
    
    pSS->FieldArray[0].pVal = pLocalBuffer;
    LastSize = pSS->FieldArray[0].Ind;			/* sComment */

    pSS->FieldArray[1].pVal= (void *)((int)(pSS->FieldArray[0].pVal) + 
			(LastSize * iRecordsPerBuffer));
    LastSize = sizeof(EWDBid);					/* idNextComment */

    pSS->FieldArray[2].pVal= &Local_idComment;	/* incoming idComment */
  } /* end if(!pLocalBuffer) */

  if(!pLocalBuffer)
  {
    logit("","InitGetCommentRecStatement(): malloc of pLocalBuffer "
          "failed! Returning.\n");
	return EWDB_RETURN_FAILURE;
  }

  /* Clear our local memory buffer, so NULLs are treated as zeros */
  memset(pLocalBuffer, 0, BUFFERSIZE);

  ewdb_base_RequestCursor(szStatement, pSS,0/*don't force rebind*/);

  return(EWDB_RETURN_SUCCESS);
}  /* End InitGetCommentRecStatement() */


/******************* PrepGetCommentRecExec *******************/
int	PrepGetCommentRecExec(EWDB_Cursor *ppCursor)
{

  if (ppCursor == NULL)
  {
    logit ("", "PrepGetCommentRecExec(): ERROR Invalid arguments passed in.\n");
    return EWDB_RETURN_FAILURE;
  }

	SSStatement.NumOfFields = NUM_FIELDS;
	SSStatement.FieldArray = SQLParamsBindArray;
	SSStatement.RecordSize = 0;

	InitGetCommentRecStatement(SQL_STRING, &SSStatement);

	*ppCursor = SSStatement.pCda;

	return (EWDB_RETURN_SUCCESS);
}  /* end PrepGetCommentRecExec() */


/******************* PostGetCommentRecExec *******************/
int PostGetCommentRecExec (char *szComment, EWDBid *idNext)
{
	char 					*pTemp;
	EWDB_OCIStatementStruct	*pSS=&SSStatement;
	EWDB_Cursor 			pCursor = SSStatement.pCda;
	int 					done = 0;
	int 					RowsRetrieved = 0;
	int 					RowsDone = 0;
	int BufferRecLen = 1;
	int 					BCurr,UCurr;
	int 					RowsProcessed;
	int retval = EWDB_RETURN_SUCCESS;

	while (!done)
	{
		if (ewdb_base_SQLFetchRows (pCursor, iRecordsPerBuffer))
		{
			if (ewdb_base_GetCursorRetCode(pCursor) == EWDB_SQL_ERROR_NO_DATA) 
				done=1;
			else
			{
				ewdb_base_ErrorReport (hEWDBC, pCursor,"PostGetCommentRecExec:ewdb_base_SQLFetchRows", 1);
				return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
			}
		}

		RowsRetrieved=ewdb_base_GetCursorRowsProcessedCount(pCursor);

		for(; RowsDone < RowsRetrieved; RowsDone++)
		{
			if(RowsDone >= BufferRecLen)
			{
				done=1;
				break;
			}

			/* Copy from DB rows into the output buffer */
			BCurr = RowsDone % iRecordsPerBuffer;
			UCurr = RowsDone;

			/* indx 1: sAddress */
			pTemp = (char *) ((pSS->FieldArray[0].Ind * BCurr) + 
										(int)(pSS->FieldArray[0].pVal));
			pTemp[pSS->FieldArray[0].pRetLens[BCurr]] = 0;
			strcpy(szComment, pTemp);

			/* indx 2: sMailServer */
			*idNext = *(EWDBid *)((pSS->FieldArray[1].Ind * BCurr) + 
										(int)(pSS->FieldArray[1].pVal));
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
                       "PostGetCommentRecExec:ewdb_base_SQLFetchRows",2);
      return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
    }

	logit("", "PostGetCommentRecExec: received multiple entries for idComment %d\n",
			Local_idComment);
	retval = EWDB_RETURN_WARNING;
  }  /* End if(RowsRetrieved > BufferRecLen) */
  
  RowsProcessed = ewdb_base_GetCursorRowsProcessedCount(pCursor);

  ewdb_base_ReleaseCursor(pCursor);

  return(retval);
}  /* End PostGetCommentRecExec() */


