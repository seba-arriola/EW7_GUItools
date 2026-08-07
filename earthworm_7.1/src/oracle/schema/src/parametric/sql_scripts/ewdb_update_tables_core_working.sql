/*                                                          */
/*   THIS FILE IS UNDER CVS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*    $Id: ewdb_update_tables_core_working.sql,v 1.18 2005/06/27 15:37:46 davidk Exp $ */
/*                                                          */
/*    Revision history:                                     */
/*     $Log: ewdb_update_tables_core_working.sql,v $
/*     Revision 1.18  2005/06/27 15:37:46  davidk
/*     Moved "ALTER TABLE Comments" command from
/*     ora_api/sql_scripts/ewdb_update_schema_working.sql to where it belongs.
/*
/*     Revision 1.17  2005/06/14 16:54:05  davidk
/*     Modified the Mij values so that they can range from
/*     -99.9999 to 99.9999.  Carol will now base the Mij values on the Scalar
/*     Exponent used for M0, and the true range should be -2*M0 to 2*M0.
/*
/*     Revision 1.16  2005/05/12 20:42:03  mark
/*     Added comments to event struct
/*
/*     Revision 1.15  2005/03/31 18:52:20  davidk
/*     Changed the EWDB_MwStruct:
/*       Added dM0 (doh!)
/*       Changed dPercentCVLD to dPercentCLVD
/*
/*     Revision 1.14  2005/03/24 18:16:57  davidk
/*     Fixed bugs during initial testing of Mw API
/*     Added 2nd time series to MwChanTS to overcome 2000 byte string limitation
/*     in OCI 7.3.
/*
/*     Revision 1.13  2005/03/23 06:24:50  davidk
/*     Modified Mw create table statements, sequences, constraints, and indexes.
/*
/*     Revision 1.12  2005/03/17 23:52:08  davidk
/*     Added Mw Tables for Moment Tensor
/*
/*     Revision 1.11  2005/02/22 19:18:39  davidk
/*     Moved the "ALTER TABLE ORIGIN" command that added a versionnum
/*     field to the origin table, for providing order among origins for the same
/*     author/event.
/*
/*     Revision 1.10  2004/12/13 21:10:53  davidk
/*     Added index on Pick.tPhase for time based pick searches.
/*
/*     Revision 1.9  2004/11/09 16:03:09  labcvs
/*     Dave added new, bigger field for phase names.  JMP
/*
/*     Revision 1.8  2004/08/10 21:49:07  davidk
/*     Updated the architecture for sql install scripts.
/*     *_util  scripts load sql procedures, functions, and views
/*     *_constants scripts populate constant-tables
/*     *_tables  scripts load/update table, columns, constraints, and indexes
/*     *_combination scripts perform one of the above tasks, but contain statements
/*     which are dependent upon more than one sub-schema, and must be run
/*     separately after all sub-schemas have been processed.
/*
/*     These changes fixed several install problems and meaningless error messages.
/*        */
/*     Revision 1.1  2002/06/18 16:10:35  lucky             */
/*     Initial revision                                     */
/*                                                          */
/************************************************************/


/***********************************************************
  New parametric tables:
**********************************************************/

/**  MW TABLES  ***********/
CREATE TABLE MwFilter
(idMwFilter NUMBER(13,0) NOT NULL,
 /* the four taper frequencies */
 dLowCutHz    NUMBER(6,3) NOT NULL,
 dLowTaperHz  NUMBER(6,3) NOT NULL,
 dHighTaperHz NUMBER(6,3) NOT NULL,
 dHighCutHz   NUMBER(6,3) NOT NULL,
 CONSTRAINT MwFilter_idMwFilter PRIMARY KEY (idMwFilter),
 CONSTRAINT MwFilter_unique
   UNIQUE(dLowCutHz, dLowTaperHz, dHighTaperHz, dHighCutHz)
);


