
#
# ellipse2arc -- convert message of TYPE_H71SUM2K into TYPE_HYP2000ARC
#

#  Basic Earthworm setup:
#
MyModuleId      MOD_ELLIPSE2ARC    # module id for this instance of template 
InRing	    SOLN_RING          # shared memory ring for input
OutRing         HYPO_RING          # shared memory ring for output
HeartBeatInterval  30              # seconds between heartbeats
LogFile            1               # 0 to completely turn off disk log file

# List the message logos to grab from transport ring
#              Installation       Module          Message Types
GetMsgsFrom    INST_ATWC    MOD_WILDCARD         # has to be TYPE_H71SUM2K

#
# OPTIONAL: Debug level
#
Debug    2


