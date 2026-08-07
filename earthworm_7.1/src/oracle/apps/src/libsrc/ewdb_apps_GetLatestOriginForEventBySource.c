#include <stdlib.h>
#include <ewdb_ora_api.h>

int ewdb_apps_GetLatestOriginForEventBySource(char * szSource, EWDBid idEvent,
                                              EWDB_OriginStruct * pLatestOrigin)
{

  int rc;
  EWDBid idSource;
  EWDB_OriginStruct *OriginList;
  int iNumFound, iNumRetrieved;
  int i, iMaxOrigin = -1, idMaxOrigin = 0;
  int iListSize = 40;

  OriginList = (EWDB_OriginStruct *) calloc(sizeof(EWDB_OriginStruct), iListSize);
  if(!OriginList)
  {
    logit("et","ewdb_apps_GetLatestOriginForEventBySource():  could not allocate %d origin structs.\n",
          iListSize);
    return(EWDB_RETURN_FAILURE);
  }
  rc = ewdb_api_GetOriginsForEvent(idEvent, OriginList, &iNumFound, &iNumRetrieved, 
                                   iListSize);
  if(rc == EWDB_RETURN_FAILURE)
  {
    logit("","%s(): ewdb_api_GetOriginsForEvent() failed with rc=%d for idEvent %d\n",
          "ewdb_apps_GetLatestOriginForEventBySource", rc, idEvent);
    goto failure;
  }
  else if(rc == EWDB_RETURN_WARNING)
  {
    iListSize = iNumFound;
    free(OriginList);
    OriginList = (EWDB_OriginStruct *) calloc(sizeof(EWDB_OriginStruct), iListSize);
    if(!OriginList)
    {
      logit("et","ewdb_apps_GetLatestOriginForEventBySource():  could not allocate %d origin structs.\n",
            iListSize);
      goto failure;
    }
    rc = ewdb_api_GetOriginsForEvent(idEvent, OriginList, &iNumFound, &iNumRetrieved, 
                                     iListSize);
    if(rc != EWDB_RETURN_SUCCESS)
    {
      logit("","%s(): ewdb_api_GetOriginsForEvent(2) failed with rc=%d for idEvent %d\n",
            "ewdb_apps_GetLatestOriginForEventBySource", rc, idEvent);
      goto failure;
    }

  }
  for(i=0; i < iNumRetrieved; i++)
  {
     if(strcmp(OriginList[i].sRealSource, szSource) == 0)
     {
       if(OriginList[i].idOrigin > idMaxOrigin)
       {
         idMaxOrigin = OriginList[i].idOrigin;
         iMaxOrigin = i;
       }
     }
  }

  if(iMaxOrigin >= 0)
  {
    /* we found one */
    memcpy(pLatestOrigin, &(OriginList[iMaxOrigin]), sizeof(EWDB_OriginStruct));
    return(EWDB_RETURN_SUCCESS);
  }
  else
  {
    return(EWDB_RETURN_WARNING);
    /* no origin by that source found for this event */
  }

failure:
  if(OriginList)
    free(OriginList);
  return(EWDB_RETURN_FAILURE);

}  /* end ewdb_apps_GetLatestOriginForEventBySource() */

