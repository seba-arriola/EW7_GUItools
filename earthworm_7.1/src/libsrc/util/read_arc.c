
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: read_arc.c,v 1.9 2004/05/28 22:35:52 dietz Exp $
 *
 *    Revision history:
 *     $Log: read_arc.c,v $
 *     Revision 1.9  2004/05/28 22:35:52  dietz
 *     changed read_phs to read loc field from phase line
 *
 *     Revision 1.8  2002/12/06 22:30:45  dietz
 *     Added 3 fields (nphS,nphtot,nPfm) to read_hyp()
 *
 *     Revision 1.7  2002/10/29 18:47:42  lucky
 *     Added origin version number
 *
 *     Revision 1.6  2000/12/06 17:50:32  lucky
 *     Make sure that the onset is read and stored
 *
 *     Revision 1.5  2000/09/12 19:07:34  lucky
 *     Defined separate FM fields for S and P picks
 *
 *     Revision 1.4  2000/08/21 19:49:39  lucky
 *     Fixed read_phs so that the Hpck structure is filled with the information
 *     for both the P and S wave picks.
 *
 *     Revision 1.3  2000/04/04 22:51:23  davidk
 *     changed the starting column position for the read of sumP->e1dp
 *     (the intermediate principal error) from 61 to 64.  Due to the error,
 *     e1dp was being set to an excessively large value that caused an
 *     error to occur in orareport as it inserted the bad value into the DB.
 *
 *     Revision 1.2  2000/03/30 15:43:39  davidk
 *     added code to handle additional arc message fields in both
 *     the summary and phase lines, that are used by orareport,
 *     so that orareport can use the read_arc routines.
 *
 *     Revision 1.1  2000/02/14 18:51:48  lucky
 *     Initial revision
 *
 *
 */

/* Routines for reading HYP2000ARC summary lines and phase lines.
 * These routines are provided for the use of arc2trig and two UW-spcific 
 * modules, uw_report and eqtee. They only read the fields that are
 * required by these modules.
 * If you need to read more info from the HYP2000ARC message, feel free to
 * make additions here. You probably will need to add to the structures in
 * read_arc.h as well.
 * Completely rewritten by Pete Lombard, 12/03/1998
 */

#include <stdio.h>
#include <stdlib.h>  /* for atof() */
#include <string.h>
#include <chron3.h>
#include <read_arc.h>

/********************************
 * Internal function prototypes *
 ********************************/
static int strib( char* );
static void read_fl2( char*, float* );
static void read_fl1( char*, float* );
static void read_i(char*, int* );

/********************************
 * External function prototypes *
 ********************************/
void logit( char *, char *, ... );          /* logit.c      sys-independent  */

/**************************************************************
 * read_hyp() reads the hypocenter line from an archive msg into an 
 *            Hsum structure (defined in read_arch.h)
 *
 *      Inputs: sumLine - character string holding the summary line
 *              shdw - character string holding the summary shadow line
 *              sumP - pointer to Hsum structure provided by the caller
 *      returns 0 on success, -1 on parsing errors
 *
 ****************************************************************/

