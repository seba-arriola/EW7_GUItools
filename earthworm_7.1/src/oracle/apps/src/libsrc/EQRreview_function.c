
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: EQRreview_function.c,v 1.12 2003/01/30 23:12:57 lucky Exp $
 *
 *    Revision history:
 *    $Log: EQRreview_function.c,v $
 *    Revision 1.12  2003/01/30 23:12:57  lucky
 *    *** empty log message ***
 *
 *    Revision 1.11  2002/09/17 16:40:35  lucky
 *    Added check for other origins before putting up link to their review
 *
 *    Revision 1.10  2002/06/28 21:06:22  lucky
 *    Lucky's pre-departure checkin. Most changes probably had to do with bug fixes
 *    in connection with the GIOC scaffold.
 *
 *    Revision 1.9  2002/05/28 17:25:41  lucky
 *    *** empty log message ***
 *
 *    Revision 1.8  2002/03/22 20:02:38  lucky
 *    Pre v6.1 checkin
 *
 *    Revision 1.5  2001/08/10 21:04:00  lucky
 *    NT display and review implemented.
 *
 *    Revision 1.4  2001/08/07 16:53:30  lucky
 *    Pre v6.0 checkin
 *
 *    Revision 1.3  2001/07/31 20:45:44  lucky
 *    Changed alarms nomenclature from User to Recipient
 *
 *    Revision 1.2  2001/07/28 00:43:53  lucky
 *     State of the code after debugging and testing prior to v6.0 release freeze.
 *
 *    Revision 1.1  2001/07/01 21:55:21  davidk
 *    Initial revision
 *
 *    Revision 1.4  2001/06/21 22:01:02  lucky
 *    Slightly changed the way in which the picks are displayed.
 *
 *    Revision 1.3  2001/06/21 21:26:25  lucky
 *    State of the code after LocalMag review was implemented and at least partially
 *    tested. The new Lomax applet (june 18 version) has been incorporated. ML's can
 *    be added, deleted, modified, as well as the other types of picks.
 *
 *    Revision 1.2  2001/05/15 18:47:36  davidk
 *    Moved functions around between the apps, DB API, and DB API INTERNAL
 *    levels.  Renamed functions and files.  Added support for amplitude
 *    magnitude types.  Reformatted makefiles.
 *
 *    Revision 1.5  2001/03/14 21:31:46  alex
 *    'pick the unpicked' now is sorted by distance. Alex
 *
 *    Revision 1.3  2001/03/13 22:28:18  alex
 *    *** empty log message ***
 *
 *    Revision 1.2  2001/03/13 21:44:35  alex
 *    'pick the unpicked' mods
 *
 *    Revision 1.1  2001/02/28 17:30:23  lucky
 *    Initial revision
 *
 *
 *
 */

/* include our own header files */
#include <sys/types.h>
#include <sys/stat.h>

#include <review.h>
#include <math.h>
#include <time_functions.h>
#include <ewdb_apps_utils.h>
#include <time_ew.h>
#include <gd.h>

#define MAX_SG2K_STR_LEN  50000

typedef struct LocalMagStruct
{
	int		valid; 				/* 1 = both mags; 0 = one mag; -1 = no mags */
	char 	Method[64];
	char 	Sta[10];
	char 	Net[10];
	char 	CommonComp[10]; /* first two characters of the component */
	char 	Comp1[10];
	char 	Comp2[10];
	int		GotWave1;
	int		GotWave2;
	char 	appletString[1024];
	double	Mag1;
	double	Mag2;
	double	MagAvg;
	double	Dist;

} LocalMagStas;

typedef struct
{
	char sta[10];
	char comp[10];
	char net[10];
	char sacfile[512];
	double range;
	double bearing;
} ITEM;


/* function prototypes */
static int  SetDefaultParams(SetVarsUserStruct * pParams);
static int  Relocate(int idEvent, char *EvtDir);
static void html_map(EWEventInfoStruct *pEvent, int debug_flag );
static void html_nomap(int idEvent);
static void html_review_form(EWEventInfoStruct *pEvent, char *WebRevDir);
static int  Retrieve3Components(char *sacfiles, char *WebRevDir,  
                                EWEventInfoStruct *pEvent, int ChanNum);

static int 	BuildLocalMagArray (EWEventInfoStruct *pEvent, 
							LocalMagStas *LMS, int LMS_len, int *NumStasBuilt);

int compRange( const void*, const void*); /* comparefunction for sorting snippets */
int compChanDist (const void *c1, const void *c2);
int LMS_SortByDist (const void *c1, const void *c2);


EWDB_OriginStruct	SortOrigin;
ITEM entry[500];	/* the array of unpicked snippets to be sorted */
int	NumLMS;
LocalMagStas	*LMS;

static	int		STATE_Map;
static	int		STATE_Unpicked;
static	EWDBid	STATE_idOrigin;
static	char	STATE_EvtDir[1024];


int		ReviewFunction (int idEvent, int idOrigin, int Action, int ShowMap, int ShowUnpicked)
{
	char						*configfile = "../params/review_event.d";
	FILE						*fp;
	EWEventInfoStruct			EventInfo;
	EWDB_WaveformStruct 		*pWaveform;
	EWDB_OriginStruct 			TmpOrigin;
	SAC_OriginStruct			SacOrig;
	int			 				i, j, done, DBFlags, NumOrigFound, NumOrigRetr;
	int							bRelocated = FALSE;
	struct stat					statbuf;
	char						cmd[1000];
	char						filename[1000];
	char						RevString[256];
	char  						WebRevDir[2048];
	char  						author[2048];

	DEBUG = 0;
	NumLMS = 0;


	STATE_Map = ShowMap;
	STATE_Unpicked = ShowUnpicked;
	STATE_idOrigin = idOrigin;


   /* Read the configuration file (path hardcoded relative to executable)
     *********************************************************************/
    ReadConfig (configfile);

	/* HTML header stuff */
	printf("Content-type: text/html\n\n");
	printf("<HTML>\n");
	printf("<HEAD><TITLE>Review %u</TITLE></HEAD>\n", idEvent);

	html_header (BackgroundColor, HeaderLogo, HeaderTag);

	printf("<CENTER>\n");
	printf("<H1><FONT COLOR=blue>Earthworm Quick Review<br></FONT></H1>\n" );
	printf("</CENTER>\n");

	/* Produce the javascript needed to run the Lomax applet */
	printf ("<SCRIPT LANGUAGE=\"JavaScript\" SRC=\"%s/lomax/%s\">\n", 
												WebDir, JavascriptFile);
	printf ("</SCRIPT></HEAD>\n");


	/* Open connection to database
	 *****************************/
	if( ewdb_api_Init(DBuser,DBpassword,DBservice ) != 0 )
	{
		html_logit ("", "Trouble connecting to database; exiting!\n");
		html_break();
		goto shutdown;
	}

	logit ("", "Connected to the DB!\n");
	logit ("", "Starting Eqreview: EventID=%u, Action=%d, Map=%d, Unpicked=%d\n", 
				idEvent, Action, ShowMap, ShowUnpicked);

	/* build the event struct file name */
	if (idOrigin <= 0)
	{
		html_logit ("", "Invalid idOrigin passed in - Abort.\n");
		html_break();
		goto shutdown;
	}
		

	sprintf (STATE_EvtDir, "%s%c%d", ReviewTmpDir, DIR_SLASH, idOrigin);

	sprintf (EvtStructFile, "%s%cEvtStruct.bin", STATE_EvtDir, DIR_SLASH);

	if (Action == ACT_OVERRIDE)
	{
		/* Delete the event directory and retrieve 
			event info from database */

#ifdef _WINNT

		sprintf (cmd, "rmdir /Q /S %s", STATE_EvtDir);
		fflush (NULL);
		system (cmd);
#else

		sprintf (cmd, "/bin/rm -rf %s", STATE_EvtDir);
		system (cmd);

#endif _WINNT

		Action = ACT_GETFROMDB;

	} /* action = OVERRIDE */
	
	/* Retrieve event info from DB into an ARC file */
	if (Action == ACT_GETFROMDB)
	{

		DBFlags = GETEWEVENTINFO_SUMMARYINFO | GETEWEVENTINFO_PICKS
                   | GETEWEVENTINFO_STAMAGS | GETEWEVENTINFO_COMPINFO
                   | GETEWEVENTINFO_COOKEDTF | GETEWEVENTINFO_WAVEFORM_DESCS;

		/*  Retrieve event info */
		if (ewdb_apps_GetDBEventInfoII (&EventInfo, idEvent, idOrigin, DBFlags) 
																!= EWDB_RETURN_SUCCESS)
		{
			html_logit ("", "Call to ewdb_apps_GetDBEventInfoII failed\n");
			html_break ();
			goto shutdown;
		}

		/* Make sure that we got the right event */
		if (idEvent != EventInfo.Event.idEvent)
		{
			html_logit ("", "Error retrieving event %d.\n", idEvent);
			html_break ();
			goto shutdown;
		}

#define DO_DEBUG
#ifdef DO_DEBUG
 logit ("", "After GetFrom DB\n");
 PrintEventInfo (&EventInfo);
#endif DO_DEBUG
#undef DO_DEBUG


		/* 
		 * Check the Origin's Source and compare against the 
		 * list of sources which we are allowed to review 
		 */
		i = 0;
		done = FALSE;
		if (EventInfo.GotLocation == TRUE)
			strcpy (author, EventInfo.PrefOrigin.sRealSource);
		else if (EventInfo.GotTrigger == TRUE)
			strcpy (author, EventInfo.CoincEvt.szSource);
			
		while ((i < NumRevSrcs) && (done == FALSE))
		{
			/* 
			 * Note that we want to be able to review automatic
			 * as well as solutions that were previously reviewed.
			 * Automatic solutions are inserted into the DB with
			 * the original source (e.g., logo of the hypoinverse
			 * module), whereas the reviewed solutions are marked
			 * with the original source with the string _REV
			 * prepended. We want to check against both.
			 */

			sprintf (RevString, "REV_%s", RevSrcs[i]);

			if (strcmp (RevSrcs[i], author) == 0)
				done = TRUE;
			else if (strcmp (RevString, author) == 0)
				done = TRUE;
			else
			{
				done = FALSE;
				i = i + 1;
			}
		} 

		if (done == FALSE)
		{
			logit ("", "Review Request denied for event %d, author %s\n", 
					EventInfo.Event.idEvent,
					EventInfo.Event.szSource);

			printf ("<center><strong><pre>\n");
			printf ("\n\nPROBLEM: Review of Event %u not permitted!\n",
									idEvent);
			printf ("The source of the Origin does not match.\n\n\n");

			printf ("</center></strong></pre>\n");
			html_break ();
			goto shutdown;
		}

			
		/* Make sure that the event directory doesn't already exist */
		if (stat (STATE_EvtDir, &statbuf) == 0)
		{
			/* 
				If the directory exist, either someone is already reviewing 
				the event, or the directory was left over because the browser
				bombed, or the reviewer didn't finish the procedure.

				We ask the user if it is Ok to over-write the directory.
			 */

			printf ("<center><strong><pre>\n");
			printf ("\n\nPROBLEM: Review directory for Event %u already exists!\n",
									idEvent);
			printf ("Someone may already be reviewing this event.\n\n\n");

			printf ("<A HREF=\"review_event\?EventID=%u?Action=%d?ShowMap=%d?"
					"ShowUnpicked=%d?OriginID=%d\"> "
					"Click here to remove the directory and continue?</A>\n", 
							idEvent, ACT_OVERRIDE, ShowMap, ShowUnpicked, idOrigin);

			printf ("</center></strong></pre>\n");
			html_break ();
			goto shutdown;
		}


		/* Create the Event directory */
		sprintf (cmd, "mkdir %s", STATE_EvtDir);
#ifdef _WINNT
		fflush (NULL);
#endif _WINNT
		system (cmd);


		/* Save SAC files and other info in the STATE_EvtDir directory */
		if (PopulateReviewDir (&EventInfo, STATE_EvtDir, POPULATE_REVIEW_PICKED) != EW_SUCCESS)
		{
			html_logit ("", "Call to PopulateReviewDir failed.\n");
			html_break ();
			goto shutdown;
		}


	} /* action = GET FROM DB */

	/* If Relocate flag is set, we invoke hyp2000 to relocate the event,
		only then do we read the resulting arc file and display the review page */

	if (Action == ACT_RELOCATE)
	{
		if (Relocate (idEvent, STATE_EvtDir) != EW_SUCCESS)
		{
			html_logit ("", "Call to Relocate failed.\n");
			html_break ();
			goto shutdown;
		}

		bRelocated = TRUE;
	} /* action = RELOCATE */


	/*
	 * NOTE:  The initial event information was written into an 
	 *  event struct file from the data available in the DB. At the same time,
	 *  we retrieved all available snippets and wrote them as SAC files
	 *  to enable graphical review.
	 *
	 *   Subsequently, the user may have changed the pick parameters
	 *    either manually via the web page, or using the graphical
	 *    reivewer. So, we re-read the contents of the evt file.
	 */

	if (Action != ACT_GETFROMDB)
	{
		/* Read the contents of the evt file */
		if ((fp = fopen (EvtStructFile, "rb")) == NULL)
		{
			html_logit ("", "Can't open %s\n", EvtStructFile);
			html_break ();
			goto shutdown;
		}

		/* Read the contents of the evt struct file*/
		fread ((void *) &EventInfo, sizeof (EWEventInfoStruct), 1, fp);

		/* Read the channel info structs */
		if (EventInfo.iNumChans > 0)
		{
			if ((EventInfo.pChanInfo = (EWChannelDataStruct *) malloc
				(EventInfo.iNumChans * sizeof (EWChannelDataStruct))) == NULL)
			{
				html_logit ("", "Can't malloc pChanInfo.\n");
				html_break ();
				goto shutdown;
			}

			fread ((void *) EventInfo.pChanInfo, 
					sizeof (EWChannelDataStruct), EventInfo.iNumChans, fp);

		}
	
		fclose (fp);

		EventInfo.iNumAllocChans = EventInfo.iNumChans;

		/* For display purposes, set the origin author to something useful */
		strcpy (EventInfo.PrefOrigin.sSource, "Under Review");
	}

	if ((Action == ACT_GETUNPICKED) ||
		((Action == ACT_GETFROMDB) && (ShowUnpicked == TRUE)))
	{
		if (PopulateReviewDir (&EventInfo, STATE_EvtDir, POPULATE_REVIEW_UNPICKED) != EW_SUCCESS)
		{
			html_logit ("", "Call to PopulateReviewDir failed.\n");
			html_break ();
			goto shutdown;
		}

		STATE_Unpicked = TRUE;
	}


	/* Create LocalMag array */
	if (EventInfo.iML != -1)
	{
		if ((LMS = (LocalMagStas *) malloc (DB_MAX_PHS_PER_EQ * 
											sizeof (LocalMagStas))) == NULL)
		{
			html_logit ("", "Could not malloc %d elements of LocalMag array.\n",
														DB_MAX_PHS_PER_EQ);
			html_break ();
			goto shutdown;
		}

		if (BuildLocalMagArray (&EventInfo, LMS, 
					DB_MAX_PHS_PER_EQ, &NumLMS) != EW_SUCCESS)
		{
			html_logit ("", "Call to BuildLocalMagArray failed.\n");
			html_break ();
			goto shutdown;
		}
	}


	if (EventInfo.iNumChans > 0)
	{
		if (ewdb_apps_BuildStationArray (&EventInfo, FALSE, 
						0.0, 0.0, 0.0, TRUE) != EWDB_RETURN_SUCCESS)
		{
			html_logit ("", "Call to ewdb_apps_BuildStationArray failed\n");
			html_break ();
			goto shutdown;
		}
	}



	/* show off the map */
	/* Put up record section link buttons */
	html_links2ora2rsec_gif (EventInfo.Event.idEvent, iNumTGDs, TGDS); 

	if (EventInfo.GotLocation == TRUE)
	{
		if (ShowMap == 1)
			html_map (&EventInfo, DEBUG); 
		else
			html_nomap (EventInfo.Event.idEvent); 

		/* Show Summary information */
		html_principal_summary (&EventInfo, bRelocated); 

		/* See if there are any other origins for this event */
        if (ewdb_api_GetOriginsForEvent (EventInfo.Event.idEvent, &TmpOrigin, 
						&NumOrigFound, &NumOrigRetr, 1) == EWDB_RETURN_FAILURE)
        {
            html_logit ("", "Call to ewdb_api_GetOriginsForEvent failed.\n");
            html_break();
            goto shutdown;
        }

        if (NumOrigFound > 1)
        {
            printf("<A HREF=\"review_other_solutions?EventID=%u\" "
                            "target=\"Review Other Solutions\">"
                            "CLICK HERE FOR OTHER SOLUTIONS</A>\n", 
													EventInfo.Event.idEvent);
       }

	}

	if (EventInfo.GotTrigger == TRUE)
	{
		/* Show Coincidence Summary information */
		html_coincidence_summary (&EventInfo, TRUE); 
	}


	/* Bring up the review form */

	sprintf (WebRevDir, "%s/tmp/%d", WebDir, idOrigin);
	html_review_form (&EventInfo, WebRevDir);


shutdown:
	html_trailer (WebHost, FooterLogo, FooterTag);

	ewdb_api_Shutdown ();

	logit("t", "review_event: terminating\n" );

	return (0);

}  /* end main */



