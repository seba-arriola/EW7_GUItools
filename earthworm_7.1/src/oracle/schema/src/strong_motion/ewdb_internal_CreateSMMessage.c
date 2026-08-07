/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_internal_CreateSMMessage.c,v 
 *
 *    Revision history:
 *     $Log: ewdb_internal_CreateSMMessage.c,v $
 *     Revision 1.2  2003/06/09 23:05:29  davidk
 *     Removed useless debugging statements.
 *
 *     Revision 1.1  2001/05/15 02:16:42  davidk
 *     Initial revision
 *
 *     Revision 1.1  2001/04/06 19:01:00  davidk
 *     Initial revision
 *
 *
 *************************************************************/


#include <stdlib.h>
#include <stdio.h>
#include <ewdb_ora_api.h>
#include <ewdb_cli_base.h>
#include <ewdb_ew_oci_base.h>

static char SQL_STRING[] =
  "Begin Create_SMMessage(OUT_RetCode => :OUT_RetCode, "
  "OUT_idSMMessage => :OUT_idSMMessage, IN_idChan => :IN_idChan, "
  "IN_tMotion => :IN_tMotion, IN_tLoad => :IN_tLoad, "
  "IN_tAlt => :IN_tAlt, IN_iAltCode => :IN_iAltCode, "
  "IN_tPGA => :IN_tPGA, IN_tPGV => :IN_tPGV, "
  "IN_tPGD => :IN_tPGD, IN_idEvent => :IN_idEvent); End;";

  
static EWDB_OCI_SFS SQLParamsBindArray[] = 
{
  {0,1,0,0,0,OA_INT,   ":OUT_RetCode"},
  {0,1,0,0,0,OA_EWDBID,":OUT_idSMMessage"},
  {0,1,0,0,0,OA_EWDBID,":IN_idChan"},
  {0,1,0,0,0,OA_DOUBLE,":IN_tMotion"},
  {0,1,0,0,0,OA_DOUBLE,":IN_tLoad"},
  {0,1,0,0,0,OA_DOUBLE,":IN_tAlt"},
  {0,1,0,0,0,OA_INT,   ":IN_iAltCode"},
  {0,1,0,0,0,OA_DOUBLE,":IN_tPGA"},
  {0,1,0,0,0,OA_DOUBLE,":IN_tPGV"},
  {0,1,0,0,0,OA_DOUBLE,":IN_tPGD"},
  {0,1,0,0,0,OA_EWDBID,":IN_idEvent"}
};

#define  NUM_FIELDS  11

static int    iRetCode;
static EWDBid idSMMessage, idChan, idEvent;
static char   szTMotion[20], szTLoad[20], szTAlt[20];
static int    iAltCode;
static char   szTPGA[20], szTPGV[20], szTPGD[20];

/* Insertion Struct for SSCreateSMMessage statement */
static EWDB_OCIStatementStruct SSStatement;


/********************************
      FUNCTION PROTOTYPES
********************************/
int PrepCreateSMMessageExec(double IN_tMotion, double IN_tLoad, double IN_tAlt, 
                            int IN_iAltCode, EWDBid IN_idChan, 
                            double IN_tPGA, double IN_tPGV, double IN_tPGD, 
                            EWDBid IN_idEvent, EWDB_Cursor *ppCursor);
int PostCreateSMMessageExec (int * pRetCode, EWDBid * pidSMMessage);
int InitCreateSMMessageStatement (char *statement, 
                                  EWDB_OCIStatementStruct *pSS);
/*******************************/

