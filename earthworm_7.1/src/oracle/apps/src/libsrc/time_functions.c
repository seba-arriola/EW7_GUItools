/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: time_functions.c,v 1.2 2001/07/01 21:55:24 davidk Exp $
 *
 *    Revision history:
 *     $Log: time_functions.c,v $
 *     Revision 1.2  2001/07/01 21:55:24  davidk
 *     Cleanup of the Earthworm Database API and the applications that utilize it.
 *     The "ewdb_api" was cleanup in preparation for Earthworm v6.0, which is
 *     supposed to contain an API that will be backwards compatible in the
 *     future.  Functions were removed, parameters were changed, syntax was
 *     rewritten, and other stuff was done to try to get the code to follow a
 *     general format, so that it would be easier to read.
 *
 *     Applications were modified to handle changed API calls and structures.
 *     They were also modified to be compiler warning free on WinNT.
 *
 *     Revision 1.1  2001/02/28 17:29:10  lucky
 *     Initial revision
 *
 *     Revision 1.4  2000/09/21 19:07:48  lucky
 *     Added EWDB_atot_msec.
 *     Modified EWDB_atot to return seconds in UTC (not local time).
 *
 *     Revision 1.3  2000/05/03 19:17:04  lucky
 *     Added EWDB_at*_msec function which returns miliseconds as well as
 *     fills the tm struct. This was required for the Dewey project.
 *
 *     Revision 1.2  2000/02/09 21:29:39  davidk
 *     added a return(0) for succ return of EWDB_time_init().
 *
 *     Revision 1.1  1999/11/09 17:40:50  lucky
 *     Initial revision
 *
 *
 */


/* include all the basic header files, NOTHING SPECIAL!! */
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <chron3.h>

/* include our own header file for function prototypes and junk */
#include <time_functions.h>

/* External Function Prototypes */
void logit(char *flag,char *format,...);
/* End External Function Prototypes */


int EWDB_time_init(void)
/******************************************************
  Function:    EWDB_time_init()
  Purpose:     Initialize the TZ environment variable to 
               GMT (UTC) timezone, so as to eliminate any
               hanky panky adjusting of time values in 
               mktime() and other system time functions.
  Parameters:    
      Input
      (NONE)

      Output
      ENVIRONMENT VARIABLE TZ:
               set to "GMT"
      
  Author:
               DK, before 04/15/1999

  Internal Functions Called:
  Library Functions Called: 
  EW Library Functions Called:
  External Library Functions Called:
  System Functions Called:  
               putenv(),fprintf()
  **********************************************************/
{
  if( putenv( "TZ=GMT" ) != 0 )  /*set environment variable for logit*/
  {
    fprintf( stdout,"time_init(): putenv: unable to set "
      "TZ environment variable.  Time's will be off by GMT-Timezone.\n" );
    return( -1 );
  }
  return(0);
}  

time_t EWDB_atot(char * pBuffer)
/******************************************************
  Function:    atot()
  Purpose:     Convert an Ascii date/time string to a
               time_t variable.
  Parameters:    
      Input
      pBuffer: Ascii time string of the form 
                      1900/01/01, 00:00:00 

 
  Return Value:
               The time_t variable the ascii string
               was converted to, or -1 on error.
  Author:
               DK, before 04/15/1999

  Internal Functions Called:
  Library Functions Called: 
               atotm(),
  EW Library Functions Called:
  External Library Functions Called:
  **********************************************************/
{
  struct tm tmTime;
  char      otime[50];
  double    retval;

	if (pBuffer == NULL)
	{
		logit ("", "EWDB_atot: invalid arguments passed in.\n");
		return -1;
	}

  /* convert the string to a struct tm */
  if(EWDB_atotm(&tmTime,pBuffer) == 0)
  {
    /* convert the struct tm to a time_t value */

    sprintf (otime, "%04d%02d%02d%02d%02d%02d.00",
                   (tmTime.tm_year + 1900), (tmTime.tm_mon + 1),
                   tmTime.tm_mday, tmTime.tm_hour,
                   tmTime.tm_min, tmTime.tm_sec);

    epochsec17 (&retval, otime);

    return ((int) retval);
  }
  else
  {
    return(-1);
  }
}  /* End of atot() */



