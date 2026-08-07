
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: config.c,v 1.8 2002/06/28 21:06:22 lucky Exp $
 *    Revision history:
 *
 *    $Log: config.c,v $
 *    Revision 1.8  2002/06/28 21:06:22  lucky
 *    Lucky's pre-departure checkin. Most changes probably had to do with bug fixes
 *    in connection with the GIOC scaffold.
 *
 *    Revision 1.7  2001/09/26 21:40:48  lucky
 *     Strong motion modifications;
 *    Modifications to allow the event list to be broken up
 *    into smaller number of events;
 *     Allow separate maximum event counts - one for the map, another
 *     for the list;
 *
 *    Revision 1.6  2001/02/28 17:29:10  lucky
 *    Massive schema redesign and cleanup.
 *
 *    Revision 1.5  2001/02/09 22:34:12  alex
 *    added NoSnippetsNoShow parameter. Alex
 *
 *    Revision 1.4  2000/08/09 16:21:40  lucky
 *    Lint cleanup
 *
 *    Revision 1.3  2000/08/07 19:39:54  lucky
 *    Added WebHost option and using html_trailer from libsrc
 *
 *    Revision 1.2  1999/11/09 16:53:02  lucky
 *    *** empty log message ***
 *
 *    Revision 1.1  1999/10/25 18:52:32  davidk
 *    Initial revision
 *
 *    Revision 1.4  1999/09/24 06:13:30  davidk
 *    added config file params for maxevents,numberofdays,default click effect.  Done in conjunction
 *    with the change in getlist, to do away with the get_events.html page, which means incorporating
 *    criteria into the getlist program via the config file and html-displayed page
 *
 *    Revision 1.2  1999/06/23 22:43:40  lucky
 *    Added Debug flag to the config file
 *
 *    Revision 1.1  1999/05/05 18:19:24  lucky
 *    Initial revision
 *
 *
 */
  
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <kom.h>

/*************************************************************************
  This is for the most part standard config file reading code, adapted
  from earthworm config files.  It uses the kom.c libraries from 
  earthworm (or wherever earthworm may have lifted them from).  The
  only really different code is near the bottom where we do all kinds
  of wierd stuff to process the map tree.  Davidk 19990419 
*************************************************************************/

/* include our program header files to get neccessary definitions and
   structs and junk. 
********************/
#include <map_display_structs.h>
#include "getlist.h"

/* Config parsing constants.  These should have more unique names
   as some OSes use words like MAXWORD   HA! we're using EWDB_MAXWORD now!
********************/
#define EWDB_MAXWORD 12

/* External global variables, including the logfile name, 
   DB login names, Module ID (left over from EW), and
   a logging switch, all of which are to be read in from
   the config file.
**********************/
int		DEBUG;

extern char BackgroundColor[];
extern char HeaderLogo[];
extern char FooterLogo[];
extern char envEW_LOG[];
extern char DBservice[],DBpassword[],DBuser[], WebHost[];
extern int MyModID,LogSwitch;
extern int MinNumStasToShow;
extern int EventsPerPage;
extern int ShowPickCol;
extern int ShowStasCol;
extern int ShowTraceCol;
extern int ShowAlarmsCol;
extern int ShowMergesCol;
extern int ShowCoincidenceLink;
/* The root Map, which all others are hung from (maybe). */
extern EWDBMapStruct MapRoot;
extern char szDefaultMapID[MAX_MAPID_LENGTH];

extern SetVarsUserStruct DefaultParams;
/* End external Globals */

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

   /*  "MAP" Specific Variables */
   EWDBMapStruct * pMap, * pMapCurr;
   EWDBMapDataStruct * pMapData;
   char szMapID[20];
   char * pUseFocalPoint;

   int MapNum,MapNumNext;
   char * pPeriod, *pCurr;
   /*  End of "MAP" Specific Variables */


