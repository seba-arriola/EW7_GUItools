/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */

CREATE OR REPLACE PROCEDURE DeleteAlarmsRule
(OUT_RetCode OUT number,
 IN_idRule number
)
as

Temp  number;

begin

	delete AlarmsRule where idRule=IN_idRule; 
	OUT_RetCode := 0;

EXCEPTION
  WHEN OTHERS THEN

		Temp := SQLCODE;
		insert into test values('DeleteAlarmsRule',IN_idRule,Temp);
		OUT_RetCode := 0 - Temp;
END;

