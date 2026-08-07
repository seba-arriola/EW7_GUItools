
  /*
    Program:
            steim123.c
    Purpose:
            a program to illustrate operation of "steim" compression algorithms, levels 1, 2, and 3
            for integer seismic data. the principle function is illustrated in the functions
            "compress_frame" and "decompress_frame" in "steimlib.c". this program shows driving these
            more-or-less general-purpose functions.

    Operation:
            the program will read an ascii text file containing a header having an arbitrary
            number of lines, followed by uncompressed data. the start of data will be
            delimited by the keyword "DATA" on a separate line. the ascii data file format is the
            same as used by Sam Stearns "EARTH1" data files to illustrate linear predictive compression.
            the user will be prompted for the compression level, and the number of times to repeat
            the compression, in order to calculate more accurate run times. after the
            last pass through the file, the compression ratio and run-time results will be printed,
            and a compressed output file, if generated (-o or -f options), will be written.

            the program may also operate in the reverse mode (-d), decompressing its own output files
            to recreate ascii text input files. the program will automatically determine the
            "endian-ness" of the compressed data, and treat it accordingly.

                 Example:
                    compression:
                         steim123 anmbhz92.asc          will compress and measure elapsed time.
                         steim123 anmbhz92.asc -o       will write a binary compressed output file
                                                             anmbhz92.sLc, where L is the level (1,2,3).
                         steim123 anmbhz92.asc -f       will do the same as -o, except the output
                                                             file will be overwritten if it exists.
                         other options:
                                -h       will omit the embedded header information.
                                -w       will endian-flip the output file

                    decompression:
                         steim123 anmbhz92.sLc -d       will write an ascii text decompressed output file
                                                             anmbhz92.txt, where L is the level (1,2,3).

    Copyright issues:

             Copyright (C) 1994 reserved by Joseph M. Steim
                                            Quanterra, Inc.
                                            325 Ayer Road
                                            Harvard, MA 01451, USA
                                            TEL: 508-772-4774
                                            FAX: 508-772-4645
                                            e-mail: steim@geophysics.harvard.edu

            This program is free software; you can redistribute it and/or modify
            it under the terms of the GNU General Public License as published by
            the Free Software Foundation; either version 2 of the License, or
            (at your option) any later version.

            This program is distributed in the hope that it will be useful,
            but WITHOUT ANY WARRANTY; without even the implied warranty of
            MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
            GNU General Public License for more details.

            If you do not have a copy of the GNU General Public License,
            write to the: Free Software Foundation, Inc.
                          675 Mass Ave, Cambridge, MA 02139, USA.

            Note particularly the following Terms excerpted from the GNU General Public License:

            You may modify your copy or copies of the Program or any portion
            of it, thus forming a work based on the Program, and copy and
            distribute such modifications or work under the terms of Section 1
            above, provided that you also meet all of these conditions:

               a) You must cause the modified files to carry prominent notices
               stating that you changed the files and the date of any change.

               b) You must cause any work that you distribute or publish, that in
               whole or in part contains or is derived from the Program or any
               part thereof, to be licensed as a whole at no charge to all third
               parties under the terms of this License.

               c) If the modified program normally reads commands interactively
               when run, you must cause it, when started running for such
               interactive use in the most ordinary way, to print or display an
               announcement including an appropriate copyright notice and a
               notice that there is no warranty (or else, saying that you provide
               a warranty) and that users may redistribute the program under
               these conditions, and telling the user how to view a copy of this
               License.  (Exception: if the Program itself is interactive but
               does not normally print such an announcement, your work based on
               the Program is not required to print an announcement.)

    Edit History:
            Ed  date      Changes
            --  --------  --------------------------------------------------
             3  94/02/26  converted from pascal.
         **************** first public release. JPL, ASL 94.02.28 ***********
             4  94/03/04  command-line option processing improved.
                          general source clean-up to ANSI compile with no warnings.
                          converted to use library functions. only example
                            driver code now in this module.
  */


  /*
   *
   * this has so far been successfully compiled on Microsoft Quick-C, Sun 4.1.1B C,
   * Sun ANSI C 2.0.1 (SPARCCompiler-C) under 4.1.3_U1, and OS9/68000 C v3.2
   *
   *    recommended cc lines:
   *        sun 4.1.1B    cc -O2 -dalign -o steim123 steim123.c steimlib.c sprofile.c
   * sun ANSI C 4.1.3U    acc -O4 -Xc -dalign -o steim123 steim123.c steimlib.c sprofile.c
   *        os9 (68020)   cc -DOSK -k=2w -O=2 -M=10k -t=/r0 -f=steim123 steim123.c steimlib.c sprofile.c
   *        os9 (68000)   cc -DOSK -k=0w -O=2 -M=10k -t=/r0 -f=steim123 steim123.c steimlib.c sprofile.c
   */

