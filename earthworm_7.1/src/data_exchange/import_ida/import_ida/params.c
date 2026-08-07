/*======================================================================
 *
 *  Read import_ida.d configuration file
 *
 *====================================================================*/
#include "import_ida.h"

#define DELIMITERS " \t"
#define MAX_TOKEN    3

extern struct params *Par;
extern unsigned char ModId;
extern char *ProgName;

static int fail(fp, path, token, lineno, retval)
FILE *fp;
char *path;
char *token;
int lineno;
int retval;
{
    fprintf(stderr, "%s: FATAL ERROR: syntax error at line ", ProgName);
    fprintf(stderr, "%d, file `%s', token `%s'\n", lineno, path, token);
    fclose(fp);
    return retval;
}

long read_params(char *path, char *buffer, int len)
{
int i;
long key;
FILE *fp;
int status, lineno, ntok;
char *token[MAX_TOKEN];

/* Open configuration file */

    if ((fp = fopen(path, "r")) == (FILE *) NULL) {
        fprintf(stderr, "%s: FATAL ERROR: fopen: ", ProgName);
        perror(path);
        return -1;
    }

/* Initialize parameter structure */

    Par->MyModName = (char *) 0;
    Par->OutRing   = (char *) 0;
    Par->PinnoFile = (char *) 0;
    Par->Network   = DEFAULT_NETWORK;
    Par->defpinno  = DEFAULT_PINNO;
    Par->hbeat     = DEFAULT_HBEAT;
    Par->maxlag    = DEFAULT_MAXLAG;
    Par->policy    = MINIMAL_DELAY;
    Par->nserver   = 0;

    lineno = 0;
    while ((status = util_getline(fp, buffer, len, '#', &lineno)) == 0) {

        ntok = util_parse(buffer, token, DELIMITERS, MAX_TOKEN, 0);

        if (strcasecmp(token[0], "MyModuleId") == 0) {
            if (ntok != 2) return fail(fp,path,token[0],lineno,-2);
            if ((Par->MyModName = strdup(token[1])) == (char *) NULL) {
                fprintf(stderr, "%s: FATAL ERROR: ", ProgName);
                perror("strdup");
                fclose(fp);
                return -3;
            }
            if (GetModId(Par->MyModName, &ModId) != 0) {
                fprintf(stderr, "%s: FATAL ERROR: unknown ",ProgName);
                fprintf(stderr, "ModuleId `%s'\n", Par->MyModName);
                fclose(fp);
                return -4;
            }

        } else if (strcasecmp(token[0], "OutRing") == 0) {
            if (ntok != 2) return fail(fp,path,token[0],lineno,-5);
            if ((Par->OutRing = strdup(token[1])) == (char *) NULL) {
                fprintf(stderr, "%s: FATAL ERROR: ", ProgName);
                perror("strdup");
                fclose(fp);
                return -6;
            }

        } else if (strcasecmp(token[0], "Network") == 0) {
            if (ntok != 2) return fail(fp,path,token[0],lineno,-7);
            if ((Par->Network = strdup(token[1])) == (char *) NULL) {
                fprintf(stderr, "%s: FATAL ERROR: ", ProgName);
                perror("strdup");
                fclose(fp);
                return -8;
            }

        } else if (strcasecmp(token[0], "HeartBeatInterval") == 0) {
            if (ntok != 2) return fail(fp,path,token[0],lineno,-9);
            Par->hbeat = atoi(token[1]);
            if (Par->hbeat < 1) {
                fprintf(stderr, "%s: FATAL ERROR: illegal ", ProgName);
                fprintf(stderr, "HeartBeatInterval `%s'\n", token[1]);
                fclose(fp);
                return -10;
            }

        } else if (strcasecmp(token[0], "PinnoFile") == 0) {
            if (ntok != 2) return fail(fp,path,token[0],lineno,-11);
            if ((Par->PinnoFile= strdup(token[1])) == (char *) NULL) {
                fprintf(stderr, "%s: FATAL ERROR: ", ProgName);
                perror("strdup");
                fclose(fp);
                return -12;
            }

        } else if (strcasecmp(token[0], "DefaultPinno") == 0) {
            if (ntok != 2) return fail(fp,path,token[0],lineno,-13);
            Par->defpinno = atoi(token[1]);

        } else if (strcasecmp(token[0], "Policy") == 0) {
            if (ntok != 2) return fail(fp,path,token[0],lineno,-14);
            if (strcasecmp(token[1], "MINIMAL_GAP") == 0) {
                Par->policy = MINIMAL_GAP;
            } else if (strcasecmp(token[1], "MINIMAL_DELAY") == 0) {
                Par->policy = MINIMAL_DELAY;
            } else {
                fprintf(stderr, "%s: FATAL ERROR: unknown ",ProgName);
                fprintf(stderr, "Policy `%s'\n", token[1]);
                fclose(fp);
                return -15;
            }

        } else if (strcasecmp(token[0], "MaximumLag") == 0) {
            if (ntok != 2) return fail(fp,path,token[0],lineno,-13);
            Par->maxlag = atoi(token[1]);
            if (Par->maxlag < 0) {
                fprintf(stderr, "%s: FATAL ERROR: illegal ", ProgName);
                fprintf(stderr, "MaximumLag `%s'\n", token[1]);
                fclose(fp);
                return -10;
            }

        } else if (strcasecmp(token[0], "server") == 0) {
            if (ntok != 3) return fail(fp,path,token[0],lineno,-16);
            if (Par->nserver == MAX_SERVERS) {
                fprintf(stderr, "%s: FATAL ERROR: increase MAX_SERVERS",
                    ProgName
                );
                fclose(fp);
                return -17;
            }
            if (strlen(token[1]) > MAX_SERVER_NAMELEN) {
                fprintf(stderr, "%s: INCREASE MAX_SERVER_NAMELEN\n", ProgName);
                fclose(fp);
                return -18;
            }
            strcpy(Par->server[Par->nserver].name, token[1]);
            if (strlen(token[2]) > MAX_SERVER_SCLEN) {
                fprintf(stderr, "%s: INCREASE MAX_SERVER_SCLEN\n", ProgName);
                fclose(fp);
                return -19;
            }
            strcpy(Par->server[Par->nserver].sc, token[2]);
            ++Par->nserver;

        } else {
            fprintf(stderr, "%s: FATAL ERROR: unrecognized ", ProgName);
            fprintf(stderr, "token `%s'\n", token[0]);
            fclose(fp);
            return -20;
        }
    }

    fclose(fp);

    if (status != 1) {
        fprintf(stderr, "%s: FATAL ERROR: %s: ", ProgName, path);
        perror("read");
        return -21;
    }
    if (Par->MyModName == (char *) 0) {
        fprintf(stderr, "%s: FATAL ERROR: %s: ", ProgName, path);
        fprintf(stderr, "missing MyModName\n");
        return -22;
    }
    if (Par->OutRing   == (char *) 0) {
        fprintf(stderr, "%s: FATAL ERROR: %s: ", ProgName, path);
        fprintf(stderr, "missing OutRing\n");
        return -23;
    }
    /**************************
     **** Pin# FUNCTIONALITY REMOVED PER Alex Bittenbinder 04/20/2005
    if (Par->PinnoFile == (char *) 0) {
        fprintf(stderr, "%s: FATAL ERROR: %s: ", ProgName, path);
        fprintf(stderr, "missing PinnoFile\n");
        return -24;
    }
     **************************/
    if (Par->nserver < 1) {
        fprintf(stderr, "%s: FATAL ERROR: %s: ", ProgName, path);
        fprintf(stderr, "no server(s) specified\n");
        return -25;
    }
    /**************************
     **** Pin# FUNCTIONALITY REMOVED PER Alex Bittenbinder 04/20/2005
    if (read_pinnos(Par->PinnoFile) < 0) {
        fprintf(stderr, "%s: FATAL ERROR: %s: ", ProgName, Par->PinnoFile);
        fprintf(stderr, "unable to load pin number lookup table\n");
        return -26;
    }
    **************************/
    if ((key = GetKey(Par->OutRing)) < 0) {
        fprintf(stderr, "%s: FATAL ERROR: unknown ",ProgName);
        fprintf(stderr, "OutRing `%s'\n", Par->OutRing);
        fclose(fp);
        return -27;
    }

    for (i = 0; i < Par->nserver; i++) Par->server[i].policy = Par->policy;

    return key;
}

