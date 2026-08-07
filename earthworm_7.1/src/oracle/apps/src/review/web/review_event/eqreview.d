
#
#  Configuration file for eqreview
#

#
# Database connection parameters
#
DBuser          db_user
DBpassword      db_pwd
DBservice       db_service

Debug
Logfiledir      ../log/

#
# hostname of the machine holding up web pages
#
WebHost		webhost.somewhere.domain

#
# ReviewSource: One or more entries (max 10) denoting the 
#   author (Source) of the Origins which are allowed to review.
#
#
ReviewSource	014024003		# Northern California
ReviewSource	USNSN			# NSN
ReviewSource	DEWEY			# Dewey type messages

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

#
# Full path to the hypoinverse program
#
PathToHypoBin		/home/earthworm/working/bin/hyp2000


#
# Full path to the hypoinverse directory
#
HypoinverseDir		/home/earthworm/web/params/hypoinverse

#
# Name of the hypoinverse configuration file
#
HypoConfig		ncal2000.hyp


#
# Name of the file containing Lomax javascript. 
# NOTE: It must exist under ReviewDir/lomax
#
#
JavascriptFile		lomax_new.js

#
# Output format (platform): sparc or intel
#
SacFormat		sparc


