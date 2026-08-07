/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_oci8_base_internal.h,v 1.1 2005/05/12 19:08:57 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_oci8_base_internal.h,v $
 *     Revision 1.1  2005/05/12 19:08:57  davidk
 *     Header file for base layer using OCI8+ routines.
 *
 *
 *
 */

  
/* Contains internal header information for ewdb_oci_base.c, which
   was created for expediting programming with
   the Oracle Call Interface(OCI). David K
******************************************************************/

#ifndef EWDB_OCI_BASE_INTERNAL_H
# define EWDB_OCI_BASE_INTERNAL_H

# include <string.h>
//# include "ewdb_verylocaloci.h"  /* non-Oracle definitions for use with OCI calls */ 
#include <oci.h>

/*****************************************************************
 * #define constants and macros
 *****************************************************************/
/* Oracle DataType Codes
 * internal/external datatype codes 
 **********************************/
#define VARCHAR2_TYPE         1   /* char[n], n<=2000                  */
#define NUMBER_TYPE           2   /* unsigned char[21]                 */
#define INT_TYPE              3   /* signed char,short,int, or long    */
#define FLOAT_TYPE            4   /* float, double                     */
#define STRING_TYPE           5   /* char[n+1] (null-terminated-string)*/
#define ROWID_TYPE           11   /* char[n]                           */
#define DATE_TYPE            12   /* char[7]                           */
#define LONGVARRAW_TYPE      24   /* unsigned char[n+ilen]             */
#define UNSIGNED_TYPE        68   /* unsigned integers                 */
#define CHAR_TYPE            96   /* fixed-length char[n] n<=255       */

#define OCI_NTS              -1   /* NULL Terminated String <not 
                                     supported by OCI8, but used as a
                                     placeholder>                      */

/*****************************************************************
 * End #define constants and macros
 *****************************************************************/

/*****************************************************************
 * Typedefs and Structures 
 *****************************************************************/
typedef struct _OCI8HandleStruct
{
   OCIEnv    * phOCI8Env;
   OCIError  * phOCI8Err;
   OCISvcCtx * phOCI8Conn;
   int         iRetCode;
}  OCI8HandleStruct;

typedef struct _OCI8CursorStruct
{
  OCI8HandleStruct * pHS;
  OCIStmt          * phStmt;
  int                iRetCode;
} OCI8CursorStruct;

/*****************************************************************
 * End Typedefs and Structures 
 *****************************************************************/

/*****************************************************************
 * EXTERNS  (To be defined by the client program or library)
 *****************************************************************/
/*****************************************************************
 * End EXTERNS 
 *****************************************************************/

/*****************************************************************
 * function prototypes for functions in this file 
 *****************************************************************/

/*****************************************************************
 *  EWDB_QuickBindForStatement( )  OCI bind all of the variables *
 *  for a SQL statement.                                         *
 *****************************************************************/
int ewdb_base_QuickBindForStatement(EWDB_OCIStatementStruct * pSS, 
                                    char * CallingFunction,int BegErrNum,
                                    int bAlreadyBound);

int EWDB_OpenOCICursors(Cda_Def * Cdaa, EWDBCursorMgmtStruct * PCMSa,
			int NumOfCursors, int ErrNum);
/*****************************************************************
 *  EWDB_OpenOCICursors() Open all Oracle Cursors (from an array)*
 *   for use                                                     *
 *****************************************************************/

int EWDB_CloseOCICursors(Cda_Def * Cdaa, int NumOfCursors, int ErrNum);
/*****************************************************************
 *  EWDB_CloseOCICursors() Close all Oracle Cursors from an array*
 *****************************************************************/

int EWDB_QOBV(OCI8CursorStruct * pCurs, void * pVal, sb2 * pInd, int ValType,
         char * ValID, ub2** ppLengths, ub2* pRetCodes, OCIBind ** ppBind, sb4 BufSkip,
         char * FuncName, int FuncNum);
/*****************************************************************
 *  EWDB_QOBV is Quick Oracle Bind by Value.  It is a wrapper    *
 *  around the OCI bind calls obndrv() and odefinps().  It       *
 *  encapsulates some of the complexity that comes with all of   *
 *  options in the Oracle calls that bind C variables to SQL vars*
 *****************************************************************/

/*****************************************************************
 * End function prototypes 
 *****************************************************************/



#endif 
