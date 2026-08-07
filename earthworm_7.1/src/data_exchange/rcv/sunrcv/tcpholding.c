#include <stdio.h>
#include "safetcp.h"
int tcpholding(host,port,sort,net,name,comp,loc,iy,id,ih,im,is,ms,nsamp,rate)
char *host;
int port;
char *name, *comp,*net,*loc,*sort;
int iy,id,ih,im,is,ms,nsamp;
double rate;
{	struct holdstruct {
		int date;				/* yyyymmdd order */
		int date_ms;		/* ms since midnight */
		int ms;					/* length of packet in ms nsamp/rate*1000 */
		char scnl[12];	/* nnssssscccll seed name */
		char sort[2];
	};
	struct holdstruct a;
	static struct tcpsocket tcp;
	extern FILE *logout;
	char scnl[13];
	
	static int path=-1;
	int mon,day,ierr;
	char *hostname, *dotadr;
	if(path == -1) 
	{	
		if( isdigit(host[0]) )
		 {	dotadr=(char *) malloc(strlen(host)+1);
			 hostname=NULL;
			 strcpy(dotadr,host);
			 fprintf(logout,"dot adr=%s\n",dotadr);
		 }
		 else
		 {	hostname=(char *) malloc(strlen(host)+1 );
		 	 strcpy(hostname, host);
			 dotadr=NULL;
			 fprintf(logout,"hostname=%s\n",hostname);
		 }
		fprintf(logout,"Open holdings socket port=%d\n",port);
		path=init_safetcp(&tcp,hostname,dotadr,port);
		if(path <=0) {
			fprintf(logout,"Cannot create holdings socket. Abort.\n");
			exit(1);
		}
	}
	
	/* convert julian date to month and year */
	datjul(iy,id,&mon,&day);
	memset(&a.scnl,32,12);
	memcpy(&a.scnl,net,2);
	memcpy(&a.scnl[2],name,strlen(name));
	memcpy(&a.scnl[7],comp,3);
	if(strlen(loc) > 0) memcpy(&a.scnl[10],loc,strlen(loc));
	a.date = iy*10000+mon*100+day;
	a.date_ms = ih*3600000+im*60000+is*1000+ms;
	a.ms = nsamp/rate*1000.+0.001;
	memcpy(&a.sort,sort,2);
	strncpy(scnl,a.scnl,12);
	scnl[12]=0;
	i4swap(a.date,&a.date);
	i4swap(a.date_ms, &a.date_ms);
	i4swap(a.ms, &a.ms);
	ierr=writetcp(&tcp,&a, 26);

}
int udpholding(host,port,sort,net,name,comp,loc,iy,id,ih,im,is,ms,nsamp,rate)
char *host;
int port;
char *name, *comp,*net,*loc,*sort;
int iy,id,ih,im,is,ms,nsamp;
double rate;
{	struct holdstruct {
		int date;				/* yyyymmdd order */
		int date_ms;		/* ms since midnight */
		int ms;					/* length of packet in ms nsamp/rate*1000 */
		char scnl[12];	/* nnssssscccll seed name */
		char sort[2];
	};
	struct holdstruct a;
	static struct tcpsocket tcp;
	extern FILE *logout;
	char scnl[13];
	
	static int path=-1;
	int mon,day,ierr;
	char *hostname, *dotadr;
	if(path == -1) 
	{	
		if( isdigit(host[0]) )
		 {	dotadr=(char *) malloc(strlen(host)+1);
			 hostname=NULL;
			 strcpy(dotadr,host);
			 fprintf(logout,"dot adr=%s\n",dotadr);
		 }
		 else
		 {	hostname=(char *) malloc(strlen(host)+1 );
		 	 strcpy(hostname, host);
			 dotadr=NULL;
			 fprintf(logout,"hostname=%s\n",hostname);
		 }
		fprintf(logout,"Open holdings socket port=%d\n",port);
		path=init_safeudp(&tcp,hostname,dotadr,port);
		if(path <=0) {
			fprintf(logout,"Cannot create holdings socket. Abort.\n");
			exit(1);
		}
	}
	
	/* convert julian date to month and year */
	datjul(iy,id,&mon,&day);
	memset(&a.scnl,32,12);
	memcpy(&a.scnl,net,2);
	memcpy(&a.scnl[2],name,strlen(name));
	memcpy(&a.scnl[7],comp,3);
	if(strlen(loc) > 0) memcpy(&a.scnl[10],loc,strlen(loc));
	a.date = iy*10000+mon*100+day;
	a.date_ms = ih*3600000+im*60000+is*1000+ms;
	a.ms = nsamp/rate*1000.+0.001;
	memcpy(&a.sort,sort,2);
	strncpy(scnl,a.scnl,12);
	scnl[12]=0;
	i4swap(a.date,&a.date);
	i4swap(a.date_ms, &a.date_ms);
	i4swap(a.ms, &a.ms);
	ierr=sendtoudp(&tcp,&a, 26); 
	fprintf(logout,"send=%d\n",ierr);
}
int udpholding2(host,port,sort,net,name,comp,loc,yymmdd,ms,nsamp,rate)
char *host;
int port;
char *name, *comp,*net,*loc,*sort;
int yymmdd,nsamp;
double rate;
{	struct holdstruct {
		int date;				/* yyyymmdd order */
		int date_ms;		/* ms since midnight */
		int ms;					/* length of packet in ms nsamp/rate*1000 */
		char scnl[12];	/* nnssssscccll seed name */
		char sort[2];
	};
	struct holdstruct a;
	static struct tcpsocket tcp;
	extern FILE *logout;
	char scnl[13];
	
	static int path=-1;
	int mon,day,ierr;
	char *hostname, *dotadr;
	if(path == -1) 
	{	
		if( isdigit(host[0]) )
		 {	dotadr=(char *) malloc(strlen(host)+1);
			 hostname=NULL;
			 strcpy(dotadr,host);
			 fprintf(logout,"dot adr=%s\n",dotadr);
		 }
		 else
		 {	hostname=(char *) malloc(strlen(host)+1 );
		 	 strcpy(hostname, host);
			 dotadr=NULL;
			 fprintf(logout,"hostname=%s\n",hostname);
		 }
		fprintf(logout,"Open holdings socket port=%d\n",port);
		path=init_safeudp(&tcp,hostname,dotadr,port);
		if(path <=0) {
			fprintf(logout,"Cannot create holdings socket. Abort.\n");
			exit(1);
		}
	}
	
	/* convert julian date to month and year */
	memset(&a.scnl,32,12);
	memcpy(&a.scnl,net,2);
	memcpy(&a.scnl[2],name,strlen(name));
	memcpy(&a.scnl[7],comp,3);
	if(strlen(loc) > 0) memcpy(&a.scnl[10],loc,strlen(loc));
	a.date = yymmdd;
	a.date_ms = ms;
	a.ms = nsamp/rate*1000.+0.001;
	memcpy(&a.sort,sort,2);
	strncpy(scnl,a.scnl,12);
	scnl[12]=0;
	/*fprintf(logout,"holding2; %s dt=%d %d ms=%d ns=%d %f\n",
			scnl,a.date,a.date_ms, a.ms,nsamp,rate);*/
	i4swap(a.date,&a.date);
	i4swap(a.date_ms, &a.date_ms);
	i4swap(a.ms, &a.ms);
	ierr=sendtoudp(&tcp,&a, 26); 
	/*fprintf(logout,"send=%d\n",ierr);*/
}
