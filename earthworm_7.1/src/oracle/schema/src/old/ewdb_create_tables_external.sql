/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */
/*                                                          */

/*
  Creating External Tables
*****************************/
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
/* CHANGED:  STATIONID from unique to PRIMARY KEY DK 05/05/00 */

/* STATION 2 idChan table, used for retrieving an idChan */
/* based on a STATIONID in the Station_External table       */
CREATE TABLE Station_2_idChan_External
(StationID NUMBER(9,0) NULL,
 idChan NUMBER(13,0) NOT NULL,
  UNIQUE(idChan),
  CONSTRAINT Station_2_idChan_StationID PRIMARY KEY (StationID)
);


/* UH external infrastructure info */
CREATE TABLE UH_Info_External
(idUHInfo NUMBER(13,0) NOT NULL,
 idChanT NUMBER(13,0) NOT NULL,
 dFullscale NUMBER(14,4) NOT NULL,
 dSensitivity NUMBER(14,6) NOT NULL,
 dNaturalFrequency NUMBER(14,4) NOT NULL,
 dDamping NUMBER(14,4) NOT NULL,
 dAzm NUMBER(14,4) NOT NULL,
 dDip NUMBER(14,4) NOT NULL,
 iGain NUMBER(13,0) NOT NULL,
 iSensorType NUMBER(13,0) NOT NULL,
  UNIQUE(idChanT),
  CONSTRAINT UH_idUHInfo PRIMARY KEY (IDUHINFO),
  CONSTRAINT Infra_idChanT FOREIGN KEY (idChanT)
	REFERENCES ChanT(idChanT)
);

/*
  Creating External Sequences
*****************************/
Create Sequence Station_ExternalSeq
                              START WITH 1 MAXVALUE 999999999 CYCLE;
Create Sequence UH_Info_ExternalSeq
                              START WITH 1 MAXVALUE 999999999 CYCLE;


