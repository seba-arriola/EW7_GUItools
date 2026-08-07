#
#                    CONFIGURATION FILE FOR PSNADSEND
#                    -----------------------------
ModuleId         MOD_PSNADSEND_A	# Module id of this instance of PsnAdSend
OutRing          WAVE_RING      	# Transport ring to write waveforms to
CommPort         1              	# Comm Port Number
PortSpeed        38400          	# Comm Port Baud Rate
HeartbeatInt     15             	# Heartbeat interval in seconds
#
#                        CHANNEL CONFIGURATION
Nchan		 8			# specify number of channels that will be recorded
ChanRate         200            	# Sampling rate in samples/second
#
#                         TIMING INFORMATION
TimeoutNoSend    30
TimeoutNoSynch   60
UpdateSysClock   0              	# 1 = PC time updated with AD board time
HighToLowPPS     0              	# 1 = PPS Signal direction is High to Low
NoPPSLedStatus   0			# 1 = Disable 1PPS LED blinking
LogMessages      1              	# 1 = Log messages from DLL & ADC to log file
TimeOffset       0              	# Time Reference offset in milliseconds
AdcDataSize      2                      # ADC Data trace buffer size. Can be 2 or 4 bytes
#
# Time Reference Types:
# 0 = Use PC Time, 1 = Garmin GPS 16 or 18, 2 = Motorola ONCORE NMEA, 
# 3 = Motorola ONCORE Binary, 4 = WWV (must have WWV option on the ADC board )
#
TimeRefType      1			# One of the types above
TimeFilePath     c:\tmp\		# where to place the time.dat file
#
#               SCN AND PIN VALUES FOR EACH DAQ CHANNEL
# Chan lines must follow the Channel Configuration lines in this file.
# Unused channels may be omitted from the list.  Pin numbers are optional.
# If a pin number is not specified for a channel, the pin number is set to
# the DAQ channel number.
#
#     Chan  Station/Comp/Network/Location
#   -------   --------------
Chan    0     CH00 VHZ NC
Chan    1     CH01 VHZ NC
Chan    2     CH02 VHZ NC
Chan    3     CH03 VHZ NC
Chan    4     CH04 VHZ NC
Chan    5     CH05 VHZ NC
Chan    6     CH06 VHZ NC
Chan    7     CH07 VHZ NC
Chan    8     CH08 VHZ NC
