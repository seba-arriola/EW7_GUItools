
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_oci_base.c,v 1.4 2002/03/22 20:50:06 lucky Exp $
 *    Revision history:
 *
 *    $Log: ewdb_oci_base.c,v $
 *    Revision 1.4  2002/03/22 20:50:06  lucky
 *    Pre v6.1 checkin
 *
 *    Revision 1.3  2002/02/18 18:11:40  davidk
 *    Fixed bug in RequestCursor() where iCursorFound was being referenced as an index instead
 *    of i (the loop variable).  iCursorFound was initialized to -1, and so we were referencing
 *    illegal memory, not to mention getting poor results.
 *
 *    Revision 1.2  2001/05/15 02:16:33  davidk
 *    Moved functions around between the apps, DB API, and DB API INTERNAL
 *    levels.  Renamed functions and files.  Added support for amplitude
 *    magnitude types.  Reformatted makefiles.
 *
 *    Revision 1.1  2001/02/28 17:19:22  lucky
 *    Initial revision
 *
 *    Revision 1.11  2001/02/21 09:07:06  davidk
 *    Overhauled ewdb_oci_base.c as part of the change from a
 *    single base layer (oci_base) to a two tier base
 *    layer (db_cli_base -> oci_base).  With the new approach,
 *    the only code that knows anything about Oracle OCI is
 *    this file.  A bunch of new functions and declarations
 *    were added to the file in order to support the list
 *    of functions in ewdb_cli_base.h.  Oracle OCI types
 *    and variables that were previously declared in other
 *    files were pulled into this one, since it is the only
 *    one that now deals with Oracle OCI.
 *
 *    See schema/src/include/internal/ewdb_cli_base.h for more info
 *    on the details of the structure of the changes made.
 *
 *    Revision 1.10  2000/06/21 22:52:32  lucky
 *     Cleaned up logit calls to make log files more readable.
 *
 *    Revision 1.9  2000/04/13 17:15:37  lucky
 *    Changed ewdb_base_Disconnect so that it only logs to file instead of to the screen.
 *
 *    Revision 1.8  2000/03/30 18:58:57  davidk
 *    added code to ewdb_base_RequestCursor() to track the time of last use of
 *    a cursor.  Before, the logic would select the first available cursor
 *    if it could not match against any of the cursors and all of the cursors
 *    were bound.  Now instead of selecting the first available cursor, it
 *    select the oldest available cursor, meaning the one that has been
 *    unused for the longest amount of time.  This hopes to fix a problem
 *    where once all the cursors are bound, then only the first one
 *    gets reselected for use.
 *
 *    Revision 1.7  2000/03/11 00:57:37  davidk
 *    removed old debugging statements, primarily in req_cursor
 *
 *    Revision 1.6  2000/01/04 01:48:08  davidk
 *    added long raw support to QOBV()
 *
 *    Revision 1.5  1999/11/29 22:22:15  davidk
 *    fixed a bug reported by Lucky, where a call to EWDB_ReqCursor() would
 *    cause an Invalid Cursor Error when a statement was executed, if
 *    that statement had been parsed and bound by EWDB_ReqCursor() before
 *    and there had been a EWDB_Disconnect() done in between.
 *    Fixed the problem by initializing the EWDBCursorMgmtStruct within
 *    EWDB_OpenOCICursors(), so that the struct that keeps track of whether
 *    a cursor has already been parsed and bound for a certain statement,
 *    is reinitialized everytime that a DB connection is made and a new
 *    set of cursors is opened up.
 *
 *    Revision 1.4  1999/11/09 18:42:46  lucky
 *    *** empty log message ***
 *
 *    Revision 1.3  1999/10/19 17:45:06  davidk
 *    fixed problem in ewdb_base_RequestCursor, where the szStatement passed in
 *    by the caller was never getting copied over to the szStatement buffer
 *    assigned to the cursor, so we were never getting a match, even on
 *    repetitive calls that should've matched
 *
 *    Revision 1.2  1999/10/19 05:01:46  davidk
 *    changed the members of structures in ewdb_ora_api.h (cosmetic),
 *    and changed the return code Constants.  Made matching changes
 *    in this file
 *
 *    Revision 1.1  1999/05/05 18:41:01  lucky
 *    Initial revision
 *
 *
 */
  
