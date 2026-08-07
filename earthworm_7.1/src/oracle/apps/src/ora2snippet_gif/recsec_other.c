
/*
 *   This file is under RCS - do not modify unless you have
 *   checked it out using the command checkout.
 *
 *    $Id: recsec_other.c,v 1.4 2001/07/01 21:55:26 davidk Exp $
 *    Revision history:
 *
 *    $Log: recsec_other.c,v $
 *    Revision 1.4  2001/07/01 21:55:26  davidk
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
 *    Revision 1.3  2001/05/15 02:15:32  davidk
 *    Moved functions around between the apps, DB API, and DB API INTERNAL
 *    levels.  Renamed functions and files.  Added support for amplitude
 *    magnitude types.  Reformatted makefiles.
 *
 *    Revision 1.2  2001/02/28 17:29:10  lucky
 *    Massive schema redesign and cleanup.
 *
 *    Revision 1.1  2000/12/18 19:13:47  lucky
 *    Initial revision
 *
 *    Revision 1.3  2000/09/20 15:27:16  lucky
 *    Cleanup logit messages
 *
 *    Revision 1.2  2000/08/09 16:25:50  lucky
 *    Lint cleanup
 *
 *    Revision 1.1  2000/02/15 19:39:25  lucky
 *    Initial revision
 *
 *    Revision 1.2  2000/01/10 22:30:36  davidk
 *    fixed bug where (0-RetVal) was being used to figure out how many XXX's had
 *    been found by a GetXXXList() API function.  The proper method is to use
 *    the NumXXXFound values passed to the function(s) as a pointer.  The bug
 *    resulted from differeences between how the API handled NumXXXsFound in
 *    version 1 and version 2.
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

   filename:       recsec_other.c 
   module:         ora2rsec_gif
   platforms:      solaris 2.7 (& maybe NT 4)
   date:           04/20/1999
   bug creator:    David Kragness

   description:   

   ora2rsec_gif provides the user with pictures of record sections
   for the requested event.  The pictures are displayed in GIF format
   and are generated with the gd_lib from Thomas Boutell.  Record
   sections are the wiggles that are generated from plotting 
   digitally sampled waveform data over time.  

   recsec_other.c contains support functions for drawing and dbretrieval
   that needs to be done by ora2rsec_gif

  Topics:

*****************************************************************/

/* include all the basic header files, NOTHING SPECIAL!! */
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>

/* for calculating distance including depth */
#include <math.h>

/* for converting between ascii time strings and time_t or struct tm */
#include <time_functions.h>

#include <ewdb_ora_api.h>
#include <ewdb_apps_utils.h>

/* include our header file */
#include "ora2rsec_gif.h"

/* The number of snippet descriptor structs to allocate for the
   fir grab on the DB retrieval call */
# define ALLOCATED_SDS 50


int DrawEventHeader(gdImagePtr gdIP, int idEvent, EWDB_OriginStruct *pOS,
                    EWDB_MagStruct *pMS,
                    int EventInfoHeight)
/******************************************************
  Function:    DrawEventHeader()
  Purpose:     Draw Event specific header information.
               
  Parameters:    
      Input
      EventID: The DB ID of the event to be drawn.
      pOS:     Pointer to Origin information for the chosen event.
      EventInfoHeight:
               (Unused)  The Height of the Event Header.

      Input/Output
      gdIP:    Pointer to the image canvas that is to be drawn
               on.


  Return Value:   0 if successful, -1 on error
  Author:
               DK, before 04/15/1999, 

  Internal Functions Called:
  Library Functions Called: 
  EW Library Functions Called:
  External Library Functions Called:
               gdImageString()
  System Functions Called:  
               sprintf()
  **********************************************************/
{
  char OTString[100],EventString[100];
  time_t tOrigin;
  int StringWidth;  /* This is our best guess to the width of the string. */
                        /* I could not find a way to get that from the graphics
                           library.  davidk 09/14/98
                        */

  tOrigin=(int)(pOS->tOrigin);
  EWDB_ttoa(&tOrigin, OTString);

  /* First event line */
  /* Cleanup! NEED A MAGNITUDE STRUCT HERE!!!!!! */
  sprintf(EventString,
          "Event ID = %6d  Time = %s.%d UTC  Mag = %1.2f(%d)",
          idEvent, OTString, (int)((pOS->tOrigin-tOrigin+.5)*100), 
          pMS->dMagAvg,pMS->iMagType);
  StringWidth=6/*pixels per char */ * strlen(EventString);
  if(StringWidth > GifWidth)
    StringWidth=GifWidth;
  gdImageString(gdIP,gdFontSmall,(GifWidth-StringWidth)/2,10,EventString,gdBlue);

  /* 2nd event line */
  sprintf(EventString,
          "Lat = %3.2f, Lon = %3.2f, Z = %3.2f",
          pOS->dLat,pOS->dLon,pOS->dDepth);
  StringWidth=250;
  gdImageString(gdIP,gdFontSmall,(GifWidth-StringWidth)/2,25,EventString,gdBlue);

  return(0);
}

int DrawEventFooter(gdImagePtr gdIP, int Left, int Top,
                    int Width, int HeaderWidth)
/******************************************************
  Function:    DrawEventFooter()
  Purpose:     Draw Event specific footer information.
               
  Parameters:    
      Input
      Left:    The left boundary (pixel) for record section
               drawing
      Top:     The top boundary (pixel) for the event footer
      Width:   The width of record section drawings including
               the record header
      Height:  The height of this event footer drawing.  Together,
               Left, Top, Height, and Width define a rectangle 
               which quazi-encapsulates this event footer.
      HeaderWidth:
               The width of the header portion of record 
               section drawings.

      Input/Output
      gdIP:    Pointer to the image canvas that is to be drawn
               on.

  Return Value:   0 if successful, -1 on error
  Author:
               DK, before 04/15/1999, 

  Internal Functions Called:
  Library Functions Called: 
               logit()
  EW Library Functions Called:
  External Library Functions Called:
               gdImageString()
  System Functions Called:  
               sprintf(),strlen()
  **********************************************************/
{

  int j=0,StartingX,TickX,TimeStrLen,CharWidth=6;
  char TimeString[32];
  int PreWidth,PreTicks;
  float TickWidth;

  /* Figure out width from TZero to End */
  PreWidth=(int)(((float)PrePickPcnt)/(100+PrePickPcnt) * (Width-HeaderWidth));
  PreTicks=(int)(PrePickPcnt * (((float)NumOfTicks)/100));

  /* Time ticks from Time Zero */
  TickWidth=((float)(Width-HeaderWidth-PreWidth)) / NumOfTicks;
  StartingX=Left+HeaderWidth+PreWidth;


  /* Label the time 0 spot according to the record section alignment */
  if(WiggleAlign==ALIGN_WIGGLE_ON_ARRIVAL)
    sprintf(TimeString,"00");
  else if(WiggleAlign==ALIGN_WIGGLE_ON_OT)
    sprintf(TimeString,"OT");
  else if(WiggleAlign==ALIGN_WIGGLE_ON_8_KM_PER_SEC)
    sprintf(TimeString,"OT+8KM/S");
  else if(WiggleAlign==ALIGN_WIGGLE_ON_6_KM_PER_SEC)
    sprintf(TimeString,"OT+6KM/S");
  else
    logit("","Unknown alignment type %d.\n",WiggleAlign);
  TimeStrLen=strlen(TimeString);
  TickX=StartingX+(int)(j*TickWidth);
  gdImageString(gdIP,gdFontSmall,TickX-(TimeStrLen*CharWidth/2),
                Top+4,TimeString,gdRed);

  /* Label every 4th tick mark */
  for(j=4;j<=NumOfTicks;j+=4)
  {
    sprintf(TimeString,"+%2.2f",((float)j)/NumOfTicks * WiggleTime);
    TimeStrLen=strlen(TimeString);
    TickX=StartingX+(int)(j*TickWidth);
    gdImageString(gdIP,gdFontSmall,TickX-(TimeStrLen*CharWidth/2),
                  Top+4,TimeString,gdRed);
  }
  return(0);

}  /* End of DrawEventFooter() */

