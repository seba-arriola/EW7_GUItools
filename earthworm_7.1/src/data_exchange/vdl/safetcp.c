/************************************************************
	This code connects to a tcp socket for a UNIX style system.
	User must provide system name or dot address.  The read and write 
	routines will reopen the socket if it is detected to be down and
	hence are "safe" routines in that they should always give logical
	responses without makeing the user deal with socket vagarities.

	D.C. Ketchum Mar 1997

*/
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/stat.h>
#include <stdio.h>
#include <errno.h> 
#include <signal.h>							/* define the signal parameters */

#include "safetcp.h"						/* define structs for this code */

/*
	Global variables 
*/
struct sockaddr_in outsock;					/* a socket data structure */
struct sockaddr_in udpsock;
int sigpipeoccurred;						/* used to tcp_handler to let world know*/

char *asctim();

/*
	TCP_HANDLER is called when bad things happen to a socket so we can
	log them!
*/
#ifdef __STDC__
  void safetcp_handler(int arg)
#else
  void safetcp_handler(arg)
  int arg;
#endif
{
	extern FILE *logout;
	if(arg == SIGPIPE) 
	{	fprintf(logout,"SIGPIPE handler %x\n",errno);
		sigpipeoccurred=1;
		return;
	}  
	else 
	{	fprintf(logout,"TCP HANDlER UNknown arg=%x\n",arg);
	}
	exit(22);
}


#ifdef __STDC__

	int readtcp(struct tcpsocket* tcp, void *buf, int len)

#else

	int readtcp(tcp, buf, len)
	struct tcpsocket* tcp;
	int len;
	void *buf;

#endif

/*
	Socket safe read - deals with socket errors transparently
	If return is zero it is an EOF and no data is available.  Negative reads
	are an error and cause the socket to automatically close and reopen.
	The user should never get a negative return.
	A positive number is the number of bytes returned by read.
*/
{
	int err;
	extern FILE *logout;					/* log error messages */
	sigpipeoccurred=0;						/* no sigpipe errors yet! */
	while ( (err=read(tcp->path, buf, len)) < 0) 
	{
		if(sigpipeoccurred) 
		{	fprintf(logout,"SIGPIPE occurred on READ. Reestablish\n");
			if(err >= 0) 
			{
				fprintf(logout,"SIGPIPE but error >= 0 is %d\n",err);
				err=-1;
			}
		}
		if( err < 0) 
		{	fprintf(logout,"Connection lost on read - reconnect\n");
			open_tcp(tcp);						/* open the unit again */
		}
	}
	return err;								/* give user response */
}



#ifdef __STDC__

  int writetcp(struct tcpsocket *tcp, void *buf, int len)

#else

  int writetcp(tcp,buf,len)
  struct tcpsocket *tcp;				/* socket data from open */
  int len;								/*Socket safe data write */
  void *buf;

#endif

{
	int ierr;
	extern FILE *logout;
	ierr=0;
	while (ierr <= 0) {						/* until we write it sucessfully */
		/*if(ierr == 0) return len;				/* DEBUG : no output */
		sigpipeoccurred=0;					/* not SIGPIPE error is assumed */
		ierr=write(tcp->path,buf,len);			/* do I/O */
		if(ierr == len) return ierr;		/* if ok return to user */
		if(ierr <= 0) fprintf(logout,"Wr error=%d %x sp=%d %s\n",
				ierr,errno,sigpipeoccurred,strerror(errno));
		if( sigpipeoccurred || ierr == 0) {/* if SIGPIPE or EOF occurred */
			open_tcp(tcp);						/* Reconnect the socket or die trying */
			ierr=-1;						/* make sure we do the I/O again *
		} else {
			fprintf(logout,"Write error not handled ierr=%d errno=%x\n",ierr,errno);
			return ierr;					/* Don;`'t know what kind of err*/
		}
	}
	return ierr;
}



int init_safetcp(tcpunit, hostname, dotadr, port)
struct tcpsocket *tcpunit;
char *hostname, *dotadr;					/* c string to name or dot address */
int port;									/* port number to open */

