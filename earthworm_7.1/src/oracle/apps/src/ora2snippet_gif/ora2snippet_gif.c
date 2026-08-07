
/*
 *   This file is under RCS - do not modify unless you have
 *   checked it out using the command checkout.
 *
 *    $Id: ora2snippet_gif.c,v 1.22 2002/05/28 19:34:19 lucky Exp $
 *    Revision history:
 *
 *    $Log: ora2snippet_gif.c,v $
 *    Revision 1.22  2002/05/28 19:34:19  lucky
 *    Added header and footer tag text
 *
 *    Revision 1.21  2002/05/28 17:26:40  lucky
 *    *** empty log message ***
 *
 *    Revision 1.20  2002/04/10 21:31:10  lucky
 *    Fixed logit calls
 *
 *    Revision 1.19  2002/03/28 20:22:02  davidk
 *    Moved the link to SAC data, outside of the if(ShowDeleteEvent) button
 *    portion of the code, so that the SAC link always gets displayed.
 *
 *    Revision 1.18  2002/03/28 20:06:47  davidk
 *    Merged the code that retrieves event information, so that there is
 *    now one set of code, where there used to be two sets: one for pick-only
 *    wiggles, and one for all wiggles.  The program now displays pick information
 *    when available, even in all-wiggles mode.
 *    Made inclusion of "Delete Event" button configable.
 *
 *    Revision 1.16  2002/03/21 18:54:55  davidk
 *    Added logic to calculate the distance of stations (reporting trace) from the epicenter,
 *    and to sort the stations based upon distance.  This only applies in the
 *    "Display All Snippets" method set in the config file by setting
 *    WiggleAlign == ALIGN_WIGGLE_SHOW_ALL
 *
 *    Revision 1.15  2002/03/20 22:55:25  lucky
 *    Fixed sizes of arrays so that we have no link-time complaints
 *
 *    Revision 1.14  2002/03/20 22:30:49  davidk
 *    remove extra '<' from html line.
 *
 *    Revision 1.13  2002/03/20 22:00:54  davidk
 *    Added WiggleType global variable to control how trace data is decimated
 *    and drawn in the GIF.
 *
 *    Changed allocation of ArrivalStruct(s) to calloc instead of malloc,
 *    so that each struct gets initialized to 0's.
 *
 *    Revision 1.12  2002/03/19 22:02:31  lucky
 *    Added title to the browser bar
 *
 *    Revision 1.11  2002/02/20 20:09:35  lucky
 *    Added optional web appearance parameters
 *
 *    Revision 1.10  2001/09/26 21:41:44  lucky
 *    Added another display time: showall. This way we only need one module
 *    to take care of snippet displays aligned by pick time as well
 *    as where no picks are available.
 *
 *    Revision 1.9  2001/08/28 23:11:54  lucky
 *    Fixed NT support - it now works under NT.
 *
 *    Revision 1.8  2001/07/01 21:55:26  davidk
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
 *    Revision 1.7  2001/06/21 21:25:39  lucky
 *    *** empty log message ***
 *
 *    Revision 1.6  2001/05/24 22:29:02  lucky
 *    Fixed PrefMag stuff
 *
 *    Revision 1.5  2001/05/15 02:15:32  davidk
 *    Moved functions around between the apps, DB API, and DB API INTERNAL
 *    levels.  Renamed functions and files.  Added support for amplitude
 *    magnitude types.  Reformatted makefiles.
 *
 *    Revision 1.4  2001/02/28 17:29:10  lucky
 *    Massive schema redesign and cleanup.
 *
 *    Revision 1.3  2001/02/13 18:26:34  lucky
 *    If a call to ewdb_api_GetWaveformSnippet does not return SUCCESS, free up
 *    malloc-ed space and set NumWaveforms to 0 so that that particular snippet
 *    is not archived.
 *
 *    Revision 1.2  2001/01/20 19:22:02  davidk
 *    changed the call to ewdb_apps_GetDBEventInfo() to call ewdb_apps_GetDBEventInfoII() to avoid
 *    all those stupid ChanCTF error messages.
 *
 *    Revision 1.1  2000/12/18 19:13:47  lucky
 *    Initial revision
 *
 *
 */
  
