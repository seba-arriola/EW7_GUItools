
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: time_functions.h,v 1.2 2001/07/01 21:55:17 davidk Exp $
 *    Revision history:
 *
 *    $Log: time_functions.h,v $
 *    Revision 1.2  2001/07/01 21:55:17  davidk
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
 *    Revision 1.1  2001/02/28 17:29:10  lucky
 *    Initial revision
 *
 *    Revision 1.3  2000/09/21 19:03:52  lucky
 *    Added prototype for EWDB_atot_msec
 *
 *    Revision 1.2  1999/11/09 16:55:39  lucky
 *    *** empty log message ***
 *
 *    Revision 1.1  1999/10/18 15:56:20  davidk
 *    Initial revision
 *
 *    Revision 1.1  1999/05/05 18:22:00  lucky
 *    Initial revision
 *
 *
 */
  

/* You need to include the following header files before
   this one:
   #include <stdlib.h>
   #include <stdio.h>
   #include <time.h>
************************************/

/* Function Prototypes for EWDB_time_functions.c */
int EWDB_time_init(void);
time_t EWDB_atot(char * pBuffer);
int    EWDB_atot_msec(char * pBuffer, double *secs);
char * EWDB_ttoa(time_t * pTime, char * pBuffer);
char * EWDB_tmtoa(struct tm *ptm, char * pBuffer);
char * EWDB_tmtoa_date(struct tm *ptm, char * pBuffer);
char * EWDB_tmtoa_time(struct tm *ptm, char * pBuffer);
int EWDB_atotm(struct tm *ptm, char * pBuffer);
int EWDB_atotm_date(struct tm *ptm, char * pBuffer);
int EWDB_atotm_time(struct tm *ptm, char * pBuffer);
int EWDB_atotm_time_msec (struct tm *ptm, int *msec, char * pBuffer);

/* End Function Prototypes for EWDB_time_functions.c */
