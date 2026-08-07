/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: chdir_sol.c,v 1.1 2002/12/20 02:39:11 lombard Exp $
 *
 *    Revision history:
 *     $Log: chdir_sol.c,v $
 *     Revision 1.1  2002/12/20 02:39:11  lombard
 *     Initial revision
 *
 *
 *
 */

/*
 *   chdir_sol.c
 */

#include <unistd.h>
#include "getfileII.h"

/*****************************************************************************
 *  chdir_ew( )  changes current working directory; Solaris version          *
 *****************************************************************************/

int chdir_ew( char *path )
{
    return( chdir( path ) );
}

