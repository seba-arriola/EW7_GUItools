
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: getlist_utils.c,v 1.5 2003/01/30 23:12:57 lucky Exp $
 *    Revision history:
 *
 *    $Log: getlist_utils.c,v $
 *    Revision 1.5  2003/01/30 23:12:57  lucky
 *    *** empty log message ***
 *
 *    Revision 1.4  2002/06/19 17:06:57  lucky
 *    Fixed handling of mags other than Md and ML
 *
 *    Revision 1.3  2002/06/19 16:16:59  lucky
 *     Added support for displaying multiple mag types, beyond just Md and ML
 *
 *    Revision 1.2  2002/05/28 17:23:45  lucky
 *    *** empty log message ***
 *
 *    Revision 1.1  2001/09/26 21:39:08  lucky
 *    Initial revision
 *
 *
 *
 */
  
/* filename:       getlist_utils.c 
   module:         getlist

	Utility routines for getlist. There is not much rhyme or reason for
	splitting the routines up this way. But, the main () function of
	getlist is about 2000 lines long... Also, this way might allow us
	to create multiple getlist variants while relying on the same
	utilities.
   
	LV September 2001.

  *************************/

/* include a bunch of header files for external functions */
#include <time.h>
#include <time_ew.h>  /* for profiling timing */
#include <time_functions.h>
#include <map_display_structs.h>
#include <getlist.h>
#include <mapserver.h>
#include <webparse.h>
#include <html_common.h>


#include <ewdb_ora_api.h>
#include <ewdb_apps_utils.h>
#include <alarms.h>


/************* EXTERNALLY DEFINED FUNCTIONS *************/

/* Defined in earthworm.h */
void CreateSpecificMutex( mutex_t * ); 
int  StartThread( thr_ret (void *), unsigned, unsigned * );
void RequestSpecificMutex( mutex_t * );
void ReleaseSpecificMutex( mutex_t * );

/* The following #ifdef is only for a function prototype, don't get excited! */
#ifdef _WINNT  /* so that compiler doesn't warn of underined getcwd */
 /* defined in direct.h as part of CRTLIB */
 char * getcwd( char *buffer, int maxlen );
#endif

/********** END OF EXTERNALLY DEFINED FUNCTIONS *************/

extern 	SetVarsUserStruct DefaultParams;
extern 	EWDBMapStruct MapRoot;
extern	char szDefaultMapID[];
extern	char szCurrentMapID[];
extern	EWDBMapDataStruct CurrentMap;
extern	EWDBMapStruct * pDefaultMap;
extern	int PrintWarningMessages;




int CreateTumbleFile (EWDB_EventListStruct *pEvents, 
                      int NumEvents, char *filename);

int ComparEvents_qsort(const void * pEvent1, const void * pEvent2);


