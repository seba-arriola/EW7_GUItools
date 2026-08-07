
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: db_cleanup.h,v 1.2 2004/09/09 19:28:26 davidk Exp $
 *
 *    Revision history:
 *
 */


#ifndef DB_CLEANUP_H
#define DB_CLEANUP_H

/* lengths of directory paths and names of things
*************************************************/
#define APP_MAXPATH 480
#define APP_MAXWORD 50
#define APP_MAXRULES 20

/* Information to delete */
#define DELETE_NONE       0
#define DELETE_WAVEFORMS  1
#define DELETE_PARAMETRIC 2
#define DELETE_ALL        3

/* Information to save */
#define SAVE_NONE         0
#define SAVE_WAVEFORMS    1
#define SAVE_PARAMETRIC   2
#define SAVE_OTHER        4
#define SAVE_ALL          7

/* Max unassociated records to delete */
#define APP_MAX_RECORDS_TO_DELETE 1000

/* Error codes for ArchiveEvent() */
#define FAILURE_NONE                       0
#define FAILURE_GETDBEVENTINFO             2
#define FAILURE_WRITESAC_STARTEVENT        4
#define FAILURE_PRODUCESAC_NEXTSTATION     8
#define FAILURE_WRITESAC_NEXTSTATION    0x10
#define FAILURE_WRITESAC_ENDEVENT       0x20
#define FAILURE_GETWAVEFORM_SNIPPET     0x40
#define FAILURE_SAVE_NO_WAVEFORMS       0x80

/* Error codes for db_cleanup: */
#define APP_SUCCESS                               0 
#define APP_WARNING                               1 
#define APP_ERROR_BAD_CONFIG_FILE                -1 
#define APP_ERROR_NOTHING_TO_CLEANUP             -2
#define APP_ERROR_EWDB_API_INIT_FAILED           -3
#define APP_ERROR_MALLOC_FAILED                  -4 
#define APP_ERROR_GETEVENTLIST_FAILED            -5
#define APP_ERROR_ARCHIVEEVENT_INIT_FAILED       -6 
#define APP_ERROR_ARCHIVEEVENT_FAILED            -7
#define APP_ERROR_DELETEEVENT_FAILED             -8
#define APP_ERROR_UNSUPPORTED_DELETEEVENT_MODE   -9
#define APP_ERROR_UNSUPPORTED_OTHER_DATA_MODE   -10
#define APP_ERROR_DELETE_DATA_FAILED            -11 
#define APP_ERROR_BAD_ARGUMENT_LIST             -12



/* struct that holds data for each rule in the config file */
typedef struct _EventRuleStruct
{
  EWDB_EventListStruct EVMin;
  EWDB_EventListStruct EVMax;
  unsigned int tStart;
  unsigned int tEnd;
  int iDataToSave;
  int iDataToDelete;
  int iDubiocity;
} EventRuleStruct;


/* Globals to set from configuration file
 ****************************************/
/* Database connection things */
extern char     DBuser[APP_MAXWORD];           /* UserId to connect to database as  */
extern char     DBpassword[APP_MAXWORD];       /* Password to datasource            */
extern char     DBservice[APP_MAXWORD];        /* DBMS instance to interact with    */

/* Archiving Vars*/
extern char szBaseArchiveDirectory[APP_MAXPATH];
extern char szOutputFormat[APP_MAXWORD];
extern int  bSetArchiveFlag;
extern int  iLargestSnippetSize;
extern int  bSetArchiveFlag;

/* Unassociated Data Vars*/
extern int    bHandleOtherData;
extern int    bSaveOtherData;
extern int    bDeleteOtherData;
extern time_t tOtherDataCutoffTime;

/* Misc vars */
extern time_t   tNow;  /* Current time at start of program */
extern int      DEBUG;
extern int      MaxEventsToHandle;
extern int      iNumEventRules;
extern EventRuleStruct EventRules[APP_MAXRULES];


/* Function prototypes
 *********************/
int  ReadConfig( char * );					/* config.c, reads configuration file */
void logit_init( char *, short, int, int ); /* logit.c      sys-independent  */
void logit( char *, char *, ... );          /* logit.c      sys-independent  */

int ArchiveEvent(EWDBid idEvent, int iDataToSave, int * pFailureCode);
int ArchiveEvent_init(char * szBaseArchiveDirectory, 
                      char * szOutputFormat, 
                      int IN_iLargestSnippetSize, int iDebug);

#endif DB_CLEANUP_H