int ewdb_internal_CreateSMMessage(EWDBid * pidSMMessage, double tMain, 
                             double tLoad, double tAlt, int iAltCode, 
                             double tPGA, double tPGV, double tPGD,
                             EWDBid idChan, EWDBid idEvent)
{
  /**************************
      Return Values:
          EWDB_RETURN_FAILURE: call failed
          EWDB_RETURN_SUCCESS: call succeeded
          others             : undefined
  **************************/

  EWDB_Cursor pCursor;
  EWDBid idSMMessage;
  int RetCode;

  ewdb_base_SetLastOraAPIActionTime ();

  if (ewdb_base_Reconnect () != EWDB_RETURN_SUCCESS)
  /* Re-establishes connection. */
  {
    logit("", "ewdb_internal_CreateSMMessage(): "
          "Could not reconnect to the database!\n");
    return (EWDB_RETURN_FAILURE);
  }

  if (PrepCreateSMMessageExec(tMain, tLoad, tAlt, iAltCode, 
                              idChan, tPGA, tPGV, tPGD, idEvent, &pCursor)
      != EWDB_RETURN_SUCCESS)
  {
    logit ("", "ewdb_internal_CreateSMMessage():PrepCreateSMMessage() failed.\n");
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  }

  if (ewdb_base_SQLExecute (pCursor))
  {
    ewdb_base_ErrorReport (hEWDBC, pCursor,"ewdb_internal_CreateSMMessage:ewdb_base_SQLExecute", 1);
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  } 
  
  /* Commit the transaction (all the previous inserts!)
     In Case there is any auditing or logging or debug
     changes made in the stored procedures.
  ****************************************************/
  if (ewdb_base_SQLCommit (hEWDBC))
  {
    ewdb_base_ErrorReport (hEWDBC, hEWDBC,"CreateSMMessage:ewdb_base_SQLCommit",2);
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  }

  if (PostCreateSMMessageExec(&RetCode, &idSMMessage) != EWDB_RETURN_SUCCESS)
  {
    logit ("", "Call to PostCreateSMMessageExec failed!\n");
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  }

  if(RetCode >= 0)
    *pidSMMessage = idSMMessage;
  else
    *pidSMMessage = 0;

  if(RetCode < 0)
    return(EWDB_RETURN_FAILURE);
  else if(RetCode == 0)
    return(EWDB_RETURN_SUCCESS);
  else
    return(EWDB_RETURN_WARNING);
}  /* end ewdb_internal_CreateSMMessage() */


int InitCreateSMMessageStatement (char *statement, EWDB_OCIStatementStruct *pSS)
{

  pSS->FieldArray[0].pVal = &iRetCode;
  pSS->FieldArray[1].pVal = &idSMMessage;
  pSS->FieldArray[2].pVal = &idChan;
  pSS->FieldArray[3].pVal = szTMotion;
  pSS->FieldArray[4].pVal = szTLoad;
  pSS->FieldArray[5].pVal = szTAlt;
  pSS->FieldArray[6].pVal = &iAltCode;
  pSS->FieldArray[7].pVal = szTPGA;
  pSS->FieldArray[8].pVal = szTPGV;
  pSS->FieldArray[9].pVal = szTPGD;
  pSS->FieldArray[10].pVal = &idEvent;

  ewdb_base_RequestCursor (statement, pSS, 0);

  return(EWDB_RETURN_SUCCESS);
}  /* End InitCreateSMMessageStatement() */


int PrepCreateSMMessageExec(double IN_tMotion, double IN_tLoad, double IN_tAlt, 
                            int IN_iAltCode, EWDBid IN_idChan, 
                            double IN_tPGA, double IN_tPGV, double IN_tPGD, 
                            EWDBid IN_idEvent, EWDB_Cursor *ppCursor)
{

  SSStatement.NumOfFields = NUM_FIELDS;
  SSStatement.FieldArray = SQLParamsBindArray;
  SSStatement.RecordSize = 0;


  /* copy the input params over to the local copies */
  sprintf (szTMotion, "%.4f", IN_tMotion);
  sprintf (szTLoad, "%.4f", IN_tLoad);
  sprintf (szTAlt, "%.4f", IN_tAlt);

  iAltCode = IN_iAltCode;

  idChan  = IN_idChan;

  sprintf (szTPGA, "%.4f", IN_tPGA);
  sprintf (szTPGV, "%.4f", IN_tPGV);
  sprintf (szTPGD, "%.4f", IN_tPGD);

  idEvent = IN_idEvent;

  /* call InitXXX() to prep a cursor */
  if (InitCreateSMMessageStatement (SQL_STRING,
                           &SSStatement) != EWDB_RETURN_SUCCESS)
  {
    logit ("", "Call to InitCreateSMMessageStatement failed!\n");
    return EWDB_RETURN_FAILURE;
  }

  /* record the cursor we got */
  *ppCursor = SSStatement.pCda;
  
  /* go home, happy! */
  return(EWDB_RETURN_SUCCESS);
}  /* End PrepCreateSMMessageExec() */


int PostCreateSMMessageExec (int * pRetCode, EWDBid * pidSMMessage)
{
  EWDB_Cursor pCursor;
  
  if (pidSMMessage == NULL || pRetCode == NULL)
  {
    logit ("", "%s(): Invalid arguments passed in.\n",
      "PostCreateSMMessageExec");
    return EWDB_RETURN_FAILURE;
  }
  
  if(iRetCode)
  {
    logit("t","SQL Proc Create_SMMessage() failed with return code %d\n",
      iRetCode);
    *pidSMMessage = 0;
  }
  else
  {
    *pidSMMessage = idSMMessage;
  }
  
  *pRetCode = iRetCode;
  
  pCursor = SSStatement.pCda;
  
  ewdb_base_ReleaseCursor (pCursor);
  
  return (EWDB_RETURN_SUCCESS);
}  /* End PostCreateSMMessageExec() */
