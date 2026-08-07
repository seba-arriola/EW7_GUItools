/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */

/* 
 Create the external schema
******************************/

/* 
 Load stored procedures
**************************/

@ewdb_get_external_stationid.sql
/

@ewdb_create_or_alter_station_ext.sql
/

@ewdb_get_idchan_from_ext_stationid.sql
/

@ewdb_station_external_2_chan.sql
/

@ewdb_select_station_external
/

/*** Create trigger to copy external station data into ***/
/*** the infrastructure schema                         ***/
@ewdb_create_trigger_station_external_2_chan.sql
/


/* Urban Hazards external infrastructure info */
@ewdb_create_uhinfo.sql
/


