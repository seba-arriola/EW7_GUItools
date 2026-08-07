/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */

CREATE OR REPLACE PROCEDURE DeleteCustomDelivery
(OUT_RetCode OUT number,
 IN_idDelivery number
)
as

Temp  number;

begin

	delete CustomDelivery where idDelivery = IN_idDelivery; 
	OUT_RetCode := 0;

EXCEPTION
  WHEN OTHERS THEN

		Temp := SQLCODE;
		insert into test values('DeleteCustomDelivery' ,IN_idDelivery, Temp);
		OUT_RetCode := 0 - Temp;
END;

