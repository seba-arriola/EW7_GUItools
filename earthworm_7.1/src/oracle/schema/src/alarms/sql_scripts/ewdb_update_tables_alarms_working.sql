/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */

/***********************************************************
  New alarms tables:
**********************************************************/

CREATE TABLE Polygon (idPolygon NUMBER(13,0) NOT NULL, 
 sPolygonName VARCHAR2(256) NOT NULL, 
 CONSTRAINT idPolygon PRIMARY KEY (idPolygon));
Create Sequence PolygonSeq  START WITH 1 MAXVALUE 999999999 CYCLE CACHE 2;

CREATE TABLE Polygon_Vert (idVertex NUMBER(13,0) NOT NULL,
 idPolygon NUMBER(13,0) NOT NULL, 
 dLat NUMBER(10,6) NOT NULL, 
 dLon NUMBER(10,6) NOT NULL, 
 iOrder NUMBER(13,0) NOT NULL, 
 CONSTRAINT idVertex PRIMARY KEY (idVertex));
Create Sequence PolygonVertSeq  START WITH 1 MAXVALUE 999999999 CYCLE CACHE 2;

CREATE TABLE AlarmGroup (idGroup NUMBER(13,0) NOT NULL, 
 sGroupName VARCHAR2(256) NOT NULL, 
 bActive NUMBER(13,0) NOT NULL, 
 CONSTRAINT idGroup PRIMARY KEY (idGroup));
Create Sequence GroupSeq  START WITH 1 MAXVALUE 999999999 CYCLE CACHE 2;

CREATE TABLE AlarmGroupRecipient (idGroupRecipient NUMBER(13,0) NOT NULL,
 idGroup NUMBER(13,0) NOT NULL, 
 idRecipientDelivery NUMBER(13,0) NOT NULL, 
 bActive NUMBER(13,0) NOT NULL, 
 CONSTRAINT idGroupRecipient PRIMARY KEY (idGroupRecipient));
Create Sequence GroupRecipientSeq  START WITH 1 MAXVALUE 999999999 CYCLE CACHE 2;


/***********************************************************
  Column Updates and additions to alarms tables:
**********************************************************/


/* Additions to the Audit table to support NEIC alarms */

ALTER TABLE AlarmsAudit  ADD (idCore NUMBER(13,0) NULL);
ALTER TABLE AlarmsAudit  ADD (iAlarmType NUMBER(13,0) NULL);
ALTER TABLE AlarmsAudit  ADD (idGroup NUMBER(13,0) NULL);
ALTER TABLE AlarmsAudit  ADD (idRecipientDelivery NUMBER(13,0) NULL);

/* Additions to the AlarmsRule table to support Hydra alarms */

ALTER TABLE AlarmsRule  ADD (idPolygon NUMBER(13,0) NULL);
ALTER TABLE AlarmsRule  ADD (bUseMag NUMBER(13,0) NULL);
ALTER TABLE AlarmsRule  ADD (iPhases NUMBER(13,0) NULL);
ALTER TABLE AlarmsRule  ADD (idGroup NUMBER(13,0) NULL);
ALTER TABLE AlarmsRule  ADD (bSecondary NUMBER(13,0) NULL);
ALTER TABLE AlarmsRule  MODIFY (idRecipientDelivery NULL);


ALTER TABLE AlarmsRecipient  ADD (bActive NUMBER(13,0) NULL);

/***********************************************************
  Constraint additions to alarms tables:
**********************************************************/

ALTER TABLE AlarmsRule  ADD (CONSTRAINT idGroup_PK FOREIGN KEY (idGroup) 
						REFERENCES AlarmGroup(idGroup));


/***********************************************************
  Index additions to alarms tables:
**********************************************************/