/* oci_base.c */
/* Contains generic functions for expediting programming with
   the Oracle Call Interface(OCI). Requires several callout
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
#include "ewdb_oci_base_internal.h"
/*****************************************************************
 * End #includes
 *****************************************************************/



/*****************************************************************
 * Oracle OCI lookup table for OCI function code labels in the
 * cursor data area:  oci_func_tab
 *****************************************************************/
static CONST text  *oci_func_tab[] =  {(text *) "not used",
/* 1-2 */       (text *) "not used", (text *) "OSQL",
/* 3-4 */       (text *) "not used", (text *) "OEXEC, OEXN",
/* 5-6 */       (text *) "not used", (text *) "OBIND",
/* 7-8 */       (text *) "not used", (text *) "ODEFIN",
/* 9-10 */      (text *) "not used", (text *) "ODSRBN",
/* 11-12 */     (text *) "not used", (text *) "OFETCH, OFEN",
/* 13-14 */     (text *) "not used", (text *) "OOPEN",
/* 15-16 */     (text *) "not used", (text *) "OCLOSE",
/* 17-18 */     (text *) "not used", (text *) "not used",
/* 19-20 */     (text *) "not used", (text *) "not used",
/* 21-22 */     (text *) "not used", (text *) "ODSC",
/* 23-24 */     (text *) "not used", (text *) "ONAME",
/* 25-26 */     (text *) "not used", (text *) "OSQL3",
/* 27-28 */     (text *) "not used", (text *) "OBNDRV",
/* 29-30 */     (text *) "not used", (text *) "OBNDRN",
/* 31-32 */     (text *) "not used", (text *) "not used",
/* 33-34 */     (text *) "not used", (text *) "OOPT",
/* 35-36 */     (text *) "not used", (text *) "not used",
/* 37-38 */     (text *) "not used", (text *) "not used",
/* 39-40 */     (text *) "not used", (text *) "not used",
/* 41-42 */     (text *) "not used", (text *) "not used",
/* 43-44 */     (text *) "not used", (text *) "not used",
/* 45-46 */     (text *) "not used", (text *) "not used",
/* 47-48 */     (text *) "not used", (text *) "not used",
/* 49-50 */     (text *) "not used", (text *) "not used",
/* 51-52 */     (text *) "not used", (text *) "OCAN",
/* 53-54 */     (text *) "not used", (text *) "OPARSE",
/* 55-56 */     (text *) "not used", (text *) "OEXFET",
/* 57-58 */     (text *) "not used", (text *) "OFLNG",
/* 59-60 */     (text *) "not used", (text *) "ODESCR",
/* 61-62 */     (text *) "not used", (text *) "OBNDRA"
};
/*****************************************************************
 * End oci_func_tab
 *****************************************************************/



/*****************************************************************
 * Global Variables 
 *****************************************************************/
static ub4 EWDB_Hda[HDA_SIZE/(sizeof(ub4))];

static Lda_Def EWDB_Lda;

static int EWDB_connected=FALSE;
/*char OCIB_Buffer[OCIB_BUFFERSIZE];*/

static EWDBCursorMgmtStruct pcmsCursors[EWDB_MAX_CURSORS];
static Cda_Def EWDBCursorList[EWDB_MAX_CURSORS];

static mutex_t EWDBCursorMutex;

static char EWDBuser[40];
static char EWDBpassword[20];
static char EWDBservice[20];

static int  EWDB_Base_Debug = EWDB_DEBUG_DB_BASE_NONE;

/* single global connection handle (ugh!!!) 
   NO MULTIPLE CONNECTIONS THROUGH API */
EWDB_ConnectionHandle hEWDBC;

/* SQL call return code values */
int EWDB_SQL_ERROR_NO_DATA;

/*****************************************************************
 * End Global Variables 
 *****************************************************************/


/*****************************************************************
 * FUNCTIONS
 *****************************************************************/

/*****************************************************************
 *  ewdb_base_Reconnect() Reconnect to data source & set up      *
 *   bindings for loading an event into database                 *
 *****************************************************************/