#include "steim.h"
#include "sprofile.h"
#include "steimlib.h"

#include <errno.h>


#define  BINREAD          "rb"
#define  BINWRITE         "wb"

#ifdef OSK
#define  ENOMEM           E_MEMFUL
#undef   BINREAD
#undef   BINWRITE
#define  BINREAD          "r"
#define  BINWRITE         "w"
#endif



#define      MAGICNUMBER  0x94070356

    LONG                        samples_read ;
    FILE                        *infile ;
    FILE                        *outfile ;
    SHORT                       ctotal ;
    LONG                        cverified ;
    BOOLEAN                     outopen ;


  /*
   * ******************************************
   * these are some utility printing routines *
   * ******************************************
   */

#ifdef __STDC__
  void dumpframe (cfp fp)
#else
  void dumpframe (fp)
  cfp fp;
#endif
    {
      SHORT i ;

      for ( i = 0; i < sizeof(*fp); i++ )
        {
          printf ("%02x",(UCHAR)((*fp)[i/4].b[i%4])) ;
          if (((i+1) % 32) == 0)
            {
              printf ("\n") ;
            }
            else
              if (((i+1) % 4) == 0)
                {
                  printf (" ") ;
                }
        }
    }




#ifdef OSK

/*  strstr(3)
 *
 *  Author: Terrence W. Holm          July 1988
 *
 *
 *  Finds the first occurrence of a substring, pointed to by
 *  <substr>, within a string pointed to by <mainstring>.
 *  If the substring is found then a pointer to it within
 *  <mainstring> is returned, otherwise NULL is returned.
 */


char *strstr(mainstring, substr)
  char *mainstring;
  char *substr;
{
  register char head_string;
  register char head_substr;

  if (mainstring == NULL || substr == NULL)
    return(NULL);

  head_substr = *substr++;

  while ((head_string = *mainstring++) != '\0')
    if (head_string == head_substr) {
      register char *tail_string = mainstring;
      register char *tail_substr = substr;

      do {
        if (*tail_substr == '\0')
          return(mainstring - 1);
      } while (*tail_string++ == *tail_substr++);
  }

  return(NULL);
}
#endif



  /*
   * ************************************************************************
   * example drivers follow for the compression and decompression functions *
   * ************************************************************************
   */


#ifdef __STDC__
  void run_compression (SHORT level,  adptype adp,  SHORT flip)
#else
  void run_compression ( level,  adp,  flip)
    SHORT level;
    adptype adp;
    SHORT flip;
