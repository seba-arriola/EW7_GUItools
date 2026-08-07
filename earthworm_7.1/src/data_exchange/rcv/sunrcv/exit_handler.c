#include <stdio.h>
#include <signal.h>
#include "exit_handler.h"

/* GLOBAL variable tag gives user defined string for output on closing */

char tag[20];

/*	This module sets up exit handlers for MOST ways a process can terminate.
	That is by intercepting most fatal signals, and by setting up the same
	routine to be called if the process executes an exit() or returns from
	main().  The user must call the exit_handler_init routine early in the
	setup code for the process and supply a user_shutdown() routine to handle
	the shutdown chores

	D.C. Ketchum	Oct 1997	Made a separate module from pieces in VDL.C

	Entry Points :  
	exit_handler_init(char *tag)		Tag is used so output from this can
										be identified.  Call this routine only
										once from the main routine as part of
										the routine startup.

	The caller must also create a routine of the form :

	void user_shutdown()	Which will be invoked when the process is exiting
							due to signal or exit()

	Date	Description
	11/20/97 change SIGWINCH to be a sigignore

/*
	The exit handler is called on system close down for normal exit.  In this
	case if the user routine does an exit() or if a signal is caught using
	these routines 
*/
#ifdef SUNOS
/*
	E X I T _ H A N D L E R 
*/
#ifdef __STDC__ 
   void exit_handler(int status, int arg)
#else
  void exit_handler(status,arg)
  int status;
  int arg;
#endif
{
	extern FILE *logout;
	int err;
	char false=0;
	fprintf(logout,"On Exit handler status=%d arg=%d\n",status,arg);
	user_shutdown();
	return;
}
#endif

#ifdef SOLARIS
#ifdef __STDC__ 
   void exit_handler()
#else
  void exit_handler()
#endif
{
	extern FILE *logout;
	int err;
	char false=0;
	fprintf(logout,"On Exit handler. SOLARIS provide no info\n");
	user_shutdown();
	return;
}
#endif
#ifdef __STDC__
  void cc_handler(int arg)
#else
  void cc_handler(arg)
  int arg;
#endif
{
	extern FILE *logout;
	int err;
	char false=0;
	fprintf(logout,"cc_handler=%d\n",arg); fflush(logout);
#ifdef SUNOS
/*	err=ioctl(0,FIONBIO, &false);	/* set non blocking i/o */
#endif
	switch (arg) {
		case SIGINT :
 			fprintf(logout,"%s control C clean up\n",tag);
			break;
		case SIGHUP :
			fprintf(logout,"%s SIGHUP exit\n",tag);
			break;
		case SIGQUIT:
			fprintf(logout,"%s SIGQUIT exit\n",tag);
			break;
		case SIGILL:
			fprintf(logout,"%s SIGILL exit\n",tag);
			break;
		case SIGTRAP:
			fprintf(logout,"%s SIGTRAP exit\n",tag);
			break;
		case SIGABRT:
			fprintf(logout,"%s SIGABRT exit\n",tag);
			break;
		case SIGEMT:
			fprintf(logout,"%s SIGEMT exit\n",tag);
			break;
		case SIGFPE:
			fprintf(logout,"%s SIGFPE exit\n",tag);
			break;
		case SIGKILL:
			fprintf(logout,"%s SIGKILL exit\n",tag);
			break;
		case SIGBUS:
			fprintf(logout,"%s SIGBUS exit\n",tag);
			break;
		case SIGSEGV:
			fprintf(logout,"%s SIGSEGV exit\n",tag);
			break;
		case SIGSYS:
			fprintf(logout,"%s SIGSYS exit\n",tag);
			break;
		case SIGALRM:
			fprintf(logout,"%s SIGALRM exit\n",tag);
			break;
		case SIGTERM:
			fprintf(logout,"%s SIGTERM exit\n",tag);
			break;
		case SIGURG:
			fprintf(logout,"%s SIGURG exit\n",tag);
			break;
		case SIGSTOP:
			fprintf(logout,"%s SIGSTOP exit\n",tag);
			break;
		case SIGTSTP:
			fprintf(logout,"%s SIGTSTP exit\n",tag);
			break;
		case SIGCONT:
			fprintf(logout,"%s SIGCONT exit\n",tag);
			break;
		case SIGCHLD:
			fprintf(logout,"%s SIGCHLD exit\n",tag);
			break;
		case SIGTTIN:
			fprintf(logout,"%s SIGTTIN exit\n",tag);
			break;
		case SIGTTOU:
			fprintf(logout,"%s SIGTTOU exit\n",tag);
			break;
		case SIGIO:
			fprintf(logout,"%s SIGIO exit\n",tag);
			break;
		case SIGXCPU:
			fprintf(logout,"%s SIGXCPU exit\n",tag);
			break;
		case SIGXFSZ:
			fprintf(logout,"%s SIGXFSZ exit\n",tag);
			break;
		case SIGWINCH:
			fprintf(logout,"%s SIGWINCH RCVed/Ignored\n",tag);
			return ;
		default:
			fprintf(logout,"%s Unknown signal=%d \n",tag,arg);
	}
	fflush(logout);
	exit(arg);				/* this will go to ON_EXIT handler which will shutdown */
}
exit_handler_init(tagit)
char *tagit;

{
	strcpy(tag,tagit);				/* move the tag */

/*
	Set signal to intercept most of the common signals
*/
	signal(SIGINT,cc_handler);
	signal(SIGHUP,cc_handler);
	signal(SIGQUIT,cc_handler);
	signal(SIGILL,cc_handler);
	signal(SIGTRAP,cc_handler);
	signal(SIGABRT,cc_handler);
	signal(SIGEMT,cc_handler);
	signal(SIGFPE,cc_handler);
	signal(SIGKILL,cc_handler);
	signal(SIGBUS,cc_handler);
	signal(SIGSEGV,cc_handler);
	signal(SIGSYS,cc_handler);
	signal(SIGTERM,cc_handler);
	signal(SIGSTOP,cc_handler);	
	signal(SIGCONT,cc_handler);
	signal(SIGTSTP,cc_handler);
	signal(SIGCHLD,cc_handler);
	signal(SIGXCPU,cc_handler);
	signal(SIGXFSZ,cc_handler);
	sigignore(SIGWINCH);
/*	printf("CC_HANDLER initialized... %s\n",tagit);*/
/*
	Set so that our exit handler is called if any exit() is called as well
*/
#ifdef SUNOS
	on_exit(exit_handler,-1);
#endif
#ifdef SOLARIS
	atexit(exit_handler);
#endif
}