int Webparse_Client_SetVars(char * szVar, char * szVal, void * pOptions)
/******************************************************
  Function:    SetVars()
  Purpose:     SetVars() is a callout function provided by 
               the web_common (webparse) routines that parse
               web parameters passed during a CGI-BIN GET or
               POST request.  SetVars() is called for each
               parameter - value pair
  Parameters:    
      Input
      szVar:   a string containing the name of the variable
      szVal:   a string containing the value of the variable

      Output
      pOptions:
               a (void *) that is passed to the web_common 
               routines during the initial call to 
               Webparse_GetAndProcessWebParams().  It is a user definable
               pointer, that the web_common routines are nice
               enough to pass through to this SetVars() function.
               You can tell I wrote the web_common routines, 
               because I only say nice things about them.
  Author: DK, before 04/15/1999

  **********************************************************/
{

  /* Warning!!!!!!:  SetVars may destroy szVal while evaluating it!!! */

  char                *pX,*pY,*pSeparator;
  SetVarsUserStruct * pParams=pOptions;

  if(strcmp(szVar,"Start_Date") == 0)
  {
    EWDB_atotm_date(&(pParams->ECSData.tmMinDate),szVal);
  }
  else if(strcmp(szVar,"Start_Time") == 0)
  {
    EWDB_atotm_time(&(pParams->ECSData.tmMinDate),szVal);
  }
  else if(strcmp(szVar,"Finish_Date") == 0)
  {
    EWDB_atotm_date(&(pParams->ECSData.tmMaxDate),szVal);
  }
  else if(strcmp(szVar,"Finish_Time") == 0)
  {
    EWDB_atotm_time(&(pParams->ECSData.tmMaxDate),szVal);
  }
  else if(strcmp(szVar,"ReviewedByType") == 0)
  {
    pParams->ECSData.SourceType=atoi(szVal);
  }
  else if(strcmp(szVar,"ReviewedBy") == 0)
  {
    strcpy(pParams->ECSData.Source,szVal);
  }
  else if(strcmp(szVar,"MinLon") == 0)
  {
    pParams->ECSData.MinLon=(float)atof(szVal);
  }
  else if(strcmp(szVar,"MaxLon") == 0)
  {
    pParams->ECSData.MaxLon=(float)atof(szVal);
  }
  else if(strcmp(szVar,"MinLat") == 0)
  {
    pParams->ECSData.MinLat=(float)atof(szVal);
  }
  else if(strcmp(szVar,"MaxLat") == 0)
  {
    pParams->ECSData.MaxLat=(float)atof(szVal);
  }
  else if(strcmp(szVar,"MinZ") == 0)
  {
    pParams->ECSData.MinZ=(float)atof(szVal);
  }
  else if(strcmp(szVar,"MaxZ") == 0)
  {
    pParams->ECSData.MaxZ=(float)atof(szVal);
  }
  else if(strcmp(szVar,"MinMag") == 0)
  {
    pParams->ECSData.MinMag=(float)atof(szVal);
  }
  else if(strcmp(szVar,"MaxMag") == 0)
  {
    pParams->ECSData.MaxMag=(float)atof(szVal);
  }
  else if(strcmp(szVar,"EventType") == 0)
  {
    pParams->ECSData.EventType=atoi(szVal);
  }
  else if(strcmp(szVar,"MaxEventsDisplayed") == 0)
  {
    pParams->ECSData.MaxEventsDisp=atoi(szVal);
  }
  else if(strcmp(szVar,"MaxEventsRetrieved") == 0)
  {
    pParams->ECSData.MaxEventsRetr=atoi(szVal);
  }
  else if(strcmp(szVar,"FirstPage") == 0)
  {
    PrintWarningMessages=atoi(szVal);
  }
  else if(strcmp(szVar,"LastDaysOnly") == 0)
  {
    pParams->ECSData.LastDaysOnly=1;
  }
  else if(strcmp(szVar,"ClickEffect") == 0)
  {
      pParams->WGSSData.ClickEffect=atoi(szVal);
  }
  else if(strcmp(szVar,"StationClickEffect") == 0)
  {
      pParams->WGSSData.StationClickEffect=atoi(szVal);
  }
  else if(strcmp(szVar,"ClickCoords") == 0)
  {
    /* Click coordinates come in the form "XPix,YPix" and
       must be parsed into X and Y.  */
    if((pSeparator=strchr(szVal,',')) == NULL)
    {
      html_logit("","SetVars(): Unparseable ClickCoords: %s.\n",szVal);
      return(-1);
    }
    else
    {
      *pSeparator=0;
      pX=szVal;
      pY=pSeparator+1; /* start with the next char after (comma/NullTerm) */
      pParams->WGSSData.XClick=atoi(pX);
      pParams->WGSSData.YClick=atoi(pY);
    }
  }  /* end if ClickCoords */
  else if(strcmp(szVar,"Lat") == 0)
  {
    pParams->LPSData.MapData.CenterLat=(float)atof(szVal);
  }
  else if(strcmp(szVar,"Lon") == 0)
  {
    pParams->LPSData.MapData.CenterLon=(float)atof(szVal);
  }
  else if(strcmp(szVar,"LonWidth") == 0)
  {
    pParams->LPSData.MapData.LonWidth=(float)atof(szVal);
  }
  else if(strcmp(szVar,"LatHeight") == 0)
  {
    pParams->LPSData.MapData.LatHeight=(float)atof(szVal);
  }
  else if(strcmp(szVar,"MapNameClicked") == 0)
  {
    /* This happens when the user clicks one of the "map name" buttons or
       hyperlinks to the side of the current map. */
    
    /* We need to make sure that the Lat/Lon get set to that of the new
       map instead of the current custom settings. */
    pParams->LPSData.bResetLatLon=atoi(szVal);
  }
  else if(strcmp(szVar,"MapName") == 0)
  {
    strcpy(pParams->LPSData.MapData.MapName,szVal);
    if(GetMapIDFromMapName(pParams->LPSData.MapData.MapName, 
                           pParams->LPSData.MapID,&MapRoot,"")
       )
    {
      html_logit("","Error: GetMapIDFromMapName() failed for %s. Continuing!\n",
        pParams->LPSData.MapData.MapName);
    }
  }
  else if(strcmp(szVar,"NextEvtID") == 0)
  {
    pParams->WGSSData.NextEvtID = (EWDBid) atoi(szVal);
  }
  else if(strcmp(szVar,"PrevEvtID") == 0)
  {
    pParams->WGSSData.PrevEvtID = (EWDBid) atoi(szVal);
  }
  else
  {
    /* The variable didn't match any of the onew we expected.  Logit() */
    html_logit("","Unrecognized Variable %s found, with Value %s.\n",szVar,szVal);
  }

  return(0);
}


int ConvertToOraAPIFormat(SetVarsUserStruct * pParams, 
                       GetListAPIReqStruct * pReq, int * pBufferSize,
                       EWDB_EventListStruct *pEVMin, EWDB_EventListStruct *pEVMax)
