/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: st_formdelta.c,v 1.2 2003/06/16 22:05:26 patton Exp $
 *
 *    Revision history:
 *     $Log: st_formdelta.c,v $
 *     Revision 1.2  2003/06/16 22:05:26  patton
 *     Fixed Microsoft WORD typedef issue
 *
 *     Revision 1.1  2000/03/13 23:48:35  lombard
 *     Initial revision
 *
 *
 *
 */

#include <dcc_std.h>
#include <dcc_time.h>

_SUB	DELTA_T ST_FormDelta(DCC_LONG dd,DCC_LONG dh,DCC_LONG dm,DCC_LONG ds,DCC_LONG dms)
{

	DELTA_T outtime;

/*-------------Normalize the date-------------*/

	timenormd(&dd,&dh,&dm,&ds,&dms);

	outtime.nday = dd;
	outtime.nhour = dh;
	outtime.nmin = dm;
	outtime.nsec = ds;
	outtime.nmsecs = dms;

	return(outtime);		/* Returning a structure! */

}
