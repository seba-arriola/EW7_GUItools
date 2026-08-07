/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */

/* 
 Creating Core Tables
************************/

/*************************************************/
CREATE TABLE Event (idEvent NUMBER(13,0) NULL, tiEventType NUMBER(13,0) NULL,
  iDubiocity NUMBER(2,0) NULL, bArchived NUMBER(1,0) NULL, 
  idComment NUMBER(13,0) NULL,  
 CONSTRAINT idEvent PRIMARY KEY (idEvent)
);

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

/*************************************************/
CREATE TABLE MechFM (idMechFM NUMBER(13,0) NULL, tiExternal NUMBER(13,0) NULL,
  xidExternal VARCHAR(16) NULL, 
  idSource NUMBER(13,0) NULL, idOrigin NUMBER(13,0) NULL,  
  idComment NUMBER(13,0) NULL,
 CONSTRAINT idMechFM PRIMARY KEY (idMechFM),
 CONSTRAINT MechFM_idOrigin FOREIGN KEY (idOrigin) 
   REFERENCES ORIGIN(IDORIGIN)
);


/*************************************************/
CREATE TABLE Pick (idPick NUMBER(13,0) NULL, tiExternal NUMBER(13,0) NULL, 
  xidExternal VARCHAR(16) NULL, idChan NUMBER(13,0) NULL, 
  sPhase VARCHAR2(6) NULL, tPhase NUMBER(14,4) NULL, cMotion VARCHAR2(1) NULL, 
  cOnset VARCHAR2(1) NULL, dSigma NUMBER(14,4) NULL,  
 CONSTRAINT idPick PRIMARY KEY (idPick)
 /* CLEANUP dSigma should be changed to tSigma since it
    is a time value. */
);

/*************************************************/
CREATE TABLE Ray (idRay NUMBER(13,0) NULL, tiExternal NUMBER(13,0) NULL, 
  xidExternal VARCHAR(16) NULL, tOn NUMBER(14,4) NULL, 
  tOff NUMBER(14,4) NULL, dAzm NUMBER(4,1) NULL, 
  dSlow NUMBER(14,4) NULL, idChan1 NUMBER(13,0) NULL,
  idChan2 NUMBER(13,0) NULL, idChan3 NUMBER(13,0) NULL,  
 CONSTRAINT idRay PRIMARY KEY (idRay)
);

/*************************************************/
CREATE TABLE TCoda (IDTCODA NUMBER(13,0) NOT NULL, 
  tiExternal NUMBER(13,0) NULL, xidExternal VARCHAR(16) NULL, 
  IDCHAN NUMBER(13,0) NULL, TCODATERMOBS NUMBER(14,4) NULL, 
  TCODATERMXTP NUMBER(14,4) NULL,  
 CONSTRAINT idTCoda PRIMARY KEY (IDTCODA)
);

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

/*************************************************/
CREATE TABLE Source (idSource NUMBER(13,0) NULL, sSource VARCHAR2(50) NOT NULL, 
  sHumanReadable VARCHAR2(100), idComment NUMBER(13,0) NULL,  
 CONSTRAINT Source_sSource UNIQUE  (sSource),
 CONSTRAINT idSource PRIMARY KEY (idSource)
);

/*************************************************/
CREATE TABLE Comments (idComment NUMBER(13,0) NULL, 
  sComment VARCHAR2(4000) NULL,
 CONSTRAINT idComment PRIMARY KEY (idComment)
);

/*************************************************/
CREATE TABLE MagLink (IDMAGLINK NUMBER(13,0) NOT NULL, 
  IDMAG NUMBER(13,0) NULL, IDDATUM NUMBER(13,0) NULL, 
  DMAG NUMBER(3,2) NULL, DWEIGHT NUMBER(4,1) NULL,  
 CONSTRAINT idMagLink PRIMARY KEY (IDMAGLINK),
 CONSTRAINT MagLink_idMag FOREIGN KEY (IDMAG) 
   REFERENCES MAGNITUDE(IDMAG)
 /* CHANGED: dMag from (4,2) to (3,2) because XX.XX
     did not make sense for a magnitude value. */
);

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

/*************************************************/
CREATE TABLE Bind (idBind NUMBER(13,0) NOT NULL, idEvent NUMBER(13,0) NULL, 
  tiCore NUMBER(13,0) NULL, idCore NUMBER(13,0) NULL,
 CONSTRAINT idBind UNIQUE (idBind),
 CONSTRAINT Bind_PK PRIMARY KEY (idEvent,tiCore,idCore),
 CONSTRAINT Bind_idEvent FOREIGN KEY
   (idEvent) REFERENCES EVENT(idEvent)
);

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

