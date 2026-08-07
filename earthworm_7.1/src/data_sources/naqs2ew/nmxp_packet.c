/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: nmxp_packet.c,v 1.10 2006/01/04 17:54:33 dietz Exp $
 *
 *    Revision history:
 *     $Log: nmxp_packet.c,v $
 *     Revision 1.10  2006/01/04 17:54:33  dietz
 *     Fixed bug in unpack_tsdata_bundle() to treat compression_bits==00 as
 *     meaning "no data in the data set" (previous versions had incorrectly
 *     handled it as meaning "no compression" and may have added extra bogus
 *     samples to output).
 *
 *     Revision 1.9  2005/07/25 19:02:58  dietz
 *     Added functions to unpack additional SOH packets used by ISTI's
 *     nmxagent program. Changes made by ISTI.
 *
 *     Revision 1.8  2003/02/14 00:17:01  dietz
 *     In unpack_serialheader_bundle(), fixed bug in reading port field.
 *     Had been reading 2 bytes, should only read 1 byte.
 *
 *     Revision 1.7  2003/02/11 01:07:00  dietz
 *     Added functions to read transparent serial packets
 *
 *     Revision 1.6  2002/11/08 21:34:09  dietz
 *     Added support for Libra time quality bundles
 *
 *     Revision 1.5  2002/11/04 18:32:47  dietz
 *     Added support for extended seismic data headers in the
 *     compressed data packet (needed for Trident data).
 *
 *     Revision 1.3  2002/10/15 23:53:26  dietz
 *     *** empty log message ***
 *
 *     Revision 1.2  2001/10/24 22:02:31  dietz
 *     Fixed a bug in nmxpf16tohf32() in setting exponent. Bug resulted
 *     in negative values being extremely large (erroneously)!
 *
 *     Revision 1.1  2001/06/20 22:35:07  dietz
 *     Initial revision
 *
 *
 *
 */

/*
 *  nmxp_packet.c   
 *  Written by Lynn Dietz, April 2001
 *
 *  Functions to read raw Nanometrics Protocol (NMXP) packets. 
 * 
 *  Written to specifications described in: 
 *
 * "Lynx Manual: Appendix B - Libra Data Format" (last revised 22 Mar 2000).
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <swap.h>
#include "nmxp_packet.h"

#define DEBUG 0


/* Valid Instrument types (stored in 5 bits)
   Each instrument code must be 3 characters or less
 ***************************************************/
#define NUM_NMXP_INSTRUMENT  11
char *NMXP_Instrument[NUM_NMXP_INSTRUMENT] =
                     { "HRD",     /*  0 - HRD        */
                       "ORI",     /*  1 - Orion      */
                       "RM3",     /*  2 - RM3        */
                       "RM4",     /*  3 - RM4        */
                       "LNX",     /*  4 - Lynx       */
                       "CYG",     /*  5 - Cygnus     */
                       "EUR",     /*  6 - Europa     */
                       "CAR",     /*  7 - Carina     */
                       "TIM",     /*  8 - TimeServer */
                       "TRI",     /*  9 - Trident    */
                       "JAN" };   /* 10 - Janus      */   
                                  /* 11-31 reserved  */

/* Valid sample rates (stored as enumerated value in 5 bits)
 ***********************************************************/
short NMXP_SampleRate[] ={ 0,         /* reserved */
                           1,         /* 1 */
                           2,         /* 2 */
                           5,         /* 3 */
                           10,        /* 4 */
                           20,        /* 5 */
                           40,        /* 6 */
                           50,        /* 7 */
                           80,        /* 8 */
                           100,       /* 9 */
                           125,       /* 10 */
                           200,       /* 11 */
                           250,       /* 12 */
                           500,       /* 13 */
                           1000,      /* 14 */
                           25,        /* 15 */
                           120 };     /* 16 */
                                      /* 17-31 reserved */
int  Num_NMXP_SampleRate = 17;

/* Protoypes for internally-used functions 
 *****************************************/
long   nmxptohl( long data  );   /*NMXP to host byte order - long  */
short  nmxptohs( short data );   /*NMXP to host byte order - short */
float  nmxptohf( float data );   /*NMXP to host byte order - float */
float  nmxpf16tohf32( char *cbyte );/* NMXP float16 to host float32 */
void   logit( char *, char *, ... );   /* logit.c sys-independent  */


/*********************************************************************
 * unpack_instid():                                                  *
 *   Reads an instrument id variable and fills out a structure       *
 *                                                                   *
 *   instid   bytes to work on (already in local byte order)         *
 *   inst     structure to fill with unpacked instrument values      *
 *                                                                   *
 *   Returns 0 on success, -1 on failure                             *
 *********************************************************************/

