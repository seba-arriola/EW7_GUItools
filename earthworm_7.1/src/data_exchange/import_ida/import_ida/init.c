/*======================================================================
 *
 * Initialization routine
 *
 *====================================================================*/
#include "import_ida.h"

/* Globals */

extern char *ProgName;
extern struct params *Par;
extern SHM_INFO *Region;
extern unsigned char ModId;
extern unsigned char InstId;

#define BUFLEN 1024

void init(int argc, char **argv)
{
long key;
int status, buflen = BUFLEN;
char buffer[BUFLEN];

	IdatapInit();

/* Get InstId from the environment */

    if ((status = GetLocalInst(&InstId)) < 0) {
        fprintf(stderr, "%s: FATAL ERROR: GetLocalInst error %d\n",
            ProgName, status
        );
        exit(1);
    }

/* Require the name of the parameter file as the single argument */

    if (argc != 2) {
        fprintf(stderr, "usage: %s param_file\n", ProgName);
        fprintf(stderr, "%s: FATAL ERROR: illegal comand line\n",
            ProgName
        );
        exit(1);
    }

/* Load the parameter file */

    if ((key = read_params(argv[1], buffer, buflen)) < 0) {
        fprintf(stderr, "%s: FATAL ERROR: read_params error %d\n",
            ProgName, key
        );
        exit(1);
    }

/* Start the logging facility */

    logit_init(argv[1], ModId, 1024, 1);
    log_params(argv[1], key);

/* Start signal handler */

    start_sighandler();

/* Attach to the output ring */

    tport_attach(Region, key);
    logit("t", "attached to %s (%d) transport ring\n", Par->OutRing, key);
}
