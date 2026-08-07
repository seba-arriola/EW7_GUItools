
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_api_CreateOrUpdateChanParams.c,v 1.2 2005/06/10 16:27:58 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_api_CreateOrUpdateChanParams.c,v $
 *     Revision 1.2  2005/06/10 16:27:58  davidk
 *     DB API Cleanup
 *
 *     Revision 1.1  2004/04/06 18:07:25  davidk
 *     Function that forces the setting of a channel's parameters for a given time window.
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
  "Begin Create_ChanT(OUT_RetCode => :OUT_RetCode,"
  " OUT_idChanT => :OUT_idChanT,"
  " IN_idChan => :IN_idChan,"
  " IN_idCompT => :IN_idCompT," 
  " IN_tOn => :IN_tOn, IN_tOff=>:IN_tOff,"
  " IN_idDeviceSlot => :IN_idDeviceSlot,"
  " IN_iPlexor => :IN_iPlexor,"
  " IN_sComment => :IN_sComment,"
  " IN_bForce => 1/*TRUE*/); End;";

static EWDB_OCI_SFS SQLParamsBindArray[] = 
{
  {0,1,0,0,0,OA_INT,   ":OUT_RetCode"},
  {0,1,0,0,0,OA_EWDBID,":OUT_idChanT"},
  {0,1,0,0,0,OA_EWDBID,":IN_idChan"},
  {0,1,0,0,0,OA_EWDBID,":IN_idCompT"},
  {0,1,0,0,0,OA_DOUBLE,":IN_tOn"},
  {0,1,0,0,0,OA_DOUBLE,":IN_tOff"},
  {0,1,0,0,0,OA_EWDBID,":IN_idDeviceSlot"},
  {0,1,0,0,0,OA_INT,":IN_iPlexor"},
  {0,1,0,0,0,OA_SZ,":IN_sComment"}
};

#define  NUM_FIELDS 9

static EWDB_ChannelStruct Local_ChanStruct;
static char Local_szTOn[20],Local_szTOff[20];
static char Local_szComment[4001] = "";
static int Local_iRetCode;

static int idDeviceSlot = 0;
static int iPlexor = 0;

/* Statement Struct for Set_Chan_Params statement */
static EWDB_OCIStatementStruct SSStatement;

/********************************
      FUNCTION PROTOTYPES
********************************/
static int PrepCreateOrUpdateChanParamsExec(EWDB_ChannelStruct * IN_pChan, EWDB_Cursor * ppCursor);
static int PostCreateOrUpdateChanParamsExec(EWDBid * pidChanT);
static int InitCreateOrUpdateChanParamsStatement(char *statement, EWDB_OCIStatementStruct *pSS);



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
   
******************************************************************/
int ewdb_api_CreateOrUpdateChanParams(EWDB_ChannelStruct * pChan)
{

  EWDB_Cursor pCursor;

  if (pChan == NULL)
  {
    logit ("", "EWDB_CreateOrUpdateChanParams(): Null pointer passed in!\n");
    return EWDB_RETURN_FAILURE;
  }

  ewdb_base_SetLastOraAPIActionTime ();


  if (ewdb_base_Reconnect () != EWDB_RETURN_SUCCESS)
  {
    logit("", "ewdb_api_CreateOrUpdateChanParams(): "
          "Could not reconnect to the database!\n");
    return (EWDB_RETURN_FAILURE);
  }

  /* CHANGED 05/10/2000 DK: Order of the function parameters
     was incorrect.  I corrected it.
  **********************************************************/
  if (PrepCreateOrUpdateChanParamsExec (pChan, &pCursor) != EWDB_RETURN_SUCCESS)
  {
    logit ("", "ewdb_api_CreateOrUpdateChanParams():PrepCreateOrUpdateChanParamsExec() failed.\n");
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  }

  if (ewdb_base_SQLExecute (pCursor))
  {
    ewdb_base_ErrorReport (hEWDBC, pCursor,"ewdb_api_CreateOrUpdateChanParams():ewdb_base_SQLExecute", 1);
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  } 

  
  /* Commit the transaction (all the previous inserts!)
     In Case there is any auditing or logging or debug
     changes made in the stored procedures.
  ****************************************************/
  if (ewdb_base_SQLCommit (hEWDBC))
  {
    ewdb_base_ErrorReport (hEWDBC, hEWDBC,"ewdb_api_CreateOrUpdateChanParams():ewdb_base_SQLCommit",2);
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  }

  if (PostCreateOrUpdateChanParamsExec (&pChan->idChanT) 
      == EWDB_RETURN_FAILURE)
  {
    logit ("", "ewdb_api_CreateOrUpdateChanParams(): "
           "Call to PostCreateOrUpdateChanParamsExec failed!\n");
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  }
  else if (PostCreateOrUpdateChanParamsExec (&pChan->idChanT) 
           == EWDB_RETURN_WARNING)
  {
    return(EWDB_RETURN_FAILURE);
  }
  ewdb_base_SetLastOraAPIActionTime ();

  return(EWDB_RETURN_SUCCESS);
}  


