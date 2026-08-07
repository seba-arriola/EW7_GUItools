
/*
 *   This file is under RCS - do not modify unless you have
 *   checked it out using the command checkout.
 *
 *    $Id: alarms_process_format.c,v 1.5 2002/05/28 19:15:59 lucky Exp $
 *
 *    Revision history:
 *    $Log: alarms_process_format.c,v $
 *    Revision 1.5  2002/05/28 19:15:59  lucky
 *    Added Footer and Header text tags
 *
 *    Revision 1.4  2002/02/20 20:03:31  lucky
 *    Added web apperance options (bkg color, header and footer logo)
 *
 *    Revision 1.3  2001/07/28 00:49:59  lucky
 *    State of the code after debugging and testing prior to v6.0 release freeze.
 *
 *    Revision 1.2  2001/07/01 21:55:10  davidk
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
 *    Revision 1.1  2001/05/18 19:07:50  lucky
 *    Initial revision
 *
 *    Revision 1.1  2001/05/15 02:15:03  davidk
 *    Initial revision
 *
 *
 */


#include <alarms.h>
#include <webparse.h>
#include <html_common.h>

#define			NUM_INIT_FORMATS		10 


typedef struct _WebOptionsStructReview
{
	int 				idFormat;
} WebOptionsStructAlarms;

static	int		PreviewFlag;



/* prototypes for funcs in this file */
static int	html_preview_format(int idFormat);
static int	html_existing_format(int idFormat);
static int	html_new_format();
static int	DoFormatInColor(char *);

int main()
{

	WebOptionsStructAlarms	Options;
	char				*configfile = "../params/alarms.d";


	/* Read the configuration file (path hardcoded relative to executable)
	 *********************************************************************/
	ReadConfig (configfile);

	logit_init ("alarms_process_format", 1, 1024, 1);
	logit ("", "alarms_process_format: starting.\n");

    /* Send header of reply back to web server */
    printf("Content-type: text/html\n\n");
    printf("<HTML>\n");
    printf ("<HEAD><TITLE>Format Manager</TITLE></HEAD>\n");

	html_header (BackgroundColor, HeaderLogo, HeaderTag);

    printf("<CENTER>\n");
    printf("<H2><FONT COLOR=blue>Earthworm Alarms: Format Manager</FONT></H2>\n");
    printf("</CENTER>\n");

	PreviewFlag = FALSE;
	if (Webparse_GetAndProcessWebParams ((void *) (&Options)) == 1)
	{
		html_logit ("", "Call to GetAndProcessWebParams() failed\n");
		goto shutdown;
	}

    /* Connect to the DB */
    if (ewdb_api_Init (DBuser, DBpassword, DBservice) != 0)
    {
        html_logit( "", "Trouble connecting to database; exiting!\n" );
    	goto shutdown;
    }


	if (Options.idFormat == NEW_ENTRY_FLAG)
	{
		if (html_new_format () != EW_SUCCESS)
		{
			html_logit ("", "Call to html_new_format failed.\n");
			goto shutdown;
		}
	}
	else
	{
		/* 
		 * If PreviewFlag is set, we show the user what the
		 * format looks like in different colors.
		 */
		if (PreviewFlag == TRUE)
		{
			if (html_preview_format (Options.idFormat) != EW_SUCCESS)
			{
				html_logit ("", "Call to html_preview_format failed.\n");
				return EW_FAILURE;
			}
		}
		else
		{
			if (html_existing_format (Options.idFormat) != EW_SUCCESS)
			{
				html_logit ("", "Call to html_existing_format failed.\n");
				goto shutdown;
			}
		}
	}
	
shutdown:
	logit ("", "alarms_process_format: terminating.\n");
    ewdb_api_Shutdown();
	html_trailer (WebHost, FooterLogo, FooterTag);

	return EW_SUCCESS;

}  



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

	WebOptionsStructAlarms *pOptions = (WebOptionsStructAlarms *) pUserParams;


    if (strcmp (szVar, "format") == 0)
	{
		pOptions->idFormat = atoi (szVal);
	}
    else if (strcmp (szVar, "preview") == 0)
	{
		PreviewFlag = TRUE;
	}
    else if (strcmp (szVar, "submit") == 0)
	{
		;		/* ignore */
	}
	else
	{
		logit ("", "Unrecognized Web Option : %s = %s\n",szVar,szVal);
	}

	return (0);
}



