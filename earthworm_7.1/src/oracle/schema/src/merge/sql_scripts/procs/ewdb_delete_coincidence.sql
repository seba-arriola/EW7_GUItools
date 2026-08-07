/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */


CREATE OR REPLACE FUNCTION Delete_CoincidenceEvent
(IN_idCoincidence number) RETURN number
as

Temp number;

begin
  delete CoincidenceEvent where idCoincidence = IN_idCoincidence;
  return(0);

EXCEPTION
  WHEN OTHERS THEN
    Temp := SQLCODE;
    if Temp = -2292 then
      return(1);
    else
      insert into test 
        values('Delete_CoincidenceEvent',IN_idCoincidence,Temp);
      return(-1);
    end if;
END;