static	int 	SetDefaultParams(SetVarsUserStruct * pParams)
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
      pParams: the mother of all structs, contains everything
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
  pParams->WGSSData.ShowUnpicked  = 0;
  pParams->WGSSData.ShowMap       = 0;
  pParams->WGSSData.idOrigin  	  = -1;
  pParams->WGSSData.ClickEffect   = CLICK_VIEW_EVENTS;
  pParams->WGSSData.StationClickEffect   = EWDB_STATION_DONT_DISPLAY_STATIONS;
  pParams->WGSSData.Options       = 0;
  pParams->WGSSData.XClick        = 0;
  pParams->WGSSData.YClick        = 0;
  pParams->WGSSData.GUIStateString[0]
                                  = 0;
  return(0);
}  /* End SetDefaultParams() */



/******************************************************************
*
* Create a system command line to call the Relocation 
*  algorithm. At this time this calls hypoinverse and localmag and 
*  works on SOLARIS only.
*
******************************************************************/
static	int		Relocate (int idEvent, char *EvtDir)
{

	char					cmd[1000];
	char					arcfile[256];
	char					evtfile[256];
	char					hypfile[256];
	char					outfile[256];
	int						magMsgSize;
	int						magChanSize;
	char					magMsg[DB_MAX_BYTES_PER_EQ];
	MAG_INFO				magSum;
	MAG_CHAN_INFO			*magChan;
	FILE					*fp;
	char					*ArcMsg;
	int						i, j, k, retsize, found, index;
	double					dDist, dAzm;
	EWEventInfoStruct		EventInfo;
	EWEventInfoStruct		RelEventInfo;
	EWDB_PeakAmpStruct		*pPeakAmp;
	EWDB_StationMagStruct 	*pStaMag;



	sprintf (arcfile, "%s%cArcFile", EvtDir, DIR_SLASH);
	sprintf (evtfile, "%s%cEvtStruct.bin", EvtDir, DIR_SLASH);


	/************************************************************

				HYPOINVERSE REVIEW PORTION 

	************************************************************/

	if ((ArcMsg = (char *) malloc (ARC_MSG_LEN * sizeof (char))) == NULL)
	{
		html_logit ("", "Can't malloc ArcMsg.\n");
		return EW_FAILURE;
	}

	/*
	 * First, read the event binary struct file and
	 * convert the event struct into Arc File
	 */

	if ((fp = fopen (evtfile, "rb")) == NULL)
	{
		html_logit ("", "Can't open %s\n", evtfile);
		return EW_FAILURE;
	}

	/* Read the contents of the evt struct file*/
	fread ((void *) &EventInfo, sizeof (EWEventInfoStruct), 1, fp);

	/* Read the channel info structs */
	if ((EventInfo.pChanInfo = (EWChannelDataStruct *) malloc
				(EventInfo.iNumChans * sizeof (EWChannelDataStruct))) == NULL)
	{
		html_logit ("", "Can't malloc pChanInfo.\n");
		return EW_FAILURE;
	}
	EventInfo.iNumAllocChans = EventInfo.iNumChans;


	fread ((void *) EventInfo.pChanInfo, sizeof (EWChannelDataStruct), 
												EventInfo.iNumChans, fp);

	fclose (fp);



	/* 
	 * If this is a coincidence event being located, we have to
	 * do something fancy first: fake an origin and magnitude.
	 */

	if (EventInfo.GotLocation == FALSE)
	{
		if (EventInfo.GotTrigger == FALSE)
		{
			logit ("", "Cannot proceed withour either Location or Coincidence.\n");
			return EW_FAILURE;
		}

		if (EventInfo.iNumChans <= 0)
		{
			logit ("", "Cannot proceed withour at least one channel.\n");
			return EW_FAILURE;
		}
	

		memset (&EventInfo.PrefOrigin, 0, sizeof (EWDB_OriginStruct));

		EventInfo.PrefOrigin.tOrigin = EventInfo.CoincEvt.tCoincidence;

		EventInfo.PrefOrigin.dLat = EventInfo.pChanInfo[0].Station.Lat;
		EventInfo.PrefOrigin.dLon = EventInfo.pChanInfo[0].Station.Lon;
		EventInfo.PrefOrigin.dDepth = 10.0;

		EventInfo.PrefOrigin.iFixedDepth = TRUE;
		EventInfo.PrefOrigin.BindToEvent = TRUE;
		EventInfo.PrefOrigin.SetPreferred = TRUE;


		memset (&EventInfo.Mags, 0, (MAX_MAGS_PER_ORIGIN * sizeof (EWDB_MagStruct)));

		EventInfo.iNumMags = 1;
		EventInfo.iPrefMag = 0;
		EventInfo.iMd = 0;

		EventInfo.Mags[0].iMagType = MAGTYPE_DURATION;
		EventInfo.Mags[0].dMagAvg = 0.0;
		EventInfo.Mags[0].bBindToEvent = TRUE;
		EventInfo.Mags[0].bSetPreferred = TRUE;
	}



	/* convert to arcfile */
	if ((ArcMsg = (char *) malloc (ARC_MSG_LEN * sizeof (char))) == NULL)
	{
		html_logit ("", "Can't malloc ArcMsg.\n");
		return EW_FAILURE;
	}

#define DO_DEBUG
#ifdef DO_DEBUG
 logit ("", "Before relocating\n");
 PrintEventInfo (&EventInfo);
#endif DO_DEBUG
#undef DO_DEBUG

	if (EWEvent2ArcMsg (&EventInfo, ArcMsg, ARC_MSG_LEN) != EW_SUCCESS)
	{
		html_logit ("", "Call to EWEvent2ArcMsg failed.\n");
		return EW_FAILURE;
	}

	if ((fp = fopen (arcfile, "wt")) == NULL)
	{
		html_logit ("", "can't open %s\n", arcfile);
		return EW_FAILURE;
	}
	fprintf (fp, "%s", ArcMsg);
	
	fclose (fp);


	/** Create the script to pass to hypoinverse **/

	sprintf (hypfile, "%s%cHyp", EvtDir, DIR_SLASH);
	sprintf (outfile, "%s%creloc_output", EvtDir, DIR_SLASH);

	if ((fp = fopen (hypfile, "wt")) == NULL)
	{
		html_logit  ("", "Can't open %s\n", hypfile);
		return EW_FAILURE;
	}

	sprintf (cmd, "@%s \n"
					"phs \n"
					"%s%cArcFile \n"
					"loc \n"
					"sto%c",
						HypoCfg, EvtDir, DIR_SLASH, '\0');

	fprintf (fp, "%s", cmd);
	fclose (fp);


	/* Chirp into the output file */
#ifdef _WINNT
	sprintf (cmd, "echo ===== START RELOCATION RUN ===== > %s", outfile);
	fflush (NULL);
#else
	sprintf (cmd, "echo ===== START RELOCATION RUN ===== > %s 2>&1", outfile);
#endif _WINNT
	system (cmd);

	/* Chirp into the output file */
#ifdef _WINNT
	sprintf (cmd, "echo ===== EXECUTE HYPOINVERSE ===== >> %s", outfile);
	fflush (NULL);
#else
	sprintf (cmd, "echo ===== EXECUTE HYPOINVERSE ===== >> %s 2>&1", outfile);
#endif _WINNT
	system (cmd);

	/* Execute hypoinverse with this script */
#ifdef _WINNT
	sprintf (cmd, "\\earthworm\\web\\bin\\nt_runHYP %s %s %s >> %s", 
									HypoDir, HypoBin, hypfile, outfile);
	fflush (NULL);
#else
	sprintf (cmd, "cd %s; %s < %s >> %s 2>&1", HypoDir, HypoBin, hypfile, outfile);
#endif _WINNT
	system (cmd);

	/* Chirp into the output file */
#ifdef _WINNT
	sprintf (cmd, "echo ===== DONE HYPOINVERSE ===== >> %s", outfile);
	fflush (NULL);
#else
	sprintf (cmd, "echo ===== DONE HYPOINVERSE ===== >> %s 2>&1", outfile);
#endif _WINNT
	system (cmd);


#ifdef _WINNT
	sprintf (cmd, "echo ===== COPY HYPOINVERSE OUTPUT ===== >> %s", outfile);
	fflush (NULL);
#else
	sprintf (cmd, "echo ===== COPY HYPOINVERSE OUTPUT ===== >> %s 2>&1", outfile);
#endif _WINNT
	system (cmd);

	/* Copy the resulting arc file over the original arc file */
#ifdef _WINNT
	/** HYPOINVERSE HACK - nt version does not produce hypo.arc files ???? **/
	sprintf (cmd, "copy %s\\hypo.arc %s\\ArcFile >> %s", HypoDir, EvtDir, outfile);
	fflush (NULL);
#else
	sprintf (cmd, "cd %s; cp hypo.arc %s/ArcFile >> %s 2>&1", HypoDir, EvtDir, outfile);
#endif _WINNT
	system (cmd);


/* Try both filenames == seems random enough */
#ifdef _WINNT
	sprintf (cmd, "copy %s\\fort.7 %s\\ArcFile >> %s", HypoDir, EvtDir, outfile);
	fflush (NULL);
#else
	sprintf (cmd, "cd %s; cp fort.7 %s/ArcFile >> %s 2>&1", HypoDir, EvtDir, outfile);
#endif _WINNT
	system (cmd);


	/* Copy the PRT file for display */
#ifdef _WINNT
	sprintf (cmd, "copy %s\\fort.15 %s\\Hyp.prt >> %s", HypoDir, EvtDir, outfile);
	fflush (NULL);
#else
	sprintf (cmd, "cd %s; cp fort.15 %s/Hyp.prt >> %s 2>&1", HypoDir, EvtDir, outfile);
#endif _WINNT
	system (cmd);



	/* Chirp into the output file */
#ifdef _WINNT
	sprintf (cmd, "echo ===== CLEANUP HYPOINVERSE OUTPUT ===== >> %s", outfile);
	fflush (NULL);
#else
	sprintf (cmd, "echo ===== CLEANUP HYPOINVERSE OUTPUT ===== >> %s 2>&1", outfile);
#endif _WINNT
	system (cmd);

	/* Cleanup */
#ifdef _WINNT
	sprintf (cmd, "del /Q %s %s\\hypo.* %s\\fort.* >> %s", 
								hypfile, HypoDir, HypoDir, outfile);
	fflush (NULL);
#else
	sprintf (cmd, "rm %s; cd %s; rm hypo.*; rm fort.* >> %s 2>&1", hypfile, HypoDir, outfile);
#endif _WINNT
	system (cmd);

	/* Chirp into the output file */
#ifdef _WINNT
	sprintf (cmd, "echo ===== DONE RELOCATION RUN ===== >> %s", outfile);
	fflush (NULL);
#else
	sprintf (cmd, "echo ===== DONE RELOCATION RUN ===== >> %s 2>&1", outfile);
#endif _WINNT
	system (cmd);


	/*
	 *  We have relocated using hypoinverse. Great. Now,
	 * we need to read the output arc file and reconcile 
	 * it with the information we have in the EventInfo.
	 *  Note the assumption that the pick information has
	 * not changed, so we only need to worry about origin 
	 * and magnitude. Maybe this is not a good assumption? 
	 */  


	if ((fp = fopen (arcfile, "rt")) == NULL)
	{
		html_logit ("", "can't open %s\n", arcfile);
		return EW_FAILURE;
	}

	retsize = fread ((void *) ArcMsg, sizeof (char), ARC_MSG_LEN, fp);

	fclose (fp);


	/* Convert the ArcMsg into the DB structs */
	/* Note: pChanInfo gets malloced inside ArcMsg2EWEvent */
	if (ArcMsg2EWEvent (&RelEventInfo, ArcMsg, retsize) != EW_SUCCESS)
	{
		html_logit ("", "Call to ArcMsg2EWEvent failed.\n");

		/* Display the output of hypoinverse -- something probably went wrong */

        printf ("<CENTER><PRE><STRONG>\n");
        printf ("Possible problem with HYPOINVERSE.\n");
        printf ("<A HREF=\"%s/reloc_output\" target=\"Relocate Output\">"
                   "Click here to see the output of reprocessing.</A>\n\n", EvtDir);
        printf ("</CENTER></PRE></STRONG>\n");
		return EW_FAILURE;
	}
	free (ArcMsg);

	/* 
	 * Compute epicentral distance if this was a triggered event 
	 */

	if (EventInfo.GotLocation == FALSE)
	{
		for (i = 0; i < EventInfo.iNumChans; i++)
		{
			if (geo_to_km (EventInfo.PrefOrigin.dLat, 
							EventInfo.PrefOrigin.dLon,
							EventInfo.pChanInfo[i].Station.Lat,
							EventInfo.pChanInfo[i].Station.Lon,
							&dDist, 
							&dAzm) != 1)
			{
				logit ("", "Call to geo_to_km failed for chan %d\n", i);
				return EW_FAILURE;
			}

			EventInfo.pChanInfo[i].Arrivals[0].dDist = dDist;
			EventInfo.pChanInfo[i].Arrivals[0].dAzm = dAzm;
		}
	}

	/* This now becomes a located event */
	EventInfo.GotLocation = TRUE;

	/* Copy the origin and magnitude information */

	EventInfo.PrefOrigin.tOrigin = RelEventInfo.PrefOrigin.tOrigin;
	EventInfo.PrefOrigin.dLat = RelEventInfo.PrefOrigin.dLat;
	EventInfo.PrefOrigin.dLon = RelEventInfo.PrefOrigin.dLon;
	EventInfo.PrefOrigin.dErrLat = RelEventInfo.PrefOrigin.dErrLat;
	EventInfo.PrefOrigin.dErrLon = RelEventInfo.PrefOrigin.dErrLon;
	EventInfo.PrefOrigin.dErZ = RelEventInfo.PrefOrigin.dErZ;
	EventInfo.PrefOrigin.iGap = RelEventInfo.PrefOrigin.iGap;
	EventInfo.PrefOrigin.dDmin = RelEventInfo.PrefOrigin.dDmin;
	EventInfo.PrefOrigin.dRms = RelEventInfo.PrefOrigin.dRms;
	EventInfo.PrefOrigin.iAssocRd = RelEventInfo.PrefOrigin.iAssocRd;
	EventInfo.PrefOrigin.iAssocPh = RelEventInfo.PrefOrigin.iAssocPh;
	EventInfo.PrefOrigin.iUsedRd = RelEventInfo.PrefOrigin.iUsedRd;
	EventInfo.PrefOrigin.iUsedPh = RelEventInfo.PrefOrigin.iUsedPh;
	EventInfo.PrefOrigin.iE0Azm = RelEventInfo.PrefOrigin.iE0Azm;
	EventInfo.PrefOrigin.iE1Azm = RelEventInfo.PrefOrigin.iE1Azm;
	EventInfo.PrefOrigin.iE2Azm = RelEventInfo.PrefOrigin.iE2Azm;
	EventInfo.PrefOrigin.iE0Dip = RelEventInfo.PrefOrigin.iE0Dip;
	EventInfo.PrefOrigin.iE1Dip = RelEventInfo.PrefOrigin.iE1Dip;
	EventInfo.PrefOrigin.iE2Dip = RelEventInfo.PrefOrigin.iE2Dip;
	EventInfo.PrefOrigin.dE0 = RelEventInfo.PrefOrigin.dE0;
	EventInfo.PrefOrigin.dE1 = RelEventInfo.PrefOrigin.dE1;
	EventInfo.PrefOrigin.dE2 = RelEventInfo.PrefOrigin.dE2;
	
	EventInfo.iNumMags = RelEventInfo.iNumMags;
	EventInfo.iPrefMag = RelEventInfo.iMd;
	EventInfo.iMd = RelEventInfo.iMd;

	/* we've just executed hypoinverse: Md is preferred, and ML is in limbo */
	EventInfo.iML = -1;

	EventInfo.Mags[EventInfo.iPrefMag].dMagAvg = RelEventInfo.Mags[RelEventInfo.iPrefMag].dMagAvg;
	EventInfo.Mags[EventInfo.iPrefMag].dMagErr = RelEventInfo.Mags[RelEventInfo.iPrefMag].dMagErr;
	EventInfo.Mags[EventInfo.iPrefMag].iNumMags = RelEventInfo.Mags[RelEventInfo.iPrefMag].iNumMags;

  /* Copy individual arrival info */
  for (i = 0; i < EventInfo.iNumChans; i++)
  {
    if (EventInfo.pChanInfo[i].iNumArrivals > 0)
    {
      found = FALSE;
      for (j = 0; (j < RelEventInfo.iNumChans) && (found == FALSE); j++)
      {
        if ((strcmp (EventInfo.pChanInfo[i].Station.Sta,
                  RelEventInfo.pChanInfo[j].Station.Sta) == 0) &&
            (strcmp (EventInfo.pChanInfo[i].Station.Comp,
                  RelEventInfo.pChanInfo[j].Station.Comp) == 0) &&
            (strcmp (EventInfo.pChanInfo[i].Station.Net,
                  RelEventInfo.pChanInfo[j].Station.Net) == 0))
        found = TRUE;
      }

      if (found == TRUE)
      {
        for (k = 0; k < EventInfo.pChanInfo[i].iNumArrivals; k++)
        {
          EventInfo.pChanInfo[i].Arrivals[k].tResPick =
                RelEventInfo.pChanInfo[j-1].Arrivals[k].tResPick;

          EventInfo.pChanInfo[i].Stamags[k].dMag =
                RelEventInfo.pChanInfo[j-1].Stamags[k].dMag;

        }
      }
      else
      {
        logit ("", "Pick for %s.%s.%s not found in the ArcFile.\n",
                  EventInfo.pChanInfo[i].Station.Sta,
                  EventInfo.pChanInfo[i].Station.Comp,
                  EventInfo.pChanInfo[i].Station.Net);
      }
    }
  }

#ifdef DO_DEBUG
 logit ("", "Before ML Review\n");
 PrintEventInfo (&EventInfo);
#endif DO_DEBUG
#undef DO_DEBUG

	/************************************************************

				ML REVIEW PORTION 

	************************************************************/

	/* Chirp into the output file */
#ifdef _WINNT
	sprintf (cmd, "echo ===== EXECUTE LOCALMAG ===== >> %s", outfile);
	fflush (NULL);
#else
	sprintf (cmd, "echo ===== EXECUTE LOCALMAG ===== >> %s 2>&1", outfile);
#endif _WINNT
	system (cmd);

	/* 
	 * Execute localmag, but first set the log directory to point 
	 * to the event directory -- we don't want to make a mess
	 * on the disk, afterall. 
	 */
#ifdef _WINNT
	sprintf (cmd, "\\earthworm\\web\\bin\\nt_runLM %s %s %s %s >> %s", 
					EvtDir, LM_progname, LM_review_configfile,
					arcfile, outfile);
	fflush (NULL);	
#else
	putenv ("EW_LOG=./");

	sprintf (cmd, "cd %s; %s %s < %s >> %s 2>&1", 
						EvtDir,  LM_progname, 
						LM_review_configfile, arcfile, outfile);

#endif _WINNT
	system (cmd);

#ifndef _WINNT
	/* reset the log directory */
	putenv (envEW_LOG);
#endif


	/* 
	 * Read the TYPE_MAGNITUDE file back in, parse it, 
	 * and update the event structure 
	 */

	sprintf (hypfile, "%s%c%s", EvtDir, DIR_SLASH, LM_outputfile);

	if ((fp = fopen (hypfile, "rt")) == NULL)
	{	
		logit ("", "Could not open LM outputfile %s; assuming no ML\n", hypfile);

	}
	else
	{
		magMsgSize = fread (magMsg, sizeof (char), DB_MAX_BYTES_PER_EQ, fp);

		fclose (fp);

		magMsg[magMsgSize] = '\0'; /* null-terminate, just in case */ 


		if (magMsgSize > 0)
		{
			/* parse the magSum part of the message
			***************************************/
			if (rd_mag (magMsg, magMsgSize, &magSum) < 0)
			{
				html_logit ("", "Call to rd_mag failed.\n");
				return EW_FAILURE;
			}

			/* We only deal with local magnitudes */
			if(magSum.imagtype != LM_LocalMagType)
			{
				html_logit ("", "Got invalid magnitude type: %d.\n", magSum.imagtype);
				return EW_FAILURE;
			}
	
			/* retrieve station specific stuff */
			magChanSize = MAX_PHS_PER_EQ * sizeof (MAG_CHAN_INFO);
			if ((magChan = (MAG_CHAN_INFO *) malloc (magChanSize)) == NULL)
			{
				html_logit ("", "Could not malloc %d MAG_CHAN_INFO structs.\n", 
																	MAX_PHS_PER_EQ);
				return EW_FAILURE;
			}
	
			if (rd_chan_mag (magMsg, magMsgSize, magChan, magChanSize) < 0)
			{
				html_logit ("", "Call to rd_chan_mag failed.\n");
				return EW_FAILURE;
			}
		
			/* Update the ML magnitude */
			if ((EventInfo.iML = EventInfo.iNumMags) >= MAX_MAGS_PER_ORIGIN)
			{
				logit ("", "Max number of mags reached, ignoring.\n");
				EventInfo.iML = -1;
			}
			else
				EventInfo.iNumMags = EventInfo.iNumMags + 1;
		

			if (EventInfo.iML >= 0)
			{
				EventInfo.Mags[EventInfo.iML].iMagType = magSum.imagtype;
				EventInfo.Mags[EventInfo.iML].dMagAvg  = (float)magSum.mag;
				EventInfo.Mags[EventInfo.iML].dMagErr  = (float)magSum.error;
				EventInfo.Mags[EventInfo.iML].iNumMags = magSum.nchannels;
	
				/* Set as preferred, if so configured */
				if (MakeLMPreferred == TRUE)
					EventInfo.iPrefMag = EventInfo.iML;
		
				/* 
				 * Remove all previous ML stamags:  We believe that
				 * whatever localmag returns is gospel.
				 */
	
				for (i = 0; i < EventInfo.iNumChans; i++)
				{
					for (j = 0; j < EventInfo.pChanInfo[i].iNumStaMags; j++)
					{
						if (EventInfo.pChanInfo[i].Stamags[j].iMagType == LM_LocalMagType)
						{
							/* remove it */	
							if (j == (EventInfo.pChanInfo[i].iNumStaMags - 1))
								EventInfo.pChanInfo[i].iNumStaMags = 
											EventInfo.pChanInfo[i].iNumStaMags - 1;
							else
							{
								for (k = j; k < (EventInfo.pChanInfo[i].iNumStaMags - 1); k++)
									memcpy (&EventInfo.pChanInfo[i].Stamags[k],
												&EventInfo.pChanInfo[i].Stamags[k+1],
												sizeof (EWDB_StationMagStruct));
	
								EventInfo.pChanInfo[i].iNumStaMags = 
											EventInfo.pChanInfo[i].iNumStaMags - 1;
							}
						}
					}
				}
		
				/* Update the individual stamags */
				for (i = 0; i < magSum.nchannels; i++)
				{
					index = -1;
					for (j = 0; (j < EventInfo.iNumChans && index == -1); j++)
					{
						if ((strcmp (magChan[i].sta, EventInfo.pChanInfo[j].Station.Sta) == 0) &&
						   (strcmp (magChan[i].comp, EventInfo.pChanInfo[j].Station.Comp) == 0) &&
						   (strcmp (magChan[i].net, EventInfo.pChanInfo[j].Station.Net) == 0))
						{
							index = j;
						}
					}

					if (index != -1)
					{
						/* channel found -- insert a new ML type stamag  */
	
						EventInfo.pChanInfo[index].iNumStaMags = 
								EventInfo.pChanInfo[index].iNumStaMags + 1;
	
	
						if (EventInfo.pChanInfo[index].iNumStaMags >= MDPCPE)
						{
							logit ("", "Chan %d: max number of stamags reached.\n", index);
						}
						else
						{
							/* insert new stamag into the structure */
	
							pStaMag = 
								&EventInfo.pChanInfo[index].Stamags[EventInfo.pChanInfo[index].iNumStaMags-1];
	
	
							pStaMag->dMag = (float)(magChan[i].mag); 
							pStaMag->iMagType = LM_LocalMagType;
							pStaMag->idChan = -1;
							pStaMag->idMagnitude = -1;
	
							pPeakAmp = &pStaMag->StaMagUnion.PeakAmp;
	
							if (magSum.imagtype == MAGTYPE_LOCAL_PEAK2PEAK)
							{
								pPeakAmp->dAmp1 = (float)(magChan[i].Amp1);
								pPeakAmp->tAmp1 = magChan[i].Time1;
								pPeakAmp->dAmpPeriod1 = (float)(magChan[i].Period1);

								pPeakAmp->dAmp2 = (float)(magChan[i].Amp2);
								pPeakAmp->tAmp2 = magChan[i].Time2;
								pPeakAmp->dAmpPeriod2 = (float)(magChan[i].Period2);
							}
							else if (magSum.imagtype == MAGTYPE_LOCAL_ZERO2PEAK)
							{
								pPeakAmp->dAmp1 = (float)(magChan[i].Amp1);
								pPeakAmp->tAmp1 = magChan[i].Time1;
								pPeakAmp->dAmpPeriod1 = (float)(magChan[i].Period1);

								pPeakAmp->dAmp2 = MAG_NULL;
								pPeakAmp->tAmp2 = MAG_NULL;
								pPeakAmp->dAmpPeriod2 = MAG_NULL;
							}
							else
							{
								/* This should never happen, but.. */
								html_logit ("", "Mag channel %d: Invalid magtype: %d\n", 
																i, magSum.imagtype);
								return EW_FAILURE;
							}

		
						} /* ML type stamag can be inserted */
	
					} /* If channel was found */
					else
					{
						/* channel not found */
					}
	
	
				} /* for loop (i) over mag channels */
		
			} /* If iML >= 0 */
	
		} /* If we got a message of size > 0 */

	} /* output from localmag was opened without error */


	/* cleanup - remove ArcFile */
#ifdef _WINNT
	sprintf (cmd, "del /Q %s", arcfile);
	fflush (NULL);
#else
	sprintf (cmd, "rm %s", arcfile);
#endif _WINNT
	system (cmd);


	/* Write EventInfo back to the binary file */
	if ((fp = fopen (evtfile, "wb")) == NULL)
	{
		html_logit ("", "can't open %s\n", evtfile);
		return EW_FAILURE;
	}

	EventInfo.iNumAllocChans = EventInfo.iNumChans;

	fwrite ((void *) &EventInfo, sizeof (EWEventInfoStruct), 1, fp);

	/* append channel info structs */

	fwrite ((void *) EventInfo.pChanInfo, sizeof (EWChannelDataStruct),
													EventInfo.iNumChans, fp);
	fclose (fp);


#ifdef DO_DEBUG
 logit ("", "After relocating\n");
 PrintEventInfo (&EventInfo);
#endif DO_DEBUG
#undef DO_DEBUG


	return EW_SUCCESS;

}



