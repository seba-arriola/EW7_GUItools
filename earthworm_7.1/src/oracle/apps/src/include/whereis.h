#include <stdlib.h>
#include <stdio.h>



typedef struct _EWDB_PlaceStruct
{
  char   szState[3];
  char   szPlaceName[70];
  char   szPlaceType[20];
  char   szCountry[30];
  double dLat;
  double dLon;
	int    iPopulation;
  int    iPlaceMajorType;
  int    iPlaceMinorType;
  int    idPlace;
}  EWDB_PlaceStruct;




#define CITY_TYPE_UNKNOWN 0
#define CITY_TYPE_CDP     5
#define CITY_TYPE_CITY    4
#define CITY_TYPE_LARGE_CITY 3
#define CITY_TYPE_METROPOLIS 2
#define CITY_TYPE_MEGOPOLIS  1
