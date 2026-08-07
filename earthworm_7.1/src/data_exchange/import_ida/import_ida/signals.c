/*======================================================================
 *
 * Signal handling thread.
 *
 *====================================================================*/
#include "import_ida.h"

#ifdef _SOLARIS
#include "util.h"

static void *signal_handler(void *dummy)
{
sigset_t set;
int sig;

    logit("t", "signal handler started (tid %d)\n", thr_self());

    sigfillset(&set); /* catch all signals defined by the system */

    while (1) {

        /* wait for a signal to arrive */

        sig = sigwait(&set);
        logit("t", "%s\n", util_sigtoa(sig));

        /* process signals */

        if (sig == SIGTERM || sig == SIGQUIT || sig == SIGINT) {
            exit(0);
        } else if (sig == SIGSEGV || sig == SIGBUS) {
            logit("t", "abort()... core dumped\n");
            abort();
        } else {
            logit("t", "signal ignored\n");
        }
    }
}

void start_sighandler()
{
int error;
thread_t tid;
sigset_t set;
static char *fid = "signals_init";

/* Block all signals */

    sigfillset(&set);
    thr_sigsetmask(SIG_SETMASK, &set, NULL);

/* Create signal handling thread to catch all nondirected signals */

    error = thr_create((void *) NULL, 0, signal_handler, (void *) NULL,
        THR_NEW_LWP | THR_DAEMON | THR_DETACHED, &tid
    );
    if (error) {
        logit("t", "%s: thr_create: error %d\n", fid, error);
        exit(1);
    }
}

#elif _WINNT

static BOOL signal_handler(DWORD fdwCtrlType)
{
    switch (fdwCtrlType) {
      case CTRL_C_EVENT:
      case CTRL_SHUTDOWN_EVENT:
      case CTRL_CLOSE_EVENT:
      case CTRL_BREAK_EVENT:
        exit(1);
        return TRUE;
      case CTRL_LOGOFF_EVENT:
      default:
        return FALSE;
    }
}

void start_sighandler()
{
BOOL status;
static CHAR *fid = "start_sighandler";

    status = SetConsoleCtrlHandler(
        (PHANDLER_ROUTINE) signal_handler, TRUE
    );
    if (!status) {
        logit("t", "%s: SetConsoleCtrlHandler failed: error %lu\n",
            fid, GetLastError()
        );
        exit(1);
    }
}

#endif
