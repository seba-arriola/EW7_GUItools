/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */



/* 
  Create tables for all schemas
***********************************/
@ewdb_create_tables_core

@ewdb_create_tables_infra

@ewdb_create_tables_external

@ewdb_create_tables_waveform

@ewdb_create_tables_sm

@ewdb_create_tables_alarms


/* 
  Create other objects for all schemas
****************************************/
@ewdb_create_core_schema

@ewdb_create_infra_schema

@ewdb_create_external_schema

@ewdb_create_waveform_schema

@ewdb_create_sm_schema

@ewdb_create_alarms_schema
