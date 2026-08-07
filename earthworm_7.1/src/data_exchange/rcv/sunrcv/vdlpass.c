/*This program is a desendant of the PASS.C field code for the Quanterra
	adapted to serve a serial port on a Sun / Unix system.  It is very
	different from PASS in that it can handle multiple inputs from more
	than one VDL or FRONT via the UNIX socket system.  These can be local
	sockets (AF_UNIX) which are files in the file system, or internet like
	sockets where data comes to a well known "port".  
       
D.C. Ketchum Feb 1998 Initial functionality

command line switches
-passout ttt  ttt becomes the serial device name to use.
-passinport 	"Well Known" port this server will respond to 
-passtcp 		dotted.domain.address.  
-passport nn 	well know port number to use (TCP output socket only). 
				If 0, no output TCP feeds

Note that the serial port should monitor the DSR (pin 6) on a DPU for 
"call is up" flow control.  Since hookups vary the definition of 
"DCD" below should be set to the correct status line from the port
as returned by "read_modem".  This value is used in function dcdon()
to check this flow control.

*/
#define DCD dsr				/* or dtr, rts, cts, car, rng or dsr to detect
								the DSR from pin 6 of the DPU */
/* Normal includes! */
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <time.h>
#include <strings.h>
#include "vdl.h"

/* Define command bytes from GOLDEN */
#define ASKTIME 42				/* SP request a time mark from master*/
#define ASKCONFIG 43			/* SP request configuation info from master*/
#define ASKTRIGGER 44			/* SP request trigger parameters */
#define TONETIME 40				/* SP reports a time mark */
#define ANSTIME 45
#define ANSCONFIG 46
#define ANSTRIGGER 47
#define ROLLBACK 4				/* Golden wants a retransmission of data */
#define NSNRESTART 41			/* USNSN has restarted.  Resend partials.*/
#define FRONTRESTART 4			/* USNSN in golden wants time/quanterra reset*/
#define GETABSCORRECT 49        /* We want to know our absolute right now*/
#define SETRATE 32				/* Golden wants to change the clock rate*/
#define ABSCORRECT 33

/* Other defs */
#define TICKS 1000000                 /* ticks to sleep*/
#define MAXROLLBACK 32          /* Number of rollback buffers to ring */
#define MAX_CONNECT 50			/* Maximum number of connections */


FILE *logout;					/* log file name */
char compdate[20];
int rollcount=2;				/* used to control setting of ROLLBACK inhibit*/
int lastdcd=0;          	    /* tracks last value of DCD ON */
int nextin=0;					/* pointer to q to put next pipein packet*/
int nextout=0;					/* pointer to next block needing releif */
int naskroll=0;					/* counts consequtive ask rolls */
int npackout=0;					/* counts packets going out, limits rollbacks*/
struct gomberg *q;
int syncup;						/* 0= Not synced call is down - wait for DCD,
									1=DCD is up - send rqst for ROLLBACK,
									2=DCD up and everything is synced - happy*/
int gbaskroll=0;				/* tracks ask roll function */
int nused;						/* estimate of # of free packet slots */
static int dbg=0;				/* define debug output flag */
static int cmddbg=0;			/* turn on certain output commands */
int ttpath;						/* path to the serial port */

char ttdev[20]="";				/* string with serial port name */
char outaddr[40]="";			/* string with output TCP host */

int port;						/* port # of output, if TCP sockets allowed */
int tcp_port=0;					/* port # for VDLPASS to serve */
char tcpaddr[50];				/* TCP dotted address if allowed */
char tag[6]="VDLPS";			/* for tcp routine tag output */
int totchar=0,totpack=0,totread=0,lastseq=-1;



