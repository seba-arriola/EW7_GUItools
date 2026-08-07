/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */

CREATE OR REPLACE PROCEDURE DeletePhoneDelivery
(OUT_RetCode OUT number,
 IN_idDelivery number
)
as

Temp  number;

begin

	delete PhoneDelivery where idDelivery=IN_idDelivery; 
	OUT_RetCode := 0;

	delete RecipientDelivery
		where idDelivery=IN_idDelivery
		AND sTableName='phone';
	OUT_RetCode := 0;

EXCEPTION
  WHEN NO_DATA_FOUND THEN

	OUT_RetCode := 0;

  WHEN OTHERS THEN

		Temp := SQLCODE;
		insert into test values('DeletePhoneDelivery',IN_idDelivery,Temp);
		OUT_RetCode := 0 - Temp;
END;

