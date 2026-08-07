
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: htmlreply.c,v 1.37 2004/06/03 16:04:31 cjbryan Exp $
 *    Revision history:
 *
 *    $Log: htmlreply.c,v $
 *    Revision 1.37  2004/06/03 16:04:31  cjbryan
 *    changed MAGTYPE_SCALAR_MOMENT TO MAGTYPE_MWP
 *
 *    Revision 1.36  2003/07/03 16:27:22  patton
 *    Added new optional command for NIEC
 *
 *    Revision 1.33  2002/03/19 17:24:40  lucky
 *    Moved links2oragif to html_common
 *
 *    Revision 1.32  2002/03/18 22:19:59  lucky
 *    Moved summary and coincidence_summary HTML routines to html_common.c in libsrc
 *
 *    Revision 1.31  2002/03/15 16:31:52  davidk
 *    Changed code so that the minimum map size allowed is 1.10 degrees square.
 *    This was done to avoid a bug in GMT that causes maps to be drawn, as if
 *    the whole area was underwater.
 *
 *    Revision 1.30  2002/03/15 15:46:53  lucky
 *    Added coincidence trigger stuff
 *
 *    Revision 1.29  2001/11/20 16:42:12  lucky
 *    Fixed to work under NT, and with events with no channels.
 *
 *    Revision 1.28  2001/10/05 21:01:20  lucky
 *    Added MaxEventsDisp and MaxEventsRetr
 *
 *    Revision 1.27  2001/08/31 22:32:04  dietz
 *    spelling fix
 *
 *    Revision 1.26  2001/07/01 21:55:15  davidk
 *    Cleanup of the Earthworm Database API and the applications that utilize it.
 *    The "ewdb_api" was cleanup in preparation for Earthworm v6.0, which is
 *    supposed to contain an API that will be backwards compatible in the
 *    future.  Functions were removed, parameters were changed, syntax was
 *    rewritten, and other stuff was done to try to get the code to follow a
 *    general format, so that it would be easier to read.
 *
 *    Applications were modified to handle changed API calls and structures.
 *    They were also modified to be compiler warning free on WinNT.
 *
 *    Revision 1.25  2001/06/21 21:52:59  lucky
 *    Added support for two distinct local mag picks - peak2peak and zero2peak.
 *    The choice is installation specific, so there is a new config option LM_method.
 *
 *    Revision 1.24  2001/06/18 18:41:08  lucky
 *    Changed Ml to ML
 *
 *    Revision 1.23  2001/06/11 17:00:49  lucky
 *    Added support for amplitude picks and magnitudes
 *
 *    Revision 1.22  2001/05/24 22:23:26  lucky
 *    Fixed to reflect the new EWEventInfo structure as well as EventList struct
 *
 *    Revision 1.21  2001/05/15 02:15:10  davidk
 *    Moved functions around between the apps, DB API, and DB API INTERNAL
 *    levels.  Renamed functions and files.  Added support for amplitude
 *    magnitude types.  Reformatted makefiles.
 *
 *    Revision 1.20  2001/04/25 18:25:17  lucky
 *    Added a check for the number of arrivals before a station is moved to the
 *    picked list. It used to only check the name of station, but this is no longer
 *    enough given that we store ancillary snippets which appear in the same list as
 *    the picks.
 *
 *    Revision 1.19  2001/04/24 16:30:48  lucky
 *    Modified label to "number of phases used" to indicate that hypoinverse
 *    does not use phases with weight of 4, even though those picks are
 *    shown in the list and are reviewable.
 *
 *    Revision 1.18  2001/02/28 17:29:10  lucky
 *    Massive schema redesign and cleanup.
 *
 *    Revision 1.17  2000/12/21 00:21:39  davidk
 *    reformatted some whitespace to make the code more readable.
 *    added some explicit format conversions to decrease the number of
 *    warnings on NT.
 *    added code to support config file alterable record section html links
 *    at the bottom of the eqparam2html page.  Replaced the hardcoded links
 *    with a loop that traverses the SPDS array of links derived from the
 *    WaveformLinks config file command.
 *
 *    Revision 1.16  2000/11/20 18:48:16  lucky
 *    Replaced Waveform buttons with links which bring up the record
 *    section page in a separate window. (as in review)
 *    Added third waveform link to bring up all available waveforms
 *
 *    Revision 1.15  2000/09/20 18:03:08  lucky
 *    Rewrote to look like eqreview: calls ewdb_apps_GetEWEventInfo and processes the Arrivals
 *    from it.
 *    Cleaned up formatting and many silly logit calls.
 *
 *    Revision 1.14  2000/09/19 21:52:18  lucky
 *    Took out Oracle initialization call -- already done in eqparam2html
 *
 *    Revision 1.13  2000/09/19 21:47:38  lucky
 *    More cleanup of logit messages
 *
 *    Revision 1.12  2000/09/19 21:06:06  lucky
 *    Cleanup of un-needed logit messages
 *
 *    Revision 1.11  2000/08/10 23:30:21  bogaert
 *    REplaced reference to PG&E with Earthworm DBMS.
 *
 *    Revision 1.10  2000/08/07 19:39:34  lucky
 *    Added WebHost option.
 *
 *    Revision 1.9  2000/05/31 17:26:35  lucky
 *    Modified code so that it can be reused in the review/alarm applications. Also, modified
 *    makefiles to use the new libsrc directory structure.
 *
 *    Revision 1.8  2000/04/12 19:14:24  lucky
 *    Changed record section display captions
 *
 *    Revision 1.7  2000/03/31 23:14:03  davidk
 *    added logic to retrieve stations for mapping, even if the stations
 *    spanned the -180/180 degree longitude barrier.  Now if you have an
 *    even that is close to the -180/180 line, you will be able to see stations
 *    on both sides of the line, instead of just the side the quake is on.
 *
 *    Revision 1.6  2000/03/31 18:42:15  bogaert
 *    *** empty log message ***
 *
 *    Revision 1.5  2000/03/30 16:22:06  davidk
 *    added code that ensures the mapsize will not be 0.0 lat by 0.0 lon
 *    even if none of the picked stations have lat/lon coordinates.
 *
 *    Revision 1.4  1999/11/17 20:43:26  davidk
 *    changed max station count to 1000 from 400, fixed the size of an earthquake at M3.0,
 *    so that the quake could be seen, without coverting nearby stations.
 *    Change color of reporting stations to red from Black.
 *    It makes them(red) slightly harder to pick out, but you
 *    don't get them confused with the black borders from un-picked stations,
 *    when looking at a picture of turkey with all of the Junk NEVER-PICK
 *    NSN stations.
 *
 *    Revision 1.3  1999/11/09 16:49:48  lucky
 *    *** empty log message ***
 *
 *    Revision 1.2  1999/10/28 21:50:37  davidk
 *    fixed some column misalignments due to width, made the map size and granularity scale
 *    based on the proximity of reporting stations.
 *
 *    Revision 1.1  1999/10/20 23:56:24  davidk
 *    Initial revision
 *
 *    Revision 1.7  1999/10/04 21:18:42  lucky
 *    reversed order in which stations are plotted: first draw silent stations
 *    then the ones that picked. This way, multi-component stations, where
 *    one component is picked, but others are silent, works. Before, it
 *    did not.
 *
 *    Revision 1.6  1999/07/20 15:30:51  lucky
 *    Added ability to send email to webmaster
 *
 *    Revision 1.5  1999/06/11 19:50:24  lucky
 *    *** empty log message ***
 *
 *    Revision 1.4  1999/06/08 20:27:03  lucky
 *    *** empty log message ***
 *
 *    Revision 1.3  1999/06/07 23:47:33  lucky
 *    added client side comment lines so that stations will be
 *    identified by their SCN and locations
 *    Also, modified station colors to black (reporting) and white (silent)
 *
 *    Revision 1.2  1999/06/04 17:56:01  lucky
 *    implemented drawing of a map of stations around the event. Stations
 *    that reported picks are drawn in different color from those
 *    that were silent.
 *
 *    Revision 1.1  1999/05/05 18:05:41  lucky
 *    Initial revision
 *
 *
 */


  
