/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */
/*    $Id: ewdb_api_GetEventForOrigin.c,v 1.4 2005/06/27 15:31:36 davidk Exp $                                                */
/*                                                          */
/*    Revision history:                                     */
/*
 *     $Log: ewdb_api_GetEventForOrigin.c,v $
 *     Revision 1.4  2005/06/27 15:31:36  davidk
 *     Fixed bugs introduced during global fetch/replace during DB cleanup.
 *
 *     Revision 1.3  2005/06/15 19:03:12  davidk
 *     DB API Cleanup
 *
 *     Revision 1.2  2003/05/27 17:20:47  michelle
 *     corrected syntax in sql szStatement (added ; at end of SQLCODE line)
 *
 *     Revision 1.1  2003/05/27 17:19:13  michelle
 *     Initial revision
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

/* ewdb_api_GetEventForOrigin.c */
#include <ewdb_ora_api.h>
#include <ewdb_cli_base.h>
#include <ewdb_ew_oci_base.h>

static char SQL_STRING[] =
          "Begin                                  "
          " :OUT_iRetCode := 0;                   "
          " select idEvent into :OUT_idEvent      "
          "  from Bind                            "
          "  where Bind.idCore = :IN_idOrigin     "
          "    and Bind.tiCore = getti('Origin'); "
          "EXCEPTION                              "
          "  WHEN OTHERS THEN                     "
          "    :OUT_idEvent := -1;                "
          "    :OUT_iRetCode  := SQLCODE;         "
          "END;                                   ";

static EWDB_OCI_SFS SQLParamsBindArray[] = 
{
  {0,1,0,0,0,OA_EWDBID,  ":IN_idOrigin"},
  {0,1,0,0,0,OA_EWDBID,  ":OUT_idEvent"},
  {0,1,0,0,0,OA_INT,     ":OUT_iRetCode"}
};

#define	NUM_FIELDS	3

static	EWDBid Local_idOrigin, Local_idEvent;
static  int Local_iRetCode;

/* Insertion Struct for Get_idChan_From_Station_External szStatement */
static EWDB_OCIStatementStruct SSStatement;


/* Prototypes of functions defined in this file */
static int InitGetEventForOriginStatement(char *szStatement,
                                       EWDB_OCIStatementStruct *pSS);
static int PrepGetEventForOriginExec(EWDBid IN_idOrigin, EWDB_Cursor *ppCursor);
static int PostGetEventForOriginExec(EWDBid *pidEvent);

int ewdb_api_GetEventForOrigin(EWDBid IN_idOrigin, EWDBid * pidEvent)
{
  
  EWDB_Cursor pCursor;
  int rc;
  
  if(pidEvent == NULL) 
  {
    logit("", "ewdb_api_GetEventForOrigin(): Null pidEvent passed in!\n");
    return EWDB_RETURN_FAILURE;
  }
  
  ewdb_base_SetLastOraAPIActionTime();
  
  if(ewdb_base_Reconnect() != EWDB_RETURN_SUCCESS)
    /* Establishes connection, and performs binding!?! */
  {
    logit("", "ewdb_api_GetEventForOrigin(): Could not reconnect to the database!\n");
    return(EWDB_RETURN_FAILURE);
  }
  
  if(PrepGetEventForOriginExec(IN_idOrigin, &pCursor) 
    != EWDB_RETURN_SUCCESS)
  {
    logit("", "ewdb_api_GetEventForOrigin(): PrepGetEventForOriginExec() failed.\n");
    return(ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }
  
  if(ewdb_base_SQLExecute(pCursor))
  {
    ewdb_base_ErrorReport(hEWDBC, pCursor,
      "ewdb_api_GetEventForOrigin(): ewdb_base_SQLExecute", 1);
    return(ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  } 
  
  /* Commit the transaction(all the previous inserts!)
  In Case there is any auditing or logging or debug
  changes made in the stored procedures.
  ****************************************************/
  if(ewdb_base_SQLCommit(hEWDBC))
  {
    ewdb_base_ErrorReport(hEWDBC, hEWDBC,
                      "ewdb_api_GetEventForOrigin(): ewdb_base_SQLCommit",2);
    return(ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }
  
  rc = PostGetEventForOriginExec(pidEvent);
  if(rc == EWDB_RETURN_FAILURE)
  {
    logit("", "ewdb_api_GetEventForOrigin(): PostGetEventForOriginExec failed!\n");
    return(ewdb_base_Disconnect(EWDB_RETURN_FAILURE));
  }
  else if(rc == EWDB_RETURN_WARNING)
  {
    return(EWDB_RETURN_FAILURE);
  }
  
  return(EWDB_RETURN_SUCCESS);
}  /* end ewdb_api_GetIdChanFromStationExternal() */


static int InitGetEventForOriginStatement(char *szStatement,
                                          EWDB_OCIStatementStruct *pSS)
{
  
  pSS->FieldArray[0].pVal = &Local_idOrigin;
  pSS->FieldArray[1].pVal = &Local_idEvent;
  pSS->FieldArray[2].pVal = &Local_iRetCode;
  
  return(ewdb_base_RequestCursor(szStatement, pSS,0/*don't force rebind*/));
}  /* end InitGetEventForOriginStatement() */


static int PrepGetEventForOriginExec(EWDBid IN_idOrigin, EWDB_Cursor *ppCursor)
{
  
  SSStatement.NumOfFields = NUM_FIELDS;
  SSStatement.FieldArray = SQLParamsBindArray;
  SSStatement.RecordSize = 0;
  
  Local_idOrigin = IN_idOrigin;

  if(InitGetEventForOriginStatement(SQL_STRING,&SSStatement)
      != EWDB_RETURN_SUCCESS)
  {
    logit("", "Call to InitGetEventForOriginStatement failed!\n");
    return EWDB_RETURN_FAILURE;
  }
  
  *ppCursor = SSStatement.pCda;
  
  return(EWDB_RETURN_SUCCESS);
}  /* end PrepGetEventForOriginExec() */


static int PostGetEventForOriginExec(EWDBid *pidEvent)
{
  EWDB_Cursor pCursor;
  
  *pidEvent=Local_idEvent;
  
  pCursor = SSStatement.pCda;
  ewdb_base_ReleaseCursor(pCursor);
  
  if(Local_idEvent <= 0)
  {
    logit("","%s: ERROR! SQL Statement returned error(%d)!\n",
          "PostGetEventForOriginExec()", Local_iRetCode);
    return(EWDB_RETURN_WARNING);
  }

  return(EWDB_RETURN_SUCCESS);
}  /* end PostGetEventForOriginExec() */
