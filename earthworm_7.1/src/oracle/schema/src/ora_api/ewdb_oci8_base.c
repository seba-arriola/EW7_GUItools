/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_oci8_base.c,v 1.5 2005/05/23 17:55:57 davidk Exp $
 *    Revision history:
 *
 *    $Log: ewdb_oci8_base.c,v $
 *    Revision 1.5  2005/05/23 17:55:57  davidk
 *    Fixed bug in ewdb_base_Shutdown() where the bInitialized value wasn't
 *    being reset to false.  Also reset the OCI8Handles struct to blank.
 *
 *    Revision 1.4  2005/05/17 20:13:12  davidk
 *    Fixed bug where ewdb_base_RequestCursor() was passing a pointer to the
 *    statement pointer, instead of just the statement pointer, in the call to
 *    OCIHandleFree(), when reusing a cursor.
 *
 *    Revision 1.3  2005/05/12 17:21:30  davidk
 *    Switched to OCI8+ routines. (tested and debugged).
 *
 *
 *
 */
  
/* ewdb_oci8_base.c */
/* Contains generic functions for expediting programming with
   the Oracle Call Interface(OCI) v8+. Requires several callout
   functions to be implemented by client.  David K
******************************************************************/

/*****************************************************************
 * #includes 
 *****************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "ewdb_system_support.h"  
#include "ewdb_cli_base.h"
#include <ewdb_oci8_base_internal.h>
/*****************************************************************
 * End #includes
 *****************************************************************/

/* Global NO_DATA pseudo-constant! */
int EWDB_SQL_ERROR_NO_DATA;


static EWDBCursorMgmtStruct pcmsCursors[EWDB_MAX_CURSORS];

static mutex_t EWDBCursorMutex;

static char EWDBuser[40];
static char EWDBpassword[20];
static char EWDBservice[20];

static int  EWDB_Base_Debug = EWDB_DEBUG_DB_BASE_NONE;
static int  bInitialized = 0;
static int  EWDB_connected=FALSE;

/* single global connection handle (ugh!!!) 
   NO MULTIPLE CONNECTIONS THROUGH API */

EWDB_ConnectionHandle hEWDBC;

static OCI8CursorStruct EWDBCursorList[EWDB_MAX_CURSORS];
static OCI8HandleStruct OCI8Handles;



/************************************************************
 * ewdb_base_Init() gets things ready for connecting to a   *
 *  database                                                *
 *                                                          *
 *   Purpose: Initialize the OCI environment & DB connection*
 *                                                          *
 *   Functions Called:                                      *
 *      System: memset,printf                               *
 *      Oracle: OCIEnvCreate, OCIHandleAlloc                *
 *      User:   ewdb_base_Reconnect, ewdb_base_Disconnect   *
 *                             David K  from    Lynn Dietz  *
 ************************************************************/
