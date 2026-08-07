#
# This is the spectrogram parameter file. This module gets data gulps
# from the waveserver(s), and computes spectrograms

#  Basic Earthworm setup:
#
LogSwitch     1           # 0 to turn off disk log file; 1 to turn it on;
                          # 2 to log to module log but not to stderr/stdout
MyModuleId    MOD_SGRAM   # module id for this instance of report 
RingName      HYPO_RING   # ring to get input from
HeartBeatInt  15          # seconds between heartbeats

wsTimeout 40 # time limit (secs) for any one interaction with a wave server.

# List of wave servers (ip port comment) to contact to retrieve trace data.
 WaveServer aaa.bbb.dd.c1  16022      "wsv  - image1"
 WaveServer aaa.bbb.dd.c2  16022      "wsv  - image2"
#WaveServer aaa.bbb.dd.c   16025      "wsv1 - Nano"

# List of target computers/directories to place output.
#       UserName     IP Address            Directory
 Target luetgert@ehzmenlo.wr.usgs.gov:/webd1/ehzweb/waveforms/sgram/ 
 Target luetgert@ehznorth.wr.usgs.gov:/webd1/ehzweb/waveforms/sgram/ 
 Target luetgert@ehzsouth.wr.usgs.gov:/webd1/ncweb/waveforms/sgram/ 
 Target luetgert@ehznorth.wr.usgs.gov:/webd1/ncweb/waveforms/sgram/ 

# Filename prefix on target computer.  This is useful for identifying
# files for automated deletion via crontab.
 Prefix nc

# Directory in which to store the temporary .gif, .link and .html files.
 GifDir   /home/luetgert/gifs/sgram/ 

# Plot Parameters - sorry it's so complex, but that's the price of versatility
        # The following is designed such that each SCN creates it's own
        # spectrogram display; one per day of data.
# S  Site
# C  Component
# N  Network
# 04 Hours/Plot      Number of hours in each GIF image.
# 05 Plot Previous   On startup, retrieve and plot n previous hours from tank.
# 06 Local Time Diff UTC - Local.  e.g. -7 -> PDT; -8 -> PST
# 07 Local Time Zone Three character time zone name.
# 08 Show UTC        UTC will be shown on one of the time axes.
# 09 Use Local       The day page will be local day rather than UTC day.
# 10 XSize           Size of plot in inches. Setting this > 100 will imply pixels
# 11 Pixels/Line     Vertical pixels per line of trace displayed
# 12 Minutes/Line    Number of minutes per line of trace displayed
# 13 Seconds/Gulp    Number of seconds per fft
# 14 Freq Max        Maximum frequency
# 15 Freq Mute       Hz to mute at low end
# 16 nbw             Display scaling. 1:linear, 2:log 3:log(1st diff) [neg for greyscale]
# 17 amax            Clipping amplitude applied to spectrum [0 for none]
# 18 scale           Scale factor for wiggle-line display.
# 19 Comment         A comment for the top of the display.
#                                      
#     S    C   N  04 05 06  07  08 09 10   11  12 13 14 15    16  17        18  Comment
#                   

 Plot MCM  VHZ NC 24 2  -7  PDT 1  0  600  1   1  60 10 0.2   2   200000    2.0 "Convict Moraine"
 Plot MCS  VHZ NC 24 2  -7  PDT 1  0  350  1   1  60 10 0.0   2   150000    2.0 "Casa Diablo Hot Springs"
 Plot MDP  VHZ NC 24 2  -7  PDT 1  0  350  1   1  60 10 0.0   2   150000    2.0 "Devil's Postpile"
 Plot MDP  DLI NC 24 2  -7  PDT 1  0  350  1   1  60 10 0.0   2   150000    2.0 "Devil's Postpile"
 Plot MEM  VHZ NC 24 2  -7  PDT 1  0  600  1   1  60 10 0.2   2   200000    2.0 "East Mammoth"
 Plot MSL  VHZ NC 24 2  -7  PDT 1  0  600  1   1  60 25 0.2   2   150000    2.0 "Sherwin Lakes"
 Plot MLC  VHZ NC 24 2  -7  PDT 1  0  600  1   1  60 10 0.2   2   150000    2.0 "Laurel Creek Canyon"
 Plot MPR  VHZ NC 24 2  -7  PDT 1  0  600  1   1  60 10 0.2   2   125000    2.0 "Pilot Ridge"
 Plot MCV  VHZ NC 24 2  -7  PDT 1  0  600  1   1  60 10 0.2   2   125000    2.0 "Convict Lake"
 Plot MGP  VHZ NC 24 2  -7  PDT 1  0  300  1   1  60 10 0.2   2   150000    2.0 "Gravel Pit"
 Plot JSF  VDZ NC 24 2  -7  PDT 1  0  600  1   1  60 25 0.0   2    2000    2.0 "Bay Area DST"