/******************************************************
      Draw map with hypocenter and stations
 **********************************************************/
static 	void	html_map (EWEventInfoStruct *pEvent, int debug_flag )
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
   int nStations,nStationsFound, nStations1, nStationsFound1;
   int nPickedStations;
   int nSilentStations;
   int RetVal;
   EQSPicture Pic={{0,0,0,0},{0.0,0.0,0.0,0.}};
   char *AreaBuf, *AreaBuf2;
   int StationBufSize=1000;
   float StationDisplayWidth=2.0;
   int done, i, j;
	 double tmpW, tmpH;

   int SEQ_done=FALSE;


	if (pEvent == NULL)
	{
		html_logit ("", "html_map: invalid arguments passed in! EXITTING!\n");
		exit(-1);
	}


   AreaBuf=malloc(300000);
   AreaBuf2=malloc(300000);

  if(!(AreaBuf && AreaBuf2))
  {
    html_logit ("", "html_map: Could not malloc Areabuffers (300k bytes each)!EXITTING!\n");
		exit(-1);
  }
  else
  {
    /* Initialize to blank */
    AreaBuf2[0] = AreaBuf[0] = 0;
  }



  /* We only care about one event */
  if((pEvents = (EWDB_EventListStruct *) malloc(sizeof(EWDB_EventListStruct))) == NULL)
  {
    html_logit ("", "Unable to allocate space for retrieval of the Event!  EXITTING!\n");
    exit(-1);
  }

  if((pStations = (EWDB_StationStruct *) malloc(StationBufSize * 
									sizeof(EWDB_StationStruct))) == NULL)
  {
    html_logit ("", "Unable to allocate space for retrieval of %d Stations!  EXITTING!\n",
     	     StationBufSize);
    exit(-1);
  }

  if((pPickedStations = (EWDB_StationStruct *) malloc(StationBufSize * 
									sizeof(EWDB_StationStruct))) == NULL)
  {
    html_logit ("", "Unable to allocate space for retrieval of %d Stations!  EXITTING!\n",
     	     StationBufSize);
    exit(-1);
  }

  if((pSilentStations = (EWDB_StationStruct *) malloc(StationBufSize * 
									sizeof(EWDB_StationStruct))) == NULL)
  {
    html_logit ("", "Unable to allocate space for retrieval of %d Stations!  EXITTING!\n",
     	     StationBufSize);
    exit(-1);
  }



   /* Set Default web and config file params */
   if(SetDefaultParams(&DefaultParams))
   {
     html_logit ("", "Failed to set Default Params!  EXITTING!\n");
     exit(-1);
   }


   /* Now Copy the defaults to the struct that will be configured. */
   memcpy(&WebParams,&DefaultParams,sizeof(SetVarsUserStruct));


  /*
   * We want to show map with all picked stations. Find the largest
   * distance and use it as the width and height of the map.
   */
  WebParams.LPSData.MapData.CenterLat = pEvent->PrefOrigin.dLat; 
  WebParams.LPSData.MapData.CenterLon = pEvent->PrefOrigin.dLon; 
  WebParams.LPSData.MapData.LatHeight = 0;
  WebParams.LPSData.MapData.LonWidth = 0;

  for (i = 0; i < pEvent->iNumChans; i++)
  {
    if ((pEvent->pChanInfo[i].iNumArrivals > 0) && 
					(pEvent->pChanInfo[i].Station.Lat != 0.0) &&
					(pEvent->pChanInfo[i].Station.Lon != 0.0))
    {

      tmpW = 2.0 * fabs (pEvent->pChanInfo[i].Station.Lon - pEvent->PrefOrigin.dLon);
      tmpH = 2.0 * fabs (pEvent->pChanInfo[i].Station.Lat - pEvent->PrefOrigin.dLat);

      if (tmpW > WebParams.LPSData.MapData.LonWidth)
        WebParams.LPSData.MapData.LonWidth = (float)tmpW;

      if (tmpH > WebParams.LPSData.MapData.LatHeight)
        WebParams.LPSData.MapData.LatHeight = (float)tmpH;
    }
  }

  /* Create a little cushion around the map */
  if (WebParams.LPSData.MapData.LatHeight <= 0.0)
    WebParams.LPSData.MapData.LatHeight = (float)0.1;
  else
    WebParams.LPSData.MapData.LatHeight = (float)(WebParams.LPSData.MapData.LatHeight + 0.1);

  if (WebParams.LPSData.MapData.LonWidth <= 0.0)
    WebParams.LPSData.MapData.LonWidth = (float)0.1;
  else
    WebParams.LPSData.MapData.LonWidth = (float)(WebParams.LPSData.MapData.LonWidth + 0.1);

   WebParams.LPSData.MapData.Rivers = 2051;
   WebParams.LPSData.MapData.Politicals = 8;
   WebParams.LPSData.MapData.bLatLonLines = 0;
   WebParams.LPSData.MapData.bBorder= 0;
   WebParams.LPSData.MapData.ProjectionType= atoi ("m");
   sprintf (WebParams.LPSData.MapData.MapName, "%d", pEvent->Event.idEvent);

   WebParams.MSISData.ImageFormat= 0;
   WebParams.MSISData.ImageFormatsSupported = IMAGE_GIF;
   sprintf (WebParams.MSISData.ImageFileName, "../html/EWDB_GL_temp_image_%ld.gif",
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
    html_logit ("", "Call to ewdb_api_GetStationList failed!  EXITTING!\n");
    exit(-1);
  }

  if(Pic.deg[PIC_LEFT] < -180.0 || Pic.deg[PIC_RIGHT] > 180.0)
  {
    if(Pic.deg[PIC_LEFT] < -180.0)
    {
      /* we need to rotate 360 degrees */

      RetVal = ewdb_api_GetStationList(Pic.deg[PIC_BOT], Pic.deg[PIC_TOP],
                                   Pic.deg[PIC_LEFT]+180.0, 180, 
                                   pEvent->PrefOrigin.tOrigin, &pStations[nStations],
                                   &nStationsFound1, &nStations1, StationBufSize-nStations);

    }
    else if(Pic.deg[PIC_RIGHT] > 180.0)
    {
      /* we need to rotate 360 degrees */
      RetVal = ewdb_api_GetStationList(Pic.deg[PIC_BOT], Pic.deg[PIC_TOP],
                                   -180.0, Pic.deg[PIC_RIGHT]-180.0,
                                   pEvent->PrefOrigin.tOrigin, &pStations[nStations],
                                   &nStationsFound1, &nStations1, StationBufSize-nStations);
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

 		   if (pEvent->pChanInfo[j].iNumArrivals > 0)
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
     html_logit ("", "Call to DrawMap failed! EXITTING!\n");
     exit(-1);
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
     html_logit ("", "Call to DrawMap failed! EXITTING!\n");
     exit(-1);
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
                 + PIC_LEFT_PIX_BORDER+PIC_RIGHT_PIX_BORDER),
               (WebParams.LPSData.PixelHeight
                 + PIC_TOP_PIX_BORDER+PIC_BTM_PIX_BORDER));



   printf("<BR>\n");
   printf("<MAP NAME=\"search\">\n");
   printf (AreaBuf);
   printf("</MAP></FORM>\n");


   printf (" <CENTER> Red triangles indicate stations that reported "
            " picks for this event. </CENTER>");

	printf ("<hr>\n");
	printf ("<br>\n");
	printf ("<center>\n");
	printf ("<A HREF=\"review_event\?EventID=%u\?ShowMap=0\?Action=%d\?"
			"ShowUnpicked=%d?OriginID=%u\"> "
			"CLICK HERE TO HIDE THE MAP</A>\n", 
				pEvent->Event.idEvent, ACT_NULL, STATE_Unpicked, STATE_idOrigin);
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
static 	void	html_nomap (int idEvent)
{

	printf ("<hr>\n");
	printf ("<br>\n");
	printf ("<center>\n");
	printf ("<A HREF=\"review_event\?EventID=%u\?ShowMap=1\?Action=%d\?"
			"ShowUnpicked=%d?OriginID=%u\"> "
			"CLICK HERE TO SHOW THE MAP</A>\n", 
				idEvent, ACT_NULL, STATE_Unpicked, STATE_idOrigin);
	printf ("<br>\n");
	printf ("<br>\n");
	printf ("</center>\n");

}



