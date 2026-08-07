/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: sleep_win.c,v 1.1 2002/12/20 02:40:33 lombard Exp $
 *
 *    Revision history:
 *     $Log: sleep_win.c,v $
 *     Revision 1.1  2002/12/20 02:40:33  lombard
 *     Initial revision
 *
 *
 *
 */


/*************************************************************
 *                         sleep_win.c                       *
 *************************************************************/

#include <windows.h>
#include "makehbfile.h"

void sleep( int sec )
{
    Sleep( 1000 * sec );
    return;
}
