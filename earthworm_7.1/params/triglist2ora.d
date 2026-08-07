#

# Configuration file for triglist2ora:
# I listen to triglist messages (TYPE_TRIGLIST2K), and stuff the contents
# into an Oracle server. 
# 

LogFile 			1
MyModuleId			MOD_TRIGLIST2ORA
RingName			NSN_RING
HeartBeatInt		10


# What message logos to listen to. Can be more than one.
# The type is hard coded to TYPE_TRIGLIST2K

GetEventsFrom	INST_USNSN	MOD_WILDCARD


# Data Base Server Identification. Not for the security squeamish.
DBuser        ewdb_main
DBpassword    main
DBservice     eqs.working


# Debug switch: the token "Debug" (without the quotes) can be stated.
# If it is, lots of weird debug messages will be produced 
Debug



#
# Wave server timeout (in seconds):
#    Abort, if a WaveServer doesn't talk to us in this many seconds.
#
TimeoutSeconds 30   

#
# list of ip addresses and ports of the  WaveServers we're to use
#
WaveServer     130.118.43.11  17000     # Mammoth
WaveServer     130.118.43.34  16031     # Menlo
WaveServer     130.118.43.34  16032     # Menlo


#
# largest trace snippet we are prepared to deal with (in KBytes)
#
TraceBufferLen 500




