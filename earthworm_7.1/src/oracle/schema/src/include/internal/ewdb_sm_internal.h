/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_sm_internal.h,v
 *
 *    Revision history:
 *     $Log: ewdb_sm_internal.h,v $
 *     Revision 1.1  2001/04/06 18:48:44  davidk
 *     Initial revision
 *
 *************************************************************/

/*************************************************************
   INTERNAL  - STRONG MOTION HEADER INFO  (NOT PART OF API)
 *************************************************************/

/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW API FORMATTED COMMENT
TYPE LIBRARY

LIBRARY  EWDB_API_LIB

SUB_LIBRARY STRONG_MOTION-INTERNAL

LOCATION THIS_FILE

DESCRIPTION The STRONG_MOTION_INTERNAL library contains the
code (both C & SQL) that supports the Strong Motion portion
of the EWDB_API_LIB.  This code is built to work with the
second coming of the EW Strong Motion schema (v2.12 and later),
and with rw_strongmotionII routines.

*************************************************
************************************************/

/******************************
 *  flags                     *
 ******************************/

/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW5 API FORMATTED COMMENT
TYPE DEFINE 

LIBRARY  EWDB_API_LIB

SUB_LIBRARY STRONG_MOTION-INTERNAL

LANGUAGE C

LOCATION THIS_FILE

CONSTANT_GROUP Strong Motion Message Query Types

CONSTANT EWDB_SM_SELECT_SMMESSAGES_W_INFO
VALUE 0
DESCRIPTION Identifies a query that retrieves Strong Motion
messages with message info only. (No channel or event information) 

CONSTANT EWDB_SM_SELECT_SMMESSAGES_W_ID_ONLY
VALUE 1
DESCRIPTION Identifies a query that retrieves only the IDs of
Strong Motion messages(idSMMessage). 

CONSTANT EWDB_SM_SELECT_SMMESSAGES_W_CHANNEL_INFO
VALUE 2
DESCRIPTION Identifies a query that retrieves Strong Motion
messages with channel and if available, event information.

CONSTANT EWDB_SM_SELECT_SMMESSAGES_W_CHANNEL_INFO_BY_EVENT
VALUE 3
DESCRIPTION Identifies a query that retrieves Strong Motion
messages with channel and event info, only for messages
that are associated with a given Event.

CONSTANT EWDB_SM_SELECT_SMMESSAGES_W_CHANNEL_INFO_WITHOUT_EVENT
VALUE 4
DESCRIPTION Identifies a query that retrieves Strong Motion
messages with channel info,  only for messages
that are not associated with any Event.

DO NOT CHANGE THE VALUES OF THE FOLLOWING CONSTANTS!!!!
They are tied to an array of strings in 
ewdb_internal_GetAllSMMessages.c!!!!!!!
*************************************************
************************************************/
/* query type constants */
#define EWDB_SM_SELECT_SMMESSAGES_W_INFO                  0
#define EWDB_SM_SELECT_SMMESSAGES_W_ID_ONLY               1
#define EWDB_SM_SELECT_SMMESSAGES_W_CHANNEL_INFO          2
#define EWDB_SM_SELECT_SMMESSAGES_W_CHANNEL_INFO_BY_EVENT 3
#define EWDB_SM_SELECT_SMMESSAGES_W_CHANNEL_INFO_WITHOUT_EVENT 4


/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW5 API FORMATTED COMMENT
TYPE DEFINE 

LIBRARY  EWDB_API_LIB

SUB_LIBRARY STRONG_MOTION-INTERNAL

LANGUAGE C

LOCATION THIS_FILE

CONSTANT_GROUP Strong Motion Motion Types

These constants are intended for use as motion
type codes in the SMMotion table(s).  They are used
by C functions that insert and retrieve SMMotion
data.

CONSTANT EWDB_SM_MOTION_TYPE_UNDEFINED
VALUE 0
DESCRIPTION Identifies an undefined type of motion.
This type of motion is generally not supported.

CONSTANT EWDB_SM_MOTION_TYPE_ACCELERATION
VALUE 1
DESCRIPTION Identifies a motion as acceleration.

CONSTANT EWDB_SM_MOTION_TYPE_VELOCITY
VALUE 2
DESCRIPTION Identifies a motion as velocity.

CONSTANT EWDB_SM_MOTION_TYPE_DISPLACEMENT
VALUE 3
DESCRIPTION Identifies a motion as diplacement.

