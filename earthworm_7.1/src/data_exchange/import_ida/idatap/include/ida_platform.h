#pragma ident "$Id: ida_platform.h,v 1.1 2004/03/17 21:18:03 lombard Exp $"
/*====================================================================
 *
 * Platform specific types, constants, macros, etc...
 *
 *===================================================================*/
#ifndef platform_h_included
#define platform_h_included

#ifdef _SOLARIS
#   ifndef SOLARIS
#       define SOLARIS
#   endif
#endif

#ifdef _WINNT
#   ifndef WINNT
#       define WINNT
#   endif
#endif

#ifdef NT
#   ifndef WINNT
#       define WINNT
#   endif
#endif

#ifdef WIN32
#   ifndef WINNT
#       define WINNT
#   endif
#endif

#ifdef _WIN32
#   ifndef WINNT
#       define WINNT
#   endif
#endif

#ifdef _LINUX
#   ifndef LINUX
#       define LINUX
#   endif
#endif

/* Solaris */

#ifdef SOLARIS
#   include <stdio.h>
#   include <time.h>
#   include <ctype.h>
#   include <fcntl.h>
#   include <unistd.h>
#   include <stdlib.h>
#   include <string.h>
#   include <stdlib.h>
#   include <stdarg.h>
#   include <errno.h>
#   include <signal.h>
#   include <sys/types.h>
#   include <sys/timeb.h>
#   include <sys/stat.h>
#   include <sys/param.h>
#   include <sys/socket.h>
#   include <netinet/in.h>
#   include <arpa/inet.h>
#   include <netdb.h>
#   define USE_SOLARIS_THREADS_AND_SEMAPHORES
#   define HAVE_SYSLOGD
#   define HAVE_SVR4_IPC
#   ifdef i386
#       define X86_UNIX32
#   endif
#   ifdef sparc
#       define SPARC_UNIX32
#   endif
#endif

#ifdef USE_SOLARIS_THREADS_AND_SEMAPHORES
#   include <thread.h>
#   include <synch.h>
#endif

/* Linux */

#ifdef LINUX
#   include <stdio.h>
#   include <time.h>
#   include <ctype.h>
#   include <fcntl.h>
#   include <unistd.h>
#   include <stdlib.h>
#   include <string.h>
#   include <stdlib.h>
#   include <stdarg.h>
#   include <errno.h>
#   include <signal.h>
#   include <sys/time.h>
#   include <sys/types.h>
#   include <sys/timeb.h>
#   include <sys/stat.h>
#   include <sys/param.h>
#   include <sys/socket.h>
#   include <netinet/in.h>
#   include <arpa/inet.h>
#   include <netdb.h>
#   define HAVE_POSIX_THREADS
#   define HAVE_POSIX_SEMAPHORES
#   define HAVE_SYSLOGD
#   define HAVE_SVR4_IPC
#   ifdef i386
#       define X86_UNIX32
#   endif
#   ifdef sparc
#       define SPARC_UNIX32
#   endif
#endif

/* FreeBSD */

#ifdef FREEBSD
#   include <stdio.h>
#   include <time.h>
#   include <ctype.h>
#   include <fcntl.h>
#   include <unistd.h>
#   include <stdlib.h>
#   include <string.h>
#   include <stdlib.h>
#   include <stdarg.h>
#   include <errno.h>
#   include <signal.h>
#   include <sys/time.h>
#   include <sys/types.h>
#   include <sys/timeb.h>
#   include <sys/stat.h>
#   include <sys/param.h>
#   include <sys/socket.h>
#   include <netinet/in.h>
#   include <arpa/inet.h>
#   include <netdb.h>
#   define HAVE_POSIX_THREADS
#   define HAVE_POSIX_SEMAPHORES
#   define HAVE_SYSLOGD
#   ifdef i386
#       define X86_UNIX32
#   endif
#   ifdef sparc
#       define SPARC_UNIX32
#   endif
#endif

