/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_cli_base.h,v 1.3 2005/05/12 19:06:34 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_cli_base.h,v $
 *     Revision 1.3  2005/05/12 19:06:34  davidk
 *     Switched to OCI8+ routines. (tested and debugged).
 *     Added additonal field to EWDB_OCI_SFS.
 *     Removed ewdb_base_QuickBindForStatement() prototype, as it
 *     doesn't belong here.  It is encapsulated by the ewdb_base_RequestCursor()
 *     call.
 *
 *     Revision 1.2  2001/05/15 02:16:27  davidk
 *     Moved functions around between the apps, DB API, and DB API INTERNAL
 *     levels.  Renamed functions and files.  Added support for amplitude
 *     magnitude types.  Reformatted makefiles.
 *
 *     Revision 1.1  2001/04/06 18:16:34  davidk
 *     Initial revision
 *
 *     Revision 1.3  2001/02/21 08:24:51  davidk
 *     completed the comments that were truncated under the last version.
 *
 *     Revision 1.2  2001/02/21 07:59:06  davidk
 *     ewdb_db_cli_base.h was created as part of the modifications
 *     to the original oci_base layer, in order to create the
 *     new oci_base or db_cli_base layer.  The db_cli_base
 *     layer abstracts the ora_api layer code from the Oracle OCI7
 *     functions that are used to access the Oracle DB.  This removes
 *     all Oracle references from EWDB API functions, which makes
 *     them easier to port between data access mechanisms and
 *     even databases.  Specifically, the changes were made to
 *     facilitate the move from Oracle OCI7 routines to OCI8 and
 *     future versions of the Oracle Call Interface.  However,
 *     the functions could easily be ported to ODBC or another DB's
 *     Call Level Interface by only implementing a base layer for
 *     that access mechanism.  Previously OCI7 code was spread
 *     among most all of the EWDB API functions.  With more functions
 *     being written everyday (most of them in new files), migrating
 *     from the older OCI7 interface to a newer one was becoming
 *     a larger and larger task.  Now the layering for database calls
 *     looks like:
 *        Application
 *            \/
 *         EWDB API
 *            \/
 *       EWDB API Code
 *            \/
 *      DB CLI Base API
 *            \/
 *     OCI BASE for OCI7
 *            \/
 *         OCI 7 API
 *            \/
 *        OCI 7 Code
 *            \/
 *     DB Procedural SQL
 *         (PL/SQL)
 *            \/
 *        DB Tables
 *
 *
 *     Previously layering was:
 *        Application
 *            \/
 *         EWDB API
 *            \/
 *       EWDB API Code
 *            \/     \/
 *            \/      OCI Base API
 *            \/           \/
 *            \/     OCI BASE for OCI7
 *            \/     \/
 *         OCI 7 API
 *            \/
 *        OCI 7 Code
 *            \/
 *     DB Procedural SQL
 *         (PL/SQL)
 *            \/
 *        DB Tables
 *
 *     All existing EWDB API code was retrofitted to use the new
 *     db_cli_base layer in place of the oci_base and OCI7 layers.
 *     This includes replacing the following in all EWDB API code:
 *      include of <p3s2_oci_base.h> with include of <p3s2_cli_base.h>
 *      Cda_Def *     with  EWDB_Cursor
 *      Lda_Def *     with  EWDB_ConnectionHandle
 *      &EWDB_Lda     with  hEWDBC
 *      pCursor->rpc  with  ewdb_base_GetCursorRowsProcessedCount()
 *      ofen()        with  ewdb_base_SQLFetchRows()
 *      pCursor->rc   with  ewdb_base_GetCursorRetCode()
 *      SQL error 
 *         code 1403  with  EWDB_SQL_ERROR_NO_DATA
 *      oexec()       with  ewdb_base_SQLExecute()
 *      ocom()        with  EWDB_Commit()
 *      orol()        with  EWDB_Rollback()
 *     
 *     
 *     (The db_cli_base API is just the old oci_base functions
 *      with additional functions and abstraction that was neccessary
 *      to remove the oracle-specific stuff from the old oci_base 
 *      calls and the EWDB API functions.)
 *     
 *     (The end effect of all of this is that the only code that knows
 *      anything about ORACLE OCI 7 is the code in p3s2_oci_base.c.)
 *     
 *
 *     Revision 1.1  2001/02/21 07:27:49  davidk
 *     Initial revision
 *
 *     Revision 1.4  2000/03/30 18:28:24  davidk
 *     added #define macro for OA_EWDBID, so that host variables could be defined
 *     as a EWDBid type, and that type could change without having to change a
 *     lot of code.
 *     cursor management struct, so that overriding cursor use could be time based.
 *
 *     Revision 1.3  2000/01/04 01:50:57  davidk
 *     added a prototype for the EWDB_Shutdown() function.
 *
 *     Revision 1.2  1999/11/29 22:21:09  davidk
 *     changed EWDB_OpenOCICursors() to include a P3DBCursorMgmtStruct
 *     array.
 *
 *     Revision 1.1  1999/11/09 18:38:26  lucky
 *     Initial revision
 *
 *
 */

  
