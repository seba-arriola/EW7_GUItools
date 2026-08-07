
/*
 *   This file is under RCS - do not modify unless you have
 *   checked it out using the command checkout.
 *
 *    $Id: review_show_alarm_msg.c,v 1.4 2002/05/28 19:34:48 lucky Exp $
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
	char	PhoneNo[256];
	char	Recipient[256];
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
	logit_init ("review_show_alarm_msg", 1, MAX_BYTES_PER_EQ, 1);
 
	/* Send header of reply back to web server */
	printf("Content-type: text/html\n\n");
	printf("<html>\n");
	printf("<HEAD><TITLE>Phone Alarm Message</TITLE></HEAD>\n");

	html_header (BackgroundColor, HeaderLogo, HeaderTag);

    printf("<center>\n");
    printf("<H2><FONT COLOR=red>Phone Alarm Message</FONT></H2>\n");
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

	printf ("<CENTER><STRONG><PRE>\n");
	printf ("Deliver the following telephone alarm message: \n");
	printf ("Recipient:  %s\n", WebParams.Recipient);
	printf ("Phone Number:  %s\n\n\n", WebParams.PhoneNo);

	printf ("<HR></STRONG></CENTER>%s\n", AlarmMsg);

	printf ("<BR><HR><BR></PRE>\n");

  /* success */
  return(0);
}


/*******************************************************************/
int Webparse_Client_SetVars(char * szVar, char * szVal, void * pUserParams)
{

/* User Callout function SetVars, passes the user a Variable, it's string value,
   and a pointer to User Defined Params, so that the user can take whatever
   desired action they want in the SetVars() function. */

	int				i;
	WebStruct * pOptions= (WebStruct *) pUserParams;

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
    else if(strcmp(szVar,"PhoneNo") == 0)
    {
      strcpy (pOptions->PhoneNo, szVal);
	}
    else if(strcmp(szVar,"Recip") == 0)
    {
		/* Remove ~ chars and replace by spaces */
		for (i = 0; i < strlen (szVal); i++)
		{
			if (szVal[i] == '~')
				pOptions->Recipient[i] = ' ';
			else
				pOptions->Recipient[i] = szVal[i];
		}
		
	}
	else
    {
      html_logit ("" ,"Unrecognized Web Option : %s = %s\n",szVar,szVal);
    }
    return(0);
}  /* End of SetVars() */
