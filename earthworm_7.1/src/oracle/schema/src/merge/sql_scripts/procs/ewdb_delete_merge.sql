/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */

CREATE OR REPLACE PROCEDURE Delete_Merge
(OUT_RetCode OUT number,
 IN_idPh      number,
 IN_idEvent number
)
as

	Temp number;

begin

	delete merge 
		where idPh = IN_idPh 
		and idEvent = IN_idEvent;

        OUT_RetCode := 1;

EXCEPTION
  WHEN OTHERS THEN
	OUT_RetCode := -1;

    insert into test values('Delete_Merge_excep',IN_idPh, IN_idEvent);
END;

