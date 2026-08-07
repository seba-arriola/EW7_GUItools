
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_ew_oci_base.c,v 1.4 2002/04/16 20:44:29 davidk Exp $
 *    Revision history:
 *
 *    $Log: ewdb_ew_oci_base.c,v $
 *    Revision 1.4  2002/04/16 20:44:29  davidk
 *    Slowed down the "DB not connected" messages so that they only come
 *    every 20 minutes instead of every 2.
 *    Added a THREAD_RETURN macro to get rid of warning messages on NT.
 *
 *    Revision 1.3  2001/05/15 02:16:33  davidk
 *    Moved functions around between the apps, DB API, and DB API INTERNAL
 *    levels.  Renamed functions and files.  Added support for amplitude
 *    magnitude types.  Reformatted makefiles.
 *
 *    Revision 1.2  2001/04/06 19:34:56  davidk
 *    modified to support the changes to ewdb_ew_oci_base.h.
 *
 *    Revision 1.1  2001/02/28 17:19:22  lucky
 *    Initial revision
 *
 *    Revision 1.8  2001/02/21 08:58:28  davidk
 *    changed the base layer include from ewdb_oci_base.h to ewdb_cli_base.h
 *    as part of the oci_base to db_cli_base conversion.  See ewdb_cli_base.h
 *    for more details on the change.
 *
 *    Revision 1.7  2001/01/09 16:15:24  lucky
 *    Added EWDB_SetOraConnectionTimeout
 *
 *    Revision 1.6  2000/07/05 20:22:58  davidk
 *    added time headers to logits, to improve info for debugging.
 *
 *    Revision 1.5  2000/06/21 22:52:32  lucky
 *     Cleaned up logit calls to make log files more readable.
 *
 *    Revision 1.4  1999/11/09 18:42:46  lucky
 *    *** empty log message ***
 *
 *    Revision 1.3  1999/10/19 18:20:25  lucky
 *    *** empty log message ***
 *
 *    Revision 1.2  1999/10/19 05:01:46  davidk
 *    changed the members of structures in ewdb_ora_api.h (cosmetic),
 *    and changed the return code Constants.  Made matching changes
 *    in this file
 *
 *    Revision 1.1  1999/05/05 18:41:01  lucky
 *    Initial revision
 *
 *
 */

#include "ewdb_system_support.h"  
#include <time.h>

#include "ewdb_cli_base.h"

#define INCLUDE_FULL_EW_OCI_HEADER
#include "ewdb_ew_oci_base.h"

static int DBConnMgr_TERMINATE=FALSE;
static time_t tLastOraAPIAction=0;



thr_ret ewdb_base_ConnMgr( void* dummy )
{
  time_t now;
  time_t tLastConnReport = 0;
  while(!DBConnMgr_TERMINATE)
  {
    if((time(&now) - tLastOraAPIAction) > EWDB_MAX_IDLE_TIME)
    {
      if(ewdb_base_ConnectedToDB())
      {
        logit("t","DBConnMgr, disconnecting from Oracle.\n");
        ewdb_base_Disconnect(EWDB_RETURN_SUCCESS);
      }
      else
      {
        if(now - tLastConnReport > 10 * EWDB_MAX_IDLE_TIME)
        {
          logit("t","DBConnMgr, DB not connected.\n");
          time(&tLastConnReport);
        }
      }
    }
    sleep_ew(EWDB_MAX_IDLE_TIME * 1000);
  }
  THREAD_RETURN(EWDB_RETURN_SUCCESS);
}  /* end EWDB_ConnMgr() */


void ewdb_base_SetLastOraAPIActionTime(void)
{
  time(&tLastOraAPIAction);
}


void ewdb_base_TerminateConnMgr(void)
{
  DBConnMgr_TERMINATE=TRUE;
}

void ewdb_base_SetOraConnectionTimeout(int tsec)
{
  time(&tLastOraAPIAction);
  tLastOraAPIAction += tsec;
}

