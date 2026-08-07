
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: retrieve_pick.c,v 1.16 2001/08/10 21:04:51 lucky Exp $
 *    Revision history:
 *
 *    $Log: retrieve_pick.c,v $
 *    Revision 1.16  2001/08/10 21:04:51  lucky
 *    NT review and display partially implemented
 *
 *    Revision 1.15  2001/08/07 16:53:49  lucky
 *    Pre v6.0 checkin
 *
 *    Revision 1.14  2001/07/31 20:47:20  lucky
 *    Changed the way that picks are saved. We really only care about the
 *    last action for each pick type for each SCN. So, we over-write whatever
 *    we received about that pick-SCN combination before and worry abotu the
 *    last step.
 *
 *    Revision 1.13  2001/07/28 00:45:23  lucky
 *     State of the code after debugging and testing prior to v6.0 release freeze.
 *
 *    Revision 1.12  2001/07/01 21:55:30  davidk
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
 *    Revision 1.11  2001/06/27 18:15:53  lucky
 *    Added two amp picks label strings.
 *
 *    Revision 1.10  2001/06/21 21:26:25  lucky
 *    State of the code after LocalMag review was implemented and at least partially
 *    tested. The new Lomax applet (june 18 version) has been incorporated. ML's can
 *    be added, deleted, modified, as well as the other types of picks.
 *
 *    Revision 1.9  2001/05/15 18:47:38  davidk
 *    Moved functions around between the apps, DB API, and DB API INTERNAL
 *    levels.  Renamed functions and files.  Added support for amplitude
 *    magnitude types.  Reformatted makefiles.
 *
 *    Revision 1.8  2001/02/28 17:30:23  lucky
 *    Massive schema redesign and cleanup.
 *
 *    Revision 1.7  2000/12/06 17:49:44  lucky
 *    *** empty log message ***
 *
 *    Revision 1.6  2000/09/21 16:59:22  lucky
 *    Added logit message which are printed only in DEBUG mode -- clean up
 *    debugging output.
 *
 *    Revision 1.5  2000/09/20 19:42:49  lucky
 *    Clean up logit messages
 *
 *    Revision 1.4  2000/09/18 17:25:00  lucky
 *    Final version before v5.1
 *
 *    Revision 1.3  2000/08/09 16:58:31  lucky
 *    Lint Cleanup
 *
 *    Revision 1.2  2000/08/09 16:49:21  lucky
 *    *** empty log message ***
 *
 *    Revision 1.1  2000/08/07 19:53:14  lucky
 *    Initial revision
 *
 *
 *
 */
  

#include <review.h>
#include <time_functions.h>
#include <webparse.h>
#include <rw_mag.h>
#include <chron3.h>

#define		PICKTYPE_P		100
#define		PICKTYPE_S		101
#define		PICKTYPE_CODA	102
#define		PICKTYPE_PPMIN	103
#define		PICKTYPE_PPMAX	104
#define		PICKTYPE_ZPMAX	105

#define		STATUS_INSERTED		10
#define		STATUS_DELETED		20
#define		STATUS_EXISTING		30

#define		MAX_ACTIONS			20
#define		MAX_PICK_TYPES		10


/* 
 * Arrange retrieved picks into an array of SCNs.
 * Each SCN contains one or more pick types which
 * hold the last action (delete, new, existing).
 * We only care about the last thing that the user
 * did with this pick type.
 */

typedef struct _PickStruct
{
	int		PickType; /* see above for valid types */
	char	PhaseLabel[32];
	char	Onset;
	char	Fm;
	double	PickTime;
	double	ErrMag;
	int		Weight;
	double	CodaDur;
	double	Amp;
	double	Period;
	int		Status;
} Pick;


typedef struct _ChanInfo
{
	char 	Sta[9];
	char 	Comp[9];
	char 	Net[9];
	int		NumPicks;
	int		P_index;
	int		S_index;
	int		Coda_index;
	int		PPmin_index;
	int		PPmax_index;
	int		ZPmax_index;
	Pick	picks[MAX_PICK_TYPES];	
} ChanInfo;


typedef struct _PickInfo
{
	int			EventID;
	int			NumChans;
	ChanInfo	chans[DB_MAX_PHS_PER_EQ];
	char		Format[EWDB_ALARMS_MAX_FORMAT_LEN];
} PickInfo;




int		GetNextToken (char *, int, char *, int *);
int		ParsePickInfo (char *, int, PickInfo *);
int		ProcessData (char *, int, PickInfo *, int *, char *);
static	int		GetPickType (char *, int *);

extern  void sacinit (struct SAChead *);
double SAC_reftime( struct SAChead *psac );



#define		PBUFLEN		5000
#define		NUM_DATA	16


