/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_ora_api.c,v 1.5 2004/12/28 16:23:08 mark Exp $
 *
 *    Revision history:
 *     $Log: ewdb_ora_api.c,v $
 *     Revision 1.5  2004/12/28 16:23:08  mark
 *     Terminate connection manager thread on shutdown
 *
 *     Revision 1.4  2001/07/23 17:17:23  davidk
 *     API Cleanup.
 *     Removed excess RCS header comments leftover from when this file contained
 *     a lot of the top-level API function source code.
 *     Removed excess #includes.
 *     Removed unneccessary function prototypes.
 *     Upped the revision# to EW5 Oracle API Rev 1.50  2001/06/10
 *
 *     Revision 1.3  2001/05/15 02:16:34  davidk
 *     Moved functions around between the apps, DB API, and DB API INTERNAL
 *     levels.  Renamed functions and files.  Added support for amplitude
 *     magnitude types.  Reformatted makefiles.
 *
 *     Revision 1.2  2001/04/06 19:34:10  davidk
 *     modified to support the changes to ewdb_ew_oci_base.h.
 *
 *     Revision 1.1  2001/02/28 17:19:22  lucky
 *     Initial revision
 *
 *
 *
 */


#include <ewdb_ora_api.h>
#include <ewdb_cli_base.h>


/* INCLUDE_FULL_EW_OCI_HEADER is needed to get all of the stuff
   out of the ewdb_ew_oci_base, including the threaded function
   prototypes. 
***************************************************************/
#define INCLUDE_FULL_EW_OCI_HEADER
#include "ewdb_ew_oci_base.h"


/* needed for performance measurements otherwise the floating 
   point stack on NT becomes hosed (for lack of a better word.  */
#include <time_ew.h>  

/* needed for internal functions */

/*****************************
 Connection Mgr thread stuff
 *****************************/
#define EWDB_THREAD_STACK  8192

static unsigned int   tidDBConnMgr;  /* id of dispatcher of socket-serving threads */


/* EWDB Initialization flag */
static int EWDB_Initialized=FALSE;

/* Debug flag */
int EWDB_Debug=0;


/*****************************
 Source Code
 *****************************/


/* Initialization function for the API.  Call this function at startup,
   to allow the API to initialize itself, and checks its ability to 
   function and connect to an Oracle database 
**********************************************************************/
int ewdb_api_Init(char * DBuser, char * DBpassword, char * DBservice)
{

  if(EWDB_Initialized)
  {
    logit("", "ewdb_api_Init():  Already initialized! Returning.\n");
    return(EWDB_RETURN_WARNING);
  }
  else
  {
    EWDB_Initialized=TRUE;
  }

  if (StartThread(ewdb_base_ConnMgr, (unsigned)EWDB_THREAD_STACK, &tidDBConnMgr) == -1)
  {
    logit("", "ewdb_api_Init(): error starting Connection Manager. Exiting.\n");
    exit(-1);
  }

  ewdb_base_SetLastOraAPIActionTime();

  logit("",
        /* ####SOON TO BE AUTOMATED DATE MESSAGE#### */
        "EW5 Oracle API Rev 1.50  2001/06/10\n"
        );


  return(ewdb_base_Init(DBuser,DBpassword,DBservice));
}


/* Shutdown all API functionality and free any related memory
**********************************************************************/
int ewdb_api_Shutdown(void)
{
  if(!EWDB_Initialized)
  {
    logit("", "ewdb_api_OraAPIShutdown():  EWDB not initialized! Returning.\n");
    return(EWDB_RETURN_WARNING);
  }
  else
  {
    EWDB_Initialized=FALSE;
  }
  ewdb_base_TerminateConnMgr();
  return(ewdb_base_Shutdown());
}


/* Set the Debug Level for the API.  See  EWDB_DEBUG_DB_BASE_NONE
   in ewdb_ora_api.h for a description of the different debug levels.
   Obviously they are not globally supported.
**********************************************************************/
void ewdb_api_Set_Debug(int IN_iDebug)
{
  EWDB_Debug = IN_iDebug;
  ewdb_base_Set_CLI_Debug(EWDB_Debug);
}
