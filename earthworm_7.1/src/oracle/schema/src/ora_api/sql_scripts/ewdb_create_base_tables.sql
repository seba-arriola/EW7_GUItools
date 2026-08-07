/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */


/*************** IMPORTANT NOTICE ***********************
 *************** IMPORTANT NOTICE ***********************


  This file contains the baseline earthworm schema 
  of version 6.1. We promised that our subsequent 
  schemas will be backward compatible to this. 

  So, this file should NEVER be changed. All modifications
  to the schema should be done either by giving express
  table modification commands, or by creating new tables.

  The way that schemas will be built is:
    - load this file -- create the initial tables
    - load schema modification scripts 
    - load utilities -- views and stored procedures

*************** IMPORTANT NOTICE ***********************
*************** IMPORTANT NOTICE ***********************/



/***********************************************************/
/******************** PARAMETRIC SCHEMA ********************/
/***********************************************************/

/*************************************************/
CREATE TABLE Event (idEvent NUMBER(13,0) NULL, tiEventType NUMBER(13,0) NULL,
  iDubiocity NUMBER(2,0) NULL, bArchived NUMBER(1,0) NULL, 
  idComment NUMBER(13,0) NULL,  
 CONSTRAINT idEvent PRIMARY KEY (idEvent)
);
Create Sequence EventSeq      START WITH 1 MAXVALUE 999999999 CYCLE CACHE 2;

/*************************************************/
CREATE TABLE Origin (idOrigin NUMBER(13,0) NULL, tiExternal NUMBER(13,0) NULL, 
  xidExternal VARCHAR2(16) NULL, idSource NUMBER(13,0) NULL, 
  tOrigin NUMBER(14,4) NULL, dLat NUMBER(10,6) NULL, dLon NUMBER(10,6) NULL, 
  dDepth NUMBER(7,3) NULL, iGap NUMBER(3,0) NULL, dDmin NUMBER(7,2) NULL, 
  dRMS NUMBER(8,4) NULL, iAssocRd NUMBER(9,0) NULL, iAssocPh NUMBER(9,0) NULL, 
  iUsedRd NUMBER(9,0) NULL, iUsedPh NUMBER(9,0) NULL, 
  iE0Azm NUMBER(3,0) NULL, iE0Dip NUMBER(3,0) NULL, dE0 NUMBER(6,2) NULL, 
  iE1Azm NUMBER(3,0) NULL, iE1Dip NUMBER(3,0) NULL, dE1 NUMBER(6,2) NULL, 
  iE2Azm NUMBER(3,0) NULL, iE2Dip NUMBER(3,0) NULL, dE2 NUMBER(6,2) NULL, 
  dErLat NUMBER(7,3) NULL, dErLon NUMBER(7,3) NULL, dErZ NUMBER(7,3) NULL, 
  tMCI NUMBER(14,4) NULL, iFixedDepth NUMBER(1,0) NULL, idComment NUMBER(13,0) NULL,
 CONSTRAINT idOrigin PRIMARY KEY  (idOrigin)
);
Create Sequence OriginSeq     START WITH 1 MAXVALUE 999999999 CYCLE CACHE 2;

/*************************************************/
CREATE TABLE Magnitude (idMag NUMBER(13,0) NOT NULL, 
  idSource NUMBER(13,0) NULL,
  dMagAvg NUMBER(4,3) NULL, iNumMags NUMBER(9,0) NULL, 
  dMagErr NUMBER(4,3) NULL, iMagType NUMBER(4,0),
  idOrigin NUMBER(13,0) NULL, idComment NUMBER(13,0) NULL,
  tiExternal NUMBER(13,0) NULL, xidExternal VARCHAR(16) NULL, 
 CONSTRAINT idMag PRIMARY KEY (idMag),
 CONSTRAINT Magnitude_idOrigin FOREIGN KEY (idOrigin) 
   REFERENCES Origin(idOrigin)
);
Create Sequence MagnitudeSeq  START WITH 1 MAXVALUE 999999999 CYCLE CACHE 2;

/*************************************************/
CREATE TABLE MechFM (idMechFM NUMBER(13,0) NULL, tiExternal NUMBER(13,0) NULL,
  xidExternal VARCHAR(16) NULL, 
  idSource NUMBER(13,0) NULL, idOrigin NUMBER(13,0) NULL,  
  idComment NUMBER(13,0) NULL,
 CONSTRAINT idMechFM PRIMARY KEY (idMechFM),
 CONSTRAINT MechFM_idOrigin FOREIGN KEY (idOrigin) 
   REFERENCES ORIGIN(IDORIGIN)
);
Create Sequence MechFMSeq     START WITH 1 MAXVALUE 999999999 CYCLE CACHE 2;

