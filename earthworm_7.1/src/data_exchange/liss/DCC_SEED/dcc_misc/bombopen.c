/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: bombopen.c,v 1.1 2000/03/13 23:45:14 lombard Exp $
 *
 *    Revision history:
 *     $Log: bombopen.c,v $
 *     Revision 1.1  2000/03/13 23:45:14  lombard
 *     Initial revision
 *
 *
 *
 */

#include <dcc_std.h>
#include <dcc_misc.h>

_SUB FILE *bombopen(char *infile, char *inmode)
{

  FILE *ofile;

  ofile = fopen(infile,inmode);

  if (ofile==NULL) 
    bombout(EXIT_ABORT,"Cannot open file %s mode %s\n",
	    infile,inmode);

  return(ofile);

}