/******************************************************
  Function:    ConvertToOraAPIFormat()
  Purpose:     This function is a format converter.  We already
               have all the information that we need to query
               the database for a list of events.  The problem
               is that our parameters are not in the proper format,
               so ConvertToORA_APIFormat() converts our parameters
               (stored in pParams) to a format that can be used to
               call the GetEventList() EWDB API call.
  Parameters:    
      Input
      pParams: This is the mother of all structs used in this
               program.  It contains all known state and criteria
               data.

      Output
      pReq:    This contains the starttime and stoptime to be used
               in the eventlist query.
      pBufferSize:
               A pointer to the maximum number of events to be
               returned by the eventlist query.
      pEVMin:  A pointer to a EWDB API EWDB_EventListStruct that 
               contains the "minimum value" criteria for the
               events to retrieve.
      pEVMax:  A pointer to API EWDB_EventListStruct that 
               contains the "maximum value" criteria for the
               events to retrieve.

  Author: DK, before 04/15/1999

  **********************************************************/
{
  EventCriteriaStruct * pCriteria = &(pParams->ECSData);


  /* Copy starttime from pParams */
  if(pCriteria->tmMinDate.tm_year)
  {
    tzset();
    pReq->StartTime=(mktime(&(pCriteria->tmMinDate)));
    if(pReq->StartTime < 0)
    {
      logit("","ConvORA_API:Bad starttime %d, setting to 0 and continuing.\n",
             pReq->StartTime);
      pReq->StartTime = 0;
    }
  }
  else
  {
    pReq->StartTime = 0;
  }

  /* Copy EndTime from pParams */
  if(pCriteria->tmMaxDate.tm_year)
  {
    tzset();
    pReq->EndTime=(mktime(&(pCriteria->tmMaxDate)));
    if(pReq->EndTime < 0)
    {
      logit("","ConvORA_API:Bad EndTime %d, setting to 0 and continuing.\n",
             pReq->EndTime);
      pReq->EndTime = 0;
    }
  }
  else
  {
    pReq->EndTime = 0;
  }

  /* Evaluate ReviewedBy */
  switch (pCriteria->SourceType)
  {
  case RB_ALL_EVENTS : 
    {
      strcpy(pEVMin->Event.szSource,"*");
      break;
    }
/*  case RB_ALL_REVIEWED_EVENTS : 
    {
      strcpy(pEVMin->Source,"**");
      break;
    }
  case RB_UNREVIEWED_EVENTS : 
    {
      strcpy(pEVMin->Source,"");
      break;
    }
*/
  case RB_REVIEWEDBY_EVENTS :  
    {
      strcpy(pEVMin->Event.szSource,pCriteria->Source);
      break;
    }
  default :	logit("","Unhandled SourceCriteria "
              "in ConvertToAPIFormat.\n");
  }  /* End switch RevByCriteria */

  /* If Min/Max Lat/Lon are default, crop them
     based on the min/max map coordinates. */
  
  if(pCriteria->MinLat != DEFAULT_MINLAT)
    pEVMin->dLat=pCriteria->MinLat;
  else
    pEVMin->dLat=pParams->LPSData.MapData.CenterLat
                - (pParams->LPSData.MapData.LatHeight/2);

  if(pCriteria->MaxLat != DEFAULT_MAXLAT)
    pEVMax->dLat=pCriteria->MaxLat;
  else
    pEVMax->dLat=pParams->LPSData.MapData.CenterLat
                + (pParams->LPSData.MapData.LatHeight/2);

  if(pCriteria->MinLon != DEFAULT_MINLON)
    pEVMin->dLon=pCriteria->MinLon;
  else
    pEVMin->dLon=pParams->LPSData.MapData.CenterLon
                - (pParams->LPSData.MapData.LonWidth/2);

  if(pCriteria->MaxLon != DEFAULT_MAXLON)
    pEVMax->dLon=pCriteria->MaxLon;
  else
    pEVMax->dLon=pParams->LPSData.MapData.CenterLon
                + (pParams->LPSData.MapData.LonWidth/2);

  /* Don't set criteria if the defaults have been used. */
  if(pCriteria->MinZ != DEFAULT_MINZ)
    pEVMin->dDepth=pCriteria->MinZ;
  if(pCriteria->MaxZ != DEFAULT_MAXZ)
    pEVMax->dDepth=pCriteria->MaxZ;
  if(pCriteria->MinMag != DEFAULT_MINMAG)
    pEVMin->dPrefMag=pCriteria->MinMag;
  if(pCriteria->MaxMag != DEFAULT_MAXMAG)
    pEVMax->dPrefMag=pCriteria->MaxMag;

  pEVMin->Event.iEventType=pCriteria->EventType;
  *pBufferSize=pCriteria->MaxEventsDisp;

  if(DEBUG) /* DK 1/9/99*/
	  logit("","Finishing ConvertToOraAPI format.");

  return(0);
}  /* End of ConvertToOraAPIFormat() */




