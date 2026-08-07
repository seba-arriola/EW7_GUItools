
#
#  Configuration file for the review system
#

#
# Database connection parameters
#
@../params/db_connect_opts.d

#Debug
Logfiledir      ../log/

#
# NetworkCode: Two letter designation of this seismic network to 
#  be used in CUBE messages
#
NetworkCode UU

#
# hostname of the machine holding up web pages
#
@../params/web_host_opts.d

#
# ReviewSource: One or more entries (max 10) denoting the
#   author (Source) of the Origins which are allowed to review.
#
#
ReviewSource    014024021               # Montana
ReviewSource    014024005               # Utah



#
# Directory where temporary review files are written. This is 
# where Arc files, as well as SAC files for each arrival are
# temporarily stored
#
# NOTE: this directory must be under the web's earthworm directory
#  because this is where the review applet needs to find the 
#  SAC files
#
ReviewDir           /home/earthworm/web/html/review

#
# WebDir: Web mapping of ReviewDir
#
WebDir				/earthworm/review


####################################
# HYPOINVERSE REVIEW SECTION
####################################

#
# Full path to the hypoinverse program
#
PathToHypoBin		/home/earthworm/working/bin/hyp2000


#
# Full path to the hypoinverse directory
#
HypoinverseDir		/home/earthworm/web/params/hyp2000

#
# Name of the hypoinverse configuration file
#
HypoConfig		utah.hyp


####################################
# LOCALMAG REVIEW SECTION
####################################

#
# LM_progname: full path to the executable which computes
#  local magnitudes (e.g., Pete's localmag)
#
LM_progname /home/earthworm/working/bin/localmag

#
# LM_writeWA_configfile: full path to the configuration file 
#  for the LM_progname executable which causes it to produce
#  SAC files with Wood-Anderson traces.
#
#
LM_writeWA_configfile /home/earthworm/web/params/localmag_writeWA.d

#
# LM_review_configfile: full path to the configuration file 
#  for the LM_progname executable which causes it to 
#  recompute the local magnitude given the potentially reviewed
#  information in the WA sac headers.
#
LM_review_configfile /home/earthworm/web/params/localmag_review.d

#
# LM_outputfile: name of the output file of the LM_progname
#  executable. For localmag, this option must be set to the  
#  value of the filename for the outputFormat option. 
#
LM_outputfile LocalMag.output

#
# LM_method: method used for computing the ML magnitude.
#  Valid methods are Peak2Peak and Zero2Peak. Zero2Peak method 
#  uses one amplitude pick from zero to peak. Peak2Peak method
#  uses two picks denoting the largest plus-to-minus or
#  minus-to-plus swing in the sliding window whose length
#  is determined by the value of slideLength in the 
#  LM_progname's configuration file.
#
#  The choice of method is installation-specific, but it must 
#  be consistent over various modules dealing with local
#  magnitude calculations. If LM_configfiles use slideLength
#  greater than 0 then then method should be set to Peak2Peak.
LM_method Peak2Peak

#
# MakeLMPreferred:  If set, the local magnitude computed, 
#  if any, will be set as preferred. Otherwise, the coda
#  magnitude is preferred
MakeLMPreferred

#
#
#



####################################
# SEISGRAM2K APPLET SETUP SECTION
####################################

#
# Name of the file containing Lomax javascript. 
# NOTE: It must exist under ReviewDir/lomax
#
#
JavascriptFile		LomaxApplet.js

#
# OPTIONAL: DisplayChannels
#   How many of the closest channels will be displayed
#   by the Lomax applet.  The number will be hard-coded
#   in the source code if this option is not specified.
#
DisplayChannels     20


#
# Output format (platform): sparc or intel
#
SacFormat		sparc


####################################
# ALARMS SECTION
####################################

#
# EwParams -- directory where earthworm configuration lives. This is
#  needed to acesss earthworm.d and its mappings between names and Ids.
#
EwParams        /home/earthworm/runUtah-alarms/params

#
# AlarmRing: name of the ring where alarm processors are waiting
#
AlarmRing   ALARM_RING

#
# MyModuleID: name of the module issuing alarms
#
MyModuleID  MOD_ALARM

#
# MyInstID: Installation name
#
MyInstID    INST_UTAH




###################################################
# Command:   WaveformLinks
#
# Description:
#           Specifies links that are created at the bottom
#           of the eqparams page, to link to record section
#           displays(waveforms).
#
# Params:
#     PROGRAM_NAME
#                    specifies the program name that the link will
#                    attempt to execute
#
#     DESCRIPTION
#                    the text of the link that will show up on the
#                    eqparams page
#
#     OPEN_IN_NEW_WINDOW
#                    a flag {1=TRUE | 0=FALSE} that determines whether the
#                    link is opened in a new window.  Specifically it specifies
#                    whether an html TARGET field is specified for the link.
#
#     WINDOW_NAME (only if OPEN_IN_NEW_WINDOW=1)
#                    Name of the targe window where the link will be opened.
###################################################
#WaveformLinks PROGRAM_NAME  DESCRIPTION    OPEN_IN_NEW_WINDOW  WINDOW_NAME
WaveformLinks "ora2rsec_gif" "60 seconds aligned on pick time" 1 "60 secs"
WaveformLinks "ora2rsec_gif_two" "200 seconds aligned on pick time" 1 "200 secs"
#WaveformLinks "ora2snippet_gif" "All Snippets" 1 "All snippets"

