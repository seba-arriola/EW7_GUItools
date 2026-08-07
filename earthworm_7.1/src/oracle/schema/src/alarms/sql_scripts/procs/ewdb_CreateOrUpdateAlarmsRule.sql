/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */
/*                                                          */

CREATE OR REPLACE PROCEDURE CreateOrUpdateAlarmsRule
(OUT_idRule out number,
 IN_dMag number,
 IN_bAuto number,
 IN_idFormat number,
 IN_idCritProgram number,
 IN_idRecipientDelivery number,
 IN_idRule number,
 IN_idPolygon number,
 IN_bUseMag number,
 IN_iPhases number,
 IN_idGroup number,
 IN_bSecondary number
)
as
/* Return Codes for OUT_idRule:
                  >0  DB idRule
                  -1  Unknown Error
*/
Temp_idRule					number;
Temp_idRecipientDelivery	number;
Temp_idGroup				number;

begin

	if (IN_idRecipientDelivery <= 0) then
		Temp_idRecipientDelivery := NULL;
	else
		Temp_idRecipientDelivery := IN_idRecipientDelivery;
	end if;

	if (IN_idGroup <= 0) then
		Temp_idGroup := NULL;
	else
		Temp_idGroup := IN_idGroup;
	end if;

  /**************************************************/
  /* Check to see if this rule already exists       */
  /**************************************************/
	select idRule into Temp_idRule from AlarmsRule
		where idRule = IN_idRule;

	/* 
		If so, update the rule
	 */
	OUT_idRule := Temp_idRule;

	update AlarmsRule
		set AlarmsRule.dMag = IN_dMag,
			AlarmsRule.bAuto = IN_bAuto,
			AlarmsRule.idFormat = IN_idFormat,
			AlarmsRule.idCritProgram = IN_idCritProgram,
			AlarmsRule.idRecipientDelivery = Temp_idRecipientDelivery,
			AlarmsRule.idPolygon = IN_idPolygon,
			AlarmsRule.bUseMag = IN_bUseMag,
			AlarmsRule.iPhases = IN_iPhases,
			AlarmsRule.idGroup = Temp_idGroup,
			AlarmsRule.bSecondary = IN_bSecondary
		where idRule = Temp_idRule;
			

EXCEPTION
	WHEN NO_DATA_FOUND THEN


		/* Insert the new rule */
		select AlarmsRuleSeq.NEXTVAL into Temp_idRule from sys.dual;

		insert into AlarmsRule 
			(idRule, dMag, bAuto, idFormat, idCritProgram, idRecipientDelivery, idPolygon, bUseMag, iPhases, idGroup, bSecondary)
			values (Temp_idRule, IN_dMag, IN_bAuto, IN_idFormat,
							IN_idCritProgram, Temp_idRecipientDelivery, IN_idPolygon, IN_bUseMag, IN_iPhases, Temp_idGroup, IN_bSecondary);


		OUT_idRule := Temp_idRule;


end;
