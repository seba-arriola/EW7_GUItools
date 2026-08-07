/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */
/*                                                          */


CREATE OR REPLACE PROCEDURE Delete_CompT
(
 OUT_iRetCode out number,
 IN_idCompT number
)
as
/* RETURN CODES:
       0:     Success
       1:     Existing Foreign Key Constraint - Ref Integrity
      -1:     Unknown Exception see Debug Table
********************************************/
Temp           number;

BEGIN
  delete from CompT where idCompT = IN_idCompT;
  OUT_iRetCode := 0;
EXCEPTION
  WHEN OTHERS THEN
    if Temp = -2292 then
      OUT_iRetCode := 1;
      return;
    else
      Temp := SQLCODE;
      insert into test values('DeleteCompT',IN_idCompT,Temp);
      OUT_iRetCode := -1;
    end if;
END;
