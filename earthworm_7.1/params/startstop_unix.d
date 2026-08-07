#
#       Startstop (Unix Version -- Linux) Configuration File
#
#    <nRing> is the number of transport rings to create.
#    <Ring> specifies the name of a ring followed by it's size
#    in kilobytes, eg        Ring    WAVE_RING 1024
#    The maximum size of a ring is 1024 kilobytes.
#    Ring names are listed in file earthworm.h.
#
  nRing              7 
  Ring   WAVE_RING 2048
  Ring   WAVE_RING_LP 1024
  Ring   INPUT_RING 1024
  Ring   HYPO_RING 256
  Ring   ALARM_RING 256
  Ring   PICK_RING 256
  Ring    SCNL_RING  1024
#
 MyModuleId    MOD_STARTSTOP  # Module Id for this program
 HeartbeatInt  50             # Heartbeat interval in seconds
 MyClassName   TS             # For this program
 MyPriority     0             # For this program
 LogFile        1             # 1=write a log file to disk, 0=don't
 KillDelay      30            # seconds to wait before killing modules on
                              #  shutdown
# statmgrDelay		2     # Uncomment to specify the number of seconds
					# to wait after starting statmgr 
					# default is 1 second

#
#    Class must be RT or TS
#    RT priorities from 0 to 59
#    TS priorities le 0
#
#    If the command string required to start a process contains
#       embedded blanks, it must be enclosed in double-quotes.
#    Processes may be disabled by commenting them out.
#    To comment out a line, preceed the line by #.
#
#
#
#Process          "export_generic export_generic.d"
#
#
Process          "slink2ew slink2ew_20.d"
Class/Priority    OTHER 0
#
Process          "slink2ew slink2ew.d"
Class/Priority    OTHER 0
#
Process          "decimate dec_2to1_in_wave.d"
Class/Priority    OTHER 0
#
Process          "scnl2scn scnl2scn_20.d"
Class/Priority    OTHER 0
