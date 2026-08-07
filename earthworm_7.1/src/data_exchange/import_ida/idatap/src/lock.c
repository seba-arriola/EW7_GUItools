#pragma ident "$Id: lock.c,v 1.1 2004/03/17 21:18:34 lombard Exp $"
/*======================================================================
 *
 *  Record locking utilities.
 *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 * Copyright (c) 1997 Regents of the University of California.
 * All rights reserved.
 *====================================================================*/
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include "idatap.h"

int util_lock(int fd, int cmd, int type, off_t offset, int whence, off_t len)
{
struct flock lock;

    lock.l_type   = type;
    lock.l_start  = offset;
    lock.l_whence = whence;
    lock.l_len    = len;

    return fcntl(fd, cmd, &lock);
}

/* Revision History
 *
 * $Log: lock.c,v $
 * Revision 1.1  2004/03/17 21:18:34  lombard
 * Initial revision
 *
 * Revision 1.2  2001/05/07 22:40:13  dec
 * ANSI function declarations
 *
 * Revision 1.1.1.1  2000/02/08 20:20:41  dec
 * import existing IDA/NRTS sources
 *
 */