int ewdb_base_Init(char * DBuser, char * DBpassword, char * DBservice)
{
  int i;

  if(bInitialized)
  {
    logit("et", "Warning ewdb_base_Init() called after OCI8 layer already initialized!  Ignoring!\n");
    return(EWDB_RETURN_WARNING);
  }

  /* Assign the connection handle to our HandleStruct */
  hEWDBC = (void *) &OCI8Handles;

  /* Create the mutex for tracking cursor check_out */
  CreateSpecificMutex(&EWDBCursorMutex);
     
  memset(&OCI8Handles, 0, sizeof(OCI8Handles));

   /* DK 05/04/05   I'm not clear on the mutexing and storage differences between mode=OCI_DEFAULT
                    and mode=OCI_THREADED.  The official DB API position, is that the API is threadsafe,
                    and will work in a multithreaded environment, but DB access must be restricted to 
                    a single thread (in addition to the Connection manager thread, which is not exposed
                    to the application.  OCI_DEFAULT appears to satisfy this position, and should have
                    less overhead than OCI_THREADED.
   ***************************************************************************************************/
  OCI8Handles.iRetCode = 
             OCIEnvCreate(&OCI8Handles.phOCI8Env, /* env handle pointer */
                          (ub4) OCI_DEFAULT,      /* mode */
                          (dvoid *)0,             /* context pointer */
                          (dvoid * (*)()) NULL,   /* malloc func pointer */
                          (dvoid * (*)()) NULL,   /* realloc func pointer */
                          (void (*)()) NULL,      /* free func pointer */
                          0,                      /* xtramemsz */
                          NULL                    /* usrmempp */
                         );

  if(OCI8Handles.iRetCode != 0)
  {
    logit("","ewdb_base_Init(): Error: OCIEnvCreate() returned %d\n", OCI8Handles.iRetCode);
    return(-2);  /* EnvCreate failed! */
  }


  if(OCI8Handles.iRetCode =
       OCIHandleAlloc((dvoid *) OCI8Handles.phOCI8Env, (dvoid **) &OCI8Handles.phOCI8Err,
                      (ub4) OCI_HTYPE_ERROR, (size_t) 0, (dvoid **) 0))
  {
    logit("et", "ewdb_oci_base: ERROR: OCIHandleAlloc() failed(%d) for Error Handle.\n",
          OCI8Handles.iRetCode);
    return OCI_ERROR;
  }

  /* This handle is alloc'd by OCILogon2() *
   * if(OCI8Handles.iRetCode =
   *     OCIHandleAlloc((dvoid *) OCI8Handles.phOCI8Env, (dvoid **) &OCI8Handles.phOCI8Conn,
   *                    (ub4) OCI_HTYPE_SVCCTX, (size_t) 0, (dvoid **) 0))
   * {
   *  logit("et", "ewdb_oci_base: ERROR: OCIHandleAlloc() failed(%d) for Connection Handle.\n",
   *        OCI8Handles.iRetCode);
   *  return OCI_ERROR;
   * }
   ***************************************************/

  for(i=0; i < EWDB_MAX_CURSORS; i++)
  {
    EWDBCursorList[i].pHS = &OCI8Handles;

    if(EWDBCursorList[i].iRetCode = 
         OCIHandleAlloc((dvoid *) OCI8Handles.phOCI8Env, (dvoid **) &EWDBCursorList[i].phStmt,
                        (ub4) OCI_HTYPE_STMT, (size_t) 0, (dvoid **) 0))
    {
      logit("et", "ewdb_oci_base: ERROR: OCIHandleAlloc() FAILED(%d) for cursor (%d)\n",
            EWDBCursorList[i].iRetCode, i);
      return OCI_ERROR;
    }
  }


  /*


  if (OCIHandleAlloc((dvoid *) envhp, (dvoid **) &srvhp,
                     (ub4) OCI_HTYPE_SERVER, (size_t) 0, (dvoid **) 0))
  {
    (void) printf("FAILED: OCIHandleAlloc()\n");
    return OCI_ERROR;
  }

  if (OCIHandleAlloc((dvoid *) envhp, (dvoid **) &authp,
                     (ub4) OCI_HTYPE_SESSION, (size_t) 0, (dvoid **) 0))
  {
    (void) printf("FAILED: OCIHandleAlloc()\n");
    return OCI_ERROR;
  }

  if (OCIDescriptorAlloc((dvoid *) envhp, (dvoid **) &clob,
                         (ub4)OCI_DTYPE_LOB, (size_t) 0, (dvoid **) 0))
  {
    (void) printf("FAILED: OCIDescriptorAlloc()\n");
    return OCI_ERROR;
  }

  if (OCIDescriptorAlloc((dvoid *) envhp, (dvoid **) &blob,
                         (ub4)OCI_DTYPE_LOB, (size_t) 0, (dvoid **) 0))
  {
    (void) printf("FAILED: OCIDescriptorAlloc()\n");
    return OCI_ERROR;
  }
*/
  /* set return code constants with OCI specific values */
  ewdb_base_SetSQLReturnCodes();

  /* set init to true */
  bInitialized = TRUE;

  /* set the connection params */
  ewdb_base_SetConnectionParams(DBuser, DBpassword, DBservice);

  if(ewdb_base_Reconnect() == EWDB_RETURN_SUCCESS)
    return(EWDB_RETURN_SUCCESS);
  else
    return(EWDB_RETURN_FAILURE);
}  /* ewdb_base_Init() */


/*****************************************************************
 *  ewdb_base_SetConnectionParams() Set the logon parameters for the
 *  database connection.
 *****************************************************************/
int ewdb_base_SetConnectionParams(char * DBuser, char * DBpassword, 
                                  char * DBservice)
{
  strncpy(EWDBuser,DBuser,sizeof(EWDBuser)-1);
  strncpy(EWDBpassword,DBpassword,sizeof(EWDBpassword)-1);
  strncpy(EWDBservice,DBservice,sizeof(EWDBservice)-1);
  
  EWDBuser[sizeof(EWDBuser)-1]=0;
  EWDBpassword[sizeof(EWDBpassword)-1]=0;
  EWDBservice[sizeof(EWDBservice)-1]=0;

  return(EWDB_RETURN_SUCCESS);
}  /* ewdb_base_SetConnectionParams() */


