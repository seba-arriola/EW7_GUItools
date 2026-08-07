/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: schedule.c,v 1.6 2002/06/28 22:22:42 davidk Exp $
 *
 *    Revision history:
 *     $Log: schedule.c,v $
 *     Revision 1.6  2002/06/28 22:22:42  davidk
 *     Fixed a bug in SCHEDULE_EXPONENTIAL, that forgot to count the current attempt
 *     when determining whether enough attempts have been made to delete the request.
 *
 *     Revision 1.5  2002/06/27 23:44:17  davidk
 *     set the retcode variable with the return code from ewdb_api_DeleteSnippetRequest()
 *
 *     Revision 1.4  2002/05/16 17:11:54  davidk
 *     Added a iScheduleMode variable for tracking the scheduling
 *     algorithm to use.  Previously scheduling was accidentally written
 *     to the iProgramMode variable that was supposed to track the
 *     retrieval mode of the program.  (fixed bug).
 *
 *     Revision 1.3  2002/05/15 21:00:12  davidk
 *     added newline to end of file.
 *
 *     Revision 1.2  2002/05/15 20:58:24  davidk
 *     added some more documentation, fixed a couple of return codes,
 *     including one for the scheduling function to return WARNING when
 *     deleting a request.
 *
 *
 ***************************************************************/


/* schedule.c contains scheduling algorithm related functionality to 
   ora_trace_fetch.
*************************************************************************/

/*************************************************************************
   HOW TO:  WRITING YOUR OWN SCHEDULING ALGORITHM

   To add your own scheduling algorithm, add code to the places in the
   following file labeled "FILL IN:".

   *NOTE*  You will also have to add code to the "schedule.h" header file.

   The steps for defining your own scheduling algorithm are:
   1) Define a constant identifying your algorithm in "schedule.h".
   2) Add prototypes for the scheduling and config functions for your
      algorithm.
   3) Add any neccessary global variables. (they should be static to this file)
   4) Add a check for your strategy type in the "RetrievalSchedulingMethod"
      section of CheckForSchedulingConfigVariables().
   5) Add a call to your scheduling config method in 
      CheckForSchedulingConfigVariables().
   6) Add your scheduling config method CheckForSCHEDULE_XXXConfigVariables(),
      that is called in Step 5.
   7) Add a call to your main scheduling method in 
      ScheduleNextAttemptForSnippetRequest().
   8) Add your main scheduling method ScheduleNextAttemptUsingSCHEDULE_XXX(),
      that is called in Step 7.
   9) Add any supporting code in your own functions.
  10) Add a description of your algorithm and the config string that 
      identifies it to the config file under the "RetrievalSchedulingMethod"
      section.
 *************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time_ew.h>
#include <kom.h>
#include <math.h>
#include <ewdb_ora_api.h>
#include "schedule.h"
#include "ora_trace_fetch.h"

/* Functions related to the act of scheduling */
int CheckForSchedulingConfigVariables();
int ScheduleNextAttemptForSnippetRequest(EWDB_SnippetRequestStruct * psrCurrent, 
                                         int iAttemptRetCode);

/* Functions related to SCHEDULE_EXPONENTIAL scheduling mechanisms */
int CheckForSCHEDULE_EXPONENTIALConfigVariables();
int ScheduleNextAttemptUsingSCHEDULE_EXPONENTIAL(EWDB_SnippetRequestStruct * psrCurrent, 
                                                 int iAttemptRetCode);

/* FILL IN:  Functions related to XXX scheduling mechanisms */


/* Globals for Scheduling 
 ***********************************************************/
int iScheduleMode= OTF_SCHEDULE_EXPONENTIAL;

/* Globals for SCHEDULE_EXPONENTIAL scheduling mechanism 
 ***********************************************************/
int   iAttemptInterval = 600;  /* seconds */
float dAttemptMultiple = 2.0;  
int   iNumSecondsBetweenPartials = 600; /* should be the same as iAttemptInterval */

/* FILL IN:  Globals for XXX scheduling mechanism 
 ***********************************************************/



/* in CheckForSchedulingConfigVariables(), you must add a call to your
    config function CheckForSCHEDULE_XXXConfigVariables(), if you need to read
    variables from the config file.
 ***********************************************************************/

/************************************************************************
 *  CheckForSchedulingConfigVariables( ) - Check a config file command  *
 *  to see if it belongs to an Attempt Scheduling mechanism.            *
 ************************************************************************/
int CheckForSchedulingConfigVariables()
{

  char * str;

  /* RetrievalSchedulingMethod */

  if( k_its("RetrievalSchedulingMethod") ) 
  {
    str = k_str(); 
    if(!strcmp(str,"SCHEDULE_EXPONENTIAL"))
    {
      iScheduleMode = OTF_SCHEDULE_EXPONENTIAL;
    }
    /* FILL IN:  your scheduling method name and constant here.
       The string is the name given to your algorithm in the 
       config file.  The constant OTF_SCHEDULE_XXX is the one
       you defined in schedule.h.
    else if(!strcmp(str,"SCHEDULE_XXX"))
    {
      iScheduleMode = OTF_SCHEDULE_XXX;
    }
    **************/
    else
    {
      fprintf(stderr,"ora_trace_fetch:  Config File Error: "
              "Illegal Value for \n"
              "\"RetrievalSchedulingMethod\" config file command: (%s).  "
              "Returning!\n",str
             );
      return(-1);
    }
		return(TRUE);
  }  /* end if RetrievalSchedulingMethod */


  if(CheckForSCHEDULE_EXPONENTIALConfigVariables())
  {
    return(TRUE);
  }
  /* 
  FILL IN:  add your scheduling method config here. 
  else if(CheckForSCHEDULE_XXXConfigVariables())
  {
    return(TRUE);
  }
  */
  else  
  {
  }
  return(FALSE);

}


