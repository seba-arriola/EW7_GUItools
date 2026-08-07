/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: log.c,v 1.1 2000/03/13 23:45:14 lombard Exp $
 *
 *    Revision history:
 *     $Log: log.c,v $
 *     Revision 1.1  2000/03/13 23:45:14  lombard
 *     Initial revision
 *
 *
 *
 */

/*----------------------------------------------------------------------*
 *	Modification and history 					*
 *-----Edit---Date----Who-------Description of changes------------------*
 *	000 07-Nov-88 RCM	Extracted from multiple routines	*
 *----------------------------------------------------------------------*/

#include <dcc_std.h>
#include <dcc_misc.h>

FILE	*LogIOV;	/* Log File */

void LOGF(formstr,p1,p2,p3,p4,p5,p6,p7,p8)
TEXT *formstr;
int p1,p2,p3,p4,p5,p6,p7,p8;
{

	fprintf(LogIOV,formstr,p1,p2,p3,p4,p5,p6,p7,p8);
}

void LOGFB(formstr,p1,p2,p3,p4,p5,p6,p7,p8)
TEXT *formstr;
int p1,p2,p3,p4,p5,p6,p7,p8;
{

	fprintf(LogIOV,formstr,p1,p2,p3,p4,p5,p6,p7,p8);
	fprintf(stderr,formstr,p1,p2,p3,p4,p5,p6,p7,p8);
}

void LOGBOMB(error,formstr,p1,p2,p3,p4,p5,p6,p7,p8)
int error;
TEXT *formstr;
int p1,p2,p3,p4,p5,p6,p7,p8;
{
	LOGF(formstr,p1,p2,p3,p4,p5,p6,p7,p8);
	LOGF("\nFatal error %x\n",error);
	if (LogIOV!=stdout&&
	    LogIOV!=stderr) fclose(LogIOV);	/* Better close it */

	bombout(error,formstr,p1,p2,p3,p4,p5,p6,p7,p8);
}
