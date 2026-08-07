/* user_proc_ew.c
 *
 * This file contains user-defined functions that are called
 * from the program rcv/station to: 
 *   1) set up an Earthworm-like environment (user_proc_cmd), 
 *   2) "dispose" of received data in an Earthworm-like manner 
 *      (ie, write messages onto transport ring) (user_proc),
 *   3) send Earthworm heartbeats on receipt of data or keepalive
 *      messages (user_heart_beat),
 *   4) shut down Earthworm things in a reasonable way (user_shutdown).
 *
 *  NOTE: I combined Dave Ketchum's user_proc.c and user_shutdowm.c
 *        in this source file and changed the makefile accordingly.  
 *        LDD:971110
 *
 *  May 2004 - D.C. Ketchum - change algorithm for forwarding data to :
 *     1) New packet is rejected if its last sample is after the 
 *        treleased time (too late) or if it is more that 10 minutes
 *        into the future relative to system time
 *     2)  Stash routine modified to 
 *          a)  Ship all packets buffered which have lingered in buffer
 *              by PacketLatency*60 seconds.
 *          b)  if buffers still full, ship oldest packet
 *          c)  Store newest packet in buffers
 *      3) Ship routine  modified to detect and print out gaps in output
 *           on a channel basis.  
 *     4)  Added a "throttle" which takes parameter ThrottleMS from config
 *         file and insures each 250000 bytes that at least that much time
 *         has past.  Sleep for enough time to make it up if it has not
 *         past.  This limits the aggregate rate to 100000*8/(throttleMS*1000)
 *         bits per second.  100000 was chosen to be small relative to the
 *         output ring buffer size.
 *      5) EOF in data simply means that the next expected sequence in 1.  
 *          (prior behavior dump all buffers when this happened.  Very bad
 *          at the end  of day when everybody dumped everything!
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>   /* required by getppid() */
#include <unistd.h>      /* required by getppid() */
#include <earthworm.h>
#include <transport.h>
#include <trace_buf.h>
#include <chron3.h>
#include <kom.h>

/* Variables which are set with command-line arguments of station.c
 ******************************************************************/
int Partial  = 0;  /* =1 if we may get partial packets; set by -partial */
int Multiple = 0;  /* =0 for single-channel processing;                 */
                   /* =1 for multiple-channel processing; set by -#     */
int Max_Chan = 0;  /* maximum # of channels to process;   set by -#     */
char * logptr;     /* pointer to translation of EW_LOG if available     */
static char pktlogfilename[120]="";/* If detail logging, filename to use */
static int packet_log=0;  /* =1 means create detailed packet logs        */
static FILE * logpkt;		 /* if packet_log is true, unit where details go */
static int lastyday=0;     /* used to track when to roll over packet log */
static void log_packet(char ,  char *, char *,char *,double , int , 
		double,int);
static void log_holding(char *net, char *station, char *comp, 
		double dtime, int nsamp, double rate);
/* Things to read or derive from Earthworm configuration file*
 ************************************************************/
static char    RingName[20];      /* name of transport ring for i/o         */
static char    MyModName[20];     /* speak as this module name/id           */
static int     LogSwitch;         /* =0 if no logfile should be written     */
static int     HeartBeatInt;      /* seconds between heartbeats to statmgr  */
static int     MaxSamplePerMsg;   /* max # samps to pack in a TRACE_BUF msg */
static int     PacketLatency = 0; /* # pckts to buffer before releasing one */
static int     Debug = 0;         /* optional Debug feature                 */
static long    MaxSilence = 0;    /* complain if we haven't seen data from a  */
                                  /*     channel in this many seconds         */  
static	double throttleSecs=1.;


/* Things to look up in the earthworm.d tables with getutil.c functions
 **********************************************************************/
static long          RingKey;       /* key of transport ring for i/o     */
static unsigned char InstId;        /* local installation id             */
static unsigned char MyModId;       /* Module Id for this program        */
static unsigned char TypeHeartBeat; 
static unsigned char TypeError;
static unsigned char TypeTraceBuf;

/* Stuff required to buffer packets to handle rollbacks
 ******************************************************/
#define MIN_PKTLATENCY  0     
#define MAX_PKTLATENCY  8
#define MAX_DATA        4096   /* same as MAX_DATA in station.c */

/* Structure to hold one packet */
typedef struct {      
   double tfirst;
   double tlast;
   double rate;
	 time_t arrived;
   int    leap;
   int    seq;
   int    eof;
   int    nsamp;
   long   idat[MAX_DATA];
} PACKET;

/* Structure to manage a series of packets for a given SCN 
 *********************************************************/
#define SITE_LEN   7
#define COMP_LEN   9
#define NET_LEN   9

typedef struct {       
   char     site[SITE_LEN];   
   char     comp[COMP_LEN];
   char     net[NET_LEN];
   int      pinno;    /* pin number assigned to this scn (AcceptSCN command)*/  
/*   double   prevtfirst;/* start-time of packet containing most recent data   */
/*   double   prevtlast;/* end-time of packet containing most recent data     */
   double   treleased;/* time of last data sample released for this SCN     */
/*   char     fullbuf;  /*=0 if <PacketLatency packets in buffer; =1 otherwise*/
	 int			gapExpected;	/* if 1, a gap is expected on next packet */
/*   char     emptybuf; /* =1 if buffer is empty; =0 otherwise                */ 
/*   int      ioldest;  /* index of oldest packet in buffer                   */
/*   int      inewest;  /* index of newest packet in buffer                     */
   int      seqnext;  /* next sequence number expected                        */
   PACKET  *packet;   /* array of packets being buffered for this SCN         */
   volatile time_t talive;  /* time (system clock) that last packet was  
                              received (watched by BigBrother thread)      */
   volatile long   silentlimit; /* # sec to wait before complaining that 
	 																no new data has been received 
                                   (updated by BigBrother thread)         */
} PKT_BUFFER;

PKT_BUFFER   *pBuffer;   /* array of packet-buffering structures          */
volatile int  Nscn  = 0; /* number of SCN's expected (AcceptSCN commands) */
int           nSeen = 0; /* number of channels we've seen so far          */

/* Earthworm error words used by rcv_ew
 **************************************/
#define   ERR_TOOBIG     0
#define   ERR_NOTRACK    1
#define   ERR_OVRFLW     2
#define   ERR_IGNORED    3
#define   ERR_DATAGAP    4
#define   ERR_SILENT     5
#define   ERR_ALIVEAGAIN 6
char      ErrText[128];       /* text for logfile/error msg */

/* Functions in this source file called in Ketchum's code
 ********************************************************/
void user_proc_cmd( int argc, char **argv );  /* process command-line args */
void user_proc( int, int, int, int, int, int, int, long *, int, 
                char *, char *, char *, int, double, int );
            /* convert a chunk of data into trace_buf msg & put it on ring */ 
void user_heart_beat( void );       /* issue timely Earthworm heartbeat    */
void user_shutdown( void );         /* shut down Earthworm things properly */

/* Functions in this source file used only in this source file  
 *************************************************************/
thr_ret BigBrother( void * );          /* BigBrother thread (monitors time  
                                            since last packet for each SCN)*/
