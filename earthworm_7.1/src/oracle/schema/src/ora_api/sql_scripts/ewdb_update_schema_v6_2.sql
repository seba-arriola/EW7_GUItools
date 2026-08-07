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
@ewdb_update_tables_alarms_v6_2
@ewdb_update_tables_external_v6_2
@ewdb_update_tables_infra_v6_2
@ewdb_update_tables_core_v6_2
@ewdb_update_tables_sm_v6_2
@ewdb_update_tables_waveform_v6_2
@ewdb_update_tables_merge_v6_2
@ewdb_update_tables_misc_v6_2
