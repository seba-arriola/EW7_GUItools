/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_api_CreatePolygonVert.c,v 1.5 2005/06/03 22:32:16 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_api_CreatePolygonVert.c,v $
 *     Revision 1.5  2005/06/03 22:32:16  davidk
 *     DB API Cleanup
 *
 *     Revision 1.4  2004/12/02 19:01:39  mark
 *     Rolled back to use strings (which apparently works this week)
 *
 *     Revision 1.2  2004/11/24 18:03:00  mark
 *     Fixed SQL error; borrowed idea of strings for floats from CreateOrigin
 *
 *     Revision 1.1  2004/11/23 16:57:49  mark
 *     Initial checkin
 *
 */


#include <ewdb_ora_api.h>
#include <ewdb_cli_base.h>
#include <ewdb_ew_oci_base.h>


static char SQL_STRING[] =
  "Begin Create_PolygonVert(OUT_idVertex => :OUT_idVertex,"
  "                         IN_idPolygon => :IN_idPolygon,"
  "                         IN_dLat => :IN_dLat,"
  "                         IN_dLon => :IN_dLon,"
  "                         IN_iOrder => :IN_iOrder); End;";

static EWDB_OCI_SFS SQLParamsBindArray[] = 
{
  {0,1,0,0,0,OA_INT,":OUT_idVertex"},
  {0,1,0,0,0,OA_INT,":IN_idPolygon"},
  {0,1,0,0,0,OA_FLOAT,":IN_dLat"},
  {0,1,0,0,0,OA_FLOAT,":IN_dLon"},
  {0,1,0,0,0,OA_INT,":IN_iOrder"},
};

#define  NUM_FIELDS  5

static char  Local_szLat[20], Local_szLon[20];

/* Insertion Struct for CreatePolygon szStatement */
static EWDB_OCIStatementStruct SSStatement;

static  EWDB_PolygonVertexStruct  Local_PolygonVertStruct;

static int PrepCreatePolygonVertExec(EWDB_PolygonVertexStruct *pPolygonVert, EWDB_Cursor *ppCursor);
static int PostCreatePolygonVertExec(EWDBid *pidVertex);
static int InitCreatePolygonVertStatement(char *szStatement, EWDB_OCIStatementStruct *pSS);


