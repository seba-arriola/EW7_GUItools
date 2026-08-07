
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: reboot_mss.c,v 1.2 2001/04/26 22:32:25 kohler Exp $
 *
 *    Revision history:
 *     $Log: reboot_mss.c,v $
 *     Revision 1.2  2001/04/26 22:32:25  kohler
 *     Added -l option to logout MSS100 serial port.
 *
 *     Revision 1.1  2001/04/25 23:44:18  kohler
 *     Initial revision
 *
 *
 *
 */
       /**********************************************************
        *                       reboot_mss                       *
        *                                                        *
        *              Program to reboot an MSS100               *
        *             or to log off its serial port.             *
        *                                                        *
        *  Usage: reboot_mss [IP address] [password] [-q] [-l]   *
        **********************************************************/

#include <stdio.h>
#include <string.h>

int  RebootMSS100( char [], char [] );
void SocketSysInit( void );

int Quiet  = 0;                  /* If 0, don't print anything to stdout */
int Logout = 0;                  /* If 0, reboot MSS100.  If 1, logout port 1 */


int main( int argc, char *argv[] )
{
   int  i;
   char ServerIP[20];            /* IP address of the MSS to reboot */
   char Password[20];            /* Password of priviledged MSS user */

/* Get command line arguments
   **************************/
   if ( argc < 3 )
   {
      printf( "\nUsage: reboot_mss <IP address> <password> [-q] [-l]\n\n" );
      printf( "The -q (quiet) and -l (logout) arguments are optional, and,\n" );
      printf( "if present, they must appear at the end of the command line.\n" );
      printf( "If -q is specified, nothing is written to stdout.\n" );
      printf( "If -l is specified, the MSS serial port is logged out,\n" );
      printf( "and no reboot occurs.\n" );
      return -1;
   }

   strcpy( ServerIP, argv[1] );
   strcpy( Password, argv[2] );

   for ( i = 3; i < argc; i++ )
   {
      if ( !strcmp(argv[i],"-q") ) Quiet  = 1;
      if ( !strcmp(argv[i],"-l") ) Logout = 1;
   }

/* Initialize the socket system
   ****************************/
   SocketSysInit();

/* Invoke the reboot function
   **************************/
   if ( RebootMSS100( ServerIP, Password ) < 0 )
   {
      if ( !Quiet )
         if ( Logout == 0 )
            printf( "Error rebooting the MSS100.\n" );
         else if ( Logout == 1 )
            printf( "Error logging off the MSS100 serial port.\n" );
      return -1;
   }

/* Success!
   ********/
   if ( !Quiet )
      if ( Logout == 0 )
         printf( "The MSS100 was successfully rebooted.\n" );
      else if ( Logout == 1 )
         printf( "The MSS100 serial port was successfully logged off.\n" );

   return 0;
}
