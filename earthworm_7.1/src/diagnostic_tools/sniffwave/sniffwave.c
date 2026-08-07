
/*
 *
 *    $Id: sniffwave.c,v 1.19 2007/03/23 15:53:15 stefan Exp $
 *
 *    Revision history:
 *     $Log: sniffwave.c,v $
 *     Revision 1.19  2007/03/23 15:53:15  stefan
 *     Earthworm Class March 2007, added statistics flag to sniffwave, y, n, or s options are now handled, tport fix made
 *
 *     Revision 1.18  2007/02/07 00:35:16  stefan
 *     help text revision
 *
 *     Revision 1.17  2007/02/02 19:06:59  stefan
 *     removed module verbosity for non-local modules
 *
 *     Revision 1.16  2006/12/28 23:07:54  lombard
 *     Added version number, printed with usage.
 *     Changed to specifically look for TYPE_TRACEBUF2 and TYPE_TRACE2_COMP_UA
 *     packets if SCNL given on command line, or to look for TYPE_TRACEBUF and
 *     TYPE_TRACE_COMP_UA packets if SCN is given on command line. Thus you cannot
 *     sniff for both SCNL and SCN packets at once with this command.
 *
 *     Revision 1.15  2006/12/01 02:12:58  stefan
 *     Added "verbose" option to print module, installation and type names along
 *     with the numbers. Without this extra parameter it works as before.
 *
 *     Revision 1.14  2006/07/26 20:23:00  stefan
 *     additional significant digits for sample rates < 1hz
 *
 *     Revision 1.13  2006/03/27 16:57:27  davek
 *     Added version and quality info to the sniffwave output for each tracebuf.
 *
 *     Revision 1.12  2005/03/30 23:13:26  dietz
 *     Now printing seconds to 4 decimal places.
 *
 *     Revision 1.11  2004/04/13 22:57:32  dietz
 *     modified to work with TYPE_TRACEBUF2 and TYPE_TRACE2_COMP_UA msgs
 *
 *     Revision 1.10  2003/10/31 17:20:43  dietz
 *     Will now sniff both TYPE_TRACEBUF and TYPE_TRACE_COMP_UA msg headers.
 *     Will accept strings "wild", "WILD", and "*" as wildcards in SCN.
 *
 *     Revision 1.9  2002/10/24 22:11:32  dietz
 *     Added message length to printed information
 *
 *     Revision 1.8  2002/07/12 17:49:57  dietz
 *     added #include <time_ew.h>
 *
 *     Revision 1.7  2002/02/02 00:28:58  dietz *     Changed to print instid and module on the tracebuf header summary line
 *
 *     Revision 1.6  2001/03/30 01:17:36  dietz
 *     *** empty log message ***
 *
 *     Revision 1.5  2001/03/30 01:10:18  dietz
 *     added newline after every 10 data samples, and an fflush(stdout)
 *     after each tracebuf message.
 *
 *     Revision 1.4  2001/03/29 23:51:03  dietz
 *     Changed ring flush statemnet to while( tport_get... != GET_NONE );
 *
 *     Revision 1.3  2000/08/08 18:18:28  lucky
 *     Lint cleanup
 *
 *     Revision 1.2  2000/05/31 18:29:10  dietz
 *     Fixed to print 4-byte data properly.
 *
 *     Revision 1.1  2000/02/14 19:36:09  lucky
 *     Initial revision
 *
 *
 */


  /*****************************************************************
   *                            sniffwave.c                        *
   *                                                               *
   * Program to read waveform messages from shared memory and      *
   * write to a disk file.                                         *
   *                                                               *
   * Modified by Lucky Vidmar Tue May 11 11:27:35 MDT 1999         *
   *   allows for more arguments which specify                     *
   *     Ring to read from                                         *
   *     SCN of the messages to print                              *
   *     Whether or not trace data should be printed               *
   *                                                               *
   *                                                               *
   * Usage: sniffwave  <Ring> <Sta> <Comp> <Net> <Loc> <Data y/n>  *
   *    or: sniffwave  <Ring> <Sta> <Comp> <Net> <Data y/n>        *
   *                                                               *
   *****************************************************************/


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <transport.h>
#include <swap.h>
#include <time_ew.h>
#include <trace_buf.h>
#include <earthworm.h>

#define NLOGO 4
#define VERSION "2.2"

int IsWild( char *str )
{
  if( (strcmp(str,"wild")==0)  ) return 1;
  if( (strcmp(str,"WILD")==0)  ) return 1;
  if( (strcmp(str,"*")   ==0)  ) return 1;
  return 0;
}