main ()
{
	int							i;
	int							datasize, totalsize;
	int							j, EvtIndex, index, GotWASac;
	double						tref, azm, dist;
	char						*configfile = "../params/review_event.d"; 
	char						EventDir[256];
	char						*ParamBuffer;
	char						SACfilename[1000];
	char						EVTfilename[1000];
	char						REVfilename[1000];
	char						cmd[1000];
	char						*sachead;
	char						*sacdata;
	char						*outbuf;
	char						*tmp;
	FILE						*fp;
	PickInfo					pickinfo;
	Pick					 	*pick1;
	Pick					 	*pick2;
	struct SAChead      		*psachd; 
	struct tm					*timestruct;
    EWEventInfoStruct           *pEventInfo;
    EWEventInfoStruct           *pLocEvent;
	EWDB_StationStruct			*pSta1;
	EWDB_StationMagStruct		*pstamag;



	/* Initialize some stuff
	 ***********************/
	DEBUG = 1;

	/* Read the configuration file (path hardcoded relative to executable)
	 *********************************************************************/
	ReadConfig (configfile);

	/* Cleanup, logfile should match config file name */
	logit_init ("retrieve_pick", 1, (PBUFLEN+1024), 1);  


	if ((ParamBuffer = (char *) malloc (PBUFLEN * sizeof (char))) == NULL)
	{
		logit ("", "Could not malloc ParamBuffer\n");
		return EW_FAILURE;
	}

	outbuf = malloc (512);

	Webparse_GetDecodedWebString (ParamBuffer, PBUFLEN);


	/* Write input to log file */
	logit ("t", "Retrieved %s\n", ParamBuffer);


    html_header ();
    printf ("<PRE><CENTER><BR><HR>\n");
    printf ("The following was retrieved from standard input:\n");
    printf ("%s\n", ParamBuffer);

    printf ("<BR><HR></CENTER></PRE>\n");



	/** Decode the param string -- fill the pick info struct */

	for (i = 0; i < DB_MAX_PHS_PER_EQ; i++)
	{
		pickinfo.chans[i].NumPicks = 0;
		pickinfo.chans[i].P_index = -1;
		pickinfo.chans[i].S_index = -1;
		pickinfo.chans[i].Coda_index = -1;
		pickinfo.chans[i].PPmin_index = -1;
		pickinfo.chans[i].PPmax_index = -1;
		pickinfo.chans[i].ZPmax_index = -1;
	}
	pickinfo.NumChans = 0;

	if (ParsePickInfo (ParamBuffer, PBUFLEN, &pickinfo) != EW_SUCCESS)
	{
		logit ("", "Call to ParsePickInfo failed\n");
		return EW_FAILURE;
	}

	if (DEBUG == 1)
	{
		logit ("", "Parsed into %d chans\n", pickinfo.NumChans);

		for (i = 0; i < pickinfo.NumChans; i++)
		{
			logit ("", "Chan %s.%s.%s  -- %d pick types\n",
							pickinfo.chans[i].Sta,
							pickinfo.chans[i].Comp,
							pickinfo.chans[i].Net,
							pickinfo.chans[i].NumPicks);
			logit ("", "P=%d, S=%d, Coda=%d, ZPmax=%d, PPmax=%d, PPmin=%d\n",
						pickinfo.chans[i].P_index,
						pickinfo.chans[i].S_index,
						pickinfo.chans[i].Coda_index,
						pickinfo.chans[i].ZPmax_index,
						pickinfo.chans[i].PPmax_index,
						pickinfo.chans[i].PPmin_index);

			for (j = 0; j < pickinfo.chans[i].NumPicks; j++)
			{
				logit ("", "Type %d, Status %d: %s %d %d %c %0.2f %0.2f %0.2f %0.2f\n",  
							j, pickinfo.chans[i].picks[j].Status,
							pickinfo.chans[i].picks[j].PhaseLabel,
							pickinfo.chans[i].picks[j].PickType,
							pickinfo.chans[i].picks[j].Onset,
							pickinfo.chans[i].picks[j].Fm,
							pickinfo.chans[i].picks[j].ErrMag,
							pickinfo.chans[i].picks[j].CodaDur,
							pickinfo.chans[i].picks[j].Amp,
							pickinfo.chans[i].picks[j].Period);
			}
		}
	}

	/* Read the binary event file file */

	/* Build the file name */
	sprintf (EventDir, "%s%c%d", ReviewTmpDir, DIR_SLASH,  pickinfo.EventID);
	sprintf (EVTfilename, "%s%cEvtStruct.bin", EventDir, DIR_SLASH);

	if ((fp = fopen (EVTfilename, "rb")) == NULL)
	{
		logit ("", "can't open %s\n", EVTfilename);
		return EW_FAILURE;
	}

	/* malloc EventInfo structures */

	if ((pEventInfo = (EWEventInfoStruct *) 
				malloc (sizeof (EWEventInfoStruct))) == NULL)
	{
		logit ("", "Could not malloc EventInfo.\n");
		return EW_FAILURE;
	} 


	if ((pLocEvent = (EWEventInfoStruct *) 
				malloc (sizeof (EWEventInfoStruct))) == NULL)
	{
		logit ("", "Could not malloc LocEvent.\n");
		return EW_FAILURE;
	} 


	/* Read the contents of the event file */
	fread ((void *) pEventInfo, sizeof (EWEventInfoStruct), 1, fp);

	/* Read the channel info structs */
	if ((pEventInfo->pChanInfo = (EWChannelDataStruct *) malloc
			(pEventInfo->iNumChans * sizeof (EWChannelDataStruct))) == NULL)
	{
		logit ("", "Can't malloc pChanInfo.\n");
		return EW_FAILURE;
	}
	pEventInfo->iNumAllocChans = pEventInfo->iNumChans;

	fread ((void *) pEventInfo->pChanInfo, sizeof (EWChannelDataStruct),
											pEventInfo->iNumChans, fp);

	fclose (fp);


	/* Allocate one single channel struct to hold retrieved data */
	if ((pLocEvent->pChanInfo = (EWChannelDataStruct *) malloc
							(1 * sizeof (EWChannelDataStruct))) == NULL)
	{
		logit ("", "Could not malloc pChanInfo.\n");
		return EW_FAILURE;
	}

	InitEWChan (&pLocEvent->pChanInfo[0]);


	/* 
	 * Story:  we just parsed a series of picks retrieved from
	 *  the applet. The trouble is that we don't know whether 
	 *  the user added picks, deleted picks, or merely modified 
	 *  them. So, in an attempt to keep all of this straight we do 
	 *  the following:
	 *
	 *   For each channel retrieved from the applet, open and read
	 *   the associated SAC header. Zero out all picks in the header
	 *   and then fill them in again from the retrieved information.
	 *   Then, convert the sac header into Event channel structure
	 *   and update the Event structure itself. Write out the updated
	 *   sac file and move on to the next channel.
	 *
	 *   The first step is to go over the arrival picks (P, S, and coda).
	 *   Then we repeat the process, but this time we modify the WA 
	 *   sac file.
	 */

	for (i = 0; i < pickinfo.NumChans; i++)
	{

		/* 
		 * Find this channel in the Event structure. We need to do this 
		 * to get to the event information, and to later replace it
 		 * with whatever was retrieved.
		 */

		EvtIndex = -1;
		for (j = 0; (j < pEventInfo->iNumChans && EvtIndex == -1); j++)
		{
			pSta1 = &pEventInfo->pChanInfo[j].Station;
			if ((strcmp (pSta1->Sta, pickinfo.chans[i].Sta) == 0) &&
					(strcmp (pSta1->Comp, pickinfo.chans[i].Comp) == 0) &&
					(strcmp (pSta1->Net, pickinfo.chans[i].Net) == 0))
			{
				EvtIndex = j;
			}
		}

		if (EvtIndex == -1)
		{
			logit ("", "Channel (%s.%s.%s) not found in the event struct.\n",
						pickinfo.chans[i].Sta, 
						pickinfo.chans[i].Comp, 
						pickinfo.chans[i].Net);
		}


		/* Initialize pLocEvent */
		pLocEvent->iNumChans = 0;
		pLocEvent->iNumAllocChans = 1;
		memset (&pLocEvent->Event, 0, sizeof(EWDB_EventStruct));
		memset (&pLocEvent->Mags, 0, MAX_MAGS_PER_ORIGIN * sizeof(EWDB_MagStruct));
		memset (&pLocEvent->PrefOrigin, 0, sizeof(EWDB_OriginStruct));

		InitEWChan (&pLocEvent->pChanInfo[0]);

		/* arrival picks */

		if ((pickinfo.chans[i].P_index != -1) || 
				(pickinfo.chans[i].S_index != -1) || 
				(pickinfo.chans[i].Coda_index != -1))
		{

			/* 
			 * Touch the .rev file to signal the rest of the 
			 * review system that this trace was reviewed 
			 * by the applet.
			 */
			sprintf (REVfilename, "%s%c%s.%s.%s.rev",
								EventDir, DIR_SLASH, 
								pickinfo.chans[i].Sta,
								pickinfo.chans[i].Comp,
								pickinfo.chans[i].Net);
		
			if ((fp = fopen (REVfilename, "wt")) == NULL)
			{
				logit ("", "can't open %s\n", REVfilename);
				return EW_FAILURE;
			}
			
			sprintf (outbuf, "%ld", time (NULL));
			
			if (fwrite ((const void *) outbuf, sizeof (outbuf), 1, fp) != 1)
			{
				logit ("", "Call to fwrite failed.\n");
				return EW_FAILURE;
			}
	
			fclose (fp);



			/* build the name of the sac file  */
			sprintf (EventDir, "%s%c%d", ReviewTmpDir, DIR_SLASH, pickinfo.EventID);
			sprintf (SACfilename, "%s%c%s.%s.%s", 
							EventDir, DIR_SLASH,  
							pickinfo.chans[i].Sta,
							pickinfo.chans[i].Comp,
							pickinfo.chans[i].Net);
	
	
			if ((fp = fopen (SACfilename, "rb")) == NULL)
			{
				logit ("", "Can't open %s\n", SACfilename);
				return EW_FAILURE;
			}
	
	
			/* Allocate space for SAC header */
			if ((sachead = (char *) malloc (SACHEADERSIZE)) == NULL)
			{
				logit ("", "Could not malloc sachead.\n");
				return EW_FAILURE;
			}
	
	
			/* Read contents of the SAC file */
			psachd = (struct SAChead *) sachead;
	
	
			if (fread (sachead, sizeof (char), 
					(size_t) SACHEADERSIZE, fp) != (size_t) SACHEADERSIZE)
			{
				logit ("", "Error reading header from file: %s!\n", SACfilename);
				fclose (fp);
				return EW_FAILURE;
			}
	
	
			datasize = psachd->npts * sizeof(float);
			totalsize = datasize + SACHEADERSIZE;
		
			/* Allocate space for SAC data */
			if ((sacdata = (char *) malloc (datasize)) == NULL)
			{
				logit ("", "Could not malloc sachead.\n");
				return EW_FAILURE;
			}
	
	
			if (fread (sacdata, sizeof (float), 
					(size_t) psachd->npts, fp) != (size_t) psachd->npts)
			{
				logit ("", "Error reading %d bytes of data from file: %s!\n",
															psachd->npts, SACfilename);
				fclose (fp);
				return EW_FAILURE;
			}
	
			/* Close file */
			fclose (fp);
	
			/* set all pick times to undefined */
			psachd->a = (float) SACUNDEF; 	/* P pick */
			psachd->t0 = (float) SACUNDEF; 	/* S pick */
			psachd->f = (float) SACUNDEF; 	/* Coda termination */
	
			/* undefine all pick labels, just in case */
			strcpy (psachd->ka, SACSTRUNDEF);
			strcpy (psachd->kt0, SACSTRUNDEF);
			strcpy (psachd->kf, SACSTRUNDEF);
	
			/* Absolute event origin time
			******************************/
			if ((tref = SAC_reftime (psachd)) < 0)
			{
				logit ("", "Call to SAC_reftime failed.\n");
				return EW_FAILURE;
			}


			/* Write retrieved picks into the sac header */
			if (pickinfo.chans[i].P_index != -1)
			{
				pick1 = &pickinfo.chans[i].picks[pickinfo.chans[i].P_index];
	
				/* pick time */
				if (pick1->Status != STATUS_DELETED)
				{
					psachd->a = (float) (pick1->PickTime - tref);
		
					sprintf (psachd->ka, "P%c%d%c  ",
									pick1->Fm,
									pick1->Weight,
									pick1->Onset);
				}
			} 
			if (pickinfo.chans[i].S_index != -1)
			{
				pick1 = &pickinfo.chans[i].picks[pickinfo.chans[i].S_index];

				/* pick time */
				if (pick1->Status != STATUS_DELETED)
				{
					psachd->t0 = (float) (pick1->PickTime - tref);
		
					sprintf (psachd->kt0, "S%c%d%c  ",
									pick1->Fm,
									pick1->Weight,
									pick1->Onset);
				}
			} 
			if (pickinfo.chans[i].Coda_index != -1)
			{
				pick1 = &pickinfo.chans[i].picks[pickinfo.chans[i].Coda_index];

				/* 
				 * Make sure that there is a P-pick; otherwise
				 * coda makes no sense. NOTE: this could be 
				 * dangerous because P pick could be coming
				 * later, but that is too scary to contemplate.
				 */
				if (pick1->Status != STATUS_DELETED)
				{
					if (psachd->a != (float) SACUNDEF)
 					{
						psachd->f = (float) (pick1->PickTime - tref);
					}
				}
			} 

			/* Write the SAC header back to file */
			if ((fp = fopen (SACfilename, "wb")) == NULL)
			{
				logit ("", "Can't open %s\n", SACfilename);
				return EW_FAILURE;
			}
	
	
			/* create an output buffer */
			if ((outbuf = (char *) malloc (totalsize)) == NULL)
			{
				logit ("", "Could not malloc outbuf.\n");
				return EW_FAILURE;
			}
	
	
			memcpy (outbuf, psachd, SACHEADERSIZE);
			tmp = outbuf + SACHEADERSIZE;
			memcpy (tmp, sacdata, datasize);
	
			if (fwrite ((const void *) outbuf, totalsize, 1, fp) != 1)
			{
				logit ("", "Call to fwrite failed.\n");
				return EW_FAILURE;
			}
	
			fclose (fp);


	
			/* 
		 	 * Now, update the LocEventInfo which will later be transferred
			 * into the EventInfo structure so that the rest of the review 
			 * system knows about the new pick info
			 */
	
			if (Sac2EWEvent (pLocEvent, psachd, sacdata, 0) != EW_SUCCESS)
			{
				logit ("", "Call to Sac2EWEvent failed.\n");
				return EW_FAILURE;
			}
	
			free (sachead);
			free (sacdata);
			free (outbuf);
	
			/*
			 * We have be prepared to handle one very special
			 *  case. Suppose that the automatic solution did not
			 *  produce an Md, yet the user makes arrival picks in 
			 *  the applet.. We need to add a magnitude of the appropriate 
			 *  type to the Event structure, otherwise those
			 *  picks would have no magnitude to be associated with.
			 */

			if (EvtIndex == -1)
			{
				logit ("", "Channel (%s.%s.%s) not found in the event struct.\n",
							pSta1->Sta, pSta1->Comp, pSta1->Net);
			}
			else
			{
				if (pEventInfo->iMd < 0)
				{
					/* No Md -- create one */
					if (pEventInfo->iNumMags < MAX_MAGS_PER_ORIGIN)
					{
						pEventInfo->iMd = pEventInfo->iNumMags;

						pEventInfo->Mags[pEventInfo->iNumMags].dMagAvg = 0.0;
						pEventInfo->Mags[pEventInfo->iNumMags].dMagErr = 0.0;
						pEventInfo->Mags[pEventInfo->iNumMags].iNumMags = 1;

						pEventInfo->Mags[pEventInfo->iNumMags].iMagType = 
												MAGTYPE_DURATION;
						
						pEventInfo->Mags[pEventInfo->iNumMags].idMag = -1;
						pEventInfo->Mags[pEventInfo->iNumMags].idOrigin = 
												pEventInfo->PrefOrigin.idOrigin;
						pEventInfo->Mags[pEventInfo->iNumMags].idEvent = 
												pEventInfo->Event.idEvent;
						pEventInfo->Mags[pEventInfo->iNumMags].bBindToEvent = TRUE;
						pEventInfo->Mags[pEventInfo->iNumMags].bSetPreferred = FALSE;

						pEventInfo->iNumMags = pEventInfo->iNumMags + 1;
					}
				}
			}
	
		} /* Do we have an arrival pick */


		/* Amplitude picks */
		if (((pickinfo.chans[i].PPmax_index != -1) &&
				(pickinfo.chans[i].PPmin_index != -1)) ||
					(pickinfo.chans[i].ZPmax_index != -1))
		{

			/* build the name of the WA sac file  */
			sprintf (EventDir, "%s%c%d", ReviewTmpDir, DIR_SLASH, pickinfo.EventID);
			sprintf (SACfilename, "%s%c%s.%s.%s.wa", 
							EventDir, DIR_SLASH, 
							pickinfo.chans[i].Sta,
							pickinfo.chans[i].Comp,
							pickinfo.chans[i].Net);

			/* Allocate space for SAC header */
			if ((sachead = (char *) malloc (SACHEADERSIZE)) == NULL)
			{
				logit ("", "Could not malloc sachead.\n");
				return EW_FAILURE;
			}

			/* Read contents of the SAC file */
			psachd = (struct SAChead *) sachead;
	
	
			GotWASac = TRUE;
			if ((fp = fopen (SACfilename, "rb")) == NULL)
			{
				logit ("", "Can't open %s; continuing without SAC support\n", 
																	SACfilename);
				GotWASac = FALSE;
			}

			if (GotWASac == TRUE)
			{
				if (fread (sachead, sizeof (char), 
						(size_t) SACHEADERSIZE, fp) != (size_t) SACHEADERSIZE)
				{
					logit ("", "Error reading header from file: %s!\n", SACfilename);
					fclose (fp);
					return EW_FAILURE;
				}
	
	
				datasize = psachd->npts * sizeof(float);
				totalsize = datasize + SACHEADERSIZE;
			
				/* Allocate space for SAC data */
				if ((sacdata = (char *) malloc (datasize)) == NULL)
				{
					logit ("", "Could not malloc sachead.\n");
					return EW_FAILURE;
				}
	
	
				if (fread (sacdata, sizeof (float), 
						(size_t) psachd->npts, fp) != (size_t) psachd->npts)
				{
					logit ("", "Error reading %d bytes of data from file: %s!\n",
												psachd->npts, SACfilename);
					fclose (fp);
					return EW_FAILURE;
				}
		
				/* Close file */
				fclose (fp);

			} /* if GotWASac  is true */
			else
			{
				/* Initialize a new sac header */
				sacinit (psachd);

				psachd->npts = 0;

				/* Set some header values that will never change */
				psachd->idep = SAC_IUNKN;   
				psachd->iztype = SAC_IBEGINTIME;
				psachd->iftype = SAC_ITIME;
				psachd->leven = 1; 
				psachd->b = 0; 
				strncpy (psachd->ko, "origin ", K_LEN);


				/* Event location */
				psachd->evla = pEventInfo->PrefOrigin.dLat;
				psachd->evlo = pEventInfo->PrefOrigin.dLon;
				psachd->evel = pEventInfo->PrefOrigin.dDepth;
				psachd->evdp = pEventInfo->PrefOrigin.dDepth;

				/* Station information */
				strcpy (psachd->kstnm, pEventInfo->pChanInfo[EvtIndex].Station.Sta);
				strcpy (psachd->kcmpnm, pEventInfo->pChanInfo[EvtIndex].Station.Comp);
				strcpy (psachd->knetwk, pEventInfo->pChanInfo[EvtIndex].Station.Net);

				psachd->stla = pEventInfo->pChanInfo[EvtIndex].Station.Lat;
				psachd->stlo = pEventInfo->pChanInfo[EvtIndex].Station.Lon;
				psachd->stel = pEventInfo->pChanInfo[EvtIndex].Station.Elev;

				if (geo_to_km (psachd->evla, psachd->evlo,
							psachd->stla, psachd->stlo, &dist, &azm) != 1)
				{
					logit ("", "Call to geo_to_km failed - ignoring.\n");
				}
				else
				{
					if (psachd->dist == (float) SACUNDEF)
						psachd->dist = (float)dist;
					
					if (psachd->az == (float) SACUNDEF)
						psachd->az = (float)azm;
					
				}
			}
	

			/* set all pick times to undefined */
			psachd->t0 = (float) SACUNDEF; 	/* ZPmax */
			psachd->t1 = (float) SACUNDEF; 	/* PPmin */
			psachd->t2 = (float) SACUNDEF; 	/* PPmax */
			psachd->user0 = (float) SACUNDEF; 	/* ZPamp */
			psachd->user1 = (float) SACUNDEF; 	/* PPamp */
	
			/* undefine all pick labels, just in case */
			strcpy (psachd->kt0, SACSTRUNDEF);
			strcpy (psachd->kt1, SACSTRUNDEF);
			strcpy (psachd->kt2, SACSTRUNDEF);
			strcpy (psachd->kuser0, SACSTRUNDEF);
			strcpy (psachd->kuser1, SACSTRUNDEF);


			/* Absolute event origin time
			******************************/
			if (GotWASac == TRUE)
			{
				if ((tref = SAC_reftime (psachd)) < 0)
				{
					logit ("", "Call to SAC_reftime failed.\n");
					return EW_FAILURE;
				}
			}


			/* 
			 * Handle the peak-to-peak amplitude picks as a special case:
			 * We expect to get back two matching picks with two times
			 * and two amplitudes. From them, we fill the SAC header 
			 * with the two pick times and a combined amplitude of
			 * PPmax - PPmin. 
			 * But first, we have to make sure that we got two matching
			 * picks.
			 */
			if ((pickinfo.chans[i].PPmax_index != -1) &&
						(pickinfo.chans[i].PPmin_index != -1))
			{

				pick1 = &pickinfo.chans[i].picks[pickinfo.chans[i].PPmin_index];
				pick2 = &pickinfo.chans[i].picks[pickinfo.chans[i].PPmax_index];

				if (GotWASac == FALSE)
				{
					/* If we are making up a sac header ... */

					time_t		ltime;

					/* set SAC reference time to the full hour of the first pick */
					ltime = (time_t) pick1->PickTime;
					timestruct = gmtime (&ltime);

					psachd->nzyear = (long) timestruct->tm_year + 1900l;
					psachd->nzjday = (long) timestruct->tm_yday + 1;
					psachd->nzhour = (long) timestruct->tm_hour;
					psachd->nzmin = (long) 0;
					psachd->nzsec = (long) 0;
					psachd->nzmsec = (long) 0;

					if ((tref = SAC_reftime (psachd)) < 0)
					{
						logit ("", "Call to SAC_reftime failed.\n");
						return EW_FAILURE;
					}
				}


				if ((pick1->Status != STATUS_DELETED) 
							&& (pick2->Status != STATUS_DELETED))
				{
					/* Update Sac Header */
	
					/* Pick times */
					psachd->t1 = (float) (pick1->PickTime - tref);
					psachd->t2 = (float) (pick2->PickTime - tref);
	
					/* peak-to-peak amplitudes */
					psachd->user1 = (float) pick1->Amp;
					psachd->user2 = (float) pick2->Amp;
		
					sprintf (psachd->kt1, "PPMin");
					sprintf (psachd->kt2, "PPMax");
					sprintf (psachd->kuser1, "PPMinAmp");
					sprintf (psachd->kuser1, "PPMaxAmp");
	
	
					/* Update the event structure */
					index = pLocEvent->pChanInfo[0].iNumStaMags;
					pstamag = &pLocEvent->pChanInfo[0].Stamags[index];
	
					pstamag->iMagType = LM_LocalMagType; 
	
					pstamag->StaMagUnion.PeakAmp.tAmp1 = pick1->PickTime;
					pstamag->StaMagUnion.PeakAmp.dAmp2 = (float)pick1->Amp;
					pstamag->StaMagUnion.PeakAmp.dAmpPeriod1 = MAG_NULL;

					pstamag->StaMagUnion.PeakAmp.tAmp2 = pick2->PickTime;
					pstamag->StaMagUnion.PeakAmp.dAmp2 = (float)pick2->Amp;
					pstamag->StaMagUnion.PeakAmp.dAmpPeriod2 = MAG_NULL;
	
					pLocEvent->pChanInfo[0].iNumStaMags = 
								pLocEvent->pChanInfo[0].iNumStaMags + 1;
				}
				else
				{
					/* 
					 * Amp picks are different -- if deletion
					 * is requested we have to delete the .wa
					 * file. Otherwise, the picks will be 
					 * recreated if the user relocates.
					 */
					if (GotWASac == TRUE)
					{
#ifdef _WINNT
						sprintf (cmd, "del /Q %s", SACfilename);
#else
						sprintf (cmd, "/bin/rm -f %s", SACfilename);
#endif _WINNT
						system (cmd);
					}
				}
			}

			else if (pickinfo.chans[i].ZPmax_index != -1)
			{
				pick1 = &pickinfo.chans[i].picks[pickinfo.chans[i].ZPmax_index];

				if (GotWASac == FALSE)
				{
					/* If we are making up a sac header ... */
					time_t		ltime;

					/* set SAC reference time to the full hour of the first pick */
					ltime = (time_t) pick1->PickTime;
					timestruct = gmtime (&ltime);

					psachd->nzyear = (long) timestruct->tm_year + 1900l;
					psachd->nzjday = (long) timestruct->tm_yday + 1;
					psachd->nzhour = (long) timestruct->tm_hour;
					psachd->nzmin = (long) 0;
					psachd->nzsec = (long) 0;
					psachd->nzmsec = (long) 0;

					if ((tref = SAC_reftime (psachd)) < 0)
					{
						logit ("", "Call to SAC_reftime failed.\n");
						return EW_FAILURE;
					}
				}

				if (pick1->Status != STATUS_DELETED)
				{
					/* Update the sac file */

					/* pick time */
					psachd->t0 = (float) (pick1->PickTime - tref);
					psachd->user0 = (float) (pick1->Amp);
					sprintf (psachd->kt0, "ZPMax");
					sprintf (psachd->kuser0, "ZPMaxAmp");
	
	
					/* Update the event structure */
					index = pLocEvent->pChanInfo[0].iNumStaMags;
					pstamag = &pLocEvent->pChanInfo[0].Stamags[index];
	
					pstamag->iMagType = LM_LocalMagType; 
	
					pstamag->StaMagUnion.PeakAmp.tAmp1 = pick1->PickTime;
					pstamag->StaMagUnion.PeakAmp.dAmp1 = (float)pick1->Amp;
					pstamag->StaMagUnion.PeakAmp.dAmpPeriod1 = MAG_NULL;


					pstamag->StaMagUnion.PeakAmp.tAmp2 = MAG_NULL;
					pstamag->StaMagUnion.PeakAmp.dAmp2 = MAG_NULL;
					pstamag->StaMagUnion.PeakAmp.dAmpPeriod2 = MAG_NULL;
	
					pLocEvent->pChanInfo[0].iNumStaMags = 
								pLocEvent->pChanInfo[0].iNumStaMags + 1;
				}
				else
				{
					if (GotWASac == TRUE)
					{
#ifdef _WINNT
						sprintf (cmd, "del /Q %s", SACfilename);
#else
						sprintf (cmd, "/bin/rm -f %s", SACfilename);
#endif _WINNT
						system (cmd);
					}
				}
			}
	
	

			/* Write the SAC header back to file */
			if ((fp = fopen (SACfilename, "wb")) == NULL)
			{
				logit ("", "Can't open %s\n", SACfilename);
				return EW_FAILURE;
			}
		
			if (GotWASac == FALSE)
				psachd->npts = 0;

			datasize = psachd->npts * sizeof(float);
			totalsize = datasize + SACHEADERSIZE;

			/* create an output buffer */
			if ((outbuf = (char *) malloc (totalsize)) == NULL)
			{
				logit ("", "Could not malloc outbuf.\n");
				return EW_FAILURE;
			}
	
			memcpy (outbuf, psachd, SACHEADERSIZE);
		
			if (GotWASac == TRUE)
			{
				tmp = outbuf + SACHEADERSIZE;
				memcpy (tmp, sacdata, datasize);
				free (sacdata);
			}
		
			if (fwrite ((const void *) outbuf, totalsize, 1, fp) != 1)
			{
				logit ("", "Call to fwrite failed.\n");
				return EW_FAILURE;
			}
		
			fclose (fp);
			free (sachead);
			free (outbuf);

			/*
			 * We have be prepared to handle one very special
			 *  case. Suppose that the automatic solution did not
			 *  produce an ML, yet the user makes amplitude picks in 
			 *  the applet.. We need to add a magnitude of the appropriate 
			 *  type to the Event structure, otherwise those
			 *  picks would have no magnitude to be associated with.
			 */

			if (EvtIndex == -1)
			{
				logit ("", "Channel (%s.%s.%s) not found in the event struct.\n",
							pSta1->Sta, pSta1->Comp, pSta1->Net);
			}
			else
			{

				/* Fill in the station information for LocEvent */
				memcpy (&pLocEvent->pChanInfo[0].Station,
						&pEventInfo->pChanInfo[EvtIndex].Station,
						sizeof (EWDB_StationStruct));

				pLocEvent->pChanInfo[0].iNumWaveforms = 
						pEventInfo->pChanInfo[EvtIndex].iNumWaveforms;

				if (pEventInfo->iML < 0)
				{
					/* No ML -- create one */
					if (pEventInfo->iNumMags < MAX_MAGS_PER_ORIGIN)
					{
						pEventInfo->iML = pEventInfo->iNumMags;

						pEventInfo->Mags[pEventInfo->iNumMags].dMagAvg = 0.0;
						pEventInfo->Mags[pEventInfo->iNumMags].dMagErr = 0.0;
						pEventInfo->Mags[pEventInfo->iNumMags].iNumMags = 1;

						if ((pickinfo.chans[i].PPmax_index != -1) &&
									(pickinfo.chans[i].PPmin_index != -1))
						{
							pEventInfo->Mags[pEventInfo->iNumMags].iMagType = 
												MAGTYPE_LOCAL_PEAK2PEAK;
						}
						else
							pEventInfo->Mags[pEventInfo->iNumMags].iMagType = 
												MAGTYPE_LOCAL_ZERO2PEAK;

						
						pEventInfo->Mags[pEventInfo->iNumMags].idMag = -1;
						pEventInfo->Mags[pEventInfo->iNumMags].idOrigin = 
												pEventInfo->PrefOrigin.idOrigin;
						pEventInfo->Mags[pEventInfo->iNumMags].idEvent = 
												pEventInfo->Event.idEvent;
						pEventInfo->Mags[pEventInfo->iNumMags].bBindToEvent = TRUE;
						pEventInfo->Mags[pEventInfo->iNumMags].bSetPreferred = FALSE;

						pEventInfo->iNumMags = pEventInfo->iNumMags + 1;
					}
				}
			}

		} /* Do we have an amplitude pick */

	
		if (EvtIndex == -1)
		{
			logit ("", "Channel (%s.%s.%s) not found in the event struct.\n",
						pSta1->Sta, pSta1->Comp, pSta1->Net);
		}
		else
		{
			memcpy (&pEventInfo->pChanInfo[EvtIndex], &pLocEvent->pChanInfo[0], 
						sizeof (EWChannelDataStruct));
		}

	} /* for loop (i) over retrieved channels */


	/* Write out the modified event structure */

	if ((fp = fopen (EVTfilename, "wb")) == NULL)
	{
		logit ("", "can't open %s\n", EVTfilename);
		return EW_FAILURE;
	}

	pEventInfo->iNumAllocChans = pEventInfo->iNumChans;

	fwrite ((void *) pEventInfo, sizeof (EWEventInfoStruct), 1, fp);

	/* append channel info structs */

	fwrite ((void *) pEventInfo->pChanInfo, sizeof (EWChannelDataStruct),
													pEventInfo->iNumChans, fp);

	fclose (fp);

	logit("","retrieve_pick: terminating\n" );

	return EW_SUCCESS;

}  /* end main */