int DrawHeader(gdImagePtr gdIP, PlotTraceStruct * pPTS, int Left, int Top, 
               int Width, int Height)
/******************************************************
  Function:    DrawHeader()
  Purpose:     Draw header information for the given record
               section.
               
  Parameters:    
      Input
      pPTS:    Pointer to a struct that contains arrival and
               snippet information for the SCN record section
               header to be drawn
      Left:    The left boundary (pixel) for this record section
               header
      Top:     The top boundary (pixel) for this record section
               header
      Width:   The width of this record section header
      Height:  The height of this record section header.  Together,
               Left, Top, Height, and Width define a rectangle 
               which encapsulates this record section header.

      Input/Output
      gdIP:    Pointer to the image canvas that is to be drawn
               on.

  Return Value:   0 if successful, -1 on error
  Author:
               DK, before 04/15/1999, 

  Internal Functions Called:
  Library Functions Called: 
  EW Library Functions Called:
  External Library Functions Called:
               gdImageString()
  System Functions Called:  
               sprintf()
  **********************************************************/
{
  static int lineheight=10;
  int linenum=0;
  char PhaseString[48];
  EWDB_ArrivalStruct * pAS=pPTS->pArrival;

  /* Write the SCN of the record */
  sprintf(PhaseString,"%s %s %s",pPTS->pSDrS->SCN.Sta,
          pPTS->pSDrS->SCN.Comp, pPTS->pSDrS->SCN.Net);
  gdImageString(gdIP,gdFontTiny,4,Top+4+(lineheight*linenum),
                PhaseString,gdBlack);
  linenum++;

  /* We do lots of height checking.  Depending on the size of
     the record section, we can display from 2 to 6 lines, and
     we want to crunch in as much info as possible without 
     spilling out of our little box.  So we check for height 
     limitations kind of like they do at theme parks where the
     signs read, "You must be atleast this tall, to ride this 
     ride."  Well we do one better than that by providing you
     something even if you're not that tall.  
     We use linenum to act as a line counter and it tells us
     how many lines we have drawn.  So we are always checking
     to make sure we can fit 1 or 2 more lines before we try
     to draw/write them.
  *************************************************************/
  if(Height >= ((linenum+1)*lineheight + 4))
  {
    /* Write the wave type and first motion */
    sprintf(PhaseString,"%s%c",pAS->szObsPhase,pAS->cMotion);
    gdImageString(gdIP,gdFontTiny,4,Top+4+(lineheight*linenum),
                  PhaseString,gdRed);

    /* Write the distance from origin in kilometers*/
    sprintf(PhaseString,"%3.1f km",pAS->dDist);
    gdImageString(gdIP,gdFontTiny,18,Top+4+(lineheight*linenum),
                  PhaseString,gdBlack);
    linenum++;
  }
  if(Height >= ((linenum+1)*lineheight + 4))
  {
    /* Write the Scale of the record section 
           (the amplitude magnification factor) */
    sprintf(PhaseString,"Scale: %2d",pPTS->pSDrS->Scale);
    gdImageString(gdIP,gdFontTiny,4,Top+4+(lineheight*linenum),
                  PhaseString,gdBlack);
    linenum++;
  }
  if(Height >= ((linenum+2)*lineheight + 4))
  {
    /* Write the "AmpMax" label */
    sprintf(PhaseString,"AmpMax:");
    gdImageString(gdIP,gdFontTiny,4,Top+4+(lineheight*linenum),
                  PhaseString,gdBlack);
    linenum++;

    /* Write the Maximum wave amplitude that occurs during the record */
    sprintf(PhaseString," %10d",pPTS->pSDrS->AmpMax);
    gdImageString(gdIP,gdFontTiny,4,Top+4+(lineheight*linenum),
                  PhaseString,gdBlack);
    linenum++;
  }
  else if(Height >= ((linenum+1)*lineheight + 4))
  {
    /* Write the "AmpMax" label and the Maximum wave Amplitude */
    sprintf(PhaseString,"AmpMx:%5d",pPTS->pSDrS->AmpMax);
    gdImageString(gdIP,gdFontTiny,4,Top+4+(lineheight*linenum),
                  PhaseString,gdBlack);
    linenum++;
  }

  return(0);
}  /* End Of DrawHeader() */


int DrawBorders(gdImagePtr gdIP, int Left, int Top, int Width, 
                int Height, int HeaderWidth)