int read_hyp( char *sumLine, char *shdw, struct Hsum *sumP )
{
  char   str[200];
  float  deg, min;
  float  sign;
  int    len;

  /* Initialize the output structure
  **********************************/
  memset( sumP, 0, sizeof(struct Hsum) ); 
 
  /* Copy incoming string to a local buffer so we can alter it.
  ************************************************************/
  strncpy ( str, sumLine, 165 ); /* this should be enough chars for us */
  len = strlen( sumLine );       /* note length of original summary line */

/*------------------------------------------------------------------------
Sample HYP2000ARC archive summary line and its shadow.  The summary line
may be up to 188 characters long.  Its full format is described in
documentation (hyp2000.shadow.doc) by Fred Klein.
199204290117039536 2577120 2407  475  0 18 98 17  16 5975 128175 6  58343COA  38    0  57 124 21   0 218  0  8COA WW D 24X   0  0L  0  0     10123D343 218Z  0   0  \n
YYYYMMDDHHMMSSssLLsLLLLlllsllllDDDDDMMMpppGGGnnnRRRRAAADDEEEEAAADDMMMMCCCrrreeeerrssshhhhvvvvpppssssddddsssdddmmmapdacpppsaeeewwwmaaawwwIIIIIIIIIIlmmmwwwwammmwwwwVvN
$1                                                                                0343   0   0\n
0123456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789 12345
0         1         2         3         4         5         6         7         8         9        10        11        12        13        14        15        16
------------------------------------------------------------------------*/
  
  /* Read origin time
  ******************/
  strncpy( sumP->cdate,    str,     14 ); /* YYYYMMDDHHMMSS */
  sumP->cdate[14] = '.';
  strncpy( sumP->cdate+15, str+14,  2 );  /* hundreths of seconds */
  sumP->cdate[17] = '\0';
  sumP->ot = julsec17( sumP->cdate );
  if ( sumP->ot == 0.0 ) 
  {
    logit( "t", "read_hyp: Error decoding origin time: %s\n",
	   sumP->cdate );
    return( -1 );
  }


  /* We don't seem to care about version numbers at this point */
  sumP->version = 1;
  
  
  /* Read the rest of the card, back to front
  ******************************************/
  if( len >= 154 )
  {
  /* We're not reading fields beyond str[154] (Fortran column 155) */
    str[154] = '\0';
    read_fl1( str+150, &sumP->wtpref );
    str[150] = '\0';
    read_fl2( str+147, &sumP->Mpref );
    sumP->labelpref = str[146];
  }
  str[146] = '\0';  sumP->qid  = atol( str+136 );
  
  str[121] = '\0';  
  read_i( str+118, &sumP->nphtot );
 
  /* Added by davidk 20000317 for use by DB PutArc */
  sumP->mdtype=str[117]; /* coda dur mag type */

  str[107+3]  = '\0';  
  read_fl2( str+107, &sumP->mdmad ); /* Mag XX */

  str[100+4]  = '\0';  
  read_fl1( str+100, &sumP->mdwt ); /* Mag Wt. */
  /* End: Added by davidk 20000317  */

  str[96] = '\0';  
  read_i( str+93, &sumP->nPfm );
  str[93]  = '\0';  
  read_fl2( str+89, &sumP->erz );
  str[89]  = '\0';
  read_fl2( str+85, &sumP->erh );
  str[85]  = '\0';
  read_i( str+82, &sumP->nphS );
  str[80]  = '\0';  
  read_fl2( str+76, &sumP->e2 );
  str[76]  = '\0';
  strncpy( sumP->reg, str+73, 4 );
  str[73]  = '\0';
  read_fl2( str+70, &sumP->Md );
  str[70]  = '\0';
  read_fl2( str+66, &sumP->e1 );
  str[66]  = '\0';  
  read_i( str+64, &sumP->e1dp );
  str[64]  = '\0';
  read_i( str+61, &sumP->e1az );
  str[61]  = '\0';
  read_fl2( str+57, &sumP->e0 );
  str[57]  = '\0';
  read_i( str+55, &sumP->e0dp );
  str[55]  = '\0';
  read_i( str+52, &sumP->e0az );
  str[52]  = '\0';
  read_fl2( str+48, &sumP->rms );
  str[48]  = '\0';
  read_i( str+45, &sumP->dmin );
  str[45]  = '\0';
  read_i( str+42, &sumP->gap );
  str[42]  = '\0';
  read_i( str+39, &sumP->nph );
  str[36]  = '\0';
  read_fl2( str+31, &sumP->z );
  
  /* longitude */
  str[31]  = '\0';  min        = (float)(atof( str+27 )/100.0);
  if ( str[26] == ' ' || str[26] == 'W' || str[26] == 'w' ) 
    sign = -1.0;
  else
    sign = 1.0;
  str[26]  = '\0';  deg        = (float)atof( str+23 );
  sumP->lon = (float)(sign * ( deg + min / 60.0 ));

  /* latitude */
  str[23]  = '\0';  min        = (float)(atof( str+19 )/100.0);
  if ( str[18] == 'S' || str[18] == 's' )
    sign = -1.0;
  else
    sign = 1.0;
  str[18]  = '\0';  deg        = (float)atof( str+16 );
  sumP->lat = (float)(sign * ( deg + min / 60.0 ));

  return ( 0 );
}