char * EWDB_ttoa(time_t * pTime, char * pBuffer)
/******************************************************
  Function:    ttoa()
  Purpose:     Convert a time_t variable to an ascii string.
  Parameters:    
      Input
      pTime:   Pointer to the time_t variable to be converted

      Output
      pBuffer: String where the Ascii representation of pTime 
               is to be placed.  Of the form 
               1900/01/01, 00:00:00 

 
  Return Value:
               A pointer to pBuffer
  Author:
               DK, before 04/15/1999

  Internal Functions Called:
  Library Functions Called: 
               tmtoa()
  EW Library Functions Called:
  External Library Functions Called:
  System Functions Called:  
               gmtime(),memcpy(),sizeof()
  **********************************************************/
{
  struct tm tmTime;
  struct tm * ptmTime;

  ptmTime=gmtime(pTime);
  memcpy(&tmTime,ptmTime,sizeof(struct tm));
  return(EWDB_tmtoa(&tmTime,pBuffer));
}  /* End of ttoa() */

char * EWDB_tmtoa(struct tm *ptm, char * pBuffer)
{
/******************************************************
  Function:    tmtoa()
  Purpose:     Convert a struct tm variable to an ascii string.
  Parameters:    
      Input
      ptm:     Pointer to the struct tm variable to be converted

      Output
      pBuffer: String where the Ascii representation of ptm 
               is to be placed.  Of the form 
               1900/01/01, 00:00:00 

  Return Value:
               A pointer to pBuffer
  Author:
               DK, before 04/15/1999

  Internal Functions Called:
  Library Functions Called: 
  EW Library Functions Called:
  External Library Functions Called:
  System Functions Called:  
               sprintf()

  ****Note: This function does not deal with timezones,
     but its underlying functions do.  Times will
     likely be off in proportion to gmtime.
  **********************************************************/
  sprintf(pBuffer,"%04u/%02u/%02u, %02u:%02u:%02u",
          ptm->tm_year+1900,ptm->tm_mon+1,ptm->tm_mday,
          ptm->tm_hour,ptm->tm_min,ptm->tm_sec);
  return(pBuffer);
}

char * EWDB_tmtoa_date(struct tm *ptm, char * pBuffer)
/******************************************************
  Function:    tmtoa_date()
  Purpose:     Convert only the date portion of a struct tm 
               variable to an ascii string.
  Parameters:    
      Input
      ptm:     Pointer to the struct tm variable to be converted

      Output
      pBuffer: String where the Ascii representation of ptm 
               is to be placed.  Of the form 
               1900/01/01

  Return Value:
               A pointer to pBuffer
  Author:
               DK, before 04/15/1999

  Internal Functions Called:
  Library Functions Called: 
  EW Library Functions Called:
  External Library Functions Called:
  System Functions Called:  
               sprintf()

  ****Note: This function does not deal with timezones,
     but its underlying functions do.  Times will
     likely be off in proportion to gmtime.
  **********************************************************/
{
  sprintf(pBuffer,"%04u/%02u/%02u",
          ptm->tm_year+1900,ptm->tm_mon+1,ptm->tm_mday);
  return(pBuffer);
}

char * EWDB_tmtoa_time(struct tm *ptm, char * pBuffer)
/******************************************************
  Function:    tmtoa_time()
  Purpose:     Convert only the time portion of a struct tm 
               variable to an ascii string.
  Parameters:    
      Input
      ptm:     Pointer to the struct tm variable to be converted

      Output
      pBuffer: String where the Ascii representation of ptm 
               is to be placed.  Of the form 
               00:00:00

  Return Value:
               A pointer to pBuffer
  Author:
               DK, before 04/15/1999

  Internal Functions Called:
  Library Functions Called: 
  EW Library Functions Called:
  External Library Functions Called:
  System Functions Called:  
               sprintf()

  ****Note: This function does not deal with timezones,
     but its underlying functions do.  Times will
     likely be off in proportion to gmtime.
  **********************************************************/
{
  sprintf(pBuffer,"%02u:%02u:%02u",
          ptm->tm_hour,ptm->tm_min,ptm->tm_sec);
  return(pBuffer);
}

