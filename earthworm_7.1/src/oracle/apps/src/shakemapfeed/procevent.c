/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: procevent.c,v 1.20 2006/02/16 01:07:29 lombard Exp $
 *
 *    Revision history:
 *     $Log: procevent.c,v $
 *     Revision 1.20  2006/02/16 01:07:29  lombard
 *     Fixed typo in staDist declaration.
 *
 *     Revision 1.19  2006/02/15 22:39:03  lombard
 *     Added station netid, location code to XML.
 *     Changed distance calculation dist() to match the algorithm used
 *     by ShakeMap.
 *
 *     Revision 1.18  2004/03/04 19:29:24  dietz
 *     Increased the sizes of the EVENTLIST arrays
 *
 *     Revision 1.17  2004/03/04 18:09:10  dietz
 *     Changed the auto-feeding scheduling configuration to allow for
 *     variable time intervals between feeds to shakemap.  Added new
 *     internal function NextScheduledUpdate().
 *
 *     Revision 1.16  2004/02/13 21:56:56  dietz
 *     minor comment tweak
 *
 *     Revision 1.15  2004/02/13 19:38:27  dietz
 *     Added new command, SMQueryMethod to control how data is
 *     requested from the DBMS. Added post-query filtering on
 *     SM data's eventid, external eventid and external author to
 *     allow data assoc with foreign networks' events to be included
 *     in local event shakemaps.
 *
 *     Revision 1.14  2004/02/05 22:28:34  dietz
 *     In initMappings() sscanf on station name mapping, changed format
 *     statement for long station name from %20c to %50c
 *
 *     Revision 1.13  2003/10/30 17:42:40  lombard
 *     Changed the sorting of SM messages used by Filter_SM_data: now sort by
 *     SCNL. This means that components of a given station will appear consecuatively
 *     in output XML, a big help for shakemap.
 *
 *     Revision 1.12  2003/06/09 21:37:45  lombard
 *     Last two parameters to the two ewdb_api_GetSMDataWithChannelInfo calls were
 *     reversed. That meant that the number of SM messages returned by these calls
 *     would be erroneous IF that number exceeded MaxDataPerEvent. That would result
 *     in out-of-bounds memory access and corrupted SM data.
 *
 *     Revision 1.11  2003/01/30 23:12:57  lucky
 *     *** empty log message ***
 *
 *     Revision 1.10  2002/04/12 22:28:27  dietz
 *     Added switch to enable either binder's eventid or the DBMS eventid to
 *     be written to the output files.  Changed logging to include both ids
 *     where applicable.
 *
 *     Revision 1.9  2001/09/26 21:36:25  lucky
 *     Fixed a bug in new_sta where pA and pS were not being properly
 *     managed. This resulted in bad XML code being passed to ShakeMap
 *     which could not process the resulting file.
 *
 *     Revision 1.8  2001/07/01 21:55:34  davidk
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
 *     Revision 1.7  2001/05/17 19:54:11  dietz
 *     Fixed station XML file to write tags acc and vel instead of pga and pgv
 *
 *     Revision 1.6  2001/05/15 02:15:45  davidk
 *     Moved functions around between the apps, DB API, and DB API INTERNAL
 *     levels.  Renamed functions and files.  Added support for amplitude
 *     magnitude types.  Reformatted makefiles.
 *
 *     Revision 1.5  2001/05/01 20:36:24  dietz
 *     Changed to work with strongmotionII stuff
 *
 *     Revision 1.4  2001/04/02 23:07:56  lombard
 *     corrected format in station line of XML writer.
 *
 *     Revision 1.3  2001/03/22 18:14:39  lombard
 *     added XML writer; added functions for finding station long names,
 *     instrument descriptions and agency names.
 *
 *     Revision 1.2  2001/03/15 00:01:19  dietz
 *     *** empty log message ***
 *
 *     Revision 1.1  2000/02/15 20:20:15  lucky
 *     Initial revision
 *
 *
 */

/*
 * procevent.c  processes an event for shakemapfeed
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <math.h>
#include <time_ew.h>
#include <ewdb_ora_api.h>
#include <ewdb_apps_utils.h>
#include "shakemapfeed.h"
#include "shake_dtd.h"

/* Define some simple macros:
   **************************/
#ifndef MAX
#define MAX(a,b) (((a)>(b)) ? (a) : (b))        /* Larger value   */
#endif
#ifndef MIN
#define MIN(a,b) (((a)<(b)) ? (a) : (b))        /* Smaller value  */
#endif
#ifndef ABS
#define ABS(a) ((a) > 0 ? (a) : -(a))           /* Absolute value */
#endif

static int   Init   = 0;       /* initialization flag               */

#define TT_ERROR           5.  /* error (sec) in travel-time calc,  */
                               /* due to simplification of velocity */
#define TIME_POST_ARRIVAL 60.  /* expect peak motion is within this */
                               /* many sec of last arrival          */
#define VRAYLEIGH_VS    0.92   /* Vsurfacewave over Vswave          */
/* Note: Surface waves are slower than S and may dominate seismograms
   at larger distances.  Rayleigh waves go ~0.92Vs (slower than Love
   waves) and should be the last things of concern to shakemap */

/* List of most recent events we've seen */
#define MAXOTHREQS 1000                 /* max # eqs in backup eq list       */
static EVENTLIST   OthrEqs[MAXOTHREQS]; /* list of eqs that we're working on */
static int         nOthrEqs;            /* cumulative number of eqs in list  */

/* List of events that we want to feed to shakemap */
#define MAXSHKEQS 250                  /* max # eqs in working list         */
static EVENTLIST  ShkEqs[MAXSHKEQS];   /* list of eqs that we're working on */
static int        nShkEqs;             /* cumulative number of eqs in list  */

/* Define UseFlag values
 ***********************/
#define SM_USE                1
#define SM_REJECT_DUPLICATE  -1
#define SM_REJECT_TIME       -2
#define SM_REJECT_COMPONENT  -3
#define SM_REJECT_WRONGEQ    -4

static float  Period1;         /* 1st interesting spectral period (s) */
static float  Period2;         /* 2nd interesting spectral period (s) */
static float  Period3;         /* 3rd interesting spectral period (s) */

static double MaxTimeTolerance; /* maximum of configured time tolerances */
static char   Note[NOTELEN];    /*string for writing errors,pages,etc */
static char   ShkFilePath[PATHLEN]; /* full path for file of most recent processing list */

static EWDB_SMChanAllStruct *psm;      /* strong motion data from dbms         */
static int                  *puseflag; /* to flag data as not usable/notusable */

/* Items for various mappings needed to write XML files for shakemapII */
static int nAgency    = 0;  /* Number of Agency table entries       */
static int nLSN       = 0;  /* Number of Station Name table entries */
static int nInstTypes = 0;  /* Number of Instr. Type table entries  */
static AGENCY   *pAgency;    /* The Agency mapping table             */
static LONGSTANAME *pLSN;    /* The long station name mapping table  */
static INSTTYPE   *pInst;    /* The instrument type mapping table    */

static AGENCY      *pA;  /* ptr to the current entry in the Agency table */
static LONGSTANAME *pS;  /* ptr to the current entry in the long station name table */

/* Local function prototypes 
 ***************************/
EVENTLIST *UpdateShkList( EVENTLIST *eqs );
EVENTLIST *UpdateOthrList( EVENTLIST *eqs );
EVENTLIST *FindEventInShkList( long qid ); 
EVENTLIST *FindEventInOthrList( long qid ); 
int    InitEvent( PARAM *parm, EWD *ewd );
int    WriteShkListFile( void );
int    ReadShkListFile( void );
int    QualifyEvent( PARAM *parm, EVENTLIST *eqs );
void   DeleteEvent( PARAM *parm, EWD *ewd, EVENTLIST *eqs );
time_t NextScheduledUpdate( PARAM *parm, EVENTLIST *eqs );
int    Filter_SM_data ( PARAM *parm, EVENTLIST *eqs,
                      EWDB_SMChanAllStruct *sm, int *useflag, int nsmdata );
int    Build_SM_file( PARAM *parm, EVENTLIST *eqs, 
                      EWDB_SMChanAllStruct *sm, int *useflag, int nsmdata );
void   logthisfile( char *fname );
double dist( float lat1, float lon1, float lat2, float lon2 );
int    CompareTime( const void *s1, const void *s2 );
int    CompareChanT( const void *s1, const void *s2 ); /*for qsort*/
int    isBlockComp( char *comp,  PARAM *parm );
int    inExpectedTimeRange( EWDB_SMChanAllStruct *sm, PARAM *parm, 
                            EVENTLIST *eqs, double ttolerance );
double NetTimeTolerance( char *net, PARAM *parm );
int    CompareAgency( const void *, const void *);
int    CompareLSN( const void *, const void *);
int    CompareInstType( const void *, const void *);
void   new_sta( EWDB_SMChanAllStruct *, int *_open, char *, FILE *, INSTTYPE *,
		EVENTLIST *eqs);
void   close_sta( FILE * );

/******************************************************************************
 * InitEvent()  initialize static globals - do only once                      *
 ******************************************************************************/