int main( int argc, char **argv )
{
   SHM_INFO        region;
   long            RingKey;         /* Key to the transport ring to read from */
   MSG_LOGO        getlogo[NLOGO], logo;
   long            gotsize;
   char            msg[MAX_TRACEBUF_SIZ];
   char           *getSta, *getComp, *getNet, *getLoc, *inRing;
   char            wildSta, wildComp, wildNet, wildLoc;
   unsigned char   Type_TraceBuf,Type_TraceBuf2;
   unsigned char   Type_TraceComp, Type_TraceComp2;
   unsigned char   InstWildcard, ModWildcard;
   short          *short_data;
   long           *long_data;
   TRACE2_HEADER  *trh;
   char            stime[256];
   char            etime[256];
   int             dataflag;
   int             i;
   int             rc;
   int             verbose = 0; /* if verbose =1 print name and number for logo ids */
   int             useLoc = 0;
   int             nLogo = NLOGO;
   static unsigned char InstId;        /* local installation id              */
   char            ModName[MAX_MOD_STR];
   unsigned char   sequence_number = 0;
   int             statistics_flag;

  /* Initialize pointers
  **********************/
  trh  = (TRACE2_HEADER *) msg;
  long_data  =  (long *)( msg + sizeof(TRACE2_HEADER) );
  short_data = (short *)( msg + sizeof(TRACE2_HEADER) );

  /* Check command line argument
  *****************************/
  if ( argc < 6 || argc > 8 )
  {
     fprintf(stderr, "%s version %s\n", argv[0], VERSION);
     fprintf(stderr,"Usage: %s <ring name> <station> <component> <network> [<loc>] <y/n/s> [v]\n", argv[0]);
     fprintf(stderr, "\n       Appending the optional \"v\" or \"verbose\" argument causes module, \n");
     fprintf(stderr, "       installation and type names to be printed in addition to usual ID numbers\n");
     fprintf(stderr, "\n       The <y/n/s> flag is a data flag. If 'y' is specified, the full data\n");
     fprintf(stderr, "       contained in the tracebuf or tracebuf2 packet is printed out. \n");
     fprintf(stderr, "       If the flag is set to s, provide max/min/avg statistics of the trace data. \n");
     fprintf(stderr, "\n       If you specify the location code or wild for the location code\n");
     fprintf(stderr, "       you'll get Tracebuf2 packets. Otherwise you'll get Tracebufs.\n");
     fprintf(stderr,"\nExample: %s WAVE_RING PHOB wild NC wild n\n", argv[0] );
     fprintf(stderr,"Example: %s WAVE_RING wild wild wild y verbose\n", argv[0] );
     exit( 1 );
  }
  inRing  = argv[1];
  getSta  = argv[2];
  wildSta  = IsWild( getSta  );
  getComp = argv[3];
  wildComp = IsWild( getComp );
  getNet  = argv[4];
  wildNet  = IsWild( getNet  );

  if (strcmp(argv[argc-1], "verbose") == 0) {
      argc--;
      verbose = 1;
  }

  if (argc == 7 ) {
      getLoc  = argv[5];
      useLoc = 1;
      wildLoc  = IsWild( getLoc  );
      fprintf(stdout, "Sniffing %s for %s.%s.%s.%s\n",
          inRing, getSta, getComp, getNet, getLoc);

  } else {
      getLoc = NULL;
      nLogo = 2;
      wildLoc = 0;
      fprintf(stdout, "Sniffing %s for %s.%s.%s\n",
          inRing, getSta, getComp, getNet);
  }

  dataflag=0;
  statistics_flag=0;
  if(strcmp (argv[5 + useLoc], "y") == 0)
     dataflag = 1;
  else if(strcmp (argv[5 + useLoc], "n") == 0)
     dataflag = 0;
  else if(strcmp (argv[5 + useLoc], "s") == 0)
     statistics_flag = 1;
  else
  {
    fprintf(stderr, "sniffwave: specify data flag - y or n or s\n");
    exit (1);
  }

  /* Open log file
  ****************/
  logit_init( "sniffwave", 99, 100, 1 );

  /* Attach to ring
  *****************/
  if ((RingKey = GetKey( inRing )) == -1 )
  {
    logit( "e", "Invalid RingName; exiting!\n" );
    exit( -1 );
  }
  tport_attach( &region, RingKey );

/* Look up local installation id
   *****************************/
   if ( GetLocalInst( &InstId ) != 0 )
   {
      printf( "sniffwave: error getting local installation id; exiting!\n" );
      return -1;
   }


  /* Specify logos to get
  ***********************/
  if ( GetType( "TYPE_TRACEBUF", &Type_TraceBuf ) != 0 ) {
     logit("e","%s: Invalid message type <TYPE_TRACEBUF>!\n", argv[0] );
     exit( -1 );
  }
  if ( GetType( "TYPE_TRACE_COMP_UA", &Type_TraceComp ) != 0 ) {
     logit("e","%s: Invalid message type <TYPE_TRACE_COMP_UA>!\n", argv[0] );
     exit( -1 );
  }
  if ( GetType( "TYPE_TRACEBUF2", &Type_TraceBuf2 ) != 0 ) {
     logit("e","%s: Invalid message type <TYPE_TRACEBUF2>!\n", argv[0] );
     exit( -1 );
  }
  if ( GetType( "TYPE_TRACE2_COMP_UA", &Type_TraceComp2 ) != 0 ) {
     logit("e","%s: Invalid message type <TYPE_TRACE2_COMP_UA>!\n", argv[0] );
     exit( -1 );
  }
  if ( GetModId( "MOD_WILDCARD", &ModWildcard ) != 0 ) {
     logit("e","%s: Invalid moduleid <MOD_WILDCARD>!\n", argv[0] );
     exit( -1 );
  }
  if ( GetInst( "INST_WILDCARD", &InstWildcard ) != 0 ) {
     logit("e","%s: Invalid instid <INST_WILDCARD>!\n", argv[0] );
     exit( -1 );
  }

  for( i=0; i<nLogo; i++ ) {
      getlogo[i].instid = InstWildcard;
      getlogo[i].mod    = ModWildcard;
  }
  getlogo[0].type = Type_TraceBuf;
  getlogo[1].type = Type_TraceComp;
  if (useLoc) {
      getlogo[2].type = Type_TraceBuf2;
      getlogo[3].type = Type_TraceComp2;
  }

  /* Flush the ring
  *****************/
  while ( tport_getmsg( &region, getlogo, nLogo, &logo, &gotsize,
            (char *)&msg, MAX_TRACEBUF_SIZ ) != GET_NONE );
  logit( "et", "sniffwave: inRing flushed.\n");

  while ( tport_getflag( &region ) != TERMINATE )
  {
    rc = tport_copyfrom( &region, getlogo, nLogo,
               &logo, &gotsize, msg, MAX_TRACEBUF_SIZ, &sequence_number );

    if ( rc == GET_NONE )
    {
      sleep_ew( 200 );
      continue;
    }

    if ( rc == GET_TOOBIG )
    {
      logit( "et", "sniffwave: retrieved message too big (%d) for msg\n",
        gotsize );
      continue;
    }

    if ( rc == GET_NOTRACK )
      logit( "et", "sniffwave: Tracking error.\n");

    if ( rc == GET_MISS_LAPPED )
      logit( "et", "sniffwave: Got lapped on the ring.\n");

    if ( rc == GET_MISS_SEQGAP )
      logit( "et", "sniffwave: Gap in sequence numbers\n");

    if ( rc == GET_MISS )
      logit( "et", "sniffwave: Missed messages\n");

    /* Check SCNL of the retrieved message */

    if ( (wildSta  || (strcmp(getSta,trh->sta)  ==0)) &&
         (wildComp || (strcmp(getComp,trh->chan)==0)) &&
         (wildNet  || (strcmp(getNet,trh->net)  ==0)) &&
         ((useLoc && (logo.type == Type_TraceBuf2 ||
          logo.type == Type_TraceComp2) &&
         (wildLoc  || (strcmp(getLoc,trh->loc) == 0))) ||
         (! useLoc && ( logo.type == Type_TraceBuf ||
             logo.type == Type_TraceComp))))
    {
      WaveMsg2MakeLocal( trh );

      datestr23 (trh->starttime, stime, 256);
      datestr23 (trh->endtime,   etime, 256);

      if( logo.type == Type_TraceBuf2  ||
          logo.type == Type_TraceComp2    ) {
          fprintf( stdout, "%s.%s.%s.%s (0x%x 0x%x) ",
                   trh->sta, trh->chan, trh->net, trh->loc, trh->version[0], trh->version[1] );
      } else {
          fprintf( stdout, "%s.%s.%s ",
                   trh->sta, trh->chan, trh->net );
      }
      if (trh->samprate < 1.0) { /* more decimal places for slower sample rates */
          fprintf( stdout, "%d %s %d %6.4f %s (%.4f) %s (%.4f) %c%c \n",
               trh->pinno, trh->datatype, trh->nsamp, trh->samprate,
               stime, trh->starttime,
               etime, trh->endtime, trh->quality[0], trh->quality[1] );
      } else {
          fprintf( stdout, "%d %s %d %.1f %s (%.4f) %s (%.4f) %c%c ",
                         trh->pinno, trh->datatype, trh->nsamp, trh->samprate,
                         stime, trh->starttime,
                         etime, trh->endtime, trh->quality[0], trh->quality[1] );
      }
      if (verbose) {

          if (InstId == logo.instid) {
              sprintf( ModName, "%s", GetModIdName(logo.mod));
          } else {
              sprintf( ModName, "UnknownRemoteMod");
          }

          fprintf( stdout, "i%d=%s m%d=%s t%d=%s len%d\n",
                 (int)logo.instid, GetInstName(logo.instid), (int)logo.mod,
                 ModName, (int)logo.type, GetTypeName(logo.type), gotsize );
      } else {
          fprintf( stdout, "i%d m%d t%d len%d\n",
                 (int)logo.instid, (int)logo.mod, (int)logo.type, gotsize );
      }

      if (dataflag == 1)
      {
        if( logo.type == Type_TraceBuf2 ||
            logo.type == Type_TraceBuf     )
        {
          if ( (strcmp (trh->datatype, "s2")==0) ||
               (strcmp (trh->datatype, "i2")==0)    )
          {
            for ( i = 0; i < trh->nsamp; i++ ) {
              fprintf ( stdout, "%6hd ", *(short_data+i) );
              if(i%10==9) fprintf ( stdout, "\n" );
            }
          }
          else if ( (strcmp (trh->datatype, "s4")==0) ||
                    (strcmp (trh->datatype, "i4")==0)    )
          {
            for ( i = 0; i < trh->nsamp; i++ ) {
              fprintf ( stdout, "%6ld ", *(long_data+i) );
              if(i%10==9) fprintf ( stdout, "\n" );
            }
          }
          else
          {
             fprintf (stdout, "Unknown datatype %s\n", trh->datatype);
          }
        } else {
             fprintf (stdout, "Data values compressed\n");
        }
        fprintf (stdout, "\n");
        fflush (stdout);
      }
      if (statistics_flag == 1)
      {
        if( logo.type == Type_TraceBuf2 ||
            logo.type == Type_TraceBuf     )
        {
          long max=0;
          long min=0;
          double avg=0.0;
          short short_value;
          long long_value;

          if ( (strcmp (trh->datatype, "s2")==0) ||
               (strcmp (trh->datatype, "i2")==0)    )
          {
            for ( i = 0; i < trh->nsamp; i++ ) {
              short_value = *(short_data+i);
              if (short_value > max || max == 0) {
		max=short_value;
              }
              if (short_value < min || min == 0) {
		min=short_value;
              }
              avg += short_value;
            }
            avg = avg / trh->nsamp;
            fprintf(stdout, "Raw Data statistics max=%ld min=%ld avg=%f\n",
                       max, min, avg); 
            fprintf(stdout, "DC corrected statistics max=%f min=%f spread=%ld\n\n",
                       (double)(max - avg), (double)(min - avg), 
                       labs(max - min)); 
          }
          else if ( (strcmp (trh->datatype, "s4")==0) ||
                    (strcmp (trh->datatype, "i4")==0)    )
          {
            for ( i = 0; i < trh->nsamp; i++ ) {
              long_value = *(long_data+i);
              if (long_value > max || max == 0) {
		max=long_value;
              }
              if (long_value < min || min == 0) {
		min=long_value;
              }
              avg += long_value;
            }
            avg = avg / trh->nsamp;
            fprintf(stdout, "Raw Data statistics max=%ld min=%ld avg=%f\n",
                       max, min, avg); 
            fprintf(stdout, "DC corrected statistics max=%f min=%f spread=%ld\n\n",
                       (double)(max - avg), (double)(min - avg), 
                       labs(max - min)); 
          }
          else
          {
             fprintf (stdout, "Unknown datatype %s\n", trh->datatype);
          }
        } else {
             fprintf (stdout, "Data values compressed\n");
        }
        fflush (stdout);
       } /* end of statistics_flag if */
      } /* end of process this tracebuf if */
     } /* end of infinite while loop */

  exit (0);
}

