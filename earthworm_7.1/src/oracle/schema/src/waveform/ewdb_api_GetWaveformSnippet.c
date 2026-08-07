
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_api_GetWaveformSnippet.c,v 1.2 2001/05/15 02:16:44 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_api_GetWaveformSnippet.c,v $
 *     Revision 1.2  2001/05/15 02:16:44  davidk
 *     Moved functions around between the apps, DB API, and DB API INTERNAL
 *     levels.  Renamed functions and files.  Added support for amplitude
 *     magnitude types.  Reformatted makefiles.
 *
 *     Revision 1.1  2001/02/28 17:19:22  lucky
 *     Initial revision
 *
 *
 */



#include <stdlib.h>
#include <stdio.h>
#include <ewdb_ora_api.h>
#include <ewdb_cli_base.h>
#include <ewdb_ew_oci_base.h>


char GET_SNIPPET_STRING[] =
     "select binSnippet from Waveform where idWaveform = :IN_idWaveform";

  
EWDB_OCI_SFS GetSnippetBindArray[] = 
{

  {0,1,0,0,0,OA_LVRAW,"1binSnippet"},
  {0,1,0,0,0,OA_INT,":IN_idWaveform"}

};

#define  NUM_FIELDS  2

static  EWDBid idWaveform,SnippetSize,SnippetBufSize=0;
static  int NewSnippetBufSize=TRUE;
static  char * pSnippet=(char *)NULL;
static  unsigned short SnippetRetLength;

/* Insertion Struct for GetSnippet statement */
EWDB_OCIStatementStruct SSGetSnippet;

/********************************
      FUNCTION PROTOTYPES
********************************/
int PrepGetSnippetExec(EWDBid IN_idWaveform, 
                           int IN_SnippetSize, EWDB_Cursor *ppCursor);
int PostGetSnippetExec(char * IN_pSnippet);
int InitGetSnippetStatement (char *statement, EWDB_OCIStatementStruct *pSS);



/**********************************************************************
**********************************************************************/
int ewdb_api_GetWaveformSnippet (EWDBid IN_idWaveform, char * IN_pWaveform,
                             int IN_WaveformLength)
{

	EWDB_Cursor pCursor;
	int		retVal;

	if (IN_pWaveform == NULL)
	{
		logit ("", "EWDB_GetSnippet:Null snippet pointer passed in!\n");
		return EWDB_RETURN_FAILURE;
	}

	ewdb_base_SetLastOraAPIActionTime ();


	if (ewdb_base_Reconnect () != EWDB_RETURN_SUCCESS)
	{
		logit ("", "Could not reconnect to the database!\n");
		return (EWDB_RETURN_FAILURE);
	}

	if (PrepGetSnippetExec (IN_idWaveform, IN_WaveformLength, &pCursor) 
      != EWDB_RETURN_SUCCESS)
	{
		logit ("", "ORA_API:EWDB_GetWaveformSnippet():PrepGetSnippetExec() failed.\n");
		return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
	}

	if (ewdb_base_SQLExecute (pCursor))
	{
		ewdb_base_ErrorReport (hEWDBC, pCursor,"EWDB_GetWaveformSnippet:ewdb_base_SQLExecute", 1);
		return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
	} 

  
	/* Commit the transaction (all the previous inserts!)
	   In Case there is any auditing or logging or debug
	   changes made in the stored procedures.
	****************************************************/
	if (ewdb_base_SQLCommit (hEWDBC))
	{
		ewdb_base_ErrorReport (hEWDBC, hEWDBC,"EWDB_GetWaveformSnippet:ewdb_base_SQLCommit",2);
		return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
	}

	if ((retVal = PostGetSnippetExec (IN_pWaveform)) != EWDB_RETURN_SUCCESS)
	{
		if (retVal == EWDB_RETURN_WARNING)
		{
			logit ("", "EWDB_GetWaveformSnippet(): "
   					        "Call to PostGetSnippetExec returned WARNING!\n");
			return (EWDB_RETURN_WARNING);
		}
		else
		{
			logit ("", "EWDB_GetWaveformSnippet(): "
   					        "Call to PostGetSnippetExec failed!\n");
			return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
		}
	}

	ewdb_base_SetLastOraAPIActionTime ();

	return(EWDB_RETURN_SUCCESS);
}