/************************************************************************
 *  CheckForSCHEDULE_EXPONENTIALConfigVariables( ) - Check a config file*
 *  command to see if it belongs to the SCHEDULE_EXPONENTIAL            *
 *  scheduling mechanism.                                               *
 ************************************************************************/
int CheckForSCHEDULE_EXPONENTIALConfigVariables()
{
  if(k_its("AttemptInterval") ) 
  {
    iAttemptInterval = k_int(); 
  }
  else if( k_its("AttemptMultiple") ) 
  {
    dAttemptMultiple = (float)k_val(); 
  }
  else
  {
    return(FALSE);
  }
  return(TRUE);
}


/* FILL IN:  your scheduling config method */
/************************************************************************
 *  CheckForSCHEDULE_XXXConfigVariables( ) - Check a config file                 *
 *  command to see if it belongs to the XXX                             *
 *  scheduling mechanism.  Remember to name your config variables       *
 *  something unique!                                                   *
 ************************************************************************/
/*
int CheckForSCHEDULE_XXXConfigVariables()
{
  if(k_its("BlahXXX") ) 
  {
    / * get the value * /
    return(TRUE);
  }

  return(FALSE);
}
************************************************************/


int ScheduleNextAttemptForSnippetRequest(EWDB_SnippetRequestStruct * psrCurrent, 
                                         int iAttemptRetCode)
{
  /* Return codes for this function and it's subfunctions:
      EWDB_RETURN_SUCCESS     success
      EWDB_RETURN_WARNING     snippet request was deleted
      EWDB_RETURN_FAILURE     failure
   *********************************************************/
  if(iScheduleMode == OTF_SCHEDULE_EXPONENTIAL)
  {
    return(ScheduleNextAttemptUsingSCHEDULE_EXPONENTIAL(psrCurrent,iAttemptRetCode));
  }
  /* 
  FILL IN:  add your scheduling method main call here. 
  else if(iScheduleMode == OTF_XXX)
  {
    return(ScheduleNextAttemptUsingSCHEDULE_XXX(psrCurrent,iAttemptRetCode));
  }
  */
  return(EWDB_RETURN_FAILURE);
}  /* end ScheduleNextAttemptForSnippetRequest()  */


int ScheduleNextAttemptUsingSCHEDULE_EXPONENTIAL(EWDB_SnippetRequestStruct * psrCurrent, 
                                                 int iAttemptRetCode)
{

  int iRetCode;
  int iMyRetCode = EWDB_RETURN_SUCCESS;

  if(psrCurrent->iNumAttempts == 0)
  {
    psrCurrent->iNumAttempts = 11;
    iRetCode = ewdb_api_UpdateSnippetRequestControlParams(psrCurrent);
    if(iRetCode != EWDB_RETURN_SUCCESS)
    {
      logit("","ERROR: ewdb_api_UpdateSnippetRequestControlParams(%d) returned %d.  Continuing.\n",
            psrCurrent->idSnipReq, iRetCode);
      iMyRetCode = iRetCode;
    }
  }

  if(iAttemptRetCode == SNIPPET_REQUEST_ATTEMPT_FAILURE)
  {
    psrCurrent->tNextAttempt = iAttemptInterval * 
                   (int) pow(dAttemptMultiple, psrCurrent->iNumAlreadyAtmptd);
    /* apply algorithm */
    if(psrCurrent->iNumAttempts <= psrCurrent->iNumAlreadyAtmptd + 1) /* count the current attempt */
    {
      iRetCode = ewdb_api_DeleteSnippetRequest(psrCurrent->idSnipReq, psrCurrent->iLockTime);
      if(iRetCode != EWDB_RETURN_SUCCESS)
      {
        logit("","ERROR: ewdb_api_DeleteSnippetRequest(%d) returned %d.  Continuing.\n",
              psrCurrent->idSnipReq, iRetCode);
        iMyRetCode = EWDB_RETURN_FAILURE;
      }
      iMyRetCode = EWDB_RETURN_WARNING;
    }
  }
  else if(iAttemptRetCode == SNIPPET_REQUEST_ATTEMPT_PARTIAL_SUCCESS)
  {
    psrCurrent->tNextAttempt = iNumSecondsBetweenPartials;
  }

  return(iMyRetCode);
}  /* end ScheduleNextAttemptUsingSCHEDULE_EXPONENTIAL() */

/* FILL IN:  your main scheduling method */
/************************************
int ScheduleNextAttemptUsingSCHEDULE_XXX(EWDB_SnippetRequestStruct * psrCurrent, 
                                         int iAttemptRetCode)
{
  / * do scheduling * /
  psrCurrent->tNextAttempt = magic-number-of-seconds-from-now;

  return(EWDB_RETURN_SUCCESS);
}  / * end ScheduleNextAttemptUsingSCHEDULE_XXX() * /
*********/
  

/* <eof> */

