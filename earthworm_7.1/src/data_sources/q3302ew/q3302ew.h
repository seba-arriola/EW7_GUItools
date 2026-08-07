
#ifndef __Q3302EW_H__
#define __Q3302EW_H__

#ifdef WIN32
#include <winsock2.h>
#endif

#define Q3302EW_NAME "q3302ew"
#define Q3302EW_VERSION "2.0"
#define Q3302EW_BUILD "$Rev: 11 $"

void q3302ew_die(char *);
void q3302ew_cleanup();
void q3302ew_cleanupAndExit();
#endif
