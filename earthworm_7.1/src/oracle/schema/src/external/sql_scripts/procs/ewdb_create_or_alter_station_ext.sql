/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */
/*    Revision history:                                     */
/*
 *     $Log: ewdb_create_or_alter_station_ext.sql,v $
 *     Revision 1.4  2004/03/17 17:59:16  davidk
 *     Fixed various bugs while testing station_maintenance tool suite.
 *
 ************************************************************/

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

Temp_StationID number;
tempsta varchar(10);
tempchan varchar(10);
tempnet varchar(10);
temploc varchar(10);

State   number;
Temp    number;

begin

State := 0;
tempsta  := RTRIM(IN_sta);
tempchan := RTRIM(IN_chan);
tempnet  := RTRIM(IN_net);
temploc  := RTRIM(IN_loc);

  State := 1;
  Temp_StationID := Get_External_StationID(tempsta,tempchan,tempnet,temploc);

  if(Temp_StationID < 0) then

    State := 2;

    select Station_ExternalSeq.NEXTVAL into Temp_StationID from sys.dual;

    insert into Station_External 
         (StationID, sta, chan, net, loc, lat, lon, elev, description)
     values(Temp_StationID, 
          tempsta, tempchan, tempnet, temploc,
          IN_lat, IN_lon, IN_elev, IN_description);
  end if;

  State := 3;
 
  OUT_StationID := Temp_StationID;


  if IN_lat != 0 then update Station_External 
    set Station_External.Lat = IN_lat where StationID = Temp_StationID; end if;

  if IN_lon != 0 then update Station_External 
    set Station_External.Lon = IN_lon where StationID = Temp_StationID; end if;

  if IN_elev != 0 then update Station_External 
    set Station_External.Elev = IN_elev where StationID = Temp_StationID; end if;

  if IN_description IS NOT NULL then update Station_External 
    set Station_External.Description = IN_description 
     where StationID = Temp_StationID; end if;


EXCEPTION
 WHEN OTHERS THEN
   Temp:= SQLCODE;
   insert into test values('COA_SE ' || IN_sta || '/' || 
                             IN_chan || '/' || IN_net || '/' || IN_loc, 
                           Temp, State);

   OUT_StationID := -1;

end;
