/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */

/***********************************************************
  Updates and additions to parametric tables go here
**********************************************************/

/*************************************************/
CREATE TABLE CoincidenceEvent (idCoincidence NUMBER(13,0) NOT NULL, 
  tCoincidence NUMBER(14,4) NULL, 
  idEvent NUMBER(13,0) NOT NULL,
  idSource NUMBER(13,0) NULL, 
  idComment NUMBER(13,0) NULL,
 CONSTRAINT idCoincidence PRIMARY KEY (idCoincidence)
);
Create Sequence CoincSeq      START WITH 1 MAXVALUE 999999999 CYCLE CACHE 2;

/*************************************************/
CREATE TABLE ChannelTrigger (idTrigger  NUMBER(13,0) NOT NULL,
  idCoincidence NUMBER(13,0) NOT NULL,
  tTrigger NUMBER(14,4) NULL,
  idChan NUMBER(13,0) NOT NULL,
 CONSTRAINT idTrigger PRIMARY KEY (idTrigger),
 CONSTRAINT Trig_idCoincidence FOREIGN KEY (idCoincidence)
    REFERENCES COINCIDENCEEVENT (IDCOINCIDENCE)
);
Create Sequence TriggerSeq      START WITH 1 MAXVALUE 999999999 CYCLE CACHE 2;

/*************************************************/
CREATE TABLE Phenomena (idPh NUMBER(13,0) NOT NULL,
  idPrefEvent NUMBER(13,0) NOT NULL,
  idSource NUMBER(13,0) NULL, 
  idComment NUMBER(13,0) NULL,
 CONSTRAINT idPh PRIMARY KEY (idPh),
 CONSTRAINT PH_idPrefEvent FOREIGN KEY (idPrefEvent) REFERENCES EVENT (IDEVENT)
);
Create Sequence PhenomenaSeq    START WITH 1 MAXVALUE 999999999 CYCLE CACHE 2;

/*************************************************/
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

