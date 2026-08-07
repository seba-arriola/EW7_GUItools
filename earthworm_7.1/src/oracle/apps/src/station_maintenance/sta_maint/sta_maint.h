/*                                                          *
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE *
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.             *
 *
 *    $Id: sta_maint.h,v 1.2 2003/12/13 02:23:06 davidk Exp $
 *
 *    Revision history:
 *     $Log: sta_maint.h,v $
 *     Revision 1.2  2003/12/13 02:23:06  davidk
 *     Changed the parse constants, so that the idField is extended,
 *     so as to start 1 character earlier in the line.  The field
 *     still ends at the same spot, so it is 1 char longer.
 *
 *
 *
 ************************************************************/

#include <string.h>

#define PM_DISPLAY_UNDEFINED 0
#define PM_DISPLAY_SITE_INFO 1
#define PM_DISPLAY_COMP_INFO 2
#define PM_DISPLAY_CHAN_INFO 4

#define RUN_QUERY_UNDEFINED 0
#define RUN_QUERY_ALL_SITET 1
#define RUN_QUERY_ALL_COMPT 2
#define RUN_QUERY_ALL_CHAN_W_RESPONSE 3
#define RUN_QUERY_ALL_CHAN_WO_RESPONSE 4

#define STA_FILE_OFFSET_VAR 45
#define STA_FILE_OFFSET_CMD  9
#define STA_FILE_OFFSET_ID  17
#define STA_FILE_OFFSET_STA 29
#define STA_FILE_OFFSET_NET 34
#define STA_FILE_OFFSET_CMP 37
#define STA_FILE_OFFSET_LOC 42

#define STA_FILE_LEN_CMD     5
#define STA_FILE_LEN_ID     11
#define STA_FILE_LEN_STA     5
#define STA_FILE_LEN_NET     3
#define STA_FILE_LEN_CMP     5
#define STA_FILE_LEN_LOC     3

#define SPECIAL_USE_PREVIOUS "$(PREV)"
#define SPECIAL_IDPREVIOUS          -2
#define SPECIAL_NULL_LOC_STRING "--"
extern int errno;
extern EWDB_ChannelStruct GlobalChan;

char * strncpy_ew(char * dest, const char * source, size_t iLenIncludingNull);

int HandleSiteCommand(char * szCmd, char * szLineBuffer, FILE * fOut);
int HandleSiteTCommand(char * szCmd, char * szLineBuffer, FILE * fOut);
int HandleCompCommand(char * szCmd, char * szLineBuffer, FILE * fOut);
int HandleCompTCommand(char * szCmd, char * szLineBuffer, FILE * fOut);
int HandleChanCommand(char * szCmd, char * szLineBuffer, FILE * fOut);
int HandleRespCommand(char * szCmd, char * szLineBuffer, FILE * fOut);


int WriteToOutputFile(char * szOutputString);

int ParsePolesAndZeroes(const char * szInputString, 
                        EWDB_TransformFunctionStruct *pFunc,
                        char * szStatusBuffer);


