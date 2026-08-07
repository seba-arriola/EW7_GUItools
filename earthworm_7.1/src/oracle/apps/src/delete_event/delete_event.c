
/*
 *   This file is under RCS - do not modify unless you have
 *   checked it out using the command checkout.
 *
 *    $Id: delete_event.c,v 1.3 2002/05/28 19:34:19 lucky Exp $
 *
 *    Revision history:
 *    $Log: delete_event.c,v $
 *    Revision 1.3  2002/05/28 19:34:19  lucky
 *    Added header and footer tag text
 *
 *    Revision 1.2  2002/03/22 23:11:22  lucky
 *    Fixed include files to avoid warnings
 *
 *    Revision 1.1  2002/03/22 20:12:32  lucky
 *    Initial revision
 *
 *
 *
 */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <time_ew.h>
#include <earthworm.h>
#include <ew_event_info.h>
#include <time_functions.h>
#include <webparse.h>
#include <review.h>
#include <review_globals.h>

#ifdef _WINNT
#define DIR_SLASH   '\\'
#else
#define DIR_SLASH   '/'
#endif



typedef struct _WebOptionsStructReview
{
	int 				EventID;

} WebOptionsStructReview;

/* Configuration parameters for ora2sac */

int main()
{

	int					i, j, NumWaves;
	WebOptionsStructReview	Options;
	char				*configfile = "../params/review_event.d";
	char				cmd[1024];
	char				filename[1000];	
	char				evtdir[1000];	
	char				forwdir[1024];
	char				SacTgtDir[1000];	
	FILE				*fp;
    EWEventInfoStruct	EventInfo;



	/* Read the configuration file (path hardcoded relative to executable)
	 *********************************************************************/
	ReadConfig (configfile);

	logit_init ("delete_event", 1, MAX_BYTES_PER_EQ, 1);
	logit ("", "delete_event: starting.\n");

   printf("Content-type: text/html\n\n");
   printf("<html>\n");
   printf("<HEAD><TITLE>Delete Event</TITLE></HEAD>\n");

	html_header (BackgroundColor, HeaderLogo, HeaderTag);

	if (Webparse_GetAndProcessWebParams ((void *) (&Options)) == 1)
	{
		html_logit ("", "Call to GetAndProcessWebParams() failed\n");
		return EW_FAILURE;
	}

    /* Prepare the DB, cause we are a comin'! */
    if (ewdb_api_Init (DBuser, DBpassword, DBservice))
    {
        html_logit ("","DB Initialization failed! Exiting.\n");
        return(-1);
    }


	printf ("<CENTER><PRE>\n");

	if (ewdb_api_DeleteEvent (Options.EventID, 0) != EWDB_RETURN_SUCCESS)
	{
		printf ("<H2>Event %d could not be deleted</H2>\n", Options.EventID); 
	}
	else
		printf ("<H2>Event %d Deleted</H2>\n", Options.EventID); 

	printf ("Close this window to return to the list of events.\n");

	html_trailer (WebHost, FooterLogo, FooterTag);

	return EW_SUCCESS;

}  /* End of main() */




/******************************************************
  Purpose:     Webparse_Client_SetVars() is a callout 
               function provided by 
               the web_common (webparse) routines that parse
               web parameters passed during a CGI-BIN GET or
               POST request.  Webparse_Client_SetVars() is 
               called for each parameter - value pair
 **********************************************************/
int Webparse_Client_SetVars(char * szVar, char * szVal, void * pUserParams)
{

/* User Callout function SetVars, passes the user a Variable, it's string value,
   and a pointer to User Defined Params, so that the user can take whatever
   desired action they want in the SetVars() function. */

	WebOptionsStructReview *pOptions = (WebOptionsStructReview *) pUserParams;
	int				sta, chan;
	char			*tmpstr;
logit ("", "szVar=%s, szVal=%s\n", szVar, szVal);

	if (strcmp (szVar, "EventID") == 0)
	{
		pOptions->EventID = atoi (szVal);
	}
    else if (strcmp (szVar, "Done") == 0)
	{
		;
	}
	else
	{
		logit ("", "Unrecognized Web Option : %s = %s\n",szVar,szVal);
	}


	return (0);

}