#endif
    {

#define                 CMPSIZE         200

      LONG              samp ;
      SHORT             ndecomp ;
      SHORT             used ;
      BOOLEAN           isopen ;
      SHORT             i, final ;
      unpacktype        receive ;
      LONG              compare [CMPSIZE] ;
      SHORT             cnxti, cnxto ;
      SHORT             dstat ;

      cnxti = 0 ;
      cnxto = 0 ;
      /*
       * initialize "ndecomp" so that no error is printed on decompressing the first frame.
       */
      ndecomp = 0 ;
      isopen = TRUE ;
      do {
        /*
         * fill the compression look-ahead (peek) buffer with enough samples
         * to fill a complete, best case compressed frame. at the same time, fill the compare
         * buffer, which will be used to check the decompressed data against the original.
         * the one-sample-at-a-time procedure for filling the peek buffer are grossly inefficient,
         * but illustrate the point.
         */
        profenter(0) ;
        while ((isopen == TRUE) && (peek_threshold_avail(adp->ccp) < 0))
          {
           if (fscanf(infile, "%ld", &samp) != 1)
                {
                  fclose (infile) ;
                  isopen = FALSE ;
                  break ;
                } ;
            peek_write (adp->ccp, &samp, 1) ;
            compare [cnxti] = samp ;
            cnxti = (cnxti + 1) % CMPSIZE ;
            ctotal = ctotal + 1 ;
            samples_read = samples_read + 1 ;
          } ;
        profexit(0) ;
        /*
         * compress the current frame, as if it were part of a "package" of frames in, say, a 4K record.
         */
        profenter(1) ;
        used = compress_adaptively (adp) ;
        profexit(1) ;
        /*
         * check for inability to compress. this only makes sense in level 2/3. in level 1,
         * we may not even know about it, because of wrapping beyond 32 bits.
         * 32 (or even 29 bits) are plenty big enough for any known seismic waveforms, however,
         * (until Quanterra comes along with another improved digitizer!)
         */
        if (used < 0)
            {
              printf ("compress frame: data too large to compress. clipping samples.\n") ;
              fix (adp->ccp) ;
            }
          else
            {
              /*
               * verify (by decompressing) the frame just compressed.
               */
              profenter(2) ;
              ndecomp = decompress_frame ((cfp)adp->ccp->framebuf, receive, &final, ndecomp, level, 0, flip, 0, &dstat) ;
              if (dferrorfatal(dstat, stdout))
                      {
                        printf ("verify: decompress_frame reports errors!\n") ;
                        dumpframe ((cfp)adp->ccp->framebuf) ;
                        exit(6);
                      } ;
              if (ndecomp == 0)
                  {
                    printf ("verify: frame decompressed with no samples\n") ;
                    dumpframe ((cfp)adp->ccp->framebuf) ;
                    exit (9) ;
                  }
              for (i = 0; i < ndecomp; i++)
                if (receive [i+2] != compare [(cnxto+i) % CMPSIZE])
                    {
                      printf ("verify: comparison error: verified so far: %ld\n",cverified) ;
                      printf ("input=%ld output=%ld\n",compare [(cnxto+i) % CMPSIZE],receive [i+2]) ;
                    }
                  else
                    cverified = cverified + 1 ;
              cnxto = (cnxto + ndecomp) % CMPSIZE ;
              ctotal = ctotal - ndecomp ;
              if (ndecomp != used)
                  printf ("verify: number of samples decompressed does not agree with compression.\n") ;
              profexit(2) ;
              /*
               * write the output file if one is open. note that there is no error checking here.
               */
              profenter(0) ;
              if (outopen)
                  fwrite ((cfp)adp->ccp->framebuf, sizeof(compressed_frame), 1, outfile);
              profexit(0) ;
            }
      /*
       * the loop will continue compressing the contents of the peek buffer
       * until done.
       */
      } while (peek_contents(adp->ccp) > 0) ;
      /*
       * if writing an output file, close it.
       */
      if (outopen)
          {
            fclose (outfile) ;
            outopen = FALSE ;
          }
      /*
       * at this point, peek_total if negative indicates the number of samples padded to
       * fill the compression frame.
       */
    }



  /*
   * this will print the compression statistics if the "print" flag is TRUE.
   * the function value will return the total number of samples processed.
   */

#ifdef __STDC__
  LONG stats (BOOLEAN print,  SHORT level,  ccptype ccp,  CHAR *argv[])
#else
  LONG stats ( print,  level,  ccp,  argv)
   BOOLEAN print;
   SHORT level;
   ccptype ccp;
   CHAR *argv[];
