
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: htmlreply.c,v 1.8 2000/12/06 17:49:21 lucky Exp $
 *
 *    Revision history:
 *    $Log: htmlreply.c,v $
 *    Revision 1.8  2000/12/06 17:49:21  lucky
 *    *** empty log message ***
 *
 *    Revision 1.7  2000/09/20 18:04:11  lucky
 *    Cleaned up, removed ifdefs to allow the lomax applet to come up in the default installation.
 *
 *    Revision 1.6  2000/09/18 17:24:16  lucky
 *    Final version before v5.1
 *
 *    Revision 1.5  2000/08/16 17:16:25  lucky
 *    Removed write_sac, added Sac2DBEvent
 *
 *    Revision 1.4  2000/08/09 17:22:52  lucky
 *    Put the HTML code that allows lomax applet to be executed inside of
 *    #define for now -- the new release will be shipped without the lomax applet.
 *
 *    Revision 1.3  2000/08/09 16:58:31  lucky
 *    Lint Cleanup
 *
 *    Revision 1.2  2000/08/08 19:08:21  lucky
 *    Changed pickformat from NON_LIN_LOC to EARTHWORM to correspond with the
 *    latest Lomax jar which sends back picks stamped with S-C-N.
 *
 *    Revision 1.1  2000/08/07 19:48:27  lucky
 *    Initial revision
 *
 *
 *
 *
 */


  
/*****************************************************************

   filename:       htmlreply.c 
   module:         eqparam2html
   platforms:      solaris 2.7 (& maybe NT 4)
   date:           04/20/1999
   bug creator:    David Kragness

   description:   

   This is a set of functions to write valid html to stdout

  Topics:

*****************************************************************/


/* include the normal system stuff plus time.h to boot*/
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <math.h>
#include <sys/stat.h>
/* include Phase III and earthworm headers */
#include <time_ew.h>
#include <earthworm.h>
#include <p3db_ora_api.h>
#include <map_display_structs.h>

/* For GIF drawing */
#include <gd.h>
#include <gdfontg.h>
#include <gdfonts.h>

#include "eqreview.h"

extern	int 	SACPABase_end_scn_review (char *);
extern	int 	SetDefaultParams (SetVarsUserStruct *);
extern	int		GetMapFromMapServer(LocationProjectionStruct * pLPSData, 
                          MapServerImageStruct * pMSISdata);
extern	int		DrawMap (gdImagePtr *, P3DB_StationStruct *, int, P3DB_EventListStruct *, int, 
					EQSPicture *, LocationProjectionStruct *, int, float,
					char *, char *, char *, int *, int, int, int);


/* define some paramaterized macros (cringe)
   for easier processing */
#define ABS(a) ((a) > 0 ? (a) : -(a))     /* Absolute value */
#define KMtoMILE(a) ((a)*.6214)           /* convert km to miles */

/* Yeah, yeah, I know this is taboo, but I need a quick/simple conv. factor */
#define KM_PER_DEG 100


/******************************************************
     Dumps HTML header, title, and beginning page
 **********************************************************/
void	html_header( void )
{

   printf("Content-type: text/html\n\n");
   printf("<html>\n");
   printf("<HEAD><TITLE>Event Review</TITLE></HEAD>\n" 
          "<BODY BGCOLOR=#EEEEEE TEXT=#333333>\n" );
   printf("<center>\n");
   printf("<H1><FONT COLOR=blue>Earthworm DBMS<br></FONT></H1>\n" );
   printf("<H2><FONT COLOR=red>Earthquake Parameters retrieved from DBMS</FONT></H2>\n");
   printf("</center>\n");


	/* Produce the javascript needed to run the Lomax applet */
	printf ("<SCRIPT LANGUAGE=\"JavaScript\" SRC=\"%s/lomax/%s\">\n", 
					WebDir, JavascriptFile);
	printf ("</SCRIPT>\n");
	printf ("</HEAD>\n");

	printf ("<html>\n");


}  /* End html_header() */




/******************************************************
  Write summary stuff to stdout(origin,event,magnitude)
 **********************************************************/
