
# configuration file for db_cleanup2


#
# Database connection parameters
#
#
DBuser     ewdb_main
DBpassword Password
DBservice  eqs_default.usgs


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
# OutDir: top level directory for Archived(SAC) files.
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



#
# Debug - optional debug flag
#
#Debug		1


###########################################################
###################### EVENT DATA #########################
#
# Enter rules for Event Data, the columns for each rule are
#EVENT  Save    Delete Max Days  Min Days  Max   Min  Lat/Lon     Dubious
#       Event    Data    Old       Old     Mag   Mag   Box         Events
#
#
#
#  Explanations for each column:
#
#    1: EVENT       This column contains the EVENT keyword to signal the
#                   program that this is an EVENT descriptor row. )
#
#    2: Save Event  This column indicates what amount of data is to be
#                   saved for the event.
#                   0 = None
#                   1 = Waveform Only
#                   2 = Parametric Only
#                   7 = All
#
#    3: Delete Data This column indicates what amount of data should be deleted
#                   for the event.
#										0 = None
#										1 = Waveform Only
#										3 = All
#
#    4: Max Days Old
#                   This column indicates the maximum number of days old that
#                   an Event meeting the criteria can be.  (example 99.99). 
#                   0 = No maximum limit
#
#    5: Min Days Old
#                   This column indicates the minimum number of days old that
#                   an Event meeting the criteria can be.  (example 99.99). 
#                   0 = No minimum limit
#
#    6: Max Mag     This column indicates the maximum magnitude (of the preferred
#                   magnitude) that an Event can have.  (example 9.99). 
#                   0 = No maximum limit
#
#    7: Min Mag     This column indicates the minimum magnitude (of the preferred
#                   magnitude) that an Event can have.  (example 9.99). 
#                   0 = No minimum limit
#
#    8: Lat/Lon Box This column indicates a lat/lon box within which the preferred
#                   origin of the event must lie.  The format of the line should
#                   be (MinLat/MinLon/MaxLat/MaxLon).  Example (-120/40/-100/50).
#                   0 = No lat/lon criteria
#
#    9: Dubious Events
#                   This column indicates whether or not the Event is excluded
#                   based on its dubiocity level.  =)
#										0 = Include non-dubious events only!
#										1 = Include dubious events only!
#								   -1 = Include all events.
#
#
#
#EVENT  Save    Delete Max Days  Min Days  Max   Min  Lat/Lon     Dubious
#       Event    Data    Old       Old     Mag   Mag   Box         Events
#
# The following line deletes all dubious events that are atleast 7 days old.
#  It saves no data from them.
EVENT     0        3      0         7       0     0      0            1

# The following line saves all data from all events with a
#  preferred magnitude <= 2.99 that are atleast 9 days old.  It then deletes
#  only the waveforms.
EVENT     7        1      0         9       2.99     0      0           -1

# The following line deletes all data from all events with a
#  preferred magnitude <= 2.99 that are atleast 31 days old.  
EVENT     0        3      0         31      2.99     0      0           -1

# The following line saves and deletes all data from all events that are 
# atleast 31 days old.  
EVENT     7        3      0         31      0     0      0              -1

# The following line deletes all data from all events with the given
# lat long box defined by two points (15,40) and (20,50) that are 
# atleast 15 days old.  
EVENT     0        3      0         31      0     0      15/40/20/50    -1
#
 										    
#
# #SetArchiveFlag will cause Events whose Parametric & Waveform data is
# saved to, have their Archived flag set.  Events with Archived flags set
# to true will NEVER be saved again, even if the rule calls for them to 
# be saved.
#SetArchiveFlag 1


#OTHER_DATA  Save_Data  Delete_Data  Min_Days
#    1: OTHER_DATA  This column contains the OTHER_DATA keyword to signal the
#                   program that this is an OTHER_DATA descriptor row. The
#                   OTHER_DATA column describes what to do with unassociated
#                   data, such as unassociated picks, waveforms, amplitudes,
#                   and strong motion measurements.
#
#    2: SaveData    This column indicates whether or not to save the data.
#                   (Currently Saving unassociated data IS NOT SUPPORTED!)
#                   0 = NO
#                   1 = YES
#
#    3: DeleteData  This column indicates whether or not to delete the data.
#                   (Currently Saving unassociated data IS NOT SUPPORTED!)
#                   0 = NO
#                   1 = YES
#
#
#    4: MinDays
#                   This is the minimum age of unassociated data that will
#                   be handled by the rule. (Example 9.5)
#
#
# The following rule deletes all unassociated data that is atleast 31 days old. 
#OTHER_DATA  Save_Data  Delete_Data  Min_Days
OTHER_DATA       0            1         31