#endif
   {
      SHORT     i, k ;
      LONG      samples_processed ;
      SHORT     padded, remaining ;
      FLOAT     cr, bps ;
      LONG      bytes_read ;
#ifdef STATISTICS
      LONG      data_bits ;
      FLOAT     dbps, ebps ;
#endif
      int       native;

      if (blocks_padded (ccp) > 0)
          {
            padded = blocks_padded (ccp) ;
            remaining = 0 ;
          }
        else
          {
            padded = 0 ;
            remaining = peek_contents (ccp);
          } ;
      samples_processed = samples_read - remaining ;
      bytes_read = samples_processed * 4 ;
      cr = 1.0*bytes_read/(frames(ccp)*sizeof(compressed_frame)-padded*4) ;
      if (!print)
          return (samples_processed) ;
      printf ("file: %s    last frame:\n",argv[1]) ;
      dumpframe ((cfp)ccp->framebuf) ;
      printf ("level: %d\n",(native=level)) ;
      printf ("samples read=%ld   bytes read=%ld\n",samples_read, bytes_read) ;
      printf ("samples processed=%ld    samples_verified=%ld\n",samples_processed,cverified) ;
      printf ("samples padded in last frame=%d  samples not processed=%d\n",(native=padded),(native=remaining)) ;
      if (ctotal > 0)
        printf ("samples remaining to verify=%d\n",(native=ctotal)) ;
      printf ("frames=%ld   total compressed bytes=%ld\n",frames(ccp), frames(ccp)*sizeof(compressed_frame)) ;
      bps = 8.0 * (frames(ccp)*sizeof(compressed_frame)) / samples_processed ;
#ifdef STATISTICS
      printf ("squeezed flags=%ld  histogram of storage units (bits) used:\n",ccp->squeezed_flags) ;
      data_bits = 0 ;
      k = 0 ;
      for (i = 0; i < 33; i++)
        if (ccp->fits[i] > 0)
            {
              printf ("fit %2d=%8ld  ",i,ccp->fits[i]) ;
              k = k + 1 ;
              if ((k % 4) == 0)
                  printf("\n");
              data_bits = data_bits + i*ccp->fits[i] ;
            } ;
      if ((k % 4) != 0)
          printf ("\n");
      dbps = 1.0*data_bits/samples_processed ;
      ebps = bps - 1.0*data_bits/samples_processed ;
      printf ("data bits per sample: %4.2f    encoding bits per sample: %4.2f\n", dbps, ebps) ;
#endif
      printf ("\n") ;
      printf ("compression ratio vs. 32-bit: %4.2f   bits per sample: %4.2f\n",cr,bps) ;
      printf ("\n") ;
      return (samples_processed) ;
    }




#ifdef __STDC__
  void open_input (char *inname)
#else
  void open_input (inname)
    char *inname;
#endif
    {
      if ((infile = fopen( inname, "r" )) == NULL )
        {
         printf ("main: cannot open file: %s\n",inname) ;
         exit(2);
        };
#ifndef OSK
      if (setvbuf( infile, NULL, _IOFBF, 4096 ) != 0 )
        {
          printf( "system would not allow setting text input buffer size\n" );
          exit(2);
        } ;
#endif
     }



#ifdef __STDC__
  void open_output (SHORT level,  BOOLEAN overwrite,  char *inname)
#else
  void open_output ( level,  overwrite, inname)
    SHORT level;
    BOOLEAN overwrite;
    char *inname;
#endif
    /*
     * this will create a file name with the extension (.sLc), where L is the
     * compression level, 1,2, or 3.
     */
    {
      SHORT   i ;
      char    outname[80] ;

      strcpy (outname, inname);
      i = strlen(outname) ;
      while (i > 0)
        {
          if (outname[i] == '.')
              break ;
          i = i - 1 ;
        } ;
      if (i == 0)
        {
         i = strlen(outname) ;
         outname[i] = '.' ;
        } ;

      outname[i+1] = 's' ;
      outname[i+2] = (char)((int)'0' + level) ;
      outname[i+3] = 'c' ;
      outname[i+4] = 0 ;

      if (overwrite == FALSE)
          if ((outfile = fopen( outname, BINREAD )) != NULL )
              {
                printf ("main: output file exists: %s\n",outname) ;
                exit(7);
              } ;
      if ((outfile = fopen( outname, BINWRITE )) == NULL )
          {
           printf ("main: cannot open binary compressed output file: %s\n",outname) ;
           exit(2);
          };
      printf ("opened for output: %s\n",outname) ;
      outopen = TRUE ;
    }


  void typeopt()
  {
    printf ("\n") ;
    printf ("valid program options command line examples are:\n") ;
    printf ("\n") ;
    printf ("compression:\n") ;
    printf ("  steim123 anmbhz92.asc     compresses and measures elapsed time.\n") ;
    printf ("  steim123 anmbhz92.asc -o  writes a binary compressed output file\n") ;
    printf ("                              anmbhz92.sLc, where L is the level (1,2,3).\n") ;
    printf ("  steim123 anmbhz92.asc -f  the same as -o, except the output\n") ;
    printf ("                              file will be overwritten if it exists.\n") ;
    printf ("    other options:\n") ;
    printf ("     -h       will omit the embedded header information.\n") ;
    printf ("     -w       will endian-flip the output file\n") ;
    printf ("\n") ;
    printf ("decompression:\n") ;
    printf ("  steim123 anmbhz92.sLc -d  writes an ascii text decompressed output file\n") ;
    printf ("                              anmbhz92.txt, where L is the level (1,2,3).\n") ;
    printf ("\n") ;
    exit(1) ;
   }