int     rcv_ew_config( char * );       /* Read earthworm configuration file*/
void    rcv_ew_lookup( void );         /* Lookup numeric values for EW names*/
void    rcv_ew_error( short, char * ); /* Issue error msg to transport ring */
void    stash_packet( int, PACKET * ); /* handle new packet in pBuffer      */
void    ship_packet( int, PACKET * );  /* break a USNSN packet into TraceBuf  
                                          messages & put on transport ring  */
double  sec_since_1970( int, int, int, int, int, int );           
                                      /* time conversion(from yr-yday-hr...)*/ 
                               
/* Misc. globals
 ****************/
SHM_INFO Region;          /* memory region to write messages to         */
char     F_attach   = 0;  /* flag =1 after tport_attach                 */
char     F_logit    = 0;  /* flag =1 after logit_init                   */
char     F_malloc   = 0;  /* flag =1 after packet buffers are malloc'd   */
char     Init_heart = 0;  /* flag =1 after user_heart_beat is initialized*/
unsigned tidBigBro;       /* Thread id of the BigBrother thread          */

pid_t    MyParentPid;     /* process id of rcv (station's parent) */     
static	hrtime_t throttleTime;/* Last time Throttle was calculated */
static	int throttleBytes;	/* count bytes since last Throttle calc */
static long totalBytes;			/* total bytes in a baud rate interval (usualy day)*/
static time_t baudTime;			/* travels with totalBytes to compute avg baud*/

char *asctim();						/* system time UTC as a string (for logging */
char *dasctim(double d);	/* time of a packet in the internal double format */

char holding_host[40];
int holding_port;
char holding_sort[3];
/*****************************************************************
 * Earthworm's user_proc_cmd()                                   *
 *                                                               *
 * This routine is called by stationcmd.c and gives the user a   *
 * chance to process command line parameters.                    *
 *                                                               *
 *  -e configfile       Name the earthworm configuration file    *
 *                                                               *
 *****************************************************************/
void user_proc_cmd(int argc, char **argv)
{
   extern FILE *logout;
   unsigned stackSize;     /* Stack size of heartbeat threads   */
   char *config_file;
   int i,j;
	 struct tm *tmpnt;
	 time_t looptime;

   config_file = (char *) NULL;
	 holding_host[0]=0;

/* Process the command line arguments, ignoring arguments we don't need
 **********************************************************************/
   for (i=0; i<argc; i++) /* for each command line argument */
   {
      fprintf(logout,"user_proc_cmd: %d arg=%s\n", i,argv[i]); /*DEBUG*/

      /* Configure Earthworm part of rcv */
      if(strcmp(argv[i],"-e") == 0)  
      {
         config_file = argv[i+1];
         fprintf( logout,
                 "user_proc_cmd: use Earthworm config file <%s>\n",
                  config_file);
      }

      /* Look for station's partial update argument */
      if(strcmp(argv[i],"-partial") == 0)  
      {
         Partial = 1;
      }

      /* Look for station's multiple channel argument */
      if(strcmp(argv[i],"-#") == 0)  
      {
         Multiple = 1;
         Max_Chan = atoi(argv[i+1]);
      }
			if(strcmp(argv[i],"-o") == 0) 
			{
				 logptr=getenv("EW_LOG");
         if(logptr == NULL) strcpy(pktlogfilename,argv[i+1]);
         else
         { strcpy(pktlogfilename,logptr);
           strcat(pktlogfilename,argv[i+1]);
         }
				 pktlogfilename[strlen(pktlogfilename)-4] = 0;
				 strcat(pktlogfilename,"pkta");

			}
			if(strcmp(argv[i],"-packet_log") == 0)
			{	
				looptime=time(NULL);
			  packet_log=1;
				if(pktlogfilename[0] != 0)
				{	tmpnt=gmtime(&looptime);
					pktlogfilename[strlen(pktlogfilename)-1]=
							48+((tmpnt->tm_yday+1) % 10);
					logpkt=fopen(pktlogfilename,"a+");
					lastyday=tmpnt->tm_yday;
					fprintf(logpkt,"%s Open init log file=%s\n",asctim(),
							pktlogfilename); fflush(logpkt);
					fprintf(logout,"%s Open init log file=%s lstday=%d\n",
							asctim(),pktlogfilename,lastyday);
				}
			}
			if(strcmp(argv[i],"-HH") == 0)
			{	strcpy(holding_host,argv[i+1]);
				fprintf(logout,"Holdings host=%s\n",holding_host);
			}
			if(strcmp(argv[i],"-HS") == 0)
			{ strncpy(holding_sort, argv[i+1],2);
				fprintf(logout,"Holding sort=%s\n",holding_sort);
			}
			if(strcmp(argv[i],"-HP") == 0)
			{	holding_port=atoi(argv[i+1]);
				fprintf(logout,"Holding port=%d\n",holding_port);
			}


   }

/* Complain if unhappy with command-line arguments
 *************************************************/
   if( config_file == (char *) NULL )
   {
      fprintf( stderr, 
     "rcv_ew: No -e arg; cannot configure Earthworm part of rcv; exiting!\n");
      exit( -1 );
   }

   if( !Multiple )
   {    
      fprintf( stderr, 
       "rcv_ew: No -# argument (required for rcv on Earthworm); exiting!\n");
      exit( -1 );
   }
 
   if( Partial )
   {    
      fprintf( stderr, 
   "rcv_ew: rcv on Earthworm won't run properly with -partial argument; exiting!\n");
      exit( -1 );
   }

/* Allocate space for managing SCN's,  
   based on Max_Chan set by the -# argument 
 ******************************************/
   pBuffer = (PKT_BUFFER *) malloc( Max_Chan*sizeof(PKT_BUFFER) );
   if( pBuffer == (PKT_BUFFER *)NULL )
   {
      fprintf( stderr,
	"rcv_ew:user_proc_cmd: Cannot allocate PKT_BUFFER for Max_Chan=%d; exiting!\n",
      Max_Chan );
      exit( -1 );
   }
   memset( pBuffer, 0, Max_Chan*sizeof(PKT_BUFFER) );
   F_malloc = 1;
     
/* Read the Earthworm configuration file(s)
 ******************************************/
   rcv_ew_config( config_file );

/* Look up important info from earthworm.d tables
 ************************************************/
   rcv_ew_lookup(); 

/* Initialize name of log-file & open it 
 ***************************************/
   logit_init( config_file, (short) MyModId, 512, LogSwitch );
   logit( "", "rcv_ew: This is STATION running as an Earthworm module. " );
   logit( "", "Read command file <%s>\n", config_file );
   F_logit = 1;

/* Attach to Input/Output shared memory ring 
 *******************************************/
   tport_attach( &Region, RingKey );
   F_attach = 1;

/* Initialize variables in packet-buffering structure,
   allocate space for buffering packets for each accepted SCN 
 ************************************************************/
   for( i=0; i<Nscn; i++ )
   {
      pBuffer[i].silentlimit =  MaxSilence;
/*dck     pBuffer[i].prevtfirst  =  0.0;
      pBuffer[i].prevtlast   =  0.0;*/
      pBuffer[i].treleased   =  0.0;
			pBuffer[i].gapExpected = 1;				/* start up expect gaps */
/*dck      pBuffer[i].fullbuf     =  0;
      pBuffer[i].emptybuf    =  1;
      pBuffer[i].ioldest     =  0;
      pBuffer[i].inewest     =  0;*/
      pBuffer[i].seqnext     = -1;/* flag that we haven't seen this one yet */
      pBuffer[i].talive      =  time(NULL);
      if( PacketLatency )
      {
         pBuffer[i].packet = (PACKET *) malloc( PacketLatency*sizeof(PACKET) );
         if( pBuffer[i].packet == (PACKET *)NULL )
         {
            logit("e","rcv_ew:user_proc_cmd: Cannot allocate %d*sizeof(PACKET) bytes for iscn=%d\n",
                   PacketLatency, i );
            exit( -1 );
         }
				 for(j=0; j<PacketLatency; j++) pBuffer[i].packet[j].tfirst = -1.;
      }
   }

/* Send first heartbeat
 **********************/
   user_heart_beat();
 
/* Start the BigBrother thread
 *****************************/
   stackSize = 4096;
   if( StartThread( BigBrother, stackSize, &tidBigBro ) == -1 )
   {
      logit( "e", 
					"rcv_ew:user_proc_cmd: Error starting BigBrother thread!\n" );
      exit( -1 );
   }

	 /* initialize throttle variables */
	 totalBytes=0;
	 baudTime = time(NULL);
	 throttleBytes=0;
	 throttleTime = gethrtime();
   return;
}

