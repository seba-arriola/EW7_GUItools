/* edgesock_thr.c -

/*----------------------------*/
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <thread.h>
#include <sys/types.h>
#include <math.h>
/*#ifdef FreeBSD
#include <sys/types.h>
#else
#ifdef OLD_SUNOS
#include <sys/types.h>
#else
#include <machine/types.h>
#endif
#endif*/
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <fcntl.h>
#include <signal.h>
#include <time.h>
#include "vdl.h"

#define FALSE 0
#define TRUE 1
     
extern FILE *logout;

int terminate_thread;
int pid;
int seq[1000];
	int get_seed(unsigned char route, unsigned char node,unsigned char chan,
	 char *name,char *comp, char *network, char *location, double
			*rate);
  void user_proc(struct nsntime tc, int ich, long idat[],int
			nsamp);


int esth_connected = FALSE ;
short dbgesth = FALSE ;
int edge_socket=0;							/* this socket results from accepts() on listen_socket*/
unsigned short edge_port = 0;
char edgeIP[20];
char *asctim();
void edgesock_shutdown();
	struct edgestruct {
		short leadin;
		short nsamp;
		char seedname[12];
		short year;
		short doy;
		short rateMantissa;
		short rateDivisor;
		unsigned char activity;
		unsigned char ioclock;
		unsigned char quality;
		char spare;
		int sec;
		int usec;
		int seq;
		int ts[1024];
	};
struct edgestruct *bufs; 
struct nsntime tcdummy;
long dummy[2];
int maxbufs=20;
int enextbuf,enext;	/* enext buf puts data in, enext takes it out*/
int maxnused;			/* max number of buffers used thus far */
/************************************************
variables used in thread code */
void * edgesockThread(void *);		/* this thread moves data from csbufs to LISS*/
unsigned edgeStackSize=4096;			
thread_t edgetid;

/*-----------------------------------------------------------------------------*/
edgesock_init( int argc, char **argv )
{	int i,k,n;

     /*------------------------------------------------------
      Parse command line.
      yes, it's a bit brutish - but others need these parms
    ----------------------------------------------------------*/
	/*configfile[0]='\0';*/
	dbgesth=FALSE;
	maxnused=0;
	for(i=0; i<argc; i++) {
		/*fprintf(logout,"arg[%d]=%s\n",i,argv[i]);*/
		if(strcmp(argv[i],"-edgeIP") == 0 || strcmp(argv[i],"-EIP") == 0) {
			strcpy(edgeIP,argv[i+1]);
			fprintf(logout,"esth : station=%s\n",edgeIP);
		}

		if(strcmp(argv[i],"-edgedbg") == 0 || strcmp(argv[i],"-EDGB") == 0) {
			dbgesth=atoi(argv[i+1]);
			fprintf(logout,"esth: verbosity=%d\n",dbgesth);
		}
		if(strcmp(argv[i],"-edgeport") == 0 || strcmp(argv[i],"-EP") == 0) {
			edge_port=atoi(argv[i+1]);
			fprintf(logout,"esth: service port= %d\n",edge_port);
		}
		if(strcmp(argv[i],"-edgebuf") == 0 || strcmp(argv[i],"-EB") == 0) {
			maxbufs=atoi(argv[i+1]);
			fprintf(logout,"esth: max buf=%d\n",maxbufs);
		}

	}
	if(edge_port == 0) return;			/* no edge sock port so no edgesock output*/
  pid=getpid();
	for(i=0; i<1000; i++) seq[i]=0;
	/* allocate memory for buffers */
	fprintf(logout,"esth: Allocate memory for %d buffers.\n",maxbufs);
	bufs = (struct edgestruct *) malloc(maxbufs*sizeof(struct edgestruct));
	enextbuf=0;

	/* these command line arguments are for manual runs only!  NETMON does not
	need them.  For dbgesth output, change port # in /etc/edgesock.ports to be 
	negative */
	/*fflush(monfile);
  
	/**************************** start thread *************/
	/* Note :THR_DETACHED is required for thr_exit() to work.  That is,
		a detached thread can truly kill itself without lingering in 
		some afterlife, waiting for some other thread to pick up it's exit
		status before it can truly cease to be ...*/

	terminate_thread=0;
	n = thr_create( NULL, edgeStackSize, edgesockThread,  (void *) 0, 
			THR_DETACHED , &edgetid);
	if(n != 0) {
		fprintf(logout,
				"esth:edgesockThread was not created errno=%d n=%d %s\n",
				errno,n, strerror(errno));
		exit(11);
	} 
	else fprintf(logout,"esth:edgesockThread created tid=%d\n",edgetid);
}