/******************************************************
     Dumps HTML header, title, and beginning page
 **********************************************************/
void    html_header( void )
{

   printf("Content-type: text/html\n\n");
   printf("<html>\n");
   printf("<HEAD><TITLE>Retrieve Pick Information</TITLE></HEAD>\n"
          "<BODY BGCOLOR=#EEEEEE TEXT=#333333>\n" );
   printf("<center>\n");
   printf("<H1><FONT COLOR=blue>Retrieve Pick Information<br></FONT></H1>\n" );
  printf("</center>\n");

}  



/******************************************************
     Get next XML token from the Buffer
 **********************************************************/
int		GetNextToken (char *ParamBuffer, int BufLen, char *token, int *TokLen)
{

	int		i = 0;
	int		j = 0;

	if ((ParamBuffer == NULL) || (token == NULL))
	{
		logit ("", "Invalid arguments passed in\n");
		return EW_FAILURE;
	}


	/* Look for opening < */
	while ((ParamBuffer[i] != '<') && (i < BufLen))
		i = i + 1;

	if (i >= BufLen)
	{
		logit ("", "Invalid XML line - can't find start of next token.\n");
		return EW_FAILURE;
	}

	i = i + 1;


	/* Look for closing > */
	while ((ParamBuffer[i] != '>') && (i < BufLen))
	{
		token[j] = ParamBuffer[i];
		i = i + 1;
		j = j + 1;
	}
	token[j] = '\0';
	
	if (i >= BufLen)
	{
		logit ("", "Invalid XML line - can't find end of next token.\n");
		return EW_FAILURE;
	}

	*TokLen = i + 1;

	return EW_SUCCESS;

}


