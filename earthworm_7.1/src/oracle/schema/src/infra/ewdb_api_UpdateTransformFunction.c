
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_api_UpdateTransformFunction.c,v 1.2 2005/06/10 16:27:59 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_api_UpdateTransformFunction.c,v $
 *     Revision 1.2  2005/06/10 16:27:59  davidk
 *     DB API Cleanup
 *
 *     Revision 1.1  2003/12/03 00:23:29  davidk
 *     Initial revision
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
  "Begin Update_CTF(OUT_Retcode => :OUT_Retcode,"
  " IN_idCTF => :IN_idCTF, IN_sFuncDesc => :IN_sFuncDesc," 
  " IN_Poles => :IN_Poles, IN_Zeroes=>:IN_Zeroes,"
  " IN_bUpdatePZ => :IN_bUpdatePZ); End;";

static EWDB_OCI_SFS SQLParamsBindArray[] = 
{
  {0,1,0,0,0,OA_INT,":OUT_Retcode"},
  {0,1,0,0,0,OA_EWDBID,":IN_idCTF"},
  {0,1,0,0,0,OA_SZ,":IN_sFuncDesc"},
  {0,1,0,0,0,OA_SZ,":IN_Poles"},
  {0,1,0,0,0,OA_SZ,":IN_Zeroes"},
  {0,1,0,0,0,OA_INT,":IN_bUpdatePZ"}
};

#define  NUM_FIELDS 6

static EWDB_TransformFunctionStruct Local_CTF;

static int  Local_iRetCode;
static int  Local_bUpdatePZ;
static char Local_szPoles[40*EWDB_MAX_POLES_OR_ZEROES];  /* enough to hold 40 Poles */
static char Local_szZeroes[40*EWDB_MAX_POLES_OR_ZEROES];  /* enough to hold 40 Zeroes */

/* Statement Struct for UpdateTransFunc szStatement */
static EWDB_OCIStatementStruct SSStatement;


/********************************
      FUNCTION PROTOTYPES
********************************/
static int PrepUpdateTransFuncExec(EWDB_TransformFunctionStruct * IN_pCTF,
                            int IN_bUpdatePZ, EWDB_Cursor * ppCursor) ;
static int PostUpdateTransFuncExec(EWDBid * pidCookedTF);
static int InitUpdateTransFuncStatement(char *szStatement, 
                                 EWDB_OCIStatementStruct *pSS);

static int ConvertPZNum2String(EWDB_PZNum * PZNums, int iCount,
                               char * szNumString);

/* Updates a Transform Function that is used to convert recorded waveforms
   back into ground motion.  The transform function can be associated
   with 0 to many channels' time periods.
*********************************************************************/
int ewdb_api_UpdateTransformFunction(EWDB_TransformFunctionStruct * pCookedTF,
                                     int IN_bUpdatePZ)
{

  EWDB_Cursor pCursor;

  if (pCookedTF== NULL)
  {
    logit ("", "ewdb_api_UpdateTransformFunction(): Null pointer passed in!\n");
    return EWDB_RETURN_FAILURE;
  }

  ewdb_base_SetLastOraAPIActionTime ();


  if (ewdb_base_Reconnect () != EWDB_RETURN_SUCCESS)
  {
    logit("", "ewdb_api_UpdateTransformFunction(): Could not reconnect to the database!\n");
    return (EWDB_RETURN_FAILURE);
  }

  if (PrepUpdateTransFuncExec (pCookedTF, IN_bUpdatePZ, &pCursor) != EWDB_RETURN_SUCCESS)
  {
    logit ("", "ewdb_api_UpdateTransformFunction(): PrepUpdateTransFuncExec() failed.\n");
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  }

  if (ewdb_base_SQLExecute (pCursor))
  {
    ewdb_base_ErrorReport (hEWDBC, pCursor,"ewdb_api_UpdateTransformFunction(): ewdb_base_SQLExecute", 1);
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  } 
  
  /* Commit the transaction (all the previous inserts!)
     In Case there is any auditing or logging or debug
     changes made in the stored procedures.
  ****************************************************/
  if (ewdb_base_SQLCommit (hEWDBC))
  {
    ewdb_base_ErrorReport (hEWDBC, hEWDBC,"ewdb_api_UpdateTransformFunction(): ewdb_base_SQLCommit",2);
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  }

  if (PostUpdateTransFuncExec (&pCookedTF->idCookedTF) 
      != EWDB_RETURN_SUCCESS)
  {
    logit ("", "ewdb_api_UpdateTransformFunction(): PostUpdateTransFuncExec failed!\n");
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  }

  ewdb_base_SetLastOraAPIActionTime ();

  return(EWDB_RETURN_SUCCESS);
}  /* end ewdb_api_UpdateTransformFunction() */


