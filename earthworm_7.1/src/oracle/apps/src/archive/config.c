
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: config.c,v 1.3 2001/07/01 21:55:13 davidk Exp $
 *
 *    Revision history:
 *     $Log: config.c,v $
 *     Revision 1.3  2001/07/01 21:55:13  davidk
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
 *     Revision 1.2  2000/08/09 16:18:58  lucky
 *     Lint cleanup
 *
 *     Revision 1.1  2000/02/15 19:02:31  lucky
 *     Initial revision
 *
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <kom.h>
#include "archive.h"

extern char DBuser[];
extern char DBservice[];
extern char DBpassword[];
extern char envEW_LOG[];
extern char SACdatadir[];
extern long TraceBufferLen;   /* bytes of largest snippet we're up for -
                                       from configuration file */
extern long MaxTraces;        /* max traces per message we'll ever deal with */


/*****************************************************************************
 *  ReadConfig() processes command file(s) using kom.c functions;            *
 *                  Exits if any errors are encountered 	             *
 *****************************************************************************/
#define ncommand  8          /* # of required commands you expect to process */

int ReadConfig( char *configfile )
{
   char     init[ncommand]; /* init flags, one byte for each required command */
   int      nmiss;          /* number of required commands that were missed   */
   char    *com;
   int      nfiles;
   int      success;
   int      i;	
   char*    str;

/* Set to zero one init flag for each required command 
 *****************************************************/   
   for( i=0; i<ncommand; i++ )  init[i] = 0;


	ArchDebug = 0;

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
  /*1*/     else if( k_its("OutDir") ) {
                str = k_str();
                if( strlen(str) < APP_MAXPATH ) {
                    strcpy( SACdatadir, str );
                } else {
                    printf("ReadConfig: Bad OutDir command in %s:\n"
                           "            pathname \"%s\" too long; maxchar=%d; exiting!\n<br>\n",
                             configfile, str, APP_MAXPATH );
                    exit( -1 );
                }
                init[1] = 1;
            }
 

  /*2*/     else if( k_its("DBservice") ) {
                str = k_str();
                if( strlen(str) < APP_MAXWORD ) {
                    strcpy( DBservice, str );
                } else {
                    printf("ReadConfig: Bad DBservice command in %s:\n"
                           "            string \"%s\" too long; maxchar=%d; exiting!\n<br>\n",
                             configfile, str, APP_MAXWORD );
                    exit( -1 );
                }
                init[2] = 1;
            }

  /*3*/     else if( k_its("DBuser") ) {
                str = k_str();
                if( strlen(str) < APP_MAXWORD ) {
                    strcpy( DBuser, str );
                } else {
                    printf("ReadConfig: Bad DBuser command in %s:\n"
                           "            username \"%s\" too long; maxchar=%d; exiting!\n<br>\n",
                             configfile, str, APP_MAXWORD );
                    exit( -1 );
                }
                init[3] = 1;
            }

  /*4*/     else if( k_its("DBpassword") ) {
                str = k_str();
                if( strlen(str) < APP_MAXWORD ) {
                    strcpy( DBpassword, str );
                } else {
                    printf("ReadConfig: Bad DBpassword command in %s:\n"
                           "            passwd \"%s\" too long; maxchar=%d; exiting!\n<br>\n",
                             configfile, str, APP_MAXWORD );
                    exit( -1 );
                }
                init[4] = 1;
            }
  /*5*/     else if( k_its("MaxTraces") )
            {
                MaxTraces = k_int();
                init[5] = 1;
            }

  /*6*/     else if( k_its("TraceBufferLen") )
            {
                TraceBufferLen = k_int() * 1024; /* convert from kilobytes to bytes */
                init[6] = 1;
            }
  /*7*/     else if( k_its("OutputFormat") ) {
                str = k_str();
                if( strlen(str) < APP_MAXWORD ) {
                    strcpy( OutputFormat, str );
                } else {
                    printf("ReadConfig: Bad OutputFormat command in %s:\n"
                           "            passwd \"%s\" too long; maxchar=%d; exiting!\n<br>\n",
                             configfile, str, APP_MAXWORD );
                    exit( -1 );
                }
                init[7] = 1;
            }


            else if( k_its("Debug") ) {
                ArchDebug=1;
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
   for ( i=0; i<ncommand; i++ )  if( !init[i] ) nmiss++;
   if ( nmiss ) {
       printf( "ReadConfig: ERROR, no " );
       if ( !init[0] )  printf( "Logfiledir "     );
       if ( !init[1] )  printf( "SACdatadir "     );
       if ( !init[2] )  printf( "DBservice "      );
       if ( !init[3] )  printf( "DBuser "         );
       if ( !init[4] )  printf( "DBpassword "     );
       if ( !init[5] )  printf( "MaxTraces  "     );
       if ( !init[6] )  printf( "TraceBufferLen " );
       if ( !init[7] )  printf( "OutputFormat " );
       printf( "command(s) in %s; exiting!\n<br>\n", configfile );
       exit( -1 );
   }

   return( 0 );
}