int EWDB_atotm(struct tm *ptm, char * pBuffer)
/******************************************************
  Function:    atotm()
  Purpose:     Convert an Ascii date/time string to a
               struct tm variable.
  Parameters:    
      Input
      pBuffer: Ascii time string of the form 
                      1900/01/01, 00:00:00 
      Output
      ptm:     Pointer to a struct tm where the contents
               of the ASCII time string are placed.
 
  Return Value:
               Success flag: 0 on success, -1 on error.
  Author:
               DK, before 04/15/1999

  Internal Functions Called:
  Library Functions Called: 
              logit()
  EW Library Functions Called:
  External Library Functions Called:
  System Functions Called:  
               strchr(),atoi()
  **********************************************************/
{
  char * pTemp;

/*1900/01/01, 00:00:00 */


  pTemp=pBuffer;

  /* Get the 4 digit year */
  pTemp=strchr(pTemp,'/');
  if(pTemp == NULL  || (pTemp-4) < pBuffer)
  {
    logit("","Returning -1 at year\n");
    return(-1);
  }
  else
    pTemp-=4; /* move back to the year digits from the '/' */
  ptm->tm_year=atoi(pTemp)-1900;

  /* Get the month */
  pTemp +=5;  /* YYYY/MM/DD move from first Y to first M */
  ptm->tm_mon=atoi(pTemp)-1;  /* -1 because we're tracking mo's since Jan. */

  /* Get the day of month */
  pTemp=strchr(pTemp,'/');
  if(pTemp == NULL)
  {
    logit("","Returning -1 at day\n");
    return(-1);
  }
  else
    pTemp++; /* move past the '/' */
  ptm->tm_mday=atoi(pTemp);

  /* Get the hour */
  pTemp=strchr(pTemp,':');
  if(pTemp == NULL)
  {
    logit("","Returning -1 at hour\n");
    return(-1);
  }
  else
    pTemp-=2; /* move back to the hour digits from the ':' */
  ptm->tm_hour=atoi(pTemp);

  /* Get the minute */
  pTemp +=3;  /* HH:MM:SS move from first H to first M */
  ptm->tm_min=atoi(pTemp);

  /* Get the second */
  pTemp=strchr(pTemp,':');
  if(pTemp == NULL)
  {
    logit("","Returning -1 at second\n");
    return(-1);
  }
  else
    pTemp++; /* move past the ':' */
  ptm->tm_sec=atoi(pTemp);

  return(0);
}  /* End of atotm() */


int EWDB_atotm_date(struct tm *ptm, char * pBuffer)
/******************************************************
  Function:    atotm_date()
  Purpose:     Convert an Ascii date string to a
               struct tm variable.
  Parameters:    
      Input
      pBuffer: Ascii date string of the form 
                      1900/01/01
      Output
      ptm:     Pointer to a struct tm where the contents
               of the ASCII time string are placed.
 
  Return Value:
               Success flag: 0 on success, -1 on error.
  Author:
               DK, before 04/15/1999

  Internal Functions Called:
  Library Functions Called: 
              logit()
  EW Library Functions Called:
  External Library Functions Called:
  System Functions Called:  
               strchr(),atoi()
  **********************************************************/
{
  char * pTemp;

/*1900/01/01, 00:00:00 */


  pTemp=pBuffer;

  /* Get the 4 digit year */
  pTemp=strchr(pTemp,'/');
  if(pTemp == NULL  || (pTemp-4) < pBuffer)
  {
    logit("","Returning -1 at year\n");
    return(-1);
  }
  else
    pTemp-=4; /* move back to the year digits from the '/' */
  ptm->tm_year=atoi(pTemp)-1900;

  /* Get the month */
  pTemp +=5;  /* YYYY/MM/DD move from first Y to first M */
  ptm->tm_mon=atoi(pTemp)-1;  /* -1 because we're tracking mo's since Jan. */

  /* Get the day of month */
  pTemp=strchr(pTemp,'/');
  if(pTemp == NULL)
  {
    logit("","Returning -1 at day\n");
    return(-1);
  }
  else
    pTemp++; /* move past the '/' */
  ptm->tm_mday=atoi(pTemp);

  return(0);
}  /* End of atotm_date() */