CREATE TABLE Mw
(idMw NUMBER(13,0) NOT NULL,
 /* six unique components of moment tensor */
 dMxx NUMBER(6,4) ,
 dMxy NUMBER(6,4) ,
 dMxz NUMBER(6,4) ,
 dMyy NUMBER(6,4) ,
 dMyz NUMBER(6,4) ,
 dMzz NUMBER(6,4) ,
 /*  Scalar momemnt.  Must be multiplied by 10^iScalarExp for proper value */
 dM0 NUMBER(3,2) NOT NULL, 
 iScalarExp NUMBER(2,0) ,
 /* Strike/Dip/Rake for the two fault-planes */
 dPFPStrike NUMBER(5,2) ,
 dPFPDip NUMBER(4,2) ,
 dPFPRake NUMBER(5,2) ,
 dAFPStrike NUMBER(5,2) ,
 dAFPDip NUMBER(4,2) ,
 dAFPRake NUMBER(5,2) ,
 /* Depth used for Mw evaluation */
 dDepth NUMBER(4,1) ,
 /* Origin used as basis for calculations */
 idOrigin NUMBER(13,0) NOT NULL,
 /* Magnitude that is derived from this Mw */
 idMag NUMBER(13,0) NULL,
 /* Number of stations used in Mw Calculation */
 iNumStations NUMBER(4,0) NOT NULL,
 /* Misfit for this Mw Calculation */
 dMisfit NUMBER(4,2),
 /*  Percent Double Couple */
 dPercentDC NUMBER(5,2),
 /*  Percent compensated linear vector dipole(CLVD) */
 dPercentCLVD NUMBER(5,2), 
 idMwFilter NUMBER(13,0) NOT NULL,
 CONSTRAINT Mw_idMw PRIMARY KEY (idMw),
 CONSTRAINT Mw_idOrigin_FK FOREIGN KEY (idOrigin)
   REFERENCES Origin (idOrigin),
 CONSTRAINT Mw_idMag_FK FOREIGN KEY (idMag)
   REFERENCES Magnitude (idMag),
 CONSTRAINT Mw_idMwFilter_FK FOREIGN KEY (idMwFilter)
   REFERENCES MwFilter (idMwFilter)
);


CREATE TABLE MwChanTS
(idMwCTS    NUMBER(13,0) NOT NULL,
 dTSStart   NUMBER(14,4) NOT NULL,
 dTSFreq    NUMBER(5,2) NOT NULL,
 iTSLen     NUMBER(3) NOT NULL,
 idMwFilter NUMBER(13,0) NOT NULL,
 idChan     NUMBER(13,0) NOT NULL,
 iType      NUMBER(1,0) NOT NULL,
 sTS1 VARCHAR2(4000) NOT NULL,
 sTS2 VARCHAR2(2000),
 CONSTRAINT MwCTS_idMwCTS PRIMARY KEY (idMwCTS),
 CONSTRAINT MwCTS_idMwFilter_FK FOREIGN KEY (idMwFilter)
   REFERENCES MwFilter (idMwFilter),
 CONSTRAINT MwCTS_idChan_FK FOREIGN KEY (idChan)
   REFERENCES Chan (idChan)
);

CREATE TABLE MwChan
(idMwChan NUMBER(13,0) NOT NULL,
 idMw     NUMBER(13,0) NOT NULL,
 iOffset  NUMBER(3,0) NOT NULL,
 idPick NUMBER(13,0),  
 idChan NUMBER(13,0) NOT NULL, 
 /* Misfit for this Mw Channel */
 dMisfit NUMBER(4,2),
 /* Links to the Real and Synthetic Time Series */
 idSynthTS   NUMBER(13,0) NOT NULL,
 idRealTS    NUMBER(13,0) NOT NULL,
 CONSTRAINT MwChan_idMwChan PRIMARY KEY (idMwChan),
 CONSTRAINT MwChan_idMw_FK FOREIGN KEY (idMw)
   REFERENCES Mw (idMw),
 CONSTRAINT MwChan_idSynthTS_FK FOREIGN KEY (idSynthTS)
   REFERENCES MwChanTS (idMwCTS),
 CONSTRAINT MwChan_idRealTS_FK FOREIGN KEY (idRealTS)
   REFERENCES MwChanTS (idMwCTS),
 CONSTRAINT MwChan_idChan_FK FOREIGN KEY (idChan)
   REFERENCES Chan (idChan)
);

/** MW SEQUENCES **/
Create Sequence MwSeq START WITH 1 MAXVALUE 999999999 CYCLE CACHE 2;
Create Sequence MwChanSeq START WITH 1 MAXVALUE 999999999 CYCLE CACHE 2;
Create Sequence MwChanTSSeq START WITH 1 MAXVALUE 999999999 CYCLE CACHE 2;
Create Sequence MwFilterSeq START WITH 1 MAXVALUE 999999999 CYCLE CACHE 2;


/***********************************************************
  Column Updates and additions to parametric tables:
**********************************************************/

/* Added idPick foreign key to PeakAmp table.  Some amplitude readings */
/* come directly from the picker, and are firmly associated with a pick */
/*********************************************************************/
ALTER TABLE PeakAmp ADD(idPick NUMBER(13,0) NULL,
                        CONSTRAINT PeakAmp_idPick FOREIGN KEY (idPick)
                         REFERENCES Pick(idPick));

/*                                                                    */
/* Added start and end times of the interval in which the amplitude   */
/* was computed                                                       */
/**********************************************************************/
ALTER TABLE PeakAmp ADD(tStartInterval NUMBER(14,4) NULL, 
                        tEndInterval NUMBER(14,4) NULL);

