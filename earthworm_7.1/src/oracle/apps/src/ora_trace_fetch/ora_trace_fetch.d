#  ora_trace_fetch.d  sample configuration file

# LogFile
# Values:	< 0 | 1 >
# Description
#  Flag that controls whether "logit" messages are writtent to 
#  a logfile on disk.  0 = No, 1 = Yes
LogFile 1   

# MyModuleId
# Values:	MANY from earthworm_global.d or earthworm.d
# Description
#  MyModuleId indicates the moduleID that this program should use
#   for logging and communicating over Earthworm Rings.
#   The value must equate to one of the "Module" constants listed
#   in either earthworm_global.d or earthworm.d
MyModuleId     MOD_ORA_TRACE_FETCH  

# RingName 
# Values: MANY from earthworm.d
# Description
#  RingName describes the Earthworm ring that this program will attach to.
#   The value must equate to one of the "Ring" constants listed
#   in earthworm.d, as well as be listed in the startstop config file.
#
#   This program uses a ring to read Trigger messages and write 
#   status information.
RingName       HYPO_RING

# HeartBeatInt
# Values: > 0
# Description
#  HeartBeatInt is the interval length in seconds between "heartbeat" messages
#   sent by this program to StatMgr.  The value represents the MINIMUM
#   amount of time between status messages.  The MAXIMUM should be
#   close to HeartBeatInt, but may be longer if the program is 
#   busy processing data.
HeartBeatInt   120


#######################
# DB Server Information

# DBservice
# Values: TEXT
# Description
#  DBservice is the Oracle Network ServiceID of the database server instance
#  that the program should connect to.  This is usually defined in a 
#  tnsnames.ora file within this machine's Oracle directory tree.  It may
#  also be defined by an Oracle Names service if one is running.
DBservice     db_service

# DBuser
# Values: TEXT
# Description
#  DBuser is the Oracle UserID that the program should use to connect to
#  the given database server.
DBuser        db_user

# DBpassword
# Values: TEXT
# Description
#  DBpassword is the password associated with the DBuser Oracle UserID.
DBpassword    db_pwd

# WaveServer
# Values: <IP_ADDRESS> <PORT#>
# Description
#  The WaveServer command describes a wave_server that the program can use
#  to obtain data.  The format of the command is
#  <IP ADDRESS> <PORT#>
#  where the <IP ADDRESS> is the AAA.BBB.CCC.DDD IP address that the
#  wave_serverV is using, and <PORT#> is the TCP Port that the server
#  is listening to.
# Note:  More than one WaveServer command can occur in a config file.
#        There must be one WaveServer command per server, and currently
#        a max of (10) servers are supported.
#        THERE MUST BE ATLEAST ONE WaveServer command
WaveServer 192.168.1.250 8888

# MaxTraces
# Values: INTEGER > 0
# Description
#  The maximum number of trace requests that ora_trace_fetch will attempt
#  to retrieve from the database each time.
#  This parameter controls how much RAM is taken up by permanent buffers
#  used to retrieve requests from the DB and maintain state information
#  while retrieving trace data.
MaxTraces 100


#######################
# OPTIONAL Commands    


# Debug
# Values: 
#		DEBUG_OTF_NONE       0    -- Only internal program errors are 
#                                            printed.  (NOT RECOMMENDED)
#		DEBUG_OTF_ERROR     10    -- Only error information is logged
#                                            DEFAULT (RECOMMENDED)
#		DEBUG_OTF_WARNING  100    -- WARNING and ERROR info is logged
#                                            GOOD FOR DEBUGGING PROBLEMS WITH
#                                            DATA, BUT GENERATES MULTI-MB LOG
#                                            FILES EACH DAY 
#		DEBUG_OTF_ALL     1000    -- DEBUG, WARNING, and ERROR info is
#                                            logged. (NOT RECOMMENDED)
#                                            For use in debugging very low-level 
#                                            problems or destroying disk drives
# Description
#  Debug value that sets debugging information level for the program.  The
#  debug value affects debug statements within the program as well
#  as debug statements in library code called by the program.
#  Debugging is off by default.  If the Debug command is listed in this 
#  config file, then debugging is turned on.
# Debug 10


