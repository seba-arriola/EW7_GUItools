
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: grid.h,v 1.1 2000/02/14 16:08:53 lucky Exp $
 *
 *    Revision history:
 *     $Log: grid.h,v $
 *     Revision 1.1  2000/02/14 16:08:53  lucky
 *     Initial revision
 *
 *     Revision 1.1  2000/02/14 16:07:49  lucky
 *     Initial revision
 *
 *
 */

/*
 * grid.h : Grid structures
 */

/* Grid/cartesion coordinates 
 ****************************/
typedef struct {
	float 	x;	/* Km north of grdOrd				*/
	float 	y;      /* Km east of grdOrd				*/
	float	z;      /* Depth in km, down negative			*/
} CART;

/* Geocentric points	
 *******************/
typedef struct {
	float	lon;
	float	lat;
	float	z;
} GEO;

/* Function prototypes for geocentric/cartesion conversion
 *********************************************************/
void grid_cart(GEO *, CART *);   /* input geocentric -> output cartesian  */
void grid_geo (GEO *, CART *);   /* input cartesian  -> output geocentric */




