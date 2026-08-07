/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */
/*                                                          */

CREATE OR REPLACE PROCEDURE CreateOrUpdateAlarmsAudit
(OUT_idAudit out number,
 IN_idEvent number,
 IN_bAuto number,
 IN_idRecipient number,
 IN_idFormat number,
 IN_idDelivery number,
 IN_sTableName varchar,
 IN_tAlarmDeclared number,
 IN_tAlarmExecuted number,
 IN_sInvocationString varchar,
 IN_idAudit number,
 IN_idCore number,
 IN_iAlarmType number,
 IN_idGroup number,
 IN_idRecipientDelivery number
)
as
/* Return Codes for OUT_idAudit:
                  >0  DB idAudit
*/
Temp_idAudit			number;

begin

  /**************************************************/
  /* Check to see if this audit already exists      */
  /**************************************************/
	select idAudit into Temp_idAudit from AlarmsAudit
		where idAudit = IN_idAudit;

	/* 
		If so, update the rule
	 */
	OUT_idAudit := Temp_idAudit;

	if IN_tAlarmDeclared != 0 then 
		update AlarmsAudit
			set AlarmsAudit.tAlarmDeclared = IN_tAlarmDeclared
				where idAudit = Temp_idAudit;
	end if;

	if IN_tAlarmExecuted != 0 then 
		update AlarmsAudit
			set AlarmsAudit.tAlarmExecuted = IN_tAlarmExecuted
				where idAudit = Temp_idAudit;
	end if;



EXCEPTION
	WHEN NO_DATA_FOUND THEN

		/* Insert the new audit */
		select AlarmsAuditSeq.NEXTVAL into Temp_idAudit from sys.dual;

		insert into AlarmsAudit 
			(idAudit, idEvent, idCore, iAlarmType, bAuto, 
					idRecipient, idFormat, idDelivery, sTableName, 
					tAlarmDeclared, tAlarmExecuted, sInvocationString,
					idGroup, idRecipientDelivery)

			values (Temp_idAudit, IN_idEvent, IN_idCore, IN_iAlarmType, IN_bAuto, 
					IN_idRecipient, IN_idFormat, IN_idDelivery, IN_sTableName, 
					IN_tAlarmDeclared, IN_tAlarmExecuted, IN_sInvocationString,
					IN_idGroup, IN_idRecipientDelivery);


		OUT_idAudit := Temp_idAudit;
end;