/*************************************************/
CREATE TABLE Pick (idPick NUMBER(13,0) NULL, tiExternal NUMBER(13,0) NULL, 
  xidExternal VARCHAR(16) NULL, idChan NUMBER(13,0) NULL, 
  sPhase VARCHAR2(6) NULL, tPhase NUMBER(14,4) NULL, cMotion VARCHAR2(1) NULL, 
  cOnset VARCHAR2(1) NULL, dSigma NUMBER(14,4) NULL,  
 CONSTRAINT idPick PRIMARY KEY (idPick)
);
Create Sequence PickSeq       START WITH 1 MAXVALUE 999999999 CYCLE CACHE 10;

/*************************************************/
CREATE TABLE Ray (idRay NUMBER(13,0) NULL, tiExternal NUMBER(13,0) NULL, 
  xidExternal VARCHAR(16) NULL, tOn NUMBER(14,4) NULL, 
  tOff NUMBER(14,4) NULL, dAzm NUMBER(4,1) NULL, 
  dSlow NUMBER(14,4) NULL, idChan1 NUMBER(13,0) NULL,
  idChan2 NUMBER(13,0) NULL, idChan3 NUMBER(13,0) NULL,  
 CONSTRAINT idRay PRIMARY KEY (idRay)
);
Create Sequence RaySeq        START WITH 1 MAXVALUE 999999999 CYCLE CACHE 2;

/*************************************************/
CREATE TABLE TCoda (IDTCODA NUMBER(13,0) NOT NULL, 
  tiExternal NUMBER(13,0) NULL, xidExternal VARCHAR(16) NULL, 
  IDCHAN NUMBER(13,0) NULL, TCODATERMOBS NUMBER(14,4) NULL, 
  TCODATERMXTP NUMBER(14,4) NULL,  
 CONSTRAINT idTCoda PRIMARY KEY (IDTCODA)
);
Create Sequence TCodaSeq      START WITH 1 MAXVALUE 999999999 CYCLE CACHE 10;

/*************************************************/
CREATE TABLE CodaAmp (idCodaAmp NUMBER(13,0) NULL, 
  tiExternal NUMBER(13,0) NULL, xidExternal VARCHAR(16) NULL, 
  idChan NUMBER(13,0) NULL,  idTCoda NUMBER(13,0) NULL,
  tOn NUMBER(14,4) NULL, tOff NUMBER(14,4) NULL, 
  iAvgAmp NUMBER(9,0) NULL,
 CONSTRAINT idCodaAmp PRIMARY KEY (idCodaAmp),
 CONSTRAINT CodaAmp_idTCoda FOREIGN KEY (idTCoda) 
   REFERENCES TCoda(idTCoda)
);  
Create Sequence CodaAmpSeq    START WITH 1 MAXVALUE 999999999 CYCLE CACHE 10;

/*************************************************/
CREATE TABLE MagType (iMagType NUMBER(4,0) NULL, 
  sMagAbbrev VARCHAR(4),
  sMagName VARCHAR(40),
  tiMagType NUMBER(13,0) NULL,
 CONSTRAINT iMagType PRIMARY KEY (iMagType)  
);

/*************************************************/
CREATE TABLE PeakAmp (idPeakAmp NUMBER(13,0) NULL, 
  tiExternal NUMBER(13,0) NULL, xidExternal VARCHAR(16) NULL, 
  idChan NUMBER(13,0) NULL, 
  dPeakAmp1 NUMBER(12,5) NULL, tAmp1 NUMBER(14,4) NULL, 
  tPeriod1 NUMBER(10,4) NULL,
  dPeakAmp2 NUMBER(12,5) NULL, tAmp2 NUMBER(14,4) NULL, 
  tPeriod2 NUMBER(10,4) NULL,
   iMagType NUMBER(4,0) NULL,
 CONSTRAINT idPeakAmp PRIMARY KEY (idPeakAmp)
);
Create Sequence PeakAmpSeq    START WITH 1 MAXVALUE 999999999 CYCLE CACHE 10;

/*************************************************/
CREATE TABLE Source (idSource NUMBER(13,0) NULL, sSource VARCHAR2(50) NOT NULL, 
  sHumanReadable VARCHAR2(100), idComment NUMBER(13,0) NULL,  
 CONSTRAINT Source_sSource UNIQUE  (sSource),
 CONSTRAINT idSource PRIMARY KEY (idSource)
);
Create Sequence SourceSeq     START WITH 1 MAXVALUE 999999999 CYCLE CACHE 10;

/*************************************************/
CREATE TABLE Comments (idComment NUMBER(13,0) NULL, 
  sComment VARCHAR2(4000) NULL,
 CONSTRAINT idComment PRIMARY KEY (idComment)
);
Create Sequence CommentsSeq   START WITH 1 MAXVALUE 999999999 CYCLE CACHE 10;