/* Set to zero one init flag for each required command 
 *****************************************************/   
   for( i=0; i<ncommand; i++ )  init[i] = 0;

	MinNumStasToShow = 0;
	EventsPerPage = -1;
	ShowPickCol = FALSE;
	ShowStasCol = FALSE;
	ShowTraceCol = FALSE;
	ShowAlarmsCol = FALSE;
	ShowMergesCol = FALSE;
	ShowCoincidenceLink = FALSE;
    strcpy (BackgroundColor, "notset");
    strcpy (HeaderLogo, "notset");
    strcpy (FooterLogo, "notset");

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
                if( strlen(str) < EWDB_MAXWORD ) {
                    strcpy( DBservice, str );
                } else {
                    fprintf( stdout,"ReadConfig: Bad <DBservice> command in <%s>:\n"
                                    "            string <%s> too long; maxchar=%d; exiting!\n",
                             configfile, str, EWDB_MAXWORD );
                    return( -1 );
                }
                init[1] = 1;
            }

  /*2*/     else if( k_its("DBuser") ) {
                str = k_str();
                if( strlen(str) < EWDB_MAXWORD ) {
                    strcpy( DBuser, str );
                } else {
                    fprintf( stdout,"ReadConfig: Bad <DBuser> command in <%s>:\n"
                                    "            username <%s> too long; maxchar=%d; exiting!\n",
                             configfile, str, EWDB_MAXWORD );
                    return( -1 );
                }
                init[2] = 1;
            }

  /*3*/     else if( k_its("DBpassword") ) {
                str = k_str();
                if( strlen(str) < EWDB_MAXWORD ) {
                    strcpy( DBpassword, str );
                } else {
                    fprintf( stdout,"ReadConfig: Bad <DBpassword> command in <%s>:\n"
                                    "            passwd <%s> too long; maxchar=%d; exiting!\n",
                             configfile, str, EWDB_MAXWORD );
                    return( -1 );
                }
                init[3] = 1;
            }
  /*4*/     else if( k_its("WebHost") ) {
                str = k_str();
                if( strlen(str) < MAXPATH ) {
                    strcpy( WebHost, str );
                } else {
                    fprintf( stdout,"ReadConfig: Bad <WebHost> command in <%s>:\n"
                                    "            passwd <%s> too long; maxchar=%d; exiting!\n",
                             configfile, str, MAXPATH );
                    return( -1 );
                }
                init[4] = 1;
            }


         /* Optional commands */
         /*********************/
  
            /* Debug 
            **************/
      else if( k_its("Debug") ) {
                DEBUG = 1;
            }

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

            /* MapPixelWidth 
            **************/
      else if( k_its("MapPixelWidth") ) 
	  {
		  WebParams.LPSData.PixelWidth = k_int();
	  }

            /* MapPixelHeight 
            **************/
      else if( k_its("MapPixelHeight") ) 
	  {
		  WebParams.LPSData.PixelHeight = k_int();
	  }

            /* MaxStationDisplayWidth 
            **************/
      else if( k_its("MaxStationDisplayWidth") ) 
	  {
		  StationDisplayWidth  = (float)k_val();
	  }

            /* MaxNumOfStationsDisplayed 
            **************/
      else if( k_its("MaxNumOfStationsDisplayed") ) 
	  {
		  GetStationListBufferSize  = k_int();
	  }

      /* Which Columns to Show 
      ****************************/

      else if( k_its("ShowPickCol") ) 
		  ShowPickCol  = TRUE;
      else if( k_its("ShowStasCol") ) 
		  ShowStasCol  = TRUE;
      else if( k_its("ShowTraceCol") ) 
		  ShowTraceCol  = TRUE;
      else if( k_its("ShowAlarmsCol") ) 
		  ShowAlarmsCol  = TRUE;
      else if( k_its("ShowMergesCol") ) 
		  ShowMergesCol  = TRUE;
      else if( k_its("ShowCoincidenceLink") ) 
		  ShowCoincidenceLink  = TRUE;

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

            /* DefaultMapID 
            **************/
      else if( k_its("DefaultMapID") ) 
	  {
		  strcpy(szDefaultMapID,k_str());
	  }

            /* DefaultClickEffect 
            **************/
      else if( k_its("DefaultClickEffect") ) 
	  {
		  DefaultParams.WGSSData.ClickEffect  = k_int();
	  }

            /* MaxNumOfEventsDisplayed 
            **************/
      else if( k_its("MaxNumOfEventsDisplayed") ) 
	  {
		  DefaultParams.ECSData.MaxEventsDisp  = k_int();
	  }
      else if( k_its("MaxNumOfEventsRetrieved") ) 
	  {
		  DefaultParams.ECSData.MaxEventsRetr  = k_int();
	  }

            /* NumOfDaysToShow 
            **************/
      else if( k_its("NumOfDaysToShow") ) 
	  {
		  DefaultParams.ECSData.NumberOfDays  = k_int();
	  }

      /* MinNumStasToShow 
       *******************/
      else if( k_its("MinNumStasToShow") ) 
	  {
		  MinNumStasToShow  = k_int();
	  }

      /* EventsPerPage 
       *******************/
      else if( k_its("EventsPerPage") ) 
	  {
		  EventsPerPage  = k_int();
	  }

