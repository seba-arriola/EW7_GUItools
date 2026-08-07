/*
 * sema_ew.c  Unix version
 *
 * This source file contains system-dependent functions for
 * handling event semaphores and mutexes
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#ifdef _USE_PTHREADS
# include <pthread.h>
# include <semaphore.h>
#else	/* assume Solaris threads, semaphores, and mutexes */
# include <synch.h>
# include <thread.h>
# define pthread_mutex_t mutex_t
# define pthread_mutex_lock mutex_lock
# define pthread_mutex_unlock mutex_unlock
# define pthread_mutex_destroy mutex_destroy
# define sem_t sema_t
# define sem_post sema_post
# define sem_wait sema_wait
# define sem_destroy sema_destroy
#endif

static sem_t semaphore;		/* Event Semaphore                        */
static pthread_mutex_t mutex;	/* Mutual Exclusion semaphore lock        */


/************************ CreateSemaphore_ew ************************
    Create an unnamed semaphore which is posted when some event is complete.
*********************************************************************/
void CreateSemaphore_ew( void )
{
   static unsigned int count = 0;
   int   rc;
   
   /* semaphore to be shared only among threads in this process */

#ifdef _USE_PTHREADS
   rc = sem_init( &semaphore, 0, count ); /* must set pshare=1 if calling process will fork */
   /*
   rc = sem_init( &semaphore, 1, count );*/ /* must set pshare=1 if calling process will fork */
#else
   void *dummy;
   rc = sema_init( &semaphore, count, USYNC_THREAD, dummy );
#endif
 
   if ( rc != 0 )
   {
      fprintf( stderr,
              "CreateSemaphore_ew: Error from sem_init: %d: %s; Exiting.\n", rc, strerror(errno) );
      exit( -1 );
   }
   return;
}

/**************************** PostSemaphore ****************************
             Let another thread know that a message is ready.
************************************************************************/
void PostSemaphore( void )
{
   int rc;

   rc = sem_post( &semaphore );
   if ( rc != 0 )
      fprintf( stderr,
              "PostSemaphore: Error from sem_post: %d: %s\n", rc, strerror(errno) );
   return;
}

/**************************** WaitSemPost ***************************
      Wait for the event semaphore to be posted by another thread.
*********************************************************************/
void WaitSemPost( void )
{
   int rc;

   rc = sem_wait( &semaphore );
   if ( rc != 0 )
   {
      fprintf( stderr,
              "WaitSemPost: Error from sem_wait: %d: %s; Exiting.\n", rc, strerror(errno) );
      exit( -1 );
   }
   return;
}

/**************************** DestroySemaphore *************************
                         Kill the event semaphore.
************************************************************************/
void DestroySemaphore( void )
{
   int rc;

   rc = sem_destroy( &semaphore );
   if ( rc != 0 )
      fprintf( stderr,
              "PostSemaphore: Error from sem_destroy: %d: %s\n", rc, strerror(errno) );
   return;
}


/************************** CreateMutex_ew *************************
     Set up mutex semaphore to arbitrate the use of some variable
        by different threads.
********************************************************************/
void CreateMutex_ew( void )
{
   int   rc;

   /* mutex to be shared only among threads in this process, not between processes */

#ifdef _USE_PTHREADS
   pthread_mutexattr_t attr;
   /* initialize mutex.  defaults values set by pthread_mutexattr_init should be:
	PTHREAD_PROCESS_PRIVATE, PTHREAD_PRIO_NONE, and PTHREAD_MUTEX_DEFAULT */
   rc = pthread_mutexattr_init(&attr);
#ifdef _MACOSX
   rc = pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);
#else
   rc = pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ADAPTIVE_NP);
#endif
   if ( rc != 0 )
   {
      fprintf( stderr,
              "CreateMutex_ew: Error from pthread_mutexattr_settype: %d: %s; Exiting.\n",
               rc, strerror(errno) );
      exit( -1 );
   }
   rc = pthread_mutex_init( &mutex, &attr );
#else
   void *dummy;
   rc = mutex_init( &mutex, USYNC_THREAD, dummy );
#endif
   if ( rc != 0 )
   {
      fprintf( stderr,
              "CreateMutex_ew: Error from pthread_mutex_init: %d: %s; Exiting.\n",
               rc, strerror(errno) );
      exit( -1 );
   }
   return;
}


/*************************** RequestMutex **************************
                       Grab the mutex semaphore
********************************************************************/
void RequestMutex( void )
{
   int   rc;

   rc = pthread_mutex_lock( &mutex );
   if ( rc != 0 )
   {
      fprintf( stderr,
              "RequestMutex: Error from pthread_mutex_lock: %d: %s; Exiting.\n",
               rc, strerror(errno) );
      exit( -1 );
   }
   return;
}


