/************************* Function Prototypes **************************/
#ifdef __STDC__
	void i2swap(short i, short *swap);
	void i4swap(long i, long *swap);
	int nsnint(struct nsntime tc, long *yr, long *day, long *hr, long *min, long *sec,
			long *ms,long *leap);
	int yrday(struct nsntime tc, long *iyear, long *doy);
	int nsnsub(struct nsntime tc1, struct nsntime tc2);
	struct nsntime maknsn(int iy, int id, int ih, int im, int is, int ms, int leap);
	struct nsntime nsnadd(struct nsntime tc, long msadd);

#else
	void i2swap();			/* (short i, short *swap) swap bytes in I2 */
	void i4swap();			/* (long i, long *swap) end for end byte swap i*4*/
	int nsnint();			/* (struct nsntime tc,long *yr,*day,*hr,*min,*sec,*ms)
							Converts Time code to its integer parts. Return
							zero unless time code is somehow illegal */
	struct nsntime nsnadd();/* (struct nsntime tc, long msadd)  Add MSADD to the
							time code and return the revised timecode.  MSADD
							can be negative.  */

#endif
