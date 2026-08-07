#  
#
#                    earthworm_global.d
 
#      !!!!   DO NOT make any changes to this file.  !!!!

#  The contents of this file are under the control of Earthworm Central.
#       Please direct any requested additions or changes to:
#
#             Barbara Bogaert    bogaert@usgs.gov

#  The values defined in earthworm_global.d are critical to proper 
#  communication and data attribution among Earthworm systems around 
#  the globe. 

#  A copy of earthworm_global.d should be placed in your EW_PARAMS directory.
#
#  The master copy of earthworm_global.d resides in the vX.XX/environment
#  directory of this Earthworm distribution.



#--------------------------------------------------------------------
#                        Installation IDs
#
#   The character string <-> numerical value mapping for all
#   installations are assigned and controlled by the Earthworm 
#   Development Team at the USGS in Golden, CO.  These mappings
#   must remain globally unique in order for exchanged data 
#   to be properly attributed to the installation that created it.
#
#   0-255 are the only valid installation ids.
#
# The maximum length of installation string is 32 characters.
#
#--------------------------------------------------------------------

 Installation   INST_WILDCARD    0   # Sacred wildcard value - DO NOT CHANGE!!!
 Installation   INST_FAIRBANKS   1 
 Installation   INST_UW          2 
 Installation   INST_MENLO       3 
 Installation   INST_CIT         4 
 Installation   INST_UTAH        5 
 Installation   INST_MEMPHIS     6 
 Installation   INST_UNR         7 
 Installation   INST_UCB         8 
 Installation   INST_PTWC        9 
 Installation   INST_IDA        10 
 Installation   INST_NC         11 
 Installation   INST_VT         12 
 Installation   INST_USNSN      13 
 Installation   INST_RICKS      14 
 Installation   INST_HVO        15 
 Installation   INST_ATWC       16 
 Installation   INST_PGE        17 
 Installation   INST_PNNL       18 
 Installation   INST_PGC        19 
 Installation   INST_AVO        20 
 Installation   INST_BUTTE      21 
 Installation   INST_LAMONT     22 
 Installation   INST_SC_CHA     23 
 Installation   INST_STLOUIS    24 
 Installation   INST_NMT        25
 Installation   INST_CVO 	26
 Installation   INST_SC_COL     27
 Installation   INST_PRSN       28
 Installation   INST_TEMP1      29
 Installation   INST_SOAPP      30
 Installation   INST_UO         31
 Installation   INST_USBR       32
 Installation   INST_UTIG       33
 Installation   INST_PSU        34
 Installation   INST_INEL       35
 Installation   INST_SISMALP    36
 Installation   INST_MIT        37
 Installation   INST_GSC        38
 Installation   INST_IRISDMC    39
 Installation   INST_BOZEMAN    40
 Installation   INST_UTK        41
 Installation   INST_CDWR       42
 Installation   INST_WO         43
 Installation   INST_NAZ        44
 Installation   INST_MVO        45
 Installation   INST_LASN       46
 Installation   INST_NAU        47
 Installation   INST_KO         48
 Installation   INST_DIGITEXX   49
 Installation   INST_DTA        50
 Installation   INST_STACHELTIER 51
 Installation   INST_CWB        52
 Installation   INST_EDI        53
 Installation   INST_MI         54
 Installation   INST_URBAN-HZ   55 
 Installation   INST_EREBUS     56 
 Installation   INST_SGS        57 
 Installation   INST_UNAVCO     58 
 Installation   INST_USGSMAG    59 
 Installation   INST_KACST	60	# King Abdulaziz City for Science & Technology
 Installation   INST_OVMP       61	# Ob. Vol. de la Montagne Pelee
 Installation   INST_COSO       62	#  Keith Richards-Dinger Coso Geothermalm NAWS
 Installation   INST_CGS        63	#  California Geological Survey
 Installation   INST_GEYSERS    64	#  Lawrence Berkeley Lab - Geysers Network
 Installation   INST_GSL        65	#  Guralp Systems Limited
 Installation   INST_UKY        66	#  University of Kentucky
 Installation   INST_POLARIS    67      #  POLARIS - Canada geophysical research
 Installation   INST_NISN       68      #  Norther Idaho Seismic Network- Ken Sprenke


 Installation   INST_QUITO      100
 Installation   INST_INETER     101
 Installation   INST_CENAPRED   102
 Installation   INST_RABAUL     103
 Installation   INST_COLIMA     104
 Installation   INST_SNET       105
 Installation   INST_OVSICORI   106	# Observatorio Vulcanologia y Sismologia de Costa Rica 
 Installation   INST_PCCHIAPAS  107	# Protección Civil de Chiapas 

 Installation   INST_RAINIERAFM 150

 Installation   INST_UNKNOWN    255