int ewdb_base_Reconnect() 
{
  if (!EWDB_connected)
  {
    if(EWDB_Base_Debug & EWDB_DEBUG_DB_BASE_CONNECT_INFO)
      logit("","EWDB_BASE:Debug:ewdb_base_Reconnect():  Connecting to DB.\n");

    /* OCILogon2 is the latest Oracle logon function which opens a connection to the 
       Oracle database indicated by szDSN, for user szUser with password szPWD. 
       OCI_DEFAULT that you want the standard connection with no sharing/pooling/caching
     ***************************************************************************/
    if(OCI8Handles.iRetCode = 
                  OCILogon2(OCI8Handles.phOCI8Env,
                            OCI8Handles.phOCI8Err,
                            &OCI8Handles.phOCI8Conn,
                            EWDBuser,     strlen(EWDBuser),
                            EWDBpassword, strlen(EWDBpassword),
                            EWDBservice,  strlen(EWDBservice),
                            OCI_DEFAULT /* mode */
                           )
      )

     {
       ewdb_base_ErrorReport((EWDB_ConnectionHandle)hEWDBC,
                             (EWDB_Cursor)hEWDBC, "ewdb_base_Reconnect:OCILogon2",
                             EWDB_ERROR_DBRECONNECT +1);
       return(EWDB_RETURN_FAILURE);
     }

     /* "statement handles" have replaced cursors in OCI8+, and they do not
        need to be opened prior to use.
     ***************************************************************/

     /* No need to Turn off auto-commit.  It is now controlled by a param to SQLExecute.
     *********************************************/

     EWDB_connected=TRUE;
   } /* End if !DB_connected */
   return( EWDB_RETURN_SUCCESS );
}  /* End ewdb_base_Reconnect */


/****************************************************************
*  ewdb_base_Disconnect()  closes the current Database connection  *
*****************************************************************/
int ewdb_base_Disconnect(int status )
{
  if(EWDB_connected)
  {
    OCILogoff(OCI8Handles.phOCI8Conn, OCI8Handles.phOCI8Err); 
    EWDB_connected = FALSE;
  }
  return(status);
}


int ewdb_base_ReleaseCursor(EWDB_Cursor pCursor)
{
  int i;

  for(i=0; i < EWDB_MAX_CURSORS; i++)
  {
    if((OCI8CursorStruct *)pCursor == &EWDBCursorList[i])
    {
      pcmsCursors[i].bCursorInUse = FALSE;
      return(0);
    }
  }
  /* invalid cursor pointer!!!  (or bad logic on our part) */
  return(-2);
}  /* end ewdb_base_ReleaseCursor() */


/*****************************************************************
 *  ewdb_base_ConnectedToDB() allows external functions to       *
 *  figure out if we are currently connected to the DB           *
 *****************************************************************/
int ewdb_base_ConnectedToDB()
{
  return(EWDB_connected);
}  /* end ewdb_base_ConnectedToDB() */


/*****************************************************************
 *  EWDB_Set_CLI_Debug() sets the debug level for the            *
 *  routines.                                                    *
 *  Debug Levels:
                  EWDB_DEBUG_DB_BASE_CONNECT_INFO
                  EWDB_DEBUG_DB_BASE_STATEMENT_PARSE_INFO
                  EWDB_DEBUG_DB_BASE_FUNCTION_ENTRY_INFO
    Debug levels can be |'d together                             *
*****************************************************************/
int ewdb_base_Set_CLI_Debug(int iDebug)
{
  EWDB_Base_Debug = iDebug;
  return(EWDB_RETURN_SUCCESS);
}  /* end ewdb_base_Set_CLI_Debug() */


/**************************************************************
 *  Name:       ewdb_base_ErrorReport                         *
 *  Purpose:    Report error conditions that occurred         *
 *              during calls to OCI API                       *
 *  Functions Called:                                         *
 *    Oracle:   oerhms                                        *
 *    User:     logit                                         *
 *  Parameters Described:                                     *
 *    hEWDBC    EWDB_ConnectionHandle;                        *
 *                         Oracle database connection handle  *
 *    pCursor   EWDB_Cursor;                                  *
 *                          Oracle cursor handle              * 
 *    pFuncName	char *;    Pointer to the name of function    *
 *                         that called ewdb_base_ErrorReport()*
 *    ErrorID   int;       An error ID unique within the      * 
 *                         calling function                   *
 *                                               -Lynn Dietz  *
 **************************************************************/