*************************************************
************************************************/
/* Motion types */
#define EWDB_SM_MOTION_TYPE_UNDEFINED    0
#define EWDB_SM_MOTION_TYPE_ACCELERATION 1
#define EWDB_SM_MOTION_TYPE_VELOCITY     2
#define EWDB_SM_MOTION_TYPE_DISPLACEMENT 3


/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW5 API FORMATTED COMMENT
TYPE DEFINE 

LIBRARY  EWDB_API_LIB

SUB_LIBRARY STRONG_MOTION-INTERNAL

LANGUAGE C

LOCATION THIS_FILE

CONSTANT_GROUP Spectral Period Overloading Codes

These constants are intended for use as codes
in the SMMotion Period field to indicate that
the motion is not associated with a given period,
but is instead some special type of motion.  The
code indicates the type.

CONSTANT EWDB_SM_PERIOD_PEAK_VALUE_CODE
VALUE -1
DESCRIPTION Identifies the motion as a peak
type.  Such as Peak Ground Acceleration(PGA),
PGV, or PGD.

*************************************************
************************************************/
/* Spectral Period overloading codes */
#define EWDB_SM_PERIOD_PEAK_VALUE_CODE -1


/*****************************
 * structs                   *
 *****************************/

/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW5 API FORMATTED COMMENT
TYPE TYPEDEF 

LIBRARY  EWDB_API_LIB

SUB_LIBRARY STRONG_MOTION-INTERNAL

LANGUAGE C

LOCATION THIS_FILE

TYPEDEF EWDB_SMMotionStruct
TYPE_DEFINITION struct _EWDB_SMMotionStruct
DESCRIPTION This struct contains information for 
a single strong motion(SMMotion), in the database
coding format.  It is used to retrieve SMMotion
measurements from the DB.

MEMBER iMotionType
MEMBER_TYPE int
MEMBER_DESCRIPTION The type of motion.  Supported
values include (EWDB_SM_MOTION_TYPE_ACCELERATION | 
EWDB_SM_MOTION_TYPE_VELOCITY | 
EWDB_SM_MOTION_TYPE_DISPLACEMENT)

MEMBER dPeriod
MEMBER_TYPE double
MEMBER_DESCRIPTION The spectral response period
of the motion, or a coded identifier such as
EWDB_SM_PERIOD_PEAK_VALUE_CODE.

*************************************************
************************************************/
typedef struct _EWDB_SMMotionStruct
{
  int iMotionType;
  double dPeriod;
  double dMeasurement;
}EWDB_SMMotionStruct;


/****************************************************
 *  FUNCTIONS - CREATE, MODIFY, OR DELETE FUNCTIONS *
 ****************************************************/

/* Used to create a Strong Motion message in the DB */
/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW5 API FORMATTED COMMENT
TYPE FUNCTION_PROTOTYPE

LIBRARY  EWDB_API_LIB

SUB_LIBRARY STRONG_MOTION-INTERNAL

LANGUAGE C

LOCATION THIS_FILE

FUNCTION ewdb_internal_CreateSMMessage

SOURCE_LOCATION ewdb_internal_CreateSMMessage.c

RETURN_TYPE int 

RETURN_VALUE EWDB_RETURN_SUCCESS
RETURN_DESCRIPTION Success.

RETURN_VALUE EWDB_RETURN_FAILURE
RETURN_DESCRIPTION Failure.  See logfile for details.


PARAMETER 1
PARAM_NAME pidSMMessage
PARAM_TYPE EWDBid * 
PARAM_DESCRIPTION  A pointer to an EWDBid where the function will
write the idSMMessage of the newly created message.  The
idSMMessage will only be written if the function returns
EWDB_RETURN_SUCCESS.

PARAMETER 2
PARAM_NAME tMain
PARAM_TYPE double
PARAM_DESCRIPTION This is the main timestamp for the message.  This
is the one given by the program/device that recorded/declared
the motion.  (seconds since 1970)

PARAMETER 3
PARAM_NAME tLoad
PARAM_TYPE double 
PARAM_DESCRIPTION This is the time that the message was first loaded
into a Database.  (This is kind of a sanity timestamp).  This
timestamp should only be assigned once, when the message is
first loaded into a DB.  (seconds since 1970)

