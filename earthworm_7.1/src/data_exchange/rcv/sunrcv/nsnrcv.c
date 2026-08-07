
/* This program opens two pipes to module FRONT.  One pipe received data
to send out of a serial port.  The other pipe is used to return any
characters comming in the serial port.  This is used to control the
"DATA" port on a Quanterra field processor.  PASS processes ROLLBACK
commands itself.  


Major Changes History :


Sep 1995	Add "ASK for ROLLBACK" to start after any DCD off to on.  Helps
			to control output until Golden is Synced up.  Limited input from
			front if rollback in progress.
Mar 1995 	


Look at command processing dropping commands.  Hardware problem
			And look at how slow output is hanging up.
Jun 1995	Increase Rollback buffer size to help with recovery from 
			disconnects (especially on continuous stations).
			
D.C. Ketchum Mar 1991
*/
#include <modes.h>
#include <stdio.h>
#include <errno.h>
#include <module.h>					/* Needed to get address of serial port*/
#include <sgstat.h>			/* define _sgm and _sgs descriptors */
#include <time.h>
#include "nsn.h"
#incldue "nsnrcvgbl.h"				/* Define Global variables */
#define dcdon() (((*t2read0) & dcdmask) >>3)	/* Returns 1 if DCD is ON  else zero*/
/*#define dcdon() (1)	/* Returns 1 if DCD is ON  else zero*/