/*************************************************/
CREATE TABLE MagLink (IDMAGLINK NUMBER(13,0) NOT NULL, 
  IDMAG NUMBER(13,0) NULL, IDDATUM NUMBER(13,0) NULL, 
  DMAG NUMBER(3,2) NULL, DWEIGHT NUMBER(4,1) NULL,  
 CONSTRAINT idMagLink PRIMARY KEY (IDMAGLINK),
 CONSTRAINT MagLink_idMag FOREIGN KEY (IDMAG) 
   REFERENCES MAGNITUDE(IDMAG)
);
Create Sequence MagLinkSeq    START WITH 1 MAXVALUE 999999999 CYCLE CACHE 2;

/*************************************************/
CREATE TABLE MechFMPick (idMechFMPick NUMBER(13,0) NULL, 
  idMechFM NUMBER(13,0) NULL, idPick NUMBER(13,0) NULL,  
 CONSTRAINT MechFMPick PRIMARY KEY (idMechFM, idPick),  
 CONSTRAINT idMechFMPick UNIQUE  (idMechFMPick),
 CONSTRAINT MechFMPick_idPick FOREIGN KEY (idPick) 
  REFERENCES PICK(IDPICK),  
 CONSTRAINT MechFMPick_idMechFM FOREIGN KEY (idMechFM) 
  REFERENCES MECHFM(IDMECHFM)
);
Create Sequence MechFMPickSeq START WITH 1 MAXVALUE 999999999 CYCLE CACHE 2;

/*************************************************/
CREATE TABLE Bind (idBind NUMBER(13,0) NOT NULL, idEvent NUMBER(13,0) NULL, 
  tiCore NUMBER(13,0) NULL, idCore NUMBER(13,0) NULL,
 CONSTRAINT idBind UNIQUE (idBind),
 CONSTRAINT Bind_PK PRIMARY KEY (idEvent,tiCore,idCore),
 CONSTRAINT Bind_idEvent FOREIGN KEY
   (idEvent) REFERENCES EVENT(idEvent)
);
Create Sequence BindSeq       START WITH 1 MAXVALUE 999999999 CYCLE CACHE 2;

/*************************************************/
CREATE TABLE Prefer (idPrefer NUMBER(13,0) NULL, 
  idPrefOrigin NUMBER(13,0) NULL, idPrefMag NUMBER(13,0) NULL, 
  idPrefMech NUMBER(13,0) NULL, idEvent NUMBER(13,0) NULL,
 CONSTRAINT idPrefer UNIQUE  (idPrefer),
 CONSTRAINT Prefer_idEvent PRIMARY KEY (idEvent),
 CONSTRAINT Prefer_idEvent_FK FOREIGN KEY
   (idEvent) REFERENCES EVENT(idEvent),
 CONSTRAINT Prefer_idOrigin FOREIGN KEY
   (idPrefOrigin) REFERENCES ORIGIN(idOrigin),
 CONSTRAINT Prefer_idMag FOREIGN KEY
   (idPrefMag) REFERENCES MAGNITUDE(idMag),
 CONSTRAINT Prefer_idMech FOREIGN KEY
   (idPrefMech) REFERENCES MECHFM(idMechFM)
);
Create Sequence PreferSeq     START WITH 1 MAXVALUE 999999999 CYCLE CACHE 2;

/*************************************************/
CREATE TABLE CodaDur (IDCODADUR NUMBER(13,0) NOT NULL, 
  IDTCODA NUMBER(13,0) NULL, IDPICK NUMBER(13,0) NULL, 
  TCODADUROBS NUMBER(14,4) NULL, TCODADURXTP NUMBER(14,4) NULL,  
 CONSTRAINT CodaDur_idPick FOREIGN KEY (IDPICK)
   REFERENCES PICK(IDPICK),  
 CONSTRAINT CodaDur_idTCoda FOREIGN KEY (IDTCODA) 
   REFERENCES TCODA(IDTCODA),  
 CONSTRAINT idCodaDur UNIQUE  (IDCODADUR),  
 CONSTRAINT CodaDur PRIMARY KEY (IDTCODA, IDPICK)
);
Create Sequence CodaDurSeq    START WITH 1 MAXVALUE 999999999 CYCLE CACHE 2;

/*************************************************/
CREATE TABLE ORIGINPICK (IDORIGINPICK NUMBER(13,0) NOT NULL, 
  IDORIGIN NUMBER(13,0) NULL, IDPICK NUMBER(13,0) NULL, 
  SPHASE VARCHAR2(6) NULL, TPHASE NUMBER(14,4) NULL, DWEIGHT NUMBER(5,2) NULL, 
  DDIST NUMBER(7,2) NULL, DAZM NUMBER(4,1) NULL, DTAKEOFF NUMBER(4,1) NULL,
  tResPick NUMBER(14,4) NULL,
 CONSTRAINT idOriginPick UNIQUE  (IDORIGINPICK),
 CONSTRAINT OriginPick_idOrigin FOREIGN KEY (IDORIGIN) 
   REFERENCES ORIGIN(IDORIGIN),
 CONSTRAINT OriginPick_idPick FOREIGN KEY (IDPICK) 
   REFERENCES PICK(IDPICK),  
 CONSTRAINT OriginPick PRIMARY KEY (IDORIGIN,IDPICK)
);
Create Sequence OriginPickSeq START WITH 1 MAXVALUE 999999999 CYCLE CACHE 2;

