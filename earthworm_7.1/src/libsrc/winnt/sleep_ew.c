
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: sleep_ew.c,v 1.1 2000/02/14 18:53:30 lucky Exp $
 *
 *    Revision history:
 *     $Log: sleep_ew.c,v $
 *     Revision 1.1  2000/02/14 18:53:30  lucky
 *     Initial revision
 *
 *
 */

/********************************************************************
 *                 sleep_ew.c    for   Windows NT                   *
 ********************************************************************/

#include <windows.h>

void sleep_ew( unsigned milliseconds )
{
   Sleep( (DWORD) milliseconds );
   return;
}