main(argc,argv)
char **argv;
int argc;
{
	fd_set read_ready,write_ready,special_ready;
	fd_set read_master,write_master,special_master;
	struct timeval timeout;
	int well_known,well_known_tcp;		/* the well known sockets to contact */
	struct sockaddr_un addr;			/* address structure */
	struct sockaddr_in server;			/* Internet servers structure */
	int fd[MAX_CONNECT];	 			/* file descriptors for sockets */

	int something,topsock;
	int i,k,err,nb;
	int deleted;
	/* initialize timeout */
	for (i=0; i<MAX_CONNECT; i++) fd[i]=-1;
	q = (struct gomberg *) malloc(MAXROLLBACK*2048);
	timeout.tv_sec=2;					/* seconds of timeout */
	timeout.tv_usec=100000;				/* usec of timeout */
	logout=stdout;						/* start with logout as stdout */

	/* Get arguments from invocation */
	cmdarg(argc,argv,ttdev);		/* parse commands into globals */
	if(ttdev[0] == 0) ttpath=init_tcp();
	else ttpath=init_ttout(ttdev);		/* open the output serial port */
	if(ttpath < 0) fprintf(logout,"Bad Serial port open %d errno=%x\n",
		ttpath,errno);
	else 
	{	nb=0;
		while ( (k=read(ttpath,&i,1) > 0)) nb++;
		fprintf(logout,"Serial line %s is open on fd %d purged=%d\n",
			ttdev,ttpath,nb);
	}
	
	/* create and bind well known sockets */
	fprintf(logout,"Open sockets\n");
	if( (well_known=socket(AF_UNIX, SOCK_STREAM,0)) < 0)
	{	perror("Socket Unix failed");
		exit(5);
	}

	/* if tcp is allowed as input, set up the port for servicing*/
	if(tcp_port > 0)
	{	if( (well_known_tcp=socket(AF_INET, SOCK_STREAM,0)) < 0)
		{	perror("Socket TCP failed");
			exit(6);
		}
		fprintf(logout,"Well_known_tcp=%d Tcp_port=%d Tcpaddr=%s for INET\n",
			well_known_tcp,tcp_port,tcpaddr);
		server.sin_family=AF_INET;
		strcpy( (char *) &server.sin_addr, tcpaddr);
		server.sin_port=tcp_port;
		if(bind(well_known_tcp,(struct sockaddr *) &server, sizeof server))
		{	fprintf(logout,"binding socket return=%x\n",errno);
			perror("Bind internet socket failed");
			exit(8);
		}
		if(listen(well_known_tcp,5) < 0) 
		{	perror("listen INET failed");
			exit(7);
		}
	}
	strcpy(addr.sun_path,"PASS_SOCKET");
	addr.sun_family=AF_UNIX;
	deleted=0;
REBIND:
	if(bind(well_known,(struct sockaddr *) &addr,strlen(addr.sun_path)+
		sizeof(addr.sun_family)) < 0)
	{	if( errno == EADDRINUSE && deleted == 0)
		{	fprintf(logout,"ADDR in Use : Delete PASS_SOCKET\n");
			system("rm -f PASS_SOCKET");
			deleted=1;
			goto REBIND;
		}
		perror("Bind pass_socket failed");
		exit(4);
	}
	if (fcntl(well_known, F_SETFL, O_NDELAY) <0 )
	{	perror("Failed to set main socket to non-blocking");
		exit(1);
	}

	/* start listening */
	fprintf(logout,"start listening\n");
	if(listen(well_known,5) < 0) 
	{	perror("listen Unix failed");
		exit(7);
	}
	if(well_known > topsock) topsock=well_known;/* FDs needing to be checked */
	if(well_known_tcp > topsock) topsock=well_known_tcp;

	FD_ZERO(&read_master);			/* clear those ready to read */
	FD_ZERO(&write_master);			/* clear those ready to write */
	FD_ZERO(&special_master);		/* specials not implemented */
	FD_SET(well_known,&read_master);/* the listener is ready */
	if(tcp_port > 0) FD_SET(well_known_tcp,&read_master);
	FD_SET(ttpath,&read_master);	/* set read bit for serial line */
	FD_SET(ttpath,&write_master);	/* set write bit for serial line */

	/* forever, loop looking for connections and data */
	for(;;) 
	{	something=0;				/* nothin has happened yet */
		read_ready=read_master;			/* set masks from master copies */
		write_ready=write_master;
		special_ready=special_master;
		if(dbg) fprintf(logout,"Top of loop\n");

		if( (nb=select(topsock+1, &read_ready, &write_ready, &special_ready,
			&timeout)) < 0)
		{	perror("Select failed");
			continue;
		}
		if(dbg) fprintf(logout,"select nb=%d topc=%d well=%d %x IS_SET=%d\n",nb,
			topsock,well_known,read_ready,FD_ISSET(well_known,&read_ready));

		/* check for new connection and add to fd and select lists */
		if( nb > 0 && FD_ISSET(well_known,&read_ready))
		{	for(i=0; i<MAX_CONNECT; i++) if(fd[i] == -1) break;
			if(i == MAX_CONNECT) 
			{	fprintf(logout,"Out of sockets connections\n");
				exit(2);
			}
			fprintf(logout,"Found new Unix connect %d\n",i);
			something=1;
			fd[i]=accept(well_known,(struct sockaddr *) 0, (int *) 0);
			if(fd[i] <0) 
			{	perror("accept failed!");
				exit(8);
			} else 
			{	fprintf(logout," assign to fd=%d\n",fd[i]);
				FD_SET(fd[i],&read_master);			/* put in select set */
				FD_SET(fd[i],&write_master);
				FD_SET(fd[i],&special_master);
			}
			if(fcntl(fd[i], F_SETFL, O_NDELAY) < 0) 
			{	perror("Failed to set socket to non-blocking");
				exit(3);
			} else
			{	if(fd[i] > topsock) topsock=fd[i];	/* keep track of biggest*/
			}
		}

		if( tcp_port > 0 && nb > 0 && FD_ISSET(well_known_tcp,&read_ready))
		{	for(i=0; i<MAX_CONNECT; i++) if(fd[i] == -1) break;
			if(i == MAX_CONNECT) 
			{	fprintf(logout,"Out of sockets connections\n");
				exit(2);
			}
			fprintf(logout,"Found new TCP connect %d\n",i);
			something=1;
			fd[i]=accept(well_known_tcp,(struct sockaddr *) 0, (int *) 0);
			if(fd[i] <0) 
			{	perror("accept TCP failed!");
				exit(8);
			} else 
			{	fprintf(logout," assign to TCP fd=%d\n",fd[i]);
				FD_SET(fd[i],&read_master);			/* put in select set */
				FD_SET(fd[i],&write_master);
				FD_SET(fd[i],&special_master);
			}
			if(fcntl(fd[i], F_SETFL, O_NDELAY) < 0) 
			{	perror("Failed to set TCP socket to non-blocking");
				exit(3);
			} else
			{	if(fd[i] > topsock) topsock=fd[i];	/* keep track of biggest*/
			}
		}


		/* for all connection, look for input */
		if(nb > 0) for(i=0; i<MAX_CONNECT; i++)
		{	if(fd[i] > 0) if(FD_ISSET(fd[i],&read_ready))
			{	fprintf(logout,"Input on fd=%d i=%d\n",fd[i],i);
				if( (err=read_it(fd[i])) < 0)	/* read data from socket*/
				{	perror("Read socket error");
				} else if(err > 0) something=1;
			}
		}

		/* Check output to serial line */
		if(FD_ISSET(ttpath,&write_ready))		/* available to write?? */
		{	err=chk_serial(ttpath);
			if(err > 0) 
			{	something=1;
				fprintf(logout,"ready to write on ttpath %d\n",ttpath);
			} else if(dbg) fprintf(logout,"Chkserial=%d\n",err); fflush(logout);
		}
		if(FD_ISSET(ttpath,&read_ready))		/* Available command read?? */
		{	err=chk_serial_cmd(ttpath);
			if(err > 0) something = 1;
			else if(dbg) fprintf(logout,"no cmd %d\n",err);
		}


		/* If nothing happened on that loop, then sleep awhile */
/*		fprintf(logout,"Sleep %d ticks\n",TICKS);*/
		if( something == 0) usleep(TICKS);
	}				/* end of infinite FOR loop */
}
	
