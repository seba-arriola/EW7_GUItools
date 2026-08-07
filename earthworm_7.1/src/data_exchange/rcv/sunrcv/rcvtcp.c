/************************************************************
	This code connects to a serial line for a UNIX style system.
*/
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/filio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdio.h>
#include <errno.h> 
#include <signal.h>							/* define the signal parameters */
struct sockaddr_in outsock;					/* a socket data structure */
char *asctim();
int out;									/* socket unit # */
int sigpipeoccurred;
int init_tcp();
int sigpipe_tcp()
{
	return sigpipeoccurred;
}
#ifdef __STDC__
  void tcp_handler(int arg)
#else
  void tcp_handler(arg)
  int arg;
#endif
{
	extern FILE *logout;
	extern int ttpath;
	extern char *sys_errlist[];				/* link to error text message*/
	fprintf(logout,"%s Error Handler errno=%x",asctim(),errno);
	fflush(logout);
	fprintf(logout," : %s\n",sys_errlist[errno]);
	fflush(logout);
	if(arg == SIGPIPE) {
		fprintf(logout,"%s SIGPIPE handler %x\n",asctim(),errno);
		sigpipeoccurred=1;
		fflush(logout);
		return;
	} else {
		fprintf(logout,"%s TCP HANDlER UNknown arg=%x\n",asctim(),arg);
	}
	exit(22);
}
#ifdef __STDC__
  int writeout(int ttin, char *buf, int len)
#else
  int writeout(ttin,buf,len)
  int ttin,len;								/*Socket safe data write */
  char *buf;
#endif
{
	int ierr;
	extern FILE *logout;
	extern char hostname[];
	extern int ttpath;/* user passes us a path but we know it may changes on reconn*/
	ierr=0;
	while (ierr <= 0) {						/* until we write it sucessfully */
		sigpipeoccurred=0;					/* not SIGPIPE error is assumed */
		ierr=write(ttpath,buf,len);			/* do I/O */
		if(ierr == len) return ierr;		/* if ok return to user */
		if(ierr <= 0) fprintf(logout,"%s Write out : error=%d %x sp=%d\n",
				asctim(),ierr,errno,sigpipeoccurred);
		if( sigpipeoccurred || ierr == 0) {/* if SIGPIPE or EOF occurred */
			fprintf(logout,"%s writeout : sigpipeoccured or ierr = 0\n",
				asctim(),sigpipeoccurred,ierr);
			init_tcp(hostname);				/* Reconnect the socket or die trying */
			ierr=-1;						/* make sure we do the I/O again */
		} else {
			fprintf(logout,"Writeout : error not handled ierr=%d errno=%x\n",
				ierr,errno);
			return ierr;					/* Don;`'t know what kind of err*/
		}
	}
	return ierr;
}			
int init_tcp(hostname)
char hostname[];
{
	extern FILE *logout;
	extern int ttpath;
	char text[100];
	static int first=1;
	short int port=2005;					/* port number to use if TCP to TCPOUT */
	int ierr;
	unsigned char *p;
	extern char *sys_errlist[];
	struct hostent *hp,*gethostbyname();	/* Internet name resolutions routines*/
	unsigned long onoff=1;
	int ioctlval;
	text[0]=0;
	ierr=-1;
	if(ttpath > 0) {
		fprintf(logout,"%s init_tcp : Close tcp path=%d\n",asctim(),ttpath);
		fflush(logout);
		close(ttpath);			/* close any prior socket */
		ttpath=-1;
	}
	ierr=-1;
	while (ierr < 0) {
		ttpath=socket(AF_INET, SOCK_STREAM,0);	/* create a Socket on unit ttpath */
		if(ttpath < 0) {
			fprintf(logout,"%s Socket creation failed=%d %x\n",asctim(),ttpath,errno);
			exit(202);
		}
		hp=gethostbyname(hostname);	/* where is it */
		if( hp == 0) {
			fprintf(logout,"%s Could not translate %s\n",asctim(),hostname);
			exit(203);
		}
		p=(unsigned char *) hp->h_addr;
		fprintf(logout,"host=%s  adr=%d %d %d %d\n",(char *) hp->h_name,
			*p,*(p+1),*(p+2),*(p+3));fflush(logout);
		bzero((char *)&outsock, sizeof(outsock));
		bcopy((char*)hp->h_addr, (char*) &outsock.sin_addr,hp->h_length);
											/* copy stuff to struct*/
		outsock.sin_port=htons(port);				/* NSNTCP at NSN3 is port 2003*/
		outsock.sin_family=hp->h_addrtype;			/* set the address type*/
		if( (ierr=connect(ttpath, (struct sockaddr *) &outsock, 
				sizeof(outsock))) < 0) {
			fprintf(logout,"%s Cannot connect to port %d=%d path=%d errno=%x %s\n",
				asctim(),port,ierr,ttpath,errno,sys_errlist[errno]);	
			fflush(logout);			/* complain */
			fprintf(logout,"%s init_tcp : Wait 60 and retry\n",asctim());
			close(ttpath);
			sleep(5);
			fprintf(logout,"%s Try to connect again \n",asctim());
		} else {
			onoff=1;							/* set non-blocking I/O */
			ioctlval=ioctl(ttpath, FIONBIO, &onoff);
			if(ioctlval != 0) {
				fprintf(logout,"%s init_tcp : Bad ioctl return=%x errno=%x\n",
					asctim(),ioctlval,errno);
				ierr=-1;
				close(ttpath);					/* close the path try again */
			} else fprintf(logout,"%s FIONBIO successfull\n",asctim());
		}
	}
	fprintf(logout,"%s TCP connection complete path=%d\n",asctim(),ttpath); 
	fflush(logout);
	if(first) {
		signal(SIGPIPE,tcp_handler);
		first=0;
		fprintf(logout,"TCP set up SIGPIPE handler\n");
	}
	return ttpath;
}
#ifdef __STDC__
   void init_tt(char *dev)
#else
  void init_tt(dev)
  char *dev;
#endif
{
	extern FILE *logout;
	fprintf(logout,"init_tt cannont be used.  TCP is output \n");
	exit(201);
}
