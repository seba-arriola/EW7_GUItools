/*
	Station.c - This image is opened by a RCV routine via a fork and execl.  Its
	standard input is set to be the pipe from RCV and through this pipe exactly one
	stations data should flow.  This station might have multiple channels.  SEED
	information is in station nsnstation2.dat.  This file comes with the release
	but can be updated using the autodrm by sending a mail message to 
	autodrm@gldfs.cr.usgs.gov with the body :
	
	BEGIN
	CLIST
	STOP

	The returned mail message should be saved as nsnstation2.dat


	command line arguments
	-dbg		Turn on Debug output

	-stationlog		Output to logout goes to "station.log?" file instead of 
					stdout.  If a non-multiple site, the logs will be named
					STN.LOG?.

	-partial	User_proc is called with incremental updates on "partial" update
				packets.  The default behavior is to wait for a complete NSN
				packet to come in before submitting to USER_PROC.  This allows
				NSN LH* channels to be processed every two minutes or so instead
				of about once per 20-40 minutes.
	-# Nchan	Set Multiple station mode with maximum of Nchan channels

	At startup we have to accept whatever channel sequence comes first for each 
	channel.  After the channel is first received we look for sequencing errors
	on the channel and report them.  We know that rollback code in RCV has gotten
	us as good a set of data as can be had.

	Much of this code has to do with "Partial update" records.  These are short 
	packets which contain incremental parts of a longer packet.  Mostly these are
	used with 1 sps long period data.  Since a full 2048 byte compresed record 
	contains as much as 45 minutes of data, it was perceived as desirable to send
	incremental pieces of this longer packet at shorter intervals (about every 2
	minutes).  Each partial packet sends the chunk of data that has been created
	since the last partial.  This data is put in the GBPRT buffers.  These buffers
	eventually contain the whole packet (as noted by NSN_FLAGS_EOF flag) and are
	processed.  The partially build packets can be decompressed at any point and
	return the data that has accumulated so far however.  At the NSN the partials
	keep getting rewritten over and over until a EOR says start writting in the
	next block.

	The user supplied routine is called with the time, and in the clear data.
	The user needs to add routines to process the data, give it SEED channel
	designations, etc.

	NSN and IRIS codes use get_name() to get the seed names.  To obtain the most
	up to date list of these references look in the FTP area on GLDFS.CR.USGS.GOV
	under directory RCV.  If a station is not in there, send mail to Dave ketchum
	ketchum@gldfs.cr.usgs.gov to get it updated.  The file is NSNSTATION.DAT.
	
	This program is normally invoked from RCV via an execv call with the same
	command line arguments (arg[0] changed to station).  The -s option is passed
	through RCV and sets this program to use a log file based on the station name.
	
	Modifications :
	Mar 1996 	Base functionality
	Jun 1996 	Add TCP/IP as input source.  Add Command line interface.
	Jul 1996 	Add conditional code for ANSI C
	Apr 1997	Add partial update calls to USER_PROC based on -partial switch
	Oct 1997	Change to use channel struct, support multiple stations/process
	Feb 1998	Change so that overflowing channel list is not fatal but uses
				the last channel structure for such channels.
	Feb 1998	Added "heart beat" function. See description in RCV.

	D.C. Ketchum Mar 1996

*/
#define MAX_DATA 4096				/*largest # of points that can be decompressed*/
#include <stdio.h>
#include <errno.h>
#include <time.h>
#include <memory.h>
#include <errno.h>
#include <signal.h>
#include "rcv.h"
#include "rcvqsub.h"				/* function prototypes for time code subs */
#include "steim.h"
#include "steimlib.h"
#include "exit_handler.h"
int max_chan=12;					/* can be changed on cmd line, # of chan*/
int node;							/* set to node number after first packet*/
int route;							/* set to route number after 1st packet */
int leapnext;						/* gbl leapnext,not used in non-field code*/
int dbg=0;							/* if true, debug output on */
int logflg=0;						/* gbl flag to use log files */
int partupd=0;						/* if <> 0, user asked for partial updates*/
int multiple=0;						/* if user does not set chan, 1 stationflg*/
FILE * logout;						/* log file descriptor */
char logfilename[80];				/* log file name */
char logpathname[50];				/* path name, used mainly by EW */

char *asctim();						/* UTC time string prototype */

#ifdef __STDC__
	main (int argc, char *argv[])
