      FUNCTION KLASS (NET,Y,X,Z)
C--ASSIGNS AN INTEGER CODE TO A HYPOCENTER BASED ON LOCATION & DEPTH.
C
C--ARGUMENTS:
C  NET=NETWORK NUMBER
C       NET=1 FOR HAWAII
C  Y=LAT IN DEG (POS NORTH)
C  X=LON IN DEG (POS EAST, NEG WEST)
C  Z=DEPTH IN KM
C
C----------------------------------------------------------
C************* NETWORK 1 = HAWAII *****************
C--ALL EARTHQUAKES ARE IN ONE OF THE FOLLOWING GROUPS,
C--IDENTIFIED BY A NUMERICAL CLASS OR 3-LETTER CODE:
C
C--SHALLOW:
C 1  SNC - SHALLOW NORTH CALDERA (0-5 KM)
C 2  SSC - SHALLOW SOUTH CALDERA (0-5 KM)
C 3  SEC - SHALLOW EAST CALDERA (0-5 KM)
C 4  SER - SHALLOW EAST RIFT (0-5 KM)
C 5  SME - SHALLOW MIDDLE EAST RIFT (0-5 KM)
C 6  KOA - KOAE FAULT ZONE (0-5 KM)
C 7  SSF - SHALLOW SOUTH FLANK (0-5 KM)
C 8  SLE - SHALLOW LOWER EAST RIFT (0-5 KM)
C
C--INTERMEDIATE DEPTH:
C 9  SF1 - KILAUEA SOUTH FLANK (5-13 KM) (WEST END)
C 10 SF2 - KILAUEA SOUTH FLANK (5-13 KM)
C 11 SF3 - KILAUEA SOUTH FLANK (5-13 KM)
C 12 SF4 - KILAUEA SOUTH FLANK (5-13 KM)
C 13 SF5 - KILAUEA SOUTH FLANK (5-13 KM) (EAST END)
C 14 LER - LOWER EAST RIFT (5-99 KM)
C 15 MLO - MAUNA LOA (0-13 KM)
C 16 LSW - LOWER SW RIFTS OF KILAUEA & MAUNA LOA (0-13 KM)
C 17 GLN - GLENWOOD (0-13 KM)
C 18 SWR - SW RIFT (0-13 KM)
C 19 INT - INTERMEDIATE CALDERA (5-13 KM)
C 20 KAO - KAOIKI (0-13 KM)
C
C--DEEP:
C 21 DEP - DEEP KILAUEA (>13 KM) (BELOW REGIONS 1-13,17-19)
C 22 DLS - DEEP LOWER SW RIFT (>13 KM) (BELOW REGION 16)
C 23 DML - DEEP MAUNA LOA (>13 KM) (BELOW REGIONS 15,20)
C
C--OUTER REGIONS, ALL DEPTHS:
C 24 LOI - LOIHI (ALL DEPTHS)
C 25 KON - SOUTH KONA (ALL DEPTHS)
C 26 HUA - HUALALAI (ALL DEPTHS)
C 27 KOH - KOHALA (ALL DEPTHS)
C 28 KEA - MAUNA KEA (ALL DEPTHS)
C 29 HIL - HILO (ALL DEPTHS)
C 30 DIS - DISTANT, EVERYWHERE ELSE
C
C---------------------------------------------------------
C--THE LATITUDE AND LONGITUDE LIMITS OF THE REGIONS ARE GIVEN BELOW.
C--WHEN THE COORDINATES IMPLY AN OVERLAP, PRECEDENCE IS GIVEN AS IN THE MAPS.
C
C NO. CODE      N.LAT.            S.LAT.            W.LON.            E.LON.
C  1  SNC      19 28            19 24.5          155 19            155 14
C  2  SSC      19 24.5          19 22            155 19            155 16.5
C  3  SEC      19 24.5          19 22            155 16.5          155 14
C  4  SER      19 26            19 20.5          155 14            155 07.2
C  5  SME      19 26            -----            155 07.2          155 00
C  6  KOA      19 22            19 20.5          155 17            155 14
C  7  SSF      -----            19 10            155 17            155 00
C  8  SLE      19 32            19 16            155 00            154 40
C  9  SF1      19 22            19 10            155 17            155 14.5
C 10  SF2      19 26            19 10            155 14.5          155 12.3
C 11  SF3      19 26            19 10            155 12.3          155 09.1
C 12  SF4      19 26            19 10            155 09.1          155 05.3
C 13  SF5      19 26            19 10            155 05.3          155 00
C 14  LER      19 32            19 16            155 00            154 40
C 15  MLO      19 43            19 19            155 35            155 19
C 16  LSW      19 19            18 40            155 43            155 25
C 17  GLN      19 43            19 26            155 19            155 00
C 18  SWR      19 22            19 10            155 25            155 17
C 19  INT      19 28            19 22            155 19            155 14
C 20  KAO      19 30            19 19            155 32            155 19
C 21  DEP      19 43            19 10            155 25            155 00
C 22  DLS      19 19            18 40            155 43            155 25
C 23  DML      19 43            19 19            155 35            155 19
C 24  LOI      19 10            18 40            155 25            155 00
C 25  KON      19 39            19 00            156 20            155 43
C 26  HUA      19 55            19 39            156 20            155 43
C 27  KOH      20 25            19 55            156 20            155 34
C 28  KEA      20 25            19 43            155 43            154 40
C 29  HIL      19 47            19 32            155 09            154 40
C---------------------------------------------------
C
C********** HAWAII ************
C
      IF (Y.GT.19.467 .OR. Y.LT.19.367 .OR. X.GT.-155.233
     2 .OR. X.LT.-155.317) GO TO 10
