/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */

/******************************************************
 
  This is a script file for creating an Earthworm database 
   
   You may want to to change the following:
    - password for user system (default: earthworm)
    - password for user ewdb_main (default: main)
    - service name for your EWDB Oracle instance (depends on Oracle setup).
 
  Before executing this script you should install Oracle
   and the EWDB instance. Refer to the scripts under
   $EW_HOME/$EW_VERSION/ora_instance_install to install  
   an EWDB instance on the hardware supported by the Earthworm
   Group.
  
  To execute this script, connect to the database as user system
   from the directory containing all of the sql scripts:
    
    cd $EW_HOME/$EW_VERSION/src/oracle/schema/sql_scripts
    sqlplus system/earthworm@service.name
    SQL> @ewdb_create_database
  
*******************************************************/

/* CHANGE -- this is where the output of this script will be logged */
spool /tmp/output

/******************************************/
/* Create DB Objects for EWDB Schema      */
/******************************************/



/*******************************************************
  The scripts below will create the earthworm schema. You will
   want to select those scripts that reflect the current version
   of your schema and the version that you are wishing to install.
 
    If you are installing a brand new database, you should load:
      - ewdb_create_base_tablespaces (connected as system)
      - ewdb_create_base_tables (connected as ewdb_main)
      - any upgrade scripts up to the current version (connected as ewdb_main)
 
    If you are upgrading from v6.0 or later, DO NOT select 
    ewdb_create_base_tablespaces and ewdb_create_base_tables. 
    Instead, run only the incremental upgrade 
    scripts from your current to the desired version.
*******************************************************/
/* CHANGE -- edit the following scripts if this is a non-standard installation */

/* 
 Uncomment the line below to build the tablespace files
 and create the user ewdb_main 
*/
/* @ewdb_create_base_tablespaces */


/*** CHANGE ewdb_main's password and service name */ 
connect ewdb_main/main@service.name

/* 
 Uncomment the line below to load the base schema
*/
/* @ewdb_create_base_tables */


/* 
 Uncomment all the update scripts necessary to bring your
 database up to the desired version
*/
@ewdb_update_schema_v6_1

@ewdb_update_schema_v6_2

/*
 Replace the next line with v6_x for version 6.x release
*/
@ewdb_update_schema_working

/* Finally, load the views and stored procedures */
@ewdb_load_utils




/**************************************************/
/*   Load installation source names               */
/*   CHANGE the sql script if different source    */
/*   names are desired.                           */
/**************************************************/

@instid_2_networkname

spool off

