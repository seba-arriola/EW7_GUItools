
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: EQRwebhelper.c,v 1.3 2002/05/28 17:25:41 lucky Exp $
 *
 *    Revision history:
 *    $Log: EQRwebhelper.c,v $
 *    Revision 1.3  2002/05/28 17:25:41  lucky
 *    *** empty log message ***
 *
 *    Revision 1.2  2002/03/22 20:02:38  lucky
 *    Pre v6.1 checkin
 *
 *    Revision 1.1  2001/07/01 21:55:21  davidk
 *    Initial revision
 *
 *    Revision 1.3  2001/06/21 21:26:25  lucky
 *    State of the code after LocalMag review was implemented and at least partially
 *    tested. The new Lomax applet (june 18 version) has been incorporated. ML's can
 *    be added, deleted, modified, as well as the other types of picks.
 *
 *    Revision 1.2  2001/05/15 18:47:36  davidk
 *    Moved functions around between the apps, DB API, and DB API INTERNAL
 *    levels.  Renamed functions and files.  Added support for amplitude
 *    magnitude types.  Reformatted makefiles.
 *
 *    Revision 1.1  2001/02/28 17:33:07  lucky
 *    Initial revision
 *
 *    Revision 1.1  2001/02/28 17:30:23  lucky
 *    Initial revision
 *
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
   applications and the webparse library routines, in
   the interest of getting parameters from the Web CGI_BIN
   interface.

  Topics:

*****************************************************************/

  
/* Include the normal junk */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <webparse.h>
#include <review.h>


/* External flag for debug output 0=No, otherwise = YES */
extern int DEBUG;


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

  WebOptionsStruct * pOptions= (WebOptionsStruct *) pUserParams;

    if(strcmp(szVar,"EventID") == 0)
    {
      pOptions->idEvent = atoi(szVal);
    }
	else if(strcmp(szVar,"OriginID") == 0)
    {
		pOptions->idOrigin=atoi(szVal);
	}
	else if(strcmp(szVar,"ShowMap") == 0)
    {
		pOptions->ShowMap=atoi(szVal);
	}
	else if(strcmp(szVar,"ShowUnpicked") == 0)
    {
		pOptions->ShowUnpicked=atoi(szVal);
	}
	else if(strcmp(szVar,"Action") == 0)
    {
		pOptions->Action = atoi(szVal);
	}
	else if(strcmp(szVar,"Type") == 0)
    {
		pOptions->Type = atoi(szVal);
	}
    else
    {
      logit ("" ,"Unrecognized Web Option : %s = %s\n",szVar,szVal);
    }
    return(0);
}  /* End of SetVars() */
