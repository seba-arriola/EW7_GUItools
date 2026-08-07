
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_api_CreateChannel.c,v 1.4 2005/06/10 16:27:58 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_api_CreateChannel.c,v $
 *     Revision 1.4  2005/06/10 16:27:58  davidk
 *     DB API Cleanup
 *
 *     Revision 1.3  2003/12/03 00:23:29  davidk
 *     Fixed the SQL Proc name called by the function.  (Now Create_Chan).
 *
 *     Revision 1.2  2001/05/15 02:16:28  davidk
 *     Moved functions around between the apps, DB API, and DB API INTERNAL
 *     levels.  Renamed functions and files.  Added support for amplitude
 *     magnitude types.  Reformatted makefiles.
 *
 *     Revision 1.1  2001/02/28 17:19:22  lucky
 *     Initial revision
 *
 *
 */


#include <ewdb_ora_api.h>
#include <ewdb_cli_base.h>
#include <ewdb_ew_oci_base.h>


static char SQL_STRING[] =
  "Begin Create_Chan(OUT_idChan => :OUT_idChan,"
  " IN_sComment => :IN_sComment); End;";

static EWDB_OCI_SFS SQLParamsBindArray[] = 
{
  {0,1,0,0,0,OA_INT,":OUT_idChan"},
  {0,1,0,0,0,OA_SZ,":IN_sComment"}
};

#define  NUM_FIELDS 2

static EWDBid Local_idChan;
/* Why is sComment 4100? DK 5/15/00 */
static char sComment[4100];

/* Statement Struct for CREATE_CHANNEL szStatement */
static EWDB_OCIStatementStruct SSStatement;

/********************************
      FUNCTION PROTOTYPES
********************************/
static int PrepCreateChannelExec(char * IN_szComment, EWDB_Cursor * ppCursor);
static int PostCreateChannelExec(EWDBid * pidChan);
static int InitCreateChannelStatement (char *szStatement, EWDB_OCIStatementStruct *pSS);
/*******************************/



/* Create a Channel record.  A channel represents the path
   that starts from a sensor in the field and ends at digital waveforms.
   Because most of a channel's properties are variable over
   time, this function does not take much as input, as it
   only creates a channel with its fixed attribute(s).  This
   function should be used to generate a new Local_idChan that can
   be used in conjunction with other functions for creating
   components and setting time based attributes of components
   and channels.
*********************************************************************/
int ewdb_api_CreateChannel (EWDBid * pidChan, char * IN_szComment)
{

  EWDB_Cursor pCursor;

  if (pidChan == NULL || IN_szComment== NULL)
  {
    logit ("", "EWDB_CreateChannel(): Null pointer passed in!\n");
    return EWDB_RETURN_FAILURE;
  }

  ewdb_base_SetLastOraAPIActionTime ();

  if (ewdb_base_Reconnect () != EWDB_RETURN_SUCCESS)
  {
    logit("", "EWDB_CreateChannel(): Could not reconnect to the database!\n");
    return (EWDB_RETURN_FAILURE);
  }

  if (PrepCreateChannelExec (IN_szComment, &pCursor) != EWDB_RETURN_SUCCESS)
  {
    logit ("", "EWDB_CreateChannel(): PrepCreateChannelExec() failed.\n");
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  }

  if (ewdb_base_SQLExecute (pCursor))
  {
    ewdb_base_ErrorReport (hEWDBC, pCursor,"EWDB_CreateChannel(): ewdb_base_SQLExecute", 1);
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  } 

  /* Commit the transaction (all the previous inserts!)
     In Case there is any auditing or logging or debug
     changes made in the stored procedures.
  ****************************************************/
  if (ewdb_base_SQLCommit (hEWDBC))
  {
    ewdb_base_ErrorReport (hEWDBC, hEWDBC,"EWDB_CreateChannel():ewdb_base_SQLCommit",2);
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  }

  if (PostCreateChannelExec (pidChan) != EWDB_RETURN_SUCCESS)
  {
    logit ("", "EWDB_CreateChannel(): "
           "PostCreateChannelExec failed!\n");
    return (EWDB_RETURN_FAILURE);
  }

  ewdb_base_SetLastOraAPIActionTime ();

  return(EWDB_RETURN_SUCCESS);
}  /* end EWDB_CreateChannel() */


static int InitCreateChannelStatement (char *szStatement, EWDB_OCIStatementStruct *pSS)
{

  pSS->FieldArray[0].pVal = &Local_idChan;
  pSS->FieldArray[1].pVal = sComment;

  return(ewdb_base_RequestCursor (szStatement, pSS, 0));
}  /* End InitCreateChannelStatement() */


static int PrepCreateChannelExec(char * IN_szComment, EWDB_Cursor * ppCursor)
{

  SSStatement.NumOfFields = NUM_FIELDS;
  SSStatement.FieldArray = SQLParamsBindArray;
  SSStatement.RecordSize = 0;

  strncpy(sComment,IN_szComment,sizeof(sComment));
  sComment[sizeof(sComment)-1]=0;

  if (InitCreateChannelStatement (SQL_STRING,
                           &SSStatement) != EWDB_RETURN_SUCCESS)
  {
    logit ("", "PrepCreateChannelExec(): InitCreateChannelStatement failed!\n");
    return EWDB_RETURN_FAILURE;
  }

  *ppCursor = SSStatement.pCda;
  
  return(EWDB_RETURN_SUCCESS);
}  /* End PrepCreateChannelExec() */


static int PostCreateChannelExec(EWDBid * pidChan)
{
  EWDB_Cursor pCursor;
  
  *pidChan=Local_idChan;
  
  pCursor = SSStatement.pCda;
  
  ewdb_base_ReleaseCursor(pCursor);

  if(Local_idChan <= 0)
  {
    logit("", "PostCreateChannelExec(): SQL Proc Create_Chan() failed with error %d\n", 
          Local_idChan);
    return(EWDB_RETURN_WARNING);
  }

  return (EWDB_RETURN_SUCCESS);
}  /* End PostCreateChannelExec() */