int ewdb_base_Reconnect() 
{


/* olog is the Oracle logon function which opens a connection to the 
   Oracle database indicated by szDSN, for user szUser with password szPWD. 
   OCI_LM_DEF indicates that you want the default BLOCKING style connection.
 ***************************************************************************/

   if (!EWDB_connected)
   {
     if(EWDB_Base_Debug & EWDB_DEBUG_DB_BASE_CONNECT_INFO)
       logit("","EWDB_BASE:Debug:ewdb_base_Reconnect():  Connecting to DB.\n");
     memset(EWDB_Hda,0,HDA_SIZE);
     memset(&EWDB_Lda,0,sizeof(Lda_Def));

     if( olog(&EWDB_Lda, (ub1 *)EWDB_Hda, (text *)EWDBuser,     OCI_NTS, 
              (text *)EWDBpassword, OCI_NTS, 
              (text *)EWDBservice,  OCI_NTS, 

              OCI_LM_DEF ) )
     {
       ewdb_base_ErrorReport((EWDB_ConnectionHandle)&EWDB_Lda,
                             (EWDB_Cursor)&EWDB_Lda, "ewdb_base_Reconnect:olog",
                             EWDB_ERROR_DBRECONNECT +1);
       return(EWDB_RETURN_FAILURE);
     }

     /* Assign the connection handle to our EWDB_Lda */
     hEWDBC = (void *) (&EWDB_Lda);
     
     /* Open all cursors (oopen) that we'll use: 
     ***************************************************************/

     /* Cda is the oracle abbreviation for a cursor.
     */

     /* Create the mutex for tracking cursor check_out */
     CreateSpecificMutex(&EWDBCursorMutex);

     /* Open the cursors */
     if(EWDB_OpenOCICursors(EWDBCursorList /* Cursor Array */,
		pcmsCursors,
        EWDB_MAX_CURSORS,
        EWDB_ERROR_DBRECONNECT + 10 /* Starting ErrNum */))
     {
       return(EWDB_RETURN_FAILURE);
     }

     /* Turn off auto-commit. Default is off, however 
     *********************************************/
     if (ocof(&EWDB_Lda))
     {
       ewdb_base_ErrorReport((EWDB_ConnectionHandle)&EWDB_Lda,
                        (EWDB_Cursor)&EWDB_Lda, "ewdb_base_Reconnect:ocof",
                        EWDB_ERROR_DBRECONNECT+2);
       return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE) );
     } 

     EWDB_connected=TRUE;
   } /* End if !DB_connected */
   return( EWDB_RETURN_SUCCESS );
}  /* End ewdb_base_Reconnect */


/*****************************************************************
 *  EWDB_OpenOCICursors() Open all Oracle Cursors (from an array)*
 *   for use                                                     *
 *****************************************************************/
int EWDB_OpenOCICursors(Cda_Def * Cdaa, EWDBCursorMgmtStruct * PCMSa,
						int NumOfCursors, int ErrNum)
/* Cdaa         Cda Array
   PCMSa        Array of Cursor MgmtStructs
   NumOfCursors Number of cursors in array
   ErrNum       the error number for the oopen call of the first
                cursor.  Used in case of error, as an argument to
                ewdb_base_ErrorReport.
*/
{
  int CursCtr;
   for(CursCtr=0; CursCtr<NumOfCursors; CursCtr++)
   {
     if (oopen(&(Cdaa[CursCtr]), &EWDB_Lda, (text *) 0, -1, -1, (text *) 0, -1))
     {
       ewdb_base_ErrorReport((EWDB_ConnectionHandle)&EWDB_Lda,
                        (EWDB_Cursor)&(Cdaa[CursCtr]),
                        "OpenOCICursors",ErrNum+CursCtr);
       return(ewdb_base_Disconnect(EWDB_RETURN_FAILURE) );
     }
	 memset(&(PCMSa[CursCtr]),0,sizeof(EWDBCursorMgmtStruct));
   }
   return(EWDB_RETURN_SUCCESS);
}  /* End EWDB_OpenOCICursors */


