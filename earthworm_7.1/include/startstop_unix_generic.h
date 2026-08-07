
/*
 *
 *    Revision history:
 *     $Log: startstop_unix_generic.h,v $
 *     Revision 1.2  2007/03/27 22:48:42  paulf
 *     _MACOSX fixes
 *
 *     Revision 1.1  2006/04/04 16:49:52  stefan
 *     startstop with reconfigure and libraries 20060404 s.lisowski
 *
 *     Revision 1.1  20060226 20:05:54  lisowski
 *     Initial revision
 *
 * startstop_unix_generic.h: startstop parameter definitions for all unix platforms.
 *
 */

  /******************************************************************
   *                         Includes                               *
   *                                                                *
   ******************************************************************/

#include <startstop_lib.h>

#ifndef __DARWIN_UNIX03
#include <wait.h>
#else
#include <sys/wait.h>
#endif

#include <unistd.h>
#include <sys/utsname.h>
#ifdef _USE_POSIX_SHM
# include <sys/mman.h>
# include <semaphore.h>
#else /* Solaris will want these three: */
# include <sys/ipc.h>
# include <sys/shm.h>
# include <sys/sem.h>
#endif
#include <sys/statvfs.h>

#ifndef _MACOSX
//#include <stropts.h>
#endif /* _MACOSX */

#ifdef _USE_PTHREADS
# include <unistd.h>
# include <pthread.h>
#else
# include <synch.h>
# include <thread.h>
#endif
#ifdef _USE_SCHED
# include <sched.h>
#else /* Solaris will want these three: */
# include <sys/priocntl.h>
# include <sys/tspriocntl.h>
# include <sys/rtpriocntl.h>
#endif
#include <limits.h>
#include <pwd.h>
#include <grp.h>

  /******************************************************************
   *                         #defines                               *
   *                                                                *
   ******************************************************************/

#define boolean unsigned char

#ifdef _SOLARIS
#define DEF_CONFIG "startstop_sol.d"
#else /* _SOLARIS */
#define DEF_CONFIG "startstop_unix.d"
#ifdef _USE_PTHREADS
# define thr_exit pthread_exit
# define fork1 fork
#else /* _USE_PTHREADS */
# define pthread_t thread_t
# define pthread_exit thr_exit
#endif /* _USE_PTHREADS */
#endif /* _SOLARIS */

#ifdef _LINUX
#define MAX(a, b)  (((a) > (b)) ? (a) : (b))
#define MIN(a, b)  (((a) < (b)) ? (a) : (b))
#define INTERNAL
#endif

  /******************************************************************
   *                         Structs                                *
   *                                                                *
   ******************************************************************/

struct {
   char   *processName;
   char   *args;			 /* *args Not used by Solaris */
   char   tcum[20];          /* Cumulative run time */
   pid_t  pid;
   char   className[4];
   int    priority;
} parent;


  /******************************************************************
   *                         Prototypes                             *
   *                                                                *
   ******************************************************************/


void *Interactive( void * );  /* Interactive thread */
void GetConfig(); /* Reads configuration file */
void Threads( void*(void *), thread_t * );

#ifdef _USE_POSIX_SIGNALS
 void 	SigtermHandler( int , siginfo_t *, void * );
#else
 void 	SigtermHandler( int );
#endif
void 	StartEarthworm( char * );
int 	RunEarthworm( int, char *[] );
void 	StopEarthworm();
void 	ConstructIds( char *, char *, uid_t *, gid_t * );
                                      	/* construct uid and gid from names */
									  	/* starting children error message */
void 	RestartChild( char * ); /* Terminate/restart child process */
void 	SetPriority( pid_t, char *, int ); /* Set priorities */

void 	EncodeStatus( char * );
void 	SendStatus( int ); 	/* Send a status msg via tranpsort */

/* SpawnChildren starts child processes besides statmanager      */
void 	SpawnChildren();
/* Turn on one child process       */
int  StartChild( int, boolean );


  /******************************************************************
   *                         Externs                                *
   *                                                                *
   ******************************************************************/

char 		starttime[TIMESTR_LEN];
int 		done;
int 		nChild;
CHILD 		*child;
METARING	metaring;
