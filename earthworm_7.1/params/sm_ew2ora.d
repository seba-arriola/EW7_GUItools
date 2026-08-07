#
#                     Configuration File for SM_ew2ora
#
MyModId            MOD_SM_EW2ORA
InRing             HYPO_RING       # Transport ring to find sm messages on,
HeartBeatInterval  15              # Heartbeat interval, in seconds,

LogFile            1               # 0 = log to stderr/stdout only
                                   # 1 = log to disk and stderr/stdout
                                   # 2 = log to disk only

LogStrongMotion    0               # OPTIONAL, 0 = don't log SM msgs
                                   #     nonzero = log SM msgs

Debug	           0               # OPTIONAL, 0 = no debug
                                   #           1 = basic debug
                                   #           2 = super debug


# Specify logos of the messages to grab from the InRing.
# TYPE_STRONGMOTION is assumed, therefore only module ID and 
# installation ID need to be specified
#---------------------------------------------------------
GetMsgsFrom    INST_WILDCARD MOD_WILDCARD  # TYPE_STRONGMOTIONII (assumed)


# Database entry configuration
#----------------------------
DBuser          db_user
DBpassword      db_pwd
DBservice       db_service
