
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: rw_triglist.c,v 1.1 2002/03/22 20:12:51 lucky Exp $
 *
 *    Revision history:
 *     $Log: rw_triglist.c,v $
 *     Revision 1.1  2002/03/22 20:12:51  lucky
 *     Initial revision
 *
 *
 */

  /************************************************************
   *                       parse_trig.c                       *
   *                                                          *
   ************************************************************/  

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <earthworm.h>
#include <ewdb_apps_utils.h>


#define LINE_LEN         500
#define BAD_TIME_VALUE   5

#define INIT_TRIG_ALLOC   50
#define TRIG_ALLOC_INCR   50

/** Lengths of fields in the time string **/
#define TIMESTR_LEN		17	/* whole string */
#define YMD_LEN			8	/* date part: YYYYMMDD */
#define HOUR_LEN		2	/* hours from HH:MM:SS.SS */
#define MIN_LEN			2	/* minutes from HH:MM:SS.SS */
#define SEC_LEN			5	/* seconds from HH:MM:SS.SS */

/* Function declarations */
extern int epochsec17 (double *, char *);
int getNextLine (char**, char*);


/***********************************************************************
 ***********************************************************************/
int read_triglist (char* msg, long msgSize, EWCoincEvtStruct *pCoinc)
{
	char line[LINE_LEN];
	char YYYYMMDD[20];
	char HHMMSS[20];
	char YYYYMMDD_2[20];
	char HHMMSS_2[20];
	char SnipDurStr[20];
	char *nextLine;
	size_t				toalloc;
	EWChanTriggerStruct	*pChan;
	EWChanTriggerStruct	*pChanTrigsOld;
	EWChanTriggerStruct	*pChanTrigsNew;


	if ((msg == NULL) || (msgSize < 0) || (pCoinc == NULL))
	{
		logit ("", "Invalid arguments passed in.\n");
		return EW_FAILURE;
	}

	/* Allocate space for channel triggers */
	if (pCoinc->iNumAlloc != 0)
	{
		logit ("", "Invalid Trig structure - <%d> channels allocated .\n", 
						pCoinc->iNumAlloc);
		return EW_FAILURE;
	}


	if ((pCoinc->pChanTrigs = (EWChanTriggerStruct *) malloc 
				(INIT_TRIG_ALLOC * sizeof (EWChanTriggerStruct))) == NULL)
	{
		logit ("", "Could not allocate %d trigger channels \n", INIT_TRIG_ALLOC);
		return EW_FAILURE;
	}
	pCoinc->iNumAlloc = INIT_TRIG_ALLOC;
	pCoinc->iNumTrigs = 0;


	nextLine = msg;

   /* we're looking for a line with the form shown below:

 EVENT DETECTED     19970729 03:01:13.22 UTC  EVENT ID: 123ABC AUTHOR: Harry SUBNET: redoubt
	 
	 ********************************************************/


	/* First line */
	getNextLine (&nextLine, line); /* step over the blank line */

	sscanf (line, "EVENT DETECTED     %s %s UTC EVENT ID: %s AUTHOR: %s", 
				YYYYMMDD, HHMMSS, pCoinc->sEventID, pCoinc->sAuthor);

	t_atodbl (YYYYMMDD, HHMMSS, &pCoinc->CoincEvt.tCoincidence);

logit ("", "Read eventid <%s>\n", pCoinc->sEventID);
logit ("", "Read author <%s>\n", pCoinc->sAuthor);
logit ("", "Read time <%f>\n", pCoinc->CoincEvt.tCoincidence);



	/* Step down to the channel list */

	getNextLine (&nextLine, line); /* step over the blank line */
	getNextLine (&nextLine, line); /* step over the column titles line */
	getNextLine (&nextLine, line); /* step over the silly dashes line */


	/* loop through the channel trigger lines until we can loop no more */
	while (getNextLine (&nextLine, line) > 0)
	{
		if (pCoinc->iNumTrigs >= pCoinc->iNumAlloc)
		{
			/* Need more channels */
			toalloc = pCoinc->iNumAlloc + TRIG_ALLOC_INCR;
			logit ("e", "Max Channels (%d) reached. Alloc to %d\n", 
					pCoinc->iNumAlloc, toalloc);


			pChanTrigsOld = pCoinc->pChanTrigs;

			if ((pChanTrigsNew = (EWChanTriggerStruct *) malloc 
				(toalloc * sizeof (EWChanTriggerStruct))) == NULL)
			{
				logit ("", "Could not allocate %d trigger channels \n", toalloc);
				return EW_FAILURE;
			}

			memcpy (pChanTrigsNew, pChanTrigsOld, 
						(pCoinc->iNumAlloc * sizeof (EWChanTriggerStruct)));

			pCoinc->pChanTrigs = pChanTrigsNew;
			pCoinc->iNumAlloc = toalloc;

			free (pChanTrigsOld);
		}

		pChan = &pCoinc->pChanTrigs[pCoinc->iNumTrigs];


		if (sscanf (line, " %s %s %s x %s %s UTC    save: %s %s %s",
				pChan->Station.Sta, pChan->Station.Comp, pChan->Station.Net,
				YYYYMMDD, HHMMSS, YYYYMMDD_2, HHMMSS_2, SnipDurStr) != 8)
		{
			logit ("", "Call to sscanf failed \n");
			return EW_FAILURE;
		}

		t_atodbl (YYYYMMDD, HHMMSS, &pChan->Trigger.tTrigger);
		t_atodbl (YYYYMMDD_2, HHMMSS_2, &pChan->tSnippetStart);

		pChan->iSnippetDuration = atoi (SnipDurStr);

		pCoinc->iNumTrigs = pCoinc->iNumTrigs + 1;

	} /* while loop over channels */

	return EW_SUCCESS;

}   


