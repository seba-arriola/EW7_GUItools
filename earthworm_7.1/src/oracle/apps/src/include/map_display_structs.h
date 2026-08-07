
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: map_display_structs.h,v 1.10 2002/05/28 17:24:05 lucky Exp $
 *    Revision history:
 *
 *    $Log: map_display_structs.h,v $
 *    Revision 1.10  2002/05/28 17:24:05  lucky
 *    *** empty log message ***
 *
 *    Revision 1.9  2002/03/21 21:37:54  lucky
 *    added ShowUnpicked
 *
 *    Revision 1.8  2001/09/26 21:43:58  lucky
 *    Added support for multi-screen event displays.
 *
 *    Revision 1.7  2001/09/10 20:46:37  lucky
 *    Added support for multi-page display, allowing the user to specify
 *    how many events shoudl be displayed per page.
 *
 *    Revision 1.6  2001/07/01 21:55:17  davidk
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
 *    Revision 1.5  2001/02/28 17:29:10  lucky
 *    Massive schema redesign and cleanup.
 *
 *    Revision 1.4  2000/05/31 16:46:31  lucky
 *    Added ShowMap field to be used in Review/Alarm applications
 *
 *    Revision 1.3  1999/11/09 16:55:39  lucky
 *    *** empty log message ***
 *
 *    Revision 1.2  1999/11/04 21:27:24  davidk
 *    added a new color (green) used for drawing unique borders(needed for circles).
 *
 *    Revision 1.1  1999/10/18 15:56:20  davidk
 *    Initial revision
 *
 *    Revision 1.7  1999/10/01 23:48:46  davidk
 *    updated getlist interface to include 2 colorcoded modes, and support
 *    for recentering a map
 *
 *    Revision 1.6  1999/09/24 06:18:32  davidk
 *    modified the ECSData struct, replaced the LastWeekOnly and LastDayOnly flag
 *    with flags that indicate how many days in the past to look (NumOfDays)
 *    and a flag that tells whether to use the time interval, or to show
 *    the last NumOfDays  (flag is LastDaysOnly)
 *
 *    Revision 1.5  1999/07/21 18:02:27  lucky
 *    increased value of MAX_MAP_CHILDREN
 *
 *    Revision 1.4  1999/06/04 17:58:20  lucky
 *     more minor adjustments needed to make DrawMap work with other modules
 *
 *    Revision 1.1  1999/05/05 18:19:24  lucky
 *    Initial revision
 *
 *
 */
  
#ifndef MAP_DISPLAY_STRUCTS_H
# define MAP_DISPLAY_STRUCTS_H
#include <time.h>
#include <gd.h>
#include <ewdb_ora_api.h>

/* Maximum Filename Length */
#define MAX_FNAME_LEN 128

/* Maximum Number of Children that a map can have */
#define MAX_MAP_CHILDREN 15
#define MAX_MAPID_LENGTH 20

/* Constants for Image Formats */
#define IMAGE_NONE  0
#define IMAGE_GIF   1
#define IMAGE_JPG   2
#define IMAGE_BMP   4
#define IMAGE_PS    8
#define IMAGE_RS1  16
#define IMAGE_RS2  32
#define IMAGE_RS3  64
#define IMAGE_RS4 128

/* Constants for Projection Types */
/* Cylindrical */
#define PROJ_NONE                    0
#define PROJ_CYL_CASSINI             1
#define PROJ_CYL_MILLER              2
#define PROJ_CYL_MERCATOR            4
#define PROJ_CYL_OBL_MERC            8
#define PROJ_CYL_PLATE_CARREE       16
#define PROJ_CYL_TM                 32 /* Transverse Mercator */
#define PROJ_CYL_UTM                64 /* Univ. Trans. Mercator */
#define PROJ_CYL_BCP               128 /* Basic Cyl. Proj. */

