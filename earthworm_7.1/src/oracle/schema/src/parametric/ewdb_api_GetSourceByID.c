/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_api_GetSourceByID.c,v 1.4 2005/06/27 15:31:36 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_api_GetSourceByID.c,v $
 *     Revision 1.4  2005/06/27 15:31:36  davidk
 *     Fixed bugs introduced during global fetch/replace during DB cleanup.
 *
 *     Revision 1.3  2005/06/17 21:00:03  davidk
 *     Fixed problems in SQL code caused by errant global replacements during API cleanup.
 *
 *     Revision 1.2  2005/06/15 19:03:12  davidk
 *     DB API Cleanup
 *
 *     Revision 1.1  2004/09/27 21:50:45  davidk
 *     Added ewdb_api_GetSourceByID()
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
        " select sSource, sHumanReadable, idComment, idInst, iSourceType, sNote "
        "   into :OUT_sSource, :OUT_sHumanReadable, :OUT_idComment, "
        "        :OUT_idInst, :OUT_iSourceType, :OUT_sNote " 
        "   from ALL_SOURCE_INFO "
        "   where idSource = :IN_idSource; "
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
  {0,1,0,0,0,OA_EWDBID,":IN_idSource"},
  {0,1,0,0,0,OA_SZ,    ":OUT_sSource"},
  {0,1,0,0,0,OA_SZ,    ":OUT_sHumanReadable"},
  {0,1,0,0,0,OA_EWDBID,":OUT_idComment"},
  {0,1,0,0,0,OA_EWDBID,":OUT_idInst"},
  {0,1,0,0,0,OA_INT,   ":OUT_iSourceType"},
  {0,1,0,0,0,OA_SZ,    ":OUT_sNote"},
  {0,1,0,0,0,OA_INT,   ":OUT_SQLCode"}
};

/* define the total number of fields to be bound */
#define NUM_FIELDS 9

/* Insertion Struct for SQL statement */
static EWDB_OCIStatementStruct SSStatement;

static EWDB_AuthorStruct Local_Author;
static EWDBid            Local_idComment;
static int               Local_iRetCode, Local_iSQLCode;


/********************************
      FUNCTION PROTOTYPES
********************************/
static int PrepGetSourceByIDExec(EWDBid IN_idSource, EWDB_Cursor * ppCursor);
static int PostGetSourceByIDExec(EWDB_AuthorStruct * pAuthor);
static int InitGetSourceByIDStatement(char * szStatement, EWDB_OCIStatementStruct *pSS);


/**********************************************************************
**********************************************************************/
int ewdb_api_GetSourceByID(EWDB_AuthorStruct * pAuthor)
{

  EWDB_Cursor  pCursor;
  int          rc;

  ewdb_base_SetLastOraAPIActionTime();

  if(!pAuthor)
  {
    logit("", "ewdb_api_GetSourceByID(): NULL Params passed in.\n");
    return(EWDB_RETURN_FAILURE);
  }

  if( ewdb_base_Reconnect() != EWDB_RETURN_SUCCESS )
  {
    return( EWDB_RETURN_FAILURE );
  }

  if( PrepGetSourceByIDExec(pAuthor->idSource,&pCursor) != EWDB_RETURN_SUCCESS)
  {
    return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }

  if( ewdb_base_SQLExecute(pCursor) )
  {
    ewdb_base_ErrorReport(hEWDBC, pCursor,"ewdb_api_GetSourceByID(): ewdb_base_SQLExecute",1);
    return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  } 

 /* Commit the transaction (all the previous inserts!)
  In Case there is any auditing or logging or debug
  changes made in the stored procedures.
  ****************************************************/
  if (ewdb_base_SQLCommit (hEWDBC))
  {
    ewdb_base_ErrorReport (hEWDBC, hEWDBC,"ewdb_api_GetSourceByID(): ewdb_base_SQLCommit",2);
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  }
 
  if( (rc=PostGetSourceByIDExec(pAuthor)) == EWDB_RETURN_FAILURE)
  {
    if(rc == EWDB_RETURN_WARNING)
      return(EWDB_RETURN_FAILURE);
    else
      return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }

  ewdb_base_SetLastOraAPIActionTime();

  return(EWDB_RETURN_SUCCESS);
}  /* end ewdb_api_GetSourceByID() */


static int InitGetSourceByIDStatement(char * szStatement, EWDB_OCIStatementStruct *pSS)
{
	pSS->FieldArray[0].pVal = &Local_iRetCode;
	pSS->FieldArray[1].pVal = &Local_Author.idSource;
	pSS->FieldArray[2].pVal = Local_Author.szSource;
	pSS->FieldArray[3].pVal = Local_Author.szSourceName;
	pSS->FieldArray[4].pVal = &Local_idComment;
	pSS->FieldArray[5].pVal = &Local_Author.idInst;
	pSS->FieldArray[6].pVal = &Local_Author.iSourceType;
	pSS->FieldArray[7].pVal = Local_Author.szNote;
	pSS->FieldArray[8].pVal = &Local_iSQLCode;

  return(ewdb_base_RequestCursor(szStatement, pSS,0));
}


static int PrepGetSourceByIDExec(EWDBid IN_idSource, EWDB_Cursor * ppCursor)
{

  /* init the IN_idSource to the caller's value */
  Local_Author.idSource=IN_idSource;

  /* init the retun codes to 0 */
  Local_iSQLCode = Local_iRetCode = 0;

  SSStatement.NumOfFields=NUM_FIELDS;
  SSStatement.FieldArray=SQLParamsBindArray;
  SSStatement.RecordSize=0;

  if(InitGetSourceByIDStatement(SQL_STRING, &SSStatement) != EWDB_RETURN_SUCCESS)
  {
    *ppCursor = NULL;
    return(EWDB_RETURN_FAILURE);
  }

  *ppCursor=SSStatement.pCda;

  return(EWDB_RETURN_SUCCESS);
}  /* end PrepXXX() */


static int PostGetSourceByIDExec(EWDB_AuthorStruct * pAuthor)
{

  ewdb_base_ReleaseCursor (SSStatement.pCda);

  if(Local_iRetCode == 0)
  {
    /* Copy the results to the users struct */
    memcpy(pAuthor,&Local_Author,sizeof(Local_Author));

    return(EWDB_RETURN_SUCCESS);
  }
  else if(Local_iRetCode == 1)
  {
    return(EWDB_RETURN_WARNING);
  }
  else
  {
    logit("","PostGetSourceByIDExec():  SQL Proc failed with EXCEPTION %d.\n",
          Local_iSQLCode);
    return(EWDB_RETURN_WARNING);
  }
}   /* End PostXXX() */

