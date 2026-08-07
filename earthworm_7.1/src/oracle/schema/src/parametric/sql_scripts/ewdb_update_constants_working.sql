/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*    $Id: ewdb_update_constants_working.sql,v 1.3 2005/03/23 06:30:06 davidk Exp $ */
/*                                                          */
/*    Revision history:                                     */
/*     $Log: ewdb_update_constants_working.sql,v $
/*     Revision 1.3  2005/03/23 06:30:06  davidk
/*     Added Mw related tables for tiXXX constants
/*
/*     Revision 1.2  2004/09/09 05:01:39  davidk
/*     Removed reference to undefined table ExternalPick.
/*
/*     Revision 1.1  2004/08/10 21:49:07  davidk
/*     Updated the architecture for sql install scripts.
/*     *_util  scripts load sql procedures, functions, and views
/*     *_constants scripts populate constant-tables
/*     *_tables  scripts load/update table, columns, constraints, and indexes
/*     *_combination scripts perform one of the above tasks, but contain statements
/*     which are dependent upon more than one sub-schema, and must be run
/*     separately after all sub-schemas have been processed.
/*
/*     These changes fixed several install problems and meaningless error messages.
/*          */
/*     Revision 1.1  2002/06/18 16:10:35  lucky             */
/*     Initial revision                                     */
/*                                                          */
/************************************************************/
insert into ewdb_tablelist values(50, 'CoincidenceEvent');
insert into ewdb_tablelist values(51, 'ChannelTrigger');
insert into ewdb_tablelist values(52, 'Mw');
insert into ewdb_tablelist values(53, 'MwChan');
insert into ewdb_tablelist values(54, 'MwChanTS');
insert into ewdb_tablelist values(55, 'MwFilter');
