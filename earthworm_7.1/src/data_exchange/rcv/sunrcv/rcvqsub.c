/*
This collection of functions manipulate the Quanterra supplied QINIT/QTRAP
interface.  The QTRAPs routine was written for calling by FORTRAN and hence 
expects all arguments to be passed by reference.  For convenience from C these
routines will pass by value in and convert to reference calls to QTRAP.  
References to the functions in the Quanterra/FORTRAN interface have FORTRAN
style names like QTRAP_ and QINIT_ (caps mandatory).

D.C. Ketchum Mar 1991
*/
#include <stdio.h>
#include <errno.h>
#include "rcv.h"
#include "rcvqsub.h"		/* Include function prototypes and globals */
/**************************** I2swap ******************************/
#ifdef __STDC__
   void i2swap(short i, short *swap)
#else
  void i2swap(i,swap)
  short i;
  short *swap;
#endif
{
	unsigned char tmp;
	union {
 		short i2;
		unsigned char sw[2];
	} u;
	u.i2=i;
	tmp=u.sw[1];
	u.sw[1]=u.sw[0];
	u.sw[0]=tmp;
	*swap=u.i2;
	return; 
} 
/************************************* i4swap ***************************/
#ifdef __STDC__
  void i4swap(long i, long *swap)
#else
  void i4swap(i,swap) 
  long i;
  long *swap; 
#endif
{ 
	char tmp[4];
 	union {
	long l;
	unsigned char b[4];
	} u;  
	memcpy(tmp,(char *) &i,sizeof (long));
	for (i=0; i< 4; i++) u.b[i]=tmp[3-i];
	*swap=u.l;
	return;
}
/***********************************************************************/
#ifdef __STDC__
  int yrday(struct nsntime tc, long *iyear, long *doy)
#else
  int yrday(tc,iyear,doy)					/* From NSN time in TC get year and day*/
  struct nsntime tc;					/* a time code in */
  long *iyear,*doy;					/* return year and day of year */
#endif
{
	*iyear=tc.iyr/2+1970;			/* return year */
	*doy=tc.doy+((tc.iyr & 1) ? 256 : 0);
	return 0;
}
#ifdef __STDC__
   int nsnsub(struct nsntime tc1, struct nsntime tc2)
#else
  int nsnsub(tc1,tc2)
  struct nsntime tc1,tc2;
#endif
/*
	calculates time difference in MS between TC1-TC2.  It does not at this
	time correctly account for leap seconds though leap years are considered
	
	D.C. Ketchum Jan 1995
*/
{
	long iy1,id1,iy2,id2;			/* years and days of time code */
	int iday;						/* days separated counter */
	yrday(tc1,&iy1,&id1);				/* get day and year */
	yrday(tc2,&iy2,&id2);				/* of second time */
	if(iy1 == iy2 && id1 == id2) {
		return (tc1.ms-tc2.ms)/16;
	}
	if(iy1 == iy2) {
		if(abs(id1-id2) > 24) {
			if(id1 > id2) return 2147483647;
			else return -2147483647;
		}
		return ((id1-id2)*86400000+(tc1.ms-tc2.ms)/16);
	}
	if(abs(iy1-iy2) > 1) {
		if(iy1 > iy2) return 2147483647;
		else return -2147483647;
	}
	if( (iy1-iy2) == 1) {			/* first day is in later year */
		iday=id1+365-id2;			/* days unless its leap year */
		if( (iy2 % 4) == 0 && (iy2 % 400) != 0) iday++;
		return (iday*86400000+(tc1.ms-tc2.ms)/16);
	} else {
		iday=id1-(id2+365);
		if( (iy2 % 4) == 0 && (iy2 % 400) != 0) iday;
		return (iday*86400000+(tc1.ms-tc2.ms)/16);
	}
/*	return 2147483647;*/
}  
#ifdef __STDC__
  struct nsntime maknsn(int iy, int id, int ih, int im, int is, int ms, int leap)
#else
  struct nsntime maknsn(iy,id,ih,im,is,ms,leap)
  int iy,id,ih,im,is,ms,leap;
#endif
{
	struct nsntime tc;
	if(iy > 1900) iy-=1900;
	tc.iyr=(iy-70)*2;
	if(id >= 256) {tc.iyr++;id-=256;}
	tc.doy=id;
	tc.ms=(ih*3600000+im*60000+is*1000+ms)*16;
	if(leap != 0) {
		if(leap == 1) ms= ms | 8;
		if(leap == -1) ms=ms | 4;
	}
	return tc;
}
	
/***********************************************************************/
#ifdef __STDC__
  int nsnint(struct nsntime tc, long *yr, long *day, long *hr, long *min, long *sec,
		long *ms,long *leap)
