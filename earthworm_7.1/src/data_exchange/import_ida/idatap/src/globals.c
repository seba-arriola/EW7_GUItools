#pragma ident "$Id: globals.c,v 1.1 2004/03/17 21:18:34 lombard Exp $"
/*======================================================================
 *
 * Declaration of xfer library globals
 *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 * Copyright (c) 1997 Regents of the University of California.
 * All rights reserved.
 *====================================================================*/
#include "idatap.h"

int xfer_errno    = 0;
int _xfer_timeout = 60;
unsigned long xfer_nsend = 0;
unsigned long xfer_nrecv = 0;

union xfer_buffer _Xfer_Buffer;
char *Xfer_Buffer = (char *) &_Xfer_Buffer;
int Xfer_Buflen   = sizeof(_Xfer_Buffer);

#ifdef IDA_SUPPORT
IDADB *Xfer_DB = (IDADB *) NULL;
#endif

/* Revision History
 *
 * $Log: globals.c,v $
 * Revision 1.1  2004/03/17 21:18:34  lombard
 * Initial revision
 *
 * Revision 1.1.1.1  2000/02/08 20:20:43  dec
 * import existing IDA/NRTS sources
 *
 */
