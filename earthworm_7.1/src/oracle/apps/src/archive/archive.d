
# configuration file for archiver


#
# Database connection parameters
#
#
DBuser     db_user
DBpassword db_pwd
DBservice  db_service


#
# Logfiledir - where log files are written
#
Logfiledir ./log/

#
# Debug (OPTIONAL)
#
Debug    1


#
# Sizes of trace memory. Determines how much memory we'll try to grab
# Max number of traces we'll ever see in one event
#
MaxTraces 10

#
# TraceBufferLen:
#    largest trace snippet we'll ever have to deal with (kb)
#
TraceBufferLen 72000

#
# OutDir: top level directory for sac files. 
#
#
OutDir   ./SACdir

#
# Specify on what platform the output files will be used:
# intel or sparc - with this information, files will be written out
# in the correct byte order.
#
OutputFormat sparc