/******************************************************
     Parse stdin string into PickInfo structure
 **********************************************************/
int		ParsePickInfo (char *ParamBuffer, int BufLen, PickInfo *pickinfo)
{

	int		i, j, TokenLen;
	int		done;
	char	token[PBUFLEN];
	char	tmpstr[PBUFLEN];
	char	StatusStr[PBUFLEN];


	if ((ParamBuffer == NULL) || (pickinfo == NULL) || (BufLen <= 0))
	{
		logit ("", "Invalid arguments passed in\n");
		return EW_FAILURE;
	}


	/** Start decoding the string - get the first token **/
	TokenLen = 0;
	if (GetNextToken (ParamBuffer, BufLen, token, &TokenLen) != EW_SUCCESS)
	{
		logit ("", "Call to GetNextToken failed.\n");
		return EW_FAILURE;
	}
	ParamBuffer = ParamBuffer + TokenLen;

	/* ingore the first token, go for the next one, shoud be Event */
	TokenLen = 0;
	if (GetNextToken (ParamBuffer, BufLen, token, &TokenLen) != EW_SUCCESS)
	{
		logit ("", "Call to GetNextToken failed.\n");
		return EW_FAILURE;
	}
	ParamBuffer = ParamBuffer + TokenLen;

	i = 0;
	while ((token[i] != ' ') && (i < BufLen))
	{
		tmpstr[i] = token[i];
		i = i + 1;
	}
	if (i >= BufLen)
	{
		logit ("", "Invalid XML line - can't find Event token.\n");
		return EW_FAILURE;
	}
	tmpstr[i] = '\0';
	
	if (strcmp (tmpstr, "Event") != 0)
	{
		logit ("", "XML ERROR: expecting <Event>, got %s\n", tmpstr);
		return EW_FAILURE;
	}

	/* Look for start of EventID */
	j = 0;
	while ((token[i] != '"') && (i < BufLen))
		i = i + 1;

	if (i >= BufLen)
	{
		logit ("", "Invalid XML line - can't find start of EventID.\n");
		return EW_FAILURE;
	}

	i = i + 1;

	/* Look for end of EventID */
	while ((token[i] != '"') && (i < BufLen))
	{
		tmpstr[j] = token[i];
		i = i + 1;
		j = j + 1;
	}
	tmpstr[j] = '\0';

	if (i >= BufLen)
	{
		logit ("", "Invalid XML line - can't find end of EventID.\n");
		return EW_FAILURE;
	}

	pickinfo->EventID = atoi (tmpstr);


	/* Look for the <ReadingList> token */
	TokenLen = 0;
	if (GetNextToken (ParamBuffer, BufLen, token, &TokenLen) != EW_SUCCESS)
	{
		logit ("", "Call to GetNextToken failed.\n");
		return EW_FAILURE;
	}
	ParamBuffer = ParamBuffer + TokenLen;


	i = 0;
	while ((token[i] != ' ') && (i < BufLen))
	{
		tmpstr[i] = token[i];
		i = i + 1;
	}
	if (i >= BufLen)
	{
		logit ("", "Invalid XML line - can't find ReadingList token.\n");
		return EW_FAILURE;
	}
	tmpstr[i] = '\0';

	if (strcmp (tmpstr, "ReadingList") != 0)
	{
		logit ("", "XML ERROR: expecting <ReadingList>, got %s\n", tmpstr);
		return EW_FAILURE;
	}

	/* Look for start of format subtoken */
	i = 0;
	j = 0;
	while ((token[i] != '"') && (i < BufLen))
		i = i + 1;

	if (i >= BufLen)
	{
		logit ("", "Invalid XML line - can't find start of Format subtoken.\n");
		return EW_FAILURE;
	}

	i = i + 1;

	/* Look for end of format subtoken */
	while ((token[i] != '"') && (i < BufLen))
	{
		tmpstr[j] = token[i];
		i = i + 1;
		j = j + 1;
	}
	tmpstr[j] = '\0';

	if (i >= BufLen)
	{
		logit ("", "Invalid XML line - can't find end of Format subtoken.\n");
		return EW_FAILURE;
	}

	if (strlen (tmpstr) >= EWDB_ALARMS_MAX_FORMAT_LEN)
	{
		logit ("", "Format string too long: %s\n", tmpstr);
		return EW_FAILURE;
	}
		
	strcpy (pickinfo->Format, tmpstr);


	/* Look for the <Reading> tokens */
	TokenLen = 0;
	if (GetNextToken (ParamBuffer, BufLen, token, &TokenLen) != EW_SUCCESS)
	{
		logit ("", "Call to GetNextToken failed.\n");
		return EW_FAILURE;
	}
	ParamBuffer = ParamBuffer + TokenLen;


	/* If we got </ReadingList>, we are done -- no picks, go home */
	if (strcmp (token, "/ReadingList") == 0)
	{
		/* We are done -- look for </Event> and exit */
		TokenLen = 0;
		if (GetNextToken (ParamBuffer, BufLen, token, &TokenLen) != EW_SUCCESS)
		{
			logit ("", "Call to GetNextToken failed.\n");
			return EW_FAILURE;
		}
		ParamBuffer = ParamBuffer + TokenLen;

		if (strcmp (token, "/Event") != 0)
		{
			logit ("", "XML ERROR: expecting </Event>, got <%s>\n", token);
			return EW_FAILURE;
		}
	}
	else
	{
		/* This better be <Reading status=STATUS> */

		if (strncmp (token, "Reading", 7) != 0)
		{
			logit ("", "XML ERROR: expecting <Reading>, got <%s>\n", tmpstr);
			return EW_FAILURE;
		}

		/* Look for status subtoken */
		if (strncmp (token+8, "status", 6) != 0)
		{
			logit ("", "Invalid XML line - can't find Status subtoken.\n");
			return EW_FAILURE;
		}


		strcpy (StatusStr, token+15);

		done = FALSE;
		/* loop until </ReadingList> is found */
		while (done == FALSE)
		{
			/* Got a reading */

			/* read data */
			TokenLen = 0;
			if (ProcessData (ParamBuffer, BufLen, pickinfo, 
										&TokenLen, StatusStr) != EW_SUCCESS)
			{
				logit ("", "Call to ProcessData failed.\n");
				return EW_FAILURE;
			}
			ParamBuffer = ParamBuffer + TokenLen;


			/* look for /Reading */
			TokenLen = 0;
			if (GetNextToken (ParamBuffer, BufLen, token, &TokenLen) != EW_SUCCESS)
			{
				logit ("", "Call to GetNextToken failed.\n");
				return EW_FAILURE;
			}
			ParamBuffer = ParamBuffer + TokenLen;

			if (strcmp (token, "/Reading") != 0)
			{
				logit ("", "XML ERROR: expecting </Reading>, got %s\n", token);
				return EW_FAILURE;
			}

			/* look for Reading */
			/* if no Reading found, set done to TRUE */
			TokenLen = 0;
			if (GetNextToken (ParamBuffer, BufLen, token, &TokenLen) != EW_SUCCESS)
			{
				logit ("", "Call to GetNextToken failed.\n");
				return EW_FAILURE;
			}
			ParamBuffer = ParamBuffer + TokenLen;

			if (strcmp (token, "/ReadingList") == 0)
			{	
				done = TRUE;
			}
			else
			{
				/* this better be another <Reading ... > token */

				if (strncmp (token, "Reading", 7) != 0)
				{
					logit ("", "XML ERROR: expecting <Reading>, got <%s>\n", tmpstr);
					return EW_FAILURE;
				}
		
				/* Look for status subtoken */
				if (strncmp (token+8, "status", 6) != 0)
				{
					logit ("", "Invalid XML line - can't find Status subtoken.\n");
					return EW_FAILURE;
				}
		
				strcpy (StatusStr, token+15);

			}
		}

		/* look for </Event> and exit */
		TokenLen = 0;
		if (GetNextToken (ParamBuffer, BufLen, token, &TokenLen) != EW_SUCCESS)
		{
			logit ("", "Call to GetNextToken failed.\n");
			return EW_FAILURE;
		}
		ParamBuffer = ParamBuffer + TokenLen;

		if (strcmp (token, "/Event") != 0)
		{
			logit ("", "XML ERROR: expecting </Event>, got <%s>\n", token);
			return EW_FAILURE;
		}
	}


	if (DEBUG == 1)
		logit ("", "Done Processing: Got %d channels\n", pickinfo->NumChans);
	
	return EW_SUCCESS;
}


