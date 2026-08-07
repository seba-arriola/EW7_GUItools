  /*
    Program:
            swrseed.c
    Purpose:
            a program to illustrate writing "steim" compressed (levels 1,2,3) "micro-SEED" binary data
            records using library functions in "steimlib.c". a micro-SEED record is one that just
            has a few items in the SEED header area, sufficient to illustrate application of the
            compression library, but not containing times/stations/channel ID's...etc.

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
             1  94/03/06  first version using library functions
  */


  /*
   *
   * this has so far been successfully compiled on Microsoft Quick-C, Sun 4.1.1B C,
   * Sun ANSI C 2.0.1 (SPARCCompiler-C) under 4.1.3_U1, and OS9/68000 C v3.2
   *
   *    recommended cc lines:
   *        sun 4.1.1B    cc -O2 -dalign -o swrseed swrseed.c steimlib.c
   * sun ANSI C 4.1.3U    acc -O4 -Xc -dalign -o swrseed swrseed.c steimlib.c
   *        os9 (68020)   cc -DOSK -k=2w -O=2 -M=10k -t=/r0 -f=swrseed swrseed.c steimlib.c
   *        os9 (68000)   cc -DOSK -k=0w -O=2 -M=10k -t=/r0 -f=swrseed swrseed.c steimlib.c
   */


#include "steim.h"
#include "steimlib.h"
#include "miniseed.h"

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
  long optionvalue (char op, long value, int argc, char *argv[])
#else
  long optionvalue (op, value, argc, argv)
  char  op;
  long  value;
  int   argc;
  char  *argv[];
#endif
  {
    char *p;

    while (--argc>0)
      if(*(p=*++argv)=='-')
        while(*++p)
          if (toupper(*p) == op)
            return (atol(++p));
    return (value);
  }


#ifdef __STDC__
  void install_SEED_micro_header (SEED_data_record *sdr, SHORT firstframe, LONG rectotal, SHORT level, SHORT flip, SHORT dframes)
#else
  void install_SEED_micro_header (sdr, firstframe, rectotal, level, flip, dframes)
    SEED_data_record    *sdr;
    SHORT               firstframe;
    LONG                rectotal;
    SHORT               level;
    SHORT               flip;
    SHORT               dframes;
#endif
    {
        LONG    *fill ;
        SHORT   i ;

       /*
        * this procedure simulates some of the operations required to install the SEED header.
        * just enough information is installed here to enable the SEED decompression functions to work.
        *
        * this will zero the SEED header area, and any frames reserved for blockette installation.
        * be careful that "firstframe" does not equal or exceed the number of data frames in the
        * record, or this will zero bytes outside the record!.
        */

       fill = (LONG *)sdr ;
       for (i = 0; i < (firstframe+1)*16; i++)
         *(fill++) = 0 ;

       /*
        * install the blockette 1000 info
        */
       switch (level)
       {
         case 1 :
           sdr->Head.DOB.encoding_format = 10 ;
           break ;
         case 2 :
           sdr->Head.DOB.encoding_format = 11 ;
           break ;
         case 3 :
         /*
          * this format code must be defined by the FDSN SEED working group
          * for level 3 compression.
          */
           sdr->Head.DOB.encoding_format = 20 ;
           break ;
       }
       switch (dframes)
       {
         case  1 :
            sdr->Head.DOB.rec_length = 7 ;
            break ;
         case  3 :
            sdr->Head.DOB.rec_length = 8 ;
            break ;
         case  7 :
            sdr->Head.DOB.rec_length = 9 ;
            break ;
         case 15 :
            sdr->Head.DOB.rec_length = 10 ;
            break ;
         case 31 :
            sdr->Head.DOB.rec_length = 11 ;
            break ;
         case 63 :
            sdr->Head.DOB.rec_length = 12 ;
            break ;
         default :
            printf ("illegal SEED record length!\n");
            exit(0);
       }
       /*
        * fill in a little blockette info - enough to be read by srdseed.
        */
       if (flip)
           {
              sdr->Head.samples_in_record = swapb((SHORT)rectotal) ;
              sdr->Head.first_data_byte = swapb ((SHORT)(firstframe+1)*64) ;
              sdr->Head.DOB.blockette_type = swapb ((SHORT)1000) ;
              sdr->Head.first_blockette_byte = swapb ((SHORT)48) ;
           }
         else
           {
              sdr->Head.samples_in_record = (SHORT)rectotal ;
              sdr->Head.first_data_byte = (firstframe+1)*64 ;
              sdr->Head.DOB.blockette_type = 1000 ;
              sdr->Head.first_blockette_byte = 48 ;
           }
    }





