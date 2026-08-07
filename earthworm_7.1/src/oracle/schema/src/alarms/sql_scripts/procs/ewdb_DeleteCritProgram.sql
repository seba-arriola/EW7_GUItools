/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */

CREATE OR REPLACE PROCEDURE DeleteCritProgram
(OUT_RetCode OUT number,
 IN_idCritProgram number
)
as

Temp  number;
Temp_idRule  number;

begin

	select idRule into Temp_idRule from AlarmsRule
		where idCritProgram = IN_idCritProgram;

	/* If we got to this point, we know that this 
		 CritProgram is still in use -- bail out.	
	 */

	OUT_RetCode := -101;
	
EXCEPTION
  WHEN NO_DATA_FOUND THEN

		OUT_RetCode := -1;

		delete CriteriaProgram where idCritProgram=IN_idCritProgram; 

		OUT_RetCode := 0;

END;

