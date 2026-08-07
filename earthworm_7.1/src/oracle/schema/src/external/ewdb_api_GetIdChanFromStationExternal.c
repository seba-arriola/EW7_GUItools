/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */
/*    $Id: ewdb_api_GetIdChanFromStationExternal.c,v 1.4 2005/06/06 16:12:37 davidk Exp $                                                */
/*                                                          */
/*    Revision history:                                     */
/*
 *     $Log: ewdb_api_GetIdChanFromStationExternal.c,v $
 *     Revision 1.4  2005/06/06 16:12:37  davidk
 *     DB API Cleanup
 *
 *     Revision 1.3  2001/07/23 16:59:18  davidk
 *     API Cleanup.
 *     Improved error handling for return codes from the SQL Proc Get_idChan_From_Ext_StationID().
 *
 *     Revision 1.2  2001/05/15 02:16:25  davidk
 *     Moved functions around between the apps, DB API, and DB API INTERNAL
 *     levels.  Renamed functions and files.  Added support for amplitude
 *     magnitude types.  Reformatted makefiles.
 *
 *     Revision 1.1  2001/02/28 17:19:22  lucky
 *     Initial revision
 *
 *     Revision 1.4  2001/02/21 09:30:26  davidk
 *     Code was retrofitted to use the new db_cli_base API
 *     in lieu of the oci_base() and OCI7 function API's.layer, which
 *     provides abstraction from the OCI7 function calls to
 *     ora_api layer code.  See comments in
 *     schema/src/include/internal/ewdb_cli_base.h for more info
 *     on the details of the changes made.
 *
 *     Revision 1.3  2000/06/21 22:52:13  lucky
 *      Cleaned up logit calls to make log files more readable.
 *
 *     Revision 1.2  2000/03/30 20:29:17  davidk
 *     fixed $Log syntax in the RCS header
 *
 *     Revision 1.1  2000/03/30 20:26:54  davidk
 *     Initial revision
 *
*     Revision 1.2  1999/11/09 18:51:00  lucky
*     *** empty log message ***
*                                              */
/*                                                          */

/* get_idchan_from_station_external.c */
#include <ewdb_ora_api.h>
#include <ewdb_cli_base.h>
#include <ewdb_ew_oci_base.h>


static char SQL_STRING[] =
          "Begin Get_idChan_From_Ext_StationID(OUT_idChan => :OUT_idChan,"
          "IN_StationID => :IN_StationID); End;";

static EWDB_OCI_SFS SQLParamsBindArray[] = 
{
  {0,1,0,0,0,OA_EWDBID,  ":OUT_idChan"},
  {0,1,0,0,0,OA_INT,     ":IN_StationID"}
};

#define	NUM_FIELDS	2

static	EWDBid Local_idChan;
static	int		 Local_StationID;

/* Insertion Struct for Get_idChan_From_Station_External statement */
static EWDB_OCIStatementStruct SSStatement;


/* Prototypes of functions defined in this file */
static int InitGetidChanFromStationStatement(char *statement,
                                       EWDB_OCIStatementStruct *pSS);
static int PrepGetidChanFromStationExec(int IN_StationID, EWDB_Cursor *ppCursor);
static int PostGetidChanFromStationExec(EWDBid *pidChan);


