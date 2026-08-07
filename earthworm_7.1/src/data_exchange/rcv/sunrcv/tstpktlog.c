#define MAX_DATA 4096				/*largest # of points that can be decompressed*/
#include <stdio.h>
#include <errno.h>
#include <time.h>
#include <memory.h>
#include <errno.h>
#include <signal.h>
#include <strings.h>
#include <stdlib.h>


char *asctim();						/* UTC time string prototype */
char * logptr;     /* pointer to translation of EW_LOG if available     */
FILE * logpkt;		 /* if packet_log is true, unit where details go      */
FILE * logout;
int leapnext=0;
char *asctim();
double sec_since_1970(int, int, int, int, int, int);
void log_packet(char ,  char *, char *,char *,double , int , double,int);/* Things to read or derive from Earthworm configuration file
*/
	main (int argc, char *argv[])

{
	 struct tm *tmpnt;
	 time_t looptime;
  extern FILE *logout;
	extern int packet_log;
	extern char pktlogfilename[];
   unsigned stackSize;     /* Stack size of heartbeat threads   */
   char *config_file;
   int i;


	char net[3]="NT";
	char station[6]="TEST%";
	char comp[4]="BHZ";
	double dtime;
	int nsamp=4000;
	double rate=40.;
	int seq;
	logout=stdout;

/* Process the command line arguments, ignoring arguments we don't need
 **********************************************************************/
   for (i=0; i<argc; i++) /* for each command line argument */
   {
      fprintf(logout,"user_proc_cmd: %d arg=%s\n", i,argv[i]); /*DEBUG*/

 
			if(strcmp(argv[i],"-o") == 0) 
			{
				 logptr=getenv("EW_LOG");
				 fprintf(logout,"ew_log=%s\n",logptr);
         if(logptr == NULL) strcpy(pktlogfilename,argv[i+1]);
         else
         { strcpy(pktlogfilename,logptr);
           strcat(pktlogfilename,argv[i+1]);
         }
				 pktlogfilename[strlen(pktlogfilename)-4] = 0;
				 strcat(pktlogfilename,"pkta");
				 fprintf(logout,"filename=%s\n",pktlogfilename);

			}
			if(strcmp(argv[i],"-packet_log") == 0)
			{	fprintf(logout,"Pack_log\n"); fflush(logout);
				looptime=time(NULL);
			  packet_log=1;
				fprintf(logout,"packet_log=%d filename=%s\n",packet_log,pktlogfilename);
				if(pktlogfilename[0] != 0)
				{	tmpnt=gmtime(&looptime);
					pktlogfilename[strlen(pktlogfilename)-1]=48+((tmpnt->tm_yday+1) % 10);
					logpkt=fopen(pktlogfilename,"a+");
					fprintf(logpkt,"%s Open log file=%s\n",asctim(),pktlogfilename);
				}
			}
			
   }
	for(i=0; i<20; i++) 
	{	dtime = sec_since_1970(2004, 257, 10, 11+i, 12, 999);
		fprintf(logout,"i=%d dtime=%f seq=%d\n",i,dtime,seq);
		log_packet('#', net, station,comp,dtime,nsamp,rate, seq++);
	}	
		exit(1);
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