# TimeoutSeconds
# Values: INTEGER NUMBER
# Description
#  The number of seconds the program will wait for data from a wave_server.
#  The program passes the timeout to the wsClient libraries, and then
#  wsClient (wave_server client) libraries ensure that the program will
#  not wait more than TimeouSeconds seconds for data.
#  If TimeoutSeconds is set to <= 0 then NO TIMEOUT will be enforced.
# The Default value is -1, and thus NO TIMEOUT.
# TimeoutSeconds 30

# TraceBufferLen
# Values: INTEGER NUMBER
# Description
#  The maximum size of a snippet (in 1000s of bytes) that the program will
#  handle.  If a snippet exceeds this size, an error message will be sent
#  to statmgr, and the snippets contents will be dropped.
# Note:  Setting the TraceBufferLen to too small a value, can cause
#  problems when merging snippets.  Say you ask for a 1 hour snippet,
#  starting now, and the program continually retrieves data for that
#  request, and continually keeps merging the data together.  After
#  accumulating an hour of data, the snippet gets too big, and the request
#  for more data is successfull, but the snippets are not merged because
#  of size problems and all of the new data is dropped.
# Note:  If you are expecting big snippets, you should take that into
#  account when setting the TimeoutSeconds config variable.  If you are
#  expecting to grab a 1MB snippet from a wave_server over the network,
#  then your timeout value better be more than a couple of seconds,
#  since the transfer will take 30 seconds on a good day.
TraceBufferLen 500

# ProgramMode
# Values: <ALWAYS_CHECK_LIST | ALWAYS_GET_SNIPPETS | STANDARD_DATA_MODE>
# Description
#  ProgramMode tells the program which mode to run in.
#  In "ALWAYS_GET_SNIPPETS" mode, the program will attempt to process
#   all snippet_requests in the database, whether they are scheduled
#   for processing or not, every time that it passes through the main
#   processing loop.  This mode is CPU intensive, DB intensive, and
#   wave_server intensive.
#
#  In "ALWAYS_CHECK_LIST" mode, the program will check the list of
#   snippet_requests in the database, each time it passes through the
#   main processing loop, whether it expects to find any or not.
#   This mode is DB license intensive, but is not very CPU or DB
#   intensive, and is no more wave_server intensive than the standard mode.
#
#  In the "Standard Mode" the program runs as efficiently as possible
#   in terms of CPU and DB resources.  The program will only
#   check the list of snippet_requests in the database, when it thinks
#   it is time to process one.  It figures out whether there is a request
#   or not based upon it's knowledge of what is already in the database,
#   and any TYPE_ALERT or TYPE_TRIGLIST2K messages it gets from an EW Ring.
#ProgramMode STANDARD_DATA_MODE

# ActionInterval
# Values: INTEGER >= 0
# Description
#  The minimum number of seconds between passes through the program's 
#  main processing loop.  The program constantly checks for 
#  Earthworm Messages and flags, but it only performs snippet-request
#  processing every X seconds, or when woken up by a relevan EW mesage.
#  What the program does each time it passes through the main processing
#  loop is determined by the "ProgramMode" config variable.
# Note: setting the ActionInterval to 0 will make the program VERY CPU
#  intensive, to the point where it will interfere with other programs
#  on the same machine, when running in ALWAYS_GET_SNIPPETS mode. 
# Note:  The default ActionInterval is 20 seconds.
#ActionInterval 20


# GracePeriod
# Values:  > 0.0
# Description
#  The number of seconds of grace that will be used when comparing
#  a snippet its corresponing snippet-request.  If a snippet-request
#  is processed, and a snippet is retrieved from a wave_server, and
#  the start/end time of the snippet are within GracePeriod seconds
#  of the start/end time of the request, then the request will be 
#  considered filled.  Otherwise, the obtained snippet will be
#  saved to the database, and the request will be modified to obtain
#  ask for the difference between the original request and the obtained
#  snippet.  If you care about getting every sample of data from 
#  your requests, then you should set GracePeriod to 0.0.
# Note:  The default value for GracePeriod is 2.0 seconds.
#GracePeriod 2.0


