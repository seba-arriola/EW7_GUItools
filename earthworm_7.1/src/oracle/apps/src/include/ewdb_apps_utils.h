/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_apps_utils.h,v 1.25 2005/05/12 20:49:16 mark Exp $
 *
 *    Revision history:
 *     $Log: ewdb_apps_utils.h,v $
 *     Revision 1.25  2005/05/12 20:49:16  mark
 *     Added comments functions
 *
 *     Revision 1.24  2005/04/25 16:58:22  davidk
 *     Added EWDB_SnippetStruct definition, which was removed
 *     from ewdb_api_waveform.h due to incompatibility problems
 *     betwee ws_clientII.h and ws_clientIIx.h.
 *
 *     Revision 1.23  2005/02/23 16:41:14  mark
 *     Added comments
 *
 *     Revision 1.22  2005/02/04 23:23:51  mark
 *     Added alarms delivery-only functions
 *
 *     Revision 1.21  2005/02/01 20:41:34  mark
 *     Added GetRecipientAlarmDelivery function
 *
 *     Revision 1.20  2004/11/23 20:42:43  mark
 *     Added polygon prototypes
 *
 *     Revision 1.19  2004/11/10 17:07:51  davidk
 *     Added ewdb_apps_GetLatestOriginForEventBySource().
 *
 *     Revision 1.18  2004/05/05 17:51:21  davidk
 *     Added ewdb_apps_GetArrivalsFromDB() prototype.
 *
 *     Revision 1.17  2003/08/05 18:47:36  lucky
 *     Removed prototype for ewdb_apps_PutDBEventInfo_DBPicks; this is now under NEIC
 *
 *     Revision 1.16  2003/07/01 19:31:42  lucky
 *     Updated prototype for ewdb_apps_PutDBEventInfo_DBPicks
 *
 *     Revision 1.15  2003/06/24 16:19:53  lucky
 *     Added ewdb_apps_PutDBEventInfo_DBPicks
 *
 *     Revision 1.14  2003/03/11 17:15:06  lucky
 *     *** empty log message ***
 *
 *     Revision 1.13  2003/01/30 23:12:57  lucky
 *     *** empty log message ***
 *
 *     Revision 1.12  2002/05/28 17:24:05  lucky
 *     *** empty log message ***
 *
 *     Revision 1.11  2002/03/22 18:23:18  lucky
 *     Added retrieval of unpicked snippets on demand to speed things up
 *
 *     Revision 1.9  2001/09/26 21:43:40  lucky
 *     Added support for multi-screen event displays.
 *
 *     Revision 1.8  2001/09/10 20:47:25  lucky
 *     Added station grouping of channels
 *
 *     Revision 1.7  2001/08/06 21:28:39  lucky
 *     *** empty log message ***
 *
 *     Revision 1.6  2001/07/28 00:43:14  lucky
 *     State of the code after debugging and testing prior to v6.0 release freeze.
 *
 *     Revision 1.5  2001/07/20 17:00:00  davidk
 *     *** empty log message ***
 *
 *     Revision 1.4  2001/07/01 21:55:16  davidk
 *     Cleanup of the Earthworm Database API and the applications that utilize it.
 *     The "ewdb_api" was cleanup in preparation for Earthworm v6.0, which is
 *     supposed to contain an API that will be backwards compatible in the
 *     future.  Functions were removed, parameters were changed, syntax was
 *     rewritten, and other stuff was done to try to get the code to follow a
 *     general format, so that it would be easier to read.
 *
 *     Applications were modified to handle changed API calls and structures.
 *     They were also modified to be compiler warning free on WinNT.
 *
 *     Revision 1.3  2001/05/15 02:15:20  davidk
 *     Moved functions around between the apps, DB API, and DB API INTERNAL
 *     levels.  Renamed functions and files.  Added support for amplitude
 *     magnitude types.  Reformatted makefiles.
 *
 *     Revision 1.2  2001/04/17 17:33:09  davidk
 *     reformatted whitespace.
 *
 *     Revision 1.1  2001/02/28 17:29:10  lucky
 *     Initial revision
 *
 *
 *
 *
 */

#ifndef EWDB_APPS_UTILS_H
# define EWDB_APPS_UTILS_H




/**********************************
* INCLUDES                        *
***********************************/


#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <ew_event_info.h>
#include <ws_clientII.h>


/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW API FORMATTED COMMENT
TYPE LIBRARY

LIBRARY  EWDB_APPS_LIB

LOCATION THIS_FILE

DESCRIPTION This is a set of functions that serves
the newer set of apps (DB, Interactive Review,
Alarms, etc.)  It includes two subsets: DB_LIB and
SUPPORT_LIB.  DB_LIB includes functionality to 
access the DB, and SUPPORT_LIB includes functionality
for accessing map servers, and dealing with web protocols,
and anything else that was designed to support the
new line of apps.


*************************************************
************************************************/


/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW API FORMATTED COMMENT
TYPE LIBRARY

LIBRARY  EWDB_APPS_LIB

SUB_LIBRARY DB_LIB

LOCATION THIS_FILE

