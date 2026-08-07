/*
	RCV.C - Written for the NSN project as an example C program to receive NSN 
	packets from a VSAT or via TCP/IP socket.  This routine will take the stream 
	which could include data from multiple stations.  The companion program
	"STATION" is spawned at the end of a pipe to take data packets from RCV.  
	STATION can be spawned one for each station coming in or it can be invoked
	as one station program handling all of the stations coming in on the 
	communications link.  The latter mode is referred to as the "multiple" station
	mode and is reflected in the global variable "multiple". 

	The main program performs the recommended
	input processing for rollbacks as described in the NSN Satellite Primer.  The
	data supplied to each pipe is as complete as it can be (except that during 
	rollback some data may be tossed) without duplicating many packets.  The user
	has three functions that need to be written to interface to this data stream.
	The routines are :

	user_proc_cmd(argc,argv)	called once at startup of station for user setups
	
	user_proc(iy,id,ih,im,is,ms,leap,name,cname,network,eof,rate, seq)
						this routine delivers one packet of data to the user
						See USER_PROC.C for an example and more documentation

	user_heartbeat()	This routine is called by RCV each time a "keepalive"
						is received.  These messages are sent from Golden when
						no data has been sent to a receive for 1 minute.  It
						is also called whenever new data is received.  Hence,
						if user_heartbeat is not called for several minute,
						the TCP_IP socket can be presumed dead.

	user_heart_beat()	Same function but in the station processes instead of
						in RCV

	user_shutdown()			invoke as part of the process termination.  user should
							perform a graceful shutdown if possible.

	In forceroll mode output is more voluminous and a rollback is forced on every
	packet whose sequence satisfies (sequence % 32) == 10.  This allows testing
	the rollback much more easily since they do not occur in nature very often.

	This was developed on a Sun 4 Workstation using the built in serial port.  By
	calling the read routine often (every 10 Ms or so) no data was lost.  On most
	loaded Sun systems a buffered Serial board is needed to keep from dropping 
	characters especially on a production system.  This rapid calling probably 
	makes this routine unsuitable for use without modification on such systems.
	By changing defined parameter NCREAD and UWAIT (microseconds to wait) the
	scan for input can be adjusted.

	Since this is example code, it has not been tested in the fire of production.
	Feel free to use it and please report any problems you have with it so we
	can make it a better example.  The USNSN does not use Sun's or even UNIX to
	in its data management center. 

	RCV command line Switches :
	-tcp	Use TCP/IP socket to NSN3.CR.USGS.GOV to get data. 
	-host	Use this host name instead for NSN3 
	-o	path	Open file "path" for stdout
	-i pathname - Use the pathname as device to obtain serial data from 
	-norollback	Do not support rollbacks.  Passive receive only.
				(-i /dev/ttya)

	Debug switches (not usually used by users)
	-! 		Turn on debugging 
	-$		Turn on force rollback flag

Note : -tcp and -i are mutually exclusive

Station Command line :
	-dbg	Turns on debugging in station processes (not usually used by users)
	-#  N	Send multiple stations to one station, N is max # channels
	-stationlog	Send logs to file station.log? or if a non-multiple station,
					to STN.LOG?.
	-s		Passed to station program indicating a log file for each station
				be opened.  If this switch is not set a single log file will be
				created and shared by all station processes.  If -# is used
				station.log will always be the log file.

	Modification history

	Mar 1996	Base functionality
	Jun 1996 	Add command line interface and TCP/IP input
	Jul 1996	Add conditionaly code for ANSI C.
	Oct 1997	Added multiple stations to one station invokation, added 
					exit_handler for user closedowns
				Added full seed name lookup in NSNSTATION2.DAT
	Feb 1998	Added "keepalive" messages from sender, Non-blocking I/O,
				and two minute time out to force new connection to server


	D.C. Ketchum Mar 1996 - Ketchum@usgs.gov (303) 273-8479

*/
#include <sys/types.h>
#include <stdio.h>
#include <errno.h>
#include <time.h>
#include <termios.h>
#include <memory.h>
#include <unistd.h>
#include <signal.h>
#include "rcv.h"
#include "exit_handler.h"
/*
	Misc Defines
*/
#define NCREAD 100					/* read 10 characters at a time */
									/* Do not increase NCREAD above 1/2 of the
									length of the shortest buffer expected.  In
									most cases the shortest buffer is about 200
									characters, so NCREAD <=100.  This limit is
									caused because the main loop requires two 
									physical reads per packet for the logic to
									process the data */