int EWDB_atotm_time(struct tm *ptm, char * pBuffer)
/******************************************************
  Function:    atotm_time()
  Purpose:     Convert an Ascii time string to a
               struct tm variable.
  Parameters:    
      Input
      pBuffer: Ascii time string of the form 
                      00:00:00
      Output
      ptm:     Pointer to a struct tm where the contents
               of the ASCII time string are placed.
 
  Return Value:
               Success flag: 0 on success, -1 on error.
  Author:
               DK, before 04/15/1999

  Internal Functions Called:
  Library Functions Called: 
              logit()
  EW Library Functions Called:
  External Library Functions Called:
  System Functions Called:  
               strchr(),atoi()
  **********************************************************/
{
  char * pTemp;

/*00:00:00 */


  pTemp=pBuffer;


  /* Get the hour */
  pTemp=strchr(pTemp,':');
  if(pTemp == NULL)
  {
    logit("","Returning -1 at hour\n");
    return(-1);
  }
  else
    pTemp-=2; /* move back to the hour digits from the ':' */
  ptm->tm_hour=atoi(pTemp);

  /* Get the minute */
  pTemp +=3;  /* HH:MM:SS move from first H to first M */
  ptm->tm_min=atoi(pTemp);

  /* Get the second */
  pTemp=strchr(pTemp,':');
  if(pTemp == NULL)
  {
    logit("","Returning -1 at second\n");
    return(-1);
  }
  else
    pTemp++; /* move past the ':' */
  ptm->tm_sec=atoi(pTemp);

  return(0);
}  /* End of atotm_time() */


int EWDB_atotm_time_msec (struct tm *ptm, int *msec, char * pBuffer)
/******************************************************
  Function:    EWDB_atotm_time_msec()
  Purpose:     Convert an Ascii time string to a
               struct tm variable, and put miliseconds
			   into the msec argument.
  Parameters:    
      Input
      pBuffer: Ascii time string of the form 
                      00:00:00.000
      Output
      ptm:     Pointer to a struct tm where the contents
               of the ASCII time string are placed.
 
  Return Value:
               Success flag: 0 on success, -1 on error.
  Author:
               Lucky Vidmar, 5/3/2000

  Internal Functions Called:
  Library Functions Called: 
              logit()
  EW Library Functions Called:
  External Library Functions Called:
  System Functions Called:  
               strchr(),atoi()
  **********************************************************/
{
  char * pTemp;

/*00:00:00.000 */


  pTemp=pBuffer;


  /* Get the hour */
  pTemp=strchr(pTemp,':');
  if(pTemp == NULL)
  {
    logit("","Returning -1 at hour\n");
    return(-1);
  }
  else
    pTemp-=2; /* move back to the hour digits from the ':' */
  ptm->tm_hour=atoi(pTemp);

  /* Get the minute */
  pTemp +=3;  /* HH:MM:SS move from first H to first M */
  ptm->tm_min=atoi(pTemp);

  /* Get the second */
  pTemp=strchr(pTemp,':');
  if(pTemp == NULL)
  {
    logit("","Returning -1 at second\n");
    return(-1);
  }
  else
    pTemp++; /* move past the ':' */
  ptm->tm_sec=atoi(pTemp);

  /* Get the milisecond */
  pTemp=strchr(pTemp,'.');
  if(pTemp == NULL)
  {
    logit("","Returning -1 at milisecond\n");
    return(-1);
  }
  else
    pTemp++; /* move past the '.' */
  *msec = atoi(pTemp);

  return(0);

}  /* End of atotm_time() */



int             EWDB_atot_msec (char * pBuffer, double *secs)
/******************************************************
  Purpose:     Convert an Ascii date/time string to
               number of seconds and miliseconds since 1970.

              Input string format:

                   2000/09/21 18:26:53.94

  **********************************************************/
{

	struct tm       tmTime;
	char            timestr[30];
	char            otime[50];
	/* size_t          val; */
	int             msec;
	double          retval;


	if ((pBuffer == NULL) || (secs == NULL))
	{
		logit ("", "EWDB_atot_msec: Invalid arguments passed in.\n");
		return -1;
	}

	/* convert the string to a struct tm */
	if (EWDB_atotm (&tmTime, pBuffer) == 0)
	{
		/* convert the struct tm to a time_t value */

		strncpy (timestr, &pBuffer[20], 2);
		timestr[2] ='\0';
		msec = atoi (timestr);

		sprintf (otime, "%04d%02d%02d%02d%02d%02d.%02d",
							(tmTime.tm_year + 1900), (tmTime.tm_mon + 1),
							tmTime.tm_mday, tmTime.tm_hour,
							tmTime.tm_min, tmTime.tm_sec, msec);

		epochsec17 (&retval, otime);

		*secs = retval;
		return 1;
	}
	else
	{
		return -1;
	}

}  /* End of EWDB_atot_msec() */


/* <End Of File> */


