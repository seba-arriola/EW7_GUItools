
  /*
    Program:
            srdseed.c
    Purpose:
            a program to illustrate reading "steim" compressed (levels 1,2,3) "micro-SEED" binary data
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
   *        sun 4.1.1B    cc -O2 -dalign -o srdseed srdseed.c steimlib.c
   * sun ANSI C 4.1.3U    acc -O4 -Xc -dalign -o srdseed srdseed.c steimlib.c
   *        os9 (68020)   cc -DOSK -k=2w -O=2 -M=10k -t=/r0 -f=srdseed srdseed.c steimlib.c
   *        os9 (68000)   cc -DOSK -k=0w -O=2 -M=10k -t=/r0 -f=srdseed srdseed.c steimlib.c
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
  LONG decode_SEED_micro_header (SEED_data_record *sdr, SHORT *firstframe, SHORT *level, SHORT *flip, SHORT *dframes)
#else
  LONG decode_SEED_micro_header (sdr, firstframe, level, flip, dframes)
    SEED_data_record    *sdr;
    SHORT               *firstframe;
    SHORT               *level;
    SHORT               *flip;
    SHORT               *dframes;
#endif
    {
      COMPWORD          ftest;
      SHORT             hfirstdata, hfirstblockette, h1000 ;
      LONG              headertotal ;
      /*
       *
       * decide whether this computer needs to "flip" the data for internal processing.
       * note that the kind of flipping will not work for hermaphroditic computers like a PDP-11
       * that can't make up its mind. it's either intel/vax or sensible order.
       */
      ftest.b[0] = 0x12 ;
      ftest.b[1] = 0x34 ;
      ftest.b[2] = 0x56 ;
      ftest.b[3] = 0x78 ;
      if (ftest.l == 0x78563412)
          *flip = 1 ;
        else
          if (ftest.l == 0x12345678)
              *flip = 0 ;
            else
              {
                    printf ("CPU word order cannot be processed\n") ;
                    exit (0) ;
              }
      if (*flip)
          {
            headertotal = swapb (sdr->Head.samples_in_record) ;
            hfirstdata = swapb (sdr->Head.first_data_byte) ;
            hfirstblockette = swapb (sdr->Head.first_blockette_byte) ;
            h1000 = swapb (sdr->Head.DOB.blockette_type) ;
          }
        else
          {
            headertotal = sdr->Head.samples_in_record ;
            hfirstdata = sdr->Head.first_data_byte ;
            hfirstblockette = sdr->Head.first_blockette_byte ;
            h1000 = sdr->Head.DOB.blockette_type ;
          }
      /*
       * find the first data frame. note that compressed data must begin on a frame boundary.
       * abort if an illegal start-of-data is found.
       */
      if ((hfirstdata % sizeof(compressed_frame)) != 0)
        {
            printf ("first data does not begin on frame boundary!\n") ;
            exit(0);
        }
      *firstframe = hfirstdata/sizeof(compressed_frame) - 1 ;
      /*
       * if a data-only blockette is present, read the encoding format and
       * record length from it. otherwise, the caller's default level will not be changed.
       * similarly, do not overwrite the caller's default number of data frames if the
       * blockette 1000 is not present.
       */
      if ((hfirstblockette == 48) && (h1000 == 1000))
            {
                switch (sdr->Head.DOB.encoding_format)
                {
                    case 10 :
                        *level = 1 ;
                        break ;
                    case 11 :
                        *level = 2 ;
                        break ;
                    /*
                     * this format code must be defined by the FDSN SEED working group
                     * for level 3 compression.
                     */
                    case 20 :
                        *level = 3 ;
                        break ;
                    default :
                        printf ("unknown encoding format %d\n",(int)sdr->Head.DOB.encoding_format) ;
                        exit (0) ;
                        break ;
                }
                switch (sdr->Head.DOB.rec_length)
                {
                    case 12 :
                        *dframes = 63 ;
                        break ;
                    case 11 :
                        *dframes = 31 ;
                        break ;
                    case 10 :
                        *dframes = 15 ;
                        break ;
                    case  9 :
                        *dframes = 7 ;
                        break ;
                    case 8 :
                        *dframes = 3 ;
                        break ;
                    case 7 :
                        *dframes = 1 ;
                        break ;
                    default :
                        printf ("unknown record length code %d\n",(int)sdr->Head.DOB.rec_length) ;
                        exit (0) ;
                        break ;
                }
            }
      return (headertotal) ;
    }



