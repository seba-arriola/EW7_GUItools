/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_internal_functions.h,v 1.14 2005/06/16 16:48:15 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_internal_functions.h,v $
 *     Revision 1.14  2005/06/16 16:48:15  davidk
 *     Parametric DB API Cleanup - Added previously unprototyped internal functions.
 *
 *     Revision 1.13  2005/05/12 20:25:42  mark
 *     Added comments functions
 *
 *     Revision 1.12  2005/03/24 18:21:18  davidk
 *     Added prototypes for ewdb_internal_TS convert functions.
 *
 *     Revision 1.11  2004/03/17 20:38:34  davidk
 *     Fixed some errors in special formatted comments.
 *
 *     Revision 1.10  2003/12/04 19:37:34  davidk
 *     Added comments.
 *     Modified the parameters for ewdb_internal_SetChanParams(), which now uses
 *     a pointer to a EWDB_ChannelStruct instead of an extensive list of separate
 *     parameters.
 *
 *     Revision 1.9  2003/09/04 20:37:11  lucky
 *     Replaced AMPLITUDE_TYPE with MAGNITUDE_TYPE
 *
 *     Revision 1.8  2003/03/05 17:49:37  davidk
 *     Added functions for retrieving Peak Amplitude data.
 *
 *     Revision 1.7  2002/05/16 17:01:13  davidk
 *     Added prototypes for concierge Mark2 internal functions.
 *
 *     Revision 1.6  2002/05/16 16:57:20  davidk
 *     Added waveform internal function prototypes and documentaiton.
 *
 *     Revision 1.5  2001/08/02 18:45:33  davidk
 *     Improved comment documentation.
 *
 *     Revision 1.4  2001/07/01 21:55:39  davidk
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
 *
 ***************************************************************/

/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW API FORMATTED COMMENT
TYPE LIBRARY

LIBRARY  EWDB_API_LIB

SUB_LIBRARY PARAMETRIC-INTERNAL

LOCATION THIS_FILE

DESCRIPTION This is a portion of the EWDB_API_LIB
that contains C and SQL code that supports the 
PARAMETRIC_API.  This library includes internal
code, NOT code that should be used by programmers
using the API to access the EW DB.

*************************************************
************************************************/


/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW5 API FORMATTED COMMENT
TYPE FUNCTION_PROTOTYPE

LIBRARY  EWDB_API_LIB

SUB_LIBRARY PARAMETRIC-INTERNAL

LOCATION THIS_FILE

LANGUAGE C

STABILITY NEW

FUNCTION ewdb_internal_CreatePeakAmp 


SOURCE_LOCATION src/oracle/schema-working/src/parametric/ewdb_internal_CreatePeakAmp.c

RETURN_TYPE int

RETURN_VALUE EWDB_RETURN_FAILURE
RETURN_DESCRIPTION  Fatal error.  See logfile for details.

RETURN_VALUE EWDB_RETURN_SUCCESS
RETURN_DESCRIPTION Success

PARAMETER 1
PARAM_NAME pPeakAmp
PARAM_TYPE EWDB_StationMagStruct *
PARAM_DESCRIPTION Pointer to the Station Magnitude structure 
filled with Peak Amplitude information to be inserted into the DB.

DESCRIPTION Inserts the following information from a StationMag
structure into the Database: the Channel from which the amplitude
was recorded(idChan), the first amplitude measurement(dAmp1), 
the time of the first amplitude measurement(tAmp1), the wave period
of the first amplitude measurement (tPeriod1),  the second 
amplitude measurement(dAmp2), the time of the second amplitude 
measurement(tAmp2), the wave period of the second amplitude 
measurement(tPeriod2), and the Magnitude type associated with the
amplitude.  All other fields from the EWDB_StationMagnitudeStruct
are ignored. <br>
Upon Successfull completion, the DB ID of the new Peak Amplitude
record is written back to pPeakAmp->idDatum and 
pPeakAmp->StaMagUnion.PeakAmp.idPeakAmp.

*************************************************
************************************************/
int ewdb_internal_CreatePeakAmp(EWDB_StationMagStruct *pPeakAmp); 


/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW5 API FORMATTED COMMENT
TYPE FUNCTION_PROTOTYPE

LIBRARY  EWDB_API_LIB

SUB_LIBRARY PARAMETRIC-INTERNAL

LOCATION THIS_FILE

LANGUAGE C

STABILITY NEW

FUNCTION ewdb_internal_CreateCodaDur 


SOURCE_LOCATION src/oracle/schema-working/src/parametric/ewdb_internal_CreateCodaDur.c

RETURN_TYPE int

RETURN_VALUE EWDB_RETURN_FAILURE
RETURN_DESCRIPTION Fatal error.  See logfile for details.

RETURN_VALUE EWDB_RETURN_SUCCESS
RETURN_DESCRIPTION Success.

PARAMETER 1
PARAM_NAME pCodaDur
PARAM_TYPE EWDB_CodaDurationStruct *
PARAM_DESCRIPTION Pointer to an EWDB_CodaDurationStruct filled by the
caller.

DESCRIPTION Function creates a coda duration measurement (the basis for
a station duration magnitude) by linking a pair of caller supplied 
P-Pick(idPick) and coda termination(idTCoda) records.  The function 
ignores all other members of the EWDB_CodaDurationStruct.  Upon 
successfull completion, the function writes the DB ID of the newly
created coda duration back to pCodaDur->idCodaDur.

*************************************************
************************************************/
int ewdb_internal_CreateCodaDur(EWDB_CodaDurationStruct *pCodaDur); 


/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW5 API FORMATTED COMMENT
TYPE FUNCTION_PROTOTYPE

LIBRARY  EWDB_API_LIB

SUB_LIBRARY PARAMETRIC-INTERNAL

LOCATION THIS_FILE

LANGUAGE C

STABILITY NEW

FUNCTION ewdb_internal_CreateStaMag 


SOURCE_LOCATION src/oracle/schema-working/src/parametric/ewdb_internal_CreateStaMag.c

RETURN_TYPE int

RETURN_VALUE EWDB_RETURN_FAILURE
RETURN_DESCRIPTION  Fatal error.  See logfile for details.

RETURN_VALUE EWDB_RETURN_SUCCESS
RETURN_DESCRIPTION Success

PARAMETER 1
PARAM_NAME pStaMag
PARAM_TYPE EWDB_StationMagStruct *
PARAM_DESCRIPTION Station Magnitude structure filled by the caller

DESCRIPTION Links a Summary Magnitude with a supporting amplitude
from a station, based on the information in the EWDB_StationMagStruct
passed by the caller.  The magnitude types of the summary 
magnitude(idMagnitude) and the amplitude/duration measurement(idDatum)
must match, or the function will fail. <br>
The function ignores the contents of the StaMagUnion in the
EWDB_StationMagStruct.

*************************************************
************************************************/
int ewdb_internal_CreateStaMag(EWDB_StationMagStruct *pStaMag); 


/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW5 API FORMATTED COMMENT
TYPE FUNCTION_PROTOTYPE