void ewdb_base_ErrorReport(EWDB_ConnectionHandle hEWDBC, 
                           EWDB_Cursor pCursor, 
                           char *pFuncName, int ErrorID)
{
    int         iRetCode;
    text        msg[513];
    OCIError  * phErr;
    sb4         errcode = 0;
    int         i;


    phErr = ((OCI8HandleStruct *)hEWDBC)->phOCI8Err;

    memset(msg, 0, sizeof(msg));

    for(i=1;1;i++)
    {
      iRetCode = OCIErrorGet((dvoid *) phErr, (ub4) i, (text *) NULL, &errcode,
                             msg, (ub4) sizeof(msg)-1, (ub4) OCI_HTYPE_ERROR);
      if(iRetCode)
      {
        if(i==1)
        {
          /* we think there's an error here, but we're not getting one out of ErrorGet.
             check for special errors */
          if(((void *)pCursor) == (void*)hEWDBC)
          {
            switch(((OCI8HandleStruct *)hEWDBC)->iRetCode)
            {
              case OCI_SUCCESS:
                logit("","WARNING: ewdb_base_ErrorReport() invoked but no Error Found!");
                break;
              case OCI_SUCCESS_WITH_INFO:
                logit("","WARNING: SUCCESS_W_INFO!");
                break;
              case OCI_INVALID_HANDLE:
                logit("","ERROR: Invalid Handle!!!");
                break;
              default:
                logit("","ERROR: ewdb_base_ErrorReport() invoked for unexpected Error(%d)!", 
                      ((OCI8HandleStruct *)hEWDBC)->iRetCode);
                break;
            }
          }  /* end if pCursor == hEWDBC - non cursor related error */
          else
          {
            switch(((OCI8CursorStruct *)pCursor)->iRetCode)
            {
              case OCI_SUCCESS:
                logit("","WARNING: ewdb_base_ErrorReport() invoked but no Error Found!");
                break;
              case OCI_SUCCESS_WITH_INFO:
                logit("","WARNING: SUCCESS_W_INFO!");
                break;
              case OCI_INVALID_HANDLE:
                logit("","ERROR: Invalid Handle!!!");
                break;
              default:
                logit("","ERROR: ewdb_base_ErrorReport() invoked for unexpected Error(%d)!", 
                      ((OCI8CursorStruct *)pCursor)->iRetCode);
                break;
            }
          }
        }  /* if i==1   FIRST call to OCIErrorGet() */
        break;
      }  /* if no error found by OCIErrorGet() */
      logit("t", "Error: %d:%s", errcode, msg);
    }  /* end for loop through all errors */

    logit("",  " [error# %d in %s]\n", ErrorID, pFuncName);

    return;

}  /* End ewdb_base_ErrorReport() */



 /****************************************************************
 *  ewdb_base_ErrorReport() is used to report and log error messages  *
 *  generated during the execution of oracle funtions            *
 *  (oexec,obndrv,oparse,etc.).                                  *
 *****************************************************************/

int ewdb_base_SQLExecute(EWDB_Cursor pCursor)
{
  OCI8CursorStruct * pCurs = (OCI8CursorStruct *)pCursor;
  ub2 sStatementType;
  int iNumIterations;


  OCIAttrGet(pCurs->phStmt, OCI_HTYPE_STMT,
                   (dvoid*)&sStatementType,
                   NULL /* sizep */,
                   OCI_ATTR_STMT_TYPE,
                   pCurs->pHS->phOCI8Err);

  if(sStatementType == OCI_STMT_SELECT)
    iNumIterations = 0;
  else
    iNumIterations = 1;


  return(pCurs->iRetCode =
         OCIStmtExecute(pCurs->pHS->phOCI8Conn,
                        pCurs->phStmt,
                        pCurs->pHS->phOCI8Err,
                        iNumIterations /* iters */,
                        0,
                        (OCISnapshot *) NULL,
                        (OCISnapshot *) NULL,
                        OCI_DEFAULT
                       )
        );
}