int  unpack_instid( short instid, NMXPINSTRUMENT *inst )
{
   memset( inst, 0, sizeof(NMXPINSTRUMENT) );
   inst->model = instid >> 11;     /*  5 bit model number  */
   inst->sn    = instid  & 0x7ff;  /* 11 bit serial number */
   if( inst->model < 0  || 
       inst->model >= NUM_NMXP_INSTRUMENT ) strcpy( inst->code, "UNK" );
   else  strcpy( inst->code, NMXP_Instrument[inst->model] );
   return( 0 );   
}


/*********************************************************************
 * unpack_tsheader_bundle():                                         *
 *   Reads a timeseries (compressed data) packet's header bundle     *
 *                                                                   *
 *   pbundle  pointer to start of bundle                             *
 *   header   structure to fill with unpacked header values          *
 *                                                                   *
 *   Returns 0 on success, -1 on failure                             *
 *********************************************************************/

int unpack_tsheader_bundle( char *pbundle, NMXPDATAHDR *header )
{
   long   ltmp;
   short  stmp;
   char   ctmp;
   char  *cbyte;

   memset( header, 0, sizeof(NMXPDATAHDR) );

   header->pkttype = pbundle[0]&31;    /* packet type: ignore retransmit bit 5 */

   memcpy( &ltmp, pbundle+1, 4 );
   header->sec = nmxptohl( ltmp );     /* full seconds since 1970/01/01 */

   memcpy( &stmp, pbundle+5, 2 );
   header->subsec = nmxptohs( stmp );  /* fractional secs in 10,000ths */

   memcpy( &stmp, pbundle+7, 2 ); 
   header->instrumentid = nmxptohs( stmp );  /* instrument id */

   memcpy( &ltmp, pbundle+9, 4 );
   header->seqnum = nmxptohl( ltmp );     /* packet sequence number */

   ctmp = pbundle[13];
   header->chan      = ctmp & 7;          /* channel:   low 3 bits */
   header->isamprate = ctmp>>3;           /* samprate: high 5 bits */ 

   if( header->isamprate > Num_NMXP_SampleRate ) {
      logit("e","unpack_tsheader_bundle: invalid sample rate index: %d\n",
             header->isamprate );
      return( -1 );
   }

/* Put a 3-byte little-endian value into a 4-byte local-order long */
   ltmp  = 0;                         
   cbyte = (char *) &ltmp;
   memcpy( cbyte, pbundle+14, 3 );         /* get 3-bytes                */
   if( cbyte[2]&0x80 ) cbyte[3]=0xff;      /* sign-extend 4th byte       */
   header->firstsample = nmxptohl( ltmp ); /* first sample value as long */

   return( 0 );
}

/*********************************************************************
 * unpack_tsxheader_bundle():                                        *
 *   Reads a timeseries (compressed data) packet's extended header   *
 *   bundle. This bundle type exists for Trident digitizers and is   *
 *   located immediately after the header bundle.                    *
 *                                                                   *
 *   pbundle  pointer to start of bundle                             *
 *   xheader  structure to fill with unpacked extended header values *
 *                                                                   *
 *   Returns 0 on success, -1 on failure                             *
 *********************************************************************/

int unpack_tsxheader_bundle( char *pbundle, NMXPDATAHDRX *xheader )
{
   long   ltmp;
   char   ctmp;

   memset( xheader, 0, sizeof(NMXPDATAHDRX) );

   if( pbundle[0] != NMXP_EXTENDEDHDR_BUNDLE ) return( -1 ); 

   memcpy( &ltmp, pbundle+1, 4 );
   xheader->firstsample = nmxptohl( ltmp );  /* 4byte first sample value */

   ctmp = pbundle[5];
   xheader->calibstat[0] = ctmp & 1;   /* bit0: chan1 calibration status */
   xheader->calibstat[1] = ctmp & 2;   /* bit1: chan2 calibration status */
   xheader->calibstat[2] = ctmp & 4;   /* bit2: chan3 calibration status */

   return( 0 );
}


/*********************************************************************
 * unpack_tsdata_bundle() Reads a timeseries compressed data bundle  *
 *                                                                   *
 *   pbundle  pointer to start of bundle                             *
 *   data1    value of last sample before this bundle                *
 *   out      structure to fill with uncompressed data values        *
 *                                                                   *
 *   Returns  0 on success, -1 on failure                            *
 *********************************************************************/

