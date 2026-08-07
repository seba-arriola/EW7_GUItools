/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: st_limits.c,v 1.1 2000/03/13 23:48:35 lombard Exp $
 *
 *    Revision history:
 *     $Log: st_limits.c,v $
 *     Revision 1.1  2000/03/13 23:48:35  lombard
 *     Initial revision
 *
 *
 *
 */

#include <dcc_std.h>
#include <dcc_time.h>

_SUB STDTIME ST_Zero()
{

	STDTIME _ans = { 0, 0, 0, 0, 0, 0 };

	return(_ans);

}

_SUB DELTA_T ST_ZeroDelta()
{

	DELTA_T _ans = { 0, 0, 0, 0, 0 };

	return(_ans);

}


_SUB STDTIME ST_End()
{
	STDTIME _ans = { 4000, 1, 0, 0, 0, 0 };

	return(_ans);
}
