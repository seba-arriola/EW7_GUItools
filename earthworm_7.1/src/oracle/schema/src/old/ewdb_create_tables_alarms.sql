/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */

/* Create Tables and Sequences */

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

/* Insert the hardcoded CUBE format */
insert into AlarmsFormat values (1, 'CUBE',
        'Predefined CUBE Insertion Format. DO NOT CHANGE!~End~',
        'Predefined CUBE Deletion Format. DO NOT CHANGE!~End~');

Create Sequence AlarmsFormatSeq  START WITH 2 MAXVALUE 999999999 CYCLE CACHE 2;



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


CREATE OR REPLACE VIEW RuleDeliveryRecipient AS
	select idRule, AlarmsRecipient.idRecipient, sTableName, idDelivery, 
						RecipientDelivery.idRecipientDelivery 
	from AlarmsRecipient, RecipientDelivery, AlarmsRule 
	where AlarmsRecipient.idRecipient = RecipientDelivery.idRecipient 
		AND AlarmsRule.idRecipientDelivery = RecipientDelivery.idRecipientDelivery;


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