void * edgesockThread (void *dummy)	
{
	char dummy_buf[2];
	int npackets,len,ierr;
	int esth_connected;
	struct sockaddr_in outsock;					/* a socket data structure */
	u_long addr;							/* IP address after translation */
  /*---------------------------------------------------------------------
    create endpoint for communication and get descriptor
    for protocol address family AF_INET ==> internetwork: UDP, TCP, etc.
    of type SOCK_STREAM ==> sequenced, reliable, two-wayconnection-based
                             byte streams
    with protocol 0 ==> internet protocol, pseudo protocol number
  -----------------------------------------------------------------------*/
	esth_connected=0;
	npackets=0;
	enext=0;
	if( (int) (addr = inet_addr(edgeIP)) == -1) 
	{	fprintf(logout,"esth: edgeIP : Bad Dot address = %s \n",edgeIP);
		exit(203);
	}
	fprintf(logout,"esth: Good Dot address %x = %s\n",addr, edgeIP);
	memset( (char *) &outsock,0,sizeof(outsock));
	memcpy(&outsock.sin_addr, &addr, 4);
	outsock.sin_port=htons(edge_port);
	outsock.sin_family=AF_INET;
	fprintf(logout,"esth: host=%x port=%d\n",outsock.sin_addr,edge_port);
	while(terminate_thread == 0) {
		ierr=-1;
		while(ierr < 0) {
			 edge_socket = socket(AF_INET, SOCK_STREAM, 0);
			 if( edge_socket < 0 )
			 {	
  			 fprintf(logout, "   esth: %s/%d: 'socket()' failed/n",
						 edgeIP,edge_port);
  			 fprintf(logout, "   esth:error:%d\n%s\n",errno,strerror(errno));
  			 exit(errno);
			 }
			 ierr = connect(edge_socket, 
			 		(struct sockaddr *) &outsock,sizeof(outsock));
			 if(ierr < 0) {
				 fprintf(logout,
				 "esth: %s Connect %s/%d failed! s=%d ierr=%d errno=%d %s\n",
					 asctim(),edgeIP,edge_port,edge_socket,
					 ierr,errno,strerror(errno));
			 	sleep(30);
			 	close(edge_socket);
			}
		}
		/*if(dbgesth) */
			fprintf(logout,"   esth:%s %d socket esth_connected=%d %s/%d\n",
				asctim(),pid,edge_socket,edgeIP,edge_port);
    fflush(logout);
		user_proc(tcdummy, 0, dummy, -1);		/* force tag output */

		npackets=0;
  	/*---------------------------------------------------------------------
      	Wait for a connection attempt on the socket.
  	-----------------------------------------------------------------------*/
    esth_connected = TRUE ;
		fcntl(edge_socket, F_SETFL, O_NONBLOCK); /* set listen to be non block*/
		sleep(1);

   /*-------------------------------------------------------------------*/
    while (esth_connected)
    {

			/* now try a read from the socket to liss.  It should return
				a EWOULDBLOCK but if it returns an EOF, we need to shutdown
				the socket and wait for a new one */
			if(  (ierr = read(edge_socket, dummy_buf, 1)) < 0) {
				if(errno == EWOULDBLOCK || errno == 0) {
					if(dbgesth & 4) charoute("ROK");
				} else {
					fprintf(logout,"esth: %s read from edge. not esth_connected. err=%d errno=%d %s\n",
						asctim(),ierr,errno,strerror(errno));

					esth_connected = 0;
					break;
				}
			} else if(ierr == 0) {
				fprintf(logout,"esth: %s read return EOF.  Close connection\n",
							asctim());
				esth_connected=0;
				break;
			} else if(ierr > 0) {
				fprintf(logout,"esth: %s read returned data!!! ierr=%d\n",
						asctim(),ierr);
			}
			charoute("5");
			while( enext != enextbuf && esth_connected) {
				len = 40+(bufs+enext)->nsamp*4;
        if ( (ierr=send( edge_socket, bufs+enext, len, 0 )) != len )
        {	charoute("\n");
           fprintf( logout, "esth: %s %s: socket write ierr=%d errno=%d %s\n",
               asctim(),edgeIP, ierr, errno, strerror(errno)) ;
           esth_connected = FALSE;
           break;
        }
				if(dbgesth) fprintf(logout, 
					"esth: %s write out enext=%d %d max=%d len=%d done.\n",
					asctim(),enext,enextbuf,maxbufs,len);
				npackets++;
				enext++;
				if(enext >= maxbufs) enext = 0;
				if(dbgesth) fprintf(logout, 
					"esth: %s write aft enext=%d %d max=%d len=%d done.\n",
					asctim(),enext,enextbuf,maxbufs,len);
      } /* Data in buffer */
				if(dbgesth) fprintf(logout, 

										"esth: %s write aft2 nxt=%d %d max=%d len=%d done.\n",
					asctim(),enext,enextbuf,maxbufs,len);
			if(enext != enextbuf) continue;		/* process any data left */
			if(terminate_thread) break;
			if(dbgesth) fprintf(logout,"esth: do wait\n");
			usleep(2000000); /* do not check too often */
      charoute("W");
    } /*while (esth_connected) */
    /*-------------------------------------------------------------------*/
    fprintf(logout, "%d %s esth: Connection Lost.  Close Socket %d\n",			
      	pid, asctim(),edge_socket);

    if( shutdown(edge_socket, SHUT_RDWR) < 0) 
      fprintf(logout, "esth: %s shutdown sock errno=%d - %s\n",
					asctim(),errno, strerror(errno));
    if( close(edge_socket) < 0)
      fprintf(logout, "esth: %s close sock errno=%d - %s\n",
					asctim(), errno, strerror(errno));
		if(terminate_thread) break;
		sleep(10);
/* end if listen() */
	}				/* while (terminate_thread==0)

 /* Clean up Thread termination*/

	if(dbgesth) fprintf(logout,"%s esth:edgesockThread : terminated\n",asctim());
	fflush(logout);

	terminate_thread=2;
	thr_exit(&errno);
} 


