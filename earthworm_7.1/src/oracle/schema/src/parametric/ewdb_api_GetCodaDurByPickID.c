/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_api_GetCodaDurByPickID.c,v 1.1 2005/06/17 20:59:20 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_api_GetCodaDurByPickID.c,v $
 *     Revision 1.1  2005/06/17 20:59:20  davidk
 *     New function to retrieve CodaDuration measurements by idPick.
 *
 *
 */

#include <ewdb_cli_base.h>
#include <ewdb_ora_api.h>
#include <ewdb_ew_oci_base.h>

static char SQL_STRING[] =
        "Begin "
        " :OUT_Retcode := 0; "
        " :OUT_SQLCode := 0; "
        " select idTCoda, tCodaTermObs, tCodaTermXtp, tCodaDurObs, tCodaDurXtp, "
        "        idCodaDur, idChan "
        "   into :OUT_idTCoda, :OUT_tCodaTermObs :OUT_tCodaTermXtp, "
        "        :OUT_tCodaDurObs, :OUT_tCodaDurXtp, :OUT_idCodaDur, :OUT_idChan " 
        "   from ALL_CODADUR_INFO "
        "   where idPick = :IN_idPick; "
        " EXCEPTION "
        "   WHEN NO_DATA_FOUND THEN "
        "     :OUT_RetCode := 1; "
        "   WHEN OTHERS THEN "
        "     :OUT_RetCode := -1; "
        "     :OUT_SQLCode := SQLCODE; "
        " END; ";

static EWDB_OCI_SFS SQLParamsBindArray[] =
{
  {0,1,0,0,0,OA_INT,   ":OUT_Retcode"},
  {0,1,0,0,0,OA_EWDBID,":IN_idPick"},
  {0,1,0,0,0,OA_EWDBID,":OUT_idTCoda"},
  {0,1,0,0,0,OA_DOUBLE,":OUT_tCodaTermObs"},
  {0,1,0,0,0,OA_DOUBLE,":OUT_tCodaTermXtp"},
  {0,1,0,0,0,OA_DOUBLE,":OUT_tCodaDurObs"},
  {0,1,0,0,0,OA_DOUBLE,":OUT_tCodaDurXtp"},
  {0,1,0,0,0,OA_EWDBID,":OUT_idCodaDur"},
  {0,1,0,0,0,OA_EWDBID,":OUT_idChan"},
  {0,1,0,0,0,OA_INT,   ":OUT_SQLCode"}
};

/* define the total number of fields to be bound */
#define NUM_FIELDS 10

/* Insertion Struct for SQL statement */
static EWDB_OCIStatementStruct SSStatement;

static EWDB_CodaDurationStruct Local_CodaDur;
static int                     Local_iRetCode, Local_iSQLCode;
static char                    Local_sztCodaTermObs[20], Local_sztCodaTermXtp[20],
                               Local_sztCodaDurObs[20], Local_sztCodaDurXtp[20];

/********************************
      FUNCTION PROTOTYPES
********************************/
static int PrepGetCodaDurByPickIDExec(EWDBid IN_idPick, EWDB_Cursor * ppCursor);
static int PostGetCodaDurByPickIDExec(EWDB_CodaDurationStruct * pCodaDur);
static int InitGetCodaDurByPickIDStatement(char * szStatement, EWDB_OCIStatementStruct *pSS);


/**********************************************************************
**********************************************************************/
int ewdb_api_GetCodaDurByPickID(EWDB_CodaDurationStruct * pCodaDur)
{

  EWDB_Cursor  pCursor;
  int          rc;

  ewdb_base_SetLastOraAPIActionTime();

  if(!pCodaDur || pCodaDur->idPick <= 0 )
  {
    logit("", "ewdb_api_GetCodaDurByPickID(): Invalid Params passed in.\n");
    return(EWDB_RETURN_FAILURE);
  }

  if( ewdb_base_Reconnect() != EWDB_RETURN_SUCCESS )
  {
    return( EWDB_RETURN_FAILURE );
  }

  if( PrepGetCodaDurByPickIDExec(pCodaDur->idPick, &pCursor) != EWDB_RETURN_SUCCESS)
  {
    return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }

  if( ewdb_base_SQLExecute(pCursor) )
  {
    ewdb_base_ErrorReport(hEWDBC, pCursor,"ewdb_api_GetCodaDurByPickID(): ewdb_base_SQLExecute",1);
    return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  } 

 /* Commit the transaction (all the previous inserts!)
  In Case there is any auditing or logging or debug
  changes made in the stored procedures.
  ****************************************************/
  if (ewdb_base_SQLCommit (hEWDBC))
  {
    ewdb_base_ErrorReport (hEWDBC, hEWDBC,"ewdb_api_GetCodaDurByPickID(): ewdb_base_SQLCommit",2);
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  }
 
  if( (rc=PostGetCodaDurByPickIDExec(pCodaDur)) == EWDB_RETURN_FAILURE)
  {
    if(rc == EWDB_RETURN_WARNING)
      return(EWDB_RETURN_FAILURE);
    else
      return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }

  ewdb_base_SetLastOraAPIActionTime();

  return(EWDB_RETURN_SUCCESS);
}  /* end ewdb_api_GetCodaDurByPickID() */