/*************************************************************************
 *  Earthworm's user_proc()                                              *
 *                                                                       *
 *  Note that EOF may be delivered by itself (i.e. with nsamp=0)         *
 *                                                                       *
 *  iy,id,ih,im,is,ms  Ints with year, day, hour, minute, second and mS  *
 *  leap	If non-zero, it's a leap second day                      *
 *  idat	Array of longs containing data                           *
 *  nsamp	Number of samples in IDAT                                *
 *  name	C-string with SEED name of station being processed       *
 *  cname	C-string with SEED name of channel being processed       *
 *  network	C-string with SEED name of network originating the data  *
 *  eof	        If set, the detection/trigger/timeseries segment is over *
 *  rate	The digitizing rate in Hz                                *
 *  seq	        The channel sequence number.  For each channel this      *
 *              should increase from 1 to 255 for each segment of        *
 *              data.  It should go back to one after the last           *
 *              record (EOF set to true)                                 *
 *                                                                       *
 *************************************************************************/
#ifdef __STDC__
  void user_proc(int iy, int id, int ih, int im, int is, int ms, int leap,
		 long idat[], int nsamp, char *name, char *cname, char *network,
		 int eof, double rate, int seq)
#else
  void  user_proc(iy,id,ih,im,is,ms,leap,idat,nsamp,name,cname,network,
		  eof,rate,seq)
  int iy,id,ih,im,is,ms,leap;   /* broken down time code               */
  int eof;                      /* if true, end of this detection on
																this channel */
  long idat[];                  /* data buf with in the clear 32 bit longs*/
  int seq;                      /* channel sequence #                 */
  int nsamp;                    /* number of samples in idat          */
  char *name;                   /* seed station name (up to 5 char)   */
  char *cname;                  /* seed channel name (up to 3 char)   */
  char *network;                /* network name (up to 2 char)        */
  double rate;                  /* digitizing rate                    */
#endif
{
   PACKET  newpacket; /* structure to hold just-received packet        */
   double  dt;        /* sampling interval (1/rate)                    */
   int     iscn;      /* index in pBuffer of this SCN                  */
   int     iout;      /* index in pBuffer.packet of a packet to ship   */
   int     iin;       /* index in pBuffer.packet to place new data     */
   int     iold;      /* working index of oldest packet in pBuffer     */
   int     inew;      /* working index of newest packet in pBuffer     */
   int     usable;    /* flag for rollback packets                     */
   int     pinno;
   int     i;
   int     rc;

   if(Debug) 
   {
      logit("e",
				"#rcv_ew:user_proc: got %s %s %s %4d %3d %02d:%02d:%02d.%03d lp=%d sq=%3d ns=%4d eof=%d rt=%7.3f dat=%6d %6d %6d ... %6d\n",
            name,cname,network,iy,id,ih,im,is,ms,leap,seq,
            nsamp,eof,rate,idat[0],idat[1],idat[2],idat[nsamp-1]);
   }

/* Stuff new data into a PACKET structure
 ****************************************/
   newpacket.tfirst  = sec_since_1970( iy, id, ih, im, is, ms );
   newpacket.rate    = rate;
   newpacket.leap    = leap; 
   newpacket.seq     = seq;
   newpacket.eof     = eof;
	 newpacket.arrived = time(NULL);			/* system time packet got to us */
   if( nsamp <= MAX_DATA ) {
      newpacket.nsamp = nsamp;
   } else {
      newpacket.nsamp = MAX_DATA;
      sprintf( ErrText, 
         "user_proc: %s %s %s seq:%d nsamp:%d overflows buffer; truncated to %d samples.",
          name, cname, network, seq, nsamp, MAX_DATA );
      rcv_ew_error( ERR_OVRFLW, ErrText );
   }
   newpacket.tlast = newpacket.tfirst + 
			 (double)(newpacket.nsamp-1)/newpacket.rate; 
   memcpy( newpacket.idat, idat, newpacket.nsamp*sizeof(long) );
			if(packet_log) log_packet('$',network, name, cname,  
				newpacket.tfirst, nsamp, rate, seq);

/* Get index of this S-C-N in pBuffer
 ************************************/
   for( iscn=0; iscn<Nscn; iscn++ )   
   {
      if( strcmp( pBuffer[iscn].site, name    ) != 0 ) continue;
      if( strcmp( pBuffer[iscn].comp, cname   ) != 0 ) continue;
      if( strcmp( pBuffer[iscn].net,  network ) != 0 ) continue;
      break;
   }
   if( iscn==Nscn ) /* this SCN is not in our AcceptSCN list */
   {
      sprintf(ErrText, 
					"user_proc: ignore <%s %s %s>; not in AcceptSCN list!", 
              name, cname, network );
      rcv_ew_error( ERR_IGNORED, ErrText );
      return;
   }
   if( pBuffer[iscn].seqnext == -1 )  /* First time we've seen this SCN! */
   {
      pBuffer[iscn].seqnext = seq;
      nSeen++;
      logit("et",
				"rcv_ew:user_proc: First packet from %s %s %s; iscn=%3d  nchan=%3d\n",
        pBuffer[iscn].site,pBuffer[iscn].comp,pBuffer[iscn].net,iscn,nSeen);
   } 

/* Note when (system clock) we received this packet 
 **************************************************/
   pBuffer[iscn].talive = newpacket.arrived;

/* Is this packet from in the future.  If so, log and discard */

	 if( ((double) newpacket.arrived - newpacket.tfirst) < -600.) {
		 logit("et","%s %s %s is from the far future (%8.1f s).  Discard...\n",
				 pBuffer[iscn].site,pBuffer[iscn].comp,pBuffer[iscn].net,
				((double) newpacket.arrived - newpacket.tfirst));
		 	return;
	}
		
/* Is this packet too old because previous data has been released
 *************************************/
   if( newpacket.tlast > pBuffer[iscn].treleased ) /* no, check it out */
   {

   /* Got the next expected seq# for this channel 
    ---------------------------------------------*/
      if( newpacket.seq == pBuffer[iscn].seqnext ) 
      {
         if( Debug ) {
            logit("et",
								"#rcv_ew:user_proc: Got expected seq=%3d for %s %s %s\n",
                  newpacket.seq,pBuffer[iscn].site,pBuffer[iscn].comp,
                  pBuffer[iscn].net ); 
         }
      }

   /* Otherwise, sequence is out of order. Complain! 
    ------------------------------------------------*/
      else 
      {
       	logit("et",
						  "rcv_ew:user_proc: out of order seq=%3d for %s %s %s. "
						 "Expect=%3d (up to %d lost)\n",
            newpacket.seq,pBuffer[iscn].site,pBuffer[iscn].comp,
            pBuffer[iscn].net,pBuffer[iscn].seqnext,
            (newpacket.seq-pBuffer[iscn].seqnext+256)%256 );
			
      }
   
   /* Put the new packet away 
    -------------------------*/
      stash_packet( iscn, &newpacket );

   } /*end if time marching forward*/
      
/* Otherwise, Data is too late.  Note it and drop it
 ****************************************************/
   else if(fabs(newpacket.tlast - pBuffer[iscn].treleased) > 0.001) {
		 logit("et","rcv_ew:user_proc:  dup data %s %s %s seq=%d\n",name,cname,network,seq);
	 } else {  
    	logit("et",
				"rcv_ew:user_proc: seq=%3d lst=%d for %s %s %s cannot be used - too old.\n",
          seq, pBuffer[iscn].seqnext, name, cname, network );
   } /* end else time went backwards */

/* On EOF, ship all remaining buffered packets; assume no rollbacks will 
 occur. If we waited for more packets with triggered data, it might be a long time!  
 **************************************************************************/
   if( newpacket.eof )
   {
      if(Debug) logit("et", 
				"#rcv_ew:user_proc: %s %s %s end-of-segment detected seq=%d\n",
                       name, cname, network, newpacket.seq );

      pBuffer[iscn].seqnext  = 1; 
   }

   return;
}


