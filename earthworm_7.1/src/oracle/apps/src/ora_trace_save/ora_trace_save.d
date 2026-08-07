#

# Configuration file for ora_trace_save:
# I listen to trigger messages (TYPE_TRIGLIST), and stuff the implied pieces
# of trace data into an Oracle server. I do this by going to any number
# of WaveServerV's, as listed in this configuration file.


LogFile 1		# 0 means don't create a disc log file. 1=> do.
MyModuleId
RingName
HeartBeatInt


# What message logos to listen to. Can be more than one.
# The type is hard coded to TYPE_TRIGLIST

GetEventsFrom	INST_MENLO	MOD_WILDCARD


# Data Base Server Identification. Not for the security squeamish.
DBservice     db_service
DBpassword    db_pwd
DBuser        db_user

# The WaveServers we'll interrogate:
TimeoutSeconds 30	# If a WaveServer doesn't talk to us in this 
			# many seconds, we'll abort that request
TravelTimeout  30	# We'll wait at most this long for the WaveServers
			# to get the time interval we want. Recall that we
			# might be asking for trace data containing arrivals
			# before the waves got to the sensors.

# list of ip addresses and ports of the  WaveServers we're to use
WaveServer 136.177.20.72 16022	# gldzappa 	

# Sizes of trace memory. Determines how much memory we'll try to grab
MaxTraces 100	# Max number of traces we'll ever see in one event
TraceBufferLen 500 	# largest trace snippet we'll ever have to deal with
			# in bytes/1000.





# Debug switch: the token "Debug" (without the quotes) can be stated.
# If it is, lots of weird debug messages will be produced 
Debug
