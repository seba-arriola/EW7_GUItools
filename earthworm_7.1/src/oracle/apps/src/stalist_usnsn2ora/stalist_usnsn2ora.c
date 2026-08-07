
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: stalist_usnsn2ora.c,v 1.12 2004/03/31 18:52:24 davidk Exp $
 *
 *    Revision history:
 *     $Log: stalist_usnsn2ora.c,v $
 *     Revision 1.12  2004/03/31 18:52:24  davidk
 *     modified to utilize BHZ as the default component, and to include
 *     E and N components if a Z component is defined.  This way we get lat/lon
 *     for all components, not just Z.
 *
 *     Revision 1.11  2002/11/03 21:31:43  lombard
 *     Corrected mode of fopen() call.
 *
 *     Revision 1.10  2001/07/01 21:55:36  davidk
 *     Cleanup of the Earthworm Database API and the applications that utilize it.
 *     The "ewdb_api" was cleanup in preparation for Earthworm v6.0, which is
 *     supposed to contain an API that will be backwards compatible in the
 *     future.  Functions were removed, parameters were changed, syntax was
 *     rewritten, and other stuff was done to try to get the code to follow a
 *     general format, so that it would be easier to read.
 *
 *     Applications were modified to handle changed API calls and structures.
 *     They were also modified to be compiler warning free on WinNT.
 *
 *     Revision 1.9  2001/05/15 02:15:51  davidk
 *     Moved functions around between the apps, DB API, and DB API INTERNAL
 *     levels.  Renamed functions and files.  Added support for amplitude
 *     magnitude types.  Reformatted makefiles.
 *
 *     Revision 1.8  2001/02/28 17:29:10  lucky
 *     Massive schema redesign and cleanup.
 *
 *     Revision 1.7  2000/09/25 15:17:49  lucky
 *     Return values from neic2scn changed to EW_SUCCESS/EW_FAILURE
 *
 *     Revision 1.6  2000/08/09 16:43:59  lucky
 *     Lint cleanup
 *
 *     Revision 1.5  2000/03/30 16:25:24  davidk
 *     changed the code to use the new External station functions instead
 *     of USNSN station functions.  Changed all of the return codes
 *     from EW_XXXX to EWDB_DB_RETURN_XXX so that I could get rid of earthworm.h.
 *     Removed some unused variables.
 *
 *     Revision 1.4  2000/02/23 19:18:41  lucky
 *     *** empty log message ***
 *
 *     Revision 1.3  2000/02/23 19:17:14  lucky
 *     Updated to use the new format of the NEIC station files. Also, looks at the
 *     cheater file to determine the full S-C-N code.
 *
 *     Revision 1.2  2000/02/15 19:47:43  lucky
 *     *** empty log message ***
 *
 *     Revision 1.1  1999/11/09 17:03:00  lucky
 *     Initial revision
 *
 *
 */

/* 

     Usage: stalist_usnsn2ora station_file DBuser DBpwd DBsid 

   station_file:  file containing USNSN station information, in the
                  following format:

    (OLD FORMAT)   AAE    90145.0N 384556.0E 2442.0Addis Ababa
    (NEW FORMAT)   AAE    9.0291N  38.7655E 2442 IU Addis Ababa, Ethiopia


     The format of this file is described in README.neic_format

    DBuser, DBpwd, 
    and DBsid:     stuff necessary to talk to the database.
                   If you don't know what those are, you should
                   probably not be running this program.
                       

    This program reads the NEIC station list and inserts the 
    data into the database. If a particular station is already
    in the database, this program will update all non-NULL values
    with the data from the station list file.

*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <earthworm.h>
#include <ewdb_ora_api.h>
#include <ewdb_apps_utils.h>
#include "stalist_usnsn2ora.h"


#define		STALIST_EOF			100

/* Column positions of various fields from NSN format file */
#define STA_POS			0
#define STA_LEN			5
#define LAT_POS			6
#define LAT_LEN			7
#define LAT_ORIENT		13

#define LON_POS			15
#define LON_LEN			8
#define LON_ORIENT		23

#define ELEV_POS		25
#define ELEV_LEN		4

#define NET_POS			30
#define NET_LEN			2

#define DESC_POS		33
#define DESC_LEN		60


#define MAX_STALIST_LINE 100

static  int             nSta;
static  NEIC2SCN        *neic2scn;


char DBuser[50];
char DBpassword[50];
char DBservice[50];

