/*
 * Standalone program to read SAC data files and write
 * earthworm TRACE_BUF2 messages.
 * That file can then be made into a tankplayer file using remux_tbuf.
 *
 * Pete Lombard; May 2001
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include "trace_buf.h"
#include "sachead.h"
#include "swap.h"
#include "time_ew.h"

#define DEF_MAX_SAMPS 100

/* Internal Function Prototypes */
void usage( char * );
static int readSACHdr(FILE *, struct SAChead *);
static double sacRefTime( struct SAChead * );
static int strib( char *string );

int main(int argc, char **argv)
{
  TRACE2_HEADER *trh;
  FILE *fp;
  struct SAChead sh;
  char *SACfile, outbuf[MAX_TRACEBUF_SIZ];
  float *seis, *sp;       /* input trace buffer */
  long *lp;
  double sTime, sampInt;
  int arg;
  int i, npts, datalen;
  int max_samps = DEF_MAX_SAMPS;
  char *sac_undef = SACSTRUNDEF;
  trh = (TRACE2_HEADER *)outbuf;
  
  if (argc < 2)
    usage( argv[0] );

  arg = 1;
  while (arg < argc && argv[arg][0] == '-')
  {
    switch(argv[arg][1])
    {
    case 'n':
      arg++;
      max_samps = atoi(argv[arg]);
      break;
    default:
      usage( argv[0] );
    }
    arg++;
  }
  if (argc - arg != 1)
    usage( argv[0] );
  
  SACfile = argv[arg];
  arg++;
  
  if ( (fp = fopen(SACfile, "rb")) == (FILE *)NULL)
  {
    fprintf(stderr, "%s: error opening %s\n", argv[0], SACfile);
    exit( 1 );
  }

  if ( readSACHdr(fp, &sh) < 0)
  {
    fclose(fp);
    exit( 1 );
  }

  npts = (int)sh.npts;
  datalen = npts * sizeof(float);
  if ( (seis = (float *) malloc((size_t)datalen)) == (float *)NULL)
  {
    fprintf(stderr, "%s: out of memory for %d SAC samples\n", argv[0], npts);
    exit( 1 );
  }
  
  if (sh.delta < 0.001)
  {
    fprintf(stderr, "SAC sample period too small: %f\n", sh.delta);
    exit( 1 );
  }

  memset((void*)trh, 0, sizeof(TRACE2_HEADER));
  
  strncpy(trh->sta, sh.kstnm, TRACE2_STA_LEN);
  trh->sta[TRACE2_STA_LEN-1] = '\0';
  strib(trh->sta);
  
  strncpy(trh->chan, sh.kcmpnm, TRACE2_CHAN_LEN-1);
  trh->chan[TRACE2_CHAN_LEN-1] = '\0';
  strib(trh->chan);
  
  strncpy(trh->net, sh.knetwk, TRACE2_NET_LEN-1);
  trh->net[TRACE2_NET_LEN-1] = '\0';
  strib(trh->net);
  
  if (memcmp(sh.khole, sac_undef, TRACE2_LOC_LEN-1) != 0 ) {
      strncpy(trh->loc, sh.khole, TRACE2_LOC_LEN);
      trh->loc[TRACE2_LOC_LEN-1] = '\0';
      strib(trh->loc);
  } else {
      strcpy(trh->loc, LOC_NULL_STRING);
  }
  trh->version[0] = TRACE2_VERSION0;
  trh->version[1] = TRACE2_VERSION1;

  
  trh->quality[0] = '\0';
  sampInt = (double)sh.delta;
  trh->samprate = 1.0/sampInt;

#ifdef _INTEL
  strcpy(trh->datatype, "i4");
#endif
#ifdef _SPARC
  strcpy(trh->datatype, "s4");
#endif

  sTime = (double)sh.b + sacRefTime( &sh );
  fprintf(stderr, "start %4.4ld,%3.3ld,%2.2ld:%2.2ld:%2.2ld.%4.4ld %f\n", sh.nzyear,
          sh.nzjday, sh.nzhour, sh.nzmin, sh.nzsec, sh.nzmsec, sTime);
  
  /* Read the sac data into a buffer */
  if ( (i = (long)fread(seis, sizeof(float), sh.npts, fp)) != npts)
  {
    fprintf(stderr, "error reading SAC data: %s\n", strerror(errno));
    exit( 1 );
  }
  fclose(fp);
  
  sp = seis;
  lp = (long *)(trh+1);
  while (npts >= max_samps)
  {
    datalen = max_samps * sizeof(float);
    trh->starttime = sTime;
    sTime += sampInt * max_samps;
    trh->endtime = sTime - sampInt;
    trh->nsamp = max_samps;
    trh->pinno = 0;      
    
    for(i = 0; i < max_samps; i++)
      lp[i] = (long)(sp[i]);
    sp += max_samps;
    npts -= max_samps;
    
    if (fwrite(trh, 1, sizeof(TRACE2_HEADER) + datalen, stdout) 
        != sizeof(TRACE_HEADER) + datalen)
    {
      fprintf(stderr, "Error writing tankfile: %s\n", strerror(errno));
      exit( 1 );
    }
  }
  if (npts > 0)
  {  /* Get the last few crumbs */
    datalen = npts * sizeof(float);
    trh->starttime = sTime;
    sTime += sampInt * npts;
    trh->endtime = sTime - sampInt;
    trh->nsamp = npts;
    trh->pinno = 0;      
    
    for(i = 0; i < npts; i++)
      lp[i] = (long)(sp[i]);
    
    if (fwrite(trh, 1, sizeof(TRACE_HEADER) + datalen, stdout) 
        != sizeof(TRACE_HEADER) + datalen)
    {
      fprintf(stderr, "Error writing tankfile: %s\n", strerror(errno));
      exit( 1 );
    }
  }
  
  return( 0 );
}


