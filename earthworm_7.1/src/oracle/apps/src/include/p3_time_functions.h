
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: p3_time_functions.h,v 1.3 2000/09/21 19:03:52 lucky Exp $
 *    Revision history:
 *
 *    $Log: p3_time_functions.h,v $
 *    Revision 1.3  2000/09/21 19:03:52  lucky
 *    Added prototype for P3_atot_msec
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
  
/* P3_time_functions.h */

/* You need to include the following header files before
   this one:
   #include <stdlib.h>
   #include <stdio.h>
   #include <time.h>
************************************/

/* Function Prototypes for P3_time_functions.c */
int P3_time_init(void);
time_t P3_atot(char * pBuffer);
int    P3_atot_msec(char * pBuffer, double *secs);
char * P3_ttoa(time_t * pTime, char * pBuffer);
char * P3_tmtoa(struct tm *ptm, char * pBuffer);
char * P3_tmtoa_date(struct tm *ptm, char * pBuffer);
char * P3_tmtoa_time(struct tm *ptm, char * pBuffer);
int P3_atotm(struct tm *ptm, char * pBuffer);
int P3_atotm_date(struct tm *ptm, char * pBuffer);
int P3_atotm_time(struct tm *ptm, char * pBuffer);
/* End Function Prototypes for P3_time_functions.c */