/**************** html_new_format  **************/
static	int	html_new_format ()
{

	printf ("<BR><CENTER>\n");

	printf ("<A HREF=\"../format_help.html\" TARGET=\"Format Help\">\n");
	printf ("Click here for help.</A><BR><BR><BR>\n");

	printf ("<STRONG>Fill in information about the new format:</STRONG>\n");

	printf ("<FORM NAME=\"NewFormatForm\" ACTION=\"alarms_insert_format\" METHOD=POST>\n");
	printf ("<TABLE BORDER=0>\n");

	printf ("<INPUT TYPE=hidden NAME=idform VALUE=%d>\n", NEW_ENTRY_FLAG);

	printf ("<TR><TD><FONT COLOR=\"olive\">Format Description</FONT></TD>\n");
	printf ("<TD><INPUT TYPE=text NAME=fdescr SIZE=20 MAXLENGTH=32></TD></TR>\n");

	printf ("<TR><TD><FONT COLOR=\"olive\">Insertion Format</FONT></TD>\n");
	printf ("<TD><TEXTAREA NAME=finsert WRAP=virtual ROWS=10 COLS=60></TEXTAREA></TD></TR>\n");

	printf ("<TR><TD><FONT COLOR=\"olive\">Deletion Format</FONT></TD>\n");
	printf ("<TD><TEXTAREA NAME=fdelete WRAP=virtual ROWS=10 COLS=60></TEXTAREA></TD></TR>\n");

    /* Last row: submit button*/
    printf ("<TR ALIGN=center>\n");
    printf ("<TD COLSPAN=2><INPUT TYPE=\"submit\" VALUE=\"SUBMIT\" NAME=\"submit\"></TD>\n");
	printf ("</TR>\n");

	printf ("</CENTER></TABLE></FORM>\n");


	return EW_SUCCESS;
}

/**************** html_existing_format  **************/
static	int	html_existing_format (int idFormat)
{

	EWDB_AlarmsFormatStruct			*pFormat;
	int							NumRetr, NumFound;

	if (idFormat < 0)
	{
		html_logit ("", "Invalid arguments passed in.\n");
		return EW_FAILURE;
	}

	if ((pFormat = (EWDB_AlarmsFormatStruct *) malloc 
						(1 * sizeof (EWDB_AlarmsFormatStruct))) == NULL)
	{
		html_logit ("", "Could not malloc pFormat\n");
		return EW_FAILURE;
	}

	if (ewdb_api_GetAlarmsFormats (idFormat, pFormat, &NumFound, &NumRetr, 1) 
																!= EWDB_RETURN_SUCCESS)
	{
		html_logit ("", "Call to ewdb_api_GetAlarmsFormats failed\n");
		return EW_FAILURE;
	}

	if (NumRetr <= 0)
	{
		html_logit ("", "ewdb_api_GetAlarmsFormats returned no Formats.\n");
		return EW_FAILURE;
	}

	printf ("<BR><CENTER>\n");


	printf ("<A HREF=\"../format_help.html\" TARGET=\"Format Help\">\n");
	printf ("Click here for help.</A><BR><BR><BR>\n");


	printf ("<STRONG>Update information about the format:</STRONG>\n");

	printf ("<FORM NAME=\"NewCritForm\" ACTION=\"alarms_insert_format\" METHOD=POST>\n");
	printf ("<TABLE BORDER=0>\n");

	printf ("<INPUT TYPE=hidden NAME=idform VALUE=%d>\n", pFormat[0].idFormat);

	printf ("<TR><TD><FONT COLOR=\"olive\">Format Description</FONT></TD>\n");
	printf ("<TD><INPUT TYPE=text NAME=fdescr SIZE=32 MAXLENGTH=256 VALUE=\"%s\"></TD></TR>\n",
						pFormat[0].sDescription);

	printf ("<TR><TD><FONT COLOR=\"olive\">Insertion Format</FONT></TD>\n");
	printf ("<TD><TEXTAREA NAME=finsert WRAP=virtual ROWS=10 COLS=60>%s"
					"</TEXTAREA></TD></TR>\n", pFormat[0].sFmtInsert);

	printf ("<TR><TD><FONT COLOR=\"olive\">Deletion Format</FONT></TD>\n");
	printf ("<TD><TEXTAREA NAME=fdelete WRAP=virtual ROWS=10 COLS=60>%s"
					"</TEXTAREA></TD></TR>\n", pFormat[0].sFmtDelete);

    /* Last two rows:  submit and delete buttons*/
    printf ("<TR ALIGN=center>\n");
    printf ("<TD COLSPAN=2><INPUT TYPE=\"submit\" VALUE=\"SUBMIT\" NAME=\"submit\"></TD>\n");
	printf ("</TR>\n");

    printf ("<TR ALIGN=center>\n");
    printf ("<TD COLSPAN=2><INPUT TYPE=\"submit\" VALUE=\"DELETE\" NAME=\"delete\"></TD>\n");
	printf ("</TR>\n");

	printf ("</CENTER></TABLE></FORM>\n");


	return EW_SUCCESS;

}


