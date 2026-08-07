/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: st_getjulian.c,v 1.2 2003/06/16 22:05:26 patton Exp $
 *
 *    Revision history:
 *     $Log: st_getjulian.c,v $
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

_SUB	DCC_LONG	ST_GetJulian(STDTIME intime)
{

	DCC_LONG jul1;

	jul1 = _julday(intime.year,1,intime.day);

	return(jul1);

}
