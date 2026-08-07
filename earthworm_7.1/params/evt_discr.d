
#
# evt_discr -- pick up hypoinverse message and figure out which ones
#  to pass on
#

#  Basic Earthworm setup:
#
MyModuleId      MOD_EVT_DISCR      # module id for this instance of template 
InRing	    SOLN_RING          # shared memory ring for input
OutRing         HYPO_RING          # shared memory ring for output
HeartBeatInterval  30                 # seconds between heartbeats
LogFile            1                  # 0 to completely turn off disk log file

# List the message logos to grab from transport ring
#              Installation       Module          Message Types
GetMsgsFrom    INST_WILDCARD    MOD_WILDCARD     # has to be TYPE_HYP2000ARC     

#
# OPTIONAL: Debug level
#
Debug    2

#
# Database connection configuration.
#
DBuser          db_user
DBpassword      db_pwd
DBservice       db_service

#
# Tolerances for discriminating among events
#

# Origin Time (secs) 
OtimeTol	30.0

# Latitude (in degrees)
LatTol		0.10

# Longitude (in degrees)
LonTol		0.10

# Depth (in km)
DepthTol	5.0

# Magnitude 
MagTol		0.1