/*************************************************/
CREATE TABLE EXTERNALEVENT (idExternalEvent NUMBER(13,0) NOT NULL, 
  idSource NUMBER(13,0) NULL, sSourceEventID VARCHAR2(40) NULL,
  idComment NUMBER(13,0) NULL,
 CONSTRAINT idExternalEvent UNIQUE (idExternalEvent),
 CONSTRAINT EXTERNALEVENT_idSource FOREIGN KEY (idSource) 
   REFERENCES Source(idSource),
 CONSTRAINT EXTERNALEVENT_SourceEventID PRIMARY KEY (sSourceEventID,idSource)
);
Create Sequence ExternalEventSeq START WITH 1 MAXVALUE 999999999 CYCLE CACHE 2;

/*************************************************/
CREATE TABLE EWDB_TABLELIST (IDTABLE NUMBER(13,0) NOT NULL, 
  STABLENAME VARCHAR2(40) NULL,
 CONSTRAINT EWDB_Tablelist_idTable UNIQUE  (idTable),
 CONSTRAINT EWDB_TABLELIST_STABLENAME PRIMARY KEY (sTableName)
);

/*************************************************/
CREATE TABLE EWDBNode (EWDBNodeID NUMBER(4,0) NOT NULL, 
  EWDBNodeName VARCHAR2(40) NULL,  idComment NUMBER(13,0) NULL,
  iIsMyNodeID NUMBER(1,0) NULL, 
 CONSTRAINT EWDBNodeID PRIMARY KEY (EWDBNodeID)
);

/*************************************************/
/* Debug table, not described in the schema documentation. */
CREATE TABLE test (PROC_NAME VARCHAR2(60) NULL,  
  ItemID NUMBER(13,0) NULL, RetCode NUMBER(13,0) NULL
);




/***************************************************************/
/******************** INFRASTRUCTURE SCHEMA ********************/
/***************************************************************/

/**************************************/
CREATE TABLE Chan (idChan NUMBER(13,0) NULL, idComment NUMBER(13,0) NULL,  
 CONSTRAINT idChan PRIMARY KEY (idChan)  
)  TABLESPACE EWDB_INFRASPACE;
Create Sequence ChanSeq       START WITH 1 MAXVALUE 999999999 CYCLE;

/**************************************/
CREATE TABLE Site (idSite NUMBER(13,0) NULL, 
  sSta VARCHAR2(7) NULL, sNet VARCHAR2(9) NULL, 
  idComment NUMBER(13,0) NULL,  
 CONSTRAINT idSite UNIQUE  (idSite),  
 CONSTRAINT Site_Sta_Net PRIMARY KEY (sSta, sNet)  
)  TABLESPACE EWDB_INFRASPACE;
Create Sequence SiteSeq       START WITH 1 MAXVALUE 999999999 CYCLE;

/**************************************/
CREATE TABLE SiteT (idSiteT NUMBER(13,0) NULL, 
  idSite NUMBER(13,0) NULL, 
  tOff NUMBER(14,4) NULL, tOn NUMBER(14,4) NULL, 
  dLat NUMBER(10,6) NULL, dLon NUMBER(10,6) NULL, 
  dElev NUMBER(7,2) NULL, idComment NUMBER(13,0) NULL,  
 CONSTRAINT SiteT_idSite FOREIGN KEY (idSite) 
   REFERENCES SITE(IDSITE),  
 CONSTRAINT SiteT_idSite_tOff PRIMARY KEY (idSite, tOff),
 CONSTRAINT idSiteT UNIQUE  (idSiteT)
)  TABLESPACE EWDB_INFRASPACE;
Create Sequence SiteTSeq      START WITH 1 MAXVALUE 999999999 CYCLE;

/**************************************/
CREATE TABLE Comp (idComp NUMBER(13,0) NULL, 
  idSite NUMBER(13,0) NULL, sComp VARCHAR2(9) NULL, 
  sLoc VARCHAR2(9) NULL, idComment NUMBER(13,0) NULL,  
 CONSTRAINT idComp PRIMARY KEY (idComp),  
 CONSTRAINT Comp_idSite FOREIGN KEY (idSite) 
   REFERENCES SITE(IDSITE),  
 CONSTRAINT Comp_SCNL UNIQUE  (idSite, sComp, sLoc)  
)  TABLESPACE EWDB_INFRASPACE;
Create Sequence CompSeq       START WITH 1 MAXVALUE 999999999 CYCLE;