#ifdef WINNT
#   include <stdio.h>
#   include <time.h>
#   include <ctype.h>
#   include <conio.h>
#   include <fcntl.h>
#   include <stdlib.h>
#   include <string.h>
#   include <stdlib.h>
#   include <stdarg.h>
#   include <share.h>
#   include <io.h>
#   include <errno.h>
#   include <signal.h>
#   include <process.h>
#   include <limits.h>
#   include <assert.h>
#   include <sys/types.h>
#   include <sys/timeb.h>
#   include <sys/stat.h>
#   include <windows.h>
#   define X86_WIN32
#endif

/* Portable data types key off the platform type defined above */

#include "stdtypes.h"

/* Windows NT Threads, Sockets, and other issues... */

#ifdef WINNT

/* Window sockets deviation from Berkeley sockets stuff */

/* Redefine these standard error numbers */
#   undef  EINTR
#   define EINTR            (WSAEINTR)
#   undef  EACCES
#   define EACCES           (WSAEACCES)
#   undef  EINVAL
#   define EINVAL           (WSAEINVAL)
#   undef  ENOINT
#   define ENOINT           (WSAENOINT)
#   undef  ENOTSOCK
#   define ENOTSOCK         (WSAENOTSOCK)
/* Define these error numbers */
#   define EPROTO           (WSAEPROTOTYPE)
#   define EALREADY         (WSAEALREADY)
#   define ETIMEDOUT        (WSAETIMEDOUT)
#   define EAFNOSUPPORT     (WSAEAFNOSUPPORT)
#   define ELOOP            (WSAELOOP)
#   define ENOSR            (WSAENOBUFS)    /* No stream resources */
#   define ENOTSOCK         (WSAENOTSOCK)
#   define EPROTOTYPE       (WSAEPROTOTYPE)
#   define ECONNRESET       (WSAECONNRESET)
#   define EWOULDBLOCK      (WSAEWOULDBLOCK)
#   define ECONNABORTED     (WSAECONNABORTED)

/* Thread sleep function, Sleep() arg is milliseconds */
#   define sleep( seconds ) (Sleep( (DWORD)((seconds) * 1000) ))

/* strcasecmp library function is stricmp */

#   define strcasecmp stricmp
#   define strncasecmp _strnicmp

#   define vsnprintf _vsnprintf
#   define snprintf  _snprintf

#   define getpid _getpid

#   define ftime _ftime

#   ifdef PATH_MAX
#      define MAXPATHLEN PATH_MAX
#   endif

#endif /* WINNT */

/* Macros for mutual exclusion portability */

#if defined (USE_SOLARIS_THREADS_AND_SEMAPHORES)
    typedef mutex_t MUTEX;
#   define MUTEX_LOCK(arg)   mutex_lock(arg)
#   define MUTEX_UNLOCK(arg) mutex_unlock(arg)
#   define MUTEX_INIT(arg)   mutex_init((arg), USYNC_THREAD, NULL)
#   define MUTEX_TRYLOCK(arg) (mutex_trylock(arg) == 0 ? TRUE : FALSE)
#   define MUTEX_INITIALIZER DEFAULTMUTEX
#elif defined (HAVE_POSIX_THREADS)
#   include <pthread.h>
    typedef pthread_mutex_t MUTEX;
#   define MUTEX_LOCK(arg)   pthread_mutex_lock(arg)
#   define MUTEX_UNLOCK(arg) pthread_mutex_unlock(arg)
#   define MUTEX_INIT(arg)   pthread_mutex_init((arg), NULL)
#   define MUTEX_TRYLOCK(arg) (pthread_mutex_trylock(arg) == 0 ? TRUE : FALSE)
#   define MUTEX_INITIALIZER PTHREAD_MUTEX_INITIALIZER
#elif defined WINNT
    typedef HANDLE MUTEX;
