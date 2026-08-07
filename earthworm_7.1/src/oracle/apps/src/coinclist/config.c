
/*
 *   This file is under RCS - do not modify unless you have
 *   checked it out using the command checkout.
 *
 *    $Id: config.c,v 1.2 2002/05/28 19:34:19 lucky Exp $
 *    Revision history:
 *
 *    $Log: config.c,v $
 *    Revision 1.2  2002/05/28 19:34:19  lucky
 *    Added header and footer tag text
 *
 *    Revision 1.1  2002/03/22 20:13:23  lucky
 *    Initial revision
 *
 *
 *
 */
  
/*****************************************************************

*****************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <kom.h>

#define MAXPATH 128
#define MAXWORD_CONFIG 12
extern char envEW_LOG[];
extern char DBservice[],DBpassword[],DBuser[], WebHost[];
extern int MyModID,LogSwitch;
extern char  BackgroundColor[];
extern char  HeaderLogo[];
extern char  FooterLogo[];
extern char  HeaderTag[];
extern char  FooterTag[];


/*****************************************************************************
 *  ReadConfig() processes command file(s) using kom.c functions;            *
 *                  Exits if any errors are encountered 	             *
 *****************************************************************************/
#define ncommand  5          /* # of required commands you expect to process */

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
	fprintf( stdout,
                "ReadConfig: Error opening command file <%s>; exiting!\n", 
                 configfile );
	return(-1 );
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
                  fprintf( stdout, 
                          "ReadConfig: Error opening command file <%s>; exiting!\n",
                           &com[1] );
                  return(-1 );
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
                       fprintf( stdout,"ReadConfig: putenv: unable to set "
                                       "EW_LOG environment variable; exiting!\n" );
                       return( -1 );
                    }
                } else {
                    fprintf( stdout,"ReadConfig: Bad <Logfiledir> command in <%s>:\n"
                                    "            pathname <%s> too long; maxchar=%d; exiting!\n",
                             configfile, str, MAXPATH );
                    return( -1 );
                }
                init[0] = 1;
            }
 /*1*/     else if( k_its("DBservice") ) {
                str = k_str();
                if( strlen(str) < MAXWORD_CONFIG ) {
                    strcpy( DBservice, str );
                } else {
                    fprintf( stdout,"ReadConfig: Bad <DBservice> command in <%s>:\n"
                                    "            string <%s> too long; maxchar=%d; exiting!\n",
                             configfile, str, MAXWORD_CONFIG );
                    return( -1 );
                }
                init[1] = 1;
            }

  /*2*/     else if( k_its("DBuser") ) {
                str = k_str();
                if( strlen(str) < MAXWORD_CONFIG ) {
                    strcpy( DBuser, str );
                } else {
                    fprintf( stdout,"ReadConfig: Bad <DBuser> command in <%s>:\n"
                                    "            username <%s> too long; maxchar=%d; exiting!\n",
                             configfile, str, MAXWORD_CONFIG );
                    return( -1 );
                }
                init[2] = 1;
            }

  /*3*/     else if( k_its("DBpassword") ) {
                str = k_str();
                if( strlen(str) < MAXWORD_CONFIG ) {
                    strcpy( DBpassword, str );
                } else {
                    fprintf( stdout,"ReadConfig: Bad <DBpassword> command in <%s>:\n"
                                    "            passwd <%s> too long; maxchar=%d; exiting!\n",
                             configfile, str, MAXWORD_CONFIG );
                    return( -1 );
                }
                init[3] = 1;
            }
  /*4*/     else if( k_its("WebHost") ) {
                str = k_str();
                if( strlen(str) < 128 ) {
                    strcpy( WebHost, str );
                } else {
                    fprintf( stdout,"ReadConfig: Bad <WebHost> command in <%s>:\n"
                                    "            passwd <%s> too long; maxchar=%d; exiting!\n",
                             configfile, str, 128 );
                    return( -1 );
                }
                init[4] = 1;
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
                fprintf( stdout, "ReadConfig: <%s> Unknown command in <%s>.\n", 
                         com, configfile );
                continue;
            }

        /* See if there were any errors processing the command 
         *****************************************************/
            if( k_err() ) {
               fprintf( stdout, 
                       "ReadConfig: Bad <%s> command in <%s>; exiting!\n",
                        com, configfile );
               return( -1 );
            }
	}
	nfiles = k_close();
   }

/* After all files are closed, check init flags for missed commands
 ******************************************************************/
   nmiss = 0;
   for ( i=0; i<ncommand; i++ )  if( !init[i] ) nmiss++;
   if ( nmiss ) {
       fprintf( stdout, "ReadConfig: ERROR, no " );
       if ( !init[0] )  fprintf( stdout, "<Logfiledir> "  );
       if ( !init[1] )  fprintf( stdout, "<DBservice> "   );
       if ( !init[2] )  fprintf( stdout, "<DBuser> "      );
       if ( !init[3] )  fprintf( stdout, "<DBpassword> "  );
       if ( !init[4] )  fprintf( stdout, "<WebHost> "  );
       fprintf( stdout, "command(s) in <%s>; exiting!\n", configfile );
       return( -1 );
   }

   return( 0 );
}