int unpack_tsdata_bundle( char *pbundle, long  data1, NMXPDATAVAL *out ) 
{
   char  compression[4];   /* compression bit values */
   char *pdataset;         /* point to start of a 4-byte data set */
   long  diff;
   long  ltmp; 
   short stmp; 
   char  ctmp;
   int   i,j;
   int   idata   = 0;
   long  lastval = data1;

   memset( out, 0 , sizeof(NMXPDATAVAL) );
   
/* Get compression values for each data set 
 ******************************************/
   ctmp = pbundle[0];
   for( i=3; i>=0; i-- )  {
      compression[i] = ctmp & 0x03;  /* get value of low 2 bits */
      ctmp >>= 2;                    /* shift 2 bits right */
   }

   if(DEBUG) {
     logit("e","rawbytes(hex): " );
     for(i=0;i<NMXP_BYTES_PER_BUNDLE;i++ ) logit("e","%x ", pbundle[i]);
     logit("e","\ncbits: ");
     for(i=0;i<4;i++ ) logit("e","%d ", compression[i]);
   }

/* Read the data sets (4 bytes each) 
 ***********************************/
   for( i=0; i<NMXP_DATASET_PER_BUNDLE; i++ ) {
     pdataset = pbundle + 1 + i*4; 

     if(DEBUG) logit("e","diff%d: ",i);

     switch( compression[i] )
     {
       case NMXP_1BYTE_DIFF: /* data set holds 4 1-byte differences */
          for( j=0; j<4; j++ ) {
            if(DEBUG) logit("e","%ld ", (long) pdataset[j]);
            out->data[idata] = lastval + (long) pdataset[j];   
            lastval = out->data[idata];
            idata++;
          }
          break;

       case NMXP_2BYTE_DIFF: /* data set holds 2 2-byte differences */
          for( j=0; j<4; j+=2 ) {
            memcpy( &stmp, pdataset+j, 2 );
            diff = (long) nmxptohs( stmp );
            if(DEBUG) logit("e","%ld ", diff);
            out->data[idata] = lastval + diff;
            lastval = out->data[idata];
            idata++;
          }
          break;

       case NMXP_4BYTE_DIFF: /* data set holds 1 4-byte difference */
          memcpy( &ltmp, pdataset, 4 );
          diff = nmxptohl( ltmp );
          if(DEBUG) logit("e","%ld ", diff);
          out->data[idata] = lastval + diff;
          lastval = out->data[idata];
          idata++;
          break;

       case NMXP_NO_DATA:    /* data set holds no data values; skip it! */
          if(DEBUG) logit("e","no data ");
          break;

       default: 
          logit( "e", "unpack_timeseries_bundle: unknown compression value: %d\n", 
                  compression[i] );
          return( -1 );

      } /*end switch */

   } /* end for each data set */
   if(DEBUG) logit("e","\n");

   out->ndata = idata;

   return( 0 );
}


/*********************************************************************
 * unpack_serialheader_bundle():                                     *
 *   Reads a transparent serial packet's header bundle               *
 *                                                                   *
 *   pbundle  pointer to start of bundle                             *
 *   header   structure to fill with unpacked header values          *
 *                                                                   *
 *   Returns 0 on success, -1 on failure                             *
 *********************************************************************/

int unpack_serialheader_bundle( char *pbundle, NMXPSERIALHDR *header )
{
   long   ltmp;
   short  stmp;

   memset( header, 0, sizeof(NMXPSERIALHDR) );

   header->pkttype = pbundle[0]&31;    /* packet type: ignore retransmit bit 5 */

   memcpy( &ltmp, pbundle+1, 4 );
   header->sec = nmxptohl( ltmp );     /* full seconds since 1970/01/01 */

   memcpy( &stmp, pbundle+5, 2 );
   header->subsec = nmxptohs( stmp );  /* fractional secs in 10,000ths */

   memcpy( &stmp, pbundle+7, 2 ); 
   header->instrumentid = nmxptohs( stmp );  /* instrument id */

   memcpy( &ltmp, pbundle+9, 4 );
   header->seqnum = nmxptohl( ltmp );     /* packet sequence number */

   memcpy( &stmp, pbundle+13, 2 ); 
   header->nbyte = nmxptohs( stmp );  /* number of bytes of payload */

   header->port = *(pbundle+15);     /* serial port number */

   return( 0 );
}


/*********************************************************************
 * unpack_sohheader_bundle():                                        *
 *   Reads a state-of-health packet's header bundle                  *
 *                                                                   *
 *   pbundle  pointer to start of bundle                             *
 *   header   structure to fill with unpacked header values          *
 *                                                                   *
 *   Returns 0 on success, -1 on failure                             *
 *********************************************************************/