/*************************************************************************

    Check for data from PIPE to FRONT.  To read more data from pipe nextin+1
    cannot lap nextout. (NEXTIN=NEXTOUT means no buffers in use).

***************************************************************************/
read_it(inpipe)
int inpipe;
{	
    int err;
    static unsigned char sequence=250; /* output GOMBERG sequence for rollback*/
    unsigned char lastgood;     /* Used by ROLLBACK logic to store last Good seq*/
    int cmd;                    /* The command code in input data*/
    unsigned char inbuf[256];   /* input and command assembly buffers*/
    unsigned char lastcmdchar;  /* character processed by command parser */
    int nc,nchar,ierr,nget,i,j,nextchar,nbad;/* character counters and counts */
    int lastrollback;			/* track for infinite rollback loops */
    struct gomberg *gb;
    int nused;                  /* nextin-nextout. # not xmitted yet*/
    int tst=0;
	extern int npackout;		/* link to total packets out */
    int ncheck;                 /* Used in ROLLBACK logic */
    int nsecwrite=1;            /* time to wait before next write */
    int idiff;
    char gbaskroll[10];
    char ttdev[6],temp[20];
    gbaskroll[0]=27; gbaskroll[1]=3; gbaskroll[2]=8; /* rqst rollback packet */
	nused=nextin-nextout;               /* how many unxmited packets */
	if(nused < 0) nused+=MAXROLLBACK;   /* if way behind, don't get more!*/
	if(dbg) fprintf(logout,"read_it : nused=%d dcd=%d nextin,out=%d %d\n",
		nused,dcdon(),nextin,nextout);
	if(dcdon() == 0 || ((nextin+1) % MAXROLLBACK) ==nextout
          || nused > MAXROLLBACK/4) return 0;
	gb=q+nextin;				/* pointer to gomberg packet*/	
	errno=0;
	err=0;
	while ( (err=read(inpipe,gb,8)) <= 0)	/* get beginning of packet */
	{	fprintf(logout,"Read_it err inpipe=%d err=%d errno=%x\n",
			inpipe,err,errno);
		if(err == 0 && errno == 0) return 0;	/* nothing to read */
		if(err < 0 && errno == EWOULDBLOCK) 
		{	usleep(TICKS);
		} else if(err < 0 && errno == ECONNRESET)
		{	close(inpipe);
			return 0;
		} else if(err < 0)
		{	fprintf(logout,"Read error err=%d errno=%x inpipe=%d ",
				err,errno,inpipe);
			perror("errtxt=");
			exit(3);
		}
	}
	nchar=gb->numbyt[0] + (gb->numbyt[1]*256 & 0xfff);
	fprintf(logout,"Read_it inpipe=%d err=%d errno=%x nchar=%d\n",
			inpipe,err,errno,nchar);

	if(dbg) printf("    PASS : from pipe. NCHAR=%d nextin=%d seq=%d\n",
			nchar,nextin,gb->packseq);
	nget=read(inpipe,( (char *) gb)+8,nchar-8);/* read in rest of packet */
	if(nget != nchar-8)               /* check for unexpected error */
		 printf("GB pipe get read size wrong. Nchar=%d nget=%d\n",
			nget,nchar);
	totread+=nchar;
	nextin=(nextin+1)%MAXROLLBACK;  /* set next place to put data */
	gb->packseq=sequence;           /* set packet outbound sequence*/
	sequence++;
	npackout++;
	return nchar;
}   
int chk_serial(ttpath)
int ttpath;
{
	time_t now,wtime;					/* internal time strcuts */
	static time_t told;					/* static keeps time across calls */
	static time_t dcdlisten;			/* time when DCD came on */
	static time_t dcdtold;				/* track time of DCD across calls */
	int idiff;
	static int lastdcd;					/* tracks DCD setting across calls */
	static int dcdup=0;					/* when set messages print? */
	extern int syncup;					/* global when true in "ask roll" wait*/
	int nget,nchar;
	extern int dbg;
	extern int lastseq;					/* last sequence # assigned */
	extern int rollback;				/* global with inhibit countdown */
	extern int syncup;					/* tracks progress of roll back syncs*/
	extern int nused;					/* number of unused packets */
	char gbaskroll[10]={27,3,8,0,0,0,0,0,0,0};/* used in ask roll logic */
	struct gomberg *gb;					/* pointer to GB packet to use */
	time(&now);							/* get current time */
	idiff=difftime(now,told);
	if(idiff >= 311) {
		told=now;                           /* set old time for next time*/
		fprintf(logout,
			"   PASS :%d in=%d out=%d #pk=%d sq=%d us=%d dcd=%d\n",
			idiff,totread*8/idiff,totchar*8/idiff,totpack,lastseq,nused,
			dcdon());
		totread=0;
		totchar=0;
		totpack=0;
	}
/***********************************************************************

    Now if data is available send it out the terminal port
    
*********************************************************************/
	/*   Check for DCD ON (X.25 call connected.  If not, Wait for it */
	if(dcdon() != lastdcd)                /* is DCD down ???? */
	{	if(dcdon() == 0) 
		{	fprintf(logout,"   PASS : DCD Down Sleep\n");
			syncup=0;                       /* Call down, sync from start*/
		}else 
		{	time(&dcdlisten);
			dcdup=1;                    /* set dcdup so message prints*/
			syncup=1;                   /* set rollback rqst mode */
		}
		lastdcd=dcdon();
	}

	if(syncup == 1)						/* are we in rqst rollback mode? */
	{	time(&now);						/* get time now */
		idiff=difftime(now,dcdtold);	/* get diff in seconds */
		if(abs(idiff) > 15)				/* only send one every 15 secs */
		{	naskroll++;
			fprintf(logout,"   PASS: ASKRL %d %5d lseq=%d %s",
			naskroll,idiff,lastseq,ctime(&now));
			nget=write(ttpath,gbaskroll,8);/* send rqst for ROLLBACK */
			if(nget != 8) fprintf(logout,"   PASS : ASKROLL wrt tt=%d err=%d %x\n",
				ttpath,nget,errno);
			time(&dcdtold);
		}
	}
	time(&now);                    		/* get time */
	idiff=difftime(now,wtime);			/* how long has it been */
	if(dcdup) 
	{	idiff=difftime(now,dcdlisten);
		if(abs(idiff) > 6) 
		{	fprintf(logout,"   PASS : DCD Up Listening again ....\n");
			dcdup=0;
		}
	}
  
	/* has limit of rate time passed */
	if(dbg) fprintf(logout," chks in,out=%d %d dcd=%d sync=%d dcdup=%d\n",
		nextin,nextout,dcdon(),syncup,dcdup);
	if(nextin != nextout && dcdon() != 0 && syncup == 2 && dcdup == 0 )
	{	gb=q+nextout;					/* pointer to gomberg to send*/
		nchar=gb->numbyt[0]+(gb->numbyt[1]*256 & 0xfff);/* Nbyte in packet */
		if(dbg)fprintf(logout,"    PASS: to DPU nchar=%d nextout=%d\n",
				nchar,nextout);
		if(rollcount > 0) 
		{	gb->numbyt[1]|= 0x80;
			fprintf(logout,"    PASS : Inh Set =%d seq=%d ch=%d chsq=%d\n",
			rollcount,gb->packseq,gb->chanid,gb->seq);
			rollcount--;
		} else  gb->numbyt[1]&= 0x7f;;
		fprintf(logout,"PASS : writing seq=%d nchar=%d nextout=%d %d %d %d\n",
			gb->packseq,nchar,nextout,gb->routeid,gb->nodeid,gb->chanid);
		lastseq=gb->packseq;            /* save packet setquence */
		time(&wtime);               /* get current time */
		nget=write(ttpath,gb,nchar);    /* write it out */
		if(nget != nchar) 
		{	fprintf(logout,"    PASS: inc wr %d %d\n",nchar,nget);
		}
/*		nsecwrite=nchar*8/3000;         /* how long to wait til next write*/
		time(&now);
		idiff=difftime(now,wtime);      /* how long did write take */
		if(abs(idiff) > 180)            /* did it take a 3 minutes*/
		{	syncup=1;                   /* call might have happened be sure*/
			fprintf(logout,"   PASS :LNG Wr resync %d seq=%d\n",
				idiff,gb->packseq);
		}
		totchar+=nchar;
		totpack++;
		nextout=(nextout+1) % MAXROLLBACK;
		return nchar;
	}
	return 0;							/* wrote nothing */
}