int ewdb_base_SQLCommit(EWDB_ConnectionHandle hEWDBC)
{
  return(OCI8Handles.iRetCode =
           OCITransCommit(((OCI8HandleStruct *)hEWDBC)->phOCI8Conn,
                          ((OCI8HandleStruct *)hEWDBC)->phOCI8Err,
                          OCI_DEFAULT
                         )
        );
}

int ewdb_base_SQLRollback(EWDB_ConnectionHandle hEWDBC)
{
  return(OCI8Handles.iRetCode =
           OCITransRollback(((OCI8HandleStruct *)hEWDBC)->phOCI8Conn,
                            ((OCI8HandleStruct *)hEWDBC)->phOCI8Err,
                            OCI_DEFAULT
                           )
        );
}

int ewdb_base_SQLFetchRows(EWDB_Cursor pCursor, int iNumRows)
{
  OCI8CursorStruct * pCurs = (OCI8CursorStruct *)pCursor;

  return(pCurs->iRetCode =
           OCIStmtFetch2(pCurs->phStmt,
                         pCurs->pHS->phOCI8Err,
                         iNumRows,
                         OCI_DEFAULT /*orientation*/,
                         0 /*offset - ignored*/,
                         OCI_DEFAULT /*mode*/
                        )
        );
}  /* end ewdb_base_SQLFetchRows */


int ewdb_base_GetCursorRetCode(EWDB_Cursor pCursor)
{
  return(((OCI8CursorStruct *)pCursor)->iRetCode);
}


int ewdb_base_GetCursorRowsProcessedCount(EWDB_Cursor pCursor)
{
  OCI8CursorStruct * pCurs = (OCI8CursorStruct *)pCursor;
  int iRPCount;


  OCIAttrGet(pCurs->phStmt, OCI_HTYPE_STMT,
                   (dvoid*)&iRPCount,
                   NULL /* sizep */,
                   OCI_ATTR_ROW_COUNT,
                   pCurs->pHS->phOCI8Err);

  return(iRPCount);

};


void ewdb_base_SetSQLReturnCodes(void)
{
  EWDB_SQL_ERROR_NO_DATA = OCI_NO_DATA;
}


/*****************************************************************
 *  Shutdown the OCI environment & DB connection
 *****************************************************************/
int ewdb_base_Shutdown(void)
{
  int rc;

  OCIHandleFree((dvoid *)OCI8Handles.phOCI8Env, (ub4) OCI_HTYPE_ENV);
  rc = OCITerminate(OCI_DEFAULT);
  bInitialized = 0;
  memset(&OCI8Handles, 0, sizeof(OCI8Handles));

  return(rc);
}


