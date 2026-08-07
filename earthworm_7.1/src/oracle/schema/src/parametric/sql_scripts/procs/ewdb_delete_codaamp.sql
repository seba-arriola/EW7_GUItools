/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */


CREATE OR REPLACE FUNCTION Delete_CodaAmp
(IN_idCodaAmp number) RETURN number
as

Temp number;

begin
  delete CodaAmp where idCodaAmp=IN_idCodaAmp;
  return(0);
EXCEPTION
  WHEN OTHERS THEN
    Temp := SQLCODE;
    if Temp = -2292 then
      return(1);
    else
      insert into test values('Delete_CodaAmp',IN_idCodaAmp,Temp);
      return(-1);
    end if;
END;