static int InitUpdateTransFuncStatement(char *szStatement, 
                                        EWDB_OCIStatementStruct *pSS)
{
  pSS->FieldArray[0].pVal = &Local_iRetCode;
  pSS->FieldArray[1].pVal = &Local_CTF.idCookedTF;
  pSS->FieldArray[2].pVal = Local_CTF.szCookedTFDesc;
  pSS->FieldArray[3].pVal = Local_szPoles;
  pSS->FieldArray[4].pVal = Local_szZeroes;
  pSS->FieldArray[5].pVal = &Local_bUpdatePZ;

  return(ewdb_base_RequestCursor (szStatement, pSS, 0));
}  /* End InitUpdateTransFuncStatement() */


static int PrepUpdateTransFuncExec(EWDB_TransformFunctionStruct * IN_pCTF,
                                   int IN_bUpdatePZ, EWDB_Cursor * ppCursor)
{
  int RetCode;

  SSStatement.NumOfFields = NUM_FIELDS;
  SSStatement.FieldArray = SQLParamsBindArray;
  SSStatement.RecordSize = 0;

  /* copy user data to local vars */
  memcpy(&Local_CTF,IN_pCTF,sizeof(Local_CTF));
  Local_bUpdatePZ = IN_bUpdatePZ;

  RetCode=ConvertPZNum2String(IN_pCTF->Poles,
                              IN_pCTF->iNumPoles,Local_szPoles);
  if(RetCode != EWDB_RETURN_SUCCESS)
  { 
    logit ("", "PrepUpdateTransFuncExec(): ConvertPZNum2String(%d) failed "
                "for poles with rc%d!\n",
           Local_CTF.idCookedTF, RetCode);
    return EWDB_RETURN_FAILURE;
  }

  RetCode=ConvertPZNum2String(IN_pCTF->Zeroes,
                              IN_pCTF->iNumZeroes,Local_szZeroes);
  if(RetCode != EWDB_RETURN_SUCCESS)
  {
    logit ("", "PrepUpdateTransFuncExec(): ConvertPZNum2String(%d) failed "
                "for zeroes with rc%d!\n",
           Local_CTF.idCookedTF, RetCode);
    return EWDB_RETURN_FAILURE;
  }

  if (InitUpdateTransFuncStatement (SQL_STRING,
                           &SSStatement) != EWDB_RETURN_SUCCESS)
  {
    logit ("", "PrepUpdateTransFuncExec(): InitUpdateTransFuncStatement failed!\n");
    return EWDB_RETURN_FAILURE;
  }

  *ppCursor = SSStatement.pCda;
  
  return(EWDB_RETURN_SUCCESS);
}  /* End PrepUpdateTransFuncExec() */


static int PostUpdateTransFuncExec(EWDBid * pidCookedTF)
{
  EWDB_Cursor pCursor;
  
  pCursor = SSStatement.pCda;

  ewdb_base_ReleaseCursor(pCursor);

  *pidCookedTF = Local_CTF.idCookedTF;

  if(Local_iRetCode < 0)
  {
    logit("","%s():ERROR! %d returned by SQL Proc %s()\n",
          "PostUpdateTransFuncExec",Local_iRetCode, "Update_CTF");
    return(EWDB_RETURN_FAILURE);
  }

  return(EWDB_RETURN_SUCCESS);
}  /* End PostUpdateTransFuncExec() */


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