DESCRIPTION This is a set of functions that lies
outside of the EWDB_API_LIB.  The functions 
provide methods of accessing DB data via the
EWDB_API_LIB that other developers have found
convenient and useful.  You can always access the
data by going directly to the EWDB_API_LIB functions,
but you may find it much more convenient to use the
functions in this library.  There are also some 
functions in this library that are the official way
of inserting or retrieving a large block of data 
like a Hypoinverse Archive message.


*************************************************
************************************************/


/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW5 API FORMATTED COMMENT
TYPE TYPEDEF

LIBRARY  EWDB_APPS_LIB

SUB_LIBRARY DB_LIB

LANGUAGE C

LOCATION THIS_FILE

TYPEDEF EWDB_SnippetStruct
TYPE_DEFINITION TRACE_REQ
DESCRIPTION Uh. I don't know.  We are using EWDB_SnippetStruct for a
Snippet data entry struct or something, and we are copying it from the
TRACE_REQ definition in the Wave_Server Client II routines
<ws_clientII.h>  Help!

*************************************************
************************************************/
typedef TRACE_REQ EWDB_SnippetStruct;



/**********************************
* FUNCTION PROTOTYPES             *
***********************************/



/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW5 API FORMATTED COMMENT
TYPE FUNCTION_PROTOTYPE

LIBRARY  EWDB_APPS_LIB

SUB_LIBRARY DB_LIB

LANGUAGE C

LOCATION THIS_FILE

STABILITY NEW

FUNCTION ewdb_apps_ArcFile2DB

SOURCE_LOCATION oracle/apps/src/libsrc/ewdb_apps_ArcFile2DB.c

RETURN_TYPE int

RETURN_VALUE EW_SUCCESS
RETURN_DESCRIPTION Successful operation

RETURN_VALUE EW_FAILURE
RETURN_DESCRIPTION  Failure -- see logfile for details.

PARAMETER 1
PARAM_NAME ArcFileName
PARAM_TYPE char *
PARAM_DESCRIPTION Name of Hypoinverse arc file to insert.

PARAMETER 2
PARAM_NAME author
PARAM_TYPE char *
PARAM_DESCRIPTION Author to be assigned to the event.

PARAMETER 3
PARAM_NAME szEventID
PARAM_TYPE char *
PARAM_DESCRIPTION External author ID.

DESCRIPTION  Reads the hypoinverse arc file, converts it
into DB event format and inserts it into he database.

*************************************************
************************************************/
int ewdb_apps_ArcFile2DB(char *ArcFileName, char *author, char *szEventID);


/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW5 API FORMATTED COMMENT
TYPE FUNCTION_PROTOTYPE

LIBRARY  EWDB_APPS_LIB

SUB_LIBRARY DB_LIB

LANGUAGE C

LOCATION THIS_FILE

STABILITY NEW

FUNCTION ewdb_apps_RetrieveIdChanExternal 

SOURCE_LOCATION oracle/apps/src/libsrc/ewdb_apps_RetrieveIdChanExternal.c

RETURN_TYPE int

RETURN_VALUE EWDB_RETURN_FAILURE
RETURN_DESCRIPTION Fatal error.  See logfile for details.

RETURN_VALUE EWDB_RETURN_SUCCESS
RETURN_DESCRIPTION Success.

PARAMETER 1
PARAM_NAME pStation
PARAM_TYPE EWDB_External_StationStruct *
PARAM_DESCRIPTION Pointer to an EWDB_External_StationStruct filled by
the caller.

DESCRIPTION Function creates or updates a component and associated
channel in the DB using an external station table and a a cheater table
to go from external station to chan.  Upon successful completion it
writes the DB ID of the new/update channel to pStation->idChan.  This
function ignores the pStation->idComp parameter.  
This function provides a convenient way to retrieve an idChan from an 
Earthworm SCN.  There is nothing official or exclusive about it.  It is
merely a convenience function that calls two others.

*************************************************
************************************************/
int ewdb_apps_RetrieveIdChanExternal(EWDB_External_StationStruct *pStation); 



/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW5 API FORMATTED COMMENT
TYPE FUNCTION_PROTOTYPE

LIBRARY  EWDB_APPS_LIB

SUB_LIBRARY DB_LIB

LANGUAGE C

LOCATION THIS_FILE

STABILITY NEW

FUNCTION ewdb_apps_DB2ArcFile

SOURCE_LOCATION oracle/apps/src/libsrc/ewdb_apps_DB2ArcFile.c

RETURN_TYPE int

RETURN_VALUE EW_SUCCESS
RETURN_DESCRIPTION Successful operation

RETURN_VALUE EW_FAILURE
RETURN_DESCRIPTION  Failure -- see logfile for details.

PARAMETER 1
PARAM_NAME idEvent
PARAM_TYPE EWDBid
PARAM_DESCRIPTION Database ID of the event to retrieve.

PARAMETER 2
PARAM_NAME ArcFileName
PARAM_TYPE char *
PARAM_DESCRIPTION Name of Hypoinverse arc file to write.

DESCRIPTION  Writes all information about the event in 
Hypoinverse archive format to the file.

*************************************************
************************************************/
int ewdb_apps_DB2ArcFile(EWDBid idEvent, char *ArcFileName);



/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW5 API FORMATTED COMMENT
TYPE FUNCTION_PROTOTYPE

