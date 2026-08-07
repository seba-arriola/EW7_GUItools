/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */


CREATE OR REPLACE FUNCTION Delete_Pick
(IN_idPick number
) RETURN number
as

Temp          number;
Temp_tiCore   number;

begin
  delete pick where idPick=IN_idPick;
  return(0);

EXCEPTION
  WHEN OTHERS THEN
    Temp := SQLCODE;
    if Temp = -2292 then
      return(1);
    else
      insert into test values('Delete_Pick',IN_idPick,Temp);
      return(-1);
    end if;
END;
