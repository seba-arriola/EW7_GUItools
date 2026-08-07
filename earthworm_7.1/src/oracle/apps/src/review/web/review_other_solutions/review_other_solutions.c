
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: review_other_solutions.c,v 1.5 2002/09/10 17:29:00 lucky Exp $
 *
 *    Revision history:
 *    $Log: review_other_solutions.c,v $
 *    Revision 1.5  2002/09/10 17:29:00  lucky
 *    stable scaffold
 *
 *    Revision 1.4  2002/06/28 21:06:22  lucky
 *    Lucky's pre-departure checkin. Most changes probably had to do with bug fixes
 *    in connection with the GIOC scaffold.
 *
 *    Revision 1.3  2002/05/28 19:34:48  lucky
 *    Added header and footer tag text
 *
 *    Revision 1.2  2002/05/28 17:27:23  lucky
 *    *** empty log message ***
 *
 *    Revision 1.1  2002/03/29 17:57:04  lucky
 *    Initial revision
 *
 *    Revision 1.1  2002/03/22 20:03:33  lucky
 *    Initial revision
 *
 *
 */
  
/*****************************************************************

 *  Given an eventid, retrieves the list of matching origins from 
 *  the database. It then displays the origins allowing the user to 
 *  chose one of them for review.
 * 
*****************************************************************/


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>


/* include earthworm headers */
#include <earthworm.h>
#include <time_ew.h>
#include <webparse.h>

/* include our own header file */
#include <review.h>

