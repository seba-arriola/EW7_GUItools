
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_internal_CreateSnippet.c,v 1.3 2003/02/04 17:56:43 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_internal_CreateSnippet.c,v $
 *     Revision 1.3  2003/02/04 17:56:43  davidk
 *     Added a call to release the cursor used to retrieve snippet data
 *     when, the ewdb_internal_CreateSnippet() call fails.
 *     In testing, noted that this call was failing due to a table size-constraint,
 *     and it resulted in a cursor-leak that resulted in a program hanging during
 *     the ewdb_internal_CreateSnippet() call, after several failures.
 *
 *     Revision 1.2  2001/07/23 17:25:16  davidk
 *     API cleanup.
 *
 *     Revision 1.1  2001/05/15 02:16:45  davidk
 *     Initial revision
 *
 *     Revision 1.1  2001/02/28 17:19:22  lucky
 *     Initial revision
 *
 *
 */


#include <ewdb_cli_base.h>
#include <ewdb_ora_api.h>
#include <ewdb_ew_oci_base.h>


static char SQL_STRING[] =
     "insert into Waveform(idWaveform,binSnippet)"
     " values(:IN_idWaveform,:IN_RawSnippet)";

  
EWDB_OCI_SFS CreateSnippetBindArray[] = 
{

  {0,1,0,0,0,OA_INT,":IN_idWaveform"},
  {0,1,0,0,0,OA_LVRAW,":IN_RawSnippet"}

};

#define  NUM_FIELDS  2


static  EWDBid idWaveform;
static  int SnippetSize,SnippetBufSize=0;
static  int NewSnippetBufSize=TRUE;
static  char * pSnippet=(char *)NULL;

/* Insertion Struct for CreateSnippet statement */
static EWDB_OCIStatementStruct SSStatement;

/********************************
      FUNCTION PROTOTYPES
********************************/
int InitCreateSnippetStatement(char *statement, EWDB_OCIStatementStruct *pSS);
int PrepCreateSnippetExec(EWDBid IN_idWaveform, char * IN_pSnippet, 
                          int IN_SnippetSize, EWDB_Cursor *ppCursor);
int PostCreateSnippetExec(void);



int ewdb_internal_CreateSnippet(EWDBid IN_idWaveform, char * IN_pSnippet,
                                int IN_SnippetSize)
{
  EWDB_Cursor pCursor;

  if(IN_pSnippet == NULL)
  {
    logit("", "EWDB_CreateSnippet: Null snippet pointer passed in!\n");
    return EWDB_RETURN_FAILURE;
  }

  ewdb_base_SetLastOraAPIActionTime();

  if(ewdb_base_Reconnect() != EWDB_RETURN_SUCCESS)
  /* Establishes connection, and performs binding!?! */
  {
    logit("", "Could not reconnect to the database!\n");
    return(EWDB_RETURN_FAILURE);
  }

  if(PrepCreateSnippetExec(IN_idWaveform, IN_pSnippet, IN_SnippetSize, 
                             &pCursor) 
      != EWDB_RETURN_SUCCESS)
  {
    logit("","ORA_API:EWDB_CreateSnippet():PrepCreateSnippetExec() failed.\n");
    /* DK 02/13/2001 removed call to disconnect, as that causes a
       commit to happen */
    return(EWDB_RETURN_FAILURE);
  }

  if(ewdb_base_SQLExecute(pCursor))
  {
    ewdb_base_ErrorReport(hEWDBC, pCursor,"EWDB_CreateSnippet:ewdb_base_SQLExecute", 1);
    logit("","EWDB_CreateSnippet() failed during oexec() for "
          "idWaveform %d\n",
          IN_idWaveform);
    /* DK 02/13/2001 removed call to disconnect, as that causes a
       commit to happen */
    /* DK 02/03/2003 added a call to release the cursor.  Normally we would
       do a disconnect at this point, but since that forces a commit, and we
       want a chance to do the rollback in the ewdb_api_CreateWaveform()
       function, we cannot do the disconnect here.  
       We could do the disconnect in ewdb_api_CreateWaveform(), but unfortunately
       we don't get enough information back in ewdb_api_CreateWaveform() to figure
       out if the error is bad enough to disconnect over.
       One of the things the disconnect does is clear out the cursors.  By not 
       disconnecting and not closing the cursor, we cause a cursor leak.  
       So, we atleast need to release the cursor here, even if we don't do anything
       else.
    */
    ewdb_base_ReleaseCursor(pCursor);

    return(EWDB_RETURN_FAILURE);
  } 

  /* CHANGED: DavidK 05/08/2000 Removed commit.*/
  /* Do not commit the transaction yet.  This insertion is
     part of a larger transaction.  Someone above us in the
     food changed will do the commit, once they are sure that
     all of the req'd data is in.  
   **********************************************************/
  if(PostCreateSnippetExec() != EWDB_RETURN_SUCCESS)
  {
    logit("", "Call to PostCreateSnippetExec failed!\n");
    /* DK 02/13/2001 removed call to disconnect, as that causes a
       commit to happen */
    return(EWDB_RETURN_FAILURE);
  }

  return(EWDB_RETURN_SUCCESS);
}  /* end ewdb_internal_CreateSnippet() */