/**************************************/
CREATE TABLE CompT (idCompT NUMBER(13,0) NULL, 
  idComp NUMBER(13,0) NULL, tOff NUMBER(14,4) NULL, 
  tOn NUMBER(14,4) NULL, sSta VARCHAR2(7) NULL, 
  sComp VARCHAR2(9) NULL, sNet VARCHAR2(9) NULL, 
  sLoc VARCHAR2(9) NULL, dLat NUMBER(10,6) NULL, 
  dLon NUMBER(10,6) NULL, dElev NUMBER(7,2) NULL, 
  dAzm NUMBER(4,1) NULL, dDip NUMBER(4,1) NULL, 
  idComment NUMBER(13,0) NULL,  
 CONSTRAINT idCompT UNIQUE  (idCompT),  
 CONSTRAINT CompT_idComp_tOff PRIMARY KEY (idComp, tOff)  
)  TABLESPACE EWDB_INFRASPACE;
Create Sequence CompTSeq      START WITH 1 MAXVALUE 999999999 CYCLE;

/**************************************/
CREATE TABLE ChanT (idChanT NUMBER(13,0) NULL, 
  idChan NUMBER(13,0) NULL, tOff NUMBER(14,4) NULL, 
  tOn NUMBER(14,4) NULL, idCompT NUMBER(13,0) NULL, 
  idDeviceslot NUMBER(13,0) NULL, iPlexor NUMBER(3,0) NULL, 
  idComment NUMBER(13,0) NULL,  CONSTRAINT idChanT UNIQUE (idChanT),  
 CONSTRAINT ChanT_idChan_tOff PRIMARY KEY (idChan, tOff),  
 CONSTRAINT ChanT_idCompT FOREIGN KEY (idCompT) 
   REFERENCES COMPT(IDCOMPT),  
 CONSTRAINT ChanT_idChan FOREIGN KEY (idChan) 
   REFERENCES Chan(idChan)
)  TABLESPACE EWDB_INFRASPACE;
Create Sequence ChanTSeq      START WITH 1 MAXVALUE 999999999 CYCLE;

/**************************************/
CREATE TABLE CookedTF (idCTF NUMBER(13,0) NULL, 
  sFuncDesc varchar(50) NULL,
 CONSTRAINT idCTF PRIMARY KEY (idCTF)
)  TABLESPACE EWDB_INFRASPACE;
Create Sequence CTFSeq        START WITH 1 MAXVALUE 999999999 CYCLE CACHE 2;

/**************************************/
CREATE TABLE ChanCTF (idChanCTF NUMBER(13,0) NULL, 
  idChanT NUMBER(13,0) NULL, idCTF NUMBER(13,0) NULL, 
  dSampRate NUMBER(8,4) NULL, dGain NUMBER NULL,
 CONSTRAINT idChanCTF UNIQUE  (idChanCTF),  
 CONSTRAINT ChanCTF_idChanT_Primary PRIMARY KEY (idChanT),  
 CONSTRAINT ChanCTF_idChanT_Foreign FOREIGN KEY (idChanT) 
   REFERENCES CHANT(IDCHANT),  
 CONSTRAINT ChanCTF_idCTF FOREIGN KEY (idCTF) 
   REFERENCES CookedTF(IDCTF)
)  TABLESPACE EWDB_INFRASPACE;
Create Sequence ChanCTFSeq    START WITH 1 MAXVALUE 999999999 CYCLE CACHE 2;

/**************************************/
CREATE TABLE PolesAndZeroes (idCTF NUMBER(13,0) NULL, 
  idPZ NUMBER(13,0) NULL, cPZType CHAR(1) NULL, 
  dReal NUMBER(14,7) NULL, dImaginary NUMBER(14,7) NULL,  
 CONSTRAINT idPZ PRIMARY KEY (idPZ),  
 CONSTRAINT PolesAndZeroes_idCTF FOREIGN KEY (idCTF) 
   REFERENCES CookedTF(IDCTF)
   /* CLEANUP cPZType should be changed to iPZType or bPZType */
   /*  whatever the standard format is for a */
   /*   binary(T or F) flag. */
)  TABLESPACE EWDB_INFRASPACE;
Create Sequence PZSeq         START WITH 1 MAXVALUE 999999999 CYCLE CACHE 2;




/*********************************************************/
/******************** EXTERNAL SCHEMA ********************/
/*********************************************************/