int InitEvent( PARAM *parm, EWD *ewd )
{
   char *shkfilename = "shklist";
   char *str;
   int   i;

/* initialize event lists 
 ************************/
   memset( OthrEqs, 0, sizeof(EVENTLIST)*MAXOTHREQS );  
   nOthrEqs = 0;  
   memset( ShkEqs,  0, sizeof(EVENTLIST)*MAXSHKEQS  );  
   nShkEqs = 0;  

/* Read in previous shakemap events from disk file
 *************************************************/
   if( (str = getenv("EW_PARAMS")) == NULL )
   {  
      logit("e", "shakemapfeed: environment variable EW_PARAMS not defined\n");
      return( INIT_FATAL );
   }  
   if( strlen(str) + strlen(shkfilename) + 4 >= PATHLEN )
   {  
      logit("e", "InitEvent: environment variable EW_PARAMS too long;"
              " increase PATHLEN and recompile\n");
      return( INIT_FATAL );
   }
   sprintf( ShkFilePath, "%s/%s%u", str, shkfilename, (unsigned int)ewd->MyModId );
  
   if( ReadShkListFile() != 0 ) {
      logit( "e", "InitEvent: error loading processing list from"
             " previous instance (%s); continuing.\n", ShkFilePath ); 
   }

/* allocate working space for strong motion data
 ***********************************************/
   psm = (EWDB_SMChanAllStruct *) calloc( parm->MaxDataPerEvent, 
                                          sizeof(EWDB_SMChanAllStruct) );
   if( psm == NULL ) {
      logit( "et", "shakemapfeed: trouble allocating structure for %d channels"
             " of strongmotion data\n", parm->MaxDataPerEvent ); 
      return( INIT_FATAL );
   }
   puseflag = (int *) calloc( parm->MaxDataPerEvent, sizeof(int) );
   if( puseflag == NULL ) {
      logit( "et", "shakemapfeed: trouble allocating useflag array for %d channels"
             " of strongmotion data\n", parm->MaxDataPerEvent ); 
      return( INIT_FATAL );
   }

/* Initialize interesting spectral periods
 *****************************************/
   Period1 = (float)0.3;
   Period2 = (float)1.0;
   Period3 = (float)3.0;

/* Initialize other globals
 **************************/
   MaxTimeTolerance = parm->DefaultTolerance;
   for( i=0; i<parm->nNetSlop; i++ ) {
      if( parm->NetSlop[i].ttolerance > MaxTimeTolerance ) 
         MaxTimeTolerance = parm->NetSlop[i].ttolerance;
   }

/* Sort and log Feeding Schedule
 *******************************/
   qsort( parm->Schedule.tsched, parm->Schedule.nsched, 
          sizeof(time_t), CompareTime );
   i = parm->Schedule.nsched - 1;
   if( parm->UpdateDuration < parm->Schedule.tsched[i] ) {
      parm->UpdateDuration = parm->Schedule.tsched[i] + 60;
   }

   logit("","ScheduleFeed(minutes):", parm->Schedule.nsched );
   for( i=0; i<parm->Schedule.nsched; i++ ) {
       logit(""," %.2f", ((float)parm->Schedule.tsched[i])/60.);
   }
   logit("","\nUpdateDuration(minutes): %.2f\n", ((float)parm->UpdateDuration)/60. ); 

/* Make sure output directory exists
 ***********************************/
   if( CreateDir( parm->OutputDir ) != EW_SUCCESS ) 
   {
      logit( "et", "shakemapfeed: trouble creating output directory: %s\n",
              parm->OutputDir ); 
      return( INIT_FATAL );
   }
   if( chdir_ew( parm->OutputDir ) )       
   {
      logit( "et", "shakemapfeed: trouble in chdir to output directory: %s\n",
              parm->OutputDir ); 
      return( INIT_FATAL );
   }

/* Make sure working directory exists
 ************************************/
   if( CreateDir( parm->TempDir ) != EW_SUCCESS ) 
   {
      logit( "et", "shakemapfeed: trouble creating working directory: %s\n",
              parm->TempDir ); 
      return( INIT_FATAL );
   }

/* Initialize connection to DBMS 
 *******************************/
   if( ewdb_api_Init( parm->DBuser, parm->DBpassword, parm->DBservice ) != 
       EWDB_RETURN_SUCCESS )
   {
      logit( "et", "shakemapfeed: error connecting to "
              "DBMS service:%s as user:%s pswd:%s\n",
              parm->DBservice, parm->DBuser, parm->DBpassword ); 
      return( INIT_FATAL );
   }

   Init = 1;
   return( INIT_OK );
}


/******************************************************************************
 * ShutdownEvent()  free allocated memory, etc                                *
 ******************************************************************************/
void ShutdownEvent( PARAM *parm )
{
   WriteShkListFile();
   free( psm );
   free( puseflag );
   free( parm->BlockComp );
   free( parm->NetSlop   );
   ewdb_api_Shutdown();
   
   return;
}


/******************************************************************************
 * WriteShkListFile()  write processing list to disk file                     *
 ******************************************************************************/
int WriteShkListFile( void ) 
{
   FILE  *fp;
   int    ioldest;
   size_t nitems;
   size_t isize;

   fp = fopen( ShkFilePath, "wb" );
   if( fp == NULL ) {
      logit("e","WriteShkListFile: error opening ShkList file %s\n",
                 ShkFilePath );
      return( -1 );
   }

/* write sizeof(EVENTLIST)
 *************************/
   isize = sizeof(EVENTLIST);
   if( fwrite( &isize, sizeof(size_t), 1, fp ) != 1 ) {
      logit("e","WriteShkListFile: error writing EVENTLIST size:%d %s\n",
                 isize, ShkFilePath );
       return( -1 );
   }

/* Haven't filled ShkEqs yet, write only completed entries  
 **********************************************************/
   if( nShkEqs <= MAXSHKEQS )  { 
      nitems = nShkEqs;
      if( fwrite( ShkEqs, sizeof(EVENTLIST), nitems, fp ) != nitems ) {
         logit("e","WriteShkListFile: error writing %d events to %s\n",
                    nitems, ShkFilePath );
         return( -1 );
      }
   } 

/* Write the whole list, but "unwrap" list (write oldest event first) 
 ********************************************************************/
   else {              
      ioldest = nShkEqs%MAXSHKEQS;  
      nitems  = MAXSHKEQS -ioldest;
      if( fwrite( &(ShkEqs[ioldest]), sizeof(EVENTLIST), nitems, fp ) != nitems ) {
         logit("e","WriteShkListFile: error writing %d events to %s\n",
                    nitems, ShkFilePath );
         return( -1 );
      }
      nitems  = ioldest;
      if( fwrite( ShkEqs, sizeof(EVENTLIST), nitems, fp ) != nitems ) {
         logit("e","WriteShkListFile: error writing %d events to %s\n",
                    nitems, ShkFilePath );
         return( -1 );
      }
   }
   fclose( fp );
   return( 0 );
}

/******************************************************************************
 * ReadShkListFile()  read processing list from disk file                     *
 ******************************************************************************/
int ReadShkListFile( void ) 
{
   FILE *fp;
   size_t isize;

   fp = fopen( ShkFilePath, "rb" );
   if( fp == NULL ) {
      logit("e","ReadShkListFile: error opening ShkList file %s\n",
                 ShkFilePath );
      return( -1 );
   }

   fread( &isize, sizeof(size_t), 1, fp );
   if( isize != sizeof(EVENTLIST) ) {
      logit("e","ReadShkListFile: invalid EVENTLIST size:%d for this"
             " executable in file: %s\n", isize, ShkFilePath );
      return( -1 );
   }

   while( fread( &(ShkEqs[nShkEqs%MAXSHKEQS]), sizeof(EVENTLIST), 1, fp ) ) {
      nShkEqs++;
   }
   fclose( fp );
   return( 0 );
}


/******************************************************************************
 * ProcessArc() reads a TYPE_HYP2000ARC message.                              *
 *  If the event meets the configured "interesting event" criteria, it puts   *
 *    the event into the ShkEqs list and performs its first shakemapfeed      *
 *  If the event doesn't qualify as "interesting," it put it into the backup  *
 *    hypocenter list, OthrEqs (just in case we get a mag message later that  *
 *    makes it interesting.                                                   *
 ******************************************************************************/
int ProcessArc( PARAM *parm, EWD *ewd, char *arcmsg, long msglen )
{
   struct Hsum  hyp;
   EVENTLIST    neweq;
   EVENTLIST   *eqs;
   char         line[256],shdw[256];
   char        *in;
   time_t       now;
   int          rc;
   
   if( !Init ) 
   {
     int initrc;
     initrc = InitEvent( parm, ewd );
     if( initrc != INIT_OK ) return( initrc );
   }
   now = time(NULL);
   
/* Read summary line & shadow
 ****************************/
   in = arcmsg;
   if( sscanf( in, "%[^\n]", line ) != 1 )  return( -1 );
   in += strlen( line ) + 1;
   if( sscanf( in, "%[^\n]", shdw ) != 1 )  return( -1 );
   in += strlen( shdw ) + 1;
   read_hyp( line, shdw, &hyp );
   
/* Load event info, assuming it's a new event that will qualify
 **************************************************************/
   memset( &neweq, 0, sizeof(EVENTLIST) );  
   neweq.time      = hyp.ot - GSEC1970;
   neweq.lat       = hyp.lat;
   neweq.lon       = hyp.lon;
   neweq.depth     = hyp.z;
   neweq.nph       = hyp.nph;
   neweq.rms       = hyp.rms;
   neweq.id        = hyp.qid;
   sprintf( neweq.idstr, "%ld", hyp.qid );
   strcpy(  neweq.author, parm->QuakeAuthor );
   neweq.dbid      = 0;
   neweq.tlastdata = 0.0;
   if( hyp.wtpref ) {  /*use preferred magnitude */
     neweq.mag     = hyp.Mpref;
     neweq.magwt   = hyp.wtpref;
     sprintf( neweq.magtype, "M%c", hyp.labelpref );
   } else {            /* use coda magnitude */
     neweq.mag     = hyp.Md;
     neweq.magwt   = hyp.mdwt;
     sprintf( neweq.magtype, "M%c", hyp.mdtype );
   }
   neweq.tinlist = now;
   neweq.tdone   = (time_t)neweq.time + parm->UpdateDuration;
   neweq.tupdate = NextScheduledUpdate(parm, &neweq); /* don't copy in update */
   neweq.nfeed   = 1;                                 /* don't copy in update */

/* Process a qualifying hypocenter 
 *********************************/
   if( QualifyEvent(parm, &neweq) ) {   
      logit("t", "ProcessArc: binderID: %ld author: %s %s:%.2f qualifies:\nY %s\n",
             neweq.id, neweq.author, neweq.magtype, neweq.mag, line );
      eqs = UpdateShkList( &neweq );

   /* Force one feed for qualifying events whose UpdateDuration has 
    * already expired (these are probably manually-rerun events). 
    ***************************************************************/
      if( now > eqs->tdone  ||  eqs->tupdate > eqs->tdone ) {
         rc = SMfeedEvent( parm, ewd, eqs );
      }
   }

/* Hypocenter didn't qualify 
 ***************************/
   else {           
      printf("didn't qualify\n");
      logit("t", "ProcessArc: binderID: %d author: %s %s:%.2f does not qualify:\nN %s\n",
             neweq.id, neweq.author, neweq.magtype, neweq.mag, line );

   /* delete a previously qualifying event 
    **************************************/
      eqs = FindEventInShkList( neweq.id );
      if( eqs != NULL ) {  
         eqs = UpdateShkList( &neweq );
         logit("t","ProcessArc: binderID: %d author: %s updated - no longer qualifies for "
                "shakemap processing.\n", eqs->id, eqs->author );
         DeleteEvent( parm, ewd, eqs );
      }

   /* or just update the other event list 
    *************************************/
      else {  
         UpdateOthrList( &neweq );
      }
   }

   return( 0 );
}