/*****************************************************************

   ora2snippet_gif provides the user with pictures of record sections
   for the requested event.  The pictures are displayed in GIF format
   and are generated with the gd_lib from Thomas Boutell.  Record
   sections are the wiggles that are generated from plotting
   digitally sampled waveform data over time.  The program is very
   config fileable in that you can vary just about anything from
   the config file, including size, length, gain, number of traces.

*****************************************************************/

/*** #includes ***/

/* include the normal stuff */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <earthworm.h>
#include <ew_event_info.h>
#include <ewdb_ora_api.h>
#include <ewdb_apps_utils.h>
#include <webparse.h>
#include <html_common.h>

#include "ora2snippet_gif.h"


/* This is where we are going to look for the config file */
# ifdef _WINNT
#define GL_CONFIG_DIR "..\\params\\"
#else
#define GL_CONFIG_DIR "../params/"
#endif _WINNT



/*** FUNCTION PROTOTYPES ***/



/*** GLOBAL VARIABLES ***/


/* Generic DB Configuration Parameters */
char envEW_LOG[512];
char WebHost[512];
char BackgroundColor[512];
char HeaderLogo[512];
char FooterLogo[512];
char HeaderTag[512];
char FooterTag[512];

char DBuser[50]="main";
char DBpassword[50]="main";
char DBservice[50]="eqsm.usgs";

int  MyModID=1, LogSwitch=1;

/* Parameters set in config file, or as constants. */
int GifWidth=480;
int TraceHeight=100;
int HeaderWidth=60;
int WiggleTime=30;
int ZoomSize=10;
int MaxPlots=20;
int NumOfTicks=16;
int WiggleAlign=ALIGN_WIGGLE_ON_OT;
int PrePickPcnt=10;
int WiggleType=WIGGLE_TYPE_HOMEGROWN;
int bShowAllWiggles = FALSE;
int bShowDeleteEventButton = FALSE;
int DEBUG=0; /* Change to 1 to enable debugging messages */

/* Parameters global to the program, but secret to the outside world. */
int GifHeight;       /* set by the function that creates the image. */
int LocalWaves=0;    /* indicates whether waves have been localized */
int BorderWidth=20;  /* difference between GIF contents and GIF size */
int EventInfoHeight=40;  /* Height reserved for Event header info */
int FooterHeight=20; /*     Height reserved for Event footer info */
int AllocatedArrivalStructs=50;  /* ???? */

/* Graphics Library Color Var's */
int gdRed,gdWhite,gdBlue,gdBrown,gdBlack,gdYellow,gdGreen,gdPurple;


/* Web params structure */
typedef struct webopts
{
	int 	idEvent;
} WebOptionsStruct;

