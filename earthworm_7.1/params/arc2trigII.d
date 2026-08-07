# This is arc2trigII's parameter file
# Derived from, you guessed it, arc2trig. The enhancements are to save snippets 
# of interest which were not listed in the arc message. This is done as follows:
#   * Get all the stations - either from hypoinverse station list or DBMS. For each station:
#     * Check the table of magnitude vs. radius. If it's within the radius, save this snippet.
#     * Check the list of mandatory stations to save (wildcards permitted). If its on the list, save.
# Saving is done as follows:
#   * Within a specified epicentral distance, save all data from origin time through S time.
#   * Outside this distance, use the travel-time table as in usnsn_loc2trig, and save from P to S.
#   * All snippets to be padded with a specified safety margin of pre and post times.

#  Basic Earthworm setup:
#
MyModuleId        MOD_ARC2TRIG   # module id for this instance of arc2trig 
InRingName           HYPO_RING   # shared memory ring for input
OutRingName          TRIG_RING   # shared memory ring for output
LogFile              1           # 0 to turn off disk log file; 1 to turn it on
                                 # 2 to log to module log but not to stderr/stdout
HeartBeatInterval    30          # seconds between heartbeats

# DBMS parameters:
# If any of these are present, we'll get the station list from the DBMS.
 DBservice        xyz.frq
 DBpassword       abc
 DBuser           def
 DBtimeoutSeconds 30   # If the DB doesn't talk to us in this many seconds, we'll abort
# Lat-Lon limits we'll use for constructing station list:
 DBLatRange   20.1   50.2
 DBLonRange -120.2 -80.1 

# Name of hypoinverse station list.
# If this is present, we'll get station list from there.
# StaListFileName calsta.hinv.qa

# Optional: Magnitude vs Distance (km) within which we'll get all snippets:
# How it works:  it finds the lowest magnitude entry which is greater 
# than the event magnitude and uses that radius for determinatin of stations. 
# For events with magnitudes largest than the largest in the Table, 
# arc2trigII used the radius of largest magnitude in the table.
MagDist 3.0 10
MagDist 3.5 10
MagDist 4.0 10
MagDist 5.0 10

# Within this distance (km), save from from origin time through S.
SaveOriginDist 100

# File name of  travel time table. As produced by /grab_bag/makeTTTable/cal_tt
@TravelTimeTable

# Optional: Mandatory channels that must be included for all events.
# List one SCN per line, as many as you need. Wildcards (*) permitted.
# Channel  * * UW 
# Channel  LON LHZ UW
 

# List the message logos to grab from transport ring
#              Installation       Module          Message Types
GetEventsFrom  INST_WILDCARD    MOD_WILDCARD    # hyp2000arc - no choice.

# Set up output directory and prefix for trigger files.
# Daily files will be written with a suffix of .trg_yyyymmdd
# Set either to "none" or "NONE" to stop writing disk files
OutputDir  c:\earthworm\run_arc2trigII\log # directory to write trigger files in
BaseName   arc2trigII          # prefix of trigger file name
                             # suffix will be .trg_yyyymmdd


# Optional parameters:
# Additional pre- and post- safety times to be added to calculated snippet times.
# PreTime 15.0	# seconds of pre-p (or origin) to save (default=15.)
# PostTime 10.0	# seconds of post-S data to save (default=10.).

Debug
