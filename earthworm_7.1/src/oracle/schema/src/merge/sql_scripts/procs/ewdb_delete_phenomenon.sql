/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */

CREATE OR REPLACE PROCEDURE Delete_Phenomenon
(OUT_RetCode OUT number,
 IN_idPh number
)
as

	Temp number;

begin

	delete phenomena 
		where idPh = IN_idPh;

        OUT_RetCode := 1;

EXCEPTION
  WHEN OTHERS THEN
	OUT_RetCode := -1;

    insert into test values('Delete_Phenomenon_excep',IN_idPh, IN_idPh);
END;

