
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: update_alarms_msg.c,v 1.1 2001/05/15 02:15:44 davidk Exp $
 *
 *    Revision history:
 *    $Log: update_alarms_msg.c,v $
 *    Revision 1.1  2001/05/15 02:15:44  davidk
 *    Initial revision
 *
 *
 *
 */
  

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

/* I forget what we grab out of unistd, but we grab something
   and to support multiple platforms this ugly ifdef is needed */
#ifndef _WINNT  /* DavidK 990119 */
# include <unistd.h> 
#endif

/* include earthworm headers */
#include <earthworm.h>
#include <kom.h>
#include <time_ew.h>  /* DavidK: done for perf. timing */
#include <ewdb_ora_api.h> 
#include <webparse.h>
#include <alarms.h>
#include <time_functions.h>
#include <map_display_structs.h>

#include "../include/review_function.h"

typedef struct _webstruct
{
	int		idEvent;
	int		AlarmNo;
	char	AlarmMsg[ALARM_MSG_SIZE];
} WebStruct;


main ()
{
	WebStruct		 			WebParams;
	char						*configfile = "../params/eqreview.d";
	char						filename[512];
	FILE						*fp;

	/* Initialize some stuff
	 ***********************/
	DEBUG = 0;

	/* Send html header back to web server
	 *************************************/
	printf("Content-type: text/html\n\n");
	printf("<html>\n");
	printf("<HEAD><TITLE>Message Review Confirmation</TITLE></HEAD>\n" 
				"<BODY BGCOLOR=#EEEEEE TEXT=#333333>\n" );


	/* Read the configuration file (path hardcoded relative to executable)
	 *********************************************************************/
	ReadConfig (configfile);

	/* Cleanup, logfile should match config file name */
	logit_init ("update_alarms_msg",1,10240,1);  


	/* initialize Web Parameters to all zeros */
	memset (&WebParams, 0, sizeof(WebOptionsStruct));

	if (Webparse_GetAndProcessWebParams((void *)(&WebParams)) != 0)
	{
		html_logit ("", "Call to Webparse_GetAndProcessWebParams failed.\n");
		html_break ();
		return EW_FAILURE;
	}

	/* Write the message out to the file */

	sprintf (filename, "%s/%d/AlarmsMsg_%d.bin",
			ReviewTmpDir, WebParams.idEvent, WebParams.AlarmNo);

	if ((fp = fopen (filename, "wb")) == NULL)
	{
		html_logit ("", "Can't open %s.\n", filename);
		return EW_FAILURE;
	}

	fwrite (WebParams.AlarmMsg, sizeof (char), ALARM_MSG_SIZE, fp);
	fclose (fp);

	printf ("<STRONG><CENTER><PRE>Alarm Message updated.</STRONG>\n\n");
	printf ("Close this window to continue with the review process.\n");


}  /* end main */


/*********************************************************************/
int Webparse_Client_SetVars(char * szVar, char * szVal, void * pUserParams)
{

	int			index;
	WebStruct *pOptions = (WebStruct *) pUserParams;
	

/*
logit ("", "szVar=%s, szVal=%s\n", szVar, szVal);
*/

    if(strcmp(szVar,"EventID") == 0)
    {
		pOptions->idEvent = atoi (szVal);
    }
    else if(strcmp(szVar,"AlarmNo") == 0)
    {
		pOptions->AlarmNo = atoi (szVal);
    }
    else if(strcmp(szVar,"alarmmsg") == 0)
    {
		strcpy (pOptions->AlarmMsg, szVal);
    }
	else
    if(strcmp(szVar,"Submit") == 0)
    {
		/* ignore */
	}
    else
    {
      html_logit ("" ,"Unrecognized Web Option : %s = %s\n",szVar,szVal);
    }
    return(0);
}  /* End of SetVars() */
