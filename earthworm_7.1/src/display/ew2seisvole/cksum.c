
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: cksum.c,v 1.1 2000/02/14 17:20:06 lucky Exp $
 *
 *    Revision history:
 *     $Log: cksum.c,v $
 *     Revision 1.1  2000/02/14 17:20:06  lucky
 *     Initial revision
 *
 *
 */


#ifndef lint
static char rcsid[] = "$Header: /cvsroot_usgs/earthworm/src/display/ew2seisvole/cksum.c,v 1.1 2000/02/14 17:20:06 lucky Exp $";
#endif 

#include <sys/types.h>

/*
 * Compute checksum to characterize a character string
 */
unsigned int
cksum(s)
	char	*s;
{
	unsigned int	sum;
	int	c;

	sum = 0;
	while ((c = *s++) != 0) {
		if (sum&01)
			sum = (sum>>1) + 0x8000;
		else
			sum >>= 1;
		sum += c;
		sum &= 0xFFFF;
	}
	return sum;
}
