/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: AlarmsConfig.c,v 1.3 2002/06/28 21:06:22 lucky Exp $
 *    Revision history:
 *
 *    $Log: AlarmsConfig.c,v $
 *    Revision 1.3  2002/06/28 21:06:22  lucky
 *    Lucky's pre-departure checkin. Most changes probably had to do with bug fixes
 *    in connection with the GIOC scaffold.
 *
 *    Revision 1.2  2002/05/28 17:25:41  lucky
 *    *** empty log message ***
 *
 *    Revision 1.1  2001/07/01 21:55:18  davidk
 *    Initial revision
 *
 *    Revision 1.1  2001/05/18 19:07:48  lucky
 *    Initial revision
 *
 *    Revision 1.1  2001/05/15 02:15:02  davidk
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


#include <kom.h>
#include <alarms.h>


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
                           "pathname \"%s\" too long; maxchar=%d; exiting!\n\n",
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
                           "string \"%s\" too long; maxchar=%d; exiting!\n\n",
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
                           "username \"%s\" too long; maxchar=%d; exiting!\n\n",
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
                            "passwd \"%s\" too long; maxchar=%d; exiting!\n\n",
                             configfile, str, EQP_MAXWORD );
                    exit( -1 );
                }
                init[3] = 1;
		  	}
  /*4*/     else if( k_its("WebHost") ) {
                str = k_str();
                if( strlen(str) < MAXPATH ) {
                    strcpy( WebHost, str );
                } else {
                    printf("ReadConfig: Bad WebHost command in %s:\n"
                            "string \"%s\" too long; maxchar=%d; exiting!\n\n",
                             configfile, str, MAXPATH );
                    exit( -1 );
                }
                init[4] = 1;
		  	}
  /*5*/     else if( k_its("AlarmDir") ) {
                str = k_str();
                if( strlen(str) < MAXPATH ) {
                    strcpy( AlarmDir, str );
                } else {
                    printf("ReadConfig: Bad AlarmDir command in %s:\n"
                            "string \"%s\" too long; maxchar=%d; exiting!\n\n",
                             configfile, str, MAXPATH );
                    exit( -1 );
                }
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
               strcpy(HeaderTag, k_str());
            }
            /* FooterTag 
            *******************/
            else if( k_its("FooterTag") )
            {
               strcpy(FooterTag, k_str());
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
       if ( !init[4] )  printf( "WebHost "     );
       if ( !init[5] )  printf( "AlarmDir "     );
       printf( "command(s) in %s; exiting!\n<br>\n", configfile );
       exit( -1 );
   }

   return( 0 );
}

