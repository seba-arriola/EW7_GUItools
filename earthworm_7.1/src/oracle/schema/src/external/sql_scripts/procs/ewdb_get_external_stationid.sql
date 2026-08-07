/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */
/*    Revision history:                                     */
/*
 *     $Log: ewdb_get_external_stationid.sql,v $
 *     Revision 1.1  2004/03/17 17:59:16  davidk
 *     Initial revision
 *
 ************************************************************/

CREATE OR REPLACE FUNCTION Get_External_StationID
(IN_sta            varchar,
 IN_chan           varchar,
 IN_net            varchar,
 IN_loc            varchar
) RETURN number

as

Temp_StationID  number;
begin

    /* DK 021704  Adding support / fixing bug for Location code*/
select StationID into Temp_StationID from Station_External 
            where sta  = IN_sta
              and chan = IN_chan
              and net  = IN_net
              AND (loc = IN_loc OR (loc IS NULL and IN_loc IS NULL));

  return(Temp_StationID);

EXCEPTION
 WHEN NO_DATA_FOUND THEN

   return(-1);

end;