#define UWAIT 1000					/* microseconds to wait when no new data ready */
#define INBUFSIZE 10240
#define index(A) (INBUFSIZE > A) ? A : A-INBUFSIZE
#define ESC 27						/* Escapes Ascii code */
#define STX 3						/* STX ASCII code */
#define DCD car						/* Set DCD to modem sig connected to "call is up" */
/*
	Global variables 
*/	
int leapnext=0;								/* used by unneeded rcvqsub.c code - not used here*/
int dbg=0;							/* Control debug output */
int rcvlog=0;							/* control detailed packet output */
int pipes[256];						/* FDs for the pipes/one per node */
int mask[256];						/* 256*route+node to uniquely id station for pipe*/
int numpipes;						/* the number of pipes currently open */
int ttpath=0;						/* make unit to terminal line global open via 	
											tt_init*/
int norollback=0;					/* if true, do not support rollbacks */
int multiple=0;						/* if true, all data to one station proc*/
int rolltime;						/* make time of last rollback global */
int forceroll=0;					/* if true, rollbacks are force now&then*/
int tcpflg=0;						/* if True use TCP to TCPOUT for input */
char path[100]="/dev/ttya";			/* set to path name of terminal unit */
int partupd=0;						/* if <> 0, user wants partial updates*/
FILE * logout;						/* path to output log file */
char **argvcpy;						/* a copy of the argv pointer */
char logfilename[80]="";			/* if log file is open, name goes here*/
int last_logday=0;					/* day log file last opened */
char hostname[50]="nsn3.cr.usgs.gov";

char *asctim();					/* return asc time in UTC, prototype */




/*
	Main routine.  Given a serial port defined in "path", read data from it and
	call sendit with NSN packets.  Implement rollbacks per Primer 

	D.C. Ketchum Mar 1996
*/
#ifdef __STDC__
	main(int argc, char *argv[])
#else
  main(argc,argv)
  int argc;							/* user input command count */
  char *argv[];						/* user input command strings */
