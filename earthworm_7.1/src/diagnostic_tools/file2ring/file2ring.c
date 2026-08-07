/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: file2ring.c,v 1.1 2001/02/09 21:29:44 dietz Exp $
 *
 *    Revision history:
 *     $Log: file2ring.c,v $
 *     Revision 1.1  2001/02/09 21:29:44  dietz
 *     Initial revision
 *
 *
 *
 */

/*
 *  file2ring:  Reads the contents of a single-message file and places
 *              it in a transport ring with the given message logo.
 *              Useful for feeding known messages to Earthworm modules
 *              for testing.
 *
 *  Lynn Dietz: February 2001
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <earthworm.h>
#include <transport.h>

#define BUFLEN MAX_BYTES_PER_EQ

main( int argc, char *argv[] )
{  
   SHM_INFO      region1;           /* main shared memory region      */
   long          ringkey;           /* key to transport ring          */
   MSG_LOGO      logo;              /* array of module,type,instid    */
   static char   msg[BUFLEN];       /* message from file              */
   int           nread;             /* number of bytes read from file */
   FILE         *fp;

/* Check command line arguments
   ****************************/
   if ( argc != 6 )
   {
        printf( "Usage:   file2ring <filename> <ring_name> <inst_id> "
                                  "<module_id> <msg_type>\n" );
        printf( "Example: file2ring test.arc HYPO_RING INST_MENLO "
                                  "MOD_EQPROC TYPE_HYP2000ARC\n" );
        return( 0 );
   }

/* Look up info from earthworm.h tables
   ************************************/
   if( (ringkey = GetKey(argv[2])) == -1 ) {
        printf( "file2ring: Invalid ring name: %s; exiting!\n",
                 argv[2] );
        return( -1 );
   }
   if ( GetInst( argv[3], &logo.instid ) != 0 ) {
      printf( "file2ring: Invalid installation id: %s; exiting!\n", 
               argv[3] );
      return( -1 );
   }
   if ( GetModId( argv[4], &logo.mod ) != 0 ) {
      printf( "file2ring: Invalid module name: %s; exiting!\n",
               argv[4] );
      return( -1 );
   }
   if ( GetType(  argv[5], &logo.type ) != 0 ) {
      printf( "file2ring: Invalid message type: %s; exiting!\n",
               argv[5] );
      return( -1 );
   }
   tport_attach( &region1, ringkey );

/* Read file into message buffer
 *******************************/
   if( (fp=fopen( argv[1], "rb")) == (FILE *) NULL )
   {
      printf( "file2ring: error opening file <%s>; exiting!\n", 
               argv[1] );
      return( -1 );
   }

   nread = fread( msg, (size_t)1, (size_t)BUFLEN, fp );
   if( !feof(fp) ) {
      printf( "file2ew: Error processing <%s>; ", argv[1] );
      if( nread==BUFLEN ) {
         printf( "file longer than msgbuffer[%d].\n", BUFLEN );
      } else {
         printf( "trouble reading file.\n" );
      }
      fclose(fp);
      return( -1 );
   }   
   fclose(fp);


/* Send archive message to transport ring
 ****************************************/
   if( tport_putmsg( &region1, &logo, nread, msg ) != PUT_OK ) {
       printf( "file2ring: Error writing msg to transport ring.\n");
   }

   tport_detach( &region1 );

   return(0);
}