int PostProcessWebParams(SetVarsUserStruct * pWebParams)
/******************************************************
  Function:    PostProcessWebParams()
  Purpose:     Do any neccessary massaging of WebParams
               after all of the param values have been 
               read in and handled via SetVars()
  Parameters:    
      Input
      pWebParams: 
               the mother of all structs, contains everything
               we know and most of what we care about.
      Output

  Author: DK, before 04/15/1999

  **********************************************************/
{

  time_t          tTemp;


  /* if both of the time values are default */
  if((!memcmp(&(pWebParams->ECSData.tmMinDate), &(DefaultParams.ECSData.tmMinDate),
              sizeof(pWebParams->ECSData.tmMinDate)))
     &&
     (!memcmp(&(pWebParams->ECSData.tmMaxDate), &(DefaultParams.ECSData.tmMaxDate),
              sizeof(pWebParams->ECSData.tmMaxDate)))
    )
  {
    if(pWebParams->ECSData.LastDaysOnly)
    {
      /* Set the maxtime to 0, so that it will not be a constraint */
      /* memset(&(pWebParams->ECSData.tmMaxDate),0,sizeof(struct tm)); */

      time(&tTemp);
      tTemp-= 60 * 60 * 24 * pWebParams->ECSData.NumberOfDays;  /*  subtract X days from now */
      gmtime_ew(&tTemp,&(pWebParams->ECSData.tmMinDate));
    }
  }  /* if both time constraint values are default */
  else
  {
    pWebParams->ECSData.LastDaysOnly=FALSE;
  }  /* else(both time constraint values are default) */


  /* We need to adjust the Lat,Lon,LonWidth, based on click coordinates,
    and ClickEffect value. 
  */
  if(pWebParams->WGSSData.XClick || pWebParams->WGSSData.YClick)
  {
    /* First we need to calculate the Lat/Lon of the click */

    /* Convert the pixel (X,Y) coordinates to (Lat,Lon) coords */
    Pix2LatLon(&(pWebParams->LPSData),
               pWebParams->WGSSData.XClick, pWebParams->WGSSData.YClick,
               &(pWebParams->LPSData.MapData.CenterLat),
               &(pWebParams->LPSData.MapData.CenterLon));

    /* Now we must adjust LonWidth and LatHeight 
       based upon ClickEffect */
    if(pWebParams->WGSSData.ClickEffect == CLICK_ZOOM_IN)
    {
      pWebParams->LPSData.MapData.LonWidth/=2;
      pWebParams->LPSData.MapData.LatHeight/=2;
    }
    else if(pWebParams->WGSSData.ClickEffect == CLICK_ZOOM_OUT)
    {
      pWebParams->LPSData.MapData.LonWidth*=2;
      pWebParams->LPSData.MapData.LatHeight*=2;
    }
    else if(pWebParams->WGSSData.ClickEffect == CLICK_RECENTER)
    {
      /*LonWidth and LatHeight stay the same */
    }
    else
    {
      html_logit("","Unrecognized ClickEffect valued %u.\n",
            pWebParams->WGSSData.ClickEffect);
      return(-1);
    }

    /* Cleanup XClick and YClick for the heck of it. */
    pWebParams->WGSSData.XClick = pWebParams->WGSSData.YClick = 0;

  }  /* End if XClick or YClick  */

  return(0);
}  /* end PostProcessWebParams() */



int CreateGUIStateStringFromWebParams(SetVarsUserStruct * pParams,
                                      SetVarsUserStruct * pDefaults,
                                      char * pStateString)