/*****************************************************************

   filename:       htmlreply.c 
   module:         eqparam2html
   platforms:      solaris 2.7 (& maybe NT 4)
   project:        USGS Earthworm DBMS
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

/* include earthworm headers */
#include <time_ew.h>
#include <ewdb_ora_api.h>
#include <ewdb_apps_utils.h>
#include <map_display_structs.h>
#include <time_functions.h>
#include <map_display_structs.h>

/* For GIF drawing */
#include <gd.h>
#include <gdfontg.h>
#include <gdfonts.h>

/* include our own header file */
#include "eqparam2html.h"

/* Functions defined in draw_map.c*/

int 	SetDefaultParams (SetVarsUserStruct *);
int   GetMapFromMapServer(LocationProjectionStruct * pLPSData, 
                          MapServerImageStruct * pMSISdata);
int		DrawMap (gdImagePtr *, EWDB_StationStruct *, int, EWDB_EventListStruct *, int, 
					EQSPicture *, LocationProjectionStruct *, int, float,
					char *, char *, char *, int *, int, int, int);

/* End Functions defined in draw_map.c */


/* define some paramaterized macros (cringe)
   for easier processing */
#define ABS(a) ((a) > 0 ? (a) : -(a))     /* Absolute value */
#define KMtoMILE(a) ((a)*.6214)           /* convert km to miles */