#ifdef __STDC__
  BOOLEAN optiontrue (CHAR  op, int   argc, char  *argv[])
#else
  BOOLEAN optiontrue ( op,  argc, argv)
  CHAR  op;
  int   argc;
  char  *argv[];
#endif
  {

    char  *p;

    while (--argc>0)
      if(*(p=*++argv)=='-')
        while(*++p)
          if (toupper(*p) == op)
            return(TRUE);
    return(FALSE);
  }



#ifdef __STDC__
  void init (BOOLEAN print,  SHORT level,  SHORT flip,  int argc,  char *argv[])
#else
  void init ( print,  level,  flip,  argc, argv)
    BOOLEAN print;
    SHORT level;
    SHORT flip;
    int argc;
    char *argv[];
#endif
    {
      SHORT     i ;
      CHAR      junk [200] ;
      SHORT     len ;
      BOOLEAN   writingheader ;
      LONG      longlevel ;
      LONG      magic ;

      samples_read = 0 ;
      ctotal = 0 ;
      cverified = 0 ;
      open_input (argv[1]) ;
      outopen = FALSE ;
      writingheader = FALSE ;
      if (print)
          {
            if (optiontrue('O', argc, argv))
                open_output (level, FALSE, argv[1]) ;
              else
                if (optiontrue('F', argc, argv))
                    open_output (level, TRUE, argv[1]) ;
            if (outopen && !optiontrue('H', argc, argv))
                {
                 /*
                  * write the header here. the first 4 bytes contains the compression level. the next
                  * 4 bytes contains a magic number for the compressed binary files.
                  * do endian-flipping if required.
                  */
                  writingheader = TRUE ;
                  longlevel = level ;
                  if (flip != 0)
                      longlevel = endianflip (longlevel) ;
                  fwrite (&longlevel, sizeof(LONG), 1, outfile);
                  magic = MAGICNUMBER ;
                  if (flip != 0)
                      magic = endianflip (magic) ;
                  fwrite (&magic, sizeof(LONG), 1, outfile);
                } ;
            printf ("first 5 lines of file header info:\n") ;
          } ;
      /*
       * write the original header information into the compressed file, a line at a time
       * terminated by 0's. continue until the DATA keyword is found on a line by itself.
       */
      i = 0 ;
      do {

       if (fgets( junk, sizeof(junk), infile ) == NULL)
            {
              printf ("main: file format error: no DATA keyword found\n") ;
              exit(8);
            } ;
        if (print)
            {
              if (i < 5)
                  printf (junk) ;
              if (writingheader)
                  {
                    len = strlen(junk) ;
                    if (len > 0)
                        {
                          junk [len-1] = 0 ;
                          fwrite (junk, sizeof(CHAR), len, outfile);
                        }
                  }
            } ;
        i = i + 1 ;
      } while (strstr (junk, "DATA") == NULL) ;
    }



  /*
   * this is the example compression driver. this code is called only if the command-line
   * -D option is not specified. this will call the appropriate lower-level routines N times
   * to calculate average execution times. note that the "compression_continuity" structure
   * is allocated only once at the beginning of a sequence of iterations.
   */

#ifdef __STDC__
  void compressmode (int argc,  char *argv[])
#else
  void compressmode (argc, argv)
    int argc;
    char *argv[];
