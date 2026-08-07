#  ora_trace_req.d  sample configuration file

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
MyModuleId     MOD_ORA_TRACE_REQ    

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

# GetEventsFrom
# Values: INSTALLATION values should come from INSTALLATION constants and
#  MODULE values should come from MODULE constants, both from
#  earthworm.d or earthworm_global.d
# Description
#  GetEventsFrom tells the program from which INSTALLATIONS/MODULES it should 
#  listen for messages.  WildcaRds are supported for both INSTALLATIONS and
#  MODULES.  See earthworm_global.d for further information.
#   
#  Multiple "GetEventsFrom" lines can appear in a config file.  The program
#  will maintain a list of acceptable message LOGO's.  Currently, a maximum
#  of (2) GetEventsFrom lines are supported.
# 
# Note: This program only listens for TYPE_TRIGLIST2K messages.
GetEventsFrom INST_WILDCARD MOD_WILDCARD


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


#######################
# OPTIONAL Commands    


# Debug
# Values: <FLAG_ONLY>
# Description
#  Debug switch that turns debugging information on for the program.  The
#  debug switch should affect debug statements within the program as well
#  as debug statements in library code called by the program.
#  If the debug switch is on, lots of weird debug messages will be produced 
#  in the log file.
#  Debugging is off by default.  If the Debug command is listed in this 
#  config file, then debugging is turned on.
# Debug


