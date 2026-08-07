/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */
/*                                                          */


CREATE OR REPLACE PROCEDURE Delete_PZ_For_CTF
(OUT_RetCode out number,
 IN_idCTF number
)
as
/* RETURN CODES:
      -1:     Unknown Exception see Debug Table
********************************************/
Temp           number;

BEGIN
  delete from PolesAndZeroes where idCTF = IN_idCTF;
  OUT_RetCode := 0;
EXCEPTION
  WHEN OTHERS THEN
  Temp := SQLCODE;
  insert into test values('DeletePZForCTF',0,Temp);
  OUT_RetCode := -1;
END;
