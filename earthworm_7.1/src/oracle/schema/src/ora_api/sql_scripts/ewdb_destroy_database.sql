/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */



drop user ew_auto cascade;
drop user ewdb_main cascade;


ALTER ROLLBACK SEGMENT EWDB_ROLLBACK OFFLINE;

drop rollback segment ewdb_rollback;

drop tablespace ewdb_rbckspace;
drop tablespace ewdb_corespace;
drop tablespace ewdb_infraspace;
drop tablespace ewdb_snippetspace;
