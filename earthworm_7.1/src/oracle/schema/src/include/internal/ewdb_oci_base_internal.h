/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_oci_base_internal.h,v 1.2 2001/05/15 02:16:27 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_oci_base_internal.h,v $
 *     Revision 1.2  2001/05/15 02:16:27  davidk
 *     Moved functions around between the apps, DB API, and DB API INTERNAL
 *     levels.  Renamed functions and files.  Added support for amplitude
 *     magnitude types.  Reformatted makefiles.
 *
 *     Revision 1.1  2001/04/06 18:22:33  davidk
 *     Initial revision
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
 *     changed EWDB_OpenOCICursors() to include a EWDBCursorMgmtStruct
 *     array.
 *
 *     Revision 1.1  1999/11/09 18:38:26  lucky
 *     Initial revision
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
# include "ewdb_verylocaloci.h"  /* non-Oracle definitions for use with OCI calls */ 

/*****************************************************************
 * #define constants and macros
 *****************************************************************/
/* Macro for HDA_SIZE */
#if (defined(__osf__) && defined(__alpha)) || defined(CRAY) || defined(KSR)
# define HDA_SIZE 512
#else
# define HDA_SIZE 256
#endif
/*****************************************************************
 * End #define constants and macros
 *****************************************************************/

/*****************************************************************
 * Typedefs and Structures 
 *****************************************************************/
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

int EWDB_QOBV(Cda_Def * pCda, void * pVal, sb2 * pInd, int ValType,
         char * ValID, ub2* pLengths, ub2* pRetCodes, sb4 BufSkip,
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