int unpack_sohheader_bundle( char *pbundle, NMXPSOHHDR *header )
{
   long   ltmp;
   short  stmp;

   memset( header, 0, sizeof(NMXPSOHHDR) );

   header->pkttype = pbundle[0]&31;    /* packet type: ignore retransmit bit 5 */

   memcpy( &ltmp, pbundle+1, 4 );
   header->sec = nmxptohl( ltmp );     /* full seconds since 1970/01/01 */

   memcpy( &stmp, pbundle+5, 2 );
   header->subsec = nmxptohs( stmp );  /* fractional secs in 10,000ths */

   memcpy( &stmp, pbundle+7, 2 ); 
   header->instrumentid = nmxptohs( stmp );  /* instrument id */

   memcpy( &ltmp, pbundle+9, 4 );
   header->seqnum = nmxptohl( ltmp );     /* packet sequence number */

   return( 0 );
}


/*********************************************************************
 * unpack_vcxocalib_bundle():                                        *
 *   Reads a VCXO calibration bundle from a status packet            *
 *                                                                   *
 *   pbundle  pointer to start of bundle                             *
 *   dat      structure to fill with unpacked values                 *
 *                                                                   *
 *   Returns 0 on success, -1 on failure                             *
 *********************************************************************/

int unpack_vcxocalib_bundle( char *pbundle, NMXPVCXOCALIB *dat )
{
   long   ltmp;
   short  stmp;

   memset( dat, 0, sizeof(NMXPVCXOCALIB) );

   if( pbundle[0] != NMXP_VCXOCALIB_BUNDLE ) return( -1 ); 

   dat->bundletype = pbundle[0];     /* bundle type */

   memcpy( &ltmp, pbundle+1, 4 );
   dat->sec = nmxptohl( ltmp );      /* full seconds since 1970/01/01 */

   memcpy( &stmp, pbundle+5, 2 );
   dat->VCXOvalue = nmxptohs( stmp );  

   memcpy( &stmp, pbundle+7, 2 );
   dat->tdiffcount = nmxptohs( stmp );  
   dat->tdiffusec  = (float) dat->tdiffcount/3.84;

   memcpy( &stmp, pbundle+9, 2 );
   dat->terrcount = nmxptohs( stmp );  
   dat->terrusec  = (float) dat->terrcount/3.84;

   memcpy( &stmp, pbundle+11, 2 );
   dat->freqerr = nmxptohs( stmp );  

   memcpy( &stmp, pbundle+13, 2 );
   dat->crystaltemp = nmxptohs( stmp );  

   dat->PLLstatus = pbundle[15];
   dat->GPSstatus = pbundle[16];

   return( 0 );
}


/*********************************************************************
 * unpack_gpslocation_bundle():                                      *
 *   Reads a GPS location bundle                                     *
 *                                                                   *
 *   pbundle  pointer to start of bundle                             *
 *   dat      structure to fill with unpacked values                 *
 *                                                                   *
 *   Returns 0 on success, -1 on failure                             *
 *********************************************************************/

int unpack_gpslocation_bundle( char *pbundle, NMXPGPSLOC *dat )
{
   float  ftmp;
   long   ltmp;

   memset( dat, 0, sizeof(NMXPGPSLOC) );

   if( pbundle[0] != NMXP_GPSLOCATION_BUNDLE ) return( -1 ); 

   dat->bundletype = pbundle[0];    /* bundle type */

   memcpy( &ltmp, pbundle+1, 4 );
   dat->sec = nmxptohl( ltmp );     /* full seconds since 1970/01/01 */

   memcpy( &ftmp, pbundle+5, 4 );
   dat->lat = nmxptohf( ftmp );

   memcpy( &ftmp, pbundle+9, 4 );
   dat->lon = nmxptohf( ftmp );

   memcpy( &ftmp, pbundle+13, 4 );
   dat->elev = nmxptohf( ftmp );

   return( 0 );
}


/*********************************************************************
 * unpack_externalsoh_bundle():                                      *
 *   Reads a fast or slow external state-of-health bundle,           *
 *   or a Libra environment state-of-health bundle                   *
 *                                                                   *
 *   pbundle  pointer to start of bundle                             *
 *   dat      structure to fill with unpacked values                 *
 *                                                                   *
 *   Returns 0 on success, -1 on failure                             *
 *********************************************************************/