int main(int argc, char * argv[])
{

  /* Please see ora2snippet_gif.h for a list of structs */
  EventSnippetDrawingStruct		ESDS;
  WebOptionsStruct				WebParams;
  char 							ConfigFile[128];
  char 							TempFileNameStr[128];
  char 							* pTemp1, *pTemp2;
  int               iRetVal;



  /* Send header of reply back to web server */
  printf("Content-type: text/html\n\n");
  printf("<html>\n");

  /* Read parameters from the config file. */
# ifdef _WINNT

  strncpy(TempFileNameStr, argv[0], sizeof(TempFileNameStr));

  pTemp1 = strrchr (TempFileNameStr, '\\');
  
  if (pTemp1 != NULL)
	  pTemp1 = pTemp1 + 1;
  else
	  pTemp1 = TempFileNameStr;

  pTemp2 = strchr (TempFileNameStr, '.');
  if (pTemp2 != NULL)
	  *pTemp2 = '\0';
  
  sprintf(ConfigFile,"%s%s.d",GL_CONFIG_DIR, pTemp1);

# else
  sprintf(ConfigFile,"%s%s.d",GL_CONFIG_DIR,argv[0]);
# endif _WINNT


  /* Go get them config file params */
  if(ReadConfig(ConfigFile))
  {
    fprintf(stdout,"Read of config file failed. Filename: %s\n",ConfigFile);
    return(-1);
  }

  /* Initialize logging */
#ifdef _WINNT
  logit_init(pTemp1,(short)MyModID,1024,LogSwitch);
#else
  logit_init(argv[0],(short)MyModID,1024,LogSwitch);
#endif _WINNT

 
  /* Get the web params from the web client */
    /* initialize Web Parameters to all zeros */
    memset (&WebParams, 0, sizeof(WebOptionsStruct));
    if (Webparse_GetAndProcessWebParams((void *)(&WebParams)) != 0)
    {
        logit ("", "Call to Webparse_GetAndProcessWebParams failed.\n");
        return (-1);
    }

    ESDS.idEvent = WebParams.idEvent;

    printf ("<HEAD><TITLE>Trace for %u</TITLE></HEAD>\n",
                                            WebParams.idEvent );
  
   /* Prepare the DB, cause we are a comin'! */
  if(ewdb_api_Init(DBuser,DBpassword,DBservice))
  {
    html_logit("","DB Initialization failed! Exiting.\n");
    return(-1);
  }


	/* 
	 * We retrieve data differently if we are told to 
	 * display all snippets. The pick-centric display
	 * gets info on picks, whereas all-picks mode
 	 * doesn't care about picks. Actually, it blatantly
	 * lies about them in order to use the same gif producing
	 * mechanism for both methods.
	 */

		/* Go get the best Origin/Magnitude for this event*/
		iRetVal = GetPrefSummaryInfoFromDB(&(ESDS.os),&(ESDS.ms),ESDS.idEvent);

    if(iRetVal)
    {
      logit("","Error in GetPrefSummaryInfoFromDB\n");
      ESDS.os.idOrigin = 0;
      if(!bShowAllWiggles)
      {
        html_logit("","Could not retrieve Summary Info for Event %d from DB!\n",
						               ESDS.idEvent);
        return(-1);
      }
    }


	/* Go get all of the arrivals that go with the origin */
	iRetVal = GetArrivalsFromDB(&(ESDS.pArrivals), ESDS.os.idOrigin, 
                                &(ESDS.NumOfArrivals));
    if(iRetVal)
    {
      logit("","Error in GetArrivalsFromDB\n");
      ESDS.NumOfArrivals = 0;
      if(!bShowAllWiggles)
      {
        html_logit("","Could not retrieve Arrival Info for Event %d from DB!\n",
					               ESDS.idEvent);
        return(-1);
      }
    }

		/* Get the List of Snippets for this event */
		iRetVal = GetSnippetListFromDB(&(ESDS.pSnippets), ESDS.idEvent, 
                                   &(ESDS.NumOfSnippets));
    if(iRetVal)
    {
      logit("","Error in GetSnippetListFromDB\n");
      ESDS.NumOfSnippets = 0;
      html_logit("","Could not retrieve Waveform Info for Event %d from DB!\n",
            ESDS.idEvent);
      return(-1);
    }

		/* Prioritize the data based on distance for now. Match up
  		   snippets with arrivals, and give me the x closest matches 
		**********************/
		iRetVal = OrderSnippetsandArrivals(&ESDS);

		/* NumOfPlots is the number of matching arrivals and snippets we have 
		   Make sure we've got at least 1 
		**********************/
		if(!ESDS.NumOfPlots)
		{
			html_logit("","No wiggles available for drawing from %d arrivals and %d "
			"snippets.\n",ESDS.NumOfArrivals,ESDS.NumOfSnippets);
			return(-1);
		}

		/* Now that we know exactly what snippets we want,
		   Go Get the Snippets. 
		***********************/
		GetSnippetsFromDB(&ESDS);


  /* Now that we got all the neccessary data from the DB,
     go Plot the traces. 
  ***********************/
  PlotTrace(&ESDS);

  ewdb_api_Shutdown();

  /* We are done! */
  return(0);
}  /* End of main() */