/******************************************************************************
 * ProcessDelete()  processes a TYPE_EQDELETE msg                             *
 ******************************************************************************/
int ProcessDelete( PARAM *parm, EWD *ewd, char *delmsg, long msglen ) 
{
  EVENTLIST *eqs;
  long       qid;
  int        rc;

  if( !Init ) 
  {
    int initrc;
    initrc = InitEvent( parm, ewd );
    if( initrc != INIT_OK ) return( initrc );
  }

/* Read incoming TYPE_EQDELETE message 
 *************************************/
  rc = sscanf( delmsg, "%ld", &qid );
  if( rc != 1 ) {
    logit("t", "ProcessDelete: trouble reading binderID from TYPE_EQDELETE msg\n" );
    return( 0 );
  }

/* Find event in processing list & cancel it by resetting tdone
 **************************************************************/
  eqs = FindEventInShkList( qid );

  if( eqs != NULL ) {
    logit("t", "ProcessDelete: TYPE_EQDELETE for binderID: %ld\n", qid );
    DeleteEvent( parm, ewd, eqs );
  } else {
    logit("t", "ProcessDelete: TYPE_EQDELETE for binderID: %ld - "
          "not in processing list!\n", qid );
  }

  return( 0 );
}


/******************************************************************************
 * ProcessMag() Reads TYPE_MAGNITUDE message and updates the magnitude of     *
 *  the binderID given in the message.                                        *
 ******************************************************************************/
int ProcessMag( PARAM *parm, EWD *ewd, char *magmsg, long msglen ) 
{
  EVENTLIST *eqs;
  MAG_INFO   minfo; 
  long       qid;
  int        inshklist;
  int        qualify;
  int        rc;

  if( !Init ) 
  {
    int initrc;
    initrc = InitEvent( parm, ewd );
    if( initrc != INIT_OK ) return( initrc );
  }

/* Read incoming TYPE_MAGNITUDE message (sample between "") 
 * "40120489 MAG Ml 3.15 AVG 1 2 0.07 -1 -1.00 -1 014024003:UCB"
 ***************************************************************/
  memset( &minfo, 0, sizeof(MAG_INFO) ); 

  rc = rd_mag( magmsg, msglen, &minfo ); 
  if( rc != 0 ) {
    logit("t", "ProcessMag: trouble reading TYPE_MAGNITUDE msg\n" );
    return( 0 );
  }
  qid = atol( minfo.qid );
 
/* Find event in processing list or other list
 *********************************************/
  inshklist = 1;
  eqs = FindEventInShkList( qid );
  if( eqs == NULL ) {    /* not in shk list - try other list */ 
    inshklist = 0;
    eqs = FindEventInOthrList( qid );
    if( eqs == NULL ) {  /* not in either list - complain & return */
       logit("t", "ProcessMag: TYPE_MAGNITUDE for binderID: %ld - "
             "no record of this binderID!\n", qid );
       return( 0 );
    }
  }

/* Make sure new magnitude has enough readings
 *********************************************/
  if( minfo.nstations < parm->MagWtThresh ) {
    logit("t", "ProcessMag: binderID: %ld author: %s ignored new magnitude: %s %.2lf wt: %.1f"
           " - weight too low to override existing.\n", 
           eqs->id, eqs->author, minfo.szmagtype, minfo.mag, (float)minfo.nstations );
    return( 0 ); 
  }
  eqs->mag       = (float)(minfo.mag);      
  eqs->magwt     = (float)minfo.nstations;     
  strcpy(eqs->magtype, minfo.szmagtype );
  eqs->tlastdata = 0.0;       /* to force feed even without new SM data */

  logit("t", "ProcessMag: binderID: %ld author: %s assigned new magnitude: %s %.2lf\n", 
         eqs->id, eqs->author, eqs->magtype, eqs->mag );
    
/* See if event still qualifies. If not, delete it.
 * If it's a new qualifier, add it to the processing list.
 * If it still qualifies, force one feed only if its UpdateDuration  
 * has already expired (these are probably manually-rerun events).
 *******************************************************************/
  qualify = QualifyEvent(parm, eqs);

  if( !qualify && inshklist ) {
    DeleteEvent( parm, ewd, eqs );
  } 
  else if( qualify ) {    
     EVENTLIST *tmp;
     if( !inshklist ) {  
       logit("t","ProcessMag: binderID: %d author: %s %s:%.2f now qualifies\n",
                  eqs->id, eqs->author, eqs->magtype, eqs->mag );
       tmp = UpdateShkList( eqs );
       eqs = tmp;
     }
  /* force feed if timers have already expired */
     if(   time(NULL) > eqs->tdone ||  
         eqs->tupdate > eqs->tdone    ) SMfeedEvent( parm, ewd, eqs );
  }

  return( 0 );
}


/******************************************************************************
 * UpdateShkList() adds a new event to, or updates an existing event in the   *
 *  processing list                                                           *
 *   Returns: pointer to the event inside the processing list                 *
 ******************************************************************************/
EVENTLIST *UpdateShkList( EVENTLIST *new )
{
   EVENTLIST *eqs;

   eqs = FindEventInShkList( new->id );

/* Not in list - add it 
 **********************/
   if( eqs == NULL ) {
      eqs = &(ShkEqs[nShkEqs%MAXSHKEQS]);
      memcpy( eqs, new, sizeof(EVENTLIST) );
      nShkEqs++;     
      logit("t","UpdateShkList: binderID: %d author: %s %s:%.2f entered in processing list.\n", 
             eqs->id, eqs->author, eqs->magtype, eqs->mag );
   }

/* Already in list; update most info (leave id info, nfeed and tupdate alone) 
 ****************************************************************************/
   else {
      eqs->time      = new->time;
      eqs->lat       = new->lat;
      eqs->lon       = new->lon;
      eqs->depth     = new->depth;
      eqs->mag       = new->mag;
      strcpy( eqs->magtype, new->magtype );
      eqs->magwt     = new->magwt;
      eqs->nph       = new->nph;
      eqs->rms       = new->rms;
      eqs->tlastdata = new->tlastdata;
      eqs->tdone     = new->tdone;
      logit("t","UpdateShkList: binderID: %d author: %s %s:%.2f updated in processing list.\n", 
             eqs->id, eqs->author, eqs->magtype, eqs->mag );
   }
   return( eqs );
}


/******************************************************************************
 * UpdateOthrList() adds a new event to, or updates an existing event in the  *
 *   the backup event list.                                                   *
 *   Returns: pointer to the event in the backup list                         *
 ******************************************************************************/
EVENTLIST *UpdateOthrList( EVENTLIST *new )
{
   EVENTLIST *eqs;
   eqs = FindEventInOthrList( new->id );
   if( eqs == NULL ) {
     eqs = &(OthrEqs[nOthrEqs%MAXOTHREQS]);
     nOthrEqs++; 
   }  
   memcpy( eqs, new, sizeof(EVENTLIST) );
   return( eqs );
}


/******************************************************************************
 * FindEventInShkList() sees if an event is already in the processing list    *
 *   Returns: pointer to event if it's in list, NULL if it isn't              *
 ******************************************************************************/
EVENTLIST *FindEventInShkList( long qid ) 
{
   EVENTLIST *eqs;
   int        i;

   for( i=nShkEqs-1; i>=nShkEqs-MAXSHKEQS; i-- )  
   {
      if( i<0 ) break;                     /* at the beginning of the list */
      eqs = &(ShkEqs[i%MAXSHKEQS]);        /* point into the event list    */
      if( qid == eqs->id ) return( eqs );  /* matches one in list          */
   }
   return( NULL );                         /* this binderID is not in list */
}

/******************************************************************************
 * FindEventInOthrList() sees if an event is already in the list of other eqs *
 *   Returns: pointer to event if it's in list, NULL if it isn't              *
 ******************************************************************************/
EVENTLIST *FindEventInOthrList( long qid ) 
{
   EVENTLIST *eqs;
   int        i;

   for( i=nOthrEqs-1; i>=nOthrEqs-MAXOTHREQS; i-- )  
   {
      if( i<0 ) break;                     /* at the beginning of the list */
      eqs = &(OthrEqs[i%MAXOTHREQS]);      /* point into the event list    */
      if( qid == eqs->id ) return( eqs );  /* matches one in list          */
   }
   return( NULL );                         /* this binderID is not in list */
}


/******************************************************************************
 * QualifyEvent() sees if an event should be fed to shakemap                  *
 *   Returns: 1 if event qualifies for shakemap, 0 if it doesn't              *
 ******************************************************************************/
int QualifyEvent( PARAM *parm, EVENTLIST *eqs ) 
{  
/* Minimum magnitude check */
   if( eqs->mag < parm->MagThresh ) {   
      logit("t","QualifyEvent: binderID: %ld author: %s disqualified; magnitude too low.\n",
             eqs->id, eqs->author );
      return 0;  
   }

/* Minimum magnitude weight check */
   if( eqs->magwt < parm->MagWtThresh ) {  
      logit("t","QualifyEvent: binderID: %ld author: %s disqualified; "
               "mag weight: %.1f too low.\n", eqs->id, eqs->author, eqs->magwt );
      return 0;  
   }

/* More quality checks could be added here... */

/* event passes all tests! */
   return( 1 ); 
}


/******************************************************************************
 * DeleteEvent() deletes the given event (actually, sets tdone to a time      *
 * earlier than current) from the processing list                             *
 ******************************************************************************/