#   define MUTEX_LOCK(arg)    WaitForSingleObject( *(arg), INFINITE )
#   define MUTEX_TRYLOCK(arg) (WaitForSingleObject( *(arg), 0 ) == WAIT_TIMEOUT ? FALSE : TRUE)
#   define MUTEX_UNLOCK(arg) ReleaseMutex( *(arg) )
#   define MUTEX_INIT(arg)   (*(arg) = CreateMutex( NULL, FALSE, NULL ))
#   define MUTEX_INITIALIZER NULL
#else
    typedef int MUTEX;
#   define MUTEX_LOCK(arg)
#   define MUTEX_UNLOCK(arg)
#   define MUTEX_INIT(arg)
#   define MUTEX_INITIALIZER 0
#endif                                 /* HAVE_POSIX_THREADS */

/* Macros for semaphore portability */

#if defined (USE_SOLARIS_THREADS_AND_SEMAPHORES)

    typedef sema_t SEMAPHORE;
#   define SEM_INIT(id, init, max) sema_init((id), (unsigned int) (init), USYNC_THREAD, NULL)
#   define SEM_POST(id)            sema_post(id)
#   define SEM_WAIT(id)            sema_wait(id)
#   define SEM_DESTROY(id)         sema_destroy(id)
#   define SEM_TRYWAIT(id)         sema_trywait(id)

#elif defined (HAVE_POSIX_SEMAPHORES)

#   include <semaphore.h>
    typedef sem_t SEMAPHORE;
#   define SEM_INIT(id, init, max) sem_init((id), 0, (unsigned int) (init))
#   define SEM_POST(id)            sem_post(id)
#   define SEM_WAIT(id)            sem_wait(id)
#   define SEM_DESTROY(id)         sem_destroy(id)
#   define SEM_TRYWAIT(id)         sem_trywait(id)

#elif defined WINNT

    typedef HANDLE SEMAPHORE;
#   define SEM_INIT(id, init, max) (*(id) = CreateSemaphore(NULL, (init), (max), NULL))
#   define SEM_POST(id)        ReleaseSemaphore( *(id), 1, NULL )
#   define SEM_WAIT(id)        WaitForSingleObject( *(id), INFINITE )
#   define SEM_WAIT_TIMEOUT(id, timeout) \
           (WaitForSingleObject( *(id), (DWORD)((timeout) * 1000) ) == WAIT_TIMEOUT ? -1 : 0)
#   define SEM_TRYWAIT(id)    (WaitForSingleObject( *(id), 0 ) == WAIT_TIMEOUT ? -1 : 0)
#    define SEM_DESTROY(id)     CloseHandle( *(id) );

#else

    typedef int SEMAPHORE;
#   define SEM_INIT(arg1, arg2, arg3)
#   define SEM_POST(arg)
#   define SEM_WAIT(arg)
#   define SEM_TRYWAIT(arg)
#endif

/* Macros for portable sockets */

#if defined unix
#   define CLOSESOCKET(sd) close(sd)
    typedef int SOCKET;
#   define INVALID_SOCKET -1
#   ifndef INADDR_NONE
#       define INADDR_NONE    -1
#   endif
#   define SOCKET_ERROR INVALID_SOCKET
#   define RECVFROM(a,b,c,d,e,f)     \
    (INT32) recvfrom(                \
        (int)                   (a), \
        (void *)                (b), \
        (size_t)                (c), \
        (int)                   (d), \
        (struct sockaddr *)     (e), \
        (int *)                 (f)  \
    )
#   define SENDTO(a,b,c,d,e,f)       \
    (INT32) sendto(                  \
        (int)                   (a), \
        (void *)                (b), \
        (size_t)                (c), \
        (int)                   (d), \
        (struct sockaddr *)     (e), \
        (int)                   (f)  \
    )
