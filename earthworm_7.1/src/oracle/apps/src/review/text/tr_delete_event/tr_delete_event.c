
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: tr_delete_event.c,v 1.1 2004/03/17 18:55:16 davidk Exp $
 *
 *    Revision history:
 *     $Log: tr_delete_event.c,v $
 *     Revision 1.1  2004/03/17 18:55:16  davidk
 *     Initial revision
 *
 *     Revision 1.8  2002/03/21 20:47:44  lucky
 *     fixed array sizes
 *
 *     Revision 1.7  2001/07/01 21:55:28  davidk
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
 *     Revision 1.6  2001/05/15 02:15:37  davidk
 *     Moved functions around between the apps, DB API, and DB API INTERNAL
 *     levels.  Renamed functions and files.  Added support for amplitude
 *     magnitude types.  Reformatted makefiles.
 *
 *     Revision 1.5  2001/02/28 17:33:50  lucky
 *     *** empty log message ***
 *
 *     Revision 1.4  2000/10/11 18:19:17  lucky
 *     Removed call to InitEWEvent -- it is called inside of ewdb_apps_GetDBEventInfo
 *
 *     Revision 1.3  2000/09/18 17:22:55  lucky
 *     Final version before v5.1
 *
 *     Revision 1.2  2000/08/09 17:01:29  lucky
 *     Lint Cleanup
 *
 *     Revision 1.1  2000/08/07 19:44:03  lucky
 *     Initial revision
 *
 *
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include <ewdb_ora_api.h>
#include <ew_event_info.h>
#include <text_review_config.h>

extern int errno;   /* system error variable */


#define NCOMMAND  4          /* # of required commands you expect to process */
#define APP_MAXPATH 480
#define APP_MAXWORD 50

char DBuser[APP_MAXWORD];
char DBservice[APP_MAXWORD];
char DBpassword[APP_MAXWORD];
char envEW_LOG[512];
char ArcFileName[APP_MAXPATH];

int     Debug;



main (int argc, char **argv)
{

	int					eventid;                /* current eventid we're processing  */


   /* Introduce ourselves
   **********************/
   if (argc != 3)
   {
		fprintf (stderr, "delete_event deletes an event from the DBMS\n");
		fprintf (stderr, "Usage:  delete_event <config file> <event ID>\n");
		exit (0);
   }

    ReadConfig (argv[1]); /* it exits if it isn't happy */

	logit_init ("delete_event", 1, 1024, 1);
   
	/* Pick up command line arguments
	*********************************/
	if ((eventid = atoi (argv[2])) < 0)
	{
		logit ("e", "Invalid Event ID: %s\n", argv[1]);
		exit (0);
	}



	/* Open connection to database
	*****************************/
	if( ewdb_api_Init (DBuser, DBpassword, DBservice) != 0 )
	{
		logit ("e", "Trouble connecting to database!\n" );
		goto shutdown;
	}

	logit ("et", "Connected!\n");

#ifdef FOO
	/* First make sure that the event exists */
    if (ewdb_apps_GetDBEventInfo (&EventInfo, eventid) != EWDB_RETURN_SUCCESS)
    {
        logit ("", "ewdb_apps_DB2ArcFile: Call to ewdb_apps_GetDBEventInfo failed\n");
        goto shutdown;
    }

	if (eventid != EventInfo.Event.idEvent)
	{
		logit ("e", "Event %d does not exist.\n", eventid);
		goto shutdown;
	}

	/* Free up pChanInfo space */
	free (EventInfo.pChanInfo);
#endif FOO

	logit ("et", "Deleting Event ID: %d\n", eventid);

	if (ewdb_api_DeleteEvent (eventid, 0) != EWDB_RETURN_SUCCESS)
	{
		logit ("e", "delete_event: Call to ewdb_api_DeleteEvent failed.\n");
		goto shutdown;
	}

	logit ("et", "Event %d successfully deleted.\n", eventid);

shutdown:
	ewdb_api_Shutdown();
	logit ("e", "delete_event: terminating\n" );

	return( 0 );

}  /* end main */