PARAMETER 4
PARAM_NAME tAlt
PARAM_TYPE double 
PARAM_DESCRIPTION This is an alternate time to tMain.  It can be assigned
by different entities.  iAltCode describes how the time was derived.
(seconds since 1970)

PARAMETER 5
PARAM_NAME iAltCode
PARAM_TYPE int 
PARAM_DESCRIPTION A code that describes how tAlt was derived. See 
SM_ALTCODE_NONE in rw_strongmotion.h for more information.

PARAMETER 6
PARAM_NAME tPGA
PARAM_TYPE double 
PARAM_DESCRIPTION (OPTIONAL) Time of the Peak Ground Acceleration(PGA) 
for this motion.  (seconds since 1970)

PARAMETER 7
PARAM_NAME tPGV
PARAM_TYPE double 
PARAM_DESCRIPTION (OPTIONAL) Time of the Peak Ground Velocity(PGV) 
for this motion.  (seconds since 1970)

PARAMETER 8
PARAM_NAME tPGD
PARAM_TYPE double 
PARAM_DESCRIPTION (OPTIONAL) Time of the Peak Ground Displacement(PGD) 
for this motion.  (seconds since 1970)

PARAMETER 9
PARAM_NAME idChan
PARAM_TYPE EWDBid 
PARAM_DESCRIPTION DB Identifier of the channel that recorded this strong 
motion message.

PARAMETER 10
PARAM_NAME idEvent
PARAM_TYPE EWDBid 
PARAM_DESCRIPTION DB Identifier of the event(that already exists 
in the DB)  that this strong motion message should be associated 
with.  The caller should set idEvent to 0 if they do not want 
this message associated with an Event.  

DESCRIPTION This function Creates an SMMessage record in the DB
and returns the idSMMessage of the new record.  It does not create
all the strong motion measurements that are part of the message.
The caller must call ewdb_internal_CreateSMMotion() for each motion
measurment in the message.  If idEvent is a positive number, then
the function will attempt to bind the new SMMessage to the Event
identified by idEvent.  

NOTE Be sure to properly initialize idEvent.  If a seemingly valid 
idEvent is passed, and no such Event exists, the function will fail, 
even though it can create the SMMessage itself without problems.
If you do not have an idEvent, but have an author and an author's
EventID, then you can use ewdb_api_CreateEvent() to get an idEvent
from an author and author's EventID.

*************************************************
************************************************/
int ewdb_internal_CreateSMMessage(EWDBid * pidSMMessage, double tMain, 
                             double tLoad, double tAlt, int iAltCode, 
                             double tPGA, double tPGV, double tPGD,
                             EWDBid idChan, EWDBid idEvent);

/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW5 API FORMATTED COMMENT
TYPE FUNCTION_PROTOTYPE

LIBRARY  EWDB_API_LIB

SUB_LIBRARY STRONG_MOTION-INTERNAL

LANGUAGE C

LOCATION THIS_FILE

FUNCTION ewdb_internal_CreateSMMotion

SOURCE_LOCATION ewdb_internal_CreateSMMotion.c

RETURN_TYPE int 

RETURN_VALUE EWDB_RETURN_SUCCESS
RETURN_DESCRIPTION Success.

RETURN_VALUE EWDB_RETURN_FAILURE
RETURN_DESCRIPTION Failure.  See logfile for details.

PARAMETER 1
PARAM_NAME pidSMMotion
PARAM_TYPE EWDBid * 
PARAM_DESCRIPTION A pointer to an EWDBid where the function will
write the idSMMotion of the newly created motion measurment.

PARAMETER 2
PARAM_NAME idSMMessage
PARAM_TYPE EWDBid 
PARAM_DESCRIPTION The DB id of the message that this motion
measurment is a part of.

PARAMETER 3
PARAM_NAME iMotionType
PARAM_TYPE int 
PARAM_DESCRIPTION The type of motion.  This must be either 
EWDB_SM_MOTION_TYPE_ACCELERATION, EWDB_SM_MOTION_TYPE_VELOCITY,
or EWDB_SM_MOTION_TYPE_DISPLACEMENT.

PARAMETER 4
PARAM_NAME dPeriod
PARAM_TYPE double 
PARAM_DESCRIPTION For spectral response measurements, this is the
time period associated with the motion.  For other types of 
measurments, this is a special code 
(see EWDB_SM_PERIOD_PEAK_VALUE_CODE) indicating a special 
measurement type.

