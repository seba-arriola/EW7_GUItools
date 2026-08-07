
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_internal_SetChanParams.c,v 1.4 2005/06/10 16:27:59 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_internal_SetChanParams.c,v $
 *     Revision 1.4  2005/06/10 16:27:59  davidk
 *     DB API Cleanup
 *
 *     Revision 1.3  2004/04/06 17:48:07  davidk
 *     format changes only.  No functional change.
 *
 *     Revision 1.2  2003/12/03 00:23:29  davidk
 *     Replaced all of the individual input params with a single
 *     EWDB_ChannelStruct pointer.
 *
 *     Revision 1.1  2001/07/23 17:10:39  davidk
 *     Initial revision
 *
 *     Revision 1.2  2001/05/15 02:16:31  davidk
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
  "Begin Set_Chan_Params(OUT_idChanT => :OUT_idChanT,"
  " IN_idChan => :IN_idChan, IN_idCompT => :IN_idCompT," 
  " IN_tOn => :IN_tOn, IN_tOff=>:IN_tOff,"
  " IN_idDeviceSlot => :IN_idDeviceSlot,"
  " IN_iPlexor => :IN_iPlexor,"
  " IN_sComment => :IN_sComment); End;";

static EWDB_OCI_SFS SQLParamsBindArray[] = 
{
  {0,1,0,0,0,OA_INT,":OUT_idChanT"},
  {0,1,0,0,0,OA_INT,":IN_idChan"},
  {0,1,0,0,0,OA_INT,":IN_idCompT"},
  {0,1,0,0,0,OA_DOUBLE,":IN_tOn"},
  {0,1,0,0,0,OA_DOUBLE,":IN_tOff"},
  {0,1,0,0,0,OA_EWDBID,":IN_idDeviceSlot"},
  {0,1,0,0,0,OA_INT,":IN_iPlexor"},
  {0,1,0,0,0,OA_SZ,":IN_sComment"}
};

#define  NUM_FIELDS 8

static EWDB_ChannelStruct Local_Chan;
static char Local_sztOn[20],Local_sztOff[20];
static char Local_szComment[4001] = "";
static int idDeviceSlot = 0;
static int iPlexor = 0;

/* Statement Struct for Set_Chan_Params statement */
static EWDB_OCIStatementStruct SSStatement;

/********************************
      FUNCTION PROTOTYPES
********************************/
static int PrepSetChanParamsExec(EWDB_ChannelStruct * IN_pChan, EWDB_Cursor * ppCursor);
static int PostSetChanParamsExec(EWDBid * pidChanT);
static int InitSetChanParamsStatement(char *statement, EWDB_OCIStatementStruct *pSS);



/* 
     idChan:    id of the Channel to set params for.
     idCompT:   id of the Component time-interval to associate
                with the Channel.
     idDevice:  The DBid of the Device that is the beginning of
                the channel.  UNUSED at this time!!
     pidChanT:  idChanT of the channel - time interval record
                that was created or updated by this function.
     tOn,tOff:  Start/End of time interval for which the params
                are correct for the channel.
     Local_szComment: Comment regarding the channel time interval.
     
   
   *Note:  idDevice is currently ignored, please set the value to 0

   uglier than SetCompParams, and with the same
   type of clueless requirements.It has to make 
   sense and it has to match up
   with the SQL function SetChanParams()
******************************************************************/
int ewdb_internal_SetChanParams(EWDB_ChannelStruct * pChan)
{

  EWDB_Cursor pCursor;

  if (pChan == NULL)
  {
    logit ("", "ewdb_internal_SetChanParams(): Null pointer passed in!\n");
    return EWDB_RETURN_FAILURE;
  }

  ewdb_base_SetLastOraAPIActionTime ();


  if (ewdb_base_Reconnect () != EWDB_RETURN_SUCCESS)
  {
    logit("", "ewdb_internal_SetChanParams(): Could not reconnect to the database!\n");
    return (EWDB_RETURN_FAILURE);
  }

  /* CHANGED 05/10/2000 DK: Order of the function parameters
     was incorrect.  I corrected it.
  **********************************************************/
  if (PrepSetChanParamsExec (pChan, &pCursor) != EWDB_RETURN_SUCCESS)
  {
    logit ("", "ewdb_internal_SetChanParams(): PrepSetChanParamsExec() failed.\n");
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  }

  if (ewdb_base_SQLExecute (pCursor))
  {
    ewdb_base_ErrorReport (hEWDBC, pCursor,"ewdb_internal_SetChanParams(): ewdb_base_SQLExecute", 1);
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  } 

  /* Commit the transaction (all the previous inserts!)
     In Case there is any auditing or logging or debug
     changes made in the stored procedures.
  ****************************************************/
  if (ewdb_base_SQLCommit (hEWDBC))
  {
    ewdb_base_ErrorReport (hEWDBC, hEWDBC,"EWDB_SetChanParams():ewdb_base_SQLCommit",2);
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  }

  if (PostSetChanParamsExec (&pChan->idChanT) 
      != EWDB_RETURN_SUCCESS)
  {
    logit ("", "ewdb_internal_SetChanParams(): Call to PostSetChanParamsExec failed!\n");
    return (EWDB_RETURN_FAILURE);
  }

  ewdb_base_SetLastOraAPIActionTime ();

  return(EWDB_RETURN_SUCCESS);
}  /* end ewdb_internal_SetChanParams() */