void	html_summary (P3DB_OriginStruct *por, P3DB_MagStruct *pmag,
												P3DB_EventStruct *pev)
{

   char  timestr[DATESTR23];
   char  loc_timestr[DATESTR23];
   char  ns = 'N';  /* default for positive lat */
   char  ew = 'E';  /* default for positive lon */
   float mlat, mlon;
   struct stat   statbuf;
   char		hypfile[256];


	if ((por == NULL) || (pmag == NULL) || (pev == NULL))
	{
		html_logit ("", "html_summary: Invalid arguments passed in\n");
		exit;
	}


/* Figure out extra things we need to print out 
 **********************************************/
   datestr23( por->tOrigin, timestr, DATESTR23 );   /* origin time as string */
   datestr23_local( por->tOrigin, loc_timestr, DATESTR23 );


   if( por->dLat < 0 ) ns='S';
   if( por->dLon < 0 ) ew='W';
   mlat = (float)( ABS(por->dLat-(int)por->dLat) * 60. );
   mlon = (float)( ABS(por->dLon-(int)por->dLon) * 60. );

   printf("<strong>\n<pre>\n"
          "PRINCIPAL EARTHQUAKE PARAMETERS\n"
          "_______________________________\n\n" );

	sprintf (hypfile, "%s/%d/hypoutput", ReviewTmpDir, pev->idEvent); 
	if (stat (hypfile, &statbuf) == 0)
	{
		/* 
		 * If there exists a hypoinverse output file, this means
		 * that this event was just reviewed by hypoinverse.
		 * Let the user know about this, and let them 
		 * see the output if they so wish
		 */
		printf ("NOTE: This solution was relocated by HYPOINVERSE.\n");
		printf ("<A HREF=\"%s/%d/hypoutput\">"
					"Click here to see the output of hypoinverse.</A>\n\n",
						WebTmpDir, pev->idEvent);
	}

   printf("Magnitude                    : %.2f  MD\n", pmag->dMagAvg);
   printf("Event Date & Time            : %s   %s  UTC\n", 
           strtok(timestr," "), &timestr[11] );
   printf("LOCAL Event Date & Time      : %s   %s  %s\n", 
           strtok(loc_timestr," "), &loc_timestr[11], getenv ("TZ") );
   printf("Location                     : %.4f %c, %.4f %c\n",
           ABS(por->dLat), ns, ABS(por->dLon), ew );
   printf("                             : (%d deg. %.2f min. %c, %d deg. %.2f min. %c)\n",
           ABS((int)por->dLat), mlat, ns, ABS((int)por->dLon), mlon, ew );
   printf("Depth                        : %.1f km. deep (%.1f miles)\n\n\n",
           por->dDepth, KMtoMILE(por->dDepth) );




   printf("ADDITIONAL EARTHQUAKE PARAMETERS\n"
          "________________________________\n\n" );
   printf("number of phases             : %6d\n", por->iUsedPh );
   printf("rms misfit                   : %6.2f  seconds\n", por->dRms );
   printf("lattitudinal location error  : %6.1f  km\n", por->dErrLat );
   printf("longitudinal location error  : %6.1f  km\n", por->dErrLon );
   printf("vertical location error      : %6.1f  km\n", por->dErZ );
   printf("maximum azimuthal gap        : %6d  degrees\n", por->iGap );
   printf("distance to nearest station  : %6.0f. km\n\n", por->dDmin );


   printf ("Database event ID:      %10u\n\n", pev->idEvent );
   printf ("Origin Author:          %s\n", por->sSource);

   printf("</pre>\n");
   printf("<table border=0><tr><td><strong>Comment:</strong>  %s\n</td></tr></table>",
          pev->szComment); 


}  /* End html_summary() */




/******************************************************
      Write links to ora2rsecgif pages for the
      current event.  Use html buttons to link
      to the ora2rsec_gif pages.
 **********************************************************/
