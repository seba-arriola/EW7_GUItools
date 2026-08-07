/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */

CREATE OR REPLACE PROCEDURE DeleteAlarmsFormat
(OUT_RetCode OUT number,
 IN_idFormat number
)
as

Temp  number;

begin

	delete AlarmsFormat where idFormat=IN_idFormat; 
	OUT_RetCode := 0;

EXCEPTION
  WHEN OTHERS THEN

		Temp := SQLCODE;
		insert into test values('DeleteAlarmsFormat',IN_idFormat,Temp);
		OUT_RetCode := 0 - Temp;
END;