/******************************************************
  Function:    CreateGUIStateStringFromWebParams()
  Purpose:     We have a bunch of data that the user has
               entered in, that is different from the default
               values, and we would like to carry that data
               over from one instance of getlist to another.
               In order to do that we need to send state info
               to the webbrowser, and have it come back to us.
               We implement this by creating a large state
               string that is applied to all URL's on the new
               webpage we are writing out.  In order to minimize
               the GUIStateString, we only record params that
               are different than the defaults, we do this by
               comparing the pParams struct to the pDefaults
               struct.
  Parameters:    
      Input
      pParams: 
               a pointer to the mother of all structs, that 
               contains everything we know and care about.
      pDefaults: 
               a pointer to the mother of all structs, that 
               contains defaults for everything we know and 
               care about.
      Output
      pStateString:
               the resulting GUI state string which contains
               all neccessary state logic in CGI-BIN GET 
               compatable format.
  Author: DK, before 04/15/1999

  **********************************************************/
{
  char szTemp[40],szTemp2[40];

  /* Min/Max for Lat,Lon,Z,Mag parameters */
  if(pParams->ECSData.MaxLat != pDefaults->ECSData.MaxLat)
  {
    sprintf(&(pStateString[strlen(pStateString)]),"MaxLat=%3.2f&",pParams->ECSData.MaxLat);
  }
  if(pParams->ECSData.MinLat != pDefaults->ECSData.MinLat)
  {
    sprintf(&(pStateString[strlen(pStateString)]),"MinLat=%3.2f&",pParams->ECSData.MinLat);
  }
  if(pParams->ECSData.MaxLon != pDefaults->ECSData.MaxLon)
  {
    sprintf(&(pStateString[strlen(pStateString)]),"MaxLon=%3.2f&",pParams->ECSData.MaxLon);
  }
  if(pParams->ECSData.MinLon != pDefaults->ECSData.MinLon)
  {
    sprintf(&(pStateString[strlen(pStateString)]),"MinLon=%3.2f&",pParams->ECSData.MinLon);
  }
  if(pParams->ECSData.MaxZ != pDefaults->ECSData.MaxZ)
  {
    sprintf(&(pStateString[strlen(pStateString)]),"MaxZ=%3.2f&",pParams->ECSData.MaxZ);
  }
  if(pParams->ECSData.MinZ != pDefaults->ECSData.MinZ)
  {
    sprintf(&(pStateString[strlen(pStateString)]),"MinZ=%3.2f&",pParams->ECSData.MinZ);
  }
  if(pParams->ECSData.MaxMag != pDefaults->ECSData.MaxMag)
  {
    sprintf(&(pStateString[strlen(pStateString)]),"MaxMag=%3.2f&",pParams->ECSData.MaxMag);
  }
  if(pParams->ECSData.MinMag != pDefaults->ECSData.MinMag)
  {
    sprintf(&(pStateString[strlen(pStateString)]),"MinMag=%3.2f&",pParams->ECSData.MinMag);
  }

  if(strcmp(pParams->ECSData.Source, pDefaults->ECSData.Source) 
      && pParams->ECSData.SourceType == RB_REVIEWEDBY_EVENTS
    )  /* Don't drag reviewed by along unless it has some meaning */
  {
    sprintf(&(pStateString[strlen(pStateString)]),"ReviewedBy=%s&",pParams->ECSData.Source);
  }

  if(pParams->ECSData.SourceType != pDefaults->ECSData.SourceType)
  {
    sprintf(&(pStateString[strlen(pStateString)]),"ReviewedByType=%d&",pParams->ECSData.SourceType);
  }
  if(pParams->ECSData.EventType != pDefaults->ECSData.EventType)
  {
    sprintf(&(pStateString[strlen(pStateString)]),"EventType=%d&",pParams->ECSData.EventType);
  }
  if(pParams->ECSData.MaxEventsDisp != pDefaults->ECSData.MaxEventsDisp)
  {
    sprintf(&(pStateString[strlen(pStateString)]),"MaxEventsDisplayed=%d&",
						pParams->ECSData.MaxEventsDisp);
  }
  if(pParams->ECSData.MaxEventsRetr != pDefaults->ECSData.MaxEventsRetr)
  {
    sprintf(&(pStateString[strlen(pStateString)]),"MaxEventsRetrieved=%d&",
						pParams->ECSData.MaxEventsRetr);
  }
  if(pParams->ECSData.LastDaysOnly != pDefaults->ECSData.LastDaysOnly)
  {
    sprintf(&(pStateString[strlen(pStateString)]),"LastDaysOnly=%d&",pParams->ECSData.LastDaysOnly);
  }

  if(memcmp(&(pParams->ECSData.tmMinDate), &(pDefaults->ECSData.tmMinDate),
            sizeof(pParams->ECSData.tmMinDate)))
  {
    sprintf(&(pStateString[strlen(pStateString)]),"Start_Date=%s&Start_Time=%s&",
           EWDB_tmtoa_date(&(pParams->ECSData.tmMinDate),szTemp),
           EWDB_tmtoa_time(&(pParams->ECSData.tmMinDate),szTemp2));
  }
  if(memcmp(&(pParams->ECSData.tmMaxDate), &(pDefaults->ECSData.tmMaxDate),
            sizeof(pParams->ECSData.tmMaxDate)))
  {

    sprintf(&(pStateString[strlen(pStateString)]),"Finish_Date=%s&Finish_Time=%s&",
           EWDB_tmtoa_date(&(pParams->ECSData.tmMaxDate),szTemp),
           EWDB_tmtoa_time(&(pParams->ECSData.tmMaxDate),szTemp2));

  }

  sprintf(&(pStateString[strlen(pStateString)]),"Lat=%3.2f&",pParams->LPSData.MapData.CenterLat);
  sprintf(&(pStateString[strlen(pStateString)]),"Lon=%3.2f&",pParams->LPSData.MapData.CenterLon );
  sprintf(&(pStateString[strlen(pStateString)]),"LonWidth=%3.2f&",pParams->LPSData.MapData.LonWidth);
  sprintf(&(pStateString[strlen(pStateString)]),"LatHeight=%3.2f&",pParams->LPSData.MapData.LatHeight );

  if(pParams->LPSData.PixelWidth  != pDefaults->LPSData.PixelWidth )
  {
    sprintf(&(pStateString[strlen(pStateString)]),"PixelWidth=%d&",pParams->LPSData.PixelWidth );
  }
  if(pParams->LPSData.PixelHeight  != pDefaults->LPSData.PixelHeight )
  {
    sprintf(&(pStateString[strlen(pStateString)]),"PixelHeight=%d&",pParams->LPSData.PixelHeight );
  }

/*  Don't Include ClickEffect, because we would get duplicates if someone clicked on the
    checkbox.  */

  if(pParams->WGSSData.Options  != pDefaults->WGSSData.Options )
  {
    sprintf(&(pStateString[strlen(pStateString)]),"Options=%d&",pParams->WGSSData.Options );
  }

  return(0);
}  /* End CreateGUIStateStringFromWebParams() */



int Pix2LatLon(LocationProjectionStruct * pLPSData,
               int XPix, int YPix,
               float* pLat, float* pLon)
