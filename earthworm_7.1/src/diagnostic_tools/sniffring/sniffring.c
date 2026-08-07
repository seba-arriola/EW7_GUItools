
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: sniffring.c,v 1.10 2007/03/13 00:59:32 stefan Exp $
 *
 *    Revision history:
 *     $Log: sniffring.c,v $
 *     Revision 1.10  2007/03/13 00:59:32  stefan
 *     minor doc addition
 *
 *     Revision 1.9  2007/03/13 00:42:32  stefan
 *     typo
 *
 *     Revision 1.8  2007/02/07 00:19:11  stefan
 *     help grammar
 *
 *     Revision 1.7  2007/02/02 19:06:30  stefan
 *     changed default behavior to what it was before, removed module verbosity for non-local modules
 *
 *     Revision 1.6  2006/12/01 01:24:22  stefan
 *     changed default to print names instead of numbers, with option to still print numbers the way it used to
 *
 *     Revision 1.5  2004/04/16 20:58:20  dietz
 *     Modified to skip printing TYPE_TRACEBUF2 and TYPE_TRACE2_COMP_UA binary messages
 *
 *     Revision 1.4  2001/02/14 00:43:32  dietz
 *     added fflush(stdout) so redirection of stdout will write to disk
 *
 *     Revision 1.3  2001/01/22 19:25:30  dietz
 *     Added printing of ascii messages.
 *
 *     Revision 1.2  2000/07/11 19:56:21  lucky
 *     Modified argument list: No more tracebuf file option -- this can be done more
 *     ellegantly with the sniffwave command.
 *     Added the possibility to sniff for message with particular logos, including
 *     wildcards. The user can specify INST_, MOD_, and TYPE_ strings desired,
 *     or omit those and only specify the ring to sniff. Therefore, sniffring RING_NAME
 *     is equivalent to sniffring RING_NAME INST_WILDCARD MOD_WILDCARD TYPE_WILDCARD.
 *
 *     Revision 1.1  2000/02/14 19:31:49  lucky
 *     Initial revision
 *
 *
 */


  /********************************************************************
   *                    sniffring - a debugging tool                  *
   *                                                                  *
   *  Command line program that latches onto a user-given transport   *
   *  ring and reads every message on that ring.  It prints the logo, *
   *  sequence # and length of the message, followed by the message   *
   *  itself (except for binary waveform messages) to the screen.     *
   *                                                                  *
   ********************************************************************/

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <earthworm.h>
#include <transport.h>

#define MAX_SIZE MAX_BYTES_PER_EQ   /* Largest message size in characters */

/* Things to look up in the earthworm.h tables with getutil.c functions
 **********************************************************************/
static unsigned char InstId;        /* local installation id              */
static unsigned char InstWildCard;  /* wildcard for installations         */
static unsigned char ModWildCard;   /* wildcard for module                */
static unsigned char TypeWildCard;  /* wildcard for message type          */
static unsigned char TypeTraceBuf;  /* binary 1-channel waveform msg      */
static unsigned char TypeTraceBuf2; /* binary 1-channel waveform msg      */
static unsigned char TypeCompUA;    /* binary compressed waveform msg     */
static unsigned char TypeCompUA2;   /* binary compressed waveform msg     */
static unsigned char InstToGet;
static unsigned char TypeToGet;
static unsigned char ModToGet;

