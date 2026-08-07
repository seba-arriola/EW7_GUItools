/* config.c  config file for eqparam2html */

/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: config.c,v 1.2 2000/12/06 17:49:21 lucky Exp $
 *    Revision history:
 *
 *    $Log: config.c,v $
 *    Revision 1.2  2000/12/06 17:49:21  lucky
 *    *** empty log message ***
 *
 *    Revision 1.1  2000/08/07 19:48:27  lucky
 *    Initial revision
 *
 *
 *
 */
  
/*************************************************************************
  This is standard config file reading code, adapted
  from earthworm config files.  It uses the kom.c libraries from 
  earthworm (or wherever earthworm may have lifted them from).  
  Davidk 19990419 
*************************************************************************/


/* include the normal system stuff*/
#include <stdlib.h>
#include <stdio.h>

/* include Phase III and earthworm headers */
#include <p3db_ora_api.h>
#include <kom.h>

/* include our own header file */
#include "eqreview.h"


/*****************************************************************************
 *  ReadConfig() processes command file(s) using kom.c functions;            *
 *                  Exits if any errors are encountered 	             *
 *****************************************************************************/
#define ncommand  13          /* # of required commands you expect to process */

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


	NumRevSrcs = 0;

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
                if( strlen(str) < MAXPATH ) 
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
                             configfile, str, MAXPATH );
                    exit( -1 );
                }
                init[0] = 1;
            }

  /*1*/     else if( k_its("DBservice") ) {
                str = k_str();
                if( strlen(str) < EQP_MAXWORD ) {
                    strcpy( DBservice, str );
                } else {
                    printf("ReadConfig: Bad DBservice command in %s:\n"
                           "            string \"%s\" too long; maxchar=%d; exiting!\n<br>\n",
                             configfile, str, EQP_MAXWORD );
                    exit( -1 );
                }
                init[1] = 1;
            }

  /*2*/     else if( k_its("DBuser") ) {
                str = k_str();
                if( strlen(str) < EQP_MAXWORD ) {
                    strcpy( DBuser, str );
                } else {
                    printf("ReadConfig: Bad DBuser command in %s:\n"
                           "            username \"%s\" too long; maxchar=%d; exiting!\n<br>\n",
                             configfile, str, EQP_MAXWORD );
                    exit( -1 );
                }
                init[2] = 1;
            }

  /*3*/     else if( k_its("DBpassword") ) {
                str = k_str();
                if( strlen(str) < EQP_MAXWORD ) {
                    strcpy( DBpassword, str );
                } else {
                    printf("ReadConfig: Bad DBpassword command in %s:\n"
                           "            passwd \"%s\" too long; maxchar=%d; exiting!\n<br>\n",
                             configfile, str, EQP_MAXWORD );
                    exit( -1 );
                }
                init[3] = 1;
            }
  /*4*/     else if( k_its("ReviewDir") ) {
                str = k_str();
                if( strlen(str) < MAXPATH ) {
                    strcpy( ReviewDir, str );
                    sprintf( ReviewTmpDir, "%s/tmp", ReviewDir );
                } else {
                    printf("ReadConfig: Bad ReviewDir command in %s:\n"
                           "            name \"%s\" too long; maxchar=%d; exiting!\n<br>\n",
                             configfile, str, MAXPATH );
                    exit( -1 );
                }
                init[4] = 1;
            }
  /*5*/     else if( k_its("PathToHypoBin") ) {
                str = k_str();
                if( strlen(str) < MAXPATH ) {
                    strcpy( HypoBin, str );
                } else {
                    printf("ReadConfig: Bad PathToHypoBin command in %s:\n"
                           "            name \"%s\" too long; maxchar=%d; exiting!\n<br>\n",
                             configfile, str, MAXPATH );
                    exit( -1 );
                }
                init[5] = 1;
            }
  /*6*/     else if( k_its("HypoConfig") ) {
                str = k_str();
                if( strlen(str) < MAXPATH ) {
                    strcpy( HypoCfg, str );
                } else {
                    printf("ReadConfig: Bad HypoConfig command in %s:\n"
                           "            name \"%s\" too long; maxchar=%d; exiting!\n<br>\n",
                             configfile, str, MAXPATH );
                    exit( -1 );
                }
                init[6] = 1;
            }

  /*7*/     else if( k_its("HypoinverseDir") ) {
                str = k_str();
                if( strlen(str) < MAXPATH ) {
                    strcpy( HypoDir, str );
                } else {
                    printf("ReadConfig: Bad HypoinverseDir command in %s:\n"
                           "            name \"%s\" too long; maxchar=%d; exiting!\n<br>\n",
                             configfile, str, MAXPATH );
                    exit( -1 );
                }
                init[7] = 1;
            }
  /*8*/     else if( k_its("JavascriptFile") ) {
                str = k_str();
                if( strlen(str) < MAXPATH ) {
                    strcpy( JavascriptFile, str );
                } else {
                    printf("ReadConfig: Bad JavascriptFile command in %s:\n"
                           "            name \"%s\" too long; maxchar=%d; exiting!\n<br>\n",
                             configfile, str, MAXPATH );
                    exit( -1 );
                }
                init[8] = 1;
            }
  /*9*/     else if( k_its("SacFormat") ) {
                str = k_str();
                if( strlen(str) < MAXPATH ) {
                    strcpy( SacFormat, str );
                } else {
                    printf("ReadConfig: Bad SacFormat command in %s:\n"
                           "            name \"%s\" too long; maxchar=%d; exiting!\n<br>\n",
                             configfile, str, MAXPATH );
                    exit( -1 );
                }
                init[9] = 1;
            }
  /*10*/     else if( k_its("WebDir") ) {
                str = k_str();
                if( strlen(str) < MAXPATH ) {
                    strcpy( WebDir, str );
                    sprintf( WebTmpDir,"%s/tmp", WebDir );
                } else {
                    printf("ReadConfig: Bad WebDir command in %s:\n"
                           "            name \"%s\" too long; maxchar=%d; exiting!\n<br>\n",
                             configfile, str, MAXPATH );
                    exit( -1 );
                }
                init[10] = 1;
            }
  /*11*/     else if( k_its("WebHost") ) {
                str = k_str();
                if( strlen(str) < MAXPATH ) {
                    strcpy( WebHost, str );
                } else {
                    printf("ReadConfig: Bad WebHost command in %s:\n"
                           "            name \"%s\" too long; maxchar=%d; exiting!\n<br>\n",
                             configfile, str, MAXPATH );
                    exit( -1 );
                }
                init[11] = 1;
            }
  /*12*/     else if( k_its("ReviewSource") ) 
             {
                if (NumRevSrcs >= MAX_REV_SRCS)
                {
                    fprintf( stderr,
                            "Too many <ReviewSource> commands in <%s>", configfile );
                    fprintf( stderr, "; max=%d; exitting!\n", (int) MAX_REV_SRCS );
                    exit( -1 );
                }

                if ((str = k_str()) == NULL)
  		        {
                    printf("ReadConfig: Bad ReviewSource command in %s:\n"
                           "            name is NULL; exiting!\n<br>\n",
                             configfile);
        		    exit (-1);
	        	}
                if( strlen(str) < EQP_MAXWORD ) 
		        {
                    if ((RevSrcs[NumRevSrcs] = malloc (EQP_MAXWORD)) == NULL)
  		            {
                        printf("ReadConfig: Call to malloc failed; exitting.\n");
        		        exit (-1);
	        	    }

                    strcpy( RevSrcs[NumRevSrcs], str );
                } else {
                    printf("ReadConfig: Bad ReviewSource command in %s:\n"
                           "            name \"%s\" too long; maxchar=%d; exiting!\n<br>\n",
                             configfile, str, EQP_MAXWORD );
                    exit( -1 );
                }
                NumRevSrcs = NumRevSrcs + 1;
                init[12] = 1;
            }

            else if( k_its("Debug") ) {
                DEBUG=1;
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
       if ( !init[1] )  printf( "DBservice "      );
       if ( !init[2] )  printf( "DBuser "         );
       if ( !init[3] )  printf( "DBpassword "     );
       if ( !init[4] )  printf( "ReviewDir "    );
       if ( !init[5] )  printf( "PathToHypoBin "    );
       if ( !init[6] )  printf( "HypoConfig "    );
       if ( !init[7] )  printf( "HypoinverseDir "    );
       if ( !init[8] )  printf( "JavascriptFile "    );
       if ( !init[9] )  printf( "SacFormat "    );
       if ( !init[10] )  printf( "WebDir "    );
       if ( !init[11] )  printf( "WebHost "    );
       if ( !init[12] )  printf( "ReviewSource "    );
       printf( "command(s) in %s; exiting!\n<br>\n", configfile );
       exit( -1 );
   }

   return( 0 );
}