void log_params(char *fname, long key)
{
char *PolicyString;
int i, minimalDelay = 0;

    if (Par->policy == MINIMAL_DELAY) {
        PolicyString = "MINIMAL_DELAY";
        minimalDelay = 1;
    } else if (Par->policy == MINIMAL_GAP) {
        PolicyString = "MINIMAL_GAP";
    } else {
        PolicyString = "UNKNOWN";
    }


    logit("t", "%s %s\n", ProgName, VERSION_STRING);
    logit("t", "Parameter file %s loaded\n", fname);
    logit("t", "MyModuleId        %s (%d)\n", Par->MyModName, ModId);
    logit("t", "OutRing           %s (%d)\n", Par->OutRing, key);
    logit("t", "PinnoFile         %s\n", Par->PinnoFile);
    logit("t", "DefaultPinno      %d\n", Par->defpinno);
    logit("t", "Policy            %s\n", PolicyString);
    if (minimalDelay) {
        logit("t", "MaximumLag        %d\n", Par->maxlag);
    }
    logit("t", "Network           %s\n", Par->Network);
    for (i = 0; i < Par->nserver; i++) {
        logit("t", "server %s %s\n",
            Par->server[i].name, Par->server[i].sc
        );
    }
}

#ifdef DEBUG_TEST

