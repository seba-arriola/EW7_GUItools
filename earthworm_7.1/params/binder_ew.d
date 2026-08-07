# This is binder's parameter file!
#---------------------------------
 MyModuleId   MOD_BINDER_EW # Module id for this instance of binder

 RingName     PICK_RING     # Public transport ring for I/O

 BufferRing   BINDER_RING   # Private ring for buffering incoming picks.
                            #  This ring's name must be defined in earthworm.d,
                            #  but must not be listed in startstop's config file
                            #  (default=BINDER_RING).

 HeartbeatInt 30            # Seconds between heartbeats

 LogFile      1             # 0 = turn off logging
                            # 1 = log to disk and stderr/stdout 
                            # 2 = log to disk but not to stderr/stdout

# List the message logos to grab from transport ring
#              Installation       Module      # Message Type
#-------------------------------------------------------------
 GetPicksFrom  INST_WILDCARD    MOD_WILDCARD  # TYPE_PICK_SCNL

# Set level of output for writing log file
#-----------------------------------------
#log_stack            # comment out to turn of grid-stack logging
 hypcode    4         # Sets a value to specify how much information should be 
                      # included in binder's log file after each event 
                      # relocation. Possible values 0-7.
                        	
# Load station list
#------------------
 maxsite 1800
 site_file "calsta.hinv"

# Load crustal model 
# Refer to file containing "lay" commands, or list them here
#-----------------------------------------------------------
#@ncal_model.d        # contents of ncal_model.d follow:
 lay   0.0     4.0
 lay   3.5     5.9
 lay   15.0    6.85
 lay   25.0    7.85

# Set FIFO lengths
#-----------------
 pick_fifo_length  1000   # optional: default=1000
 quake_fifo_length  100   # optional: default=100

# Define association grid, set stacking parameters.
#--------------------------------------------------
 dspace     4.0
 grdlat    32.5    43.0
 grdlon  -125.0  -114.5
 grdz       0.0    20.0
 rstack   100.0
 tstack     0.65          # truncates to nearest 10th sec
 stack     50
 thresh    18
 focus     30
 grid_wt 0  4
 grid_wt 1  3
 grid_wt 2  2
 grid_wt 3  1

# Set parameters for associating picks with active hypocenters
#-------------------------------------------------------------
 rAvg_Factor 7.0
 taper   0.0     1.0
 taper   50.0    1.0
 taper   200.0   2.5
 taper   400.0   4.5
# bfh: 11/5/94: "variable taper" proportional to Origin Time Uncertainty:
# Set both to 0.0 to get Carl's original distance taper only.
 taper_OT 2.0    1.0
 t_dif   -3.0  120.

# Set parameters for locating events
#-----------------------------------
 wt 0 1.0
 wt 1 0.5
 wt 2 0.25
 wt 3 0.125
 ph P  1.0
 ph Pn 0.5
 ph Pg 0.5
 r_max  400.0
 zrange 1.0 30.0
 MaxStep 25.0  5.0
 MinXYZstep 0.1
 MaxIter 4
 MaxDeltaRms 1.0001
 locate_eq  25   2
 locate_eq  50   4
 locate_eq  75   8
 locate_eq 100  16

# Set parameters for assessing groups of picks
# with Bill Ellsworth's resampling technique:
#----------------------------------------------
 assess_pk  8  12
 maxtrial   500
 maxwt      3
 v_halfspace  5.0
 residual_cut  5.0  10.0
#log_accepted

# Load in the next valid quake sequence number
#---------------------------------------------
 EventIdFile quake_id.d

# THE END
