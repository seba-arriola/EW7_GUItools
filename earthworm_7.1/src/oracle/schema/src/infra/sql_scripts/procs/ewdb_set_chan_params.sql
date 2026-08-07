/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */

CREATE OR REPLACE PROCEDURE Set_Chan_Params
(
 OUT_idChanT OUT number,
 IN_idChan number,
 IN_idCompT number,
 IN_tOn number,
 IN_tOff number,
 IN_idDeviceSlot number,
 IN_iPlexor number,
 IN_sComment varchar
)
as

Temp_idChanT       number;
Temp_tOn           number;
Temp_tOff          number;
Temp_idComment     number;

Temp               number;
State              number;
Temp_RetCode       number;
begin

  State := 2;

  Create_ChanT(Temp_RetCode,Temp_idChanT,IN_idChan,IN_tOn,IN_tOff,
               IN_idCompT,IN_idDeviceSlot,IN_iPlexor,IN_sComment,0/* Don't FORCE */);
  if Temp_RetCode < 0 then
    /* Error */
    OUT_idChanT := -100 + Temp_RetCode;
    return;
  end if;

  OUT_idChanT := Temp_idChanT;

  
EXCEPTION
  WHEN OTHERS THEN
    Temp := SQLCODE;
    insert into test values('Set_Chan_Params_ex',Temp,State);
    OUT_idChanT := -1;   /* Unknown error */

END;