main ()
{

	WebOptionsStruct 			WebParams;
	char						*configfile = "../params/review_event.d";
	EWDBid						idpOrig, idpMag, idpMech;
	EWDB_OriginStruct			*pOrigins;
	EWDB_MagStruct				*pMags;
	int							NumOrigFound, NumOrigRetr;
	int							NumMagFound, NumMagRetr;
	int							indPrefOrig, i, j, m;
	char						pTimeBuffer[256];
	time_t						TempTime;

	/* Read the configuration file (path hardcoded relative to executable)
	 *********************************************************************/
	ReadConfig (configfile);

    logit_init ("review_other_solutions", 1, MAX_BYTES_PER_EQ, 1);
	logit ("t", "review_other_solutions starting.\n"); 

	/* initialize Web Parameters to all zeros */
	memset (&WebParams, 0, sizeof(WebOptionsStruct));
	if (Webparse_GetAndProcessWebParams((void *)(&WebParams)) != 0)
	{
		printf ("Call to Webparse_GetAndProcessWebParams failed.\n");
		exit (-1);
	}


    /* Send html header back to web server
     *************************************/
    printf("Content-type: text/html\n\n");
    printf("<html>\n");

    printf ("<HEAD><TITLE>Origins for %d</TITLE></HEAD>\n", WebParams.idEvent );

	html_header (BackgroundColor, HeaderLogo, HeaderTag);

    printf("<center>\n");
    printf("<H2><FONT COLOR=red>All Solutions for Event %d</FONT></H2>\n", 
													WebParams.idEvent);


    /* Connect to the DB */
    if (ewdb_api_Init (DBuser, DBpassword, DBservice ) != 0 )
    {
        html_logit ("", "Trouble connecting to database; exiting!\n" );
        html_break();
		goto shutdown;
    }


	/* Get preferred stuff */
	if (ewdb_api_GetPreferredSummaryInfo (WebParams.idEvent, 
				&idpOrig, &idpMag, &idpMech) != EWDB_RETURN_SUCCESS)
	{
		html_logit ("", "Call to ewdb_api_GetPreferredSummaryInfo failed.\n");
        html_break();
		goto shutdown;
    }


	/* Get other origins */
	if ((pOrigins = (EWDB_OriginStruct *) malloc (MAX_ORIGINS_PER_EVENT *
										sizeof (EWDB_OriginStruct))) == NULL)
	{
		html_logit ("", "Could not malloc %d Origins.\n", MAX_ORIGINS_PER_EVENT);
        html_break();
		goto shutdown;
    }

	if (ewdb_api_GetOriginsForEvent (WebParams.idEvent, pOrigins, 
			&NumOrigFound, &NumOrigRetr, MAX_ORIGINS_PER_EVENT) != EWDB_RETURN_SUCCESS)
	{
		html_logit ("", "Call to ewdb_api_GetOriginsForEvent failed.\n");
        html_break();
		goto shutdown;
    }

	indPrefOrig = -1;
	for (i = 0; i < NumOrigRetr; i++)
	{
		if (pOrigins[i].idOrigin == idpOrig)
			indPrefOrig = i;
	}

logit ("", "Retrieved %d out of %d Origins, pref is %d\n", 
	NumOrigRetr, NumOrigFound, indPrefOrig);


	if (indPrefOrig < 0)
	{
		html_logit ("", "Could not find preferred Origin.\n");
        html_break();
		goto shutdown;
    }

	printf ("<PRE><STRONG>Retrieved %d Origins; Prefered Origin is highlighted.</STRONG>\n\n", NumOrigRetr);

	printf("To enter quick review for the solution, click on the link in the Origin column.\n\n");

	printf ("</PRE>\n");

    /*  Write the table header */
    printf("<table border=1> <tr bgcolor=#2222AA VALIGN=TOP align=center>\n");

    printf("<td valign=center><strong><font size=\"+1\" color=#FFFFFF>Origin</td>\n");
    printf("<td valign=center><strong><font size=\"+1\" color=#FFFFFF>Origin Time</td>\n");
    printf("<td valign=center><strong><font size=\"+1\" color=#FFFFFF>Lat</td>\n");
    printf("<td valign=center><strong><font size=\"+1\" color=#FFFFFF>Lon</td>\n");
    printf("<td valign=center><strong><font size=\"+1\" color=#FFFFFF>Depth</td>\n");
    printf("<td valign=center><strong><font size=\"+1\" color=#FFFFFF>Mags</td>\n");
    printf("<td valign=center><strong><font size=\"+1\" color=#FFFFFF>Source</td>\n");

    printf("</tr>\n");

	if ((pMags = (EWDB_MagStruct *) malloc (MAX_MAGS_PER_ORIGIN *
										sizeof (EWDB_MagStruct))) == NULL)
	{
		html_logit ("", "Could not malloc %d Mags.\n", MAX_MAGS_PER_ORIGIN);
        html_break();
		goto shutdown;
    }

	for (i = 0; i < NumOrigRetr; i++)
	{
		if (ewdb_api_GetMagsForOrigin (pOrigins[i].idOrigin, pMags, &NumMagFound, 
					&NumMagRetr, MAX_MAGS_PER_ORIGIN) != EWDB_RETURN_SUCCESS)
		{
			html_logit ("", "Call to ewdb_api_GetMagsForOrigin failed.\n");
	        html_break();
			goto shutdown;
   		}

		if (i == indPrefOrig)
			printf("<TR BGCOLOR=\"silver\" ALIGN=CENTER>\n");
		else
			printf("<TR ALIGN=CENTER>\n");


		/* origin id */
		printf ("<TD><A HREF=\"review_event?EventID=%u?OriginID=%u?Action=%d\""
				"target=\"Event Review\"> %d </TD>\n", 
					WebParams.idEvent, pOrigins[i].idOrigin, ACT_GETFROMDB, pOrigins[i].idOrigin);


		TempTime = (time_t) (pOrigins[i].tOrigin);
		printf ("<TD>%s</TD>\n", EWDB_ttoa (&TempTime, pTimeBuffer));

		/* Write the location fields */
		printf ("<TD> %2.2f </TD>\n", pOrigins[i].dLat);
		printf ("<TD> %3.2f </TD>\n", pOrigins[i].dLon);
		printf ("<TD> %1.1f </TD>\n", pOrigins[i].dDepth);

		/* Write the magnitude fields */
		if (NumMagRetr > 0)
		{
			printf ("<TD>");
			for (m = 0; m < NumMagRetr; m++)	
				printf ("%4.1f%s\n", pMags[m].dMagAvg, MagNames[pMags[m].iMagType]);
			printf ("</TD>");
		}
		else
			printf ("<TD>---</TD>\n");



		/* write the source */
		printf ("<TD> %s </TD>\n", pOrigins[i].sSource);

		/* close the row */
		printf ("</TR>\n");


	} /* loop over retrieved origins */

	/* close the table */
	printf ("</TABLE>\n");

	printf ("<BR><BR>\n");


shutdown:
    ewdb_api_Shutdown();
    html_trailer (WebHost, FooterLogo, FooterTag);
    logit ("t", "review_other_solutions: terminating\n" );
	exit (0);

}