static int InitSetChanParamsStatement(char *statement, EWDB_OCIStatementStruct *pSS)
{
 
  pSS->FieldArray[0].pVal = &Local_Chan.idChanT;
  pSS->FieldArray[1].pVal = &Local_Chan.Comp.idChan;
  pSS->FieldArray[2].pVal = &Local_Chan.idCompT;
  pSS->FieldArray[3].pVal = Local_sztOn;
  pSS->FieldArray[4].pVal = Local_sztOff;
  pSS->FieldArray[5].pVal = &idDeviceSlot;
  pSS->FieldArray[6].pVal = &iPlexor;
  pSS->FieldArray[7].pVal = Local_szComment;

  return(ewdb_base_RequestCursor (statement, pSS, 0));
}  /* End InitSetChanParamsStatement() */


static int PrepSetChanParamsExec(EWDB_ChannelStruct * IN_pChan, EWDB_Cursor * ppCursor)
{

  SSStatement.NumOfFields = NUM_FIELDS;
  SSStatement.FieldArray = SQLParamsBindArray;
  SSStatement.RecordSize = 0;

  memcpy(&Local_Chan,IN_pChan,sizeof(Local_Chan));

  /* When the infrastructure schema is created and idDevices
     are permitted to be something other than NULL, then
     uncomment the following line. DavidK 2000/05/10 
  *********************************************************/
  /* idDevice = IN_idDevice */

  sprintf(Local_sztOn,"%.2f",Local_Chan.tOn);
  sprintf(Local_sztOff,"%.2f",Local_Chan.tOff);

  if (InitSetChanParamsStatement (SQL_STRING,
                           &SSStatement) != EWDB_RETURN_SUCCESS)
  {
    logit ("", "PrepSetChanParamsExec(): InitSetChanParamsStatement failed!\n");
    return EWDB_RETURN_FAILURE;
  }

  *ppCursor = SSStatement.pCda;
  
  return(EWDB_RETURN_SUCCESS);
}  /* End PrepSetChanParamsExec() */


static int PostSetChanParamsExec(EWDBid * pidChanT)
{
  EWDB_Cursor pCursor;
  
  *pidChanT=Local_Chan.idChanT;
  
  pCursor = SSStatement.pCda;
  
  ewdb_base_ReleaseCursor(pCursor);

  if(*pidChanT <= 0)
  {
    logit("","PostSetChanParamsExec():  ERROR SQL Proc Set_Chan_Params() returned error %d\n",
          Local_Chan.idChanT);
    return(EWDB_RETURN_WARNING);
  }
  else
    return(EWDB_RETURN_SUCCESS);
}  /* End PostSetChanParamsExec() */
