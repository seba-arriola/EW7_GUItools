
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: hypo2ora.c,v 1.8 2001/07/01 21:55:16 davidk Exp $
 *
 *    Revision history:
 *     $Log: hypo2ora.c,v $
 *     Revision 1.8  2001/07/01 21:55:16  davidk
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
 *     Revision 1.7  2001/05/15 02:15:20  davidk
 *     Moved functions around between the apps, DB API, and DB API INTERNAL
 *     levels.  Renamed functions and files.  Added support for amplitude
 *     magnitude types.  Reformatted makefiles.
 *
 *     Revision 1.6  2001/02/28 17:29:10  lucky
 *     Massive schema redesign and cleanup.
 *
 *     Revision 1.5  2000/08/28 16:06:43  lucky
 *     get_ew_event_info.h -> ew_event_info.h
 *
 *     Revision 1.4  2000/08/15 19:13:13  lucky
 *     *** empty log message ***
 *
 *     Revision 1.3  2000/06/13 19:09:15  lucky
 *     Fixed to work with the new schema, as well as with the new library routines
 *     to read arc files, convert them to DB structs, and insert those into the DB.
 *
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


#include <earthworm.h>
#include <kom.h>
#include <ewdb_ora_api.h>
#include <ewdb_apps_utils.h>
#include <ew_event_info.h>

#define NCOMMAND  4          /* # of required commands you expect to process */
#define APP_MAXPATH 480
#define APP_MAXWORD 50


extern int errno;   /* system error variable */


char DBuser[APP_MAXWORD];
char DBservice[APP_MAXWORD];
char DBpassword[APP_MAXWORD];
char envEW_LOG[APP_MAXPATH];

int		Debug;

#define		MAX_ARC_MSG		100000

static	int ReadConfig (char *);
static	int PrintUsage (void);


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
	logit_init ("hypo2ora",1,1024,1);
   
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
		logit ("", "hypo2ora: call to ewdb_apps_ArcFile2DB failed.\n");
		return EW_FAILURE;
	}


shutdown:
	ewdb_api_Shutdown();
	logit("t","hypo2ora: terminating\n" );
	return( 0 );

}  



/**************************************************
 *  PrintUsage() prints usage hints to the screen *
  **************************************************/
static	int PrintUsage (void)
{
	fprintf(stderr, "hypo2ora reads ARC file and writes the event info to the DBMS\n");
	fprintf(stderr, "Usage: hypo2ora <config file> <ARC file name>\n");
	return 0;
}





/*****************************************************************************
 *  ReadConfig() processes command file(s) using kom.c functions;            *
 *                  Exits if any errors are encountered 	             *
 *****************************************************************************/

static	int ReadConfig( char *configfile )
{
	char     init[NCOMMAND]; /* init flags, one byte for each required command */
	int      nmiss;          /* number of required commands that were missed   */
	char    *com;
	int      nfiles;
	int      success;
	int      i;	
	char*    str;

/* Set to zero one init flag for each required command 
 *****************************************************/   
	for (i = 0; i < NCOMMAND; i++ )  
		init[i] = 0;


	Debug = 0;

/* Open the main configuration file 
 **********************************/
   nfiles = k_open( configfile ); 
   if ( nfiles == 0 ) {
	printf("ReadConfig: Error opening command file %s; exiting!\n<br>\n", 
                configfile );
	exit( -1 );
   }

/* Process all command files
 ***************************/
   while(nfiles > 0)   /* While there are command files open */
   {
        while(k_rd())        /* Read next line from active file  */
        {  
	    com = k_str();         /* Get the first token from line */

        /* Ignore blank lines & comments
         *******************************/
            if( !com )           continue;
            if( com[0] == '#' )  continue;

        /* Open a nested configuration file 
         **********************************/
            if( com[0] == '@' ) {
               success = nfiles+1;
               nfiles  = k_open(&com[1]);
               if ( nfiles != success ) {
                  printf("ReadConfig: Error opening command file %s; exiting!\n<br>\n",
                           &com[1] );
                  exit( -1 );
               }
               continue;
            }

        /* Process anything else as a command 
         ************************************/
  /*0*/     if( k_its("Logfiledir") ) {
                str = k_str();
                if( strlen(str) < APP_MAXPATH ) 
                {
                    sprintf( envEW_LOG, "EW_LOG=%s", str );  
                    if( putenv( envEW_LOG ) != 0 )  /*set environment variable for logit*/
                    {
                       printf("ReadConfig: putenv: unable to set "
                              "EW_LOG environment variable; exiting!\n<br>\n" );
                       exit( -1 );
                    }
                } else {
                    printf("ReadConfig: Bad Logfiledir command in %s:\n"
                           "            pathname \"%s\" too long; maxchar=%d; exiting!\n<br>\n",
                             configfile, str, APP_MAXPATH );
                    exit( -1 );
                }
                init[0] = 1;
            }

  /*1*/     else if( k_its("DBservice") ) {
                str = k_str();
                if( strlen(str) < APP_MAXWORD ) {
                    strcpy( DBservice, str );
                } else {
                    printf("ReadConfig: Bad DBservice command in %s:\n"
                           "            string \"%s\" too long; maxchar=%d; exiting!\n<br>\n",
                             configfile, str, APP_MAXWORD );
                    exit( -1 );
                }
                init[1] = 1;
            }

  /*2*/     else if( k_its("DBuser") ) {
                str = k_str();
                if( strlen(str) < APP_MAXWORD ) {
                    strcpy( DBuser, str );
                } else {
                    printf("ReadConfig: Bad DBuser command in %s:\n"
                           "            username \"%s\" too long; maxchar=%d; exiting!\n<br>\n",
                             configfile, str, APP_MAXWORD );
                    exit( -1 );
                }
                init[2] = 1;
            }

  /*3*/     else if( k_its("DBpassword") ) {
                str = k_str();
                if( strlen(str) < APP_MAXWORD ) {
                    strcpy( DBpassword, str );
                } else {
                    printf("ReadConfig: Bad DBpassword command in %s:\n"
                           "            passwd \"%s\" too long; maxchar=%d; exiting!\n<br>\n",
                             configfile, str, APP_MAXWORD );
                    exit( -1 );
                }
                init[3] = 1;
            }

            else if( k_its("Debug") ) {
                Debug = 1;
            }

         /* Unknown command
          *****************/ 
	    else {
                printf( "ReadConfig: \"%s\" Unknown command in %s.\n<br>\n", 
                         com, configfile );
                continue;
            }

        /* See if there were any errors processing the command 
         *****************************************************/
            if( k_err() ) {
               printf("ReadConfig: Bad %s command in %s; exiting!\n<br>\n",
                        com, configfile );
               exit( -1 );
            }
	}
	nfiles = k_close();
   }

/* After all files are closed, check init flags for missed commands
 ******************************************************************/
   nmiss = 0;
   for ( i=0; i<NCOMMAND; i++ )  if( !init[i] ) nmiss++;
   if ( nmiss ) {
       printf( "ReadConfig: ERROR, no " );
       if ( !init[0] )  printf( "Logfiledir "     );
       if ( !init[1] )  printf( "DBservice "      );
       if ( !init[2] )  printf( "DBuser "         );
       if ( !init[3] )  printf( "DBpassword "     );
       printf( "command(s) in %s; exiting!\n<br>\n", configfile );
       exit( -1 );
   }

   return( 0 );
}
