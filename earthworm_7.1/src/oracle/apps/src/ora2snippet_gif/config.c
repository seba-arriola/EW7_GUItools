
/*
 *   This file is under RCS - do not modify unless you have
 *   checked it out using the command checkout.
 *
 *    $Id: config.c,v 1.11 2002/05/28 19:34:19 lucky Exp $
 *    Revision history:
 *
 *    $Log: config.c,v $
 *    Revision 1.11  2002/05/28 19:34:19  lucky
 *    Added header and footer tag text
 *
 *    Revision 1.10  2002/05/28 17:26:40  lucky
 *    *** empty log message ***
 *
 *    Revision 1.9  2002/03/28 20:06:32  davidk
 *    Made Logfiledir an optional parameter.
 *    Added ShowAllWiggles and ShowDeleteEventButton config parameters.
 *
 *    Revision 1.8  2002/03/20 22:00:32  davidk
 *    Added config file command for DEBUG flag.
 *
 *    Revision 1.7  2002/03/20 21:17:38  davidk
 *    fixed a type in a comment.
 *
 *    Revision 1.6  2002/03/20 21:16:23  davidk
 *    Added config file parameter for WiggleType (decimation type).
 *
 *    Revision 1.5  2002/02/20 20:10:20  lucky
 *    bkg color, header, and footer logos
 *
 *    Revision 1.3  2001/09/26 21:41:44  lucky
 *    Added another display time: showall. This way we only need one module
 *    to take care of snippet displays aligned by pick time as well
 *    as where no picks are available.
 *
 *    Revision 1.2  2001/02/28 17:29:10  lucky
 *    Massive schema redesign and cleanup.
 *
 *    Revision 1.1  2000/12/18 19:13:47  lucky
 *    Initial revision
 *
 *    Revision 1.2  2000/08/07 19:40:39  lucky
 *    Added WebHost option and using html_trailer from libsr
 *
 *    Revision 1.1  2000/02/15 19:39:25  lucky
 *    Initial revision
 *
 *    Revision 1.1  2000/01/07 18:25:24  davidk
 *    Initial revision
 *
 *    Revision 1.1  1999/05/05 18:31:15  lucky
 *    Initial revision
 *
 *
 */
  
/*****************************************************************

   ora2snippet_gif provides the user with pictures of record sections
   for the requested event.  The pictures are displayed in GIF format
   and are generated with the gd_lib from Thomas Boutell.  Record
   sections are the wiggles that are generated from plotting 
   digitally sampled waveform data over time.  

   config.c contains standard config file reading routines that 
   use the functions in the Earthworm kom.o object.

*****************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <kom.h>
#include "ora2snippet_gif.h"

#define MAXPATH 128
#define MAXWORD_CONFIG 12
extern char envEW_LOG[];
extern char DBservice[],DBpassword[],DBuser[], WebHost[];
extern char BackgroundColor[], HeaderLogo[], FooterLogo[], HeaderTag[], FooterTag[];
extern int MyModID,LogSwitch;

/*****************************************************************************
 *  ReadConfig() processes command file(s) using kom.c functions;            *
 *                  Exits if any errors are encountered 	             *
 *****************************************************************************/
#define ncommand  4          /* # of required commands you expect to process */

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
	bShowDeleteEventButton = FALSE;


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

  /*0*/     if( k_its("WebHost") ) {
                str = k_str();
                if( strlen(str) < 128 ) {
                    strcpy( WebHost, str );
                } else {
                    fprintf( stdout,"ReadConfig: Bad <WebHost> command in <%s>:\n"
                                    "            passwd <%s> too long; maxchar=%d; exiting!\n",
                             configfile, str, 128 );
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

         /* Optional commands */
         /*********************/
  
            /* Logswitch 
            **************/
      else if( k_its("LogFile") ) {
                LogSwitch = k_int();
            }

            /* MyModID 
            **************/
      else if( k_its("MyModuleId") ) {
                MyModID = k_int();
            }

            /* DisplayWidth 
            **************/
      else if( k_its("DisplayWidth") ) {
                GifWidth = k_int();
            }

            /* HeaderWidth 
            **************/
      else if( k_its("DisplayWidth") ) {
                HeaderWidth = k_int();
            }

            /* TraceHeight 
            **************/
      else if( k_its("TraceHeight") ) {
                TraceHeight = k_int();
            }

            /* DisplayTime 
            **************/
      else if( k_its("DisplayTime") ) {
                WiggleTime = k_int();
            }

            /* ScaleFactor 
            **************/
      else if( k_its("ScaleFactor") ) {
                ZoomSize = k_int();
            }

            /* MaxPlots 
            **************/
      else if( k_its("MaxPlots") ) {
                MaxPlots = k_int();
            }

            /* NumOfTicks 
            **************/
      else if( k_its("NumOfTicks") ) {
                NumOfTicks = k_int();
            }

            /* WiggleAlign 
            **************/
      else if( k_its("WiggleAlign") ) {
                WiggleAlign = k_int();
            }

            /* PrePickPcnt 
            **************/
      else if( k_its("PreAlgnPcntg") ) {
                PrePickPcnt = k_int();
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

            /* WiggleType  
            **************/
            else if( k_its("WiggleType") ) 
            {
                WiggleType = k_int();
            }

            /* ShowAllWiggles  
            **************/
            else if( k_its("ShowAllWiggles") ) 
            {
                bShowAllWiggles = TRUE;
            }

            /* ShowDeleteEventButton  
            **************/
            else if( k_its("ShowDeleteEventButton") ) 
            {
                bShowDeleteEventButton = TRUE;
            }

        /* Logfiledir
         ************************************/
           else if( k_its("Logfiledir") ) {
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
            }
            /* Debug
            **************/
            else if( k_its("Debug") ) 
            {
                DEBUG = k_int();
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
       if ( !init[0] )  fprintf( stdout, "<WebHost> "  );
       if ( !init[1] )  fprintf( stdout, "<DBservice> "   );
       if ( !init[2] )  fprintf( stdout, "<DBuser> "      );
       if ( !init[3] )  fprintf( stdout, "<DBpassword> "  );
       fprintf( stdout, "command(s) in <%s>; exiting!\n", configfile );
       return( -1 );
   }

   return( 0 );
}