/*****************************************************************
 *  EWDB_CloseOCICursors() Close all Oracle Cursors from an array*
 *****************************************************************/
int EWDB_CloseOCICursors(Cda_Def * Cdaa, int NumOfCursors, int ErrNum)
/* Cdapa        Cda Pointer Array
   NumOfCursors Number of cursors in array
   ErrNum       the error number for the oopen call of the first
                cursor.  Used in case of error, as an argument to
                ewdb_base_ErrorReport.
*/
{
  int CursCtr;
   for(CursCtr=0; CursCtr<NumOfCursors; CursCtr++)
   {
     if (oclose(&(Cdaa[CursCtr])))
     {
       ewdb_base_ErrorReport((EWDB_ConnectionHandle)&EWDB_Lda,
                        (EWDB_Cursor)&(Cdaa[CursCtr]),
                        "CloseOCICursors",ErrNum+CursCtr);
       return(EWDB_RETURN_FAILURE);  /* This function is called by ewdb_base_Disconnect() */
     }
   }
   return(EWDB_RETURN_SUCCESS);
}  /* End EWDB_CloseOCICursors */


/*****************************************************************
 *  ewdb_base_QuickBindForStatement( )  OCI bind all of the      *
 *  variables for a SQL statement.                               *
 *****************************************************************/
int ewdb_base_QuickBindForStatement(EWDB_OCIStatementStruct * pSS, 
                                    char * CallingFunction,int BegErrNum)
{
  int i;
  EWDB_OCI_SFS * pCurr;


  for(i=0; i < pSS->NumOfFields; i++)
  {
    pCurr=&(pSS->FieldArray[i]);


    if(pCurr->UseField)
    {
      if(EWDB_QOBV(pSS->pCda,pCurr->pVal,&(pCurr->Ind),pCurr->Type,
        pCurr->FieldID,pCurr->pRetLens,pCurr->pRetCodes,pSS->RecordSize,
        CallingFunction,BegErrNum+i))
      {
        return(EWDB_RETURN_FAILURE);
      }

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
int EWDB_QOBV(Cda_Def * pCda, void * pVal, sb2 * pInd, int ValType,
         char * ValID, ub2* pLengths, ub2* pRetCodes, sb4 BufSkip,
         char * FuncName, int FuncNum)
{
/* WHAT THE HECK IS THIS??? DK990924 */
# define MAX_ARRAY_LENGTH 100  

  static sword size;
  static int OValType;
  static sb2 DummyInd[MAX_ARRAY_LENGTH];         /* Dummy Indicator Array */
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
      break;
    }
  case OA_FLOAT:
    {
      OValType=VARCHAR2_TYPE;
      size= OCI_NTS;
      break;
    }
  case OA_SZ:
    {
      OValType=VARCHAR2_TYPE;
      size= OCI_NTS;
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
    if(obndrv(pCda,(text *) ValID, OCI_NTS,(ub1 *)pVal, 
      size, OValType,-1,pInd,(text *)0,-1,-1))
    {
      ewdb_base_ErrorReport(&EWDB_Lda,pCda,FuncName,FuncNum);
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
      if(odefinps(/* Cursor Pointer */ pCda, 
                  /*Opcode:Array of Structs*/ 1, 
                  /*Position in SQL stmt*/ atoi(ValID),
                  /*Pointer to my Buffer */(ub1 *)pVal, 
                  /*Size of Buffer */size, 
                  /*Type of C Var */OValType, 
                  /*Scale Not Used in C */0,
                  /*Indicator Array */pInd,
                  /*fmt Not Used in C */0,
                  /*fmtl Not Used in C */0,
                  /*fmtt Not Used in C */0,
                  /*Array of My Buffer Len's*/pLengths, 
                  /*Array of Return Codes*/0, 
                  /*Size to skip between bufs in array*/0,
                  /*Size to skip between ind bufs in array*/0,
                  /*Size to skip between len's in array*/0,
                  /*Size to skip between retcodes in array*/0))
      {      
        logit("","Error during LONGVARRAW_TYPE odefin()\n");
        ewdb_base_ErrorReport(&EWDB_Lda,pCda,FuncName,FuncNum);
        return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE) );
      }
    }  /* OValType != LONGVARRAW_TYPE */
    else
    {
      if(odefinps(/* Cursor Pointer */ pCda, 
                  /*Opcode:Array of Structs*/ 1, 
                  /*Position in SQL stmt*/ atoi(ValID),
                  /*Pointer to my Buffer */(ub1 *)pVal, 
                  /*Size of Buffer */size, 
                  /*Type of C Var */OValType, 
                  /*Scale Not Used in C */0,
                  /*Indicator Array */DummyInd,
                  /*fmt Not Used in C */0,
                  /*fmtl Not Used in C */0,
                  /*fmtt Not Used in C */0,
                  /*Array of My Buffer Len's*/pLengths,
                  /*Array of Return Codes*/pRetCodes, 
                  /*Size to skip between bufs in array*/BufSkip,
                  /*Size to skip between ind bufs in array*/0,
                  /*Size to skip between len's in array*/2,
                  /*Size to skip between retcodes in array*/0))
      {      
        logit("","Error during odefin()\n");
        ewdb_base_ErrorReport(&EWDB_Lda,pCda,FuncName,FuncNum);
        return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE) );
      }
    }  /* OValType != LONGVARRAW_TYPE */
  }

  return(EWDB_RETURN_SUCCESS);
}  /* End EWDB_QOBV() */


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
    sword  n;
    text   msg[513];
    Lda_Def *pLda;
    Cda_Def *pCda;

    pLda = (Lda_Def *)hEWDBC;
    pCda = (Cda_Def *)pCursor;

    n = oerhms(pLda, pCda->rc, msg, (sword) sizeof(msg) );
    logit("t", "Error: %s", msg);
    if (pCda->fc > 0)
    {
        logit("", " in OCI function %s",
               oci_func_tab[pCda->fc]);
    }
    logit("",  " [error# %d in %s]\n", ErrorID, pFuncName);

    return;
}  /* End ewdb_base_ErrorReport() */


