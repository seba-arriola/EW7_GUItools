
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: review_confirm.c,v 1.7 2002/05/28 19:34:48 lucky Exp $
 *
 *    Revision history:
 *    $Log: review_confirm.c,v $
 *    Revision 1.7  2002/05/28 19:34:48  lucky
 *    Added header and footer tag text
 *
 *    Revision 1.6  2002/05/28 17:27:23  lucky
 *    *** empty log message ***
 *
 *    Revision 1.5  2002/03/22 20:03:33  lucky
 *    Pre v6.1 checkin
 *
 *    Revision 1.4  2001/07/28 00:45:23  lucky
 *     State of the code after debugging and testing prior to v6.0 release freeze.
 *
 *    Revision 1.3  2001/07/01 22:31:42  davidk
 *    removed C++ style comment that was making trouble.
 *
 *    Revision 1.2  2001/07/01 21:55:31  davidk
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
 *    Revision 1.1  2001/06/21 21:26:25  lucky
 *    Initial revision
 *
 *
 */
  

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

/* include earthworm headers */
#include <earthworm.h>
#include <kom.h>
#include <time_ew.h>  /* DavidK: done for perf. timing */
#include <webparse.h>
#include <time_functions.h>
#include <map_display_structs.h>

#include <review.h>


#define     ARC_MSG_LEN         100000

void		html_delete (WebOptionsStruct *);
void		html_insert (WebOptionsStruct *);


main ()
{
	WebOptionsStruct 			WebParams;
	char						*configfile = "../params/review_event.d"; 
	

	/* Initialize some stuff
	 ***********************/
	DEBUG = 0;

	/* Read the configuration file (path hardcoded relative to executable)
	 *********************************************************************/
	ReadConfig (configfile);

	/* Cleanup, logfile should match config file name */
	logit_init ("review_confirm",1,MAX_BYTES_PER_EQ,1);  

	logit ("", "review_confirm: starting.\n");

	printf("Content-type: text/html\n\n");
	printf("<html>\n");
	printf("<HEAD><TITLE>Confirm Next Review Action</TITLE></HEAD>\n");

	html_header (BackgroundColor, HeaderLogo, HeaderTag);

	printf("</center>\n");


	/* initialize Web Parameters to all zeros */
	memset (&WebParams, 0, sizeof(WebOptionsStruct));
	if (Webparse_GetAndProcessWebParams((void *)(&WebParams)) != 0)
	{
		html_logit ("", "Call to Webparse_GetAndProcessWebParams failed.\n");
		html_break ();
		goto shutdown;
	}

	/* If Delete flag is set, we delete the event from the DBMS */
	if (WebParams.Action == ACT_DELETE)
	{
		html_delete (&WebParams);
	}


	/* If Insert flag is set, we insert the event into the DBMS */
	else if (WebParams.Action == ACT_INSERT)
	{
		html_insert (&WebParams);
	}
	else
	{
		html_logit ("", "Unknown Action %d\n", WebParams.Action);
		html_break ();
		goto shutdown;
	}


shutdown:
	html_trailer (WebHost, FooterLogo, FooterTag);
	logit("","review_confirm: terminating\n" );

	return (0);

}  /* end main */



/***********************************************************
       Write html to prompt the user to confirm the deletion
 ***************************************************************/
void		html_delete (WebOptionsStruct *pOptions)
{


	printf ("<CENTER><PRE>\n");
	printf ("<br><br>\n");
	printf ("This action will permanently delete event %u from the database.\n\n",
									pOptions->idEvent);
	printf ("Are you sure? \n\nClick on CANCEL to go back, \n"
				"DELETE WITHOUT ALARMS to bypass the alarms and delete the event,\n"
				"or DELETE WITH ALARMS to review alarms before deleting the event.\n");

	printf ("<FORM NAME=\"ButtonForm\" ACTION=\"NULL\" METHOD=POST>\n");
	printf ("<TABLE>\n");

	printf ("<TR><TD COLSPAN=2 ALIGN=center>\n");
	printf ("<INPUT TYPE=\"BUTTON\" VALUE=\"CANCEL\" "
			"OnClick=\"window.location.href='review_event?EventID=%u?"
			"OriginID=%u?Action=%d'; return true;\">\n",
						pOptions->idEvent, pOptions->idOrigin, ACT_NULL);
	printf ("</TD></TR>\n");

	printf ("<TR><TD>\n");
	printf ("<INPUT TYPE=\"BUTTON\" VALUE=\"DELETE WITHOUT ALARMS\" "
			"OnClick=\"window.location.href='review_process_event?EventID=%u?"
			"OriginID=%u?Action=%d?Type=%d'; return true;\">\n",
						pOptions->idEvent, pOptions->idOrigin, ACT_ALARMS_NO, ACT_DELETE);
	printf ("</TD>\n");

	printf ("<TD>\n");
	printf ("<INPUT TYPE=\"BUTTON\" VALUE=\"DELETE WITH ALARMS\" "
			"OnClick=\"window.location.href='review_process_alarms?EventID=%u?"
			"OriginID=%u?Type=%d'; return true;\">\n",
							pOptions->idEvent, pOptions->idOrigin, ACT_DELETE);
	printf ("</TD></TR>\n");

    printf ("</TABLE>\n");
    printf ("</FORM>\n");

	printf ("</CENTER>\n");

}

/***********************************************************
       Write html to prompt the user to confirm the insertion
 ***************************************************************/
void		html_insert (WebOptionsStruct *pOptions)
{

	printf ("<CENTER><PRE>\n");
	printf ("<br><br>\n");
	printf ("This action will update event %u in the database.\n\n",
									pOptions->idEvent);
	printf ("Are you sure? \n\nClick on CANCEL to go back, \n"
				"INSERT WITHOUT ALARMS to bypass the alarms and insert the event, \n"
				"or INSERT WITH ALARMS to review alarms before inserting the event.\n");

	printf ("<FORM NAME=\"ButtonForm\" ACTION=\"NULL\" METHOD=POST>\n");
	printf ("<TABLE>\n");

	printf ("<TR><TD COLSPAN=2 ALIGN=center>\n");
	printf ("<INPUT TYPE=\"BUTTON\" VALUE=\"CANCEL\" "
			"OnClick=\"window.location.href='review_event?EventID=%u?"
			"OriginID=%u?Action=%d'; return true;\">\n",
							pOptions->idEvent, pOptions->idOrigin, ACT_NULL);
	printf ("</TD></TR>\n");


	printf ("<TR><TD>\n");
	printf ("<INPUT TYPE=\"BUTTON\" VALUE=\"INSERT WITHOUT ALARMS\" "
			"OnClick=\"window.location.href='review_process_event?EventID=%u?"
			"OriginID=%u?Action=%d\?Type=%d'; return true;\">\n",
						pOptions->idEvent, pOptions->idOrigin, ACT_ALARMS_NO, ACT_INSERT);
	printf ("</TD>\n");

	printf ("<TD>\n");
	printf ("<INPUT TYPE=\"BUTTON\" VALUE=\"INSERT WITH ALARMS\" "
			"OnClick=\"window.location.href='review_process_alarms?EventID=%u?"
			"OriginID=%u?Type=%d'; return true;\">\n",
						pOptions->idEvent, pOptions->idOrigin, ACT_INSERT);
	printf ("</TD></TR>\n");

    printf ("</TR>\n");
    printf ("</TABLE>\n");
    printf ("</FORM>\n");


	printf ("</CENTER>\n");

}