/***************************************************************************
 *  Earthworm's user_heart_beat()                                         *
 *                                                                        *
 *  Called by station.c.  Send an Earthworm heartbeat if the time since the*
 *  last heartbeat is greater than HeartbeatInt (set in config file).      *
 *                                                                         *
 **************************************************************************/
void user_heart_beat( void )
{       
   static time_t   tlasthb;      /* time of last heartbeat sent          */
   static MSG_LOGO logo;
   char            msg[32];
   unsigned short  length;
   time_t          tcurrent;     /* current time */

   time( &tcurrent );

/* Initialize stuff, force the first heartbeat out
 *************************************************/
   if( !Init_heart )
   {
      MyParentPid = getppid();  
      logo.instid = InstId;
      logo.mod    = MyModId;
      logo.type   = TypeHeartBeat;
      tlasthb     = tcurrent - HeartBeatInt - 1;
      Init_heart = 1;
   }

/* Is it time to beat our heart?
 *******************************/
   if( (tcurrent-tlasthb) >= HeartBeatInt )
   {
   /* Include the process id in the heartbeat message so that this process  */
   /* can be automatically restarted by statmgr/startstop.  We need to use  */
   /* the parent pid, because the parent "rcv" is the process that startstop*/
   /* knows about, even though the heartbeat is actually coming from a      */
   /* function called by the "station" process.                             */

      sprintf( msg, "%ld %ld\n\0", (long) tcurrent, (long) MyParentPid );
      length = strlen( msg );
      if( tport_putmsg( &Region, &logo, length, msg ) != PUT_OK )
         logit( "et", "rcv_ew: Error sending heartbeat to transport ring\n" );
      tlasthb = tcurrent;
      if(Debug) logit("et","#user_heart_beat: sent heartbeat: %s", msg );
      return;
   }

   if(Debug) logit("et","#user_heart_beat: not time yet.\n");
   return;
}


/***************************************************************************
 *  Earthworm's user_shutdown()                                            *
 *                                                                         *
 *  Detaches from transport ring, and performs any other cleanup duties.   *
 *                                                                         *
 **************************************************************************/
void user_shutdown( void )
{
   extern FILE *logout;
   int i;

   fprintf(logout," USERSHUTDOWN called\n");

   if(F_malloc)
   {
      for( i=0; i<Nscn; i++ ) free( pBuffer[i].packet );
      free( pBuffer );
   }  

   if(F_attach) tport_detach( &Region );

   if(F_logit)  logit("t","rcv_ew:user_shutdown: Cleanup complete.\n" );
  
   return;
}

/***************************************************************************
 *  stash_packet()                                                         *
 *  Put another USNSN packet into the packet buffer, shipping the oldest   *
 *  packet first, if the buffer is full.                                   *
 **************************************************************************/
