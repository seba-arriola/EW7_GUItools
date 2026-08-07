
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: file2db.c,v 1.7 2002/03/21 20:48:17 lucky Exp $
 *
 *    Revision history:
 *     $Log: file2db.c,v $
 *     Revision 1.7  2002/03/21 20:48:17  lucky
 *     fixed array sizes
 *
 *     Revision 1.6  2001/07/01 21:55:29  davidk
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
 *     Revision 1.5  2001/05/15 02:15:38  davidk
 *     Moved functions around between the apps, DB API, and DB API INTERNAL
 *     levels.  Renamed functions and files.  Added support for amplitude
 *     magnitude types.  Reformatted makefiles.
 *
 *     Revision 1.4  2001/02/28 17:39:59  lucky
 *     Massive schema redesign and cleanup.
 *
 *     Revision 1.3  2000/09/18 17:23:36  lucky
 *     Final version before v5.1
 *
 *     Revision 1.2  2000/08/09 17:01:29  lucky
 *     Lint Cleanup
 *
 *     Revision 1.1  2000/08/07 19:44:46  lucky
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


int PrintUsage (void);


extern int errno;   /* system error variable */


char DBuser[APP_MAXWORD];
char DBservice[APP_MAXWORD];
char DBpassword[APP_MAXWORD];
char envEW_LOG[512];
char ArcFileName[APP_MAXPATH];

int		Debug;

#define		MAX_ARC_MSG		100000



main (int argc, char **argv)
{

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
	logit_init ("file2db",1,1024,1);
   
	logit ("","startup. Debug=%d\n", Debug);
  
	logit("","\n/*************************************\n"
	          "  Initializing ORA_API                 \n"
	          "*************************************/\n");

	/* Open connection to database
	*****************************/
	if( ewdb_api_Init (DBuser, DBpassword, DBservice) != 0 )
	{
		logit( "e", "Trouble connecting to database; exiting!\n" );
		goto shutdown;
	}


	/* Call the library routine to read the Arc file
	   and write the info into the DB
	 */

	if (ewdb_apps_ArcFile2DB (argv[2], "Reviewed", NULL) != EW_SUCCESS)
	{
		logit ("", "file2db: call to ewdb_apps_ArcFile2DB failed.\n");
		return EW_FAILURE;
	}


shutdown:
	ewdb_api_Shutdown();
	logit("t","file2db: terminating\n" );
	return( 0 );

}  /* end main */



/**************************************************
 *  PrintUsage() prints usage hints to the screen *
  **************************************************/
int PrintUsage (void)
{
	fprintf(stderr, "file2db reads ARC file and writes the event info to the DBMS\n");
	fprintf(stderr, "Usage: file2db <config file> <ARC file name>\n");
	return 0;
}