#else
  int nsnint(tc,yr,day,hr,min,sec,ms,leap)
  struct nsntime tc;
  long *yr,*day,*hr,*min,*sec,*ms,*leap;
#endif
/*  Convert a Time code to its correct time expressed as integers.  0 is 
normal return.  -1 returned means the time code had more the 24 hours in it
and was not a leap second.*/
{
	int msec;						/* error return */
	*leap=0;
	*yr=1970+tc.iyr/2;				/* get year */
	*day=tc.doy+((tc.iyr & 1) ? 256 : 0);/* compute day of year */
	msec=tc.ms/16;					/* get number of MS since midnight */
/*	printf ("NSNINT msec=%d\n",msec); */
	*hr=msec/3600000;				/* compute hour of day */
	msec%=3600000;					/* remaining MS */
	*min=msec/60000;				/* compute minutes */
	msec%=60000;					/* remaining MS */
	*sec=msec/1000;					/* compute number of seconds */
	*ms=msec % 1000;
	msec=0;							/* use it as an error flag */
	if(*hr == 24) {
		if((tc.ms && 8) != 0) {
			(*hr)--;
			*min=59;
			*sec=60;
		}
		else
			msec=-1;
	}
	return msec;
}
/***********************************************************************/
/*
	nsnadd adds MSADD milleseconds to the USNSN time code in TC and returns
	a time code at that time. 
*/
#ifdef __STDC__
  struct nsntime nsnadd(struct nsntime tc, long msadd)
#else
  struct nsntime nsnadd(tc,msadd)
  struct nsntime tc;
  long msadd;
#endif
{
	extern leapnext;				/* indicate leap status for tomorrow*/
	long yr,day,hr,min,sec,ms,leap,flags,nday,max,err,msorg,dayorg;
	ms=tc.ms/16;					/* get number of ms */
	ms+=msadd;						/* add right number of ms to it */
	max=86400000;						/* assume a normal day */
	if((tc.ms & 8) != 0) max=86401000;/* Its a positive leap day */
	if((tc.ms & 4) != 0) max=86399000;/* Its a negative leap day */
 	if(ms >= 0 && ms <= max) {		/* Easy case just add ms and return */
		flags=tc.ms & 15;			/* get flags */
		tc.ms=ms*16+flags;			/* build new one */
		return tc;
	}
	err=nsnint(tc,&yr,&day,&hr,&min,&sec,&msorg,&leap); /* break down time */
/*
If the number of MS is negative, add one days MS to it until its positive
and borrow from the days.  Correct the year if the day becomes < 0 
*/
	msorg=ms;						/* same MS count */
	dayorg=day;
	if(ms < 0) {					/* ms < 0  backwards in time */
		while (ms < 0) {
			ms+=86400000;			/* add a days worth */
			day--;					/* borrowd a day */
		}
		if(day <= 0) {				/* gone to prior year */
			yr--;					/* set year to prior */
			nday=(((yr % 4) == 0 && (yr % 400) != 0) ? 366 : 365); /* # of day */
			day+=nday;				/* correct day of year */
		}
	}
/*
	MS must be positive.  Adjust for days forward.  If tomorrow is a leap
	second day, take it into account.  If today is leap second, take it
	into account (MAX was set above to handle this case).  We must presume
	days further in the future that tommorrow are not leap second days.
*/
	while (ms > max) {
		day++;						/* bump to tomorrow */
		ms-=max;					/* remove from today */
		max=86400000;				/* assume a normal day */
		if((day-dayorg) == 1) {		/* are we beyond tommorrow */
			if(leapnext != 1) max=86401000;/* Its a positive leap day */
			if(leapnext == -1) max=86399000;/* Its a negative leap day */
		}
		nday=(((yr % 4) == 0 && (yr % 400) != 0) ? 366 : 365);
		if(day > nday) {			/* Happy new year */
			yr++;
			day-=nday;				/* correct for number of days */
		}
	}
/*
The MS is now guaranteed to be positive.  Compute effect on days 
*/
/*	fprintf(logout,"Going to diff day %d %d %d\n",ms,day,yr);*/
	tc.iyr=(yr-1970)*2;			/* create year part */
	if(day < 256)
		tc.doy=day;				/* simple case no carry to yr part */
	else {
		tc.doy=day % 256;		/* set doy part */
		tc.iyr|=1;				/* set 256s of days in year byte */
	}
	tc.ms=ms*16;
	if(msorg > 0 && leapnext !=0 && (day-dayorg) == 1) {
		if(leapnext == 1) tc.ms|=8;		/* positive leap seconde */
		if(leapnext == -1) tc.ms|=4;	/* negative leap second */
	}
	return tc;
}
