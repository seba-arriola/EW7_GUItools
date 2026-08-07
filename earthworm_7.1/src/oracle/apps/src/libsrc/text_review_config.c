
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: text_review_config.c,v 1.1 2001/07/01 21:55:23 davidk Exp $
 *
 *    Revision history:
 *     $Log: text_review_config.c,v $
 *     Revision 1.1  2001/07/01 21:55:23  davidk
 *     Initial revision
 *
 *     Revision 1.2  2000/08/09 17:01:29  lucky
 *     Lint Cleanup
 *
 *     Revision 1.1  2000/08/07 19:43:05  lucky
 *     Initial revision
 *
 *
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <kom.h>

#define NCOMMAND  5          /* # of required commands you expect to process */
#define APP_MAXPATH 480
#define APP_MAXWORD 50


extern 	char 	DBuser[];
extern 	char 	DBservice[];
extern 	char 	DBpassword[];
extern 	char 	envEW_LOG[];
extern 	char 	ArcFileName[];
extern	int		Debug;



/*****************************************************************************
 *  ReadConfig() processes command file(s) using kom.c functions;            *
 *                  Exits if any errors are encountered 	             *
 *****************************************************************************/

int ReadConfig( char *configfile )
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
  /*1*/     else if( k_its("ArcFileName") ) {
                str = k_str();
                if( strlen(str) < APP_MAXPATH ) {
                    strcpy( ArcFileName, str );
                } else {
                    printf("ReadConfig: Bad ArcFileName command in %s:\n"
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
       if ( !init[1] )  printf( "ArcFileName "     );
       if ( !init[2] )  printf( "DBservice "      );
       if ( !init[3] )  printf( "DBuser "         );
       if ( !init[4] )  printf( "DBpassword "     );
       printf( "command(s) in %s; exiting!\n<br>\n", configfile );
       exit( -1 );
   }

   return( 0 );
}