/******************************************************
  Function:    DrawBorders()
  Purpose:     Draw the borders and tick marks for the given record
               section.
               
  Parameters:    
      Input
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

      Input/Output
      gdIP:    Pointer to the image canvas that is to be drawn
               on.

  Return Value:   0 if successful, -1 on error
  Author:
               DK, before 04/15/1999, 

  Internal Functions Called:
  Library Functions Called: 
               logit()
  EW Library Functions Called:
  External Library Functions Called:
               gdImageLine()
  System Functions Called:  
               sprintf()
  **********************************************************/
{
  int j,StartingX;
  int PreWidth,PreTicks;
  float TickWidth;

  /* Draw the five border lines: Top, Bottom, Left, 
     Between Header and Actual Trace, and Right */
  gdImageLine(gdIP,Left,Top,Left+Width,Top,gdBlack);
  gdImageLine(gdIP,Left,Top+Height,Left+Width,Top+Height,gdBlack);
  gdImageLine(gdIP,Left,Top,Left,Top+Height,gdBlack);
  gdImageLine(gdIP,Left+HeaderWidth,Top,Left+HeaderWidth,Top+Height,gdBlack);
  gdImageLine(gdIP,Left+Width,Top,Left+Width,Top+Height,gdBlack);
 
  /* Figure out width from TZero to End */
  PreWidth=(int)(((float)PrePickPcnt)/(100+PrePickPcnt) * (Width-HeaderWidth));
  PreTicks=(int)(PrePickPcnt * (((float)NumOfTicks)/100));
  if(DEBUG)
    logit("","PrePickPcnt %d, PreTicks %d, PreWidth %d.\n",
          PrePickPcnt,PreTicks,PreWidth);

  /* Time ticks from Time Zero */
  TickWidth=((float)(Width-HeaderWidth-PreWidth)) / NumOfTicks;
  StartingX=Left+HeaderWidth+PreWidth;

  if(DEBUG)
    logit("","TickWidth %d, StartingX %d, Left+HeaderWidth %d.\n",
          TickWidth,StartingX,Left+HeaderWidth);

  for(j=0-PreTicks;j<NumOfTicks;j++)
  {
    if(!(j%8))  /* Whole Tick */
    {
      gdImageLine(gdIP,StartingX + (int)(j*TickWidth),Top,
                       StartingX + (int)(j*TickWidth),Top+((Height/2)+1),gdBlack);
      gdImageLine(gdIP,StartingX + (int)(j*TickWidth),Top+Height,
                       StartingX + (int)(j*TickWidth),
                       Top+Height-((Height/2)+1),gdBlack);
    }
    else if(!(j%4))  /* Major Tick */
    {
      gdImageLine(gdIP,StartingX + (int)(j*TickWidth),Top,
                       StartingX + (int)(j*TickWidth),Top+((Height/10)+1),gdBlack);
      gdImageLine(gdIP,StartingX + (int)(j*TickWidth),Top+Height,
                       StartingX + (int)(j*TickWidth),
                       Top+Height-((Height/10)+1),gdBlack);
    }
    else if(!(j%2))  /* Medium Tick */
    {
      gdImageLine(gdIP,StartingX + (int)(j*TickWidth),Top,
                       StartingX + (int)(j*TickWidth),Top+((Height/20)+1),gdBlack);
      gdImageLine(gdIP,StartingX + (int)(j*TickWidth),Top+Height,
                       StartingX + (int)(j*TickWidth),
                       Top+Height-((Height/20)+1),gdBlack);
    }
    else   /* Minor Tick */
    {
      gdImageLine(gdIP,StartingX + (int)(j*TickWidth),Top,
                       StartingX + (int)(j*TickWidth),
                       Top+((Height/40)+1),gdBlack);
      gdImageLine(gdIP,StartingX + (int)(j*TickWidth),Top+Height,
                       StartingX + (int)(j*TickWidth),
                       Top+Height-((Height/40)+1),gdBlack);
    }
  }  /* end for j < NumOfTicks */


  return(0);
}  /* End of DrawBorders */

int DrawActualTrace(gdImagePtr gdIP, SnippetDrawingStruct * pSDrS)
/******************************************************
  Function:    DrawActualTrace()
  Purpose:     Calculate and draw the actual wiggly lines 
               for a given record section
               
  Parameters:    
      Input
      pSDrS:   Pointer to all information neccessary to draw
               the record section, including the dimensions
               of the drawing area, the Snippet data, and
               station info, time granularity, and the kitchen
               sink.

      Input/Output
      gdIP:    Pointer to the image canvas that is to be drawn
               on.


  Return Value:   0 if successful, -1 on error
  Author:
               DK, before 04/15/1999, 

  Internal Functions Called:
               CalculateTraceParams(),GetTraceDataPoint()
  Library Functions Called: 
               logit()
  EW Library Functions Called:
  External Library Functions Called:
               gdImageLine()
  System Functions Called:  
  **********************************************************/
{
  double NextTime;
  int LastPointIsValid=0;
  int CurrentOffset=0;
  int RetVal, i, DataPoint;
  int LastY, ThisY;
  float DataPointsPerPixel;
  unsigned int PossibleVerticalPoints;
  int Max,Min;

  if(!pSDrS->pTrace)
  {
    logit("","DrawActualTrace()NULL Snippet for %s,%s,%s.  Returning error.\n",
          pSDrS->SCN.Sta,pSDrS->SCN.Comp,pSDrS->SCN.Net);
    return(-1);
  }

  NextTime=pSDrS->StartTime;

  /* Calculate the possible conceivable amplitude range based on whether the
     data is 2 byte or 4 byte.  We call this the "possible conceivable"
     amplitude range, because a lot fo the 4 byte data (for example) is
     24-bit, which can possibly span a 4-byte (32-bit)address range. But
     we don't know the exact possible bit range of the data, we only have
     the data broken down into 2 byte and 4 byte. 
  *************************************************/
  if(pSDrS->pTrace->datatype[1] == '4')
    PossibleVerticalPoints = ((unsigned int) -1);
  else if(pSDrS->pTrace->datatype[1] == '2')
    PossibleVerticalPoints = ((unsigned short) -1);
  else
  {
    logit("","DrawActualTrace: Unmanagable datatype in initial "
             "TRACE_HEADER: %s, for %s,%s,%s:%9.2f\n",
             pSDrS->pTrace->datatype,pSDrS->SCN.Sta,
             pSDrS->SCN.Comp,pSDrS->SCN.Net,pSDrS->StartTime);
    return(-1);
  }


  /* Calculate the conversion factor from amplitude data 
     points to pixels.  For 4-byte data, this would be 
     (2^8^4)/ TraceHeight, where TraceHeight might be 100 pixels.
  **************************************************/
  DataPointsPerPixel = ((float)PossibleVerticalPoints)/TraceHeight;
  if(DEBUG)
    logit("","DPPP %.2f, PVP %d, TH %d\n",DataPointsPerPixel,
          PossibleVerticalPoints,TraceHeight);

  /* Calculate Zoom Size and Max Amplitude */
  /* ZoomSize is the magnification factor used to increase the 
     wave amplitude in terms of pixels.  Imagine a 2-byte snippet
     of trace data from a 12-bit digitizer. The data range for this
     particular snippet is (-2048, 2047).  In order to accomodate all
     2-byte data, the maximum allowable amplitudes for drawing would
     be (-32k,32k -1).  Without some sort of waveform amplitude 
     magnification, the waveform would look really tiny, even though
     it was maxed out on the sensor, because -2048 doesn't look like
     much on a scale from 0 to - 32768.  This note has probably caused
     great confusion, and I fear that I will never explain this clearly,
     so I would recommend that you run this program, and set ZoomFactor
     to 1 in the config file.  The waves will be accurately drawn for
     there particular datatype.  Look at how small (vertically) they
     are, even for a large earthquake.  Now set the ZoomFactor value to
     10, and see that the waves appear 10 times bigger and much easier
     to look at.  Lastly set the ZoomFactor to 0, and this program will
     automatically calculate and use the maximum zoom factor supported 
     by the data.  The Scale(ZoomFactor) used is displayed in the header
     on the left of the wiggles.
     DavidK 04/21/99 during code cleanup.  Original code was written
     much earlier, and I have been frustrated while trying to explain
     it ever since.
  *********************************************************************/
  if(ZoomSize)
  {
    pSDrS->Scale=ZoomSize;
  }
  else
  {
    /*  ZoomSize is auto */
    CalculateTraceParams(pSDrS->pTrace,CurrentOffset,pSDrS->TraceLen,
                         &(pSDrS->AmpMax),&Max,&Min,&(pSDrS->SampleSize));
    if(DEBUG)
    {
      logit("","CalcTracParams() completed.  AmpMax %d, Max %d, Min %d.\n",
            pSDrS->AmpMax,Max,Min);
      logit("","SampleSize = %d.\n",pSDrS->SampleSize);
    }

    /* We cannot scale based upon the range Max - Min, because we would
       like to keep the 0 line in place, thus we must scale based on
       the maximum of 2*Max or |2*Min|
    ***********************************/
    if(Max > 0-Min)
    {
      pSDrS->Scale=PossibleVerticalPoints/(2*Max);
    }
    else
    {
      pSDrS->Scale=PossibleVerticalPoints/(2*(0-Min));
    }
  }
      

/* Magnify Trace(Amplitude) By Local Zoom Size */
  if(DEBUG)
    logit("","Scale = %d.\n",pSDrS->Scale);
  DataPointsPerPixel/=pSDrS->Scale;
  if(DEBUG)
     logit("","Trace Height = %d DataPPerPix= %.2f, Scale = %d.\n",
           TraceHeight,DataPointsPerPixel,pSDrS->Scale);

  /* Get and plot all of the relevant datapoints */
  for(i=pSDrS->StartingX+1; i < (pSDrS->PixelWidth + pSDrS->StartingX); i++)
  {
    if(!(RetVal=GetTraceDataPoint(NextTime,pSDrS->pTrace,&CurrentOffset,
                 pSDrS->TraceLen,&DataPoint,(float).02 /*seconds*/)))
    {
      ThisY=(int)(pSDrS->CenterY-(DataPoint/DataPointsPerPixel));
      if(ThisY < (pSDrS->CenterY - (TraceHeight/2)))
        ThisY= pSDrS->CenterY - (TraceHeight/2);
      if(ThisY > (pSDrS->CenterY + (TraceHeight/2)))
        ThisY= pSDrS->CenterY + (TraceHeight/2);

      /* Draw a line from the last point to the current point, but only
         if the last point was valid */
      if(LastPointIsValid)
      {
        gdImageLine(gdIP,i-1,LastY,i,ThisY,gdBlue);
      }
      else
      {
        gdImageLine(gdIP,i-1,ThisY,i,ThisY,gdBlue);
        /*logit("","Lastpoint !valid. Calling gdImageLine(X,(%d,%d),(%d,%d),X).\n",
              i-1,LastY,i,ThisY); */
        LastPointIsValid=1;
      }
      LastY=ThisY;
    }
    else
    {
      LastPointIsValid=0;
      if(RetVal < GTDP_INGAP_ERROR) /* This implies either GTDP_BADOFFSET_ERROR
                                     or GTDP_CORRUPTDATA_ERROR both of which
                                     are like getting the plague (sp?)  */
      {
        logit("","GetTraceDataPoint() returned %d with %d as datapoint.\n",
              RetVal,DataPoint);
        return(-1);  /* This is a more fitting ending to a bad error
                        than just letting it slide DK 042199 */
      }

      if(RetVal == GTDP_RIGHTOFSNIPPET_ERROR)
      {
        logit("","Quitting after %d of %d datapoints processed, due to right of snippet.\n",
                  i-pSDrS->StartingX-1,
                  (pSDrS->PixelWidth + pSDrS->StartingX)-pSDrS->StartingX);
        break;  /* we are right of snippet, no more data. */
      }
    }

    NextTime+=pSDrS->SecsPerPixel;
  }  /* end of for datapoint requests loop */

  return(0);
}  /* end DrawActualTrace() */


