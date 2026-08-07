  /*
   *    File:
   *            sprofile.h  -   header file for profiling execution time of steim compression
   *                            functions
   */

  extern void profinit() ;
#ifdef __STDC__
  extern void profinit (void) ;
#else
  extern void profinit () ;
#endif


#ifdef __STDC__
  extern void profname (SHORT p,  CHAR *name) ;
#else
  extern void profname () ;
#endif


#ifdef __STDC__
  extern void profenter (SHORT p) ;
#else
  extern void profenter () ;
#endif


#ifdef __STDC__
  extern void profexit (SHORT p) ;
#else
  extern void profexit () ;
#endif


#ifdef __STDC__
  extern void profdump (SHORT scale) ;
#else
  extern void profdump () ;
#endif


#ifdef __STDC__
  extern FLOAT profsecs (SHORT p) ;
#else
  extern FLOAT profsecs () ;
#endif



