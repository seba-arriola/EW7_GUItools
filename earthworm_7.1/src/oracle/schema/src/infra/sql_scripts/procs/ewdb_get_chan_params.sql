/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */
/*                                                          */

CREATE OR REPLACE PROCEDURE Get_Chan_Params
(
 IN_idChanT        number,
 IN_idChan         number,
 IN_tOn            number,
 IN_tOff           number,
 OUT_iRetCode  OUT number,
 OUT_idChanT   OUT number,
 OUT_idChan    OUT number,
 OUT_idComp    OUT number,
 OUT_idCompT   OUT number,
 OUT_dLat      OUT number,
 OUT_dLon      OUT number,
 OUT_dElev     OUT number,
 OUT_dAzm      OUT number,
 OUT_dDip      OUT number,
 OUT_tOn       OUT number,
 OUT_tOff      OUT number,
 OUT_sSta      OUT varchar,
 OUT_sComp     OUT varchar,
 OUT_sNet      OUT varchar,
 OUT_sLoc      OUT varchar
)
as

/*  errors
                 -1  Unknown Exception
                 -2  Unexpected NO_DATA_FOUND error 
                 -3  IN_idChanT and IN_idChan were both invalid
                 -4  ChanT record not found for given idChanT
                 -5  ChanT record not found for given time/idchan 
*/


Temp_idChanT       number;
Temp               number;
State              number;
Temp_iRetCode      number := 0;

begin

  State := 1;

  if(IN_idChanT > 0) then
    State := 11;
    Temp_idChanT := IN_idChanT;
    select count(idChanT) into Temp
     from ChanT
     where idChanT = IN_idChanT;
  elsif(IN_idChan > 0) then
    State := 12;
    select count(idChanT) into Temp
     from ChanT
     where idChan = IN_idChan
       AND tOn <= IN_tOff 
       AND tOff > IN_tOn;

    if(Temp = 0) then
      OUT_iRetCode := -5;
      return;
    elsif(Temp > 1) then
      Temp_iRetCode := 1;
    end if;

    select idChanT into Temp_idChanT
     from ChanT
     where idChan = IN_idChan
       AND tOn <= IN_tOff 
       AND tOff > IN_tOn;
  else
    State := 13;
    /* neither IN_idChanT nor IN_idChan valid */
    OUT_iRetCode := -3;
    return;
  end if;

  State := 2;

  select idChanT, idChan, idCompT, idComp, dLat, dLon, dElev,
         dAzm, dDip, tOn, tOff, sSta, sComp, sNet, sLoc
    into OUT_idChanT, OUT_idChan, OUT_idCompT, OUT_idComp, 
         OUT_dLat, OUT_dLon, OUT_dElev, OUT_dAzm, OUT_dDip, OUT_tOn, OUT_tOff,
         OUT_sSta, OUT_sComp, OUT_sNet, OUT_sLoc
    from ALL_STATION_INFO
    where idChanT = Temp_idChanT;

  State := 3;
   OUT_iRetCode := Temp_iRetCode;


EXCEPTION
  WHEN NO_DATA_FOUND THEN
    if State = 11 then
      OUT_iRetCode := -4;  /* ChanT record not found for given idChanT */
    else
    Temp := SQLCODE;
    insert into test values('Get_Chan_Params_NDF',Temp,State);
    OUT_idChanT := -2;  /* Unexpected NDF error*/
    end if;

  WHEN OTHERS THEN
    Temp := SQLCODE;
    insert into test values('Get_Chan_Params_Ex',Temp,State);
    OUT_idChanT := -1;  /* Unknown error */
end;