void DeleteEvent( PARAM *parm, EWD *ewd, EVENTLIST *eqs ) 
{
  eqs->tdone = time(NULL)-60;   /* cancel it by resetting tdone  */
  sprintf( Note, "binderID: %ld author: %s removed from processing list.", 
           eqs->id, eqs->author );
  logit("t", "DeleteEvent: %s\n", Note );
  shakemapfeed_page( parm, ewd, Note );
  return; 
}


/******************************************************************************
 * ReviewEvents() goes thru event list and builds shakemap files for those    *
 * events whose timers have expired                                           *
 ******************************************************************************/
int ReviewEvents( PARAM *parm, EWD *ewd )
{
  EVENTLIST *eqs;
  time_t     now;
  int        rc, i;

  if( !Init ) 
  {
    int initrc;
    initrc = InitEvent( parm, ewd );
    if( initrc != INIT_OK ) return( initrc );
  }

  time(&now);

/* Go thru event list backwards, starting with most recent event 
 ***************************************************************/
  for( i=nShkEqs-1; i>=nShkEqs-MAXSHKEQS; i-- )  
  {
     if( i<0 ) break;                    /* at the beginning of the list  */
     eqs = &(ShkEqs[i%MAXSHKEQS]);       /* point into the event list     */
     if( now > eqs->tdone   ) continue;  /* too late for new updates      */
     if( now < eqs->tupdate ) continue;  /* not time for update yet       */
     rc = SMfeedEvent( parm, ewd, eqs );
     eqs->tupdate = NextScheduledUpdate( parm, eqs );
  }
 
  WriteShkListFile(); 
  return( 0 );
}


/******************************************************************************
 * NextScheduledUpdate() returns the time that the next shakemapfeed should   *
 *  occur, based the current time and the configured UpdateSchedule           *
 ******************************************************************************/
time_t NextScheduledUpdate( PARAM *parm, EVENTLIST *eqs )
{
  time_t     now, tdiff;
  int        is;

/* Schedule updates within the configured schedule 
 *************************************************/
  time(&now);
  tdiff = now - eqs->tinlist; 
  for( is=0; is<parm->Schedule.nsched; is++ ) 
  {
     if( tdiff <= parm->Schedule.tsched[is] ) 
     {
        return( eqs->tinlist + parm->Schedule.tsched[is] );
     }
  }

/* Or keep scheduling feeds at the last schedule interval 
 ********************************************************/
  return( now + parm->Schedule.tsched[parm->Schedule.nsched-1] );
}


/******************************************************************************
 * SMfeedEvent() requests all pertinent strongmotion data from the dbms for   *
 *  a given event, builds a file in the format that shakemap likes to read.   *
 ******************************************************************************/
