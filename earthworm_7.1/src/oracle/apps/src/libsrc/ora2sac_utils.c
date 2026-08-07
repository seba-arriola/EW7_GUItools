
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ora2sac_utils.c,v 1.3 2002/06/28 21:06:22 lucky Exp $
 *
 *    Revision history:
 *     $Log: ora2sac_utils.c,v $
 *     Revision 1.3  2002/06/28 21:06:22  lucky
 *     Lucky's pre-departure checkin. Most changes probably had to do with bug fixes
 *     in connection with the GIOC scaffold.
 *
 *     Revision 1.2  2002/05/28 17:25:41  lucky
 *     *** empty log message ***
 *
 *     Revision 1.1  2001/09/26 21:41:30  lucky
 *     Initial revision
 *
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <kom.h>
#include <ora2sac.h>

extern char DBuser[];
extern char DBservice[];
extern char DBpassword[];
extern char envEW_LOG[];
extern char SACDataDir[];
extern char BackgroundColor[];
extern char HeaderLogo[];
extern char FooterLogo[];
extern char HeaderTag[];
extern char FooterTag[];
extern long TraceBufferLen;   /* bytes of largest snippet we're up for -
                                       from configuration file */
extern long MaxTraces;        /* max traces per message we'll ever deal with */


/*****************************************************************************
 *  ReadConfig() processes command file(s) using kom.c functions;            *
 *                  Exits if any errors are encountered 	             *
 *****************************************************************************/
 /* FTP connection things */
 extern char  FTPDir[];
 extern char  FTPHost[];
 extern int  numWaveRetrieved;
 extern char BinDir[];


# define NCOMMAND  11          /* # of required commands you expect to process */

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
   for( i=0; i<NCOMMAND; i++ )  init[i] = 0;


	OraDebug = 0;
    strcpy (BackgroundColor, "notset");
    strcpy (HeaderLogo, "notset");
    strcpy (FooterLogo, "notset");



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
  /*1*/     else if( k_its("SACDataDir") ) {
                str = k_str();
                if( strlen(str) < APP_MAXPATH ) {
                    strcpy( SACDataDir, str );
                } else {
                    printf("ReadConfig: Bad SACDataDir command in %s:\n"
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
  /*8*/     else if( k_its("FTPHost") )
            {
                if ((str = k_str()) != NULL)
                    strcpy( FTPHost, str );

                init[8] = 1;
            }
  /*9*/     else if( k_its("FTPDir") )
            {
                if ((str = k_str()) != NULL)
                    strcpy( FTPDir, str );

                init[9] = 1;
            }
  /*10*/     else if( k_its("BinDir") )
            {
                if ((str = k_str()) != NULL)
                    strcpy( BinDir, str );

                init[10] = 1;
            }
            else if( k_its("Debug") ) {
                OraDebug=1;
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
       if ( !init[1] )  printf( "SACdatadir "     );
       if ( !init[2] )  printf( "DBservice "      );
       if ( !init[3] )  printf( "DBuser "         );
       if ( !init[4] )  printf( "DBpassword "     );
       if ( !init[5] )  printf( "MaxTraces  "     );
       if ( !init[6] )  printf( "TraceBufferLen " );
       if ( !init[7] )  printf( "OutputFormat " );
       if ( !init[8] )  printf( "FTPHost " );
       if ( !init[9] )  printf( "FTPDir " );
       if ( !init[10])  printf( "BinDir " );

       printf( "command(s) in %s; exiting!\n<br>\n", configfile );
       exit( -1 );
   }

   return( 0 );
}


/* Write html header to stdout */
void html_instructions ()
{

   printf("<center>\n");
   printf("<H1><FONT COLOR=blue>Earthworm DBMS<br></FONT></H1>\n" );
   printf("<H2><FONT COLOR=red>Download Event Seismograms in SAC Format</FONT></H2>\n");
   printf("<br><br><pre>\n");

   printf ("SAC Seismograms for the requested events are ready for download.\n"
           "Seismograms are available in <I>TAR</I> or <I>ZIP</I> formats.\n"
           "Clicking on the appropriate link below will begin the download process. \n"
           "You will be prompted for a local directory on your computer where\n"
           "the Seismograms should be saved.\n\n"
           "Close this window when download is finished.\n\n");
		

   printf("</pre></center>\n");
   return;

}


void html_tablerow( int eventid, char *ftphost, char *ftpdir, 
						char *yrmondir, char *eventdir, int nfiles )
{
   printf("<tr>\n"
          "<td align=center>%d</td> \n"
          "<td align=center><A HREF=\"http://%s%s/%s/%s.tar\">%s.tar</A></td> \n"
          "<td align=center><A HREF=\"http://%s%s/%s/%s.zip\">%s.zip</A></td> \n"
          "<td align=center>%d</td>\n"
          "</tr>\n",
			eventid, 
			ftphost, ftpdir, yrmondir, eventdir, eventdir, 
			ftphost, ftpdir, yrmondir, eventdir, eventdir, 
			nfiles );

   return;
}

void html_tableheader( void )
{
   printf("<center>\n"
          "<Table border=1>\n"
          "<tr bgcolor=\"#0000AA\"><strong>\n");
   printf("<td align=center><strong>"
          "  <font size=\"+1\" color=\"#FFFFFF\">Event ID</td>\n");
   printf("<td align=center><strong>"
          "  <font size=\"+1\" color=\"#FFFFFF\">Download in TAR format</td>\n");
   printf("<td align=center><strong>"
          "  <font size=\"+1\" color=\"#FFFFFF\">Download in ZIP format</td>\n");
   printf("<td align=center><strong>"
          "  <font size=\"+1\" color=\"#FFFFFF\">Number of Channels</td>\n");
   printf("</tr></strong>\n");
   return;
}