int unpack_externalsoh_bundle( char *pbundle, NMXPEXTSOH *dat )
{
   float  ftmp;
   long   ltmp;

   memset( dat, 0, sizeof(NMXPEXTSOH) );

   if( pbundle[0] != NMXP_FASTEXTSOH_BUNDLE &&
       pbundle[0] != NMXP_SLOWEXTSOH_BUNDLE &&
       pbundle[0] != NMXP_LIBRAENVSOH_BUNDLE   ) return( -1 ); 

   dat->bundletype = pbundle[0];    /* bundle type */

   memcpy( &ltmp, pbundle+1, 4 );
   dat->sec = nmxptohl( ltmp );     /* full seconds since 1970/01/01 */

   memcpy( &ftmp, pbundle+5, 4 );
   dat->soh1 = nmxptohf( ftmp );

   memcpy( &ftmp, pbundle+9, 4 );
   dat->soh2 = nmxptohf( ftmp );

   memcpy( &ftmp, pbundle+13, 4 );
   dat->soh3 = nmxptohf( ftmp );

   return( 0 );
}


/*********************************************************************
 * unpack_slowintsoh_bundle():                                       *
 *   Reads a HRD slow internal state-of-health bundle                *
 *                                                                   *
 *   pbundle  pointer to start of bundle                             *
 *   dat      structure to fill with unpacked values                 *
 *                                                                   *
 *   Returns 0 on success, -1 on failure                             *
 *********************************************************************/

int unpack_slowintsoh_bundle( char *pbundle, NMXPHRDSOH *dat )
{
   float  ftmp;
   long   ltmp;

   memset( dat, 0, sizeof(NMXPHRDSOH) );

   if( pbundle[0] != NMXP_SLOWINTSOH_BUNDLE ) return( -1 ); 

   dat->bundletype = pbundle[0];     /* bundle type */

   memcpy( &ltmp, pbundle+1, 4 );
   dat->sec = nmxptohl( ltmp );      /* full seconds since 1970/01/01 */

   memcpy( &ftmp, pbundle+5, 4 );
   dat->voltage = nmxptohf( ftmp );  /* battery voltage, volts */

   memcpy( &ftmp, pbundle+9, 4 ); 
   dat->VCXOtemp = nmxptohf( ftmp ); /* VCXO temp, degrees Celsius */

   memcpy( &ftmp, pbundle+13, 4 );
   dat->radioSNR = nmxptohf( ftmp ); /* radio SNR,  xxxx (not used) */

   return( 0 );
}


/*********************************************************************
 * unpack_gpstimequal_bundle():                                      *
 *   Reads a GPS time quality bundle from a status packet            *
 *                                                                   *
 *   pbundle  pointer to start of bundle                             *
 *   dat      structure to fill with unpacked values                 *
 *                                                                   *
 *   Returns 0 on success, -1 on failure                             *
 *********************************************************************/

int unpack_gpstimequal_bundle( char *pbundle, NMXPGPSTIMEQUAL *dat )
{
   long   ltmp;
   short  stmp;

   memset( dat, 0, sizeof(NMXPGPSTIMEQUAL) );

   if( pbundle[0] != NMXP_GPSTIMEQUAL_BUNDLE ) return( -1 ); 

   dat->bundletype = pbundle[0];     /* bundle type */

   memcpy( &ltmp, pbundle+1, 4 );
   dat->sec = nmxptohl( ltmp );      /* full seconds since 1970/01/01 */

   memcpy( &stmp, pbundle+5, 2 );
   dat->ontime = nmxptohs( stmp );  

   memcpy( &stmp, pbundle+7, 2 );
   dat->offtime = nmxptohs( stmp );  

   memcpy( &stmp, pbundle+9, 2 );
   dat->tlock = nmxptohs( stmp );  

   memcpy( &stmp, pbundle+11, 2 );
   dat->tdiffcount = nmxptohs( stmp );  
   dat->tdiffusec  = (float) dat->tdiffcount/3.84;

   memcpy( &stmp, pbundle+13, 2 );
   dat->VCXOoffset = nmxptohs( stmp );  
   dat->DACoffset  = (float) dat->VCXOoffset/16.0;

   dat->offnote = pbundle[15];
   dat->mode    = pbundle[16];

   return( 0 );
}


/*********************************************************************
 * unpack_librasoh_bundle():                                         *
 *   Reads a Libra Instrument state-of-health bundle                 *
 *                                                                   *
 *   pbundle  pointer to start of bundle                             *
 *   dat      structure to fill with unpacked values                 *
 *                                                                   *
 *   Returns 0 on success, -1 on failure                             *
 *********************************************************************/