int dcdon()
{
	int dtr,rts,cts,car,rng,dsr;
	read_modem(ttpath,&dtr,&rts,&cts,&car,&rng,&dsr);
	if(DCD) return 1;		/* note :DCD is #defined to be one of the signals*/
	else return 0;		
}

int chk_serial_cmd(ttpath)
int ttpath;
{
#define XON 17
#define XOFF 19	
	extern int dbg;
	int i,j;
	int nbad,lastcmdchar,lastgood,lastrollback,rb_deadman;
	static int nextchar=0;
	extern int npackout;
	char inbuf[256],inline[256];
	int ierr;
	int outpipe;
	int cmd;				/* the command code decoded */
	int ncheck;				/* used in rollback logic */
	int nc,nget;
	nget=read(ttpath,inline,256);
	if(nget < 0) 
	{	fprintf(logout,"    PASS : err read nch=%d errno=%x\n",nget,errno);
		return -1;
	}
	if(nget == 0)
	{	fprintf(logout,"cmd read = 0???\n");
		return 0;
	}
	if(dbg) fprintf(logout,"CHKCMD nget=%d tt=%d %d %d %d %d %d %d\n",nget,
		ttpath,inline[0],inline[1],inline[2],inline[3],inline[4],inline[5]);
	fflush(logout);
	/* purge any control chars XON or XOFF from command line */
	j=0;
	for (i=0; i<nget; i++) 
	{	if(inline[i] == XON || inline[i] == XOFF) 
			fprintf(logout,"     PASS : Purging XON/XOFF %d\n",inline[i]);
		else 
		{
			inline[j]=inline[i];
			j++;
		}
	}
	nget=j++;

	/* read through characters and try to make command packet */
	nbad=0;
	i=0;
	while (i < nget)      /* Until we run out of characters*/
	{	if(nextchar ==0)   /* We are trying to sync up */
		{	if(inline[i] ==3 && (lastcmdchar ==27 || i == 0))
			{	nextchar=2;
				inbuf[1]=27;
				nc=7;       /* we need 7 characters to compute length*/
				if(dbg) printf("    PASS : nc in cmd=%d\n",nc);
			} else 
			{	if(inline[i] != 27) 
				{	nbad++;
					if( (nbad % 15) == 1) 
						printf("\n    PASS purge cmd nget=%d i=%d %4d",
							nget,i,inline[i]);
/*					else printf("%4d",inline[i]);*/
					syncup=1;          /* set rollback rqst mode */
				}
			}
		}
		if(nextchar != 0) 
		{	inbuf[nextchar]=inline[i];  /* Put character in cmd buf */
			if(nextchar == 6)      /* Do we have enough to know bytes*/
			{	nc=(inbuf[3]-32)*16+(inbuf[4]-32)+(inbuf[6]-32)*256;
				if(cmddbg) printf("    PASS : Cmd len=%d %x %x %x\n",
						nc,inbuf[2],inbuf[3],inbuf[4]);
				if(nc < 14 || nc > 256) 
				{	printf("    PASS : Cmd len out of range=%d %x %x %x\n",nc,
						inbuf[2],inbuf[3],inbuf[6]);
					nextchar=-1;        /* go to unsynced state */
					syncup=1;           /* go to rqst rollback state */
					nc=1;
				}
			}
			nextchar++;
			if(nextchar >nc && nc > 14) 
			{	if(dbg || cmddbg) 
				{	printf("    PASS :Send CMD to Front : \n");
					for (j=1; j<= nc; j++) 
					{	printf("%4d",inbuf[j]);
						if(j % 20 == 19) printf("\n");
					}
					printf("\n");
				}
/***************************************************************************

Command buffer is complete.  Either handle it or send it to FRONT

**************************************************************************/
				cmd=0;
				cmd=(inbuf[27]-32)*16+(inbuf[28]-32);
				if(cmddbg) printf("    PASS : cmd=%d \n",cmd);
				switch (cmd) 
				{	case ROLLBACK:
					syncup=2;               /* got rollback, state=2*/
					naskroll=0;
					if(rollcount > 0 ) 
					{	printf("Rollback in progress.  ROLLBACK ignored\n");
						nextchar=0;
					} else 
					{	lastgood=(inbuf[29]-32)*16+inbuf[30]-32;
						if(lastgood == lastrollback) rb_deadman++;
						else rb_deadman=0;
						lastrollback=lastgood;      /* track last rlbk*/
						ncheck=(npackout < MAXROLLBACK) ? /* limit if startup*/
							npackout : MAXROLLBACK;
						printf("    PASS : Roll to %d max=%d chk=%d",
							lastgood,MAXROLLBACK,ncheck);
						rollcount=1;        /* insure inhibit bits */
						for (j=0; j< ncheck; j++) 
						{	if( (q+j)->packseq == lastgood) 
							{	printf(" - Fnd seq j=%d old nxtout=%d\n",j,nextout);
								nextout=j;  /* restart it */
								break;
							}
						}	
						if((j == ncheck && ncheck != MAXROLLBACK) || rb_deadman > 5)
						{	if(rb_deadman > 5 ) printf("- Deadman expire");
							printf("- Not found. Startup. Ignore.\n");
							rollcount=4;
						}
						if(j >= MAXROLLBACK) 
						{	nextout=(nextin+2) % MAXROLLBACK;
							rollcount=4;
							printf(" - Not Found!! set to nextin+2=%d\n",nextout);
							for(i=0; i<MAXROLLBACK; i++) 
							{	if( (i%20) == 19) printf("%d\n",(q+i)->packseq);
								else printf("%d ",(q+i)->packseq);
							}
							printf("\n");
						}
						nextchar=0;     
					}
					break;
					default:
					ierr=write(outpipe,&inbuf[1],nc);
					nextchar=0;
					break;
				}	 	/* end of casE on command byte */
			}           /* end of if on enough characters */
			if(nbad > 0) printf("\n");
		}		/* end of if nexchar != 0 */
		lastcmdchar=inline[i];
		i++;
	}			/* end of while more characters*/
	return nget;
}


