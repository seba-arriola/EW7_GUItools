/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_system_support.h,v 1.5 2002/04/16 20:49:46 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_system_support.h,v $
 *     Revision 1.5  2002/04/16 20:49:46  davidk
 *     fixed the new macro (too many parins).
 *
 *     Revision 1.4  2002/04/16 20:48:20  davidk
 *     fixed newline.
 *
 *     Revision 1.3  2002/04/16 20:46:00  davidk
 *     Added a THREAD_RETURN macro to avoid warning messages on NT,
 *     which treats a thread_return value as a VOID.  The macro
 *     does a normal return(value); return on all platforms other than
 *     NT.  On NT it does a void return.
 *
 *     Revision 1.2  2001/04/06 18:41:17  davidk
 *     removed all of the superfluous definitions that are already
 *     done in earthworm.h.
 *
 *     Revision 1.1  1999/11/09 18:38:26  lucky
 *     Initial revision
 *
 *
 */

#include <earthworm.h>


/* define a THREAD_RETURN function, to get rid of warnings on
   platforms where thread functions return a void 
   DK 0411/2002
   **********************************************/
#ifdef _WINNT
# define THREAD_RETURN(x) return
#else
# define THREAD_RETURN(x) return(x)
#endif