/*  this routine is invoked from the exit_handlers or interrupt handlers. It
basically cleans up the sockets before exiting. */
void edgesock_shutdown()
{
/*	if(shutdown(listen_socket, SHUT_RDWR) < 0) 
	{	fprintf(logout,"%d Shutdown of listen socket faild %d = %s\n",
			getpid(), errno, strerror(errno));
			
	}
*/
	fprintf(logout,"esth: %s _shutdown() start port=%d\n",
		asctim(), edge_port); fflush(logout);
	if(edge_port == 0) return;				/* no edgesock configured- no op*/
	
	/* If the thread is running let it shutdown */
	if(terminate_thread == 0) {
		terminate_thread=1;
		while(terminate_thread==1) usleep(100000);
	}
	
	/* close up the listen or data port if they are open */
	fprintf(logout,"esth: %s shutdown() shutdown.\n",asctim()); fflush(logout);
	if(shutdown(edge_socket, SHUT_RDWR)  , 0) 	/* ditto the last output socket */
		fprintf(logout, "esth: %s shutdown sock err=%d - %s\n",
				asctim(), errno, strerror(errno));

	fprintf(logout,"esth: %s shutdown() close edge_socket.\n",
			asctim()); fflush(logout);
	if(close(edge_socket) < 0) 					/* ditto the last output socket */
		fprintf(logout, "esth: %s close sock err=%d - %s\n",
				asctim(), errno, strerror(errno));
	fprintf(logout,"esth: %s shutdown() wait terminate.\n",asctim()); fflush(logout);
	fprintf(logout,"%d esth: %s EXIT - close socket\n",
			getpid(),asctim()); fflush(logout);
	return;
}