/******************************************************
     Write form allowing the user to review phase info
 **********************************************************/
static 	void	html_review_form (EWEventInfoStruct *pEvent, char *WebRevDir)
{

	EWDB_ArrivalStruct 				*par;
	EWDB_StationStruct 				*psta;
	EWDB_StationMagStruct 			*pstamag;
	char  							timestr[DATESTR23];
	int   							i, j, wt, ChanNameMode;
	int   							NumFiles, NumStas;
	char  							*lomax_str;
	char  							*sacfile;
	char  							tmpstr[512];
	double							range;
	double							bearing;
	


	if (pEvent == NULL)
	{
		html_logit ("", "html_review_form: Invalid arguments passed in! EXITTING!\n");
		exit(-1);
	}

	printf("\n");


	printf ("<FORM NAME=\"ReviewForm\" ACTION=\"review_next_action\" METHOD=POST>\n");

	printf ("<HR><BR><CENTER><PRE><STRONG>\n");

	printf ("EDIT EVENT PARAMETERS\n");
	printf ("</STRONG>\n");
	printf ("NOTE: Relocating the event will override any modifications to the magnitudes.\n");


	/* Modifiable magnitudes */
	printf ("</PRE><TABLE border=2 cellspacing=2>");

	printf ("<TR align=center>\n");
	printf ("<TD>Magnitude (Md)</TD>\n");
	if (pEvent->iMd >= 0)
	{
		printf ("<TD> <INPUT NAME=\"Md\" TYPE=text SIZE=5 "
					"MAXLENGTH=5 VALUE=\"%0.2f\"></TD>\n",
							pEvent->Mags[pEvent->iMd].dMagAvg);
	}
	else
	{
		printf ("<TD> <INPUT NAME=\"Md\" TYPE=text SIZE=5 "
					"MAXLENGTH=5 VALUE=\"N/A\"></TD>\n");
	}

	/* Preferred selector button */
	if ((pEvent->iMd >= 0) && (pEvent->iMd == pEvent->iPrefMag))
		printf ("<TD> <INPUT TYPE=radio NAME=\"PrefMag\" "
						"VALUE=\"%d\" CHECKED> Preferred?</TD>\n", MAGTYPE_DURATION); 
	else
		printf ("<TD> <INPUT TYPE=radio NAME=\"PrefMag\" "
						"VALUE=\"%d\"> Preferred?</TD>\n", MAGTYPE_DURATION); 
	printf ("</TR>\n");


	printf ("<TR align=center>\n");
	printf ("<TD>Magnitude (ML)</TD>\n");
	if (pEvent->iML >= 0)
	{
		printf ("<TD> <INPUT NAME=\"ML\" TYPE=text SIZE=5 "
						"MAXLENGTH=5 VALUE=\"%0.2f\"></TD>\n",
							pEvent->Mags[pEvent->iML].dMagAvg);
	}
	else
	{
		printf ("<TD> <INPUT NAME=\"ML\" TYPE=text SIZE=5 "
							"MAXLENGTH=5 VALUE=\"N/A\"></TD>\n");
	}

	/* Preferred selector button */
	if ((pEvent->iML >= 0) && (pEvent->iML == pEvent->iPrefMag))
		printf ("<TD> <INPUT TYPE=radio NAME=\"PrefMag\" "
					"VALUE=\"%d\" CHECKED> Preferred?</TD>\n", LM_LocalMagType); 
	else
		printf ("<TD> <INPUT TYPE=radio NAME=\"PrefMag\" "
						"VALUE=\"%d\">  Preferred?</TD>\n", LM_LocalMagType); 
	printf ("</TR>\n");


	printf ("</TABLE><PRE>\n\n");

	printf ("<BR><BR><HR><BR><STRONG>\n");
	printf ("EDIT PICK PARAMETERS\n");
	printf ("</STRONG>\n");

	lomax_str = malloc (MAX_SG2K_STR_LEN);
	sacfile = malloc (50000);

	/*******************************
     *  Arrival picks section
     ******************************/

	printf ("<B>ARRIVAL PICKS</B>\n\n");

	/*** Put up a link allowing multi-channel Lomax review ***/

	/* 
	 * For each channel, if it contains an arrival pick AND
	 * has waveforms to display, add it to the list of files to review. 
 	 * Do this up to NumArrivalPicksToShow
	 */
	if (NumArrivalPicksToShow > 0)
	{

		NumFiles = 0;
		for (i = 0; ((i < pEvent->iNumChans) && (NumFiles < NumArrivalPicksToShow)); i++)
		{
	
			psta = &pEvent->pChanInfo[i].Station;
	
			/* Check for waveforms and picks */
			if ((pEvent->pChanInfo[i].iNumWaveforms > 0) 
					&& (pEvent->pChanInfo[i].iNumArrivals > 0))
			{
				if (NumFiles == 0)
				{
					sprintf (sacfile, "'%s/%s.%s.%s#%d'", WebRevDir,
							psta->Sta, psta->Comp, psta->Net, NumFiles);
				}
				else
				{
					sprintf (tmpstr, ", '%s/%s.%s.%s#%d'", WebRevDir,
							psta->Sta, psta->Comp, psta->Net, NumFiles);
					strcat (sacfile, tmpstr);
				}
	
				NumFiles = NumFiles + 1;
			}
		}
	
	
		if (NumFiles != 0)
		{
			/* build the rest of the lomax string */
			BuildAppletString (lomax_str, pEvent, WebHost, sacfile);
	
			/* display the link */
			printf ("<A HREF=\"%s\">Click here to review %d closest picks.</A>\n",
							lomax_str, NumFiles);
		}
		else
		{
			/* display our regrets that there are no traces to review */
			logit ("", "No waveforms to display -- skipping multi-channel applet.\n");
		}
	}
	printf ("<BR><BR><BR><BR><BR>\n");


	printf ("Select the Checkbox on the left to delete a pick.\n");
	printf ("Manually modify the time and weight of a pick.\n");
	printf ("Click on the Channel name to graphically review a pick.\n");
	printf ("</PRE></CENTER>\n");

	printf ("<BR><BR>\n");
	printf("<CENTER>\n");	
	printf ("<TABLE border=2 cellspacing=2>\n");
	printf ("<TR align=center>\n");
	printf ("<TH>Delete Pick?</TH>\n");
	printf ("<TH>Channel</TH>\n");
	printf ("<TH>Phase</TH>\n");
	printf ("<TH>FM</TH>\n");
	printf ("<TH>Pick Time</TH>\n");
	printf ("<TH>Quality</TH>\n");
	printf ("<TH>Weight</TH>\n");
	printf ("<TH>Res</TH>\n");
	printf ("<TH>Dist</TH>\n");
	printf ("<TH>Md Mag</TH>\n");
	printf ("<TH>Delete Mag?</TH>\n");
	printf ("</TR>\n");

	for (i = 0; i < pEvent->iNumChans; i++ )
	{

		psta = &pEvent->pChanInfo[i].Station;

		if (pEvent->pChanInfo[i].iNumArrivals > 0) 
		{
			/* Put up the hidden field denoting our SCN */
			printf ("<INPUT TYPE=hidden NAME=\"scn\" VALUE=\"%s_%s_%s_\">\n",
					psta->Sta, psta->Comp, psta->Net);
		}

		for (j = 0; j < pEvent->pChanInfo[i].iNumArrivals; j++)
		{
			par = &pEvent->pChanInfo[i].Arrivals[j];
			pstamag = &pEvent->pChanInfo[i].Stamags[j];
	
			printf ("<TR align=center>\n");

			/* Put up the delete pick checkbox */
			printf ("<TD align=center> <INPUT NAME=\"DelPick%d\" TYPE=CHECKBOX> </TD>\n", j);


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
					if (Retrieve3Components (sacfile, WebRevDir, pEvent, i) 
																!= EW_SUCCESS)
					{
						html_logit ("", "Call to Retrieve3Components failed! EXITTING!\n");
						exit(-1);
					}

					BuildAppletString (lomax_str, pEvent, WebHost, sacfile);
	
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

				/* Modifiable pick quality field */
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
	
				/* pick time and modifiable weight */
				printf ("<TD WIDTH=\"200\">%s</TD>\n", timestr);
				printf ("<TD><INPUT NAME=\"Wt%d\" TYPE=text SIZE=3 MAXLENGTH=1 VALUE=\"%d\"></TD>\n",
												j, wt);
				printf("<TD>%3.2f</TD>\n", par->dWeight);
				printf("<TD>%6.2f</TD>\n", par->tResPick);
				printf("<TD>%5.f</TD>\n", par->dDist);
			}
			else
			{
				printf("<TD>%s</TD>\n", " ");
				printf("<TD>%c</TD>\n", ' ');
				printf ("<TD WIDTH=\"200\">%s</TD>\n", " ");
				printf ("<TD><INPUT NAME=\"Wt%d\" TYPE=text SIZE=3 MAXLENGTH=1 VALUE=\"0\"></TD>\n",
												j);
				printf("<TD>%s</TD>\n", " ");
				printf("<TD>%s</TD>\n", " ");
				printf("<TD>%s</TD>\n", " ");
	
			}
	
			if (pstamag != NULL)
			{
				/* Md value */
				if(pstamag->iMagType == MAGTYPE_DURATION) 
				{
					if ((pstamag->dMag == 0.0) || (pstamag->dWeight >= 4))
						printf ("<TD>%5s</TD>\n", "---");
					else
						printf ("<TD>%5.1f</TD>\n", pstamag->dMag);
        		}
				else
					printf ("<TD>%5s</TD>\n", "---");
			}
			else
			{
				printf ("<TD>%5s</TD>\n", "---");
			}
	
			/* Put up the delete mag checkbox */
			printf ("<TD align=center> <INPUT NAME=\"DelMagn%d\" TYPE=CHECKBOX> </TD>\n", j);
	
			printf ("\n");
			printf ("</TR>\n");

		} /* loop over arrivals */

	} /* loop over channels */
	printf("</TABLE>\n");
	printf ("<BR>\n");
	printf ("<BR>\n");



	/*******************************
     *  Amplitude picks section 
     ******************************/

	printf ("<HR><PRE><B>AMPLITUDE PICKS</B>\n\n");

	/* 
 	 * Go through the LMS array and build an applet string
	 */

	if (NumAmplitudePicksToShow > 0)
	{
		NumFiles = 0;
		NumStas = 0;
		for (i = 0; ((i < NumLMS) && (NumStas < NumAmplitudePicksToShow)); i++)
		{
			if (LMS[i].valid != -1)
			{
				/* Check for waveforms */
				if (LMS[i].GotWave1 == TRUE)
				{
					if (NumFiles == 0)
					{
						sprintf (sacfile, "'%s/%s.%s.%s.wa#%d'", WebRevDir,
								LMS[i].Sta, LMS[i].Comp1, LMS[i].Net, NumFiles);
					}
					else
					{
						sprintf (tmpstr, ", '%s/%s.%s.%s.wa#%d'", WebRevDir,
								LMS[i].Sta, LMS[i].Comp1, LMS[i].Net, NumFiles);
						strcat (sacfile, tmpstr);
					}
	
					NumFiles = NumFiles + 1;
				}

				if (LMS[i].GotWave2 == TRUE)
				{
					if (NumFiles == 0)
					{
						sprintf (sacfile, "'%s/%s.%s.%s.wa#%d'", WebRevDir,
								LMS[i].Sta, LMS[i].Comp2, LMS[i].Net, NumFiles);
					}
					else
					{
						sprintf (tmpstr, ", '%s/%s.%s.%s.wa#%d'", WebRevDir,
								LMS[i].Sta, LMS[i].Comp2, LMS[i].Net, NumFiles);
						strcat (sacfile, tmpstr);
					}
	
					NumFiles = NumFiles + 1;
				}

				NumStas = NumStas + 1;
			}
		}

	
		if (NumFiles != 0)
		{
			/* build the rest of the lomax string */
			BuildAppletString (lomax_str, pEvent, WebHost, sacfile);
	
			/* display the link */
			printf ("<A HREF=\"%s\">Click here to review %d closest stations.</A>\n",
							lomax_str, NumStas);
		}
		else
		{
			/* display our regrets that there are no traces to review */
			logit ("", "No waveforms to display -- skipping multi-channel ML applet.\n");
		}
	}
	printf ("<BR><BR><BR><BR><BR>\n");

	printf ("Click on the Station name to graphically review amplitude picks.\n");
	printf ("</PRE></CENTER>\n");

	printf ("<BR><BR>\n");
	printf("<CENTER>\n");	
	printf ("<TABLE border=2 cellspacing=2>\n");
	printf ("<TR align=center>\n");
	printf ("<TH>Station</TH>\n");
	printf ("<TH>Dist</TH>\n");
	printf ("<TH>Method</TH>\n");
	printf ("<TH>ML Mag</TH>\n");
	printf ("</TR>\n");

	for (i = 0; i < NumLMS; i++ )
	{
		if (LMS[i].valid != -1)
		{
			printf ("<TR align=center>\n");

			if ((LMS[i].GotWave1 == TRUE) && (LMS[i].GotWave2 == TRUE))
			{
				sprintf (sacfile, "'%s/%s.%s.%s.wa#0','%s/%s.%s.%s.wa#1' ", 
						WebRevDir, LMS[i].Sta, LMS[i].Comp1, LMS[i].Net,
						WebRevDir, LMS[i].Sta, LMS[i].Comp2, LMS[i].Net);

				ChanNameMode = 1;
			}
			else if (LMS[i].GotWave1 == TRUE)
			{
				sprintf (sacfile, "'%s/%s.%s.%s.wa'", 
						WebRevDir, LMS[i].Sta, LMS[i].Comp1, LMS[i].Net);
				ChanNameMode = 1;
			}
			else if (LMS[i].GotWave2 == TRUE)
			{
				sprintf (sacfile, "'%s/%s.%s.%s.wa'", 
						WebRevDir, LMS[i].Sta, LMS[i].Comp2, LMS[i].Net);
				ChanNameMode = 1;
			}
			else
				ChanNameMode = 0;

					
			if (ChanNameMode != 0)
			{
				BuildAppletString (lomax_str, pEvent, WebHost, sacfile);
				printf ("<TD WIDTH=\"100\"><A HREF=\"%s\">%-5s %3s %4s</A></TD>\n", 
								lomax_str, LMS[i].Sta, LMS[i].CommonComp, LMS[i].Net);
			}
			else
				printf ("<TD WIDTH=\"100\">%-5s %3s %4s</TD>\n", 
										 LMS[i].Sta, LMS[i].CommonComp, LMS[i].Net);

		
			printf("<TD WIDTH=\"100\">%d</TD>\n", (int) LMS[i].Dist);

			printf("<TD WIDTH=\"100\">%s</TD>\n", LMS[i].Method);

			printf ("<TD WIDTH=\"100\">%5.2f</TD>\n", LMS[i].MagAvg);

			printf ("</TR>\n");
	
		}

	} /* loop over LMS array entries */

	printf("</TABLE>\n");
	printf ("<BR><BR><HR><BR>\n");


	/*******************************
     *  Unpicked snippets section
     ******************************/
	if (STATE_Unpicked == TRUE)
	{
		/* Ze plot: first we fill in an array of entries which make up the table.
		Then we sort. Then we issue the HTML. Alex 3/14/1*/
		int icol;
		int nEntries=0;		/* number of entries in the table */
		int NCOL=4;				/* number of entries accross page */
		int MAXSNIPPETS = 500;	/* max snippets we can deal with */

		/* Fill the 'entries' structures of the unpicked snippet table
		**************************************************************/
		for (i=0; i<pEvent->iNumChans; i++)							/* over channels */
		{
			if(pEvent->pChanInfo[i].iNumArrivals != 0 ) continue;	/* only the unpicked channels, please */

			for(j=0; j<pEvent->pChanInfo[i].iNumWaveforms; j++)		/* over snippets from this channel */
			{
				if (Retrieve3Components (sacfile, WebRevDir, pEvent, i)	!= EW_SUCCESS)
				{
					html_logit ("", "Call to Retrieve3Components failed! EXITTING!\n");
					exit(-1);
				}
				psta = &pEvent->pChanInfo[i].Station;

				/* stuff into entry table */
				strcpy(entry[nEntries].sta, psta->Sta);
				strcpy(entry[nEntries].comp, psta->Comp);
				strcpy(entry[nEntries].net, psta->Net);
				strcpy(entry[nEntries].sacfile, sacfile);

				/* compute distance and bearing */
				if( geo_to_km(pEvent->PrefOrigin.dLat,         pEvent->PrefOrigin.dLon,
							  pEvent->pChanInfo[i].Station.Lat,pEvent->pChanInfo[i].Station.Lon,
							  &range,                          &bearing) != 1)
				{
					html_logit ("", "Call to geo_to_km() failed.\n");
				}

				/* and stuff into entry table */
				entry[nEntries].range = range;
				entry[nEntries].bearing = bearing;

				nEntries++;
				if(nEntries == MAXSNIPPETS) 
				{
					html_logit ("", "WARNING: Unpicket snippet list truncated.\n");
					goto listFull;
				}
			}					/* over snippets from this channel */
		}					/* over channels */
listFull:
		/* Sort the list
		****************/
		qsort(entry, nEntries, sizeof(ITEM), compRange);

		printf("<CENTER><PRE>\n");
		printf("<HR><B>UNPICKED SNIPPETS\n");

		/*** Put up a link allowing multi-channel Lomax review ***/

		/* 
		 * For each channel, if it contains an arrival pick AND
		 * has waveforms to display, add it to the list of files to review. 
 		 * Do this up to NumArrivalPicksToShow
		 */
		if (NumArrivalPicksToShow > 0)
		{
	
			NumFiles = 0;
			for (i = 0; ((i < nEntries) && (NumFiles < NumArrivalPicksToShow)); i++)
			{
		
				if (NumFiles == 0)
				{
					sprintf (sacfile, "'%s/%s.%s.%s#%d'", WebRevDir,
							entry[i].sta, entry[i].comp, entry[i].net, NumFiles);
				}
				else
				{
					sprintf (tmpstr, ", '%s/%s.%s.%s#%d'", WebRevDir,
							entry[i].sta, entry[i].comp, entry[i].net, NumFiles);
					strcat (sacfile, tmpstr);
				}
		
				NumFiles = NumFiles + 1;
			}
		
		
			if (NumFiles != 0)
			{
				/* build the rest of the lomax string */
				BuildAppletString (lomax_str, pEvent, WebHost, sacfile);
		
				/* display the link */
				printf ("<A HREF=\"%s\">Click here to pick %d closest channels.</A>\n\n",
								lomax_str, NumFiles);
			}
			else
			{
				/* display our regrets that there are no traces to review */
				logit ("", "No waveforms to display -- skipping multi-channel applet.\n");
			}

		} /* multi-channel display */


		/* put out the unpicked snippet table
		*************************************/
		printf ("</PRE><TABLE border=2 cellspacing=2>\n");
		printf ("<TR align=center>\n");
		for(icol=0;icol<NCOL;icol++)
		{
			printf ("<TH>Channel</TH>\n");
			printf ("<TH>Dist</TH>\n");
			printf ("<TH>Azm</TH>\n");
		}
		printf ("</TR>\n");
		icol=0;
		
		for(i=0;i<nEntries;i++)
		{
			BuildAppletString (lomax_str, pEvent, WebHost, entry[i].sacfile);

			printf ("<TD><A HREF=\"%s\">%-5s %3s %4s</A></TD>\n", 
					lomax_str, entry[i].sta, entry[i].comp, entry[i].net);

			printf("<TD>%5.f</TD>\n", entry[i].range);
			printf("<TD>%3.f</TD>\n", entry[i].bearing);
			icol++;
			printf ("\n");
			if(icol==NCOL)
			{
				printf ("</TR>\n");
				icol=0;
			}
		}
		printf("</TABLE>\n");

	/* End of 'pick the unpicked' mod: show table of snippets without any picks
	*****************************************************************************/
	}

	printf ("<BR>\n");
	printf ("<BR>\n");
	printf("<CENTER>\n");


	printf ("<TABLE>\n");


	printf ("<INPUT NAME=EventID TYPE=HIDDEN VALUE=\"%u\">\n", pEvent->Event.idEvent);
	printf ("<INPUT NAME=OriginID TYPE=HIDDEN VALUE=\"%u\">\n", STATE_idOrigin);
	printf ("<INPUT NAME=Map TYPE=HIDDEN VALUE=\"%u\">\n", STATE_Map);
	printf ("<INPUT NAME=Unpicked TYPE=HIDDEN VALUE=\"%u\">\n", STATE_Unpicked);

	/* first row */
	printf ("<TR ALIGN=center>\n");
	printf ("<TD>\n");
	printf ("<INPUT TYPE=\"submit\" VALUE=\"Refresh\" NAME=\"Refresh\">\n");
	printf ("</TD>\n");

	if (STATE_Unpicked == FALSE)
	{
		printf ("<TD>\n");
		printf ("<INPUT TYPE=\"submit\" VALUE=\"Retrieve Unpicked\" NAME=\"GetUnpicked\">\n");
		printf ("</TD>\n");
	}

	printf ("</TR>\n");

	/* second row */
	printf ("<TR ALIGN=center>\n");

	printf ("<TD>\n");
	printf ("<INPUT TYPE=\"submit\" VALUE=\"Cancel\" NAME=\"Cancel\">\n");
	printf ("</TD>\n");


	printf ("<TD>\n");
	printf ("<INPUT TYPE=\"submit\" VALUE=\"Done\" NAME=\"Done\">\n"); 
	printf ("</TD>\n");

	printf ("</TR ALIGN=center>\n");


	printf("</CENTER></TABLE></FORM>\n");
	printf("</pre>\n");

	free (sacfile);

}  /* End html_review_form() */