/* Yeah, yeah, I know this is taboo, but I need a quick/simple conv. factor */
#define KM_PER_DEG 100



void html_arrivals(EWEventInfoStruct *pEvent)
/******************************************************
  Function:    html_arrivals()
  Purpose:     Write html arrival stuff to stdout

  Author:
               DK, before 04/15/1999, actually it was probably
               written by Dietz in 1998, and then screwed up
               by DK in 1999.

  **********************************************************/
{
   EWDB_ArrivalStruct 			*par;
   EWDB_StationStruct 			*psta;
   EWDB_StationMagStruct 	*pstamag;
   char  timestr[DATESTR23];
   int   i, j, k, wt;


	if (pEvent == NULL)
	{
		html_logit ("", "html_arrivals: Invalid arguments passed in.\n");
		exit(-1);
	}

 
   printf("<strong>\n<pre>\n"
          "\nINDIVIDUAL STATION PARAMETERS<HR>\n");

   printf ("\nARRIVAL PICKS\n"
          "_______________________________________________________________________\n\n" 
          "Sta   Comp  Net  Loc  phase  time UTC     resid  wt  dist   Md\n"
          "                             hh:mm:sec     sec        km   \n\n");


   for (i = 0; i < pEvent->iNumChans; i++)
   {
		for (j = 0; j < pEvent->pChanInfo[i].iNumArrivals; j++)
		{
		     par = &pEvent->pChanInfo[i].Arrivals[j];
/*		     pstamag = &pEvent->pChanInfo[i].Stamags[j];*/
		     psta = &pEvent->pChanInfo[i].Station;

		     if (par != NULL)
		     {
		       datestr23( par->tObsPhase, timestr, DATESTR23 ); 
		     }
		     else
		     {
		       strcpy(timestr,"                      ");  
		     }

		     if (psta != NULL)
		     {
		       printf("%-5s %3s %4s %4s ",psta->Sta,psta->Comp,psta->Net,psta->Loc);
		     }
		     else
		       printf("%-5s %3s %4s %4s ","","","","");

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

		       printf ("   %c%c%c   %s  %5.2f   %d %4.f   ",
		              par->szObsPhase[0], par->cMotion, par->cOnset, &timestr[11],
		              par->tResPick, wt,  par->dDist);
		     }
		     else
		       printf("%c%c%c   %s  %6s   %5s  ",
		              ' ', ' ',' ', &timestr[11],
		              "","");

			 for (k = 0; k < pEvent->pChanInfo[i].iNumStaMags; k++)
			 {
				 pstamag = &pEvent->pChanInfo[i].Stamags[j];

				 if (pstamag != NULL)
				 {
					if (pstamag->iMagType == MAGTYPE_DURATION)
					{
						printf("%0.1f", pstamag->dMag);
					}
					else
					{
						 printf("%s", "");
					}
				 }
			 }
					
/*
			 pstamag = &pEvent->pChanInfo[i].Stamags[j];

		     if (pstamag != NULL)
		     {

				 printf("%0.1f", pstamag->dMag);
		     }
		     else
		       printf("%s", "");*/
		     printf("\n");


		} /* loop over arrivals */

   } /* loop over channels for arrival picks */



   printf ("\nAMPLITUDE PICKS\n"
          "________________________________________________________________\n\n" 
          "Sta   Comp  Net  Loc   Magnitude      Time UTC         Magnitude \n"
          "                         Type         hh:mm:sec   \n\n");

	for (i = 0; i < pEvent->iNumChans+1; i++)
	{
		psta = &pEvent->pChanInfo[i].Station;
		
		for (j = 0; j < pEvent->pChanInfo[i].iNumStaMags; j++)
		{
			pstamag = &pEvent->pChanInfo[i].Stamags[j];
			if ((pstamag != NULL) 
				/*
				
				&& 
				  ((pstamag->iMagType == MAGTYPE_LOCAL_PEAK2PEAK) || 
				   (pstamag->iMagType == MAGTYPE_LOCAL_ZERO2PEAK))*/)
			{
				if (pstamag->iMagType == MAGTYPE_LOCAL_ZERO2PEAK)
				{
					if (strcmp (LM_method, "Zero2Peak") == 0)
					{
						/* One row for zero-to-peak amplitude */

						if (pstamag != NULL)
							datestr23 (pstamag->StaMagUnion.PeakAmp.tAmp1, 
																timestr, DATESTR23); 
						else
							strcpy (timestr,"                      ");  
				
						if (psta != NULL)
						{
							printf ("%-5s %3s %4s %4s", 
										psta->Sta, psta->Comp, psta->Net, psta->Loc);
						}
						else
							printf("%-5s %3s %4s %4s","","","","");
		
			
						printf("    ML 0-P_max    %s     %5.1f\n", 
										&timestr[11], pstamag->dMag);

					} /* One or two amplitude picks */
				}
				
				if (pstamag->iMagType == MAGTYPE_LOCAL_PEAK2PEAK)
				{
					if (strcmp (LM_method, "Peak2Peak") == 0)
					{
						/* One row for minimum peak-to-peak amplitude */
		
						if (pstamag != NULL)
							datestr23 (pstamag->StaMagUnion.PeakAmp.tAmp1, 
														timestr, DATESTR23); 
						else
							strcpy (timestr,"                      ");  
				
			
						if (psta != NULL)
							printf ("%-5s %3s %4s %4s", 
										psta->Sta, psta->Comp, psta->Net, psta->Loc);
						else
							printf("%-5s %3s %4s %4s","","","","");
		

						printf("    ML P-P_max    %s     %5.1f\n", 
										&timestr[11], pstamag->dMag);
			

						/* Another row for maximum peak-to-peak amplitude */
		
						if (pstamag != NULL)
							datestr23 (pstamag->StaMagUnion.PeakAmp.tAmp2, 
															timestr, DATESTR23); 
						else
							strcpy (timestr,"                      ");  
				
			
						if (psta != NULL)
							printf ("%-5s %3s %4s %4s", 
										psta->Sta, psta->Comp, psta->Net, psta->Loc);
						else
							printf("%-5s %3s %4s %4s","","","","");
		
						printf("    ML P-P_min    %s     %5.1f\n", 
										&timestr[11], pstamag->dMag);
			
		
					} /* two amplitude picks */
/*					else
					{
						html_logit("", "Invalid LocalMag method <%s>.  EXITTING!!!\n", LM_method);
						exit(-1);
					}*/
				
				}
				if (pstamag->iMagType == MAGTYPE_MOMENT)
				{
					if (pstamag != NULL)
						datestr23 (pstamag->StaMagUnion.PeakAmp.tAmp1, 
														timestr, DATESTR23); 
					else
						strcpy (timestr,"                      ");  
					
			
					if (psta != NULL)
						printf ("%-5s %3s %4s %4s", 
						psta->Sta, psta->Comp, psta->Net, psta->Loc);
					else
						printf("%-5s %3s %4s %4s","","","","");
							
					printf("    MM            %s     %5.1f\n", 
										&timestr[11], pstamag->dMag);

				}
				if (pstamag->iMagType == MAGTYPE_BODYWAVE)
				{
					if (pstamag != NULL)
						datestr23 (pstamag->StaMagUnion.PeakAmp.tAmp1, 
														timestr, DATESTR23); 
					else
						strcpy (timestr,"                      ");  
					
			
					if (psta != NULL)
						printf ("%-5s %3s %4s %4s", 
						psta->Sta, psta->Comp, psta->Net, psta->Loc);
					else
						printf("%-5s %3s %4s %4s","","","","");
							
					printf("    MB            %s     %5.1f\n", 
										&timestr[11], pstamag->dMag);
				}
				if (pstamag->iMagType == MAGTYPE_SURFACEWAVE)
				{
					if (pstamag != NULL)
						datestr23 (pstamag->StaMagUnion.PeakAmp.tAmp1, 
														timestr, DATESTR23); 
					else
						strcpy (timestr,"                      ");  
					
			
					if (psta != NULL)
						printf ("%-5s %3s %4s %4s", 
						psta->Sta, psta->Comp, psta->Net, psta->Loc);
					else
						printf("%-5s %3s %4s %4s","","","","");
							
					printf("    MS            %s     %5.1f\n", 
										&timestr[11], pstamag->dMag);
				}
				if (pstamag->iMagType == MAGTYPE_MWP)
				{
					if (pstamag != NULL)
						datestr23 (pstamag->StaMagUnion.PeakAmp.tAmp1, 
														timestr, DATESTR23); 
					else
						strcpy (timestr,"                      ");  
					
			
					if (psta != NULL)
						printf ("%-5s %3s %4s %4s", 
						psta->Sta, psta->Comp, psta->Net, psta->Loc);
					else
						printf("%-5s %3s %4s %4s","","","","");
							
					printf("    Mwp           %s     %5.1f\n", 
										&timestr[11], pstamag->dMag);
				}
				if (pstamag->iMagType == MAGTYPE_MBLG)
				{
					if (pstamag != NULL)
						datestr23 (pstamag->StaMagUnion.PeakAmp.tAmp1, 
														timestr, DATESTR23); 
					else
						strcpy (timestr,"                      ");  
					
			
					if (psta != NULL)
						printf ("%-5s %3s %4s %4s", 
						psta->Sta, psta->Comp, psta->Net, psta->Loc);
					else
						printf("%-5s %3s %4s %4s","","","","");
							
					printf("    MBlg          %s     %5.1f\n", 
										&timestr[11], pstamag->dMag);
				}
	
			} /* do we have a valid stamag */

		} /* loop over stamags */

	} /* loop over channels for amplitude picks */


   printf("</pre>\n");

   return;
}  /* End html_arrivals() */



