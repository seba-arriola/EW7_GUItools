/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */

CREATE OR REPLACE PROCEDURE DeleteAlarmsRecipient
(OUT_RetCode OUT number,
 IN_idRecipient number
)
as

Temp  number;

begin

	/* First delete the RecipientDelivery entries */
	delete RecipientDelivery where idRecipient = IN_idRecipient; 

	/* Now delete the Recipient entry */
	delete AlarmsRecipient where idRecipient = IN_idRecipient; 

	OUT_RetCode := 0;

EXCEPTION
  WHEN OTHERS THEN

		Temp := SQLCODE;
		insert into test values('DeleteAlarmsRecipient',IN_idRecipient,Temp);
		OUT_RetCode := 0 - Temp;
END;

