/* @(#)util.h	2.12 01/31/97 */
/*======================================================================
 *
 *  include/util.h
 *
 *  Defines, data structures, and function prototypes for use 
 *  with the util library.
 *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 * Copyright (c) 1997 Regents of the University of California.
 * All rights reserved.
 *====================================================================*/
#ifndef util_h_included
#define util_h_included

#include "ida_platform.h"

#ifndef MAXPATHLEN
#define MAXPATHLEN 1024
#endif

/*  Typical machine byte orders  */

#ifndef LTL_ENDIAN_ORDER
#define LTL_ENDIAN_ORDER 0x10325476  /* VAX, 80x86   */
#endif

#ifndef BIG_ENDIAN_ORDER
#define BIG_ENDIAN_ORDER 0x76543210  /* Sun, MC680x0 */
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef SEEK_SET
#define SEEK_SET 0
#endif

#ifndef SEEK_CUR
#define SEEK_CUR 1
#endif

#ifndef SEEK_END
#define SEEK_END 2
#endif

/* Macros */

#ifdef strerror
#define syserrmsg(code) strerror(code)
#else
char *syserrmsg(int);
#endif

#ifndef leap_year
#define leap_year(i) ((i % 4 == 0 && i % 100 != 0) || i % 400 == 0)
#endif

#ifndef daysize
#define daysize(i) (365 + leap_year(i))
#endif

#define year_secs(year) (long) util_ydhmsmtod((int)year, 0, 0, 0, 0, 0)

#ifdef MSDOS
#include <fcntl.h>
#include <io.h>
#define SETMODE(fd, mode) setmode((fd), (mode))
#else
#define SETMODE(fd, mode) 1
#ifndef O_TEXT
#define O_TEXT 0x4000
#endif
#ifndef O_BINARY
#define O_BINARY 0x8000
#endif
#endif /* ifdef MSDOS */

/* Various record locking macros */

#define util_rlock(fd, offset, whence, len) \
        util_lock(fd, F_SETLK, F_RDLCK, offset, whence, len)

#define util_wlock(fd, offset, whence, len) \
        util_lock(fd, F_SETLK, F_WRLCK, offset, whence, len)

#define util_rlockw(fd, offset, whence, len) \
        util_lock(fd, F_SETLKW, F_RDLCK, offset, whence, len)

#define util_wlockw(fd, offset, whence, len) \
        util_lock(fd, F_SETLKW, F_WRLCK, offset, whence, len)

#define util_unlock(fd, offset, whence, len) \
        util_lock(fd, F_SETLKW, F_UNLCK, offset, whence, len)

/*  Misc. useful constants  */

#define BYTES_PER_KBYTE 1024
#define BYTES_PER_MBYTE 1048576
#define BYTES_PER_GBYTE 1073741824

/* Typedefs */

typedef void    Sigfunc(int);

#ifdef strerror
#define syserrmsg(code) strerror(code)
#else
char *syserrmsg(int);
#endif

#endif