#endif
    {
        SHORT     j ;
        SHORT     level ;
        SHORT     iterations ;
        LONG      total ;
        SHORT     flip ;
        adptype   adp ;
        int       native;

     printf ("\n");
     do {
        printf ("compression level (1,2,3)? ") ;
        if (scanf ("%d", &native) != 1)
          exit(0);
        level = native ;
     } while ((level < 1) || (level > 3)) ;
     do {
        printf ("iterations (to improve timing accuracy, 3-10 is a good number)? ") ;
        if (scanf ("%d",&native) != 1)
          exit(0);
        iterations = native ;
     } while ((iterations < 1) || (iterations > 100));
     flip = optiontrue ('W', argc, argv) ;
     profinit () ;
     profname (0," file i/o time") ;
     profname (1," compression time") ;
     profname (2," decompression and verify time") ;
     profname (3," total run time") ;
     /*
      * initialize the "adaptivity_control" to use best differencing in the first 8 frames of each 63
      * on which to base differencing of the remainder of the "package".
      */
     if ((adp = (adptype)init_adaptivity (1, 8, 63, level, flip)) == NULL )
       exit (ENOMEM) ;
     for (j = 1; j <= iterations; j++)
        {
          printf ("iteration: %d\n",(native=j)) ;
          clear_compression (adp->ccp, level) ;
          init ((j==iterations), level, flip, argc, argv) ;
          profenter(3) ;
          run_compression (level, adp, flip) ;
          profexit(3) ;
          total = stats ((j==iterations), level, adp->ccp, argv) ;
        } ;
     printf ("execution times normalized per iteration:\n") ;
     profdump (iterations) ;
     printf ("\n") ;
     printf ("compression time (secs) for 50000 samples: %4.2f\n",profsecs(1)*(50000.0/(total*iterations))) ;
  }



  /*
   * this is the example decompression driver. most of the code here is in parsing for file names
   * and opening the appropriate input and output files. this code is called only if the command-line
   * -D option is specified.
   */

#ifdef __STDC__
  void decompressmode(int argc,  char *argv[])
#else
  void decompressmode( argc, argv)
  int argc;
  char *argv[];