/************************ ReleaseMutex_ew **************************
                    Release the mutex semaphore
********************************************************************/
void ReleaseMutex_ew( void )
{
   int   rc;

   rc = pthread_mutex_unlock( &mutex );
   if ( rc != 0 )
   {
      fprintf( stderr,
              "ReleaseMutex_ew: Error from pthread_mutex_unlock: %d: %s; Exiting.\n",
               rc, strerror(errno) );
      exit( -1 );
   }
   return;
}


/***************************** CloseMutex **************************
                        Destroy mutex semaphore
********************************************************************/
void CloseMutex( void )
{
   int   rc;

   rc = pthread_mutex_destroy( &mutex );
   if ( rc != 0 )
   {
      fprintf( stderr,
              "CloseMutex: Error from pthread_mutex_destroy: %d: %s; Exiting.\n",
               rc, strerror(errno) );
      exit( -1 );
   }
   return;
}

/************************ CreateSpecificMutex **********************
        As Above, but allows many to be created. Story: The original
        routines were written presuming that only one mutex would
        be used. These routines have become imbedded in much code.
        The ...Specific... versions are created for wave_server2.
        Alex 1/19/97
********************************************************************/
void CreateSpecificMutex( pthread_mutex_t* mp )
{
   int   rc;

#ifdef _USE_PTHREADS
   pthread_mutexattr_t attr;
   /* initialize mutex.  defaults values set by pthread_mutexattr_init should be:
	PTHREAD_PROCESS_PRIVATE, PTHREAD_PRIO_NONE, and PTHREAD_MUTEX_DEFAULT */
   rc = pthread_mutexattr_init(&attr);
#ifdef _MACOSX
   rc = pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);
#else
   rc = pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ADAPTIVE_NP);
#endif
   if ( rc != 0 )
   {
      fprintf( stderr,
              "CreateMutex_ew: Error from pthread_mutexattr_settype: %d: %s; Exiting.\n",
               rc, strerror(errno) );
      exit( -1 );
   }
   rc = pthread_mutex_init( mp, &attr );
#else
   void *dummy;
   rc = mutex_init( mp, USYNC_THREAD, dummy );
#endif
   if ( rc != 0 )
   {
      fprintf( stderr,
              "CreateSpecificMutex_ew: Error from pthread_mutex_init: %d: %s; Exiting.\n",
               rc, strerror(errno) );
      exit( -1 );
   }
   return;
}


/********************* RequestSpecificMutex ************************
                     Grab the mutex semaphore
********************************************************************/
void RequestSpecificMutex( pthread_mutex_t* mp )
{
   int   rc;
   /*
   rc = pthread_mutex_trylock( &mutex );
      fprintf( stderr,
              "RequestMutex: trylock: %d:errno %d %s %s  pid %d\n",
               rc,errno, strerror(errno),strerror(rc),getpid() );
	      */
      /*
   if ( rc != 0 )
   {
	   ;
   }else{
	   return;
   }
  pthread_cleanup_push(pthread_mutex_unlock, mp);
   */

   rc = pthread_mutex_lock( mp );
   /*
   fprintf( stderr,
           "RequestSpecificMutex: mutex_lock: %d:%x %s %s pid %d\n",
            rc,mp, strerror(errno),strerror(rc) ,getpid());
	    */
   if ( rc != 0 )
   {
      fprintf( stderr,
              "RequestSpecificMutex: Error from pthread_mutex_lock: %d:%x %s; Exiting.\n",
               rc,mp, strerror(errno) );
      exit( -1 );
   }
   return;
}


/******************** ReleaseSpecificMutex *************************
                  Release the mutex semaphore
********************************************************************/
void ReleaseSpecificMutex( pthread_mutex_t* mp )
{
   int   rc;

   rc = pthread_mutex_unlock( mp );
   if ( rc != 0 )
   {
      fprintf( stderr,
              "MReleaseSpecificMutex: Error from pthread_mutex_unlock: %d: %s; Exiting.\n",
               rc, strerror(errno) );
      exit( -1 );
   }
   return;
}



/********************** CloseSpecificMutex *************************
                      Destroy mutex semaphore
********************************************************************/
void CloseSpecificMutex( pthread_mutex_t* mp )
{
   int   rc;

   rc = pthread_mutex_destroy( mp );
   if ( rc != 0 )
   {
      fprintf( stderr,
              "CloseSpecificMutex: Error from pthread_mutex_destroy: %d: %s; Exiting.\n",
               rc, strerror(errno) );
      exit( -1 );
   }
   return;
}

