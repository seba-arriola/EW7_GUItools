
/*
 *   This file is under RCS - do not modify unless you have
 *   checked it out using the command checkout.
 *
 *    $Id: ora2snippet_gif.h,v 1.4 2002/03/28 20:07:48 davidk Exp $
 *    Revision history:
 *
 *    $Log: ora2snippet_gif.h,v $
 *    Revision 1.4  2002/03/28 20:07:48  davidk
 *    Added global variables for ShowAllWiggles and ShowDeleteEventButton config
 *    file commands.
 *    Added prototype for CompareArrivalDistances() for use with qsort() in sorting
 *    waveforms by epicentral distance.
 *
 *    Revision 1.3  2002/03/20 22:01:37  davidk
 *    Added a new member(bTraceHeavilyDemeaned) to the SnippetDrawingStruct,
 *    to note that the trace has been significantly De-Meaned.  This means
 *    that there was a significant bias in the trace.
 *
 *    Cosmetic: Stretched out the declaration of CTPDataStruct, so that
 *    each member is on its own line.
 *
 *    Modified the CTPDataStruct.  Rearranged the members, added two new
 *    members(NumDataPoints, dSumOfDataPoints) used for calculating the
 *    mean trace value, and then demeaning the gifs.
 *
 *
 *    Added prototype for GetTraceDataPointRange()
 *    Added WiggleType constants for describing different
 *    decimation/drawing methods.
 *
 *    Revision 1.2  2002/03/19 22:02:46  lucky
 *    Added support for deletion of events
 *
 *    Revision 1.1  2001/09/26 21:44:37  lucky
 *    Initial revision
 *
 *
 *
 */
  
#ifndef ORA2SNIPPET_GIF_H 
# define ORA2SNIPPET_GIF_H

/* Include time header file */
#include <time.h>

/* Earthworm library header files */
#include <trace_buf.h>
#include <ewdb_ora_api.h>
#include <swap.h>

/* Graphics library headers */
#include <gd.h>
#include <gdfontg.h>
#include <gdfontl.h>
#include <gdfontmb.h>
#include <gdfonts.h>
#include <gdfontt.h>


typedef struct _SnippetDrawingStruct
{
  TRACE_HEADER  * pTrace;
  int             idWaveform;
  double          StartTime;
  int             PixelWidth;
  double          SecsPerPixel;  /* Granularity */
  int             StartingX;
  int             CenterY;
  int             HypoDistm;
  int             TraceLen;
  int             AmpMax;
  int             Scale;
  int             SampleSize;
  int             bTraceHeavilyDemeaned;
  EWDB_StationStruct   SCN;
}  SnippetDrawingStruct;

typedef struct _PlotTraceStruct
{
  EWDB_ArrivalStruct * pArrival;
  SnippetDrawingStruct * pSDrS;
}  PlotTraceStruct;

typedef struct _EventSnippetDrawingStruct
{
  int               idEvent;
  EWDB_OriginStruct os;
  EWDB_MagStruct ms;
  PlotTraceStruct * pPTS;
  int               NumOfPlots;
  EWDB_ArrivalStruct * pArrivals;
  int               NumOfArrivals;
  SnippetDrawingStruct
                  * pSnippets;
  int               NumOfSnippets;
}  EventSnippetDrawingStruct;


/* Catchall struct used by CalculateTraceParams() 
   for tracking trace parameters.  
 *************************************************/
typedef struct _CTPData
{
  int DataPoint;
  int Last;
  int LocalMax;
  int LocalMin;
  int Max;
  int Min;
  unsigned int AmpMax;
  unsigned int AmpMaxOffset;
  int Direction;
  int NumDataPoints;
  double dSumOfDataPoints;
} CTPDataStruct;


/* Function prototypes
 *********************/

/*** FROM config.c ***/
int  ReadConfig( char * );                      /* reads configuration file */


/*** FROM ora2snippet_gif.c ***/
int PlotATrace(gdImagePtr gdIP, PlotTraceStruct * pPTS, 
               int Left, int Top, int Width, int Height,
               int HeaderWidth);
int PlotTrace(EventSnippetDrawingStruct * pESDrS);
int GetSnippetsFromDB(EventSnippetDrawingStruct * pESDrS);


/*** FROM support.c ***/
int DrawEventHeader(gdImagePtr gdIP, int idEvent, EWDB_OriginStruct *pOS,
                    EWDB_MagStruct *pMS,
                    int EventInfoHeight);
