/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */


CREATE OR REPLACE FUNCTION Delete_TCoda  
(
IN_idTCoda number
) RETURN number
as

Temp number;

begin
  delete CodaAmp where idTCoda=IN_idTCoda;
  delete TCoda where idTCoda=IN_idTCoda;
  delete CodaDur where idTCoda = IN_idTCoda;
  return(0);
EXCEPTION
  WHEN OTHERS THEN
    Temp := SQLCODE;
    if Temp = 2292 then /* foreign key still in use */
      return(1);
    else
      insert into test values('DeleteTCoda',IN_idTCoda,Temp);
      return(-1);
    end if;
END;
