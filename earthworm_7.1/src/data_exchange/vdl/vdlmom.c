/*
	VDLMOM.C - This is a parent who execvp off a bunch of children using command 
	lines stored internally.  Each command can be entirely different.  When any 
	of the children die, the signal information is printed and the process restarted.
	In this way a fixed number of processes can be kept going as long as the
	this process remains up.  First written to keep a series of VDL processes
	running at UCSD using the NRTS_TAP interface. 

	Put the command lines to execute in file "vdlmom.setup" on default path.  VDLMOM
	will read these in and spawn a process for each line.  Lines which start with "!"
	are considered to be comments and ignored.   

	If VDLMOM dies or is killed via the kill command, then it will kill each of the
	children, wait for all to die, and then exit.  In some rare cases the children
	have managed to survive and no progress has been made on figuring out how this
	can happen.

	MAXCMDS	sets the array limit for the number of commands VDLMOM can maintain.
	
	each child maintains a file with a unique ID.  The base of the name comes from the
	-i in its command line.  This has the extension ".vdlmom" and contains information
	on the time of processing of data.  If this file is not being updated, VDLMOM will
	kill the child periodically to get it going again.

	D.C. Ketchum May 1995
*/
#define MAXCMDS 100
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <time.h>
static FILE *logout;
char *asctim();
char *sigtostr();
char *cmds[MAXCMDS];
int pid[MAXCMDS];
FILE *monfile[MAXCMDS];
long lastnsamp[MAXCMDS];
char *tags[MAXCMDS];


/*
	catch all signals.  If its just a SIGCHLD, return and let main loop restart the
	child.  All others, exit after closing down all children.
*/
void cc_handler(arg)
int arg;
{
	int done,i,statusp,ipid,low,low2;
	fprintf(logout,"%s CC_HANDLER: Signal received=%d %s\n",
		asctim(),arg,sigtostr(arg));  	
	fflush(logout);
	if(arg == SIGCHLD) return;
	
	/* kill all of the children */
	for(i=0; i<MAXCMDS; i++) 
	{	if(pid[i] > 0) 
		{	fprintf(logout,"%s Send QUIT to PID=%d\n",asctim(),pid[i]);
			kill(pid[i],SIGTERM);
		}
	}
	
	/* wait for all children to exit, process their termination */
	done=0;
	while (done == 0 ) 
	{	ipid=wait(&statusp);
		fprintf(logout,"\n\n********************************\n");
		fprintf(logout,"%s Child with pid=%d has died.  Status=%x\n", 
			asctim(),ipid,statusp);
		for(i=0; i< MAXCMDS; i++) if(ipid == pid[i]) break;/* which child is it */
		fprintf(logout,"child is number %d line=%s\n",i,cmds[i]);
		low=statusp & 0x7f;
		low2=(statusp & 0xff00) / 256;
		if(low == 0177) 
		{	fprintf(logout,"Child stopped with signal=%x\n",low2);
			fprintf(logout,"signal=%s\n",sigtostr(low2));
		} else if (low != 0) 
		{	fprintf(logout,"Child terminated due to signal=%x # %s\n",
				low,sigtostr(low));
		} else 
		{	fprintf(logout,"Child did exit=%d\n",low2);

		}
		pid[i]=0;
		
		/* wait a bit and check to see if all children have exited */
		usleep(100000);
		for(i=0; i<MAXCMDS; i++) if(pid[i] > 0) break;
		if(i >= MAXCMDS) done=1;
	}
	fprintf(logout,"%s All children dead!\n",asctim()); fflush(logout);
	exit(1);
}