int ewdb_base_RequestCursor(char * szStatement, 
                       EWDB_OCIStatementStruct * pOCISS_DBStruct,
                       int bForceReBind)
{
  int iCursorFound=-1;
  int i;
  int iLoopPassCount=1;
  int tMin=2000000000;  /* I forget what the name of MAX_INT is */
  OCI8CursorStruct * pCurs;


  if(strlen(szStatement) > EWDB_MAX_STATEMENT_LENGTH)
  {
    logit("","ewdb_base_RequestCursor(): Error: szStatement string was "
             "too long!\n String is:\n%s\n",
             szStatement
         );
    return(-2);  /* Statement too long */
  }

  RequestSpecificMutex(&EWDBCursorMutex);

  while(iCursorFound == -1)
  {
    for(i=0; i < EWDB_MAX_CURSORS; i++)
    {
      if(!pcmsCursors[i].bCursorInUse)
      {
        /* The cursor is not in use, so it is eligible for us to use */
        if(iLoopPassCount == 1)
        {
          if(   pcmsCursors[i].bCursorIsParsed 
            && !strcmp(szStatement,pcmsCursors[i].szStatement)
            )
          {
          /* We've found a cursor that matches our statement, and it is
            already parsed and bound!! Lucky day!  */
            iCursorFound=i;
            break;
          }
        }
        else if(iLoopPassCount == 2)
        {
        /* don't take someone else's cursor if there are still 
          free ones, that haven't yet been parsed */
          if(!pcmsCursors[i].bCursorIsParsed)
          {  
            iCursorFound=i;
            strcpy(pcmsCursors[iCursorFound].szStatement,szStatement);
            break;
          }
          /* else it's second pass and this cursor is already 
          parsed, don't use it, as it doesn't match us, and
          there may still be free ones out there.
          */
        }
        else  /* > 2 */
        {
          /* dk 021402
             replaced iCursorFound with i, for Cursor array, so that
             we could loop through the cursors instead of using -1.
             Fixed bad read bug.
          *********************************************/

          if(pcmsCursors[i].tLastUse < tMin)
          {
            tMin=pcmsCursors[i].tLastUse;
            iCursorFound=i;
            strcpy(pcmsCursors[iCursorFound].szStatement,szStatement);
            pcmsCursors[i].bCursorIsParsed = 0;
            /* by not breaking here, we will check all of the cursors
               and end up with the oldest one (most time since last use)
            ************************************************************/
          }
        }   /* else iLoopPassCount */
      }  /* end if(!bCursorInUse) */
    }  /* end for i=Cursors in array */

    if(iLoopPassCount > 2)
    {
      if(iCursorFound == -1)
        sleep_ew(1000);  /* sleep 1 second before trying again */
    }
    iLoopPassCount++;
  }  /* end while we don't have a cursor */

  /* set the cursors "last used time", so that this cursor is
     not the first to get overwritten
  ***********************************************************/
  pcmsCursors[iCursorFound].tLastUse=time(NULL);

  /* link the cursor to the current statement struct, and our temp short-cut var */
  pOCISS_DBStruct->pCda = (EWDB_Cursor)pCurs = &(EWDBCursorList[iCursorFound]);

  if(iLoopPassCount > 3)
  {
    /* We grabbed a used cursor that was set for a different statement.
       That statement has a bunch of column-binds associated with it.
       We don't have access to the statement, so we can't free those binds
       one at a time.  Instead, just free the cursor, and realloc it. */
    if(pCurs->iRetCode = OCIHandleFree((EWDBCursorList[iCursorFound].phStmt), 
                                       OCI_HTYPE_STMT))
    {
      logit("","Failed to Free handle for cursor(%d) during reassign.\n", iCursorFound);
    }

    if(pCurs->iRetCode = 
         OCIHandleAlloc((dvoid *) OCI8Handles.phOCI8Env, (dvoid **) &EWDBCursorList[iCursorFound].phStmt,
                        (ub4) OCI_HTYPE_STMT, (size_t) 0, (dvoid **) 0))
    {
      logit("et", "ewdb_oci_base: ERROR: OCIHandleAlloc() FAILED(%d) for re-alloc of cursor (%d)\n",
            EWDBCursorList[i].iRetCode, i);
      return OCI_ERROR;
    }

  }

  /* Now we have a cursor, but we have to make sure its prepped */
  /* If the cursor is not yet parsed, parse it */
  if(!pcmsCursors[iCursorFound].bCursorIsParsed)
  {
    /* parse the statement into the cursor */
    if(pCurs->iRetCode = OCIStmtPrepare(pCurs->phStmt,
                                        pCurs->pHS->phOCI8Err,
                                        (text *) pcmsCursors[iCursorFound].szStatement,
                                        strlen(pcmsCursors[iCursorFound].szStatement),
                                        OCI_NTV_SYNTAX,
                                        OCI_DEFAULT
                                       )
      )
    {
      ewdb_base_ErrorReport(hEWDBC,&(EWDBCursorList[iCursorFound]),"ewdb_base_RequestCursor",1);
      logit("","Parse failed for sql string: [%s]\n",pcmsCursors[iCursorFound].szStatement);
      return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE) );
    }


    pcmsCursors[iCursorFound].bCursorIsParsed=1;
    pcmsCursors[iCursorFound].bCursorIsBound=0;
  }

  /* Force rebinding if flagged to do so */
  if(bForceReBind)
  {
    pcmsCursors[iCursorFound].bCursorIsBound=0;
  }

  if(!pcmsCursors[iCursorFound].bCursorIsBound)
  {
    if(ewdb_base_QuickBindForStatement(pOCISS_DBStruct, "ewdb_base_RequestCursor", 100, FALSE))
    {
      logit("","Bind failed for sql string: [%s]\n",szStatement);
      return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE) );
    }
    pcmsCursors[iCursorFound].bCursorIsBound=1;
  }
  else
  {
    /* rebind, but only the fields required to be rebound each time.... */
    if(ewdb_base_QuickBindForStatement(pOCISS_DBStruct, "ewdb_base_RequestCursor", 100, TRUE))
    {
      logit("","Bind failed for sql string: [%s]\n",szStatement);
      return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE) );
    }
  }


  pcmsCursors[iCursorFound].bCursorInUse = TRUE;

  ReleaseSpecificMutex(&EWDBCursorMutex);

  return(0);
}  /* End ewdb_base_RequestCursor() */