/* Azimuthal */
#define PROJ_AZM_LAMBERT           256
#define PROJ_AZM_EQUIDISTANT       512
#define PROJ_AZM_GNOMONIC         1024
#define PROJ_AZM_ORTHOGRAPHIC     2048
#define PROJ_AZM_GEN_STEREO       4096 /* General Stereographic */

/* Conic */
#define PROJ_CON_ALBERS           8192
#define PROJ_CON_LAMBERT         16384

/* Miscellaneous */
#define PROJ_MISC_HAMMER         32768
#define PROJ_MISC_SINUSOIDAL     65536
#define PROJ_MISC_ECKERT_VI     131072
#define PROJ_MISC_ROBINSON      262144
#define PROJ_MISC_WINKEL_TRIPEL 524288
#define PROJ_MISC_MOLLWEIDE    1048576

/* Non-Geographical Not Supported */
#define PROJ_TEST_DK_TEST      2097152
/* End Constants for Projection Types */


/* Possible values for WebGuiStateStruct.ClickEffect */
#define CLICK_NOTHING                0
#define CLICK_ZOOM_IN                1
#define CLICK_ZOOM_OUT               2
#define CLICK_RECENTER               3
#define CLICK_VIEW_EVENTS            4

/* Possible values for WebGuiStateStruct.StationClickEffect */
#define EWDB_STATION_DISPLAY_STATIONS 1
#define EWDB_STATION_DISPLAY_DEFAULT 2
#define EWDB_STATION_DONT_DISPLAY_STATIONS 4

#define MAX_AUTHOR_LENGTH 128
#define MAX_GUI_STATE_STRING_LEN 255


typedef struct _EWDBMapDataStruct
{
  float Lat1,Lon1,Lat2,Lon2;
  float CenterLat, CenterLon;
  float LatHeight, LonWidth;
  int   Rivers;
  int   Politicals;
  char  bLatLonLines;
  char  bBorder;
  char  MapName[30];
  unsigned int ProjectionType;

} EWDBMapDataStruct;

typedef struct _EWDBMapStruct
{
  EWDBMapDataStruct Map;
  int   NumOfChildren;
  struct _EWDBMapStruct * pChildren[MAX_MAP_CHILDREN+1];
  struct _EWDBMapStruct * pParent;
  /*  MapIsRoot if pParent=NULL */
}  EWDBMapStruct;


typedef struct _EventCriteriaStruct
{
  float                  MaxLat,MinLat,MaxLon,MinLon;
  float                  MaxZ,MinZ;
  struct tm              tmMaxDate, tmMinDate;
  float                  MaxMag,MinMag;
  int                    SourceType;
  char                   Source[MAX_AUTHOR_LENGTH];
  int                    EventType;
  int                    MaxEventsDisp;
  int                    MaxEventsRetr;
  int                    LastDaysOnly;
  int                    NumberOfDays;
} EventCriteriaStruct;

typedef struct _LocationProjectionStruct
{
  /* Lat lon is in terms of degrees only, no
     minutes or seconds.  Ex: 37.45 degrees */
  EWDBMapDataStruct      MapData;
  int                    PixelWidth;
  int                    PixelHeight;
  void *                 pPSI;  /* Pointer to Projection Specific Info */
  int                    PSILen;
  char                   MapID[MAX_MAPID_LENGTH];
  int                    bResetLatLon;
} LocationProjectionStruct;


typedef struct _MapServerImageStruct
{
  char                   ImageFileName[MAX_FNAME_LEN];
  int                    ImageFormat;
  int                    ImageFormatsSupported;
} MapServerImageStruct;


typedef struct _WebGUIStateStruct
{
  int                    StationClickEffect;
  int                    ClickEffect;
  int                    idOrigin;   /** ADDED BY LUCKY FOR REVIEW PAGES **/
  int                    ShowUnpicked;   /** ADDED BY LUCKY FOR REVIEW PAGES **/
  int                    ShowMap;   /** ADDED BY LUCKY FOR REVIEW PAGES **/
  EWDBid                 NextEvtID; /** ADDED BY LUCKY FOR ENHANCED GETLIST **/
  EWDBid                 PrevEvtID; /** ADDED BY LUCKY FOR ENHANCED GETLIST **/
  int                    Options;
  int                    XClick,YClick;
  char                   GUIStateString[MAX_GUI_STATE_STRING_LEN];
} WebGUIStateStruct;