int main( int argc, char *argv[] )
{
   SHM_INFO inRegion;
   MSG_LOGO getlogo[1];
   MSG_LOGO logo;
   char     *inRing;           /* Name of input ring           */
   long     inkey;             /* Key to input ring            */
   char     msg[MAX_SIZE+1];
   unsigned char inseq;        /* transport seq# in input ring */
   int      rc;
   long     length;
   char     verbose = 1;       /* verbose will print names     */
   char     ModName[MAX_MOD_STR];


/* Check program arguments
   ***********************/
   if ( (argc < 2) || (argc > 6) || (argc == 4) )
   {
      printf( "Usage 1:  sniffring <ringname>\n" );
      printf( "Usage 2:  sniffring <ringname> <instid> <mod> <type>\n" );
      printf( "Usage 3:  sniffring <ringname> verbose \n" );
      printf( "Usage 4:  sniffring <ringname> <instid> <mod> <type> verbose\n" );
      printf( "Sniffring shows full ring, inst and module names if 'v' or \n" );
      printf( "'verbose' is specified as the final parameter. Otherwise you'll\n" );
      printf( "see numeric IDs for these three, as defined in earthworm*.d files. \n" );
      printf( "(Modules from external installations are not identified other than by number.)\n" );
      return -1;
   }
   inRing  = argv[1];

/* Look up local installation id
   *****************************/
   if ( GetLocalInst( &InstId ) != 0 )
   {
      printf( "sniffring: error getting local installation id; exiting!\n" );
      return -1;
   }
   if ( GetInst( "INST_WILDCARD", &InstWildCard ) != 0 )
   {
      printf( "sniffring: Invalid installation name <INST_WILDCARD>" );
      printf( "; exiting!\n" );
      return -1;
   }

/* Look up module ids & message types earthworm.h tables
   ****************************************************/
   if ( GetModId( "MOD_WILDCARD", &ModWildCard ) != 0 )
   {
      printf( "sniffring: Invalid module name <MOD_WILDCARD>; exiting!\n" );
      return -1;
   }
   if ( GetType( "TYPE_WILDCARD", &TypeWildCard ) != 0 )
   {
      printf( "sniffring: Invalid message type <TYPE_WILDCARD>; exiting!\n" );
      return -1;
   }
   if ( GetType( "TYPE_TRACEBUF", &TypeTraceBuf ) != 0 )
   {
      printf( "sniffring: Invalid message type <TYPE_TRACEBUF>; exiting!\n" );
      return -1;
   }
   if ( GetType( "TYPE_TRACEBUF2", &TypeTraceBuf2 ) != 0 )
   {
      printf( "sniffring: Invalid message type <TYPE_TRACEBUF2>; exiting!\n" );
      return -1;
   }
   if ( GetType( "TYPE_TRACE_COMP_UA", &TypeCompUA ) != 0 )
   {
      printf( "sniffring: Invalid message type <TYPE_TRACE_COMP_UA>; exiting!\n" );
      return -1;
   }
   if ( GetType( "TYPE_TRACE2_COMP_UA", &TypeCompUA2 ) != 0 )
   {
      printf( "sniffring: Invalid message type <TYPE_TRACE2_COMP_UA>; exiting!\n" );
      return -1;
   }


   if ((argc == 2) || (argc == 3))
   {
   /* Initialize getlogo to all wildcards (get any message)
    ******************************************************/
      getlogo[0].type   = TypeWildCard;
      getlogo[0].mod    = ModWildCard;
      getlogo[0].instid = InstWildCard;
   }
   if ((argc == 2) || (argc == 5)) {
      verbose = 0;
   }
   if ((argc == 5) || (argc == 6))  {
      if (GetInst (argv[2], &InstToGet) != 0)
      {
         printf( "sniffring: Invalid installation name %s", argv[2]);
         return -1;
      }
      if (GetModId (argv[3], &ModToGet) != 0)
      {
         printf( "sniffring: Invalid module name %s", argv[3]);
         return -1;
      }
      if (GetType (argv[4], &TypeToGet) != 0)
      {
         printf( "sniffring: Invalid message type name %s", argv[4]);
         return -1;
      }
      getlogo[0].type   = TypeToGet;
      getlogo[0].mod    = ModToGet;
      getlogo[0].instid = InstToGet;
   }



/* Look up transport region keys earthworm.h tables
   ************************************************/
   if( ( inkey = GetKey(inRing) ) == -1 )
   {
        printf( "sniffring: Invalid input ring name <%s>; exiting!\n",
                 inRing );
        return -1;
   }

/* Attach to input and output transport rings
   ******************************************/
   tport_attach( &inRegion,  inkey );


/* Flush all old messages from the ring
   ************************************/
#ifndef _NOFLUSH
   while( tport_copyfrom( &inRegion, getlogo, (short)1, &logo,
                          &length, msg, MAX_SIZE, &inseq ) != GET_NONE );
#endif

/* Grab next message from the ring
   *******************************/
   while( tport_getflag( &inRegion ) != TERMINATE )
   {
      rc = tport_copyfrom( &inRegion, getlogo, (short)1, &logo,
                           &length, msg, MAX_SIZE, &inseq );

      if ( rc == GET_OK )
      {
      /* This is the most likely case.  Output msg at bottom of if-elseif */
      }

      else if ( rc == GET_NONE )
      {
         sleep_ew( 200 );
         continue;
      }

      else if ( rc == GET_NOTRACK )
      {
         printf( "sniffring error in %s: no tracking for msg; i:%d m:%d t:%d seq:%d\n",
                  inRing, (int)logo.instid, (int)logo.mod, (int)logo.type, (int)inseq );
      }

      else if ( rc == GET_MISS_LAPPED )
      {
         printf( "sniffring error in %s: msg(s) overwritten; i:%d m:%d t:%d seq:%d\n",
                  inRing, (int)logo.instid, (int)logo.mod, (int)logo.type, (int)inseq );
      }

      else if ( rc == GET_MISS_SEQGAP )
      {
         printf( "sniffring error in %s: gap in msg sequence; i:%d m:%d t:%d seq:%d\n",
                  inRing, (int)logo.instid, (int)logo.mod, (int)logo.type, (int)inseq );
      }

      else if ( rc == GET_TOOBIG )
      {
         printf( "sniffring error in %s: input msg too big; i:%d m:%d t:%d seq:%d\n",
                  inRing, (int)logo.instid, (int)logo.mod, (int)logo.type, (int)inseq );
         continue;
      }
      if (verbose) {
          if (InstId == logo.instid) {
               sprintf( ModName, "%s", GetModIdName(logo.mod));
          } else {
               sprintf( ModName, "UnknownRemoteMod:%d", logo.mod);
          }
          /* Print message logo names, etc. to the screen
          ****************************************/
          printf( "%d Received %s %s %s <seq:%3d> <Length:%6ld>\n",
              (int)time (NULL), GetInstName(logo.instid), ModName, GetTypeName(logo.type), (int)inseq, length );

      } else {
          /* Backward compatibility */
          /* Print message logo, etc. to the screen
          ****************************************/
          printf( "%d Received <inst:%3d> <mod:%3d> <type:%3d> <seq:%3d> <Length:%6ld>\n",
                  (int)time (NULL),  (int)logo.instid, (int)logo.mod, (int)logo.type, (int)inseq, length );

      }


   /* Print numbers and names, etc. to the screen
    ****************************************
      printf( "%d Received <inst:%3d=%s> <mod:%3d=%s> <type:%3d=%s> <seq:%3d> <Length:%6ld>\n",
          (int)time (NULL),  (int)logo.instid, GetInstName(logo.instid), (int)logo.mod, GetModIdName(logo.mod), (int)logo.type, GetTypeName(logo.type), (int)inseq, length );*/



   /* Print any non-binary (non-waveform) message to screen
    *******************************************************/
      if( logo.type == TypeTraceBuf2 ) continue;
      if( logo.type == TypeCompUA2   ) continue;
      if( logo.type == TypeTraceBuf  ) continue;
      if( logo.type == TypeCompUA    ) continue;

      msg[length] = '\0'; /* Null-terminate message */
      printf( "%s\n", msg );
      fflush( stdout );
   }

/* Detach from shared memory regions and terminate
   ***********************************************/
   tport_detach( &inRegion );
   return 0;
}

