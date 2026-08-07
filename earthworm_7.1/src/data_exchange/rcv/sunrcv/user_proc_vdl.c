#include <stdio.h>
#include <errno.h>
/*
	A do nothing user_proc_cmd.  This routine is called by stationcmd.c
	and gives the user a chance to process command line parameters.
*/
int dbgvdl=0;			/* if -dbgvdl this is non-zero, more output results*/
int p[2];
void user_proc_cmd(argc,argv)
int argc;
char *argv[];
{
	extern FILE *logout;
	int pid;
	int i;
	char line[10];
/*	char pname[]="feedme_rcv";		/* testing program name */
	char pname[]="vdlrcv";			/* vdl program for RCV */
	char dashz[]="-z";
	char ** argvcpy;
	argvcpy=(char **) malloc( (argc+4)*sizeof(char *) );
	for (i=0; i<argc; i++) 
	{	fprintf(logout,"User_proc_cmd %d arg=%s\n",i,argv[i]);
		argvcpy[i]=argv[i];
		if( strcmp(argv[i],"-dbgvdl") == 0) 
		{	dbgvdl=1;
			fprintf(logout,"USER_PROC_VDL debug on ! \n");
		}
	}
	if(pipe(p) < 0) {perror("user_proc open pipe!"); exit(101);}
	if( (pid = fork() ) == 0) 			/* if true, I am the child */
	{	sprintf(line,"%d",p[0]);	/* read end pipe unit number inherited */
		fprintf(logout,"%s line=%s\n",pname,line);
		argvcpy[argc]=dashz;
		argc++;
		argvcpy[argc]=line;
		argvcpy[argc+1]=0;

		execv(pname,argvcpy);			/* start vdl */
		fprintf(logout,"user_proc_vdl cmd : execv should not get here! %d\n",
			errno);
		perror("Execv error ");
		exit(100);
	}
	else								/* I the parent. return */
	{	fprintf(logout,"read=%d write=%d\n",p[0],p[1]);
		close(p[0]);					/* we write data so close read end */
		fprintf(logout,"VDL child is pid=%d\n",pid);
	}	
	return;
}

/*
	Example User proc -
	Note that EOF may be delivered by itself (i.e. with nsamp=0)

	nsamp	Number of samples in IDAT.
	idat	Array of longs containing data
	name	C-string with SEED name of station being processed
	cname	C-string with SEED name of channel being processed
	eof		If set, the detection/trigger/timeseries segment is over
	rate	The digitizing rate in Hz
	seq		The channel sequence number

*/
#ifdef __STDC__
  void user_proc(int iy, int id, int ih, int im, int is, int ms, int leap, long idat[],
		int nsamp, char *name, char *cname, char *network, int eof, double rate, int seq)
#else
  void  user_proc(iy,id,ih,im,is,ms,leap,idat,nsamp,name,cname,network,eof,rate,seq)
  int iy,id,ih,im,is,ms,leap;		/* broken down time code */
  int eof;							/* if true, end of this detection on this channel*/
  long idat[];						/* data buf with in the clear 32 bit longs */
  int seq;							/* channel sequence # */
  int nsamp;						/* number of frames in */
  char * name;						/* seed station name */
  char * cname;						/* seed channel name */
	char * network;
  double rate;						/* digitizing rate */
#endif
{
	extern FILE *logout;
	int ierr;
	if(dbgvdl) fprintf(logout, 
"%s %s %s %4d %3d %2d:%2d:%2d.%3d lp=%d sq=%3d ns=%4d eof=%d rt=%7.3f dat=%6d %6d %6d\n",
	name,cname,network,iy,id,ih,im,is,ms,leap,seq,nsamp,eof,rate,
	idat[0],idat[1],idat[2]);
	ierr=write(p[1],name,5);
	if(ierr != 5) 
	{	perror("write"); fprintf(logout,"write erro=%d %d\n",ierr,errno);
	}
	ierr=write(p[1],cname,4);
	if(ierr != 4) 
	{	perror("write"); fprintf(logout,"write erro=%d %d\n",ierr,errno);
	}
	ierr=write(p[1],&rate,sizeof(double));
	if(ierr != sizeof(double) ) 
	{	perror("write"); fprintf(logout,"write erro=%d %d\n",ierr,errno);
	}
	ierr=write(p[1],&seq,4);
	if(ierr != 4) 
	{	perror("write"); fprintf(logout,"write erro=%d %d\n",ierr,errno);
	}
	ierr=write(p[1],&eof,4);
	if(ierr != 4) 
	{	perror("write"); fprintf(logout,"write erro=%d %d\n",ierr,errno);
	}
	ierr=write(p[1],&iy,4);
	if(ierr != 4) 
	{	perror("write"); fprintf(logout,"write erro=%d %d\n",ierr,errno);
	}
	ierr=write(p[1],&id,4);
	if(ierr != 4) 
	{	perror("write"); fprintf(logout,"write erro=%d %d\n",ierr,errno);
	}
	ierr=write(p[1],&ih,4);
	if(ierr != 4) 
	{	perror("write"); fprintf(logout,"write erro=%d %d\n",ierr,errno);
	}
	ierr=write(p[1],&im,4);
	if(ierr != 4) 
	{	perror("write"); fprintf(logout,"write erro=%d %d\n",ierr,errno);
	}
	ierr=write(p[1],&is,4);
	if(ierr != 4) 
	{	perror("write"); fprintf(logout,"write erro=%d %d\n",ierr,errno);
	}
	ierr=write(p[1],&ms,4);
	if(ierr != 4) 
	{	perror("write"); fprintf(logout,"write erro=%d %d\n",ierr,errno);
	}
	ierr=write(p[1],&nsamp,4);
	if(ierr != 4) 
	{	perror("write"); fprintf(logout,"write erro=%d %d\n",ierr,errno);
	}
	ierr=write(p[1],&idat[0],nsamp*4);
	if(ierr != nsamp*4) 
	{	perror("write"); fprintf(logout,"write erro=%d %d\n",ierr,errno);
	}
	if(dbgvdl) fprintf(logout,"User_proc done!\n"); fflush(logout);
}


user_heart_beat()
{	extern FILE *logout;
	if(dbgvdl) fprintf(logout,"Heartbeat\n");
}
user_shutdown()
{}