#endif
{
	int dtr,rts,cts,car,rng,dsr;	/* returned by read_modem with state of lines*/
	int err;						/* used by most routines returning errors */
	unsigned char in[INBUFSIZE];	/* input buffer for data bytes */
	int inpnt=0;					/* points into inbuf with char as rcved*/
	int pnt=0;						/* the emptier of input data buffer */
	int sync=0;						/* when non=zero the data stream is synced up */
	int i;						/* temporary indices */
	int nbexpect=-1;				/* number of bytes expected in packet (from hdr)*/
	int nbytes,nbyte;				/* temp variable containing # of bytes in in[] */
	int inhibit=0;					/* counts down packets that need inhibit bit set*/
	int waitinh=0;					/* if true, waiting for rollback inhibit bit set*/
	unsigned int lastseq=(unsigned) -1;		/* set to last sequence Number */
	int lastforceseq=-1;			/* used in force sequence error debug code */
	int lastleadforce=-1;			/* used in force sequence error debug code */
	struct gomberg gb;				/* a NSN data struct, Data will be copied here*/
	char *ifnd;						/* used in memchr as pointer to lead ins */
	extern char *sys_errlist[];		/* connect to list of error message strings */
	long debug2=0;					/* test variable for No data reads */
	long debug3=0;
	int yr,day,hr,min,sec,ms,leap;	/* for time code breakdown */
	time_t lasttime,looptime;
	int lastyday;					/* last day of year for log file close/opens */
	struct tm *tmpnt;				/* time pointer used in day of year calculation*/
	char keepalive[8]={ESC,STX,8,0,1,0,0,0};
/*
	Initialization
*/
	argvcpy=argv;
	logout=stdout;						/* start with terminal output */
	cmdarg(argc,argv);					/* parse any command line arguments */
	printf("RCV top3  This is the main program\n");
	looptime=time(NULL);
	if(logfilename[0] != 0)
	{	tmpnt=gmtime(&looptime);
		logfilename[strlen(logfilename)-1]=48+((tmpnt->tm_yday+1) % 10);
		printf("logfilename=%s\n",logfilename);
		logout=fopen(logfilename,"a+");
		fprintf(logout,"%s Init Open log file=%s\n",asctim(),logfilename);
	}
	exit_handler_init("RCV",user_shutdown);	/* set up exit/signal handler for sys*/
	numpipes=0;							/* No children pipes yet! */
/*
	This is a infinite loop getting data, trying to packetize or sync
	issuing rollbacks
*/
	if(multiple) 
	{	gb.lead1=0;
		gb.lead2=0;
		gb.numbyt[0]=8;
		gb.numbyt[1]=0;
		sendit(&gb);					/* start up the multiple station*/
	}
	if(tcpflg) ttpath=init_tcp(hostname);		/* open path to TCP port */
	else ttpath=init_ttin(path);		/* Open the serial Line */
	fprintf(logout,"%s Out Open path=%s ttpath=%d\n",asctim(),path,ttpath);
	if(ttpath <=0) {
		fprintf(logout,"RCV : Bad TERMINAL/TCP open %s %d\n",path,ttpath);
		exit(0);
	}

	if(tcpflg == 0) {
		DCD=0;
		while (DCD == 0) {						/* until call is up */
			err=read_modem(ttpath,&dtr,&rts,&cts,&car,&rng,&dsr);/* chk modem status */
			if(DCD == 0) {					/* is the DSR on VSAT up??? */
				fprintf(logout,"%s RCV : SERIAL : Wait for call up ... car=%d dsr=%d\n",
					asctim(),car,dsr);
				usleep(10000000);			/* sleep ten second */
			}
		}
	}
	rollback(255);						/* force a rollback, we do not know who */
	waitinh=1;							/* force rollback to start */
	fprintf(logout,"%s Top of main loop\n",asctim());
	lasttime=time(NULL);
for(;;) {								/* an infinite loop */
	nbyte=NCREAD;
	nbyte = ((INBUFSIZE-inpnt) < NCREAD) ? INBUFSIZE-inpnt : NCREAD;/* set read size*/
/*
	The read is non-blocking.  If there is no data it will return a -1 and
	errno will be set to EAGAIN which is the same as EWOULDBLOCK.  If we
	do not return any data for two minutes, init_tcp() is called to restart
	the link
*/
	err=0;
	errno=0;
	looptime=time(NULL);
	while ( (err=read(ttpath,&in[inpnt],nbyte)) == 0) { /* try to read data */
		usleep(UWAIT);					/* Let some time go by and try again */
		debug2++;
		if( (debug2 % 10000) == 0) fprintf(logout,"%s readloop %d %x %s",
			asctim(),debug2,errno,ctime(&looptime));
		if(errno == EINTR) {			/* are we in an EINTR */
			fprintf(logout,"%s RCV: EINTR found. Try reopen on TCP err=%d\n",
				asctim(),err);
			sleep(2);					/* wait a bit */
			ttpath=init_tcp(hostname);			/* try to reopen connection */
			lasttime=time(NULL);
			err=0;
		}
		errno=0;
	}
	if(err == 0) fprintf(logout,"%s RCV : EOF on Input read\n",asctim());
	if(err > 0) {
/*		fprintf(logout,"          Got %d \n         ",err);		
		for(i=0; i< err ; i++) 
		(( i% 20) == 19) ? fprintf(logout,"%3d\n          ",in[inpnt+i]) : 
							fprintf(logout,"%3d ",in[inpnt+i]);
		fprintf(logout,"\n");*/
		inpnt+=err;						/* adjust for new bytes in buffer*/
		if(inpnt >= INBUFSIZE) inpnt-=INBUFSIZE;	/* wrap around ring buffer */
		nbytes=(inpnt >= pnt) ? inpnt-pnt : inpnt-pnt+INBUFSIZE;
		if(dbg)fprintf(logout,"RCV start 1 nb=%d in=%d pnt=%d sync=%d\n",
			nbytes,inpnt,pnt,sync);fflush(logout);
/*
	Check for keepalive
*/

		if (in[index(pnt+2)] == 8 && in[index(pnt+3)] == 0 &&
			in[index(pnt)] == ESC && in[index(pnt+1)] == STX &&
			in[index(pnt+4)] == 1) {/*keep alive?? */
			lasttime=time(NULL);	/* reset timer */
			fprintf(logout,"%s RCV : Keepalive\n",asctim());
			user_heartbeat();		/* Mainly for EW */
			pnt+=8;					/* point to next leadin */
			if(pnt >= INBUFSIZE) pnt-=INBUFSIZE;	/* wrap ring if needed */
			sync=1;					/* must be synced */
			nbytes=0;
			for(i=0; i<numpipes; i++) if(pipes[i] > 0) 
				write(pipes[i],keepalive,8);
		}
		if(sync) {						/* if we are synced up */
/*			if(dbg) fprintf(logout,
				"RCV : SYNC : inpnt=%d pnt=%d nbytes=%d nbexpect=%d\n",
				inpnt,pnt,nbytes,nbexpect);*/
			if(nbexpect > 0) {					/* do we know what to expect ? */
				if(nbytes >= nbexpect) {		/* have we satisfied the packet len? */
					if(pnt+nbexpect < INBUFSIZE) 
						memcpy(&gb,&in[pnt],nbexpect);/* move GB packet*/
					else {				/* packet spans ring buffer boundary */
						memcpy(&gb,&in[pnt],INBUFSIZE-pnt);		/* move first */
						memcpy( ((char *) &gb)+INBUFSIZE-pnt,&in[0],
									nbexpect-INBUFSIZE+pnt);
					}
					inhibit= (gb.numbyt[1] & 0x80) ? 1 : 0;	/* is inhibit on */
					if(inhibit) fprintf(logout,"%s RCV : Inhibit2 on seq=%d\n",
						asctim(),gb.packseq);
					if(norollback) inhibit = 1;		/* all pkts ok by switch*/
					if(waitinh && inhibit) waitinh=0;	/* clear wait for inhibit*/
					if(waitinh == 0 ) {				/* if not waiting for inh*/
						lastseq=gb.packseq;			/* its ok, save seq number*/
						sendit(&gb);				/* send the data packet */
						lasttime=time(NULL);
						user_heartbeat();			/* let user status know*/
					} else {						/* waiting for inhibit bit*/
						fprintf(logout,"%s RCV : Wait for inh. Discard seq=%d\n",
								asctim(),gb.packseq);
						waitinh++;					/* count number we wait */
						if(waitinh > 50) {			/* trap deadlock if rolling */
							waitinh=0;				/* non-rollback stream*/
							fprintf(logout,"RCV : Danger : rollback timed out!!!\n");
							lastseq=gb.packseq;			/* force it to be o.k. */
						}
						rollback(lastseq);				/* reissure roll back */
					}
					pnt+=nbexpect;		/* point to next packet */
					nbexpect=-1;		/* do not yet know size of packet */
					if(logfilename[0] != 0)
					{	tmpnt=gmtime(&looptime);
						if(tmpnt->tm_yday != lastyday) {
							fprintf(logout,"%s Close log file=%s\n",asctim(),logfilename);
							fclose(logout);
							logfilename[strlen(logfilename)-1]=
								48+((tmpnt->tm_yday+1)%10);
							logout=fopen(logfilename,"w");
							fprintf(logout,"%s Open log file2=%s %d %d\n",
									asctim(),logfilename,tmpnt->tm_yday,lastyday);
							lastyday=tmpnt->tm_yday;
						}
					}
					if(pnt >= INBUFSIZE) pnt-=INBUFSIZE;
				} 
			} else {					/* start of packet, check out leadins/size*/
				if(nbytes >= 8 ) {
					inhibit= (in[index(pnt+3)] & 0x80) ? 1 : 0;	/* is inhibit on */
					if(dbg && inhibit) fprintf(logout,"%s RCV : Inhibit on seq=%d\n",
						asctim(),in[index(pnt+7)]);
					if(forceroll && (lastseq % 32) == 10 && lastleadforce != lastseq){
						in[pnt]=0;					/*cause rollback*/
						lastleadforce=lastseq;		/*remember - avoid infinite loop*/
					}
					if(in[pnt] != ESC || in[index(pnt+1)] != STX) {
						sync=0;						/* not expected lead in bytes */
						fprintf(logout,"%s RCV : Leadins not right %d %d\n",
							asctim(),in[pnt],in[index(pnt+1)]);
						nbexpect=0;					/* indicate we are lost */
						rollback(lastseq);			/* need to roll back */
						waitinh=1;					/* and wait for it */
					} else {						/* lead ins ok,compute length*/
						if(forceroll && (lastseq % 32) == 26 && 
								lastforceseq !=lastseq) { 
							in[index(pnt+7)]=0;		/* force bad seq*/
							lastforceseq=lastseq;
							fprintf(logout,"%s RCV : Force bad seq\n",asctim());
						}
						if(lastseq == -1) {				/* set initial packetseq */
							if(in[index(pnt+7)]== 0) lastseq=255;/* is it on wrap*/
							else lastseq=in[index(pnt+7)]-1;/* no,set to previous*/
						}
						if( ((lastseq+1) % 256) !=in[index(pnt+7)] && 
							!inhibit) {					/* is it expected */
							fprintf(logout,"%s RCV : Out of seq. expect=%d got=%d\n",
								asctim(),lastseq+1,in[index(pnt+7)]);
							rollback(lastseq);			/* send rollback */
							waitinh=1;					/* discard till rollback*/
						}					/* o.k. how many bytes expected*/
						nbexpect=in[index(pnt+2)]+256*(in[index(pnt+3)] & 7);
						if(dbg) fprintf(logout,"%s RCV : NBEXPECT=%d inpnt=%d pnt=%d\n",
							asctim(),nbexpect,inpnt,pnt);
						if(nbexpect == 8 && in[index(pnt+4)] == 0) { /* ask rlbk*/
							fprintf(logout,"%s RCV : Ask for rollback rcvd seq=%d\n",
								asctim(),lastseq);
							rollback(lastseq);			/* respond with RB */
							waitinh=1;							/* mark wait for inh*/
							pnt+=8;									/* point at lead in*/
							if(pnt >= INBUFSIZE) pnt-=INBUFSIZE;/* wrap ring */
							sync=1;									/* we must be synced */
						} else  {
							if(nbexpect < 8 || nbexpect > 2048) {/*is it in range*/
								fprintf(logout,"%s RCV : Sync lost bad nbytes=%d\n",
									asctim(),nbexpect);
								rollback(lastseq);		/* do rollback */
								waitinh=1;						/* wait for roll resp*/
								sync=0;								/* we have lost sync */
								pnt+=8;								/* look for new packet */
								if(pnt >=INBUFSIZE) pnt-=INBUFSIZE;/*wrap ring*/
							}
						}
					}
				}
			}
/*
	Not synced up.  Look for the ESC STX
*/
		} else {								/* if not synced */
			while(nbytes > 0) {					/* while we have unexamined bytes */
				if( (pnt+nbytes) > INBUFSIZE) nbytes=INBUFSIZE-pnt;/* limit to end*/
				ifnd=memchr(&in[pnt],ESC,nbytes);	/* find next ESC character */
				if(dbg) fprintf(logout,"%s RCV start 2\n",asctim());fflush(logout);
				if(ifnd != NULL) {
					i=ifnd-(char *) &in[pnt];
/*					if(dbg) fprintf(logout,"RCV : try to sync pnt=%d nbytes=%d i=%d\n",
							pnt,nbytes,i);*/
					if(i < nbytes-2) {
						if(in[pnt+i+1] == STX) {	/* possible sync up */
							pnt+=i;					/* set new start point */
							if(pnt >= INBUFSIZE) pnt-=INBUFSIZE;/* wrap ring */
							nbytes=0;				/* done with search */
							sync=1;					/* we think we are synced up */
							if(dbg) fprintf(logout,"%s RCV : Synced UP.. nbytes=%d\n",
								asctim(),nbytes);
							nbexpect=-1;			/* have not looked at header */
						} else {
							fprintf(logout,"%s RCV : Not a STX pnt=%d ipnt=%d i=%d\n",
								asctim(),pnt,inpnt,i);
							pnt+=i+1;				/* Point beyond ESC */
							nbytes-=i+1;			/* reduce space to look through */
						}
					} else {
						if(dbg) fprintf(logout,
							"%s RCV : ESC found in last byte pnt=%d inpnt=%d i=%d\n",
							asctim(),pnt,inpnt,i);
						nbytes=0;				/* force out till more chars in */
					}
				} else {
/*					if(dbg) fprintf(logout,
							"%s RCV : No esc found in nb=%d pnt=%d inpnt=%d\n",
							asctim(),nbytes,pnt,inpnt);*/
					pnt+=nbytes;					/* increment to new search spot */
					nbytes=0;						/* esc not found */
				}
			}
			if(pnt >= INBUFSIZE) pnt-=INBUFSIZE;	/* wrap ring if needed */
/*			if(dbg) fprintf(logout,
					"%s RCV : end sync loop sync=%d pnt=%d inpnt=%d\n",
					asctim(),sync,pnt,inpnt);*/
		}
	
	/*	Read did not return a positive number.  Deal with it */
	} else {
		if(err == 0 || (err < 0 && errno == EAGAIN) )/*Solaris needs EAGAIN */
		{	if( tcpflg && ((long) (time(NULL) - lasttime) > 120)) /*timeout??*/
			{	fprintf(logout,"%s RCV : time out 120 no keepalives. %d\n",
					asctim(),time(NULL)-lasttime);
				ttpath=init_tcp(hostname);
				lasttime=time(NULL);
				if(dbg)fprintf(logout,"After init_tcp %d\n",UWAIT);
			}
			debug3++;
			if( dbg &&  (debug3 % 10000) == 0) 
				fprintf(logout,"EAGAIN=%x %d %x %s",
				EAGAIN,debug3,errno,ctime(&looptime));
			usleep(UWAIT);
		} else
		{	fprintf(logout,"%s RCV : read error=%d %x\n",asctim(),err,errno);
			if(sigpipe_tcp() && tcpflg) 
			{ 	fprintf(logout,"%s RCV : SIGPIPE occurred.  Initialize link\n",
						asctim());
				fflush(logout);
				init_tcp(hostname);
				lasttime=time(NULL);
			} else if( tcpflg && errno == ECONNRESET)
			{	fprintf(logout,"%s RCV : Connection reset.  Restart link...\n",
					asctim());
				init_tcp(hostname);
				lasttime=time(NULL);
			} else if(tcpflg) 
			{	fprintf(logout,"%s RCV : Restart TCP Err=%s\n",
					asctim(),strerror(errno));
				init_tcp(hostname);
				lasttime=time(NULL);
			} else
			{	fprintf(logout,
				"%s RCV : Bad read ttpath=%d nbyte=%d err=%d errno=%x %x sigpipe=%d tcpflg=%d\n",
				asctim(),ttpath,nbyte,err,errno,ECONNRESET,sigpipe_tcp(),tcpflg);
				fflush(logout);
				exit(0);
			}
		} 
	}	/* end of else on new data in */
}		/* end of infinite for loop */
}		/* end of main */
#ifdef __STDC__
	void sendit(struct gomberg *gb)