static int InitGetCodaDurByPickIDStatement(char * szStatement, EWDB_OCIStatementStruct *pSS)
{
	pSS->FieldArray[0].pVal = &Local_iRetCode;
	pSS->FieldArray[1].pVal = &Local_CodaDur.idPick;
	pSS->FieldArray[2].pVal = &Local_CodaDur.idTCoda;
	pSS->FieldArray[3].pVal = Local_sztCodaTermObs;
	pSS->FieldArray[4].pVal = Local_sztCodaTermXtp;
	pSS->FieldArray[5].pVal = Local_sztCodaDurObs;
	pSS->FieldArray[6].pVal = Local_sztCodaDurXtp;
	pSS->FieldArray[7].pVal = &Local_CodaDur.idCodaDur;
	pSS->FieldArray[8].pVal = &Local_CodaDur.idChan;
	pSS->FieldArray[9].pVal = &Local_iSQLCode;

  return(ewdb_base_RequestCursor(szStatement, pSS,0));
}


static int PrepGetCodaDurByPickIDExec(EWDBid IN_idPick, EWDB_Cursor * ppCursor)
{

  /* init the IN_idSource to the caller's value */
  memset(&Local_CodaDur, 0, sizeof(Local_CodaDur));
  memset(Local_sztCodaTermObs, 0, sizeof(Local_sztCodaTermObs));
  memset(Local_sztCodaTermXtp, 0, sizeof(Local_sztCodaTermXtp));
  memset(Local_sztCodaDurObs, 0, sizeof(Local_sztCodaDurObs));
  memset(Local_sztCodaDurXtp, 0, sizeof(Local_sztCodaDurXtp));
  Local_iSQLCode = Local_iRetCode = 0;

  /* copy users data to localbuffers*/
  Local_CodaDur.idPick = IN_idPick;

  SSStatement.NumOfFields=NUM_FIELDS;
  SSStatement.FieldArray=SQLParamsBindArray;
  SSStatement.RecordSize=0;

  if(InitGetCodaDurByPickIDStatement(SQL_STRING, &SSStatement) != EWDB_RETURN_SUCCESS)
  {
    *ppCursor = NULL;
    return(EWDB_RETURN_FAILURE);
  }

  *ppCursor=SSStatement.pCda;

  return(EWDB_RETURN_SUCCESS);
}  /* end PrepXXX() */


static int PostGetCodaDurByPickIDExec(EWDB_CodaDurationStruct * pCodaDur)
{

  ewdb_base_ReleaseCursor (SSStatement.pCda);

  if(Local_iRetCode == 0)
  {
    Local_CodaDur.tCodaTermObs = atof(Local_sztCodaTermObs);
    Local_CodaDur.tCodaTermXtp = atof(Local_sztCodaTermXtp);
    Local_CodaDur.tCodaDurObs = atof(Local_sztCodaDurObs);
    Local_CodaDur.tCodaDurXtp = atof(Local_sztCodaDurXtp);

    /* Copy the results to the users struct */
    memcpy(pCodaDur,&Local_CodaDur,sizeof(Local_CodaDur));

    return(EWDB_RETURN_SUCCESS);
  }
  else if(Local_iRetCode == 1)
  {
    return(EWDB_RETURN_WARNING);
  }
  else
  {
    logit("","PostGetCodaDurByPickIDExec():  SQL Proc failed with EXCEPTION %d.\n",
          Local_iSQLCode);
    return(EWDB_RETURN_WARNING);
  }
}   /* End PostXXX() */