int html_map (EWEventInfoStruct *pEvent,  int debug_flag )
/******************************************************
  Function:    html_map()
  Purpose:     Draw map with hypocenter and stations
  Parameters:    
      Input
      pOrigin: Pointer to an EWDB_OriginStruct whose contents
               will be used to generate the map

      pPick:   Pointer to an EWDB_ArrivalStruct whose contents
               will be used to generate the map.

      nPick:   Number of picks in pPick.
               

  Author:      Lucky Vidmar, 6/3/1999
               
  **********************************************************/
{

   SetVarsUserStruct WebParams;
   SetVarsUserStruct DefaultParams;
   float    MAP_ENVELOPE = 2.0;
   gdImagePtr pImage;
   EWDB_EventListStruct *pEvents;
   EWDB_StationStruct *pStations;
   EWDB_StationStruct *pPickedStations;
   EWDB_StationStruct *pSilentStations;
   int nEvents;
   int nStations,nStations1, nStationsFound;
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
		exit(-1);
	}


   AreaBuf=malloc(300000);
   AreaBuf2=malloc(300000);
  if(!(AreaBuf && AreaBuf2))
  {
    html_logit ("", " Could not malloc Areabuffers (300k bytes each)");
    return(-1);
  }
  else
  {
    /* Initialize to blank */
    AreaBuf2[0] = AreaBuf[0] = 0;
  }



  /* We only care about one event */
  if((pEvents = (EWDB_EventListStruct *) malloc(sizeof(EWDB_EventListStruct))) == NULL)
  {
    html_logit ("","Unable to allocate space for retrieval of the Event.\n");
				
    return(-1);
  }

  if((pStations = (EWDB_StationStruct *) malloc(StationBufSize * 
									sizeof(EWDB_StationStruct))) == NULL)
  {
    html_logit ("","Unable to allocate space for retrieval of %d Stations.\n",
     	     StationBufSize);
    return(-1);
  }

  if((pPickedStations = (EWDB_StationStruct *) malloc(StationBufSize * 
									sizeof(EWDB_StationStruct))) == NULL)
  {
    html_logit ("","Unable to allocate space for retrieval of %d Stations.\n",
     	     StationBufSize);
    return(-1);
  }

  if((pSilentStations = (EWDB_StationStruct *) malloc(StationBufSize * 
									sizeof(EWDB_StationStruct))) == NULL)
  {
    html_logit ("","Unable to allocate space for retrieval of %d Stations.\n",
     	     StationBufSize);
    return(-1);
  }



   /* Set Default web and config file params */
   if(SetDefaultParams(&DefaultParams))
   {
     html_logit ("", "Failed to set Default Params.\n");
     return(-1);
   }


   /* Now Copy the defaults to the struct that will be configured. */
   memcpy(&WebParams,&DefaultParams,sizeof(SetVarsUserStruct));


   /** Now fill in what we know about these maps **/
   WebParams.LPSData.MapData.CenterLat = pEvent->PrefOrigin.dLat; 
   WebParams.LPSData.MapData.CenterLon = pEvent->PrefOrigin.dLon; 
   /* This is the first strategy.  We can always compute Height/Width
      this way, but it is not neccessarily going to turn out well. Imagine
      a 7 in LA, and a 5 in Antartica.  Neither will show well.  */
   WebParams.LPSData.MapData.LatHeight = (float)(pow(MAP_ENVELOPE,
									pEvent->Mags[pEvent->iPrefMag].dMagAvg/1.5)/1.25);
   WebParams.LPSData.MapData.LonWidth = (float)pow(MAP_ENVELOPE,
									pEvent->Mags[pEvent->iPrefMag].dMagAvg/1.5);


   WebParams.LPSData.MapData.LatHeight= (float)-1.0;
   WebParams.LPSData.MapData.LonWidth = (float)-1.0;

   if (pEvent->iNumChans > 0)
   {
	   if (pEvent->iNumChans < 10)
   	       MagicSEQ = pEvent->iNumChans;
	   else
   	       MagicSEQ=10;

	   for (iSEQ = MagicSEQ-1; iSEQ < pEvent->iNumChans && !SEQ_done;iSEQ++)
   	   {
          if(&pEvent->pChanInfo[iSEQ].Arrivals[0] != NULL)
          {
            WebParams.LPSData.MapData.LatHeight = (float)(
	    		pEvent->pChanInfo[iSEQ].Arrivals[0].dDist/KM_PER_DEG/1.25*2);
            WebParams.LPSData.MapData.LonWidth = (float)(
	     			pEvent->pChanInfo[iSEQ].Arrivals[0].dDist/KM_PER_DEG*2);
            SEQ_done=TRUE;
          }
       }

       for(iSEQ=MagicSEQ-1;iSEQ>=0 && !SEQ_done;iSEQ--)
       {
         if(&pEvent->pChanInfo[iSEQ].Arrivals[0] != NULL)
         {
           WebParams.LPSData.MapData.LatHeight = (float)
				(pEvent->pChanInfo[iSEQ].Arrivals[0].dDist/KM_PER_DEG/1.25*2);
           WebParams.LPSData.MapData.LonWidth =  (float)
				(pEvent->pChanInfo[iSEQ].Arrivals[0].dDist/KM_PER_DEG*2);
           SEQ_done=TRUE;
         }
       }
   } /* If we have any channels */


   /* set the minimum map size to 1.1 degrees, so that we avoid
      the underwater maps.  DavidK 031502 */
   if(WebParams.LPSData.MapData.LatHeight <= 1.1)
     WebParams.LPSData.MapData.LatHeight= (float)1.10;
   if(WebParams.LPSData.MapData.LonWidth <= 1.1)
     WebParams.LPSData.MapData.LonWidth = (float)1.10;

   WebParams.LPSData.MapData.Rivers = 2051;
   WebParams.LPSData.MapData.Politicals = 8;
   WebParams.LPSData.MapData.bLatLonLines = 0;
   WebParams.LPSData.MapData.bBorder= 0;
   WebParams.LPSData.MapData.ProjectionType= atoi ("m");
   sprintf (WebParams.LPSData.MapData.MapName, "%d", pEvent->Event.idEvent);

   WebParams.MSISData.ImageFormat= 0;
   WebParams.MSISData.ImageFormatsSupported = IMAGE_GIF;
   sprintf (WebParams.MSISData.ImageFileName, "../html/EWDB_GL_temp_image_%d.gif",
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


  RetVal = ewdb_api_GetStationList(Pic.deg[PIC_BOT], Pic.deg[PIC_TOP],
                     Pic.deg[PIC_LEFT], Pic.deg[PIC_RIGHT],pEvent->PrefOrigin.tOrigin,
                     pStations, &nStationsFound, &nStations, StationBufSize);


  if(RetVal == EWDB_RETURN_FAILURE)
  {
    logit("","Call to GetStationList failed!\n");
    return(-1);
  }

  if(Pic.deg[PIC_LEFT] < -180.0 || Pic.deg[PIC_RIGHT] > 180.0)
  {
    if(Pic.deg[PIC_LEFT] < -180.0)
    {
      /* we need to rotate 360 degrees */
      RetVal = ewdb_api_GetStationList(Pic.deg[PIC_BOT], Pic.deg[PIC_TOP],
                                   Pic.deg[PIC_LEFT]+180.0, 180, 
                                   pEvent->PrefOrigin.tOrigin, &pStations[nStations],
                                   &nStationsFound, &nStations1, StationBufSize-nStations);
    }
    else if(Pic.deg[PIC_RIGHT] > 180.0)
    {
      /* we need to rotate 360 degrees */
      RetVal = ewdb_api_GetStationList(Pic.deg[PIC_BOT], Pic.deg[PIC_TOP],
                                   -180.0, Pic.deg[PIC_RIGHT]-180.0,
                                   pEvent->PrefOrigin.tOrigin, &pStations[nStations],
                                   &nStationsFound, &nStations1, StationBufSize-nStations);
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

  logit("","pEVent->iNumChans = %d, nStations = %d\n",pEvent->iNumChans,nStations);

  for (i = 0; i < nStations; i++)
  {

    done = 0;
    j = 0;
    
    while ((done == 0) && (j < pEvent->iNumChans)) 
    {
      
      if(pEvent->pChanInfo[j].iNumArrivals > 0)
			{ 
      	if ((strcmp (pStations[i].Sta, pEvent->pChanInfo[j].Station.Sta) == 0) &&
       	 (strcmp (pStations[i].Comp, pEvent->pChanInfo[j].Station.Comp) == 0) &&
       	 (strcmp (pStations[i].Net, pEvent->pChanInfo[j].Station.Net) == 0))
      	{
       	 done = 1;
     		}	
			}
      
      if(done != 1)
        j = j + 1;
    }
    
    if (done == 1)
    {
      /* move this station into pPickedStations */
      memcpy (&pPickedStations[nPickedStations], 
        &pStations[i], sizeof (EWDB_StationStruct));
      
      nPickedStations = nPickedStations + 1;
      
    }
    else
    {
      /* move this station into pSilentStations */
      memcpy (&pSilentStations[nSilentStations], 
        &pStations[i], sizeof (EWDB_StationStruct));
      
      nSilentStations = nSilentStations + 1;
      
    }
  }
  
   /* First draw just the stations that were silent */
   nEvents = 0;
   if (DrawMap (&pImage, pSilentStations, nSilentStations, pEvents, nEvents,
                  &Pic, &(WebParams.LPSData), EWDB_STATION_DISPLAY_STATIONS,
                  StationDisplayWidth, WebParams.MSISData.ImageFileName,
                  AreaBuf, AreaBuf2, &MC_white, 1, 0, debug_flag))
   {
     html_logit("","Call to DrawMap failed\n");
     return (-1);
   }


   /*
    * Now prepare the event pointer, then draw it along with 
    * the stations that were picked
    */

   nEvents = 1;
   nEvents = 1;
   pEvents[0].Event.idEvent = pEvent->Event.idEvent;
   strcpy (pEvents[0].Event.szSource, pEvent->PrefOrigin.sSource);
   pEvents[0].dOT = pEvent->PrefOrigin.tOrigin;
   pEvents[0].dLat = pEvent->PrefOrigin.dLat;
   pEvents[0].dLon = pEvent->PrefOrigin.dLon;
   pEvents[0].dDepth = pEvent->PrefOrigin.dDepth;
   pEvents[0].dPrefMag = 3; /* we are fixing the size as small,
                               since we are only showing location
                               of a single quake on this map, 
                               instead of size an location of many quakes.
                               This way it is possible to see stations
                               very close to the quake.  The proper
                               value is (pMag->dMagAvg) */
   

   if (DrawMap (&pImage, pPickedStations, nPickedStations, pEvents, nEvents,
                  &Pic, &(WebParams.LPSData), EWDB_STATION_DISPLAY_STATIONS,
                  StationDisplayWidth, WebParams.MSISData.ImageFileName,
                  AreaBuf, AreaBuf2, &MC_red, 0, 1, debug_flag))
   {
     logit("","Call to DrawMap failed\n");
     return (-1);
   }


   printf("<TITLE>%s</TITLE>\n",WebParams.LPSData.MapData.MapName);
   printf("<BODY>\n");
   printf("<FORM NAME=\"MAPFORM\" ACTION=\"NULL\"" EXE_EXT
         " METHOD=POST>\n");

   printf ("<CENTER> <IMG SRC=\"getimage"EXE_EXT"?GL_IMAGE_ID=%d\" "
           "USEMAP=\"#search\" ALT=\"%s\" "
           "BORDER=0 WIDTH=%d HEIGHT=%d ></A></CENTER></td>\n",
               getpid(),WebParams.LPSData.MapData.MapName,
               (WebParams.LPSData.PixelWidth
                 + PIC_LEFT_PIX_BORDER+PIC_RIGHT_PIX_BORDER),
               (WebParams.LPSData.PixelHeight
                 + PIC_TOP_PIX_BORDER+PIC_BTM_PIX_BORDER));



   printf("<BR>\n");
   printf("<MAP NAME=\"search\">\n");
   printf (AreaBuf);
   printf("</MAP></FORM>\n");


   printf (" <CENTER> Red triangles indicate stations that reported "
            " picks for this event. </CENTER>");

   free (pStations);
   free (pPickedStations);
   free (pSilentStations);
   free (pEvents);
   free (AreaBuf);
   free (AreaBuf2);


   return(0);

}  /* End html_map() */


int SetDefaultParams(SetVarsUserStruct * pParams)
/******************************************************
  Function:    SetDefaultParams()
  Purpose:     Set parameters to any default values that 
               we care about.  As this involves hard coded
               parameters in a source file, it is pretty 
               obsolete, but it 0's out a couple of things
               and sounds cool, so we keep it around.
               DEFAULT constants are set in getlist.h
  Parameters:    
      Output
      pParams: 
               the mother of all structs, contains everything
               we know and most of what we care about.

  Author: DK, before 04/15/1999

  **********************************************************/
{
  /* ECSData  Earthquake Criteria Data (from map_display_structs.h)*/
  pParams->ECSData.MaxLat         = DEFAULT_MAXLAT;
  pParams->ECSData.MinLat         = DEFAULT_MINLAT;
  pParams->ECSData.MaxLon         = DEFAULT_MAXLON;
  pParams->ECSData.MinLon         = DEFAULT_MINLON;

  pParams->ECSData.MaxZ           = DEFAULT_MAXZ;
  pParams->ECSData.MinZ           = DEFAULT_MINZ;
  pParams->ECSData.MaxMag         = DEFAULT_MAXMAG;
  pParams->ECSData.MinMag         = DEFAULT_MINMAG;

  pParams->ECSData.SourceType     = DEFAULT_SOURCE_TYPE;
  pParams->ECSData.Source[0]      = 0;
  pParams->ECSData.EventType      = DEFAULT_EVENT_TYPE;
  pParams->ECSData.MaxEventsDisp  = DEFAULT_MAX_EVENTS;
  pParams->ECSData.MaxEventsRetr  = DEFAULT_MAX_EVENTS;
  pParams->ECSData.LastDaysOnly   = TRUE;
  pParams->ECSData.NumberOfDays   = DEFAULT_NUMBER_OF_DAYS;

  EWDB_atotm_date(&(pParams->ECSData.tmMaxDate),DEFAULT_END_DATE_STR);
  EWDB_atotm_time(&(pParams->ECSData.tmMaxDate),DEFAULT_END_TIME_STR);
  EWDB_atotm_date(&(pParams->ECSData.tmMinDate),DEFAULT_START_DATE_STR);
  EWDB_atotm_time(&(pParams->ECSData.tmMinDate),DEFAULT_START_TIME_STR);

  /* LPSData  Location/Projection Data (from map_display_structs.h)*/
  pParams->LPSData.PixelWidth     = 400;
  pParams->LPSData.PixelHeight    = 400;
  pParams->LPSData.pPSI           = NULL;
  pParams->LPSData.PSILen         = 0;

  /* MSISData  MapServer Image Data (from map_display_structs.h)*/
  pParams->MSISData.ImageFormat = 0;
  pParams->MSISData.ImageFormatsSupported 
                                  = IMAGE_GIF;

  /* WGSSData  Web GUI State String Data (from map_display_structs.h)*/
  /* WGSSData */
  pParams->WGSSData.ShowMap       = 0;
  pParams->WGSSData.ClickEffect   = CLICK_VIEW_EVENTS;
  pParams->WGSSData.StationClickEffect   = EWDB_STATION_DONT_DISPLAY_STATIONS;
  pParams->WGSSData.Options       = 0;
  pParams->WGSSData.XClick        = 0;
  pParams->WGSSData.YClick        = 0;
  pParams->WGSSData.GUIStateString[0]
                                  = 0;
  return(0);
}  /* End SetDefaultParams() */



