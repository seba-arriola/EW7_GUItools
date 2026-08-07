
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: db_cleanup.h,v 1.2 2001/07/01 21:55:14 davidk Exp $
 *
 *    Revision history:
 *
 */


#ifndef _H_ARCHIVE
#define _H_ARCHIVE

/* lengths of directory paths and names of things
*************************************************/
#define APP_MAXPATH 480
#define APP_MAXWORD 50


/* Globals to set from configuration file
 ****************************************/
/* Database connection things */
char  DBservice[APP_MAXWORD];        /* DBMS instance to interact with    */
char  DBuser[APP_MAXWORD];           /* UserId to connect to database as  */
char  DBpassword[APP_MAXWORD];       /* Password to datasource            */
char  envEW_LOG[APP_MAXPATH+8]; 
int	  CleanDebug;
int	  StartDate;
int	  NumberOfDays;
int	  DeleteTraceOnly;
int	  Save;
char  SACdatadir[APP_MAXPATH];
char  OutputFormat[APP_MAXWORD];
long  TraceBufferLen;



/* Function prototypes
 *********************/
int  ReadConfig( char * );					/* config.c, reads configuration file */
void logit_init( char *, short, int, int ); /* logit.c      sys-independent  */
void logit( char *, char *, ... );          /* logit.c      sys-independent  */


#endif _H_ARCHIVE