int GetSnippetsFromDB(EventSnippetDrawingStruct * pESDrS)
/******************************************************
  Function:    GetSnippetsFromDB()
  Purpose:     Retrieve the actual waveform data (as snippets)
               from the database, for a give list of snippets
  Parameters:    
      Input/Output
      pESDrs:  pointer to a structure that contains all of
               the data for a particular event, including
               the origin, a list of snippets, a list of
               arrivals, and then a matched list of arrivals
               and snippets.  This function loads the
               actual binary waveform data into the list
               of matched snippet/arrival structures.

  Return Value:   0 if successful, -1 on error

  **********************************************************/
{
  int NumOfPlots,i,RetVal;
  SnippetDrawingStruct * pSDRS;

    /* We've got the snippet list, but now we needs to get us 
     some of dem dare snippet thingamajiggers, with the 
     binary data inside. But we only want to get the ones
     that we are interested in plotting.  */

  /* Adjust the number of plots based on MaxPlots */
  /* MaxPlots is the maximum number of record sections
     that we could possibly want to plot. It is set up
     so that you don't end up drawing all of the records
     for a given event if there are a lot of them.  Example:
     You would hate to have to draw out all the
     snippets for a Mag 6 event in California.  There would
     probably be 400, each 4+ minutes long.  It would
     take decades
  *****************/
  if(pESDrS->NumOfPlots > MaxPlots)
  {
    NumOfPlots=MaxPlots;
  }
  else
  {
    NumOfPlots=pESDrS->NumOfPlots;
  }
  if(DEBUG)
    logit("","%d plots to retrieve. MP=%d, NoP=%d.\n",
          NumOfPlots,MaxPlots, pESDrS->NumOfPlots);

  /* for each snippet that we are interested in:
      malloc space for the binary waveform data, 
       based upon the size listed in its snippet 
       descriptor
      and call the Ora_API to get the actual snippet
  **********************/
  for(i=0; i < NumOfPlots; i++)
  {
    pSDRS=pESDrS->pPTS[i].pSDrS;
    /* Create space for snippet entrada */
    pSDRS->pTrace=(TRACE_HEADER *)malloc(pSDRS->TraceLen);
    if(!pSDRS->pTrace)
    {
      logit("","malloc failed for snippet of %d bytes from "
                "SCN %s,%s,%s for event %d.\n",
                pSDRS->TraceLen,pSDRS->SCN.Sta,pSDRS->SCN.Comp,pSDRS->SCN.Net,
                pESDrS->idEvent);
      continue;
    }


    if(DEBUG)
      logit("","pTrace for snippet %d is %u.\n",i,pSDRS->pTrace);

    /* Get the little rascal, might want to check the ret code */
    RetVal=ewdb_api_GetWaveformSnippet(pSDRS->idWaveform, (char *)(pSDRS->pTrace),
                      pSDRS->TraceLen);
    if(RetVal != EWDB_RETURN_SUCCESS)
    {
      free(pSDRS->pTrace);
      pSDRS->pTrace=NULL;
      logit("","GetSnippet() returned %d in GetSnippetsFromDB().\n"
               "Setting returned snippet to NULL.  Available "
               "buffer for snippet was %d bytes.\n  idWaveform was %d.\n",
               RetVal,pSDRS->TraceLen,pSDRS->idWaveform);
      continue;
    }

    /* We have successfully grabbed the snippet, now lets fill in the
       associated data.
    */
  }  /* end for each plot */

  pESDrS->NumOfPlots=NumOfPlots;

  return(0);
}  /* End GetSnippetsFromDB() */


