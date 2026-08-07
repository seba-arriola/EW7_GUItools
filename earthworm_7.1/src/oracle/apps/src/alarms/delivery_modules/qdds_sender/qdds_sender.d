
#
#  Configuration file for qdds_sender
#

#  Basic Earthworm setup:
#
MyModName          MOD_QDDS_SENDER   
RingName           HYPO_RING      
LogFile            1             
HeartBeatInt       30             

# List the message logos to grab from transport ring
#              Installation       Module          Message Types
GetAlarmsFrom  INST_WILDCARD    MOD_WILDCARD      # must be TYPE_QDDS_MSG

#
# Database connection parameters
#
DBservice       eqsg.usgs
DBuser          ewdb_main
DBpassword      main

#
# Option Debug flag
#
Debug	1
