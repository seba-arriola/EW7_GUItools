
/*
 *   This file is under RCS - do not modify unless you have
 *   checked it out using the command checkout.
 *
 *    $Id: stainfo.c,v 1.10 2005/04/25 15:27:50 davidk Exp $
 *    Revision history:
 *
 *    Revision 1.1  1999/05/05 18:05:41  lucky
 *    Initial revision
 *
 *
 */
  
/*****************************************************************

   filename:       stainfo.c 
   module:         stainfo
   platforms:      solaris 2.7 (& maybe NT 4)
   date:           04/20/1999
   bug creator:    David Kragness

   description:   

 *  Given an eventid, eqparam2html grabs the event's hypocenter 
 *  and parametric data out of the database and prints the
 *  info out in a pretty html form that the user can read.
 *  It also provides links to the record section displays for
 *  a certain event by providing hyperlinks to ora2rsec_gif and
 *  its derivatives.

  Topics:

*****************************************************************/


/* include the normal system stuff pluss errno to boot. */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

/* I forget what we grab out of unistd, but we grab something
   and to support multiple platforms this ugly ifdef is needed */
#ifndef _WINNT  /* DavidK 990119 */
# include <unistd.h> /* Winnt has no idea what this is, ha ha!! */
#endif

/* In case things are a little slow we would like to figure out
   what to work on, that is what things need the most speed
   improvement */

/* include earthworm headers */
#include <kom.h>
#include <time_ew.h>  /* DavidK: done for perf. timing */
#include <ewdb_ora_api.h> 
#include <ewdb_apps_utils.h> 
#include <html_common.h>

/* include our own header file */
#include "stainfo.h"


int   DEBUG;                     /* debug flag */

/* Globals to set from configuration file 
 ****************************************/

/* Environment stuff */
char  envEW_LOG[512];        /* where environment variable EW_LOG 
                                        will be stored                  */
/* Database connection things */
char  DBservice[50];        /* DBMS instance to interact with    */
char  DBuser[50];           /* UserId to connect to database as  */
char  DBpassword[50];       /* Password to datasource            */

char  BackgroundColor[512]; 
char  HeaderLogo[512];
char  FooterLogo[512];
char  HeaderTag[512];
char  FooterTag[512];



extern int errno;   /* system error variable */


main( int argc, char **argv )
/******************************************************
  Function:    main()
  Purpose:     main() is the main function.  It calls all
               neccessary support functions to get you 
               parameters for a given seismic event.  It also
               provides you links to record section displays
               for the requested event.  For people like me
               who don't know what record sections are, they
               are wiggles, like you would see on a seismograph.
  Parameters:    
      Input
      argc:   Number of command like arguments
      argv:   Array of pointers to command line arguments

  Author: DK, before 04/15/1999

  **********************************************************/
{
   char               *configfile = "../params/stainfo.d";
   EWDB_StationStruct  sSta;              /* structure for station data       */
   int                 i;
   char                strChan[20];
   int                 idChan;
   int                 ParamsTime;

/* Read the configuration file (path hardcoded relative to executable)
 *********************************************************************/
   ReadConfig( configfile );

   /* moved logit_init to "Logfiledir" handler in ReadConfig(),
      so that logit_init() can be called as soon as the logfile
      dir is known, but not until then.  This way, the logit()
      error messages in ReadConfig can be properly output. 
      In response to bug reported by Ali M.  2005/04/11  (DK)
    *************************************************************/

   if (argc != 2)
   {
     logit ("", "stainfo: idChan argument missing; exitting!\n");
     return -1;
   }


  i = 0;
  while (argv[1][i] != ',')
  {
    strChan[i] = argv[1][i];
    i = i + 1;;
  }
  strChan[i] = '\0';


	idChan = atoi (strChan);

  logit ("", "stainfo: looking for %s (%d)\n", strChan, idChan);

/* Send html header back to web server
 *************************************/

   printf("Content-type: text/html\n\n");
   printf("<html>\n");
   printf("<HEAD><TITLE>stainfo: Station Information</TITLE></HEAD>\n");
   
   html_header(BackgroundColor, HeaderLogo, HeaderTag);

   printf("<center>\n");
   printf("<H1><FONT COLOR=blue>Earthworm DBMS<br></FONT></H1>\n" );
   printf("<H2><FONT COLOR=red>Station Information</FONT></H2>\n");
   printf("</center>\n");



/* Open connection to database
 *****************************/

   if( ewdb_api_Init(DBuser, DBpassword, DBservice) != 0 )
   {
      logit( "e", "Trouble connecting to database; exiting!\n" );
      html_break();
   }


   ParamsTime = time(NULL);
   if (ewdb_api_GetComponentInfo (idChan, ParamsTime, &sSta) != 0)
   {
     logit ("", "stainfo: Call to ewdb_api_GetComponentInfo failed; exitting!\n");
     ewdb_api_Shutdown();
     html_trailer (FTPHost, FooterLogo, FooterTag);
     return -1;
   }

   html_station (&sSta, FTPHost, FTPDir);

   ewdb_api_Shutdown();
   html_trailer(FTPHost, FooterLogo, FooterTag);
   logit("t","stainfo: terminating\n" );

   return( 0 );
}  /* end main */