int SMfeedEvent( PARAM *parm, EWD *ewd, EVENTLIST *eqs )
{
   EWDB_CriteriaStruct   crit;       /* time,lat,lon,depth ranges criteria   */
   EWDB_SMChanAllStruct *sm;         /* strong motion data from dbms         */
   EWDB_EventStruct      event;
   int       *useflag;               /* to flag duplicate data as not usable */
   double     maxtt;                 /* maximum travel-time (seconds)        */
   double     maxkm;                 /* maximum epicentral distance          */
   double     tlastdbload;           /* most recent dbms load-time in SM data*/
   TPHASE     treg[10];              /* regional phase travel-times tlay.h   */
   time_t     sec_since_eq;  
   int        nph;                   /* number of regional phases            */
   int        nsm_total  = 0;
   int        nsm_got    = 0;
   int        nsm_indbms = 0;
   int        nsm_use    = 0;        /* useable number of sm readings */
   int        nshkdata   = 0;
   int        rc;
   int        i, ism;

   sec_since_eq = time(NULL) - (time_t)eqs->time;

/* Initialize variables
 **********************/
   sm      = psm;
   useflag = puseflag;
   memset( sm, 0, sizeof(EWDB_SMChanAllStruct)*parm->MaxDataPerEvent );
   memset( useflag, 0, sizeof(int)*parm->MaxDataPerEvent );
   memset( &event, 0, sizeof(EWDB_EventStruct) );

/* Log that we're starting
 *************************/
   if( eqs->dbid != 0 ) {
     logit("et","SMfeedEvent: starting shakemap file %d for binderID: %ld author: %s dbmsID: %ld; "
           "time_since_eq: %02d:%02d:%02d\n",  eqs->nfeed, eqs->id, eqs->author, eqs->dbid,
            sec_since_eq/3600, (sec_since_eq%3600)/60, sec_since_eq%60 );
   }
/* Get DBMS id for this binderID & author
 ****************************************/
   else {
      logit("et","SMfeedEvent: starting shakemap file %d for binderID: %ld author: %s dbmsID: ?; "
            "time_since_eq: %02d:%02d:%02d\n",  eqs->nfeed, eqs->id, eqs->author,
             sec_since_eq/3600, (sec_since_eq%3600)/60, sec_since_eq%60 );
      strcpy( event.szSourceEventID, eqs->idstr  );
      strcpy( event.szSource,        eqs->author );
      if( ewdb_api_CreateEvent( &event ) != EWDB_RETURN_SUCCESS ) {
         logit( "et", "SMfeedEvent: trouble getting DB idEvent\n" );
         return( -1 );
      }
      eqs->dbid = event.idEvent;
      logit("et", "SMfeedEvent: binderID: %ld author: %s -> dbmsID: %ld\n",
                   eqs->id, eqs->author, eqs->dbid );
   }

/* get list of possible reporting stations from dbms (scn,lat,lon)
     or
   use only list of stations in config file (still get lat,lon from dbms) */ 

/* cull list to those within delta-lat/lon range of earthquake. */

/* calculate time-of-interest at each site. */

/* ask dbms for any info from each site during time of interest range. */

/*    or
   just ask dbms for all readings within a lat/lon range and time range */

/* Find distance to farthest grid corner 
 ***************************************/
/* Define a box as distance from epicenter */
   maxkm = dist( eqs->lat, eqs->lon,
                 eqs->lat+parm->LatRange, eqs->lon+parm->LonRange );

/* Or define box as the entire region that shakemap could use... 
 * maxkm = 0.0;
 * maxkm = MAX( maxkm, dist( eqs->lat, eqs->lon, parm->LatMax, parm->LonMax ) );
 * maxkm = MAX( maxkm, dist( eqs->lat, eqs->lon, parm->LatMax, parm->LonMin ) );
 * maxkm = MAX( maxkm, dist( eqs->lat, eqs->lon, parm->LatMin, parm->LonMax ) );
 * maxkm = MAX( maxkm, dist( eqs->lat, eqs->lon, parm->LatMin, parm->LonMin ) );
 */
   if(parm->Debug) logit("et","SMfeedEvent: maxkm=%.0lf\n", maxkm );

/* Calculate maximum travel time in some sensible way 
  ***************************************************/
   maxtt = 0.0; 
   nph = t_region( maxkm, eqs->depth, treg );
   for( i=0; i<nph; i++ ) if( treg[i].t > maxtt ) maxtt = treg[i].t;
   maxtt /= VRAYLEIGH_VS;   /* allow extra time for surface waves */
   if(parm->Debug) logit("et","SMfeedEvent: maxtraveltime=%.0lf\n", maxtt );

/* Fill out criteria structure & request from DBMS  
 *************************************************/
   crit.MinTime  = (int)(eqs->time - MaxTimeTolerance);
   crit.MaxTime  = (int)(eqs->time + maxtt + TT_ERROR + 
                         TIME_POST_ARRIVAL + MaxTimeTolerance);   
/* Define a box as distance from epicenter */
   crit.MinLon   = eqs->lon - parm->LonRange;
   crit.MaxLon   = eqs->lon + parm->LonRange;
   crit.MinLat   = eqs->lat - parm->LatRange;
   crit.MaxLat   = eqs->lat + parm->LatRange;

/* Or define box as the entire region that shakemap could use... 
 * crit.MinLon   = parm->LonMin;
 * crit.MaxLon   = parm->LonMax;
 * crit.MinLat   = parm->LatMin;
 * crit.MaxLat   = parm->LatMax;
 */
   crit.MinDepth = crit.MaxDepth = 0;

   if( parm->Debug > 1 )
   {
      logit("","SMfeedEvent: query criteria:\n"
            " crit.Min-MaxTime:  %d to %d\n"
            " crit.Min-MaxLon:   %.6f to %.6f\n"
            " crit.Min-MaxLat:   %.6f to %.6f\n"
            " crit.Min-MaxDepth: %.2f to %.2f\n",
             crit.MinTime,crit.MaxTime,crit.MinLon,crit.MaxLon,
             crit.MinLat,crit.MaxLat,crit.MinDepth,crit.MaxDepth );
   }

/* A. Original 2-stage SM retrieval based on association 
 *******************************************************/
   if( parm->SMQueryMethod == 0 ) 
   {
   /* Stage A1: Get all SM messages associated with this idEvent
    *           (regardless of time range)
    ************************************************************/
      crit.Criteria = EWDB_CRITERIA_USE_IDEVENT |
                      EWDB_CRITERIA_USE_LAT     |
                      EWDB_CRITERIA_USE_LON;

      rc = ewdb_api_GetSMDataWithChannelInfo( &crit,
                         "*", "*", "*", "*",
                         (EWDBid)eqs->dbid, EWDB_SM_SEARCH_FOR_ALL_SMMESSAGES,
                         &(sm[0]), parm->MaxDataPerEvent,
                         &nsm_indbms, &nsm_got );

      if( rc != EWDB_RETURN_SUCCESS )
      {
         logit("et","SMfeedEvent: trouble getting associated SM data from DBMS "
                    "for binderID: %ld author: %s dbmsID: %ld\n", 
                     eqs->id, eqs->author, eqs->dbid );
      }
      if( nsm_got < nsm_indbms )
      {
         logit("et","SMfeedEvent: retrieved only %d of %d associated SM records "
                    "in DBMS for binderID: %ld author: %s dbmsID: %ld\n", 
                     nsm_got, nsm_indbms, eqs->id, eqs->author, eqs->dbid );
      } else if( parm->Debug ) {
         logit("et","SMfeedEvent: got %d associated SM records from DBMS\n", nsm_got);
      }
      nsm_total = nsm_got;
   
   /* Stage A2: Get all unassociated SM messages
    *           in the proper lat/lon & time range
    **********************************************/
      crit.Criteria = EWDB_CRITERIA_USE_TIME |
                      EWDB_CRITERIA_USE_LAT  |
                      EWDB_CRITERIA_USE_LON;

      rc = ewdb_api_GetSMDataWithChannelInfo( &crit,
                         "*", "*", "*", "*",
                         (int)NULL, EWDB_SM_SEARCH_FOR_ALL_UNASSOCIATED_MESSAGES,
                         &(sm[nsm_got]), parm->MaxDataPerEvent-nsm_got,
                         &nsm_indbms, &nsm_got );
   
      if( rc != EWDB_RETURN_SUCCESS )
      {
         logit("et","SMfeedEvent: trouble getting unassociated SM data from "
                    "DBMS for binderID: %ld author: %s dbmsID: %ld\n", 
                     eqs->id, eqs->author, eqs->dbid );
      }
      if( nsm_got < nsm_indbms )
      {
         logit("et","SMfeedEvent: retrieved only %d of %d unassociated SM records "
                    "in DBMS for binderID: %ld author: %s dbmsID: %ld\n", 
                     nsm_got, nsm_indbms, eqs->id, eqs->author, eqs->dbid );
      } else if( parm->Debug ) {
         logit("et","SMfeedEvent: got %d UNassociated SM records from DBMS\n", nsm_got);
      }
      nsm_total += nsm_got;
   
   } /* End of original 2-stage SM Query */

/* B. New 1-stage retrieval based only on time and space.
 * Post-retrieval filtering will disqualify SM data from other events
 * having the same author as this event, but will allow unassociated SM data 
 * and SM data associated with other eventids having a different author.
 ****************************************************************************/
   else
   {
   /* Stage B1: Get all SM messages in the proper lat/lon & time range 
    ******************************************************************/
      crit.Criteria = EWDB_CRITERIA_USE_TIME |
                      EWDB_CRITERIA_USE_LAT  |
                      EWDB_CRITERIA_USE_LON;

      rc = ewdb_api_GetSMDataWithChannelInfo( &crit,
                         "*", "*", "*", "*",
                         (int)NULL, EWDB_SM_SEARCH_FOR_ALL_SMMESSAGES,
                         &(sm[0]), parm->MaxDataPerEvent,
                         &nsm_indbms, &nsm_got );
   
      if( rc != EWDB_RETURN_SUCCESS )
      {
         logit("et","SMfeedEvent: trouble getting SM data from DBMS in "
                    "space/time range for binderID: %ld author: %s dbmsID: %ld\n", 
                     eqs->id, eqs->author, eqs->dbid );
      }
      if( nsm_got < nsm_indbms )
      {
         logit("et","SMfeedEvent: retrieved only %d of %d SM records in DBMS "
                    "in space/time range for binderID: %ld author: %s dbmsID: %ld\n", 
                     nsm_got, nsm_indbms, eqs->id, eqs->author, eqs->dbid );
      } else if( parm->Debug ) {
         int nassoc=0;
         int nunassoc=0;
         for( ism=0; ism<nsm_got; ism++ ) {
             if     ( sm[ism].idEvent == eqs->dbid ) nassoc++;
             else if(!sm[ism].idEvent              ) nunassoc++;
         }
         logit("et","SMfeedEvent: got %d SM records in space/time range from DBMS\n", nsm_got);
         logit("et","SMfeedEvent: (%d associated; %d UNassoc; %d assoc w/other eqs)\n",
                nassoc, nunassoc, nsm_got-nassoc-nunassoc );
      }
      nsm_total = nsm_got;

   } /* End of new 1-stage SM Query */

   if( parm->Debug > 1 )
   {
      logit("","Before Filter_SM_data\n" );
      logit("","   idEvent   idSMMsg   tDBLoad   S.C.N.L   tMotion       PkAcc  PkVel\n" );
      for(ism=0;ism<nsm_total;ism++)    
      {
         logit("","%10d %10d %.0lf  %s.%s.%s.%s  %.2lf  %.2lf  %.2lf\n",
               sm[ism].idEvent, sm[ism].idSMMessage, sm[ism].SMChan.tload,
               sm[ism].Station.Sta,sm[ism].Station.Comp,sm[ism].Station.Net,sm[ism].Station.Loc,
               sm[ism].SMChan.t,sm[ism].SMChan.pga,sm[ism].SMChan.pgv );
      }
   }

/* Filter SM data and mark them as usable or not.
   (eliminate duplicate data, data where timestamp doesn't
   match expected arrival, data from unwanted components)
 *********************************************************/
   nsm_use = Filter_SM_data( parm, eqs, sm, useflag, nsm_total );
   logit("et","SMfeedEvent: %d of %d retrieved entries will be sent to shakemap\n",
              nsm_use, nsm_total );

   if( parm->Debug )
   {
      logit("","Use codes:\n"
               "  1 ACCEPT: Send this data to shakemap\n"
               "%3d REJECT: duplicate data for this channel\n"
               "%3d REJECT: out of expected time range\n"
               "%3d REJECT: configured to ignore this component code\n"
               "%3d REJECT: associated with different local event\n",
                SM_REJECT_DUPLICATE, SM_REJECT_TIME,
                SM_REJECT_COMPONENT, SM_REJECT_WRONGEQ );
      logit("","Use     dbmsID    tDBLoad   S.C.N.L     Lat        Lon       "
               "tMotion     tAlternate-code   PkAcc  PkVel\n" );

      for(ism=0;ism<nsm_total;ism++)    /* for each channel of data */
      {
         logit("","%3d %10d %.0lf  %s.%s.%s.%s  %.4f %.4f  %.2lf  %.0lf - %d  %.2lf  %.2lf\n",
               useflag[ism], sm[ism].idEvent, sm[ism].SMChan.tload,
               sm[ism].Station.Sta,sm[ism].Station.Comp,sm[ism].Station.Net,sm[ism].Station.Loc,
               sm[ism].Station.Lat,sm[ism].Station.Lon,
               sm[ism].SMChan.t,sm[ism].SMChan.talt,sm[ism].SMChan.altcode,
               sm[ism].SMChan.pga,sm[ism].SMChan.pgv );
      }
   }
   if( nsm_use == 0 )
   {
      sec_since_eq = time(NULL) - (time_t)eqs->time;
      logit("et","SMfeedEvent: no shakemap file built for binderID: %ld "
            "author: %s dbmsID: %ld "
            "(no usable data); time_since_eq: %02d:%02d:%02d\n", 
             eqs->id, eqs->author, eqs->dbid,
             sec_since_eq/3600, (sec_since_eq%3600)/60, sec_since_eq%60 );
      return( 0 );
   }

/* Return now if no new data has arrived since last shakemap was built
 *********************************************************************/
   tlastdbload = 0.0;
   for(i=0;i<nsm_total;i++) {
      if(sm[i].SMChan.tload > tlastdbload) tlastdbload = sm[i].SMChan.tload;
   }
   if(parm->Debug) logit("et","SMfeedEvent: eqs->tlastdata=%.0lf  tlastdbload=%.0lf\n", 
                          eqs->tlastdata,  tlastdbload );

   if( eqs->tlastdata >= tlastdbload )
   {
      sec_since_eq = time(NULL) - (time_t)eqs->time;
      logit("et","SMfeedEvent: no shakemap file built for binderID: %ld "
            "author: %s dbmsID: %ld "
            "(no new data); time_since_eq: %02d:%02d:%02d\n", 
             eqs->id, eqs->author, eqs->dbid,
             sec_since_eq/3600, (sec_since_eq%3600)/60, sec_since_eq%60 );
      return( 0 );
   }

/* Build shakemap file & place in configured directory. 
 ******************************************************/
   rc = Build_SM_file( parm, eqs, &(sm[0]), useflag, nsm_total );

   if( rc== 0 ) { /*successful*/
     sec_since_eq = time(NULL) - (time_t)eqs->time;
     sprintf( Note, "completed shakemap file %d for binderID: %ld author: %s dbmsID: %ld; "
             "time_since_eq: %02d:%02d:%02d", eqs->nfeed, eqs->id, eqs->author, eqs->dbid, 
              sec_since_eq/3600, (sec_since_eq%3600)/60, sec_since_eq%60 );
     logit("et","SMfeedEvent: %s\n", Note );
     shakemapfeed_page( parm, ewd, Note );
   } else { /* error building file */
     logit("et","SMfeedEvent: trouble writing shakemap file %d for "
              "binderID: %ld author: %s dbmsID: %ld; "
              "time_since_eq: %02d:%02d:%02d\n", eqs->nfeed, 
               eqs->id, eqs->author, eqs->dbid, 
               sec_since_eq/3600, (sec_since_eq%3600)/60, sec_since_eq%60 );
   }
   eqs->tlastdata = tlastdbload;
   eqs->nfeed++;

   return( 0 );
}


/*********************************************************************
 *   Filter_SM_data()  Sort the EWDB_SMChanAllStruct struct by       *
 *     SCNL and load time; flag duplicate data as not-usable         *
 *     Returns number of usable channels.                            *
 *********************************************************************/
#define MAX_OTHER_EQ 10

