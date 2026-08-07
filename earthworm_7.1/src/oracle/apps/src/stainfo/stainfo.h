
/*
 *   This file is under RCS - do not modify unless you have
 *   checked it out using the command checkout.
 *
 *    $Id: stainfo.h,v 1.6 2002/05/28 19:25:40 lucky Exp $
 *    Revision history:
 *
 *    $Log: stainfo.h,v $
 *    Revision 1.6  2002/05/28 19:25:40  lucky
 *    Added header and footer tag text
 *
 *    Revision 1.5  2002/02/20 20:16:07  lucky
 *    added web appearance parameters
 *
 *    Revision 1.4  2001/07/01 21:55:35  davidk
 *    Cleanup of the Earthworm Database API and the applications that utilize it.
 *    The "ewdb_api" was cleanup in preparation for Earthworm v6.0, which is
 *    supposed to contain an API that will be backwards compatible in the
 *    future.  Functions were removed, parameters were changed, syntax was
 *    rewritten, and other stuff was done to try to get the code to follow a
 *    general format, so that it would be easier to read.
 *
 *    Applications were modified to handle changed API calls and structures.
 *    They were also modified to be compiler warning free on WinNT.
 *
 *    Revision 1.3  2001/02/28 17:29:10  lucky
 *    Massive schema redesign and cleanup.
 *
 *    Revision 1.2  2000/08/07 19:41:15  lucky
 *    *** empty log message ***
 *
 *    Revision 1.1  2000/02/15 19:45:44  lucky
 *    Initial revision
 *
 *    Revision 1.2  1999/09/22 16:16:18  lucky
 *    FTP connection stuff is now configurable from the config file
 *
 *    Revision 1.1  1999/06/11 19:49:27  lucky
 *    Initial revision
 *
 *    Revision 1.1  1999/05/05 18:05:54  lucky
 *    Initial revision
 *
 *
 */
  
/* eqparam2html.h */

/*
 *   This file is under RCS - do not modify unless you have
 *   checked it out using the command checkout.
 *
 *    $Id: stainfo.h,v 1.6 2002/05/28 19:25:40 lucky Exp $
 *    Revision history:
 *
 *    $Log: stainfo.h,v $
 *    Revision 1.6  2002/05/28 19:25:40  lucky
 *    Added header and footer tag text
 *
 *    Revision 1.5  2002/02/20 20:16:07  lucky
 *    added web appearance parameters
 *
 *    Revision 1.4  2001/07/01 21:55:35  davidk
 *    Cleanup of the Earthworm Database API and the applications that utilize it.
 *    The "ewdb_api" was cleanup in preparation for Earthworm v6.0, which is
 *    supposed to contain an API that will be backwards compatible in the
 *    future.  Functions were removed, parameters were changed, syntax was
 *    rewritten, and other stuff was done to try to get the code to follow a
 *    general format, so that it would be easier to read.
 *
 *    Applications were modified to handle changed API calls and structures.
 *    They were also modified to be compiler warning free on WinNT.
 *
 *    Revision 1.3  2001/02/28 17:29:10  lucky
 *    Massive schema redesign and cleanup.
 *
 *    Revision 1.2  2000/08/07 19:41:15  lucky
 *    *** empty log message ***
 *
 *    Revision 1.1  2000/02/15 19:45:44  lucky
 *    Initial revision
 *
 *    Revision 1.2  1999/09/22 16:16:18  lucky
 *    FTP connection stuff is now configurable from the config file
 *
 *    Revision 1.1  1999/06/11 19:49:27  lucky
 *    Initial revision
 *
 *    Revision 1.1  1999/05/05 18:05:54  lucky
 *    Initial revision
 *
 *
 */
  

#define EQP_MAXWORD	         30
#define MAXPATH	        (EQP_MAXWORD)*4+4

/* hard code the Maximum number of phases per Earthquake */
#define MAX_PHS_PER_EQ  250

/* Extern variables 
********************************/
extern int   DEBUG;                  /* debug flag */    

/* Environment stuff */
extern char  envEW_LOG[];   /* where environment variable EW_LOG 
                                        will be stored                  */
/* Database connection things */
extern char  DBservice[];        /* DBMS instance to interact with    */
extern char  DBuser[];           /* UserId to connect to database as  */
extern char  DBpassword[];       /* Password to datasource            */

extern char  BackgroundColor[]; 
extern char  HeaderLogo[]; 
extern char  FooterLogo[]; 
extern char  HeaderTag[]; 
extern char  FooterTag[]; 

/* FTP connection things */
char  FTPDir[256];
char  FTPHost[256];


extern int errno;   /* system error variable */
/* End of External variables 
********************************/


/* Function prototypes
 *********************/

/*** FROM config.c ***/
int  ReadConfig( char * );                      /* reads configuration file */

/*** FROM eqparam2html.c ***/
int  CompareValue( const void *, const void * );    /* for sorting filelist */

/*** FROM webhelper.c ***/
int  InputEventIds( int **, int * );    /* webparse.c  eventid's from stdin  */

/*** FROM htmlreply.c ***/
void html_origin(EWDB_OriginStruct * );  
void html_arrivals(EWDB_ArrivalStruct *, int ); 
void html_station(EWDB_StationStruct *pStation, 
                  char *ftphost, char *ftpdir );


/*** FROM EXTERNAL LIBRARIES ***/
void logit_init( char *, short, int, int ); /* logit.c      sys-independent  */
void logit( char *, char *, ... );          /* logit.c      sys-independent  */

/* End of Function prototypes
 ****************************/