/******************************************************
  Function:    Pix2LatLon()
  Purpose:     Convert pixel coordinates for a given map to
               Lat/Lon coordinates, based on map parameters
               including size, projection type, and focal point
  Parameters:    
      Input
      pLPSData: 
               a pointer to the location and projection data
               for the map we are interpreting.
      XPiX:    the X pixel coordinate on the map
      YPiX:    the Y pixel coordinate on the map

      Output
      pLat:    a pointer to the resulting Lat coordinate of the
               input X,Y pixel coordinates.
      pLon:    a pointer to the resulting Lon coordinate of the
               input X,Y pixel coordinates.
  Author: DK, before 04/15/1999

  *Note:  This function is currently VERY STUPID!!!!!!
          It has at most the intelligence of a consultant
  **********************************************************/
{
  float PixsPerLonDeg,PixsPerLatDeg;
  int XPixCenter,YPixCenter;
  /* LonWidth does not include borders.  Map is expected to be
     (LatHeight, LonWidth), plus borders. */
  /* XPix and YPiX for map don't include borders */
  /* We are going to HARD_CODE the borders for now, since
     this is our first crack at this, and we don't know quite
     what we are doing.  1/29/99 davidk */
  /* I love the royal WE */

  PixsPerLonDeg=pLPSData->PixelWidth/pLPSData->MapData.LonWidth;
  PixsPerLatDeg=pLPSData->PixelHeight/pLPSData->MapData.LatHeight;
  XPixCenter=PIC_LEFT_PIX_BORDER + pLPSData->PixelWidth/2;
  YPixCenter=PIC_TOP_PIX_BORDER + pLPSData->PixelHeight/2;
  *pLat=pLPSData->MapData.CenterLat+((YPixCenter-YPix) / PixsPerLatDeg);
  *pLon=pLPSData->MapData.CenterLon-((XPixCenter-XPix) / PixsPerLonDeg);

  return(0);
}  /* End of Pix2LatLon() */



int TraverseMapTree(EWDBMapStruct * pMapCurr, char * sznum)        
/******************************************************
  Function:    TraverseMapTree()
  Purpose:     Traverse the given map tree, printing the
               info of each map found.  This could be 
               genericized to do something when a map is
               found instead of just printing.  Oh well, it
               works for now.  Currently a debugging function.
  Parameters:    
      Input
      pMapCurr:  
               Pointer to the current location/map in the tree.
      sznum:   The ID of the curren map.
      
  Author: DK, before 04/15/1999

  **********************************************************/
{
  char sznewnum[20];
  int dki;

  printf("Map:  %s\t%s\t%.2f\t%.2f\t%.2f\t%.2f\n<br>\n", 
         sznum,pMapCurr->Map.MapName,
         pMapCurr->Map.CenterLat,pMapCurr->Map.CenterLon,
         pMapCurr->Map.LatHeight,pMapCurr->Map.LonWidth);
  for(dki=1;dki < MAX_MAP_CHILDREN; dki++)
  {
    if(pMapCurr->pChildren[dki] == NULL)
    {
      /*printf("%s%d is NULL.\n<br>\n",sznum,dki);*/
    }
    else
    {
      sprintf(sznewnum,"%s%d.",sznum,dki);
      TraverseMapTree(pMapCurr->pChildren[dki],sznewnum);
    }
  }
  return(0);
}