void stash_packet( int iscn, PACKET *pkt )
{
	int shipped,iold,i,empty;
	time_t now;
	double oldest,age;
 /*dckint  iout;    /* index in pBuffer.packet of a packet to ship   */
 /*dckint  iin;     /* index in pBuffer.packet to place new data     */

/* We're not buffering any packets, ship it right away 
*****************************************************/
 if( PacketLatency == 0 )
 {
     ship_packet( iscn, pkt );
     pBuffer[iscn].seqnext    = (pkt->seq+1)%256;
     return;
 }

/* if this packet is a duplicate of an existing packet, override it
*********************************************/

	if(Debug) logit("et","#stash : %s %s %s iscn=%d\n",
		pBuffer[iscn].site, pBuffer[iscn].comp, pBuffer[iscn].net, iscn );

	for(i=0; i<PacketLatency; i++) {
		if(fabs(pkt->tfirst - pBuffer[iscn].packet[i].tfirst) < 0.001 &&
				pkt->seq == pBuffer[iscn].packet[i].seq) {
			pBuffer[iscn].packet[i] = *pkt;
			logit("et","stash : %s %s %s replacing packet %d\n",
				pBuffer[iscn].site, pBuffer[iscn].comp, pBuffer[iscn].net,i);
			return;
		}
	}

/* Ship all packets which are timed out to potentially make room!
*****************************************************************/

	now = time(NULL);
	shipped=1;
	empty = -1;
	while (shipped) {
		shipped=0;
		oldest = 1.e30;
		iold=-1;
		age=0;
		for(i=0; i<PacketLatency; i++) {

			/* if this packet is now too old to send, purge it out */
  		if( pBuffer[iscn].packet[i].tfirst > 0. && 
					pBuffer[iscn].packet[i].tlast < pBuffer[iscn].treleased ) {
		 		logit("et","Stash : %s %s %s saved packet %d is too old. discard ....\n",
				 	pBuffer[iscn].site, pBuffer[iscn].comp, pBuffer[iscn].net,i);
				pBuffer[iscn].packet[i].tfirst = -1.;
			}

			/*  save the index of the first free buffer */
			if(pBuffer[iscn].packet[i].tfirst < 0. && empty == -1) empty = i;

			/* if this packet is in use and is older, save its index and age */
			if(pBuffer[iscn].packet[i].tfirst > 0. &&	/* is this packet in use */
					pBuffer[iscn].packet[i].tfirst < oldest) {
				oldest = pBuffer[iscn].packet[i].tfirst;
				iold = i;
				age = difftime(now, pBuffer[iscn].packet[i].arrived);
			}
		}

		/* the oldest used buffer is pointed to by iold and is age old, if it is
		aged enough send it out */
		if( iold >=0 && age > PacketLatency*60) {
	  	if(Debug) logit("et","#stash : %s %s %s ship old=%d age=%5.1f\n", 
				pBuffer[iscn].site, pBuffer[iscn].comp, pBuffer[iscn].net,iold,age );
			ship_packet(iscn, &pBuffer[iscn].packet[iold]);
			shipped=1;
		}					
	}

	/* Put the packet in empty.  If empty is -1, 
	then force shipment of oldest and put there */
	if(empty == -1) {
	  if(Debug) logit("et","#stash: %s %s %s Force Oldest %d age=%5.1f\n", 
		pBuffer[iscn].site, pBuffer[iscn].comp, pBuffer[iscn].net,
			iold,difftime(now,pBuffer[iscn].packet[iold].arrived) );
		ship_packet(iscn, &pBuffer[iscn].packet[iold]);
		empty = iold;
	}
	if(Debug) logit("et","#stash2: %s %s %s iscn=%d iold=%d iempty=%d age=%5.0f\n",
		pBuffer[iscn].site, pBuffer[iscn].comp, pBuffer[iscn].net,
		iscn,iold,empty, 
		(iold >=0) ? difftime(now,pBuffer[iscn].packet[iold].arrived) : -1.);
		
					
	/* if this packet is now too old to send, purge it out */
  if( pkt->tlast < pBuffer[iscn].treleased ) {
		logit("et","Stash : %s %s %s packet is too old. discard ....\n",
			pBuffer[iscn].site, pBuffer[iscn].comp, pBuffer[iscn].net);
		return;
	}
	
	/* if there is no empty, then error out.  This should never happen!*/
	if(empty == -1) {
		 logit("et","stash : %s %s %s could not stash empty=%d iold=%d\n",
				pBuffer[iscn].site, pBuffer[iscn].comp, pBuffer[iscn].net,empty,iold);
	}
	else {
		if(Debug) logit("et","#stash : %s %s %s stash in %d\n", 
			pBuffer[iscn].site, pBuffer[iscn].comp, pBuffer[iscn].net,empty );
		pBuffer[iscn].packet[empty] = *pkt;
		pBuffer[iscn].seqnext     = (pkt->seq+1)%256;
	}

	return;
}

/****************************************************************************
 *  ship_packet()                                                           *
 *  Break a USNSN packet into Earthworm TRACE_BUF messages and put them onto*
 *  a transport ring.                                                       *
 ***************************************************************************/
void ship_packet( int iscn, PACKET *pkt )
{	extern FILE *logout;
   TracePacket   tbuf;      /* Earthworm trace data buffer (trace_buf.h) */
   MSG_LOGO      logo;      /* used to label messages in transport ring  */
   long         *tdata;     /* pointer to trace_buf data samples         */
   long          tbuflen;   /* length of this trace_buf message          */
   int           packn;     /* # samples to pack into current message    */
   int           npacked;   /* # samples processed so far                */
   int           rc;
	 double    		 gap;				/* used to see if a gap is present */
	 useconds_t	utime;				/* time to throttle in uSecs if needed */
	 double			elapsed;				/* used in throttle calc */
	 char    pfbuf[500];
	 hrtime_t 	now;
	 time_t     tnow;

   if(Debug)
   {
      logit("e",
				"#rcv_ew:ship_packet: ship %s %s %s seq=%3d  %.3lf dat=%6d %6d %6d\n",
             pBuffer[iscn].site, pBuffer[iscn].comp, pBuffer[iscn].net, 
             pkt->seq, pkt->tfirst, pkt->idat[0], pkt->idat[1], pkt->idat[2]);
   }

/* Initialize some stuff
 ***********************/
   logo.instid = InstId;
   logo.mod    = MyModId;
   logo.type   = TypeTraceBuf;
   tdata       = (long *)( tbuf.msg + sizeof(TRACE_HEADER) );

/* Fill out non-varying parts of trace_buf header
 ************************************************/
   tbuf.trh.pinno    = pBuffer[iscn].pinno;
   tbuf.trh.samprate = pkt->rate;
   strncpy( tbuf.trh.sta,  pBuffer[iscn].site, 7 );
   strncpy( tbuf.trh.net,  pBuffer[iscn].net,  9 );    
   strncpy( tbuf.trh.chan, pBuffer[iscn].comp, 9 );
#ifdef _SPARC
   strcpy( tbuf.trh.datatype, "s4" );
#else
   strcpy( tbuf.trh.datatype, "i4" );
#endif
   tbuf.trh.quality[0] = '\0';  tbuf.trh.quality[1] = '\0'; 
   tbuf.trh.pad[0] = '\0';      tbuf.trh.pad[1] = '\0'; 

/* Loop thru all of the samples in this packet
 *********************************************/
   packn   = MaxSamplePerMsg;
   npacked = 0;

/* check an note a gap in the data 
************************************/
	 gap = pkt->tfirst - pBuffer[iscn].treleased - 1./pkt->rate;
	 if(gap > 0.5/pkt->rate && pBuffer[iscn].treleased > 0.1) {
		 if(!pBuffer[iscn].gapExpected) {
			  sprintf( ErrText,
						"ship_packet: %s %s %s gap of %7.3f secs seq=%d %s",
						pBuffer[iscn].net, pBuffer[iscn].site, pBuffer[iscn].comp, 
						gap, pkt->seq, dasctim(pBuffer[iscn].treleased));
         rcv_ew_error( ERR_DATAGAP, ErrText );
		 }
		 else {
			  sprintf( ErrText,
						"ship_packet: %s %s %s gap of %7.3f secs seq=%d %s EXPECTED",
						pBuffer[iscn].net, pBuffer[iscn].site, pBuffer[iscn].comp, 
						gap, pkt->seq, dasctim(pBuffer[iscn].treleased));
         rcv_ew_error( ERR_DATAGAP, ErrText );
		 }
	 }
	 pBuffer[iscn].gapExpected = pkt->eof;		/* if this is an EOF, next one is gap*/

	 
/* If this packet overlaps the previously released packet,
   move to the first sample that hasn't been sent yet.
 *********************************************************/
   if( pkt->tfirst <= (pBuffer[iscn].treleased + 0.5/pkt->rate) )
   {
      npacked = (int)((pBuffer[iscn].treleased-pkt->tfirst)*pkt->rate + 1.5);
      if(Debug) {
        logit("e",
					"#rcv_ew:ship_packet: %s %s %s seq=%3d overlaps previous packet by", 
          pBuffer[iscn].site, pBuffer[iscn].comp, pBuffer[iscn].net, 
					pkt->seq);  
        if(npacked==1) logit("e"," %d sample\n",  npacked );
        else           logit("e"," %d samples\n", npacked );
      }
   }

   while( npacked < pkt->nsamp )
   {
      if( packn > (pkt->nsamp-npacked) ) packn = pkt->nsamp-npacked;

   /* Fill out variable parts of trace_buf header
    *********************************************/
      tbuf.trh.nsamp     = packn;
      tbuf.trh.starttime = pkt->tfirst + (double)npacked/pkt->rate;
      tbuf.trh.endtime   = tbuf.trh.starttime + (double)(packn-1)/pkt->rate;
		

   /* Fill out trace_buf body with data samples
    *******************************************/
      memcpy( (void *)tdata, (void *)&pkt->idat[npacked], sizeof(long)*packn);
   
   /* Put new message onto the transport ring
    *****************************************/
		log_holding( tbuf.trh.net, tbuf.trh.sta, tbuf.trh.chan, 
			tbuf.trh.starttime, packn, pkt->rate);
			if(packet_log) log_packet('#',tbuf.trh.net, tbuf.trh.sta, tbuf.trh.chan,
				tbuf.trh.starttime, tbuf.trh.nsamp, pkt->rate, -1);
      tbuflen = sizeof(TRACE_HEADER) + sizeof(long)*tbuf.trh.nsamp;
			throttleBytes += tbuflen;
			totalBytes += tbuflen;
      rc = PUT_NOTRACK;
			while (rc == PUT_NOTRACK) {
				rc = tport_putmsg( &Region, &logo, tbuflen, tbuf.msg );
				if(rc == PUT_NOTRACK) fprintf(logout,"%s PUT_NOTRACK\n",asctim());
				if(rc != PUT_OK) usleep(20000);
			}
      switch(rc)
      {
      case PUT_OK:
                 break;
      case PUT_NOTRACK:
               rcv_ew_error( ERR_NOTRACK, 
               "ship_packet: TRACE_BUF msg not sent, NTRACK_PUT exceeded." );
                break;
      case PUT_TOOBIG:
                sprintf( ErrText,
                "ship_packet: TRACE_BUF msg[%ld] not sent, too big for %s.",
                     tbuflen, RingName );
                rcv_ew_error( ERR_TOOBIG, ErrText );
                break;
      }
   
    /*if(Debug && rc!=PUT_TOOBIG)
      {
         logit("e",
	"#rcv_ew:ship_packet:  put %s %s %s pin:%d %.3lf %.3lf rt:%.1lf ns:%4d\n",
                tbuf.trh.sta, tbuf.trh.chan, tbuf.trh.net, tbuf.trh.pinno,
                tbuf.trh.starttime, tbuf.trh.endtime,
                tbuf.trh.samprate, tbuf.trh.nsamp );
      }*/

      npacked += packn;

   } /*end while over pkt->nsamp */

/* Update time of last data sample released 
 ******************************************/
   pBuffer[iscn].treleased = tbuf.trh.endtime;
	 pkt->tfirst = -1.;				/* mark buffer as available */
	 if(throttleBytes > 250000) {
		 now = gethrtime();
		 elapsed = ((double)(now-throttleTime))*0.000000001;
		 if(elapsed < 0.) elapsed = throttleSecs+.1;
		 /*sprintf(pfbuf,"  *** Throttle %lld %lld %d elapse %8.1f rate %6.1f\n",
				 now, throttleTime,throttleBytes,elapsed, throttleBytes*8./elapsed);
		 logit("et","%s",pfbuf);*/
		 if( elapsed < throttleSecs) {
			 utime = (throttleSecs - elapsed) * 1000000. +0.5;
			 logit("et", "Throttle for %d usecs diff=%8.3f tsecs=%8.3f\n", utime,
					 elapsed, throttleSecs);
			 if(utime > 0) usleep(utime);
		 }
		 throttleTime = gethrtime();
		 throttleBytes = 0;
	 }
	 if(totalBytes > 100000000 ) {
		 tnow=time(NULL);
		 logit("et","%s Total bytes=%d baud=%8.0f\n",asctim(),
				totalBytes, totalBytes*8/(difftime(tnow,baudTime)+1.)); 
		 totalBytes=0;
		 baudTime = tnow;
	}
}

