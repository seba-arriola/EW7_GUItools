
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_api_CreateTransformFunction.c,v 1.4 2005/06/10 16:27:58 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_api_CreateTransformFunction.c,v $
 *     Revision 1.4  2005/06/10 16:27:58  davidk
 *     DB API Cleanup
 *
 *     Revision 1.3  2003/12/03 00:23:29  davidk
 *     Reformatted file to match current template.  Non-functional changes ONLY.
 *
 *     Revision 1.2  2001/05/15 02:16:29  davidk
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
  "Begin Create_CTF(OUT_idCTF => :OUT_idCTF,"
  " OUT_idChanCTF => :OUT_idChanCTF, IN_sFuncDesc => :IN_sFuncDesc," 
  " IN_Poles => :IN_Poles, IN_Zeroes=>:IN_Zeroes,"
  " IN_idChanT => :IN_idChanT, IN_dGain=>:IN_dGain,"
  " IN_dSampRate=>:IN_dSampRate); End;";

static EWDB_OCI_SFS SQLParamsBindArray[] = 
{
  {0,1,0,0,0,OA_INT,":OUT_idCTF"},
  {0,1,0,0,0,OA_INT,":OUT_idChanCTF"},
  {0,1,0,0,0,OA_SZ,":IN_sFuncDesc"},
  {0,1,0,0,0,OA_SZ,":IN_Poles"},
  {0,1,0,0,0,OA_SZ,":IN_Zeroes"},
  {0,1,0,0,0,OA_INT,":IN_idChanT"},
  {0,1,0,0,0,OA_DOUBLE,":IN_dGain"},
  {0,1,0,0,0,OA_DOUBLE,":IN_dSampRate"}
};

#define  NUM_FIELDS 8

static EWDB_TransformFunctionStruct Local_CTF;

static EWDBid Local_idChanCTF=0;
static EWDBid Local_idChanT=0;

static char Local_szdGain[] = "0";
static char Local_szdSampRate[] = "0";
static char Local_szPoles[40*EWDB_MAX_POLES_OR_ZEROES];  /* enough to hold 40 Poles */
static char Local_szZeroes[40*EWDB_MAX_POLES_OR_ZEROES];  /* enough to hold 40 Zeroes */

/* Statement Struct for CreateTransFunc statement */
static EWDB_OCIStatementStruct SSStatement;


/********************************
      FUNCTION PROTOTYPES
********************************/
static int PrepCreateTransFuncExec(EWDB_TransformFunctionStruct * IN_pCTF,
                            EWDB_Cursor * ppCursor) ;
static int PostCreateTransFuncExec(EWDBid * pidCookedTF);
static int InitCreateTransFuncStatement(char *statement, 
                                 EWDB_OCIStatementStruct *pSS);

static int ConvertPZNum2String(EWDB_PZNum * PZNums, int iCount,
                               char * szNumString);

/* Creates a Transform Function that is used to convert recorded waveforms
   back into ground motion.  The transform function can be associated
   with 0 to many channels' time periods.
*********************************************************************/
int ewdb_api_CreateTransformFunction (EWDBid * pidCookedTF, 
                                      EWDB_TransformFunctionStruct * pCookedTF)
{

  EWDB_Cursor pCursor;

  if (pidCookedTF == NULL || pCookedTF== NULL)
  {
    logit ("", "ewdb_api_CreateTransformFunction(): Null pointer passed in!\n");
    return EWDB_RETURN_FAILURE;
  }

  ewdb_base_SetLastOraAPIActionTime ();


  if (ewdb_base_Reconnect () != EWDB_RETURN_SUCCESS)
  {
    logit("", "ewdb_api_CreateTransformFunction():  "
          "Could not reconnect to the database!\n");
    return (EWDB_RETURN_FAILURE);
  }

  if (PrepCreateTransFuncExec (pCookedTF, &pCursor) != EWDB_RETURN_SUCCESS)
  {
    logit ("", "ewdb_api_CreateTransformFunction(): PrepCreateTransFuncExec() failed.\n");
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  }

  if (ewdb_base_SQLExecute (pCursor))
  {
    ewdb_base_ErrorReport (hEWDBC, pCursor,"ewdb_api_CreateTransformFunction(): ewdb_base_SQLExecute", 1);
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  } 

    /* Commit the transaction (all the previous inserts!)
     In Case there is any auditing or logging or debug
     changes made in the stored procedures.
  ****************************************************/
  if (ewdb_base_SQLCommit (hEWDBC))
  {
    ewdb_base_ErrorReport (hEWDBC, hEWDBC,"ewdb_api_CreateTransformFunction(): ewdb_base_SQLCommit",2);
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  }

  if (PostCreateTransFuncExec (pidCookedTF) 
      != EWDB_RETURN_SUCCESS)
  {
    logit ("", "ewdb_api_CreateTransformFunction(): PostCreateTransFuncExec failed!\n");
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  }

  ewdb_base_SetLastOraAPIActionTime ();

  return(EWDB_RETURN_SUCCESS);
}  /* end ewdb_api_CreateTransformFunction() */