static int InitCreateOrUpdateChanParamsStatement(char *statement, EWDB_OCIStatementStruct *pSS)
{
 
  pSS->FieldArray[0].pVal = &Local_iRetCode;
  pSS->FieldArray[1].pVal = &Local_ChanStruct.idChanT;
  pSS->FieldArray[2].pVal = &Local_ChanStruct.Comp.idChan;
  pSS->FieldArray[3].pVal = &Local_ChanStruct.idCompT;
  pSS->FieldArray[4].pVal = Local_szTOn;
  pSS->FieldArray[5].pVal = Local_szTOff;
  pSS->FieldArray[6].pVal = &idDeviceSlot;
  pSS->FieldArray[7].pVal = &iPlexor;
  pSS->FieldArray[8].pVal = Local_szComment;

  return(ewdb_base_RequestCursor (statement, pSS, 0));
}  /* End InitCreateOrUpdateChanParamsStatement() */


static int PrepCreateOrUpdateChanParamsExec(EWDB_ChannelStruct * IN_pChan, EWDB_Cursor * ppCursor)
{

  SSStatement.NumOfFields = NUM_FIELDS;
  SSStatement.FieldArray = SQLParamsBindArray;
  SSStatement.RecordSize = 0;

  memcpy(&Local_ChanStruct, IN_pChan, sizeof(Local_ChanStruct));

  /* When the infrastructure schema is created and idDevices
     are permitted to be something other than NULL, then
     uncomment the following line. DavidK 2000/05/10 
  *********************************************************/
  /* idDevice = IN_idDevice */

  sprintf(Local_szTOn,"%.2f",Local_ChanStruct.tOn);
  sprintf(Local_szTOff,"%.2f",Local_ChanStruct.tOff);

  if (InitCreateOrUpdateChanParamsStatement (SQL_STRING,
                           &SSStatement) != EWDB_RETURN_SUCCESS)
  {
    logit ("", "Call to InitCreateOrUpdateChanParamsStatement failed!\n");
    return EWDB_RETURN_FAILURE;
  }

  *ppCursor = SSStatement.pCda;
  
  return(EWDB_RETURN_SUCCESS);
}  /* End PrepCreateOrUpdateChanParamsExec() */


static int PostCreateOrUpdateChanParamsExec(EWDBid * pidChanT)
{
  EWDB_Cursor pCursor;
  
  pCursor = SSStatement.pCda;
  
  ewdb_base_ReleaseCursor(pCursor);

  if(!pidChanT)
  {
    logit("","PostCreateOrUpdateChanParamsExec(): Null Parameters passed in!\n");
    return(EWDB_RETURN_FAILURE);
  }

  *pidChanT=Local_ChanStruct.idChanT;
  
  if(Local_iRetCode != 0)
  {
    logit("","%s(): Warning SQL Proc %s() returned error(%d)\n",
          "PostCreateOrUpdateChanParamsExec", "Create_ChanT", Local_iRetCode);
    return(EWDB_RETURN_WARNING);
  }
  else
    return(EWDB_RETURN_SUCCESS);
}  /* End PostCreateOrUpdateChanParamsExec() */
