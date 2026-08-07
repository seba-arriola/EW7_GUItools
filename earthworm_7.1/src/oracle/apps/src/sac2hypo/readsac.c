
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: readsac.c,v 1.3 2000/05/02 15:41:19 lucky Exp $
 *
 *    Revision history:
 *     $Log: readsac.c,v $
 *     Revision 1.3  2000/05/02 15:41:19  lucky
 *     Major rewrite - using DBEventSacArc for the routines to translate
 *     between Sac and DBstructs, and then from DBstructs to ARC messages.
 *     readsac stuff is no longer used.
 *
 *     Revision 1.2  2000/03/29 20:58:58  lucky
 *     added include of ws_clientII.h
 *
 *     Revision 1.1  2000/02/14 19:13:13  lucky
 *     Initial revision
 *
 *
 */

/*
 *   This file is under RCS - do not modify unless you have
 *   checked it out using the command checkout.
 *
 *    $Id: readsac.c,v 1.3 2000/05/02 15:41:19 lucky Exp $
 *    Revision history:
 *
 *    $Log: readsac.c,v $
 *    Revision 1.3  2000/05/02 15:41:19  lucky
 *    Major rewrite - using DBEventSacArc for the routines to translate
 *    between Sac and DBstructs, and then from DBstructs to ARC messages.
 *    readsac stuff is no longer used.
 *
 *    Revision 1.2  2000/03/29 20:58:58  lucky
 *    added include of ws_clientII.h
 *
 *    Revision 1.1  2000/02/14 19:13:13  lucky
 *    Initial revision
 *
 *    Revision 1.1  1999/05/05 18:49:22  lucky
 *    Initial revision
 *
 *
 */
  
/* readsac.c 
 *   Functions to read a sac header and fill it 
 *   structures in the ora_api format.
 *   Lynn Dietz  Aug. 1998
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ws_clientII.h>
#include <sachead.h>
#include <time_ew.h>
#include "sac2hypo.h"

/* Function prototypes
 *********************/
double SAC_reftime( struct SAChead * );    /* convert reference time to double sec */
size_t SAC_Ktostr( char *, int, char * );  /* convert K-field to null-term string  */

/******************************************************
 * SAC_reftime() reads the reference time in the SAC  *
 *   header and returns it in double seconds since    *
 *   January 1, 1970 00:00:00 GMT                     *
 *   Returns: -1 on error                             *
 ******************************************************/
double SAC_reftime( struct SAChead *psac )
{
   struct tm stm;
   time_t    tt;
   double    tref;

/* Find seconds from 1970.01.01 to Jan 1 of the correct year
 ***********************************************************/
   stm.tm_year  = (int) psac->nzyear - 1900l;  /* tm_year = yrs since 1900 */
   stm.tm_mon   = 0;                           /* January */
   stm.tm_mday  = 1;                           /* 1st */
   stm.tm_hour  = (int) psac->nzhour;
   stm.tm_min   = (int) psac->nzmin;
   stm.tm_sec   = (int) psac->nzsec;
   stm.tm_isdst = 0;
   stm.tm_wday  = 0;                           /* not used by timegm */
   stm.tm_yday  = 0;                           /* not used by timegm */

	tt = timegm_ew (&stm);

	if (tt < 0) 
	{
		fprintf (stderr, "Call to timegm_ew failed.\n");
		return tt;
	}

/* Then add the number of seconds for the day-of-year plus milliseconds.
 ***********************************************************************/
   tref = (double)tt +                  /* sec since 1970 to Jan 1 this year */
          86400.*(psac->nzjday-1) +     /* sec since Jan 1 in this year      */
          (double)psac->nzmsec/1000.;   /* milliseconds */
  
   return( tref );
}


/****************************************************************
 * SAC_Ktostr() convert a SAC header K-field (which is filled   *
 *   with white-space) into a null-terminated character string  *
 *   Returns the length of the new string.                      *
 ****************************************************************/