/*******************************************************
*
* Retrieve3Components (): for a given S-C-N, make sure 
*  that all three components (Z,N,E) exist as 
*  SAC files. 
*
*   Algorithm: 
*  - is the component one of Z,E,N?
*  - search through the other channels, looking for
*	 the other two components
*  - produce the list of filenames to pass to the Lomax applet.
*    Note that the order of components MUST be Z, E, N.
*  - if 3 components are not present, return only the current
*     component
*
*******************************************************/
static int		Retrieve3Components (char *sacfiles, char *WebRevDir,  
			EWEventInfoStruct *pEvent, int ChanNum)
{

	int		i;
	int		GotComp1, GotComp2;
	char	getsta[10];
	char	getcomp1[10];
	char	getcomp2[10];
	char	getnet[10];
  char  *z;
  char  *e;
  char  *n;
	EWDB_StationStruct	*psta;  


	if ((sacfiles == NULL) || (WebRevDir == NULL) || 
						(pEvent == NULL) || (ChanNum < 0))
	{
		logit ("", "Invalid arguments passed in.\n");
		return EW_FAILURE;
	}
		

  /*
   * NOTE: the order of components in a 3-component display
   *  is relevant to a seismologist. The vertical comes first,
   *  followed by E and N horizontals.
   */

	psta = &pEvent->pChanInfo[ChanNum].Station; 

	/* Produce the output for the current chan */
	sprintf (sacfiles, "'%s/%s.%s.%s'", WebRevDir,
						psta->Sta, psta->Comp, psta->Net); 

	/* is the components Z,N,E */
	getcomp1[0] = '\0';
	getcomp2[0] = '\0';
	if (psta->Comp[2] == 'Z')
	{
		strcpy (getsta, psta->Sta);
		strcpy (getcomp1, psta->Comp);
		strcpy (getcomp2, psta->Comp);
		strcpy (getnet, psta->Net);

		getcomp1[2] = 'N';
		getcomp2[2] = 'E';
    z = psta->Comp;
    e = getcomp2;
    n = getcomp1;
	}
	else if (psta->Comp[2] == 'N')
	{
		strcpy (getsta, psta->Sta);
		strcpy (getcomp1, psta->Comp);
		strcpy (getcomp2, psta->Comp);
		strcpy (getnet, psta->Net);

		getcomp1[2] = 'Z';
		getcomp2[2] = 'E';
    z = getcomp1;
    e = getcomp2;
    n = psta->Comp;
	}
	if (psta->Comp[2] == 'E')
	{
		strcpy (getsta, psta->Sta);
		strcpy (getcomp1, psta->Comp);
		strcpy (getcomp2, psta->Comp);
		strcpy (getnet, psta->Net);

		getcomp1[2] = 'N';
		getcomp2[2] = 'Z';
    z = getcomp2;
    e = psta->Comp;
    n = getcomp1;
	}


	if (getcomp1[0] != '\0')
	{

		GotComp1 = FALSE;
		GotComp2 = FALSE;
		for (i = 0; i < pEvent->iNumChans; i++)
		{
			if (pEvent->pChanInfo[i].iNumWaveforms > 0)
			{
				psta = &pEvent->pChanInfo[i].Station; 

				if ((strcmp (psta->Sta, getsta) == 0) &&
					(strcmp (psta->Comp, getcomp1) == 0) &&
					(strcmp (psta->Net, getnet) == 0))
				{
					GotComp1 = TRUE;
				}
				else if ((strcmp (psta->Sta, getsta) == 0) &&
					(strcmp (psta->Comp, getcomp2) == 0) &&
					(strcmp (psta->Net, getnet) == 0))
				{
					GotComp2 = TRUE;
				}

			} /* are there any snippets */

		} /* loop over chans */
		
    if ((GotComp1 == TRUE) && (GotComp2 == TRUE))
    {
      /* we have all three components - yipeeeee */
      /* create the list of sacfiles */
      sprintf (sacfiles, " '%s/%s.%s.%s#0', '%s/%s.%s.%s#1', '%s/%s.%s.%s#2'",
            WebRevDir, getsta, z, getnet,
            WebRevDir, getsta, e, getnet,
            WebRevDir, getsta, n, getnet);
    }

  } /* the current station is either Z,N,E component */

  return EW_SUCCESS;

}