int GetPrefSummaryInfoFromDB(EWDB_OriginStruct * pOS, EWDB_MagStruct * pMS,
                             int idEvent)
/******************************************************
  Function:    GetPrefSummaryInfoFromDB()
  Purpose:     Fill user allocated structs with the preferred
               summary infor for Event #EventID. 
               
  Parameters:    
      Input
      EventID: The DB EventID for a chosen event.

      Output
      pOS:     Pointer to Origin information for the given
               EventID
      pMS:     Pointer to Magnitude information for the given
               EventID

  Return Value:   0 if successful, -1 on error
  Author:
               DK, before 04/15/1999, 

  Oracle API Functions:
               ewdb_api_GetPreferredSummaryInfo(),ewdb_api_GetOrigin(),
               ewdb_api_GetMagnitude()
  System Functions Called:  
  **********************************************************/
{
  
  EWDBid idMag,idMech,idOrigin;
  int RetVal;

  RetVal=ewdb_api_GetPreferredSummaryInfo(idEvent,&idOrigin,&idMag,&idMech);
  if(RetVal == EWDB_RETURN_FAILURE)
  {
    logit("","GetPrefSummaryInfoFromDB(): ewdb_api_GetPreferredSummaryInfo() Failed!\n");
    return(-1);
  }

  ewdb_api_GetOrigin(idOrigin,pOS);
  if(RetVal == EWDB_RETURN_FAILURE)
  {
    logit("","GetPrefSummaryInfoFromDB(): ewdb_api_GetPreferredSummaryInfo() Failed!\n");
    return(-1);
  }

  ewdb_api_GetMagnitude(idMag,pMS);
  if(RetVal == EWDB_RETURN_FAILURE)
  {
    logit("","GetPrefSummaryInfoFromDB(): ewdb_api_GetPreferredSummaryInfo() Failed!\n");
    return(-1);
  }

  return(0);
}  /* End of GetPrefSummaryInfoFromDB() */


