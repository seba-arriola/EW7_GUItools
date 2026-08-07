#include <stdio.h>
/*
	A do nothing user_proc_cmd.  This routine is called by stationcmd.c
	and gives the user a chance to process command line parameters.
*/
void user_proc_cmd(argc,argv)
int argc;
char *argv[];
{
	extern FILE *logout;
	int i;
	for (i=0; i<argc; i++) fprintf(logout,"User_proc_cmd %d arg=%s\n",
			i,argv[i]);
	return;
}

/*
	Example User proc -
	Note that EOF may be delivered by itself (i.e. with nsamp=0)

	iy,id,ih,im,is,ms	Ints with year, day, hour, minute,second and mS
	leap	If non=zero, its a leap second day.
	idat	Array of longs containing data
	nsamp	Number of samples in IDAT.
	name	C-string with SEED name of station being processed
	cname	C-string with SEED name of channel being processed
	network	C-string with SEED name of network originating the data
	eof		If set, the detection/trigger/timeseries segment is over
	rate	The digitizing rate in Hz
	seq		The channel sequence number.  For each channel this should increase
			from 1 to 255 for each segment of data.  It should go back to one
			after the last record (EOF set to true).

*/
#ifdef __STDC__
  void user_proc(int iy, int id, int ih, int im, int is, int ms, int leap,
		 long idat[], int nsamp, char *name, char *cname, char *network,
		 int eof, double rate, int seq)
#else
  void  user_proc(iy,id,ih,im,is,ms,leap,idat,nsamp,name,cname,network,
		eof,rate,seq)
  int iy,id,ih,im,is,ms,leap;		/* broken down time code */
  int eof;							/* if true, end of this detection on this channel*/
  long idat[];						/* data buf with in the clear 32 bit longs */
  int seq;							/* channel sequence # */
  int nsamp;						/* number of frames in */
  char * name;						/* seed station name */
  char * cname;						/* seed channel name */
  char * network;					/* network name */
  double rate;						/* digitizing rate */
#endif
{
	extern FILE *logout;
	fprintf(logout, 
"%s %s %s %4d %3d %2d:%2d:%2d.%3d lp=%d sq=%3d ns=%4d eof=%d rt=%7.3f dat=%6d %6d %6d\n",
	name,cname,network,iy,id,ih,im,is,ms,leap,seq,nsamp,eof,rate,
	idat[0],idat[1],idat[2]);
}


user_heart_beat()
{	extern FILE *logout;
	fprintf(logout,"Station heart beat!\n"); fflush(logout);
	return;
}
