
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: unlocker.c,v 1.1 2002/05/15 20:45:45 davidk Exp $
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

#include "unlocker.h"

main(int argc, char **argv)
{

  char   szConfigFileName[256];
  int    iCurrentRequest;
	EWDB_SnippetRequestStruct    *pRequestBuffer;
  int rc;
  int iNumRequestsRetrieved,iNumRequestsFound;
  int iRetCode = 0;
  time_t tThreshold;
  int iNumRequestsUnlocked=0;
  int bDone = FALSE;


  /* Introduce ourselves
  **********************/
  if(argc != 2 || strlen(argv[1]) >= sizeof(szConfigFileName))
  {
    strcpy(szConfigFileName, "unlocker");
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
    logit("et", "Usage: unlocker config_file.\n");
    iRetCode = APP_ERROR_BAD_ARGUMENT_LIST;
    goto shutdown;
  }

  /* Compute current time */
  tNow = time(NULL);
  
  /* Read the configuration file(path hardcoded relative to executable)
  *********************************************************************/
  if(ReadConfig(argv[1]) != EW_SUCCESS)
  {
    fprintf(stderr,"unlocker(%s): ERROR: ReadConfig() returned error while "
                   "parsing config file.\n",
            ctime(&tNow));
    iRetCode = APP_ERROR_BAD_CONFIG_FILE;
    goto shutdown;
  }

  /* Start up Logging
  *******************/
  logit_init(szConfigFileName,1,1024,1);
  
  logit("","\n"
           "/*************************************\n"
           "  Initializing ORA_API                 \n"
           " *************************************/\n");
  
    /* Open connection to database
  *****************************/
  if(ewdb_api_Init(DBuser, DBpassword, DBservice) != EWDB_RETURN_SUCCESS)
  {
    logit("", "unlocker: Trouble connecting to database as(%s@%s); exiting!\n", 
          DBuser, DBpassword);
    iRetCode = APP_ERROR_EWDB_API_INIT_FAILED;
    goto shutdown;
  }
  
  if(DEBUG)
  {
    logit("", "Connected to DB!\n");
  }
  
  
  pRequestBuffer = (EWDB_SnippetRequestStruct *)malloc(sizeof(EWDB_SnippetRequestStruct) 
                                                * MaxRequestsToHandle);
  if(!pRequestBuffer)
  {
    logit("et","unlocker: Could not allocate %d EWDB_SnippetRequestStructs.  Returning Error!\n",
          MaxRequestsToHandle);
    iRetCode = APP_ERROR_MALLOC_FAILED;
    goto shutdown;
  }


  tThreshold = time(NULL) - tDeltaUnlocking;


  while(!bDone)
  {
    /* initialize request counter */
    iNumRequestsUnlocked=0;

    /* Get the list that matches our limited criteria
    **********************************************************/
    rc = ewdb_api_GetListOfOldSnipReqs(pRequestBuffer, tThreshold, 
                                       &iNumRequestsFound, &iNumRequestsRetrieved,
                                       MaxRequestsToHandle);
  
    if(rc == EWDB_RETURN_FAILURE)
    {
		  logit("", "Call to ewdb_api_GetListOfOldSnipReqs() failed -- see logfile.\n");
      iRetCode = APP_ERROR_GETOLDREQUESTLIST_FAILED;
      goto shutdown;
    }

    if(iNumRequestsRetrieved)
    {
      logit("t","%d/%d requests found/retrieved with stale locks this time through.\n",
            iNumRequestsFound, iNumRequestsRetrieved);
    }
    else
    {
      logit("t","No requests found this time through!\n");
      bDone = TRUE;
      continue;
    }
  
    /**************** LOOP OVER EVENTS ********************/
    for(iCurrentRequest = 0; iCurrentRequest < iNumRequestsRetrieved; iCurrentRequest++)
    {
      rc = ewdb_internal_ReleaseReservedSnipReqs(pRequestBuffer[iCurrentRequest].iLockTime);
      /* DOH!! We are using an internal function call.  Not good, but this is kind
         of a unique application, we don't want other people writing this kind of stuff.
         DavidK 051502
       **************************************************************/
      if(rc == EWDB_RETURN_SUCCESS)
      {
        /* increment the unlock counter.  We may have multiple locked requests with
           the same iLockTime, at which point successive calls will fail.  As long as
           we get one that succeeds, we are happy. 
        *******/
        iNumRequestsUnlocked++;
      }
    }  /* end for each rule in the config file. */

    if(!iNumRequestsUnlocked)
    {
      logit("t","Unlocker:  Failed to unlock locked requests.  %d of %d requests unlocked.\n",
            iNumRequestsUnlocked, iNumRequestsRetrieved);
    }

    sleep_ew(10000);  /* 10 seconds */

  }  /* end while !bDone */

shutdown:

	logit("t","unlocker: terminating\n" );

	return(iRetCode);

}  /* end main */






