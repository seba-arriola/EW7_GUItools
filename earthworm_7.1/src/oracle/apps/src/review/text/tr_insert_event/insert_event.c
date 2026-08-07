
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: insert_event.c,v 1.7 2002/03/21 20:48:38 lucky Exp $
 *
 *    Revision history:
 *     $Log: insert_event.c,v $
 *     Revision 1.7  2002/03/21 20:48:38  lucky
 *     fixed array sizes
 *
 *     Revision 1.6  2001/07/01 21:55:30  davidk
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
 *     Revision 1.5  2001/05/15 02:15:39  davidk
 *     Moved functions around between the apps, DB API, and DB API INTERNAL
 *     levels.  Renamed functions and files.  Added support for amplitude
 *     magnitude types.  Reformatted makefiles.
 *
 *     Revision 1.4  2001/02/28 17:40:17  lucky
 *     Massive schema redesign and cleanup.
 *
 *     Revision 1.3  2000/09/18 17:23:44  lucky
 *     Final version before v5.1
 *
 *     Revision 1.2  2000/08/09 17:01:29  lucky
 *     Lint Cleanup
 *
 *     Revision 1.1  2000/08/07 19:45:25  lucky
 *     Initial revision
 *
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ewdb_ora_api.h>
#include <ewdb_apps_utils.h>
#include <text_review_config.h>


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

   /* Introduce ourselves
   **********************/
   if (argc != 2)
   {
		fprintf (stderr, "insert_event inserts an event into the DBMS\n");
		fprintf (stderr, "Usage:  insert_event <config file>\n");
		exit (0);
   }

    ReadConfig (argv[1]); /* it exits if it isn't happy */

	logit_init ("insert_event", 1, 1024, 1);
   
	logit ("e", "Inserting Event from file %s.\n", ArcFileName);

	/* Open connection to database
	*****************************/
	if( ewdb_api_Init (DBuser, DBpassword, DBservice) != 0 )
	{
		logit ("e", "Trouble connecting to database!\n" );
		goto shutdown;
	}

	if (ewdb_apps_ArcFile2DB (ArcFileName, "Reviewed", NULL) != EW_SUCCESS)
	{
		logit ("e", "insert_event: Call to ewdb_apps_ArcFile2DB failed.\n");
		goto shutdown;
	}

	logit ("e", "Event successfully inserted.\n");

shutdown:
	ewdb_api_Shutdown();
	logit ("e", "insert_event: terminating\n" );

	return( 0 );

}  /* end main */