void	html_links2ora2rsec_gif(int EventID)
{

  printf("<hr>\n");
  printf("<strong>\n<pre>\n"
         "VIEW SEISMOGRAMS\n" 
         "</pre></strong>\n" );

  printf("<br>\n");
  printf("<br>\n");

	printf ("<A HREF=\"ora2rsec_gif\?EventID=%u\" target=\"60 sec View\">60 sec view, aligned on pick times</A>\n", EventID);

	printf ("<br>\n");
	printf ("<br>\n");

	printf ("<A HREF=\"ora2rsec_gif_two\?EventID=%u\" target=\"200 sec View\">200 sec view, aligned on pick times</A>\n", EventID);

	printf ("<br>\n");
	printf ("<br>\n");

	printf ("<A HREF=\"ora2snippet_gif\?EventID=%u\" target=\"All Snippets\">All Snippets</A>\n", EventID);

	printf ("<br>\n");
	printf ("<br>\n");
	printf ("<br>\n");

  printf("<hr>\n");

}  /* End html_links2ora2rsec_gif() */




/******************************************************
      Draw map with hypocenter and stations
 **********************************************************/
void	html_map (DBEventInfoStruct *pEvent, int debug_flag )
{

   SetVarsUserStruct WebParams;
   SetVarsUserStruct DefaultParams;
   float    MAP_ENVELOPE = 2.0;
   gdImagePtr pImage;
   P3DB_EventListStruct *pEvents;
   P3DB_StationStruct *pStations;
   P3DB_StationStruct *pPickedStations;
   P3DB_StationStruct *pSilentStations;
   P3DB_External_StationStruct Station;
   int nEvents;
   int nStations,nStations1;
   int nPickedStations;
   int nSilentStations;
   int RetVal;
   EQSPicture Pic={{0,0,0,0},{0.0,0.0,0.0,0.}};
   char *AreaBuf, *AreaBuf2;
   int StationBufSize=1000;
   float StationDisplayWidth=2.0;
   int done, i, j;

   int MagicSEQ,iSEQ;
   int SEQ_done=FALSE;


	if (pEvent == NULL)
	{
		html_logit ("", "html_map: invalid arguments passed in.\n");
		exit;
	}


   AreaBuf=malloc(300000);
   AreaBuf2=malloc(300000);

  if(!(AreaBuf && AreaBuf2))
  {
    html_logit ("", "html_map: Could not malloc Areabuffers (300k bytes each)");
    exit;
  }
  else
  {
    /* Initialize to blank */
    AreaBuf2[0] = AreaBuf[0] = 0;
  }



  /* We only care about one event */
  if((pEvents = (P3DB_EventListStruct *) malloc(sizeof(P3DB_EventListStruct))) == NULL)
  {
    html_logit ("", "Unable to allocate space for retrieval of the Event.\n");
				
    exit;
  }

  if((pStations = (P3DB_StationStruct *) malloc(StationBufSize * 
									sizeof(P3DB_StationStruct))) == NULL)
  {
    html_logit ("", "Unable to allocate space for retrieval of %d Stations.\n",
     	     StationBufSize);
    exit;
  }

  if((pPickedStations = (P3DB_StationStruct *) malloc(StationBufSize * 
									sizeof(P3DB_StationStruct))) == NULL)
  {
    html_logit ("", "Unable to allocate space for retrieval of %d Stations.\n",
     	     StationBufSize);
    exit;
  }

  if((pSilentStations = (P3DB_StationStruct *) malloc(StationBufSize * 
									sizeof(P3DB_StationStruct))) == NULL)
  {
    html_logit ("", "Unable to allocate space for retrieval of %d Stations.\n",
     	     StationBufSize);
    exit;
  }



   /* Set Default web and config file params */
   if(SetDefaultParams(&DefaultParams))
   {
     html_logit ("", "Failed to set Default Params.\n");
     exit;
   }


   /* Now Copy the defaults to the struct that will be configured. */
   memcpy(&WebParams,&DefaultParams,sizeof(SetVarsUserStruct));


   /** Now fill in what we know about these maps **/
   WebParams.LPSData.MapData.CenterLat = pEvent->PrefOrigin.dLat; 
   WebParams.LPSData.MapData.CenterLon = pEvent->PrefOrigin.dLon; 
   /* This is the first strategy.  We can always compute Height/Width
      this way, but it is not neccessarily going to turn out well. Imagine
      a 7 in LA, and a 5 in Antartica.  Neither will show well.  */
   WebParams.LPSData.MapData.LatHeight = (float) (pow (MAP_ENVELOPE, 
						pEvent->PrefMag.dMagAvg/1.5)/1.25);
   WebParams.LPSData.MapData.LonWidth = (float) (pow (MAP_ENVELOPE, 
						pEvent->PrefMag.dMagAvg/1.5));

   if (pEvent->iNumChans < 10)
   {
     MagicSEQ = pEvent->iNumChans;
   }
   else
   {
     MagicSEQ = 10;
   }
   for (iSEQ = MagicSEQ - 1; iSEQ < pEvent->iNumChans && !SEQ_done; iSEQ++)
   {
     if (&pEvent->pChanInfo[iSEQ].Arrivals[0] != NULL)
     {
       WebParams.LPSData.MapData.LatHeight = 
			pEvent->pChanInfo[iSEQ].Arrivals[0].dDist/KM_PER_DEG/1.25*2;
       WebParams.LPSData.MapData.LonWidth = 
			pEvent->pChanInfo[iSEQ].Arrivals[0].dDist/KM_PER_DEG*2;
       SEQ_done=TRUE;
     }
   }

   for (iSEQ = MagicSEQ - 1; iSEQ >= 0 && !SEQ_done; iSEQ--)
   {
     if (&pEvent->pChanInfo[iSEQ].Arrivals[0] != NULL)
     {
       WebParams.LPSData.MapData.LatHeight = 
			pEvent->pChanInfo[iSEQ].Arrivals[0].dDist/KM_PER_DEG/1.25*2;
       WebParams.LPSData.MapData.LonWidth = 
			pEvent->pChanInfo[iSEQ].Arrivals[0].dDist/KM_PER_DEG*2;
       SEQ_done=TRUE;
     }
   }


   if(WebParams.LPSData.MapData.LatHeight <= 0.0)
     WebParams.LPSData.MapData.LatHeight= 0.1;
   if(WebParams.LPSData.MapData.LonWidth <= 0.0)
     WebParams.LPSData.MapData.LonWidth = 0.1;

   WebParams.LPSData.MapData.Rivers = 2051;
   WebParams.LPSData.MapData.Politicals = 8;
   WebParams.LPSData.MapData.bLatLonLines = 0;
   WebParams.LPSData.MapData.bBorder= 0;
   WebParams.LPSData.MapData.ProjectionType= atoi ("m");
   sprintf (WebParams.LPSData.MapData.MapName, "%d", pEvent->Event.idEvent);

   WebParams.MSISData.ImageFormat= 0;
   WebParams.MSISData.ImageFormatsSupported = P2_IMAGE_GIF;
   sprintf (WebParams.MSISData.ImageFileName, "../html/P3GL_temp_image_%ld.gif",
								          getpid());



   /* Get GMT to generate us a map */
   GetMapFromMapServer(&(WebParams.LPSData),&(WebParams.MSISData));


  /* Create EQSPicture coordinates from MSISData */
  /* EQSPicture is just a structure that holds the location
     of a map in terms of both degrees and pixels */
  Pic.deg[PIC_LEFT]=  WebParams.LPSData.MapData.CenterLon
                       - (WebParams.LPSData.MapData.LonWidth/2);
  Pic.deg[PIC_TOP]=   WebParams.LPSData.MapData.CenterLat
                       + (WebParams.LPSData.MapData.LatHeight/2);
  Pic.deg[PIC_RIGHT]=  WebParams.LPSData.MapData.CenterLon
                       + (WebParams.LPSData.MapData.LonWidth/2);
  Pic.deg[PIC_BOT]=   WebParams.LPSData.MapData.CenterLat
                       - (WebParams.LPSData.MapData.LatHeight/2);
  Pic.pix[PIC_LEFT]=  0;
  Pic.pix[PIC_TOP]=   0;
  Pic.pix[PIC_RIGHT]= WebParams.LPSData.PixelWidth;
  Pic.pix[PIC_BOT]=   WebParams.LPSData.PixelHeight;


  RetVal = P3DB_GetStationList(Pic.deg[PIC_BOT], Pic.deg[PIC_TOP],
                     Pic.deg[PIC_LEFT], Pic.deg[PIC_RIGHT],pEvent->PrefOrigin.tOrigin,
                     pStations, StationBufSize, &nStations);


  if(RetVal == -1)
  {
    html_logit ("", "Call to GetStationList failed!\n");
    exit;
  }

  if(Pic.deg[PIC_LEFT] < -180.0 || Pic.deg[PIC_RIGHT] > 180.0)
  {
    if(Pic.deg[PIC_LEFT] < -180.0)
    {
      /* we need to rotate 360 degrees */

      RetVal = P3DB_GetStationList(Pic.deg[PIC_BOT], Pic.deg[PIC_TOP],
                                   Pic.deg[PIC_LEFT]+180.0, 180, 
                                   pEvent->PrefOrigin.tOrigin, &pStations[nStations],
                                   StationBufSize-nStations, &nStations1);

    }
    else if(Pic.deg[PIC_RIGHT] > 180.0)
    {
      /* we need to rotate 360 degrees */
      RetVal = P3DB_GetStationList(Pic.deg[PIC_BOT], Pic.deg[PIC_TOP],
                                   -180.0, Pic.deg[PIC_RIGHT]-180.0,
                                   pEvent->PrefOrigin.tOrigin, &pStations[nStations],
                                   StationBufSize-nStations, &nStations1);
    }
    nStations+=nStations1;
  }

  /*
   * Go through the list of all stations in the current (lat,lon)
   * box and separate those that picked the event from those 
   * that did not
   */

  nPickedStations = 0;
  nSilentStations = 0;

  for (i = 0; i < nStations; i++)
  {

     done = 0;
     j = 0;

     while ((done == 0) && (j < pEvent->iNumChans)) 
     {

		strcpy (Station.Sta, pEvent->pChanInfo[j].Station.Sta);
		strcpy (Station.Comp, pEvent->pChanInfo[j].Station.Comp);
		strcpy (Station.Net, pEvent->pChanInfo[j].Station.Net);



		if ((strcmp (pStations[i].Sta, pEvent->pChanInfo[j].Station.Sta) == 0) &&
			(strcmp (pStations[i].Comp, pEvent->pChanInfo[j].Station.Comp) == 0) &&
			(strcmp (pStations[i].Net, pEvent->pChanInfo[j].Station.Net) == 0))
		{
			done = 1;
		}

       if(done != 1)
          j = j + 1;
     }

     if (done == 1)
     {
        /* move this station into pPickedStations */
        memcpy (&pPickedStations[nPickedStations], 
                    &pStations[i], sizeof (P3DB_StationStruct));

        nPickedStations = nPickedStations + 1;

     }
     else
     {
        /* move this station into pSilentStations */
        memcpy (&pSilentStations[nSilentStations], 
                    &pStations[i], sizeof (P3DB_StationStruct));

        nSilentStations = nSilentStations + 1;

     }
  }

   /* First draw just the stations that were silent */
   nEvents = 0;
   if (DrawMap (&pImage, pSilentStations, nSilentStations, pEvents, nEvents,
                  &Pic, &(WebParams.LPSData), P3_STATION_DISPLAY_STATIONS,
                  StationDisplayWidth, WebParams.MSISData.ImageFileName,
                  AreaBuf, AreaBuf2, &MC_white, 1, 0, debug_flag))
   {
     html_logit ("", "Call to DrawMap failed\n");
     exit;
   }

   /*
    * Now prepare the event pointer, then draw it along with 
    * the stations that were picked
    */

   nEvents = 1;
   nEvents = 1;
   pEvents[0].idEvent = pEvent->Event.idEvent;
   strcpy (pEvents[0].Source, pEvent->PrefOrigin.sSource);
   pEvents[0].OT = pEvent->PrefOrigin.tOrigin;
   pEvents[0].Lat = pEvent->PrefOrigin.dLat;
   pEvents[0].Lon = pEvent->PrefOrigin.dLon;
   pEvents[0].Depth = pEvent->PrefOrigin.dDepth;
   pEvents[0].Magnitude = 3; /* we are fixing the size as small,
                               since we are only showing location
                               of a single quake on this map, 
                               instead of size an location of many quakes.
                               This way it is possible to see stations
                               very close to the quake.  The proper
                               value is (pMag->dMagAvg) */
   

   if (DrawMap (&pImage, pPickedStations, nPickedStations, pEvents, nEvents,
                  &Pic, &(WebParams.LPSData), P3_STATION_DISPLAY_STATIONS,
                  StationDisplayWidth, WebParams.MSISData.ImageFileName,
                  AreaBuf, AreaBuf2, &MC_red, 0, 1, debug_flag))
   {
     html_logit ("", "Call to DrawMap failed\n");
     exit;
   }

   printf("<TITLE>%s</TITLE>\n",WebParams.LPSData.MapData.MapName);
   printf("<BODY>\n");
   printf("<FORM NAME=\"MAPFORM\" ACTION=\"NULL\"" EXE_EXT
         " METHOD=POST>\n");

   printf ("<CENTER> <IMG SRC=\"getimage"EXE_EXT"?GL_IMAGE_ID=%ld\" "
           "USEMAP=\"#search\" ALT=\"%s\" "
           "BORDER=0 WIDTH=%d HEIGHT=%d ></A></CENTER></td>\n",
               getpid(),WebParams.LPSData.MapData.MapName,
               (WebParams.LPSData.PixelWidth
                 + P2_PIC_LEFT_PIX_BORDER+P2_PIC_RIGHT_PIX_BORDER),
               (WebParams.LPSData.PixelHeight
                 + P2_PIC_TOP_PIX_BORDER+P2_PIC_BTM_PIX_BORDER));



   printf("<BR>\n");
   printf("<MAP NAME=\"search\">\n");
   printf (AreaBuf);
   printf("</MAP></FORM>\n");


   printf (" <CENTER> Red triangles indicate stations that reported "
            " picks for this event. </CENTER>");

	printf ("<hr>\n");
	printf ("<br>\n");
	printf ("<center>\n");
	printf ("<A HREF=\"eqreview\?EventID=%u\?ShowMap=0\?Action=%d\"> CLICK HERE TO HIDE THE MAP</A>\n", pEvent->Event.idEvent, ACT_NULL);
	printf ("<br>\n");
	printf ("<br>\n");
	printf ("</center>\n");

   free (pStations);
   free (pPickedStations);
   free (pSilentStations);
   free (pEvents);
   free (AreaBuf);
   free (AreaBuf2);


}  /* End html_map() */




