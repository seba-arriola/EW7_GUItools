/*                                                          */
/*   THIS FILE IS UNDER CVS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */

CREATE OR REPLACE PROCEDURE DeleteGroup
(OUT_RetCode OUT number,
 IN_idGroup number
)
as

Temp  number;

begin

	delete AlarmsRule where idGroup=IN_idGroup;
	delete AlarmGroupRecipient where idGroup=IN_idGroup;
	delete AlarmGroup where idGroup=IN_idGroup; 

	OUT_RetCode := 0;


EXCEPTION
  WHEN OTHERS THEN

		Temp := SQLCODE;
		insert into test values('DeleteGroup',IN_idGroup,Temp);
		OUT_RetCode := 0 - Temp;
END;

