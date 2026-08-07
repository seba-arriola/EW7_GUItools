/*======================================================================
 *
 * Pin number lookups.
 *
 *====================================================================*/
#include "import_ida.h"

#define DELIMITERS " \t"
#define MIN_TOKEN    5
#define MAX_TOKEN    21
#define BUFLEN       256

extern char *ProgName;
extern struct params *Par;

static int nentry = 0;
static struct lookup_table {
    char sname[TRACE2_STA_LEN];
    char cname[TRACE2_CHAN_LEN];
    char nname[TRACE2_NET_LEN];
    char lname[TRACE2_LOC_LEN];
    int pinno;
} *entry;

int read_pinnos(char *path)
{
FILE *fp;
int i, status, lineno, ntok;
char buffer[BUFLEN], *token[MAX_TOKEN];

/* Open data file */

    if ((fp = fopen(path, "r")) == (FILE *) NULL) {
        fprintf(stderr, "%s: FATAL ERROR: fopen: ", ProgName);
        perror(path);
        return -1;
    }

/* Read once to count how many entries */

    lineno = 0;
    nentry = 0;
    while ((status = util_getline(fp, buffer, BUFLEN, '#', &lineno)) == 0) {
        if (strncmp(buffer, "//", 2) == 0) continue;
        ntok = util_parse(buffer, token, DELIMITERS, MAX_TOKEN, 0);
        if (ntok < MIN_TOKEN) continue;
        ++nentry;
    }

    if (nentry == 0) {
        fprintf(stderr, "%s: FATAL ERROR: %s: ", ProgName, path);
        fprintf(stderr, "no valid entries found\n");
        return -2;
    }

/* Allocate space for everything and read it in */

    entry = (struct lookup_table *) malloc(nentry*sizeof(struct lookup_table));
    if (entry == (struct lookup_table *) NULL) {
        fprintf(stderr, "%s: FATAL ERROR: ", ProgName);
        perror("malloc");
        return -3;
    }

    i = 0;
    rewind(fp);
    lineno = 0;
    while ((status = util_getline(fp, buffer, BUFLEN, '#', &lineno)) == 0) {
        if (strncmp(buffer, "//", 2) == 0) continue;
        ntok = util_parse(buffer, token, DELIMITERS, MAX_TOKEN, 0);
        if (ntok < MIN_TOKEN) continue;

        if (i == nentry) {
            fprintf(stderr, "%s: FATAL ERROR: ", ProgName);
            fprintf(stderr, "logic error 1\n");
            return -4;
        }

        entry[i].pinno =   atoi(token[1]);

		if (strlen(token[2]) > TRACE2_STA_LEN-1) {
			fprintf(stderr, "%s: station name %s too long!\n",
				ProgName, token[2]
			);
			return -5;
		} else {
        	strcpy(entry[i].sname, token[2]);
		}

		if (strlen(token[3]) > TRACE2_NET_LEN-1) {
			fprintf(stderr, "%s: network name %s too long!\n",
				ProgName, token[3]
			);
			return -6;
		} else {
        	strcpy(entry[i].nname, token[3]);
		}

		if (strlen(token[4]) > TRACE2_CHAN_LEN-1) {
			fprintf(stderr, "%s: channel name %s too long!\n",
				ProgName, token[4]
			);
			return -7;
		} else {
        	strcpy(entry[i].cname, token[4]);
		}

		if (strlen(token[5]) > TRACE2_LOC_LEN-1) {
			fprintf(stderr, "%s: location name %s too long!\n",
				ProgName, token[5]
			);
			return -7;
		} else {
        	strcpy(entry[i].lname, token[5]);
		}

        ++i;
    }

    if (i != nentry) {
        fprintf(stderr, "%s: FATAL ERROR: ", ProgName);
        fprintf(stderr, "logic error 2\n");
        return -8;
    }

#ifdef DEBUG
    printf("%s: %d pinnos found\n", ProgName, nentry);
#endif
    return nentry;
}

void set_pinno(TracePacket *trace)
{
int i;

    for (i = 0; i < nentry; i++) {
        if (
            strcasecmp(trace->trh2.net,  entry[i].nname) == 0 &&
            strcasecmp(trace->trh2.sta,  entry[i].sname) == 0 &&
            strcasecmp(trace->trh2.chan, entry[i].cname) == 0 &&
            strcasecmp(trace->trh2.loc,  entry[i].lname) == 0 
        ) {
            trace->trh2.pinno = entry[i].pinno;
            return;
        }
    }

    trace->trh2.pinno = Par->defpinno;
}

#ifdef DEBUG_TEST

int list_pinnos()
{
int i;

    for (i = 0; i < nentry; i++) {
        printf("%2d: %4d %4s %3s %2s\n",
            i, entry[i].pinno,
            entry[i].sname, entry[i].cname, entry[i].nname
        );
    }

    return nentry;
}

#endif /* DEBUG_TEST */
