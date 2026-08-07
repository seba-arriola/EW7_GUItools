
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: readcfg.c,v 1.9 2004/03/04 18:07:52 dietz Exp $
 *
 *    Revision history:
 *     $Log: readcfg.c,v $
 *     Revision 1.9  2004/03/04 18:07:52  dietz
 *     Added new command ScheduleFeed to allow for variable time intervals
 *     between feeds to shakemap. Replaces DelayFirstFeed and UpdateInterval
 *     commands.
 *
 *     Revision 1.8  2004/02/13 19:35:57  dietz
 *     Added new optional command, SMQueryMethod, to control how strong motion
 *     data is requested from the DBMS.
 *
 *     Revision 1.7  2002/04/12 22:35:21  dietz
 *     Added a new required command, UseEventID, to choose whether binder's
 *     eventid or the DBMS eventid is written to the output files.  Also
 *     changed all fprintf(stderr,) to logit("e",...) so that configuration
 *     errors are written to the log file.
 *
 *     Revision 1.6  2001/07/01 21:55:35  davidk
 *     Cleanup of the Earthworm Database API and the applications that utilize it.
 *     The "ewdb_api" was cleanup in preparation for Earthworm v6.0, which is
 *     supposed to contain an API that will be backwards compatible in the
 *     future.  Functions were removed, parameters were changed, syntax was
 *     rewritten, and other stuff was done to try to get the code to follow a
 *     general format, so that it would be easier to read.
 *
 *     Applications were modified to handle changed API calls and structures.
 *     They were also modified to be compiler warning free on WinNT.
 *
 *     Revision 1.5  2001/05/15 02:15:46  davidk
 *     Moved functions around between the apps, DB API, and DB API INTERNAL
 *     levels.  Renamed functions and files.  Added support for amplitude
 *     magnitude types.  Reformatted makefiles.
 *
 *     Revision 1.4  2001/05/01 20:39:03  dietz
 *     changed to work with strongmotionII msgs and schema
 *
 *     Revision 1.3  2001/03/22 18:14:39  lombard
 *     added MappingFile optional command; changed to use standard macros
 *     for Earthworm logo fields.
 *
 *     Revision 1.2  2001/03/15 00:01:38  dietz
 *     *** empty log message ***
 *
 *     Revision 1.1  2000/02/15 20:20:15  lucky
 *     Initial revision
 *
 *
 */

/*
 * readcfg.c  configures shakemapfeed
 */

#include <stdio.h>
#include <string.h>
#include <earthworm.h>
#include "shakemapfeed.h"

/******************************************************************************
 *  shakemapfeed_config() processes command file(s) using kom.c functions;    *
 *                    exits if any errors are encountered.                    *
 ******************************************************************************/
#define  ncommand  25       /* # of required commands you expect to process   */ 