charoute(s)
char *s;
{	
	if(dbgesth & 4 ) fprintf(logout,"%s",s);
	return 0;
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
	
	NOTE: Hack if nsamp = -1 the text in global tag is sent along

*/
#ifdef __STDC__
  void user_proc(struct nsntime tc, int ich, long idat[],int nsamp)
#else
  void 
	user_proc(tc,ich,idat,nsamp)
	struct nsntime tc;
  long idat[];						/* data buf with in the clear 32 bit longs */
  int nsamp;						/* number of frames in */
#endif
{	
	struct edge_header {
		short leadin;
		short nsamp;
		char seedname[12];
		short year;
		short doy;
		short rateMantissa;
		short rateDivisor;
		unsigned char activity;
		unsigned char ioclock;
		unsigned char quality;
		char spare;
		int sec;
		int usec;
		int seq;
	};
	struct edge_header hdr;
	extern FILE *logout;
	extern struct chan_desc *ch;
	extern int net;
	extern int node;
	extern char tag[];
	char network[3], location[3], name[6], cname[4],seedname[14];
	double rate,ratio;
	int ierr,i,nused;
	int iy,id,ih,im,is,ms,leap;
	static nbad_sta=0;
	if(edge_port == 0) return;			/* no edge sock port so no edgesock output*/
	nsnint(tc, &iy,&id,&ih,&im,&is,&ms,&leap);
	/*fprintf(logout, 
"esth:%4d %3d %2d:%2d:%2d.%3d ich=%d %d %s lp=%d sq=%3d ns=%4d\n",
	iy,id,ih,im,is,ms,ich,(ch+ich)->stat_chan,(ch+ich)->txt,leap,seq[ich],nsamp);
	*/
	if(nsamp > 0) {
		get_seed((char) net, (char) node, (ch+ich)->stat_chan,
				name,cname, network,location,&rate);
		if(dbgesth) fprintf(logout, 
		"esth: %s %s %s %s %4d %3d %2d:%2d:%2d.%3d lp=%d sq=%3d ns=%4d rt=%7.3f dat=%6d %6d %6d\n",
		asctim(),name,cname,network,iy,id,ih,im,is,ms,leap,seq[ich],nsamp,rate,
		idat[0],idat[1],idat[2]);

		if(strcmp(network,"??") == 0 || name[0] <='9') { 
			if(nbad_sta < 100) fprintf(logout,
					"esth: %s bad channel (no network or name) nt=%s nm=%s  %d\n",
					asctim(), network, name,nbad_sta);
			nbad_sta++;
			return;
		}
		ratio = fabs(ch[ich].freq_calc -rate)/rate;
		if( ratio > 0.01) {
			fprintf(logout,"nsnstation2 %d %d %d %s %s %s %s rate=%f cfr=%f ratio=%f do not agree\n",
					net,node,(ch+ich)->stat_chan, name,cname,network,location, rate,ch[ich].freq_calc,ratio);
			rate = ch[ich].freq_calc;
		}
	}
	
	hdr.leadin=0xa1b2;
	hdr.nsamp = nsamp;
	if(nsamp == 0) {
		fprintf(logout,"esth: %s user_proc called with no samples!\n",
				asctim());
		fflush(logout);
		return;
	}
	if(nsamp == -1) {
		hdr.nsamp=0;
		strcpy(hdr.seedname, tag);
		fprintf(logout,"esth: %s Add tag called %s %s\n",
				asctim(),tag,hdr.seedname); fflush(logout);
	}
	else {
		/* prepare the seedname from its components, fill spaces */
		memset(&hdr.seedname,0,12);
		strcpy(&hdr.seedname[0], network);
		strcpy(&hdr.seedname[2], name);
		strcpy(&hdr.seedname[7], cname);
		strcpy(&hdr.seedname[10], location);
		for(i=0; i<12; i++) if(hdr.seedname[i] == 0) hdr.seedname[i]=' ';
		strncpy(seedname,&hdr.seedname[0],12);
		seedname[12]=0;
		hdr.year=iy;
		hdr.doy=id;
		hdr.sec = ih*3600+im*60+is;
		hdr.usec= ms*1000;
		hdr.seq=seq[ich];
		if(rate >= .9999) {
			hdr.rateMantissa = rate*100+.001;
			hdr.rateDivisor= -100;
			/*if(hdr.rateMantissa % 10 != 0) fprintf(logout,"Wierd frequency=%f %d %d\n",
				rate,hdr.rateMantissa,hdr.rateDivisor);*/
		}
		else {
			hdr.rateMantissa = rate * 10000+.001;
			hdr.rateDivisor = -10000;
		}
		hdr.activity = 0;
		hdr.ioclock=0;
		hdr.quality=0;
		ierr = -1;
		if(dbgesth) fprintf(logout,
			"esth: %s Que to %d buf=%d hdr=%d %s%s%s%s %s %d %d %d.%d\n",
			asctim(),enextbuf,
			bufs+enextbuf,&hdr,network,name,cname,location,seedname,iy,id,hdr.sec,hdr.usec);fflush(logout);
		memcpy(&(bufs+enextbuf)->ts[0], &idat[0], nsamp*4);
	}
	memcpy(bufs+enextbuf, &hdr, 40);
	enextbuf++;
	if(enextbuf >= maxbufs)  enextbuf=0;
	nused = enextbuf - enext;
	if(nused < 0) nused += maxbufs;
	if(nused > maxnused) {
		maxnused=nused;
		fprintf(logout,"esth : ** high water nused=%d\n",maxnused);
	}
	if(enextbuf == enext) fprintf(logout,
		"esth: %s ***** lap ring buffer %d.\n",asctim(),enextbuf);
}

#ifdef __STDC__
	int get_seed(unsigned char route, unsigned char node,unsigned char chan,
	 char *name,char *comp, char *network, char *location, double *rate)
#else
  get_seed(route,node,chan,name,comp,network,location,rate)
  unsigned char route,node,chan;
  char * name, *comp, *network, *location;
   double *rate;
#endif
{
	FILE *in;
	extern FILE *logout;
	int i,rt,nd;
	char line[100];
	char nsnname[6];
	char *pnt;
	int failed;
	struct nsnstation {
		char network[3];
		char station[6];
		char comp[4];
		char location[3];
		unsigned char net;
		unsigned char node;
		unsigned char chan;
		float rate;
	};
#define MAX_SEED_CHAN 4000
	static struct nsnstation ch[MAX_SEED_CHAN];
	static time_t lastread=0;
	static int last_nchan;
	time_t now;
	static int nchan;
	int chn;
	failed=0;
	now=time(&now);
	/*fprintf(logout,"now=%d lastread=%d\n",now,lastread);*/
	if(lastread == 0 || abs(now-lastread) > 300) {
	 lastread=now;
	 in=fopen("nsnstation2.dat","r");		/* open station file */

	 if(in  == NULL) 						/* cannot find file */
	 {	fprintf(logout,
			 "*** nsnstation2.dat not found. No seed name. Make one up %d\n",in);
	 } else {
		 while (fgets(line,80,in) != NULL) 			/* get a line */
		 {	/*fprintf(logout,"%d %d %dline=%s\n",line[0],line[1],line[2],line);*/
			 if(line[0] == 'S' && line[1] == 't' && line[2] == 'a') break;
		 }

		 nchan=0;
		 while( fgets(line,80,in) != NULL) {
			 pnt=strchr(line,'-');					/* net,node,chan delimiter*/
			 if(pnt != NULL) 						/* if we found delimiter */
			 {	i=(long) pnt- (long) line;			/* index to line */
				 i=i-17;								/* start of data section*/
 				/*fprintf(logout,"%d %d %d line=%s\n",	
					 i,line[i+17],line[i+20],line);*/
				 if(i >= 0 && line[i+17] == '-' && line[i+20] == '-')
				 { if(line[i+10] == ' ') line[i+10]='?';
					 if(line[i+11] == ' ') line[i+11]='?';
					 sscanf(&line[i],
					 "%c%c%c%c%c %c%c%c %c%c%c%c %2x-%2x-%2x %lf\n",
					 &name[0],&name[1],&name[2],&name[3],&name[4],
					 &comp[0],&comp[1],&comp[2],&network[0],&network[1],
					 &location[0],&location[1],
					 &rt,&nd,&chn, rate);			/* read in the data */
					 name[5]=0;
					 comp[3]=0;
					 network[2]=0;
					 location[2]=0;
					 for(i=0; i<5; i++) if(name[i] == ' ') name[i]=0;/*null term*/
/*
					 Stations with route of zero are Phoenix/Multiple station
					 per mode site.  if any of these network types show up,
					 allow there network
 */
 /*					if( rt == 0 ) rt=route; /* removed 9/98 DCK for ATWC */
					 strcpy(ch[nchan].network,network);
					 strcpy(ch[nchan].station,name);
					 strcpy(ch[nchan].comp, comp);
					 strcpy(ch[nchan].location,location);
					 ch[nchan].net=rt;
					 ch[nchan].node=nd;
					 ch[nchan].chan=chn;
					 ch[nchan].rate=*rate;
					 nchan++;
					 if(nchan == MAX_SEED_CHAN) {
						 fprintf(logout,"***** get_seed needs more stations!\n"); nchan--;
					 }
				 }
			 }
		 }
	 }

	 if(last_nchan != nchan) {
		 fprintf(logout,"%s Number of nsnstation2.dat recs=%d\n",asctim(),nchan);
		 /*for(i=0; i<nchan; i++) fprintf(logout,
						 "%d name=%s comp=%s net=%s lc=%s rt=%d nd=%d ch=%d %10.3f\n",
						 i,ch[i].station,ch[i].comp,ch[i].network,ch[i].location,
			 		ch[i].net,ch[i].node,ch[i].chan,ch[i].rate);
		 fflush(logout);*/

	 }
	 last_nchan=nchan;
	 fclose(in);
 }
 for(i=0; i<nchan; i++) {
	 
	 if( ch[i].net == route && node == ch[i].node && chan == ch[i].chan)
	 {	strcpy(network, ch[i].network);
	 		strcpy(location, ch[i].location);
			strcpy(name, ch[i].station);
			strcpy(comp, ch[i].comp);
			*rate=ch[i].rate;
		   /*fprintf(logout,"%s at %d Fnd %s %s %s %d %d %d %10.3f\n",
			 asctim(),i,name,comp,network,route,node,chan, *rate); fflush(logout);*/
		 return 1;
	 }
 }
	
 	/*fprintf(logout,
	"%s *** No Match found for seed name.  nchan=%d rt=%d nd=%d ch=%d\n",
		asctim(),nchan,route,node,chan);
	/*for(i=0; i<nchan; i++) fprintf(logout,
						 "%d name=%s comp=%s net=%s lc=%s rt=%d nd=%d ch=%d %10.3f\n",
						 i,ch[i].station,ch[i].comp,ch[i].network,ch[i].location,
			 		ch[i].net,ch[i].node,ch[i].chan,ch[i].rate);
		fprintf(logout,"Turn on debug and try again....\n");*/
	sprintf(name,"%2d%3d",route,node);
	sprintf(comp,"%3d",chan);
	strcpy(network,"??");
	strcpy(location,"??");
	for(i=0; i<5; i++) if(name[i] == ' ') name[i]='0';;
	return -1;
}
#ifdef __STDC__
  int nsn_chan(unsigned char chanid, char *name, double *rate)
#else
  int nsn_chan(chanid,name,rate)
  unsigned char chanid;
  char * name;
  double * rate;
#endif
{
	static char range[]={'H','L','H','L'};
	static char band[4][11]={
				'H','B','B','B','L','B','B','M','?','?','?',/* 0-31 BB high gain*/
				'H','B','B','B','L','B','B','M','?','?','?',/* 32-63 BB low gain*/
				'E','S','S','S','L','S','S','M','?','?','?',/* 64-95 SP high gain*/
				'E','S','S','S','L','S','S','M','?','?','?'};/* 96-127 Sp low gain*/
	static double rates[14]=
		{80.,40.,20.,10.,1.,40.,20.,4.,0.,0.,0.,40.,40.,40.};
/* digit rates*/
	int irange;
	extern FILE *logout;
	if(chanid > 127) {
		fprintf(logout,"*** NSN illegal channel=%d \n",chanid);
		name="???";
		return -1;
	}
	if(chanid == 127) strcpy(name,"PWZ");
	else {
		irange=(0xE0 & chanid) /32;			/* get range */
		chanid=(chanid & 0x1f);				/* get only channel part of id */
		*rate=rates[chanid/3];				/* set digitizing rate */
		name[0]=band[irange][chanid/3];		/* set band */
		name[1]=range[irange];				/* set  range (normally H or L) */
		switch ( chanid % 3) {				/* choose direction */
			case 0: name[2]='N'; break;
			case 1: name[2]='E'; break;
			case 2: name[2]='Z'; break;
		}
	}
	return 0;
}