int Filter_SM_data( PARAM *parm, EVENTLIST *eqs, 
                  EWDB_SMChanAllStruct *sm, int *useflag, int nsmdata )
{
   EWDB_SMChanAllStruct  *wsm;         /* working pointer  */
   EWDB_SMChanAllStruct  *usm  = NULL; /* working pointer to last "useable" channel */
   EWDB_EventStruct       othereq[MAX_OTHER_EQ];   /* external event info */
   EWDB_EventStruct      *wother;      /* working pointer into othereq */
   double                 ttol;   
   int                    iuse = 0;    /* index of last usable channel */
   int                    nuse = 0;    /* number of usable channels */
   int                    ism;
   int                    nfound, ngot, i;
   int                    nother = 0;
 
/* Sort EWDB_SMChanAllStruct by SCNL and DBMS load time
 ******************************************************/
   qsort( sm, nsmdata, sizeof(EWDB_SMChanAllStruct), CompareChanT );
    
   for(ism=0;ism<nsmdata;ism++)    /* for each channel of data */
   {
      wsm = &(sm[ism]);

   /* Check for blocked component codes. 
    ************************************/
      if( isBlockComp( wsm->Station.Comp, parm ) )
      {
         useflag[ism] = SM_REJECT_COMPONENT;
         continue;
      }

   /* Check eventid and author of all SM data with associations.
    ************************************************************/
      if( wsm->idEvent != (int)NULL  &&  /* associated with an event, */
          wsm->idEvent != eqs->dbid )    /* but not with this event   */
      {
      /* Did we get external info for this dbmsID yet? */
         for( i=0; i<nother; i++ ) {   
            if( wsm->idEvent == othereq[i].idEvent ) {
                wother = &othereq[i];    /* Yes, we did already got it!   */
                break;
            }
         }  
              
      /* New dbmsID: must get external info */
         if( i==nother ) {             
            if( nother < MAX_OTHER_EQ ) {  
               wother = &othereq[i];     /* Add new entry to other eq list  */
               nother++;
            } else {                   
               wother = &othereq[0];     /* List is full, replace 1st entry */
            }
            memset( wother, 0, sizeof(EWDB_EventStruct) );
            if( ewdb_api_GetExternalEventInfo( wsm->idEvent, wother, 
                    &nfound, &ngot, 1 ) != EWDB_RETURN_SUCCESS ) {
               logit("et","Filter_SM_data: error getting external event info for dbmsID: %ld\n",
                      wsm->idEvent );
            } else {
               logit("et","Filter_SM_data: dbmsID: %ld -> externalid: %s author: %s\n",
                     wother->idEvent, wother->szSourceEventID, wother->szSource );
            }
         }

      /* Reject data if author matches this event, 
       * and the external eventid does NOT match this event's.
       * If both match, don't reject - it's probably associated with
       * the same event in a different dbms than we first started with. */    
     
         if( strcmp(wother->szSource,        eqs->author)==0  &&
             strcmp(wother->szSourceEventID, eqs->idstr )!=0     )
         {
            useflag[ism] = SM_REJECT_WRONGEQ;
            continue;
         }
      }

   /* Check times; verify that data is for this event. 
    * Use different time tolerances for different network codes. 
    ************************************************************/
      ttol = NetTimeTolerance( wsm->Station.Net, parm );
      if( !inExpectedTimeRange( wsm, parm, eqs, ttol ) )
      {
         useflag[ism] = SM_REJECT_TIME;
         continue;
      }

   /* Previously-usable channel; flag duplicate data. 
    *************************************************/
      if( nuse  &&  wsm->Station.idChan == usm->Station.idChan )
      {
         if( wsm->SMChan.tload < usm->SMChan.tload ) /* it's older data */
         {
            useflag[ism]  = SM_REJECT_DUPLICATE;   /* don't use it */
            continue;            
         }
         else  /* it's more recent data */
         {
            useflag[iuse] = SM_REJECT_DUPLICATE;   /* don't use older one    */
            useflag[ism]  = SM_USE;                /* use newer one instead  */
            iuse = ism;
            usm  = wsm;
         }
      }

   /* New SM data passes all the tests.
    ***********************************/
      else 
      {
        useflag[ism] = SM_USE;
        nuse++;
        iuse = ism;
        usm  = wsm;
      }

   }

   return( nuse );
}


/*********************************************************************
 *   Build_SM_file()   Build a shakemap input file(s)                *
 *********************************************************************/
int Build_SM_file( PARAM *parm, EVENTLIST *eqs, 
                   EWDB_SMChanAllStruct *sm, int *useflag, int nsmdata )
{
   char                  tmpfile[PATHLEN*2],outfile[PATHLEN*2];
   char                  fname[PATHLEN-1];
   EWDB_SMChanAllStruct *wsm;  /*working pointer */
   FILE                 *out;
   struct tm             otm;
   time_t                ot;
   int                   hsec;
   int                   ism,irsa;
   long                  id;

/* Make sure we're in the output directory
 *****************************************/
   if( chdir_ew( parm->OutputDir ) )       
   {
      logit( "et", "Build_SM_file: Trouble in chdir to output directory: %s\n",
              parm->OutputDir ); 
      return( -1 );
   }

/* Choose which id to use in building the file
 *********************************************/
   if( parm->UseEventID == USE_DBMSID ) id = eqs->dbid;
   else                                 id = eqs->id;

/* Work on Shakemap1 format; build filenames, open file 
 ******************************************************/
   if( parm->ShakemapFormat == 1 )
   {
      sprintf( fname,   "%ld.dig", id );
      sprintf( tmpfile, "%s/%s", parm->TempDir,   fname );
      sprintf( outfile, "%s/%s", parm->OutputDir, fname );
      out = fopen( tmpfile, "w");
      if( out == 0L ) 
      {
         logit("et", "Build_SM_file: Unable to open ShakeMap file: %s\n", tmpfile ); 
         return( -1 );
      }   

   /* Write the ShakeMap1 file
    **************************/ 
      ot = (time_t) eqs->time;
      gmtime_ew( &ot, &otm );
      hsec = (int)( (eqs->time - ot + .005) * 100.0 );
     
      fprintf(out, "------------------------------------------------------------\n");
      fprintf(out, "                 NCSN Ground Motion Amplitude Data           \n");
      fprintf(out, "                     for Event Number : %ld       \n", id);
      fprintf(out, "------------------------------------------------------------\n");
      fprintf(out, "Summary of Results :                                      \n\n");
      fprintf(out, "Origin Time      :  %d/%02d/%02d     %02d:%02d:%02d.%02d UTC \n",
                    otm.tm_year+1900, otm.tm_mon+1, otm.tm_mday, otm.tm_hour,  
                    otm.tm_min, otm.tm_sec, hsec );
      fprintf(out, "Location         : Lat : %6.2f  Long : %7.2f  Depth : %6.2f  \n", 
                    eqs->lat, eqs->lon, eqs->depth );
      fprintf(out, "Magnitude        :   %5.2f \n", eqs->mag );
      fprintf(out, "Phases Used      :   %d   Solution  RMS : %5.2f    \n", 
                    eqs->nph, eqs->rms);
      fprintf(out, "Components in Ml :   %d   Magnitude RMS : %5.2f  \n\n", 
                    0, 0);
      fprintf(out, "------------------------------------------------------------\n");
      fprintf(out, "Digital Seismic Network Results (in cgs) :    \n\n");
      fprintf(out, "Sta  Comp :   Accel   Vel   Displ   SP_%.1f   SP_%.1f   SP_%.1f   WA_Amp   Ml100\n",
                    Period1, Period2, Period3 );
   
      for(ism=0;ism<nsmdata;ism++)    /* for each channel */
      {
         float    sp1,sp2,sp3;
         float    period;
   
         if( useflag[ism]!=SM_USE ) continue;   /* only use flagged data */
         wsm = &(sm[ism]);
         sp1 = sp2 = sp3 = 0.0;
         for( irsa=0; irsa<wsm->SMChan.nrsa; irsa++ )  /* for each spectral value for this channel*/
         {
            period = (float)(wsm->SMChan.pdrsa[irsa]);         /* save only the interesting spectral values */
            if     ( ABS(period-Period1) < 0.1 ) sp1 = (float)(wsm->SMChan.rsa[irsa]);
            else if( ABS(period-Period2) < 0.1 ) sp2 = (float)(wsm->SMChan.rsa[irsa]);
            else if( ABS(period-Period3) < 0.1 ) sp3 = (float)(wsm->SMChan.rsa[irsa]);
         }
         fprintf(out, "%s  %s : %7.2f %7.3f  %7.4f %7.3f %7.3f %7.3f  %9.2f %7.2f\n", 
                 wsm->SMChan.sta, wsm->SMChan.comp, 
                 (wsm->SMChan.pga!=SM_NULL ? wsm->SMChan.pga : 0.0 ),
                 (wsm->SMChan.pgv!=SM_NULL ? wsm->SMChan.pgv : 0.0 ),
                 (wsm->SMChan.pgd!=SM_NULL ? wsm->SMChan.pgd : 0.0 ),
                 sp1, sp2, sp3, 0.0, 0.0 );
      }
      fclose( out );
   
   /* Log newly created file if so desired
    **************************************/
      if( parm->LogShakemapFile ) logthisfile( tmpfile );
   
   /* Move completed file to output directory
    *****************************************/
      if( rename( tmpfile, outfile ) != 0 )
      {
         logit("et","Build_SM_file: Error renaming ShakeMap file: %s to: %s\n", 
                tmpfile, outfile );
         return( -1 );
      }
   } /* end of Shakemap1 format */

/* Work on Shakemap2 format (XML)
 ********************************/
   else if( parm->ShakemapFormat == 2 )
   {
     float sp1, sp2, sp3, period;
     char laststa[TRACE_STA_LEN+1], lastnet[TRACE_NET_LEN+1];
     char lastInst[INST_TYPE_LEN+1];
     int sta_open;
     INSTTYPE kInst, *pI;
     const double G = 9.81;  /* Convert from cm/sec/sec to percent G */
     
     /* Write the "eventID_event.xml" file of event information */
     sprintf( fname,   "%ld_event.xml", id );
     sprintf( tmpfile, "%s/%s",     parm->TempDir,  fname );
     sprintf( outfile, "%s/%s", parm->OutputDir, fname );
     out = fopen( tmpfile, "w");
     if( out == 0L ) 
     {
       logit("et", "Build_SM_file: Unable to open ShakeMap file: %s\n", tmpfile ); 
       return( -1 );
     }   
     
     ot = (time_t) eqs->time;
     gmtime_ew( &ot, &otm );

     fprintf(out,
             "<?xml version=\"1.0\" encoding=\"US-ASCII\" standalone=\"yes\"?>\n");
     fprintf(out, "<!DOCTYPE earthquake [\n%s]>\n", EARTHQUAKE_DTD);
     fprintf(out, "<earthquake id=\"%ld\" lat=\"%.2f\" lon=\"%.2f\" mag=\"%.2f\" "
             "year=\"%d\" month=\"%d\" day=\"%d\" hour=\"%d\" minute=\"%d\" "
             "second=\"%d\" timezone=\"GMT\" depth=\"%.2f\" locstring=\"\" "
             "created=\"%ld\" />\n", id, eqs->lat, eqs->lon, eqs->mag,
             otm.tm_year+1900, otm.tm_mon+1, otm.tm_mday, otm.tm_hour,
             otm.tm_min, otm.tm_sec, eqs->depth, time((time_t *)0));
     fclose(out);
     
   /* Log newly created file if so desired
    **************************************/
      if( parm->LogShakemapFile ) logthisfile( tmpfile );
   
   /* Move completed file to output directory
    *****************************************/
      if( rename( tmpfile, outfile ) != 0 )
      {
         logit("et","Build_SM_file: Error renaming ShakeMap file: %s to: %s\n", 
                tmpfile, outfile );
         return( -1 );
      }

     /* Write the "eventID_dig_dat.xml" file of ground motion observations */
     sprintf( fname,   "%ld_dig_dat.xml", id );
     sprintf( tmpfile, "%s/%s", parm->TempDir,   fname );
     sprintf( outfile, "%s/%s", parm->OutputDir, fname );
     out = fopen( tmpfile, "w");
     if( out == 0L ) 
     {
       logit("et", "Build_SM_file: Unable to open ShakeMap file: %s\n", tmpfile ); 
       return( -1 );
     }   
     
     fprintf(out,
             "<?xml version=\"1.0\" encoding=\"US-ASCII\" standalone=\"yes\"?>\n");
     fprintf(out, "<!DOCTYPE stationlist [\n%s]>\n", STALIST_DTD);
     fprintf(out, "<stationlist created=\"%ld\">\n", time((time_t *)0));
     
     memset(laststa, 0, TRACE_STA_LEN+1);
     memset(lastnet, 0, TRACE_NET_LEN+1);
     memset(lastInst, 0, INST_TYPE_LEN+1);
     sta_open = 0;
     pI = (INSTTYPE *)NULL;
     for (ism = 0; ism < nsmdata; ism++)  /* for each channel */
     {
       if (useflag[ism] != SM_USE) continue;
       wsm = &(sm[ism]);

       /* Look up the instrument type; in shakemap, this is attached  *
        * to a station name; in earthworm it is attached to something *
        * like the first 2 letters of C in SCN(L)                     */
       if (nInstTypes > 0)
       {
         strcpy(kInst.sta, wsm->SMChan.sta);
         strcpy(kInst.comp, wsm->SMChan.comp);
         strcpy(kInst.net, wsm->SMChan.net);
         pI = (INSTTYPE *)bsearch((void *)&kInst, (void *)pInst, nInstTypes,
                                  sizeof(INSTTYPE), CompareInstType);
       }
       
       /* If station, net or instType has changed, start new station entry */
       if ( (strcmp(laststa, wsm->SMChan.sta) != 0) || 
            (strcmp(lastnet, wsm->SMChan.net) != 0) || 
            ( pI != (INSTTYPE *)NULL && strcmp(lastInst, pI->instType) != 0) )
       {
         new_sta(wsm, &sta_open, lastnet, out, pI, eqs);
         strcpy(laststa, wsm->SMChan.sta);
         if (pI != (INSTTYPE *)NULL) strcpy(lastInst, pI->instType);
       }
       
       /* Extract the desired spectral response values */
       sp1 = sp2 = sp3 = SM_NULL;
       for (irsa = 0; irsa < wsm->SMChan.nrsa; irsa++) 
       {
         period = (float)(wsm->SMChan.pdrsa[irsa]);
         if     ( ABS(period-Period1) < 0.1 ) sp1 = (float)(wsm->SMChan.rsa[irsa]);
         else if( ABS(period-Period2) < 0.1 ) sp2 = (float)(wsm->SMChan.rsa[irsa]);
         else if( ABS(period-Period3) < 0.1 ) sp3 = (float)(wsm->SMChan.rsa[irsa]);
       }

       /* Write component info */
       fprintf(out, "<comp name=\"%s\">\n", wsm->SMChan.comp);
       if( wsm->SMChan.pga!=SM_NULL ) fprintf(out, "<acc value=\"%.4f\" />\n", wsm->SMChan.pga / G);
       if( wsm->SMChan.pgv!=SM_NULL ) fprintf(out, "<vel value=\"%.4f\" />\n", wsm->SMChan.pgv);
       if( sp1 != SM_NULL ) fprintf(out, "<psa03 value=\"%.4f\" />\n", sp1 / G);
       if( sp2 != SM_NULL ) fprintf(out, "<psa10 value=\"%.4f\" />\n", sp2 / G);
       if( sp3 != SM_NULL ) fprintf(out, "<psa30 value=\"%.4f\" />\n", sp3 / G);
       fprintf(out, "</comp>\n");
     }
     if (sta_open)
       close_sta(out);

     fprintf(out, "</stationlist>\n");
     fclose(out);

   /* Log newly created file if so desired
    **************************************/
     if( parm->LogShakemapFile ) logthisfile( tmpfile );
     
     /* Move completed file to output directory
      *****************************************/
     if( rename( tmpfile, outfile ) != 0 )
     {
       logit("et","Build_SM_file: Error renaming ShakeMap file: %s to: %s\n", 
             tmpfile, outfile );
       return( -1 );
     }
     
   }  /* End of shakemap2 format */

   return( 0 );
}

