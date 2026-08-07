#

# Configuration file for mag2ora:
# I listen to magnitude messages (TYPE_MAGNITUDE), and stuff the contents
# into an Oracle server. It uses rw_mag.h to go from magnitude name to the
# official number - which the DB recognizes.

LogFile 1		# 0 means don't create a disc log file. 1=> do.
MyModuleId
RingName
HeartBeatInt


# What message logos to listen to. Can be more than one.
# The type is hard coded to TYPE_MAGNITUDE

GetEventsFrom	INST_USNSN	MOD_WILDCARD


# Data Base Server Identification. Not for the security squeamish.
DBservice     db_service
DBpassword    db_pwd
DBuser        db_user

# TimeoutSeconds: how long to wait for the origin to become
# available in the DB.
TimeoutSeconds	10

#
# SetAsPreferred (optional) should be un-commented out in order to flag 
# all magnitudes inserted by mag2ora as preferred. This is an 
# installation specific business rule.
#
SetAsPreferred

#
# DoAlarms (optional) if uncommented, mag2ora will run the alarms
#  system after inserting a magnitude
#
DoAlarms

#
# NetworkCode (optional): two letter network designation code to be used
#  in building QDDS alarm messages. OPTIONAL, set to blank otherwise.
#
NetworkCode  UU


# Debug switch: the token "Debug" (without the quotes) can be stated.
# If it is, lots of weird debug messages will be produced 
Debug

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