PARAMETER 5
PARAM_NAME dMeasurement
PARAM_TYPE double 
PARAM_DESCRIPTION The measurement of the motion.  This value is 
in terms of centimeters and seconds, so should be either
cm/s/s for acceleration, cm/s for velocity, or cm for displacement.


DESCRIPTION This function Creates an SMMotion record in the DB
and returns the idSMMotion of the new record.  

*************************************************
************************************************/
int ewdb_internal_CreateSMMotion(EWDBid * pidSMMotion,
                                 EWDBid idSMMessage, int iMotionType,
                                 double dPeriod, double dMeasurement);


/****************************************************
 *  FUNCTIONS - READ ONLY                           *
 ****************************************************/

/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW5 API FORMATTED COMMENT
TYPE FUNCTION_PROTOTYPE

LIBRARY  EWDB_API_LIB

SUB_LIBRARY STRONG_MOTION-INTERNAL

LANGUAGE C

LOCATION THIS_FILE

FUNCTION ewdb_internal_GetAllSMMessages

SOURCE_LOCATION ewdb_internal_GetAllSMMessages.c

RETURN_TYPE int 

RETURN_VALUE EWDB_RETURN_SUCCESS
RETURN_DESCRIPTION Success.

RETURN_VALUE EWDB_RETURN_FAILURE
RETURN_DESCRIPTION Failure.  See logfile for details.

RETURN_VALUE EWDB_RETURN_WARNING
RETURN_DESCRIPTION Partial Success.  The function successfully
executed, but either 1)the caller's buffer wasn't large enough 
to retrieve all of the messages, and/or 2) there was a problem
retrieving one or more of the messages.  If the value written
to pNumRecordsFound at the completion of the function is greater
than the iBufferRecLen parameter passed to the function, then
more records were found than the caller's buffer could hold.
If the value written to pNumRecordsRetrieved is less than 
the iBufferRecLen parameter passed to the function, then the
function failed to retrieve atleast one of the messages that
it found.


PARAMETER 1
PARAM_NAME IN_szSta
PARAM_TYPE char * 
PARAM_DESCRIPTION The Station name criteria for retrieving 
messages.  This param is only used for criteria if 
IN_pCriteria->Criteria includes the EWDB_CRITERIA_USE_SCNL
flag.  '*' may be used as a wildcard.  This param must 
always be a valid pointer, the function may fail if it
is set to NULL.

PARAMETER 2
PARAM_NAME IN_szComp
PARAM_TYPE char * 
PARAM_DESCRIPTION The Component name criteria for retrieving 
messages.  This param is only used for criteria if 
IN_pCriteria->Criteria includes the EWDB_CRITERIA_USE_SCNL
flag.  '*' may be used as a wildcard.  This param must 
always be a valid pointer, the function may fail if it
is set to NULL.

PARAMETER 3
PARAM_NAME IN_szNet
PARAM_TYPE char * 
PARAM_DESCRIPTION The Network name criteria for retrieving 
messages.  This param is only used for criteria if 
IN_pCriteria->Criteria includes the EWDB_CRITERIA_USE_SCNL
flag.  '*' may be used as a wildcard.  This param must 
always be a valid pointer, the function may fail if it
is set to NULL.

PARAMETER 4
PARAM_NAME IN_szLoc
PARAM_TYPE char * 
PARAM_DESCRIPTION The Location name criteria for retrieving 
messages.  This param is only used for criteria if 
IN_pCriteria->Criteria includes the EWDB_CRITERIA_USE_SCNL
flag.  '*' may be used as a wildcard.  This param must 
always be a valid pointer, the function may fail if it
is set to NULL.

PARAMETER 5
PARAM_NAME IN_idEvent
PARAM_TYPE EWDBid 
PARAM_DESCRIPTION The Event criteria for retrieving 
messages.  This param is only used for criteria if 
IN_pCriteria->Criteria includes the EWDB_CRITERIA_USE_IDEVENT
flag.  This param must be set to a valid(existing) idEvent,
if IN_pCriteria->Criteria includes the 
EWDB_CRITERIA_USE_IDEVENT flag, or the function will fail.

PARAMETER 6
PARAM_NAME IN_iQueryType
PARAM_TYPE int 
PARAM_DESCRIPTION  The type of search query to use to retrieve
messages and related info.  See EWDB_SM_SELECT_SMMESSAGES_W_INFO
and related constants for more details.

