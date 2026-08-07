/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: mag2ora.h,v 1.1 2001/06/21 21:24:11 lucky Exp $
 *
 *    Revision history:
 *     $Log: mag2ora.h,v $
 *     Revision 1.1  2001/06/21 21:24:11  lucky
 *     Initial revision
 *
 *     Revision 1.2  2001/02/28 17:29:10  lucky
 *     Massive schema redesign and cleanup.
 *
 *     Revision 1.1  2000/03/31 17:32:54  lucky
 *     Initial revision
 *
 *     Revision 1.2  2000/01/04 20:05:29  davidk
 *     converted code from ora_trace_save.c to ora_trace_save.c to working
 *     with schema2 and later API's.
 *
 *     Revision 1.1  1999/11/09 16:37:22  lucky
 *     Initial revision
 *
 *
 */

/*
 * ora_trace_save.h : Include file for ora_trace_save.c; 
 *              sets up database-specific stuff.
 */

/* Globals relating to database; set from configuration file 
 ***********************************************************/
char  DBservice[30];       /* OCI data source to interact with  */
char  DBuser[30];          /* UserId to connect to database as  */
char  DBpassword[30];      /* Password to datasource            */
int   Debug;               /* if non-zero, print debug messages */

#include <ewdb_ora_api.h> 