#else
    main(argc,argv)
	int argc;
 	char *argv[];
#endif
{
	struct chan_def 
	{	char name[6];			/* Seed station name */
		char cname[4];			/* Seed channel name */
		char network[3];		/* Seed Network name */
		char location[3];		/* Seed Location name */
		long icsteim[2];		/* storage for STEIM integration constants*/
		long ia0;				/* NSN style integration constant */
		long chan_mask;			/* 3 byte chan ID route, node, stat/chan*/
		int lastseq;			/* last sequence received on this channel*/
		int lastupd;			/* partial updates, last offset */
		int prtstart;			/* flag used in partial updates */
		double rate;			/* digitizing rate */
		struct gomberg *gbprt;	/* if partial update, point to memory buf */
	};
	struct chan_def *ch;		/* array of pointers to the channel */

/*
	Variables and flags 
*/
	long new_mask;					/* contains triplet for current pkt*/
	struct gomberg gb;			/* space to put and NSN packet */
	int partial;						/* set true if partial update */
	int eor;								/* set true if partial mode & eor */
	int err;								/* error returns from functions */
	int nbytes;							/* set to packet len each packet */
	int det;								/* to detection sequence */
	long iy,id,ih,im,is,ms,leap;		/* time year, day, hr, min, sec,ms*/
/*
	Steim Decompression related variables.
*/
	short sdcmp;						/* Steim Number of samples returned */
	short final;						/* ptr to data[] with last integrationcons*/
	short recursion=0;			/* always zero for steim */
	short flip=0;						/* indicates whether to use native endianness*/
	short ignore=0;					/* number of IC to ignore in mid frame */
	short statcode;					/* returned with status of compression */
	short level;						/* compression level (1,2, or 3) */
	int dpnt;								/* point to output buffer idat for decomp data */
	int ipnt;								/* pointer into frames in gomberg packet */
	long unpacked[300];			/* temp to unpack data from */
	
/*
	NSN compression related variables 
*/
	long i4sav;
	int eod;								/* decompress sets true if end reached*/
	int	ovr;								/* dcmprs sets if idat overflwd*/
	int ndcmp,ndsav;				/* tracks size of uncompressed idat */
	int eof;								/* used in call to user_proc to indicate end of WF*/
	int ierr;								/* error return from DCMPRS */
/*
	Other variables
*/
	char filename[100];			/* filename of log file */
	int discon;							/* if true, discontinuity found */
	long idat[MAX_DATA];					/* buffer for decompressed data */
	int inhibit;						/* true if inhibit bit set in numbyt*/
	int route=-1;						/* startup state */
	int nsamp;							/* Number of samples decompressed */
	int ioff,nb,iend,nz,new;/* temp variables */
	int i,j;
	struct nsntime tc,tc2;	/* a correct NSN structure */
	int ich;								/* pnt to current chan idx*/
	time_t now;							/*  time for log file open/close*/
	struct tm *tmpnt;				/* parts of time from GMTIME */
	int lastyday;						/* track for log file changes */
/*
	Initialize variables
*/
	logfilename[0]=0;						/* no log file unless on cmd line*/
	logpathname[0]=0;						/* no log file unless on cmd line*/
	logout=stdout;							/* at first log to terminal */
	cmdarg(argc,argv);					/* mainly get max_chan, debug vars*/
	exit_handler_init("STATION");
	if(logfilename[0] != 0) {
		now=time(NULL);						/* current time */
		tmpnt=gmtime(&now);				/* convert to parts */
		logfilename[strlen(logfilename)-1]=48+((tmpnt->tm_yday+1)%10);
		fprintf(logout,"%s Open initial log file =%s\n",asctim(),logfilename);
		logout=fopen(logfilename,"a+");/* open initial log file */
		fprintf(logout,"\n\n%s Open initial log file2 =%s\n",asctim(),logfilename);
		lastyday=tmpnt->tm_yday;
	}
/*
	Allocate memory based on max_chan and initialize channel structures
*/
	if( (ch=(struct chan_def *) malloc(max_chan*sizeof(struct chan_def)))
		 == NULL)
	{	fprintf(logout,"MALLOC for max_chan=%d channels failed!",max_chan);
		exit(101);
	}
	memset(ch,0,max_chan*sizeof(struct chan_def));	/* zero channel memory*/
	for(i=0; i<max_chan; i++) 			/* for each channel */
	{	ch[i].lastupd=0;							/* partial updates, last offset sent */
		ch[i].ia0=2123740000;					/* set startup integration constant */
		ch[i].icsteim[1]=212374000;		/* ditto for steim */
		ch[i].chan_mask=-1;						/* channel is not in use */
	}
	fprintf(logout,"Partial Update userproc=%d\n",partupd);
/*
	The infinite loop that gets data and processes it 
*/
for (;;) 										/* forever please */
{	errno=0;		
	err=read(0,(char *) &gb,8);					/* get first 8 bytes */
	if(err != 8) {
		fprintf(logout,"%s %s Station : rt=%d nd=%d bad read 8. err=%d errno=%x\n",
			asctim(),ch[ich].name,route,node,err,errno);	
		perror("Station : read 8 error");
		exit(1);								/* we better never get here */
	}
	if(dbg) fprintf(logout,"%s start 1\n",asctim() ); fflush(logout);
	nbytes=(gb.numbyt[1] & 7)*256+gb.numbyt[0];	/* how many bytes in packet */
	if(nbytes == 8 && gb.routeid == 1)
	{	user_heart_beat();						/* user written heartbeat EW */
		continue;								/* go to bottom of for loop*/
	}
	inhibit = (gb.numbyt[1] & 0x80) ? 1 : 0;	/* set inhibit state */
	det=gb.detseq[0]+gb.detseq[1]*256;
	if( (nbytes-8) > 0) 
		err=read(0,(char *) &gb.tc[0],nbytes-8);	/* read rest of packet */
	if(err <= 0) {
		fprintf(logout,"%s Station : rt=%d nd=%d bad read rest. err=%d errno=%x\n",
			asctim(),route,node,err,errno);	
		perror("Station : read 8 error");
	}
	user_heart_beat();
	tc.iyr=gb.tc[0];							/* set day of year */
	tc.doy=gb.tc[1];
	if(dbg) fprintf(logout,"start 2\n"); fflush(logout);
	memcpy((char *) &tc.ms,(char *) &gb.tc[2],4);/* move time code ms to struct */
 
	nsnint(tc,&iy,&id,&ih,&im,&is,&ms,&leap);
/*	printf(
"%s %s %4d %3d %2d:%2d:%2d.%3d nb=%d rt=%d nd=%d ch=%d cseq=%d pseq=%d det=%d frm=%d\n",
		name,cname,iy,id,ih,im,is,ms,
		nbytes,gb.routeid,gb.nodeid,gb.chanid,gb.seq,gb.packseq,det,gb.format);*/
/*
	Got a whole packet. Process it.
	This would be a great place to convert routes and nodes to SEED station's name!
	and to convert the gb.chan byte into a SEED channel name.
*/
	if(route == -1) {
		route=gb.routeid;
		node=gb.nodeid;
	}
	if(dbg) fprintf(logout,"start 3\n"); fflush(logout);
	discon=0;									/* assume no sequence error */
	new_mask=0;								/* clear mask bits */
	memcpy(&new_mask,&gb.routeid,3);		/* put the triplet in the mask */
	if(gb.chanid == 0 || gb.lead1 == 0) {
		if(gb.chanid == 0) status_pack(&gb);
		if(gb.lead1 == 0) fprintf(logout,"%s Multiple startup packet!\n",asctim());
	} else {
		new=-1;
		for(i=0; i<max_chan; i++) {
			if(new == -1 && ch[i].chan_mask == -1) 
				{new=i; break;}				/* no such chan*/
			if(ch[i].chan_mask == new_mask) break;
		}
		if(i == max_chan) {					/* are we out of channels */
			fprintf(logout,"%s We are out of channels.  Max=%d %s %s \n",
				asctim(),max_chan,ch[ich].name,ch[ich].cname);
			i=max_chan-1;					/* use last one for all garbage*/
		}
		if(i == new) {						/* is this a new channel ? */
			ch[i].chan_mask=new_mask;		/* set mask for this channel */
			ch[i].lastseq=gb.seq;			/* set initial sequence number */
			if( get_seed(gb.routeid,gb.nodeid,gb.chanid,
				ch[i].name,ch[i].cname,ch[i].network,ch[i].location,
				&ch[i].rate) <= 0) 
				fprintf(logout,"%s *** Seed name not found rt=%d node=%d ch=%d %s %s\n",
						asctim(),gb.routeid,gb.nodeid,gb.chanid,ch[i].name,ch[i].cname);
			if(dbg)fprintf(logout,
				"%s %s %s %s New rt=%d nd=%d ch=%d lseq=%d ich=%d ia0=%d\n",	
				asctim(),ch[i].name,ch[i].cname,ch[i].network,
				gb.routeid,gb.nodeid,gb.chanid,gb.seq,i,ch[i].ia0);
			if(multiple == 0)
			{	strcpy(logfilename,logpathname);
				strcat(logfilename,ch[i].name);/* station name */
				for(j=0; j<50; j++) if(logfilename[j] == ' ') 
					logfilename[j]=0;	
				strcat(logfilename,".LOGA");	/* add .log to it*/
				fprintf(logout,"%s multiple=%d set log file name=%s\n",
					asctim(),multiple,logfilename);
				lastyday=0;					/* force new log file open */
			}

		} else {							/* If not new! */
			if(dbg) fprintf(logout,"start 4\n"); fflush(logout);
			fflush(logout);
			if( (gb.seq % 16) == 1) get_seed(gb.routeid,gb.nodeid,gb.chanid,
				ch[i].name,ch[i].cname,ch[i].network,ch[i].location,
				&ch[i].rate);
			if(ch[i].lastseq != -1) 		/* if -1 expecting new sequence */
			{	if(gb.format == 0 && (gb.flags & NSN_FLAGS_PARTIAL) !=0){/*Part upd*/
					if( (ch[i].lastseq+1) % 256 != gb.seq &&/* is it next one */
						ch[i].lastseq != gb.seq) {			/* or same one */
						fprintf(logout,
							"%s %s %s *** LP chan=%d lseq=%d got=%d Out of chan seq\n",
							asctim(),ch[i].name,ch[i].cname,gb.chanid,ch[i].lastseq,gb.seq);
						discon=1;				/* mark discontinuous */
						ch[i].ia0=2123740000;	/* not cont, user action??? */
						ch[i].icsteim[1]=2123740000;/* not cont, set steim no IA0 chk*/
					}
				} else {							/* not a partial update */
					if( ((ch[i].lastseq+1) % 256) != gb.seq) {
					/* out of seq, but that is expected from NSNCOMM on EOF */
						if((gb.format == FORM_STEIM || 
							gb.format == FORM_STEIM2 ||
							gb.format == FORM_STEIM3) && gb.detseq[0] == 1 &&
							ch[i].lastseq > 1 ) 	{	/* fake eof on old */
							gb.flags |=NSN_FLAGS_EOF;	/* NSNCOM versions */
							fprintf(logout,"%s Override to EOF\n",asctim());
						}
						if((gb.format == FORM_STEIM || gb.format == FORM_STEIM2 ||
						gb.format == FORM_STEIM3) && (gb.flags & NSN_FLAGS_EOF) != 0) 
							ch[i].lastseq=-1;		/* new seq expected on chan */
						else {		/* on ANSS this is not interesing in 2 stream mode */
							/*fprintf(logout,
								"%s %s %s STM rt=%d nd=%d ch=%d lseq=%d got=%d Out of chan seq.\n",
								asctim(),ch[i].name,ch[i].cname,gb.routeid,gb.nodeid,
								gb.chanid,ch[i].lastseq,gb.seq);*/
							ch[i].icsteim[1] = 212374000;/* not continuous, user?? */
							ch[i].ia0 =2123740000;	/* not continuous, user action? */
							discon=1;
						}
					}
				}
				ch[i].lastseq=gb.seq;			/* set last seq for next time */
			} else {							/* expect new seq (after eof) */
				ch[i].icsteim[1] = 212374000;	/* not continuous, user?? */
				ch[i].ia0=2123740000;			/* not cont, user action??? */
				ch[i].lastseq=gb.seq;			/* accept whatever comes */
				if(gb.seq != 1)
				fprintf(logout,
				"%s %s *** New seq did not start at 1. Found=%d  rt=%d nd=%d ch=%d\n",
				ch[i].name,ch[i].cname,gb.seq,gb.routeid,gb.nodeid,gb.chanid);	/* report when its not one */
			}
		}
		if(dbg) fprintf(logout,"start 5\n"); fflush(logout);
		ich=i;								/* ich will point to this channel */

		now=time(NULL);					/* current time */
		tmpnt=gmtime(&now);				/* convert to parts */
		if(tmpnt->tm_yday != lastyday && logfilename[0] != 0)
		{	fprintf(logout,"%s Close2 log file %s\n",asctim(),logfilename);
			logfilename[strlen(logfilename)-1]=48+((tmpnt->tm_yday+1)%10);
			fclose(logout);					/* close old log file */
			logout=fopen(logfilename,"w");	/* open log file name */
			fprintf(logout,"%s Open2 log file %s day=%d\n",
				asctim(),logfilename,tmpnt->tm_yday);
			lastyday=tmpnt->tm_yday;	/* save to check next rollover*/
		}

/***********************************************************8
	IRIS/STEIM data packet 
Iris style stations use 6 streams of up to 6 channels each.  Tehy are encoded
in the station channel byte as stream*32+channel whre stream and channel numbers
both start at zero.  The streams are :

0 BB 20 Hz or 40 Hz depending on processor type
1 80 Hz
2 Not known
3 40 Hz BB High Gain Channels 1-3
3 40 Hz BB low gains in channels 4-6
4 1 Hz BB long period

Iris stations deleiver between 3 and 5 Steim compression blockettes in each
packet.  Further, they send an empty packet with the EOF bit turned on for
each channel at the end of a trigger.  LP data compes continuously from most
sites.
Note: EOF records from IRIS come in with entirely zeroed packets and must have 
the form changed to form_steim so the NSN code does not try to pick them up.
************************************************************/
		if(gb.format == 0 && gb.flags == 1 && gb.doy == 0 && gb.detseq[0] == 0 &&
		gb.detseq[1] == 0 && gb.seq == 0) gb.format=FORM_STEIM;/* IRIS eof packet?*/
		level=0;								/* assume no level */
		switch (gb.format) {					/* Choose based on packet format*/
		case FORM_STEIM:						/* Steim I compression */
			level=1;
		case FORM_STEIM2:						/* Steim II compression */
		case FORM_STEIM2A:
			if(level == 0) level=2;				/* set level to Steim II */
		case FORM_STEIM3:						/* Steim III compression */
			if(level == 0) level=3;				/* set level to steim III */
			if(dbg) fprintf(logout,"%s %s nb=%d frm=%d rev=%d off=%d srt=%d soh=%x ns=%d ctrl=%d \n",
				ch[ich].name,ch[ich].cname,nbytes,gb.format,gb.u.sti.revision,
				gb.u.sti.offset,gb.u.sti.samprate,gb.u.sti.soh,
				gb.u.sti.nsamp,gb.u.sti.control);
			nsamp=gb.u.sti.nsamp;					/* get number of samples */
			eof=0;
			if( (gb.flags && NSN_FLAGS_EOF) != 0) 	/* is this an EOD */
			{	ch[ich].icsteim[1]=212374000;		/* new for decompress */
				ch[ich].ia0=212374000;				/* no IA0 expected */
				if(dbg) fprintf(logout,"%s %s Steim EOF on channel =%d \n",
					ch[ich].name,ch[ich].cname,gb.chanid);
				eof=1;								/* set eof for user_proc */
				user_proc(iy,id,ih,im,is,ms,leap,idat,0,	/* eof */
					ch[ich].name,ch[ich].cname,ch[ich].network,eof,0.,-1);
			}
			else 								/* data packet */
			{	ipnt=0;
				dpnt=0;
				if(ch[ich].icsteim[1] == 212374000) 
				{
					sdcmp=0;
				} else 
				{	sdcmp=1;
					unpacked[1]=ch[ich].icsteim[0];	/*last words from lst time*/
					unpacked[2]=ch[ich].icsteim[1];	/* set other word */
				}
				memset(idat,0,MAX_DATA*sizeof(long));/* zero buffer */
				while (ipnt < nbytes-28) 
				{	sdcmp=decompress_frame(
						(compressed_frame *)&gb.u.sti.data[ipnt],
						unpacked,&final,sdcmp,level,recursion,flip,ignore,
						&statcode);
					memcpy((char *) &idat[dpnt],(char *) &unpacked[2],
							sdcmp*sizeof(long));
					if(dbg) {
						fprintf(logout,
			"Steim dcon=%d final=%d level=%d rec=%d flip=%d ign=%d stat=%d ipnt=%d dpnt=%d\n",
						sdcmp,final,level,recursion,flip,ignore,statcode,ipnt,dpnt);
						fprintf(logout,"data(0,1)=%d %d  end=%d %d %d\n",unpacked[0],unpacked[1],
						unpacked[sdcmp-1],unpacked[sdcmp],unpacked[sdcmp+1]);
					}


					if(dferrorfatal(statcode,stdout)) 	/* fatal errors?? */
					{	fprintf(logout,
							"%s %s *** Decompress_frame reports fatal error!\n",
							ch[ich].name,ch[i].cname);
						break;
					}
					dpnt+=sdcmp;			/* where do we put next data */
					ipnt+=64;				/* increment to next frame to process*/

				}
				ch[ich].icsteim[0]=unpacked[sdcmp];
				ch[ich].icsteim[1]=unpacked[sdcmp+1];/* save IC for next time */
				if(dbg) fprintf(logout,
					"%s %s Steim dcmprs: ns=%d dpnt=%d ic=%d %d\n",
					ch[ich].name,ch[ich].cname,nsamp,dpnt,
					ch[ich].icsteim[0],ch[ich].icsteim[1]);
				if(dpnt != nsamp) fprintf(logout,
					"%s %s *** # smp dcmprss err ns=%d dpnt=%d\n",
					ch[ich].name,ch[ich].cname,nsamp,dpnt);
				if(gb.u.sti.samprate > 0) ch[ich].rate=gb.u.sti.samprate;
				else ch[ich].rate=1./(-gb.u.sti.samprate);
				user_proc(iy,id,ih,im,is,ms,leap,idat,dpnt,ch[ich].name,
					ch[ich].cname,ch[ich].network,eof,ch[ich].rate,gb.seq);
			}
			break;								/* leave case */
/*************************************************************

	NSN_FLAGS_PARTIAL	If set in flags byte, then this is a partial update record.
						The number of new bytes and offset in the data buffer are
						stored at the beginning of the data section.

	NSN_FLAGS_EOR		On partial records if this bit is set, this is the last
						partial buffer making up the full buffer.  It indicates
						that after the update, the buffers is ready to process.

	chanid	Description
	0-2		80 Hz Broad Band Data Z, N, E (large amplitude events normally)
	3-5		40 Hz Broad Band Data Z, N, E (normally triggered stream)
	6-8		20 Hz Broad Band Data Z, N, E( Not normally used at NSN)
	9-11	10 Hz Broad Band Data Z, N, E (not normally used at NSN)
	12-14	1 Hz  Broad Band Data Z, N, E (Partial updates possible)
	15-17	40 Hz Broad Band Data Z, N, E (continuous stream)
	18-21	20 Hz Broad Band Data Z, N, E (continuous stream)

Other channel codings are possible.  Please see the appendix of the
"USNSN Satellite Primer" for more details.
******************************************************************/
		case FORM_NSN:
		if(dbg) fprintf(logout,
"%s %s %s %4d %3d %2d:%2d:%2d.%3d nb=%d rt=%d nd=%d ch=%d cseq=%d pseq=%d det=%d frm=%d\n",
		asctim(),ch[ich].name,ch[ich].cname,iy,id,ih,im,is,ms,
		nbytes,gb.routeid,gb.nodeid,gb.chanid,gb.seq,gb.packseq,det,gb.format);
			partial=((gb.flags & NSN_FLAGS_PARTIAL) != 0) ? 1 :0;/* set partl flg*/
			if(partial) 						/* is this a partial update? */
			{	if(ch[ich].gbprt == NULL) 
				{	ch[ich].gbprt=
						(struct gomberg *) malloc(sizeof(struct gomberg));
				}
				if(discon) ch[ich].prtstart=0;	/*discontinuity start up again*/
				ioff=gb.u.data[2]+gb.u.data[3]*256;	/* set offset to put data at */
				nb=nbytes-4-14-6;						/* how much new data is there */
				if(ch[ich].prtstart == 0) 	/* do we expect to be at start? */
				{	ch[ich].ia0=2123740000;		/* probably not continuous now */
					if(ioff != 0)  
					{	fprintf(logout,
					"%s %s %s Partial Wait for 0. Discard.. ioff=%d sq=%d nb=%d\n",
							asctim(),ch[ich].name,ch[ich].cname,ioff,gb.seq,nb);
						break;									/* can't process go to end */
					}
				} else 
				{	if(ch[ich].prtstart < ioff) /* we are missing some data */
					{	fprintf(logout,
							"%s %s %s *** Missing section! ioff=%d expect %d \n",
							asctim(),ch[ich].name,ch[ich].cname,ioff,ch[ich].prtstart);
						ch[ich].ia0=2123740000;	/* probably not continuous now */
						ch[ich].prtstart=0;			/* Need to discard till offset zero */
						break;									/* no processing this packet */
					}
				}
				ch[ich].prtstart=ioff+nb-1;		/* what do we expect next */
				if( (gb.flags & NSN_FLAGS_EOR) != 0) eor=1;/* end of record */
				else eor=0;
				if(dbg) fprintf(logout,
					"%s %s %s prtl upd ioff=%d sq=%d nb=%d ch=%d flg=%x eor=%d\n",
					asctim(),ch[ich].name,ch[ich].cname,ioff,gb.seq,nb,
					gb.chanid,gb.flags,eor);
				memcpy((char *)&ch[ich].gbprt->lead1,(char *)&gb.lead1,20);/*upd hdr prt*/
				if(ioff < 0 || ioff > 2038) 
				{	fprintf(logout,
						"%s %s %s *** Prtial update bad ioff=%4d eor=%d\n",
						asctim(),ch[ich].name,ch[ich].cname,ch[ich].network,
						ioff,eor);
				} else 
				{	if(nb > 0) memcpy((char *) &(ch[ich].gbprt->u.data[ioff]),
									  (char *) &gb.u.data[4],nb);
					ch[ich].gbprt->u.data[ioff+nb]=0;	/* zero next byte */
					memcpy((char *) &(ch[ich].gbprt->u.data[4]),
						   (char *) &gb.u.data[0],2);		/* mv nsamps */
					ch[ich].gbprt->numbyt[0]=(ioff+nb+20) % 256;/* len of packet now */
					ch[ich].gbprt->numbyt[1]=(ioff+nb+20) / 256;/* len of packet now */
				}
				if(eor == 0 && partupd == 0) break;	/* whole packet not in yet*/

				if(eor) ch[ich].prtstart=0;	/* next should be beginning of pkt*/
				nbytes=ioff+nb+20;
				memcpy( (void *) &gb, (void *) ch[ich].gbprt, 
					ioff+nb+20);			/* move prtl pkt to GB*/
											/* partial pkt could be written*/
											/* or decompressed here if the users */
											/* needs near real time access to LP */
			}								/* not a partial, normal packet */
/*
	The nsnpacket in GB is a complete one (either a non-partial update or the
	last partial packet for this packet.  Handle End-of-File condition and 
	decompress.
*/
			iend=nbytes-20;					/* number of data bytes in gb.data*/
			nz=2028-iend;						/* number of bytes to zero */
			if( (gb.flags & NSN_FLAGS_EOF) != 0) 
			{	memcpy((char *) &i4sav,(char *) &gb.u.data[iend-4],4);/* save last 4 bytes */
				memset((char *) &gb.u.data[iend-4],0,4);	/* clear to end */
			}
			if(nz > 0) memset((char *) &gb.u.data[iend],0,nz);/* clear to end */
			eof=0;
			if( (gb.flags & NSN_FLAGS_EOF) != 0) 	/* move to end of buffer */
			{	memcpy((char *)&gb.u.data[2014],(char *)&i4sav,4);/*Rev IC@end*/
				eof=1;										/* pass eof to user_proc */
				ch[ich].lastseq=-1;				/* new seq expected on channel*/
			}
			/* tell ndcmprs to use IA0 as 1st sample instead 1st difference */
			if(ch[ich].ia0 == 2123740000) ndcmp=-1;
			else 
			{	ndcmp=0;
				idat[0]=ch[ich].ia0;
			}
			memset(&idat[1],0,(MAX_DATA-1)*sizeof(long));/* zero data array */
			ndsav=ndcmp;
/*			fprintf(logout,"%s %s ich=%d ndcmp=%d ia0=%d %d\n",
					ch[ich].name,ch[ich].cname,ich,ndcmp,ch[ich].ia0,idat[0]);*/
			ierr=dcmprs(MAX_DATA,&ndcmp,idat,&eod,&ovr,&gb.u.data[0]);

			if(err != 1) dcmper(ierr);		/* print message on error */
/*
	If user want partial updates and this is partial data, fix the time
	etc. so user_proc gets what it needs.
*/
			if(dbg || ovr || eod || ierr != 1) 
				fprintf(logout,
			"%s %s %s ierr=%d nd=%d eod=%d ovr=%d ia0=%d ldat=%d %d dat=%d %d %d %d %d\n",
				asctim(),ch[ich].name,ch[ich].cname,ierr,ndcmp,eod,ovr,
				ch[ich].ia0,idat[ndcmp],idat[ndcmp+1],
				idat[0],idat[1],idat[2],idat[3],idat[4]);
			if(partial && partupd) 
			{	tc2=nsnadd(tc,ch[ich].lastupd*1000/ch[ich].rate);/* time of sample */
				nsnint(tc2,&iy,&id,&ih,&im,&is,&ms,&leap);/* break up new time */
				if(gb.seq == 1 && ndsav == -1) ndcmp++;
				user_proc(iy,id,ih,im,is,ms,leap,&idat[ndsav+1+ch[ich].lastupd],
					ndcmp-ch[ich].lastupd,ch[ich].name,ch[ich].cname,
					ch[ich].network,eof,ch[ich].rate,gb.seq);
				ch[ich].lastupd=ndcmp;
				if( eor != 0) 
				{	ch[ich].ia0=idat[ndcmp];		/* save integration const */
					ch[ich].lastupd=0;				/* look for 1st byte */
				}
				if(eof != 0) 
				{	ch[ich].ia0=2123740000;			/* not continuous, reset*/
					ch[ich].lastupd=0;				/* look for 1st byte */
				}
				break;
			} 
			else
			{	ch[ich].ia0=idat[ndcmp];			/* get next reverse const */
				if(eof != 0) ch[ich].ia0=2123740000;/* time series ended, no IC */

/*				note : if new compression string there is one more sample */

				if(gb.seq == 1 && ndsav == -1) ndcmp++;
				user_proc(iy,id,ih,im,is,ms,leap,&idat[ndsav+1],ndcmp,
					ch[ich].name,ch[ich].cname,ch[ich].network,
					eof,ch[ich].rate,gb.seq);
				break;
			}
		default:
			fprintf(logout,"%s *** Unknown format =%d\n",
				ch[ich].name,gb.format);
			break;
		}     							/* end of switch on packet type */
	}									/* end of if on is this a data packet */
}										/* end of infinite looop */
}										/* end of main */
#ifdef __STDC__
	int status_pack(struct gomberg *gb)
