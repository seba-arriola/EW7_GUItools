/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */

CREATE OR REPLACE PROCEDURE Get_idChanT
(
 OUT_RetCode OUT number,
 OUT_idChanT OUT number,
 IN_idChan number,
 IN_tTime number
)
as

Temp               number;
State              number;
begin

  State := 0;

  select min(idChanT) into OUT_idChanT from ChanT 
   where idChan = IN_idChan 
     AND tOff  >= IN_tTime
     AND tOn   <= IN_tTime;

  OUT_RetCode := 0;

EXCEPTION
  WHEN NO_DATA_FOUND THEN
    OUT_RetCode := -2;
    /* No ChanT record found for the idChanT */

  WHEN OTHERS THEN
    Temp := SQLCODE;
    insert into test values('Get_idChanT',Temp,State);
    OUT_RetCode := -1;
END;