PARAMETER 7
PARAM_NAME IN_pCriteria
PARAM_TYPE EWDB_CriteriaStruct * 
PARAM_DESCRIPTION  The criteria struct for specifying criteria
types and values for the query to retrieve messages.  
IN_pCriteria->Criteria, contains the flags which indicate
which criteria to use in searching for messages.  NOTE: The 
Depth criteria is not supported by this function!  See 
EWDB_CriteriaStruct for more detail on criteria.

PARAMETER 8
PARAM_NAME pBuffer
PARAM_TYPE void * 
PARAM_DESCRIPTION  A buffer allocated by the caller, where the
function will place the retrieved messages.  See the description
of iBufferRecLen immediately below, for more information on 
buffer records.

PARAMETER 9
PARAM_NAME iBufferRecLen
PARAM_TYPE int 
PARAM_DESCRIPTION  The length of the buffer(pBuffer) in terms of
records.  This function supports multiple record types for the 
retrieval buffer, dependent upon the iQueryType parameter.
If iQueryType is (EWDB_SM_SELECT_SMMESSAGES_W_INFO, 
EWDB_SM_SELECT_SMMESSAGES_W_CHANNEL_INFO, 
EWDB_SM_SELECT_SMMESSAGES_W_CHANNEL_INFO_BY_EVENT,
or EWDB_SM_SELECT_SMMESSAGES_W_CHANNEL_INFO_WITHOUT_EVENT) then
the buffer records are expected to be of type EWDB_SMChanAllStruct.
If iQueryType is EWDB_SM_SELECT_SMMESSAGES_W_ID_ONLY, then the 
buffer records are expected to be of type EWDBid.

PARAMETER 10
PARAM_NAME pNumRecordsFound
PARAM_TYPE int * 
PARAM_DESCRIPTION  A pointer to an int where the function will
write the number of messages found to meet the given criteria.
NOTE:  The number found is not the number retrieved and written
to the caller's buffer.  That is NumRecordsRetrieved.  If the
number of messages found is greater than the caller's buffer will
hold, then the function will return a warning.

PARAMETER 11
PARAM_NAME pNumRecordsRetrieved
PARAM_TYPE int * 
PARAM_DESCRIPTION  A pointer to an int where the function will
write the number of messages actualy retrieved by the function.
This number will be less than or equal to the number found.

DESCRIPTION This function is the big, bad, nasty, smelly, 4000Lb
gorilla of strong motion data retrieval.  It is a multiplexed
work of some nefarious creature.  Based on the parameters passed
in, it executes one of mutiple(Currently 5) genres of queries to
retrieve strong motion messages, and then retrieves the data into
one of multiple(currently 2) buffer types.  This function exists
because it was deemed better to write one large hairy function
than to write ten simpler functions that had lots of redundant
code between them.  The caller passes in a set of criteria and
a type of query to execute, and then the function executes the
query and retrieves the data into the caller's buffer, in a
record format determined by the query type.

NOTE This function retrieves only the messages and (optionally)
non-motion associated info, such as Channel and Event association.
It does not retrieve the actual motion measurements.  This must
be done by calling ewdb_internal_GetSMMotionsForMessage() after 
calling this function.

*************************************************
************************************************/
int ewdb_internal_GetAllSMMessages(char * IN_szSta, char * IN_szComp,
                                   char * IN_szNet, char * IN_szLoc,
                                   EWDBid IN_idEvent, int IN_iQueryType,
                                   EWDB_CriteriaStruct * IN_pCriteria,
                                   void * pBuffer, int iBufferRecLen,
                                   int * pNumRecordsFound, 
                                   int * pNumRecordsRetrieved);

/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW5 API FORMATTED COMMENT
TYPE FUNCTION_PROTOTYPE

LIBRARY  EWDB_API_LIB

SUB_LIBRARY STRONG_MOTION-INTERNAL

LANGUAGE C

LOCATION THIS_FILE

FUNCTION ewdb_internal_GetSMMotionsForMessage

SOURCE_LOCATION ewdb_internal_GetSMMotionsForMessage.c

RETURN_TYPE int 

RETURN_VALUE EWDB_RETURN_SUCCESS
RETURN_DESCRIPTION Success.