int compRange( const void* r1, const void* r2)
{
	ITEM* e1 = (ITEM*)r1;
	ITEM* e2 = (ITEM*)r2;
	
	if( e1->range  < e2->range) return(-1);
	if( e1->range == e2->range) return(0);
	return(1);
}



void	BuildAppletString (char *OutString, EWEventInfoStruct *pEvent, 
										char *webhost, char *channelnames)
{
 
	char	tmpstr[MAX_SG2K_STR_LEN];
	char	optstr[MAX_SG2K_STR_LEN];
	int		i;
	EWDBid	tgtid;

	if ((OutString == NULL) || (pEvent == NULL) || (webhost == NULL) ||
						(channelnames == NULL))
	{
		logit ("", "BuildAppletString got invalid args -- returning NULL string.\n");
	}
	else
	{

		if (pEvent->GotLocation == TRUE)
			tgtid = pEvent->PrefOrigin.idOrigin;
		else
		{
			if (pEvent->GotTrigger == TRUE)
				tgtid = pEvent->CoincEvt.idCoincidence;
			else
			{
				logit ("", "Must have either Coincidence or Solution!\n");
			}
		}

		sprintf (tmpstr, "javascript:void seis_view_n_03("
				"'seisview', 'Event %d', null, [%s], null, null, "
				"'SAC_BINARY', 'SUN_UNIX', 'EARTHWORM', ' ',"
				"[  ['event.id', %d],"
				   "['event.info', 'Info'],"
   				   "['event.url', 'http://%s/earthworm/cgi-bin/retrieve_pick']," 
				   "['event.protocol', 'CGI_BIN_EARTHWORM'],"
				   "['event.sendall', 'YES'],"
				   "['channel.response.ext', 'pz']",
						 tgtid, channelnames, tgtid, webhost);

		/* Add optional parameters */
		for (i = 0; i < NumSeisGramOptions; i++)
		{
			sprintf (optstr, ",['%s', '%s']", 
							SeisGramOptions[i].param,
							SeisGramOptions[i].value);
	
			strcat (tmpstr, optstr);
		}

		sprintf (OutString, "%s])", tmpstr);
	}

}

