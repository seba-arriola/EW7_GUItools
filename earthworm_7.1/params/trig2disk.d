

#
# Configuration file for trig2disk:
# I listen to trigger messages (TYPE_TRIGLIST), and stuff the implied pieces
# of trace data into an Oracle server. I do this by going to any number
# of WaveServerV's, as listed in this configuration file.
#


MyModuleId                 MOD_TRIG2DISK
RingName                   HYPO_RING
HeartBeatInt               30
LogFile                    1   # 0 means don't create a disc log file. 1=> do.
                               # 2 means log to disk file but not stderr/stdout


#
# What message logos to listen to. Can be more than one.
# The type is hard coded to TYPE_TRIGLIST
#
GetEventsFrom	           INST_MENLO  MOD_WILDCARD

#
# list of ip addresses and ports of the WaveServers we're to use
#
WaveServer                 111.222.33.444     16022	

#
# If a WaveServer doesn't talk to us in this 
# many seconds, we'll abort that request
#
TimeoutSeconds             30  

#
# Max number of traces we'll ever see in one event
#
MaxTraces                  500	

#
# kilobytes of the largest trace snippet we'll ever have to deal with
#
TraceBufferLen             1000  	

#
# Debug switch:  Debug will be logged if uncommented 
#
Debug

#
# SCNL list of stations to write for each trigger message,  these get
# written in addition to SCNLs in the trigger message.
#
#@memphis.scnl

#
# Minimum number of seconds to save for extra channels that we
# are saving (i.e., those channels not in the TYPE_TRIGLIST message)
#
MinDuration                 30

#
# number of sample periods after which we declare a gap
#
GapThresh 100

# Optional queue commands
# Number of trigger messages to hold in queue; default is 10
#QueueSize 10

# Optional queue dumpfile name, for saving state of queue
QueueFile trig2disk.que

# Optional delay time: trig2disk will wait this many seconds from the
# time it receives a trigger message until it starts to process the message
DelayTime 5


#
# Select format of output data and the directory where it is written
# Only one of the following pairs should be uncommented.
#
# SUDS
#
#DataFormat                  suds
#OutDir                      "/home/earthworm/SUDS"
#
# SAC
#
DataFormat                   sac
OutDir                       "c:\earthworm\SAC"
#
# AH
#
#DataFormat                  ah
#OutDir                     "/home/earthworm/AH"
#
# SEISAN
#
#DataFormat                  seisan
#OutDir                     "/home/earthworm/seisan"
#
# GSE
#
#DataFormat                  gse_int
#OutDir                     "/home/earthworm/gse"
#
# Tankplayer
#
#DataFormat                 tank
#OutDir                     "./tanks/"
#
# Mini-SEED
# Mini-SEED output format is currently only available on Solaris
#
#DataFormat                 mseed
#OutDir                     "/earthworm/data/mseed"

# PSN4
#
# NOTE PSN4 output format is only avialable for WINDOWS!
#  a station.lst file must also exist in the EW_PARAMS directory too (see example in docs)
#
#DataFormat                 psn4
#OutDir                     "c:\earthworm\psn4"



#
# Specify on what platform the output files will be used:
# intel or sparc - with this information, files will be written out
# in the correct byte order.
#
OutputFormat                sparc


