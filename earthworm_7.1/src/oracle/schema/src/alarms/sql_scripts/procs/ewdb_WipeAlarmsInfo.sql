/*                                                          */
/*   THIS FILE IS UNDER CVS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */

CREATE OR REPLACE PROCEDURE WipeAlarmsInfo
(OUT_RetCode OUT number
)
as

Temp  number;

begin

	delete AlarmsRule;
	delete AuditCustomDelivery;
	delete AuditEmailDelivery;
	delete AuditPagerDelivery;
	delete AuditPhoneDelivery;
	delete AuditQddsDelivery;
	delete AlarmsAudit;
	delete CustomDelivery;
	delete EmailDelivery;
	delete PagerDelivery;
	delete PhoneDelivery;
	delete QddsDelivery;
	delete AlarmGroupRecipient;
	delete AlarmGroup;
	delete RecipientDelivery;
	delete AlarmsRecipient;
	delete Polygon_Vert;
	delete Polygon;

	delete AlarmsFormat 
		where sDescription != 'CUBE'
		AND sDescription != 'hypoTWC'; 

	commit;

	OUT_RetCode := 0;


EXCEPTION
  WHEN OTHERS THEN

		Temp := SQLCODE;
		insert into test values('WipeAlarmsInfo',0,Temp);
		OUT_RetCode := 0 - Temp;
END;

