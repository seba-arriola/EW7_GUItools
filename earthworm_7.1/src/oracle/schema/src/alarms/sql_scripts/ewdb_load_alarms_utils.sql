/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */

/************************
   Create Utility Views 
**************************/

CREATE OR REPLACE VIEW RuleDeliveryRecipient AS
    select idRule, AlarmsRecipient.idRecipient, sTableName, idDelivery,
                        RecipientDelivery.idRecipientDelivery
    from AlarmsRecipient, RecipientDelivery, AlarmsRule
    where AlarmsRecipient.idRecipient = RecipientDelivery.idRecipient
        AND AlarmsRule.idRecipientDelivery = RecipientDelivery.idRecipientDelivery;

CREATE OR REPLACE VIEW ALL_POLYGON_INFO as 
  select * from polygon;

CREATE OR REPLACE VIEW ALL_PhoneDelivery_INFO as 
  select * from PhoneDelivery;

CREATE OR REPLACE VIEW ALL_AuditPhoneDelivery_INFO as 
  select * from AuditPhoneDelivery;

CREATE OR REPLACE VIEW ALL_Polygon_Vert_INFO as 
  select * from Polygon_Vert;

CREATE OR REPLACE VIEW ALL_QddsDelivery_INFO as 
  select * from QddsDelivery;

CREATE OR REPLACE VIEW ALL_AuditQddsDelivery_INFO as 
  select * from AuditQddsDelivery;

CREATE OR REPLACE VIEW ALL_RecipientDelivery_INFO as 
  select * from RecipientDelivery;

CREATE OR REPLACE VIEW ALL_PagerDelivery_INFO as 
  select * from PagerDelivery;

CREATE OR REPLACE VIEW ALL_EmailDelivery_INFO as 
  select * from EmailDelivery;

CREATE OR REPLACE VIEW ALL_AuditEmailDelivery_INFO as 
  select * from AuditEmailDelivery;

CREATE OR REPLACE VIEW ALL_CustomDelivery_INFO as 
  select * from CustomDelivery;

CREATE OR REPLACE VIEW ALL_AuditCustomDelivery_INFO as 
  select * from AuditCustomDelivery;

CREATE OR REPLACE VIEW ALL_AlarmsRule_INFO_W_Format  as 
  select ar.*, af.sdescription, af.sfmtinsert, af.sfmtdelete 
  from AlarmsRule ar, AlarmsFormat af
  where ar.idFormat = af.idFormat;

CREATE OR REPLACE VIEW ALL_RuleDeliveryRecipient_INFO  as 
  select * from RuleDeliveryRecipient;

CREATE OR REPLACE VIEW ALL_AlarmsRecipient_INFO as 
  select * from AlarmsRecipient;

CREATE OR REPLACE VIEW ALL_AlarmGroupRecipient_INFO as 
  select * from AlarmGroupRecipient;

CREATE OR REPLACE VIEW ALL_AlarmGroup_INFO as 
  select * from AlarmGroup;

CREATE OR REPLACE VIEW ALL_CriteriaProgram_INFO as 
  select * from CriteriaProgram;

CREATE OR REPLACE VIEW ALL_AlarmsAR_INFO_W_Format  as 
  select aa.*, af.sdescription sFmtDescription, af.sfmtinsert, af.sfmtdelete ,
         ar.sDescription sUsrDescription, ar.dPriority, ar.bActive
   from AlarmsAudit aa, AlarmsRecipient ar, AlarmsFormat af
  where aa.idFormat = af.idFormat
    and aa.idRecipient = ar.idRecipient;



/************************
 Load Stored Procedures 
**************************/

/* Create Section */

@ewdb_CreateOrUpdateAlarmsFormat
/

@ewdb_CreateAlarmsRecipient
/

@ewdb_CreateEmailDelivery
/

@ewdb_CreateOrUpdateEmailDelivery
/

@ewdb_CreateOrUpdateAlarmsAudit
/

@ewdb_CreateOrUpdateAlarmsCriteria
/

@ewdb_CreateOrUpdateAlarmsRule
/

@ewdb_CreatePagerDelivery
/

@ewdb_CreatePhoneDelivery
/

@ewdb_CreatePolygon
/

@ewdb_CreatePolygonVert
/

@ewdb_CreateQddsDelivery
/

@ewdb_CreateCustomDelivery
/

@ewdb_CreateAlarmsGroup
/

@ewdb_CreateGroupRecipient
/

/* Delete Section */
@ewdb_DeleteAlarmsFormat.sql
/

@ewdb_DeleteAlarmsRule.sql
/

@ewdb_DeleteAlarmsRecipient.sql
/

@ewdb_DeleteCritProgram.sql
/

@ewdb_DeleteEmailDelivery.sql
/

@ewdb_DeletePagerDelivery.sql
/

@ewdb_DeletePhoneDelivery.sql
/

@ewdb_DeletePolygon.sql
/

@ewdb_DeleteQddsDelivery.sql
/

@ewdb_DeleteCustomDelivery.sql
/

@ewdb_delete_alarms_by_event.sql
/

@ewdb_DeleteGroup.sql
/

@ewdb_DeleteGroupRecipient.sql
/

@ewdb_WipeAlarmsInfo.sql
/

/* Misc Section */
@ewdb_GetAndIncrementCubeVersion.sql
/