/*****************************/
CREATE TABLE Station_External 
(STATIONID NUMBER(9,0) NOT NULL, 
 STA VARCHAR2(10) NOT NULL, 
 CHAN VARCHAR2(10) NOT NULL, 
 NET VARCHAR2(10) NOT NULL, 
 LOC VARCHAR2(10) NULL, /** External station name **/
 LAT NUMBER(10,6) NULL,
 LON NUMBER(10,6) NULL, 
 ELEV NUMBER(12,4) NULL,
 DESCRIPTION VARCHAR2(100) NULL,
  UNIQUE(STA,CHAN,NET,LOC),
  CONSTRAINT Station_External_STATIONID PRIMARY KEY (STATIONID)
);
Create Sequence Station_ExternalSeq START WITH 1 MAXVALUE 999999999 CYCLE;

/* STATION 2 idChan table, used for retrieving an idChan */
/* based on a STATIONID in the Station_External table       */
CREATE TABLE Station_2_idChan_External
(StationID NUMBER(9,0) NULL,
 idChan NUMBER(13,0) NOT NULL,
  UNIQUE(idChan),
  CONSTRAINT Station_2_idChan_StationID PRIMARY KEY (StationID)
);



/*********************************************************/
/******************** WAVEFORM SCHEMA ********************/
/*********************************************************/

/******************************/
CREATE TABLE WaveformDesc 
(idWaveform NUMBER(13,0) NULL,
  idChan NUMBER(13,0) NULL, tStart NUMBER(14,4) NULL,
  tEnd NUMBER(14,4), iDataFormat NUMBER(9,0) NULL, iByteLen NUMBER(10,0) NULL, 
  iStorageType NUMBER(9,0) NULL, sStorageInfo VARCHAR2(40) NULL,
 CONSTRAINT WaveformDesc_PK_idWaveform PRIMARY KEY(idWaveform),
 CONSTRAINT WaveformDesc_idChan FOREIGN KEY (idChan) 
   REFERENCES Chan(idChan)
) 
 TABLESPACE "EWDB_SNIPPETSPACE";


/******************************/
CREATE TABLE Waveform
(idWaveform NUMBER(13,0) NULL,
  binSNIPPET LONG RAW NULL,
 CONSTRAINT Waveform_idWaveform PRIMARY KEY(idWaveform),
 CONSTRAINT Waveform_FK_idWaveform FOREIGN KEY (idWaveform) 
   REFERENCES WaveformDesc(idWaveform)
) 
  TABLESPACE "EWDB_SNIPPETSPACE";
Create Sequence WaveformSeq      START WITH 1 MAXVALUE 999999999 CYCLE CACHE 2;




/**************************************************************/
/******************** STRONG MOTION SCHEMA ********************/
/**************************************************************/


/**********************************/
CREATE TABLE SMMessage (idSMMessage NUMBER(13,0) NULL, 
  idChan NUMBER(13,0) NULL, tMotion NUMBER(14,4) NULL, 
  tLoad NUMBER(14,4) NULL, tAlternate NUMBER(14,4) NULL,
  iAltCode NUMBER(2,0) NULL, tPGA NUMBER(14,4) NULL,
  tPGV NUMBER(14,4) NULL, tPGD NUMBER(14,4) NULL,
 CONSTRAINT idSMMessage PRIMARY KEY (idSMMessage),
 CONSTRAINT SMMessage_idChan FOREIGN KEY (idChan) 
   REFERENCES Chan(idChan)
);
Create Sequence SMMotionSeq   START WITH 1 MAXVALUE 999999999 CYCLE;

/**********************************/
CREATE TABLE SMMotion (idSMMotion NUMBER(13,0) NOT NULL, 
  idSMMessage NUMBER(13,0) NULL, iMotionType NUMBER(1,0) NULL, 
  dPeriod NUMBER(6,3) NULL, 
  idChan NUMBER(13,0) NULL, dMeasurement NUMBER(10,6) NULL,
 CONSTRAINT idSMMotion PRIMARY KEY (idSMMotion),
 CONSTRAINT SMMotion_UK UNIQUE (idSMMessage,iMotionType,dPeriod),
 CONSTRAINT SMMotion_idChan FOREIGN KEY (idChan) 
   REFERENCES Chan(idChan),
 CONSTRAINT SMMotion_idSMMessage FOREIGN KEY (idSMMessage) 
   REFERENCES SMMessage(idSMMessage)
);
Create Sequence SMMessageSeq  START WITH 1 MAXVALUE 999999999 CYCLE;




/*******************************************************/
/******************** ALARMS SCHEMA ********************/
/*******************************************************/

/*************************************************/
CREATE TABLE AlarmsRecipient (idRecipient NUMBER(13,0) NOT NULL, 
 dPriority NUMBER(13,0) NOT NULL, 
 sDescription VARCHAR2(256) NOT NULL, 
 CONSTRAINT idRecipient PRIMARY KEY (idRecipient)
);
Create Sequence AlarmsRecipientSeq  START WITH 1 MAXVALUE 999999999 CYCLE CACHE 2;

