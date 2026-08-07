/* config.c  config file for eqparam2html */

/*
 *   This file is under RCS - do not modify unless you have
 *   checked it out using the command checkout.
 *
 *    $Id: config.c,v 1.6 2005/04/25 15:28:02 davidk Exp $
 *    Revision history:
 *
 *    $Log: config.c,v $
 *    Revision 1.6  2005/04/25 15:28:02  davidk
 *    Moved logit_init to "Logfiledir" handler in ReadConfig(),
 *    so that logit_init() can be called as soon as the logfile
 *    dir is known, but not until then.  This way, the logit()
 *    error messages in ReadConfig can be properly output.
 *    In response to bug reported by Ali M.  2005/04/11
 *        *************************************************************/
 *
 *    Revision 1.5  2002/06/06 19:07:42  patton
 *    Made logit changes.
 *
 *    Revision 1.4  2002/05/28 19:34:19  lucky
 *    Added header and footer tag text
 *
 *    Revision 1.3  2002/02/20 20:16:07  lucky
 *    added web appearance parameters
 *
 *    Revision 1.2  2001/02/28 17:29:10  lucky
 *    Massive schema redesign and cleanup.
 *
 *    Revision 1.1  2000/02/15 19:45:44  lucky
 *    Initial revision
 *
 *    Revision 1.2  1999/09/22 16:16:43  lucky
 *    FTP connection stuff is now configurable from the config file
 *
 *    Revision 1.1  1999/06/11 19:49:02  lucky
 *    Initial revision
 *
 *    Revision 1.1  1999/05/05 18:05:41  lucky
 *    Initial revision
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

/* include earthworm headers */
#include <ewdb_ora_api.h>
#include <ewdb_apps_utils.h>
#include <kom.h>

/* include our own header file */
#include "stainfo.h"


/*****************************************************************************
 *  ReadConfig() processes command file(s) using kom.c functions;            *
 *                  Exits if any errors are encountered 	             *
 *****************************************************************************/
#define ncommand  6          /* # of required commands you expect to process */

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

    strcpy (BackgroundColor, "notset");
    strcpy (HeaderLogo, "notset");
    strcpy (FooterLogo, "notset");
    strcpy (HeaderTag, "notset");
    strcpy (FooterTag, "notset");

/* Open the main configuration file 
 **********************************/
   nfiles = k_open( configfile ); 
   if ( nfiles == 0 ) {
	logit ("e","ReadConfig: Error opening command file %s; exiting!\n<br>\n", 
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
                  logit ("e","ReadConfig: Error opening command file %s; exiting!\n<br>\n",
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
                       logit ("e","ReadConfig: putenv: unable to set "
                              "EW_LOG environment variable; exiting!\n<br>\n" );
                       exit( -1 );
                    }
                } else {
                    printf("ReadConfig: Bad Logfiledir command in %s:\n"
                           "            pathname \"%s\" too long; maxchar=%d; exiting!\n<br>\n",
                             configfile, str, MAXPATH );
                    exit( -1 );
                }
                /* Initialize logging. */
                logit_init("stainfo",0,1024,1);  

                init[0] = 1;
            }

  /*1*/     else if( k_its("DBservice") ) {
                str = k_str();
                if( strlen(str) < EQP_MAXWORD ) {
                    strcpy( DBservice, str );
                } else {
                    logit ("e","ReadConfig: Bad DBservice command in %s:\n"
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
                    logit ("e","ReadConfig: Bad DBuser command in %s:\n"
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
                    logit ("e","ReadConfig: Bad DBpassword command in %s:\n"
                           "            passwd \"%s\" too long; maxchar=%d; exiting!\n<br>\n",
                             configfile, str, EQP_MAXWORD );
                    exit( -1 );
                }
                init[3] = 1;
            }
  /*4*/     else if( k_its("FTPHost") )
            {
                if ((str = k_str()) != NULL)
                    strcpy( FTPHost, str );

                init[4] = 1;
            }
  /*5*/     else if( k_its("FTPDir") )
            {
                if ((str = k_str()) != NULL)
                    strcpy( FTPDir, str );

                init[5] = 1;
            }


            else if( k_its("Debug") ) {
                DEBUG=1;
            }
            /* BackgroundColor
            *******************/
              else if( k_its("BackgroundColor") )
            {
               strcpy(BackgroundColor,k_str());
            }

            /* HeaderLogo
            *******************/
              else if( k_its("HeaderLogo") )
            {
                  strcpy(HeaderLogo,k_str());
            }

            /* FooterLogo
            *******************/
            else if( k_its("FooterLogo") )
            {
               strcpy(FooterLogo,k_str());
            }
            /* HeaderTag
            *******************/
            else if( k_its("HeaderTag") )
            {
               strcpy(HeaderTag,k_str());
            }
            /* FooterTag
            *******************/
            else if( k_its("FooterTag") )
            {
               strcpy(FooterTag,k_str());
            }


         /* Unknown command
          *****************/ 
	    else {
                logit ("e", "ReadConfig: \"%s\" Unknown command in %s.\n<br>\n", 
                         com, configfile );
                continue;
            }

        /* See if there were any errors processing the command 
         *****************************************************/
            if( k_err() ) {
               logit ("e", "ReadConfig: Bad %s command in %s; exiting!\n<br>\n",
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
       logit ("e", "ReadConfig: ERROR, no " );
       if ( !init[0] )  logit ("e", "Logfiledir "     );
       if ( !init[1] )  logit ("e", "DBservice "      );
       if ( !init[2] )  logit ("e", "DBuser "         );
       if ( !init[3] )  logit ("e", "DBpassword "     );
       if ( !init[4] )  logit ("e", "FTPHost "        );
       if ( !init[5] )  logit ("e", "FTPDir "         );
       logit ("e", "command(s) in %s; exiting!\n<br>\n", configfile );
       exit( -1 );
   }

   return( 0 );
}