int 	main (int argc, char **argv)
{

	char					nsn_sta_name[10];
	char					sta[10];
	char					comp[10];
	char					net[10];
	EWDB_External_StationStruct 		Station;
	FILE 					*fptr;
	char					*line;
	char					tmps[20];
	int						lineno;
	char					*tmpline;
	char					*tmpcurr;
	char                                    cTemp;

	if (argc < 6 )
	{
		printf ("usage:  stalist_usnsn2ora <STATION_LIST_FILENAME>  "
					"<NEIC_CHEATER_FILENAME> <DBUSER> <DBPWD> <DBSID>\n");
		return EWDB_RETURN_FAILURE;
	}

	/* else */
	strcpy (DBuser, argv[3]);
	strcpy (DBpassword, argv[4]);
	strcpy (DBservice, argv[5]);

	logit_init ("stalist_usnsn2ora", 27, 1024, 1); 



	if ((fptr = fopen (argv[1], "r")) == NULL) 
	{
		logit ("e", "Could not open station file %s!\n", argv[1]);
		return EWDB_RETURN_FAILURE;
	}

	/* Read in the cheater file */
	if (GetNEICStaList (&neic2scn, &nSta, argv[2]) != EW_SUCCESS)
	{
		logit ("e", "Call to GetNEICStaList failed\n");
		return EWDB_RETURN_FAILURE;
	}

  
	logit ("e", "Opening DB connection with %s, %s, %s\n",
						DBuser, DBpassword, DBservice);
  
	if (ewdb_api_Init (DBuser, DBpassword, DBservice) !=
										EWDB_RETURN_SUCCESS)
	{
		logit ("e", "Call to ewdb_api_Init failed.\n");
		return EWDB_RETURN_FAILURE;
	}

	if ((line = malloc (MAX_STALIST_LINE * sizeof (char))) == NULL)
	{
		logit ("e", "Malloc failed!\n");
		return EWDB_RETURN_FAILURE;
	}

	lineno = 0;
	while (GetNextLine (&lineno, line, fptr) != STALIST_EOF)
	{
    memset(&Station,0,sizeof(Station));

		tmpline = line;

		/* copy station name */
		strncpy (nsn_sta_name, tmpline, STA_LEN);
		nsn_sta_name[STA_LEN] = '\0';

		/* copy network code */
		strcpy (Station.Station.Net, nsn_sta_name);


    /* copy latitude orientation */
    cTemp = tmpline[LAT_ORIENT];

		/* copy latitude */
		tmpcurr = &(tmpline[LAT_POS]);
		/* copy latitude */
		strncpy (tmps, tmpcurr, LAT_LEN);
		tmps[LAT_LEN] = '\0';


		Station.Station.Lat = (float)atof(tmps);

                if(cTemp=='S')
                  Station.Station.Lat=(float)(0.0-Station.Station.Lat);


                /* copy longitude orientation */
                cTemp = tmpline[LON_ORIENT];

		/* copy longitude */
		tmpcurr = &(tmpline[LON_POS]);
		strncpy (tmps, tmpcurr, LON_LEN);
		tmps[LON_LEN] = '\0';

		Station.Station.Lon = (float)atof(tmps);

                if(cTemp=='W')
                  Station.Station.Lon=(float)(0.0-Station.Station.Lon);


		/* copy elevation */
		tmpcurr = &(tmpline[ELEV_POS]);
		strncpy (tmps, tmpcurr, ELEV_LEN);
		tmps[ELEV_LEN] = '\0';
		Station.Station.Elev = (float) atof (tmps);


		/* copy network code */
		tmpcurr = &(tmpline[NET_POS]);
		strncpy (tmps, tmpcurr, NET_LEN);
		tmps[NET_LEN] = '\0';
		strcpy (Station.Station.Net, tmps);

		/* copy description/location string */
			
		/* compute the length of the description string */
		tmpcurr = &(tmpline[DESC_POS]);
		strcpy (Station.Description, tmpcurr);

		
		/* Check the cheater table for the full S-C-N */
		if (MatchNeic2SCN (nsn_sta_name, sta, comp, net, neic2scn, nSta) != EW_SUCCESS)
		{
			logit ("e", "Call to MatchNeic2SCN failed.\n");
			return EWDB_RETURN_FAILURE;
		}
		

		/* 
			Make sure that the network code from the cheater table
		   	matches what is in the station file
		 */


		/************************* STORY *************************

		 We are noticing that the network codes in the NEIC's
		 automatic station list and our cheater file do not always
		 agree. Barbara is trying to figure out which station code
		 is correct.

		 In the meantime, below are two sections of code. 
		 To use the network codes from the cheater table, define 
		 the variable USE_CHEATER.  To use NEIC's network codes, make
		 sure that USE_CHEATER is not defined.
		 Then, recompile and run stalist_usnsn2ora.

		*********************** END OF STORY ********************/

		strcpy (Station.Station.Sta, sta);
		/* DK  We want to do all components with the available information.
     ******************************************************************/
    if(strcmp(comp,"???")== 0)
    {
      /* make BHZ the default, it MIGHT be wrong, but it's always ATLEAST as
         good as ???*/
      strcpy(comp,"BHZ");
    }
    strcpy (Station.Station.Comp, comp);

		/* If network codes are not different, AND the station 
		   is listed in the cheater table .. */
		if ((strcmp (net, Station.Station.Net) != 0) && 
							(strcmp (net, "???") != 0))
		{
			logit ("e", "MISTMATCH: NEIC station %s (%s-%s-%s) has network code %s in the cheater table.\n",
							nsn_sta_name, Station.Station.Sta, Station.Station.Comp, Station.Station.Net, net);

#define USE_CHEATER
#ifdef USE_CHEATER

			strcpy (Station.Station.Net, net);

#endif USE_CHEATER

		}

		logit ("e", "Inserting station %d: %s-%s-%s, %0.2f, %0.2f, %0.1f, %s\n", 
						lineno, Station.Station.Sta, Station.Station.Comp, Station.Station.Net, 
						Station.Station.Lat, Station.Station.Lon,
						Station.Station.Elev, Station.Description);

		Station.Station.Azm = 0.0;
		Station.Station.Dip = 0.0;
	

		/* Insert into database */
		if (ewdb_api_CreateOrAlterExternalStation (&Station) 
												!= EWDB_RETURN_SUCCESS)
		{
			logit ("e", "Call to ewdb_api_CreateOrAlterExternalStation failed \n");
			return EWDB_RETURN_FAILURE;
		}

		if (Station.StationID < 0)
		{
			logit ("e", "ewdb_api_CreateOrAlterExternalStation returned ERROR, StationID is %d\n", 
								Station.StationID);
			return EWDB_RETURN_FAILURE;
		}

		logit ("", "Returned from ewdb_api_CreateOrAlterExternalStation; StationID is %d\n", 
									Station.StationID);

    /*  If this is a Z component, do N and E components too */
    if(Station.Station.Comp[2] == 'Z' && Station.StationID > 0)
    {
      Station.Station.Comp[2] = 'N';
      Station.StationID = 0;
      /* Insert into database */
      if (ewdb_api_CreateOrAlterExternalStation (&Station) 
        != EWDB_RETURN_SUCCESS)
      {
        logit ("e", "Call to ewdb_api_CreateOrAlterExternalStation failed \n");
        return EWDB_RETURN_FAILURE;
      }
      
      Station.Station.Comp[2] = 'E';
      Station.StationID = 0;
      /* Insert into database */
      if (ewdb_api_CreateOrAlterExternalStation (&Station) 
        != EWDB_RETURN_SUCCESS)
      {
        logit ("e", "Call to ewdb_api_CreateOrAlterExternalStation failed \n");
        return EWDB_RETURN_FAILURE;
      }
      
    }

  }  /* end while next line */


	return 0;  /* DK Changed from failure 031804 */

}  /* end main() */




int		GetNextLine (int *lineno, char *line, FILE *fp)
{

	int		i;
	char	c;
	
	if ((lineno == NULL) || (line == NULL) || (fp == NULL))
	{
		logit ("e", "Null parameters passed in\n");
		return EWDB_RETURN_FAILURE;
	}


	i = 0;
	while (1)
	{

		if ((c = fgetc (fp)) == EOF)
		{
			*lineno = *lineno + 1;
			return STALIST_EOF;
		}
		else
		{
			line[i] = c;

			if (c == '\n')
			{
				line[i] = '\0';
				*lineno = *lineno + 1;
				return EWDB_RETURN_SUCCESS;
			}

			i = i + 1;
			if (i > MAX_STALIST_LINE)
			{
				logit ("e", "MAX_LINE exceeded on line %d.\n", *lineno);
				return EWDB_RETURN_FAILURE;
			}
		}
	}
}

