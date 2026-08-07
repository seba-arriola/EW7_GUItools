
#include <string.h>
#include <ewdb_ora_api.h>


#define CITY_STATE_START 0
#define CITY_NAME_START  9
#define CITY_POPULATION_START 73
#define CITY_LAT_START 143
#define CITY_LON_START 153

#define CITY_STATE_LEN 2
#define CITY_NAME_LEN  64
#define CITY_POPULATION_LEN 9
#define CITY_LAT_LEN 10
#define CITY_LON_LEN 10


#define PLACE_TYPE_GENERIC_CITY 1


int main(int argc, char ** argv)
{
 
	char 				szLine[512];
	FILE 				*fp;
	EWDB_PlaceStruct 	Place;
	int 				iGoodCities=0;
	int 				iRetCode, lineno;

	EWDB_PlaceStruct	Min;
	EWDB_PlaceStruct	Max;
	EWDB_PlaceStruct	*pPlaces;
	int					found, retr, BufLen, i;

	if(argc < 4)
	{
		printf ("Usage:  load_placelist <city file> <db_user> <db_passwd> <db_sid>\n");
		return (-1);
	}

    logit_init ("load_placelist", 0, 1024, 1);

	if ((fp = fopen(argv[1], "r")) == NULL)
	{
		logit ("e", "Error opening input file %s\n", argv[1]);
		return (-1);
	}

	if (ewdb_api_Init (argv[2], argv[3], argv[4]) != EWDB_RETURN_SUCCESS)
	{
		logit ("e", "Call to ewdb_api_Init() failed\n");
		return(-1);
	}


#ifdef TEST
/* Test retrieval routine */

BufLen = 10;
if ((pPlaces = (EWDB_PlaceStruct *) malloc (BufLen * sizeof (EWDB_PlaceStruct))) == NULL)
{
	logit ("e", "Could not malloc places buffer.\n");
	return EW_FAILURE;
}

memset (&Min, 0, sizeof (EWDB_PlaceStruct));
memset (&Max, 0, sizeof (EWDB_PlaceStruct));

Min.dLat = 36.40;
Max.dLat = 45.48;

Min.dLon = -114.50;
Max.dLon =  -108.40;

Min.iPopulation = -1;
Max.iPopulation = -1;

Min.iPlaceMajorType = -1;
Min.iPlaceMinorType = -1;


logit ("e", "Calling ewdb_api_GetPlaceList\n");
if (ewdb_api_GetPlaceList (pPlaces, BufLen, &Min, &Max, &retr, &found) != EWDB_RETURN_SUCCESS)
{
	logit ("e", "Call to ewdb_api_GetPlaceList failed.n");
	return EW_FAILURE;
}

logit ("e", "Retrieved %d out of %d places\n", retr, found);
for (i = 0; i < retr; i++)
	logit ("e", "Place %d:  %s, %s\t%15d\t(%5.3f,%7.3f, %6.0f) (%d,%d)\n", i,
pPlaces[i].szPlaceName, pPlaces[i].szState, pPlaces[i].iPopulation, 
pPlaces[i].dLat, pPlaces[i].dLon, pPlaces[i].dElev,
pPlaces[i].iPlaceMajorType, pPlaces[i].iPlaceMinorType);

return 0;
#endif TEST


	lineno = 0;
	while(fgets(szLine, sizeof(szLine),fp))
	{
		lineno = lineno + 1;
logit ("e", "Processing line %d\n", lineno);

		memset (&Place, 0, sizeof(EWDB_PlaceStruct));

		strtok (szLine,"\n");  /* make sure we take out the newline */

		/* 
		 * Pass everything into an encapsulated routine.  
		 * This will allow us to drop in various formats later.		
		 */
		
		if (EWDB_ParsePlaceLine_GNISCol (&Place, szLine) != EW_SUCCESS)
		{
			logit ("e", "Call to EWDB_ParsePlaceLine_GNISCol failed for line %d\n", lineno);
			continue;
		}


		if (ewdb_api_CreatePlace(&Place) != EWDB_RETURN_SUCCESS)
		{
			logit ("e", "## FAILURE ## %s, %s\t%15d\t(%5.3f,%7.3f, %6.0f) (%d,%d)\n",
				Place.szPlaceName, Place.szState, Place.iPopulation, 
				Place.dLat, Place.dLon, Place.dElev,
				Place.iPlaceMajorType, Place.iPlaceMinorType);
		}
		else
		{
			logit ("e", "## SUCCESS ## %s, %s\t%15d\t(%5.3f,%7.3f, %6.0f) (%d,%d)\n",
				Place.szPlaceName, Place.szState, Place.iPopulation, 
				Place.dLat, Place.dLon, Place.dElev,
				Place.iPlaceMajorType, Place.iPlaceMinorType);
    	}

	}  /* end while not done reading file */
    

	fclose (fp);

	return(0);

}

