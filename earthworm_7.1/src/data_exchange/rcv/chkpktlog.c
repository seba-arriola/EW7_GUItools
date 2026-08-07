#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>   /* required by getppid() */
#include <unistd.h>      /* required by getppid() */
#include <earthworm.h>
#include <transport.h>
#include <trace_buf.h>
#include <chron3.h>
#include <kom.h>
struct channel {
	char station[6];
	char network[3];
	char comp[4];
	double lastTime;
	int nsamp;
	int lastseq;
	double dt;
	double secsFound;
	double overlap;
	double secsMissing;
	int ngaps;
	int noverlap;
};
#define MAXCHAN 2000
	
main(int argc, char **argv) 
{	char tag;
  char net[3]="";
	char station[6]="";
  char comp[4]=""; 
	char type=0;
	char line[100];
	char d18[19]="";
	double dtime;
	FILE *in;
	char *s;
	int nsamp,  int seq;
	double rate;
	struct channel ch[MAXCHAN];	/* track each channel */
	int ncomp=0;								/* number of ch in use */
	
	for(i = 0; i<argc; i++) {
		if(strcmp(argv[i],"-s") == 0) strcpy(station,argv[i+1]);
		if(strcmp(argv[i],"-n") == 0) strcpy(network,argv[i+1]);
		if(strcmp(argv[i],"-c") == 0) strcpy(comp,argv[i+1]);
		if(strcmp(argv[i],"-t") == 0) type = *argv[i+1];
		
	}
	in = fopen(argv[argc-1],"r");
	if(in == NULL) {
		printf("no such file %s errno=%d %s\n",argv[argc-1],errno, strerror(errno));
		exit(-1);
	}
	s= fgets(line,100,in);
	while (s != NULL) {
		sscanf("%c%2s%5s%3s %18s %5d %5f %3d",itype,nt,st,cm, d18, ns, rt, sq);
		dtime = epoch18(&dtime, d18);
		if( type = 0 || itype == type {
			if(net[0] == 0 || strcmp(net, nt) == 0) {
				if(station[0] == 0 || strcmp(station,st) == 0) {
					if(comp[0] == 0 || strcmp(comp, cm) == 0) {
						ichn=-1;
						for(i = 0; i<ncomp; i++) {
							if(strcmp(ch[i].net, nt) == 0 && strcmp(ch[i].station, st) == 0 &&
								 strcmp(ch[i].comp, cm) == 0 && ch[i].type == itype) {
								ichn=i;
								break;
							}
						}
						if(ichn == -1) {			/* its a new channel */
							ch[ncomp].type = itype;
							strcpy(ch[ncomp].net, nt);
							strcpy(ch[ncomp].station, st);
							strcpy(ch[ncomp].comp, cm);
							ch[ncomp].lastTime = dtime - 1./rate;
							ch[ncomp].nsamp=0;
							ch[ncomp].seq = -1;
							ch[ncomp].dt = 1./rt;
							ch[ncomp].secsFound=0.;
							ch[ncomp].secsMissing=0.;
							ch[ncomp].ngaps=0;
							ch[ncomp].noverlap=0;
							ch[ncomp].overlap=0.;
							ichn=ncomp;
							printf("new chan %d %s %s %s %c at %18s sq=%d\n",ncomp,nt,st,cm,itype,d18,sq);
							ncomp++;
						}

						/* is there a gap ?*/
						gap=dtime - ch[ichn].lastTime - ch[ichn].dt;
						if(gap > ch[ichn].dt/2.) {
							printf("Gap of %5.2f secs in %s %s %s %c at %s\n", 
									(dtime -ch[ichn].lastTime - [ichn].dt), nt,st,cm,itype, d18);
							secsMissing += secsMissing+
							ngaps++;
						} else if(gap < ch[ichn].dt/2.) {
							printf("Overlap of %5.2f secs in %s %s %s %c at %s\n",
									gap, nt,st,cm,itype,d18);
							ch[ichn].overlap += gap;
							ch[ichn].noverlap++;
						}
						if(sq >= 0) {
							if(sq != (ch[ichn].seq+1)% 256)) 
								printf("Seq out of order %d should be %d in %s %s %s %c at %s\n",
									sq, (ch[ichn].seq+1)%256, nt, st,cm,itype,d18);
						}
						ch[ichn].lastTime = dtime + (nsamp-1)*ch[ichn].dt;
						ch[ichn].nsamp = ns;
						ch[ichn].lastseq = sq;
					}
				}
			}
		}
		s = fgets(line,100, in);

	}
	for(i=0; i<ncomps; i++) {
		printf("%s %s %s %c gaps=%d %8.2f secs overlaps=%d %8.2f found=%8.2f\n",
				ch[i].net,ch[i].station, ch[i].comp, ch[i].itype, 
				ch[i].ngaps, ch[i].secsMissing,ch[i].noverlap, ch[i].overlap,
				ch[i].secsFound);
	}
	exit(0);
}	