/* Write a new <station> line in the XML file; *
 * optionally closes the previous <station> line */
void new_sta( EWDB_SMChanAllStruct *wsm, int *sta_open, char *lastnet, 
              FILE *f, INSTTYPE *pI, EVENTLIST *eqs)
{
  AGENCY kAgency;
  LONGSTANAME kLSN;
  float staDist;
  
  /* Look up the long "agency" name from the network code */
  if (nAgency > 0)
  {
    if (strcmp(lastnet, wsm->SMChan.net) != 0)
    {  /* net changed; look up new agency */
      pA = (AGENCY *) NULL;
      strcpy(kAgency.net, wsm->SMChan.net);
      pA = (AGENCY *) bsearch((void *)&kAgency, (void *)pAgency, nAgency, 
                              sizeof(AGENCY), CompareAgency);
      strcpy(lastnet, wsm->SMChan.net);
    }
  }
  
  /* Look up the long station name */
  if (nLSN > 0)
  {
    pS = (LONGSTANAME *) NULL;
    strcpy(kLSN.sta, wsm->SMChan.sta);
    strcpy(kLSN.net, wsm->SMChan.net);
    pS = (LONGSTANAME *)bsearch((void *)&kLSN, (void *)pLSN, nLSN, 
                                sizeof(LONGSTANAME), CompareLSN);
  }
  else
    pS = (LONGSTANAME *)NULL;
  
  if (*sta_open)
    close_sta(f);
  *sta_open = 1;

  staDist = (float)dist(eqs->lat, eqs->lon, wsm->Station.Lat, wsm->Station.Lon);

  fprintf(f, "<station code=\"%s\" name=\"%s\" insttype=\"%s\" lat=\"%f\" "
          "lon=\"%f\" source=\"%s\" netid=\"%s\" "
	  "commtype=\"DIG\" dist = \"%.1f\" loc=\"%s\">\n", 
	  wsm->SMChan.sta,
          ( (pS == (LONGSTANAME *)NULL) ? "" : pS->longName), 
          ( (pI == (INSTTYPE *)NULL) ? "" : pI->instType),
          wsm->Station.Lat, wsm->Station.Lon,
          ( (pA == (AGENCY *)NULL) ? wsm->SMChan.net : pA->agency),
	  wsm->SMChan.net, staDist, wsm->SMChan.loc);
  return;
}

/* Write the closing line for <station> to the XML file f */
void close_sta( FILE *f )
{
  fprintf(f, "</station>\n");
  return;
}


/************************************************************************
 * logthisfile() writes the contents of file to the module's daily log  *
 ************************************************************************/
void logthisfile( char *fname )
{
   FILE *fp;
   char string[256];
   int  nr;

   fp = fopen( fname, "r" );
   if( fp == NULL ) {
      logit("","logthisfile: unable to open file %s\n", fname );
      return;
   }
   logit("","Contents of file %s:\n", fname );
   while( (nr=fread( string, 1, 255, fp )) != 0 ) {
     string[nr]='\0';
     logit( "", "%s", string );
   }
   logit("","End of file %s.\n", fname );
   fclose( fp );
   return;
}


/********************************************************************
 *                                                                  *
 * Great-circle distance between 2 points on surface                *
 * Both points must be on the same hemisphere                       *
 * Ported from ShakeMap perl/lib/Distance.pm                        *
 ********************************************************************/
double dist( float lat1, float lon1, float lat2, float lon2 )
{
  
  double dlat,dlon,avlat,f,bb,aa,dist,az;
  
  /* Ensure right hemisphere */
  lon1 = -fabs(lon1);
  lon2 = -fabs(lon2);
  
  dlat  = (lat1-lat2)*(0.0174533);
  dlon  = (lon2-lon1)*(0.0174533);
  avlat = (lat2+lat1)*(0.00872665);
  f     = sqrt (1.0 - 6.76867E-3*(pow(sin(avlat),2)));
  bb    = (cos(avlat) * 6378.276 * dlon / f);
  aa    = (6335.097) * dlat /(f*f*f);
  dist  = sqrt (aa*aa + bb*bb);
  
  return (dist);

}



/************************************************************************
 * CompareTime() a function passed to qsort; used to sort an array      *
 *   of times in the Schedule of feeds to shakemap                      *
 ************************************************************************/
int CompareTime( const void *s1, const void *s2 )
{
   time_t t1 = *((time_t *)s1);
   time_t t2 = *((time_t *)s2);

   if( t1 > t2 ) return(  1 );
   if( t1 < t2 ) return( -1 );
   return( 0 );
}


/************************************************************************
 * CompareChanT() a function passed to qsort; used to sort an array     *
 *   of EWDB_SMChanAllStruct by SNCL (alphabetical order) and           *
 *   DBMS load time (descending order; most recent first)               *
 ************************************************************************/
