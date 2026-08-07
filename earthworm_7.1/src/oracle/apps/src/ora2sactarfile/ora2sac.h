
/*
 *   This file is under RCS - do not modify unless you have
 *   checked it out using the command checkout.
 *
 *    $Id: ora2sac.h,v 1.1 2001/07/01 21:55:25 davidk Exp $
 *    Revision history:
 *
 *    $Log: ora2sac.h,v $
 *    Revision 1.1  2001/07/01 21:55:25  davidk
 *    Initial revision
 *
 *    Revision 1.9  2001/05/15 02:15:21  davidk
 *    Moved functions around between the apps, DB API, and DB API INTERNAL
 *    levels.  Renamed functions and files.  Added support for amplitude
 *    magnitude types.  Reformatted makefiles.
 *
 *    Revision 1.8  2001/04/13 21:30:19  lucky
 *    Changed APP_MAXPATH to 512
 *
 *    Revision 1.7  2001/02/28 17:29:10  lucky
 *    Massive schema redesign and cleanup.
 *
 *    Revision 1.6  2000/08/28 16:09:30  lucky
 *    get_ew_event_info.h  -> ew_event_info.h
 *
 *    Revision 1.5  2000/08/16 17:12:39  lucky
 *    Removed write_sac, added Sac2EWEvent
 *
 *    Revision 1.4  2000/03/30 16:15:25  davidk
 *    Lucky cleaned up ora2sac.h and moved it to src/include so that ora2sactarfile
 *    and ora2sac would compile easier with the same codebase.
 *
 *    Revision 1.3  2000/03/15 08:01:11  davidk
 *    added an ORA2SACTARFILE ifdef to handle the different prototypes
 *    for html_tablerow().
 *
 *    Revision 1.2  2000/03/11 00:24:52  davidk
 *    moved all of the global variables out of this header file.  All of the SAC junk
 *    moved to library header files, so we now include the headers for the library files.
 *
 *    Revision 1.1  1999/05/05 18:32:39  lucky
 *    Initial revision
 *
 *
 */
  
/*
 * ora2sac.h : Include file for ora2sac.c
 *
 */


#include <ewdb_ora_api.h>
#include <ew_event_info.h>
#include <sac_2_ewevent.h>
#include <webparse.h>

#define APP_MAXWORD	         50
#define APP_MAXPATH	        512
#define MAX_PHS_PER_EQ  250
#define NQUICKLOOK  10

#define SUCCESS  0
#define FAILURE -1

#define ORA2SAC             100
#define ORA2SACTARFILE      200


typedef struct _Ora2SAC_EventListStruct
{
  int NumEvents;
  int * pEvents;
  int LastEvent;
} Ora2SAC_EventListStruct;



/* Globals to set from configuration file
 ****************************************/
extern int   OraDebug;
extern long  TraceBufferLen;
extern long  MaxTraces;
extern char  OutputFormat[APP_MAXWORD];