EWDBMapStruct * GetMapFromID(char * szDefaultMapID)
/******************************************************
  Function:    GetMapFromID()
  Purpose:     Retrieve a pointer to the map with the 
               requested ID.
  Parameters:    
      Input
      szDefaultMapID:
               The MapID in string form of the desired map.
      GLOBAL VARIABLE MapRoot:
               MapRoot is a global representing the root of 
               the map tree. 
      
  Author: DK, before 04/15/1999

  **********************************************************/
{
  EWDBMapStruct * pMapCurr;
  int MapNum,MapNumNext;
  char * pCurr, * pPeriod;
  

  /* The number is in szDefaultMapID (Example 1.2.1.3) */

  /* initialize pCurr to the beginning of the map id */
  pCurr=szDefaultMapID;

  /* initialize pMapCurr to the root map */
  pMapCurr=&MapRoot;

  /* get first number in map id */
  MapNum=atoi(pCurr);

  /* if there was no legit MapNum, throw a temper tantrum */
  if(!(MapNum > 0))
  {
    html_logit("","Bad mapID %s, passed to GetMapFromID().\n", szDefaultMapID);
    return(NULL);
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

  /* while we haven't retrieved all of the numbers from MapID */
  while(MapNum != -1)
  {
    if(pMapCurr->pChildren[MapNum] == NULL)
    {
      html_logit("","GetMapFromID(): MapID not found: %s.\n",  szDefaultMapID);
      return(NULL);
    }
    else
    {
      if(MapNumNext == -1)
      {
        /*We've found the map */
        return(pMapCurr->pChildren[MapNum]);
      }
      else
      {
        pMapCurr=pMapCurr->pChildren[MapNum];
      }
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
  printf("GetMapFromID():Somehow we got out the bottom of the "
         "loop for MapID %s\n<br>\n",
          szDefaultMapID);
  return(0);
}


int GetMapIDFromMapName(char * szMapName, char * szMapID, 
                        EWDBMapStruct * pMapCurr, char * szCurrentID)
/******************************************************
  Function:    GetMapIDFromMapName()
  Purpose:     Retrieve the ID of a map with the requested name.

  Parameters:    
      Input
      szMapName:
               Name of the map to search for.
      pMapCurr:
               Pointer to the current Map/spot in the maptree.
      szCurrentID:
               ID of the current spot in the maptree.
      Output
      szMapID: ID of the map with name equal to szMapName
      
  Author: DK, before 04/15/1999

  **********************************************************/
{
  char szNewID[20];
  int dki;

  /* Yeah this isn't very efficient but it is quick to write and
     the trees should be small atleast for now. dk 040999 */
  for(dki=1;dki < MAX_MAP_CHILDREN; dki++)
  {
    if(pMapCurr->pChildren[dki] == NULL)
    {
      /*printf("%s%d is NULL.\n<br>\n",sznum,dki);*/
    }
    else
    {
      if(strcmp(pMapCurr->pChildren[dki]->Map.MapName,szMapName))
      {
        /* We haven't found it yet, keep looking */
        sprintf(szNewID,"%s%d.",szCurrentID,dki);
        if(!GetMapIDFromMapName(szMapName,szMapID,
                               pMapCurr->pChildren[dki],szNewID))
        {
          /* The map was found somewhere down the tree.  Return */
          return(0);
        }
      }
      else
      {
        /*We found it, file the paperwork and getst the geld */
        sprintf(szMapID,"%s%d",szCurrentID,dki);
        return(0);
      }
    }
  }
  return(1);
}


/*************************************************************
 *  
 * CreateTumbleFile:  given the list of earthquakes, creates
 *   an html file that invokes a 3D-Tumbler applet.
 * 
 *************************************************************/
int		CreateTumbleFile (EWDB_EventListStruct *pEvents, 
								int NumEvents, char *filename)
{

	FILE	*fp;
	int		i;

	if ((pEvents == NULL) || (NumEvents < 0) || (filename == NULL))
	{
		html_logit ("", "Invalid arguments passed in!\n");
		return -1;
	}


	/* Open the tumble file */
	if ((fp = fopen (filename, "wt")) == NULL)
	{
		html_logit ("", "Could not open file %s for writing!\n", filename);
		return -1;
	}


	fprintf (fp, "<html>\n");
	fprintf (fp, "<head>\n");
	fprintf (fp, "<title>Earthquake Tumbler</title>\n");
	fprintf (fp, "<body>\n");
	fprintf (fp, "<h1>Earthquake Tumbler</h1>\n");
	fprintf (fp, "<hr>\n");
	fprintf (fp, "<p>\n");

	fprintf (fp, "<applet code=\"XYZApp.class\" ARCHIVE=\"eqtumbler.jar\" width=\"400\" height=\"400\">\n");

	/* Print out magnitudes */
	fprintf (fp, "<param name=\"magnitude\" value=\"");

	fprintf (fp, "%0.1f", pEvents[0].dPrefMag);
	for (i = 1; i < NumEvents; i++)
		fprintf (fp, ",%0.1f", pEvents[i].dPrefMag);
	fprintf (fp, "\">\n");


	/* Print out longitudes */
	fprintf (fp, "<param name=\"xval\" value=\"");

	fprintf (fp, "%0.3f", pEvents[0].dLon);
	for (i = 1; i < NumEvents; i++)
		fprintf (fp, ",%0.3f", pEvents[i].dLon);
	fprintf (fp, "\">\n");

	/* Print out lattitudes */
	fprintf (fp, "<param name=\"yval\" value=\"");

	fprintf (fp, "%0.3f", pEvents[0].dLat);
	for (i = 1; i < NumEvents; i++)
		fprintf (fp, ",%0.3f", pEvents[i].dLat);
	fprintf (fp, "\">\n");

	/* Print out depths */
	fprintf (fp, "<param name=\"depth\" value=\"");

	fprintf (fp, "%0.3f", pEvents[0].dDepth);
	for (i = 1; i < NumEvents; i++)
		fprintf (fp, ",%0.3f", pEvents[i].dDepth);
	fprintf (fp, "\">\n");

	fprintf (fp, "</applet>\n"); 

	/* Gridbag section */
	fprintf (fp, "<applet code=\"gridbag.class\" ");
	fprintf (fp, "archive=\"eqtumbler.jar\" ");
	fprintf (fp, "width=\"400\" height=\"200\" name=\"gridbag\">\n");
	fprintf (fp, "</applet>\n"); 
	
	/* post mortem */
	fprintf (fp, "</p> \n </body> \n </html>\n");

	fclose (fp);
	return 0;
}

int ComparEvents_qsort(const void * pEvent1, const void * pEvent2)
{

  /* Remember that we want the list in descending time
     order, so higher times go to the front of the 
     list. (Greater than means -1)
  ****************************************************/
        
  if(  ((EWDB_EventListStruct *)pEvent1)->dOT 
     > ((EWDB_EventListStruct *)pEvent2)->dOT
    )
    return(-1);
  else if(  ((EWDB_EventListStruct *)pEvent1)->dOT 
          == ((EWDB_EventListStruct *)pEvent2)->dOT
         )
    return(0);
  else
    return(1);
}




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
  pParams->WGSSData.ClickEffect   = CLICK_ZOOM_IN;
  pParams->WGSSData.StationClickEffect   = EWDB_STATION_DONT_DISPLAY_STATIONS;
  pParams->WGSSData.Options       = 0;
  pParams->WGSSData.XClick        = 0;
  pParams->WGSSData.YClick        = 0;
  pParams->WGSSData.GUIStateString[0] = 0;
  pParams->WGSSData.NextEvtID     = -1; /* default: start at top */
  pParams->WGSSData.PrevEvtID     = -1; /* default: start at top */
  return(0);
}  /* End SetDefaultParams() */



int		Getlist_FillAuxInfo (AuxEventStruct *pAux, 
		EWDB_EventListStruct *pEvent, int MinStasToShow)
{

	EWEventInfoStruct		EventInfo;
	EWDB_AlarmAuditStruct	Audit;
	int						i, j, NumFound, NumRetr, DBFlags, NumStas;


	if ((pAux == NULL) || (pEvent == NULL)) 
	{
		logit ("", "Invalid arguments passed in.\n");
		return EW_FAILURE;
	}

	pAux->NumStas = 0;
	pAux->NumPhases = 0;
	pAux->NumAmpPicks = 0;
    pAux->NumSnippets = 0;
	pAux->NumAlarms = 0;
	pAux->iPrefMag = -1;
	pAux->iNumMags = 0;
	pAux->idOrigin = -1;


	/* 
	   First, get the number of stations. If it is above the
	   limit, continue retrieving everything else.
	   OTherwise, all this work is wasted.
	*/

	if (ewdb_api_GetNumStationsForEvent(pEvent->Event.idEvent,
                            &NumStas) != EWDB_RETURN_SUCCESS)
	{
		logit ("", "Call to ewdb_api_GetNumStationsForEvent failed.\n");
		pAux->NumStas = 0;
	}
	else
		pAux->NumStas = NumStas;	

	if (pAux->NumStas < MinStasToShow)
	{
		/* Skip the event */
		return EW_SUCCESS;
	}

	DBFlags = GETEWEVENTINFO_SUMMARYINFO | GETEWEVENTINFO_PICKS
					| GETEWEVENTINFO_STAMAGS | GETEWEVENTINFO_COMPINFO |
					GETEWEVENTINFO_WAVEFORM_DESCS;

	/* Get everything at once */
	if (ewdb_apps_GetDBEventInfoII (&EventInfo, 
			pEvent->Event.idEvent, -1, DBFlags) != EWDB_RETURN_SUCCESS)
	{
		logit ("", "Call to ewdb_apps_GetDBEventInfo failed\n");
		return EW_FAILURE;
	}

	/* Fill in auxiliary info -- origin and mag values */

	pAux->idOrigin = EventInfo.PrefOrigin.idOrigin;

	pAux->iNumMags = EventInfo.iNumMags;
	pAux->iPrefMag = EventInfo.iPrefMag;

	memcpy (pAux->Mags, EventInfo.Mags, (MAX_MAGS_PER_ORIGIN * sizeof (EWDB_MagStruct)));


	/* Fill in auxiliary info -- count things */

    pAux->NumAmpPicks = 0;

	for (i = 0; i < EventInfo.iNumChans; i++)
	{
		if (EventInfo.pChanInfo[i].iNumWaveforms > 0)
			pAux->NumSnippets =
					pAux->NumSnippets + EventInfo.pChanInfo[i].iNumWaveforms;

		if (EventInfo.pChanInfo[i].iNumArrivals > 0)
			pAux->NumPhases =
					pAux->NumPhases + EventInfo.pChanInfo[i].iNumArrivals;


        for (j = 0; j < EventInfo.pChanInfo[i].iNumStaMags; j++)
        {
            if (EventInfo.pChanInfo[i].Stamags[j].iMagType != MAGTYPE_DURATION)
            {
                pAux->NumAmpPicks =
                        pAux->NumAmpPicks + EventInfo.pChanInfo[i].iNumStaMags;
            }
        }

	}

	/* Fill in auxiliary info -- alarms */
	if (EWDB_GetAudits(pEvent->Event.idEvent, &Audit, 0, 
			NULL, &NumFound, &NumRetr, 1) == EWDB_RETURN_FAILURE)
	{
		html_logit ("", "Call to EWDB_GetAudits failed;.\n");
	}
	else
	{
		if (NumRetr > 0)
			pAux->NumAlarms = NumFound;
		else
			pAux->NumAlarms = 0;
	}


	if (EventInfo.iNumAllocChans > 0)
		free (EventInfo.pChanInfo);

	EventInfo.iNumAllocChans = 0;

}
