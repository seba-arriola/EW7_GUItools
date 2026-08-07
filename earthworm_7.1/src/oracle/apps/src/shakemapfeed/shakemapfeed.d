# shakemapfeed's parameter file

# Basic Earthworm setup
#----------------------
MyModuleId         MOD_SHAKEMAPFEED # module id for this instance of template 
RingName           HYPO_RING        # shared memory ring for input/output
HeartBeatInterval  30               # seconds between heartbeats
LogFile            1                # 0 = log to screen only
                                    # 1 = log to disk and screen
                                    # 2 = log to disk only
Debug              1                # 0 for no debug messages
                                    # 1 logs some debug messages
                                    # >1 adds more debug stuff

# List the message logos to grab from transport ring -
# shakemapfeed can process three different message types:
#  GetEventsFrom     command is required (at least one).
#  GetMagnitudesFrom command is optional.
#  GetEQDeletesFrom  command is optional.
#                 Installation    Module     # Message Types 
#--------------------------------------------------------------
GetEventsFrom     INST_WILDCARD MOD_WILDCARD # TYPE_HYP2000ARC (hardcoded)
GetMagnitudesFrom INST_WILDCARD MOD_WILDCARD # TYPE_MAGNITUDE  (hardcoded)  
GetEQDeletesFrom  INST_WILDCARD MOD_WILDCARD # TYPE_EQDELETE   (hardcoded)  

# Set criteria for creating files for shakemap
#---------------------------------------------
MagThresh         3.5               # Minimum magnitude to feed shakemap
MagWeightThresh   2.0               # Minimum magnitude weight (~no. of stas)

TempDir          /home/tmp          # Full path of working directory.
OutputDir        /home/shakemap     # Full path of directory in which to 
                                    #   write completed shakemap files.
ShakemapFormat    2                 # Specify format of output files
                                    #   1=shakemap1, 2=shakemap2(XML)
UseEventID        1                 # Specify which eventid to use in output
                                    #   1=Binder id, 2=DBMS id
LogShakemapFile   1                 # non-zero causes shakemap file to be
                                    #   written to shakemapfeeder's log
QuakeAuthor       014024003         # MsgLogo of original TYPE_HYP2000ARC
                                    #   msg, written as a 9-char ascii string
                                    #   msgtype-moduleid-installationid.
                                    #   Used with binder's eventid to look up
                                    #   the DBMS id of the event.
MappingFile       NCSN.map          # File contain mappings of instrument type,
                                    #   long station names and agency names
                                    #   MappingFile is optional.

# Set schedule for automatically generating new shakemap files.
# Use multiple ScheduleFeed commands (up to 20) to set auto-feed times
# in minutes after the event first reached shakemapfeed.
# Shakemapfeed will continue auto-feeds at the last ScheduleFeed interval
# until UpdateDuration minutes after the event origin time.
#   ScheduleFeed   <minutes after receiving event>        
#   UpdateDuration <minutes after event origin time>
#-------------------------------------------------------------------------
ScheduleFeed      0.25  # 15 sec  after receiving event msg
ScheduleFeed      5     #  5 min   "
ScheduleFeed     15     # 15 min   "
ScheduleFeed     60     #  1 hour  "

UpdateDuration  480     #  8 hour after event origin time

# Set criteria for requesting strongmotion data from the DBMS
#------------------------------------------------------------
DBservice         db_service        # which instance DBMS to connect to
DBuser            db_user           # as this user name
DBpassword        db_pswd           # with this password

MaxDataPerEvent   1000              # Maximum number of channels of SM data
                                    #   to retrieve from DBMS for each event.

SMQueryMethod     1                 # 0: do 2-stage query (default behavior).
                                    #   Get all SM data associated with this event, 
                                    #   then all unassociated SM data in space/time
                                    #   range. Will never see SM data associated
                                    #   with other events.
                                    # non-zero: query with space-time range only. 
                                    #   Gets SM data assoc with this event,
                                    #   assoc w/other events and UNassoc data.
                                    #   Post query filtering disqualifies SM data
                                    #   assoc with other local events, but allows
                                    #   SM data assoc with foreign eventids.

LatitudeRange     5.0               # request data within Y degrees lat of eq.
LongitudeRange    6.0               # request data within X degrees lon of eq.

# Add list of components to block (optional):
#--------------------------------------------
BlockComponent VHZ
BlockComponent HV1

# Set up time tolerance in seconds (how sloppy are timestamps?)
#--------------------------------------------------------------
DefaultTimeTolerance     1.0   # tolerance for unlisted networks
NetTimeTolerance     PG 30.0    
NetTimeTolerance     NP 30.0
NetTimeTolerance     CE 50.0

# Specify a simple velocity model ala binder_ew
# One "lay depth-to-top P-velocity" command per layer
#----------------------------------------------------
lay  0.0 6.5
lay 25.0 8.0
psratio 1.78

# List page groups to notify when a shakemap file is generated (optional).
# One group per command, up to 10 PageOnShakemapFeed commands.
#-------------------------------------------------------------------------
PageOnShakemapFeed jack_seismologist
PageOnShakemapFeed jill_seismologist