#ifdef __STDC__
  void cmdarg(int argc, char *argv[],char *ttdev)
#else
  void cmdarg(argc,argv,ttdev)
  int argc;
  char *argv[];
	char *ttdev;
#endif
{	int i;
	extern FILE *logout;
	extern char tcpaddr[];
	extern int tcp_port;			/* port to advertise for input */
	extern int port;				/* port to hit with output */
	extern int dbg;
	fprintf(logout,"#args=%d\n",argc);
	for (i=1; i<argc; i++)
	{	fprintf(logout,"%d #%s% \n",i,argv[i]);
		if(strcmp(argv[i], "-passtcp") == 0)
		{	strcpy(outaddr,argv[i+1]);
			fprintf(logout,"TCP Addr=%s\n",outaddr);
		}
		if(strcmp(argv[i],"-passport") == 0)
		{	port=atoi(argv[i+1]);
			fprintf(logout,"TCP port=%d\n",port);
		}
		if(strcmp(argv[i],"-pass!") == 0)
		{	dbg=1;
			fprintf(logout,"Dbg on\n");
		}
		if(strcmp(argv[i],"-passout") == 0)
		{	strcpy(ttdev,argv[i+1]);
			fprintf(logout,"TT device=%s\n",ttdev);
		}
		if(strcmp(argv[i],"-passinport") == 0)
		{	tcp_port=atoi(argv[i+1]);
			fprintf(logout,"PASS well known port=%d\n",tcp_port);
		}
	}
	return;
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
