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

@ewdb_create_or_alter_station_ext.sql
/

@ewdb_get_idchan_from_ext_stationid.sql
/

@ewdb_station_external_2_chan.sql
/

/*** Create trigger to copy external station data into ***/
/*** the infrastructure schema                         ***/
@ewdb_create_trigger_station_external_2_chan.sql
/


/* Urban Hazards external infrastructure info */
@ewdb_create_uhinfo.sql
/


