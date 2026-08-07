
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: html_common.c,v 1.9 2003/01/30 23:12:57 lucky Exp $
 *    Revision history:
 *
 *    $Log: html_common.c,v $
 *    Revision 1.9  2003/01/30 23:12:57  lucky
 *    *** empty log message ***
 *
 *    Revision 1.8  2002/09/17 16:20:41  lucky
 *    Removed link to other origins -- should only be available if there were
 *    any other origins in the DB
 *
 *    Revision 1.7  2002/06/19 16:17:17  lucky
 *     Added support for displaying multiple mag types, beyond just Md and ML
 *
 *    Revision 1.6  2002/05/28 17:25:41  lucky
 *    *** empty log message ***
 *
 *    Revision 1.5  2001/07/01 21:55:23  davidk
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
 *    Revision 1.4  2001/01/30 20:46:07  lucky
 *    Updated html_trailer so that the webmaster message is printed
 *    only if webhost != NULL.
 *
 *    Revision 1.3  2000/08/16 17:49:48  lucky
 *    *** empty log message ***
 *
 *    Revision 1.2  2000/06/21 16:32:59  lucky
 *    Deleted html_error; it was replaced by html_logit in logit.c
 *
 *
 *
 *
 */



#include <stdlib.h>
#include <stdio.h>
#include <review.h>
#include <review_globals.h>
#include <time_ew.h>
#include <sys/types.h>
#include <sys/stat.h>



static	char		errtxt[1024];



/******************************************************
    Write an html break line to stdout
 **********************************************************/
void	html_break( void )
{

   printf("<br>\n");

}  /* End html_break() */


/******************************************************
       Write start-html stuff to stdout
 **********************************************************/
void    html_header (char *BackgroundColor, char *HeaderLogo, char *HeaderTag)
{

	if ((BackgroundColor == NULL) || (HeaderLogo == NULL))
	{
		logit ("", "html_header: invalid parameters; exit!\n");
		exit (-1);
	}

    /* Background color, if specified */
    if (strcmp (BackgroundColor, "notset") == 0)
        printf ("<BODY BGCOLOR=\"#FFFFFF\" TEXT=#333333>\n");
    else
        printf ("<BODY BGCOLOR=\"%s\" TEXT=#333333>\n", BackgroundColor);

    /* Header logo, if specified */
    if (strcmp (HeaderLogo, "notset") != 0)
	{
        printf ("<CENTER>");

		printf ("<IMG SRC=\"%s\"", HeaderLogo);

	    if (strcmp (HeaderTag, "notset") != 0)
			printf (" ALT=\"%s\"", HeaderTag);

		printf (" align=CENTER></CENTER><BR><HR><BR>\n");
	}

}  /* End html_header() */



/******************************************************
       Write end-html stuff to stdout
 **********************************************************/
void	html_trailer (char *webhost, char *FooterLogo, char *FooterTag)
{

    /* Footer logo, if specified */
    if (strcmp (FooterLogo, "notset") != 0)
	{
        printf ("<HR><BR><CENTER>");

		printf ("<IMG SRC=\"%s\"", FooterLogo);

	    if (strcmp (FooterTag, "notset") != 0)
			printf (" ALT=\"%s\"", FooterTag);

		printf (" align=CENTER> </CENTER><BR>\n");

	}

    /* Print out the postmortem, allowing people to complain */
	if (webhost != NULL)
	{
		printf ("</strong><center><br><i>\n");
		printf ("Please forward any comments or questions about this site to "
			"the <A HREF=\"mailto:webmaster@%s\">Webmaster</A>.\n", webhost);
		printf ("</center></i><br>\n");
	}

   printf("</html>\n");
   
}  /* End html_trailer() */





/******************************************************
  Function:    html_principal_summary()
  Purpose:     Write summary stuff to stdout(origin,event,magnitude)
  Parameters:    
      Input
      pEvt:    Pointer to an EWDB_EventStruct whose contents
               will be written in html format to stdout.
      bReview: If set to TRUE, this is invoked by review -- we 
               show some additional info. Otherwise, bland info only              
    
  **********************************************************/