/**************** html_preview_format  **************/
static	int	html_preview_format (int idFormat)
{

	EWDB_AlarmsFormatStruct			*pFormat;
	int							NumRetr, NumFound;

	if (idFormat < 0)
	{
		html_logit ("", "Invalid arguments passed in.\n");
		return EW_FAILURE;
	}

	if ((pFormat = (EWDB_AlarmsFormatStruct *) malloc 
						(1 * sizeof (EWDB_AlarmsFormatStruct))) == NULL)
	{
		html_logit ("", "Could not malloc pFormat\n");
		return EW_FAILURE;
	}

	if (ewdb_api_GetAlarmsFormats (idFormat, pFormat, &NumFound, &NumRetr, 1) 
																!= EWDB_RETURN_SUCCESS)
	{
		html_logit ("", "Call to ewdb_api_GetAlarmsFormats failed\n");
		return EW_FAILURE;
	}

	if (NumRetr <= 0)
	{
		html_logit ("", "ewdb_api_GetAlarmsFormats returned no Formats.\n");
		return EW_FAILURE;
	}

	printf ("<PRE><CENTER>\n");
	printf ("<STRONG>Preview Format: %s.</STRONG>\n\n", pFormat[0].sDescription);

	printf ("<HR>Insertion Format:\n\n</CENTER>");
	DoFormatInColor (pFormat[0].sFmtInsert);


	printf ("<CENTER><HR>Deletion Format:\n\n</CENTER>");
	DoFormatInColor (pFormat[0].sFmtDelete);

	printf ("<HR>\n");

    printf ("\n\n<CENTER><BR><BR><A HREF=\"alarms_manager\">");
    printf ("Click here to return to the Alarms Manager.</A></PRE></CENTER>\n");


	return EW_SUCCESS;
}



/**************** DoFormatInColor  **************/
static	int	DoFormatInColor (char *fmt)
{

	char 	*tmp1;
	char	*tmp2;
	char	token[256];
	int		done;

	if (fmt == NULL)
	{
		html_logit ("", "Invalid arguments passed in.\n");
		return EW_FAILURE;
	}

	done = FALSE;
	tmp1 = fmt;
	while (done == FALSE)
	{
		if ((tmp2 = strchr (tmp1, FMT_CHAR)) == NULL)
		{
			html_logit ("", "Invalid format! Did you forget the ~End~ token?\n");
			return EW_FAILURE;
		}
		*tmp2 = '\0';
		printf ("%s", tmp1);

		/* Get the token */
		*tmp2 = FMT_CHAR;
		tmp1 = tmp2 + 1;
		
		if ((tmp2 = strchr (tmp1, FMT_CHAR)) == NULL)
		{
			html_logit ("", "Invalid format! Make sure to surround tokens with '~'?\n");
			return EW_FAILURE;
		}
		*tmp2 = '\0';
		strcpy (token, tmp1);
		*tmp2 = FMT_CHAR;

		printf ("<FONT COLOR=\"#FF0000\">%s</FONT>", token);

		/* are we done? */
		if (strcmp (token, "End") == 0)
			done = TRUE;
		else
			tmp1 = tmp2 + 1;
	}

	return EW_SUCCESS;
}