int PlotTrace(EventSnippetDrawingStruct * pESDrS)
/******************************************************
  Function:    PlotTrace()
  Purpose:     Draw the wiggles based upon the datapoints
               in the binary snippets.
               
  Parameters:    
      Input/Output
      pESDrs:  pointer to a structure that contains all of
               the data for a particular event, including
               the origin, a list of snippets, a list of
               arrivals, and then a matched list of arrivals
               and snippets, including the binary waveform data.

  Return Value:   0 if successful, -1 on error
  Author: DK, before 04/15/1999, 
  **********************************************************/
{
  
  FILE * fOut;
  char fname[100];
  int i;
  gdImagePtr gdIP;
  int NumOfPlots;


  /* Adjust the number of plots based on MaxPlots */
  if(pESDrS->NumOfPlots > MaxPlots)
  {
    NumOfPlots=MaxPlots;
  }
  else
  {
    NumOfPlots=pESDrS->NumOfPlots;
  }

  if(DEBUG)
    logit("","Entering PlotTrace, numplots=%d, max=%d.\n", NumOfPlots, MaxPlots);


  /* Open a drawing canvas based on size */
  /* Canvas size = (GifWidth,header + (TraceHeight * NumOfPlots)) */
  GifHeight=TraceHeight * NumOfPlots + EventInfoHeight+ FooterHeight+5;
  if(!(gdIP=gdImageCreate(GifWidth, GifHeight)))
  {
    logit("","PlotTrace:  Unable to initialize trace image.\n");
    return(-1);
  }

  if(DEBUG)
    logit("","gdImageCreate(%d,%d) completeed successfully.\n",
            GifHeight,TraceHeight * NumOfPlots + EventInfoHeight);


  /* Allocate global colors. */

  /* First allocate patriotic colors. */
  gdRed = gdImageColorAllocate(gdIP, 255, 0, 0);
  gdWhite = gdImageColorAllocate(gdIP, 255, 255, 255);
  gdBlue = gdImageColorAllocate(gdIP, 0, 0, 255);

  gdYellow = gdImageColorAllocate(gdIP, 255, 255, 0);
  gdPurple = gdImageColorAllocate(gdIP, 255, 0, 255);
  gdBlack = gdImageColorAllocate(gdIP, 0, 0, 0);
  gdGreen = gdImageColorAllocate(gdIP, 0, 255, 0);

  /* Make everything white, and white = transparent */
  gdImageFilledRectangle(gdIP,0,0,GifWidth,
         TraceHeight * NumOfPlots + EventInfoHeight+ FooterHeight+5,
         gdWhite);

  gdImageColorTransparent(gdIP, gdWhite);

  /* Write out header info for the event. */
  DrawEventHeader(gdIP,pESDrS->idEvent,&(pESDrS->os),
                  &(pESDrS->ms),EventInfoHeight);

  /* Iterate through each plot */
  for(i=0;i<NumOfPlots;i++)
  {

    /* Fill in remaining global pieces of SnippetDrawingStruct */
    if(DEBUG)
      logit("","Calling PlotATrace(%d) for idChan %d: <%s>.<%s>.<%s> "
               "in PlotTrace().\n", i,
            pESDrS->pPTS[i].pArrival->idChan,
			pESDrS->pPTS[i].pSDrS->SCN.Sta,
			pESDrS->pPTS[i].pSDrS->SCN.Comp,
			pESDrS->pPTS[i].pSDrS->SCN.Net);
			

    pESDrS->pPTS[i].pSDrS->PixelWidth=GifWidth-BorderWidth-HeaderWidth;
    pESDrS->pPTS[i].pSDrS->SecsPerPixel=
      ((float)WiggleTime *(1+(((float)PrePickPcnt)/100)))/
                                        (pESDrS->pPTS[i].pSDrS->PixelWidth);
    if(WiggleAlign==ALIGN_WIGGLE_ON_OT)
    {
      if(pESDrS->os.tOrigin)
      {
        pESDrS->pPTS[i].pSDrS->StartTime=
            pESDrS->os.tOrigin - (((float)WiggleTime)*PrePickPcnt/100);
      }
    }
    else if(WiggleAlign==ALIGN_WIGGLE_ON_ARRIVAL)
    {
      if(pESDrS->pPTS[i].pArrival->tObsPhase)
      {
        pESDrS->pPTS[i].pSDrS->StartTime=
            pESDrS->pPTS[i].pArrival->tObsPhase- (((float)WiggleTime)*PrePickPcnt/100);
      }
    }
    else if(WiggleAlign==ALIGN_WIGGLE_ON_8_KM_PER_SEC)
    {
      if(pESDrS->os.tOrigin)
      {
        pESDrS->pPTS[i].pSDrS->StartTime=
              pESDrS->os.tOrigin - (((float)WiggleTime)*PrePickPcnt/100)
                            + (pESDrS->pPTS[i].pArrival->dDist/8.0);
      }
    }
    else if(WiggleAlign==ALIGN_WIGGLE_ON_6_KM_PER_SEC)
    {
      if(pESDrS->os.tOrigin)
      {
        pESDrS->pPTS[i].pSDrS->StartTime=
              pESDrS->os.tOrigin - (((float)WiggleTime)*PrePickPcnt/100)
                            + (pESDrS->pPTS[i].pArrival->dDist/6.0);
      }
    }
    else
    {
      logit("","PlotTrace(): Unknown Wiggle Alignment: %d.\n",WiggleAlign);
    }


    /* Draw the wiggles for the current trace. */
    PlotATrace(gdIP, &(pESDrS->pPTS[i]),0,EventInfoHeight+(i*TraceHeight),
               GifWidth-BorderWidth,TraceHeight,HeaderWidth);


  }  /* end for i < NumOfPlots */

  /* Write out footer info. */
  DrawEventFooter(gdIP, 0, TraceHeight * NumOfPlots + EventInfoHeight+5,
                  GifWidth-BorderWidth, HeaderWidth);

  if(DEBUG)
    logit("","Finished plotting all traces in PlotTrace.\n");

  /* Make output image interlaced (allows "fade in" in some viewers,
  and in the latest web browsers) */
  gdImageInterlace(gdIP, 1); 

  if(DEBUG)
    logit("","Interlaced GIF in PlotTrace().\n");

  /* Save the canvas as a gif. */
  sprintf(fname,"../html/image_%d.gif", getpid());
  fOut = fopen(fname, "wb");
  if(!fOut)
  {
    logit("","Could not open %s\n",fname);
    return(-1);
  }

  gdImageGif(gdIP, fOut);
  if(DEBUG)
    logit("","saved GIF in PlotTrace().\n");

  fclose(fOut);

  /* Clean up */
  gdImageDestroy(gdIP);
  if(DEBUG)
    logit("","destroyed GIF resources in PlotTrace().\n");


  /* Center everything */
  printf("<Center>\n");
  /* Write HTML to the browser, including a IMG SRC=canvas location. */
#ifdef _WINNT
  /* getimage doesn't work on NT DK 990119 due to a Pipe problem */
  printf("<IMG SRC=\"../image_%d.gif\" ",getpid());
#else
  printf("<IMG SRC=\"getimage"EXE_EXT"?GL_IMAGE_ID=%d\" ",(int) getpid());
#endif /* _WINNT */

  printf("WIDTH=%d HEIGHT=%d HSPACE=0 VSPACE=0 BORDER=0 ALIGN=TOP>\n",
         GifWidth, GifHeight);
  printf("<br>\n");

  printf("<br>\n");
  printf("<hr>\n");

  /* Begin SAC link */

  printf("<br>\n");
  printf("<pre><center>\n");
    
  printf ("<A HREF=\"ora2sactarfile?EVENT%d=On\">Download Seismograms "
          "in SAC Format</A>\n\n\n", pESDrS->idEvent);
  printf ("<BR><HR>\n");

  if(bShowDeleteEventButton)
  {
    /* Begin Delete Button */
    printf("<BR><PRE>\n");
    
    printf ("WARNING!!! WARNING!!! WARNING!!! \n"
            "Clicking the button below will PERMANENTLY delete "
            "this event from the database.\n");
    
    /* Put up the delete button */
    printf ("<FORM ACTION=\"delete_event\" EXE_EXT METHOD=POST>\n");
    printf ("<INPUT NAME=EventID TYPE=HIDDEN VALUE=\"%u\">\n", pESDrS->idEvent);
    
    printf ("<INPUT TYPE=\"SUBMIT\" VALUE=\"DELETE EVENT\">\n\n");
    printf ("</FORM><HR>\n");
  }  /* end if(bShowDeleteEventButton) */


 
   /* Print out the postmortem, allowing people to complain */
	html_trailer(WebHost, FooterLogo, FooterTag);


  if(DEBUG)
    logit("","Completed PlotTrace() successfully.\n");

  return(0);
}  /* End of PlotTrace() */