/**************** MAIN *******************/
main(argc,argv)
int argc;
char *argv[];
{
	int ipid;					/* pid of child returned by wait */
	int statusp;				/* status of dead child */
	int err,i;
	char * ierr;
	int low,low2;
	struct tm *tm;
	time_t now;
	long nsamp;
	FILE *iunit;
	char line[200];
	int mon_next=0;
	
	for (i=0; i<MAXCMDS; i++) {cmds[i]=0; monfile[i]=NULL; }
	iunit=fopen("vdlmom.setup","r");				/* read in list of commands */
	ierr=(char* )-1;								/* force value of IERR */
	i=0;											/* count the command lines */
	while ( (ierr=fgets(line,200,iunit)) != 0 ) 
	{	if(line[0] != '!' && line[0] != '#') 		/* if its not a comment */
		{	cmds[i]=(char *) malloc(strlen(line)+2);/* allocate mem for cmd */
			strcpy(cmds[i],line);					/* copy command to allocated mem*/
			*(cmds[i]+strlen(line)-1)=0;			/* null out the \n at end */
			printf("%d=%s\n",i,cmds[i]);			/* user feedback */
			if(strlen(line) > 5) i++;				/* point to next command */
			else cmds[i]= 0;						/* no line for near blanks */
		}
	}
	fclose(iunit);
/*
		Set CMDS with pointers to the command lines.
*/
	logout=fopen("vdlmom.log","a+");
	if(logout == NULL) printf("Cannot open VDLMOM.LOG!\n");
	printf("VDLMOM : logout=%d %d\n",logout,logout->_file);
	
	fprintf(logout,"\n\n %s ******* STARTUP ********\n",asctim());
	fflush(logout);
/*
	Start up all of the processes 
*/

	for(i=0; i<MAXCMDS; i++) 
	{	if(cmds[i] != 0) 
		{	printf("Newproc : %d %s\n",i,cmds[i]);
			/*fprintf(logout,"%s Newproc : %s\n",asctim(),cmds[i]); fflush(logout);*/
			newproc(cmds,pid,i);
			usleep(100000);
		}
	}
	signal(SIGINT,cc_handler);				/* Make all signals go to cc_handler */
	signal(SIGHUP,cc_handler);
	signal(SIGQUIT,cc_handler);
	signal(SIGILL,cc_handler);
	signal(SIGTRAP,cc_handler);
	signal(SIGABRT,cc_handler);
	signal(SIGEMT,cc_handler);
	signal(SIGFPE,cc_handler);
	signal(SIGKILL,cc_handler);
	signal(SIGBUS,cc_handler);
	signal(SIGSEGV,cc_handler);
	signal(SIGSYS,cc_handler);
	signal(SIGTERM,cc_handler);
	signal(SIGSTOP,cc_handler);	
	signal(SIGCONT,cc_handler);
	signal(SIGTSTP,cc_handler);
	signal(SIGCHLD,cc_handler);
	signal(SIGXCPU,cc_handler);
	signal(SIGXFSZ,cc_handler);
	signal(SIGWINCH,cc_handler);
#ifdef SUNOS
	signal(SIGLOST,cc_handler);
#endif
	sleep(2);
			
/*
	Wait for a process to die.  Find its command line from the returned PID
	and restart that process 
*/
	fprintf(logout,"\n\n**** Hit infinite loop\n\n"); fflush(logout);
	for(;;) 
	{	fflush(logout);
		ipid=wait3(&statusp, WNOHANG, NULL);				/* Wait for child to die */
		if(ipid > 0) {
			fprintf(logout,"\n %s ****************",asctim());
			fprintf(logout,"Child with pid=%d has died.  Status=%x\n",ipid,statusp);
			fflush(logout);
			for(i=0; i< MAXCMDS; i++) if(ipid == pid[i]) break;/* which child is it */
			if(ipid == pid[i]) 					/* i is array index of dead child */
			{	fprintf(logout,"child is number %d line=%s\n",i,cmds[i]);
				low=statusp & 0x7f;
				low2=(statusp & 0xff00) / 256;
				if(low == 0177) 
				{	fprintf(logout,"Child stopped with signal=%x\n",low2);
					fprintf(logout,"signal=%s\n",sigtostr(low2));
				} 
				else if (low != 0) 
				{	fprintf(logout,"Child terminated due to signal=%x # %s\n",
						low,sigtostr(low));
				}
				else 
				{	fprintf(logout,"Child did exit=%d\n",low2);
				}
				fflush(logout);
				newproc(cmds,pid,i);
				fprintf(logout,"********************************\n");
				fflush(logout);
				sleep(2);
			} else 					/* it was not a child who died.  Who did?*/
			{	if(ipid == -1) fprintf(logout,"VDLMOM has died!\n");/* it was us!!! */
				else fprintf(logout,"Pid is unknown to VDLMOM!\n");
				fflush(logout);
			}			/* if ipid matches */
		}				/* end of if wait3 returns > 0 */
		
		/* if there is a monitor file, read its contents and see if we need to restart*/
		if(monfile[mon_next] != NULL) {
			rewind(monfile[mon_next]);
			fscanf(monfile[mon_next],"%d %d",&now, &nsamp);
			
			/* if > 1200 seconds with no data, autorestart */
			if( abs(time(NULL) - now) > 1200 ) {
				fprintf(logout,"%s %s Child %d no activity diff=%d stop pid=%d\n",
						asctim(), tags[mon_next], mon_next,  
						time(NULL) - now,pid[mon_next]); fflush(logout);
				kill(pid[mon_next], SIGTERM);
				
			}
		}
		
		/* check another one next time */
		mon_next++;
		if(mon_next > MAXCMDS) mon_next=0;
		usleep(60000000/MAXCMDS);						/* so each chan check each 60 seconds */
	}					/* end of infinite loop */
	fprintf(logout, "VDLMOM infinite loop has exited! Impossible!\n");
	fflush(logout);
}