void shakemapfeed_config( char *configfile, PARAM *param )
{
   char  init[ncommand];    /* init flags, one byte for each required command */
   int   nmiss;             /* number of required commands that were missed   */
   char *com;
   char *str;
   char  readby[25];
   int   nfiles;
   int   success;
   int   i;

/* Set to zero one init flag for each required command 
 *****************************************************/   
   for( i=0; i<ncommand; i++ )  init[i] = 0;

/* Initialize PARAM structure to all zeros, then set defaults
 ************************************************************/
   memset( param, 0, sizeof(PARAM) );

/* Open the main configuration file 
 **********************************/
   nfiles = k_open( configfile ); 
   if ( nfiles == 0 ) {
        logit( "e",
               "shakemapfeed: Error opening command file <%s>; exiting!\n", 
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
                  logit( "e", 
                         "shakemapfeed: Error opening command file <%s>; exiting!\n",
                         &com[1] );
                  exit( -1 );
               }
               continue;
            }

        /* Process anything else as a command 
         ************************************/
            strcpy( readby, "shakemapfeed_config" );

  /*0*/     if( k_its("LogFile") ) {
                param->LogSwitch = k_int();
                init[0] = 1;
            }
  /*1*/     else if( k_its("MyModuleId") ) {
                str = k_str();
                if(str && strlen(str)<MAX_MOD_STR) strcpy( param->MyModName, str );
                else
                {
                   logit( "e", "shakemapfeed: Bad <MyModName> command in <%s>;"
                          " exiting!\n", configfile );
                   exit( -1 );
                }
                init[1] = 1;
            }
  /*2*/     else if( k_its("RingName") ) {
                str = k_str();
                if(str && strlen(str)<MAX_RING_STR) strcpy( param->RingName, str );
                else
                {
                   logit( "e", "shakemapfeed: Bad <RingName> command in <%s>;"
                          " exiting!\n", configfile );
                   exit( -1 );
                }
                init[2] = 1;
            }
  /*3*/     else if( k_its("HeartBeatInterval") ) {
                param->HeartBeatInterval = k_long();
                init[3] = 1;
            }


         /* Enter installation & module to get event messages from
          ********************************************************/
  /*4*/     else if( k_its("GetEventsFrom") ) {
                int nlogo = param->nLogo;
                if ( nlogo+1 >= MAXLOGO ) {
                    logit( "e", 
                           "shakemapfeed: Too many <Get*From> commands in "
                           "<%s>; max=%d; exiting!\n", configfile, (int)MAXLOGO );
                    exit( -1 );
                }
                if( ( str=k_str() ) ) {
                   if( GetInst(str,&(param->GetLogo[nlogo].instid)) != 0 ) {
                       logit( "e", 
                              "shakemapfeed: Invalid installation name <%s> "  
                              "in <GetEventsFrom> cmd; exiting!\n", str );
                       exit( -1 );
                   }
                }
                if( ( str=k_str() ) ) {
                   if( GetModId(str,&(param->GetLogo[nlogo].mod)) != 0 ) {
                       logit( "e", "shakemapfeed: Invalid module name <%s>"  
                              " in <GetEventsFrom> cmd; exiting!\n", str );
                       exit( -1 );
                   }
                }
                if( GetType("TYPE_HYP2000ARC",&(param->GetLogo[nlogo].type)) != 0 ) {
                    logit( "e", 
                           "shakemapfeed: Invalid message type <TYPE_HYP2000ARC>"
                           "; exiting!\n" );
                    exit( -1 );
                }
             /* logit("e","GetLogo[%d] inst:%d module:%d type:%d\n",
                           nlogo, (int) param->GetLogo[nlogo].instid,
                                  (int) param->GetLogo[nlogo].mod,
                                  (int) param->GetLogo[nlogo].type ); */  /*DEBUG*/
                param->nLogo++;
                init[4] = 1;
            }
  /*5*/     else if( k_its("MagThresh") ) {
                param->MagThresh = (float)k_val();
                init[5] = 1;
            }
  /*6*/     else if( k_its("UpdateInterval") ) {  /* arg in minutes */
                param->Schedule.nsched = 2;
                param->Schedule.tsched[1] = (int)(60.0*k_val()); /* stored as sec */
                init[6] = 1;
            }
  /*7*/     else if( k_its("UpdateDuration") ) {  /* arg in minutes, stored as sec */
                param->UpdateDuration = (int)(60.0*k_val());   /* sec */
                init[7] = 1;
            }
  /*8*/     else if( k_its("LatitudeRange") ) {
            /* Define a box as distance from epicenter */
                param->LatRange = (float)k_val();   /* decimal degrees */
            /* Or define box as the entire region that shakemap could use... 
             *  param->LatMin = (float)k_val();    
             *  param->LatMax = (float)k_val();
             *  if( param->LatMin > param->LatMax ) {
             *    float tmp = param->LatMin;
             *    param->LatMin = param->LatMax;
             *    param->LatMax = tmp;
             *  }
             */
                init[8] = 1;
            }
  /*9*/     else if( k_its("LongitudeRange") ) {
            /* Define a box as distance from epicenter */
                param->LonRange = (float)k_val();   /* decimal degrees */
            /* Or define box as the entire region that shakemap could use... 
             *  param->LonMin = (float)k_val();     
             *  param->LonMax = (float)k_val();
             *  if( param->LonMin > param->LonMax ) {
             *    float tmp = param->LonMin;
             *    param->LonMin = param->LonMax;
             *    param->LonMax = tmp;
             *  }
             */
                init[9] = 1;
            }
  /*10*/    else if( k_its("OutputDir") ) {
                str = k_str();  
                if(str && strlen(str)<PATHLEN) strcpy( param->OutputDir, str );
                else
                {
                   logit( "e", "shakemapfeed: Bad <OutputDir> command in <%s>;"
                          " exiting!\n", configfile );
                   exit( -1 );
                }
                init[10] = 1;
            }
  /*11*/    else if( k_its("DBservice") ) {
                str = k_str();  
                if(str && strlen(str)<WORDLEN) strcpy( param->DBservice, str );
                else
                {
                   logit( "e", "shakemapfeed: Bad <DBservice> command in <%s>;"
                          " exiting!\n", configfile );
                   exit( -1 );
                }
                init[11] = 1;
            }
  /*12*/    else if( k_its("DBuser") ) {
                str = k_str();  
                if(str && strlen(str)<WORDLEN) strcpy( param->DBuser, str );
                else
                {
                   logit( "e", "shakemapfeed: Bad <DBuser> command in <%s>;"
                          " exiting!\n", configfile );
                   exit( -1 );
                }
                init[12] = 1;
            }
  /*13*/    else if( k_its("DBpassword") ) {
                str = k_str();  
                if(str && strlen(str)<WORDLEN) strcpy( param->DBpassword, str );
                else
                {
                   logit( "e", "shakemapfeed: Bad <DBpassword> command in <%s>;"
                          " exiting!\n", configfile );
                   exit( -1 );
                }
                init[13] = 1;
            }
  /*14*/    else if( k_its("Debug") ) {
                param->Debug = k_int();
                init[14] = 1;
            }

  /*15*/    else if( t_com() ) {   /* lay commands; velocity model */   
                strcpy( readby, "t_com" ); 
                init[15] = 1;
            } 

  /*16*/    else if( k_its("DefaultTimeTolerance") ) {     
                param->DefaultTolerance = k_val();
                init[16] = 1;
            }

  /*17*/    else if( k_its("MaxDataPerEvent") ) {     
                param->MaxDataPerEvent = k_int();
                init[17] = 1;
            }

  /*18*/    else if( k_its("LogShakemapFile") ) {     
                param->LogShakemapFile = k_int();
                init[18] = 1;
            }

  /*19*/    else if( k_its("ShakemapFormat") ) {     
                param->ShakemapFormat = k_int();
                if( param->ShakemapFormat!=1 && param->ShakemapFormat!=2 ) {
                   logit( "e", "shakemapfeed: Invalid ShakemapFormat %d ;"
                          " (valid values = 1,2); exiting!\n",
                          param->ShakemapFormat );
                   exit( -1 );
                }
                init[19] = 1;
            }
  /*20*/    else if( k_its("DelayFirstFeed") ) {  /* arg in seconds */  
                param->Schedule.nsched = 2;
                param->Schedule.tsched[0] = (int)k_val(); /* stored as seconds */
                init[20] = 1;
            }

  /*21*/    else if( k_its("MagWeightThresh") ) {
                param->MagWtThresh = (float)k_val();
                init[21] = 1;
            }

  /*22*/    else if( k_its("TempDir") ) {
                str = k_str();  
                if(str && strlen(str)<PATHLEN) strcpy( param->TempDir, str );
                else
                {
                   logit( "e", "shakemapfeed: Bad <TempDir> command in <%s>;"
                          " exiting!\n", configfile );
                   exit( -1 );
                }
                init[22] = 1;
            }

  /*23*/    else if ( k_its("QuakeAuthor") ) {
                if( (str = k_str()) != NULL ) {
                  param->QuakeAuthor = strdup( str );
                  if( param->QuakeAuthor == NULL) {
                    logit( "e", "shakemapfeed: out of memory for <QuakeAuthor>\n");
                    exit( -1 );
                  }
                  init[23] = 1;
                }
            }

  /*24*/   else if ( k_its("UseEventID") ) {
                param->UseEventID = k_int();
                if( param->UseEventID != USE_BINDERID  &&
                    param->UseEventID != USE_DBMSID ) {
                  logit( "e", "shakemapfeed: Invalid UseEventID %d ;"
                         " (valid values = %d,%d); exiting!\n",
                         USE_BINDERID,USE_DBMSID );
                  exit( -1 );
                }
                init[24] = 1;
            }

  /*opt*/   else if( k_its("GetMagnitudesFrom") ) {
                int nlogo = param->nLogo;
                if ( nlogo+1 >= MAXLOGO ) {
                    logit( "e", 
                           "shakemapfeed: Too many <Get*From> commands in "
                           "<%s>; max=%d; exiting!\n", configfile, (int)MAXLOGO );
                    exit( -1 );
                }
                if( ( str=k_str() ) ) {
                   if( GetInst(str,&(param->GetLogo[nlogo].instid)) != 0 ) {
                       logit( "e", 
                              "shakemapfeed: Invalid installation name <%s> "  
                              "in <GetMagnitudesFrom> cmd; exiting!\n", str );
                       exit( -1 );
                   }
                }
                if( ( str=k_str() ) ) {
                   if( GetModId(str,&(param->GetLogo[nlogo].mod)) != 0 ) {
                       logit( "e", "shakemapfeed: Invalid module name <%s>"  
                              " in <GetMagnitudesFrom> cmd; exiting!\n", str );
                       exit( -1 );
                   }
                }
                if( GetType("TYPE_MAGNITUDE",&(param->GetLogo[nlogo].type)) != 0 ) {
                    logit( "e", 
                           "shakemapfeed: Invalid message type <TYPE_MAGNITUDE>"
                           "; exiting!\n" );
                    exit( -1 );
                }
             /* logit("e","GetLogo[%d] inst:%d module:%d type:%d\n",
                           nlogo, (int) param->GetLogo[nlogo].instid,
                                  (int) param->GetLogo[nlogo].mod,
                                  (int) param->GetLogo[nlogo].type ); */  /*DEBUG*/
                param->nLogo++;
            }

  /*opt*/   else if( k_its("GetEQDeletesFrom") ) {
                int nlogo = param->nLogo;
                if ( nlogo+1 >= MAXLOGO ) {
                    logit( "e", 
                           "shakemapfeed: Too many <Get*From> commands in "
                           "<%s>; max=%d; exiting!\n", configfile, (int)MAXLOGO );
                    exit( -1 );
                }
                if( ( str=k_str() ) ) {
                   if( GetInst(str,&(param->GetLogo[nlogo].instid)) != 0 ) {
                       logit( "e", 
                              "shakemapfeed: Invalid installation name <%s> "  
                              "in <GetEQDeletesFrom> cmd; exiting!\n", str );
                       exit( -1 );
                   }
                }
                if( ( str=k_str() ) ) {
                   if( GetModId(str,&(param->GetLogo[nlogo].mod)) != 0 ) {
                       logit( "e", "shakemapfeed: Invalid module name <%s>"  
                              " in <GetEQDeletesFrom> cmd; exiting!\n", str );
                       exit( -1 );
                   }
                }
                if( GetType("TYPE_EQDELETE",&(param->GetLogo[nlogo].type)) != 0 ) {
                    logit( "e", 
                           "shakemapfeed: Invalid message type <TYPE_EQDELETE>"
                           "; exiting!\n" );
                    exit( -1 );
                }
             /* logit("e","GetLogo[%d] inst:%d module:%d type:%d\n",
                           nlogo, (int) param->GetLogo[nlogo].instid,
                                  (int) param->GetLogo[nlogo].mod,
                                  (int) param->GetLogo[nlogo].type ); */  /*DEBUG*/
                param->nLogo++;
            }

  /*opt*/   else if( k_its("BlockComponent") ) {
                COMPONENT *tmp;
                tmp = (COMPONENT *) realloc( param->BlockComp,
                                             (param->nBlock+1)*sizeof(COMPONENT) );
                if( tmp == NULL ) {
                   logit( "e", "shakemapfeed: Error allocating BlockComp memory;"
                          " exiting!\n" );
                   exit( -1 );
                }
                param->BlockComp = tmp;
                str = k_str();
                if(str && strlen(str)<=TRACE_CHAN_LEN) {
                   strcpy( param->BlockComp[param->nBlock], str );
                   param->nBlock++;
                } else {
                   logit( "e", "shakemapfeed: Bad <BlockComponent> command" 
                          " (code <%s> too long) in <%s>; exiting!\n", 
                          str, configfile );
                   exit( -1 );
                }
            /*  for( ib=0;ib<param->nBlock;ib++ )                              */ /*DEBUG*/
            /*    logit("e",("BlockComp %d : %s\n", ib, param->BlockComp[ib] );*/ /*DEBUG*/
            }

  /*opt*/   else if( k_its("NetTimeTolerance") ) {
                TIMETOLERANCE *tmp;
                tmp = (TIMETOLERANCE *) realloc( param->NetSlop,
                                             (param->nNetSlop+1)*sizeof(TIMETOLERANCE ) );
                if( tmp == NULL ) {
                   logit( "e", "shakemapfeed: Error allocating NetTimeTolerance"
                          " memory; exiting!\n" );
                   exit( -1 );
                }
                param->NetSlop = tmp;
                str = k_str();
                if(str && strlen(str)<=TRACE_NET_LEN) {
                   strcpy( param->NetSlop[param->nNetSlop].net, str );
                   param->NetSlop[param->nNetSlop].ttolerance = k_val();
                   param->nNetSlop++;
                } else {
                   logit( "e", "shakemapfeed: Bad <NetTimeTolerance> command" 
                          " (net <%s> too long) in <%s>; exiting!\n", 
                          str, configfile );
                   exit( -1 );
                }
            /*  for( inet=0;inet<param->nNetSlop;inet++ )             */ /*DEBUG*/
            /*    logit("e",("NetTimeTolerance %d : %s %.2lf\n", inet,*/
            /*                param->NetSlop[inet].net,               */
            /*                param->NetSlop[inet].ttolerance );      */ /*DEBUG*/
            }

  /*opt*/   else if( k_its("PageOnShakemapFeed") ) {
                if( param->nPage >= MAXPAGE ) {
                   logit( "e", "shakemapfeed: too many PageOnShakemapFeed "
                          "cmds (max=%d); exiting!\n", MAXPAGE );
                   exit( -1 );
                }
                str = k_str();
                if( str  &&  strlen(str)<WORDLEN ) {
                   strcpy( param->Page[param->nPage], str);
                   param->nPage++;
                } else {
                   logit( "e", "shakemapfeed: Invalid PageOnShakemapFeed "
                          "arg <%s>; must be 1-%d chars; exiting!\n",
                           str, WORDLEN-1 );
                   exit( -1 );
                }
            }
 
  /*opt*/   else if ( k_its("MappingFile") ) {
                if ( (str = k_str()) != NULL) {
                  param->MappingFile = strdup( str );
                  if (param->MappingFile == NULL) {
                    logit( "e", "shakemapfeed: out of memory for <MappingFile>;"
                           " exiting!\n");
                    exit( -1 );
                  }
                }
            }

  /*opt*/   else if( k_its("SMQueryMethod") ) {
                param->SMQueryMethod = k_int();
                init[18] = 1;
            }

  /*opt*/   else if( k_its("ScheduleFeed") ) { /* arg in min, stored as sec */
                if( param->Schedule.nsched >= MAXSCHED ) {
                   logit( "e", "shakemapfeed: too many ScheduleFeed cmds (max=%d);"
                          " exiting!\n", MAXSCHED );
                   exit( -1 );
                }
                param->Schedule.tsched[param->Schedule.nsched] = (time_t)(60*k_val());
                param->Schedule.nsched++;
                init[6]  = 1;  /* Replaces UpdateInterval command */
                init[20] = 1;  /* Replaces DelayFirstFeed command */
            }

         /* Unknown command
          *****************/ 
            else {
                logit( "e", "shakemapfeed: <%s> Unknown command in <%s>;"
                       " exiting!\n",  com, configfile );
                exit( -1 );
            }

        /* See if there were any errors processing the command 
         *****************************************************/
            if( k_err() ) {
               logit( "e", 
                      "shakemapfeed: Bad <%s> command for %s() in <%s>; exiting!\n",
                       com, readby, configfile );
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
       logit( "e", "shakemapfeed: ERROR, no " );
       if ( !init[0] )  logit( "e", "<LogFile> "           );
       if ( !init[1] )  logit( "e", "<MyModuleId> "        );
       if ( !init[2] )  logit( "e", "<RingName> "          );
       if ( !init[3] )  logit( "e", "<HeartBeatInterval> " );
       if ( !init[4] )  logit( "e", "<GetEventsFrom> "     );
       if ( !init[5] )  logit( "e", "<MagThresh> "         );
       if ( !init[6] )  logit( "e", "<UpdateInterval> "    );
       if ( !init[7] )  logit( "e", "<UpdateDuration> "    );
       if ( !init[8] )  logit( "e", "<LatitudeRange> "     );
       if ( !init[9] )  logit( "e", "<LongitudeRange> "    );
       if ( !init[10] ) logit( "e", "<OutputDir> "         );
       if ( !init[11] ) logit( "e", "<DBservice> "         );
       if ( !init[12] ) logit( "e", "<DBuser> "            );
       if ( !init[13] ) logit( "e", "<DBpassword> "        );
       if ( !init[14] ) logit( "e", "<Debug> "             );
       if ( !init[15] ) logit( "e", "<lay> "               );
       if ( !init[16] ) logit( "e", "<DefaultTimeTolerance> " );
       if ( !init[17] ) logit( "e", "<MaxDataPerEvent> "   );
       if ( !init[18] ) logit( "e", "<LogShakemapFile> "   );
       if ( !init[19] ) logit( "e", "<ShakemapFormat> "    );
       if ( !init[20] ) logit( "e", "<DelayFirstFeed> "    );
       if ( !init[21] ) logit( "e", "<MagWeightThresh> "   );
       if ( !init[22] ) logit( "e", "<TempDir> "           );
       if ( !init[23] ) logit( "e", "<QuakeAuthor> "       );
       if ( !init[24] ) logit( "e", "<UseEventID> "        );
       logit( "e", "command(s) in <%s>; exiting!\n", configfile );
       exit( -1 );
   }

   return;
}

