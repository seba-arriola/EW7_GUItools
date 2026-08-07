
# configuration file for archiver


#
# Database connection parameters
#
#
DBuser     p3_main
DBpassword main
DBservice  sm.usgs


#
# Logfiledir - where log files are written
#
Logfiledir /tmp/log/

#
# Debug - optional debug flag
#
Debug		1

#
# This program is meant to be run out of cron to clean
# up old events. The configuration options below allows
# for this period cleanup by specifying the start date 
# with respect to the date of the cleanup, and the number
# of days to clean up.
#
# Further configuration options allow the user to specify 
# the type of cleanup desired. The user can delete waveform
# data only or both parametric and waveform data. The user 
# can also save the waveform data in SAC format.
#

#
# StartDate: determines the start date of cleanup. This
#  should be specified as the number of days from now 
#  when we should start cleaning up. For example, a 
#  StartDate of 7 means that we will start by deleting events
#  which are 7 days old
#
StartDate		700


#
# NumberOfDays: determines how many days worth of 
#  events will be deleted.
#
NumberOfDays	700


#
# DeleteTraceOnly: determines what is deleted. 
#  0 -- Delete both parametric and waveform data
#  1 -- Delete snippets only 
#
DeleteTraceOnly		0

#
# Save: determines whether snippets are saved in SAC format
#  0 -- Do not save snippets
#  1 -- Save snippets 
#
Save		0


#
# If Save option is set to 1, the following parameters are 
#  required to configure the SAC writing mechanism. If Save 
#  is set to 0, the values of the parameters below are ignored (but,
#  they are still required to be in here).

#
# OutDir: top level directory for SAC files.
#
#
OutDir   /home/lucky/SACdir

#
# Specify on what platform the output files will be used:
# intel or sparc - with this information, files will be written out
# in the correct byte order.
#
OutputFormat sparc

#
# TraceBufferLen:
#    largest trace snippet we'll ever have to deal with (kb)
#
TraceBufferLen 72000


