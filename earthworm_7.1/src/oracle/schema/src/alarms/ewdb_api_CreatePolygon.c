/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_api_CreatePolygon.c,v 1.2 2005/06/03 22:32:16 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_api_CreatePolygon.c,v $
 *     Revision 1.2  2005/06/03 22:32:16  davidk
 *     DB API Cleanup
 *
 *     Revision 1.1  2004/11/23 16:57:49  mark
 *     Initial checkin
 *
 */


#include <ewdb_ora_api.h>
#include <ewdb_cli_base.h>
#include <ewdb_ew_oci_base.h>


static char SQL_STRING[] =
  "Begin Create_Polygon(OUT_idPolygon => :OUT_idPolygon,"
  "                     IN_sPolygonName => :IN_sPolygonName); End;";

static EWDB_OCI_SFS SQLParamsBindArray[] = 
{
  {0,1,0,0,0,OA_INT,":OUT_idPolygon"},
  {0,1,0,0,0,OA_SZ,":IN_sPolygonName"}
};

#define  NUM_FIELDS  2


/* Insertion Struct for CreatePolygon szStatement */
static EWDB_OCIStatementStruct SSStatement;

static  EWDB_PolygonStruct  LocalPolygonStruct;

static int PrepCreatePolygonExec(EWDB_PolygonStruct *pPolygon, EWDB_Cursor *ppCursor);
static int PostCreatePolygonExec(EWDB_PolygonStruct *pPolygon);
static int InitCreatePolygonStatement(char *szStatement, EWDB_OCIStatementStruct *pSS);


int ewdb_api_CreatePolygon(EWDB_PolygonStruct *pPolygon)
{
  EWDB_Cursor pCursor;
  int rc;

  if (pPolygon == NULL)
  {
    logit ("", "ewdb_api_CreatePolygon(): Null parameters passed in!\n");
    return EWDB_RETURN_FAILURE;
  }

  ewdb_base_SetLastOraAPIActionTime ();

  if (ewdb_base_Reconnect () != EWDB_RETURN_SUCCESS)
  /* Establishes connection, and performs binding!?! */
  {
    logit ("", "ewdb_api_CreatePolygon(): Could not reconnect to the database!\n");
    return (EWDB_RETURN_FAILURE);
  }

  if (PrepCreatePolygonExec (pPolygon, &pCursor) != EWDB_RETURN_SUCCESS)
  {
    logit ("", "ewdb_api_CreatePolygon(): PrepCreatePolygonExec() failed.\n");
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  }

  if (ewdb_base_SQLExecute (pCursor))
  {
    ewdb_base_ErrorReport (hEWDBC, pCursor,"ewdb_api_CreatePolygon(): ewdb_base_SQLExecute", 1);
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  } 

  /* Commit the transaction (all the previous inserts!)
     In Case there is any auditing or logging or debug
     changes made in the stored procedures.
  ****************************************************/
  if (ewdb_base_SQLCommit (hEWDBC))
  {
    ewdb_base_ErrorReport (hEWDBC, hEWDBC,"ewdb_api_CreatePolygon(): ewdb_base_SQLCommit",2);
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  }
  
  rc = PostCreatePolygonExec(pPolygon);
  if(rc == EWDB_RETURN_FAILURE)
  {
    logit("", "ewdb_api_CreatePolygon(): PostCreatePolygonExec failed!\n");
    return(ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }
  else if(rc == EWDB_RETURN_WARNING)
  {
    return(EWDB_RETURN_FAILURE);
  }

  return(EWDB_RETURN_SUCCESS);
}


static int InitCreatePolygonStatement (char *szStatement, EWDB_OCIStatementStruct *pSS)
{

  if ((szStatement == NULL) || (pSS == NULL))
  {
    logit ("", "InitCreatePolygonStatement(): Null parameters passed in!\n");
    return EWDB_RETURN_FAILURE;
  }

  pSS->FieldArray[0].pVal = &(LocalPolygonStruct.idPolygon);
  pSS->FieldArray[1].pVal = LocalPolygonStruct.szPolygonName;

  if (ewdb_base_RequestCursor (szStatement, pSS, 0) != 0)
  {
    logit ("", "InitCreatePolygonStatement(): Call to ewdb_base_RequestCursor failed.\n");
    return EWDB_RETURN_FAILURE;
  }

  return EWDB_RETURN_SUCCESS;
}


static int PrepCreatePolygonExec(EWDB_PolygonStruct *pPolygon, EWDB_Cursor *ppCursor)
{
  if ((pPolygon == NULL) || (ppCursor == NULL))
  {
    logit ("", "PrepCreatePolygonExec(): Null parameters passed in!\n");
    return EWDB_RETURN_FAILURE;
  }

  SSStatement.NumOfFields = NUM_FIELDS;
  SSStatement.FieldArray = SQLParamsBindArray;
  SSStatement.RecordSize = 0;

  /* Copy the incoming struct into the local struct */
  memcpy (&LocalPolygonStruct, pPolygon, sizeof(EWDB_PolygonStruct));

  if(InitCreatePolygonStatement (SQL_STRING, &SSStatement)
     != EWDB_RETURN_SUCCESS)
  {
    logit ("", "PrepCreatePolygonExec(): InitCreatePolygonStatement failed!\n");
    return EWDB_RETURN_FAILURE;
  }

  *ppCursor = SSStatement.pCda;
  
  return EWDB_RETURN_SUCCESS;
}


static int PostCreatePolygonExec(EWDB_PolygonStruct *pPolygon)
{
  EWDB_Cursor pCursor;

  if (pPolygon == NULL)
  {
    logit ("", "PostCreatePolygonExec(): Invalid arguments passed in.\n");
    return EWDB_RETURN_FAILURE;
  }

  pPolygon->idPolygon = LocalPolygonStruct.idPolygon;

  pCursor = SSStatement.pCda;

  ewdb_base_ReleaseCursor (pCursor);

  if(pPolygon->idPolygon <= 0)
  {
    if(pPolygon->idPolygon == -1)
    {
      logit("","PostCreatePolygonExec():  SQL Proc Create_Polygon() returned "
          "\"Unknown error\".  See SQL debug table for details.\n");
    }
    else
    {
      logit("","PostCreatePolygonExec():  SQL Proc Create_Polygon() returned "
          "the following error(%d).  Please see that proc for details.\n",
          pPolygon->idPolygon);
    }
    return(EWDB_RETURN_WARNING);
  }

  return (EWDB_RETURN_SUCCESS);
}  /* end PostCreatePolygonExec() */
