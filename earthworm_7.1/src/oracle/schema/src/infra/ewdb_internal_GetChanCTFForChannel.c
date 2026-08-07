
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_internal_GetChanCTFForChannel.c,v 1.4 2005/06/21 20:24:52 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_internal_GetChanCTFForChannel.c,v $
 *     Revision 1.4  2005/06/21 20:24:52  davidk
 *     Fixed bugs in the SQL syntax, introduced during cleanup.
 *
 *     Revision 1.3  2005/06/10 16:27:59  davidk
 *     DB API Cleanup
 *
 *     Revision 1.2  2001/07/14 07:54:18  davidk
 *     Added code to issue a WARNING return code when the given ChanT doesn't
 *     have a transfer function.  This way the calling function can differentiate
 *     between a channel without a function, and an error in the process of retrieving
 *     a function.
 *
 *     Revision 1.1  2001/07/01 21:55:40  davidk
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
  "Begin "
  "  select dGain,dSampRate,idCTF "
  "   into :OUT_dGain,:OUT_dSampRate,:OUT_idCookedTF "
  "   from ALL_ChanCTF "
  "   where idChanT = :IN_idChanT; "
  "EXCEPTION "
  "  WHEN NO_DATA_FOUND THEN "
  "    :OUT_idCookedTF := 0; "
  "  WHEN OTHERS THEN "
  "    :OUT_idCookedTF := -1; "
  "End;";


static EWDB_OCI_SFS SQLParamsBindArray[] = 
{
  {0,1,0,0,0,OA_DOUBLE,":OUT_dGain"},
  {0,1,0,0,0,OA_DOUBLE,":OUT_dSampRate"},
  {0,1,0,0,0,OA_INT,":OUT_idCookedTF"},
  {0,1,0,0,0,OA_INT,":IN_idChanT"},
};

#define  NUM_FIELDS 4

static  EWDBid    Local_idChanT,Local_idCookedTF;
static  char Local_szGain[20],Local_szSampRate[20];

/* Statement Struct for GetChanCTFFC szStatement */
static EWDB_OCIStatementStruct SSStatement;


/********************************
      FUNCTION PROTOTYPES
********************************/
static int PrepGetChanCTFFCExec(EWDBid IN_idChanT, EWDB_Cursor * ppCursor);
static int PostGetChanCTFFCExec(EWDB_ChanTCTFStruct * pChanCTF);
static int InitGetChanCTFFCStatement(char *szStatement, 
                                     EWDB_OCIStatementStruct *pSS);

int ewdb_internal_GetChanCTFForChannel(EWDB_ChanTCTFStruct * pChanCTF)
{

/*  Caller fills in the EWDB_ChanTCTFStruct.idChanT field */
  EWDB_Cursor pCursor;
  int         rc;

  if (pChanCTF== NULL)
  {
    logit ("", "ewdb_internal_GetChanCTFForChannel():Null pointer passed in!\n");
    return EWDB_RETURN_FAILURE;
  }

  ewdb_base_SetLastOraAPIActionTime ();


  if (ewdb_base_Reconnect () != EWDB_RETURN_SUCCESS)
  {
    logit("", "ewdb_internal_GetChanCTFForChannel(): "
          "Could not reconnect to the database!\n");
    return (EWDB_RETURN_FAILURE);
  }

  if (PrepGetChanCTFFCExec (pChanCTF->idChanT, &pCursor) != EWDB_RETURN_SUCCESS)
  {
    logit ("", "ORA_API:ewdb_internal_GetChanCTFForChannel():PrepGetChanCTFFCExec() failed.\n");
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  }

  if (ewdb_base_SQLExecute (pCursor))
  {
    ewdb_base_ErrorReport (hEWDBC, pCursor,"ewdb_internal_GetChanCTFForChannel():ewdb_base_SQLExecute", 1);
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  } 

  
  /* Commit the transaction (all the previous inserts!)
     In Case there is any auditing or logging or debug
     changes made in the stored procedures.
  ****************************************************/
  if (ewdb_base_SQLCommit (hEWDBC))
  {
    ewdb_base_ErrorReport (hEWDBC, hEWDBC,"ewdb_internal_GetChanCTFForChannel():ewdb_base_SQLCommit",2);
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  }

  rc = PostGetChanCTFFCExec(pChanCTF);
  
  ewdb_base_SetLastOraAPIActionTime ();

  if (rc == EWDB_RETURN_FAILURE)
  {
    logit ("", "ewdb_internal_GetChanCTFForChannel(): "
           "Call to PostGetChanCTFFCExec failed!\n");
    return (EWDB_RETURN_FAILURE);
  }
  else
  {
    return(rc);
  }

}  /* end ewdb_internal_GetChanCTFForChannel() */


static int InitGetChanCTFFCStatement(char *szStatement, 
                                     EWDB_OCIStatementStruct *pSS)
{
  pSS->FieldArray[0].pVal = Local_szGain;
  pSS->FieldArray[1].pVal = Local_szSampRate;
  pSS->FieldArray[2].pVal = &Local_idCookedTF;
  pSS->FieldArray[3].pVal = &Local_idChanT;

  return(ewdb_base_RequestCursor (szStatement, pSS, 0));
}  /* InitGetChanCTFFCStatement() */


static int PrepGetChanCTFFCExec(EWDBid IN_idChanT, EWDB_Cursor * ppCursor) 
{

  SSStatement.NumOfFields = NUM_FIELDS;
  SSStatement.FieldArray = SQLParamsBindArray;
  SSStatement.RecordSize = 0;

  /* initialize output buffers */
  memset(Local_szGain, 0, sizeof(Local_szGain));
  memset(Local_szSampRate, 0, sizeof(Local_szSampRate));
  Local_idCookedTF = 0;

  /* copy user's data over to local vars */
  Local_idChanT=IN_idChanT;

  if (InitGetChanCTFFCStatement (SQL_STRING,
                           &SSStatement) != EWDB_RETURN_SUCCESS)
  {
    logit ("", "PrepGetChanCTFFCExec(): InitGetChanCTFFCStatement failed!\n");
    return EWDB_RETURN_FAILURE;
  }

  *ppCursor = SSStatement.pCda;
  
  return(EWDB_RETURN_SUCCESS);
}  /* end PrepGetChanCTFFCExec() */


static int PostGetChanCTFFCExec(EWDB_ChanTCTFStruct * pChanCTF)
{
  EWDB_Cursor pCursor;
  
  pCursor = SSStatement.pCda;

  ewdb_base_ReleaseCursor(pCursor);

  pChanCTF->dGain=atof(Local_szGain);
  pChanCTF->dSampRate=atof(Local_szSampRate);
  pChanCTF->tfsFunc.idCookedTF=Local_idCookedTF;

  if(Local_idCookedTF <= 0)
  {
    if(Local_idCookedTF == 0)
    {
      return(EWDB_RETURN_WARNING);
    }

    logit("","%s():ERROR! %d returned by SQL Query\n",
          "PostGetChanCTFFCExec",Local_idCookedTF);
    return(EWDB_RETURN_FAILURE);
  }

  return(EWDB_RETURN_SUCCESS);
}  /* PostGetChanCTFFCExec() */