#else
  void sendit(gb)
  struct gomberg *gb;
#endif
{	
	int idup,err;
	int pid;								/* gets pid of file after fork */
	int i;									/* points to array of pipes/masks */
	int nbytes,det;
	int p[2];								/* pipe ins and outs go here */
	static char pname[]="station";
	int yr,day,hr,min,sec,ms,leap;	/* for debug breaking up time code */
	struct nsntime tc;
	nbytes=gb->numbyt[0]+(gb->numbyt[1] & 7)*256;
	det=gb->detseq[0]+gb->detseq[1]*256;
	
	/* debug- mess with time code, print out message */
	tc.iyr=gb->tc[0];							/* set day of year */
	tc.doy=gb->tc[1];
	sscanf(asctim()+11,"%2d:%2d:%2d",&hr,&min,&sec);
	i=hr*3600+min*60+sec;
	memcpy((char *) &tc.ms,(char *) &gb->tc[2],4);/* move timecode ms to struct */
	nsnint(tc,&yr,&day,&hr,&min,&sec,&ms,&leap);
	if(rcvlog) fprintf(logout,
		"%s nb=%4d %2.2x-%2.2x-%2.2x %2.2d:%2.2d:%2.2d sq=%3d %3d%5d\n",
		asctim()+11,nbytes,gb->routeid,gb->nodeid,gb->chanid,hr,min,sec,gb->packseq,
		gb->seq, i-(hr*3600+min*60+sec));
		
	/* search through pips for recipient */
	for( i=0; i<numpipes; i++) {
		if(mask[i] == gb->routeid*256+gb->nodeid) break;/* is it the one */
	}	
	if( (i >= numpipes && multiple == 0) || (multiple && numpipes == 0) ) {				/* pipe & Child need to be created */
		if(pipe(p) <0) { perror("opening pipe!"); return;}/* better never happen*/
		if( (pid = fork () ) ==0) {			/* am I the child ? */
			close(p[1]);					/* close write pipe in child */
			close(0);						/* close standard input */
			if( (idup=dup(p[0])) != 0) { 	/* should return zero (std in) */
				fprintf(logout,"RCV : failed to dup! %d\n",idup); 
				exit(1);
			}
			argvcpy[0]=pname;
			execvp("station",argvcpy);/* replace this image with station */
			fprintf(logout,
			"RCV : Child better never get here after execv!! errno=%x\n",errno);
			exit(100);						/* or here! */
		} else {							/* end of child side of start up */
			fprintf(logout,"%s RCV : Pipe to station rt=%d node=%d fd=%d\n",
				asctim(),gb->routeid,gb->nodeid,p[1]);
			close(p[0]);					/* close read side of pipe we are parent*/
			i=numpipes;
			pipes[i]=p[1];					/* save FD for writing to this child */
			mask[i]=gb->routeid*256+gb->nodeid;	/* save unique mask for site */
			numpipes++;							/* create a new pipe */
		}
	}									/* end of do we need to start a child */
	if(multiple) i=0;					/* only one pipe if multiple true */
	err=write(pipes[i],gb, (nbytes & 0x3fff));
	if(err != (nbytes & 0x3fff)) {
		fprintf(logout,
			"%s RCV : Write to node failed err=%d errno=%x rt=%d nd=%d\n",
			asctim(),err,errno,gb->routeid,gb->nodeid);
		perror("Write to pipe failed");	
	}
}
#ifdef __STDC__
	void rollback(int seq)