void html_principal_summary (EWEventInfoStruct *pEvt, int bReview)
{
	char  				timestr[DATESTR23];
	char  				loc_timestr[DATESTR23];
	char  				ns = 'N';  /* default for positive lat */
	char  				ew = 'E';  /* default for positive lon */
	float 				mlat, mlon;
	EWDB_OriginStruct 	*por;
	char				hypfile[256];
	struct stat			statbuf;
	int					m;


	if (pEvt == NULL)
    {
		logit ("", "Invalid arguments passed in; exitting\n");
		exit (-1);
	}

	por = &pEvt->PrefOrigin;

/* Figure out extra things we need to print out 
 **********************************************/
	datestr23 ( por->tOrigin, timestr, DATESTR23 );   /* origin time as string */
	datestr23_local ( por->tOrigin, loc_timestr, DATESTR23 );   /* origin time as string */
	if (por->dLat < 0) 
		ns='S';
	if (por->dLon < 0) 
		ew='W';

	mlat = (float)( ABS(por->dLat-(int)por->dLat) * 60. );
	mlon = (float)( ABS(por->dLon-(int)por->dLon) * 60. );

	printf("<strong>\n<pre>\n"
		"PRINCIPAL EARTHQUAKE PARAMETERS\n"
		"_______________________________\n\n" );


	if (bReview == TRUE)
	{
		printf ("<CENTER>\n");
		printf ("NOTE: This solution has been relocated.\n");

		sprintf (hypfile, "%s%c%d%creloc_output", 
					ReviewTmpDir, DIR_SLASH, pEvt->PrefOrigin.idOrigin, DIR_SLASH); 

		if (stat (hypfile, &statbuf) == 0)
		{
			/* 
			 * If the reloc_output file exists, this means
			 * that this event was just reprocessed by hypoinverse and localmag.
			 * Let the user know about this, and let them 
			 * see the output if they so wish
			 */
			printf ("<A HREF=\"%s/%d/reloc_output\" target=\"Relocate Output\">"
						"Click here to see the output of reprocessing.</A>\n",
							WebTmpDir, pEvt->PrefOrigin.idOrigin);

		}

		/* Also show the PRT file if it exists */
		sprintf (hypfile, "%s%c%d%cHyp.prt", 
					ReviewTmpDir, DIR_SLASH, pEvt->PrefOrigin.idOrigin, DIR_SLASH); 

		if (stat (hypfile, &statbuf) == 0)
		{
			printf ("<A HREF=\"%s/%d/Hyp.prt\" target=\"Hypoinverse PRT File \">"
						"Click here to see the hypoinverse PRT file.</A>\n\n",
							WebTmpDir, pEvt->PrefOrigin.idOrigin);
		}

		printf ("</CENTER>\n");
	}
		

	if (pEvt->iNumMags <= 0)
		printf("Magnitudes                   : None Available\n");
	else
	{
		printf("Magnitudes                   : ");

		for (m = 0; m < pEvt->iNumMags; m++)
			printf ("%3.1f%s ", pEvt->Mags[m].dMagAvg, MagNames[pEvt->Mags[m].iMagType]);

		printf ("\n");

	}


   printf("Event Date & Time            : %s   %s  UTC\n", 
           strtok(timestr," "), &timestr[11] );
   printf("LOCAL Event Date & Time      : %s   %s  %s\n", 
           strtok(timestr," "), &loc_timestr[11], getenv ("TZ") );
   printf("Location                     : %.4f %c, %.4f %c\n",
           ABS(por->dLat), ns, ABS(por->dLon), ew );
   printf("                             : (%d deg. %.2f min. %c, %d deg. %.2f min. %c)\n",
           ABS((int)por->dLat), mlat, ns, ABS((int)por->dLon), mlon, ew );
   printf("Depth                        : %.1f km. deep (%.1f miles)\n\n\n",
           por->dDepth, KMtoMILE(por->dDepth) );


   printf("ADDITIONAL EARTHQUAKE PARAMETERS\n"
          "________________________________\n\n" );
   printf("number of phases used        : %6d\n", por->iUsedPh );
   printf("rms misfit                   : %6.2f  seconds\n", por->dRms );
   printf("latitudinal  location error  : %6.1f  km\n", por->dErrLat );
   printf("longitudinal location error  : %6.1f  km\n", por->dErrLon );
   printf("vertical location error      : %6.1f  km\n", por->dErZ );
   printf("maximum azimuthal gap        : %6d  degrees\n", por->iGap );
   printf("distance to nearest station  : %6.0f. km\n\n", por->dDmin );

   printf("Database event ID: %10u\n", pEvt->Event.idEvent );

   printf("Author:            %s\n\n\n",por->sSource);

   printf("</pre>\n");
   return;
}  /* End html_principal_summary() */




