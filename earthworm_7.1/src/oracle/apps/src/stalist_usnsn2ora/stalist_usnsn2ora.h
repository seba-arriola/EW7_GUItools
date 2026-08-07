
#define	MAXLEN	10

typedef struct neic2scn
{

    char    neic_sta[MAXLEN];
    char    sta[MAXLEN];
    char    comp[MAXLEN];
    char    net[MAXLEN];

} NEIC2SCN;

/* Function prototypes
   ******************/

int IsComment( char string[] );
int  MatchNeic2SCN (char *neic_sta, char *sta, char *comp, 
                    char *net, NEIC2SCN *neic2scn, int nSta);
int GetNEICStaList (NEIC2SCN **Sta, int *Nsta, char *filename);
int		GetNextLine (int *lineno, char *line, FILE *fp);