RETURN_VALUE EWDB_RETURN_FAILURE
RETURN_DESCRIPTION Failure.  See logfile for details.

RETURN_VALUE EWDB_RETURN_WARNING
RETURN_DESCRIPTION Partial Success.  The function successfully
executed, but the caller's buffer wasn't large enough 
to retrieve all of the messages.  See *pNumRecordsFound for
the neccessary buffersize to retrieve all selected motion
measurments.

PARAMETER 1
PARAM_NAME idSMMessage
PARAM_TYPE EWDBid 
PARAM_DESCRIPTION  The DB ID of the strong motion message for
which the caller wants to retrieve motion measurments.

PARAMETER 2
PARAM_NAME pBuffer
PARAM_TYPE EWDB_SMMotionStruct * 
PARAM_DESCRIPTION  Buffer allocated by the caller, where the
function will write the retrieved motion measurements for the
given SMMessage.

PARAMETER 3
PARAM_NAME iBufferRecLen
PARAM_TYPE int 
PARAM_DESCRIPTION  The length of the caller's buffer(pBuffer) in
terms of EWDB_SMMotionStruct records.

PARAMETER 4
PARAM_NAME pNumRecordsFound
PARAM_TYPE int * 
PARAM_DESCRIPTION  A pointer to an int where the function will
write the number of motions found.
NOTE:  The number found is not the number retrieved and written
to the caller's buffer.  That is NumRecordsRetrieved.  If the
number of messages found is greater than the caller's buffer will
hold, then the function will return a warning.

PARAMETER 5
PARAM_NAME pNumRecordsRetrieved
PARAM_TYPE int * 
PARAM_DESCRIPTION  A pointer to an int where the function will
write the number of motion measurements actualy retrieved by 
the function.  This will be less than or equal to the number found.

DESCRIPTION This function retrieves all strong motion measurements
for a given strong motion message(SMMessage).  It writes the results
as an array of EWDB_SMMotionStruct.

*************************************************
************************************************/
int ewdb_internal_GetSMMotionsForMessage(EWDBid idSMMessage, 
                                         EWDB_SMMotionStruct * pBuffer,
                                         int iBufferRecLen, 
                                         int * pNumRecordsFound,
                                         int * pNumRecordsRetrieved);


/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW5 API FORMATTED COMMENT
TYPE FUNCTION_PROTOTYPE

LIBRARY  EWDB_API_LIB

SUB_LIBRARY STRONG_MOTION-INTERNAL

LANGUAGE C

LOCATION THIS_FILE

FUNCTION ewdb_internal_GetSMMessages

SOURCE_LOCATION ewdb_internal_GetSMMessages.c

RETURN_TYPE int 

RETURN_VALUE EWDB_RETURN_SUCCESS
RETURN_DESCRIPTION Success.

RETURN_VALUE EWDB_RETURN_FAILURE
RETURN_DESCRIPTION Failure.  See logfile for details.

RETURN_VALUE EWDB_RETURN_WARNING
RETURN_DESCRIPTION Partial Success.  The function successfully
executed, but either 1)the caller's buffer wasn't large enough 
to retrieve all of the messages, and/or 2) there was a problem
retrieving one or more of the messages.  If the value written
to pNumRecordsFound at the completion of the function is greater
than the iBufferRecLen parameter passed to the function, then
more records were found than the caller's buffer could hold.
If the value written to pNumRecordsRetrieved is less than 
the iBufferRecLen parameter passed to the function, then the
function failed to retrieve atleast one of the messages that
it found.


PARAMETER 1
PARAM_NAME pcsCriteria
PARAM_TYPE EWDB_CriteriaStruct * 
PARAM_DESCRIPTION  The criteria struct for specifying criteria
types and values for the query to retrieve messages.  
pcsCriteria->Criteria, contains the flags which indicate
which criteria to use in searching for messages.  NOTE: The 
Depth criteria is not supported by this function!  See 
EWDB_CriteriaStruct for more detail on criteria.

PARAMETER 2
PARAM_NAME szSta
PARAM_TYPE char * 
PARAM_DESCRIPTION The Station name criteria for retrieving 
messages.  This param is only used for criteria if 
pcsCriteria->Criteria includes the EWDB_CRITERIA_USE_SCNL
flag.  '*' may be used as a wildcard.  This param must 
always be a valid pointer, the function may fail if it
is set to NULL.

