
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: archive.h,v 1.2 2001/07/01 21:55:13 davidk Exp $
 *
 *    Revision history:
 *     $Log: archive.h,v $
 *     Revision 1.2  2001/07/01 21:55:13  davidk
 *     Cleanup of the Earthworm Database API and the applications that utilize it.
 *     The "ewdb_api" was cleanup in preparation for Earthworm v6.0, which is
 *     supposed to contain an API that will be backwards compatible in the
 *     future.  Functions were removed, parameters were changed, syntax was
 *     rewritten, and other stuff was done to try to get the code to follow a
 *     general format, so that it would be easier to read.
 *
 *     Applications were modified to handle changed API calls and structures.
 *     They were also modified to be compiler warning free on WinNT.
 *
 *     Revision 1.1  2000/02/15 19:02:31  lucky
 *     Initial revision
 *
 *
 */

 /*
 * archive.h : Include file for archive.c
 *
 */

#ifndef _H_ARCHIVE
#define _H_ARCHIVE

/* lengths of directory paths and names of things
*************************************************/
#define APP_MAXPATH 480
#define APP_MAXWORD 50
#define MAX_PHS_PER_EQ  250
#define NQUICKLOOK  10

/* Environment stuff */
char  envEW_LOG[APP_MAXPATH+8];        /* where environment variable EW_LOG
                                        will be stored                  */
/* SAC, Hypoinverse stuff */
char  SACdatadir[APP_MAXPATH+1];
char  SACmacrodir[APP_MAXPATH+1];
int   nQuickLook;
char  HypoinvCmdFile[APP_MAXPATH+1];

/* Database connection things */
char  DBservice[APP_MAXWORD];        /* DBMS instance to interact with    */
char  DBuser[APP_MAXWORD];           /* UserId to connect to database as  */
char  DBpassword[APP_MAXWORD];       /* Password to datasource            */

/* Globals to set from configuration file
 ****************************************/
int   ArchDebug;           /* debug flag */
long  TraceBufferLen;
long  MaxTraces;
char  OutputFormat[APP_MAXWORD];



/* DANGER DANGER DANGER: these are also defined in ora2sac.h
************************************************************/
#define MAX_PHS_PER_EQ  250
#define NQUICKLOOK  10
#define SUCCESS  0
#define FAILURE -1

 /* Structure to hold a SAC file name and a sorting parameter
 ***********************************************************/

typedef struct _FileListStruct
{
        char   fname[APP_MAXWORD];  /* name of file         */
        double fsort;           /* parameter to sort on */
} FileListStruct;


/* Function prototypes
 *********************/
int  ReadConfig( char * );					/* config.c, reads configuration file */
void logit_init( char *, short, int, int ); /* logit.c      sys-independent  */
void logit( char *, char *, ... );          /* logit.c      sys-independent  */


#endif _H_ARCHIVE
