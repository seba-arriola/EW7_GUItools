/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */


CREATE OR REPLACE FUNCTION Delete_CodaDur
(IN_idCodaDur number) RETURN number
as

Temp            number;
Temp_idPick     number;
Temp_idTCoda    number;

begin
  select idPick,idTCoda into Temp_idPick,Temp_idTCoda from CodaDur
    where idCodaDur=IN_idCodaDur;
  delete CodaDur where idCodaDur=IN_idCodaDur;
  Temp:=Delete_Pick(Temp_idPick);
  Temp:=Delete_TCoda(Temp_idTCoda);
  return(0);
EXCEPTION
  WHEN OTHERS THEN
    Temp := SQLCODE;
    if Temp = -2292 then
      return(1);
    else
      insert into test values('Delete_CodaDur',IN_idCodaDur,Temp);
      return(-1);
    end if;
END;
