
# configuration file for unlocker

# unlocker is a program that breaks stale locks
# on snippet requests.
#
# This program is meant to be run out of cron to clean
# up old events. The configuration options below allows
# for this period cleanup by specifying the start date
# with respect to the date of the cleanup, and the number
# of days to clean up.
#



#
# Database connection parameters
#
#
DBuser     ewdb_main
DBpassword PASSWORD
DBservice  eqsX.usgs


#
# Logfiledir - where log files are written
#  *NOTE*  Must end in '/' or '\' to reflect it is a directory
#
Logfiledir j:\temp\

# 
# HoursTillUnlocking - the number of hours before a lock should be 
#   considered stale and unlocked by this program.
#   This program uses HoursTillUnlocking as a deltaT for a threshold
#   time, to determine which locks are still valid and which should
#   be treated as stale and forced open.
HoursTillUnlocking 0.5


#
# Debug - optional debug flag
#
#Debug		1


