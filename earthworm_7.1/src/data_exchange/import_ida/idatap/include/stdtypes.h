#pragma ident "$Id: stdtypes.h,v 1.2 2005/04/20 18:26:25 davidk Exp $"
/*======================================================================
 *
 * Portable data types
 *
 *====================================================================*/
#ifndef stdtypes_h_included
#define stdtypes_h_included

#if defined (X86_WIN32)

#include <windows.h>

 /* Characters */
typedef char CHAR;

 /* Boolean values */
 /* BOOL is defined by the Win32 API in windows.h */

 /* Signed and unsigned 8 bit integers */
#ifndef INT8
typedef signed __int8 INT8;
#endif
#ifndef UINT8
typedef unsigned __int8 UINT8;
#endif

 /* 16 bit integer values */
#ifndef INT16
typedef signed __int16 INT16;
#endif
#ifndef UINT16
typedef unsigned __int16 UINT16;
#endif UINT16

 /* 32 bit integer values */
/* INT32 is typedef'd in WIN32 in BASETSD.H which is included by windows.h 
 * #ifndef INT32
 * typedef signed __int32 INT32;
 * #endif
 * #ifndef UINT32
 * typedef unsigned __int32 UINT32;
 * #endif
 **** SO NO NEED FOR THESE TYPEDEFS.  OBVIOUSLY THE IFDEF doesn't work
 **** since the values are typedef'd, not #defined in the preprocessor.
 *********************************************************************/

 /* 64 bit integers */
typedef signed __int64 INT64;
typedef unsigned __int64 UINT64;

 /* 32 bit IEEE 754 Real */
typedef float REAL32;

 /* 64 bit IEEE 754 Real */
typedef double REAL64;

 /* 80 bit IEEE 754 Real */
typedef long double REAL80;

#elif defined(X86_UNIX32) || defined(SPARC_UNIX32)

#include <inttypes.h>

    /* Void type */
typedef void VOID;

 /* Characters */
typedef char CHAR;

 /* Signed and unsigned 8 bit integers */
typedef int8_t INT8;
typedef uint8_t UINT8;

 /* 16 bit integer values */
typedef int16_t INT16;
typedef uint16_t UINT16;

 /* 32 bit integer values */
typedef int32_t INT32;
typedef uint32_t UINT32;

 /* 64 bit integers */
typedef int64_t INT64;
typedef uint64_t UINT64;

 /* 32 bit IEEE 754 Real */
typedef float REAL32;

 /* 64 bit IEEE 754 Real */
typedef double REAL64;

 /* 80 bit IEEE 754 Real */
typedef long double REAL80;

 /* Boolean values */
typedef INT8 BOOL;

#else

#    error "can't determine platform!"

#endif

#ifndef TRUE
#    define TRUE  ((BOOL) 1)
#endif

#ifndef FALSE
#    define FALSE ((BOOL) 0)
#endif

#endif

/* Revision History
 *
 * $Log: stdtypes.h,v $
 * Revision 1.2  2005/04/20 18:26:25  davidk
 * Removed conditional typedef of INT32 and UINT32 for windows, as those
 * values are already typedef'd in the windows header files that are included.
 * The removal fixes compiler warning about benign type redefinition.
 *
 * Revision 1.1  2004/03/17 21:18:03  lombard
 * Initial revision
 *
 */
