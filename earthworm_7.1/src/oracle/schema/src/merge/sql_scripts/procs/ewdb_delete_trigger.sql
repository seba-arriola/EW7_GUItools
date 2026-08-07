/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */


CREATE OR REPLACE FUNCTION Delete_ChannelTrigger
(IN_idTrigger number) RETURN number
as

Temp number;

begin
  delete ChannelTrigger where idTrigger = IN_idTrigger;
  return(0);

EXCEPTION
  WHEN OTHERS THEN
    Temp := SQLCODE;
    if Temp = -2292 then
      return(1);
    else
      insert into test 
        values('Delete_ChannelTrigger',IN_idTrigger,Temp);
      return(-1);
    end if;
END;