/******************************************************
     Process Pick data and stuff it into pickinfo
 **********************************************************/
int		ProcessData (char *ParamBuffer, int BufLen, PickInfo *pickinfo, 
									int	*TokenLen, char *StatusString)
{

	int		i, index, np;
	int		PickType;
	char	data[NUM_DATA][PBUFLEN];
	char	line[PBUFLEN];
	char	tmpstr[PBUFLEN];
	double secs;
	Pick	*pick;


	if ((ParamBuffer == NULL) || (BufLen <= 0) || 
			(pickinfo == NULL)  || (TokenLen == NULL))
	{
		logit ("", "Invalid arguments passed in.\n");
		return EW_FAILURE;
	}

	/* look for start of next token */
	i = 0;
	while ((ParamBuffer[i] != '<') && (i < BufLen))
	{
		line[i] = ParamBuffer[i]; 
		i = i + 1;
	}

	if (i >= BufLen)
	{
		logit ("", "Invalid XML line - cannot find start of next token.\n");
		return EW_FAILURE;
	}

	line[i] = '\0';

	*TokenLen = i;

	sscanf (line, "%s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s\n", 
				data[0], data[1], data[2], data[3], data[4],
				data[5], data[6], data[7], data[8], data[9],
				data[10], data[11], data[12], data[13], data[14], data[15]);


	if (DEBUG == 1)
	{
		logit ("", "sta is %s\n", 		data[0]);
		logit ("", "comp is %s\n", 		data[1]);
		logit ("", "net is %s\n", 		data[2]);
		logit ("", "inst is %s\n", 		data[3]);
		logit ("", "comp is %s\n", 		data[4]);
		logit ("", "onset is %s\n",		data[5]);
		logit ("", "desc is %s\n", 		data[6]);
		logit ("", "fm is %s\n", 		data[7]);
		logit ("", "date is %s\n", 		data[8]);
		logit ("", "time is %s\n", 		data[9]);
		logit ("", "secs is %s\n", 		data[10]);
		logit ("", "error is %s\n", 	data[11]);
		logit ("", "errmag is %s\n", 	data[12]);
		logit ("", "coda is %s\n", 		data[13]);
		logit ("", "amp is %s\n", 		data[14]);
		logit ("", "period is %s\n",	data[15]);
	}


	/* See if we already have this channel */
	index = -1;
	for (i = 0; ((i < pickinfo->NumChans) && (index == -1)); i++)
	{
		if ((strcmp (pickinfo->chans[i].Sta, data[0]) == 0) && 
		    (strcmp (pickinfo->chans[i].Comp, data[1]) == 0) && 
		    (strcmp (pickinfo->chans[i].Net, data[2]) == 0))
		{
			index = i;
		}
	}
		
	if (index == -1)
	{
		/* New channel */
		index = pickinfo->NumChans;
		strcpy (pickinfo->chans[index].Sta, data[0]);
		strcpy (pickinfo->chans[index].Comp, data[1]);
		strcpy (pickinfo->chans[index].Net, data[2]);

		pickinfo->chans[index].P_index = -1;
		pickinfo->chans[index].S_index = -1;
		pickinfo->chans[index].Coda_index = -1;
		pickinfo->chans[index].PPmin_index = -1;
		pickinfo->chans[index].PPmax_index = -1;
		pickinfo->chans[index].ZPmax_index = -1;
		pickinfo->chans[index].NumPicks = 0;
		pickinfo->NumChans = pickinfo->NumChans + 1;

		for (i = 0; i < MAX_PICK_TYPES; i++)
			pickinfo->chans[index].picks[i].PickType = -1;
	}

	/* See if we already have this pick type */
	GetPickType (data[6], &PickType);

	pick = NULL;
	for (i = 0; i < pickinfo->chans[index].NumPicks; i++)
	{
		if (pickinfo->chans[index].picks[i].PickType == PickType)
		{
			pick = &pickinfo->chans[index].picks[i];
			np = i;
		}
	}

	if (pick == NULL)
	{
		/* New pick type */
		if (pickinfo->chans[index].NumPicks < MAX_PICK_TYPES)
		{
			np = pickinfo->chans[index].NumPicks;
			pick = &pickinfo->chans[index].picks[np];
			pickinfo->chans[index].NumPicks = 
					pickinfo->chans[index].NumPicks + 1;
		}
		else
		{
			logit ("", "Max pick types (%d) exceeded - exit!\n",
							MAX_PICK_TYPES);
			return EW_FAILURE;
		}
	}
		
	pick->PickType = PickType;
	strcpy (pick->PhaseLabel, data[6]);


	/* Fill the status value */
	if (strcmp (StatusString, "existing") == 0)
		pick->Status = STATUS_EXISTING;
	else if (strcmp (StatusString, "deleted") == 0)
		pick->Status = STATUS_DELETED;
	else if (strcmp (StatusString, "inserted") == 0)
		pick->Status = STATUS_INSERTED;
	else
	{
		logit ("", "Invalid pick status: %s\n", StatusString);
		return EW_FAILURE;
	}
		

	if (pick->PickType == PICKTYPE_P)
		pickinfo->chans[index].P_index = np;
	else if (pick->PickType == PICKTYPE_S)
		pickinfo->chans[index].S_index = np;
	else if (pick->PickType == PICKTYPE_CODA)
		pickinfo->chans[index].Coda_index = np;
	else if (pick->PickType == PICKTYPE_ZPMAX)
		pickinfo->chans[index].ZPmax_index = np;
	else if (pick->PickType == PICKTYPE_PPMAX)
		pickinfo->chans[index].PPmax_index = np;
	else if (pick->PickType == PICKTYPE_PPMIN)
		pickinfo->chans[index].PPmin_index = np;
	else
	{
		logit ("", "Invalid pick type: %d\n", pick->PickType);
		return EW_FAILURE;
	}


	if (data[5][0] != '?')
		pick->Onset = data[5][0]; 
	else
		pick->Onset = ' ';


	/* first check the "official" first motion field */
	if (data[7][0] != '?')
		pick->Fm = data[7][0];
	else if ((data[6][1] != ' ') && (data[6][1] != '\0') && (data[6][1] != '\n'))
	{
		/* now check the second character of the phase description */
		pick->Fm = data[6][1];
	}
	else
	{
		/* If all else fails, punt */
		pick->Fm = ' ';
	}


	/* 
	 * If this is an existing pick, set the weight from the
	 * pick label. The applet currently does not ship back the
 	 * previously defined error.
	 */
	if (pick->Status == STATUS_EXISTING)
	{
		int		wt;

		wt = data[6][2] - '0'; 

		if (wt == 3)
			pick->ErrMag = 0.08;
		else if (wt == 2)
			pick->ErrMag = 0.05;
		else if (wt == 1)
			pick->ErrMag = 0.03;
		else if (wt == 0)
			pick->ErrMag = 0.02;
		else
			pick->ErrMag = 99.99;
	}
	else
	{
		pick->ErrMag = atof (data[12]);
	}



	pick->CodaDur = atof (data[13]);
	pick->Amp = atof (data[14]);
	pick->Period = atof (data[15]);


	/* Convert error into weight */
	if (pick->ErrMag <= 0.02)
		pick->Weight = 0;
	else if (pick->ErrMag <= 0.03)
		pick->Weight = 1;
	else if (pick->ErrMag <= 0.05)
		pick->Weight = 2;
	else if (pick->ErrMag <= 0.08)
		pick->Weight = 3;
	else
		pick->Weight = 4;
	

	/* convert time */
	if ((secs = (atof (data[10]))) < 0.0)
	{
		logit ("", "Call to atof failed. \n");
		return EW_FAILURE;
	}

	sprintf (tmpstr, "%s%s%0.2f", data[8], data[9], secs); 

	if (epochsec17 (&pick->PickTime, tmpstr) != 0)
	{
		logit ("", "Call to epochsec17 failed.\n");
		return EW_FAILURE;
	}


	return EW_SUCCESS;

}


static	int		GetPickType (char *label, int *PickType)
{


	if ((label == NULL) || (PickType == NULL))
	{
		logit ("", "Invalid arguments passed in.\n");
		return EW_FAILURE;
	}


	if (label[0] == 'S')
		*PickType = PICKTYPE_S;
	else if (label[0] == '_')
		*PickType = PICKTYPE_CODA;
	else if ((strcmp (label, "PPMin") == 0) || (strcmp (label, "P-P_min") == 0))
		*PickType = PICKTYPE_PPMIN;
	else if ((strcmp (label, "PPMax") == 0) || (strcmp (label, "P-P_max") == 0))
		*PickType = PICKTYPE_PPMAX;
	else if ((strcmp (label, "ZPMax") == 0) || (strcmp (label, "0-P_max") == 0))
		*PickType = PICKTYPE_ZPMAX;
	else
		*PickType = PICKTYPE_P;
	
	return EW_SUCCESS;
}