LIBRARY  EWDB_API_LIB

SUB_LIBRARY PARAMETRIC-INTERNAL

LOCATION THIS_FILE

LANGUAGE C

STABILITY NEW

FUNCTION ewdb_internal_CreateTCoda 


SOURCE_LOCATION src/oracle/schema-working/src/parametric/ewdb_internal_CreateTCoda.c

RETURN_TYPE int

RETURN_VALUE EWDB_RETURN_FAILURE
RETURN_DESCRIPTION  Fatal error.  See logfile for details.

RETURN_VALUE EWDB_RETURN_SUCCESS
RETURN_DESCRIPTION Success

PARAMETER 1
PARAM_NAME pTCoda
PARAM_TYPE EWDB_CodaDurationStruct *
PARAM_DESCRIPTION CodaDuration structure filled by the caller.

DESCRIPTION Creates a coda termination record based upon the
EWDB_CodaDurationStruct passed by the caller.  The function
ignores all parameters of the CodaDuration structure except 
the data channel(idChan), the coda termination 
time (tCodaTermXtp), and the observed coda termination 
time (tCodaTermObs) if applicable .  It does not look at the 
average amplitude values(pCodaAmp), so that pointer may 
be left as NULL!

*************************************************
************************************************/
int ewdb_internal_CreateTCoda(EWDB_CodaDurationStruct *pTCoda); 


/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW5 API FORMATTED COMMENT
TYPE FUNCTION_PROTOTYPE

LIBRARY  EWDB_API_LIB

SUB_LIBRARY PARAMETRIC-INTERNAL

LOCATION THIS_FILE

LANGUAGE C

STABILITY NEW

FUNCTION ewdb_internal_GetStaMagsForCodaDur 

SOURCE_LOCATION src/oracle/schema-working/src/parametric/ewdb_internal_GetStaMagsForCodaDur.c

RETURN_TYPE int

RETURN_VALUE EWDB_RETURN_FAILURE
RETURN_DESCRIPTION  Fatal error.  See logfile for details.

RETURN_VALUE EWDB_RETURN_SUCCESS
RETURN_DESCRIPTION Success

RETURN_VALUE EWDB_RETURN_WARNING
RETURN_DESCRIPTION Partial Success.  The call executed successfully,
but the caller's buffer was not large enough to accommodate all of 
the StaMags found.

PARAMETER 1
PARAM_NAME idMagnitude
PARAM_TYPE EWDBid
PARAM_DESCRIPTION DB ID of the summary magnitude for which the caller
wants to retrieve associated station magnitudes.  (The caller wants
to retrieve station magnitudes for this summary magnitude.)

PARAMETER 2
PARAM_NAME pStaMags
PARAM_TYPE EWDB_StationMagStruct *
PARAM_DESCRIPTION 

PARAMETER 3
PARAM_NAME pNumStaMagsFound
PARAM_TYPE int *
PARAM_DESCRIPTION Pointer to an integer where the function will write
the number of station magnitudes found.

PARAMETER 4
PARAM_NAME pNumStaMagsRetrieved
PARAM_TYPE int *
PARAM_DESCRIPTION Pointer to an integer where the function will write
the number of station magnitudes placed in the callers buffer(pStaMags).

PARAMETER 5
PARAM_NAME BufferLen
PARAM_TYPE int
PARAM_DESCRIPTION Size of the pStaMags buffer as a multiple of
EWDB_StationMagStruct. (example: 15 structs)

DESCRIPTION The function retrieves a list of "Coda Duration" based
station magnitudes that support a single summary duration magnitude(Md)
for an Event.  See EWDB_StationMagStruct for a
description of the information retrieved for each associated stamag.
Because this function deals with only duration measurements, it
will never fill in the StaMagUnion.PeakAmp information

*************************************************
************************************************/
int ewdb_internal_GetStaMagsForCodaDur(EWDBid idMagnitude, 
                                       EWDB_StationMagStruct * pStaMags,
                                       int * pNumStaMagsFound, 
                                       int * pNumStaMagsRetrieved,
                                       int BufferLen);



/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW5 API FORMATTED COMMENT
TYPE FUNCTION_PROTOTYPE

LIBRARY  EWDB_API_LIB

SUB_LIBRARY PARAMETRIC-INTERNAL

LOCATION THIS_FILE

LANGUAGE C

STABILITY NEW

FUNCTION ewdb_internal_GetStaMagsForPeakAmp 


SOURCE_LOCATION src/oracle/schema-working/src/parametric/ewdb_internal_GetStaMagsForPeakAmp.c

RETURN_TYPE int

RETURN_VALUE EWDB_RETURN_FAILURE
RETURN_DESCRIPTION  Fatal error.  See logfile for details.

RETURN_VALUE EWDB_RETURN_SUCCESS
RETURN_DESCRIPTION Success

RETURN_VALUE EWDB_RETURN_WARNING
RETURN_DESCRIPTION Partial Success.  The call executed successfully,
but the caller's buffer was not large enough to accommodate all of 
the StaMags found.

PARAMETER 1
PARAM_NAME idMagnitude
PARAM_TYPE EWDBid
PARAM_DESCRIPTION DB ID of the summary magnitude for which the caller
wants to retrieve associated station magnitudes.  (The caller wants
to retrieve station magnitudes for this summary magnitude.)

PARAMETER 2
PARAM_NAME pStaMags
PARAM_TYPE EWDB_StationMagStruct *
PARAM_DESCRIPTION 

PARAMETER 3
PARAM_NAME pNumStaMagsFound
PARAM_TYPE int *
PARAM_DESCRIPTION Pointer to an integer where the function will write
the number of station magnitudes found.

PARAMETER 4
PARAM_NAME pNumStaMagsRetrieved
PARAM_TYPE int *
PARAM_DESCRIPTION Pointer to an integer where the function will write
the number of station magnitudes placed in the callers buffer(pStaMags).

PARAMETER 5
PARAM_NAME BufferLen
PARAM_TYPE int
PARAM_DESCRIPTION Size of the pStaMags buffer as a multiple of
EWDB_StationMagStruct. (example: 15 structs)

DESCRIPTION The function retrieves a list of "Peak Amplitude" 
based station magnitudes that support a single summary amplitude 
magnitude (ML, Mb, etc.) for an Event.  See EWDB_StationMagStruct for a
description of the information retrieved for each associated stamag.
Because this function deals with only amplitude based measurements, it
will never fill in the StaMagUnion.CodaDur information

*************************************************
************************************************/
int ewdb_internal_GetStaMagsForPeakAmp(EWDBid idMagnitude, 
                                       EWDB_StationMagStruct * pStaMags,
                                       int * pNumStaMagsFound, 
                                       int * pNumStaMagsRetrieved,
                                       int BufferLen);



/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW API FORMATTED COMMENT
TYPE LIBRARY

LIBRARY  EWDB_API_LIB

SUB_LIBRARY WAVEFORM-INTERNAL

LOCATION THIS_FILE