LIBRARY  EWDB_APPS_LIB

SUB_LIBRARY DB_LIB

LANGUAGE C

LOCATION THIS_FILE

STABILITY NEW

FUNCTION ewdb_apps_GetDBEventInfo

SOURCE_LOCATION oracle/apps/src/libsrc/ewdb_apps_GetDBEventInfo.c

RETURN_TYPE int

RETURN_VALUE EWDB_RETURN_FAILURE
RETURN_DESCRIPTION  Fatal error.  See logfile for details.

RETURN_VALUE EWDB_RETURN_SUCCESS
RETURN_DESCRIPTION Success

PARAMETER 1
PARAM_NAME pEventInfo
PARAM_TYPE EWEventInfoStruct *
PARAM_DESCRIPTION Pointer to an event info struct.  This struct
must be alloced by the caller, but the function will alloc
any neccessary dynamic storage that is pointed to by the struct.

PARAMETER 2
PARAM_NAME idEvent
PARAM_TYPE EWDBid
PARAM_DESCRIPTION The EWDB ID of the Event for which the caller
wishes to retrieve data.

DESCRIPTION This is a convenient function for retrieving an 
Event from the DB.  Most data for an event is retrieved from 
the DB and stuffed into an EWEventInfoStruct.  
There are other low level functions that may be used to rerieve 
data from the DB, this one just provides a very convenient 
interface for retrieving DB Event data. 

NOTE This function is the official way to get an Event out of 
the DB and into an EWEventInfoStruct.  This is important 
because many review tools utilize this struct. <br> 
Please also see ewdb_apps_GetDBEventInfoII().

*************************************************
************************************************/
int ewdb_apps_GetDBEventInfo(EWEventInfoStruct * pEventInfo, EWDBid idEvent);

/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW5 API FORMATTED COMMENT
TYPE FUNCTION_PROTOTYPE

LIBRARY  EWDB_APPS_LIB

SUB_LIBRARY DB_LIB


LANGUAGE C

LOCATION THIS_FILE

STABILITY NEW

FUNCTION ewdb_apps_Set_GetDBEventInfo_Debug

SOURCE_LOCATION oracle/apps/src/libsrc/ewdb_apps_GetDBEventInfo.c

RETURN_TYPE int

RETURN_VALUE EWDB_RETURN_FAILURE
RETURN_DESCRIPTION  Fatal error.  See logfile for details.

RETURN_VALUE EWDB_RETURN_SUCCESS
RETURN_DESCRIPTION Success

PARAMETER 1
PARAM_NAME bDebug
PARAM_TYPE int
PARAM_DESCRIPTION Flag indicating the level of debugging
information that the ewdb_apps_GetDBEventInfoII() function
should provide.  Currently only TRUE and FALSE are supported.

DESCRIPTION Turns debugging info on and off for 
ewdb_apps_GetDBEventInfoII() and ewdb_apps_GetDBEventInfoII().

*************************************************
************************************************/
int ewdb_apps_Set_GetDBEventInfo_Debug (int bDebug);



/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW5 API FORMATTED COMMENT
TYPE FUNCTION_PROTOTYPE

LIBRARY  EWDB_APPS_LIB

SUB_LIBRARY DB_LIB

LANGUAGE C

LOCATION THIS_FILE

STABILITY NEW

FUNCTION ewdb_apps_GetDBEventInfoII

SOURCE_LOCATION oracle/apps/src/libsrc/ewdb_apps_GetDBEventInfo.c

RETURN_TYPE int

RETURN_VALUE EWDB_RETURN_FAILURE
RETURN_DESCRIPTION  Fatal error.  See logfile for details.

RETURN_VALUE EWDB_RETURN_SUCCESS
RETURN_DESCRIPTION Success

PARAMETER 1
PARAM_NAME pEventInfo
PARAM_TYPE EWEventInfoStruct *
PARAM_DESCRIPTION Pointer to an event info struct.  This struct
must be alloced by the caller, but the function will alloc
any neccessary dynamic storage that is pointed to by the struct.

PARAMETER 2
PARAM_NAME idEvent
PARAM_TYPE EWDBid
PARAM_DESCRIPTION The EWDB ID of the Event for which the caller
wishes to retrieve data.

PARAMETER 3
PARAM_NAME idOrigin
PARAM_TYPE EWDBid
PARAM_DESCRIPTION If >0, this is the EWDB ID of the Origin which will
be saved as the prefered origin. Otherwise, the origin from the Prefer
table will be returned. If the origin matching idOrigin is not bound 
with idEvent, an error is returned.

PARAMETER 4
PARAM_NAME Flags
PARAM_TYPE int
PARAM_DESCRIPTION Flags describing what types of data to retrieve
for the given event.  The following flags are supported, and should
be OR'd together into Flags.  
(GETDBEVENTINFO_PICKS, GETDBEVENTINFO_STAMAGS, GETDBEVENTINFO_COMPINFO,
 GETDBEVENTINFO_COOKEDTF, GETDBEVENTINFO_WAVEFORM_DESCS, 
 GETDBEVENTINFO_WAVEFORMS)
Summary info is always retrieved!!!!!

