/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: nearest_town_api_utils.c,v 1.1 2002/10/08 18:14:52 lucky Exp $
 *    Revision history:
 *
 *    $Log: nearest_town_api_utils.c,v $
 *    Revision 1.1  2002/10/08 18:14:52  lucky
 *    Initial revision
 *
 *
 */



#include <stdio.h>
#include <string.h>
#include <ewdb_apps_utils.h>

int   DEBUG;

static 	int 	DecodeGNISLocation (char *locline, double *lat, double *lon);

/********************** EWDB_ParsePlaceLine_GNISCol ***********************

	Fill the EWDB_PlaceStruct structure from the string containing
	one line from the Place file formatted according to the GNIS
	columnar record format used for populated places listing.  Source
	at http://geonames.usgs.gov (last visited, Sept 27, 2002)


  		By Lucky Vidmar Fri Sep 27 20:57:31 GMT 2002

********************** EWDB_ParsePlaceLine_GNISCol ***********************/

int		EWDB_ParsePlaceLine_GNISCol (EWDB_PlaceStruct *pPlace, char *line)
{

	int		i;
	char 	szBuffer[512];
	char	*szTemp; 
	char	*szTemp3;


	if ((pPlace == NULL) || (line == NULL))
	{
		logit ("", "Invalid arguments passed in.\n");
		return EW_FAILURE;
	}
    

	memset (szBuffer, 0, sizeof(szBuffer));

	/* State Alpha Code */
    memcpy (pPlace->szState, line, 2);
	pPlace->szState[2] = '\0';


	/* 
	 * Feature (place) name:
     *  This is more complex than it looks: Name consists of
     *  one or more words, and we want to strip the trailing spaces.
	 *  So, read in the whole thing, then march backwards until we find
	 *  a non-space.
     */

    /* grab the enitre name */
	memcpy (szBuffer, (line+4), 35);
	szBuffer[35] = '\0';

	i = 34;
	while ((i >= 0) && (szBuffer[i] == ' '))
		i = i - 1;

	if (i < 0)
	{
		logit ("", "Empty place name string <%s>\n", szBuffer);
		return EW_FAILURE;
	}

	szBuffer[i+1] = '\0';
	strcpy (pPlace->szPlaceName, szBuffer);
		
    
	/* Place type */
    memcpy (pPlace->szPlaceType, (line+40), 10);
	pPlace->szPlaceType[10] = '\0';

	/* Country */
	strcpy (pPlace->szCountry, "USA");


	/* County: must go through the same nonsense as with place name  */
	memcpy (szBuffer, (line+51), 20);
	szBuffer[20] = '\0';

	i = 19;
	while ((i >= 0) && (szBuffer[i] == ' '))
		i = i - 1;

	if (i < 0)
	{
		logit ("", "Empty county name string <%s>\n", szBuffer);
		return EW_FAILURE;
	}

	szBuffer[i+1] = '\0';
	strcpy (pPlace->szCounty, szBuffer);
		
    
	/* Location */
    memcpy (szBuffer, (line+72), 15);
	szBuffer[15] = '\0';

	if (DecodeGNISLocation (szBuffer, &pPlace->dLat, &pPlace->dLon) != EW_SUCCESS)
	{
		logit ("", "Call to DecodeGNISLocation failed for <%s>.\n", szBuffer);
		return EW_FAILURE;
	}

	/* Elevation */
    memcpy (szBuffer, (line+110), 5);
	szBuffer[5] = '\0';
	pPlace->dElev = atof (szBuffer);

	/* Population */
    memcpy (szBuffer, (line+116), 8);
	szBuffer[8] = '\0';
	pPlace->iPopulation = atoi (szBuffer);

    pPlace->iPlaceMajorType = EWDB_PLACE_TYPE_GENERIC_CITY;
    
	if (pPlace->iPopulation < 10000)
		pPlace->iPlaceMinorType = EWDB_CITY_TYPE_CITY;
	else
		pPlace->iPlaceMinorType = EWDB_CITY_TYPE_LARGE_CITY;

	return EW_SUCCESS;

}



static 	int 	DecodeGNISLocation (char *locline, double *lat, double *lon)
{

	double	deg, min, sec, sign;
	char	sztmp[5];

	if ((locline == NULL) || (lat == NULL) || (lon == NULL))
	{
		logit ("", "Invalid arguments passed in.\n");
		return EW_FAILURE;
	}


	/* lat */
	memcpy (sztmp, locline, 2);
	sztmp[2] = '\0';
	deg = atof(sztmp);

	memcpy (sztmp, (locline+2), 2);
	sztmp[2] = '\0';
	min = atof(sztmp);

	memcpy (sztmp, (locline+4), 2);
	sztmp[2] = '\0';
	sec = atof(sztmp);

	if (locline[6] == 'N')
		sign = 1.0;
	else 
		sign = -1.0;

	*lat = sign * (deg + (min / 60.0) + (sec / 3600.0));


	/* lon */
	memcpy (sztmp, (locline+7), 3);
	sztmp[3] = '\0';
	deg = atof(sztmp);

	memcpy (sztmp, (locline+10), 2);
	sztmp[2] = '\0';
	min = atof(sztmp);

	memcpy (sztmp, (locline+12), 2);
	sztmp[2] = '\0';
	sec = atof(sztmp);

	if (locline[14] == 'W')
		sign = -1.0;
	else 
		sign = 1.0;

	*lon = sign * (deg + (min / 60.0) + (sec / 3600.0));

	return EW_SUCCESS;

} 


