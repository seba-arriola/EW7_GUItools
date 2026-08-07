/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: st_deltacomp.c,v 1.1 2000/03/13 23:48:35 lombard Exp $
 *
 *    Revision history:
 *     $Log: st_deltacomp.c,v $
 *     Revision 1.1  2000/03/13 23:48:35  lombard
 *     Initial revision
 *
 *
 *
 */

#include <dcc_std.h>
#include <dcc_time.h>

_SUB	int ST_DeltaComp(DELTA_T intime,DELTA_T insect)
{

	if (intime.nday>insect.nday) 		return(1);
	if (intime.nday<insect.nday) 		return(-1);
	if (intime.nhour>insect.nhour) 		return(1);
	if (intime.nhour<insect.nhour) 		return(-1);
	if (intime.nmin>insect.nmin) 		return(1);
	if (intime.nmin<insect.nmin) 		return(-1);
	if (intime.nsec>insect.nsec) 		return(1);
	if (intime.nsec<insect.nsec) 		return(-1);
	if (intime.nmsecs>insect.nmsecs)	return(1);
	if (intime.nmsecs<insect.nmsecs)	return(-1);
	return(0);

}
