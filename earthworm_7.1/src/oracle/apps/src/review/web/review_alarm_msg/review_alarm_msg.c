
/*
 *   This file is under RCS - do not modify unless you have
 *   checked it out using the command checkout.
 *
 *    $Id: review_alarm_msg.c,v 1.6 2003/03/11 17:15:13 lucky Exp $
 *    Revision history:
 *
 */
  
/*** #includes ***/

/* include the normal stuff */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <earthworm.h>
#include <ewdb_ora_api.h>
#include <ewdb_apps_utils.h>
#include <webparse.h>
#include <alarms.h>

#include <review.h>

/* Web params structure */
typedef struct webopts
{
	int 	idEvent;
	int 	idOrigin;
	int 	AlarmNo;
	int 	isInsert;
} WebStruct;


int main(int argc, char * argv[])
{

	char		*configfile = "../params/review_event.d";
	WebStruct	WebParams;
	char		filename[512];
	char		AlarmMsg[ALARM_MSG_SIZE];
	FILE		*fp;


	/* Go get them config file params */
	if (ReadConfig (configfile))
	{
		fprintf(stdout,"Read of config file failed. Filename: %s\n",configfile);
		return (-1);
	}

	/* Initialize logging */
	logit_init ("review_alarm_msg", 1, MAX_BYTES_PER_EQ, 1);
 
	/* Send header of reply back to web server */
	printf("Content-type: text/html\n\n");
	printf("<html>\n");
	printf("<HEAD><TITLE>Review Message</TITLE></HEAD>\n");

	html_header (BackgroundColor, HeaderLogo, HeaderTag);

    printf("<center>\n");
    printf("<H2><FONT COLOR=red>Review Alarm Message</FONT></H2>\n");
    printf("</center>\n");


	/* Get the web params from the web client */
	/* initialize Web Parameters to all zeros */
	memset (&WebParams, 0, sizeof(WebOptionsStruct));
	if (Webparse_GetAndProcessWebParams((void *)(&WebParams)) != 0)
	{
		html_logit ("", "Call to Webparse_GetAndProcessWebParams failed.\n");
		return (-1);
	}


	/* open and read the message file */
	sprintf (filename, "%s%c%d%cAlarmsMsg_%d.bin", 
					ReviewTmpDir, DIR_SLASH,
					WebParams.idOrigin, DIR_SLASH, WebParams.AlarmNo);

	if ((fp = fopen (filename, "rb")) == NULL)
	{
		html_logit ("", "Can't open %s\n", filename);
		return (-1);
	}

	fread (AlarmMsg, sizeof (char), ALARM_MSG_SIZE, fp);
	fclose (fp);

	/* put up the review form */

	printf ("<CENTER><STRONG>\n");
	printf ("<PRE>\n");
	printf ("Review or modify the message to be sent.\n\n");

	printf ("Click <I>Update All Messages</I> to apply modifications to all "
			"notifications of this format.\n");
	printf ("Click <I>Update Current Only</I> to apply modifications only to "
			"current notification message.\n");
	printf ("</PRE></STRONG>\n");

	printf ("<BR><BR>\n");

	printf ("<FORM NAME=\"AlReviewFm\" ACTION=\"review_update_alarms_msg\" METHOD=POST>\n");

	printf ("<TABLE border=2 cellspacing=2>\n");

	printf ("<INPUT TYPE=hidden NAME=\"EventID\" VALUE=%d>", WebParams.idEvent);
	printf ("<INPUT TYPE=hidden NAME=\"OriginID\" VALUE=%d>", WebParams.idOrigin);
	printf ("<INPUT TYPE=hidden NAME=\"AlarmNo\" VALUE=%d>", WebParams.AlarmNo);
	printf ("<INPUT TYPE=hidden NAME=\"isInsert\" VALUE=%d>", WebParams.isInsert);

	printf ("<TR><TD colspan=2>");
	printf ("<TEXTAREA NAME=alarmmsg WRAP=virtual ROWS=10 COLS=60>%s", AlarmMsg);
	printf ("</TEXTAREA></TD></TR>\n");

	printf ("<TR><TD ALIGN=center colspan=1> "
			"<INPUT TYPE=\"submit\" VALUE=\"Update All Messages\" NAME=\"All\"></TD>\n");

	printf ("<TD ALIGN=center colspan=1> "
			"<INPUT TYPE=\"submit\" VALUE=\"Update Current Only\" NAME=\"Current\"></TD></TR>\n");

	printf ("</TABLE></FORM>\n");

  /* success */

	html_trailer (WebHost, FooterLogo, FooterTag);
  return(0);
}


/*******************************************************************/
int Webparse_Client_SetVars(char * szVar, char * szVal, void * pUserParams)
{

/* User Callout function SetVars, passes the user a Variable, it's string value,
   and a pointer to User Defined Params, so that the user can take whatever
   desired action they want in the SetVars() function. */

  WebStruct * pOptions= (WebStruct *) pUserParams;

/*
logit ("", "szVar=%s, szVal=%s\n", szVar, szVal);
*/

    if(strcmp(szVar,"EventID") == 0)
    {
      pOptions->idEvent = atoi(szVal);
    }
    else if(strcmp(szVar,"OriginID") == 0)
    {
      pOptions->idOrigin = atoi(szVal);
    }
    else if(strcmp(szVar,"AlarmNo") == 0)
    {
      pOptions->AlarmNo = atoi(szVal);
    }
    else if(strcmp(szVar,"isInsert") == 0)
    {
      pOptions->isInsert = atoi(szVal);
    }
	else
    {
      html_logit ("" ,"Unrecognized Web Option : %s = %s\n",szVar,szVal);
    }
    return(0);
}  /* End of SetVars() */