int TypeRequiresRebind(int ValType)
{
  switch (ValType)
  {
  case OA_DOUBLE:
  case OA_FLOAT:
  case OA_SZ:
  case OA_LVRAW:
    return(TRUE);
  default:
    return(FALSE);
  }
}


/*****************************************************************
 *  ewdb_base_QuickBindForStatement( )  OCI bind all of the      *
 *  variables for a SQL statement.                               *
 *****************************************************************/
int ewdb_base_QuickBindForStatement(EWDB_OCIStatementStruct * pSS, 
                                    char * CallingFunction,int BegErrNum,
                                    int bAlreadyBound)
{
  int i;
  EWDB_OCI_SFS * pCurr;


  for(i=0; i < pSS->NumOfFields; i++)
  {
    pCurr=&(pSS->FieldArray[i]);

    pCurr->pBind = NULL;


    if(pCurr->UseField)
    {
      if((!bAlreadyBound) || TypeRequiresRebind(pCurr->Type))
      {
        if(EWDB_QOBV(pSS->pCda,pCurr->pVal,&(pCurr->Ind),pCurr->Type,
          pCurr->FieldID,&pCurr->pRetLens,pCurr->pRetCodes,(OCIBind **)&pCurr->pBind, 
          pSS->RecordSize,
          CallingFunction,BegErrNum+i))
        {
          return(EWDB_RETURN_FAILURE);
        }
      }  /* if this is first bind, or a field-type that requires a rebind */
    }  /* end if UseField */
    else
    {
      if(EWDB_Base_Debug & EWDB_DEBUG_DB_BASE_STATEMENT_PARSE_INFO)
        logit("","Not Binding %dth element due to UseField flag!\n",i);
    }
  }
  return(EWDB_RETURN_SUCCESS);
}  /* End ewdb_base_QuickBindForStatement */



/*****************************************************************
 *  EWDB_QOBV is Quick Oracle Bind by Value.  It is a wrapper    *
 *  around the OCI bind calls obndrv() and odefinps().  It       *
 *  encapsulates some of the complexity that comes with all of   *
 *  options in the Oracle calls that bind C variables to SQL vars*
 *****************************************************************/
