#include <stdio.h>
#include <string.h>
#include <time.h>
#include <math.h>
char * asctim();
FILE *logout;
int get_seed(unsigned char , unsigned char node,unsigned char chan,
	 char *name,char *comp, char *network, char * location, double *rate);
int nsn_chan(unsigned char chan, char *comp, double *rate);	
main()
{
	int netid,nodeid,chan;
	int err;
	double rate;
	char name[6],comp[4],network[3],location[3];
	logout=stdout;
	for(;;) {
		printf("Enter Net id : ");  scanf("%d",&netid);
		printf("Enter Node id: ");	scanf("%d",&nodeid);
		printf("Enter chan id: ");	scanf("%d",&chan);
		err=get_seed(netid,nodeid,chan,name,comp,network,location, &rate);
		fprintf(logout,"%d %d %d ",netid,nodeid,chan);;	fflush(logout);
		fprintf(logout,"name=%s ",name);;	fflush(logout);
		fprintf(logout,"comp=%s ",comp);;	fflush(logout);
		fprintf(logout,"network=%s ",network);	fflush(logout);
		fprintf(logout,"loc=%s ",location);	fflush(logout);
		fprintf(logout,"rate=%10.3f\n",rate);
	}
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
#define MAX_SEED_CHAN 2000
	struct nsnstation ch[MAX_SEED_CHAN];
	static time_t lastread=0;
	time_t now;
	int nchan,chn;
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
					 if(failed) fprintf(logout,
						 "%s\nname=%s comp=%s net=%s rt=%d nd=%d ch=%d %10.3f\n",
						 &line[i],name,comp,network,rt,nd,ch, *rate);fflush(logout);
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
	 nchan--;
	 fprintf(logout,"%s Number of nsnstation2.dat recs=%d\n",asctim(),nchan);
	 fclose(in);
 }
 for(i=0; i<nchan; i++) {
	 
	 if( ch[i].net == route && node == ch[i].node && chan == ch[i].chan)
	 {	strcpy(network, ch[i].network);
	 		strcpy(location, ch[i].location);
			strcpy(name, ch[i].station);
			strcpy(comp, ch[i].comp);
			*rate=ch[i].rate;
		 /*fprintf(logout,"%s Fnd %s %s %s %d %d %d %10.3f\n",
			 asctim(),name,comp,network,rt,nd,ch, *rate); fflush(logout);*/
		 return 1;
	 }
 }

	if(rt == route && node == nd && (rt == 1 || rt == 15))
		strcpy(nsnname,name);		/* save NSN station name */



	/* if we get to here, it was not in the file.  Error out */
	if( rt == route && node == nd &&
		(route == 1 || route == 15 ) &&		/* if its an NSN chan*/
		nsn_chan(chan, comp, rate) ==  0)	/* and translate chan o.k.*/
	{	strcpy(name,nsnname);
		strcpy(network,"US");
		fprintf(logout,"%s NSN Fnd %s %s %s %d %d %d %10.3f\n",asctim(),name,comp,
					network,rt,nd,ch, *rate); fflush(logout);
		return 2;
	}
	
 	fprintf(logout,
	"%s *** No Match found for seed name.  infd=%d rt=%d nd=%d ch=%d\n",
		asctim(),in,route,node,chan);
		fprintf(logout,"Turn on debug and try again....\n");
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
