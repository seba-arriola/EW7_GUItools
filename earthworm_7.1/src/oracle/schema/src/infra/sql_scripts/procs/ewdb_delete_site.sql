/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */
/*                                                          */


CREATE OR REPLACE PROCEDURE Delete_Site
(
 OUT_iRetCode out number,
 IN_idSite number
)
as
/* RETURN CODES:
       0:     Success
       1:     Existing Foreign Key Constraint - Ref Integrity
      -1:     Unknown Exception see Debug Table
********************************************/
Temp           number;

BEGIN
  delete Site where idSite = IN_idSite;
  OUT_iRetCode := 0;
EXCEPTION
  WHEN OTHERS THEN
    Temp := SQLCODE;

    if Temp = -2292 then
      OUT_iRetCode := 1;
    else
      insert into test values('DeleteSite',IN_idSite,Temp);
      OUT_iRetCode := -1;
    end if;
END;