int GetArrivalsFromDB(EWDB_ArrivalStruct ** ppAS, int idOrigin, int * pNumArrivals)
/******************************************************
  Function:    GetArrivalsFromDB()
  Purpose:     Retrieve arrivals from the DB for a specified
               Origin ID, using the ORA_API functions.
               
  Parameters:    
      Input
      OriginID: The DB OriginID for a chosen origin.
      GLOBAL VARIABLE AllocatedArrivalStructs:
               Number of ArrivalStructs to allocate
               for getting arrivals from DB.  Note that
               this is used merely as a starting number.
               The function will attempt to retrieve all
               of the arrivals even if it is more than
               this Global Var.

      Output
      ppAS:    Pointer to a pointer to a list of Arrival Structs.  
               This pointer allows the function to set the
               underlying pAS so that it points at the list
               of Arrival Structs that are retrieved from the 
               database, after space is malloced.
      pNumArrivals:
               Number of arrivals retrieved from the DB.
  Return Value:   0 if successful, -1 on error
  Author:
               DK, before 04/15/1999, 

  Internal Functions Called:
  Library Functions Called: 
               logit(),
  Oracle API Functions:
               GetArrivals(),
  EW Library Functions Called:
  External Library Functions Called:
  System Functions Called:  
               malloc(),sizeof(),free()
  **********************************************************/
{

  EWDB_ArrivalStruct * pAS;
  int RetVal;
  int ArrivalsFound;


  if(!(pAS=(EWDB_ArrivalStruct *)malloc(AllocatedArrivalStructs * sizeof(EWDB_ArrivalStruct))))
  {
    logit("","malloc of ArrivalStructs(%d) failed in GetArrivalsFromDB.\n",
          AllocatedArrivalStructs);
    return(-1);
  }

  /* Oh boy, we start with a global variable, but then allocate more
     if we need it, all the while, no resetting the global var.??
     davidk 20000105
  *****************************************/
  RetVal=ewdb_api_GetArrivals(idOrigin, pAS, &ArrivalsFound, pNumArrivals,
                     AllocatedArrivalStructs);

  if(RetVal == EWDB_RETURN_FAILURE)
  {
    logit("","GetArrivals() failed in GetArrivalsFromDB().\n");
    return(-1);
  }
  if(ArrivalsFound > AllocatedArrivalStructs)
  {
    /* We did not allocate enough space, so lets try it again. */
    free(pAS);
    if(!(pAS=(EWDB_ArrivalStruct *)malloc(ArrivalsFound * sizeof(EWDB_ArrivalStruct))))
    {
      logit("","malloc of %d ArrivalStructs failed in "
            "GetArrivalsFromDB().\n",ArrivalsFound);
      return(-1);
    }
    RetVal=ewdb_api_GetArrivals(idOrigin, pAS, &ArrivalsFound, pNumArrivals,
                     ArrivalsFound);
    if(RetVal == EWDB_RETURN_FAILURE)
    {
      logit("","GetArrivals()2 failed in GetArrivalsFromDB().\n");
      return(-1);
    }
  }  /* end if(ArrivalsFound > AllocatedArrivalStructs) */

  /* Copy the address of the ArrivalStructs out to the caller's pointer. */
  *ppAS=pAS;
  return(0);
}  /* End of GetArrivalsFromDB() */


int GetSnippetListFromDB(SnippetDrawingStruct ** ppSDrS, 
                      EWDBid idEvent, int * pNumSnippets)
/******************************************************
  Function:    GetSnippetListFromDB()
  Purpose:     Retrieve snippet descriptors from the DB 
               for a specified Event, using the ORA_API functions.
               
  Parameters:    
      Input
      EventID: The DB EventID for a chosen event.

      Output
      ppSDrS:    Pointer to a pointer to a list of Snippet
               Descriptor Structs.  
               This pointer allows the function to set the
               underlying pSDrS pointer  so that it points at 
               the list of Snippet Descriptors that are retrieved 
               from the database, after space is malloced.
      pNumSnippets:
               Number of snippet descriptors retrieved from the DB.

  Return Value:   0 if successful, -1 on error
  Author:
               DK, before 04/15/1999, 

  Internal Functions Called:
  Library Functions Called: 
               logit(),
  Oracle API Functions:
               GetSnippetList(),
  EW Library Functions Called:
  External Library Functions Called:
  System Functions Called:  
               malloc(),sizeof(),free()
  **********************************************************/
{

  EWDB_WaveformStruct * pWS;
  EWDB_StationStruct * pSS;
  SnippetDrawingStruct * pSDRS;
  int RetVal,i,NumSnippetsFound;


  if(!(pWS=(EWDB_WaveformStruct *)malloc(ALLOCATED_SDS * sizeof(EWDB_WaveformStruct))))
  {
    logit("","malloc of WSs failed in GetArrivalsFromDB().\n");
    return(-1);
  }

  if(!(pSS=(EWDB_StationStruct *)malloc(ALLOCATED_SDS * sizeof(EWDB_StationStruct))))
  {
    logit("","malloc of SSs failed in GetArrivalsFromDB().\n");
    return(-1);
  }


  RetVal=ewdb_api_GetWaveformListByEvent(idEvent,pWS,pSS,TRUE,
                                         &NumSnippetsFound,pNumSnippets,
                                         ALLOCATED_SDS);

  if(RetVal == EWDB_RETURN_FAILURE)
  {
    logit("","ewdb_api_GetWaveformListByEvent() failed in GetSnippetsFromDB().\n");
    return(-1);
  }
  if(NumSnippetsFound > ALLOCATED_SDS)
  {
    /* We did not allocate enough space, so lets try it again. */
    free(pWS);
    if(!(pWS=(EWDB_WaveformStruct *)malloc(NumSnippetsFound * sizeof(EWDB_WaveformStruct))))
    {
      logit("","malloc of %d WSs failed in "
            "GetSnippetsFromDB().\n",NumSnippetsFound);
      return(-1);
    }
    if(!(pSS=(EWDB_StationStruct *)malloc(NumSnippetsFound * sizeof(EWDB_StationStruct))))
    {
      logit("","malloc of %d SSs failed in "
            "GetSnippetsFromDB().\n",NumSnippetsFound);
      return(-1);
    }
  RetVal=ewdb_api_GetWaveformListByEvent(idEvent,pWS,pSS,TRUE,
                                         &NumSnippetsFound,pNumSnippets,
                                         NumSnippetsFound);
    if(RetVal == EWDB_RETURN_FAILURE)
    {
      logit("","ewdb_api_GetWaveformListByEvent()2 failed in GetSnippetsFromDB().\n");
      return(-1);
    }
  }  /* End if(RetVal) from GetSnippetList() call */

  /* Now, malloc an array of snippet drawing structs, which
     containt snippet(char) pointers, plus some snippet and
     station information. */
  if(!(pSDRS=(SnippetDrawingStruct *)
              calloc(*pNumSnippets * sizeof(SnippetDrawingStruct),1)))
  {
    logit("","malloc of array of %d SDrS's "
          "failed in GetSnippetsFromDB().\n", *pNumSnippets);
    return(-1);
  }

  /* This is what we were thinking when designing this function:
     We need logic that does this:  (Don't know if this hurts or helps)
     once we get a list of snippet descriptors, while mallocing
     the space for snippet array pointer, or maybe instead of,
     malloc spot for snippetdrawingstruct pointers, or even
     snippet drawingstructs.  now I have an array of snippet
     drawing structs, retrieve the snippet into the trace parameter,
     and then grab the SCN info for each snippet into the SDS.SCN
     record.  By mallocing the SDSs, instead of pointers, this 
     becomes simple.  Remember also to put the length in the
     TraceLen parameter of the SDS struct.  This makes this
     function a bit more proprietary, but we really need
     to store length unless we are going to go through this
     db_connection process all over again.  Thanks,
     David  (Sometime in 1998)
  */


  /* Now iterate through each snippet descriptor, copy the information
     from the SnippetDescriptor to the SnippetDrawingStruct for each
     snippet and  also grab the SCN information for each. */
  for(i=0;i<(*pNumSnippets);i++)
  {
    pSDRS[i].idWaveform=pWS[i].idWaveform;
    pSDRS[i].TraceLen=pWS[i].iByteLen;
    memcpy(&(pSDRS[i].SCN),&(pSS[i]),sizeof(EWDB_StationStruct));

    /*
    strcpy(pSDRS[i].SCN.Sta, pSS[i].Sta);
    strcpy(pSDRS[i].SCN.Comp, pSS[i].Comp);
    strcpy(pSDRS[i].SCN.Net, pSS[i].Net);
    GetStation(pSDRS[i].SCN.Sta,pSDRS[i].SCN.Comp,pSDRS[i].SCN.Net,
               &(pSDRS[i].SCN));
    */

    /* Don't get the snippets yet.  These are just the contestants,
       it takes a lot of time to grab snippets, we will do that only
       for finalists. 
    ************************************/
  }  /* end for each snippet descriptor */

  /* set the caller's pointer */
  *ppSDrS=pSDRS;  

  /* free the intermediary(SnippetDescriptorStruct) array, because we
     copied all of the useful information from it to the 
     SnippetDrawingStruct array.  */
  free(pWS);
  free(pSS);

  /* done */
  if(DEBUG)
    logit("","Successfully completed GetSnippetsFromDB().\n");
  return(0);
}

