
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: webhelper.c,v 1.4 2001/05/15 02:15:11 davidk Exp $
 *    Revision history:
 *
 *    $Log: webhelper.c,v $
 *    Revision 1.4  2001/05/15 02:15:11  davidk
 *    Moved functions around between the apps, DB API, and DB API INTERNAL
 *    levels.  Renamed functions and files.  Added support for amplitude
 *    magnitude types.  Reformatted makefiles.
 *
 *    Revision 1.3  2000/01/07 17:31:33  davidk
 *    removed the printf() error message that complains about an
 *    unrecognized web option.  The error is still logged to the
 *    logfile.  This was done because sometimes there are
 *    superfluous optoins (like the value of a submit button),
 *    and we don't want to complain about those back to the user.
 *
 *    Revision 1.2  1999/11/09 16:49:48  lucky
 *    *** empty log message ***
 *
 *    Revision 1.1  1999/10/20 23:56:24  davidk
 *    Initial revision
 *
 *    Revision 1.1  1999/05/05 18:05:41  lucky
 *    Initial revision
 *
 *
 */
  
/*****************************************************************

   filename:       webhelper.c 
   module:         eqparam2html
   platforms:      solaris 2.7 (& maybe NT 4)
   date:           04/20/1999
   bug creator:    David Kragness

   description:   

   This file contains code that acts as a go between for
   applications and the webparse EWDB library routines, in
   the interest of getting parameters from the Web CGI_BIN
   interface.

  Topics:

*****************************************************************/

  
/* Include the normal junk */
#include <webparse.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* Length of the word "EVENT" */
#define EVENT_LEN 5

/* External flag for debug output 0=No, otherwise = YES */
extern int DEBUG;

/* Homemade struct for grabbing event webparams */
typedef struct _WebOptionsStruct2
{
  int NumEvents;
  int * pEvents;
  int LastEvent;
} WebOptionsStruct2;


/* External functions */
void   logit_init( char *prog, short mid, int bufSize, int logflag );
void   logit( char *, char *, ... );          /* Function prototype  */
/* End external functions */


int InputEventIds(int ** pEventList, int * pNumEventsToRetrieve)
/******************************************************
  Function:    InputEventIds()
  Purpose:     InputEventIds is a highlevel function called 
               from an application that gets a list of 
               events for the app to deal with.
  Parameters:    
      Output
      pEventList:
               Pointer to a list of event IDs
      pNumEventsToRetrieve:
               Pointer to the number of event IDs in the list

  Author:
               DK, before 04/15/1999, actually it was probably
               written by Dietz in 1998, and then screwed up
               by DK in 1999.

  Internal Functions Called:            
  Library Functions Called: 
               GetAndProcessWebParams()
  EW Library Functions Called:
  External Library Functions Called:
  System Functions Called:  
               memset(),sizeof()
  **********************************************************/
{

  WebOptionsStruct2 Options;
  int RetVal;

  /* initialized Options to all zeros */
  memset( &Options, 0, sizeof(WebOptionsStruct2) );

  RetVal=Webparse_GetAndProcessWebParams((void *)(&Options));
  if(RetVal)
  {
    logit("","GetAndProcessWebParams() failed to read params from web.\n");
    printf("","eqparam2html(): Error occured in GetAndProcessWebParams().\n"
              "<br>\nExiting!!!\n<br>\n");
    return(-1);
  }

  /* Copy the Options struct variables back to the user passed vars. */
  *pEventList=Options.pEvents;
  *pNumEventsToRetrieve=Options.LastEvent;

  return(0);

}  /* End of InputEventIds() */


int Webparse_Client_SetVars(char * szVar, char * szVal, void * pUserParams)
/******************************************************
  Function:    Webparse_Client_SetVars()
  Purpose:     Webparse_Client_SetVars() is a callout 
               function provided by 
               the web_common (webparse) routines that parse
               web parameters passed during a CGI-BIN GET or
               POST request.  Webparse_Client_SetVars() is 
               called for each parameter - value pair
  Parameters:    
      Input
      szVar:   a string containing the name of the variable
      szVal:   a string containing the value of the variable

      Output
      pOptions:
               a (void *) that is passed to the web_common 
               routines during the initial call to 
               GetAndProcessWebParams().  It is a user definable
               pointer, that the web_common routines are nice
               enough to pass through to this SetVars() function.
               You can tell I wrote the web_common routines, 
               because I only say nice things about them.
  Author:
               DK, before 04/15/1999

  Internal Functions Called:
  Library Functions Called: 
               logit()
  EW Library Functions Called:
  External Library Functions Called:
  System Functions Called:  
               atoi(),strcmp(),malloc(),strncmp(),printf()
  **********************************************************/
{

/* User Callout function SetVars, passes the user a Variable, it's string value,
   and a pointer to User Defined Params, so that the user can take whatever
   desired action they want in the SetVars() function. */

  WebOptionsStruct2 * pOptions= (WebOptionsStruct2 *) pUserParams;

    if(DEBUG)
      logit("","SetVars():Expression: %s = %s\n",szVar,szVal);

    if(strcmp(szVar,"NUMEVENTS") == 0)
    {
      if((pOptions->NumEvents=atoi(szVal)) <= 0)
        return(-1);
      else
      {
        pOptions->pEvents=(int *)malloc(pOptions->NumEvents * sizeof(int));
        pOptions->LastEvent=0;
      }
    }
    else
    if(strncmp(szVar,"EVENT",EVENT_LEN) == 0)
    {
      if( pOptions->NumEvents <= 0 )
      {
        pOptions->NumEvents=1;
        pOptions->pEvents=(int *)malloc(pOptions->NumEvents * sizeof(int));
        pOptions->LastEvent=0;
      }
      pOptions->pEvents[pOptions->LastEvent++]=atoi(&(szVar[EVENT_LEN]));
    }
    else
    {
      logit("","Unrecognized Web Option : %s = %s\n",szVar,szVal);
    }
    return(0);
}  /* End of SetVars() */