DESCRIPTION This is a portion of the EWDB_API_LIB
that contains C and SQL code that supports the 
WAVEFORM_API.  This library includes internal
code, not code that should be used by programmers
using the API to access the EW DB.

*************************************************
************************************************/

/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW5 API FORMATTED COMMENT
TYPE FUNCTION_PROTOTYPE

LIBRARY  EWDB_API_LIB

SUB_LIBRARY WAVEFORM-INTERNAL

LANGUAGE C

LOCATION THIS_FILE

STABILITY MODIFIED

FUNCTION ewdb_internal_CreateSnippet


SOURCE_LOCATION src/oracle/schema-working/src/waveform/ewdb_internal_CreateSnippet.c

RETURN_TYPE int

RETURN_VALUE EWDB_RETURN_FAILURE
RETURN_DESCRIPTION  Fatal error.  See logfile for details.

RETURN_VALUE EWDB_RETURN_SUCCESS
RETURN_DESCRIPTION Success

PARAMETER 1
PARAM_NAME IN_idWaveform
PARAM_TYPE EWDBid
PARAM_DESCRIPTION Database ID of the Waveform associated with this snippet.

PARAMETER 2
PARAM_NAME IN_pSnippet
PARAM_TYPE char *
PARAM_DESCRIPTION Pointer to the binary waveform snippet to be inserted.

PARAMETER 3
PARAM_NAME IN_SnippetSize
PARAM_TYPE int
PARAM_DESCRIPTION Length of the snippet.

DESCRIPTION Inserts the binary waveform snippet into the database.  A waveform
descriptor must already exist in the DB for the given waveform.  The idWaveform
is the DB ID of the existin waveform descriptor.

*************************************************
************************************************/
int ewdb_internal_CreateSnippet(EWDBid IN_idWaveform, char * IN_pSnippet,
                                int IN_SnippetSize);



/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW5 API FORMATTED COMMENT
TYPE FUNCTION_PROTOTYPE

LIBRARY  EWDB_API_LIB

SUB_LIBRARY WAVEFORM-INTERNAL

LANGUAGE C

LOCATION THIS_FILE

STABILITY MODIFIED

FUNCTION ewdb_internal_CreateWaveformDesc


SOURCE_LOCATION src/oracle/schema-working/src/waveform/ewdb_internal_CreateWaveformDesc.c

RETURN_TYPE int

RETURN_VALUE EWDB_RETURN_FAILURE
RETURN_DESCRIPTION  Fatal error.  See logfile for details.

RETURN_VALUE EWDB_RETURN_SUCCESS
RETURN_DESCRIPTION Success

PARAMETER 1
PARAM_NAME pWaveform
PARAM_TYPE EWDB_WaveformStruct *
PARAM_DESCRIPTION Input waveform structure filled by the caller.

PARAMETER 2
PARAM_NAME bBindToEvent
PARAM_TYPE int
PARAM_DESCRIPTION Set to 1 if this waveform record should be 
bound to the event specified by idEvent.

PARAMETER 3
PARAM_NAME idEvent
PARAM_TYPE EWDBid 
PARAM_DESCRIPTION Database ID of the Event with which this 
Waveform Descriptor is associated.

DESCRIPTION Insert a waveform descriptor record into the database and
associate it with an Event(if bBindToEvent is set to TRUE).

*************************************************
************************************************/
int ewdb_internal_CreateWaveformDesc(EWDB_WaveformStruct *pWaveform,
                                     int bBindToEvent, EWDBid idEvent);



/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW5 API FORMATTED COMMENT
TYPE FUNCTION_PROTOTYPE

LIBRARY  EWDB_API_LIB

SUB_LIBRARY WAVEFORM-INTERNAL

LANGUAGE C

LOCATION THIS_FILE

STABILITY NEW

FUNCTION ewdb_internal_UpdateSnippet


SOURCE_LOCATION src/oracle/schema-working/src/waveform/ewdb_internal_UpdateSnippet.c

RETURN_TYPE int

RETURN_VALUE EWDB_RETURN_FAILURE
RETURN_DESCRIPTION  Fatal error.  See logfile for details.

RETURN_VALUE EWDB_RETURN_SUCCESS
RETURN_DESCRIPTION Success

PARAMETER 1
PARAM_NAME IN_idWaveform
PARAM_TYPE EWDBid
PARAM_DESCRIPTION Database ID of the Waveform associated with this snippet.

PARAMETER 2
PARAM_NAME IN_pSnippet
PARAM_TYPE char *
PARAM_DESCRIPTION Pointer to the binary waveform snippet to be inserted.

PARAMETER 3
PARAM_NAME IN_iSnippetSize
PARAM_TYPE int
PARAM_DESCRIPTION Length of the snippet.

DESCRIPTION Updates the binary waveform snippet in the database, replacing the old
binary snippet with the new one.
A snippet and a waveform descriptor must already exist in the DB 
for the given idWaveform.  The idWaveform is the 
DB ID of the existing waveform descriptor and snippet.

*************************************************
************************************************/
int ewdb_internal_UpdateSnippet(EWDBid IN_idWaveform, char * IN_pSnippet,
                                int IN_SnippetSize);



/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW5 API FORMATTED COMMENT
TYPE FUNCTION_PROTOTYPE

LIBRARY  EWDB_API_LIB

SUB_LIBRARY WAVEFORM-INTERNAL

LANGUAGE C

LOCATION THIS_FILE

STABILITY NEW

FUNCTION ewdb_internal_UpdateWaveformDesc


SOURCE_LOCATION src/oracle/schema-working/src/waveform/ewdb_internal_UpdateWaveformDesc.c

RETURN_TYPE int

RETURN_VALUE EWDB_RETURN_FAILURE
RETURN_DESCRIPTION  Fatal error.  See logfile for details.

RETURN_VALUE EWDB_RETURN_SUCCESS
RETURN_DESCRIPTION Success

PARAMETER 1
PARAM_NAME pWaveform
PARAM_TYPE EWDB_WaveformStruct *
PARAM_DESCRIPTION Input waveform structure filled by the caller.