C--KILAUEA CALDERA
C--DEP
      KLASS=21
      IF (Z.GT.13.) RETURN
C--INT
      KLASS=19
      IF (Z.GT.5.) RETURN
C--SNC
      KLASS=1
      IF (Y.GT.19.409) RETURN
C--SSC
      KLASS=2
      IF (X.LT.-155.275) RETURN
C--SEC
      KLASS=3
      RETURN
C----------------------------------------------------------
10    IF (Y.GT.19.433 .OR. Y.LT.19.167 .OR. X.GT.-155.
     2 .OR. X.LT.-155.283) GO TO 30
C--EAST RIFT OR SOUTH FLANK
C--DEP
      KLASS=21
      IF (Z.GT.13.) RETURN
      IF (Z.GT.5.) GO TO 20
C--SHALLOW EAST RIFT
C--SSF
      KLASS=7
      IF (Y.LT.19.342 .OR. Y.LT.70.1+.3271*X) RETURN
C--KOA
      KLASS=6
      IF (X.LT.-155.233) RETURN
C--SER
      KLASS=4
      IF (X.LT.-155.12) RETURN
C--SME
      KLASS=5
      RETURN
C------------------------------------------------------
C--SOUTH FLANK
C--SF1
20    KLASS=9
      IF (X.LT.-155.242) RETURN
C--SF2
      KLASS=10
      IF (X.LT.-155.205) RETURN
C--SF3
      KLASS=11
      IF (X.LT.-155.152) RETURN
C--SF4
      KLASS=12
      IF (X.LT.-155.088) RETURN
C--SF5
      KLASS=13
      RETURN
C-----------------------------------------------------
C--OUTER REGIONS WITH DEPTH DISCRIMINATION
C
30    IF (X.LT.-155.417 .OR. X.GT.-155.283 .OR. Y.GT.19.367
     2 .OR. Y.LT.19.167) GO TO 35
C--SWR
      KLASS=18
      IF (Z.LT.13.) RETURN
C--DEP
      KLASS=21
      RETURN
C
35    IF (Y.GT.19.5 .OR. Y.LT.19.317 .OR. X.GT.-155.317
     2 .OR. X.LT.-155.533) GO TO 40
C--KAO
      KLASS=20
      IF (Z.LT.13.) RETURN
C--DML
      KLASS=23
      RETURN
C
40    IF (Y.GT.19.533 .OR. Y.LT.19.267 .OR. X.GT.-154.667
     2 .OR. X.LT.-155.) GO TO 45
C--SLE
      KLASS=8
      IF (Z.LT.5.) RETURN
C--LER
      KLASS=14
      RETURN
C
45    IF (Y.GT.19.317 .OR. Y.LT.18.667 .OR. X.GT.-155.417
     2 .OR. X.LT.-155.717) GO TO 50
C--LSW
      KLASS=16
      IF (Z.LT.13.) RETURN
C--DLS
      KLASS=22
      RETURN
C
50    IF (X.LT.-155.717 .OR. X.GT.-155.317 .OR. Y.LT.19.317
     2 .OR. Y.GT.19.583) GO TO 55
C--MLO
      KLASS=15
      IF (Z.LT.13.) RETURN
C--DML
      KLASS=23
      RETURN
C---------------------------------------------------------
C--TESTS FOR OUTER REGIONS INCLUDING ALL DEPTHS
C--KON
55    KLASS=25
      IF (Y.LT.19.650 .AND. Y.GT.19. .AND. X.GT.-156.333
     2 .AND. X.LT.-155.717) RETURN
C--HUA
      KLASS=26
      IF (Y.LT.19.917 .AND. Y.GT.19.650 .AND. X.GT.-156.333
     2 .AND. X.LT.-155.717) RETURN
C--KOH
      KLASS=27
      IF (Y.LT.20.417 .AND. Y.GT.19.917 .AND. X.GT.-156.333
     2 .AND. X.LT.-155.567) RETURN
C--LOI
      KLASS=24
      IF (Y.LT.19.167 .AND. Y.GT.18.667 .AND. X.GT.-155.417
     2 .AND. X.LT.-155.) RETURN
C--HIL
      KLASS=29
      IF (Y.LT.19.783 .AND. Y.GT.19.533 .AND. X.GT.-155.15
     2 .AND. X.LT.-154.667) RETURN
C
      IF (Y.GT.19.583 .OR. Y.LT.19.43 .OR. X.LT.-155.317
     2 .OR. X.GT.-155.) GO TO 60
C--GLN
      KLASS=17
      IF (Z.LT.13.) RETURN
C--DEP
      KLASS=21
      RETURN
C--KEA
60    KLASS=28
      IF (Y.LT.20.417 .AND. Y.GT.19.583 .AND. X.GT.-155.717
     2 .AND. X.LT.-154.667) RETURN
C--DIS, EVERYTHING ELSE
      KLASS=30
      RETURN
      END
