/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: orareport.h,v 1.1 1999/11/09 16:37:31 lucky Exp $
 *
 *    Revision history:
 *     $Log: orareport.h,v $
 *     Revision 1.1  1999/11/09 16:37:31  lucky
 *     Initial revision
 *
 *
 */

/*
 * dbreport.h : Include file for dbreport.c; 
 *              sets up database-specific stuff.
 */

/* Globals relating to database; set from configuration file 
 ***********************************************************/
char  DBservice[30];       /* OCI data source to interact with  */
char  DBuser[30];          /* UserId to connect to database as  */
char  DBpassword[30];      /* Password to datasource            */
int   Debug;               /* if non-zero, print debug messages */

#include <ora_api.h> 