int ewdb_api_CreatePolygonVert(EWDB_PolygonVertexStruct *pPolygonVert)
{
  EWDB_Cursor pCursor;
  int rc;

  if (pPolygonVert == NULL)
  {
    logit ("", "ewdb_api_CreatePolygonVert(): Null parameters passed in!\n");
    return EWDB_RETURN_FAILURE;
  }

  ewdb_base_SetLastOraAPIActionTime ();

  if (ewdb_base_Reconnect () != EWDB_RETURN_SUCCESS)
  /* Establishes connection, and performs binding!?! */
  {
    logit ("", "ewdb_api_CreatePolygonVert(): Could not reconnect to the database!\n");
    return (EWDB_RETURN_FAILURE);
  }

  if (PrepCreatePolygonVertExec (pPolygonVert, &pCursor) != EWDB_RETURN_SUCCESS)
  {
    logit ("", "ewdb_api_CreatePolygonVert(): PrepCreatePolygonVertExec() failed.\n");
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  }

  if (ewdb_base_SQLExecute (pCursor))
  {
    ewdb_base_ErrorReport (hEWDBC, pCursor,"ewdb_api_CreatePolygonVert(): ewdb_base_SQLExecute", 1);
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  } 

  /* Commit the transaction (all the previous inserts!)
     In Case there is any auditing or logging or debug
     changes made in the stored procedures.
  ****************************************************/
  if (ewdb_base_SQLCommit (hEWDBC))
  {
    ewdb_base_ErrorReport (hEWDBC, hEWDBC,"ewdb_api_CreatePolygonVert(): ewdb_base_SQLCommit",2);
    return (ewdb_base_Disconnect (EWDB_RETURN_FAILURE));
  }
  
  rc = PostCreatePolygonVertExec(&pPolygonVert->idVertex);
  if(rc == EWDB_RETURN_FAILURE)
  {
    logit("", "ewdb_api_CreatePolygonVert(): PostCreatePolygonVertExec failed!\n");
    return(ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }
  else if(rc == EWDB_RETURN_WARNING)
  {
    return(EWDB_RETURN_FAILURE);
  }

  return(EWDB_RETURN_SUCCESS);
}  /* end ewdb_api_CreatePolygonVert() */


static int InitCreatePolygonVertStatement (char *szStatement, EWDB_OCIStatementStruct *pSS)
{
  if ((szStatement == NULL) || (pSS == NULL))
  {
    logit ("", "InitCreatePolygonVertStatement(): Null parameters passed in!\n");
    return EWDB_RETURN_FAILURE;
  }

  pSS->FieldArray[0].pVal = &(Local_PolygonVertStruct.idVertex);
  pSS->FieldArray[1].pVal = &(Local_PolygonVertStruct.idPolygon);
  pSS->FieldArray[2].pVal = Local_szLat;
  pSS->FieldArray[3].pVal = Local_szLon;
  pSS->FieldArray[4].pVal = &(Local_PolygonVertStruct.iOrder);

  if (ewdb_base_RequestCursor (szStatement, pSS, 0) != 0)
  {
    logit ("", "InitCreatePolygonVertStatement(): Call to ewdb_base_RequestCursor failed.\n");
    return EWDB_RETURN_FAILURE;
  }

  return EWDB_RETURN_SUCCESS;
}


static int PrepCreatePolygonVertExec(EWDB_PolygonVertexStruct *pPolygonVert, EWDB_Cursor *ppCursor)
{
  if ((pPolygonVert == NULL) || (ppCursor == NULL))
  {
    logit ("", "PrepCreatePolygonVertExec(): Null parameters passed in!\n");
    return EWDB_RETURN_FAILURE;
  }

  SSStatement.NumOfFields = NUM_FIELDS;
  SSStatement.FieldArray = SQLParamsBindArray;
  SSStatement.RecordSize = 0;

  /* Copy the incoming struct into the local struct */
  memcpy (&Local_PolygonVertStruct, pPolygonVert, sizeof(EWDB_PolygonVertexStruct));

  sprintf (Local_szLat, "%3f", pPolygonVert->dLat);
  sprintf (Local_szLon, "%3f", pPolygonVert->dLon);

  if(InitCreatePolygonVertStatement (SQL_STRING, &SSStatement)
     != EWDB_RETURN_SUCCESS)
  {
    logit ("", "Call to InitCreatePolygonVertStatement failed!\n");
    return EWDB_RETURN_FAILURE;
  }

  *ppCursor = SSStatement.pCda;
  
  return EWDB_RETURN_SUCCESS;
}


static int PostCreatePolygonVertExec(EWDBid *pidVertex)
{
  EWDB_Cursor pCursor;

  if (pidVertex == NULL)
  {
    logit ("", "PostCreatePolygonVertExec(): Invalid arguments passed in.\n");
    return EWDB_RETURN_FAILURE;
  }

  *pidVertex = Local_PolygonVertStruct.idVertex;

  pCursor = SSStatement.pCda;
  ewdb_base_ReleaseCursor (pCursor);

  if(Local_PolygonVertStruct.idVertex <= 0)
  {
    if(Local_PolygonVertStruct.idVertex == -1)
    {
      logit("","PostCreatePolygonVertExec():  SQL Proc Create_PolygonVert() returned "
          "\"Unknown error\".  See SQL debug table for details.\n");
    }
    else
    {
      logit("","PostCreatePolygonVertExec():  SQL Proc Create_PolygonVert() returned "
          "the following error(%d).  Please see that proc for details.\n",
          Local_PolygonVertStruct.idVertex);
    }
    return(EWDB_RETURN_WARNING);
  }

  return (EWDB_RETURN_SUCCESS);
}  /* end PostCreatePolygonVertExec() */