int InitCreateSnippetStatement(char *statement, EWDB_OCIStatementStruct *pSS)
{
  
  static int bForceRebind=TRUE;

  if((statement == NULL) || (pSS == NULL))
  {
    logit("", "Null parameters passed in!\n");
    return EWDB_RETURN_FAILURE;
  }
  
  if(NewSnippetBufSize)
  {
    pSS->FieldArray[0].pVal = &(idWaveform);
    pSS->FieldArray[1].pVal = pSnippet;
  }

  pSS->FieldArray[1].pRetCodes=(unsigned short *)SnippetSize;
  /* we overloaded pRetCodes with the SnippetSize for this
     datatype */
  
  NewSnippetBufSize = FALSE;
  
  if(ewdb_base_RequestCursor(statement, pSS, bForceRebind) != 0)
  {
    logit("", "Call to ewdb_base_RequestCursor failed.\n");
    return EWDB_RETURN_FAILURE;
  }
  
  return(EWDB_RETURN_SUCCESS);
}  /* End InitCreateSnippetStatement() */


int PrepCreateSnippetExec(EWDBid IN_idWaveform, char * IN_pSnippet, 
                          int IN_SnippetSize, EWDB_Cursor *ppCursor)
{

  if((IN_pSnippet == NULL) ||(ppCursor == NULL))
  {
    logit("", "PrepCreateSnippetExec(): Null parameters passed in!\n");
    return EWDB_RETURN_FAILURE;
  }

  SSStatement.NumOfFields = NUM_FIELDS;
  SSStatement.FieldArray = CreateSnippetBindArray;
  SSStatement.RecordSize = 0;

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
      logit("", "EWDB_CreateSnippet:ERROR! Failed to malloc %d "
                 "bytes after freeing %d!\n",
             SnippetSize,SnippetBufSize);
    }
    SnippetBufSize=SnippetSize;
  }
  memcpy(pSnippet,IN_pSnippet,SnippetSize);

  /* we are now prepped and ready */

  if(InitCreateSnippetStatement(SQL_STRING,
                                  &SSStatement)
      != EWDB_RETURN_SUCCESS)
  {
    logit("", "Call to InitCreateSnippetStatement failed!\n");
    return EWDB_RETURN_FAILURE;
  }

  *ppCursor = SSStatement.pCda;
  
  return(EWDB_RETURN_SUCCESS);
}  /* End PrepCreateSnippetExec() */


int PostCreateSnippetExec(void)
{
  EWDB_Cursor pCursor;

  pCursor = SSStatement.pCda;

  ewdb_base_ReleaseCursor(pCursor);

  return(EWDB_RETURN_SUCCESS);
}  /* End PostCreateSnippetExec() */