DESCRIPTION This function is similar to ewdb_apps_GetDBEventInfo(), 
however it provides control over what types of data are retrieved 
for an Event, thus making it much more flexible.
This is a convenient function for retrieving an 
Event from the DB.  Most data for an event is retrieved from 
the DB and stuffed into an EWEventInfoStruct.  
There are other low level functions that may be used to rerieve 
data from the DB, this one just provides a very convenient 
interface for retrieving DB Event data. 

NOTE This function is the official way to get an Event out of 
the DB and into an EWEventInfoStruct.  This is important 
because many review tools utilize this struct. <br> 
Please also see ewdb_apps_GetDBEventInfoII().

*************************************************
************************************************/
int ewdb_apps_GetDBEventInfoII(EWEventInfoStruct * pEventInfo, EWDBid idEvent, EWDBid idOrigin, int Flags);


/************************************************
************ SPECIAL FORMATTED COMMENT **********
TYPE FUNCTION_PROTOTYPE

LIBRARY  EWDB_APPS_LIB

SUB_LIBRARY DB_LIB

LANGUAGE C

LOCATION THIS_FILE


FUNCTION ewdb_apps_PutDBEventInfo

STABILITY NEW

SOURCE_LOCATION oracle/apps/src/libsrc/ewdb_apps_PutDBEventInfo.c

RETURN_TYPE int

RETURN_VALUE Default_Return_Value
RETURN_DESCRIPTION Description of the default return value

PARAMETER 1
PARAM_NAME *pEventInfo
PARAM_TYPE EWEventInfoStruct
PARAM_DESCRIPTION Pointer to an event info struct, that the caller
has allocated and filled with information for the event that they
wish to insert into the DB.

PARAMETER 2
PARAM_NAME *author
PARAM_TYPE char
PARAM_DESCRIPTION The author of the Event.  This should be the
caller, or the login of a human operating the calling program.

PARAMETER 3
PARAM_NAME *szEventID
PARAM_TYPE char
PARAM_DESCRIPTION The EventID given to the event by the author.

PARAMETER 4
PARAM_NAME NewEvent
PARAM_TYPE int
PARAM_DESCRIPTION Flag indicating whether the function should create a new
Event or associate the information with an existing one.

DESCRIPTION Function inserts the Event data from pEventInfo into the 
DB and associates it with either a new event or with an existing event,
depending on the value of the NewEvent flag.

NOTE When NewEvent is set to TRUE, the function creates an event, 
otherwise the function associates the event information with the 
DB Event identified by pEventInfo->Event.idEvent.

*************************************************
************************************************/
int ewdb_apps_PutDBEventInfo(EWEventInfoStruct *pEventInfo, char *author,
                   char *szEventID, int NewEvent);


/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW5 API FORMATTED COMMENT
TYPE FUNCTION_PROTOTYPE

LIBRARY  EWDB_APPS_LIB

SUB_LIBRARY DB_LIB

LANGUAGE C

LOCATION THIS_FILE

STABILITY NEW

FUNCTION ewdb_apps_putaway_StartSnippetEvent

SOURCE_LOCATION oracle/apps/src/libsrc/ewdb_apps_putaway.c

RETURN_TYPE int

RETURN_VALUE EWDB_RETURN_FAILURE
RETURN_DESCRIPTION Fatal error.  See logfile for details.

RETURN_VALUE EWDB_RETURN_SUCCESS
RETURN_DESCRIPTION Success.

PARAMETER 1
PARAM_NAME pidEvent
PARAM_TYPE EWDBid *
PARAM_DESCRIPTION A pointer to an EWDBid where the
function will write The DB identifier of the Event
it created.

PARAMETER 2
PARAM_NAME sSourceEventID
PARAM_TYPE char *
PARAM_DESCRIPTION The identifier given to the Event
by the author, in string format.  This is an
identifier external to the DB, not to be confused with
idEvent which is the identifier given by the DB.

PARAMETER 3
PARAM_NAME sAuthor
PARAM_TYPE char *
PARAM_DESCRIPTION The name of the author(source) of the
new Event.

PARAMETER 3
PARAM_NAME Debug
PARAM_TYPE int
PARAM_DESCRIPTION Flag indicating whether or not
debug messages should be written to the log file.
Set Debug to TRUE for debug log messages.

DESCRIPTION Function creates a new event in the
DB, and returns the idEvent DB identifier for
the new Event.

NOTE EWDB_ewdb_apps_putaway_StartSnippetEvent(), 
EWDB_ewdb_apps_putaway_NextSnippetForAnEvent()
and EWDB_ewdb_apps_putaway_CloseSnippetEvent() are modeled 
after the Earthworm trace disposal routines for use with
XXX_trace_save Earthworm programs like ora_trace_save,
trig2disk, and waveman2disk.  The logic behind the routines, 
is to call a StartEvent() function to open the processing of 
an event.  Next, NextSnippetForAnEvent() is called repeatedly 
for each of the snippets associated with the event.  
Finally CloseEvent() is called to end processing for an Event.  
Although the 3 aforementioned API functions support this 
processing model, they are not limited to it.  The caller can 
thus call EWDB_ewdb_apps_putaway_NextSnippetForAnEvent()
to add a snippet to a previous event.  The caller does
not have to call EWDB_ewdb_apps_putaway_StartSnippetEvent() before
calling EWDB_ewdb_apps_putaway_NextSnippetForAnEvent().  As long as the
caller provides a valid existing idEvent to
EWDB_ewdb_apps_putaway_NextSnippetForAnEvent(), the call should function
properly.  A side effect of this functionality is that
EWDB_ewdb_apps_putaway_CloseSnippetEvent() does nothing.