{	extern FILE *logout;					/* log file id */

	tcpunit->path=0;
	fprintf(logout,"init_safetcp : h=%d dot=%d port=%d\n", hostname, dotadr,port);
	fflush(logout);
	tcpunit->port=port;						/* save port number in structure */
	if(hostname != NULL) 					/* save host name if given */
	{ tcpunit->hostname = (char *) malloc( strlen(hostname)+1);
		strcpy(tcpunit->hostname, hostname);
	} 
	else
	{	tcpunit->hostname = NULL;			/* not given, use NULL */
	}	
	if(dotadr != NULL) 						/* save dot adr if given */
	{ fprintf(logout,"Dot adr found=%s %d\n",dotadr,strlen(dotadr)); 
		fflush(logout);
		tcpunit->dotadr = (char *) malloc( strlen(dotadr)+1);
		strcpy(tcpunit->dotadr, dotadr);
	}
	else
	{	tcpunit->dotadr = NULL;				/* not given, use NULL */
	}
	tcpunit->path=open_tcp(tcpunit);		/* open the socket*/
	return tcpunit->path;					/* return path for abusers */
}


open_tcp(tcp)
struct tcpsocket *tcp;
{
	extern FILE *logout;					/* log file id */
	u_long addr;							/* IP address after translation */
	int ierr;
	unsigned char *p;
	struct hostent *hp,*gethostbyname();	/* Internet name resolutions */
	struct hostent *gethostbyaddr();		/* Get host by iternet addrs */

	if(tcp->path > 0)
	{	fprintf(logout,"Close Safetcp path=%d %s\n",tcp->path,tcp->hostname);
		fflush(logout);
		close(tcp->path);					/* close any prior socket */
		tcp->path=-1;						/* path not open */
	}
	ierr=-1;								/* set bad so while loop works*/
	while (ierr < 0) 						/* while no socket created */
	{	tcp->path=socket(AF_INET, SOCK_STREAM,0);/* create a Socket on path */
		if(tcp->path < 0) 
		{	fprintf(logout,"Safe Socket creation failed=%d %x\n",
			tcp->path,errno);
			exit(202);
		}
		if(tcp->hostname != NULL) 
		{	fprintf(logout,"gethost by name %s\n",tcp->hostname);
			hp=gethostbyname(tcp->hostname);		/* where is it */
			if( hp == 0) 
			{	fprintf(logout,"Could not translate %s\n",tcp->hostname);
				exit(203);
			}
			fprintf(logout,"Hostname translated %x\n",hp->h_addr);
			p=(unsigned char *) hp->h_addr;
			fprintf(logout,"host=%s  adr=%d %d %d %d  port=%d l=%d\n",(char *) hp->h_name,
				*p,*(p+1),*(p+2),*(p+3), tcp->port, hp->h_length);fflush(logout);
			memset((char *)&outsock, 0, sizeof(outsock));
			memcpy( (char*) &outsock.sin_addr, (char*)hp->h_addr, hp->h_length);
												/* copy stuff to struct*/
			outsock.sin_port=htons(tcp->port);	/* correct byte order */
			outsock.sin_family=hp->h_addrtype;	/* set the address type*/		} 
		else if (tcp->dotadr != NULL) 
		{	if( (int) (addr = inet_addr(tcp->dotadr)) == -1) 
			{	fprintf(logout,"Bad Dot address = %s \n",tcp->dotadr);
				exit(203);
			}
			fprintf(logout,"Good Dot address %x = %s\n",addr, tcp->dotadr);
			memset( (char *) &outsock,0,sizeof(outsock));
			memcpy(&outsock.sin_addr, &addr, 4);
			outsock.sin_port=htons(tcp->port);
			outsock.sin_family=AF_INET;
			fprintf(logout,"host=%x port=%d\n",outsock.sin_addr,tcp->port);
		} 
		else 
		{
			fprintf(logout,"TCP_INIT must have host name or dot address!");
			exit(203);
		}

		fprintf(logout,"Connect..."); fflush(logout);
		if( (ierr=connect(tcp->path, (struct sockaddr *) &outsock, 
			sizeof(outsock))) < 0) 
		{	fprintf(logout,"Failed! \n"); fflush(logout);
			if(tcp->hostname == NULL) {
				fprintf(logout,
				"Cannot connect to dot port %d %s=%d path=%d errno=%x %s\n",
				tcp->port,tcp->dotadr,ierr,tcp->path,errno,
				strerror(errno));
			}
			else
			{	fprintf(logout,
				"Cannot connect to port %d %s=%d path=%d errno=%x %s\n",
				tcp->port,tcp->hostname,ierr,tcp->path,errno,
				strerror(errno));
			}	
			fflush(logout);					/* complain */
			close(tcp->path);
			fprintf(logout,"%s Closed.  Wait 60 and retry\n",asctim());
			fflush(logout);			
			sleep(60);
			if(tcp->hostname != NULL) fprintf(logout,"Try to connect again %s \n",
				tcp->hostname); 
			if(tcp->dotadr != NULL) fprintf(logout,"Try to connect again %s \n",
				tcp->dotadr);
			fflush(logout);
		}
		else 
		{fprintf(logout," succeeded...\n"); fflush(logout);
		}
	}
	fprintf(logout,"Safe TCP connection complete path=%d\n",tcp->path); 	
	fflush(logout);
	signal(SIGPIPE,safetcp_handler);
	return tcp->path;
}

