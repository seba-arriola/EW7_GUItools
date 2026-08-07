
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: review_event.c,v 1.2 2001/07/01 21:55:31 davidk Exp $
 *
 *    Revision history:
 *    $Log: review_event.c,v $
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
  
/*****************************************************************

 *  Given an eventid, retrieves the hypoinverse arc file from 
 *  the database. It then reads the file and fills the DB structs 
 *  with the event info. 
 *  Then, puts up an html form that allows the user to 
 *  review and modify the event parameters. 
 * 

*****************************************************************/


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>


/* include earthworm headers */
#include <earthworm.h>
#include <webparse.h>

/* include our own header file */
#include <review.h>

main ()
{

	WebOptionsStruct 			WebParams;
	char						*configfile = "../params/review_event.d";

	/* Read the configuration file (path hardcoded relative to executable)
	 *********************************************************************/
	ReadConfig (configfile);

    logit_init ("review_event", 1, MAX_BYTES_PER_EQ, 1);
	logit ("t", "review_event starting.\n"); 

	/* initialize Web Parameters to all zeros */
	memset (&WebParams, 0, sizeof(WebOptionsStruct));
	if (Webparse_GetAndProcessWebParams((void *)(&WebParams)) != 0)
	{
		printf ("Call to Webparse_GetAndProcessWebParams failed.\n");
		exit (-1);
	}

	ReviewFunction (WebParams.idEvent, WebParams.Action, WebParams.ShowMap);

	exit (0);

}
