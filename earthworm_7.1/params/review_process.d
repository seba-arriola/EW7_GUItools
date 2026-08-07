 
# ora2rsec_gif.d sample config file


################## REQUIRED PARAMETERS ####################
# Logfiledir sets the directory the program uses for logging.
Logfiledir ../log/

# DBservice sets the service name of the Oracle database from which 
#  the program will retrieve information.
DBservice db_service

# DBuser sets the user name that the program will use to connect
#  to the Oracle database identified by DBservice
DBuser db_user

# DBpassword sets the password the program uses in conjunction with
#  the DBuser login
DBpassword db_pwd

################ END REQUIRED PARAMETERS ##################
################## OPTIONAL PARAMETERS ####################

# LogFile tells the program whether it should log to a file.
#  Default: YES.  1=YES, 0=NO
#LogFile 0

# MyModuleId sets the programs moduleId, used to identify
#  earthworm programs, and related to logfile names.
#  Default: 1.  All Natural int's should be valid, but
#  some numbers are reserved!?!
#MyModuleId 1

# DisplayWidth sets the total width of the wiggles image(GIF)
#  that the program draws, in pixels.
#  Default: 480.  This includes room for the displayed trace,
#  the header for each trace, and the border area.
#DisplayWidth 480

# HeaderWidth sets the portion of DisplayWidth, allocated
#  for header information including S,C,N, first motion,
#  distance from origin, scale factor, and maximum amplitude.
#  Default: 60.  Acceptable Range: 0 - DisplayWidth.
#HeaderWidth 60

# TraceHeight sets the number of vertical pixels allocated
#  for each trace.  The smaller the number, the more squished
#  the traces will look, but the more traces you will be able
#  to view on the screen without scrolling.
#  Default: 100.  Acceptable Range: 0 - 2000.
TraceHeight 64

# DisplayTime sets the length in seconds of the wiggle display
#  after the alignment.  If alignment is on OT, then this is
#  the number of seconds of trace data that will be displayed
#  after OT.  This does not included any trace displayed prior
#  to the alignment point(OT in the example).
#  Default: 30 secs.  Acceptable range: integral values between 
#  1 - 2,000,000,000+.  Note:  the USEFUL!! display time is limited
#  to the time range of trace data stored in the database.
DisplayTime 60

# ScaleFactor sets the amount of magnification applied to ALL traces.
#  With a scale factor of 1, the traces are displayed, such that the
#  vertical amplitude of the trace relative to the vertical space 
#  allocated for the trace is equal to the relation between the trace
#  values and the maximum possible values of trace with that datatype.
#  Example:  For a given piece of trace, the max value is 3000, and
#  the min value is -3000.  The trace is of datatype 'i2', meaning 
#  2-byte data, so the vertical distance in pixels between the min
#  and the max will be (1500 - (-1500)) * TraceHeight/(64k -1). If
#  TraceHeight is 100, then the distance would be 4.577 or 4 pixels.
#  Since you probably want to utilize more than 4 out of 100 pixels,
#  we invented ScaleFactor, which allows you to magnify the trace
#  relative to the range for its datatype.  So if ScaleFactor is 20,
#  max becomes 30000, and min becomes -30000, and thus 80 of 100
#  pixels are used.  ScaleFactor sets the magnification of ALL traces.
#  Because some traces require diffent magnification than others,
#  (low gain, and DST stations don't mix well with high gain stations)
#  we invented, or atleast implemented autoscaling.  By setting 
#  ScaleFactor to 0, the program will attempt to determine the "BEST"
#  scale for each station.
#  Default: 10.  Range:  Integral values in 0 - (2^32 -1).
ScaleFactor 0

# MaxPlots sets the maximum number of traces that will be plotted onto
# the wiggle image(GIF).  The number that will actually be displayed
# is the smaller of: the number retrieved from the database or MaxPlots.
# Default:  20
#MaxPlots 20

# NumOfTicks, sets the number of time tick marks, used as a
# time reference in each trace display.  NumOfTicks sets the
# number of tick marks displayed after the alignment time.
# Tick marks prior to the alignment time will be proportional
# to those after the alignment time.  See "DisplayTime" and
# "PercentOfTimeBeforePick" for more information.  We
# recommended that you use a multiple of 4 for NumOfTicks.
# Default 16
#NumOfTicks 16

# WiggleAlign sets the alignment method for the traces.  The
#  possible alignment methods are:
#   ALIGN_WIGGLE_ON_OT 1
#     Align the traces on the Origin Time.
#
#   ALIGN_WIGGLE_ON_ARRIVAL 2
#     Align the traces on the Arrival(Pick) Time for each trace.
#
#   ALIGN_WIGGLE_ON_8_KM_PER_SEC 3
#     Align the traces on Origin Time + X seconds, where:
#        X = DistanceFromOriginToTraceStation(km) / 8.0(km/sec) 
#             for each trace.
#
#   ALIGN_WIGGLE_ON_6_KM_PER_SEC 4
#     Align the traces on Origin Time + X seconds, where:
#        X = DistanceFromOriginToTraceStation(km) / 6.0(km/sec) 
#             for each trace.
#
#  Default: 1  (OT).  Acceptable Range, integral values 1 - 4.
#WiggleAlign 1

# PreAlgnPcntg is complicated to explain.  It sets
#  the amount of time before the alignment time to
#  be displayed, relative to WiggleTime.  If the traces
#  are aligned on OT(WiggleAlign=1), and WiggleTime=30 secs,
#  then if PreAlgnPcntg = 10, 33 seconds of data will be shown;
#  3 seconds (10% of 30) before OT, and 30 seconds (WiggleTime)
#  after OT.  The percentage set by PreAlgnPcntg is relative to
#  WiggleTime, and it does not replace any of the post align
#  data time, but instead gets added onto it.  
#  Example, if you wanted to see 30 seconds of data total for
#  each trace, aligned on the arrival time, and you wanted 3 secs
#  of each trace to be prior to the arrival time, then you would
#  make the following settings:
#    WiggleTime    27
#    WiggleAlign    2
#    PreAlgnPcntg  12
#   such that the traces would be aligned on the arrival times,
#   there would be 27 seconds of post arrival data (displayed),
#   and there would be 3.24 ~= 3 sec of pre arrival data.
#  Default 10 %.  Acceptable range:  integral values between 0 - (2^31 -1)
#PreAlgnPcntg 10

################ END OPTIONAL PARAMETERS ##################
