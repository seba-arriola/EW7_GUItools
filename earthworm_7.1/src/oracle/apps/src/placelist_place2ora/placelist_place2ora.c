
#include <string.h>
#include "whereis.h"
#include <ewdb_ora_api.h>


#define PLACE_TYPE_NOISE_SITE 6  
#define PLACE_TYPE_STRUCTURE  5

#define NOISE_SITE_AIRPORT  1
#define STRUCTURE_BRIDGE    1
#define STRUCTURE_DAM       2
#define STRUCTURE_TUNNEL    3
#define NOISE_SITE_MINE     5
#define NOISE_SITE_OILFIELD 6

int ewdb_api_CreatePlace(EWDB_PlaceStruct * pPlace);


int main(int argc, char ** argv)
{
 
 
  char szLine[512];
  char szBuffer[512];
  FILE * fp;
  EWDB_PlaceStruct * pPlace;
  char * szField;
  EWDB_PlaceStruct Place;
  int iGoodCities=0;
  int iRetCode;
  int bIgnoreLine, bLineReadOK;
  int i;
  char * szNextField;

  pPlace = &Place;

  if(argc < 4)
  {
    printf("Usage:  placelist_place2ora <place file> <db_user> <db_passwd> <db_sid>\n");
    return(-1);
  }

  fp = fopen(argv[1],"r");

  if(!fp)
  {
    printf("Error opening input file %s\n",argv[1]);
    return(-1);
  }

  logit_init("placelist_place2ora",0,1024,1);

  iRetCode = ewdb_api_Init(argv[2], argv[3], argv[4]);

  if(iRetCode != EWDB_RETURN_SUCCESS)
  {
    printf("Error %d while calling ewdb_api_Init()\n");
    return(-1);
  }

  while(fgets(szLine, sizeof(szLine),fp))
  {
    memset(pPlace, 0, sizeof(EWDB_PlaceStruct));
    
    strtok(szLine,"\n");  /* make sure we take out the newline */
    
    bIgnoreLine = FALSE;
    
    szField = strtok(szLine,",");
    
    for(i=1; szField != NULL; i++)
    {
      
      /* because we are running multiple interleaved strtok() 
         calls on multiple different strings, we need to keep
         track of incrementing pointers for strtok() on our
         own.  (We can't call strtok(NULL,x), because we are 
         mixing calls on different strings)
       */
      szNextField = szField + strlen(szField) + 1;

      switch(i)
      {
      case 1:
        /* first remove quotes("") */
        strcpy(szBuffer, szField);
        szField = strtok(szBuffer,"\"");
        strncpy(pPlace->szState, szField, 2);
        break;
      case 2:
        /* first remove quotes("") */
        strcpy(szBuffer, szField);
        szField = strtok(szBuffer,"\"");
        strncpy(pPlace->szPlaceName, szField, sizeof(pPlace->szPlaceName)-1);
        break;
      case 3:
        /* first remove quotes("") */
        strcpy(szBuffer, szField);
        szField = strtok(szBuffer,"\"");
        strncpy(pPlace->szPlaceType, szField, sizeof(pPlace->szPlaceType)-1);
        break;
      case 4: /* county */
      case 5:
      case 6:
      case 7:
      case 8:
        break;
      case 9:
        pPlace->dLat = atof(szField);
        break;
      case 10:
        pPlace->dLon = atof(szField);
        break;
      default:
        break;
      }  /* end switch */
      
      
      if(i == 10)
      {
        bLineReadOK = TRUE;
        break;
      }
      
      szField = strtok(szNextField,",");
      
    }  /* end for i < num fields */

    if(!bLineReadOK)
    {
      //line error
      bIgnoreLine = TRUE;
    }
    else
    {
      if(!strcmp("airport",pPlace->szPlaceType))
      {
        pPlace->iPlaceMajorType = PLACE_TYPE_NOISE_SITE;
        pPlace->iPlaceMinorType = NOISE_SITE_AIRPORT;
      }
      else if(!strcmp("bridge",pPlace->szPlaceType))
      {
        pPlace->iPlaceMajorType = PLACE_TYPE_STRUCTURE;
        pPlace->iPlaceMinorType = STRUCTURE_BRIDGE;
      }
      else if(!strcmp("dam",pPlace->szPlaceType))
      {
        pPlace->iPlaceMajorType = PLACE_TYPE_STRUCTURE;
        pPlace->iPlaceMinorType = STRUCTURE_DAM;
      }
      else if(!strcmp("tunnel",pPlace->szPlaceType))
      {
        pPlace->iPlaceMajorType = PLACE_TYPE_STRUCTURE;
        pPlace->iPlaceMinorType = STRUCTURE_TUNNEL;
      }
      else if(!strcmp("mine",pPlace->szPlaceType))
      {
        pPlace->iPlaceMajorType = PLACE_TYPE_NOISE_SITE;
        pPlace->iPlaceMinorType = NOISE_SITE_MINE;
      }
      else if(!strcmp("oilfield",pPlace->szPlaceType))
      {
        pPlace->iPlaceMajorType = PLACE_TYPE_NOISE_SITE;
        pPlace->iPlaceMinorType = NOISE_SITE_OILFIELD;
      }
      /*else if(!strcmp("hospital",pPlace->szPlaceType))
      {
      pPlace->iPlaceMajorType = PLACE_TYPE_NOISE_SITE;
      pPlace->iPlaceMinorType = NOISE_SITE_OILFIELD;
      }
      */
      else
      {
        bIgnoreLine = TRUE;
      }
    }
    
    
    if(bIgnoreLine)
      continue;

    /* else */

    strcpy(pPlace->szCountry,"USA");
    iRetCode = ewdb_api_CreatePlace(pPlace);
    if(iRetCode != EWDB_RETURN_FAILURE)
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
      logit("e","## FAILURE ## %s, %s\t%15d\t(%5.3f,%7.3f) (%d,%d - %s)\n",
        pPlace->szPlaceName, pPlace->szState, pPlace->iPopulation, 
        pPlace->dLat, pPlace->dLon,
        pPlace->iPlaceMajorType, pPlace->iPlaceMinorType, pPlace->szPlaceType);
    }
    
  }  // end while(!feof)
  

  return(0);
}