int unpack_librasoh_bundle( char *pbundle, NMXPLIBRASOH *dat )
{
   long   ltmp;
   short  stmp;

   memset( dat, 0, sizeof(NMXPLIBRASOH) );

   if( pbundle[0] != NMXP_LIBRAINSTSOH_BUNDLE ) return( -1 ); 

   dat->bundletype = pbundle[0];     /* bundle type */

   memcpy( &ltmp, pbundle+1, 4 );
   dat->sec = nmxptohl( ltmp );      /* full seconds since 1970/01/01 */

   memcpy( &stmp, pbundle+5, 2 );
   dat->freqerr  = nmxptohs( stmp ); /* ten Mhz frequency error */

   dat->SSPBtemp = nmxpf16tohf32( pbundle+7 );  /* SSPB temp, degrees Celsius */
   dat->WWtemp   = nmxpf16tohf32( pbundle+9 );  /* WW temp, degrees Celsius */
   dat->TXtemp   = nmxpf16tohf32( pbundle+11 ); /* TX temp, degrees Celsius */
   dat->voltage  = nmxpf16tohf32( pbundle+13 ); /* battery voltage */

   return( 0 );
}


/*********************************************************************
 * unpack_libragpstimequal_bundle():                                 *
 *   Reads a Libra GPS Time Quality bundle                           *
 *                                                                   *
 *   pbundle  pointer to start of bundle                             *
 *   dat      structure to fill with unpacked values                 *
 *                                                                   *
 *   Returns 0 on success, -1 on failure                             *
 *********************************************************************/

int unpack_libragpstimequal_bundle( char *pbundle, NMXPLIBRAGPSTIMEQUAL *dat )
{
   long   ltmp;
   short  stmp;
   float  ftmp;

   memset( dat, 0, sizeof(NMXPLIBRAGPSTIMEQUAL) );

   if( pbundle[0] != NMXP_LIBRAGPSTIMEQUAL_BUNDLE ) return( -1 ); 

   dat->bundletype = pbundle[0];     /* bundle type */

   memcpy( &ltmp, pbundle+1, 4 );
   dat->sec = nmxptohl( ltmp );      /* full seconds since 1970/01/01 */

   memcpy( &stmp, pbundle+5, 2 );
   dat->GPSstatus  = nmxptohs( stmp ); /* GPS status */

   memcpy( &stmp, pbundle+7, 2 );
   dat->nSat  = nmxptohs( stmp ); /* # usable satellites */

   memcpy( &ftmp, pbundle+9, 4 );
   dat->PDOP = nmxptohf( ftmp );  /* PDOP */

   memcpy( &ftmp, pbundle+13, 4 );
   dat->TDOP = nmxptohf( ftmp );  /* TDOP */

   return( 0 );
}


/*********************************************************************
 * unpack_librasystimequal_bundle():                                 *
 *   Reads a Libra System Time Quality bundle                        *
 *                                                                   *
 *   pbundle  pointer to start of bundle                             *
 *   dat      structure to fill with unpacked values                 *
 *                                                                   *
 *   Returns 0 on success, -1 on failure                             *
 *********************************************************************/

int unpack_librasystimequal_bundle( char *pbundle, NMXPLIBRASYSTIMEQUAL *dat )
{
   long   ltmp;
   short  stmp;

   memset( dat, 0, sizeof(NMXPLIBRASYSTIMEQUAL) );

   if( pbundle[0] != NMXP_LIBRASYSTIMEQUAL_BUNDLE ) return( -1 ); 

   dat->bundletype = pbundle[0];     /* bundle type */

   memcpy( &ltmp, pbundle+1, 4 );
   dat->sec = nmxptohl( ltmp );      /* full seconds since 1970/01/01 */

   memcpy( &ltmp, pbundle+5, 4 );
   dat->tquality = nmxptohl( ltmp );   /* system time quality */

   memcpy( &stmp, pbundle+9, 2 );
   dat->PLLstatus = nmxptohs( stmp );  /* PLL status */

   memcpy( &stmp, pbundle+11, 2 );
   dat->tdiff = nmxptohs( stmp );      /* system time - GPS time (nanosec) */

   memcpy( &stmp, pbundle+13, 2 );
   dat->tvelocity  = nmxptohs( stmp ); /* time velocity, whatever that is */

   dat->compensation = nmxpf16tohf32( pbundle+15 ); /* current compensation */

   return( 0 );
}

int unpack_rcvsloterr_bundle(char *pbundle, NMXPRCVSLOTERRBUNDLE *dat)
{
   long   ltmp;

   memset( dat, 0, sizeof(NMXPRCVSLOTERRBUNDLE) );

   if( pbundle[0] != NMXP_RXSLOTERR_BUNDLE ) return( -1 ); 

   dat->bundletype = pbundle[0];     /* bundle type */

   memcpy( &ltmp, pbundle+1, 4 );
   dat->sec = nmxptohl( ltmp );      /* full seconds since 1970/01/01 */

   memcpy( &ltmp, pbundle+5, 4 );
   dat->IPaddr = nmxptohl(ltmp);

   memcpy( &ltmp, pbundle+9, 4 );
   dat->badPkt = nmxptohl(ltmp);

   memcpy( &ltmp, pbundle+13, 4 );
   dat->goodPkt = nmxptohl(ltmp);

   return( 0 );
}