/******************************************************
       Start with no map
 **********************************************************/
void	html_nomap (int idEvent)
{

	printf ("<hr>\n");
	printf ("<br>\n");
	printf ("<center>\n");
	printf ("<A HREF=\"eqreview\?EventID=%u\?ShowMap=1\?Action=%d\"> CLICK HERE TO SHOW THE MAP</A>\n", idEvent, ACT_NULL);
	printf ("<br>\n");
	printf ("<br>\n");
	printf ("</center>\n");

}



/******************************************************
       Put up Relocate, Delete, and Re-insert buttons
 **********************************************************/
void	html_buttons (DBEventInfoStruct *pEvent)
{

	if (pEvent == NULL)
	{
		html_logit ("", "html_buttons: invalid arguments passed in.\n");
		exit;
	}

	printf ("<hr>\n");
	printf ("<br>\n");
	printf ("<center>\n");

	printf ("<A HREF=\"eqreview\?EventID=%u\?GetFromDB=0\?Relocate=1\"> CLICK HERE TO RELOCATE THE EVENT</A>\n", pEvent->Event.idEvent);

	printf ("<br>\n");
	printf ("<br>\n");
	printf ("<hr>\n");

}


/******************************************************
     Write form allowing the user to review phase info
 **********************************************************/
void	html_review_form (DBEventInfoStruct *pEvent)
{

	P3DB_ArrivalStruct 				*par;
	P3DB_StationStruct 				*psta;
	P3DB_StationMagnitudeStruct 	*pstamag;
	char  							timestr[DATESTR23];
	int   							i, j, wt;
	char  							szMagType[10];
	char  							lomax_str[512];
	char  							sacfile[512];

	if (pEvent == NULL)
	{
		html_logit ("", "html_review_form: Invalid arguments passed in.\n");
		exit;
	}


	strcpy(szMagType, "Mag");

	printf("\n");

	printf ("<FORM NAME=\"ReviewForm\" ACTION=\"review_process\" METHOD=POST>\n");

	printf ("<HR><BR><CENTER><PRE><STRONG>\n");

	printf ("EDIT EVENT PARAMETERS\n");
	printf ("</STRONG>\n");
	printf ("NOTE: Relocating the event will override any changes made to the Principal Event Parameters.");
	printf ("<TABLE border=2 cellspacing=2>");
	printf ("<TR align=center>\n");
	printf ("<TD>  Magnitude  </TD>\n");
	printf ("<TD> <INPUT NAME=\"Mag\" TYPE=text SIZE=5 MAXLENGTH=5 VALUE=\"%0.2f\"></TD>\n",
					pEvent->PrefMag.dMagAvg);
	printf ("</TR></TABLE>\n\n");

	printf ("<BR><BR><HR><BR><STRONG>\n");
	printf ("EDIT PICK PARAMETERS\n");
	printf ("</STRONG>\n");
	printf ("Select the Checkbox on the left to delete a Pick.\n");
	printf ("Manually modify the Pick time.\n");
	printf ("Click on the Channel name to graphically review the Pick.\n");
	printf ("</PRE></CENTER>\n");

	printf ("<BR><BR>\n");
	printf ("<TABLE border=2 cellspacing=2>\n");
	printf ("<TR align=center>\n");
	printf ("<TH>Delete Pick?</TH>\n");
	printf ("<TH>Channel</TH>\n");
	printf ("<TH>Phase</TH>\n");
	printf ("<TH>FM</TH>\n");
	printf ("<TH>Pick Time</TH>\n");
	printf ("<TH>Weight</TH>\n");
	printf ("<TH>Res</TH>\n");
	printf ("<TH>Dist</TH>\n");
	printf ("<TH>Azm</TH>\n");
	printf ("<TH>Coda</TH>\n");
	printf ("<TH>Mag</TH>\n");
	printf ("<TH>Mag Type</TH>\n");
	printf ("</TR>\n");

	for (i = 0; i < pEvent->iNumChans; i++ )
	{

		printf ("<TR align=center>\n");

		for (j = 0; j < pEvent->pChanInfo[i].iNumArrivals; j++)
		{

			/* Put up the delete checkbox */
			printf ("<TD align=center> <INPUT NAME=\"Del%d_%d\" TYPE=CHECKBOX> </TD>\n", i, j);


			par = &pEvent->pChanInfo[i].Arrivals[j];
			pstamag = &pEvent->pChanInfo[i].Stamags[j];
			psta = &pEvent->pChanInfo[i].Station;
	
	
			if (par != NULL)
			{
				datestr23 (par->tObsPhase, timestr, DATESTR23); 
			}
			else
			{
				strcpy (timestr,"                      ");  /* set timestr to be blank */
			}
	
	
			if (psta != NULL)
			{
					
				if (pEvent->pChanInfo[i].iNumWaveforms > 0) 
				{
					sprintf (sacfile, "%s/tmp/%d/%s.%s.%s", WebDir, 
						pEvent->Event.idEvent, psta->Sta, psta->Comp, psta->Net);

					sprintf (lomax_str, "javascript:void seis_view_n_02("
						"'seisview', '%s %s %s', null, ['%s'], null, null, "
						"'SAC_BINARY', 'SUN_UNIX', 'EARTHWORM', ' ', [%d, 'Event Info', "
						"'http://%s/earthworm/cgi-bin/retrieve_pick', "
						"'CGI_BIN_EARTHWORM'])",
								psta->Sta, psta->Comp, psta->Net, sacfile,
								pEvent->Event.idEvent, WebHost);
	
	
					printf ("<TD><A HREF=\"%s\">%-5s %3s %4s %4s</A></TD>\n", 
							lomax_str, psta->Sta, psta->Comp, psta->Net, psta->Loc);
				}
				else
				{
					printf ("<TD>%-5s %3s %4s %4s</TD>\n", 
							psta->Sta, psta->Comp, psta->Net, psta->Loc);
				}
			}
			else
			{
				printf("<TD>%-5s %3s %4s %4s</TD>\n","","","","");
			}

	
			if (par != NULL)
			{
				if (par->cMotion == 0x00)
					par->cMotion = ' ';
				if (par->cOnset == 0x00)
					par->cOnset = ' ';

				/* Modifiable pick weight field */
				if (par->dSigma <= 0.02)
					wt = 0;
				else if (par->dSigma <= 0.03)
					wt = 1;
				else if (par->dSigma <= 0.05)
					wt = 2;
				else if (par->dSigma <= 0.08)
					wt = 3;
				else
					wt = 4;
					
		
				printf("<TD>%c</TD>\n", par->szObsPhase[0]);
				printf("<TD>%c</TD>\n", par->cMotion);
	
				/* Modifiable pick time field */
				printf ("<TD><INPUT NAME=\"Val%d_%d\" TYPE=text SIZE=22 MAXLENGTH=22 VALUE=\"%s\"></TD>\n",
											i, j, &timestr);
				printf ("<TD><INPUT NAME=\"Wt%d_%d\" TYPE=text SIZE=3 MAXLENGTH=1 VALUE=\"%d\"></TD>\n",
												i, j, wt);
				printf("<TD>%6.2f</TD>\n", par->tResPick);
				printf("<TD>%5.f</TD>\n", par->dDist);
				printf("<TD>%3.f</TD>\n", par->dAzm);
			}
			else
			{
				printf("<TD>%s</TD>\n", " ");
				printf("<TD>%c</TD>\n", ' ');
				printf ("<TD><INPUT NAME=\"Val%d_%d\" TYPE=text SIZE=22 MAXLENGTH=22 VALUE=\" \"></TD>\n",
												i, j);
				printf ("<TD><INPUT NAME=\"Wt%d_%d\" TYPE=text SIZE=3 MAXLENGTH=1 VALUE=\"0\"></TD>\n",
												i, j);
				printf("<TD>%s</TD>\n", " ");
				printf("<TD>%s</TD>\n", " ");
				printf("<TD>%s</TD>\n", " ");
	
			}
	
			if (pstamag != NULL)
			{
				printf ("<TD>%5.1f</TD>\n", pstamag->dMeasurementCalc);
				printf ("<TD>%3.2f</TD>\n", pstamag->dMag);
				printf ("<TD>%s</TD>\n", pstamag->szMagType);
			}
			else
			{
				printf ("<TD>%s</TD>\n", " ");
				printf ("<TD>%s</TD>\n", " ");
				printf ("<TD>%s</TD>\n", " ");
			}
	
	
			printf ("\n");
			printf ("</TR>\n");

		} /* loop over arrivals */

	} /* loop over channels */

	printf("</TABLE>\n");


	printf ("<BR>\n");
	printf ("<BR>\n");
	printf("<CENTER>\n");


	printf ("<TABLE>\n");

	/* first row */
	printf ("<TR>\n");
	printf ("<TD>\n");
	printf ("<INPUT TYPE=\"submit\" VALUE=\"Refresh\" NAME=\"Refresh\">\n");
	printf ("<INPUT NAME=EventID TYPE=HIDDEN VALUE=\"%u\">\n",
									pEvent->Event.idEvent);

	printf ("</TD>\n");

	printf ("</TR>\n");

	/* second row */
	printf ("<TR>\n");

	printf ("<TD>\n");
	printf ("<INPUT TYPE=\"submit\" VALUE=\"Cancel\" NAME=\"Cancel\">\n");
	printf ("<INPUT NAME=EventID TYPE=HIDDEN VALUE=\"%u\">\n",
									pEvent->Event.idEvent);

	printf ("</TD>\n");


	printf ("<TD>\n");
	printf ("<INPUT TYPE=\"submit\" VALUE=\"Done\" NAME=\"Done\">\n"); 
	printf ("<INPUT NAME=EventID TYPE=HIDDEN VALUE=\"%u\">\n",
									pEvent->Event.idEvent);
	printf ("</TD>\n");

	printf ("</TR>\n");


	printf("</CENTER></TABLE></FORM>\n");
	printf("</pre>\n");

}  /* End html_review_form() */