/****************************************************************************
 *  sec_since_1970()                                                        *
 *  Converts time given in: year, day-of-year, hour, min, sec, millisec     *
 *                      to: (double) seconds since 1970                     *
 ***************************************************************************/
double sec_since_1970( int yr, int yday, int hr, int min, int sec, int msec )
{
   struct Greg g;
   double t;
   double sec1970 = 11676096000.00;  /* # seconds between Carl Johnson's    */
                                     /* time 0 and 1970-01-01 00:00:00.0 GMT*/
   g.year   = yr;               
   g.month  = 1;
   g.day    = 1;
   g.hour   = hr;
   g.minute = min;
   g.second = 0.;

   t = (double) (60.0*julmin(&g))  +  
       (double) (86400.0*(yday-1)) + 
       (double) sec + 
       (double) msec*(0.001);

   return( t-sec1970 );
}
double magic_factor(  )
{

   double sec1970 = 11676096000.00;  /* # seconds between Carl Johnson's  */  
  
   return(sec1970 );
}

/***************************************************************************
 *  rcv_ew_config() processes command file(s) using kom.c functions;       *
 *                  Exits if any errors are encountered 	                 *
 **************************************************************************/
#define ncommand  8          /*# of required commands you expect to process*/

int rcv_ew_config( char *configfile )
{
   char     init[ncommand]; /* init flags,one byte for each required command*/
   int      nmiss;          /* number of required commands that were missed*/
   char    *com;
   int      nfiles;
   int      success;
   int      i;	
   char*    str;
   int      truelimit;  
	 extern 	FILE *logout;

/* Set to zero one init flag for each required command 
 *****************************************************/   
   for( i=0; i<ncommand; i++ )  init[i] = 0;

/* Open the main configuration file 
 **********************************/
   nfiles = k_open( configfile ); 
	 fprintf(stderr,"open user config=%s\n",configfile);
   if ( nfiles == 0 ) {
	fprintf( stderr,
                "rcv_ew_config: Error opening command file <%s>; exiting!\n", 
                 configfile );
	exit( -1 );
   }

/* Process all command files
 ***************************/
   while(nfiles > 0)   /* While there are command files open */
   {
        while(k_rd())        /* Read next line from active file  */
        {  
	    com = k_str();         /* Get the first token from line */

        /* Ignore blank lines & comments
         *******************************/
            if( !com )           continue;
            if( com[0] == '#' )  continue;

        /* Open a nested configuration file 
         **********************************/
            if( com[0] == '@' ) {
               success = nfiles+1;
               nfiles  = k_open(&com[1]);
               if ( nfiles != success ) {
                  fprintf( stderr, 
                 "rcv_ew_config: Error opening command file <%s>; exiting!\n",
                           &com[1] );
                  exit( -1 );
               }
               continue;
            }

        /* Process anything else as a command 
         ************************************/
  /*0*/     if( k_its("LogFile") ) {
                LogSwitch = k_int();
                init[0] = 1;
            }
  /*1*/     else if( k_its("MyModuleId") ) {
                str = k_str();
                if(str) strcpy( MyModName, str );
                init[1] = 1;
            }
  /*2*/     else if( k_its("RingName") ) {
                str = k_str();
                if(str) strcpy( RingName, str );
                init[2] = 1;
            }
  /*3*/     else if( k_its("HeartBeatInt") ) {
                HeartBeatInt = k_int();
                init[3] = 1;
            }

  /*4*/     else if( k_its("MaxSamplePerMsg") ) {
                MaxSamplePerMsg = k_int();
                truelimit = (MAX_TRACEBUF_SIZ - sizeof(TRACE_HEADER))/
										sizeof(long);
                if( MaxSamplePerMsg > truelimit )
                {
                   fprintf( stderr, 
         "rcv_ew_config:  MaxSamplePerMsg <%d> too large; must be <= %d!\n",
                            MaxSamplePerMsg, truelimit );
                   fprintf( stderr, 
         "rcv_ew_config:  change <MaxSamplePerMsg> command in <%s>; exiting!\n",
                            configfile );
                   exit( -1 );
                } 
                init[4] = 1;
            }

  /*5*/     else if( k_its("PacketLatency") ) {
                PacketLatency = k_int();
								fprintf(logout,"Packet Latency=%d\n",PacketLatency);
                if( PacketLatency<MIN_PKTLATENCY || 
										PacketLatency>MAX_PKTLATENCY )
                {
                   fprintf( stderr, 
                    "rcv_ew_config:  Invalid PacketLatency=%d (%d to %d allowed)\n",
                            MIN_PKTLATENCY, MAX_PKTLATENCY );
                   fprintf( stderr, 
                   "rcv_ew_config:  change <PacketLatency> command in <%s>; exiting!\n",
                            configfile );
                   exit( -1 );
                }
                init[5] = 1;
            }

  /*6*/     else if( k_its("MaxSilence") ) {
                MaxSilence = k_long()*60;
                init[6] = 1;
            }

  /*7*/     else if( k_its("AcceptSCN") ) {  
                if( Nscn >= Max_Chan )
                {
                    fprintf( stderr,
                      "rcv_ew_config: Too many <AcceptSCN> commands in <%s>;\n",
                             configfile );
                    fprintf( stderr, 
                     "        limited by command-line-arg <-# %d>; exiting!\n",
												 Max_Chan );
                    exit( -1 );
                }
                str = k_str();   /* read station site code */
                if( strlen(str) < SITE_LEN ) {
                    strcpy( pBuffer[Nscn].site, str );
                } else {
                    fprintf( stderr,
										"rcv_ew_config: error in <AcceptSCN> command in <%s>:\n",
                            configfile );
                    fprintf( stderr,
										"        site code <%s> too long; maxchar=%d; exiting!\n",
                            str, SITE_LEN-1 );
                    exit( -1 );
                };
                str = k_str();   /* read component code */
                if( strlen(str) < COMP_LEN ) {
                    strcpy( pBuffer[Nscn].comp, str );
                } else {
                    fprintf( stderr,
										"rcv_ew_config: error in <AcceptSCN> command in <%s>:\n",
                            configfile );
                    fprintf( stderr,
										"      channel code <%s> too long; maxchar=%d; exiting!\n",
                            str, COMP_LEN-1 );
                    exit( -1 );
                };
                str = k_str();  /* read network code */
                if( strlen(str) < NET_LEN ) {
                    strcpy( pBuffer[Nscn].net, str );
                } else {
                    fprintf( stderr,
										"rcv_ew_config: error in <AcceptSCN> command in <%s>:\n",
                            configfile );
                    fprintf( stderr,
										"       network code <%s> too long; maxchar=%d; exiting!\n",
                            str, NET_LEN-1 );
                    exit( -1 );
                };
                pBuffer[Nscn].pinno = k_int();   /* read pin number */
                if( (pBuffer[Nscn].pinno < 0)    ||
                    (pBuffer[Nscn].pinno > 32767)   )   /* Invalid pin number */
                {  
                    fprintf( stderr,
												"rcv_ew_config: error in <AcceptSCN> command in <%s>:\n",
                            configfile );
                    fprintf( stderr,
												"               pin number <%d> outside allowed range (0-32767); exiting!\n",
                            pBuffer[Nscn].pinno );
                    exit( -1 );
                }  
                Nscn++;
                init[7] = 1;
            }

            else if( k_its("Debug") ) {   /* optional command */
                Debug = 1;
            }
						else if( k_its("ThrottleMS") ) {
							throttleSecs = k_long()*0.001;
							/*fprintf(stderr,"ThrottleSecs=%f\n",throttleSecs);*/
						}
         /* Unknown command
          *****************/ 
	    else {
                fprintf( stderr, 
										"rcv_ew_config: <%s> Unknown command in <%s>.\n", 
                         com, configfile );
                continue;
            }

        /* See if there were any errors processing the command 
         *****************************************************/
            if( k_err() ) {
               fprintf( stderr, 
                   "rcv_ew_config: Bad <%s> command in <%s>; exiting!\n",
                        com, configfile );
               exit( -1 );
            }
	}
	nfiles = k_close();
   }

/* After all files are closed, check init flags for missed commands
 ******************************************************************/
   nmiss = 0;
   for ( i=0; i<ncommand; i++ )  if( !init[i] ) nmiss++;
   if ( nmiss ) {
       fprintf( stderr, "rcv_ew_config: ERROR, no " );
       if ( !init[0] )  fprintf( stderr, "<LogFile> "         );
       if ( !init[1] )  fprintf( stderr, "<MyModuleId> "      );
       if ( !init[2] )  fprintf( stderr, "<RingName> "        );
       if ( !init[3] )  fprintf( stderr, "<HeartBeatInt> "    );
       if ( !init[4] )  fprintf( stderr, "<MaxSamplePerMsg> " );
       if ( !init[5] )  fprintf( stderr, "<PacketLatency> "   );
       if ( !init[6] )  fprintf( stderr, "<MaxSilence> "      );
       if ( !init[7] )  fprintf( stderr, "<AcceptSCN> "       );
       fprintf( stderr, "command(s) in <%s>; exiting!\n", configfile );
       exit( -1 );
   }

   return( 0 );
}
/****************************************************************************
 *  rcv_ew_lookup( )   Look up important info from earthworm.d tables       *
 ****************************************************************************/