#else	
  status_pack(gb)
  struct gomberg *gb;
#endif
{
	int nbytes;
	int inhibit;
	int det;
	extern FILE *logout;
	nbytes=(gb->numbyt[1] & 7)*256+gb->numbyt[0];	/* how many bytes in packet */
	inhibit = (gb->numbyt[1] & 0x80) ? 1 : 0;	/* set inhibit state */
	det=gb->detseq[0]+gb->detseq[1]*256;
	fprintf(logout,
	"nb=%4d inh=%d Rt=%2d nd=%3d ch=%3d psq=%3d flg=%2x doy=%3d csq=%3d det=%5d\n",
		nbytes,inhibit,gb->routeid,gb->nodeid,gb->chanid,
		gb->packseq,gb->flags,gb->doy,gb->seq,det);
	return 0;

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
		 for(i=0; i<nchan; i++) fprintf(logout,
						 "%d name=%s comp=%s net=%s lc=%s rt=%d nd=%d ch=%d %10.3f\n",
						 i,ch[i].station,ch[i].comp,ch[i].network,ch[i].location,
			 		ch[i].net,ch[i].node,ch[i].chan,ch[i].rate);
		 fflush(logout);

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
	
 	fprintf(logout,
	"%s *** No Match found for seed name.  nchan=%d rt=%d nd=%d ch=%d\n",
		asctim(),nchan,route,node,chan);
	/*for(i=0; i<nchan; i++) fprintf(logout,
						 "%d name=%s comp=%s net=%s lc=%s rt=%d nd=%d ch=%d %10.3f\n",
						 i,ch[i].station,ch[i].comp,ch[i].network,ch[i].location,
			 		ch[i].net,ch[i].node,ch[i].chan,ch[i].rate);*/
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
