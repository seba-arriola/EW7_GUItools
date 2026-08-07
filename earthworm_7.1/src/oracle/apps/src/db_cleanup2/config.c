
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: config.c,v 1.1 2001/07/14 07:39:19 davidk Exp $
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

#include "db_cleanup.h"



#define DAYS_TO_SECONDS 60 * 60 * 24

char  envEW_LOG[APP_MAXPATH+8]; 

/* Globals to set from configuration file
 ****************************************/
/* Database connection things */
char     DBuser[APP_MAXWORD];           /* UserId to connect to database as  */
char     DBpassword[APP_MAXWORD];       /* Password to datasource            */
char     DBservice[APP_MAXWORD];        /* DBMS instance to interact with    */

/* Archiving Vars*/
char szBaseArchiveDirectory[APP_MAXPATH];
char szOutputFormat[APP_MAXWORD];
int  bSetArchiveFlag = FALSE;
int  iLargestSnippetSize = 204800;
int  bSetArchiveFlag;

/* Unassociated Data Vars*/
int    bHandleOtherData = FALSE;
int    bSaveOtherData   = FALSE;
int    bDeleteOtherData = FALSE;
time_t tOtherDataCutoffTime;

/* Misc vars */
time_t   tNow;  /* Current time at start of program */
int      DEBUG = FALSE;
int      MaxEventsToHandle = 1000;
int      iNumEventRules    = 0;

