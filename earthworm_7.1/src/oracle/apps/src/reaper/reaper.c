
/*
*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
*   CHECKED IT OUT USING THE COMMAND CHECKOUT.
*
*    $Id: reaper.c,v 1.6 2004/09/09 19:29:02 davidk Exp $
*
*    Revision history:
*
*/



/*
* This program is meant to be run out of cron(or some other 
* periodic invocation method). Its function is to keep the 
* number of events in the database manageable, by deleting 
* the oldest events. 
*
* The configuration options StartDate and NumberOfDays determine
* how many and which events are deleted. For example, StartDate
* of 7 and NumberOfDays of 1 means that this program will delete
* all events for one day starting 7 days ago. 
*
* 7/26/02 alex: tweak to do multi-pass delete
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <time.h>
#include <ewdb_ora_api.h>

#include "reaper.h"

EWDB_EventListStruct * pevmin;
int main(int argc, char **argv)
{
  
  char   szConfigFileName[256];
  int    iev;
  EWDB_EventListStruct   *pEventBuffer;
  int rc;
  int NumEventsRetrieved;
  EWDB_EventListStruct EVMin, EVMax;
  int iRetCode = 0;
  long leftToDelete;
  int  iSQLRetCode;

  /* Introduce ourselves
  **********************/
  if(argc != 2 || strlen(argv[1]) >= sizeof(szConfigFileName))
  {
    strcpy(szConfigFileName, "reaper");
  }
  else
  {
    strcpy(szConfigFileName, argv[1]);
    szConfigFileName[strlen(szConfigFileName) -2 /* .d */] = 0x00;
  }
  
  /* Introduce ourselves
  **********************/
  if(argc != 2)
  {
    printf("Usage: reaper config_file.\n");
    return APP_ERROR_BAD_ARGUMENT_LIST;
  }
  
  /* Compute current time */
  tNow = time(NULL);
  
  /* Initialize EVMax and EVMin */
  memset(&EVMax,0,sizeof(EWDB_EventListStruct));
  memset(&EVMin,0,sizeof(EWDB_EventListStruct));
  strcpy(EVMin.Event.szSource, "*");
  pevmin = &EVMin;
  
  /* Start up Logging
  *******************/
  logit_init(szConfigFileName,0,1024,1);
  logit("","Multi-pass version 7/26/02\n");
  
  /* Read the configuration file(path hardcoded relative to executable)
  *********************************************************************/
  if(ReadConfig(argv[1]) != EW_SUCCESS)
  {
    fprintf(stderr,"reaper(%s): ERROR: ReadConfig() returned error while "
      "parsing config file.\n",
            ctime(&tNow));
    iRetCode = APP_ERROR_BAD_CONFIG_FILE;
    goto shutdown;
  }
  
  
  logit("","\n"
    "/*************************************\n"
    "  Initializing ORA_API                 \n"
    " *************************************/\n");
  
  /* Open connection to database
  *****************************/
  if(ewdb_api_Init(DBuser, DBpassword, DBservice) != EWDB_RETURN_SUCCESS)
  {
    logit("", "reaper: Trouble connecting to database as(%s@%s); exiting!\n", 
      DBuser, DBpassword);
    iRetCode = APP_ERROR_EWDB_API_INIT_FAILED;
    goto shutdown;
  }
  
  if(DEBUG)
  {
    logit("", "Connected to DB!\n");
  }
    
  pEventBuffer = (EWDB_EventListStruct *)malloc(sizeof(EWDB_EventListStruct) 
    * MaxEventsToHandle);
  if(!pEventBuffer)
  {
    logit("et","reaper: Could not allocate %d EWDB_EventListStructs.  Returning Error!\n",
      MaxEventsToHandle);
    iRetCode = APP_ERROR_MALLOC_FAILED;
    goto shutdown;
  }
  
  /* loop: delete "MaxEventsToHandle" per pass
  ********************************************/
  do
  {
    /* Get the list that matches our criteria
    *****************************************/
    rc = ewdb_api_GetEventList(pEventBuffer, MaxEventsToHandle,
      0/* no start time */, tReckoning,
      &EVMin, &EVMax, 
      &NumEventsRetrieved);
    
    if(rc == EWDB_RETURN_FAILURE)
    {
      logit("", "Call to ewdb_api_GetEventList() failed -- see logfile.\n");
      iRetCode = APP_ERROR_GETEVENTLIST_FAILED;
      goto shutdown;
    }
    if(rc < -1)  /* there was more than we could eat */
    {
      logit("e","Warning: %d events found. Deleting %d on this pass.\n", 
        -rc, MaxEventsToHandle);
      leftToDelete = -rc;
    }
    else if( rc == EWDB_RETURN_SUCCESS )
    {
      logit("e"," %d events found. Deleting %d events\n",
        NumEventsRetrieved,NumEventsRetrieved);
      leftToDelete = 0;
    }

    
    if(DEBUG)
    {
      
      for(iev = 0; iev < NumEventsRetrieved; iev++)
      {
        logit("", "%d (%s): %0.2f ==>  %0.2f, %0.2f, %0.2f, %0.2f\n",
          pEventBuffer[iev].Event.idEvent, 
          pEventBuffer[iev].Event.szSourceName, 
          pEventBuffer[iev].dOT, 
          pEventBuffer[iev].dLat, 
          pEventBuffer[iev].dLon, 
          pEventBuffer[iev].dDepth, 
          pEventBuffer[iev].dPrefMag);
      }
    }
    
    
    /**************** LOOP OVER EVENTS ********************/
    for(iev = 0; iev < NumEventsRetrieved; iev++)
    {
      if(ewdb_api_DeleteEvent(pEventBuffer[iev].Event.idEvent, FALSE) 
        == EWDB_RETURN_FAILURE)
      {
        logit("","reaper(): ERROR: Failed to delete Event %d!  "
          "Returning ERROR!\n", 
          pEventBuffer[iev].Event.idEvent);
        iRetCode = APP_ERROR_DELETEEVENT_FAILED;
        goto shutdown;
      }
      logit("t","Successfully deleted Event %d\n",
        pEventBuffer[iev].Event.idEvent);
      
    }  /* end for each event found. */

  /* end of loop for multi-pass delete
  ************************************/
  } while ( leftToDelete > 0 );

  if(pEventBuffer)
  {
    free(pEventBuffer);
    pEventBuffer = NULL;
  }
    
  /* delete unassociated data */
  do
  { 
    logit("t","Deleting Unassociated data(%d records).\n",APP_MAX_RECORDS_TO_DELETE);
    rc = ewdb_api_DeleteDataBeforeTime(tReckoning, EWDB_UNASSOCIATED_DATA_ALL,
                                       APP_MAX_RECORDS_TO_DELETE,
                                       &iSQLRetCode);
  }  while((rc == EWDB_RETURN_WARNING) && 
           (iSQLRetCode & EWDB_DELETE_PARAMS_WARNING_TOO_MANY));

  if(rc == EWDB_RETURN_FAILURE)
  {
    logit("et","reaper: ERROR: Unable to delete unassociated data "
               "before time %s(%d)\n",
    ctime(&tNow),tNow);
    iRetCode = APP_ERROR_DELETE_DATA_FAILED;
  }
  else if(rc == EWDB_RETURN_WARNING)
  {
    logit("t","reaper: WARNING: Unable to delete all data. FK constraint(s).\n");
    iRetCode = 1;
  }
  else
  {
    logit("t","reaper: SUCCESS:   Data Deleted.\n");
    iRetCode = 0;
  }
    
shutdown:
  ewdb_api_Shutdown();
  
  logit("t","reaper: terminating\n" );
  
  return(iRetCode);
  
}  /* end main */