/******************************************************
  Write coincidence summary stuff to stdout
 **********************************************************/
void	html_coincidence_summary (EWEventInfoStruct *pev, EWDBid bReview)
{

   char  		timestr[DATESTR23];
   char  		loc_timestr[DATESTR23];
	int			i, j;

	if (pev == NULL)
	{
		logit ("", "html_coincidence_summary: Invalid arguments passed in!  EXITTING!\n");
		exit(-1);
	}


/* Figure out extra things we need to print out 
 **********************************************/
   datestr23 (pev->CoincEvt.tCoincidence, timestr, DATESTR23 ); 
   datestr23_local (pev->CoincEvt.tCoincidence, loc_timestr, DATESTR23 );


   printf("<HR><strong>\n<pre>\n"
          "COINCIDENCE TRIGGER PARAMETERS\n"
          "_______________________________\n\n" );

   printf ("Coincidence Time             : %s   %s  UTC\n",
           strtok(timestr," "), &timestr[11] );
   printf ("LOCAL Coincidence Time       : %s   %s  %s\n\n",
           strtok(timestr," "), &loc_timestr[11], getenv ("TZ") );

   printf ("Database event ID            : %10u\n", pev->Event.idEvent );
   printf ("Author                       : %s\n", pev->CoincEvt.szHumanReadable);


   printf ("<strong>\nINDIVIDUAL CHANNEL TRIGGERS\n"
          "_________________________________________________\n\n"
          "Sta   Comp  Net  Loc          time UTC     \n"
          "                              hh:mm:sec     \n\n");


    for (i = 0; i < pev->iNumChans; i++)
    {
        for (j = 0; j < pev->pChanInfo[i].iNumTriggers; j++)
        {
            datestr23 (pev->pChanInfo[i].Triggers[j].tTrigger, timestr, DATESTR23);

            printf ("%-5s %3s %4s %4s   %s\n",
                        pev->pChanInfo[i].Station.Sta,
                        pev->pChanInfo[i].Station.Comp,
                        pev->pChanInfo[i].Station.Net,
                        pev->pChanInfo[i].Station.Loc,
                        timestr);
        }
    }


   printf ("</PRE>\n");

}


/******************************************************
  Function:    html_links2ora2rsec_gif()
  Purpose:     Write links to ora2rsecgif pages for the
               current event.  Use html buttons to link
               to the ora2rsec_gif pages.
  Parameters:    
      EventID: A database ID for a seismic event.  Used to 
               tell the ora2rsec_gif program which event to
               retrieve data for.
 
      NumDispOpt: how many display options are specified

      pDispOpt:  array of display options

  **********************************************************/
void  html_links2ora2rsec_gif(int EventID, int NumDispOpt, 
							TraceGifDisplayStruct *pDispOpt)
{
	int 	i;


	printf ("<HR>\n");
	printf ("<STRONG>\n<PRE>\n VIEW SEISMOGRAMS \n</PRE></STRONG>\n" );

	for (i = 0; i < NumDispOpt; i++)
	{
		if (pDispOpt[i].bUseSeparateWindow)
		{
			printf ("<A HREF=\"%s\?EventID=%u\" target=\"%s\">%s</A>\n", 
					pDispOpt[i].szProgramName, EventID,
					pDispOpt[i].szTargetName, pDispOpt[i].szDescription);
		}
		else
		{
			printf ("<A HREF=\"%s\?EventID=%u\">%s</A>\n", 
					pDispOpt[i].szProgramName, EventID,
					pDispOpt[i].szDescription);
		}
    
		printf ("<BR><BR>\n");
	}

}  /* End html_links2ora2rsec_gif() */