/* Contains header information for expediting programming with
   the DB Call Level Interfaces such as Oracle Call Interface(OCI),
   and Microsoft's ODBC. David K
******************************************************************/
/* adding more information about the changes made as part of the new
   oci_base layer abstraction.  DK 02/20/2001
******************************************************************/

#ifndef EWDB_CLI_H
# define EWDB_CLI_H

# include <string.h>

/*****************************************************************
 * #defines
 *****************************************************************/
/* Datatype Constants for use with OCI_IFS */
# define OA_INT 1
# define OA_SHORT 2
# define OA_DOUBLE 3
# define OA_FLOAT 4
# define OA_SZ 5
# define OA_CHAR 6
# define OA_LVRAW 9

/* define EWDBid type as int */
# define OA_EWDBID OA_INT

#define EWDB_ERROR_DBRECONNECT 0
#define EWDB_MAX_CURSORS 5
#define EWDB_MAX_STATEMENT_LENGTH 2048


/****************************/
/* #defines to be copied to */
/* your API client header   */
/* file.                    */
/****************************/
#define TRUE 1
#define FALSE 0

#define EWDB_RETURN_SUCCESS 0
#define EWDB_RETURN_FAILURE -1
#define EWDB_RETURN_WARNING 1

/* Debug Levels */
#define EWDB_DEBUG_DB_BASE_NONE                  0
#define EWDB_DEBUG_DB_BASE_CONNECT_INFO          1
#define EWDB_DEBUG_DB_BASE_STATEMENT_PARSE_INFO  2
#define EWDB_DEBUG_DB_BASE_FUNCTION_ENTRY_INFO   4
#define EWDB_DEBUG_DB_API_FUNCTION_ENTRY_INFO    8
#define EWDB_DEBUG_DB_BASE_ALL                  -1

/* size of datatype that holds column data lenghts
   upon return of a fetch function */
#define EWDB_FIELD_RET_LEN sizeof(unsigned short)


/* size of a string buffer large enough to handle a
   double when converted to a string */
#define EWDB_DOUBLE_STRING_LEN 20

/*****************************************************************
 * End #defines
 *****************************************************************/


/*****************************************************************
 * Typedefs and Structures 
 *****************************************************************/

typedef void * EWDB_ConnectionHandle;

typedef void * EWDB_Cursor;

/* Statement Field Struct(EWDB_OCI_SFS) for use with each 
   field in an oracle SQL operation.
************************************************/
typedef struct _EWDB_OCI_SFS
{
  void * pVal;
  unsigned short    UseField;
  short    Ind;
  unsigned short *  pRetLens;
  unsigned short *  pRetCodes;
  int    Type;
  char * FieldID;
  void * pBind;
} EWDB_OCI_SFS;


/* Statement Struct(OCI_InsertStruct) for use with each
   oracle SQL operation
************************************************/
typedef struct _EWDB_OCIStatementStruct
{
  EWDB_Cursor pCda;
  int NumOfFields;
  int BegErrNum;
  int RecordSize;
  EWDB_OCI_SFS * FieldArray; /* Insert Field Struct */
} EWDB_OCIStatementStruct;

typedef struct _EWDBCursorMgmtStruct
{
   char szStatement[EWDB_MAX_STATEMENT_LENGTH];
   int  bCursorInUse;
   int  bCursorIsParsed;
   int  bCursorIsBound;
   int  tLastUse;
} EWDBCursorMgmtStruct;

/*****************************************************************
 * End Typedefs and Structures
 *****************************************************************/