void usage( char *argv0 )
{
  fprintf(stderr, "Usage: %s [-n max-samples] infile >> outfile\n", argv0);
  exit( 1 );
}

  
/*
 * readSACHdr: read the header portion of a SAC file into memory.
 *  arguments: file pointer: pointer to an open file from which to read
 *             filename: pathname of open file, for logging.
 * returns: 0 on success
 *         -1 on error reading file
 *     The file is left open in all cases.
 */
static int readSACHdr(FILE *fp, struct SAChead *psh)
{
  int i;
  struct SAChead2 *psh2;
  
  psh2 = (struct SAChead2 *)psh;
  
  if (fread( psh, sizeof(struct SAChead2), 1, fp) != 1)
  {
    fprintf(stderr, "readSACHdr: error reading SAC file: %s\n",
            strerror(errno));
    return -1;
  }
  
  /* SAC files are always in "_SPARC" byte order; swap if necessary */
#ifdef _INTEL
  for (i = 0; i < NUM_FLOAT; i++)
    SwapLong( (long *) &(psh2->SACfloat[i]));
  for (i = 0; i < MAXINT; i++)
    SwapLong( (long *) &(psh2->SACint[i]));
#endif
  
  return 0;
}

/*
 * sacRefTime: return SAC reference time as a double.
 *             Uses a trick of mktime() (called by timegm_ew): if tm_mday
 *             exceeds the normal range for the month, tm_mday and tm_mon
 *             get adjusted to the correct values. So while mktime() ignores
 *             tm_yday, we can still handle the julian day of the SAC header.
 *             This routine does NOT check for undefined values in the
 *             SAC header.
 *  Returns: SAC reference time as a double.
 */
static double sacRefTime( struct SAChead *pSH )
{
  struct tm tms;
  double sec;
  
  tms.tm_year = pSH->nzyear - 1900;
  tms.tm_mon = 0;    /* Force the month to January */
  tms.tm_mday = pSH->nzjday;  /* tm_mday is 1 - 31; nzjday is 1 - 366 */
  tms.tm_hour = pSH->nzhour;
  tms.tm_min = pSH->nzmin;
  tms.tm_sec = pSH->nzsec;
  tms.tm_isdst = 0;
  sec = (double)timegm_ew(&tms);
  
  return (sec + (pSH->nzmsec / 1000.0));
}

/*
 * strib: strips trailing blanks (space, tab, newline)
 *    Returns: resulting string length.
 */
static int strib( char *string )
{
  int i, length = 0;
  
  if ( string && (length = strlen( string )) > 0)
  {
    for ( i = length-1; i >= 0; i-- )
    {
      switch ( string[i])
      {
      case ' ':
      case '\n':
      case '\t':
        string[i] = '\0';
        break;
      default:
        return ( i+1 );
      }
    }
  }
  
  return length;
}
