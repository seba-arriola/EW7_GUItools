
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: db2file.c,v 1.7 2002/03/21 20:47:24 lucky Exp $
 *
 *    Revision history:
 *     $Log: db2file.c,v $
 *     Revision 1.7  2002/03/21 20:47:24  lucky
 *     fixed array sizes
 *
 *     Revision 1.6  2001/07/01 21:55:28  davidk
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
 *     Revision 1.5  2001/05/15 02:15:37  davidk
 *     Moved functions around between the apps, DB API, and DB API INTERNAL
 *     levels.  Renamed functions and files.  Added support for amplitude
 *     magnitude types.  Reformatted makefiles.
 *
 *     Revision 1.4  2001/02/28 17:39:27  lucky
 *     Massive schema redesign and cleanup.
 *
 *     Revision 1.3  2000/09/18 17:22:43  lucky
 *     Final version before v5.1
 *
 *     Revision 1.2  2000/08/09 17:01:29  lucky
 *     Lint Cleanup
 *
 *     Revision 1.1  2000/08/07 19:43:05  lucky
 *     Initial revision
 *
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ewdb_ora_api.h>
#include <ew_event_info.h>
#include <ewdb_apps_utils.h>

#define NCOMMAND  5          /* # of required commands you expect to process */
#define APP_MAXPATH 480
#define APP_MAXWORD 50


extern int errno;   /* system error variable */


char DBuser[APP_MAXWORD];
char DBservice[APP_MAXWORD];
char DBpassword[APP_MAXWORD];
char envEW_LOG[512];
char ArcFileName[APP_MAXPATH];

int		Debug;

int ReadConfig(char *configfile);
int PrintUsage(void);




main (int argc, char **argv)
{

	int					eventid;                /* current eventid we're processing  */


   /* Introduce ourselves
   **********************/
   if (argc != 3)
   {
	   PrintUsage();
	   exit(0);
   }
   


   /* Read the configuration file (path hardcoded relative to executable)
   *********************************************************************/
	ReadConfig (argv[1]); /* it exits if it isn't happy */

   /* Start up Logging
   *******************/
	logit_init ("db2file",1,1024,1);
   
	/* Make sure that the event ID is valid */
	eventid = atoi (argv[2]);

	if (eventid < 0)
	{
		logit ("", "Invalid event ID %d\n", eventid);
		return EW_FAILURE;
	}
	
  
	/* Open connection to database
	*****************************/
	if( ewdb_api_Init (DBuser, DBpassword, DBservice) != 0 )
	{
		logit ( "", "Trouble connecting to database; exiting!\n" );
		goto shutdown;
	}


	/* Call the library routine to retrieve event info
	   and write the Arc file
	 */

logit ("", "Calling ewdb_apps_DB2ArcFile with %d, %s\n", eventid, ArcFileName);

	if (ewdb_apps_DB2ArcFile (eventid, ArcFileName) != EW_SUCCESS)
	{
		logit ("", "db2file: call to ewdb_apps_DB2ArcFile failed.\n");
		return EW_FAILURE;
	}


shutdown:
	ewdb_api_Shutdown();
	logit("t","db2file: terminating\n" );
	return( 0 );

}  /* end main */



/**************************************************
 *  PrintUsage() prints usage hints to the screen *
  **************************************************/
int PrintUsage (void)
{
	fprintf(stderr, "db2file retrieves event info from the DBMS and writes it in ARC format\n");
	fprintf(stderr, "Usage: db2file <config file> <event ID>\n");
	return 0;
}