/**************************************************************************
 *    getNextLine(msg, line) moves the next line from 'msg' into 'line    *
 *                           returns the number of characters in the line *
 *			     Returns negative if error.			  *
 **************************************************************************/
int getNextLine ( char** pNxtLine, char* line)
{
   int i;
   char* nxtLine;

   nxtLine=*pNxtLine;

   for (i =0; i< LINE_LEN; i++)
	{
	line[i] = *nxtLine++;
	if ( (int)line[i] == 0 )
		{
		return(-1); /*  Not good */
		}
	if (line[i] == '\n') goto normal;
	}
   logit("","getNextLine error: line too long \n");
   return(-1);

   normal:
   line[i+1]=0;
   *pNxtLine = nxtLine;
   return(i);
   
}
/* ----------------------------------- end of getNextLine() -------------------------------------- */


/**************************************************************************
 *    t_atodbl() takes date and time strings, and converts to double      *
 *               seconds since 1970. Note  that the time of day string    *
 *               is of the form "hh:mm:ss.ss"                             *
 *	         Returns negative if error.			          *
 **************************************************************************/


int t_atodbl(char* YYYYMMDD, char* HHMMSS, double* starttime) 
{
   char timestr[TIMESTR_LEN+1]; /* need space for the null-terminator */
   int ret;
   int tgtind, srcind;	/* indices for copying fields into timestr */

	/*************************  Y2K  ************************
	 *
	 * All instances of YYMMDD have been changed to YYYYMMDD
	 *
	 *************************  Y2K  ************************/

   /* we want a string of  the form yyyymmddhhmmss.ss */
   /*				    01234567890123456             */

   tgtind = 0;
   srcind = 0;

   /* Copy in the date */
   strcpy(timestr,YYYYMMDD);

   /* Append the hour */
   tgtind = tgtind + YMD_LEN;
   strncpy(&timestr[tgtind], HHMMSS, HOUR_LEN);

   /* Append the minute */
   tgtind = tgtind + HOUR_LEN;
   srcind = srcind + HOUR_LEN + 1; /** skip over the ':' in time field */
   strncpy(&timestr[tgtind] ,&HHMMSS[srcind], MIN_LEN); 

   /* Append the seconds */
   tgtind = tgtind + MIN_LEN;
   srcind = srcind + MIN_LEN + 1; /** skip over the ':' in time field */
   strncpy(&timestr[tgtind], &HHMMSS[srcind], SEC_LEN); 

   /* Null terminate the string */
   tgtind = tgtind + SEC_LEN;
   timestr[tgtind] = '\0';


   /* convert to double seconds */
   ret = epochsec17(starttime, timestr);
   if (ret < 0)
	{
	logit("t","bad return from epochsec17\n");
	return(-1);
	} 
   return(1);
}
/* ----------------------------------- end of t_atodbl() ----------------------------------------- */