/*************************************************/
CREATE TABLE EXTERNALEVENT (idExternalEvent NUMBER(13,0) NOT NULL, 
  idSource NUMBER(13,0) NULL, sSourceEventID VARCHAR2(40) NULL,
  idComment NUMBER(13,0) NULL,
 CONSTRAINT idExternalEvent UNIQUE (idExternalEvent),
 CONSTRAINT EXTERNALEVENT_idSource FOREIGN KEY (idSource) 
   REFERENCES Source(idSource),
 CONSTRAINT EXTERNALEVENT_SourceEventID PRIMARY KEY (sSourceEventID,idSource)
);

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
 /* CLEANUP EWDBNodeName should be changed to sEWDBNodeName
    to reflect that the field is a string. */
);

/*************************************************/
/* Debug table, not described in the schema documentation. */
CREATE TABLE test (PROC_NAME VARCHAR2(60) NULL,  
  ItemID NUMBER(13,0) NULL, RetCode NUMBER(13,0) NULL
);





/*****  LUCKY'S COINCIDENCE TABLES Jan 2002 *******/

/*************************************************/
CREATE TABLE CoincidenceEvent (idCoincidence NUMBER(13,0) NOT NULL, 
  tCoincidence NUMBER(14,4) NULL, 
  idEvent NUMBER(13,0) NOT NULL,
  idSource NUMBER(13,0) NULL, 
  idComment NUMBER(13,0) NULL,
 CONSTRAINT idCoincidence PRIMARY KEY (idCoincidence)
);
/* Move */
Create Sequence CoincSeq      START WITH 1 MAXVALUE 999999999 CYCLE CACHE 2;



CREATE TABLE ChannelTrigger (idTrigger  NUMBER(13,0) NOT NULL,
  idCoincidence NUMBER(13,0) NOT NULL,
  tTrigger NUMBER(14,4) NULL,
  idChan NUMBER(13,0) NOT NULL,
 CONSTRAINT idTrigger PRIMARY KEY (idTrigger),
 CONSTRAINT Trig_idCoincidence FOREIGN KEY (idCoincidence)
    REFERENCES COINCIDENCEEVENT (IDCOINCIDENCE)
);
Create Sequence TriggerSeq      START WITH 1 MAXVALUE 999999999 CYCLE CACHE 2;

/**** HACK HACK 
Drop Sequence TriggerSeq; 
Drop TABLE ChannelTrigger;
Drop Sequence CoincSeq;
Drop TABLE CoincidenceEvent;
****/

/*****  LUCKY'S MERGE TABLES Mar 2002 *******/
CREATE TABLE Phenomena (idPh NUMBER(13,0) NOT NULL,
  idPrefEvent NUMBER(13,0) NOT NULL,
  idSource NUMBER(13,0) NULL, 
  idComment NUMBER(13,0) NULL,
 CONSTRAINT idPh PRIMARY KEY (idPh),
 CONSTRAINT PH_idPrefEvent FOREIGN KEY (idPrefEvent) REFERENCES EVENT (IDEVENT)
);
Create Sequence PhenomenaSeq    START WITH 1 MAXVALUE 999999999 CYCLE CACHE 2;


CREATE TABLE Merge (idMerge NUMBER(13,0) NOT NULL,
  idPh NUMBER(13,0) NOT NULL,
  idEvent NUMBER(13,0) NOT NULL,
  idSource NUMBER(13,0) NULL, 
  idComment NUMBER(13,0) NULL,
 CONSTRAINT idMerge PRIMARY KEY (idMerge),
 CONSTRAINT Merge_idPh FOREIGN KEY (idPh) REFERENCES PHENOMENA (IDPH),
 CONSTRAINT Merge_idEvent FOREIGN KEY (idEvent) REFERENCES EVENT (IDEVENT)
);
Create Sequence MergeSeq    START WITH 1 MAXVALUE 999999999 CYCLE CACHE 2;

/*** HACK
drop Sequence MergeSeq
drop TABLE Merge
drop Sequence PhenomenaSeq
drop TABLE Phenomena
****/



/*************************************************/
/*########  END OF CREATE TABLE SCRIPTS  #########*/
/*************************************************/