int EWDB_QOBV(OCI8CursorStruct * pCurs, void * pVal, sb2 * pInd, int ValType,
         char * ValID, ub2** ppLengths, ub2* pRetCodes, OCIBind ** ppBind, sb4 BufSkip,
         char * FuncName, int FuncNum)
{
/* WHAT THE HECK IS THIS??? DK990924 */
# define MAX_ARRAY_LENGTH 100  

  static sword size;
  static   int size2;
  static int OValType;
  static sb2 DummyInd[MAX_ARRAY_LENGTH];         /* Dummy Indicator Array */
  static ub2 *pLength, *pRetCode;
  /* static sb2 DummyLO[MAX_ARRAY_LENGTH]; */    /* Dummy Length Out Array */

  switch (ValType)
  {
  case OA_INT:
    {
      OValType=INT_TYPE;
      size=sizeof(int);
      break;
    }
  case OA_SHORT:
    {
      OValType=INT_TYPE;
      size=sizeof(short);
      break;
    }
  case OA_CHAR:
    {
      OValType=INT_TYPE;
      size=sizeof(char);
      break;
    }
  case OA_DOUBLE:
    {
      OValType=VARCHAR2_TYPE;
      size= OCI_NTS;
      size2 = 20;
      break;
    }
  case OA_FLOAT:
    {
      OValType=VARCHAR2_TYPE;
      size= OCI_NTS;
      size2 = 15;
      break;
    }
  case OA_SZ:
    {
      OValType=VARCHAR2_TYPE;
      size= OCI_NTS;
      size2 = 4000;
      break;
    }
  case OA_LVRAW:
    {
      OValType=LONGVARRAW_TYPE;
      size= (sword)pRetCodes;
      break;
    }
  default:
    {
      logit("","Bad type passed to EWDB_QOBV(): %d\n",ValType);
      return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE) );
    }
  }

  if(ValID[0] == ':')
  {
    pLength  = (ub2 *) NULL;
    pRetCode = (ub2 *) NULL;

    if(size == OCI_NTS)
    {
      if(strncmp(ValID, ":OUT", 4) == 0)
      {
        size = size2;
        pLength = (ub2 *) ppLengths;
        pRetCode = (ub2 *) (((unsigned int)ppLengths)+sizeof(ub2));
      }
      else
      {
        size = strlen(pVal);
      }
    }

    if(OCIBindByName(pCurs->phStmt, 
                     ppBind,
                     pCurs->pHS->phOCI8Err,
                     (text *) ValID,
                     strlen(ValID),
                     (ub1 *)pVal, 
                     size,
                     OValType,
                     pInd,
                     pLength /*alenp*/, 
                     pRetCode /*rcodep*/, 
                     (ub4)   0 /*maxarr_len*/,
                     (ub4 *) 0 /*cur_ele_p*/, 
                     OCI_DEFAULT /*mode*/
                    )
      )
    {
      ewdb_base_ErrorReport(hEWDBC,pCurs,FuncName,FuncNum);
      return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE) );
    }
  }
  else
  {
    if(size == OCI_NTS)
    {
      /* Um, ...
         Going to overload the value of the
         indicator variable, to carry the size
         of string item buffers in the case
         of list retrieval.  I know this is bad,
         but in the interests of not having to
         retrofit, I am doing it.  Max size of
         2 bytes supported.  Davidk 6/22/98
      */
      size=*pInd;
    }



/*odefinps(Cda_Def *cursor, ub1 opcode, sword pos,
      ub1 *bufctx, sb4 bufl, sword ftype, <sword scale>,
     [sb2 *indp],<text *fmt>, <sb4 fmtl>, <sword fmtt>,
     [ub2 *rlenp], [ub2 *rcodep], sb4 buf_skip,
     sb4 ind_skip, sb4 len_skip, sb4 rc_skip);


  if (odefinps(&cda, 1, 1, (ub1 *) &emp_records[0].empno, 
               (ub4) sizeof(emp_records[0].empno), SQLT_INT, 0, 
               (sb2 *) &emp_records_inds[0].empno, 
               (text *)0, 0,0,(ub2 *) 0, (ub2 *) 0, 
               (sb4) sizeof(emp_record), (sb4) 
               sizeof(emp_record_indicators), 0, 0))
    exit(OCI_EXIT_FAILURE);

*/



    if(!BufSkip)
    {
      BufSkip=size;
    }

    if(OValType == LONGVARRAW_TYPE)
    {
      if(OCIDefineByPos(pCurs->phStmt,
                        ppBind,
                        pCurs->pHS->phOCI8Err,
                        /*Position in SQL stmt*/ atoi(ValID),
                        /*Pointer to my Buffer */(ub1 *)pVal, 
                        /*Size of Buffer */size, 
                        /*Type of C Var */OValType, 
                        /*Indicator Array */pInd,
                        /*Array of My Buffer Len's*/*ppLengths,
                        /*Array of Return Codes*/0,
                        OCI_DEFAULT
                       )
         )
      {      
        logit("","Error during LONGVARRAW_TYPE odefin()\n");
        ewdb_base_ErrorReport(hEWDBC,pCurs,FuncName,FuncNum);
        return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE) );
      }
    }  /* OValType != LONGVARRAW_TYPE */
    else
    {
      if((pCurs->iRetCode = OCIDefineByPos(pCurs->phStmt,
                                          ppBind,
                                          pCurs->pHS->phOCI8Err,
                                          /*Position in SQL stmt*/ atoi(ValID),
                                          /*Pointer to my Buffer */(ub1 *)pVal, 
                                          /*Size of Buffer */size, 
                                          /*Type of C Var */OValType, 
                                          /*Indicator Array */DummyInd,
                                          /*Array of My Buffer Len's*/*ppLengths,
                                          /*Array of Return Codes*/pRetCodes,
                                          OCI_DEFAULT
                                         )
          )
         ||
          (pCurs->iRetCode = OCIDefineArrayOfStruct(*ppBind,
                                                    pCurs->pHS->phOCI8Err,
                                                    /*Size to skip between bufs in array*/BufSkip,
                                                    /*Size to skip between ind bufs in array*/0,
                                                    /*Size to skip between len's in array*/2,
                                                    /*Size to skip between retcodes in array*/0
                                                   )
         )
        )

      {      
        logit("","Error during OCIDefineByPos() or OCIDefineArrayOfStruct()\n");
        ewdb_base_ErrorReport(hEWDBC,pCurs,FuncName,FuncNum);
        return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE) );
      }
    }  /* OValType != LONGVARRAW_TYPE */
  }

  return(EWDB_RETURN_SUCCESS);
}  /* End EWDB_QOBV() */