/******************************************************************************
 *  shakemapfeed_lookup( )   Look up important info from earthworm.h tables   *
 ******************************************************************************/
void shakemapfeed_lookup( PARAM *param, EWD *ewd )
{
/* Initialize EWD structure to all zeros
 ***************************************/
   memset( ewd, 0, sizeof(EWD) );

/* Look up keys to shared memory regions
   *************************************/
   if( ( ewd->RingKey = GetKey(param->RingName) ) == -1 ) {
        logit( "e", "shakemapfeed:  Invalid ring name <%s>; exiting!\n", 
               param->RingName);
        exit( -1 );
   }

/* Look up installations of interest
   *********************************/
   if ( GetLocalInst( &(ewd->InstId) ) != 0 ) {
      logit( "e", 
             "shakemapfeed: error getting local installation id; exiting!\n" );
      exit( -1 );
   }

/* Look up modules of interest
   ***************************/
   if ( GetModId( param->MyModName, &(ewd->MyModId) ) != 0 ) {
      logit( "e", "shakemapfeed: Invalid module name <%s>; exiting!\n", 
      param->MyModName );
      exit( -1 );
   }

/* Look up message types of interest
   *********************************/
   if ( GetType( "TYPE_HEARTBEAT", &(ewd->TypeHeartBeat) ) != 0 ) {
      logit( "e", 
             "shakemapfeed: Invalid message type <TYPE_HEARTBEAT>; exiting!\n" );
      exit( -1 );
   }
   if ( GetType( "TYPE_ERROR", &(ewd->TypeError) ) != 0 ) {
      logit( "e", 
             "shakemapfeed: Invalid message type <TYPE_ERROR>; exiting!\n" );
      exit( -1 );
   }
   if ( GetType( "TYPE_HYP2000ARC", &(ewd->TypeHyp2000Arc) ) != 0 ) {
      logit( "e", 
             "shakemapfeed: Invalid message type <TYPE_HYP2000ARC>; exiting!\n" );
      exit( -1 );
   }
   if ( GetType( "TYPE_STRONGMOTION", &(ewd->TypeSM) ) != 0 ) {
      logit( "e", 
             "shakemapfeed: Invalid message type <TYPE_STRONGMOTION>; exiting!\n" );
      exit( -1 );
   }
   if ( GetType( "TYPE_PAGE", &(ewd->TypePage) ) != 0 ) {
      logit( "e", 
             "shakemapfeed: Invalid message type <TYPE_PAGE>; exiting!\n" );
      exit( -1 );
   }
   if ( GetType( "TYPE_EQDELETE", &(ewd->TypeEQDelete) ) != 0 ) {
      logit( "e", 
             "shakemapfeed: Invalid message type <TYPE_EQDELETE>; exiting!\n" );
      exit( -1 );
   }
   if ( GetType( "TYPE_MAGNITUDE", &(ewd->TypeMagnitude) ) != 0 ) {
      logit( "e", 
             "shakemapfeed: Invalid message type <TYPE_MAGNITUDE>; exiting!\n" );
      exit( -1 );
   }
   return;
} 
