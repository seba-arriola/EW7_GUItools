#
# Configuration file for ora2sactarfile
#
#
#

# Logfiledir: where the log file is written
Logfiledir      ../log/

#
# BinDir: Directory where the SAC_create_tarfile lives
#
BinDir      /home/earthworm/web/bin

#
# SACDataDir: where sac data goes
#
#  This must be under web/html in order for files 
#  to be downloaded using HTTP protocol.
#  If you don't want this, i.e. you want to use anonymous ftp,
#  you are on your own.
#
SACDataDir      /home/earthworm/web/html/pub/SACdata

#
# DB related stuff
#
@../params/db_connect_opts.d

# Optional
#Debug

#
# FTP related stuff
#

# FTPHost: proper name of the local anon ftp site
#   e.g.:  gldylan.cr.usgs.gov
FTPHost gldzappa.cr.usgs.gov

# FTPDir: Path to the top level directory where the tar files go
#   e.g.:  /pub/SACdata
#     note: the yyyymm is created under this directory by the 
#           SAC_create_tarfile script. 
FTPDir /earthworm/pub/SACdata

#
# SAC writing related parameters
#
#
# Sizes of trace memory. Determines how much memory we'll try to grab
# Max number of traces we'll ever see in one event
#
MaxTraces 500

#
# TraceBufferLen:
#    largest trace snippet we'll ever have to deal with (kb)
#
TraceBufferLen 72000

#
# Specify on what platform the output files will be used:
# intel or sparc - with this information, files will be written out
# in the correct byte order.
#
OutputFormat sparc

