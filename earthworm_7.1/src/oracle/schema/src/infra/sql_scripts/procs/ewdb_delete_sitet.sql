/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */
/*                                                          */


CREATE OR REPLACE PROCEDURE Delete_SiteT
(
 OUT_iRetCode out number,
 IN_idSiteT number
)
as
/* RETURN CODES:
       0:     Success
       1:     Existing Foreign Key Constraint - Ref Integrity
      -1:     Unknown Exception see Debug Table
********************************************/
Temp           number;

BEGIN
  delete SiteT where idSiteT = IN_idSiteT;
  OUT_iRetCode := 0;
EXCEPTION
  WHEN OTHERS THEN
    Temp := SQLCODE;

    if Temp = -2292 then
      OUT_iRetCode := 1;
    else
      insert into test values('DeleteSiteT',IN_idSiteT,Temp);
      OUT_iRetCode := -1;
    end if;
END;