#--------------------------------------------------------------------------
#                          Message Types
#
#  Define all message name/message-type pairs that will be global
#  to all Earthworm systems.
#
#  VALID numbers are:
#
#   0- 99 Message types 0-99 are defined in the file earthworm_global.d.
#         These numbers are reserved by Earthworm Central to label types 
#         of messages which may be exchanged between installations. These 
#         string/value mappings must be global to all Earthworm systems 
#         in order for exchanged messages to be properly interpreted.
#         
#
#  OFF-LIMITS numbers:
#
# 100-255 Message types 100-255 are defined in each installation's  
#         earthworm.d file, under the control of each Earthworm 
#         installation. These values should be used to label messages
#         which remain internal to an Earthworm system or installation.
# 
#         
# The maximum length of the type string is 32 characters.
#             
#--------------------------------------------------------------------------

# Global Earthworm message-type mappings (0-99):
 Message  TYPE_WILDCARD        0  # wildcard value - DO NOT CHANGE!!!   
 Message  TYPE_ADBUF           1  # multiplexed waveforms from DOS adsend
 Message  TYPE_ERROR           2  # error message
 Message  TYPE_HEARTBEAT       3  # heartbeat message
 Message  TYPE_NANOBUF         5  # single-channel waveforms from nanometrics
 Message  TYPE_PICK2K         10  # P-wave arrival time (with 4 digit year) 
                                  #   from pick_ew 
 Message  TYPE_CODA2K         11  # coda info (plus station code) from pick_ew 
#Message  TYPE_PICK2          12  # P-wave arrival time from picker & pick_ew
#Message  TYPE_CODA2          13  # coda info from picker & pick_ew
 Message  TYPE_HYP2000ARC     14  # hyp2000 (Y2K hypoinverse) event archive  
                                  #   msg from eqproc/eqprelim
 Message  TYPE_H71SUM2K       15  # hypo71-format hypocenter summary msg 
                                  #   (with 4-digit year) from eqproc/eqprelim
#Message  TYPE_HINVARC        17  # hypoinverse event archive msg from 
                                  #   eqproc/eqprelim
#Message  TYPE_H71SUM         18  # hypo71-format summary msg from
                                  #   eqproc/eqprelim
 Message  TYPE_TRACEBUF       20  # single-channel waveforms from NT adsend, 
                                  #   getdst2, nano2trace, rcv_ew, import_ida...
 Message  TYPE_LPTRIG         21  # single-channel long-period trigger from 
                                  #   lptrig & evanstrig
 Message  TYPE_CUBIC          22  # cubic-format summary msg from cubic_msg
 Message  TYPE_CARLSTATRIG    23  # single-channel trigger from carlstatrig
#Message  TYPE_TRIGLIST       24  # trigger-list msg (used by tracesave modules)
                                  #   from arc2trig, trg_assoc, carlsubtrig
 Message  TYPE_TRIGLIST2K     25  # trigger-list msg (with 4-digit year) used 
                                  #   by tracesave modules from arc2trig, 
                                  #   trg_assoc, carlsubtrig
 Message  TYPE_TRACE_COMP_UA  26  # compressed waveforms from compress_UA
 Message  TYPE_STRONGMOTION   27  # single-instrument peak accel, peak velocity,
                                  #   peak displacement, spectral acceleration 
 Message  TYPE_MAGNITUDE      28  # event magnitude: summary plus station info
 Message  TYPE_STRONGMOTIONII 29  # event strong motion parameters
 Message  TYPE_LOC_GLOBAL     30  # Global location message used by NEIC & localmag



#      !!!!   DO NOT make any changes to this file.  !!!!