*************************************************
************************************************/
int ewdb_apps_putaway_StartSnippetEvent(EWDBid * pidEvent, char * sSourceEventID, 
                      char * sAuthor, int Debug);


/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW5 API FORMATTED COMMENT
TYPE FUNCTION_PROTOTYPE

LIBRARY  EWDB_APPS_LIB

SUB_LIBRARY DB_LIB

LANGUAGE C

LOCATION THIS_FILE

STABILITY NEW

FUNCTION ewdb_apps_putaway_NextSnippetForAnEvent

SOURCE_LOCATION oracle/apps/src/libsrc/ewdb_apps_putaway.c

RETURN_TYPE int

RETURN_VALUE EWDB_RETURN_FAILURE
RETURN_DESCRIPTION Fatal error.  See logfile for details.

RETURN_VALUE EWDB_RETURN_SUCCESS
RETURN_DESCRIPTION Success.

PARAMETER 1
PARAM_NAME idEvent
PARAM_TYPE EWDBid
PARAM_DESCRIPTION The database ID of an existing Event
with which the caller wishes to associate a new
waveform snippet.

PARAMETER 2
PARAM_NAME pSnippet
PARAM_TYPE EWDB_EW_SnippetStruct *
PARAM_DESCRIPTION A pointer to waveform snippet structure,
that includes parametric data describing a waveform
snippet along with the actual waveform snippet.

PARAMETER 3
PARAM_NAME OUT_pidWaveform
PARAM_TYPE EWDBid *
PARAM_DESCRIPTION A pointer to a EWDBid where the
function will write The DB identifier of the
waveform snippet it created.

PARAMETER 4
PARAM_NAME INOUT_pidChan
PARAM_TYPE EWDBid *
PARAM_DESCRIPTION A pointer to a EWDBid where the
function will write The DB channel identifier of
the channel of the waveform snippet it created.
If you already know the idChan for the channel
of the snippet, you can set (*INOUT_pidChan) to
the proper idchan, otherwise
WARNING!!!! (*INOUT_pidChan) should be set to 0, or the function
will attempt to treat it as a valid idchan.

DESCRIPTION Function inserts a waveform snippet along
with descriptor information, into the DB.  It associates
the snippet with the Event identified by idEvent.  It
returns the DB identifier of the new Waveform along
with the DB identifier of the Waveform's associated
channel.

*************************************************
************************************************/
int ewdb_apps_putaway_NextSnippetForAnEvent(EWDBid idEvent, EWDB_SnippetStruct * pSnippet,
                          EWDBid * OUT_pidWaveform,
                          EWDBid * INOUT_pidChan);


/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW5 API FORMATTED COMMENT
TYPE FUNCTION_PROTOTYPE

LIBRARY  EWDB_APPS_LIB

SUB_LIBRARY DB_LIB

LANGUAGE C

LOCATION THIS_FILE

STABILITY NEW

FUNCTION ewdb_apps_putaway_CloseSnippetEvent

SOURCE_LOCATION oracle/apps/src/libsrc/ewdb_apps_putaway.c

RETURN_TYPE int

RETURN_VALUE EWDB_RETURN_SUCCESS
RETURN_DESCRIPTION Success.

DESCRIPTION Function is a do nothing placeholder
routine that exists so the API waveform insertion
routines(see note under EWDB_ewdb_apps_putaway_StartSnippetEvent)
can resemble the Earthworm XXX_trace_save putaway
routines.

*************************************************
************************************************/
int ewdb_apps_putaway_CloseSnippetEvent();

#define		POPULATE_UH					-201
#define		POPULATE_REVIEW_PICKED		-202
#define		POPULATE_REVIEW_UNPICKED	-203

/* Alarms section */


#define 	ALARM_MSG_SIZE    	 	32000

/*  Apps Structures 
********************/

typedef struct _AlarmsRecipientDeliveryStruct
{
	EWDB_AlarmsRecipientStruct		Recipient;
	EWDB_AlarmDeliveryUnionStruct	Delivery[EWDB_ALARMS_MAX_RECIPIENT_DELIVERIES];
	int								NumDeliveries;
} AlarmsRecipientDeliveryStruct;


typedef struct _AlarmsRecipientStructRuleDelivery
{
    EWDB_AlarmsRecipientStruct		    Recipient;
    EWDB_AlarmDeliveryUnionStruct       Delivery[EWDB_ALARMS_MAX_RECIPIENT_DELIVERIES];
	int									NumDeliveries;
	int									NumRules;
	EWDB_AlarmsRuleStruct				Rule[EWDB_ALARMS_MAX_RULES_PER_RECIPIENT];
	int									RuleUseFlag[EWDB_ALARMS_MAX_RULES_PER_RECIPIENT];
} AlarmsRecipientStructRuleDelivery;