int unpack_burst_bundle(char *pbundle, NMXPBURSTBUNDLE *dat)
{
   float  ftmp;
   long   ltmp;
   short  stmp;
   char *cptr;

   memset( dat, 0, sizeof(NMXPBURSTBUNDLE) );

   if( pbundle[0] != NMXP_BURST_BUNDLE ) return( -1 ); 

   dat->bundletype = pbundle[0];     /* bundle type */

   memcpy( &ltmp, pbundle+1, 4 );
   dat->sec = nmxptohl( ltmp );      /* full seconds since 1970/01/01 */

   memcpy( &ltmp, pbundle+5, 4 );
   dat->IPaddr = nmxptohl(ltmp);
   dat->state = pbundle[9];

   /* fprintf(stderr, "DEBUG: unpack_burst_bundle: Burst bytes 5-9 value %d\n", dat->IPaddr); */

   ltmp=0;
   cptr = (char *) &ltmp;
   memcpy( cptr, pbundle+10, 3 );
   if( cptr[2]&0x80 ) cptr[3]=0xff;      /* sign-extend 4th byte       */
   dat->goodBurst = nmxptohl( ltmp );  

   ltmp=0;
   cptr = (char *) &ltmp;
   memcpy( cptr, pbundle+13, 3 );
   if( cptr[2]&0x80 ) cptr[3]=0xff;      /* sign-extend 4th byte       */
   dat->badBurst = nmxptohl( ltmp );  

   dat->spare = pbundle[16];

   return( 0 );
}

int unpack_timeserverpll_bundle(char *pbundle, NMXPTIMESERVERTIMEPLL *dat)
{
   float  ftmp;
   long   ltmp;
   short  stmp;
   char *cptr;

   memset( dat, 0, sizeof(NMXPTIMESERVERTIMEPLL) );

   if( pbundle[0] != NMXP_TIMESRV_PLLSOH_BUNDLE ) return( -1 ); 

   dat->bundletype = pbundle[0];     /* bundle type */

   memcpy( &ltmp, pbundle+1, 4 );
   dat->sec = nmxptohl( ltmp );      /* full seconds since 1970/01/01 */

   memcpy( &ltmp, pbundle+5, 3 );
   cptr = (char *) &ltmp;
   *(cptr+3) = 0;
   dat->subsec = nmxptohl( ltmp );      /* subsec time in fast counts */
  
   dat->status = pbundle[8];

   memcpy( &ltmp, pbundle+9, 4 );
   dat->timeError = nmxptohl( ltmp );      /* time error in fast counts  */
   dat->freqError = pbundle[13];

   memcpy( &ltmp, pbundle+14, 3 );
   cptr = (char *) &ltmp;
   *(cptr+3) = 0;
   dat->timeSince = nmxptohl( ltmp );      /* time since GPS lock loss*/

   return( 0 );
}

int unpack_timeservergps_bundle(char *pbundle, NMXPTIMESERVERGPSSOH *dat)
{
   float  ftmp;
   long   ltmp;
   short  stmp;

   memset( dat, 0, sizeof(NMXPTIMESERVERGPSSOH) );

   if( pbundle[0] != NMXP_TIMESRV_GPSSOH_BUNDLE ) return( -1 ); 

   dat->bundletype = pbundle[0];     /* bundle type */

   memcpy( &ltmp, pbundle+1, 4 );
   dat->sec = nmxptohl( ltmp );      /* full seconds since 1970/01/01 */

   dat->status = pbundle[5];
   dat->numvisSats = pbundle[6];
   dat->numtrackSats = pbundle[7];
   dat->UTCoffset = pbundle[8];

   memcpy( &stmp, pbundle+9, 2 );
   dat->clockBias = nmxptohs( stmp );  

   memcpy( &stmp, pbundle+11, 2 );
   dat->freqBias = nmxptohs( stmp );  

   dat->recvTemp = nmxpf16tohf32( pbundle+13 ); /* current compensation */
   dat->antVolts = nmxpf16tohf32( pbundle+15 ); /* current compensation */
   return( 0 );
}

