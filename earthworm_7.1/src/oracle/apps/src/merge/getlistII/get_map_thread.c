
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: get_map_thread.c,v 1.4 2001/02/28 17:29:10 lucky Exp $
 *    Revision history:
 *
 *    $Log: get_map_thread.c,v $
 *    Revision 1.4  2001/02/28 17:29:10  lucky
 *    Massive schema redesign and cleanup.
 *
 *    Revision 1.3  2000/09/20 16:41:00  lucky
 *    Cleanup many un-needed logit calls
 *
 *    Revision 1.2  1999/11/09 16:53:02  lucky
 *    *** empty log message ***
 *
 *    Revision 1.1  1999/10/25 18:52:32  davidk
 *    Initial revision
 *
 *    Revision 1.2  1999/09/24 04:34:45  davidk
 *    changed include statement to include the system map_display_structs.h instead of the local
 *    one.
 *
 *    Revision 1.1  1999/05/05 18:19:24  lucky
 *    Initial revision
 *
 *
 */
  
  /* T1 Call the map server to get the base map */
  
#include <platform.h>
#include <time_ew.h>

#include <mapserver.h>
#include <map_display_structs.h>
#include "getlist.h"

/* externally defined functions */

/* Defined in earthworm.h */
void CreateSpecificMutex( mutex_t * ); 
int  StartThread( thr_ret (void *), unsigned, unsigned * );
void RequestSpecificMutex( mutex_t * );
void ReleaseSpecificMutex( mutex_t * );

/* End of Externally defined functions */



/* Params I may be passed from the web browser/server.
   Filled out by SetVars().  */
extern SetVarsUserStruct WebParams;
extern SetVarsUserStruct DefaultParams;
extern mutex_t MapServerMutex;

thr_ret GetMapThread( void* dummy )
/******************************************************
  Function:    GetMapThread()
  Purpose:     GetMapThread() is a thread that calls the
               neccessary EWDB mapserver routines to
               get a map of a desired region.
  Parameters:    
      Input
      dummy:   unused

      Global Variable WebParams:
               Mother of all structs, contains map data,
               image data, and map filename data that we
               must pass to the mapserver routines, so that
               the proper map can be built, along with all
               data needed to generate the GUI State String.
               
      Global Variable DefaultParams:
               Mother of all structs, contains default data,
               used in call to the GUI State String generator
               so that only non-default values in WebParams
               are recorded.
  Author:
               DK, before 04/15/1999

  Internal Functions Called:
               CreateGUIStateStringFromWebParams()
               

  Library Functions Called: 
               logit(),GetMapFromMapServer()

  EW Library Functions Called:
               RequestSpecificMutex(),hrtime_ew(),
               ReleaseSpecificMutex()

  External Library Functions Called:

  System Functions Called:  
               

  **********************************************************/
{

  /* Grab our mutex, so that the rest of the program waits for us. */
  RequestSpecificMutex( &MapServerMutex );


  GetMapFromMapServer(&(WebParams.LPSData),&(WebParams.MSISData));


  /* Compare the WebParams struct to the DefaultParams struct, and copy all
     the modified params (that we care about) from the WebParams struct to
     the GUIStateString */
  CreateGUIStateStringFromWebParams(&WebParams, &DefaultParams,
                                    WebParams.WGSSData.GUIStateString);


  /* Release our mutex, so that the fun can continue. */
  ReleaseSpecificMutex( &MapServerMutex );

  /* This return is a fickle spot, because thr_ret is defined as multiple
     types on multiple platforms, including void in WIN32 */
  return((thr_ret)0);
}
