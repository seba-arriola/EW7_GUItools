
/*
 *   This file is under RCS - do not modify unless you have
 *   checked it out using the command checkout.
 *
 *    $Id: ora2rsec_gif.h,v 1.2 2001/02/28 17:29:10 lucky Exp $
 *    Revision history:
 *
 *    $Log: ora2rsec_gif.h,v $
 *    Revision 1.2  2001/02/28 17:29:10  lucky
 *    Massive schema redesign and cleanup.
 *
 *    Revision 1.1  2000/12/18 19:13:47  lucky
 *    Initial revision
 *
 *    Revision 1.1  2000/02/15 19:39:25  lucky
 *    Initial revision
 *
 *    Revision 1.1  2000/01/07 18:28:33  davidk
 *    Initial revision
 *
 *    Revision 1.1  1999/05/05 18:31:15  lucky
 *    Initial revision
 *
 *
 */
  
 /*ora2rsec_gif.h*/
#ifndef ORA2RSEC_GIF_H 
# define ORA2RSEC_GIF_H

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
  int direction,DataPoint,Last,LocalMax,LocalMin,Max,Min;
  unsigned int AmpMax,AmpMaxOffset;
} CTPDataStruct;


/* Function prototypes
 *********************/

/*** FROM config.c ***/
int  ReadConfig( char * );                      /* reads configuration file */


/*** FROM ora2rsec_gif.c ***/
int PlotATrace(gdImagePtr gdIP, PlotTraceStruct * pPTS, 
               int Left, int Top, int Width, int Height,
               int HeaderWidth);
int PlotTrace(EventSnippetDrawingStruct * pESDrS);
int GetSnippetsFromDB(EventSnippetDrawingStruct * pESDrS);


/*** FROM recsec_other.c ***/
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
int DrawEventFooter(gdImagePtr gdIP, int Left, int Top,
                    int Width, int HeaderWidth);
int CalculateTraceParams(TRACE_HEADER * pTrace,int Offset,int TraceLen,
                         int * pAmpMax,int *pMax, int *pMin, 
                         int * pSampleSize);
int OrderSnippetsandArrivals(EventSnippetDrawingStruct * pESDrS);


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


/* DavidK 1/15/1999 to support winnt servers */
#ifdef _WINNT
# define EXE_EXT ".exe"
#else
# define EXE_EXT 
#endif


#endif /* GET_RECSEC_H */