typedef struct _AlarmList
{
    EWDB_AlarmAuditStruct		    	Audit;
    EWDB_AlarmDeliveryUnionStruct       Delivery;
	char								AlarmMsg[ALARM_MSG_SIZE];
} AlarmList;


/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW5 API FORMATTED COMMENT
TYPE FUNCTION_PROTOTYPE

LIBRARY  EWDB_APPS_LIB

SUB_LIBRARY DB_LIB

LANGUAGE C

LOCATION THIS_FILE

STABILITY NEW

FUNCTION GetRecipientAlarmRules

SOURCE_LOCATION oracle/apps/src/libsrc/alarms_api_utils.c

RETURN_TYPE int

PARAMETER 1
PARAM_NAME idRecipient
PARAM_TYPE EWDBid
PARAM_DESCRIPTION Database ID of the recipient to inquire about; must be a valid ID.

PARAMETER 2
PARAM_NAME pRecipientRules
PARAM_TYPE AlarmsRecipientStructRuleDelivery *
PARAM_DESCRIPTION Pointer to a pre-allocated buffer that will be filled with the rule
					information for this recipient.

DESCRIPTION  This collects all the alarm rules for a single recipient,
			including multiple email, pager, phone, etc. deliveries.

RETURNS:	EW_FAILURE on failure
			EW_SUCCESS on success

*************************************************
************************************************/
int     GetRecipientAlarmRules (int idRecipient, 
				AlarmsRecipientStructRuleDelivery *pRecipientRules);

/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW5 API FORMATTED COMMENT
TYPE FUNCTION_PROTOTYPE

LIBRARY  EWDB_APPS_LIB

SUB_LIBRARY DB_LIB

LANGUAGE C

LOCATION THIS_FILE

STABILITY NEW

FUNCTION GetRecipientAlarmDelivery

SOURCE_LOCATION oracle/apps/src/libsrc/alarms_api_utils.c

RETURN_TYPE int

RETURNS:	EW_FAILURE on failure
			EW_SUCCESS on success

PARAMETER 1
PARAM_NAME idRecipient
PARAM_TYPE EWDBid
PARAM_DESCRIPTION Database ID of the recipient to inquire about; must be a valid ID.

PARAMETER 2
PARAM_NAME pStruct
PARAM_TYPE AlarmsRecipientDeliveryStruct *
PARAM_DESCRIPTION Pointer to a pre-allocated buffer that will be filled with the delivery
					information for this recipient.

DESCRIPTION  This collects all the alarm delivery information for a single recipient,
			including multiple email, pager, phone, etc. deliveries.

*************************************************
************************************************/
int     GetRecipientAlarmDelivery(EWDBid idRecipient, AlarmsRecipientDeliveryStruct *pStruct);

int		GetAlarmDelivery(int idRecipientDelivery, EWDB_AlarmsRecipientStructSummary *pSummary,
						 EWDB_AlarmDeliveryUnionStruct *pDelivery);

int     PrintRecipientAlarmRules (AlarmsRecipientStructRuleDelivery *pRecipientRules);

int     EWDB_InsertAudit (EWDB_AlarmAuditStruct *pAudit, 
								EWDB_AlarmDeliveryUnionStruct *pDelivery);

int     EWDB_GetAudits (int idEvent, EWDB_AlarmAuditStruct *pAudit,
                            int GetDeliveryInfo, EWDB_AlarmDeliveryUnionStruct *pDelivery,
                            int *NumFound, int *NumRetrieved, int BufferLen);

/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW5 API FORMATTED COMMENT
TYPE FUNCTION_PROTOTYPE

LIBRARY  EWDB_APPS_LIB

SUB_LIBRARY DB_LIB

LANGUAGE C

LOCATION THIS_FILE

STABILITY NEW

FUNCTION DeleteAlarmsRecipient

SOURCE_LOCATION oracle/apps/src/libsrc/alarms_api_utils.c

RETURN_TYPE int

PARAMETER 1
PARAM_NAME pRecipientRules
PARAM_TYPE AlarmsRecipientStructRuleDelivery *
PARAM_DESCRIPTION Pointer to a struct containing all the data associated with this recipient.
			This struct must be filled in prior to calling this function.

DESCRIPTION  This deletes an alarm recipient from the database, and all associated deliveries
			and rules.  Same effect as calling DeleteAlarmsRecipientAndRules, except you need
			to use the more-comprehensive AlarmsRecipientStructRuleDelivery structure.

RETURNS:	EW_FAILURE on failure
			EW_SUCCESS on success

*************************************************
************************************************/
int     DeleteAlarmsRecipient (AlarmsRecipientStructRuleDelivery *pRecipientRules);

/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW5 API FORMATTED COMMENT
TYPE FUNCTION_PROTOTYPE

LIBRARY  EWDB_APPS_LIB

SUB_LIBRARY DB_LIB

LANGUAGE C

LOCATION THIS_FILE

STABILITY NEW

FUNCTION DeleteAlarmsRecipientAndRules

SOURCE_LOCATION oracle/apps/src/libsrc/alarms_api_utils.c

RETURN_TYPE int

