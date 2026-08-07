/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */

CREATE OR REPLACE PROCEDURE Station_External_2_Chan
(
 OUT_idChan OUT number,
 IN_StationID  number,
 IN_sSta       varchar,
 IN_sComp      varchar,
 IN_sNet       varchar,
 IN_sLoc       varchar,
 IN_dLat       number,
 IN_dLon       number,
 IN_dElev      number,
 IN_sDescription varchar  /* we do nothing with desc*/
)
as



Temp_StationID     number;
Temp_idChan        number;
Temp_idChanT       number;
Temp_idComp        number;
Temp_idCompT       number;

Temp               number;
State              number;

begin

  State := 1;

  Temp_StationID := IN_StationID;

  State := 2;

  Get_idChan_From_Ext_StationID(Temp_idChan, Temp_StationID, 1, IN_sSta, IN_sComp, IN_sNet, IN_sLoc);
  if Temp_idChan <= 0 then
    OUT_idChan := -3;
    return;
  end if;

  State := 3;

  select count(idchan) into Temp 
   from ALL_STATION_INFO 
   where idChan = Temp_idChan;

  /* if information already exists for this channel, then let it lie.
     Don't overwrite potentially good information, with our garbage.
     The user/programmer will have to find a better way to update information.
     We don't want to blow away all of the historical component information,
     just because someone accidentally runs stalist_XXX_2_ora!!!  DK 021704
   ******************************************************************************/
  if(Temp > 0) then
    OUT_idChan := Temp_idChan;
    return;
  end if;
  
 
  Set_Comp_Params(Temp_idCompT,IN_sSta,IN_sComp,IN_sNet,IN_sLoc,
                          0 /*IN_tOn*/,
                          9999999999.99 /*IN_tOff*/,
                          IN_dLat,IN_dLon,IN_dElev,
                          NULL /*IN_dAzm*/,
                          NULL /*IN_dDip*/,
                          ''/*Comment*/);
  if Temp_idCompT < 0 then
    /* error */
    OUT_idChan := -1000 + Temp_idCompT;
    return;
  end if;

  /* this is a hack (brutal) but the entire function is a hack,
     so maybe it's ok  19990901 dk */
  Set_Chan_Params(Temp_idChanT,Temp_idChan,Temp_idCompT,
                          0 /*IN_tOn*/,
                          9999999999.99 /*IN_tOff*/,
                          NULL,NULL,NULL);

  if Temp_idChanT <= 0 then
    OUT_idChan := -6;
    return;
  end if;


  OUT_idChan := Temp_idChan;

EXCEPTION
  WHEN OTHERS THEN
    Temp := SQLCODE;
    insert into test values('station_external_2_chan',Temp,State);
    OUT_idChan := -1;
end;
                    