char *t2read0;						/* declare pointer to read reg 0 */
int dcdmask;						/* mask to use for DCD on */
int quittime=0;
int lastsig=0;
gotsignal(signum)
int signum;
{
	lastsig=signum;
	switch(signum) {
		case (3):
			printf("    PASS : Signum=%d Set Quittime...\n",signum);
			quittime=1;
			break;
		default:
			break;
	}
}
main()
{
#define ticks 3					/* ticks to sleep*/
#define MAXROLLBACK 20			


/* Number of rollback buffers to ring */
	int syncup;					/* 0= Not synced call is down - wait for DCD,
								1=DCD is up - send rqst for ROLLBACK,
								2=DCD up and every thing is synced and happy*/
	unsigned char *malloc();	/* declare type for MALLOC */
	struct sgbuf desc;			/* Declare structure for path option descriptor*/
	static long dbg=0;			/* define debug output flag */
	static	int cmddbg=0;		/* turn on certain output commands */
	int ttpath;					/* The path number to the pipes and serial line*/	
	int err;
	short forever;				/* This is always zero to have infinite loop*/
	unsigned char sequence=250;	/* this is the GOMBERG sequence for rollback*/
	unsigned char lastgood;		/* Used by ROLLBACK logic to store last Good seq*/
	int cmd;					/* The command code in input data*/
	int rollcount=0;			/* used to control setting of ROLLBACK inhibit*/
	unsigned char inline[2048];	/* input buffer */
	unsigned char inbuf[2048];	/* input and command assembly buffers*/
	static testmode=0;
	unsigned char lastcmdchar;	/* character processed by command parser */
 	int nc,nchar,ierr,nget,i,j,nextchar,nbad;/* character counters and counts */
	short mode,perm;			/* used in open to set file modes/permissions*/
	int lastrollback,rb_deadman;/* track for infinite rollback loops */
	int naskroll=0;				/* counts consequtive ask rolls */
	struct gomberg *gb,*q;
	mod_dev *t2mod;				/* t2 mod is a pointer to a module */
	mod_dev *modlink();			/* declare type of modlink module */
	int initialsize;			/* Initialsize of pipes */
	int nextin=0;				/* pointer to q to put next pipein packet*/
	int nextout=0;				/* pointer to next block needing releif */
	int nused;					/* nextin-nextout. # not xmitted yet*/
	int tst=0;
	int lastdcd=0;				/* tracks last value of DCD ON */
	int npackout=0;
	int ncheck;					/* Used in ROLLBACK logic */
	int nsecwrite=1;			/* time to wait before next write */
	char compdate[20];
	time_t told,now,dcdtold,wtime;	/* time structures for status/rollback rqst*/
	time_t dcdlisten;			/* time DCD went high. So message is delayed */
	int dcdup;					/* if not zero, wait for 6 sec after dcdlisten
									and print message */
	int idiff;
	int totchar=0,totpack=0,totread=0,lastseq=-1;
	char gbaskroll[10];
	int _sysdate();
	unsigned char systime[4];
	int date,tick,dayweek,ticksys;
	gbaskroll[0]=27; gbaskroll[1]=3; gbaskroll[2]=8;;	/* rqst rollback packet */
	for(i=3; i<10; i++) gbaskroll[i]=0;
	dcdup=0;
	rb_deadman=0;
	syncup=0;
	lastdcd=0;
	time(&told);					/* set initial time for status timer */
	dcdtold=told;					/* and request rollback timer */
	datetime(compdate);
	printf("    PASS: as of %s\n",compdate);
/*
	Setup parameters for PIPE opens.
*/
	q=(struct gomberg *)
		malloc(MAXROLLBACK*2048);		/* Allocate space for rollback buffers */
	if(q == 0) printf("    NSNRCV : Malloc Q failed \n");	
	initialsize=2048;
	if(dbg) printf("   PASS : mode=%x  Perm=%x\n",mode,perm);
/*				OPEN Data terminal Path 			*/
	ttpath=init_tt("/dev/tty01");


	nextchar=0;	
	lastcmdchar=0;
	forever=0;
	while (forever == 0){
/*********************************************************************


		Check for input on data port (commands from GOlden).
		1) The input buffer for commands is small (<=50) 
		2) since so, data in commands is often dropped while pass is 
			suspended on a write of data.  We need to read it fre


quently.
		
***********************************************************************/
		
			nget=read(ttpath,inline,nchar);
			if(cmddbg)	printf("    PASS : nget=%d errno=%d raw buf. : \n",nget,errno);
			if( (nget == -1) && (errno == 244) ) { /* buffer overrun ???*/
				nextchar=0;							/* abort command in progr*/
				printf("    PASS : cmd i


nput overrun. Abort command \n");
				syncup=1;						/* rqst rollback state */
			}
/*			if(cmddbg) {
				for (j=0; j< nget; j++) {
					printf("%4d",inline[j]);
					if(j % 20 == 19) printf("\n");
				}
				printf("\n");
			}*/
			if(nget < 0) printf("    PASS : err read nch=%d errno=%x\n",nget,errno);
			j=0;
			for (i=0; i<nget; i++) {
				if(inline[i] == XON || inline[i] == XOFF) 
					printf("     PASS : Purging XON/XOFF %d\n",inline[i]);
				else {
					inline[j]=inline[i];
					j++;
				}
			}
			ng


et=j++;
/*			printf("    PASS : Read in of nchar=%d is NGET=%d\n",nchar,nget);*/
			nbad=0;
			i=0;
			while (i < nget) {		/* Until we run out of characters*/
				if(nextchar ==0) {	/* We are trying to sync up */
					if(inline[i] ==3 && (lastcmdchar ==27 || i == 0)) {
						nextchar=2;
						inbuf[1]=27;
						nc=7;		/* we need 7 characters to compute length*/
						if(dbg) printf("    PASS : nc in cmd=%d\n",nc);
					} else {
						if(inline[i] != 27) {
							nbad++;
							printf(((nbad%15) == 1) ?
			


				 "\n    PASS purge cmd char %4d" : "%4d",inline[i]);
							 syncup=1;			/* set rollback rqst mode */
						}
					}
				}
				if(nextchar != 0) {
					inbuf[nextchar]=inline[i];	/* Put character in cmd buf */
					if(nextchar == 6) {		/* Do we have enough to know bytes*/
						nc=(inbuf[3]-32)*16+(inbuf[4]-32)+(inbuf[6]-32)*256;
						if(cmddbg) printf("    PASS : Cmd len=%d %x %x %x\n",
						nc,inbuf[2],inbuf[3],inbuf[4]);
						if(nc < 14 || nc > 256) {
							printf("    PASS : Cmd len out of rang


e=%d\n",nc);
							nextchar=-1;		/* go to unsynced state */
							syncup=1;			/* go to rqst rollback state */
							nc=1;
						}
					}
					nextchar++;
					if(nextchar >nc && nc > 14) {
						if(dbg || cmddbg) {
							printf("    PASS :Send CMD to Front : \n");
							for (j=1; j<= nc; j++) {
								printf("%4d",inbuf[j]);
								if(j % 20 == 19) printf("\n");
							}
							printf("\n");
						}
/***************************************************************************


Command buffer is comp


lete.  Either handle it or send it to FRONT


**************************************************************************/
						cmd=0;
						cmd=(inbuf[27]-32)*16+(inbuf[28]-32);
						if(cmddbg) printf("    PASS : cmd=%d \n",cmd);
						switch (cmd) {
							case ROLLBACK:
							syncup=2;				/* got rollback, state=2*/
							naskroll=0;
							if(rollcount > 0 ) {
/*								rollcount=4;*/
								printf("Rollback in progress.  ROLLBACK ignored\n");
								nextchar=0;
							} else {
								lastgood=(in


buf[29]-32)*16+inbuf[30]-32;
								if(lastgood == lastrollback) rb_deadman++;
								else rb_deadman=0;
								lastrollback=lastgood;		/* track last rlbk*/
								ncheck=(npackout < MAXROLLBACK) ? /* limit if startup*/
									npackout : MAXROLLBACK;
								printf("    PASS : Roll to %d max=%d chk=%d",
								lastgood,MAXROLLBACK,ncheck);
								rollcount=1;		/* insure inhibit bits */
								for (j=0; j< ncheck; j++) {
 									if( (q+j)->packseq == lastgood) {
										printf(" - Fnd seq j=%d 


old nxtout=%d\n",
										j,nextout);
										nextout=j;	/* restart it */
										break;
									}
								}
								if((j == ncheck && ncheck != MAXROLLBACK) ||
									rb_deadman > 5) {
									if(rb_deadman > 5 ) printf("- Deadman expire");
									printf("- Not found. Startup. Ignore.\n");
									rollcount=4;
								}
								if(j >= MAXROLLBACK) {
									nextout=(nextin+2) % MAXROLLBACK;
									rollcount=4;
									printf(" - Not Found!! set to nextin+2=%d\n",nextout);
									for(i


=0; i<MAXROLLBACK; i++) 
										printf("%d ",(q+i)->packseq);
									printf("\n");
								}
								nextchar=0;		
							}
							break;
							default:
							ierr=write(outpipe,&inbuf[1],nc);
							nextchar=0;
							break;
						}
					}		/* end of cass on command byte */
				}			/* end of if on enough characters */
				lastcmdchar=inline[i];
				i++;
			}
			if(nbad > 0) printf("\n");
			if(_gs_rdy(ttpath) == 0) {
				tsleep(2);	/* if we are empty, sleep */
			}
		}				/* No characters available


 from terminal line*/
		if(nget == 0) {
			tsleep(ticks);			/* sleep a little while*/
		}
		time(&now);							/* get the time */
		idiff=difftime(now,told);				/* differnce in time */
		if(dcdup) {
			idiff=difftime(now,dcdlisten);
			if(abs(idiff) > 6) {
				printf("   PASS : DCD Up Listening again ....\n");
				dcdup=0;
			}
		}
}