#ifdef __STDC__
    void validate_frames (SHORT firstframe, SHORT level, SHORT dframes) 
#else
    void validate_frames (firstframe, level, dframes) 
      SHORT firstframe; 
      SHORT level; 
      SHORT dframes; 
#endif
    {
      if ((dframes < 1) || (dframes >= MAXSEEDFRAMES))
        {
          printf("illegal number of data frames specified!\n");
          exit(0);
        }
      if ((firstframe < 0) || (firstframe > dframes))
        {
          printf("illegal first data frame!\n");
          exit(0);
        }
      if ((level < 1) || (level > 3))
        {
          printf("illegal compression level!\n");
          exit(0);
        }
    }

#ifdef __STDC__
  void sreadseed(int argc,  char *argv[])
#else
  void sreadseed( argc, argv)
  int argc;
  char *argv[];
#endif
  {
      CHAR                      outname[80] ;
      SHORT                     i, p, chindex ;
      LONG                      *udata ;
      SEED_data_record          *sdr ;
      LONG                      rectotal, headertotal ;
      LONG                      total ;
      SHORT                     dstat ;
      SHORT                     firstframe, flip, level ;
      dcptype                   dcp[4] ;
      SHORT                     dframes ;
      FILE                      *outfile ;
      FILE                      *infile ;
      int                       numread ;

      static CHAR               dcptable [3] = { 'Z', 'N', 'E' } ;



      /*
       * allocate a data buffer to hold the maximum possible number of decompressed samples.
       * this typically requires at least a "compact" model on a PC.
       */
      if ((udata = (LONG *)malloc(sizeof(LONG)*MAXSEEDSAMPLES)) == NULL )
        exit (ENOMEM);

      /*
       * allocate a 4K SEED record into which to read from disk
       */
      if ((sdr = (SEED_data_record *)malloc(sizeof(SEED_data_record))) == NULL )
        exit (ENOMEM);

      /*
       * allocate 4 (Z,N,E + an extra) decompression_continuity structures. this is a quick
       * and dirty way of illustrating decompression of multiple channels. see the code below
       */
      for (chindex = 0; chindex < 4; chindex++)
        if ((dcp[chindex] = init_generic_decompression ()) == NULL )
            exit (ENOMEM) ;

      /*
       * open the SEED compressed file to read.
       */
      if( (infile = fopen( argv[1], BINREAD )) == NULL )
        {
         printf ("main: cannot open binary compressed input file: %s\n",argv[1]) ;
         exit(2);
        };
      /*
       * open an output text file in which to write decompressed data. the file name
       * handling could be done shorter with more use of the string library, although
       * these functions are not supported in every library, so do it the brute force way.
       */
      if (optiontrue('O', argc, argv))
          {
          /*
           * make a name (root.txt) for the output text file, and create it.
           */
          strcpy (outname, argv[1]) ;
          i = strlen(outname) ;
          while (i)
            if (outname[--i] == '.')
              break ;
          if (!i)
            outname [i=strlen(outname)] = '.' ;
          outname[i+1] = 0 ;
          strcat (outname, "txt") ;
          if ((outfile = fopen( outname, "w" )) == NULL )
            {
                printf ("main: cannot open text output file: %s\n",outname) ;
                exit(2);
            }
          /*
           * write a single line with the word "DATA" if the -h option not specified.
           */
          if (!optiontrue('H', argc, argv))
            fprintf (outfile,"DATA\n") ;
          }
        else
          outfile = NULL ;

      p = 0 ;
      total = 0 ;
      /*
       * set the defaults that will be used if the SEED header contains no blockette 1000:
       * level 1, 4K records.
       */
      level = 1 ;
      dframes = 63 ;
      /*
       * read the header first, to see how big the record is.
       */
      while ((numread = fread(sdr, sizeof(sdr->Head), 1, infile)) == 1)
            {
              /*
               * this is a very crude illustration showing maintenance of multiple
               * decompression_continuity structures for each channel, and how to call
               * decompress_generic_record. note that no time processing, occurs here.
               * this crude example will only work with 3 channels,
               * whose channel names end in Z, N, or E.
               */
              chindex = 0 ;
              while (chindex < 3)
                if (sdr->Head.channel_ID[2] == dcptable[chindex])
                    break ;
                  else
                    chindex++ ;
              /*
               * decode just enough crucial bits from the header.
               * actual time/status/channelID processing will take place in a real function.
               */
              headertotal = decode_SEED_micro_header (sdr, &firstframe, &level, &flip, &dframes) ;
              validate_frames (firstframe, level, dframes) ;
              /*
               * read the data portion of the appropriate size, based on the decoded information,
               * into the "Frames" area of the record, i.e. following the header.
               */
              if ((numread = fread(sdr->Frames, sizeof(compressed_frame)*dframes, 1, infile)) != 1)
                {
                    printf ("partial or corrupt SEED record!\n") ;
                    exit (0) ;
                }
              /*
               * decompress the record into the array "udata".
               */
              rectotal = decompress_generic_record ((generic_data_record *)sdr, udata, &dstat, dcp[chindex],
                                                    firstframe, headertotal, level, flip, dframes) ;
              /*
               * this just prints the channel ID and the number of samples decompressed from the record
               */
              printf ("\n%c%c%c = %ld\n",sdr->Head.channel_ID[0],sdr->Head.channel_ID[1],sdr->Head.channel_ID[2],rectotal);
              /*
               * this will print any error status returned from processing the record, and
               * abort on a "fatal" error, i.e. a record that cannot be decompressed.
               * there is no need to abort, (there is nothing going on like memory request denied),
               * so the action is up to the caller. a copy of the status messages will also be written
               * into the output file if one is being written.
               */
              dferrorfatal (dstat, outfile) ;
              if (dferrorfatal(dstat, stdout))
                      {
                        printf ("decompress: decompress_SEED_record reports errors!\n") ;
                        exit(3) ;
                      } ;
              /*
               * totalize the number of sammples. this is valid only if you've checked errors as above,
               * otherwise the decompression function may return negative values.
               */
              total = total + rectotal ;
              /*
               * for each sample decompressed, write to the output text file. note that there is only *one*
               * output file, so in this example it doesn't make sense to decompress multiple channels
               * (from a multiplexed SEED input file) into a single output file.
               */
              if (outfile != NULL)
                for (i = 0; i < (SHORT)rectotal; i++)
                  {
                    fprintf (outfile, "%ld ", udata[i]) ;
                    p = (p + 1) % 8 ;
                    if (p == 0)
                      fprintf (outfile, "\n") ;
                  }
            }
      /*
       * close the output file gracefully
       */
      if (outfile != NULL)
        {
            fprintf (outfile, "\n") ;
            fclose (outfile) ;
        }
      fclose (infile) ;
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
    printf ("srdseed decompressor version 940306-C, Copyright (C) 1994 by Joseph M. Steim\n") ;
    printf ("srdseed comes with ABSOLUTELY NO WARRANTY\n") ;
    printf ("This is free software; you are welcome to redistribute and modify it under\n") ;
    printf ("conditions of the GNU General Public License, Free Software Foundation, Inc.\n") ;
    sreadseed (argc, argv) ;
    exit (0);
  }