int CompareChanT( const void *s1, const void *s2 )
{
   int comp;
   EWDB_SMChanAllStruct *ch1 = (EWDB_SMChanAllStruct *) s1;
   EWDB_SMChanAllStruct *ch2 = (EWDB_SMChanAllStruct *) s2;

   if ((comp = strcmp( ch1->Station.Net, ch2->Station.Net))) return comp;
   if ((comp = strcmp( ch1->Station.Sta, ch2->Station.Sta))) return comp;
   if ((comp = strcmp( ch1->Station.Comp, ch2->Station.Comp))) return comp;
   if ((comp = strcmp( ch1->Station.Loc, ch2->Station.Loc))) return comp;

/* idChan's are the same, test DBMS load time */
   if( ch1->SMChan.tload > ch2->SMChan.tload ) return -1;
   if( ch1->SMChan.tload < ch2->SMChan.tload ) return  1;

   return 0;
}


/************************************************************************
 * isBlockComp() checks the given Component code to see if it's in the  *
 *   configured list of blocked components                              *
 *   Returns: 1 if it's in the list, 0 if it isn't                      *
 ************************************************************************/
int isBlockComp( char *comp,  PARAM *parm )
{
   int ib;

   for( ib=0; ib<parm->nBlock; ib++ )
   {
     if( strcmp( parm->BlockComp[ib], comp ) == 0 ) return( 1 );         
   }
   return( 0 );
}


/************************************************************************
 * inExpectedTimeRange() checks the given SM timestamp(s) & configured  *
 *   tolerances to see if the data could reasonably belong to the       *
 *   given event.                                                       *
 *   Returns: 1 if it's reasonable, 0 if it isn't                       *
 ************************************************************************/
int inExpectedTimeRange( EWDB_SMChanAllStruct *sm, PARAM *parm, EVENTLIST *eqs,
                         double ttolerance )
{
   double  maxtt = 0.0;        /* travel-time of last arrival (seconds) */
   double  mintt = 99999.9;    /* travel-time of earliest arrival       */
   double  tearliest, tlatest; /* allowable time range                  */
   double  r;                  /* distance from epicenter to station    */
   double  outrange;           /* seconds out of trange */
   TPHASE  treg[10];           /* regional phase travel-times tlay.h    */
   int     nph;                /* number of regional phases             */
   int     i;

   r   = dist( eqs->lat, eqs->lon, sm->Station.Lat, sm->Station.Lon );
   nph = t_region( r, eqs->depth, treg );

   if(parm->Debug>1) logit("","PHS  %s.%s.%s.%s  r:%.1lf",
       sm->Station.Sta, sm->Station.Comp, sm->Station.Net, sm->Station.Loc, r ); 

   for( i=0; i<nph; i++ ) {
      if( treg[i].t > maxtt ) maxtt = treg[i].t;
      if( treg[i].t < mintt ) mintt = treg[i].t;
      if(parm->Debug>1) logit("","  %s=%.2lf", Phs[treg[i].phase], treg[i].t ); 
   }
   if( parm->Debug>1 ) logit("","\n");

   tearliest = eqs->time + mintt - TT_ERROR - ttolerance;
   tlatest   = eqs->time + maxtt/VRAYLEIGH_VS + TT_ERROR + TIME_POST_ARRIVAL + ttolerance;

   outrange = 0.0;
   if     ( sm->SMChan.t < tearliest ) outrange = sm->SMChan.t-tearliest; 
   else if( sm->SMChan.t > tlatest   ) outrange = sm->SMChan.t-tlatest;

   if( outrange != 0.0 ) {
      if( parm->Debug ) {
         logit("e","Out of trange: %s.%s.%s.%s  r:%.1lf  outby: %.2lf  t: %.2lf  trange: %.2lf - %.2lf\n",
               sm->Station.Sta, sm->Station.Comp, sm->Station.Net, sm->Station.Loc,
               r, outrange, sm->SMChan.t, tearliest, tlatest );
      }
      return( 0 );
   }

   return( 1 );
}


/************************************************************************
 * NetTimeTolerance() returns the configured time tolerance for the     *
 *   given network code.                                                *
 ************************************************************************/
double NetTimeTolerance( char *net, PARAM *parm )
{
   int inet;
   for( inet=0; inet<parm->nNetSlop; inet++ )
   {
     if( strcmp( parm->NetSlop[inet].net, net ) == 0 ) 
         return( parm->NetSlop[inet].ttolerance );         
   }
   return( parm->DefaultTolerance );
}


/************************************************************************
 * CompareAgency() a function passed to qsort; used to sort an array    *
 *   of AGENCY by idChan (ascending order) and            *
 *   DBMS load time (descending order; most recent first)               *
 ************************************************************************/
int CompareAgency( const void *s1, const void *s2 )
{
   AGENCY *t1 = (AGENCY *) s1;
   AGENCY *t2 = (AGENCY *) s2;

   return( strcmp( t1->net, t2->net) );
}

/******************************************************************
 * CompareLSN() a function passed to qsort; used to sort an array *
 *   of LONGSTANAME by SN (alphabetical order)                    *
 ******************************************************************/
int CompareLSN( const void *s1, const void *s2 )
{
  int rc;
  LONGSTANAME *t1 = (LONGSTANAME *) s1;
  LONGSTANAME *t2 = (LONGSTANAME *) s2;

   rc = strcmp( t1->sta, t2->sta );
   if ( rc != 0 ) return(rc);
   rc = strcmp( t1->net,  t2->net );
   return(rc);
}

/***********************************************************************
 * CompareInstType() a function passed to qsort; used to sort an array *
 *   of INSTTYPE structures by SCN (alphabetical order)                *
 ***********************************************************************/
int CompareInstType( const void *s1, const void *s2 )
{
  int rc;
  INSTTYPE *t1 = (INSTTYPE *) s1;
  INSTTYPE *t2 = (INSTTYPE *) s2;

   rc = strcmp( t1->sta, t2->sta );
   if ( rc != 0 ) return(rc);
   rc = strcmp( t1->comp, t2->comp);
   if ( rc != 0 ) return(rc);
   rc = strcmp( t1->net,  t2->net );
   return(rc);
}


/****************************************************************
 * initMappings() initialize the three mapping tables used in   *
 *   writing XML files for shakemapII. We read the mapping file *
 *   allocate space for the table, fill in and sort the table.  *
 *   This should be called at module startup; otherwise errors  *
 *   could embarrass us after an event.                         *
 *   Returns: 0 on success                                      *
 *           -1 on error (out of memory, parse errors)          *
 ****************************************************************/
int initMappings( char *mapFile )
{
  FILE *fh;
  char line[NOTELEN];
  int len;
  int ns, na, ni;

  if ( (fh = fopen(mapFile, "r")) == (FILE *)NULL)
  {
    logit("e", "initMappings: error opening <%s> for reading\n", mapFile);
    return -1;
  }
  
  /* Scan the file once, counting entries of each type */
  while ( fgets( line, NOTELEN, fh) != NULL)
  {
    if (line[0] == '#') continue;  /* Skip comments */
    if (memcmp( line, "Agency", 6) == 0)
      nAgency++;
    else if (memcmp( line, "Station", 7) == 0)
      nLSN++;
    else if (memcmp( line, "Inst", 4) == 0)
      nInstTypes++;
  }
  rewind( fh );
  
  if (nAgency > 0)
  {
    if ( (pAgency = (AGENCY *)calloc((size_t)nAgency, sizeof(AGENCY))) 
         == (AGENCY *)NULL)
    {
      logit("e", "initMappings: out of memory for %d Agency structs\n", 
            nAgency);
      return -1;
    }
  }
  if (nLSN > 0)
  {
    if ( (pLSN = (LONGSTANAME *)calloc((size_t)nLSN, sizeof(LONGSTANAME))) 
         == (LONGSTANAME *)NULL)
    {
      logit("e", "initMappings: out of memory for %d StaName structs\n", 
            nLSN);
      return -1;
    }
  }
  if (nInstTypes > 0)
  {
    if ( (pInst = (INSTTYPE *) calloc((size_t)nInstTypes, sizeof(INSTTYPE)))
         == (INSTTYPE *)NULL)
    {
      logit("e", "initMappings: out of memory for %d InstType structs\n", 
            nInstTypes);
      return -1;
    }
  }
  
  /* Read the file again, parsing the entries */
  ns = na = ni = 0;
  while ( fgets( line, NOTELEN, fh) != NULL)
  {
    if (line[0] == '#') continue;  /* Skip comments */
    len = strlen(line);
    if (line[len-1] == '\n') line[len-1] = '\0'; /* chomp the newline */
    if (memcmp( line, "Agency", 6) == 0)
    {
      if (na == nAgency)
      {
        logit("e", "initMappings: error counting Agency lines\n");
        return -1;
      }
      if ( 2 != sscanf(line, "Agency %8s %40c", pAgency[na].net, 
                      pAgency[na].agency))
      {
        logit("e", "initMappings: error parsing <%s>\n", line);
        return -1;
      }
      na++;
    }
    else if (memcmp( line, "Station", 7) == 0)
    {
      if (ns == nLSN)
      {
        logit("e", "initMappings: error counting Station lines\n");
        return -1;
      }
      if ( 3 != sscanf(line, "Station %6s %8s %50c", pLSN[ns].sta, 
                       pLSN[ns].net, pLSN[ns].longName))
      {
        logit("e", "initMappings: error parsing <%s>\n", line);
        return -1;
      }
      ns++;
    }
    else if (memcmp( line, "Inst", 4) == 0)
    {
      if (ni == nInstTypes)
      {
        logit("e", "initMappings: error counting Inst lines\n");
        return -1;
      }
      if ( 4 != sscanf(line, "Inst %6s %8s %8s %50c", pInst[ni].sta,
                       pInst[ni].comp, pInst[ni].net, 
                       pInst[ni].instType))
      {
        logit("e", "initMappings: error parsing <%s>\n", line);
        return -1;
      }
      ni++;
    }
  }
  fclose( fh );

  /* Sort the tables for lookup using bsearch */
  if (nAgency > 1)
    qsort(pAgency, nAgency, sizeof(AGENCY), CompareAgency);
  if (nLSN > 1)
    qsort(pLSN, nLSN, sizeof(LONGSTANAME), CompareLSN);
  if (nInstTypes > 1)
    qsort(pInst, nInstTypes, sizeof(INSTTYPE), CompareInstType);

  return 0;
}