size_t SAC_Ktostr( char *s1, int len1, char *s2 )
{
   int i;
   char tmp[K_LEN+1];

/* NULL-fill the target
 **********************/
   for( i=0; i<len1; i++ ) s1[i]='\0';

/* Is the K-field NULL ? 
 ***********************/
   strncpy( tmp, s2, K_LEN ); 
   tmp[K_LEN] = '\0';
   if( strcmp( tmp, SACSTRUNDEF ) == 0 )  return( 0 );

/* Null terminate after last non-space character 
 ***********************************************/
   for( i=K_LEN-1; i>=0; i-- )     /* start at the end of the string */
   {
     if( tmp[i]!=' ' &&  tmp[i]!='\0' )  /* find last non-space char */
     {
        tmp[i+1]='\0';   /* null-terminate after last non-space char */
        break;
     }
   }
   if( i<0 ) return( 0 );  /* K-field was empty */

   strncpy( s1, tmp, len1-1 );
   return( strlen( s1 ) );
}


/********************************************************************
 * SAC_toOrigin() reads a SAC header and fills in an OriginStruct   *
 *    Returns:  0 if everything was read properly.                  *
 *             -1 if there was no event info in the SAC header.     *
 *                In this case, the ArrivalStruct is full of NULLs  *
 ********************************************************************/
int SAC_toOrigin( struct SAChead *psac, OriginStruct *por )
{
   double tref;

/* Put all nulls in the OriginStruct
 ***********************************/
   memset( por, NULL, sizeof(OriginStruct) );

/* Return now if there's no origin time
 **************************************/
   if( psac->o == (float)SACUNDEF ) return( -1 );

/* Absolute event origin time  
 ****************************/
   tref    = SAC_reftime( psac );
   por->ot = (double)psac->o + tref;    

/* Event location
 ****************/
   if( psac->evla != (float)SACUNDEF ) por->lat = psac->evla;
   if( psac->evlo != (float)SACUNDEF ) por->lon = psac->evlo;
   if( psac->evdp != (float)SACUNDEF ) por->z   = psac->evdp / 1000.;

   return( 0 );  
}

/********************************************************************
 * SAC_toArrival() reads a SAC header and fills in an ArrivalStruct *
 *    Returns:  0 if everything was read properly.                  *
 *             -1 if there was no arrival in the SAC header.        *
 *                In this case, the ArrivalStruct is full of NULLs  *
 ********************************************************************/
int SAC_toArrival( struct SAChead *psac, ArrivalStruct *parr )
{
   double tref;
   char   cqual;
   char   kstr[K_LEN+1];
   size_t klen;

/* Put all nulls in the ArrivalStruct
 ************************************/
   memset( parr, NULL, sizeof(ArrivalStruct) );

/* Return now if there's no pick
 *******************************/
   if( psac->a == (float)SACUNDEF ) return( -1 );

/* Read Station, Channel, Network codes
 **************************************/
   SAC_Ktostr( parr->sta,  7, psac->kstnm  );
   SAC_Ktostr( parr->chan, 9, psac->kcmpnm );
   SAC_Ktostr( parr->net,  9, psac->knetwk );

/* Absolute Arrival time & description 
 *************************************/
   tref    = SAC_reftime( psac );
   parr->t = (double)psac->a + tref;   

   klen = SAC_Ktostr( kstr, K_LEN+1, psac->ka );
   if( klen >= 3 )
   {          
      cqual = kstr[klen-1];
      parr->qual  = (short)atoi(&cqual);
      parr->fm    = kstr[klen-2];
      strncpy(parr->ph, kstr, klen-2 );
      parr->ph[2] = '\0';
   }

/* Coda stuff
 ************/
   if( psac->f != (float)SACUNDEF )
   {
      parr->xtpcoda = (short)(psac->f - psac->a);
      klen = SAC_Ktostr( kstr, K_LEN+1, psac->kf );
      if(klen) sscanf( kstr, "%hd", &parr->Mdwt );
   }

   return( 0 );
}