int init_safeudp(tcpunit, hostname, dotadr, port)
struct tcpsocket *tcpunit;
char *hostname, *dotadr;					/* c string to name or dot address */
int port;									/* port number to open */

{	extern FILE *logout;					/* log file id */

	tcpunit->path=0;
	fprintf(logout,"init_safeudp : h=%d dot=%d port=%d\n", hostname, dotadr,port);
	fflush(logout);
	tcpunit->port=port;						/* save port number in structure */
	if(hostname != NULL) 					/* save host name if given */
	{ tcpunit->hostname = (char *) malloc( strlen(hostname)+1);
		strcpy(tcpunit->hostname, hostname);
	} 
	else
	{	tcpunit->hostname = NULL;			/* not given, use NULL */
	}	
	if(dotadr != NULL) 						/* save dot adr if given */
	{ fprintf(logout,"Dot adr found=%s %d\n",dotadr,strlen(dotadr)); 
		fflush(logout);
		tcpunit->dotadr = (char *) malloc( strlen(dotadr)+1);
		strcpy(tcpunit->dotadr, dotadr);
	}
	else
	{	tcpunit->dotadr = NULL;				/* not given, use NULL */
	}
	tcpunit->path=open_udp(tcpunit);		/* open the socket*/
	return tcpunit->path;					/* return path for abusers */
}

open_udp(tcp)
struct tcpsocket *tcp;
{
	extern FILE *logout;					/* log file id */
	u_long addr;							/* IP address after translation */
	int ierr;
	unsigned char *p;
	struct hostent *hp,*gethostbyname();	/* Internet name resolutions */
	struct hostent *gethostbyaddr();		/* Get host by iternet addrs */