#endif
  {
      CHAR                      outname[80] ;
      compressed_frame          frame, *framep ;
      SHORT                     i, p ;
      SHORT                     numread ;
      SHORT                     numdecompress ;
      unpacktype                samples ;
      SHORT                     dlevel ;
      SHORT                     final ;
      LONG                      total ;
      CHAR                      c ;
      CHAR                      line[200] ;
      LONG                      fmagic, flevel ;
      SHORT                     flip ;
      SHORT                     dstat ;
      LONG                      dframes ;
      const CHAR                backs[11] = {8,8,8,8,8,8,8,8,8,8,0} ;
      int                       native;

      printf ("\n") ;
      profinit () ;
      profname (0," binary compressed file read time") ;
      profname (1," text file write time") ;
      profname (2," decompression time") ;

      framep = (compressed_frame*)frame;

      if( (outfile = fopen( argv[1], BINREAD )) == NULL )
        {
         printf ("main: cannot open binary compressed input file: %s\n",argv[1]) ;
         exit(2);
        };

       /*
       * read the start of header here. the first 4 bytes contains the compression level. the next
       * 4 bytes contains a magic number for the compressed binary files. assume initially that
       * endian-flipping is not needed.
       */
      flip = 0 ;
      fread(&flevel, sizeof(LONG), 1, outfile);
      fread(&fmagic, sizeof(LONG), 1, outfile);
      if ((flevel < 1) || (flevel > 3) || (fmagic != MAGICNUMBER))
          {
            /*
             * try flipping the "endian-ness", and see whether that works.
             */
            flevel = endianflip (flevel) ;
            fmagic = endianflip (fmagic) ;
            if ((flevel < 1) || (flevel > 3) || (fmagic != MAGICNUMBER))
                {
                  printf ("decompressed file format error: \n") ;
                  printf ("file not produced by 'steim123' or contains no header.\n") ;
                  exit(0);
                } ;
            /*
             * it will be necessary to flip the incoming file
             */
            flip = 1 ;
          } ;
      dlevel = (SHORT)flevel ;
      if (flip != 0)
        printf ("endian-flipping word order for internal processing.\n") ;
      printf ("decompression level: %d\n",(native=dlevel)) ;
      /*
       * make a name (root.txt) for the output text file, and create it
       */
      strcpy (outname, argv[1]) ;
      i = strlen(outname) ;
      while (i > 0)
        {
          if (outname[i] == '.')
              break ;
          i = i - 1 ;
        };
      if (i > 0)
          {
            outname[i+1] = 't' ;
            outname[i+2] = 'x' ;
            outname[i+3] = 't' ;
            outname[i+4] = 0 ;
            if ((infile = fopen( outname, "w" )) == NULL )
              {
                  printf ("main: cannot open text output file: %s\n",outname) ;
                  exit(2);
              };
#ifndef OSK
            if( setvbuf( infile, NULL, _IOFBF, 4096 ) != 0 )
              {
                  printf( "system would not allow setting text output buffer size\n" );
                  exit(2);
              } ;
#endif
          }
        else
          {
            printf ("main: open output file has no extension: %s\n",argv[1]) ;
            exit(0);
          } ;
      /*
       * read each line of the header and write to the output text file.
       */
      do {
         p = 0 ;
         do {
          numread = fread(&c, sizeof(CHAR), 1, outfile);
              if (numread != 1)
                 {
                    printf ("main: reached premature end of compressed file.\n") ;
                    exit(EOF) ;
                 } ;
              line [p++] = c ;
         } while ((c != 0) && (p < sizeof(line))) ;
         if (strlen(line) > 0)
            fprintf (infile, "%s\n", line) ;
      } while (strstr (line, "DATA") == NULL) ;
      /*
       * header complete, begin reading and decompressing frame by frame.
       * at this point, the need to "endian-flip" the data has been determined. the loop below
       * illustrates the actual use of "decompress_frame". the code above is unique to the special
       * file formats used in this program.
       * initialize "numdecompress" so that no error is printed on decompressing the first frame.
       */
      numdecompress = 0 ;
      p = 0 ;
      total = 0 ;
      dframes = 0 ;
      do {
        /*
         * read a single compressed frame
         */
        profenter (0) ;
        numread = fread(frame, 1, sizeof(compressed_frame), outfile);
        dframes++ ;
        profexit (0) ;
        if (numread == sizeof(compressed_frame))
            {
              /*
               * decompress the frame into the array "samples".
               */
              profenter (2) ;
              numdecompress = decompress_frame (framep, samples, &final, numdecompress, dlevel, 0, flip, 0, &dstat) ;
              profexit (2) ;
              if (dferrorfatal(dstat, stdout))
                      {
                        printf ("decompress: decompress_frame reports errors!\n") ;
                        exit(3) ;
                      } ;
              if (numdecompress == 0)
                  printf ("decompress: frame decompressed with no samples\n") ;
              /*
               * the decompressed data are available here in the "samples" array, starting at "samples[2]".
               * write the decompressed frame, 8 samples per line separated by spaces into a text file.
               */
              profenter (1) ;
              for (i = 1; i <= numdecompress; i++)
                {
                  fprintf (infile, "%ld ", samples[i+1]) ;
                  p = (p + 1) % 8 ;
                  if (p == 0)
                    fprintf (infile, "\n") ;
                  total = total + 1 ;
                  if ((total % 1000) == 0)
                    {
                      printf ("%s%10ld",backs,total) ;
                      fflush (stdout) ;
                    }
                } ;
              profexit (1) ;
            } ;
        /*
         * continue until hit end of file on the compressed input file.
         */
      } while (numread > 0) ;
      profenter (1) ;
      fprintf (infile, "\n") ;
      fclose (infile) ;
      profexit (1) ;
      fclose (outfile) ;
      /*
       * print some statistics
       */
      printf ("\n") ;
      printf ("total samples: %ld\n",total) ;
      printf ("execution times:\n") ;
      profdump (1) ;
      printf ("\n") ;
      printf ("decompression rate (samples/sec): %4.2f\n", (1.0*total)/profsecs(2)) ;
    }




 /*
  * this is the main entry point.
  * from here,
  *   1. if no input file is specified on the command-line, a help screen is printed, and the program exits.
  *   2. if a -D option is specified, the program expects to find a compressed file, and will try to
  *         decompress it.
  *   3. if a -D option is not specified, the program expects to find an ascii input file, and will try
  *         to compress it, and optionally, write an output file
  */

#ifdef __STDC__
  void main (int   argc,  char  *argv[],  char  **envp)
#else
  void main (argc, argv, envp)
    int   argc;
    char  *argv[];
    char  **envp;
#endif
  {
    printf ("steim123 compressor version 940304-C, Copyright (C) 1994 by Joseph M. Steim\n") ;
    printf ("steim123 comes with ABSOLUTELY NO WARRANTY\n") ;
    printf ("This is free software; you are welcome to redistribute and modify it under\n") ;
    printf ("conditions of the GNU General Public License, Free Software Foundation, Inc.\n") ;
    if (argc < 2)
      typeopt();
    if (optiontrue('?', argc, argv))
      typeopt();
    if (optiontrue('D', argc, argv))
        decompressmode (argc, argv);
      else
        compressmode (argc, argv);
    exit (0);
  }