typedef struct _SetVarsUserStruct
{
  EventCriteriaStruct       ECSData;
  LocationProjectionStruct  LPSData;
  MapServerImageStruct      MSISData;
  WebGUIStateStruct         WGSSData;
} SetVarsUserStruct;

 

/***** Globally defined map colors, computed in DrawEqsInit *****/
int		MC_white;
int		MC_blue;
int		MC_red;
int		MC_yellow;
int		MC_black;
int		MC_triangle;
int		MC_green;




/* DavidK 1/15/1999 to support winnt servers */
#ifdef _WINNT
# define EXE_EXT ".exe"
#else
# define EXE_EXT
#endif

#define PIC_LEFT_PIX_BORDER  6
#define PIC_TOP_PIX_BORDER   6
#define PIC_RIGHT_PIX_BORDER 6
#define PIC_BTM_PIX_BORDER   6


/* EQSPicture Constants */
#define PIC_LEFT 0
#define PIC_TOP 1
#define PIC_RIGHT 2
#define PIC_BOT 3

typedef struct _EQSPicture
{
  int pix[4];
  float deg[4];
} EQSPicture;






/******************************************************
  Function:    DrawMap ()
  Purpose:     Draw map showing events and stations

  Parameters:    
      Input
      pStations:
               Pointer to the list of stations to draw.
      NumStations:
               Number of stations to draw.
      pEvents:
               Pointer to the list of events to draw.
      NumEvents:
               Number of events to draw.
      pPic:
               Coordinates in both pixels and lat/lon degrees of
               the image to draw on
      pLPSData:
               Pointer to the Location/Projection data for the
               current map.
      DisplayStationsFlag:
               If 1 display stations no matter what, if 0 display
               stations only if we are within the MaxStationDisplayWidth.
      MaxStationDisplayWidth:
               Width in degrees below which we display stations.
      ImageFileName:
               Name of the GMT generated map used as a base for
               drawing events and stations.
      station_color:
               Color to use for drawing stations.
      initialize:
               If 0 DrawEqsInit will NOT be called.
      convert2gif:
               If 1 TimeStampAndConvert will be called, otherwise
               it is assumed that more calls to DrawMap will be made
               and the conversion will happen in the last call.
      debug_flag:
               Print debug info if not 0.

      Input/Output
      pImage:  pointer to a gdlib image.  used as a handle for
               the image on which to draw.

      Output
      AreaBuf: 
               Character buffer in which to store an htmlized list
               of the events.  Both AreaBuf and AreaBuf2 come in
               blank but pre-malloced.
      AreaBuf2:
               Temporary buffer in which to store an htmlized list
               of the events.
      
  Author:
               Lucky Vidmar, 06/02/1999

  Internal Functions Called:
    DrawEqsInit(), PlotStations(), PlotQuakesOnIndex(),
    Timestamp_gdImage_AndConvert2GIF()
               
  Library Functions Called: 
               
  External Library Functions Called:
  **********************************************************/
int DrawMap(gdImagePtr *pImage, EWDB_StationStruct *pStations, 
            int NumStations, EWDB_EventListStruct *pEvents, int NumEvents, 
            EQSPicture *pPic, LocationProjectionStruct *pLPSData,
            int DisplayStationsFlag, float MaxStationDisplayWidth,
            char *ImageFileName, char *AreaBuf, char *AreaBuf2,
            int *station_color, int initialize, int convert2gif, 
            int debug_flag);

#endif /* #ifndef MAP_DISPLAY_STRUCTS_H */