#elif defined WINNT
#   define CLOSESOCKET(sd) closesocket(sd)
#   define RECVFROM(a,b,c,d,e,f)     \
    (INT32) recvfrom(                \
        (SOCKET)                (a), \
        (char FAR *)            (b), \
        (int)                   (c), \
        (int)                   (d), \
        (struct sockaddr FAR *) (e), \
        (int FAR *)             (f)  \
    )
#   define SENDTO(a,b,c,d,e,f)       \
    (INT32) sendto(                  \
        (SOCKET)                (a), \
        (char FAR *)            (b), \
        (int)                   (c), \
        (int)                   (d), \
        (struct sockaddr FAR *) (e), \
        (int)                   (f)  \
    )
#endif /* unix */

/* Macros for portable threads */

#if defined (USE_SOLARIS_THREADS_AND_SEMAPHORES)

    typedef thread_t THREAD;
    typedef void *THREAD_FUNC;
#   define THREAD_CREATE(tp,fp,ap) (thr_create(NULL, 0,(fp),(ap), THR_DETACHED | THR_NEW_LWP, (tp)) ? FALSE : TRUE)
#   define THREAD_SELF()           thr_self()
#   define THREAD_EXIT(sp)         thr_exit((sp))
#   define THREAD_SIGSETMASK(a,b,c) thr_sigsetmask(a, b, c)

#elif defined (HAVE_POSIX_SEMAPHORES)

#   include <pthread.h>
    typedef pthread_t THREAD;
    typedef void *THREAD_FUNC;
#   define THREAD_CREATE(tp,fp,ap) (pthread_create((tp),NULL,(fp),(void *)(ap)) ? FALSE : TRUE)
#   define THREAD_SELF()           pthread_self()
#   define THREAD_EXIT(sp)         pthread_exit((sp))
#   define THREAD_SIGSETMASK(a,b,c) pthread_sigmask(a, b, c)

#elif defined WINNT

    typedef HANDLE THREAD;
#   define THREAD_FUNC void
#   define THREAD_CREATE(tp,fp,ap) \
((*(tp) = (THREAD) _beginthread((fp),0,(void*)(ap))) == (THREAD) -1 ? FALSE : TRUE)
#   define THREAD_JOIN(tp)         WaitForSingleObject(*(tp), INFINITE)
#   define THREAD_SELF()           GetCurrentThreadId()
#   define THREAD_EXIT(sp)         _endthread()
#   define THREAD_ERRNO           WSAGetLastError()

#else

    typedef int THREAD;
    typedef void THREAD_FUNC;
#   define THREAD_CREATE(tp,fp,ap)
#   define THREAD_SELF()
#   define THREAD_EXIT(sv)
#endif

/* Syslog facility */

#ifdef HAVE_SYSLOGD
#   include <syslog.h>
#endif

/* System V message queues */

#ifdef HAVE_SVR4_IPC
#   include <sys/msg.h>
#endif

/* Other types */

#if defined(WINNT)
    typedef struct _timeb TIMEB;
#else
    typedef struct timeb TIMEB;
#endif

/* Compile time discovery of host byte order */

#if defined(X86_16BIT) || defined(X86_WIN32) || defined(X86_UNIX32)
#   ifndef LTL_ENDIAN_HOST
#       define LTL_ENDIAN_HOST
#   endif
#elif defined(SPARC_UNIX32)
#   ifndef BIG_ENDIAN_HOST
#       define BIG_ENDIAN_HOST
#   endif
#endif

/* Some useful constants */

#ifndef MAXPATHLEN
#   define MAXPATHLEN 255
#endif

#ifndef MAXHOSTNAMELEN
#   define MAXHOSTNAMELEN 255
#endif

#endif

/* Revision History
 *
 * $Log: ida_platform.h,v $
 * Revision 1.1  2004/03/17 21:18:03  lombard
 * Initial revision
 *
 * Revision 1.2  2001/05/20 16:00:48  dec
 * removed dos and windows support, added solaris mt specific support
 *
 * Revision 1.1  2001/05/07 22:46:49  dec
 * introduced
 *
 */
