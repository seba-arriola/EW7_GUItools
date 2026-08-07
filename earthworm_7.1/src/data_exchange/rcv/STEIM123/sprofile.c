
  /*
   *    File:
   *            sprofile.c  -   for profiling execution time of steim compression
   *                            functions
   */

  /*           Copyright (C) 1994 reserved by Joseph M. Steim
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
             3  94/03/06  split from main into library.
   */


#include "steim.h"
#include <time.h>

#ifdef sun
#define  CLOCKS_PER_SEC   1000000
#endif

#ifdef OSK
#define  CLOCKS_PER_SEC   100
#endif


#define     NPROF         10
#define     SECSPERTICK   (1.0/CLOCKS_PER_SEC)


  typedef struct
  {
     clock_t tstart ;
     LONG       ticks ;
     CHAR       rname[50] ;
  } proftype ;


  static proftype profile[NPROF] ;

  /*
   * *****************************************************************************
   * the following supports execution time profiling for performance measurement *
   * *****************************************************************************
   */

   void profinit()
  {
    SHORT i ;

    for ( i = 0; i < NPROF; i++ )
        {
          profile [i].ticks = 0 ;
          profile [i].rname[0] = '\0' ;
        } ;
  }


#ifdef __STDC__
  void profname (  SHORT p,  CHAR *name)
#else
  void profname (p, name)
  SHORT p;
  CHAR *name;
#endif
    {
      strcpy (profile[p].rname, name) ;
    }


#ifdef __STDC__
  void profenter (SHORT p)
#else
  void profenter (p)
  SHORT p;
#endif
    {
      profile[p].tstart = clock() ;
    }


#ifdef __STDC__
  void profexit (SHORT p)
#else
  void profexit (p)
  SHORT p;
#endif
    {
      proftype *pr;
      pr = &profile[p];
      pr->ticks = pr->ticks + (clock() - pr->tstart) ;
    }


#ifdef __STDC__
  void profdump (SHORT scale)
#else
  void profdump (scale)
  SHORT scale;
#endif
    {
      SHORT p ;

      printf("\n");
      for ( p = 0; p < NPROF; p++ )
        if (profile[p].ticks > 0)
             printf ("%6.2f sec %s\n",SECSPERTICK*profile[p].ticks/scale,profile[p].rname) ;
    }


#ifdef __STDC__
  FLOAT profsecs (SHORT p)
#else
  FLOAT profsecs (p)
  SHORT p;
#endif
    {
      if (profile[p].ticks > 0)
          return ((FLOAT)SECSPERTICK*profile[p].ticks) ;
        else
          return (0.005) ;
    }