int DrawHeader(gdImagePtr gdIP, PlotTraceStruct * pPTS, int Left, int Top, 
               int Width, int Height);
int DrawBorders(gdImagePtr gdIP, int Left, int Top, int Width, 
                int Height, int HeaderWidth);
int DrawActualTrace(gdImagePtr gdIP, SnippetDrawingStruct * pSDrS);
int GetPrefSummaryInfoFromDB(EWDB_OriginStruct * pOS, EWDB_MagStruct * pMS,
                             int idEvent);
int GetArrivalsFromDB(EWDB_ArrivalStruct ** ppAS, int idOrigin, int * pNumArrivals);
int GetSnippetListFromDB(SnippetDrawingStruct ** ppSDrS, 
                      int idEvent, int * pNumSnippets);
int GetTraceDataPoint(double time, TRACE_HEADER * pTrace,
                      int * pOffset, int TraceLen,
                      int * pDataPoint, float GracePeriod);
int GetTraceDataPointRange(double tPoint, double tLast, 
                      TRACE_HEADER * pTrace,
                      int * pOffset, int TraceLen,
                      int * pDataPointMax,
                      int * pDataPointMin,
                      int   iSearchMethod,
                      float GracePeriod);
int DrawEventFooter(gdImagePtr gdIP, int Left, int Top,
                    int Width, int HeaderWidth);
int CalculateTraceParams(TRACE_HEADER * pTrace,int Offset,int TraceLen,
                         int * pAmpMax,int *pMax, int *pMin, 
                         int * pSampleSize, int * pMean);
int OrderSnippetsandArrivals(EventSnippetDrawingStruct * pESDrS);
int RemoveTraceMean (SnippetDrawingStruct *pSDS, double *TraceMean);
int CompareArrivalDistances(const void * pElem1, const void * pElem2);


/*** FROM webhelper.c ***/
int  InputEventIds( int **, int * );    /* webparse.c  eventid's from stdin  */


/*** FROM EXTERNAL LIBRARIES ***/
void logit_init( char *, short, int, int ); /* logit.c      sys-independent  */
void logit( char *, char *, ... );          /* logit.c      sys-independent  */


/* End of Function prototypes
 ****************************/


/*** Externs ***/

/* Parameters set in config file */
extern int GifWidth;
extern int TraceHeight;
extern int HeaderWidth;
extern int WiggleTime;
extern int ZoomSize;
extern int MaxPlots;
extern int NumOfTicks;
extern int WiggleAlign;
extern int PrePickPcnt;
extern int AllocatedArrivalStructs;
extern int WiggleType;
extern int bShowAllWiggles;
extern int bShowDeleteEventButton;

/* Parameters set as constants. */
extern int LocalWaves;
extern int EventInfoHeight;
extern int FooterHeight;

/* Graphics Library Color Var's */
extern int gdRed,gdWhite,gdBlue,gdBrown,gdBlack,gdYellow,gdGreen;

/* Debug flag */
extern int DEBUG;


/*** #define Constants ***/
/* Trace Alignment constants */
#define ALIGN_WIGGLE_ON_OT 1
#define ALIGN_WIGGLE_ON_ARRIVAL 2
#define ALIGN_WIGGLE_ON_8_KM_PER_SEC 3
#define ALIGN_WIGGLE_ON_6_KM_PER_SEC 4
#define ALIGN_WIGGLE_SHOW_ALL 5

/* Calculate Trace Params directional constants */
#define CTP_FLAT 0
#define CTP_UP   1
#define CTP_DOWN -1

/* GetTraceDataPoint() return code constants */
#define GTDP_UNDEFINED_ERROR -1
#define GTDP_LEFTOFOFFSET_ERROR -2
#define GTDP_RIGHTOFSNIPPET_ERROR -3
#define GTDP_INGAP_ERROR -4
#define GTDP_BADOFFSET_ERROR -5
#define GTDP_CORRUPTDATA_ERROR -6

/* WiggleTypes */
#define WIGGLE_TYPE_SINGLE_DATAPOINT 1
#define WIGGLE_TYPE_MAX_DIST 2
#define WIGGLE_TYPE_DOUBLE_PEAK 3
#define WIGGLE_TYPE_HOMEGROWN 4


#ifdef _WINNT
# define EXE_EXT ".exe"
#else
# define EXE_EXT 
#endif


#endif 
