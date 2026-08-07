/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*    $Id: ewdb_update_tables_merge_working.sql,v 1.1 2004/09/09 06:04:10 davidk Exp $ */
/*                                                          */
/*    Revision history:                                     */
/*     $Log: ewdb_update_tables_merge_working.sql,v $
/*     Revision 1.1  2004/09/09 06:04:10  davidk
/*     no message
/*
/*     Revision 1.8  2004/08/10 21:49:07  davidk
/*     Updated the architecture for sql install scripts.
/*     *_util  scripts load sql procedures, functions, and views
/*     *_constants scripts populate constant-tables
/*     *_tables  scripts load/update table, columns, constraints, and indexes
/*     *_combination scripts perform one of the above tasks, but contain statements
/*     which are dependent upon more than one sub-schema, and must be run
/*     separately after all sub-schemas have been processed.
/*
/*     These changes fixed several install problems and meaningless error messages.
/*        */
/*     Revision 1.1  2002/06/18 16:10:35  lucky             */
/*     Initial revision                                     */
/*                                                          */
/************************************************************/

/***********************************************************
  Column Updates and additions to merge tables:
**********************************************************/


/***********************************************************
  Constraint additions to merge tables:
**********************************************************/


/***********************************************************
  Index additions to merge tables:
**********************************************************/