int InitCreateTransFuncStatement(char *statement, 
                                 EWDB_OCIStatementStruct *pSS)
{
  pSS->FieldArray[0].pVal = &(Local_CTF.idCookedTF);
  pSS->FieldArray[1].pVal = &Local_idChanCTF;
  pSS->FieldArray[2].pVal = Local_CTF.szCookedTFDesc;
  pSS->FieldArray[3].pVal = Local_szPoles;
  pSS->FieldArray[4].pVal = Local_szZeroes;
  pSS->FieldArray[5].pVal = &Local_idChanT;
  pSS->FieldArray[6].pVal = Local_szdGain;
  pSS->FieldArray[7].pVal = Local_szdSampRate;

  return(ewdb_base_RequestCursor (statement, pSS, 0));

}  /* End InitCreateTransFuncStatement() */


int PrepCreateTransFuncExec(EWDB_TransformFunctionStruct * IN_pCTF,
                            EWDB_Cursor * ppCursor) 
{
  int RetCode;

  SSStatement.NumOfFields = NUM_FIELDS;
  SSStatement.FieldArray = SQLParamsBindArray;
  SSStatement.RecordSize = 0;

  memcpy(&Local_CTF,IN_pCTF,sizeof(Local_CTF));

  RetCode=ConvertPZNum2String(IN_pCTF->Poles,
                              IN_pCTF->iNumPoles,Local_szPoles);
  if(RetCode != EWDB_RETURN_SUCCESS)
  { 
    logit ("", "PrepCreateTransFuncExec(): ConvertPZNum2String() failed "
           "for poles with rc%d!\n",RetCode);
    return EWDB_RETURN_FAILURE;
  }

  RetCode=ConvertPZNum2String(IN_pCTF->Zeroes,
                              IN_pCTF->iNumZeroes,Local_szZeroes);
  if(RetCode != EWDB_RETURN_SUCCESS)
  {
    logit ("", "PrepCreateTransFuncExec(): ConvertPZNum2String() failed "
           "for zeroes with rc%d!\n",RetCode);
    return EWDB_RETURN_FAILURE;
  }

  if (InitCreateTransFuncStatement (SQL_STRING,
                           &SSStatement) != EWDB_RETURN_SUCCESS)
  {
    logit ("", "PrepCreateTransFuncExec(): InitCreateTransFuncStatement failed!\n");
    return EWDB_RETURN_FAILURE;
  }

  *ppCursor = SSStatement.pCda;
  
  return(EWDB_RETURN_SUCCESS);
}  /* End PrepCreateTransFuncExec() */


int PostCreateTransFuncExec(EWDBid * pidCookedTF)
{
  EWDB_Cursor pCursor;
  
  pCursor = SSStatement.pCda;

  ewdb_base_ReleaseCursor(pCursor);

  *pidCookedTF = Local_CTF.idCookedTF;

  if(Local_CTF.idCookedTF <= 0)
  {
    logit("","%s():ERROR! %d returned by SQL Proc\n",
          "PostCreateTransFuncExec",Local_CTF.idCookedTF);
    return(EWDB_RETURN_WARNING);
  }

  return(EWDB_RETURN_SUCCESS);
}  /* End PostCreateTransFuncExec() */


/********************************************************************
********************************************************************/
static int ConvertPZNum2String(EWDB_PZNum * PZNums, int iCount,
                               char * szNumString)
{
  int i;
  char szTemp[100];

  szNumString[0] = 0x00;
  for(i=0; i<iCount; i++)
  {
    sprintf(szTemp,"%.7f %.7f ",PZNums[i].dReal,PZNums[i].dImag);
    strcat(szNumString,szTemp);
  }

  return(EWDB_RETURN_SUCCESS);
}