PARAMETER 1
PARAM_NAME pRecipient
PARAM_TYPE AlarmsRecipientDeliveryStruct *
PARAM_DESCRIPTION Pointer to a struct containing all the data associated with this recipient.
			This struct must be filled in prior to calling this function.

DESCRIPTION  This deletes an alarm recipient from the database, and all associated deliveries
			and rules.  Same effect as calling DeleteAlarmsRecipient, except you can use the
			less-comprehensive AlarmsRecipientDeliveryStruct structure.

RETURNS:	EW_FAILURE on failure
			EW_SUCCESS on success

*************************************************
************************************************/
int		DeleteAlarmsRecipientAndRules(AlarmsRecipientDeliveryStruct *pRecipient);

int     ExecuteAlarms (AlarmList *pAlarms, int NumAlarms,
            char *InvocationString, int RingKey, int InstId, int ModId);


/* In ewdb_apps_GetDBEventInfo.c */
int ewdb_apps_BuildStationArray (EWEventInfoStruct * pEventInfo, int CompareLocation, double LatSlack, double LonSlack, double ElevSlack, int CompDist);


/* in core_api_utils.c */
int ewdb_apps_InsertPeakAmpWithMag (EWDB_StationMagStruct *);


/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW5 API FORMATTED COMMENT
TYPE FUNCTION_PROTOTYPE

LIBRARY  EWDB_APPS_LIB

SUB_LIBRARY DB_LIB

LANGUAGE C

LOCATION THIS_FILE

STABILITY NEW

FUNCTION ewdb_apps_GetArrivalsFromDB

SOURCE_LOCATION oracle/apps/src/libsrc/ewdb_apps_GetArrivalsFromDB.c

RETURN_TYPE int

PARAMETER 1
PARAM_NAME ppAS
PARAM_TYPE EWDB_ArrivalStruct **
PARAM_DESCRIPTION Pointer to a buffer that the function will allocate and
fill with arrival information.  Be sure to FREE the Arrival buffer after
use.

PARAMETER 2
PARAM_NAME ppCS
PARAM_TYPE EWDB_ChannelStruct **
PARAM_DESCRIPTION Pointer to a buffer that the function will allocate and
fill with channel information that parallels the arrival info.  
Be sure to FREE the Channel buffer after use.

PARAMETER 3
PARAM_NAME idOrigin
PARAM_TYPE EWDBid
PARAM_DESCRIPTION This function retrieves a list of arrivals associated
with an origin.  idOrigin is the database ID of the origin for which
the caller wants a list of arrivals.

PARAMETER 4
PARAM_NAME pNumArrivals
PARAM_TYPE int *
PARAM_DESCRIPTION Pointer to an integer where the function will write
the number of arrivals (with channel info) retrieved.

DESCRIPTION The function retrieves a list of arrivals that are
associated with a given origin.  Along with each arrival struct the function
retrieves a parallel channel struct that contains channel information for
the given arrival.  Each Arrival.pStation is linked to each Channel.Comp
structure.  See EWDB_ArrivalStruct and EWDB_ChannelStruct for a
description of the information retrieved for each associated arrival/channel.
This function Allocates all memory needed to store the Arrival and Channel
information.  Do not pre-allocate buffers for the data.

NOTE:  This function ALLOCATES two buffers (upon successful completion).  These buffers
MUST BE FREED BY THE CALLER after the caller is done with them.

*************************************************
************************************************/
int ewdb_apps_GetArrivalsFromDB(EWDB_ArrivalStruct ** ppAS, 
                                EWDB_ChannelStruct ** ppCS, int idOrigin,
                                int * pNumArrivals);



/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW5 API FORMATTED COMMENT
TYPE FUNCTION_PROTOTYPE

LIBRARY  EWDB_APPS_LIB

SUB_LIBRARY DB_LIB

LANGUAGE C

LOCATION THIS_FILE

STABILITY NEW

FUNCTION ewdb_apps_GetLatestOriginForEventBySource

SOURCE_LOCATION oracle/apps/src/libsrc/ewdb_apps_GetLatestOriginForEventBySource.c

RETURN_TYPE int

RETURN_VALUE EWDB_RETURN_FAILURE
RETURN_DESCRIPTION  Fatal error.  See logfile for details.

RETURN_VALUE EWDB_RETURN_WARNING
RETURN_DESCRIPTION  Error.  No Origin found for the given Source for the 
given Event.

RETURN_VALUE EWDB_RETURN_SUCCESS
RETURN_DESCRIPTION Success