/**************************************************************
 *  ewdb_base_Disconnect()  closes the current Database       *
 *    connection.                                 Lynn Dietz  *
 **************************************************************/
int ewdb_base_Disconnect(int status )
{

    if(EWDB_Base_Debug)
      logit("","EWDB_Base:Debug:ewdb_base_Disconnect():  Disconnecting from DB.\n");
    EWDB_CloseOCICursors(EWDBCursorList,EWDB_MAX_CURSORS,10000);
    EWDB_connected=0;
    /* free any malloced memory for indicators, buffers, 
       or cursors */
    ologof(&EWDB_Lda);

    /* Close the mutex for tracking cursor check_out */
    CloseSpecificMutex(&EWDBCursorMutex);

    return(status);
}  /* End ewdb_base_Disconnect() */



/************************************************************
 * ewdb_base_Init() gets things ready for connecting to a   *
 *  database                                                *
 *                                                          *
 *   Purpose: Initialize the OCI environment & DB connection*
 *                                                          *
 *   Functions Called:                                      *
 *      System: memset,printf                               *
 *      Oracle: opinit                                      *
 *      User:   ewdb_base_Reconnect, ewdb_base_Disconnect        *
 *                             David K  from    Lynn Dietz  *
 ************************************************************/
int ewdb_base_Init(char * DBuser, char * DBpassword, char * DBservice)
{

/* opinit is the initial Oracle OCI function.
   Before using any other OCI calls, set OCI to:
      multi-thread safe  (OCI_EV_TSF) 
      or single-threaded (OCI_EV_DEF)
 ***********************************************/
   opinit( OCI_EV_TSF );

   if(ewdb_base_SetConnectionParams(DBuser,DBpassword,DBservice) 
      == EWDB_RETURN_FAILURE)
     return(EWDB_RETURN_FAILURE);

   /* set return code constants with OCI specific values */
   ewdb_base_SetSQLReturnCodes();

   if(ewdb_base_Reconnect() == EWDB_RETURN_SUCCESS)
     return(EWDB_RETURN_SUCCESS);
   else
     return(EWDB_RETURN_FAILURE);
}  /* ewdb_base_Init() */