void rcv_ew_lookup( void )
{
/* Look up keys to shared memory regions
   *************************************/
   if( ( RingKey = GetKey(RingName) ) == -1 ) {
	fprintf( stderr,
 	        "rcv_ew_lookup:  Invalid ring name <%s>; exiting!\n", 
                 RingName);
	exit( -1 );
   }

/* Look up installations of interest
   *********************************/
   if ( GetLocalInst( &InstId ) != 0 ) {
      fprintf( stderr, 
        "rcv_ew_lookup: error getting local installation id; exiting!\n" );
      exit( -1 );
   }

/* Look up modules of interest
   ***************************/
   if ( GetModId( MyModName, &MyModId ) != 0 ) {
      fprintf( stderr, 
         "rcv_ew_lookup: Invalid module name <%s>; exiting!\n", 
          MyModName );
      exit( -1 );
   }

/* Look up message types of interest
   *********************************/
   if ( GetType( "TYPE_HEARTBEAT", &TypeHeartBeat ) != 0 ) {
      fprintf( stderr, 
         "rcv_ew_lookup: Invalid message type <TYPE_HEARTBEAT>; exiting!\n" );
      exit( -1 );
   }
   if ( GetType( "TYPE_ERROR", &TypeError ) != 0 ) {
      fprintf( stderr, 
         "rcv_ew_lookup: Invalid message type <TYPE_ERROR>; exiting!\n" );
      exit( -1 );
   }
   if ( GetType( "TYPE_TRACEBUF", &TypeTraceBuf ) != 0 ) {
      fprintf( stderr, 
         "rcv_ew_lookup: Invalid message type <TYPE_TRACEBUF>; exiting!\n" );
      exit( -1 );
   }
   return;
} 