#Plot JSG  VDZ NC 24 2  -7  PDT 1  0  600  1   1  60 25 0.0   2   150000    2.0 "Bay Area Nano"

#Plot NPT  V   HV 24 1  -10 HST 1  0  600  1   1  60 10 0.0   2   30000    "North Pit Hawaii"
#Plot AHU  V   HV 24 1  -10 HST 1  0  600  1   1  60 10 0.0   2   30000    "Hawaii"

#Plot SEP  EHZ UW 24 1  -7  PDT 1  0  600  1   1  60 10 0.2   2   30000    "Mt. St. Helens"
#Plot RCM  EHZ UW 24 1  -7  PDT 1  0  600  1   1  60 10 0.0   2   30000    "Mt. Rainier"
#Plot NSM  VDZ NC 24 1  -7  PDT 1  0  600  1   1  60 25 0.0   1   0        "Bay Area Nano"
#Plot CMW1 ADZ NC 24 1  -7  PDT 1  0  600  1   1  60 25 0.0   1   0        "Bay Area Nano"
#Plot CGP1 VDN NC 24 1  -7  PDT 1  0  600  1   1  60 25 0.0   1   0        "Bay Area Nano"
#Plot CMW1 VDN NC 24 1  -7  PDT 1  0  600  1   1  60 25 0.0   1   0        "Bay Area Nano"
#Plot HSF  VHZ NC 24 1  -7  PDT 1  0  600  1   1  60 25 0.0   1   0    "San Juan Bautista"
#Plot GPM  VHZ NC 24 1  -7  PDT 1  0  600  1   1  60 25 0.0   2   0    "Geysers"
#Plot KRP  VHZ NC 24 1  -7  PDT 1  0  600  1   1  60 25 0.0   1   0    "Cape Mendocino"
#Plot RCF  EHZ UW 24 1  -7  PDT 1  0  600  1   1  60 25 0.0   1   0    "Mt. Rainier"
#Plot HBO  EHZ UW 24 1  -7  PDT 1  0  600  1   1  60 25 0.0   1   0    "Somewhere in Washington"
#Plot BIB  EHZ CN 24 1  -7  PDT 1  0  600  1   1  60 25 0.0   1   0    "Bowen Island, Canada"


    # *** Optional Commands ***

 Days2Save     8      # Number of days to display on web page; default=7
 UpdateInt     8      # Number of minutes between updates; default=2
 RetryCount    1      # Number of attempts to get a trace from server; default=10
 Logo    smusgs.gif   # Name of logo in GifDir to be plotted on each image

# We accept a command "SaveDrifts" which logs maximum values to GIF directory.
# SaveDrifts

# We accept a command "Make_HTML" to construct and ship index.html file.
# Make_HTML     

# We accept a command "BuildOnRestart" to totally rebuild images on restart.
# BuildOnRestart     

# We accept a command "Debug" which turns on a bunch of log messages
# Debug
# WSDebug
