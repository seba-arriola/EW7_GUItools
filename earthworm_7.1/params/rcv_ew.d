#
# rcv_ew configuration file
#
# RCV under Earthworm is started by startstop, and given the command line 
#     found in startstop.d.  That's where RCV get's its customary command 
#     line arguments. 
# This file is read by the "user_proc" routines, which determine RCV's 
#     identity to Earthworm.
#
 MyModuleId       MOD_RCV_EW   # module id for this export,
 RingName         WAVE_RING    # transport ring to use for input/output,
 HeartBeatInt     30           # Heartbeat interval in seconds (Earthworm internal)
 LogFile          1            # If 0, don't write logfile at all,
                               # If 2, write to module log but not stderr/stdout
 MaxSamplePerMsg  1000          # #data samples in largest message we'll ever create
# MaxSamplePerMsg  120          # #data samples in largest message we'll ever create
 PacketLatency    0            # #packets to buffer before shipping (to handle rollbacks)
 Debug    # uncomment to write debug messages

# Monitor the time since the last packet received for each sta/comp/net.
#  if MaxSilence  > 0, rcv_ew will issue a message to statmgr when it has not
#                      seen a packet for MaxSilence minutes.  If it has issued
#                      such a message, it will also issue another message when it
#                      starts receiving data for the SCN again.
#  if MaxSilence <= 0, rcv_ew will not monitor the time since last packet.
#
 MaxSilence  120    # number of minutes to wait before complaining that
                    # no new data is being received for a given SCN

# List each sta/comp/net that you expect from Golden in a "AcceptSCN" command.
#     If rcv_ew sees an SCN which is not listed here, it will be ignored.
#     On each line after the SCN, list a pinnumber to use for this SCN.

#          site comp net pinno
#          ---- ---- --- -----
AcceptSCN  HWUT BHZ  US  6007
AcceptSCN  HWUT BHN  US  6008
AcceptSCN  HWUT BHE  US  6009
AcceptSCN  HWUT LHZ  US  6010
AcceptSCN  HWUT LHN  US  6011
AcceptSCN  HWUT LHE  US  6012

AcceptSCN  WVOR BHZ  US  6013
AcceptSCN  WVOR BHN  US  6014
AcceptSCN  WVOR BHE  US  6015
AcceptSCN  WVOR LHZ  US  6016
AcceptSCN  WVOR LHN  US  6017
AcceptSCN  WVOR LHE  US  6018

AcceptSCN  ALQ  BHZ  US  6001
AcceptSCN  ALQ  BHN  US  6002
AcceptSCN  ALQ  BHE  US  6003
AcceptSCN  ALQ  LHZ  US  6004
AcceptSCN  ALQ  LHN  US  6005
AcceptSCN  ALQ  LHE  US  6006

