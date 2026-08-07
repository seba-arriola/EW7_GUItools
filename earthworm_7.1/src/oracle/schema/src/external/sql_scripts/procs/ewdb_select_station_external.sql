/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */
/*
 *     $Log: ewdb_select_station_external.sql,v $
 *     Revision 1.3  2004/03/17 17:59:16  davidk
 *     Fixed various bugs while testing station_maintenance tool suite.
 *
 ************************************************************/

CREATE OR REPLACE PROCEDURE Select_Station_External
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

tempsta varchar(10);
tempchan varchar(10);
tempnet varchar(10);
temploc varchar(10);
begin

tempsta := RTRIM(IN_sta);
tempchan := RTRIM(IN_chan);
tempnet := RTRIM(IN_net);
temploc := RTRIM(IN_loc);

 OUT_StationID := Get_External_StationID(tempsta,tempchan,tempnet,temploc);

EXCEPTION
 WHEN OTHERS THEN
   OUT_StationID := -1;
end;