int PlotATrace(gdImagePtr gdIP, PlotTraceStruct * pPTS, 
               int Left, int Top, int Width, int Height,
               int HeaderWidth)
/******************************************************
  Function:    PlotATrace()
  Purpose:     Draw the wiggles for a single SCN on a gdImage.
               
  Parameters:    
      Input
      pPTS:    Pointer to a struct that contains arrival and
               snippet information for the SCN record section
               to be drawn
      Left:    The left boundary (pixel) for this record section
               drawing
      Top:     The top boundary (pixel) for this record section
               drawing
      Width:   The width of this record section drawing including
               the record header
      Height:  The height of this record section drawing.  Together,
               Left, Top, Height, and Width define a rectangle 
               which encapsulates this record section drawing.
      HeaderWidth:
               The width of the header portion of this record 
               section drawing.

      Output
      gdImagePtr:
               Pointer to the image that is to be drawn upon.

  Return Value:   0 if successful, -1 on error
  Author: DK, before 04/15/1999, 

  **********************************************************/
{
  
  int PickX;

  /* Initialize PlotTraceStruct Params */
  pPTS->pSDrS->CenterY=Top+(Height/2);
  pPTS->pSDrS->StartingX=Left+HeaderWidth;

  if(DrawBorders(gdIP,Left,Top,Width,Height,HeaderWidth))
  {
    logit("","DrawBorders() failed for idChan %d.\n",
          pPTS->pArrival->idChan);
    return(-1);
  }

  if(DrawActualTrace(gdIP, pPTS->pSDrS))
  {
    logit("","DrawActualTrace() failed for idChan %d.\n",
          pPTS->pArrival->idChan);
    return(-1);
  }

  if(DrawHeader(gdIP, pPTS,Left,Top,HeaderWidth,Height))
  {
    logit("","DrawHeader() failed for idChan %d.\n",
          pPTS->pArrival->idChan);
    return(-1);
  }

  /* Mark the pick and coda-termination if applicable. */
  PickX=(int)((pPTS->pArrival->tObsPhase - pPTS->pSDrS->StartTime) /
        pPTS->pSDrS->SecsPerPixel
        +Left+HeaderWidth);
  if(DEBUG)
  {
    logit("","Left %d, HeaderWidth %d,SecsPerPixel %.4f,"
             "ST %.2f, AT %.2f,PickX %d\n",
          Left,HeaderWidth,pPTS->pSDrS->SecsPerPixel,
          pPTS->pSDrS->StartTime,pPTS->pArrival->tObsPhase,PickX);
  }

  if(PickX >= Left+HeaderWidth && PickX <= Left+Width)
    gdImageLine(gdIP,PickX,Top+(Height/8),PickX,Top+(7*Height/8),gdRed);

  return(0);
}  /* End of PlotATrace() */ 