/****************************************************************************
 * rcv_ew_error() builds an error message & puts it into shared memory.    *
 *                Also writes errors to log file & screen.                 *
 ***************************************************************************/
void rcv_ew_error( short ierr, char *note )
{	extern FILE* logout;
   MSG_LOGO    logo;
   char	       msg[256];
   long	       size;
   long        t;
 
/* Build the message
 *******************/ 
   logo.instid = InstId;
   logo.mod    = MyModId;
   logo.type   = TypeError;

   time( &t );

   sprintf( msg, "%ld %hd %s\n\0", t, ierr, note);
   logit( "et", "rcv_ew:%s\n", note );
	 fprintf(logout,"%s rcv_ew:%d=%s\n",asctim(),ierr,note);

   size = strlen( msg );   /* don't include the null byte in the message */ 	

/* Write the message to shared memory
 ************************************/
   if( tport_putmsg( &Region, &logo, size, msg ) != PUT_OK )
   {
      logit("et","rcv_ew_error: Error putting error:%d in %s\n", 
             ierr, RingName );
   }

   return;
}


/*****************************************************************
 *                            BigBrother                         *
 *          Watch transport ring for termination flag            *
 *   and monitor time since last packet received for each SCN.   *
 *****************************************************************/

thr_ret BigBrother( void *dummy )
{
   char     msg[128];       /* buffer for writing error messages            */
   time_t   tcurrent;       /* time now (system clock)                      */
   time_t   tsilent;        /* number of seconds since last packet          */
   int  checkaliveint = 60; /* #sec at which to check on well-being of SCN's*/
   int  nsec;               /* number of seconds since last "silent check"  */
   int  i;

   nsec = 0;           
 
   while( 1 )
   {

   /* See if a we've been asked to stop running 
    *******************************************/
      if( tport_getflag(&Region) == TERMINATE  ||
          tport_getflag(&Region) == MyParentPid )
      {
         logit("t", "rcv_ew:BigBrother: Termination requested; exiting!\n" );
         break;  
      }

   /* Check the time of the last data received from each SCN 
    ********************************************************/
      if( MaxSilence > 0  &&  nsec >= checkaliveint )
      { 
         time( &tcurrent );

         for( i=0; i<Nscn; i++ )
         {
            tsilent = tcurrent - pBuffer[i].talive;

            /* Announce that a previously silent SCN is alive again... */
            if( (pBuffer[i].silentlimit >
								MaxSilence) &&					/* We reported a silent interval, */
                (tsilent < MaxSilence)) /* but we've gotten data again    */
            {
               sprintf( msg, " Once again receiving data from %s %s %s.",
                        pBuffer[i].site, pBuffer[i].comp, pBuffer[i].net );
               rcv_ew_error( ERR_ALIVEAGAIN, msg );
               pBuffer[i].silentlimit = MaxSilence;
            }

            /* ...or complain if this SCN has been silent for too long */
            else if( tsilent >= pBuffer[i].silentlimit )
            {
               sprintf( msg, " No data from %s %s %s in %d minutes.",
               pBuffer[i].site, pBuffer[i].comp, pBuffer[i].net, tsilent/60 );
               rcv_ew_error( ERR_SILENT, msg );
               pBuffer[i].silentlimit += MaxSilence;       
            }
         }
         nsec = 0;
      }

      sleep_ew( 1000 );
      nsec++;
   }

   exit(0); /* exit_handler() will be called on exit to clean up! */
}
void log_packet(char tag,  char *net, char *station, char *comp, 
		double dtime, int nsamp, double rate, int seq)
{	time_t looptime;
	int nday;
	char time_str[20];
	struct tm *tmpnt;
	/* open log file if a new day */
	if(pktlogfilename[0] != 0)
	{	looptime=time(NULL);
		tmpnt=gmtime(&looptime);
		nday=tmpnt->tm_yday;
		if(nday != lastyday || logpkt == NULL) {
			
			if(logpkt != NULL) {
				fprintf(logpkt,"%s Close log file=%s logpkt=%d \n",asctim(),
						pktlogfilename,logpkt);
				fclose(logpkt);
			}			
			pktlogfilename[strlen(pktlogfilename)-1]=48+((nday+1)%10);
			fprintf(logpkt,"%s Open2 log file=%s day=%d lastyday=%d logpkt=%d\n",
				asctim(),pktlogfilename,nday,lastyday,logpkt); fflush(logpkt);
			logpkt=fopen(pktlogfilename,"w");
			lastyday=nday;
		}
		date18(dtime+magic_factor(), time_str);
		fprintf(logpkt,"%c%2s%5s%3s %18s %5d %5.1f %3d\n",tag, net, station, comp, 
			time_str,nsamp,rate,seq);
		
	}
}
void log_holding(char *net, char *station, char *comp, 
		double dtime, int nsamp, double rate)
{	
	char time_str[20];
	int yymmdd,ms,i;
	/* open log file if a new day */
	if(holding_host[0] == 0) return;
	date18(dtime+magic_factor(), time_str);
	/* timestr is yyyymmddhhmmss.mmm Decode from ascii */
	yymmdd=0;
	for(i=0; i<8; i++) yymmdd =yymmdd*10 + time_str[i]-'0';
	ms=0;
	ms += (time_str[8]-'0')*36000000;
	ms += (time_str[9]-'0')*3600000;
	ms += (time_str[10]-'0')*600000;
	ms += (time_str[11]-'0')*60000;
	
	/* the date18 routine return 17 char if the second is one digit, shift it*/
	if(strlen(time_str) == 18) {
		ms += (time_str[12]-'0')*10000;
		ms += (time_str[13]-'0')*1000;
		ms += (time_str[15]-'0')*100;
		ms += (time_str[16]-'0')*10;
		ms += (time_str[17]-'0');
	}
	else {
		ms += (time_str[12]-'0')*1000;
		ms += (time_str[14]-'0')*100;
		ms += (time_str[15]-'0')*10;
		ms += (time_str[16]-'0');
		
	}
	/*fprintf(logout,"log_holding %s %s %s %s ns=%d rate=%f\n",
		net,station,comp,time_str,nsamp,rate);*/
	udpholding2(holding_host, holding_port,holding_sort,net,station,comp,
			" ",yymmdd,ms,nsamp,rate);
		

}

char * dasctim(double d) {
	static char tim[20];
	double s;
	long days;
	int hr, min, secs, ms;
	days = d/86400.;

	/* secs since midnight as a double */
	s = d - ((double)days)*86400.;
	
	/* convert to int secs since midnight */
	secs = s;
	ms = (s - (double) secs) *1000. + 0.5;
	hr = secs/3600;
	secs = secs - hr * 3600;
	min = secs / 60;
	secs = secs - min * 60;
	sprintf(tim,"%2.2d:%2.2d:%2.2d.%3.3d", hr, min, secs, ms);
	return &tim[0];
}
	
