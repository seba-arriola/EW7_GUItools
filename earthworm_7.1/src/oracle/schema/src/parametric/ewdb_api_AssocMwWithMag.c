/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_api_AssocMwWithMag.c,v 1.4 2005/06/15 19:03:12 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_api_AssocMwWithMag.c,v $
 *     Revision 1.4  2005/06/15 19:03:12  davidk
 *     DB API Cleanup
 *
 *     Revision 1.3  2005/03/24 00:16:03  davidk
 *     Fixed bugs during initial testing.
 *
 *     Revision 1.2  2005/03/23 18:55:42  davidk
 *     Added a SQL_Commit call to have the transaction commit.  Otherwise it rolls
 *     back when the cursor is closed.  Every statement should have a commit,
 *     even queries that don't appear to touch anything.
 *
 *     Revision 1.1  2005/03/22 23:38:56  davidk
 *     Added API functions for Mw
 *
 *     Revision 1.1  2005/03/19 01:57:02  davidk
 *     Added some functions for Mw.
 *
 *
 *
 */

#include <ewdb_cli_base.h>
#include <ewdb_ora_api.h>
#include <ewdb_ew_oci_base.h>

static char SQL_STRING[] =
        "Begin Associate_Mw_With_Mag(OUT_Retcode => :OUT_Retcode, IN_idMw => :IN_idMw, "
        " IN_idMag => :IN_idMag); End;";

static EWDB_OCI_SFS SQLParamsBindArray[] =
{
  {0,1,0,0,0,OA_INT,":OUT_Retcode"},
  {0,1,0,0,0,OA_EWDBID,":IN_idMw"},
  {0,1,0,0,0,OA_EWDBID,":IN_idMag"}
};

/* define the total number of fields to be bound */
#define NUM_FIELDS 3

/* Insertion Struct for GetStationList statement */
static EWDB_OCIStatementStruct SSStatement;


static EWDBid Local_idMw, Local_idMag;
static int    Local_iRetcode;

/********************************
      FUNCTION PROTOTYPES
********************************/
static int PrepAssocMwWithMagExec(EWDBid idMw, EWDBid idMag, EWDB_Cursor * ppCursor);
static int PostAssocMwWithMagExec();
static int InitAssocMwWithMagStatement(char * szStatement, EWDB_OCIStatementStruct *pSS);


/**********************************************************************
**********************************************************************/
int ewdb_api_AssocMwWithMag(EWDBid idMw, EWDBid idMag)
{
  int rc;

  EWDB_Cursor  pCursor;
 
  if(!idMw || !idMag)
  {
    logit("et", "ERROR!  ewdb_api_AssocMwWithMag(): Null idMw(%d) or idMag(%d) passed as input! Returning!\n",
          idMw, idMag);
    return( EWDB_RETURN_FAILURE );
  }

  ewdb_base_SetLastOraAPIActionTime();

  if( ewdb_base_Reconnect() != EWDB_RETURN_SUCCESS )
  {
    return( EWDB_RETURN_FAILURE );
  }

  if( PrepAssocMwWithMagExec(idMw, idMag,&pCursor) != EWDB_RETURN_SUCCESS)
  {
    logit("","ewdb_api_AssocMwWithMag(): PrepAssocMwWithMagExec() failed!\n");
    return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }
  
  if( ewdb_base_SQLExecute(pCursor) )
  {
    ewdb_base_ErrorReport(hEWDBC, pCursor,"ewdb_api_AssocMwWithMag(): ewdb_base_SQLExecute",1);
    return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  } 

  if(ewdb_base_SQLCommit(hEWDBC))
  {
    ewdb_base_ErrorReport(hEWDBC, hEWDBC,
                      "ewdb_api_AssocMwWithMag(): ewdb_base_SQLCommit",2);
    return(ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }
  
  if((rc = PostAssocMwWithMagExec()) == EWDB_RETURN_FAILURE)
  {
    logit("","ewdb_api_AssocMwWithMag(): PostAssocMwWithMagExec() failed!\n");
    return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }

  ewdb_base_SetLastOraAPIActionTime();

  return(rc);
}  /* end ewdb_api_AssocMwWithMag() */


static int InitAssocMwWithMagStatement(char * szStatement, EWDB_OCIStatementStruct *pSS)
{
	pSS->FieldArray[0].pVal = &Local_iRetcode;
	pSS->FieldArray[1].pVal = &Local_idMw;
	pSS->FieldArray[2].pVal = &Local_idMag;

  return(ewdb_base_RequestCursor(szStatement, pSS,0/*don't force rebind*/));
}


static int PrepAssocMwWithMagExec(EWDBid idMw, EWDBid idMag, EWDB_Cursor * ppCursor)
{

  SSStatement.NumOfFields=NUM_FIELDS;
  SSStatement.FieldArray=SQLParamsBindArray;
  SSStatement.RecordSize=0;

  Local_idMw  = idMw;
  Local_idMag = idMag;

  if(InitAssocMwWithMagStatement(SQL_STRING, &SSStatement) != EWDB_RETURN_SUCCESS)
  {
    *ppCursor = NULL;
    return(EWDB_RETURN_FAILURE);
  }

  *ppCursor=SSStatement.pCda;

  return(EWDB_RETURN_SUCCESS);
}  /* end PrepXXX() */


static int PostAssocMwWithMagExec(EWDBid * pidMw)
{

  ewdb_base_ReleaseCursor (SSStatement.pCda);

  if(Local_iRetcode == 0)
  {
    return(EWDB_RETURN_SUCCESS);
  }
  else
  {
    if(Local_iRetcode == -2)
    {
      logit("","PostAssocMwWithMagExec(): SQL Proc Create_Mw() returned INVALID idMw(%d)\n",
            Local_idMw);  
    }
    else if(Local_iRetcode == -3)
    {
      logit("","PostAssocMwWithMagExec(): SQL Proc Create_Mw() returned INVALID idMag(%d)\n",
            Local_idMag);  
    }
    else
    {
      logit("","PostAssocMwWithMagExec(): SQL Proc Create_Mw(%d,%d) returned "
            "unexpected ERROR(%d).  See debug table for further details\n",
            Local_idMw, Local_idMag, Local_iRetcode);  
      return(EWDB_RETURN_FAILURE);
    }
    return(EWDB_RETURN_WARNING);
  }
}   /* End PostXXX() */

