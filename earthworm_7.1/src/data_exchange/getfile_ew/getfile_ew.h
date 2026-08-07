/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: getfile_ew.h,v 1.2 2002/11/03 19:02:35 lombard Exp $
 *
 *    Revision history:
 *     $Log: getfile_ew.h,v $
 *     Revision 1.2  2002/11/03 19:02:35  lombard
 *     Added RCS header
 *     Added multiple include protection.
 *
 *
 *
 */

#ifndef GETFILE_EW_H
#define GETFILE_EW_H

/*   getfile.h    */

#define GETFILE_FAILURE   -1
#define GETFILE_SUCCESS    0
#define GETFILE_DONE       1
#define BUFLEN          4096      /* Max value is 999999 */

typedef struct
{
   char ip[20];            /* IP address of trusted client */
   char indir[80];         /* Where to store files from the client */
   unsigned long ipint;    /* IP address stored as an unsigned long */
} CLIENT;


#endif