#else
  void rollback(seq)
  int seq;
#endif
{
	int dtr,rts,cts,car,rng,dsr;			/* modem signals */
	extern int tcpflg;
	struct gomberg gb;
	unsigned char buf[30];					/* buffer to put encoded data */
	extern int norollback;					/* are rollback allowed */
	int it,err;
	it=time(0);								/* get time now */
	if(norollback) return;					/* not allowed, return now! */
	if(abs(it-rolltime) < 2) {				/* is it too soon to roll again */
		fprintf(logout,"%s RCV : rollback too soon! sq=%d\n",asctim(),seq);
		return;
	}	
	rolltime=it;							/* save time of last rollback */
	gb.lead1=27; gb.lead2=3;				/* set lead ins */
	gb.numbyt[0]=30; gb.numbyt[1]=0;		/* set packet length */
	gb.routeid=0; gb.nodeid=0; gb.chanid=0;	/* set null addressing */
	gb.format=4;							/* indicate rollback */
	gb.flags=seq;							/* set sequence number */
	cvtcmd(&gb.numbyt[0],buf,14);			/* convert bytes */
	memcpy(&gb.numbyt[0],buf,28);
	if(tcpflg == 0) {						/* Check modem if serial input */
		DCD=0;
		while (DCD == 0) {					/* until call is up */
			err=read_modem(ttpath,&dtr,&rts,&cts,&car,&rng,&dsr);
			if(DCD == 0) usleep(100000);	/* sleep tenth second */
		}
	}
/*	fprintf(logout,"ld1=%x ld2=%x nb1=%x nb2=%x %d %d\n",&gb.lead1,&gb.lead2,
		&(gb.numbyt[0]),&(gb.numbyt[1]),gb.numbyt[0],gb.numbyt[1] );*/
	err=writeout(ttpath,(char *) &gb,30);				/* send to end */
	if(err <=0) {
		fprintf(logout,"%s RCV rollback(): writeout err=%d errno=%x\n",
				asctim(),err,errno);
		perror("writing rollback");
	}else 
		fprintf(logout,"%s   RCV : Rollback() seq=%d \n",asctim(),seq);
	fflush(logout);
}
#ifdef __STDC__
	void cvtcmd(unsigned char *in, unsigned char *out, int nchar)
#else
  void cvtcmd(in,out,nchar)
  unsigned char *in;
  unsigned char *out;
  int nchar;
#endif
{
/*
	convert nchar of binary data bytes into printable ascii by storing each
	4 bit nibble in a separate byte biased by ASCII space 
*/
	int i=0;
	int j;
	int k;
	for(j=0; j<nchar; j++)	{			/* for each byte in */
		k=in[j];
		out[i]=k/16+32;				/* set high order nibble */
		out[i+1]= (k % 16)+32;		/* set low order nibble */
		i+=2;							/* two bytes done in out each trip */
	}
}
/* asctime returns a pointer to a string with the current GMT in it */
char *asctim()
{
	char *t;			/* pointer to string */
	struct tm *tm;
	time_t now;
	now=time(&now);		/* what time is it */
	tm=gmtime(&now);	/* convert to GMT */
	t=asctime(tm);		/* parse to a string */
	*(t+20)=0;			/* eliminate the new line and year */
	return (t);			/* hand to user */
}
