/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ora_trace_fetch.h,v 1.6 2003/02/04 17:58:39 davidk Exp $
 *
 *    Revision history:
 *     $Log: ora_trace_fetch.h,v $
 *     Revision 1.6  2003/02/04 17:58:39  davidk
 *     Added debug-level constants.
 *     Added EW Status ERROR constant for a scheduling algorithm error.
 *
 *     Revision 1.5  2002/05/16 17:11:02  davidk
 *     Moved a bunch of constants from ora_trace_fetch.c to ora_trace_fetch.h
 *     so that they could also be used by schedule.c.
 *
 *     Revision 1.4  2002/05/14 21:04:28  davidk
 *     moved scheduling constants to schedule.h
 *
 *     Revision 1.3  2002/05/13 23:19:24  davidk
 *     removed unneccessary variables, addded constants.
 *
 *     Revision 1.2  2001/02/28 17:29:10  lucky
 *     Massive schema redesign and cleanup.
 *
 *     Revision 1.1  2001/01/18 17:12:48  davidk
 *     Initial revision
 *
 *
 */

/*
 * ora_trace_fetch.h : Include file for ora_trace_fetch.c; 
 *              copied from ora_trace_save.h.
 */


/* Constants for program mode.
   Program Mode controls what action is taken each time the
   program runs through the main processing loop.
 ***********************************************************/
#define OTF_PROGRAM_MODE_UNDEFINED           0
#define OTF_PROGRAM_MODE_STANDARD_DATA_MODE  1
#define OTF_PROGRAM_MODE_ALWAYS_CHECK_LIST   2
#define OTF_PROGRAM_MODE_ALWAYS_GET_SNIPPETS 3


/* Error messages used by ora_trace_fetch 
 ***************************************/
#define  ERR_MISSMSG       0   /* message missed in transport ring       */
#define  ERR_TOOBIG        1   /* retreived msg too large for buffer     */
#define  ERR_NOTRACK       2   /* msg retreived; tracking limit exceeded */
#define  ERR_API_INIT_FAILED 3 /* ORA_API_INIT failed */
#define  ERR_XALLOC_ERROR  4   /* unable to dynamically allocate memory  */
#define  ERR_UNKNOWN       5   /* msg retreived; unkown error */
#define  ERR_QUEUE         6   /* error queueing message for sending     */
#define  ERR_MSG_STACKER   7   /* error with message stacker thread      */
#define  ERR_SNPT_MAKER    8   /* error with snippet maker thread        */
#define  ERR_WAVE_SERVER   9   /* error getting data from a wave_server  */
#define  ERR_SNIPPET2TRREQ 10  /* error in call to snippet2trReq         */
#define  ERR_ORA_STSNEVENT 11  /* error in call to DB ewdb_apps_putaway_StartSnippetEvent()*/
#define  ERR_WSGETTRACEBIN 12  /* error in call to wsGetTraceBin()       */
#define  ERR_STUFF_SNPT    13  /* can't stuff snippet into dbms          */
#define  ERR_BAD_SNIPREQ   14  /* can't parse a snippet request          */
#define  ERR_SNIPPET_TOO_BIG 15/* snippet length exeeds MaxSnippetSize   */
#define  ERR_SNIPPET_OVERLAP 16/* snippets overlap and we can't merge them */
#define  ERR_SCHED_SNIPREQ 17  /* sched alg not able to reschedule snipreq */


/* Snippet retrieval statii used by ora_trace_fetch 
 ****************************************************/
#define SNIPPET_REQUEST_ATTEMPT_PARTIAL_SUCCESS 1
#define SNIPPET_REQUEST_ATTEMPT_FAILURE        -1
#define SNIPPET_REQUEST_ATTEMPT_SUCCESS         2
#define SNIPPET_REQUEST_ATTEMPT_UNDEFINED       0

/* Defines 
 *********/
#define TRUE    1
#define FALSE    0

#define MAX_STR     255
#define MAXLINE    1000
#define MAX_NAME_LEN    50
#define MAXCOLS     100
#define MAXTXT           350
#define MAX_WAVESERVERS   100
#define MAX_ADRLEN        20

#define DEBUG_OTF_NONE       0
#define DEBUG_OTF_ERROR     10
#define DEBUG_OTF_WARNING  100
#define DEBUG_OTF_INFO    1000
#define DEBUG_OTF_ALL    10000

extern char errText[MAXTXT];   /* string for log/error messages          */

#include <ewdb_ora_api.h> 