/************************************************************
 * ewdb_base_Shutdown() free's up all DB related params     *
 *                                                          *
 *   Purpose: shutdown the OCI environment.                 *
 *                                                          *
 *   Functions Called:                                      *
 *      None                                                *
 *                             David K  from    Lynn Dietz  *
 ************************************************************/
int ewdb_base_Shutdown(void)
{

  /* OCI 7 does not provide a shutdown function, only a log-off. 
     DK 04/17/2001 */
  return(EWDB_RETURN_SUCCESS);

}  /* ewdb_base_Shutdown() */


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


int ewdb_base_RequestCursor(char * szStatement, 
                            EWDB_OCIStatementStruct * pOCISS_DBStruct,
                            int bForceReBind)
{
  int iCursorFound=-1;
  int i;
  int iLoopPassCount=1;
  int tMin=2000000000;  /* I forget what the name of MAX_INT is */

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

/****  COMMENTING THIS OUT - it's producing unreadable log files

   Lucky 2/27/02

          logit("t","ewdb_base_RequestCursor(): looping through %dth cursor at (%u).\n", iCursorFound,pcmsCursors);

****/
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

  /* Now we have a cursor, but we have to make sure its prepped */
  pOCISS_DBStruct->pCda=&(EWDBCursorList[iCursorFound]);

  /* If the cursor is not yet parsed, parse it */
  if(!pcmsCursors[iCursorFound].bCursorIsParsed)
  {
    /* parse the statement into the cursor */
    if( oparse(&(EWDBCursorList[iCursorFound]), (text *) pcmsCursors[iCursorFound].szStatement,
               OCI_NTS,0 /* OCI_DEFER_FULL_PARSE*/, (ub4) OCI_VERSION_7 ))
    {
      ewdb_base_ErrorReport(&EWDB_Lda,&(EWDBCursorList[iCursorFound]),"ewdb_base_RequestCursor",1);
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
    if(ewdb_base_QuickBindForStatement(pOCISS_DBStruct, "ewdb_base_RequestCursor",100))
    {
      logit("","Bind failed for sql string: [%s]\n",szStatement);
      return( ewdb_base_Disconnect(EWDB_RETURN_FAILURE) );
    }
    pcmsCursors[iCursorFound].bCursorIsBound=1;
  }


  pcmsCursors[iCursorFound].bCursorInUse = TRUE;

  ReleaseSpecificMutex(&EWDBCursorMutex);

  return(0);
}  /* End ewdb_base_RequestCursor() */


int ewdb_base_ReleaseCursor(EWDB_Cursor pCursor)
{
  int i;

  i=( ((int) pCursor) - ((int) EWDBCursorList) ) 
      / sizeof(Cda_Def);

  if(i < 0 || i > EWDB_MAX_CURSORS)
  {
    /* invalid cursor pointer!!!  (or bad logic on our part) */
    return(-2);
  }

  pcmsCursors[i].bCursorInUse = FALSE;

  return(0);
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


int ewdb_base_SQLExecute(EWDB_Cursor pCursor)
{
  return(oexec((Cda_Def *)pCursor));
}

int ewdb_base_SQLCommit(EWDB_ConnectionHandle hEWDBC)
{
  return(ocom((Lda_Def *)hEWDBC));
}

int ewdb_base_SQLRollback(EWDB_ConnectionHandle hEWDBC)
{
  return(orol((Lda_Def *)hEWDBC));
}

int ewdb_base_SQLFetchRows(EWDB_Cursor pCursor, int iNumRows)
{
  return(ofen((Cda_Def *)pCursor, iNumRows));
}

int ewdb_base_GetCursorRetCode(EWDB_Cursor pCursor)
{
  return(((Cda_Def *)pCursor)->rc);
}

int ewdb_base_GetCursorRowsProcessedCount(EWDB_Cursor pCursor)
{
  return(((Cda_Def *)pCursor)->rpc);
}

void ewdb_base_SetSQLReturnCodes(void)
{
  EWDB_SQL_ERROR_NO_DATA = 1403;
}


/*****************************************************************
 * End FUNCTIONS
 *****************************************************************/