int OrderSnippetsandArrivals(EventSnippetDrawingStruct * pESDrS)
/******************************************************
  Function:    GetSnippetListFromDB()
  Purpose:     Match snippets up with arrivals, and then attempt
               to order them by distance.
               
  Parameters:    
      Input/Output
      pESDrs:  pointer to a structure that contains all of
               the data for a particular event, including
               the origin, a list of snippets, and a list of
               arrivals.  This function takes the snippet and
               arrival lists, matches items from the two up,
               and creates a list of matched items, ordered
               by distance from the Origin Hypocenter.
               
  Return Value:   0 if successful, -1 on error

  Author:
               DK, before 04/15/1999, 

  Internal Functions Called:
  Library Functions Called: 
               logit()
  Oracle API Functions:
  EW Library Functions Called:
  External Library Functions Called:
  System Functions Called:  
               sizeof(),strcmp(),sqrt(),pow(),memcpy()
  **********************************************************/
{
  /* yuck */
  /* this function is charged with cleaning up all of the 
     pieces that the others have left behind, all of the
     gaps in the design, and anything else that comes along.

     What we've got is an array of arrivals, an array of raw snippets,
     and an empty array of snippet drawing structs.  We need to
     match snippets up with arrivals.  Then we have to go back into
     the database, and grab the location of each station, for which
     we have info.  We then have to calculate the distance of each
     station, and sort from there.  Sounds good, yeah?
  */

  int MaxSnippets; 
  int ctr,i,j,k=0;
  EWDB_ArrivalStruct * pAR;
  SnippetDrawingStruct * pSDrS;
  EWDB_OriginStruct * pOR;
  PlotTraceStruct * pPTS;
  PlotTraceStruct PTSTemp;

  if(DEBUG)
    logit("","Entered OrderSnippetXXX.\n");

  /* Set MaxSnippets to the maximum number of snippets we could possible
     care about. 
  ***********************/
  if(pESDrS->NumOfSnippets > pESDrS->NumOfArrivals)
    MaxSnippets = pESDrS->NumOfSnippets;
  else
    MaxSnippets = pESDrS->NumOfArrivals;


  /* Allocate a PlotTraceStruct array, which will contain the matched
     pairs of snippets and arrivals.  PlotTraceStructs only contains
     pointers to snippets and arrivals, we're not going to replicate
     all of that data.  Phew!!!
  ***********************/
  pESDrS->pPTS= (PlotTraceStruct *) calloc(MaxSnippets * sizeof(PlotTraceStruct),1);
  if(!pESDrS->pPTS)
  {
    logit("","Malloc failed for %d PlotTraceStructs in "
             "OrderSnippetsandArrivals().\n", MaxSnippets);
    return(-1);
  }

  if(DEBUG)
    logit("","After MaxSnippets=%d in OrderSnippetXXX.\n", MaxSnippets);

  /* shortcut to current plotting struct */
  pPTS=&(pESDrS->pPTS[k]);

  /* go through the list of arrivals.  For each arrival, go
     through the list of snippets until we find one with
     an SCN that matches the current arrival 
  ******************/
  for(i=0; i<pESDrS->NumOfArrivals; i++)
  {
    /* give us a shortcut to the current arrival, for me,
       the dimwitted programmer who can barely type. */
    pAR=&(pESDrS->pArrivals[i]);


    for(j=0; j<pESDrS->NumOfSnippets; j++)
    {
      /* shortcut for current SDrS */
      pSDrS=&(pESDrS->pSnippets[j]);

      if(pAR->idChan == pSDrS->SCN.idChan)
      {
        /* we have found a match, maybe!!! */
        if(pSDrS->TraceLen == 0)  /* Empty Snippet */
        {
          /* useless snippet, blow it off! */
          continue;
        }
        /* we found a good match, copy both to the current plotting struct */
        pPTS->pArrival=pAR;
        pPTS->pSDrS=pSDrS;
        k++;
        pPTS=&(pESDrS->pPTS[k]);
        break;
      }

    }  /* end for j < NumOfSnippets */
    if(j == pESDrS->NumOfSnippets)
    {
      /* We didn't find a match for the current arrival.
         No idea what to do.  Ignore it for now, until
         someone commands otherwise.
      */
    }
  }   /* end for i < NumOfArrivals */

  if(DEBUG)
    logit("","Finished Looping through arrivals in OrderSnippetXXX. \n");

  /* OK, so we have matched up all of the arrival/snippet pairs,
     and we have exactly k of them.  Now we probably want to grope
     for distances from the Hypocenter, and sort the records by
     distance.
  **************************************/

  /* Shortcut reference to the Origin structure */
  pOR=&(pESDrS->os);

  /* anybody ever heard of qsort()???? */
  for(i=0;i<k;i++)
  {
    pPTS=&(pESDrS->pPTS[i]);
    /* We should use epicentral distance since it is standard. */
    for(j=0;j<i;j++)
    {
      if(pPTS->pArrival->dDist < pESDrS->pPTS[j].pArrival->dDist)
      { 
        memcpy(&PTSTemp,pPTS,sizeof(PlotTraceStruct));
        for(ctr=i;ctr>j;ctr--)
        {
          memcpy(&(pESDrS->pPTS[ctr]),&(pESDrS->pPTS[ctr-1]),
                 sizeof(PlotTraceStruct));
        }
        memcpy(&(pESDrS->pPTS[ctr]),&PTSTemp,sizeof(PlotTraceStruct));
        break;
      }
    }
  }  /* for i < k where k is number of PTS's to sort. */


  /* set number of plots in the event drawing struct. */
  pESDrS->NumOfPlots=k;

  if(DEBUG)
    logit("","Successfully completed OrderSnippetXXX.  pESDrS->NoP=%d\n",
           pESDrS->NumOfPlots);

  return(0);
}  /* End of OrderSnippetsandArrivals() */


