/*                                                          */
/*   THIS FILE IS UNDER CVS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */

CREATE OR REPLACE PROCEDURE DeleteGroupRecipient
(OUT_RetCode OUT number,
 IN_idGroup number,
 IN_idRecipientDelivery number
)
as

Temp  number;

begin

	delete AlarmGroupRecipient where idGroup=IN_idGroup AND idRecipientDelivery=IN_idRecipientDelivery;

	OUT_RetCode := 0;


EXCEPTION
  WHEN OTHERS THEN

		Temp := SQLCODE;
		insert into test values('DeleteGroupRecipient',IN_idRecipientDelivery,Temp);
		OUT_RetCode := 0 - Temp;
END;