DESCRIPTION Update an existing waveform descriptor record in 
the database.  The record maintains any existing associations it
has.  pWaveform should be a pointer to an EWDB_WaveformStruct.
pWaveform->idWaveform, pWaveform->tStart, pWaveform->tEnd, 
pWaveform->binSnippet, and pWaveform->iByteLen must be filled in
and valid.  idWaveform must be the DB ID of an existing 
waveform within the DB.  pWaveform->idChan is ignored (You are not
allowed to change the channel for an existing snippet.  If you have
erroniously associated a snippet with a channel, you must delete the
old snippet, and create a new one to fix the problem.
pWaveform->iDataFormat is not currently supported.

*************************************************
************************************************/
int ewdb_internal_UpdateWaveformDesc(EWDB_WaveformStruct *pWaveform);



/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW5 API FORMATTED COMMENT
TYPE FUNCTION_PROTOTYPE

LIBRARY  EWDB_API_LIB

SUB_LIBRARY WAVEFORM-INTERNAL

LANGUAGE C

LOCATION THIS_FILE

STABILITY MODIFIED

FUNCTION ewdb_internal_GetWaveformList


SOURCE_LOCATION src/oracle/schema-working/src/waveform/ewdb_internal_GetWaveformList.c

RETURN_VALUE EWDB_RETURN_FAILURE
RETURN_DESCRIPTION  Fatal error.  See logfile for details.

RETURN_VALUE EWDB_RETURN_SUCCESS
RETURN_DESCRIPTION Success

PARAMETER 1
PARAM_NAME IN_idEvent
PARAM_TYPE EWDBid
PARAM_DESCRIPTION Database ID of the Event whose waveform
snippets we are interested in.

PARAMETER 2
PARAM_NAME IN_pWaveformBuffer
PARAM_TYPE EWDB_WaveformStruct
PARAM_DESCRIPTION Pointer to a pre-allocated array of waveform structs
which will be filled by this function.

PARAMETER 3
PARAM_NAME IN_BufferRecLen
PARAM_TYPE int
PARAM_DESCRIPTION Length of the IN_pWaveformBuffer array.

DESCRIPTION The function retrieves a list of waveforms that are
associated with a given event.  See EWDB_WaveformStruct for a
description of the information retrieved for each associated waveform.

NOTE This function only retrieves the waveform DESCRIPTORS, it does not
actually retrieve the binary waveform.

*************************************************
************************************************/
int ewdb_internal_GetWaveformList(EWDBid IN_idEvent,
                                  EWDB_WaveformStruct * IN_pWaveformBuffer, 
                                  int IN_BufferRecLen);


/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW5 API FORMATTED COMMENT
TYPE FUNCTION_PROTOTYPE

LIBRARY  EWDB_API_LIB

SUB_LIBRARY WAVEFORM-INTERNAL

LANGUAGE C

LOCATION THIS_FILE

STABILITY MODIFIED

FUNCTION ewdb_internal_GetWaveformListWithCompT


SOURCE_LOCATION src/oracle/schema-working/src/waveform/ewdb_internal_GetWaveformListWithCompT.c

RETURN_TYPE int

RETURN_VALUE EWDB_RETURN_FAILURE
RETURN_DESCRIPTION  Fatal error.  See logfile for details.

RETURN_VALUE EWDB_RETURN_SUCCESS
RETURN_DESCRIPTION Success

PARAMETER 1
PARAM_NAME IN_idEvent
PARAM_TYPE EWDBid
PARAM_DESCRIPTION Database ID of the Event whose waveform
snippets we are interested in.

PARAMETER 2
PARAM_NAME IN_pWaveformBuffer
PARAM_TYPE EWDB_WaveformStruct
PARAM_DESCRIPTION Pointer to a pre-allocated array of waveform structs
which will be filled by this function.

PARAMETER 3
PARAM_NAME IN_pStationBuffer
PARAM_TYPE EWDB_StationStruct
PARAM_DESCRIPTION Pointer to a pre-allocated array of station structs
which will be filled by this function.

PARAMETER 4
PARAM_NAME IN_BufferRecLen
PARAM_TYPE int
PARAM_DESCRIPTION Length of the IN_pWaveformBuffer array.

DESCRIPTION The function retrieves a list of waveforms (and their 
associated station/component info) that are associated with a given 
event.  See EWDB_WaveformStruct and EWDB_StationStruct for a
description of the information retrieved for each associated waveform.

NOTE This function only retrieves the waveform DESCRIPTORS, it does not
actually retrieve the binary waveform.

*************************************************
************************************************/
int ewdb_internal_GetWaveformListWithCompT(EWDBid IN_idEvent,
                                           EWDB_WaveformStruct * IN_pWaveformBuffer,
                                           EWDB_StationStruct * IN_pStationBuffer,
                                           int IN_BufferRecLen);




/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW5 API FORMATTED COMMENT
TYPE FUNCTION_PROTOTYPE

LIBRARY  EWDB_API_LIB

SUB_LIBRARY WAVEFORM-INTERNAL

LANGUAGE C

LOCATION THIS_FILE

STABILITY NEW

FUNCTION ewdb_internal_CheckidWaveform


SOURCE_LOCATION src/oracle/schema-working/src/waveform/ewdb_internal_CheckidWaveform.c

RETURN_TYPE int

RETURN_VALUE EWDB_RETURN_FAILURE
RETURN_DESCRIPTION  Fatal error.  See logfile for details.

RETURN_VALUE EWDB_RETURN_SUCCESS
RETURN_DESCRIPTION Success

PARAMETER 1
PARAM_NAME IN_idWaveform
PARAM_TYPE EWDBid
PARAM_DESCRIPTION DB ID that the caller wants validated.

DESCRIPTION Checks the DB waveform table to validate that there
exists a Waveform record with the given idWaveform.  Returns
EWDB_RETURN_SUCCESS if a matching record is found, otherwise
returns EWDB_RETURN_FAILURE.

*************************************************
************************************************/
int ewdb_internal_CheckidWaveform(EWDBid idWaveform);


/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW API FORMATTED COMMENT
TYPE LIBRARY

LIBRARY  EWDB_API_LIB

SUB_LIBRARY COOKED_INFRASTRUCTURE-INTERNAL

LOCATION THIS_FILE

DESCRIPTION This is a portion of the EWDB_API_LIB
that contains C and SQL code that supports the 
COOKED_INFRASTRUCTURE_API.  This library includes 
internal code, NOT code that should be used by 
programmers using the API to access the EW DB.

*************************************************
************************************************/


/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW5 API FORMATTED COMMENT
TYPE FUNCTION_PROTOTYPE

LIBRARY  EWDB_API_LIB

SUB_LIBRARY COOKED_INFRASTRUCTURE-INTERNAL

LOCATION THIS_FILE

LANGUAGE C

STABILITY NEW

FUNCTION ewdb_internal_GetChanCTFForChannel 


SOURCE_LOCATION src/oracle/schema-working/src/infra/ewdb_internal_GetChanCTFForChannel.c

RETURN_TYPE int

RETURN_VALUE EWDB_RETURN_FAILURE
RETURN_DESCRIPTION Fatal error.  See logfile for details.

RETURN_VALUE EWDB_RETURN_SUCCESS
RETURN_DESCRIPTION Success.

PARAMETER 1
PARAM_NAME pChanCTF
PARAM_TYPE EWDB_ChanTCTFStruct *
PARAM_DESCRIPTION Pointer to a caller allocated EWDB_ChanTCTFStruct.
The function will read the idChanT(channel time interval identifier)
from the struct, and then write the gain(dGain), sample
rate(dSampRate), and cooked transform function identifier(idCookedTF)
associated with the given idChanT back to the structure.

DESCRIPTION Retrieves from the DB, the ChanCTF response information for the given
channel time interval (pChanCTF->idChanT); specifically it retrieves gain, 
sample rate, and the DB ID of the Cooked Transform Function for the channel, which
it writes back to the structure pointed to by pChanCTF.<br>
<br>
In the event of a SQL error.  EWDB_RETURN_FAILURE is returned, and the SQL error
is written to pChanCTF->tfsFunc.idCookedTF.
<br>DK 2003/12/03

NOTE ewdb_api_GetTransformFunctionForChan() should generally be called
instead of this function.  It provides the whole suite of transfer 
function information, including the transfer function.

*************************************************
************************************************/
int ewdb_internal_GetChanCTFForChannel(EWDB_ChanTCTFStruct * pChanCTF);


/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW5 API FORMATTED COMMENT
TYPE FUNCTION_PROTOTYPE

LIBRARY  EWDB_API_LIB

SUB_LIBRARY COOKED_INFRASTRUCTURE-INTERNAL

LOCATION THIS_FILE

LANGUAGE C

STABILITY NEW

FUNCTION ewdb_internal_GetTransformFunctionDesc 


SOURCE_LOCATION src/oracle/schema-working/src/infra/ewdb_internal_GetTransformFunctionDesc.c

RETURN_TYPE int

RETURN_VALUE EWDB_RETURN_FAILURE
RETURN_DESCRIPTION  Fatal error.  See logfile for details.

RETURN_VALUE EWDB_RETURN_SUCCESS
RETURN_DESCRIPTION Success

PARAMETER 1
PARAM_NAME idCookedTF
PARAM_TYPE EWDBid
PARAM_DESCRIPTION Database ID for the cooked infrastructure record.

PARAMETER 2
PARAM_NAME szCookedTFDesc
PARAM_TYPE char *
PARAM_DESCRIPTION Output description for this record.  This field should
point to a buffer (allocated by the caller) of atleast 
sizeof(EWDB_TransformFunctionStruct.szCookedTFDesc) bytes.

DESCRIPTION  Retrieves the description for a Transform function from
the DB, using the CookedTF record identifier (idCookedTF).

*************************************************
************************************************/ 
int ewdb_internal_GetTransformFunctionDesc(EWDBid idCookedTF, 
                                           char * szCookedTFDesc);



/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW5 API FORMATTED COMMENT
TYPE FUNCTION_PROTOTYPE

LIBRARY  EWDB_API_LIB

SUB_LIBRARY COOKED_INFRASTRUCTURE-INTERNAL

LOCATION THIS_FILE

LANGUAGE C

STABILITY NEW

FUNCTION ewdb_internal_GetTransformFunction 


SOURCE_LOCATION src/oracle/schema-working/src/infra/ewdb_internal_GetTransformFunction.c

RETURN_TYPE int

RETURN_VALUE EWDB_RETURN_FAILURE
RETURN_DESCRIPTION Fatal error.  See logfile for details.

RETURN_VALUE EWDB_RETURN_SUCCESS
RETURN_DESCRIPTION Success.

PARAMETER 1
PARAM_NAME idCookedTF
PARAM_TYPE EWDBid
PARAM_DESCRIPTION DB ID of the Cooked Transform function for which the caller
wants to obtain Poles and Zeroes.

PARAMETER 2
PARAM_NAME pCookedTF
PARAM_TYPE EWDB_TransformFunctionStruct *
PARAM_DESCRIPTION Pointer to a caller allocated
EWDB_TransformFunctionStruct where the function will write the cooked
transform function (Poles and Zeroes) requested by the caller.
The transform Function is used to transform recorded waveforms back into ground motion.

DESCRIPTION Function retrieves the poles/zeroes based transform
function identified by idCookedTF from the DB.  It stores the 
results in the structure pointed to by pCookedTF.  If no poles/zeroes 
are found for the given idCookedTF, success is still
returned, but pCookedTF->iNumPoles=pCookedTF->iNumZeroes=0.

NOTE Call ewdb_api_GetTransformFunctionForChan() to retrieve the transform
function for a given channel along with gain and sample rate.

*************************************************
************************************************/
int ewdb_internal_GetTransformFunction(EWDBid idCookedTF,
                                       EWDB_TransformFunctionStruct * pCookedTF);

/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW5 API FORMATTED COMMENT
TYPE FUNCTION_PROTOTYPE

LIBRARY  EWDB_API_LIB

SUB_LIBRARY COOKED_INFRASTRUCTURE-INTERNAL

LOCATION THIS_FILE

LANGUAGE C

STABILITY NEW

FUNCTION ewdb_internal_SetChanParams 


SOURCE_LOCATION src/oracle/schema/src/infra/ewdb_internal_SetChanParams.c

RETURN_TYPE int

RETURN_VALUE EWDB_RETURN_FAILURE
RETURN_DESCRIPTION  Fatal error.  See logfile for details.

RETURN_VALUE EWDB_RETURN_SUCCESS
RETURN_DESCRIPTION Success

PARAMETER 1
PARAM_NAME pChan
PARAM_TYPE EWDB_ChannelStruct *
PARAM_DESCRIPTION Pointer to the ChannelStruct which the caller has
filled with the parameters for the given channel.

DESCRIPTION Sets the parameters for a channel for a given time period.
pChan->Comp.idChan defines the channel.  pChan->tOn/tOff define the
time period.  pchan->idCompT defines the Component(time-interval) with
which the channel is associated for the given time period.  All other
pChan members are ignored.<br>
Upon Successfull completion, the DB ID of the new ChanT(channel time-interval)
record is written back to pChan->idChanT.<br>
<br>
In the event of a SQL error.  EWDB_RETURN_FAILURE is returned, and the SQL error
is written to pChan->idChanT.
<br>DK 2003/12/03

*************************************************
************************************************/
int ewdb_internal_SetChanParams(EWDB_ChannelStruct * pChan);


/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW API FORMATTED COMMENT
TYPE LIBRARY

LIBRARY  EWDB_API_LIB

SUB_LIBRARY RAW_INFRASTRUCTURE_API

LOCATION THIS_FILE

DESCRIPTION This is the Raw Infrastructure portion
of the EWDB_API_LIB library.  It provides access to
the station history, inventory, and response portions
of the Earthworm DB.  It has not yet been tested and
should not be used without first contacting the 
Earthworm central.

*************************************************
************************************************/
/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW API FORMATTED COMMENT
TYPE LIBRARY

LIBRARY  EWDB_API_LIB

SUB_LIBRARY RAW_INFRASTRUCTURE-INTERNAL

LOCATION THIS_FILE

DESCRIPTION This is a portion of the EWDB_API_LIB
that contains constants SQL code that supports
the RAW_INFRASTRUCTURE_API. This library includes internal
code, not code that should be used by programmers
using the API to access the EW DB.

*************************************************
************************************************/


int ewdb_api_BindDeviceToSlot(EWDBid IN_idDeviceToBind, EWDBid IN_idSlotToBind,
                              EWDBid IN_idContextDevice, 
                              double IN_tOn, double IN_tOff, int IN_bOverwrite, 
                              int * pSQLRetCode, EWDBid * pidDeviceBind);

int ewdb_api_CreateDeviceSlot(EWDBid IN_idSlotType, char * IN_szSlotName,
                              int * pSQLRetCode, EWDBid * pidDeviceSlot);

int ewdb_api_CreateDeviceType(EWDBid IN_idModule, char * IN_szDeviceTypeName,
                              int * pSQLRetCode, EWDBid * pidDeviceType);

int ewdb_api_CreateModule(EWDBid IN_idSlotType, char * IN_szModuleName,
                          int * pSQLRetCode, EWDBid * pidModule);

int ewdb_api_CreateModuleEntry(EWDBid IN_idModule, int IN_iPlexor,
                               EWDBid IN_idSubDevSlot, int IN_iSubPlexor,
                               int * pSQLRetCode, EWDBid * pidModuleEntry);

int ewdb_api_CreateModuleTemplate(EWDBid IN_idDeviceSlot, int IN_iPlexor,
                                  EWDBid IN_idNextDevSlot, int IN_iNextPlexor,
                                  int * pSQLRetCode, EWDBid * pidModuleTemplate);

int ewdb_api_CreateOrUpdateDevice(EWDBid IN_idDeviceModel,
                                  char * IN_szDeviceName, 
                                  EWDBid IN_idDeviceType,
                                  int IN_bIsGeneric, int IN_bIsRealizable,
                                  int * pSQLRetCode, EWDBid * pidDevice);

int ewdb_api_CreateSlotType(int IN_iNumInputs, int IN_iNumOutputs,
                            char * IN_szSlotTypeName,
                            int * pSQLRetCode, EWDBid * pidSlotType);

int ewdb_api_GetDeviceInfo(EWDBid IN_idDevice, int IN_bWantRealizable,
                           EWDBid * pidModule, char * OUT_szDeviceName,
                           EWDBid * pidDeviceModel, EWDBid * pidDeviceType,
                           int * pbIsRealizable, int * pSQLRetCode);

int ewdb_api_GetDeviceSlotInfo(EWDBid IN_idDeviceSlot, char * OUT_szSlotName,
                               int * pSQLRetCode, EWDBid * pidSlotType);

int ewdb_api_GetDeviceTypeInfo(EWDBid IN_idDeviceType, 
                               char * OUT_szDeviceTypeName,
                               int * pSQLRetCode, EWDBid * pidModule);


int ewdb_api_GetModuleEntry(EWDBid IN_idModule, int IN_iPlexor,
                            EWDBid * pidSubDevSlot, EWDBid * piSubPlexor, 
                            int * pSQLRetCode);

int ewdb_api_GetModuleInfo(EWDBid IN_idModule,
                           EWDBid * pidSlotType, char * OUT_szModuleName,
                           char * OUT_szSlotTypeName,
                           int * piNumInputs, int * piNumOutputs, 
                           int * pSQLRetCode);

int ewdb_api_GetNextPlexor(EWDBid IN_idDeviceSlot, int IN_iPlexor,
                           EWDBid * pidNextDevSlot, EWDBid * piNextPlexor,
                           int * pSQLRetCode);

int ewdb_api_GetPlexorForChanT(EWDBid IN_idChanT, int IN_idChan,  
                               double IN_tTime, EWDBid * pidDeviceSlot, 
                               EWDBid * piPlexor,
                               int * pSQLRetCode);

int ewdb_api_GetSlotTypeInfo(EWDBid IN_idSlotType,
                             char * OUT_szSlotTypeName,
                             int * piNumInputs, int * piNumOutputs, 
                             int * pSQLRetCode);

int ewdb_api_SetDeviceFunction(EWDBid IN_idDeviceToBind, 
                               EWDBid IN_idFunctionToBind,
                               char * IN_szFunctionType, 
                               double IN_tOn, double IN_tOff,
                               int IN_bOverwrite, int IN_bOverridable,
                               int * pSQLRetCode, EWDBid * pidFunctionBind);

int ewdb_api_SetPlexorForChannel(EWDBid IN_idChan, EWDBid IN_idDeviceSlot,
                                 int IN_iPlexor, double IN_tOn, double IN_tOff,
                                 int * pSQLRetCode, EWDBid * pidChanT);



/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW5 API FORMATTED COMMENT
TYPE FUNCTION_PROTOTYPE

LIBRARY  EWDB_API_LIB

SUB_LIBRARY WAVEFORM-INTERNAL

LOCATION THIS_FILE

LANGUAGE C

STABILITY NEW

FUNCTION ewdb_internal_CreateSnippetRequest 


SOURCE_LOCATION src/oracle/schema-working/src/concierge/ewdb_internal_CreateSnippetRequest.c

RETURN_TYPE int

RETURN_VALUE EWDB_RETURN_FAILURE
RETURN_DESCRIPTION Fatal error.  See logfile for details.

RETURN_VALUE EWDB_RETURN_SUCCESS
RETURN_DESCRIPTION Success.

PARAMETER 1
PARAM_NAME pSnipReq
PARAM_TYPE EWDB_SnippetRequestStruct *
PARAM_DESCRIPTION Pointer to an EWDB_SnippetRequestStruct that contains 
parameters describing what waveform snippet should be retrieved.


DESCRIPTION The Function creates a SnippetRequest record in the DB.
The record describes snippet data that the caller wants stored in the DB.
(This call does not retrieve snippet data, it only records the request.)
A snippet is essentially a piece of data from a given channel for a given
time interval.  The EWDB_SnippetRequestStruct passed by the caller describes
both the snippet to be retrieved and also the frequency for attempting
to retrieve the snippet.  The function will write the DB ID of the newly 
created snippet request back to pSnipReq->idSnipReq.

NOTE The function ignores pSnipReq->ComponentInfo and 
pSnipReq->idExistingWaveform.

*************************************************
************************************************/
int ewdb_internal_CreateSnippetRequest(EWDB_SnippetRequestStruct *pSnipReq);

/* DK CLEANUP 051602  Add documentation for these internal functions. */

int ewdb_internal_GetReservedSnipReqs(EWDB_SnippetRequestStruct * pBuffer,
                                             int iReserveKey,
                                             int * pNumItemsFound, 
                                             int * pNumItemsRetrieved,
                                             int BufferLen);

int ewdb_internal_LockReservedSnipReqs(int iReserveKey, int * piLockTime);

int ewdb_internal_LockSnipReq(EWDB_SnippetRequestStruct * pSnipReq,
                              int IN_iReserveKey, int * piLockTime);

int ewdb_internal_ReleaseReservedSnipReqs(int iReserveKey);

int ewdb_internal_ReserveSnippetRequests(EWDBid idSnipReq,
                                         time_t tThreshold, 
                                         int iRequestGroup,
                                         int iReserveKey);




/************************************************
************ SPECIAL FORMATTED COMMENT **********
TYPE FUNCTION_PROTOTYPE

LIBRARY EWDB_API_LIB

SUB_LIBRARY PARAMETRIC-INTERNAL

LANGUAGE C

LOCATION THIS_FILE


FUNCTION ewdb_internal_GetAmpsByTime

STABILITY NEW

SOURCE_LOCATION THIS_FILE

RETURN_TYPE int

RETURN_VALUE EWDB_RETURN_FAILURE
RETURN_DESCRIPTION Fatal error.  See logfile for details.

RETURN_VALUE EWDB_RETURN_SUCCESS
RETURN_DESCRIPTION Success.

RETURN_VALUE EWDB_RETURN_WARNING
RETURN_DESCRIPTION Partial success.  A list of amplitudes was retrieved,
but the caller's buffer was not large enough to accomadate all of the
amplitudes found.  See pNumAmpsFound for the number of arrivals found
and pNumAmpsRetrieved for the number of amplitudes placed in the
caller's buffer.

PARAMETER 1
PARAM_NAME tStart
PARAM_TYPE time_t
PARAM_DESCRIPTION Start time of criteria window.  (Secs since 1970)

PARAMETER 2
PARAM_NAME tEnd
PARAM_TYPE time_t
PARAM_DESCRIPTION End time of criteria window.  (Secs since 1970)

PARAMETER 3
PARAM_NAME iMagType
PARAM_TYPE MAGNITUDE_TYPE
PARAM_DESCRIPTION Type of amplitude the caller is searching for.
"Amplitude Type" constants are defined in <earthworm_defs.h>.

PARAMETER 4
PARAM_NAME pAmpBuffer
PARAM_TYPE EWDB_PeakAmpStruct *
PARAM_DESCRIPTION Buffer allocated by the caller, where the function
will write the list of amplitudes retrieved.

PARAMETER 5
PARAM_NAME pNumAmpsFound
PARAM_TYPE int *
PARAM_DESCRIPTION Pointer to an integer where the function will write
the number of amplitudes found.

PARAMETER 6
PARAM_NAME pNumAmpsRetrieved
PARAM_TYPE int *
PARAM_DESCRIPTION Pointer to an integer where the function will write
the number of amplitudes retrieved and placed in the caller's buffer(pAmpBuffer).

PARAMETER 7
PARAM_NAME iBufferLen
PARAM_TYPE int
PARAM_DESCRIPTION Size of the pAmpBuffer buffer in as a multiple of
EWDB_PeakAmpStruct. (example: 15 structs)  If pStationBuffer is
not NULL, then it must also contain space for this many 
EWDB_StationStruct records.

DESCRIPTION Function retrieves a list of amplitudes of a given type
for a given time window.  Use tStart and tEnd to specify the time 
window for which you are interested in amplitudes.  

*************************************************
************************************************/
int ewdb_internal_GetAmpsByTime(time_t IN_tStart, time_t IN_tEnd, MAGNITUDE_TYPE IN_iMagType,
                                EWDB_PeakAmpStruct * pAmpBuffer,
                                int * pNumAmpsFound, int * pNumAmpsRetrieved,
                                int iBufferLen);


/*********************************************************/

/************************************************
************ SPECIAL FORMATTED COMMENT **********
TYPE FUNCTION_PROTOTYPE

LIBRARY EWDB_API_LIB

SUB_LIBRARY PARAMETRIC-INTERNAL

LANGUAGE C

LOCATION THIS_FILE


FUNCTION ewdb_internal_GetAmpsByTime_w_StaInfo

STABILITY NEW

SOURCE_LOCATION THIS_FILE

RETURN_TYPE int

RETURN_VALUE EWDB_RETURN_FAILURE
RETURN_DESCRIPTION Fatal error.  See logfile for details.

RETURN_VALUE EWDB_RETURN_SUCCESS
RETURN_DESCRIPTION Success.

RETURN_VALUE EWDB_RETURN_WARNING
RETURN_DESCRIPTION Partial success.  A list of amplitudes (plus associated
channel/station info) was retrieved,
but the caller's buffer was not large enough to accomadate all of the
amplitudes found.  See pNumAmpsFound for the number of arrivals found
and pNumAmpsRetrieved for the number of amplitudes placed in the
caller's buffer.

PARAMETER 1
PARAM_NAME tStart
PARAM_TYPE time_t
PARAM_DESCRIPTION Start time of criteria window.  (Secs since 1970)

PARAMETER 2
PARAM_NAME tEnd
PARAM_TYPE time_t
PARAM_DESCRIPTION End time of criteria window.  (Secs since 1970)

PARAMETER 3
PARAM_NAME iMagType
PARAM_TYPE MAGNITUDE_TYPE
PARAM_DESCRIPTION Type of amplitude the caller is searching for.
"Amplitude Type" constants are defined in <earthworm_defs.h>.

PARAMETER 4
PARAM_NAME pAmpBuffer
PARAM_TYPE EWDB_PeakAmpStruct *
PARAM_DESCRIPTION Buffer allocated by the caller, where the function
will write the list of amplitudes retrieved.

PARAMETER 5
PARAM_NAME pStationBuffer
PARAM_TYPE EWDB_StationStruct *
PARAM_DESCRIPTION Buffer allocated by the caller, where the function
will write the list of station/channel info associated with
the amplitudes retrieved.  pStationBuffer must point to an allocated
buffer with room for atleast (iBufferLen) EWDB_StationStructs.

PARAMETER 6
PARAM_NAME pNumAmpsFound
PARAM_TYPE int *
PARAM_DESCRIPTION Pointer to an integer where the function will write
the number of amplitudes found.

PARAMETER 7
PARAM_NAME pNumAmpsRetrieved
PARAM_TYPE int *
PARAM_DESCRIPTION Pointer to an integer where the function will write
the number of amplitudes retrieved and placed in the caller's buffer(pAmpBuffer).

PARAMETER 8
PARAM_NAME iBufferLen
PARAM_TYPE int
PARAM_DESCRIPTION Size of the pAmpBuffer buffer in as a multiple of
EWDB_PeakAmpStruct. (example: 15 structs)  pStationBuffer must also 
contain space for this many EWDB_StationStruct records.

DESCRIPTION Function retrieves a list of amplitudes of a given type,
plus associated station/channel info for a given time window.  
Use tStart and tEnd to specify the time 
window for which you are interested in amplitudes.  


*************************************************
************************************************/
int ewdb_internal_GetAmpsByTime_w_StaInfo(time_t IN_tStart, time_t IN_tEnd, MAGNITUDE_TYPE IN_iMagType,
                                          EWDB_PeakAmpStruct * pAmpBuffer,
                                          EWDB_StationStruct * pStationBuffer,
                                          int * pNumAmpsFound, int * pNumAmpsRetrieved,
                                          int iBufferLen);

/*********************************************************/


/************************************************
************ SPECIAL FORMATTED COMMENT **********
TYPE FUNCTION_PROTOTYPE

LIBRARY EWDB_API_LIB

SUB_LIBRARY PARAMETRIC-INTERNAL

LANGUAGE C

LOCATION THIS_FILE


FUNCTION ewdb_internal_GetAmpsByEvent

STABILITY NEW

SOURCE_LOCATION THIS_FILE

RETURN_TYPE int

RETURN_VALUE EWDB_RETURN_FAILURE
RETURN_DESCRIPTION Fatal error.  See logfile for details.

RETURN_VALUE EWDB_RETURN_SUCCESS
RETURN_DESCRIPTION Success.

RETURN_VALUE EWDB_RETURN_WARNING
RETURN_DESCRIPTION Partial success.  A list of amplitudes was retrieved,
but the caller's buffer was not large enough to accomadate all of the
amplitudes found.  See pNumAmpsFound for the number of arrivals found
and pNumAmpsRetrieved for the number of amplitudes placed in the
caller's buffer.

PARAMETER 1
PARAM_NAME idEvent
PARAM_TYPE EWDBid
PARAM_DESCRIPTION DB identifier of the Event for which the caller
wants a list of associated amplitudes.

PARAMETER 2
PARAM_NAME iMagType
PARAM_TYPE MAGNITUDE_TYPE
PARAM_DESCRIPTION Type of amplitude the caller is searching for.
"Amplitude Type" constants are defined in <earthworm_defs.h>.

PARAMETER 3
PARAM_NAME pAmpBuffer
PARAM_TYPE EWDB_PeakAmpStruct *
PARAM_DESCRIPTION Buffer allocated by the caller, where the function
will write the list of amplitudes retrieved.

PARAMETER 4
PARAM_NAME pNumAmpsFound
PARAM_TYPE int *
PARAM_DESCRIPTION Pointer to an integer where the function will write
the number of amplitudes found.

PARAMETER 5
PARAM_NAME pNumAmpsRetrieved
PARAM_TYPE int *
PARAM_DESCRIPTION Pointer to an integer where the function will write
the number of amplitudes retrieved and placed in the caller's buffer(pAmpBuffer).

PARAMETER 6
PARAM_NAME iBufferLen
PARAM_TYPE int
PARAM_DESCRIPTION Size of the pAmpBuffer buffer in as a multiple of
EWDB_PeakAmpStruct. (example: 15 structs)

DESCRIPTION Function retrieves a list of amplitudes of a given type
that are associated with a given DB Event.  Use idEvent to define the
DB Event for which you want a list of associated amplitudes. 

NOTE This function only retrieves amplitudes that are directly associated
with an Event.  It will not retrieve amplitudes that have been associated
with an Event via a Magnitude.


*************************************************
************************************************/
int ewdb_internal_GetAmpsByEvent(EWDBid IN_idEvent, MAGNITUDE_TYPE IN_iMagType,
                                 EWDB_PeakAmpStruct * pAmpBuffer,
                                 int * pNumAmpsFound, int * pNumAmpsRetrieved,
                                 int iBufferLen);

/*********************************************************/

/************************************************
************ SPECIAL FORMATTED COMMENT **********
TYPE FUNCTION_PROTOTYPE

LIBRARY EWDB_API_LIB

SUB_LIBRARY PARAMETRIC-INTERNAL

LANGUAGE C

LOCATION THIS_FILE


FUNCTION ewdb_internal_GetAmpsByEvent_w_StaInfo

STABILITY NEW

SOURCE_LOCATION THIS_FILE

RETURN_TYPE int

RETURN_VALUE EWDB_RETURN_FAILURE
RETURN_DESCRIPTION Fatal error.  See logfile for details.

RETURN_VALUE EWDB_RETURN_SUCCESS
RETURN_DESCRIPTION Success.

RETURN_VALUE EWDB_RETURN_WARNING
RETURN_DESCRIPTION Partial success.  A list of amplitudes was retrieved,
along with associated channel/station information,
but the caller's buffer was not large enough to accomadate all of the
amplitudes found.  See pNumAmpsFound for the number of arrivals found
and pNumAmpsRetrieved for the number of amplitudes placed in the
caller's buffer.

PARAMETER 1
PARAM_NAME idEvent
PARAM_TYPE EWDBid
PARAM_DESCRIPTION DB identifier of the Event for which the caller
wants a list of associated amplitudes.

PARAMETER 2
PARAM_NAME iMagType
PARAM_TYPE MAGNITUDE_TYPE
PARAM_DESCRIPTION Type of amplitude the caller is searching for.
"Amplitude Type" constants are defined in <earthworm_defs.h>.

PARAMETER 3
PARAM_NAME pAmpBuffer
PARAM_TYPE EWDB_PeakAmpStruct *
PARAM_DESCRIPTION Buffer allocated by the caller, where the function
will write the list of amplitudes retrieved.

PARAMETER 4
PARAM_NAME pStationBuffer
PARAM_TYPE EWDB_StationStruct *
PARAM_DESCRIPTION Buffer allocated by the caller, where the function
will write the list of station/channel info associated with
the amplitudes retrieved.  

PARAMETER 5
PARAM_NAME pNumAmpsFound
PARAM_TYPE int *
PARAM_DESCRIPTION Pointer to an integer where the function will write
the number of amplitudes found.

PARAMETER 6
PARAM_NAME pNumAmpsRetrieved
PARAM_TYPE int *
PARAM_DESCRIPTION Pointer to an integer where the function will write
the number of amplitudes retrieved and placed in the caller's buffer(pAmpBuffer).

PARAMETER 7
PARAM_NAME iBufferLen
PARAM_TYPE int
PARAM_DESCRIPTION Size of the pAmpBuffer buffer in as a multiple of
EWDB_PeakAmpStruct. (example: 15 structs)  pStationBuffer must also 
contain space for this many EWDB_StationStruct records.

DESCRIPTION Function retrieves a list of amplitudes of a given type,
along with associated station/channel info, 
that are associated with a given DB Event.  Use idEvent to define the
DB Event for which you want a list of associated amplitudes.  

NOTE This function only retrieves amplitudes that are directly associated
with an Event.  It will not retrieve amplitudes that have been associated
with an Event via a Magnitude.


*************************************************
************************************************/
int ewdb_internal_GetAmpsByEvent_w_StaInfo(EWDBid IN_idEvent, MAGNITUDE_TYPE IN_iMagType,
                                           EWDB_PeakAmpStruct * pAmpBuffer,
                                           EWDB_StationStruct * pStationBuffer,
                                           int * pNumAmpsFound, int * pNumAmpsRetrieved,
                                           int iBufferLen);

/*********************************************************/

int ewdb_internal_TS_SQLtoC(char * szSQLTSBuffer1, char * szSQLTSBuffer2, double * pTSList, int* piNumDP);
int ewdb_internal_TS_CtoSQL(double * pTSList, int iNumDP, char * szSQLTSBuffer1, char * szSQLTSBuffer2 );

int ewdb_internal_CreateComment(EWDBid *pidComment, char *IN_szComment);
int ewdb_internal_UpdateCommentNext(EWDBid idComment, EWDBid idNextComment);
int ewdb_internal_DeleteComment(EWDBid idComment);

int ewdb_internal_CreateMwChanTS(EWDB_MwTimeSeriesStruct * pMwCTS);
int ewdb_internal_CreateMwChan(EWDB_MwChanStruct * pMwChan);