int ProcessNextDP(CTPDataStruct * pCTPD)
/******************************************************
  Function:    ProcessNextDP()
  Purpose:     Examine the current datapoint and see if it
               affects any of our statistics for the record section.
               
  Parameters:    
      Input/Output
      pCTPD :  Catchall struct for tracking trace parameters.  Contains
               things like min/max, current traced datapoint value,
               local min, local max, etc.
               
  Return Value:   0    Success
                 -1    Failure
                  1    Success, New AmpMax
  Author:
               DK, before 04/15/1999, 

  Internal Functions Called:
  Library Functions Called: 
               logit()
  Oracle API Functions:
  EW Library Functions Called:
  External Library Functions Called:
  System Functions Called:  
  **********************************************************/
{
  int RetVal=0;

  /* Reset Max and Min if the current datapoint is more/less
     than them */
  if(pCTPD->DataPoint > pCTPD->Max)
    pCTPD->Max=pCTPD->DataPoint;
  if(pCTPD->DataPoint < pCTPD->Min)
    pCTPD->Min=pCTPD->DataPoint;


  /* Check and see if the wave was going a certain direction (up/down)
     as of the last data point */
  if(pCTPD->direction)
  {
    if(pCTPD->direction == CTP_DOWN)
    {
      /* Check to see if we are still going down, adjust the local min
         and amplitude max if neccessary */
      if(pCTPD->DataPoint >= pCTPD->Last)
      {
        pCTPD->LocalMin=pCTPD->Last;

        if((unsigned int)(pCTPD->LocalMax - pCTPD->LocalMin) > (pCTPD->AmpMax))
        {
          pCTPD->AmpMax=pCTPD->LocalMax - pCTPD->LocalMin;
          RetVal=1;
        }
        if(pCTPD->DataPoint > pCTPD->Last)
        {
          pCTPD->direction=CTP_UP;
        }
        else
          pCTPD->direction=CTP_FLAT;
      }
    }
    else  /* CTP_UP */
    {
      /* Check to see if we are still going up, adjust the local max
         and amplitude max if neccessary */
      if(pCTPD->direction != CTP_UP)
      {
        logit("","Thought direction up, but is %d.\n",pCTPD->direction);
      }
      if(pCTPD->DataPoint <= pCTPD->Last)
      {
        pCTPD->LocalMax = pCTPD->Last;

        if((unsigned int)(pCTPD->LocalMax - pCTPD->LocalMin) > pCTPD->AmpMax)
        {
          pCTPD->AmpMax = pCTPD->LocalMax - pCTPD->LocalMin;
          RetVal=1;
        }
        if(pCTPD->DataPoint < pCTPD->Last)
          pCTPD->direction=CTP_DOWN;
        else
          pCTPD->direction=CTP_FLAT;
      }  /* if(pCTPD->DataPoint <= pCTPD->Last) */
    }  /* else CTP_UP */
  }    /* if(direction) */
  else  /* direction=CTP_FLAT */
  {
    if(pCTPD->DataPoint != pCTPD->Last)
    {
      if(pCTPD->DataPoint > pCTPD->Last)
      {
        pCTPD->direction=CTP_UP;
      }
      else
        pCTPD->direction=CTP_DOWN;
    }
  }  /* else  direction=CTP_FLAT */

  return(RetVal);
}  /* ProcessNextDP() */


int CalculateTraceParams(TRACE_HEADER * pTrace,int Offset,int TraceLen,
                         int * pAmpMax,int *pMax, int *pMin, 
                         int * pSampleSize)
/******************************************************
  Function:    CalculateTraceParams()
  Purpose:     Go through a snippet or atleast a snipette,
               find and report the Maximum Amplitude, the
               Max and Min data values, and the 
               SampleSize (2 or 4 byte) for the series of data.
               
  Parameters:    
      Input
      pTrace : Pointer to Snippet
      Offset : The offset from the start of the snippet, where
               we are supposed to start sniffing. (Starting Point)
      TraceLen:
               How far past pTrace we are supposed to sniff.
               (Ending Point)

      Output
      pAmpMax: Where we stick the Maximum Amplitude for the given trace.
      pMax:    Where we stick the max value for the give trace.
      pMin:    Where we stick the min value for the give trace.
      pSampleSize:
               Pointer to the location where we stick the sample
               size of the data for the snippet (2 byte or 4 byte)
      GLOBAL VARIABLES LocalWaves:
               Indicates whether snippets have been converted
               to the byte order of this machine.
               
  Return Value:   0 on success, -1 on error
  Author:
               DK, before 04/15/1999, 

  Internal Functions Called:
               ProcessNextDP()
  Library Functions Called: 
               logit()
  Oracle API Functions:
  EW Library Functions Called:
               WaveMsgMakeLocal()
  External Library Functions Called:
  System Functions Called:  
               sizeof(),atoi()
  **********************************************************/
{
  CTPDataStruct CTPD;
  int NumOfSamples;
  int SampleSize;
  TRACE_HEADER * pCurr;
  int PointNum,RetVal;
  unsigned int AmpMaxOffset;



  if(DEBUG)
    logit("","Entered CalculateTraceParams().\n");

  /* Get the starting TraceBuf message */
  pCurr=(TRACE_HEADER *)(((int) pTrace) + Offset);

  /* Convert the binary trace message to the byte ordering
     of this machine 
  *******************/
  WaveMsgMakeLocal(pCurr);
  /* Record that we have already MakeLocal'd the waves. */
  LocalWaves=1;

  if(DEBUG)
    logit("","pCurr= %u, pTrace %u, T_H %u.\n",
          pCurr,pTrace,sizeof(TRACE_HEADER));

  /* Get the datapoint size for this snippet.  Currently
     2-byte or 4-byte
  ********************/
  SampleSize=atoi(&(pCurr->datatype[1]));

  if(DEBUG)
    logit("","SampleSize is %d, after atoi(deref).\n",SampleSize);

  /* Set CTPD.Last to be the value of the first datapoint of the trace.
  ************/
  if(SampleSize == 2)
  {
    CTPD.Last=*((short *)
                 (((int)pCurr)+sizeof(TRACE_HEADER))
                );  /* this could just be pCurr+1 */
  }
  else if(SampleSize == 4)  /* SampleSize == 4 */
  {
    CTPD.Last=*((int *)
                 (((int)pCurr)+sizeof(TRACE_HEADER))
                );
  }
  else
  {
    logit("","CalculateTraceParams(): Unhandleable datatype: %s\n",
      pTrace->datatype);
    return(-1);
  }

  /* Initialize the Min's and Max's to the value of the first datapoint. */
  CTPD.Max=CTPD.Min=CTPD.LocalMin=CTPD.LocalMax=CTPD.Last;
  /* Set the direction to flat and the max amplitude to 0 */
  CTPD.direction=CTP_FLAT;
  CTPD.AmpMax=0;

  /* Keep going through all of the Trace_Buf messages until we
     get to the end of the snippet */
  while(Offset < TraceLen)
  {
    pCurr=(TRACE_HEADER *)(((int) pTrace) + Offset);
    WaveMsgMakeLocal(pCurr);
    NumOfSamples=pCurr->nsamp;

    /* Go through all the datapoints in the current Trace Buf packet */
    for(PointNum=0;PointNum<NumOfSamples;PointNum++)
    {
      if(SampleSize == 2)
      {
        CTPD.DataPoint=*((short *)
                    (((int)pCurr)+sizeof(TRACE_HEADER)+ (2*PointNum))
                   );
      }
      else  /* SampleSize == 4 */
      {
        CTPD.DataPoint=*((int *)
                    (((int)pCurr)+sizeof(TRACE_HEADER)+ (4*PointNum))
                   );
      }

      /* Process this datapoint */
      RetVal=ProcessNextDP(&CTPD);
      if(RetVal)
      {
        if(RetVal == 1)
        {
          AmpMaxOffset=((int)pCurr)+sizeof(TRACE_HEADER)+(SampleSize*PointNum);
        }
        else
        {
          logit("","CalculateTraceParams(): ProcessNextDP() failed.\n");
          return(-1);
        }
      }

      /*  Move current to last(previous) */
      CTPD.Last=CTPD.DataPoint;

    }  /* End for PointNum < NumOfSamples */

    Offset+=sizeof(TRACE_HEADER)+(NumOfSamples*SampleSize);
  }  /* End while(Offset < TraceLen) */

  /* Copy local values back to caller's variables */
  *pMax=CTPD.Max;
  *pMin=CTPD.Min;
  *pAmpMax=CTPD.AmpMax;
  *pSampleSize=SampleSize;
  return(0);
}  /* End of CalculateTraceParams() */