ALTER TABLE Event ADD(idInternalComment NUMBER(13,0) NULL,
                      idContribMagComment NUMBER(13,0) NULL);

/*                                                                    */
/* In Hydra(NEIC) atleast, amplitudes are being stored in digital     */
/* Counts, and 7 digits of precision(12,5) was not enough to          */
/* amplitudes for some broadband sensors.  9 digits(14,5) seems to be */
/* enough, but 11(16,5) was deemed the proper theoretical size, 2^36  */
/**********************************************************************/
ALTER TABLE PeakAmp MODIFY(DPEAKAMP1 NUMBER(16,5));
ALTER TABLE PeakAmp MODIFY(DPEAKAMP2 NUMBER(16,5));


/*                                                                    */
/* In Hydra(NEIC) atleast, phase names can be atleast 8 characters,   */
/* such as PKPPKPdf                                                   */
/**********************************************************************/
ALTER TABLE Pick MODIFY(sPhase varchar(9));
ALTER TABLE OriginPick MODIFY(sPhase varchar(9));

/*                                                                    */
/* Version number for determining order among origin publications     */
/**********************************************************************/
ALTER TABLE ORIGIN ADD(iVersionNum NUMBER(2,0) NULL);

/***********************************************************
  Constraint additions to parametric tables:
**********************************************************/

/*************************************************/
/* THE FOLLOWING SCRIPTS REQUIRE THE SOURCE TABLE */
ALTER TABLE Origin
 ADD( 
   CONSTRAINT Origin_idSource FOREIGN KEY (idSource) 
     REFERENCES SOURCE(idSource)
);

ALTER TABLE Magnitude
 ADD( 
   CONSTRAINT Magnitude_idSource FOREIGN KEY (idSource) 
     REFERENCES SOURCE(idSource)
);
ALTER TABLE MechFM
 ADD( 
   CONSTRAINT MechFM_idSource FOREIGN KEY (idSource) 
     REFERENCES SOURCE(idSource)
);

/***************************************************/
/* THE FOLLOWING SCRIPTS REQUIRE THE MagType TABLE */
ALTER TABLE Magnitude
 ADD( 
 CONSTRAINT Magnitude_iMagType FOREIGN KEY (iMagType) 
   REFERENCES MagType(iMagType)
);

ALTER TABLE PeakAmp
 ADD( 
 CONSTRAINT PeakAmp_iMagType FOREIGN KEY (iMagType) 
   REFERENCES MagType(iMagType)
);

/*************************************************/
/* THE FOLLOWING SCRIPTS REQUIRE THE COMMENTS TABLE */
ALTER TABLE Comments ADD(idNextComment NUMBER(13,0) NULL);


/***********************************************************
  Index additions to parametric tables:
**********************************************************/
CREATE INDEX Bind_idEvent ON  Bind(idEvent,tiCore) ;
CREATE INDEX CodaAmp_idTCoda ON CodaAmp(idTCoda);
CREATE INDEX Bind_idCore ON Bind(idCore);

create index CodaDur_idPick_IDX
  on Codadur(idpick) tablespace ewdb_corespace;

create index MagLink_idMag_IDX
  on Maglink(idmag) tablespace ewdb_corespace;

create index Mag_idOrigin_IDX
  on Magnitude(idorigin) tablespace ewdb_corespace;

CREATE INDEX OriginPick_idPick ON OriginPick(idPick);
CREATE INDEX Peakamp_idPick ON Peakamp(idPick);
CREATE INDEX Peakamp_tAmp1 ON PeakAmp(tAmp1);
CREATE INDEX Peakamp_iMagType ON PeakAmp(iMagType);
CREATE INDEX Peakamp_idChan ON PeakAmp(idChan);
CREATE INDEX Pick_idChan ON Pick(idChan);
CREATE INDEX Pick_xidExternal_idSource ON Pick(xidexternal,idsource);
CREATE INDEX Pick_tPhase ON Pick(tPhase);

/* Mw Indexes */
CREATE INDEX Mw_idOrigin      ON Mw (idOrigin);
CREATE INDEX Mw_idMag         ON Mw (idMag);
CREATE INDEX Mw_idMwFilter    ON Mw (idMwFilter);
CREATE INDEX MwCTS_idMwFilter ON MwChanTS (idMwFilter);
CREATE INDEX MwCTS_idChan     ON MwChanTS (idChan);

CREATE INDEX MwChan_idMw ON MwChan (idMw);
CREATE INDEX MwChan_idChan ON MwChan (idChan);
CREATE INDEX MwChan_idSynthTS ON MwChan (idSynthTS);
CREATE INDEX MwChan_idRealTS  ON MwChan (idRealTS);
