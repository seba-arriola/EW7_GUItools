
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: config.c,v 1.1 2002/05/15 20:45:45 davidk Exp $
 *
 *    Revision history:
 *
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <kom.h>
#include <ewdb_ora_api.h>
#include <time.h>

#include "unlocker.h"



#define HOURS_TO_SECONDS (60 * 60) 

char  envEW_LOG[APP_MAXPATH+8]; 

/* Globals to set from configuration file
 ****************************************/
/* Database connection things */
char     DBuser[APP_MAXWORD];           /* UserId to connect to database as  */
char     DBpassword[APP_MAXWORD];       /* Password to datasource            */
char     DBservice[APP_MAXWORD];        /* DBMS instance to interact with    */

/* Data Selection Vars*/
time_t tDeltaUnlocking;

/* Misc vars */
time_t   tNow;  /* Current time at start of program */
int      DEBUG = FALSE;
int      MaxRequestsToHandle=1000;


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

   double   dTemp;

   /* Set to zero one init flag for each required command 
   *****************************************************/   
   for( i=0; i<ncommand; i++ )  init[i] = 0;
   
   /* Open the main configuration file 
   **********************************/
   nfiles = k_open( configfile ); 
   if ( nfiles == 0 ) {
     fprintf(stderr,"ReadConfig: Error opening command file %s; exiting!\n<br>\n", 
       configfile );
     return(EW_FAILURE);
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
       if( com[0] == '@' ) 
       {
         success = nfiles+1;
         nfiles  = k_open(&com[1]);
         if ( nfiles != success ) {
           fprintf(stderr,"ReadConfig: Error opening command file %s; exiting!\n<br>\n",
             &com[1] );
           return(EW_FAILURE);
         }
         continue;
       }
       
       /* Process anything else as a command 
       ************************************/

       
       /*0*/     
       if ( k_its("DBuser") ) 
       {
         str = k_str();
         if( strlen(str) < APP_MAXWORD ) 
         {
           strcpy( DBuser, str );
         } 
         else 
         {
           fprintf(stderr,"ReadConfig: Bad DBuser command in %s:\n"
             "            username \"%s\" too long; maxchar=%d; exiting!\n<br>\n",
             configfile, str, APP_MAXWORD );
           return(EW_FAILURE);
         }
         init[0] = 1;
       }
       

       /*1*/
       else if( k_its("DBpassword") ) 
       {
         str = k_str();
         if( strlen(str) < APP_MAXWORD ) 
         {
           strcpy( DBpassword, str );
         } 
         else 
         {
           fprintf(stderr,"ReadConfig: Bad DBpassword command in %s:\n"
             "            passwd \"%s\" too long; maxchar=%d; exiting!\n<br>\n",
             configfile, str, APP_MAXWORD );
           return(EW_FAILURE);
         }
       init[1] = 1;
       }
       

       /*2*/ 
       else if( k_its("DBservice") ) 
       {
         str = k_str();
         if( strlen(str) < APP_MAXWORD ) 
         {
           strcpy( DBservice, str );
         } else 
         {
           fprintf(stderr,"ReadConfig: Bad DBservice command in %s:\n"
             "            string \"%s\" too long; maxchar=%d; exiting!\n<br>\n",
             configfile, str, APP_MAXWORD );
           return(EW_FAILURE);
         }
         init[2] = 1;
       }
       

  /*3*/  
       else if( k_its("Logfiledir") ) {
         str = k_str();
         if( strlen(str) < APP_MAXPATH )
         {
           sprintf(envEW_LOG, "EW_LOG=%s", str );
           if( putenv( envEW_LOG ) != 0 )  /*set environment variable for logit*/
           {
             fprintf(stderr,"ReadConfig: putenv: unable to set "
               "EW_LOG environment variable; exiting!\n<br>\n" );
             return(EW_FAILURE);
           }
         } else {
           fprintf(stderr,"ReadConfig: Bad Logfiledir command in %s:\n"
             "            pathname \"%s\" too long; maxchar=%d; exiting!\n<br>\n",
             configfile, str, APP_MAXPATH );
           return(EW_FAILURE);
         }
         init[3] = 1;
       }
       
  /*4*/  
       /* Read Time of Unlocking
       ****************************/
       else if( k_its("HoursTillUnlocking") )
       {
         /* get Time of Unlocking */
         dTemp = k_val();
         tDeltaUnlocking = (time_t) (0 - (dTemp * HOURS_TO_SECONDS)); 
                                       /* convert days to secs */
         init[4] = 1;
       }


       else if( k_its("MaxRequests") ) {
         MaxRequestsToHandle = k_int();
       }

       else if( k_its("Debug") ) {
         DEBUG = 1;
       }
       
       /* Unknown command
       *****************/ 
       else {
         fprintf(stderr, "ReadConfig: \"%s\" Unknown command in %s.\n<br>\n", 
           com, configfile );
         continue;
       }
       
        /* See if there were any errors processing the command 
         *****************************************************/
            if( k_err() ) {
               fprintf(stderr,"ReadConfig: Bad %s command in %s; exiting!\n<br>\n",
                        com, configfile );
               return(EW_FAILURE);
            }
	}
	nfiles = k_close();
   }

/* After all files are closed, check init flags for missed commands
 ******************************************************************/
   nmiss = 0;
   for ( i=0; i<ncommand; i++ )  if( !init[i] ) nmiss++;
   if ( nmiss ) {
       fprintf(stderr, "unlocker: ReadConfig: ERROR, no " );
       if ( !init[0] )  logit("", "DBuser "         );
       if ( !init[1] )  logit("", "DBservice "      );
       if ( !init[2] )  logit("", "DBpassword "     );
       if ( !init[3] )  logit("", "Logfiledir "     );
       if ( !init[4] )  logit("", "HoursTillUnlocking "     );
       fprintf(stderr, "command(s) in %s; exiting!\n<br>\n", configfile );
       return(EW_FAILURE);
   }

   return(EW_SUCCESS);
}  /* end ReadConfig() */
