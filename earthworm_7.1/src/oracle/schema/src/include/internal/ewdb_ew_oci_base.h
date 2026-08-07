/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_ew_oci_base.h,v 1.3 2001/05/15 02:16:27 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_ew_oci_base.h,v $
 *     Revision 1.3  2001/05/15 02:16:27  davidk
 *     Moved functions around between the apps, DB API, and DB API INTERNAL
 *     levels.  Renamed functions and files.  Added support for amplitude
 *     magnitude types.  Reformatted makefiles.
 *
 *     Revision 1.2  2001/04/06 18:30:12  davidk
 *     Added INCLUDE_FULL_EW_OCI_HEADER compiler definition,
 *     that allows the Connection Manager stuff to be
 *     included only by files that need it, while the
 *     other 99% of the files can get access to
 *     the ewdb_base_SetLastOraAPIActionTime() prototype for
 *     compilation.
 *
 *     Revision 1.1  2001/04/06 18:22:33  davidk
 *     Initial revision
 *
 *     Revision 1.2  2001/01/09 16:15:10  lucky
 *     Added prototype for EWDB_SetOraConnectionTimeout
 *
 *     Revision 1.1  1999/11/09 18:38:26  lucky
 *     Initial revision
 *
 *
 */


#ifndef EWDB_EW_OCI_BASE_H
# define EWDB_EW_OCI_BASE_H

/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW5 API FORMATTED COMMENT
TYPE FUNCTION_PROTOTYPE

LIBRARY  EWDB_API_LIB

SUB_LIBRARY EWDB_BASE

LANGUAGE C

LOCATION THIS_FILE


FUNCTION ewdb_base_SetLastOraAPIActionTime

RETURN_TYPE void

DESCRIPTION Sets the time of the last ora_api action call
to NOW.  The connection manager will close the connection 
to the database EWDB_MAX_IDLE_TIME seconds 
after the last API call action.

*************************************************
************************************************/
void ewdb_base_SetLastOraAPIActionTime(void);


/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW5 API FORMATTED COMMENT
TYPE FUNCTION_PROTOTYPE

LIBRARY  EWDB_API_LIB

SUB_LIBRARY EWDB_BASE

LANGUAGE C

LOCATION THIS_FILE


FUNCTION ewdb_base_SetOraConnectionTimeout

RETURN_TYPE void

PARAMETER 1
PARAM_NAME tsec
PARAM_TYPE int
PARAM_DESCRIPTION The number of seconds from now that
the connection manager should timeout the connection and
close it.  (Should obviously be > 0)

DESCRIPTION Sets the connection timeout, so that the
connection manager will not close the database connection
until NOW + tsec seconds.  This call is useful if you
know that a database call will take a long time.  DeleteEvent()
is an excelent example.  The DB connection may timeout
if you simply use ewdb_base_SetLastOraAPIActionTime(),
whereas you can call ewdb_base_SetOraConnectionTimeout()
to set the timeout to 15 minutes, so that you don't get a
disconnect during the delete.  You can then call 
ewdb_base_SetOraConnectionTimeout() after the DB call has 
executed in order to reset the timeout to normal.

*************************************************
************************************************/
void ewdb_base_SetOraConnectionTimeout(int tsec);
/* sets the connection timeout - we need this
   for long running calls such as DeleteEvent
**********************************/

# ifdef INCLUDE_FULL_EW_OCI_HEADER
#  define EWDB_MAX_IDLE_TIME 120
/* number of seconds after the latest oracle
   related action that the Connection Manager
   will shutdown the connection.
********************************/


/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW5 API FORMATTED COMMENT
TYPE FUNCTION_PROTOTYPE

LIBRARY  EWDB_API_LIB

SUB_LIBRARY EWDB_BASE

LANGUAGE C

LOCATION THIS_FILE


FUNCTION ewdb_base_ConnMgr

RETURN_TYPE thr_ret

PARAMETER 1
PARAM_NAME dummy
PARAM_TYPE void *
PARAM_DESCRIPTION Default parameter.  Unused by this
function.

DESCRIPTION The connection manager thread.  This thread
runs periodically and brings the connection to the DB 
down if it has timed out(been inactive for a certain
amount of time.)  By having a connection manager, the
DB connection can be left open in between consecutive
DB calls, yet still closed when not in use.  The idea
behind the connection mgr is to optimize the efficiency
of accessing the DB, relative to limiting the use of DB
licenses and the overhead time required to raise a DB 
connection.

*************************************************
************************************************/
thr_ret ewdb_base_ConnMgr( void* dummy );
/* starts the connection manager thread */


/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW5 API FORMATTED COMMENT
TYPE FUNCTION_PROTOTYPE

LIBRARY  EWDB_API_LIB

SUB_LIBRARY EWDB_BASE

LANGUAGE C

LOCATION THIS_FILE


FUNCTION ewdb_base_TerminateConnMgr

RETURN_TYPE void

DESCRIPTION Causes the connection manager to shutdown.
It may take a couple of seconds for the connection 
manager to shutdown if it is currently asleep.
Should only be called when terminating use of the 
DB and API code.  It SHOULD NOT be called by individual
functions when they are finished using the DB.

*************************************************
************************************************/

# endif /* INCLUDE_FULL_EW_OCI_HEADER */

#endif /* EWDB_EW_OCI_BASE_H */
