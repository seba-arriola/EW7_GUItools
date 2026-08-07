/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: concierge.h,v 1.2 2002/07/16 20:48:25 davidk Exp $
 *
 *    Revision history:
 *     $Log: concierge.h,v $
 *     Revision 1.2  2002/07/16 20:48:25  davidk
 *     Added prototype for ewdb_apps_PreprocessSnippetReqs().
 *
 *     Revision 1.1  2002/07/16 19:30:35  davidk
 *     Initial revision
 *
 */


#define MAX_STR			255
#define SHORT_STR		64
#define MEDIUM_STR		(SHORT_STR*2)
#define MAX_MAGDIST		10		/* max number of mag-distance entries */
#define MAX_MAND_STA	100		/* max number of mandatory SCN's */
#define MAX_STA			5000	/* the most stations we can handle */
#define EARTH_CIRCUM    40000.0
#define NZMAX           100
#define NDMAX           100




/* The big-deal data structures
*******************************/
/*typedef struct
{
	char		sta[TRACE_STA_LEN];
	char		comp[TRACE_CHAN_LEN];
	char		net[TRACE_NET_LEN];
} SCN_NAMES;
*/

typedef struct
{
	int			selected;
	double		startTime;
	double		duration;
} SEL_STA;


typedef struct trace_req_opts
{

	int  		iRequestGroup;/* tdefault "Request Group" */
	int  		iNumAttempts; 	/* number of attempts to schedule a request for.  */
	int			StalistFromDB;	/* Get the station list from DB */

	/* Box from which to get the station list */
	double		StaLatMax;
	double		StaLatMin;
	double		StaLonMax;
	double		StaLonMin;

	double		FixedDuration;

	char		StationListFile[MAX_STR];	/* hypoinverse station list file */

	/* Save the following snippets no matter what */
	int			numMand;
	EWDB_StationStruct	MandSta[MAX_MAND_STA];

	/* Save all snippets within this distance for this magnitude */
	int			numMagDist;			/* number of mag/distance table entries */
	double		Mag[MAX_MAGDIST];
	double		Dist[MAX_MAGDIST];

	double		SaveOriginDist;		/* save everything closer than this */
	double		PrePickTime;		/* save this many seconds before the pick */
	double		PostPickTime;		/* save this many seconds after the pick */

	/* Travel time table options */
	int			numZ;			/* number of depth values in the table */
	int			numDist;		/* number of distance values in the table */
	double		deltaZ;			/* depth increment between table values */
	double		deltaDist;		/* distance increment between table values */
	double		STT[NDMAX][NZMAX];	/* The S travel times */
	double		PTT[NDMAX][NZMAX];	/* The S travel times */

} TraceRequestOptions;

/* Prototypes of functions in concierge_api_utils.c
 *********************/

int    DetermineSnippetRequests(EWEventInfoStruct *pEvent, EWDB_SnippetRequestStruct *pSnippet,
                                TraceRequestOptions *pOptions, int NumSnipAlloc, int *NumSnipRet);

int     RunConciergeConfig (TraceRequestOptions *pOptions);
int     InitConciergeConfig (TraceRequestOptions *pOptions);
int     ewdb_apps_PreprocessSnippetReqs(EWDB_SnippetRequestStruct * pRequest);