newproc(cmds,pid,i)
int pid[];
char *cmds[];
int i;
/*
	Start up a process based on the command line in CMDS[I].  Put the pid of
	the child in pid[i].  The command line is parsed into separate strings as
	is usual of the argc,argv command interface used in UNIX.  We have a limit
	of MAXARG white space delimited args though more could be allocated here or the
	routine made general by mallocing space

	D.C. Ketchum May 1995
*/
{
#define MAXARG 50
	extern FILE *logout;
	int k,l,j,err;
	char line[100];
	char tag[100];
	int monitor;
	static char *argv2[MAXARG];					/* pointers to command args */
	for(k=0; k<MAXARG; k++) 
		if(argv2[k] == 0) 	/* if first time, alloc for args */
		{	argv2[k]=(char *) malloc(200);
/*		printf("Malloc arg2=%d\n",k);*/
		}

	/* create the argv2 array with the white space delimited strings */
	fprintf(logout,"\n*********\n%s NewProc:%d |%s \n",asctim(),i,cmds[i]);
	k=0; 								/* counter of arguments */
	l=0;								/* counter of characters per argument*/
	for (j=0; j<strlen(cmds[i]); j++) 	/*scan the input line */
	{	*(argv2[k]+l)=*(cmds[i]+j);		/* move one character */
		if(isspace(*(argv2[k]+l)) != 0) /* is it the end of the command */
		{	*(argv2[k]+l)=0;			/* yes,make it null terminated string*/
			l=0;						/* new argument string offset */
			k++;						/* point to new argument */
			if(k >= MAXARG) 
			{	printf("MAXARG=%d Exceeded! Rebuild VDLMOM bigger ! \n",
					MAXARG);
				exit(1);
			}
		} else	l++;					/* no, bump pointer to arg string */
	}
	*(argv2[k]+l)=0;					/* null terminate last argument */
	k++;											/* true number of arguments */
	argv2[k]=0;							/* UNIX says last entry must be zero */
	
	/* if the child has a -m (monitor), we need to record the -i tag for the monitor
		file name 
	*/
	monitor=0;
	for(j=0; j<k; j++) {
		/*printf(" %d s=%s|\n",j,argv2[j]);*/
		if( strcmp(argv2[j], "-m") == 0) monitor=1;	/* is this a monitor station */
		if( strcmp(argv2[j], "-i") == 0) strcpy(tag, argv2[j+1]);/* save tag */
		tags[i]= (char *) malloc(strlen(tag)+1);
		strcpy(tags[i], tag);
	}				
	
	/* dump the command stuff to the log. */
/*	err=fprintf(logout,"k=%d ",k);
	for(j=0; j<k; j++) fprintf(logout," %d s=%s|",j,argv2[j]); fprintf(logout,"\n");
	fflush(logout);*/
		
/* fork off and process accordingly 
	first we fork off to have a new process.  Then this child process is overlaid
	with the desired program using the execvp subroutine 
*/
	pid[i]=fork();							/* fork ourself off */
	if(pid[i] != 0 ) {
		fprintf(logout,"%s NEWPROC : %d %d Child's pid=%d = %s\n",
			asctim(),i,logout,pid[i],cmds[i]);
		if(monitor) {
			if(monfile[i] == NULL) {
				sprintf(line,"%s_mon.vdlmom",tag);
				fprintf(logout, "Open %d %s \n",i,line);
				monfile[i]=fopen(line,"r");
				if(monfile[i] == NULL) {		/* failed to open.  Wait and try again */
					sleep(4);
					monfile[i]=fopen(line,"r");
				}
			}
		}		/* endif monitor */
	}
	else 	
	{													/* we are the child, replace ourselves accordingly */
			
		err=execvp(argv2[0],argv2);			/* start the command using execvp*/
		fprintf(logout,"   **** How did I return from an execvp command?? err=%x errno=%x\n",
					err,errno);				/* you should never return ! */
		fflush(logout);
		exit(99);
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
	return (t+4);			/* hand to user */
}



char *sigtostr(sig)
int sig;
/*
	sigtostr - converts a UNIX signal number to a text string describing the signal.
	Based on information in the MAN pages on sigvec.

	D.C. Ketchum May 1995
*/
{
	static char sighup[]="Hangup";
	static char sigint[]="Interrupt";
	static char sigquit[]="Quit";
	static char sigill[]="Illegal Instruction";
	static char sigtrap[]="Trace Trap ";
	static char sigiot[]="IOT instruction";
	static char sigabrt[]="Abort signal";
	static char sigemt[]="EMT Instruction";
	static char sigfpe[]="Floating point exeption";
	static char sigkill[]="Kill issued";
	static char sigbus[]="Bus error";
	static char sigsegv[]="Segmentation violation";
	static char sigsys[]="Bad Argument to system call";
	static char sigpipe[]="Write on pipe or socket with no one to read it";
	static char sigalrm[]="Alarm clock";
	static char sigterm[]="Software Termination signal from kill";
	static char sigurg[]="Urgent condition on I/O channel";
	static char sigstop[]="Sendable stop signal not from tty";
	static char sigtstp[]="Stop signal from TTY";
	static char sigcont[]="Continue a stopped process";
	static char sigchld[]="to parent on child stop or exit ";
	static char sigcld[]="System V name for SIGCHLD";
	static char sigttin[]="To reader pgrp upon background tty read";
	static char sigttou[]="To writer pgrp upon background tty write";
	static char sigio[]="I/O possible signal";
	static char sigpoll[]="Ssysterm V name of I/O signal";
	static char sigxcpu[]="Exceeded CPU Limit";
	static char sigxfsz[]="Exceed file size limit";
	static char sigvtalrm[]="Virtual time alarm";
	static char sigprof[]="Profiling time alarm";
	static char sigwinch[]="Window changed";
	static char siglost[]="Rosource lost (record lock lost)";
	static char sigusr1[]="User defined signal 1";
	static char sigusr2[]="User defined signal 2";
	static char unknown[]="Unknown Termination Signal";
/*
	Basically a case statement to return right one 
*/
	if(sig == SIGHUP) return sighup;
	if(sig == SIGINT) return sigint;
	if(sig == SIGQUIT) return sigquit;
	if(sig == SIGILL) return sigill;
	if(sig == SIGTRAP) return sigtrap;
	if(sig == SIGIOT) return sigiot;
	if(sig == SIGABRT) return sigabrt;
	if(sig == SIGEMT) return sigemt;
	if(sig == SIGFPE) return sigfpe;
	if(sig == SIGKILL) return sigkill;
	if(sig == SIGBUS) return sigbus;
	if(sig == SIGSEGV) return sigsegv;
	if(sig == SIGSYS) return sigsys;
	if(sig == SIGPIPE) return sigpipe;
	if(sig == SIGALRM) return sigalrm;
	if(sig == SIGTERM) return sigterm;
	if(sig == SIGURG) return sigurg;
	if(sig == SIGSTOP) return sigstop;
	if(sig == SIGTSTP) return sigtstp;
	if(sig == SIGCONT) return sigcont;
	if(sig == SIGCHLD) return sigchld;
	if(sig == SIGCLD) return sigcld;
	if(sig == SIGTTIN) return sigttin;
	if(sig == SIGTTOU) return sigttou;
	if(sig == SIGIO) return sigio;
	if(sig == SIGXCPU) return sigxcpu;
	if(sig == SIGXFSZ) return sigxfsz;
	if(sig == SIGVTALRM) return sigvtalrm;
	if(sig == SIGPROF) return sigprof;
	if(sig == SIGWINCH) return sigwinch;
#ifdef SUNOS
	if(sig == SIGLOST) return siglost;
#endif
	if(sig == SIGUSR1) return sigusr1;
	if(sig == SIGUSR2) return sigusr2;
	return unknown;						/* default is we do not know */
}