/*************************************************/
CREATE TABLE RecipientDelivery (idRecipientDelivery NUMBER(13,0) NOT NULL, 
    idRecipient NUMBER(13,0) NOT NULL, 
    sTableName VARCHAR2(64) NOT NULL, 
	idDelivery NUMBER(13,0) NOT NULL,
 CONSTRAINT idRecipientDelivery UNIQUE (idRecipientDelivery),
 CONSTRAINT AlarmsBind_PK  PRIMARY KEY (idRecipient, sTableName, idDelivery),
 CONSTRAINT RecipientDelivery_idRecipient 
				FOREIGN KEY (idRecipient) REFERENCES ALARMSRECIPIENT(IDRECIPIENT)
);
Create Sequence RecipientDeliverySeq  START WITH 1 MAXVALUE 999999999 CYCLE CACHE 2;

/*************************************************/
CREATE TABLE CriteriaProgram (idCritProgram NUMBER(13,0) NOT NULL, 
 sProgName VARCHAR2(256) NOT NULL, 
 sProgDir VARCHAR2(256) NOT NULL, 
 sProgDescription VARCHAR2(256) NOT NULL, 
 CONSTRAINT idCritProgram PRIMARY KEY (idCritProgram));
Create Sequence AlarmsCritSeq  START WITH 1 MAXVALUE 999999999 CYCLE CACHE 2;

/*************************************************/
CREATE TABLE EmailDelivery (idDelivery NUMBER(13,0) NOT NULL, 
 sAddress VARCHAR2(256) NOT NULL, 
 sMailServer VARCHAR2(256) NULL, 
 CONSTRAINT idEmailDelivery PRIMARY KEY (idDelivery));
Create Sequence EmailDeliverySeq  START WITH 1 MAXVALUE 999999999 CYCLE CACHE 2;

/*************************************************/
CREATE TABLE PagerDelivery (idDelivery NUMBER(13,0) NOT NULL, 
 sNumber VARCHAR2(256) NOT NULL, 
 sPagerCompany VARCHAR2(256) NULL, 
 CONSTRAINT idPagerDelivery PRIMARY KEY (idDelivery));
Create Sequence PagerDeliverySeq  START WITH 1 MAXVALUE 999999999 CYCLE CACHE 2;

/*************************************************/
CREATE TABLE PhoneDelivery (idDelivery NUMBER(13,0) NOT NULL, 
 sPhoneNumber VARCHAR2(256) NOT NULL, 
 CONSTRAINT idPhoneDelivery PRIMARY KEY (idDelivery));
Create Sequence PhoneDeliverySeq  START WITH 1 MAXVALUE 999999999 CYCLE CACHE 2;

/*************************************************/
CREATE TABLE QddsDelivery (idDelivery NUMBER(13,0) NOT NULL, 
 sQddsDirectory VARCHAR2(256) NOT NULL, 
 CONSTRAINT idQddsDelivery PRIMARY KEY (idDelivery));
Create Sequence QddsDeliverySeq  START WITH 1 MAXVALUE 999999999 CYCLE CACHE 2;

/*************************************************/
CREATE TABLE CustomDelivery (idDelivery NUMBER(13,0) NOT NULL, 
 sDescription VARCHAR2(256) NOT NULL, 
 CONSTRAINT idCustomDelivery PRIMARY KEY (idDelivery));
Create Sequence CustomDeliverySeq  START WITH 1 MAXVALUE 999999999 CYCLE CACHE 2;

/*************************************************/
CREATE TABLE AlarmsFormat (idFormat NUMBER(13,0) NOT NULL, 
	sDescription VARCHAR2(256) NOT NULL,
	sFmtInsert VARCHAR2(4000) NOT NULL,
	sFmtDelete VARCHAR2(4000) NOT NULL,
 CONSTRAINT idFormat PRIMARY KEY (idFormat));

/*
 * STORY: There used to be only one hardcoded format (CUBE), but there will
 *  probably be more.  So, we'll leave ourselves enough room in the AlarmsFormat
 *  table to store these predefined, hardcoded formats.
 *    To that end, our AlarmsFormatSeq will start at 100, allowing us 100 such
 *  formats.  Use the examples below to insert your own hardcoded formats.
 *    See determine_alarms.c for explanation on how alarms messages are built
 *  from formats, including hardcoded ones.
 */


/* Insert the hardcoded CUBE format */
insert into AlarmsFormat values (1, 'CUBE',
        'Predefined CUBE Insertion Format. DO NOT CHANGE!~End~',
        'Predefined CUBE Deletion Format. DO NOT CHANGE!~End~');

/* Insert the hardcoded hypoTWC format */
insert into AlarmsFormat values (2, 'hypoTWC',
        'Predefined hypoTWC Insertion Format. DO NOT CHANGE!~End~',
        'Predefined hypoTWC Deletion Format. DO NOT CHANGE!~End~');