#ifdef __STDC__
  void swriteseed(int argc,  char *argv[])
#else
  void swriteseed( argc, argv)
  int argc;
  char *argv[];
#endif
  {
      CHAR                      outname[80] ;
      SHORT                     i ;
      LONG                      sample ;
      SEED_data_record          *sdr ;
      LONG                      rectotal, total, samples_read, stopat ;
      CHAR                      line[200] ;
      SHORT                     level, flip, frames, blockindex, firstframe ;
      gdptype                   gdp ;
      FILE                      *outfile ;
      FILE                      *infile ;
      COMPWORD                  ftest ;
      BOOLEAN                   isopen, wasdata ;
      SHORT                     dataframes, adapt ;
      int                       native, numwritten;

      /*
       * read the desired compression level
       */
      printf ("\n");
      do {
        printf ("compression level (1,2,3)? ") ;
        if (scanf ("%d", &native) != 1)
          exit(0);
        level = native ;
      } while ((level < 1) || (level > 3)) ;

      /*
       * read the desired number of data frames, 1 to 63.
       * this defines records from 128 to 4096 bytes total (including header).
       */
      printf ("\n");
      do {
        printf ("number of data frames (1 - 63)? ") ;
        if (scanf ("%d", &native) != 1)
          exit(0);
        dataframes = native ;
      } while ((dataframes < 1) || (dataframes > 63)) ;

      if ((stopat=optionvalue('M',(long)2000000000,argc,argv)) != (long)2000000000)
        printf ("reading maximum %ld samples\n",stopat);
      /*
       * calculate over how many frames per record compression will adapt.
       * set up the differencing adaptivity to use at least the first 12.5% of the data in a full record.
       */
      adapt = (dataframes + 8) / 8 ;
      /*
       * see if flipping is necessary (a vax or intel must flip to write sane order)
       */
      ftest.b[0] = 0x12 ;
      ftest.b[1] = 0x34 ;
      ftest.b[2] = 0x56 ;
      ftest.b[3] = 0x78 ;
      if (ftest.l == 0x78563412)
          flip = 1 ;
        else
          if (ftest.l == 0x12345678)
              flip = 0 ;
            else
              {
                printf ("CPU word order cannot be processed\n") ;
                exit (0) ;
              }

      /*
       * allocate a SEED record in the desired length into which to compress. this would normally be done
       * for each channel.
       */
      if ((sdr=(SEED_data_record *)malloc(sizeof(compressed_frame)*dataframes+sizeof(SEED_fixed_data_record_header)))==NULL)
        exit (ENOMEM);

      /*
       * allocate a compression continuity and generic data record control structure. this
       * would normally be done for each channel. cast the SEED record pointer into the
       * generic type expected by the compression library.
       */
      if ((gdp = (gdptype)init_generic_compression (1, adapt, dataframes, level, flip, (generic_data_record *)sdr)) == NULL )
        exit (ENOMEM) ;

      /*
       * open the SEED compressed file to read.
       */
      if( (infile = fopen( argv[1], "r" )) == NULL )
        {
         printf ("main: cannot open text input file: %s\n",argv[1]) ;
         exit(1);
        };
      /*
       * continue reading until the DATA keyword is found on a line by itself.
       */
      do {
       if (fgets( line, sizeof(line), infile ) == NULL)
            {
              printf ("main: file format error: no DATA keyword found\n") ;
              exit(2);
            } ;
      } while (strstr (line, "DATA") == NULL) ;

      /*
       * open an output binary file in which to write decompressed data. the file name
       * handling could be done shorter with more use of the string library, although
       * these functions are not supported in every library, so do it the brute force way.
       */
      if (optiontrue('O', argc, argv))
          {
          /*
           * make a name (root.see) for the output binary file, and create it.
           */
          strcpy (outname, argv[1]) ;
          i = strlen(outname) ;
          while (i)
            if (outname[--i] == '.')
              break ;
          if (!i)
            outname [i=strlen(outname)] = '.' ;
          outname[i+1] = 0 ;
          strcat (outname, "see") ;
          if ((outfile = fopen( outname, BINWRITE )) == NULL )
            {
                printf ("main: cannot open output binary file: %s\n",outname) ;
                exit(2);
            }
          printf ("opened for output: %s\n",outname) ;
          }
        else
          outfile = NULL ;

      samples_read = 0;
      total = 0 ;
      isopen = TRUE ;
      /*
       * initialize the history for the SEED record, and reserve no frames
       * for blockettes.
       */
      firstframe = 0 ;
      clear_generic_compression (gdp, firstframe) ;
      do {
        /*
         * read until there are at least enough samples to fill a best-case frame, or until
         * the input file is read to the end. for illustration's sake, check that the sample
         * is actually written into the peek buffer. if it's not, something is wrong with
         * compress_frame, or the PEEKELEMS define.
         */
        while ((isopen == TRUE) && (peek_threshold_avail(gdp->adp->ccp) < 0))
          {
           if (fscanf(infile, "%ld", &sample) != 1)
                {
                  fclose (infile) ;
                  isopen = FALSE ;
                  break ;
                }
            if (peek_write(gdp->adp->ccp, &sample, 1) != 1)
                printf ("compression peek buffer failed to accept new data!\n") ;
            samples_read = samples_read + 1 ;
            if (samples_read >= stopat)
                {
                  printf ("finishing when %ld hit maximum %ld\n",samples_read,stopat);
                  fclose (infile) ;
                  isopen = FALSE ;
                  break ;
                }
          }
        /*
         * compress a frame into the record
         */
        frames = compress_generic_record (gdp, firstframe) ;
        /*
         * the peek buffer may have underflowed at this point.
         * if you call "clear_compression" here, however, the "frame" counter
         * will be set to zero, which is not what you want to do to finish writing
         * the available data in a padded record. if you run out of data (peek_contents < 0),
         * you should finish building the current record, then call "clear_compression" before
         * reading new data.
         *
         * check for a full record. a full record with the input file closed
         * is the last record to be written, and cause to exit the loop.
         */
        if (frames == dataframes)
            {
                /*
                 * install any header elements here, including the number of samples.
                 * any frames that have been reserved for blockettes will have to have
                 * appropriate contents stuffed in them, which is not done by the compression library.
                 */
                rectotal = generic_record_samples(gdp) ;
                total += rectotal ;
                /*
                 * install a little header information, just enough to be able to automatically
                 * decompress this data. in a real system, a full-blown header installation
                 * would be performed here.
                 */
                install_SEED_micro_header (sdr, firstframe, rectotal, level, flip, dataframes) ;
                /*
                 * print a message telling what was put into this record
                 */
                printf ("samples: record=%ld  total=%ld \n",rectotal,total);
                /*
                 * write the compressed record to the output file
                 */
                if (outfile != NULL)
                  fwrite (sdr, 1, (sizeof(compressed_frame)*(dataframes+1)), outfile);
                /*
                 * the "firstframe" value if > 0 may be set here to reserve frames in the next
                 * record to be compressed. this is arbitrarily set here (just to test the feature)
                 * to continuously cycle from 0 to 4 (within limits, so as not to reserve
                 * more frames than there are in the record) reserved frames. this code will
                 * write an empty record every (dataframes+1) records if dataframes <= 4.
                 */
                if (optiontrue('R', argc, argv))
                    firstframe = (firstframe+1) % ((dataframes > 4) ? 5 : dataframes+1) ;
                /*
                 * prepare the SEED buffer for the next record.
                 */
                clear_generic_compression (gdp, firstframe) ;
                /*
                 * continue until the input file has been read to the end, and there are no more
                 * samples to process in the compression peek buffer.
                 */
                if (!isopen && (peek_contents(gdp->adp->ccp) <= 0))
                    break ;
            }

      } while (TRUE) ;
      /*
       * close the output file gracefully
       */
      if (outfile != NULL)
        fclose (outfile) ;
      printf ("\ndone. total samples: %ld\n",total) ;
    }





#ifdef __STDC__
  void main (int   argc,  char  *argv[],  char  **envp)
#else
  void main (argc, argv, envp)
    int   argc;
    char  *argv[];
    char  **envp;
#endif
  {
    printf ("swrseed compressor version 940306-C, Copyright (C) 1994 by Joseph M. Steim\n") ;
    printf ("swrseed comes with ABSOLUTELY NO WARRANTY\n") ;
    printf ("This is free software; you are welcome to redistribute and modify it under\n") ;
    printf ("conditions of the GNU General Public License, Free Software Foundation, Inc.\n") ;
    swriteseed (argc, argv) ;
    exit(0) ;
  }
