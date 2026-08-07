/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */

CREATE OR REPLACE PROCEDURE Create_Or_Alter_Station_Ext
(OUT_StationID out number,
IN_sta varchar,
IN_chan varchar,
IN_net varchar,
IN_loc varchar,
IN_lat number,
IN_lon number,
IN_elev number,
IN_description varchar
)

as

TempStationID number;
tempsta varchar(10);
tempchan varchar(10);
tempnet varchar(10);
temploc varchar(10);
begin

tempsta := RTRIM(IN_sta);
tempchan := RTRIM(IN_chan);
tempnet := RTRIM(IN_net);
temploc := RTRIM(IN_loc);

select StationID into TempStationID from Station_External 
            where sta  =tempsta
              and chan = tempchan
              and net  = tempnet
              and loc IS NULL;


OUT_StationID :=  TempStationID;

if IN_lat != 0 then update Station_External 
  set Station_External.Lat = IN_lat where StationID = TempStationID; end if;

if IN_lon != 0 then update Station_External 
  set Station_External.Lon = IN_lon where StationID = TempStationID; end if;

if IN_elev != 0 then update Station_External 
  set Station_External.Elev = IN_elev where StationID = TempStationID; end if;

if IN_description IS NOT NULL then update Station_External 
  set Station_External.Description = IN_description 
   where StationID = TempStationID; end if;


EXCEPTION
 WHEN NO_DATA_FOUND THEN

  select Station_ExternalSeq.NEXTVAL into TempStationID from sys.dual;

  insert into Station_External 
         (StationID, sta, chan, net, loc, lat, lon, elev, description)
   values(TempStationID, 
          tempsta, tempchan, tempnet, temploc,
          IN_lat, IN_lon, IN_elev, IN_description);

   OUT_StationID := TempStationID;

end;
