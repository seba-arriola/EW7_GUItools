
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: unlocker.h,v 1.1 2002/05/15 20:45:45 davidk Exp $
 *
 *    Revision history:
 *
 */


#ifndef UNLOCKER_H
#define UNLOCKER_H

/* lengths of directory paths and names of things
*************************************************/
#define APP_MAXPATH 480
#define APP_MAXWORD 50
#define APP_MAXRULES 20


/* Error codes for db_cleanup: */
#define APP_ERROR_BAD_CONFIG_FILE                -1 
#define APP_ERROR_NOTHING_TO_CLEANUP             -2
#define APP_ERROR_EWDB_API_INIT_FAILED           -3
#define APP_ERROR_MALLOC_FAILED                  -4 
#define APP_ERROR_GETOLDREQUESTLIST_FAILED       -5
#define APP_ERROR_UNLOCKREQUEST_FAILED           -8
#define APP_ERROR_BAD_ARGUMENT_LIST             -12


/* Globals to set from configuration file
 ****************************************/
/* Database connection things */
extern char     DBuser[APP_MAXWORD];           /* UserId to connect to database as  */
extern char     DBpassword[APP_MAXWORD];       /* Password to datasource            */
extern char     DBservice[APP_MAXWORD];        /* DBMS instance to interact with    */

extern time_t tDeltaUnlocking;

/* Misc vars */
extern time_t   tNow;  /* Current time at start of program */
extern int      DEBUG;
extern int      MaxRequestsToHandle;

/* Function prototypes
 *********************/
int  ReadConfig( char * );					/* config.c, reads configuration file */
void logit_init( char *, short, int, int ); /* logit.c      sys-independent  */
void logit( char *, char *, ... );          /* logit.c      sys-independent  */

#endif UNLOCKER_H