#define BUFFER_LEN 1024

char *ProgName;
struct params *Par;
SHM_INFO *Region;
unsigned char InstId;
unsigned char ModId;

main(argc, argv)
int argc;
char *argv[];
{
long key;
int i;
char buffer[BUFFER_LEN];
struct params par;
char *policy;
int minimalDelay = 0;
SHM_INFO region;

    ProgName = argv[0];
    Par      = &par;
    Region   = &region;

    if (argc != 2) exit(-1);

    key = read_params(argv[1], buffer, BUFFER_LEN);
    if (key < 0) {
        fprintf(stderr, "%s: read_params failed: status %d\n", argv[0], key);
        exit(1);
    }

    if (Par->policy == MINIMAL_DELAY) {
        policy = "MINIMAL_DELAY";
        minimalDelay = 1;
    } else if (Par->policy == MINIMAL_GAP) {
        policy = "MINIMAL_GAP";
    } else {
        policy = "UNKNOWN";
    }

    printf("MyModuleId        %s (%d)\n", Par->MyModName, ModId);
    printf("OutRing           %s (%d)\n", Par->OutRing, key);
    printf("HeartBeatInterval %ld\n",     Par->hbeat);
    printf("PinnoFile         %s\n",      Par->PinnoFile);
    printf("DefaultPinno      %d\n",      Par->defpinno);
    printf("Policy            %s\n",      policy);
    if (minimalDelay) {
        printf("MaximumLag        %d\n",      Par->maxlag);
    }
    printf("Network           %s\n",      Par->Network);

    for (i = 0; i < Par->nserver; i++) {
        printf("server %s %s\n", Par->server[i].host, Par->server[i].sc);
    }

    printf("\n");
    list_pinnos();

    printf("\n");
    /*printf("pinno(tau, bhz, ii) = %d\n", pinno("tau", "bhz", "ii"));
    printf("pinno(abc, bhz, ii) = %d\n", pinno("abc", "bhz", "ii"));*/

    exit(0);
}

#endif /* DEBUG_TEST */
