#
# Config file for getlist
#

@../params/db_connect_opts.d

Logfiledir ../log/

#
# hostname of the machine holding up web pages
#
@../params/web_host_opts.d

# Appearance of web pages
@/home/earthworm/web/params/web_appearance.d

#ShowPickCol
ShowStasCol
ShowTraceCol
#ShowAlarmsCol
#ShowCoincidenceLink



MapPixelWidth 400
MapPixelHeight 400
MaxNumOfStationsDisplayed 5000

Debug
#LogFile 1                   # Turn Logging On/Off
#MyModuleID 0                # Change the ModuleID reported by logit()
#MaxStationDisplayWidth 2.00 #Width in degrees below which we display stations

#DefaultClickEffect 4
# /* Possible values for DefaultClickEffect */
#define P2_CLICK_NOTHING                0
#define P2_CLICK_ZOOM_IN                1
#define P2_CLICK_ZOOM_OUT               2
#define P2_CLICK_VIEW_EVENTS            4

MaxNumOfEventsDisplayed 100  # Maximum number of events that will be displayed on the map
MaxNumOfEventsRetrieved 10000  # Maximum number of events that will be displayed in the list

#NumOfDaysToShow 7           #Number of days in the past from the current time
#                             that should be shown by default

MinNumStasToShow 3         # If present, only events with more than
                            # this many stations will be shown in
                            # the event list

EventsPerPage    10         # If present, Show this many events
                            # on each page, and include links to 
                            # navigate back and forth


DefaultMapID 1.6.6

#Map    Num     Name                Lat1      Lon1  Lat2     Lon2      Riv    Pol  L/L Lines Border  Proj   Focal PtCtr LatCtr Lon
Map     1       World(West)          -100       0    90      360        1     8      0     0    a       Y         10      270
Map     2       World(East)          -100       0    90      360        1     8      0     0    a       Y         10       90
Map     2.1     Africa                -37     -20    40       62        3     8      0     0    m       N
Map     2.2     Asia                   15      55    57      125        3     8      0     0    s       Y         35       90
Map     2.3     Aust/Indo/NZ          -55      90    20      195        3     8      0     0    m       N
Map     1.3     Canada                 41    -142    85      -50        3     8      0     0    s       Y         63      -96
Map     2.4     Europe                 30     -30    72       50        3     8      0     0    m       N
Map     2.5     Japan                  27     120    55      160        3     8      0     0    s       Y         40      140
Map     1.4     Middle_America          6    -118    33      -58        3     8      0     0    m       N
Map     2.6     Middle_East             5      30    50       75        3     8      0     0    m       N
Map     1.5     South_America         -60     -95    15      -25        3     8      0     0    m       N
Map     1.6     USA                    15    -128    60      -64        3     8      0     0    m       N
Map     1.7     Western_Pacific       -25     120    35      210        3     8      0     0    m       N
Map     1.6.1   Montana                41    -117    50     -103        3     8      0     0    m       Y         47.00   -110  
Map     1.6.2   California             45.25 -110    29.75  -132     2051     8      0     0    m       Y         37.50   -122  
Map     1.6.3   Pac_NW	               54.25 -110    38.75  -132     2051     8      0     0    m       Y         46.50   -122  
Map     1.6.4   New_Madrid             40.25  -93    32.75   -85     2051     8      0     0    m       Y         36.50    -89  
Map     1.6.5   Hawaii                 24    -154.50 18     -160.50  2051     8      0     0    m       N           
Map     1.6.6   Utah_Region            35.43 -119.52 46.68  -103.52  2051     8      0     0    m       N                
Map     1.6.6.1   Utah                 36.75 -114.25 42.50  -108.75  2051     8      0     0    m       N           
Map     1.6.6.2   Yellowstone          44    -111.50 45.17  -109.75  2051     8      0     0    m       N           
Map     1.6.7   Alaska                 45    -109    72     -179        3     8      0     0    a       Y         65     -150
Map     2.7     Asia2                   0      40    80      160        3     8      0     0    m       Y         40       100
Map     1.6.2.1 Mammoth                37.85 -119.10 37.35  -118.40  1027     8      0     0    m       Y         37.60   -118.75
Map     1.6.2.2 Geysers                38.55 -123.15 39.05  -122.45  1027     8      0     0    m       Y         38.80   -122.80

#  River Codes  (use the sum of the desired rivers)
# 1 = Permanent major rivers
# 2 = Additional major rivers
# 4 = Additional rivers
# 8 = Minor rivers
# 16 = Double lined rivers
# 32 = Intermittent rivers - major
# 64 = Intermittent rivers - additional
# 128 = Intermittent rivers - minor
# 256 = Major canals
# 512 = Minor canals
# 1024 = All rivers and canals (1-10)
# 2048 = All permanent rivers (1-4)
# 4096 = All intermittent rivers (6-8)
# 8192 = All canals (9-10)


#  Political Border Codes  (use the sum of the desired boundaries)
# 1 = National boundaries
# 2 = State boundaries within the Americas
# 4 = Marine boundaries
# 8 = All boundaries (1-3)

