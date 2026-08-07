/*
	Include files with NSN definitions 

	Update History
	2-Oct-92	Changed Channel def to include MS_DELAY for filtering delays.	
	5-Nov-93	Simplified for SD version
*/

/*

	Define Common Structures 
*/
struct nsntime {
	unsigned char iyr;	/* year-1970)*2.  Low order bit is 256s of days*/
	unsigned char doy;	/* day of year modula 256 */
	long ms;			/* the milleseconds since midnight *16 + flags */
};
/*
	Definitions for the FLAGS within a Gomberg NSN record and GOMBERG STRUCT
*/
#define NSN_FLAGS_EOF 1
#define NSN_FLAGS_PARTIAL 8
#define NSN_FLAGS_EOR 16
#define NSN_FLAGS_CONTINUOUS 32
#define NSN_FLAGS_TRIGGER 64
#define NSN_FLAGS_POWER 32
struct nsntime qnsntim(),nsnadd(),buftim();	/* prototype some functions */
	struct gomberg {
		unsigned char lead1;			/* always 27 (esc) */
		unsigned char lead2;			/* always 3 (STX) */
		unsigned char numbyt[2];		/* byte reversed number of bytes */
		unsigned char routeid;			/* USNSN route ID */
		unsigned char nodeid;			/* Node ID for this Guy */
		unsigned char chanid;			/* Channel/station ID */
		unsigned char packseq;			/* The packet sequence ID*/
		struct nsntime tc;				/* a USNSN time code */
		unsigned char format;			/* a format code */
		unsigned char flags;			/* data flags */
		unsigned char doy;				/* doy detection started */
		unsigned char seq;				/* the sequence number */
		unsigned char detseq[2];		/* detection sequence */
		unsigned char buf[2028];

	} ;
/*
	Trigger/Channel parameters and DEFS 
*/
#define LOW_GAIN 32					/* add to Low Gain versions of chans*/
#define SH_CHAN 64					/* Add to channels that are SP */
#define CH_X20	0
#define CH_Y20	1
#define CH_Z20	2
#define CH_X40	0
#define CH_Y40	1
#define CH_Z40	2
#define CH_X1	3
#define CH_Y1	4
#define	CH_Z1	5
#define MAX_CH 	CH_Z1				/* Last channel # */
#define MAX_LCH CH_Z1				/* Some dimensioning uses this from USNSN*/
#include "cmprs.h"
/***********************************************************************
			E N D   O F   N S N . H
***********************************************************************/

