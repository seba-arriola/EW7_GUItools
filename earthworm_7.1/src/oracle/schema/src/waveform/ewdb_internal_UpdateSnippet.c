
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_internal_UpdateSnippet.c,v 1.1 2002/05/16 17:06:04 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_internal_UpdateSnippet.c,v $
 *     Revision 1.1  2002/05/16 17:06:04  davidk
 *     Initial revision
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
#include <ewdb_internal_functions.h>


static char SQL_STRING[] =
     "update Waveform "
     " set binSnippet = :IN_RawSnippet "
     " where idWaveform = :IN_idWaveform";

  
EWDB_OCI_SFS UpdateSnippetBindArray[] = 
{

  {0,1,0,0,0,OA_INT,":IN_idWaveform"},
  {0,1,0,0,0,OA_LVRAW,":IN_RawSnippet"}

};

#define  NUM_FIELDS  2


static  EWDBid idWaveform;
static  int SnippetSize,SnippetBufSize=0;
static  int NewSnippetBufSize=TRUE;
static  char * pSnippet=(char *)NULL;

/* Insertion Struct for UpdateSnippet statement */
static EWDB_OCIStatementStruct SSStatement;

/********************************
      FUNCTION PROTOTYPES
********************************/
int InitUpdateSnippetStatement(char *statement, EWDB_OCIStatementStruct *pSS);
int PrepUpdateSnippetExec(EWDBid IN_idWaveform, char * IN_pSnippet, 
                          int IN_SnippetSize, EWDB_Cursor *ppCursor);
int PostUpdateSnippetExec(void);



int ewdb_internal_UpdateSnippet(EWDBid IN_idWaveform, char * IN_pSnippet,
                                int IN_SnippetSize)
{
  EWDB_Cursor pCursor;

  if(IN_pSnippet == NULL)
  {
    logit("", "EWDB_UpdateSnippet: Null snippet pointer passed in!\n");
    return EWDB_RETURN_FAILURE;
  }

  if(ewdb_internal_CheckidWaveform(IN_idWaveform) != EWDB_RETURN_SUCCESS)
  {
    logit("t","ewdb_internal_CheckidWaveform() failed for idWaveform %d\n",
          IN_idWaveform);
    return(EWDB_RETURN_FAILURE);
  }

  ewdb_base_SetLastOraAPIActionTime();

  if(ewdb_base_Reconnect() != EWDB_RETURN_SUCCESS)
  /* Establishes connection, and performs binding!?! */
  {
    logit("", "Could not reconnect to the database!\n");
    return(EWDB_RETURN_FAILURE);
  }

  if(PrepUpdateSnippetExec(IN_idWaveform, IN_pSnippet, IN_SnippetSize, 
                             &pCursor) 
      != EWDB_RETURN_SUCCESS)
  {
    logit("","ORA_API:EWDB_UpdateSnippet():PrepUpdateSnippetExec() failed.\n");
    /* DK 02/13/2001 removed call to disconnect, as that causes a
       commit to happen */
    return(EWDB_RETURN_FAILURE);
  }

  if(ewdb_base_SQLExecute(pCursor))
  {
    ewdb_base_ErrorReport(hEWDBC, pCursor,"EWDB_UpdateSnippet:ewdb_base_SQLExecute", 1);
    logit("","EWDB_UpdateSnippet() failed during oexec() for "
          "idWaveform %d\n",
          IN_idWaveform);
    /* DK 02/13/2001 removed call to disconnect, as that causes a
       commit to happen */
    return(EWDB_RETURN_FAILURE);
  } 

  /* CHANGED: DavidK 05/08/2000 Removed commit.*/
  /* Do not commit the transaction yet.  This insertion is
     part of a larger transaction.  Someone above us in the
     food changed will do the commit, once they are sure that
     all of the req'd data is in.  
   **********************************************************/
  if(PostUpdateSnippetExec() != EWDB_RETURN_SUCCESS)
  {
    logit("", "Call to PostUpdateSnippetExec failed!\n");
    /* DK 02/13/2001 removed call to disconnect, as that causes a
       commit to happen */
    return(EWDB_RETURN_FAILURE);
  }

  return(EWDB_RETURN_SUCCESS);
}  /* end ewdb_internal_UpdateSnippet() */


int InitUpdateSnippetStatement(char *statement, EWDB_OCIStatementStruct *pSS)
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
}  /* End InitUpdateSnippetStatement() */


int PrepUpdateSnippetExec(EWDBid IN_idWaveform, char * IN_pSnippet, 
                          int IN_SnippetSize, EWDB_Cursor *ppCursor)
{

  if((IN_pSnippet == NULL) ||(ppCursor == NULL))
  {
    logit("", "PrepUpdateSnippetExec(): Null parameters passed in!\n");
    return EWDB_RETURN_FAILURE;
  }

  SSStatement.NumOfFields = NUM_FIELDS;
  SSStatement.FieldArray = UpdateSnippetBindArray;
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
      logit("", "EWDB_UpdateSnippet:ERROR! Failed to malloc %d "
                 "bytes after freeing %d!\n",
             SnippetSize,SnippetBufSize);
    }
    SnippetBufSize=SnippetSize;
  }
  memcpy(pSnippet,IN_pSnippet,SnippetSize);

  /* we are now prepped and ready */

  if(InitUpdateSnippetStatement(SQL_STRING,
                                  &SSStatement)
      != EWDB_RETURN_SUCCESS)
  {
    logit("", "Call to InitUpdateSnippetStatement failed!\n");
    return EWDB_RETURN_FAILURE;
  }

  *ppCursor = SSStatement.pCda;
  
  return(EWDB_RETURN_SUCCESS);
}  /* End PrepUpdateSnippetExec() */


int PostUpdateSnippetExec(void)
{
  EWDB_Cursor pCursor;

  pCursor = SSStatement.pCda;

  ewdb_base_ReleaseCursor(pCursor);

  return(EWDB_RETURN_SUCCESS);
}  /* End PostUpdateSnippetExec() */


