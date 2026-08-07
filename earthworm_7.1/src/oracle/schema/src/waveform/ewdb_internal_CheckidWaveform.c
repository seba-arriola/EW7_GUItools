
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_internal_CheckidWaveform.c,v 1.1 2002/05/16 17:06:04 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_internal_CheckidWaveform.c,v $
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


static char SQL_STRING[] =
     "Begin "
     "  select idWaveform into :OUT_idWaveform "
     "   from Waveform "
     "   where idWaveform = :IN_idWaveform; "
     "EXCEPTION "
     " WHEN OTHERS THEN "
     "  :OUT_idWaveform := -1; "
     "END;";

  
EWDB_OCI_SFS CheckidWaveformBindArray[] = 
{

  {0,1,0,0,0,OA_EWDBID,":IN_idWaveform"},
  {0,1,0,0,0,OA_EWDBID,":OUT_idWaveform"},

};

#define  NUM_FIELDS  2


static  EWDBid idWaveform;
static  EWDBid OUT_idWaveform;

/* Insertion Struct for CheckidWaveform statement */
static EWDB_OCIStatementStruct SSStatement;

/********************************
      FUNCTION PROTOTYPES
********************************/
int InitCheckidWaveformStatement(char *statement, EWDB_OCIStatementStruct *pSS);
int PrepCheckidWaveformExec(EWDBid IN_idWaveform, EWDB_Cursor *ppCursor);
int PostCheckidWaveformExec(void);



int ewdb_internal_CheckidWaveform(EWDBid IN_idWaveform)
{
  EWDB_Cursor pCursor;

  if(IN_idWaveform <= 0)
  {
    logit("", "EWDB_CheckidWaveform: Invalid idWaveform(%d) passed in!\n",IN_idWaveform);
    return(EWDB_RETURN_FAILURE);
  }

  ewdb_base_SetLastOraAPIActionTime();

  if(ewdb_base_Reconnect() != EWDB_RETURN_SUCCESS)
  /* Establishes connection, and performs binding!?! */
  {
    logit("", "Could not reconnect to the database!\n");
    return(EWDB_RETURN_FAILURE);
  }

  if(PrepCheckidWaveformExec(IN_idWaveform, &pCursor) 
      != EWDB_RETURN_SUCCESS)
  {
    logit("","ORA_API:EWDB_CheckidWaveform():PrepCheckidWaveformExec() failed.\n");
    /* DK 02/13/2001 removed call to disconnect, as that causes a
       commit to happen */
    return(EWDB_RETURN_FAILURE);
  }

  if(ewdb_base_SQLExecute(pCursor))
  {
    ewdb_base_ErrorReport(hEWDBC, pCursor,"ewdb_internal_CheckidWaveform:ewdb_base_SQLExecute", 1);
    logit("","ewdb_internal_CheckidWaveform() failed during SQLExecute() for "
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
  if(PostCheckidWaveformExec() != EWDB_RETURN_SUCCESS)
  {
    logit("", "Call to PostCheckidWaveformExec failed!\n");
    /* DK 02/13/2001 removed call to disconnect, as that causes a
       commit to happen */
    return(EWDB_RETURN_FAILURE);
  }

  return(EWDB_RETURN_SUCCESS);
}  /* end ewdb_internal_CheckidWaveform() */


int InitCheckidWaveformStatement(char *statement, EWDB_OCIStatementStruct *pSS)
{
  
  if((statement == NULL) || (pSS == NULL))
  {
    logit("", "Null parameters passed in!\n");
    return EWDB_RETURN_FAILURE;
  }
  
  pSS->FieldArray[0].pVal = &(idWaveform);
  pSS->FieldArray[1].pVal = &(OUT_idWaveform);

  if(ewdb_base_RequestCursor(statement, pSS, FALSE) != 0)
  {
    logit("", "Call to ewdb_base_RequestCursor failed.\n");
    return EWDB_RETURN_FAILURE;
  }
  
  return(EWDB_RETURN_SUCCESS);
}  /* End InitCheckidWaveformStatement() */


int PrepCheckidWaveformExec(EWDBid IN_idWaveform, EWDB_Cursor *ppCursor)
{

  SSStatement.NumOfFields = NUM_FIELDS;
  SSStatement.FieldArray = CheckidWaveformBindArray;
  SSStatement.RecordSize = 0;

  /* Copy the incoming data into local vars */
  idWaveform = IN_idWaveform;

  /* we are now prepped and ready */

  if(InitCheckidWaveformStatement(SQL_STRING, &SSStatement)
      != EWDB_RETURN_SUCCESS)
  {
    logit("", "Call to InitCheckidWaveformStatement failed!\n");
    return EWDB_RETURN_FAILURE;
  }

  *ppCursor = SSStatement.pCda;
  
  return(EWDB_RETURN_SUCCESS);
}  /* End PrepCheckidWaveformExec() */


int PostCheckidWaveformExec(void)
{
  EWDB_Cursor pCursor;

  pCursor = SSStatement.pCda;

  ewdb_base_ReleaseCursor(pCursor);

  if(OUT_idWaveform == -1)
    return(EWDB_RETURN_FAILURE);
  else
    return(EWDB_RETURN_SUCCESS);
}  /* End PostCheckidWaveformExec() */


