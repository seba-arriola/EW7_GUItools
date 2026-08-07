/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */


/*
  Creating Strong Motion Tables
**********************************/
CREATE TABLE SMMessage (idSMMessage NUMBER(13,0) NULL, 
  idChan NUMBER(13,0) NULL, tMotion NUMBER(14,4) NULL, 
  tLoad NUMBER(14,4) NULL, tAlternate NUMBER(14,4) NULL,
  iAltCode NUMBER(2,0) NULL, tPGA NUMBER(14,4) NULL,
  tPGV NUMBER(14,4) NULL, tPGD NUMBER(14,4) NULL,
 CONSTRAINT idSMMessage PRIMARY KEY (idSMMessage),
 CONSTRAINT SMMessage_idChan FOREIGN KEY (idChan) 
   REFERENCES Chan(idChan)
);


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

/*
  Creating Strong Motion Sequences
************************************/
Create Sequence SMMotionSeq   START WITH 1 MAXVALUE 999999999 CYCLE;
Create Sequence SMMessageSeq  START WITH 1 MAXVALUE 999999999 CYCLE;


