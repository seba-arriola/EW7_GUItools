#include <stdio.h>
#include <errno.h>			/* error  number defs */
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#ifdef __STDC__
  void cmdarg(int argc, char *argv[])
#else
  void cmdarg(argc,argv)
  int argc;
  char *argv[];
#endif
/*********************** STATIONCMD.c *********************************
*
*	calling Sequence :	cmdarg(argc,argv)	
*	argc	int with number of arguments parsed by shell
*	*argv[]	pointer to array of string pointers with text strings parsed by shell
*
*	Programmed by : D.C. Ketchum
*	Created :		May 1996
*	Date Modified Description of Mod
*	22-May-95		Base Functionality
**********************************************************************/
{
	extern int logflg;
	extern FILE *logout;
	extern int partupd;
	extern int dbg;
	extern int max_chan;			/* old parameter now dynamic # of parms*/
	extern int multiple;			/* connect to single vs multiple var */
	extern char logfilename[];
	extern char logpathname[];
	char * logptr;					/* point to EW_LOG translation if any */
	void user_proc_cmd();
 	int i;
	for (i=1; i<argc; i++) 	{	/* for each command line argument */
/*		fprintf(logout,"%d #%s# ",i,argv[i]);*/
		if(strcmp(argv[i],"-partial") == 0)
		{	partupd=1;
			fprintf(logout,"Part update to user_proc=%d\n",partupd);
		}
		if(strcmp(argv[i],"-stationlog") == 0) {	/* Set the path to the TT */
			logptr=getenv("EW_LOG");
			if(logptr == NULL) strcpy(logfilename,"station.loga");
			else
			{	strcpy(logfilename,logptr);
				strcat(logfilename,"station.loga");
				strcpy(logpathname,logptr);
			}
			fprintf(logout,"Set log filename to %s\n",logfilename);
		}
		if(strcmp(argv[i],"-dbg") == 0)
		{	dbg=1;
			fprintf(logout,"Debug on\n");
		}
		if(strcmp(argv[i],"-#") == 0)
		{	max_chan=atoi(argv[i+1]);		/* set maximum # of chans */
			fprintf(logout,"Maxchan=%d\n",max_chan);
			multiple=1;
		}
	}
	user_proc_cmd(argc,argv);
	fprintf(logout,"return cmd\n");
	return;
}
 
