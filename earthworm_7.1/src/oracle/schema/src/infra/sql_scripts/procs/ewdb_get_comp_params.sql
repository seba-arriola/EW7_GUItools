/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */
/*                                                          */

CREATE OR REPLACE PROCEDURE Get_Comp_Params
(
 OUT_idCompT OUT number,
 IN_idChan number,
 IN_tTime number,
 OUT_idComp  OUT number,
 OUT_dLat    OUT number,
 OUT_dLon    OUT number,
 OUT_dElev   OUT number,
 OUT_dAzm    OUT number,
 OUT_dDip    OUT number,
 OUT_sSta    OUT varchar,
 OUT_sComp   OUT varchar,
 OUT_sNet    OUT varchar,
 OUT_sLoc    OUT varchar
)
as

/*  errors
                 -1  Unknown Exception
                 -2   ChanT record not found for given time/idchan 
                 -3   CompT record not found for ChanT record 
                 -4   Unknown NO_DATA_FOUND error 
*/


Temp_idChanT       number;
Temp_idCompT       number;
Temp               number;
State              number;

begin

  State := 1;

  select idChanT,idCompT into Temp_idChanT,Temp_idCompT from ChanT
    where idChan = IN_idChan
      and tOn  < IN_tTime
      and tOff > IN_tTime;

  State := 2;

  select idCompT, idComp, dLat, dLon, dElev,
         dAzm, dDip, sSta, sComp, sNet, sLoc
    into OUT_idCompT, OUT_idComp, OUT_dLat, OUT_dLon, OUT_dElev,
         OUT_dAzm, OUT_dDip, OUT_sSta, OUT_sComp, OUT_sNet, OUT_sLoc
    from CompT
    where idCompT = Temp_idCompT;

EXCEPTION
  WHEN NO_DATA_FOUND THEN
    if State = 1 then
      insert into test values('GetCompParams ' || IN_tTime, IN_idChan,State);
      OUT_idCompT := -2;  /* ChanT record not found for given time/idchan */

    elsif State = 2 then
      insert into test values('Get_Comp_Params ' || IN_tTime,Temp_idChanT,State);
      OUT_idCompT := -3;  /* CompT record not found for ChanT record */

    else
      OUT_idCompT := -4;  /* Unknown NDF error */
    end if;

  WHEN OTHERS THEN
    Temp := SQLCODE;
    insert into test values('Get_Comp_Params_Ex',Temp,State);
    OUT_idCompT := -1;  /* Unknown error */
end;

