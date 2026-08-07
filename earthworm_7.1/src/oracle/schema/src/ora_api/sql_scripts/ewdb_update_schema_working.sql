/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */



/**************** STORY ***********************

  We promised to be backward compatible to 
  the earthworm schema in version 6.0. Now we
  have to keep our word. In order to do this,
  all subsequent releases must contain only
  modifications to existing tables or additions
  of new tables.

  The schema loading will consist of loading
  the base schema (optionally -- if the user already
  has v6.0 or later loaded, this can be skipped), 
  followed by incremental updates to the tables
  for each release.  At the end, the views and 
  stored procedures are re-loaded.

**************** STORY ***********************/

/***********************************************
 * Update the base schema to the current version 
 ************************************************/
@ewdb_update_tables_alarms_working
@ewdb_update_tables_external_working
@ewdb_update_tables_infra_working
@ewdb_update_tables_core_working
@ewdb_update_tables_sm_working
@ewdb_update_tables_waveform_working
@ewdb_update_tables_misc_working
@ewdb_update_tables_merge_working
@ewdb_update_tables_combination_constraints_working
@ewdb_update_constants_working