EventRuleStruct EventRules[APP_MAXRULES];



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

   double   dTemp;
   char *   szTemp;
   char *   pTmp;

   /* Set to zero one init flag for each required command 
   *****************************************************/   
   for( i=0; i<ncommand; i++ )  init[i] = 0;
   
   /* Open the main configuration file 
   **********************************/
   nfiles = k_open( configfile ); 
   if ( nfiles == 0 ) {
     logit("e","ReadConfig: Error opening command file %s; exiting!\n<br>\n", 
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
           logit("e","ReadConfig: Error opening command file %s; exiting!\n<br>\n",
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
           logit("e","ReadConfig: Bad DBuser command in %s:\n"
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
           logit("e","ReadConfig: Bad DBpassword command in %s:\n"
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
           logit("e","ReadConfig: Bad DBservice command in %s:\n"
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
             logit("e","ReadConfig: putenv: unable to set "
               "EW_LOG environment variable; exiting!\n<br>\n" );
             return(EW_FAILURE);
           }
         } else {
           logit("e","ReadConfig: Bad Logfiledir command in %s:\n"
             "            pathname \"%s\" too long; maxchar=%d; exiting!\n<br>\n",
             configfile, str, APP_MAXPATH );
           return(EW_FAILURE);
         }
         init[3] = 1;
       }
       
       /*4*/ 
       else if( k_its("OutDir") ) 
       {
         str = k_str();
         if( strlen(str) < APP_MAXPATH ) 
         {
           strcpy( szBaseArchiveDirectory, str );
         }
         else
         {
           logit("e","ReadConfig: Bad OutDir command in %s:\n"
             "            pathname \"%s\" too long; maxchar=%d; exiting!\n<br>\n",
             configfile, str, APP_MAXPATH );
           return(EW_FAILURE);
         }
         init[4] = 1;
       }
       

       /*5*/
       else if( k_its("OutputFormat") ) 
       {
         str = k_str();
         if( strlen(str) < APP_MAXWORD ) 
         {
           strcpy(szOutputFormat, str );
         }
         else 
         {
           logit("e","ReadConfig: Bad OutputFormat command in %s:\n"
             "            passwd \"%s\" too long; maxchar=%d; exiting!\n<br>\n",
             configfile, str, APP_MAXWORD );
           return(EW_FAILURE);
         }
         init[5] = 1;
       }

       
       /************************* OPTIONAL LINES **************************/

       /* Read Event Rule Descriptor Line
       ****************************/
       else if( k_its("EVENT") )
       {
         if( iNumEventRules == 0)
         {
           /* Do Nothing special */
         }
         
         if( iNumEventRules >= APP_MAXRULES) 
         {
           logit("et","db_cleanup: More than %d EVENT rule lines in config file. "
                      "Exiting!\n",APP_MAXRULES);
           return(EW_FAILURE);
         }

         /* memset the current rule struct */
         memset(&(EventRules[iNumEventRules]),0,sizeof(EventRuleStruct));
         strcpy(EventRules[iNumEventRules].EVMin.Event.szSource, "*");


         /* Get SAVE and DELETE columns */
         EventRules[iNumEventRules].iDataToSave   = k_int();
         EventRules[iNumEventRules].iDataToDelete = k_int();
         
         /* Get Min/Max Days Old columns */
         /* Min and Max get reversed, because the config file talks about
         days old, and what we care about in here is dates, so days from
         1970 */
         dTemp = k_val();
         if(dTemp == 0)
         {
           EventRules[iNumEventRules].tStart = 0;
         }
         else
         {
           EventRules[iNumEventRules].tStart = tNow - (int)
                                               (dTemp * DAYS_TO_SECONDS); 
                                               /* convert days to secs */
         }
           
         dTemp = k_val();
         if(dTemp == 0)
         {
           EventRules[iNumEventRules].tEnd = 0;
         }
         else
         {
           EventRules[iNumEventRules].tEnd = tNow - (int)
                                               (dTemp * DAYS_TO_SECONDS); 
                                               /* convert days to secs */
         }
         
         /* Get Min/Max Mag columns */
         EventRules[iNumEventRules].EVMax.dPrefMag = (float)k_val();
         EventRules[iNumEventRules].EVMin.dPrefMag = (float)k_val();
         
         /* Get Lat/Lon column */
         szTemp = k_str();
         if(!szTemp)
         {
           /* some sort of error along the line! */
           logit("et","db_cleanup: Error parsing EVENT line(%d): Lat/Lon box "
                      "in Config File. Quitting!\n",
                 iNumEventRules + 1);
           return(EW_FAILURE);
         }
         else
         {
           if(!(strcmp("0", szTemp) == 0))
           {
             /* we think we got a lat lon box */
             /* Get MinLat */
             pTmp = strchr(szTemp,'/');
             if(!pTmp)
             {
               logit("et","db_cleanup: Error parsing EVENT line(%d): MinLat"
                          "in Config File. Quitting!\n",
                     iNumEventRules + 1);
               return(EW_FAILURE);
             }
             else
             {
               *pTmp = 0x00;
               EventRules[iNumEventRules].EVMin.dLat = (float) atof(szTemp);
             }
             szTemp = pTmp + 1;
             
             /* Get MinLon */
             pTmp = strchr(szTemp,'/');
             if(!pTmp)
             {
               logit("et","db_cleanup: Error parsing EVENT line(%d): MinLon"
                          "in Config File. Quitting!\n",
                     iNumEventRules + 1);
               return(EW_FAILURE);
             }
             else
             {
               *pTmp = 0x00;
               EventRules[iNumEventRules].EVMin.dLon = (float) atof(szTemp);
             }
             szTemp = pTmp + 1;
             
             /* Get MaxLat */
             pTmp = strchr(szTemp,'/');
             if(!pTmp)
             {
               logit("et","db_cleanup: Error parsing EVENT line(%d): MaxLat"
                          "in Config File. Quitting!\n",
                     iNumEventRules + 1);
               return(EW_FAILURE);
             }
             else
             {
               *pTmp = 0x00;
               EventRules[iNumEventRules].EVMax.dLat = (float) atof(szTemp);
             }
             szTemp = pTmp + 1;
             
             /* Get MaxLon */
             EventRules[iNumEventRules].EVMax.dLon = (float) atof(szTemp);
             
             /* We've got the box, now validate it */
             if(!(EventRules[iNumEventRules].EVMin.dLat < EventRules[iNumEventRules].EVMax.dLat && 
                  EventRules[iNumEventRules].EVMin.dLon < EventRules[iNumEventRules].EVMax.dLon))
             {
               logit("et","db_cleanup: Error handling EVENT line(%d): "
                          "Invalid Lat/Lon Box: (MinLat=%f, MaxLat=%f, "
                          "MinLon=%f, MaxLon=%f) in Config File. Quitting!\n",
                     iNumEventRules + 1,
                     EventRules[iNumEventRules].EVMin.dLat,
                     EventRules[iNumEventRules].EVMax.dLat,
                     EventRules[iNumEventRules].EVMin.dLon,
                     EventRules[iNumEventRules].EVMax.dLon
                    );
               return(EW_FAILURE);
             }
           }  /* if we think we got a lat/lon box */
           else
           {
             /* we got just a plain old zero.  No need to do anything */
           }
         }  /* else bad parse for Lat/Lon box failed */
         
         /* Get Dubiocity column */
         EventRules[iNumEventRules].iDubiocity = k_int();

         /* successfully parsed the rule, now increment the Rules ctr */
         iNumEventRules++;
       }  /* end if its EVENT */
      
       /* Read Event Rule Descriptor Line
       ****************************/
       else if( k_its("OTHER_DATA") )
       {
         if(bHandleOtherData)
         {
           logit("et","db_cleanup: ERROR: More than one OTHER_DATA line "
                      "in config file.  Only one is allowed Returning!\n");
           return(EW_FAILURE);
         }
         
         bHandleOtherData = TRUE;

         /* get SAVE and DELETE columns */
         bSaveOtherData   = k_int();
         bDeleteOtherData = k_int();

         /* get Min_Days column */
         dTemp = k_val();
         tOtherDataCutoffTime = (time_t) (tNow - (dTemp * DAYS_TO_SECONDS)); 
                                                 /* convert days to secs */
       }

       else if( k_its("TraceBufferLen") )
       {
         iLargestSnippetSize = k_int() * 1024; /* convert from kilobytes to bytes */
       }

       else if( k_its("SetArchiveFlag") )
       {
         bSetArchiveFlag = k_int();
       }
       
       else if( k_its("Debug") ) {
         DEBUG = 1;
       }
       
       /* Unknown command
       *****************/ 
       else {
         logit("e", "ReadConfig: \"%s\" Unknown command in %s.\n<br>\n", 
           com, configfile );
         continue;
       }
       
        /* See if there were any errors processing the command 
         *****************************************************/
            if( k_err() ) {
               logit("e","ReadConfig: Bad %s command in %s; exiting!\n<br>\n",
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
       logit("e", "db_cleanup: ReadConfig: ERROR, no " );
       if ( !init[0] )  logit("", "DBuser "         );
       if ( !init[1] )  logit("", "DBservice "      );
       if ( !init[2] )  logit("", "DBpassword "     );
       if ( !init[3] )  logit("", "Logfiledir "     );
       if ( !init[4] )  logit("", "OutDir "     );
       if ( !init[5] )  logit("", "OutputFormat "     );
       logit("e", "command(s) in %s; exiting!\n<br>\n", configfile );
       return(EW_FAILURE);
   }

   return(EW_SUCCESS);
}
