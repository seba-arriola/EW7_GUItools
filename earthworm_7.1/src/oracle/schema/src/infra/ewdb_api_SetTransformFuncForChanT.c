
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_api_SetTransformFuncForChanT.c,v 1.4 2005/06/10 16:27:59 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_api_SetTransformFuncForChanT.c,v $
 *     Revision 1.4  2005/06/10 16:27:59  davidk
 *     DB API Cleanup
 *
 *     Revision 1.3  2003/12/04 19:32:03  davidk
 *     Non functional changes, to make this function look like the current template,
 *     using SQL_STRING, SSStatement, and other standardized variable names.
 *
 *     Revision 1.2  2001/05/15 02:16:32  davidk
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
  "Begin Assoc_CTF(OUT_idChanCTF => :OUT_idChanCTF,"
  " IN_idCTF => :IN_idCTF, IN_idChanT => :IN_idChanT," 
  " IN_dGain => :IN_dGain, IN_dSampRate=>:IN_dSampRate); End;";

static EWDB_OCI_SFS SQLParamsBindArray[] = 
{
  {0,1,0,0,0,OA_INT,":OUT_idChanCTF"},
  {0,1,0,0,0,OA_INT,":IN_idCTF"},
  {0,1,0,0,0,OA_INT,":IN_idChanT"},
  {0,1,0,0,0,OA_DOUBLE,":IN_dGain"},
  {0,1,0,0,0,OA_DOUBLE,":IN_dSampRate"}
};

#define  NUM_FIELDS 5

static EWDBid Local_idChanCTF,Local_idCTF,Local_idChanT;
static char Local_szGain[20], Local_szSampRate[20];

/* Statement Struct for SetTransFuncForChanT szStatement */
static EWDB_OCIStatementStruct SSStatement;

/********************************
      FUNCTION PROTOTYPES
********************************/
static int PrepSetTransFuncForChanTExec(EWDBid IN_idChanT, EWDBid IN_idCTF,
                                double IN_dGain, double IN_dSampRate,
                                EWDB_Cursor * ppCursor) ;
static int PostSetTransFuncForChanTExec(void);
static int InitSetTransFuncForChanTStatement(char *szStatement, 
                                     EWDB_OCIStatementStruct *pSS);
                                     



/* Associates an existing transform function with a Channel for a 
   given time period.  Records the gain and samplerate that are 
   specific to that channel.
*********************************************************************/
int ewdb_api_SetTransformFuncForChanT(EWDBid IN_idChanT, double IN_dGain,
                                      EWDBid IN_idCookedTF, double IN_dSampRate)
{

  EWDB_Cursor pCursor;

  ewdb_base_SetLastOraAPIActionTime ();


  if (ewdb_base_Reconnect () != EWDB_RETURN_SUCCESS)
  {
    logit("", "ewdb_api_SetTransformFuncForChanT(): Could not reconnect to the database!\n");
    return (EWDB_RETURN_FAILURE);
  }

  if (PrepSetTransFuncForChanTExec(IN_idChanT, IN_idCookedTF, IN_dGain, 
                               IN_dSampRate, &pCursor) != EWDB_RETURN_SUCCESS)
  {
    logit ("", "ewdb_api_SetTransformFuncForChanT(): PrepSetTransformFuncForChanTExec() failed.\n");
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  }

  if (ewdb_base_SQLExecute (pCursor))
  {
    ewdb_base_ErrorReport (hEWDBC, pCursor,"ewdb_api_SetTransformFuncForChanT(): ewdb_base_SQLExecute", 1);
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  } 

  /* Commit the transaction (all the previous inserts!)
     In Case there is any auditing or logging or debug
     changes made in the stored procedures.
  ****************************************************/
  if (ewdb_base_SQLCommit (hEWDBC))
  {
    ewdb_base_ErrorReport (hEWDBC, hEWDBC,"ewdb_api_SetTransformFuncForChanT(): ewdb_base_SQLCommit",2);
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  }

  if (PostSetTransFuncForChanTExec() != EWDB_RETURN_SUCCESS)
  {
    logit ("", "ewdb_api_SetTransformFuncForChanT(): PostSetTransformFuncForChanTExec failed!\n");
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  }

  ewdb_base_SetLastOraAPIActionTime ();

  return(EWDB_RETURN_SUCCESS);
}  /* end ewdb_api_SetTransformFuncForChanT() */


static int InitSetTransFuncForChanTStatement(char *szStatement, 
                                             EWDB_OCIStatementStruct *pSS)
{
  pSS->FieldArray[0].pVal = &Local_idChanCTF;
  pSS->FieldArray[1].pVal = &Local_idCTF;
  pSS->FieldArray[2].pVal = &Local_idChanT;
  pSS->FieldArray[3].pVal = Local_szGain;
  pSS->FieldArray[4].pVal = Local_szSampRate;

  return(ewdb_base_RequestCursor (szStatement, pSS, 0));
}  /* End InitSetTransFuncForChanTStatement() */


static int PrepSetTransFuncForChanTExec(EWDBid IN_idChanT, EWDBid IN_idCTF,
                                        double IN_dGain, double IN_dSampRate,
                                        EWDB_Cursor * ppCursor) 
{

  SSStatement.NumOfFields = NUM_FIELDS;
  SSStatement.FieldArray = SQLParamsBindArray;
  SSStatement.RecordSize = 0;

  Local_idChanT = IN_idChanT;
  Local_idCTF = IN_idCTF;
  sprintf(Local_szGain,"%.6E",IN_dGain);
  sprintf(Local_szSampRate,"%.2f",IN_dSampRate);

  if (InitSetTransFuncForChanTStatement (SQL_STRING,
                           &SSStatement) != EWDB_RETURN_SUCCESS)
  {
    logit ("", "PrepSetTransFuncForChanTExec(): InitSetTransFuncForChanTStatement failed!\n");
    return EWDB_RETURN_FAILURE;
  }

  *ppCursor = SSStatement.pCda;
  
  return(EWDB_RETURN_SUCCESS);
}  /* End PrepSetTransFuncForChanTExec() */


static int PostSetTransFuncForChanTExec(void)
{
  EWDB_Cursor pCursor;
  
  pCursor = SSStatement.pCda;

  ewdb_base_ReleaseCursor(pCursor);

  if(Local_idChanCTF <= 0)
  {
    logit("","%s(%d/%d):ERROR! %d returned by SQL Proc\n",
          "PostSetTransFuncForChanTExec",
          Local_idChanT, Local_idCTF, Local_idChanCTF);
    return(EWDB_RETURN_FAILURE);
  }

  return(EWDB_RETURN_SUCCESS);
}  /* End PostSetTransFuncForChanTExec() */

