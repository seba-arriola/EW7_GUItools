
# configuration file for "reaper"
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
#   *NOTE*  Logfiledir must end in a '/' or a '\' to
#   denote a directory.  For example
#         Logfiledir j:\temp
#   results in the logfile j:\tempMyLogfile050502.log being created,
#   while
#         Logfiledir j:\temp\ 
#   results in the logfile j:\temp\MyLogfile050502.log being created.
#   the latter is usually what most people want.
#
Logfiledir j:\temp\



#
# Debug - optional debug flag
#
#Debug		1


#
# DaysTillReckoning - The number of days of data kept in the database
#   DaysTillReckoning is the number of days (double value) data is allowed
#   to live in the database.  Data older than DaysTillReckoning will
#   be deleted by the Reaper.  If reaper is run at 2002/02/22 12:49:09
#   and DaysTillReckoning is set to 6.5, then all data prior to 
#   2002/02/16 00:49:09 will be deleted by the reaper.
DaysTillReckoning     290.5