Create Sequence AlarmsFormatSeq  START WITH 100 MAXVALUE 999999999 CYCLE CACHE 2;

/*************************************************/
CREATE TABLE AlarmsRule (idRule NUMBER(13,0) NOT NULL, 
  dMag NUMBER(14,4) NULL,
  bAuto NUMBER(13,0) NULL,
  idFormat NUMBER(13,0) NOT NULL,
  idCritProgram NUMBER(13,0) NULL,
  idRecipientDelivery NUMBER(13,0) NOT NULL,
 CONSTRAINT idRule PRIMARY KEY (idRule),
 CONSTRAINT idFormat_PK FOREIGN KEY (idFormat) 
						REFERENCES ALARMSFORMAT(IDFORMAT),
 CONSTRAINT idRecipientDel_PK FOREIGN KEY (idRecipientDelivery) 
						REFERENCES RECIPIENTDELIVERY(IDRECIPIENTDELIVERY));
Create Sequence AlarmsRuleSeq  START WITH 1 MAXVALUE 999999999 CYCLE CACHE 2;

/*************************************************/
CREATE TABLE AlarmsAudit (idAudit NUMBER(13,0) NOT NULL, 
  idEvent NUMBER(13,0) NOT NULL,
  bAuto NUMBER(13,0) NOT NULL,
  idRecipient NUMBER(13,0) NOT NULL,
  idFormat NUMBER(13,0) NOT NULL,
  sTableName VARCHAR2(256) NOT NULL, 
  idDelivery NUMBER(13,0) NOT NULL,
  tAlarmDeclared NUMBER(14,4) NULL,
  tAlarmExecuted NUMBER(14,4) NULL,
  sInvocationString VARCHAR2(256) NOT NULL, 
 CONSTRAINT idAudit PRIMARY KEY (idAudit));
Create Sequence AlarmsAuditSeq  START WITH 1 MAXVALUE 999999999 CYCLE CACHE 2;

/*************************************************/
CREATE TABLE AuditEmailDelivery (idDelivery NUMBER(13,0) NOT NULL, 
 sAddress VARCHAR2(256) NOT NULL, 
 sMailServer VARCHAR2(256) NULL, 
 CONSTRAINT Audit_idEmailDelivery PRIMARY KEY (idDelivery));
Create Sequence AuditEmailDeliverySeq  START WITH 1 MAXVALUE 999999999 CYCLE CACHE 2;

/*************************************************/
CREATE TABLE AuditPagerDelivery (idDelivery NUMBER(13,0) NOT NULL, 
 sNumber VARCHAR2(256) NOT NULL, 
 sPagerCompany VARCHAR2(256) NULL, 
 CONSTRAINT Audit_idPagerDelivery PRIMARY KEY (idDelivery));
Create Sequence AuditPagerDeliverySeq  START WITH 1 MAXVALUE 999999999 CYCLE CACHE 2;

/*************************************************/
CREATE TABLE AuditPhoneDelivery (idDelivery NUMBER(13,0) NOT NULL, 
 sPhoneNumber VARCHAR2(256) NOT NULL,
 CONSTRAINT Audit_idPhoneDelivery PRIMARY KEY (idDelivery));
Create Sequence AuditPhoneDeliverySeq  START WITH 1 MAXVALUE 999999999 CYCLE CACHE 2;

/*************************************************/
CREATE TABLE AuditQddsDelivery (idDelivery NUMBER(13,0) NOT NULL, 
 sQddsDirectory VARCHAR2(256) NOT NULL, 
 CONSTRAINT Audit_idQddsDelivery PRIMARY KEY (idDelivery));
Create Sequence AuditQddsDeliverySeq  START WITH 1 MAXVALUE 999999999 CYCLE CACHE 2;

/*************************************************/
CREATE TABLE AuditCustomDelivery (idDelivery NUMBER(13,0) NOT NULL, 
 sDescription VARCHAR2(256) NOT NULL, 
 CONSTRAINT Audit_idCustomDelivery PRIMARY KEY (idDelivery));
Create Sequence AuditCustomDeliverySeq  START WITH 1 MAXVALUE 999999999 CYCLE CACHE 2;

/*************************************************/
CREATE TABLE CubeVersionNumber (idVersion NUMBER(13,0) NOT NULL, 
 idEvent NUMBER (13,0) NOT NULL, 
 iVersionNumber NUMBER (13,0) NOT NULL,
 CONSTRAINT Audit_idVersion PRIMARY KEY (idVersion));
Create Sequence CubeVersionSeq  START WITH 1 MAXVALUE 999999999 CYCLE CACHE 2;


/*************************************************
 POPULATE CONSTANT TABLES
 *************************************************/
@ewdb_load_constants

commit;




