
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: errexit.c,v 1.2 2004/04/12 22:30:02 dietz Exp $
 *
 *    Revision history:
 *     $Log: errexit.c,v $
 *     Revision 1.2  2004/04/12 22:30:02  dietz
 *     included stdlib.h
 *
 *     Revision 1.1  2000/02/14 18:46:17  lucky
 *     Initial revision
 *
 *
 */
#include <stdlib.h>

void ErrExit( int returnCode )
{
   exit( returnCode );
}