int GetTraceDataPoint(double time, TRACE_HEADER * pTrace,
                      int * pOffset, int TraceLen,
                      int * pDataPoint, float GracePeriod)

/******************************************************
  Function:    GetTraceDataPoint()
  Purpose:     Retrieve the value of the next trace data point
               
  Parameters:    
      Input
      time:    The time for the next trace data point
      pTrace:  Pointer to Snippet
      TraceLen:
               How far past pTrace we are supposed to sniff.
               (Ending Point)
      GracePoint:
               How far the time for a datapoint can vary from the
               time the caller specifies before we have to complain
               about the data point not being available.

      Input/Output
      pOffset: Pointer to the offset from the start of the snippet, 
               where we are supposed to start sniffing. (Starting Point)

      Output
      pDataPoint:
               Where we stick the Value of the datapoint we found.
               
  Return Value:
               0                         : Success
               GTDP_UNDEFINED_ERROR      : Undefined Error
               GTDP_LEFTOFOFFSET_ERROR   : Time is Left of Offset
               GTDP_RIGHTOFSNIPPET_ERROR : Time is Right of Snippet
               GTDP_INGAP_ERROR          : Time is in Gap
               GTDP_BADOFFSET_ERROR      : Bad Offset
               GTDP_CORRUPTDATA_ERROR    : Corrupted TraceData
  Author:
               DK, before 04/15/1999, 

  Internal Functions Called:
  Library Functions Called: 
  Oracle API Functions:
  EW Library Functions Called:
  External Library Functions Called:
  System Functions Called:  
  **********************************************************/
{
  TRACE_HEADER * pCurr;
  int Offset=*pOffset;
  int PointNum,SizeOfThisRecord,FirstPass=1;
  int SampSize;

  /* First Record Only */
  if(Offset >= TraceLen)
  {
    logit("","In GetTraceDataPoint(), Offset=%d,TraceLen=%d\n",
          Offset,TraceLen);
    return(GTDP_BADOFFSET_ERROR);
  }

  while(1)
  {
    /* Create a shortcut to the current record */
    pCurr=(TRACE_HEADER *)(((int)pTrace) + Offset);
    if(!LocalWaves)
      WaveMsgMakeLocal(pCurr);

    /* Make sure that the record's starttime and 
       endtime are somewhat in check.  */
    if(pCurr->starttime >= pCurr->endtime)
    {
      logit("","pTrace is %u, pCurr is %u.\n",pTrace,pCurr);
      logit("","Corrupted trace data point found at offset %d.\n"
               "tbuf: st:%f et:%f nsamp:%f .\n",
               Offset,pCurr->starttime,pCurr->endtime,
               pCurr->nsamp);
      return(GTDP_CORRUPTDATA_ERROR);
    }
    
    /* check to see if the time we are searching for is
       greater than the start of the record.  Allow a
       caller defined fudge factor when checking time. */
    if(time < (pCurr->starttime - GracePeriod))
    {
      if(FirstPass)
        /* Left of First Record. */
        return(GTDP_LEFTOFOFFSET_ERROR);
      else
        /* In gaps between records. */
        return(GTDP_INGAP_ERROR);
    }

    /* This assumes less than 4 second packets,
       bad bad bad!!!!! DK 042199*/
    /*if(pCurr->starttime < (pCurr->endtime - 4))
      {
        logit("","Times are funny: st %f, et %f.\n",
                 pCurr->starttime, pCurr->endtime);
        return(-1);
      }
    */
    /* check to see it the time is within this 
       record, give or take a grace period.  */
    if(time < (pCurr->endtime + GracePeriod))
    {
      /* We found the data point, extract it. */

      /* Figure out which datapoint in the record it is. */
      PointNum = (int)((time - pCurr->starttime) * pCurr->samprate);
      /* Correct in case we utilized GracePeriod. */
      if(PointNum < 0)
        PointNum=0;
      if(PointNum >= pCurr->nsamp)
        PointNum=pCurr->nsamp-1;


      /* only handle 2,4 bit data.  commented code for 64-bit */
      /* This statement only compatible with 9(?) byte data or smaller */

      /* Calculate byte size for each sample. */
      SampSize=pCurr->datatype[1]-'0';
      
      if(SampSize == 2)
      {
        *pDataPoint=*((short *)
                      (((int)pCurr)+sizeof(TRACE_HEADER)+
                       (2*PointNum)
                     ));
        
        *pOffset=Offset;
        /* Ah, the wholy grail.  Success. */
        return(0);
      }  /* if sampsize == 2 */

      if(SampSize == 4)
      {
        *pDataPoint=*((int *)
                      (((int)pCurr)+sizeof(TRACE_HEADER)+
                       (4*PointNum)
                     ));
        *pOffset=Offset;
        /* Ah, the wholy grail.  Success. */
        return(0);
      } /* end if sampsize == 4 */
      else
      {
        logit("","Unmanageable datatype: %s in TRACE_HEADER.\n",
          pCurr->datatype);
        return(GTDP_UNDEFINED_ERROR);
      }

    }  /* end if(time <= pCurr->endtime) */
    else
    {
      /* goto the next sample */
      SampSize=pCurr->datatype[1]-'0';
      if(SampSize == 2 || SampSize == 4)
      {
        SizeOfThisRecord=sizeof(TRACE_HEADER) + 
          SampSize * pCurr->nsamp;
        Offset+=SizeOfThisRecord;
        if(Offset >= TraceLen)
          return(GTDP_RIGHTOFSNIPPET_ERROR);
        FirstPass=0;
      }
      else
      {
        logit("","Unmanageable datatype: %s in TRACE_HEADER.\n",
          pCurr->datatype);
        return(-1);
      }
    }  /* else from if(time <= pCurr->endtime) */
  }  /* end while(1) keep going through the trace until we hit
         a return statement.  Can you spell infinite loop?
         But no no, this time will be different.  
      ************************/
}  /* end GetTraceDataPoint() */