int compChanDist (const void *c1, const void *c2)
{
	double 	dist1, dist2;
	double 	azm1, azm2;
	EWChannelDataStruct *s1 = (EWChannelDataStruct *)c1;
	EWChannelDataStruct *s2 = (EWChannelDataStruct *)c2;


	if (geo_to_km (SortOrigin.dLat, SortOrigin.dLon,
					s1->Station.Lat, s1->Station.Lon,
					&dist1, &azm1) != 1)
	{
		logit ("", "Call to geo_to_km failed for item 1.\n");
		return (0);
	}

	if (geo_to_km (SortOrigin.dLat, SortOrigin.dLon,
					s2->Station.Lat, s2->Station.Lon,
					&dist2, &azm2) != 1)
	{
		logit ("", "Call to geo_to_km failed for item 2.\n");
		return (0);
	}

	if (dist1  < dist2) 
		return(-1);
	if (dist1 == dist2) 
		return(0);
	return(1);

}


/*******************************************************
*
* BuildLocalMagArray (): Given the Event structure,
*  build an array of structures which group together
*  two horizonatal components for each station 
*  where MLs were calculated.
*  The two MLs are then averaged and the applet
*  invocation string is computed for later use.
*
*******************************************************/
static int		BuildLocalMagArray (EWEventInfoStruct *pEvent, 
							LocalMagStas *LMS, int LMS_len, int *NumStasBuilt)
{

	int		i, k, filled, found;
	EWDB_StationMagStruct 	*pstamag;
	double	dist, azm;
	char		common[10];
	
	if ((pEvent == NULL) || (LMS == NULL))
	{
		logit ("", "Invalid arguments passed in.\n");
		return EW_FAILURE;
	}


	for (i = 0; i < LMS_len; i++)
	{
		LMS[i].valid = -1;
		LMS[i].GotWave1 = FALSE;
		LMS[i].GotWave2 = FALSE;
        sprintf (LMS[i].Sta, "");
        sprintf (LMS[i].Net, "");
        sprintf (LMS[i].Comp1, "");
        sprintf (LMS[i].Comp2, "");
	}

	filled = 0;
	for (i = 0; i < pEvent->iNumChans; i++)
	{
		if (pEvent->pChanInfo[i].iNumStaMags > 0)
		{
			pstamag = &pEvent->pChanInfo[i].Stamags[0];

			if (pstamag->iMagType == LM_LocalMagType)
			{
				/* Find this station */
				found = -1;
				strncpy (common, pEvent->pChanInfo[i].Station.Comp, 2);
				common[2] = '\0';

				for (k = 0; ((k < filled) && (found == -1)); k++)
				{
					if ((strcmp (common, LMS[k].CommonComp) == 0) && 
							(strcmp (pEvent->pChanInfo[i].Station.Sta, LMS[k].Sta) == 0))
						found = k;
				}

				if (found != -1)
				{
					/* This is the second entry for this station */
					LMS[found].Mag2 = pstamag->dMag;
					LMS[found].valid = 1;
					LMS[found].MagAvg = (LMS[found].Mag1 +  LMS[found].Mag2) / 2.0;

					strcpy (LMS[found].Comp2, pEvent->pChanInfo[i].Station.Comp);

					if (pEvent->pChanInfo[i].iNumWaveforms > 0)
							LMS[found].GotWave2 = TRUE;
				}
				else
				{
					/* Add a new station if we have room */
					if (filled < LMS_len)
					{
						LMS[filled].valid = 0;
						LMS[filled].MagAvg = pstamag->dMag;
						LMS[filled].Mag1 = pstamag->dMag;

						if (geo_to_km (pEvent->PrefOrigin.dLat, pEvent->PrefOrigin.dLon,
								pEvent->pChanInfo[i].Station.Lat,
								pEvent->pChanInfo[i].Station.Lon, &dist, &azm) != 1)
						{
							logit ("", "Call to geo_to_km() failed.\n");
							return EW_FAILURE;
						}

						LMS[filled].Dist = dist;

						strcpy (LMS[filled].Sta, pEvent->pChanInfo[i].Station.Sta);
						strcpy (LMS[filled].Net, pEvent->pChanInfo[i].Station.Net);
						strcpy (LMS[filled].Comp1, pEvent->pChanInfo[i].Station.Comp);
						strncpy (LMS[filled].CommonComp, pEvent->pChanInfo[i].Station.Comp, 2);
						LMS[filled].CommonComp[2] = '\0';

						strcpy (LMS[filled].Method, LM_method);

						if (pEvent->pChanInfo[i].iNumWaveforms > 0)
							LMS[filled].GotWave1 = TRUE;

						filled = filled + 1;
					}
				}
			}
		}
	}

	/* sort by distance */
	qsort(LMS, filled, sizeof (LocalMagStas), LMS_SortByDist);

	*NumStasBuilt = filled;
	return EW_SUCCESS;

}


