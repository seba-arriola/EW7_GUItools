/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: arc2trigII.h,v 1.4 2001/07/01 21:55:12 davidk Exp $
 *
 *    Revision history:
 *     $Log: arc2trigII.h,v $
 *     Revision 1.4  2001/07/01 21:55:12  davidk
 *     Cleanup of the Earthworm Database API and the applications that utilize it.
 *     The "ewdb_api" was cleanup in preparation for Earthworm v6.0, which is
 *     supposed to contain an API that will be backwards compatible in the
 *     future.  Functions were removed, parameters were changed, syntax was
 *     rewritten, and other stuff was done to try to get the code to follow a
 *     general format, so that it would be easier to read.
 *
 *     Applications were modified to handle changed API calls and structures.
 *     They were also modified to be compiler warning free on WinNT.
 *
 *     Revision 1.3  2001/05/15 02:15:05  davidk
 *     Moved functions around between the apps, DB API, and DB API INTERNAL
 *     levels.  Renamed functions and files.  Added support for amplitude
 *     magnitude types.  Reformatted makefiles.
 *
 *     Revision 1.2  2001/05/01 19:40:35  davidk
 *     removed redundant function prototypes for read_hyp() and read_phs().
 *
 *     Revision 1.1  2001/03/07 00:05:04  alex
 *     Initial revision
 *
 *     Revision 1.1  2000/02/14 16:04:49  lucky
 *     Initial revision
 *
 *
 */

/*
 * arc2trigII.h : Include file for arc2trigII.c;
 */

#define MAX_STR			255
#define SHORT_STR		64
#define MEDIUM_STR		(SHORT_STR*2)
#define MAX_MAGDIST		10		/* max number of mag-distance entries */
#define MAX_MAND_STA	100		/* max number of mandatory SCN's */
#define MAX_STA			5000	/* the most stations we can handle */

/* Things read from config file
 ******************************/
char    OutputDir[SHORT_STR]; /* directory to write "triggers" to  */ 
char    TrigFileBase[SHORT_STR/2]; /* prefix of trigger file name  */

/* The big-deal data structures
*******************************/
typedef struct
{
	char		sta[TRACE_STA_LEN];
	char		comp[TRACE_CHAN_LEN];
	char		net[TRACE_NET_LEN];
} SCN_NAMES;

typedef struct
{
	int			selected;
	double		startTime;
	double		duration;
} SEL_STA;

/* Prototypes of functions in arc2trigII.c
 *********************/
int sonOfSiteRead( char* filename );
int arc2trigII_hinvarc( char* arcmsg, char* trigMsg, MSG_LOGO incoming_logo );
int phs2SelSta();
int MagDist2SelSta( );
int SetTimesInSelSta();
int SelSta2TrigMsg( char* trigMsg );
double tau( double xmag );
int MandSta2Selected( );
int match( char* sta, char* comp, char* net, EWDB_StationStruct* staStruct);
void arc2trig_config( char *configfile );
void arc2trig_lookup( void );
void arc2trig_status( unsigned char type, short ierr, char *note );
double Stime( double ot, double depth, double dist);
double Ptime( double ot, double depth, double dist);
int readTTval(double* val);
void bldtrig_hyp( char *trigmsg, MSG_LOGO incoming, MSG_LOGO outgoing);