/***************************************************************
 * read_phs() reads a phase line & its shadow from archive msg 
 *            into an Hpck structure (defined in read_arch.h)
 *      Inputs: phs - character string holding one phase line
 *              shdw - character string holding the phase shadow line
 *              pckP - pointer to Hpck structure provided by the caller
 *     
 *      returns 0 on success, -1 on parsing errors
 *
 ***************************************************************/

int read_phs( char *phs, char *shdw, struct Hpck *pckP )
{
	char 	str[120];
	float  	Psec, Ssec;
	int 	i;
	double	Cat; /* common arrival time up to yyyymmddhhmm */

   
/*------------------------------------------------------------------------
Sample TYPE_HYP2000ARC station archive card (P-arrival) and its shadow. 
  Phase card is 116 chars (including newline); 
  Shadow is up to 96 chars (including newline).
PWM  NC VVHZ  PD0199204290117  877  -8136    0   0   0      0 0  0  61   0 169 8400  0   77 88325  0 932   0WD 01  \n
SSSSSNNxcCCCxrrFWYYYYMMDDHHMMPPPPPRRRRWWWSSSSSrrxWRRRRAAAAAAAuuWWWDDDDddddEEEEAAAwwppprDDDDAAAMMMmmmppppssssSDALLaa\
$   6 5.49 1.80 7.91 3.30 0.10 PSN0   77 PHP3 1853 39 340 47 245 55 230 63  86 71  70 77  48   \n
$xnnnAA.AAQQ.QQaa.aaqq.qqRR.RRxccccDDDDDxPHpwAAAAATTTAAAAtttaaaaTTTAAAAtttaaaaTTTAAAAtttaaaaMMM\
0123456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789 123456
--------------------------------------------------------------------------*/


  /* Copy phase string to a local buffer so we can alter it.
  ************************************************************/
  memset(str,0,120);
  strncpy ( str, phs, 120 );

  /* Data source code */
  pckP->datasrc = str[108];

  /* Duration magnitude */
  str[94+3] = '\0';
  read_fl2( str+94, &pckP->Md);

  /* azimuth to station */
  str[91+3] = '\0';
  read_i( str+91, &pckP->azm);

  /* Coda duration */
  str[87+4] = '\0';
  read_i( str+87, &pckP->codalen );

  /* duration magnitude weight code */
  str[82+1] = '\0';
  read_i( str+82, &pckP->codawt );

  /* emergence angle at source */
  str[78+3] = '\0';
  read_i( str+78, &pckP->takeoff);

  /* epicentral distance */
  str[74+4] = '\0';
  read_fl1( str+74, &pckP->dist);

  /* S weight actually used */
  str[63+3]  = '\0';
  read_fl2( str+63, &pckP->Swt);

  /* S travel time residual */
  str[50+4]  = '\0';
  read_fl2( str+50, &pckP->Sres );

  /* Assigned S weight code */
  str[49+1]  = '\0';
  read_i( str+49, &pckP->Squal );

  pckP->Sfm = str[48];

  /* S phase label */
  pckP->Slabel = str[47];

  /* S phase onset */
  pckP->Sonset = str[46];

  /* Second of S arrival */
  str[41+5]  = '\0';  
  Ssec = (float)(atof( str+41 )/100.0 );

  /* P weight actually used */
  str[38+3]  = '\0';
  read_fl2( str+38, &pckP->Pwt);

  /* P travel time residual */
  str[34+4]  = '\0';
  read_fl2( str+34, &pckP->Pres );

  /* Seconds of P arrival */
  str[29+5]  = '\0';  
  Psec = (float)(atof( str+29 )/100.0 );

  /* Figure out the common time */
  strncpy( pckP->cdate, str+17, 12 ); /* YYYYMMDDHHMM */
  pckP->cdate[12] = '\0';
  strcat(pckP->cdate, "00.00" );
  Cat = julsec17( pckP->cdate );
  if ( Cat == 0.0 ) 
  {
    logit( "t", "read_phs: Error decoding arrival time: %s\n",
                    pckP->cdate );
    return( -1 );
  }

  /* Calculate P and S arrival times */
  pckP->Pat = Cat + Psec;
  pckP->Sat = Cat + Ssec;

  /* Assigned P weight code */
  str[16+1]  = '\0';
  read_i( str+16, &pckP->Pqual );

  pckP->Pfm = str[15];

  /* P phase label */
  pckP->Plabel = str[14];

  /* P phase onset */
  pckP->Ponset = str[13];

  /* Location code */
  str[111+2]  = '\0';  
  strcpy(pckP->loc, str+111); 
  strib(pckP->loc);

  /* Component code */
  str[9+3]  = '\0';  
  strcpy(pckP->comp, str+9); 
  strib(pckP->comp);

  /* network code */
  str[5+2]   = '\0';  
  strcpy(pckP->net,  str +5); 
  strib(pckP->net);

  /* station site code */
  str[5]   = '\0';  
  strcpy(pckP->site, str); 
  strib(pckP->site);


  /* Copy shadow string to a local buffer so we can alter it.
  ************************************************************/
  memset(str,0,120);
  strncpy ( str, shdw, 120 );

  str[88+4] = '\0';
  read_i( str+88,  &(pckP->caav[5]) );

  str[85+3] = '\0';
  read_i( str+85, &(pckP->ccntr[5]) );

  str[81+4] = '\0';
  read_i( str+81,  &(pckP->caav[4]) );

  str[78+3] = '\0';
  read_i( str+78, &(pckP->ccntr[4]) );

  str[74+4] = '\0';
  read_i( str+74,  &(pckP->caav[3]) );

  str[71+3] = '\0';
  read_i( str+71, &(pckP->ccntr[3]) );

  str[67+4] = '\0';
  read_i( str+67,  &(pckP->caav[2]) );

  str[64+3] = '\0';
  read_i( str+64, &(pckP->ccntr[2]) );

  str[60+4] = '\0';
  read_i( str+60,  &(pckP->caav[1]) );

  str[57+3] = '\0';
  read_i( str+57, &(pckP->ccntr[1]) );

  str[53+4] = '\0';
  read_i( str+53,  &(pckP->caav[0]) );

  str[50+3] = '\0';
  read_i( str+50, &(pckP->ccntr[0]) );

  for( i=0; i<6; i++ ) 
  {
    if( pckP->ccntr[i]==0 ) 
    {
	    pckP->ccntr[i]    = (long)NULL;
      pckP->caav[i]     = (long)NULL;
    }
  }

  str[45+5] = '\0';
  read_i( str+45, &pckP->pamp );

  str[35+5] = '\0';
  read_i( str+35, &pckP->codalenObs);


  return ( 0 );
}


/*
 * Strip trailing blanks and newlines from string.
 */
static int strib( char *string )
{
  int i, length;
  
  length = strlen( string );
  if ( length )
  {
    for ( i = length-1; i >= 0; i-- )
    {
      if ( string[i] == ' ' || string[i] == '\n' )
	string[i] = '\0';
      else
	return ( i+1 );
    }
  } 
  else
    return length;
  return ( i+1 );
}

static void read_fl2( char* str, float* quant )
{
  if ( strib( str ) )
    *quant = (float)(atof( str)/100.0 );
  else
    *quant = NO_FLOAT_VAL;
}

static void read_fl1( char* str, float* quant )
{
  if ( strib( str ) )
    *quant = (float)(atof( str)/10.0 );
  else
    *quant = NO_FLOAT_VAL;
}

static void read_i( char* str, int* quant )
{
  if ( strib( str ) )
    *quant = atoi( str );
  else
    *quant = NO_INT_VAL;
}

