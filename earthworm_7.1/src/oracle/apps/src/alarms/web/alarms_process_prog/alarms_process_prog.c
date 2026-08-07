
/*
 *   This file is under RCS - do not modify unless you have
 *   checked it out using the command checkout.
 *
 *    $Id: alarms_process_prog.c,v 1.5 2002/05/28 19:15:59 lucky Exp $
 *
 *    Revision history:
 *    $Log: alarms_process_prog.c,v $
 *    Revision 1.5  2002/05/28 19:15:59  lucky
 *    Added Footer and Header text tags
 *
 *    Revision 1.4  2002/02/20 20:03:31  lucky
 *    Added web apperance options (bkg color, header and footer logo)
 *
 *    Revision 1.3  2001/07/28 00:49:59  lucky
 *    State of the code after debugging and testing prior to v6.0 release freeze.
 *
 *    Revision 1.2  2001/07/01 21:55:11  davidk
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
 *    Revision 1.1  2001/05/18 19:07:52  lucky
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

#define			NUM_INIT_CRITS		10 


typedef struct _WebOptionsStructReview
{
	int 				idCritProgram;
} WebOptionsStructAlarms;


/* function prototypes */
static int html_new_program();
static int html_existing_program(int idCritProgram);


int main()
{

	WebOptionsStructAlarms	Options;
	char				*configfile = "../params/alarms.d";


	/* Read the configuration file (path hardcoded relative to executable)
	 *********************************************************************/
	ReadConfig (configfile);

	logit_init ("alarms_process_prog", 1, 1024, 1);
	logit ("", "alarms_process_prog: starting.\n");

    /* Send header of reply back to web server */
    printf("Content-type: text/html\n\n");
    printf("<HTML>\n");

    printf ("<HEAD><TITLE>Program Manager</TITLE></HEAD>\n");
    
	html_header (BackgroundColor, HeaderLogo, HeaderTag);

    printf("<CENTER>\n");
    printf("<H2><FONT COLOR=blue>Earthworm Alarms: Program Manager</FONT></H2>\n");
    printf("</CENTER>\n");

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


	if (Options.idCritProgram == NEW_ENTRY_FLAG)
	{
		if (html_new_program () != EW_SUCCESS)
		{
			html_logit ("", "Call to html_new_program failed.\n");
			goto shutdown;
		}
	}
	else
	{
		if (html_existing_program (Options.idCritProgram) != EW_SUCCESS)
		{
			html_logit ("", "Call to html_existing_program failed.\n");
			goto shutdown;
		}
	}
	
shutdown:
	logit ("", "alarms_process_prog: terminating.\n");
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


  if (strcmp (szVar, "crit") == 0)
	{
		pOptions->idCritProgram = atoi (szVal);
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



/**************** html_new_program  **************/
static	int	html_new_program ()
{

	printf ("<BR><BR><CENTER>\n");
	printf ("<STRONG>Fill in information about the new program:</STRONG>\n");

	printf ("<FORM NAME=\"NewCritForm\" ACTION=\"alarms_insert_prog\" METHOD=POST>\n");
	printf ("<TABLE BORDER=0>\n");

	printf ("<INPUT TYPE=hidden NAME=idprog VALUE=%d>\n", NEW_ENTRY_FLAG);

	printf ("<TR><TD><FONT COLOR=\"olive\">Program Name</FONT></TD>\n");
	printf ("<TD><INPUT TYPE=text NAME=pname SIZE=32 MAXLENGTH=256></TD></TR>\n");

	printf ("<TR><TD><FONT COLOR=\"olive\">Working Directory</FONT></TD>\n");
	printf ("<TD><INPUT TYPE=text NAME=pdir SIZE=32 MAXLENGTH=256></TD></TR>\n");

	printf ("<TR><TD><FONT COLOR=\"olive\">Description</FONT></TD>\n");
	printf ("<TD><INPUT TYPE=text NAME=pdesc SIZE=32 MAXLENGTH=256></TD></TR>\n");

    /* Last row: submit button*/
    printf ("<TR ALIGN=center>\n");
    printf ("<TD COLSPAN=2><INPUT TYPE=\"submit\" VALUE=\"SUBMIT\" NAME=\"submit\"></TD>\n");
	printf ("</TR>\n");

	printf ("</CENTER></TABLE></FORM>\n");


	return EW_SUCCESS;
}

/**************** html_existing_program  **************/
static	int	html_existing_program (int idCritProgram)
{

	EWDB_AlarmsCritProgramStruct		*pCrit;
	int							NumRetr, NumFound;

	if (idCritProgram < 0)
	{
		html_logit ("", "Invalid arguments passed in.\n");
		return EW_FAILURE;
	}

	if ((pCrit = (EWDB_AlarmsCritProgramStruct *) malloc 
						(1 * sizeof (EWDB_AlarmsCritProgramStruct))) == NULL)
	{
		html_logit ("", "Could not malloc pCrit\n");
		return EW_FAILURE;
	}

	if (ewdb_api_GetAlarmsCriteriaList (idCritProgram, pCrit, &NumFound, &NumRetr, 1) 
																!= EWDB_RETURN_SUCCESS)
	{
		html_logit ("", "Call to ewdb_api_CreatePagerDelivery failed\n");
		return EW_FAILURE;
	}

	printf ("<BR><BR><CENTER>\n");
	printf ("<STRONG>Update information about the program:</STRONG>\n");

	printf ("<FORM NAME=\"NewCritForm\" ACTION=\"alarms_insert_prog\" METHOD=POST>\n");
	printf ("<TABLE BORDER=0>\n");

	printf ("<INPUT TYPE=hidden NAME=idprog VALUE=%d>\n", pCrit[0].idCritProgram);

	printf ("<TR><TD><FONT COLOR=\"olive\">Program Name</FONT></TD>\n");
	printf ("<TD><INPUT TYPE=text NAME=pname SIZE=32 MAXLENGTH=256 VALUE=\"%s\"></TD></TR>\n",
						pCrit[0].sProgName);

	printf ("<TR><TD><FONT COLOR=\"olive\">Working Directory</FONT></TD>\n");
	printf ("<TD><INPUT TYPE=text NAME=pdir SIZE=32 MAXLENGTH=256 VALUE=\"%s\"></TD></TR>\n",
						pCrit[0].sProgDir);

	printf ("<TR><TD><FONT COLOR=\"olive\">Description</FONT></TD>\n");
	printf ("<TD><INPUT TYPE=text NAME=pdesc SIZE=32 MAXLENGTH=256 VALUE=\"%s\"></TD></TR>\n",
						pCrit[0].sProgDescription);

    /* Last two rows: submit and delete buttons*/
    printf ("<TR ALIGN=center>\n");
    printf ("<TD COLSPAN=2><INPUT TYPE=\"submit\" VALUE=\"SUBMIT\" NAME=\"submit\"></TD>\n");
	printf ("</TR>\n");

    printf ("<TR ALIGN=center>\n");
    printf ("<TD COLSPAN=2><INPUT TYPE=\"submit\" VALUE=\"DELETE\" NAME=\"delete\"></TD>\n");
	printf ("</TR>\n");

	printf ("</CENTER></TABLE></FORM>\n");


	return EW_SUCCESS;

}
