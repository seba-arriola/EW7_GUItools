
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: AlarmsWebhelper.c,v 1.2 2001/07/31 20:45:44 lucky Exp $
 *
 *    Revision history:
 *    $Log: AlarmsWebhelper.c,v $
 *    Revision 1.2  2001/07/31 20:45:44  lucky
 *    Changed alarms nomenclature from User to Recipient
 *
 *    Revision 1.1  2001/07/01 21:55:20  davidk
 *    Initial revision
 *
 *    Revision 1.1  2001/05/18 19:07:49  lucky
 *    Initial revision
 *
 *    Revision 1.1  2001/05/15 02:15:02  davidk
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

  
#include <alarms.h>

/* External flag for debug output 0=No, otherwise = YES */
extern int DEBUG;


/* External functions */
void   logit_init( char *prog, short mid, int bufSize, int logflag );
void   logit( char *, char *, ... );          /* Function prototype  */
/* End external functions */



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

  WebAlarmsOptions * pOptions= (WebAlarmsOptions *) pUserParams;

    if(strcmp(szVar,"RecipientId") == 0)
    {
      pOptions->idRecipient = atoi(szVal);
    }
	else
    if(strcmp(szVar,"RuleID") == 0)
    {
		pOptions->idRule=atoi(szVal);
	}
    else
    {
      html_logit ("" ,"Unrecognized Web Option : %s = %s\n",szVar,szVal);
    }
    return(0);
}  /* End of SetVars() */