int LMS_SortByDist( const void* r1, const void* r2)
{
	LocalMagStas *e1 = (LocalMagStas *)r1;
	LocalMagStas *e2 = (LocalMagStas *)r2;
	
	if (e1->Dist  < e2->Dist) return(-1);
	if (e1->Dist == e2->Dist) return(0);
	return(1);
}


/*******************************************************
*
* PopulateReviewDir (): Given the Event structure,
*  and the name of the event directory,
*  fill the event directory with the event info, such as   
*  the binary file containing the Event structure, as well
*  as the SAC files of tracedata.
*
*    If Method flag is set to POPULATE_UH, the SAC headers
*    will be filled with the external information for 
*    urban hazards, and SAC traces of ground motion will 
*    be written.
* 
*    If Method is POPULATE_REVIEW_PICKED, write out only  
*    those traces that have amplitude or arrival picks
*    associated with them. If POPULATE_REVIEW_UNPICKED 
*    is selected, write out unpicked traces.
*
*******************************************************/
int		PopulateReviewDir (EWEventInfoStruct *pEvent, 
								char *evtDir, int Method)
{
	char				evtFile[MAXPATH];
	char				filename[1000];
	char				cmd[1000];
	FILE				*fp;
	SAC_OriginStruct	SacOrig;
	SAC_ExtChanStruct	SacExtChan;
	EWDB_External_UH_InfraInfo	*pUHInfo;
	EWDB_WaveformStruct	*pWaveform;
	int					i, idChanT, NumFound, NumRetr, GetIt;
	long				SacDataLen, MaxLen;
	char				*binSnippet;


	if ((pEvent == NULL) || (evtDir == NULL))
	{
		html_logit ("", "Invalid arguments passed in.\n");
		return EW_FAILURE;
	}

	if (Method == POPULATE_UH)
	{
		if ((pUHInfo = (EWDB_External_UH_InfraInfo *) 
				malloc (1 * sizeof (EWDB_External_UH_InfraInfo))) == NULL)
		{
			logit ("", "Could not malloc space for UH external info.\n");
			return EW_FAILURE;
		}
	}

	
	if (Method != POPULATE_REVIEW_UNPICKED)
	{
		sprintf (evtFile, "%s%cEvtStruct.bin", evtDir, DIR_SLASH);
	

		/* Write the event struct to the binary file */
			
		if ((fp = fopen (evtFile, "wb")) == NULL)
		{
			html_logit ("", "can't open %s\n", evtFile);
			return EW_FAILURE;
		}
	
		fwrite (pEvent, sizeof (EWEventInfoStruct), 1, fp);
	
		/* append channel info structs */
	
		fwrite ((void *) pEvent->pChanInfo, sizeof (EWChannelDataStruct), 
															pEvent->iNumChans, fp);
		fclose (fp);
	}


	/* Find the longest snippet we will deal with */
	MaxLen = 0;
	for (i = 0; i < pEvent->iNumChans; i++)
	{
		if (pEvent->pChanInfo[i].iNumWaveforms > 0)
		{
			if (pEvent->pChanInfo[i].Waveforms[0].iByteLen > MaxLen)
				MaxLen = pEvent->pChanInfo[i].Waveforms[0].iByteLen;
		}
	}

	MaxLen = MaxLen + 1000; /* Just to be safe */
	if ((binSnippet = malloc (MaxLen)) == NULL)
	{
		logit ("", "Could not malloc snippet buffer of %d bytes.\n", MaxLen);
		return EW_FAILURE;
	}


	/* Insist on a  minimum sac snippet length */
	SacDataLen = MAX_SNIPPET_LEN + SACHEADERSIZE;

	if (SacBufferLen > SacDataLen) 
		SacDataLen = SacBufferLen;

	if (Method != POPULATE_REVIEW_UNPICKED)
	{
		if (SACPABase_init (SacDataLen, evtDir, TRUE, SacFormat) != EW_SUCCESS)
		{
			html_logit ("", "Call to SACPABase_init failed!\n");
			return EW_FAILURE;
		}
	}
	else
	{
		if (SACPABase_init (SacDataLen, evtDir, FALSE, SacFormat) != EW_SUCCESS)
		{
			html_logit ("", "Call to SACPABase_init failed!\n");
			return EW_FAILURE;
		}
	}


	/* Retrieve SAC data */

	SacOrig.dLat = pEvent->PrefOrigin.dLat;
	SacOrig.dLon = pEvent->PrefOrigin.dLon;
	SacOrig.dElev = pEvent->PrefOrigin.dDepth;
	SacOrig.tOrigin = pEvent->PrefOrigin.tOrigin;

	if (SACPABase_next_ev_review (evtDir, 
					pEvent->Event.idEvent, &SacOrig) != EW_SUCCESS)
	{
		html_logit ("", "Call to SACPABase_next_ev_review failed!\n");
		return EW_FAILURE;
	}

	for (i = 0; i < pEvent->iNumChans; i++)
	{
		GetIt = FALSE;
		if (Method == POPULATE_UH)
		{
			if (pEvent->pChanInfo[i].iNumWaveforms > 0)
			{
				GetIt = TRUE;
			}
		}
		else if (Method == POPULATE_REVIEW_PICKED)
		{
			if ((pEvent->pChanInfo[i].iNumWaveforms > 0) &&
					(pEvent->pChanInfo[i].iNumStaMags > 0))
			{
				GetIt = TRUE;
			}
		}
		else if (Method == POPULATE_REVIEW_UNPICKED)
		{
			if ((pEvent->pChanInfo[i].iNumWaveforms > 0) &&
					(pEvent->pChanInfo[i].iNumStaMags <= 0))
			{
				GetIt = TRUE;
			}
		}
		else
		{
			logit ("", "Invalid method %d\n", Method);
			return EW_FAILURE;
		}


		if (GetIt == TRUE)
		{
			/* Retrieve snippets for this channel */
			pWaveform = &(pEvent->pChanInfo[i].Waveforms[0]);

			pWaveform->binSnippet = binSnippet;

			if (ewdb_api_GetWaveformSnippet (pWaveform->idWaveform, 
							pWaveform->binSnippet, pWaveform->iByteLen) 
														!= EWDB_RETURN_SUCCESS)
			{
				logit ("", "Call to ewdb_api_GetWaveformSnippet failed for Chan %d\n", i);
				continue;
			}


			GapCount = 0;
			if (ProduceSAC_NextStationForEvent 
							(&(pEvent->pChanInfo[i])) != EW_SUCCESS)
			{
				logit ("", "%s.%s.%s: "
					"Call to ProduceSAC_NextStationForEvent failed; continuing.\n",
					pEvent->pChanInfo[i].Station.Sta, 
					pEvent->pChanInfo[i].Station.Comp, 
					pEvent->pChanInfo[i].Station.Net);

				pEvent->pChanInfo[i].iNumWaveforms = 0;
				continue;
			}

			/* 
			 * Check for gaps: if this is a horizontal component and
			 * our data has gaps, don't write it out because we don't
 			 * want localmag to see it.
			 */

			if ((pEvent->pChanInfo[i].Station.Comp[2] == 'E') ||
					(pEvent->pChanInfo[i].Station.Comp[2] == 'N'))
			{
				if (GapCount > 0)
				{
					logit ("", "%s.%s.%s: GapCount is %d; skipping.\n",
							pEvent->pChanInfo[i].Station.Sta, 
							pEvent->pChanInfo[i].Station.Comp, 
							pEvent->pChanInfo[i].Station.Net, GapCount);

					pEvent->pChanInfo[i].iNumWaveforms = 0;
					continue;
				}
			}


			/* Fill in urban hazards external info */
			if (Method == POPULATE_UH)
			{

				idChanT = -1;
				if (ewdb_api_GetidChanT (pEvent->pChanInfo[i].Station.idChan,
		                            0, &idChanT) != EWDB_RETURN_SUCCESS)
				{
					logit ("e", "Call to ewdb_api_GetidChanT failed.\n");
					goto close_scn;
				}
	
				if (idChanT < 0)
				{
					logit ("e", "ewdb_api_GetidChanT returned bad idChanT: %d.\n",
											idChanT);
					goto close_scn;
				}



				if (ewdb_api_UH_GetExternalInfraRecord (idChanT, 
						pUHInfo, 1, &NumFound, &NumRetr) != EWDB_RETURN_SUCCESS)
				{
					logit ("", "Call to ewdb_api_UH_GetExternalInfraRecord failed.\n");
					goto close_scn;
				}

				if (NumFound != NumRetr)
				{
					logit ("", "Warning: UH Info retrieved %d out of %d\n",
							NumRetr, NumFound);
				}

				if (NumRetr <= 0)
				{
/*
					logit ("", "ewdb_api_UH_GetExternalInfraRecord "
								"returned no records out of %d\n", 
											NumFound);
*/
					goto close_scn;
				}


				if (pUHInfo[0].idUHInfo < 0)
				{
					logit ("e", "ewdb_api_UH_GetExternalInfraRecord "
								"returned bad idUHInfo: %d.\n",
											pUHInfo[0].idUHInfo);
					goto close_scn;
				}


				SacExtChan.dNaturalFrequency = pUHInfo[0].dNaturalFrequency;
				SacExtChan.dDamping = pUHInfo[0].dDamping;
				SacExtChan.dFullscale = pUHInfo[0].dFullscale;
				SacExtChan.dSensitivity = pUHInfo[0].dSensitivity;
				SacExtChan.dAzm = pUHInfo[0].dAzm;
				SacExtChan.dDip = pUHInfo[0].dDip;
				SacExtChan.iGain =  pUHInfo[0].iGain;
				SacExtChan.iSensorType = pUHInfo[0].iSensorType;

				SACPABase_write_extinfo (&SacExtChan);

			}


close_scn:
			/* Save the SAC data */
			if (SACPABase_end_scn () != EW_SUCCESS)
			{
				html_logit ("", "Call to SACPABase_end_scn failed.\n");
				return EW_FAILURE;
			}

			pWaveform->binSnippet = NULL;

		} /* Do we have any snippets to save */

	} /* for Loop over channels */


	/*
	 * Get WA sac files: we need to execute localmag
	 * which produces SAC files of WA synthetic traces
	 * that will be passed on the the SeisGram2K applet
	 * for review of amplitude picks.
	 */
	

#ifdef _WINNT

	sprintf (cmd, "\\earthworm\\web\\bin\\nt_getWA %s %s %s", 
				evtDir, LM_progname, LM_writeWA_configfile);

	fflush (NULL);
#else
	putenv ("EW_LOG=./");
	sprintf (cmd, "cd %s; %s %s > /dev/null 2>&1", evtDir, LM_progname, 
						LM_writeWA_configfile);

#endif _WINNT
	system (cmd);

#ifndef _WINNT
	/* reset the log directory */
	putenv (envEW_LOG);
#endif

	/* Write the original message Author (Logo) to file */
	sprintf (filename, "%s%cAuthor", evtDir, DIR_SLASH);
	if ((fp = fopen (filename, "wt")) == NULL)
	{
		html_logit ("", "can't open %s\n", filename);
		return EW_FAILURE;
	}

	if (pEvent->GotLocation == TRUE)
		fprintf (fp, "%s", pEvent->PrefOrigin.sRealSource);
	else if (pEvent->GotTrigger == TRUE)
		fprintf (fp, "%s", pEvent->CoincEvt.szSource);

	fclose (fp);

	free (binSnippet);


	return EW_SUCCESS;

}

