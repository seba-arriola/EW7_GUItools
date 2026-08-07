/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */

/***********************************************************
  Updates and additions to waveform tables go here
**********************************************************/

/**************************************/
CREATE TABLE SnippetRequest (idSnipReq NUMBER(13,0) NOT NULL,
  tStart NUMBER(14,4) NULL, tEnd NUMBER(14,4) NULL,
  idChan NUMBER(13,0) NULL, idEvent NUMBER(13,0) NULL,
 CONSTRAINT idSnipReq PRIMARY KEY  (idSnipReq)
);
Create Sequence SnippetRequestSeq      START WITH 1 MAXVALUE 999999999 CYCLE CACHE 2;

/**************************************/
CREATE TABLE SnippetRetrievalSchedule (idSnipReq NUMBER(13,0) NOT NULL,
  tNextAttempt NUMBER(10,0) NOT NULL, tInitialRequest NUMBER(10,0) NOT NULL,
  iNumAttempts NUMBER(3,0) NOT NULL, iNumAlreadyAtmptd NUMBER(3,0) NULL,
  idWaveform NUMBER(13,0) NULL,
  iRetCode NUMBER(3,0) NULL, iRequestGroup NUMBER(2,0) NULL,
  iLockTime NUMBER(10,0) NULL, sNote VARCHAR2(200) NULL,
 CONSTRAINT SnippetRS_idSnipReq PRIMARY KEY  (idSnipReq)
);

