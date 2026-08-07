
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: db_cleanup2.c,v 1.2 2004/09/09 19:28:27 davidk Exp $
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
 */
 

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <time.h>
#include <ewdb_ora_api.h>

#include "db_cleanup.h"


main(int argc, char **argv)
{

  char   szConfigFileName[256];
  int    iev, i;
  EWDB_EventStruct UpdateEventStruct;
	EWDB_EventListStruct 	*pEventBuffer;
  int rc;
  int NumEventsRetrieved;
  int iFailureCode;
  int iRetCode = 0;
  int iSQLRetCode;


  /* Introduce ourselves
  **********************/
  if(argc != 2 || strlen(argv[1]) >= sizeof(szConfigFileName))
  {
    strcpy(szConfigFileName, "db_cleanup");
  }
  else
  {
    strcpy(szConfigFileName, argv[1]);
    szConfigFileName[strlen(szConfigFileName) -2 /* .d */] = 0x00;
  }
  
  /* Start up Logging
  *******************/
  logit_init(szConfigFileName,1,1024,1);
  
  /* Introduce ourselves
  **********************/
  if(argc != 2)
  {
    logit("et", "Usage: db_cleanup config_file.\n");
    iRetCode = APP_ERROR_BAD_ARGUMENT_LIST;
    goto shutdown;
  }

  /* Compute current time */
  tNow = time(NULL);
  
  /* Read the configuration file(path hardcoded relative to executable)
  *********************************************************************/
  if(ReadConfig(argv[1]) != EW_SUCCESS)
  {
    logit("et","db_cleanup: ERROR: ReadConfig() returned error while parsing config file.\n");
    iRetCode = APP_ERROR_BAD_CONFIG_FILE;
    goto shutdown;
  }

  if(!(iNumEventRules || bHandleOtherData))
  {
    logit("et","db_cleanup: ERROR!!  No EVENT or OTHER_DATA lines in config file.  "
                "Nothing to cleanup archive.  Can't go on!  Exiting\n");
    iRetCode = APP_ERROR_NOTHING_TO_CLEANUP;
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
    logit("", "db_cleanup: Trouble connecting to database as(%s@%s); exiting!\n", 
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
    logit("et","db_cleanup: Could not allocate %d EWDB_EventListStructs.  Returning Error!\n",
          MaxEventsToHandle);
    iRetCode = APP_ERROR_MALLOC_FAILED;
    goto shutdown;
  }


  /* Initialize the UpdateEventStruct, in case we have 
     to update an event later on. */
  memset(&UpdateEventStruct, 0, sizeof(UpdateEventStruct));
  
  /* Here we start looping through the criteria structs that were
  setup in the config file. */
  
  
  /* Loop through each of the rules */
  for(i=0; i < iNumEventRules; i++)
  {
    
    /* Get the list that matches the current rule's criteria
     **********************************************************/
    rc = ewdb_api_GetEventList(pEventBuffer, MaxEventsToHandle,
                                   EventRules[i].tStart, EventRules[i].tEnd,
                                   &(EventRules[i].EVMin), &(EventRules[i].EVMax), 
                                   &NumEventsRetrieved);

	  if(rc == EWDB_RETURN_FAILURE)
	  {
		  logit("", "Call to ewdb_api_GetEventList() failed -- see logfile.\n");
      iRetCode = APP_ERROR_GETEVENTLIST_FAILED;
		  goto shutdown;
	  }

    if(DEBUG)
    {
      if(EventRules[i].iDataToDelete = DELETE_WAVEFORMS)
        logit("", "Will clean up trace data from %d events.\n", NumEventsRetrieved);
      else 
        logit("", "Will clean up all data from %d events.\n", NumEventsRetrieved);

      if(EventRules[i].iDataToSave != SAVE_NONE)
        logit("", "Trace data in SAC format saved in %s.\n", szBaseArchiveDirectory); 

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

	  /* Initialize Archiving Environment if neccessary */
    if(EventRules[i].iDataToSave != SAVE_NONE)
	  {
		  if(ArchiveEvent_init(szBaseArchiveDirectory, szOutputFormat, 
                           iLargestSnippetSize, DEBUG) != EW_SUCCESS)
		  {
			  logit("", "db_cleanup: Call to ArchiveEvent_init() failed!\n" );
        iRetCode = APP_ERROR_ARCHIVEEVENT_INIT_FAILED;
			  goto shutdown;
		  }
	  }


	  /**************** LOOP OVER EVENTS ********************/
	  for(iev = 0; iev < NumEventsRetrieved; iev++)
	  {
      /* If we want to save this event, and it meets our criteria for saving,
         then save it.     */
      if(EventRules[i].iDataToSave != SAVE_NONE && !(pEventBuffer[iev].Event.bArchived))
		  {
        rc = ArchiveEvent(pEventBuffer[iev].Event.idEvent, 
                          EventRules[i].iDataToSave, &iFailureCode);
        if(rc != EW_SUCCESS)
        {
          if(rc == EW_FAILURE)
          {
            logit("","Unable to archive type %d data for event %u  Failure Code: %d\n",
                  EventRules[i].iDataToSave, pEventBuffer[iev].Event.idEvent,
                  iFailureCode);
            iRetCode = APP_ERROR_ARCHIVEEVENT_FAILED;
            goto shutdown;
          }
          else
          {
            logit("","Received the following warning from ArchiveEvent "
                     "for event %u.  Failure Code: %d\n",
                  pEventBuffer[iev].Event.idEvent,
                  iFailureCode);
          }
        }
        if(EventRules[i].iDataToSave == SAVE_ALL)
        {
          if(bSetArchiveFlag)
          {
            UpdateEventStruct.idEvent = pEventBuffer[iev].Event.idEvent;
            UpdateEventStruct.bArchived = TRUE;

            rc = ewdb_api_UpdateEvent(&UpdateEventStruct, EWDB_UPDATE_EVENT_ARCHIVED);
            if(rc != EWDB_RETURN_SUCCESS)
            {
              logit("t","WARNING!  Could not mark Event %u as archived!  Continuing!\n",
                    UpdateEventStruct.idEvent);
            }
          }
        }  /* end  if SAVE_ALL */
      }  /* end  if we want to save some portion of this event. */
      if(EventRules[i].iDataToDelete != DELETE_NONE)
		  {
        if(EventRules[i].iDataToDelete == DELETE_WAVEFORMS)
        {
          if(ewdb_api_DeleteEvent(pEventBuffer[iev].Event.idEvent, TRUE) 
             == EWDB_RETURN_FAILURE)
          {
            logit("","db_cleanup(): ERROR: Failed to delete waveforms "
                     "for Event %d!  Returning ERROR!\n", 
                  pEventBuffer[iev].Event.idEvent);
            iRetCode = APP_ERROR_DELETEEVENT_FAILED;
            goto shutdown;
          }
        }
        else if(EventRules[i].iDataToDelete == DELETE_ALL)
        {
          if(ewdb_api_DeleteEvent(pEventBuffer[iev].Event.idEvent, FALSE) 
             == EWDB_RETURN_FAILURE)
          {
            logit("","db_cleanup(): ERROR: Failed to delete Event %d!  "
                     "Returning ERROR!\n", 
                  pEventBuffer[iev].Event.idEvent);
            iRetCode = APP_ERROR_DELETEEVENT_FAILED;
            goto shutdown;
          }
        }
        else
        {
            logit("","db_cleanup(): ERROR: Unsupported Delete mode(%d) "
                     "selected in rule %d! Returning ERROR!\n", 
                  pEventBuffer[iev].Event.idEvent, i);
            iRetCode = APP_ERROR_UNSUPPORTED_DELETEEVENT_MODE;
            goto shutdown;
        }
        logit("t","Successfully deleted %d type data for Event %d\n",
              EventRules[i].iDataToDelete, pEventBuffer[iev].Event.idEvent);

      }  /* end if iDataToDelete  */
    }  /* end for each event that matches this rule. */
  }  /* end for each rule in the config file. */


/* delete unassociated data */
  if(bHandleOtherData)
  {
    if(bSaveOtherData) 
    {
      logit("et","db_cleanup: ERROR: Config file indicates that unassociated "
                 "data should be saved, but this option is not supported by "
                 "the program.  Returning Error!  Please fix the config file "
                 "and restart\n");
      iRetCode = APP_ERROR_UNSUPPORTED_OTHER_DATA_MODE;
      goto shutdown;
    }

    if(bDeleteOtherData)
    {
      do
      { 
        logit("t","Deleting Unassociated data(%d records) before time %d.\n",
              APP_MAX_RECORDS_TO_DELETE, tOtherDataCutoffTime);
        rc = ewdb_api_DeleteDataBeforeTime(tOtherDataCutoffTime, EWDB_UNASSOCIATED_DATA_ALL,
                                           APP_MAX_RECORDS_TO_DELETE,
                                           &iSQLRetCode);
      }  while((rc == EWDB_RETURN_WARNING) && 
        (iSQLRetCode & EWDB_DELETE_PARAMS_WARNING_TOO_MANY));
      
      if(rc == EWDB_RETURN_FAILURE)
      {
        logit("et","db_cleanup: ERROR: Unable to delete unassociated data "
          "before time %s(%d)\n",
          ctime(&tNow),tNow);
        iRetCode = APP_ERROR_DELETE_DATA_FAILED;
      }
      else if(rc == EWDB_RETURN_WARNING)
      {
        logit("t","db_cleanup: WARNING: Unable to delete all data. FK constraint(s).\n");
        if(iRetCode == 0)
          iRetCode = 1;
      }
      else
      {
        logit("t","db_cleanup: SUCCESS:   Data Deleted.\n");
      }
    }
  }  /* end if bHandleOtherData */

shutdown:
	ewdb_api_Shutdown();

	logit("t","db_cleanup: terminating\n" );

	return(iRetCode);

}  /* end main */