/*****************************************************************
 * EXTERNS  (To be defined by the client program or library)
 *****************************************************************/
extern EWDB_ConnectionHandle hEWDBC;

/* SQL call return code values */
extern int EWDB_SQL_ERROR_NO_DATA;

/* High level debug value */
extern int EWDB_Debug;

/*****************************************************************
 * End EXTERNS
 *****************************************************************/

/*****************************************************************
 * callout function prototypes
 *****************************************************************/
void logit(char*,char*, ... );
/*****************************************************************
 * End callout function prototypes
 *****************************************************************/

/*****************************************************************
 * function prototypes for functions in this file
 *****************************************************************/

int ewdb_base_Init(char * DBuser, char * DBpassword, char * DBservice);
/************************************************************
 * ewdb_base_Init() gets things ready for connecting to a   *
 *  database                                                *
 *                                                          *
 *   Purpose: Initialize the OCI environment & DB connection*
 *                                                          *
 *   Functions Called:                                      *
 *      System: memset,printf                               *
 *      Oracle: opinit                                      *
 *      User:   ewdb_base_Reconnect, ewdb_base_Disconnect             *
 *                             David K  from    Lynn Dietz  *
 ************************************************************/
/*****************************************************************
 *  Initialize the OCI environment & DB connection
 *****************************************************************/

int ewdb_base_Shutdown(void);
/*****************************************************************
 *  Shutdown the OCI environment & DB connection
 *****************************************************************/

int ewdb_base_SetConnectionParams(char * DBuser, char * DBpassword, 
                                  char * DBservice);
/*****************************************************************
 *  EWDB_SetConnectionParams() Set the logon parameters for the
 *  database connection.
 *****************************************************************/

int ewdb_base_Reconnect(); 
/*****************************************************************
 *  ewdb_base_Reconnect() Reconnect to data source & set up bindings*
 *                   for loading an event into database          *
 *****************************************************************/


void ewdb_base_ErrorReport(EWDB_ConnectionHandle hEWDBC, 
                           EWDB_Cursor pCursor, 
                           char *pFuncName, int ErrorID);
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
 *                         that called ewdb_base_ErrorReport().    *
 *    ErrorID   int;       An error ID unique within the      * 
 *                         calling function                   *
 *                                               -Lynn Dietz  *
 **************************************************************/
 /****************************************************************
 *  ewdb_base_ErrorReport() is used to report and log error messages  *
 *  generated during the execution of oracle funtions            *
 *  (oexec,obndrv,oparse,etc.).                                  *
 *****************************************************************/

int ewdb_base_Disconnect(int status );
 /****************************************************************
 *  ewdb_base_Disconnect()  closes the current Database connection  *
 *****************************************************************/

int ewdb_base_RequestCursor(char * szStatement, 
                       EWDB_OCIStatementStruct * pOCISS_DBStruct,
                       int bForceReBind);

int ewdb_base_ReleaseCursor(EWDB_Cursor pCursor);

int ewdb_base_ConnectedToDB(); 
/*****************************************************************
 *  EWDB_ConnectedToDB() allows external functions to figure out *
 *  if we are currently connected to the DB                      *
 *****************************************************************/

int ewdb_base_Set_CLI_Debug(int iDebug);
/*****************************************************************
 *  EWDB_Set_CLI_Debug() sets the debug level for the            *
 *  routines.                                                    *
 *  Debug Levels:
 *                EWDB_DEBUG_DB_BASE_CONNECT_INFO
 *                EWDB_DEBUG_DB_BASE_STATEMENT_PARSE_INFO
 *                EWDB_DEBUG_DB_BASE_FUNCTION_ENTRY_INFO
 *  Debug levels can be |'d together                             *
*****************************************************************/

int ewdb_base_SQLExecute(EWDB_Cursor pCursor);
int ewdb_base_SQLCommit(EWDB_ConnectionHandle hEWDBC);
int ewdb_base_SQLRollback(EWDB_ConnectionHandle hEWDBC);
int ewdb_base_SQLFetchRows(EWDB_Cursor pCursor, int iNumRows);
int ewdb_base_GetCursorRetCode(EWDB_Cursor pCursor);
int ewdb_base_GetCursorRowsProcessedCount(EWDB_Cursor pCursor);
void ewdb_base_SetSQLReturnCodes(void);

/*****************************************************************
 * End function prototypes 
 *****************************************************************/

#endif 