int Webparse_Client_SetVars(char * szVar, char * szVal, void * pUserParams)
/******************************************************
  Function:    Webparse_Client_SetVars()
  Purpose:     Webparse_Client_SetVars() is a callout 
               function provided by 
               the web_common (webparse) routines that parse
               web parameters passed during a CGI-BIN GET or
               POST request.  Webparse_Client_SetVars() is 
               called for each parameter - value pair
  **********************************************************/
{
  WebOptionsStruct * pOptions= (WebOptionsStruct *) pUserParams;

    if(strcmp(szVar,"EventID") == 0)
    {
      pOptions->idEvent = atoi(szVal);
    }
	else
    {
      html_logit ("" ,"Unrecognized Web Option : %s = %s\n",szVar,szVal);
    }
    return(0);
}  /* End of SetVars() */



int CompareArrivalDistances(const void * pElem1, const void * pElem2)
{
  PlotTraceStruct * p1 = (PlotTraceStruct *) pElem1;
  PlotTraceStruct * p2 = (PlotTraceStruct *) pElem2;

  if(!(p1->pArrival) && !(p2->pArrival))
    return(0);
  else if(!(p1->pArrival))
    return(1);
  else if(!(p2->pArrival))
    return(-1);
  else if(p1->pArrival->dDist < p2->pArrival->dDist)
    return(-1);
  else if(p1->pArrival->dDist > p2->pArrival->dDist)
    return(1);
  else
    return(0);
}

