/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */

CREATE OR REPLACE TRIGGER Station_External_2_Chan
 AFTER INSERT OR UPDATE ON Station_External
 FOR EACH ROW 

  declare
   Temp_idChan number;

  begin

    Station_External_2_Chan (Temp_idChan,:new.STATIONID,
		                         :new.sta, :new.chan,:new.net, :new.loc, 
                             :new.Lat, :new.Lon, :new.Elev, :new.description);

    if Temp_idChan <= 0 then
      insert into test values('Station_External_2_Chan',Temp_idChan,:new.StationID);
    end if;
  end;


