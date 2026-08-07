
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: eqreview.c,v 1.5 2001/02/28 17:30:23 lucky Exp $
 *
 *    Revision history:
 *    $Log: eqreview.c,v $
 *    Revision 1.5  2001/02/28 17:30:23  lucky
 *    Massive schema redesign and cleanup.
 *
 *    Revision 1.4  2000/12/06 17:49:21  lucky
 *    *** empty log message ***
 *
 *    Revision 1.3  2000/09/18 17:24:16  lucky
 *    Final version before v5.1
 *
 *    Revision 1.2  2000/08/09 16:58:31  lucky
 *    Lint Cleanup
 *
 *    Revision 1.1  2000/08/07 19:48:27  lucky
 *    Initial revision
 *
 *
 *
 *
 */
  
/*****************************************************************

   filename:       eqreview.c 
   module:         eqreview
   author:         Lucky Vidmar

   description:   

 *  Given an eventid, eqreview retrieves the hypoinverse arc file from 
 *  the database. It then reads the file and fills the DB structs 
 *  with the event info. 
 *  Then, eqreview puts up an html form that allows the user to 
 *  review and modify the event parameters. 
 * 

*****************************************************************/


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

/* I forget what we grab out of unistd, but we grab something
   and to support multiple platforms this ugly ifdef is needed */
#ifndef _WINNT  /* DavidK 990119 */
# include <unistd.h> 
#endif

/* include earthworm headers */
#include <earthworm.h>
#include <webparse.h>
#include <kom.h>

/* include our own header file */
#include "../include/review_function.h"

main ()
{
	WebOptionsStruct 			WebParams;

	/* initialize Web Parameters to all zeros */
	memset (&WebParams, 0, sizeof(WebOptionsStruct));
	if (Webparse_GetAndProcessWebParams((void *)(&WebParams)) != 0)
	{
		printf ("Call to Webparse_GetAndProcessWebParams failed.\n");
		exit (-1);
	}

	ReviewFunction (WebParams.idEvent, WebParams.Action, WebParams.ShowMap);

	exit (0);

}