/**********************************************************************
 OK, here's where we get off the beaten path!!!!!!!!!!!
 We have this thing called a "Map".  It is somewhat like a "Tank"
 variable in a wave_server config file.  This is greek for TROUBLE.
 We use Map to indicate that a line is describing part of a Map Tree.
 We would like to have a bunch of preset maps, and somehow link them
 in a heirarchical tree structure, so the top level is world, and then
 the next level is continents, and then countries, and states, and
 hot spots, and the kwikimart in Springfield...., etc.)

  A typical line with the column headers displayed above looks like
#Map    Num     Name                Lat1  Lon1  Lat2  Lon2      Riv    Pol  L/L Lines Border  Proj   Focal Pt Ctr Lat Ctr Lon
Map     1       Western_Hemisphere   -100     0    90   360        0     0      0     0    a       Y            10    270
#Map     1.3     USA     ......... # other stuff goes here  
#Map     1.3.2   New_Madrid ...... # other stuff goes here
#Map     1.3.2.4 Memphis .... .... # other stuff goes here

 This map is number 1 as opposed to number 1.3.2.4 so it is at the
 top of the heirarchy in the map tree (close to the root).
 Name of the map is Western_Hemisphere # No spaces
 Lat1 -100 degrees # Lat1,Lon1 and Lat2,Lon2 describ the coordinates of the
                   # map unless a focal point is specified, in which the
                   # focal point specifies the center of the map, and the
                   # lats and lons specified are used to determine the 
                   # size only of the map in Lat and Lon terms
 Lon1    0 degrees
 Lat2   90 deg
 Lon2  360 deg
 Riv   a flag that indicates what rivers you want displayed.  See the
       sample getlist.d config file and your mapserver documentation.
 Pol   a flag similar to rivers that specifies political boundaries
 L/L Lines  indicates whether Lat/Lon lines should be drawn on the map.
 Border Indicates whether a fancy border should be drawn.
 Proj  Indicates the projection type used
 FocalPoint indicates if focal point information is supplied on this line(Y/N)
 Ctr Lat (Center Lat) the center of the map in Lat terms
 Ctr Lon (Center Lon) the center of the map in Lon degrees

 all done, hope you are not too confused  DK 19990419
********************************************************************/

            /* Map 
            **************/
      else if( k_its("Map") ) 
	  {
        /* We need to figure out what map this is */
        /* We will assume that maps must be numeric 
           order in the sense that 1.1 cannot come before
           1, and 1.3 can't come before 1.2
        */
        if((pMap=calloc(sizeof(EWDBMapStruct),1)) == NULL)
        {
          fprintf(stdout,"Cannot calloc %d bytes for NewMap %s.\n",
                  sizeof(EWDBMapStruct),k_str());
          return(-1);
        }

        /* set pMapData for programmer centric shortcut */
        pMapData=&(pMap->Map);

        /* Grab everythign up to but not including the Focal Point flag 
        ***************************************************************/
        strcpy(szMapID,k_str());  /* Copy the map # to a temp buffer */
        strcpy(pMapData->MapName,k_str());  /* Get the map name "Cupertino,CA" */
        pMapData->Lat1=(float)atof(k_str());  /*Get starting Lat for map */
        pMapData->Lon1=(float)atof(k_str());  /*Get starting Lon for map */
        pMapData->Lat2=(float)atof(k_str());  /*Get ending   Lat for map */
        pMapData->Lon2=(float)atof(k_str());  /*Get ending   Lon for map */
        pMapData->Rivers=k_int();        /* Get the "Rivers to draw" value */
        pMapData->Politicals=k_int();    /* Get the "Boundaries to draw" value */
        pMapData->bLatLonLines=k_int();  /* Get the "Draw Lat/Lon Lines?" value */
        pMapData->bBorder=k_int();       /* Get the "Draw Borders?" value */
        pMapData->ProjectionType=atoi(k_str());/* Get the map projection */

      /*  printf("Lat1 %4.2f, Lon1 %4.2f, Lat2 %4.2f, Lon2 %4.2f, Pol %d.\n",
               pMapData->Lat1,pMapData->Lon1,pMapData->Lat2,pMapData->Lon2,
               pMapData->Politicals);
      */

        /* Get the focal point flag.
           Get the focal point info iff the flag is Y */
        pUseFocalPoint=k_str();
        if(pUseFocalPoint == NULL)
        {
          fprintf(stdout,"Error encountered while trying "
                  "to read (FocalPoint:Y/N).  Error:EOL.\n");
          return(-1);
        }
        if(pUseFocalPoint[0]=='Y' || pUseFocalPoint[0]=='y')
        {
          /*printf("Using Focal Point due to flag (Y/y).\n<br>\n");*/
          pMapData->CenterLat=(float)atof(k_str());  /* Get Focal Lat */
          pMapData->CenterLon=(float)atof(k_str());  /* Get Focal Lon */
        }
        else
        {
          /*printf("Not Using Focal Point. flag was ##%s##.\n<br>\n",pUseFocalPoint);*/
          pMapData->CenterLat=(pMapData->Lat1 + pMapData->Lat2)/2;  /* Set Focal Lat */
          pMapData->CenterLon=(pMapData->Lon1 + pMapData->Lon2)/2;  /* Set Focal Lon */
        }

        /* Calculate the Latheight and Lonwidth of the map */
        pMapData->LatHeight=pMapData->Lat1 - pMapData->Lat2;
        if(pMapData->LatHeight < 0)
        {
          pMapData->LatHeight=0-pMapData->LatHeight;
        }
        pMapData->LonWidth = pMapData->Lon1 - pMapData->Lon2;
        if(pMapData->LonWidth < 0)
        {
          pMapData->LonWidth=0-pMapData->LonWidth;
        }

        /* Finally the dreadfull task of figuring out where this map goes. */
        /* The number is in szMapID (Example 1.2.1.3) */
        /* Find its place in the Map tree */

        /* initialize pCurr to the beginning of the map id */
        pCurr=szMapID;

        /* initialize pMapCurr to the root map */
        pMapCurr=&MapRoot;

        /* get first number in map id */
        MapNum=atoi(pCurr);

        /* if there was no legit MapNum, throw a temper tantrum */
        if(!(MapNum > 0))
        {
          fprintf(stdout,"Bad Map Number in Map ID %s, for map %s.\n",
                  szMapID,pMap->Map.MapName);
          free(pMap);
          return(-1);
        }

        /* find the first '.' */
        pPeriod=strchr(pCurr,'.');
        pCurr=pPeriod+1;

        /* if period not found set MapNumNext to terminator */
        /* otherwise grab the next Map number */
        if(pPeriod == NULL)
        {
          MapNumNext=-1;
        }
        else
        {
          MapNumNext=atoi(pCurr);
        }
        /* ############### DEBUG ############## 
        ** printf("Traverse at top.\n<br>\n");
        ** printf("MapNum = %d, MapNumNext = %d\n<br>\n",MapNum,MapNumNext);
        ** TraverseMapTree(&MapRoot,"");
          ############# END DEBUG ############ */

        /* while we haven't retrieved all of the numbers from MapID */
        while(MapNum != -1)
        {
          if(pMapCurr->pChildren[MapNum] == NULL)
          {
            if(MapNumNext != -1)
            {
              fprintf(stdout,"ConfigFile: Misordered MapID: %s.\n",  szMapID);
              return(-1);
            }
            /*else*/
            /*We've found a home for this map */
            pMapCurr->pChildren[MapNum]=pMap;
            pMapCurr->NumOfChildren++;
            pMap->pParent=pMapCurr;  /* double link the tree */
          }
          else
          {
            pMapCurr=pMapCurr->pChildren[MapNum];
          }
          MapNum=MapNumNext;
          if(MapNumNext != -1)
          {
            /* find the next '.' */
            pPeriod=strchr(pCurr,'.');
            pCurr=pPeriod+1;

            /* if period not found set MapNumNext to terminator */
            /* otherwise grab the next Map number */
            if(pPeriod == NULL)
              MapNumNext=-1;
            else
              MapNumNext=atoi(pCurr);
          }
        }  /* while MapNum != -1 */


        /* ############### DEBUG ############## 
        ** printf("Traverse at bottom.\n<br>\n");
        ** TraverseMapTree(&MapRoot,"");
         ############# END DEBUG ############ */
    }  /* if( k_its("Map") */

    /* Done with that darn "Map" tag 
    *******************************/

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
}  /* End ReadConfig() */
