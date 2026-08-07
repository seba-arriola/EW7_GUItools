
#include <string.h>
#include "whereis.h"
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


int ewdb_api_CreatePlace(EWDB_PlaceStruct * pPlace);

int main(int argc, char ** argv)
{
 
 
  char szLine[512];
  char szBuffer[512];
  FILE * fp;
  EWDB_PlaceStruct * pPlace;
  char * szTemp, *szTemp3;
  EWDB_PlaceStruct Place;
  int iGoodCities=0;
  int iRetCode;

  pPlace = &Place;

  if(argc < 4)
  {
    printf("Usage:  placelist_city2ora <city file> <db_user> <db_passwd> <db_sid>\n");
    return(-1);
  }

  fp = fopen(argv[1],"r");

  if(!fp)
  {
    printf("Error opening input file %s\n",argv[1]);
    return(-1);
  }

  logit_init("placelist_city2ora",0,1024,1);

  iRetCode = ewdb_api_Init(argv[2], argv[3], argv[4]);

  if(iRetCode != EWDB_RETURN_SUCCESS)
  {
    printf("Error %d while calling ewdb_api_Init()\n");
    return(-1);
  }

  while(fgets(szLine, sizeof(szLine),fp))
  {
    memset(pPlace, 0, sizeof(EWDB_PlaceStruct));
    memset(szBuffer, 0, sizeof(szBuffer));

    strtok(szLine,"\n");  /* make sure we take out the newline */
    
    memcpy(pPlace->szState, &szLine[CITY_STATE_START], 2);
    
    /* the city name is a pain in the butt.  It is the simple
    name of a city (possibly including spaces), with the city
    type appended to the end.  So you have to read it word for
    word adding it to a string, and then you have to throw out
    the last word, because it's not part of the city name.
    */
    
    // grab the enitre name
    memcpy(szBuffer, &szLine[CITY_NAME_START], 64);
    
    // initialize poiters -- set szTemp to NULL and szTemp3 to NOT NULL
    szTemp  = NULL;
    szTemp3 = strtok(szBuffer," ");
    
    
    // loop until we hit the end of the string, save up to the current(last) word
    while(szTemp3)
    {
      // if we have a word and it is not the last word, then save it
      if(szTemp && szTemp3)
      {
        strcat(pPlace->szPlaceName, szTemp);
        strcat(pPlace->szPlaceName, " ");
      }
      
      // copy the current word to a savebuffer
      szTemp = szTemp3;
        
      // get the next word
      szTemp3 = strtok(NULL," ");
    }  // while not at the end of the name
    
    // remove the trailing space
    if(pPlace->szPlaceName[0] != 0) // not a blank string
      pPlace->szPlaceName[strlen(pPlace->szPlaceName)-1] = 0;
    
    // population
    memcpy(szBuffer, &szLine[CITY_POPULATION_START], CITY_POPULATION_LEN);
    szBuffer[CITY_POPULATION_LEN] = 0;
    pPlace->iPopulation = atoi(szBuffer);
    
    // Lat
    memcpy(szBuffer, &szLine[CITY_LAT_START], CITY_LAT_LEN);
    szBuffer[CITY_LAT_LEN] = 0x00;
    pPlace->dLat = atof(szBuffer);
    
    // Lon
    memcpy(szBuffer, &szLine[CITY_LON_START], CITY_LON_LEN);
    szBuffer[CITY_LON_LEN] = 0x00;
    pPlace->dLon = atof(szBuffer);
    
    // EOL
    
    
    pPlace->iPlaceMajorType = PLACE_TYPE_GENERIC_CITY;
    
    if(pPlace->iPopulation == 0)
      pPlace->iPlaceMinorType = CITY_TYPE_UNKNOWN;
    else if(pPlace->iPopulation < 10000) 
      pPlace->iPlaceMinorType = CITY_TYPE_CDP;
    else if(pPlace->iPopulation < 100000) 
      pPlace->iPlaceMinorType = CITY_TYPE_CITY;
    else if(pPlace->iPopulation < 250000) 
      pPlace->iPlaceMinorType = CITY_TYPE_LARGE_CITY;
    else if(pPlace->iPopulation < 1000000) 
      pPlace->iPlaceMinorType = CITY_TYPE_METROPOLIS;
    else
      pPlace->iPlaceMinorType = CITY_TYPE_MEGOPOLIS;
    // end if;

    strcpy(pPlace->szCountry,"USA");
    iRetCode = ewdb_api_CreatePlace(pPlace);
    if(iRetCode == EWDB_RETURN_SUCCESS)
    {
      iGoodCities++;
      printf("SUCCESS - %s, %s\n",
           pPlace->szPlaceName, pPlace->szState);
      /* printf("%s,%s\t\t%15d\t(%5.3f,%7.3f) (%d,%d - %s)\n",
           pPlace->szPlaceName, pPlace->szState, pPlace->iPopulation, 
           pPlace->dLat, pPlace->dLon,
           pPlace->iPlaceMajorType, pPlace->iPlaceMinorType, pPlace->szPlaceType);
       */
    }
    else
    {
      printf("## FAILURE ## %s, %s\t%15d\t(%5.3f,%7.3f) (%d,%d - %s)\n",
           pPlace->szPlaceName, pPlace->szState, pPlace->iPopulation, 
           pPlace->dLat, pPlace->dLon,
           pPlace->iPlaceMajorType, pPlace->iPlaceMinorType, pPlace->szPlaceType);
    }

  }  // end while(!feof)
    

  return(0);
}