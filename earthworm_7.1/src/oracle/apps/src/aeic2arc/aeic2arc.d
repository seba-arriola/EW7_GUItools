
#
# aeic2arc -- convert message of TYPE_AIEC_LOC into TYPE_HYP2000ARC
#

#  Basic Earthworm setup:
#
MyModuleId      MOD_AEIC2ARC      # module id for this instance of template 
InRing	    	NSN_RING          # shared memory ring for input
OutRing         TRIG_RING          # shared memory ring for output
HeartBeatInterval  30              # seconds between heartbeats
LogFile            1               # 0 to completely turn off disk log file

# List the message logos to grab from transport ring
#              Installation       Module          Message Types
GetMsgsFrom    INST_FAIRBANKS    MOD_WILDCARD     # has to be TYPE_AEIC_LOC

#
# OPTIONAL: Debug level
#
Debug    2