PARAMETER 3
PARAM_NAME szComp
PARAM_TYPE char * 
PARAM_DESCRIPTION The Component name criteria for retrieving 
messages.  This param is only used for criteria if 
pcsCriteria->Criteria includes the EWDB_CRITERIA_USE_SCNL
flag.  '*' may be used as a wildcard.  This param must 
always be a valid pointer, the function may fail if it
is set to NULL.

PARAMETER 4
PARAM_NAME szNet
PARAM_TYPE char * 
PARAM_DESCRIPTION The Network name criteria for retrieving 
messages.  This param is only used for criteria if 
pcsCriteria->Criteria includes the EWDB_CRITERIA_USE_SCNL
flag.  '*' may be used as a wildcard.  This param must 
always be a valid pointer, the function may fail if it
is set to NULL.

PARAMETER 5
PARAM_NAME szLoc
PARAM_TYPE char * 
PARAM_DESCRIPTION The Location name criteria for retrieving 
messages.  This param is only used for criteria if 
pcsCriteria->Criteria includes the EWDB_CRITERIA_USE_SCNL
flag.  '*' may be used as a wildcard.  This param must 
always be a valid pointer, the function may fail if it
is set to NULL.

PARAMETER 6
PARAM_NAME idEvent
PARAM_TYPE EWDBid 
PARAM_DESCRIPTION The Event criteria for retrieving 
messages.  This param is only used for criteria if 
pcsCriteria->Criteria includes the EWDB_CRITERIA_USE_IDEVENT
flag.  This param must be set to a valid(existing) idEvent,
if pcsCriteria->Criteria includes the 
EWDB_CRITERIA_USE_IDEVENT flag, or the function will fail.

PARAMETER 7
PARAM_NAME iQueryType
PARAM_TYPE int 
PARAM_DESCRIPTION  The type of search query to use to retrieve
messages and related info.  See EWDB_SM_SELECT_SMMESSAGES_W_INFO
and related constants for more details.

PARAMETER 8
PARAM_NAME szStructType
PARAM_TYPE char * 
PARAM_DESCRIPTION  A string representation of the structure type
of the caller's buffer.  (The format the caller is expecting to
retrieve data in.)  Currently only "SM_INFO" and 
"EWDB_SMChanAllStruct" are supported, any other structure type 
will cause the function to fail.  

PARAMETER 9
PARAM_NAME pBuffer
PARAM_TYPE void * 
PARAM_DESCRIPTION  A buffer allocated by the caller, where the
function will place the retrieved messages.  See the description
of iBufferRecLen immediately below, for more information on 
buffer records.

PARAMETER 10
PARAM_NAME iBufferRecLen
PARAM_TYPE int 
PARAM_DESCRIPTION  The length of the buffer(pBuffer) in terms of
records.  The record type is given by the szStructType
param.  See szStructType(above) for a description of what
structure types are supported.

PARAMETER 11
PARAM_NAME pNumRecordsFound
PARAM_TYPE int * 
PARAM_DESCRIPTION  A pointer to an int where the function will
write the number of messages found to meet the given criteria.
NOTE:  The number found is not the number retrieved and written
to the caller's buffer.  That is NumRecordsRetrieved.  If the
number of messages found is greater than the caller's buffer will
hold, then the function will return a warning.

PARAMETER 12
PARAM_NAME pNumRecordsRetrieved
PARAM_TYPE int * 
PARAM_DESCRIPTION  A pointer to an int where the function will
write the number of messages actualy retrieved by the function.
This number will be less than or equal to the number found.

DESCRIPTION This function retrieves strong motion messages and
associated info for all messages that meet the criteria given
by the caller.  This function retrieves the strong motion 
messages(SMMessages) complete with all of the motion measurements
for each message, and Channel and Event association information
for each message if the caller's buffer will accomodate it.

*************************************************
************************************************/
int ewdb_internal_GetSMMessages(EWDB_CriteriaStruct * pcsCriteria,
                                char * szSta, char * szComp,
                                char * szNet, char * szLoc,
                                EWDBid idEvent, int iQueryType, char * szStructType,
                                void * pBuffer, int BufferRecLen,
                                int * pNumRecordsFound, int * pNumRecordsRetrieved);


/*************************************************************
   END INTERNAL  - STRONG MOTION HEADER  (NOT PART OF API)
 *************************************************************/
