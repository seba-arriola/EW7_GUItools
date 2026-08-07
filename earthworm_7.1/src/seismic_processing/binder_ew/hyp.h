
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: hyp.h,v 1.1 2000/02/14 16:08:53 lucky Exp $
 *
 *    Revision history:
 *     $Log: hyp.h,v $
 *     Revision 1.1  2000/02/14 16:08:53  lucky
 *     Initial revision
 *
 *     Revision 1.1  2000/02/14 16:07:49  lucky
 *     Initial revision
 *
 *
 */

/*
 * hyp.h : L1 hypocenter routine structures and definitions
 */
typedef struct {
	double	t;
	double	lat;
	double	lon;
	double	z;
	float	rms;
	float	dmin;
	float	ravg;
	float	gap;
	int	nph;
} HYP;

typedef struct {
	double	t;
	long	id;
	float	res;	/* Residual (returned)				*/
	int	site;	/* Site table index				*/
	int	wt100;	/* Equation weight (0-100)			*/
	char	ph;	/* Phase index (0-5)				*/
	char	ie;	/* Implusivity ('I' or 'E')			*/
	char	wt;	/* Picker weight ('0' - '4')			*/
	char	fm;	/* First motion ('U','+',' ','-','D')		*/
        char    flag;   /* Outcome of WLE's tests: 'T'= belongs to eq   */
                        /*                         'F'= bad association */
                        /*                         'G'= glitch pick     */
                        /*                         '?'= not sure        */
} PIX;
