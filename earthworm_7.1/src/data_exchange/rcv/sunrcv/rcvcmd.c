#include <stdio.h>
#include <errno.h>			/* error  number defs */
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#ifdef __STDC__
  void cmdarg(int argc, char *argv[])
#else
  void cmdarg(argc,argv)
  int argc;
  char *argv[];
#endif
/*********************** RCVCMD.c *********************************
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
	 
	extern char path[];
	extern FILE * logout;
	extern int dbg;					/* link to debug on flag */
	extern int tcpflg;				/* Link to TCP to TCPOUT flag */
	extern int forceroll;			/* link to force rollbacks flag */
	extern char hostname[], *dotadr;	/* Host or dot address */
	extern int port;				/* socket port number */
	extern int partupd;				/* does user want partial updates? */
	extern char seed_name[];
	extern int multiple;
	extern int rcvlog;
	extern char logfilename[];
	extern int norollback;			/* if true, disable rollback code */
	int i;
	char *logptr;					/* pointer to getenv for EW_LOG */
	fprintf(logout,"# args=%d\n",argc);
	for (i=1; i<argc; i++) 	{	/* for each command line argument */
		fprintf(logout,"%d #%s# \n",i,argv[i]);
		if(strcmp(argv[i],"-i") == 0) 	/* Set the path to the TT */
		{	strcpy(path,argv[i+1]);
			fprintf(logout,"TT Path set to %s\n",path);
		}
		if(strcmp(argv[i],"-tcp") == 0) 
		{	tcpflg=1;
			fprintf(logout,"TCP to TCPOUT on \n");
		}
		if( strcmp(argv[i],"-norollback") == 0)
		{	norollback = 1;
			fprintf(logout,"No rollbacks allowed - Passive mode!\n");
		}
		if(strcmp(argv[i],"-!") == 0 ) 
		{	dbg=1;
			fprintf(logout,"dbg on ");
		}
		if(strcmp(argv[i],"-$") == 0 ) 	
		{	forceroll=1;
			fprintf(logout,"force roll on ");
		}
		if(strcmp(argv[i],"-host") == 0)
		{	strcpy(hostname,argv[i+1]);
			fprintf(logout,"Host=%s\n",hostname);
		}
		if(strcmp(argv[i],"-rcvlog") == 0) {
			rcvlog=1;
			fprintf(logout,"rcvlog=%d\n",rcvlog);
		}
		if(strcmp(argv[i],"-o") == 0) 
		{	fprintf(logout,"%s done ",argv[i+1]);
			logptr=getenv("EW_LOG");
			if(logptr == NULL) strcpy(logfilename,argv[i+1]);
			else
			{	strcpy(logfilename,logptr);
				strcat(logfilename,argv[i+1]);
			}
			fprintf(logout,"Log output redirected to %s\n",logfilename);
		}
		if(strcmp(argv[i],"-#") == 0) 
		{	multiple=1;					/* must be multiple */
			fprintf(logout,"Multiple mode \n");
		}
	}
/*	fprintf(logout,"\n");
	fflush(logout);*/
	return;
}
 