int unpack_tridentpll_bundle(char *pbundle, NMXPTRIDENTPLLSTATUSSOH *dat)
{
   float  ftmp;
   long   ltmp;
   short  stmp;

   memset( dat, 0, sizeof(NMXPTRIDENTPLLSTATUSSOH) );

   if( pbundle[0] != NMXP_TRIDENT_PLLSTAT_BUNDLE ) return( -1 ); 

   dat->bundletype = pbundle[0];     /* bundle type */

   memcpy( &ltmp, pbundle+1, 4 );
   dat->sec = nmxptohl( ltmp );      /* full seconds since 1970/01/01 */

   memcpy( &stmp, pbundle+5, 2 );
   dat->currentState = nmxptohs( stmp ); 

   memcpy( &stmp, pbundle+7, 2 );
   dat->DACcounts = nmxptohs( stmp );  

   memcpy( &ftmp, pbundle+9, 4 );
   dat->terr = nmxptohf( ftmp );  /* time error micro secs */

   memcpy( &ftmp, pbundle+13, 4 );
   dat->temp = nmxptohf( ftmp );  /* temp */

   return( 0 );
}


/***************************************************************
 *  nmxptohl()                                                 *
 *  converts a long from NMXP (Intel) to host byte order       *
 ***************************************************************/
long nmxptohl( long data )
{
#ifdef _SPARC
   SwapLong( &data );
#endif
   return( data );
}

/***************************************************************
 *  nmxptohs()                                                 *
 *  converts a short from NMXP (Intel) to host byte order      *
 ***************************************************************/
short nmxptohs( short data )
{
#ifdef _SPARC
   SwapShort( &data );
#endif
   return( data );
}

/***************************************************************
 *  nmxptohf()                                                 *
 *  converts a float from NMXP (Intel) to host byte order      *
 ***************************************************************/
float nmxptohf( float data )
{
#ifdef _SPARC
   SwapFloat( &data );
#endif
   return( data );
}

/***************************************************************
 *  nmxpf16tohf32()                                            *
 *  Given the address of the first of two bytes that represent *
 *  a 16-bit float in NMXP (Intel) byte order, return a        *  
 *  32-bit float in host byte order                            *
 ***************************************************************/
float nmxpf16tohf32( char *cbyte )
{
/*-------------------------------------------------------------------
  Nanometrics description of their 16-bit floating point number
  -------------------------------------------------------------------
  The layout, from MSB to LSB is:
  
        1 sign bit (MSB)
        6 bit exponent
        9 bit mantissa
  
  Value is determined by:
  
        value = (-1 ** sign) * (1 + mantissa / mantissa_base)
                     * 2 ** (exponent - exponent_base)
  
        where:   mantissa_base = 2 ** 9 (for 9 bit mantissa)
                 exponent_base = 31     (for 6 bit exponent)
  
  This provides 3-digit accuracy over a range of 2**62.
        Minimum value = 2 ** -30 = 9.3e-10
        Maximum value = 2 **  32 = 4.3e+09
  
  The SOH bundles contain the 2 bytes in Little-endian order.
  -------------------------------------------------------------------*/

   char  hibyte   = cbyte[1];  /* high-order byte of 16-bit float */ 
   char  lobyte   = cbyte[0];  /*  low-order byte of 16-bit float */
   short sign     = 0;
   short exponent = 0;
   union {
     short s;
     char  c[2];
   } mantissa;
   float value;  

/* Pull the 16 bits apart into sign, exponent and mantissa.
   For mantissa, we'll first stuff the bits into a big-endian order
   short because it's easier for my brain to keep straight, 
   then we'll swap it back to little-endian space if we need to. 
   Finally calculate the value with the formula given by Nanometrics
   (see comments above).  Store the result in a 32-bit float!
 *******************************************************************/
   memset( mantissa.c, 0, 2 );   /* initialize mantissa to zero */
   sign     = (hibyte>>7) & 1;   /* hibite.bit7    */ 
   exponent = (hibyte>>1) & 63;  /* hibtye.bit1-6  */
   mantissa.c[1] = lobyte;       /* lobyte.bit0-7 to mantissa.c[1].bit0-7 */
   mantissa.c[0] = hibyte & 1;   /* hibyte.bit0   to mantissa.c[0].bit0   */
#ifdef _INTEL
   SwapShort( &mantissa.s );
#endif
   value = pow( -1.0, (double)sign ) * 
           (1.0 + (double)mantissa.s/512) * 
           pow(2.0,(double)(exponent-31));
    
   if(DEBUG) { 
      logit("e","nmxpf16tohf32:  sign: %d  exponent: %d  mantissa: %d  "
                "calc_value: %.3f\n", sign, exponent, mantissa.s, value ); 
   }

   return( value );
}
