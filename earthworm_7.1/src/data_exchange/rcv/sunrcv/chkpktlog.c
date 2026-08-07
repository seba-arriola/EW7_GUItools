#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>   /* required by getppid() */
#include <unistd.h>      /* required by getppid() */
#include <earthworm.h>
#include <transport.h>
#include <trace_buf.h>
#include <errno.h>
#include <chron3.h>
#include <kom.h>
struct channel {
	char station[6];
	char comp[4];
	char type;
	char net[3];
	double lastTime;
	int nsamp;
	int lastseq;
	double dt;
	double secsFound;
	double overlap;
	double secsMissing;
	int ngaps;
	int noverlap;
	int npack;
	int nsecondStream;
};
#define MAXCHAN 2000
	
main(int argc, char **argv) 
{	char tag;
  char net[3]="", nt[3]="";				/* compare network andinput network */
	char station[6]="", st[6]="";		/* compare staiton and input station */
  char comp[4]="", cm[4]=""; 			/* compare component and input component */
	char type=0,itype;							/* compare type and input type */
	char line[100];	
	char d18[19]="";								/* string of input time of packet in EW 18 form */
	double dtime;										/* conversion of d18 to a double per EW */
	FILE *in;												/* input file */
	FILE *logout;
	char *s;
	int ns;
  int seq,sq;
	int i,j,ichn,nline;
	int dbg=0;
	double rate,rt,gap;
	struct channel ch[MAXCHAN],tmp;	/* track each channel */
	int ncomp=0;								/* number of ch in use */
	
	logout=stdout;
	if(argc <= 1) {
		printf("Usage : chkpktlog [-s station][-n network][-c component][-t type char] filename\n");
		exit(0);
	}
	/* parse the command land args - mainly stations/network/compnent/type to use */
	for(i = 0; i<argc; i++) {
		if(strcmp(argv[i],"-s") == 0) strcpy(station,argv[i+1]);
		if(strcmp(argv[i],"-n") == 0) strcpy(net,argv[i+1]);
		if(strcmp(argv[i],"-c") == 0) strcpy(comp,argv[i+1]);
		if(strcmp(argv[i],"-t") == 0) type = *argv[i+1];
		if(strcmp(argv[i],"-dbg") == 0) dbg=1;
		if(strcmp(argv[i],"-o") == 0) {
			logout = fopen(argv[i+1],"w");
		}
		
	}
	
	if(dbg) printf("Matching type=%c nt=%s sta=%s comp=%s\n",type, net,station,comp);
	/* open the file */
	in = fopen(argv[argc-1],"r");
	if(in == NULL) {
		printf("no such file %s errno=%d %s\n",argv[argc-1],errno, strerror(errno));
		exit(-1);
	}
	
	/* preget the first line */
	s=fgets(line,100,in);
	s= fgets(line,100,in);
	nline=0;
	while (s != NULL) {
	/*fprintf(logout,"line=%s\n",line);*/

		itype=line[0];
		strncpy(nt,&line[1],2);
		nt[2]=0;
		strncpy(st,&line[3],5);
		if(st[0] == ' ') {strncpy(st, &st[1],4); st[4]=0;}
		if(st[0] == ' ') {strncpy(st, &st[1],3); st[3]=0;}
		st[5]=0;
		strncpy(cm,&line[8],3);
		cm[3]=0;
		sscanf(&line[12],"%18s %5d %5lf %3d", d18, &ns, &rt, &sq);
		if(d18[0] == ' ') strcpy(d18, &d18[1]);
		epochsec18(&dtime, d18);
		/*if(dbg) fprintf(logout,"    ty=%c nt=%s st=%s cmp=%s d18=%s ns=%d rt=%6.2lf sq=%3d\n",
			itype,nt,st,cm,d18,ns,rt,sq);*/
		
		/* if type, net, station, component match or are null, then process the line */
		if( type == 0 || itype == type) {
			if(net[0] == 0 || strcmp(net, nt) == 0) {
				if(station[0] == 0 || strcmp(station,st) == 0) {
					if(comp[0] == 0 || strcmp(comp, cm) == 0) {
						if(dbg) fprintf(logout,"%s%5s %s%c %s  ns=%4d rt=%6.2lf sq=%3d dtim=%14.3f\n",
							nt,st,cm,itype,d18,ns,rt,sq,dtime);
						
						/* find the structure for this type/net/station/comp  or create it */
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
							ch[ncomp].lastTime = dtime - 1./rt;
							ch[ncomp].nsamp=0;
							ch[ncomp].lastseq = -1;
							ch[ncomp].dt = 1./rt;
							ch[ncomp].secsFound=0.;
							ch[ncomp].secsMissing=0.;
							ch[ncomp].ngaps=0;
							ch[ncomp].noverlap=0;
							ch[ncomp].overlap=0.;
							ch[ncomp].npack=0;
							ch[ncomp].nsecondStream=0;
							ichn=ncomp;
							fprintf(logout,"%s%5s %s%c  new chan %d at %18s sq=%d\n",
									nt,st,cm,itype,ncomp,d18,sq);
							ncomp++;
						}

						/* is there a gap ?*/
						gap=dtime - ch[ichn].lastTime - ch[ichn].dt;
						if(gap > -30000.*ch[ichn].dt) {
							if(gap > ch[ichn].dt/2.) {
								fprintf(logout,"%s%5s %s%c %s Gap of %8.3f secs dtime=%f last=%f\n", 
										nt,st,cm,itype,d18,(dtime -ch[ichn].lastTime -
										ch[ichn].dt),dtime,ch[ichn].lastTime);
								ch[ichn].secsMissing += gap;
								ch[ichn].ngaps++;
							} else if(gap < -ch[ichn].dt/2.) {
								fprintf(logout,"%s%5s %s%c %s Overlap of %8.3f secs dtime=%f last=%f\n",
										nt,st,cm,itype,d18, gap, dtime,ch[ichn].lastTime);
								ch[ichn].overlap += gap;
								ch[ichn].noverlap++;
							}

							/* if this one has a sequence, look for out of sequence */
							if(sq >= 0  && ch[ichn].lastseq >= 0) {
								if(sq != (ch[ichn].lastseq+1)% 256) {
									fprintf(logout,"%s%5s %s%c %s Seq out of order %d should be %d \n",
										nt, st,cm,itype,d18, sq, (ch[ichn].lastseq+1)%256);
								}	

							}
							/* store data for next time this channel comes up - time, nsamp and sequence*/
							ch[ichn].lastTime = dtime + (ns-1)*ch[ichn].dt;
							ch[ichn].nsamp = ns;
							ch[ichn].lastseq = sq;
							ch[ichn].npack++;
							ch[ichn].secsFound += ns*ch[ichn].dt;
						}
						else {
							fprintf(logout,"%s%5s %s%c %s %d Second Stream discard %f\n",
											nt, st,cm,itype,d18, sq, gap );
							ch[ichn].nsecondStream++;
						}
					}

				}
			}
		}
		s = fgets(line,100, in);
		nline++;
	}
	for (i=0; i<ncomp-1; i++) {
		for(j=i+1; j<ncomp; j++) {
			if(memcmp(ch[i].station, ch[j].station, 14) > 0) {
				tmp = ch[i];
				ch[i]=ch[j];
				ch[j]=tmp;
			}
		}
	}
	for(i=0; i<ncomp; i++) {
		fprintf(logout,"%s%5s %s%c  2nd=%3d npk=%5d gaps=%3d%9.2f ovr=%3d %9.2f fnd=%6.0f\n",
			ch[i].net,ch[i].station, ch[i].comp, ch[i].type, ch[i].nsecondStream,ch[i].npack,
			ch[i].ngaps, ch[i].secsMissing,ch[i].noverlap, ch[i].overlap,
			ch[i].secsFound);
	}
	exit(0);

}