int InitGetSnippetStatement (char *statement, EWDB_OCIStatementStruct *pSS)
{
  
  int bForceRebind;
  
  if ((statement == NULL) ||  (pSS == NULL))
  {
    logit ("", "InitGSS:Null parameters passed in!\n");
    return EWDB_RETURN_FAILURE;
  }
  
  if(NewSnippetBufSize)
  {
    
    pSS->FieldArray[0].pVal = pSnippet;
    pSS->FieldArray[1].pVal = &(idWaveform);
    
    pSS->FieldArray[0].pRetCodes=(unsigned short *)SnippetSize;
    /* we overloaded pRetCodes with the SnippetSize for this
    datatype */
    pSS->FieldArray[0].pRetLens=&SnippetRetLength;
  }
  
  bForceRebind = NewSnippetBufSize;
  NewSnippetBufSize = FALSE;
  
  if (ewdb_base_RequestCursor (statement, pSS, bForceRebind) != 0)
  {
    logit ("", "InitGetSnippetStatement(): "
           "Call to ewdb_base_RequestCursor failed.\n");
    return EWDB_RETURN_FAILURE;
  }
  
  return(EWDB_RETURN_SUCCESS);
}


int PrepGetSnippetExec (EWDBid IN_idWaveform, 
                           int IN_SnippetSize, EWDB_Cursor *ppCursor)
{

  if (ppCursor == NULL)
  {
    logit ("", "PrepGetSnippetExec(): Null parameters passed in!\n");
    return EWDB_RETURN_FAILURE;
  }

  SSGetSnippet.NumOfFields = NUM_FIELDS;
  SSGetSnippet.FieldArray = GetSnippetBindArray;
  SSGetSnippet.RecordSize = 0;

  /* Copy the incoming data into local vars */
  idWaveform = IN_idWaveform;
  SnippetSize = IN_SnippetSize;

  if(SnippetSize > SnippetBufSize)
  {
    NewSnippetBufSize=TRUE;
    if(pSnippet)
    {
      free(pSnippet);
    }
    if((pSnippet=(char *)malloc(SnippetSize)) == NULL)
    {
      logit ("", "EWDB_GetSnippet:ERROR! Failed to malloc %d "
                 "bytes after freeing %d!\n",
             SnippetSize,SnippetBufSize);
    }
    SnippetBufSize=SnippetSize;
  }

  /* we are now prepped and ready */

  if (InitGetSnippetStatement (GET_SNIPPET_STRING,
                           &SSGetSnippet) != EWDB_RETURN_SUCCESS)
  {
    logit ("", "Call to InitGetSnippetStatement failed!\n");
    return EWDB_RETURN_FAILURE;
  }

  *ppCursor = SSGetSnippet.pCda;
  
  return EWDB_RETURN_SUCCESS;
}


int PostGetSnippetExec (char * IN_pSnippet)
{
  EWDB_Cursor  pCursor = SSGetSnippet.pCda;
  int   ActLen;


  if (ewdb_base_SQLFetchRows(pCursor, 1))
  {
    if (ewdb_base_GetCursorRetCode(pCursor) == EWDB_SQL_ERROR_NO_DATA) 
    {
      logit("","PostGetSnippetExec(): No rows retrieved for idWaveform %d\n",
            *(int *)(SSGetSnippet.FieldArray[1].pVal));
      ActLen=0;
    }
    else
    {
      ewdb_base_ErrorReport(hEWDBC, pCursor,"PostGetSnippetExec:ewdb_base_SQLFetchRows",1);
      return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
    }
  }
  else
  {
    ActLen = *(unsigned short *)(SSGetSnippet.FieldArray[0].pRetLens);

    memcpy(IN_pSnippet,pSnippet,SnippetSize);

  }  /* ewdb_base_SQLFetchRows was successful */

  ewdb_base_ReleaseCursor(SSGetSnippet.pCda);

  if (ActLen == 0)
    return (EWDB_RETURN_WARNING);
  
  return (EWDB_RETURN_SUCCESS);
}
