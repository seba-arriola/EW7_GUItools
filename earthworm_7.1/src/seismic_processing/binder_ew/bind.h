
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: bind.h,v 1.4 2004/10/21 16:49:27 dietz Exp $
 *
 *    Revision history:
 *     $Log: bind.h,v $
 *     Revision 1.4  2004/10/21 16:49:27  dietz
 *     Modified to allow pick association with entire quake list (previously
 *     only attempted assoc with 10 most recent quakes). Changes required
 *     keeping track of the earliest pick sequence number associated with each
 *     quake so that a hypocenter update is not attempted if some of the
 *     supporting picks are no longer in the pick FIFO.
 *
 *     Revision 1.3  2004/05/17 20:57:03  dietz
 *     changed site field from short to int
 *
 *     Revision 1.2  2004/05/14 23:35:37  dietz
 *     modified to work with TYPE_PICK_SCNL messages only
 *
 *     Revision 1.1  2000/02/14 16:08:53  lucky
 *     Initial revision
 *
 *     Revision 1.1  2000/02/14 16:07:49  lucky
 *     Initial revision
 *
 *
 */

/*
 * bind.h : Associator data.
 */

#ifndef _BIND_H
#define _BIND_H

long mQuake;			/* Size of quake fifo			*/
long lQuake;			/* Next sequential quake id		*/
typedef struct {
	double	t; 		/* Eq origin time (sec after 1600)	*/
	double	lat;		/* Eq latitude (decimal degrees)	*/
	double	lon;		/* Eq longitude (decimal degrees)	*/
	double	z;		/* Eq depth (km)			*/
	float	rms;		/* rms residual (sec)			*/
	float	dmin;		/* Distance to closest station		*/
	float	ravg;		/* Average or median station distance	*/
	float	gap;		/* Largest azimuth without picks	*/
	short	npix;		/* Number of picks associated		*/
	short	nmod;		/* Modification count			*/
	short   assessed;       /* 0=not yet assessed w/WLE resampling  */
        long    lpickfirst;     /* lPick of 1st pick assoc w/this eq    */
} QUAKE;
long *iQuake;			/* Chronological quake index list	*/
QUAKE *pQuake;			/* Quake fifo				*/

long mPick;			/* Size of pick fifo			*/
long lPick;			/* Next sequential pick id		*/
typedef struct {
	double		t;	/* pick time; seconds after 1600	*/
	unsigned char 	instid; /* instid of pick's source module	*/
	unsigned char	src;	/* pick's source module id		*/
	int		seq;	/* Pick serial number from source	*/
	unsigned long	quake;	/* associated with this quake id	*/
	int  		site;	/* station identifier 			*/
	char		phase;	/* associated as this phase		*/	
	char		ie;	/* impulsive/emergent descriptor	*/
	char		fm;	/* first motion (polarity)		*/
	char		wt;	/* pick quality weight (0-4)		*/
        long            lpick;  /* sequential pick id for this pick     */
} PICK;
long *iPick;			/* Chronological pick index list	*/
PICK *pPick;			/* Pick fifo				*/


#endif
