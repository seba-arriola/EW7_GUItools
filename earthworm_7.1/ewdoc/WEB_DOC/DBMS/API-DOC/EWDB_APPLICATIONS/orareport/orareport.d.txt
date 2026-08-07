
#
# This is orareport's parameter file

#  Basic Earthworm setup:
#
MyModuleId         MOD_REPORT_B   # module id for this instance of report 
RingName           HYPO_RING      # ring to get input from
LogFile            1              # 0 to completely turn off disk log file
HeartBeatInt       15             # seconds between heartbeats

# List the message logos to grab from transport ring
#              Installation       Module          Message Types
GetEventsFrom  INST_WILDCARD    MOD_EQPROC     # hinvarc 

# Set up database stuff
DBservice   db_service
DBuser      db_user
DBpassword  db_pwd

# Optional command for debug output.  Comment out to turn off debug output.
# Debug 

#
# Optional command: If uncommented, orareport will run the alarms
#  system after inserting the event.
#
DoAlarms

#
# NetworkCode: two letter network designation code to be used
#  in building QDDS alarm messages. OPTIONAL, set to blank otherwise.
#
NetworkCode	 UU

#
# Nearest Town display options -- OPTIONAL
#
#   minPopulation:  towns with more than this are considered BIG
#   showPopulation:  if set to non-zero, populations of towns will be shown
#
#    if these are not present, minPopulation = 1; showPopulation = 0
#
minPopulation   30000
showPopulation  1

