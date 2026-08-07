
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: pau.c,v 1.5 2007/03/28 18:20:22 paulf Exp $
 *
 *    Revision history:
 *     $Log: pau.c,v $
 *     Revision 1.5  2007/03/28 18:20:22  paulf
 *     added _MACOSX flag
 *
 *     Revision 1.4  2006/04/04 19:02:25  stefan
 *     removing deendency on old .h file
 *
 *     Revision 1.3  2005/07/27 17:25:20  friberg
 *     added _LINUX directive for appropriate startstop.h
 *
 *     Revision 1.2  2001/05/11 20:44:14  dietz
 *     Moved bulk of processing into a separate file, setflags.c.
 *     Pau.c basically just handles command-line arguments now.
 *     No functional changes.
 *
 *     Revision 1.1  2000/02/14 19:04:51  lucky
 *     Initial revision
 *
 *
 */


/*  pau.c   simple program to attach to earthworm's
 *          shared memory region(s) and set the flag
 *          in the header to the terminate flag
 */

/* 5/21/98: changed to allow config file to be specified on the command line.
 * If none is specified, then a default file is used depending on the OS.
 * PNL, UW Geophysics. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <transport.h>

/* include the appropriate definition of DEF_CONFIG for the OS */
#if defined(_SOLARIS) || defined(_LINUX) || defined(_MACOSX)
#include <startstop_unix_generic.h>
#endif

#ifdef _WINNT
#include <startstop_winlib.h>
#endif
#ifdef _OS2
#include <startstop_os2.h>
#endif

int setflags( char *execname, char *cfgfile, int flagvalue );

main( int argc, char **argv )
{
   printf( "pau: Requesting shutdown of entire Earthworm system!\n" );

/* If given, use config file name on command line
   **********************************************/
   if ( argc == 2 )
   {
      setflags( "pau", argv[1], TERMINATE );
   }

/* Otherwise, use default config file
   **********************************/
   else
   {
      printf( "pau: Using default startstop config file %s\n",
               DEF_CONFIG );
      setflags( "pau", DEF_CONFIG, TERMINATE );
   }

   return 0;
}