PARAMETER 1
PARAM_NAME szSource
PARAM_TYPE char *
PARAM_DESCRIPTION Name of the source for which the caller desires the
latest origin.  (This will be compared against the sSource from
the Source table.)  (The non human-readable author string.

PARAMETER 2
PARAM_NAME idEvent
PARAM_TYPE EWDBid
PARAM_DESCRIPTION The EWDB ID of the Event for which the caller
wishes to retrieve data.

PARAMETER 3
PARAM_NAME pLatestOrigin
PARAM_TYPE EWDB_OriginStruct *
PARAM_DESCRIPTION Pointer to an EWDB_OriginStruct where the function
will write the latest origin from the given source for the given event.

DESCRIPTION This is a convenient function for retrieving the latest
origin from a given source for a given Event from the DB.  
Retrieved Origin information is written to the pLatestOrigin pointer.

*************************************************
************************************************/
int ewdb_apps_GetLatestOriginForEventBySource(char * szSource, EWDBid idEvent,
                                              EWDB_OriginStruct * pLatestOrigin);


/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW5 API FORMATTED COMMENT
TYPE FUNCTION_PROTOTYPE

LIBRARY  EWDB_APPS_LIB

SUB_LIBRARY DB_LIB

LANGUAGE C

LOCATION THIS_FILE

STABILITY NEW

FUNCTION ewdb_apps_CreatePolygon

SOURCE_LOCATION oracle/apps/src/libsrc/ewdb_apps_CreatePolygon.c

RETURN_TYPE int

RETURN_VALUE EWDB_RETURN_FAILURE
RETURN_DESCRIPTION  Fatal error.  See logfile for details.

RETURN_VALUE EWDB_RETURN_SUCCESS
RETURN_DESCRIPTION Success

PARAMETER 1
PARAM_NAME pPolygon
PARAM_TYPE EWDB_PolygonStruct *
PARAM_DESCRIPTION  On entry, contains the name of the new polygon.  On exit,
also contains the DB ID for the new polygon (if successful).

PARAMETER 2
PARAM_NAME pVertices
PARAM_TYPE EWDB_PolygonVertexStruct *
PARAM_DESCRIPTION  Array of polygon vertices.  The latitude, longitude, and order
must be valid on entry; the polygon ID and vertex ID will be filled in on exit.

PARAMETER 3
PARAM_NAME numVertices
PARAM_TYPE int
PARAM_DESCRIPTION  number of vertices contained in pVertices parameter.

DESCRIPTION  Creates a new polygon in the DB.  This polygon can later be used
for alarm notification.

*************************************************
************************************************/
int ewdb_apps_CreatePolygon(EWDB_PolygonStruct *pPolygon, EWDB_PolygonVertexStruct *pVertices,
							int numVertices);


/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW5 API FORMATTED COMMENT
TYPE FUNCTION_PROTOTYPE

LIBRARY  EWDB_APPS_LIB

SUB_LIBRARY DB_LIB

LANGUAGE C

LOCATION THIS_FILE

STABILITY NEW

FUNCTION ewdb_apps_GetFullComment

SOURCE_LOCATION oracle/apps/src/libsrc/ewdb_apps_GetFullComment.c

RETURN_TYPE int

RETURN_VALUE EWDB_RETURN_FAILURE
RETURN_DESCRIPTION  Fatal error.  See logfile for details.

RETURN_VALUE EWDB_RETURN_WARNING
RETURN_DESCRIPTION  Comment found, but input buffer not large enough to contain
	the full comment.

RETURN_VALUE EWDB_RETURN_SUCCESS
RETURN_DESCRIPTION Success

PARAMETER 1
PARAM_NAME idComment
PARAM_TYPE EWDBid
PARAM_DESCRIPTION  DB ID of the comment to retrieve.

PARAMETER 2
PARAM_NAME szComment
PARAM_TYPE char *
PARAM_DESCRIPTION  On exit, contains the full comment from idComment.  If this buffer
	isn't large enough, it will contain the first n chars of the comment.

PARAMETER 3
PARAM_NAME iCommentSize
PARAM_TYPE int
PARAM_DESCRIPTION  On entry, contains the size of szComment.  On exit, contains the number
	of characters found in the full comment, including NULL terminator, if the original
	buffer was too small.  Use this value to resize your input buffer, ya stingy punk.

DESCRIPTION  Gets a complete comment from the DB.  If the comment spans multiple DB entries,
	this will get all the entries and assemble them into one comment.

*************************************************
************************************************/
int ewdb_apps_GetFullComment(EWDBid idComment, char *szComment, int *iCommentSize);


/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW5 API FORMATTED COMMENT
TYPE FUNCTION_PROTOTYPE

LIBRARY  EWDB_APPS_LIB

SUB_LIBRARY DB_LIB

LANGUAGE C

LOCATION THIS_FILE

STABILITY NEW

FUNCTION ewdb_apps_UpdateFullComment

SOURCE_LOCATION oracle/apps/src/libsrc/ewdb_apps_UpdateFullComment.c

RETURN_TYPE int

RETURN_VALUE EWDB_RETURN_FAILURE
RETURN_DESCRIPTION  Fatal error.  See logfile for details.

RETURN_VALUE EWDB_RETURN_SUCCESS
RETURN_DESCRIPTION Success

PARAMETER 1
PARAM_NAME idComment
PARAM_TYPE EWDBid *
PARAM_DESCRIPTION  On entry, DB ID of the comment to update.  This comment will be deleted
					from the DB.
					On exit, contains the new DB ID.

PARAMETER 2
PARAM_NAME szComment
PARAM_TYPE char *
PARAM_DESCRIPTION  Comment to update


DESCRIPTION  Updates a comment in the DB.  This deletes the complete comment (all snippets),
		then creates a new comment in the DB, spanning multiple entries as needed.
		idComment will contain the ID to the first new snippet.

*************************************************
************************************************/
int ewdb_apps_UpdateFullComment(EWDBid *idComment, char *szComment);

#endif
