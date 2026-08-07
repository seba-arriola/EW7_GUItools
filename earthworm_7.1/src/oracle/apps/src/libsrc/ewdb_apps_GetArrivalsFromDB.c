#include <ewdb_ora_api.h>


int ewdb_apps_GetArrivalsFromDB(EWDB_ArrivalStruct ** ppAS, 
                                EWDB_ChannelStruct ** ppCS, int idOrigin,
                                int * pNumArrivals)
{
  int NumItemsFound, NumItemsRetrieved;
  int BufferSize = 200;
  int ret;
  EWDB_ArrivalStruct * pAS = NULL;
  EWDB_ChannelStruct * pCS = NULL;
  
  if(!(ppAS && ppCS &&& pNumArrivals))
  {
    logit("t","%s:  Error NULL input param\n", "ewdb_apps_GetArrivalsFromDB()");
    return(EWDB_RETURN_FAILURE);
  }
  
	if (idOrigin <= 0)
	{
		logit ("", "ewdb_apps_GetArrivalsFromDB(): idOrigin(%d) is invalid.\n", idOrigin);
		return EWDB_RETURN_FAILURE;
	}
		
  /* allocate the buffers */
  pAS = (EWDB_ArrivalStruct *) malloc(BufferSize * sizeof(EWDB_ArrivalStruct));
  if(!pAS)
  {
    logit("t","ewdb_apps_GetArrivalsFromDB(): malloc 1A failed for %d bytes.\n", 
          BufferSize * sizeof(EWDB_ArrivalStruct));
    return(EWDB_RETURN_FAILURE);
  }

  pCS = (EWDB_ChannelStruct *) malloc(BufferSize * sizeof(EWDB_ChannelStruct));
  if(!pCS)
  {
    logit("t","ewdb_apps_GetArrivalsFromDB(): malloc 1B failed for %d bytes.\n", 
          BufferSize * sizeof(EWDB_ChannelStruct));
    return(EWDB_RETURN_FAILURE);
  }

  /* Retrieve the Arrivals for this event. */
  if(ewdb_api_GetArrivalsWChanInfo(idOrigin,pAS,pCS,
     &NumItemsFound,&NumItemsRetrieved, 
     BufferSize) == EWDB_RETURN_FAILURE)
  {
    logit ("", "%s:  Call 1 to ewdb_api_GetArrivalsWChanInfo failed.\n",
      "ewdb_apps_GetArrivalsFromDB()");
    ret = EWDB_RETURN_FAILURE;
    goto Abort;
  }
  
  
  /* check to see if our buffer was large enough to retrieve all the arrivals */
  if(NumItemsFound > NumItemsRetrieved)
  { 
    /* enlarge the buffers */
    free(pAS);  pAS=NULL;
    free(pCS);  pCS=NULL;
    BufferSize = NumItemsFound;


    pAS = (EWDB_ArrivalStruct *) malloc(BufferSize * sizeof(EWDB_ArrivalStruct));
    if(!pAS)
    {
      logit("t","ewdb_apps_GetArrivalsFromDB(): malloc 2A failed for %d bytes.\n", 
        BufferSize * sizeof(EWDB_ArrivalStruct));
      return(EWDB_RETURN_FAILURE);
    }
    
    pCS = (EWDB_ChannelStruct *) malloc(BufferSize * sizeof(EWDB_ChannelStruct));
    if(!pCS)
    {
      logit("t","ewdb_apps_GetArrivalsFromDB(): malloc 2B failed for %d bytes.\n", 
        BufferSize * sizeof(EWDB_ChannelStruct));
      return(EWDB_RETURN_FAILURE);
    }
    
    /* Retrieve the Arrivals for this event. */
    if(ewdb_api_GetArrivalsWChanInfo(idOrigin,pAS,pCS,
      &NumItemsFound,&NumItemsRetrieved, 
      BufferSize) == EWDB_RETURN_FAILURE)
    {
      logit ("", "%s:  Call 2 to ewdb_api_GetArrivalsWChanInfo failed.\n",
        "ewdb_apps_GetArrivalsFromDB()");
      ret = EWDB_RETURN_FAILURE;
      goto Abort;
    }
  }  /* end  if(NumItemsFound > NumItemsRetrieved) */

  /* set the caller's variables */
  *ppAS = pAS;
  *ppCS = pCS;
  return(EWDB_RETURN_SUCCESS);
  
Abort:
  if(pAS) {    free(pAS); pAS=NULL; }
  if(pCS) {    free(pCS); pCS=NULL; }
  *ppAS = NULL;
  *ppCS = NULL;
  return(EWDB_RETURN_FAILURE);  
}  /* end ewdb_apps_GetArrivalsFromDB() */




