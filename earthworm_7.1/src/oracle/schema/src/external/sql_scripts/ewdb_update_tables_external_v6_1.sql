/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */
/*                                                          */

/**************************************************
   New or modified tables for the external schema
*************************************************/

/**********************************/ 
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
Create Sequence UH_Info_ExternalSeq START WITH 1 MAXVALUE 999999999 CYCLE;


