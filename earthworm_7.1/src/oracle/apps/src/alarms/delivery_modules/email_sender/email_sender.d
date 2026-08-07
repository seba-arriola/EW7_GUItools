
#
#  Configuration file for email_sender
#

#  Basic Earthworm setup:
#
MyModName          MOD_EMAIL_SENDER   
RingName           HYPO_RING      
LogFile            1             
HeartBeatInt       15             

# List the message logos to grab from transport ring
#              Installation       Module          Message Types
GetAlarmsFrom  INST_WILDCARD    MOD_WILDCARD      # must be TYPE_EMAIL_MSG

#
# Database connection parameters
#
DBservice       eqsg.usgs
DBuser          ewdb_main
DBpassword      main


#
# Optional subject line, otherwise subject will be the 
# database ID of the event triggering the alarm
#
Subject "Earthworm Alarm"

#
# Option Debug flag
#
Debug	1