int ewdb_api_GetIdChanFromStationExternal(EWDBid * pidChan, int IN_StationID)
{
  
  EWDB_Cursor pCursor;
  int rc;
  
  if(pidChan == NULL) 
  {
    logit("", "ewdb_api_GetIdChanFromStationExternal(): Null parameters passed in!\n");
    return EWDB_RETURN_FAILURE;
  }
  
  ewdb_base_SetLastOraAPIActionTime();
  
  if(ewdb_base_Reconnect() != EWDB_RETURN_SUCCESS)
    /* Establishes connection, and performs binding!?! */
  {
    logit("", "ewdb_api_GetIdChanFromStationExternal(): Could not reconnect to the database!\n");
    return(EWDB_RETURN_FAILURE);
  }
  
  if(PrepGetidChanFromStationExec(IN_StationID, &pCursor) 
    != EWDB_RETURN_SUCCESS)
  {
    logit("", "ewdb_api_GetIdChanFromStationExternal():  "
      "PrepGetidChanFromStationExec() failed.\n");
    return(ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }
  
  if(ewdb_base_SQLExecute(pCursor))
  {
    ewdb_base_ErrorReport(hEWDBC, pCursor,
      "ewdb_api_GetIdChanFromStationExternal(): ", 1);
    return(ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  } 
  
  /* Commit the transaction(all the previous inserts!)
  In Case there is any auditing or logging or debug
  changes made in the stored procedures.
  ****************************************************/
  if(ewdb_base_SQLCommit(hEWDBC))
  {
    ewdb_base_ErrorReport(hEWDBC, hEWDBC,
                      "ewdb_api_GetIdChanFromStationExternal(): ewdb_base_SQLCommit",2);
    return(ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }
  
  rc = PostGetidChanFromStationExec(pidChan);
  if(rc == EWDB_RETURN_FAILURE)
  {
    logit("", "ewdb_api_GetIdChanFromStationExternal(): PostGetidChanFromStationExec failed!\n");
    return(ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }
  else if(rc == EWDB_RETURN_WARNING)
  {
    logit("", "SQL ERROR in call to PostGetidChanFromStationExec!\n");
    return(EWDB_RETURN_FAILURE);
  }
  
  return(EWDB_RETURN_SUCCESS);
}  /* end ewdb_api_GetIdChanFromStationExternal() */


static int InitGetidChanFromStationStatement(char *statement,
                                      EWDB_OCIStatementStruct *pSS)
{
  
  pSS->FieldArray[0].pVal = &Local_idChan;
  pSS->FieldArray[1].pVal = &Local_StationID;
  
  if(ewdb_base_RequestCursor(statement, pSS, 0) != 0)
  {
    logit("", "InitGetidChanFromStationStatement(): Call to ewdb_base_RequestCursor failed.\n");
    return(EWDB_RETURN_FAILURE);
  }
  return(EWDB_RETURN_SUCCESS);
}  /* end InitGetidChanFromStationStatement() */


static int PrepGetidChanFromStationExec(int IN_StationID, EWDB_Cursor *ppCursor)
{
  
  if(ppCursor == NULL)
  {
    logit("", "PrepGetidChanFromStationExec(): Null parameters passed in!\n");
    return EWDB_RETURN_FAILURE;
  }
  
  SSStatement.NumOfFields = NUM_FIELDS;
  SSStatement.FieldArray = SQLParamsBindArray;
  SSStatement.RecordSize = 0;
  
  Local_StationID = IN_StationID;
  Local_idChan    = 0;

  if(InitGetidChanFromStationStatement(SQL_STRING,&SSStatement)
      != EWDB_RETURN_SUCCESS)
  {
    logit("", "PrepGetidChanFromStationExec(): InitGetidChanFromStationStatement failed!\n");
    return EWDB_RETURN_FAILURE;
  }
  
  *ppCursor = SSStatement.pCda;
  
  return(EWDB_RETURN_SUCCESS);
}  /* end PrepGetidChanFromStationExec() */


static int PostGetidChanFromStationExec(EWDBid *pidChan)
{
  EWDB_Cursor pCursor;
  
  pCursor = SSStatement.pCda;
  ewdb_base_ReleaseCursor(pCursor);
  
  if(pidChan == NULL)
  {
    logit("", "PostGetidChanFromStationExec(): Invalid arguments passed in.\n");
    return EWDB_RETURN_FAILURE;
  }
  
  *pidChan=Local_idChan;
  
  if(Local_idChan <= 0)
  {
    logit("","%s: ERROR!  SQL Proc %s returned error(%d)!\n",
          "PostGetidChanFromStationExec()", 
          "Get_idChan_From_Ext_StationID()", Local_idChan);
    return(EWDB_RETURN_WARNING);
  }

  return(EWDB_RETURN_SUCCESS);
}  /* end PostGetidChanFromStationExec() */