	if(tcp->path > 0)
	{	fprintf(logout,"Close Safeudp path=%d %s\n",tcp->path,tcp->hostname);
		fflush(logout);
		close(tcp->path);					/* close any prior socket */
		tcp->path=-1;						/* path not open */
	}
	ierr=-1;								/* set bad so while loop works*/
	while (ierr < 0) 						/* while no socket created */
	{	tcp->path=socket(AF_INET, SOCK_DGRAM,0);/* create a Socket on path */
		if(tcp->path < 0) 
		{	fprintf(logout,"Safe UDP Socket creation failed=%d %x\n",
			tcp->path,errno);
			exit(202);
		}
		if(tcp->hostname != NULL) 
		{	fprintf(logout,"UDP gethost by name %s\n",tcp->hostname);
			hp=gethostbyname(tcp->hostname);		/* where is it */
			if( hp == 0) 
			{	fprintf(logout,"Could not translate %s\n",tcp->hostname);
				exit(203);
			}
			fprintf(logout,"Hostname translated %x\n",hp->h_addr);
			p=(unsigned char *) hp->h_addr;
			fprintf(logout,"host=%s  adr=%d %d %d %d  port=%d l=%d\n",(char *) hp->h_name,
				*p,*(p+1),*(p+2),*(p+3), tcp->port, hp->h_length);fflush(logout);
			memset((char *)&outsock, 0, sizeof(outsock));
			memcpy( (char*) &outsock.sin_addr, (char*)hp->h_addr, hp->h_length);
												/* copy stuff to struct*/
			outsock.sin_port=htons(tcp->port);	/* correct byte order */
			outsock.sin_family=hp->h_addrtype;	/* set the address type*/		} 
		else if (tcp->dotadr != NULL) 
		{	if( (int) (addr = inet_addr(tcp->dotadr)) == -1) 
			{	fprintf(logout,"Bad Dot address = %s \n",tcp->dotadr);
				exit(203);
			}
			fprintf(logout,"UDP Good Dot address %x = %s\n",addr, tcp->dotadr);
			memset( (char *) &outsock,0,sizeof(outsock));
			/*memcpy(&outsock.sin_addr, &addr, 4);*/
			outsock.sin_addr.s_addr = htonl(INADDR_ANY);
			outsock.sin_port=htons(0);
			outsock.sin_family=AF_INET;
			fprintf(logout,"host=%x port=%d\n",outsock.sin_addr,tcp->port);
		} 
		else 
		{
			fprintf(logout,"TCP_INIT must have host name or dot address!");
			exit(203);
		}

		fprintf(logout,"bind..."); fflush(logout);
		if( (ierr=bind(tcp->path, (struct sockaddr *) &outsock, 
			sizeof(outsock))) < 0)  
		{	fprintf(logout,"Failed! \n"); fflush(logout);
			if(tcp->hostname == NULL) {
				fprintf(logout,
				"Cannot connect to dot port %d %s=%d path=%d errno=%x %s\n",
				tcp->port,tcp->dotadr,ierr,tcp->path,errno,
				strerror(errno));
			}
			else
			{	fprintf(logout,
				"Cannot bind UDP to port %d %s=%d path=%d errno=%x %s\n",
				tcp->port,tcp->hostname,ierr,tcp->path,errno,
				strerror(errno));
			}	
			fflush(logout);					/* complain */
			close(tcp->path);
			fprintf(logout,"%s Closed.  Wait 60 and retry\n",asctim());
			fflush(logout);			
			sleep(60);
			if(tcp->hostname != NULL) fprintf(logout,"Try to connect again %s \n",
				tcp->hostname); 
			if(tcp->dotadr != NULL) fprintf(logout,"Try to connect again %s \n",
				tcp->dotadr);
			fflush(logout);
		}
		else 
		{fprintf(logout," succeeded...\n"); fflush(logout);
			memset(&udpsock,0,sizeof(udpsock));
			udpsock.sin_family=AF_INET;
			memcpy(&udpsock.sin_addr,&addr, 4);
			udpsock.sin_port=htons(tcp->port);
		}
	}
	fprintf(logout,"Safe TCP connection complete path=%d\n",tcp->path); 	
	fflush(logout);
	signal(SIGPIPE,safetcp_handler);
	return tcp->path;
}

int sendtoudp(tcp,buf,len)
struct tcpsocket *tcp;
char *buf;
int len;
{	int ierr;
	extern FILE *logout;
	ierr=sendto(tcp->path, buf, len,0,(struct sockaddr *) &udpsock,sizeof(udpsock));
	if(ierr == -1) fprintf(logout,"sendtoudp: ierr=%d errno=%d %s\n",ierr,errno,strerror(errno));
	return ierr;
}
