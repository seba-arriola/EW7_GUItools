/******************************************************************
 * mags.c (PART 1: CORE MAGNITUDES & ARRAYS)                      *
 * *
 * Contains magnitude determination functions for use in          *
 * many routines (Ml, Mb, Ms, Mw).                                *
 * *
 * By:   Whitmore - Jan., 2001                                    *
 * 64-bits, LFS and Buffer Overflows shielded.                    *
 ******************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <time.h>
#include <string.h>
#include <stdarg.h>
#include <earthworm.h>
#include "earlybirdlib.h"

#ifndef max
#define max(a,b) ((a) > (b) ? (a) : (b))
#endif

/* External prototypes for Mwp algorithms located in mags_mwp.c */
extern double ComputeMwpMag( double dMaxIntDisp, double dDelta );
extern void AutoMwp( STATION *Sta, PPICK *pPBuf, double dSN, int iMwpSeconds, int iS );

/* Global Variables */
/* WC&ATWC long period seismometer response */
double dLPResp[] = {.18,.31,.42,.52,.64,.75,.86,.91,1.1,1.13,1.15,1.17,
        1.18,1.19,1.2,1.16,1.12,1.08,1.04,1.0,.95,.90,.85,.80,.75,
        .71,.67,.63,.59,.55 };
/* Response of the basic long period filter (14s-28s) used in lpproc */
double dLPFResp[] = {.001,.001,.001,.001,.001,.013,.023,.033,0.082,0.105,
        0.197,0.303,0.513,0.724,0.855,1.000,1.013,1.026,1.026,1.026,1.020,
        1.013,1.020,1.026,0.974,0.921,0.816,0.724,0.605,0.500};
/* Honolulu LP (???) response */	
double dLPHResp[] = {.013,.025,.039,.05,.062,.075,.088,.1,.117,.137,         
        .187,.26,.32,.48,.613,.68,.77,.87,.98,1.,.98,.94,.89,.83,
        .735,.7,.65,.58,.5,.444 };	
/* WC&ATWC short period high-gain response */	
double dSPResp[] = {0.4,2.66,3.58,3.02,2.61,2.05,1.75,1.46,1.2,1.0,
        .8,.64,.53,.43,.36,.31,.27,.21,.19,.16,.13,.11,.1,.09,
        .08,.072,.063,.056,.048,.046,.043,.039,.036,.032,.030,
        .027,.025,.0225,.021,.020 };
/* Response of the basic short period filter (5Hz-2.0s) used in pick_wcatwc */
double dSPFResp[] = {0.0833,0.7420,0.9924,1.0076,1.0076,1.0076,1.0076,1.0076,
        1.0076,1.0076,1.0038,1.0000,1.0000,1.0000,0.9621,0.9242,0.8864,0.8409,
        0.7803,0.7121,0.6515,0.5833,0.5227,0.4621,0.4167,0.3712,0.3333,0.2954,
        0.2727,0.2424,0.2200,0.1970,0.1818,0.1667,0.1515,0.1363,0.1250,0.1136,
        0.1023,0.0909};
/* Response of another short period filter (5Hz-1.5s) used in pick_wcatwc */
double dSPF2Resp[] = {0.076,0.705,0.977,1.0,1.0,1.0,1.0,0.985,0.977,0.970,
        .909,.833,.735,.629,.523,.492,.455,.439,.409,.394,.349,.311,.280,
        .242,.220,.197,.159,.136,.121,.106,.087,.072,.059,.055,.051,
        .048,.045,.0425,.0417,.0378 };
/* WC&ATWC Short period, low gain response */
double dSPLResp[] = {.08,.18,.40,.70,1.01,1.26,1.33,1.3,1.16,1.0,.88,
        .76,.64,.52,.39,.35,.31,.26,.21,.16,.14,.125,.112,.10,.091,
        .082,.073,.064,.055,.047};
/* Distance in degrees, array, s-p times up to 160s */
double dSPDist[] = {.1,.2,.25,.3,.4,.46,.48,.5,.6,.7,.8,.85,.9,1.0,1.1,
        1.2,1.3,1.4,1.5,1.6,1.65,1.7,1.8,1.9,2.0,2.1,2.2,2.3,2.4,2.5,
        2.55,2.6,2.7,2.8,2.9,3.,3.1,3.2,3.3,3.4,3.45,3.5,3.6,3.7,3.8,
        3.9,4.,4.1,4.2,4.3,4.4,4.45,4.5,4.6,4.7,4.8,4.9,5.0,5.1,5.2,
        5.3,5.35,5.4,5.5,5.6,5.7,5.8,5.9,6.0,6.1,6.2,6.3,6.4,6.5,6.6,
        6.7,6.8,6.9,7.0,7.1,7.2,7.3,7.4,7.5,7.6,7.7,7.8,7.9,8.0,8.1,
        8.2,8.25,8.3,8.4,8.5,8.6,8.7,8.75,8.8,8.9,9.0,9.1,9.2,9.25,
        9.3,9.4,9.5,9.6,9.7,9.75,9.8,9.85,9.9,10.0,10.1,10.2,10.25,10.3,10.4,
        10.5,10.6,10.7,10.8,10.9,11.0,11.1,11.2,11.25,11.3,11.4,11.5,11.6,11.7,
        11.8,11.9,12.0,12.1,12.2,12.3,12.4,12.5,12.6,12.7,12.8,12.9,13.0,13.1,
        13.2,13.3,13.4,13.5,13.55,13.6,13.7,13.8,13.9,14.0,14.1,14.2,14.3};
/* Richter b-values for Mb and MB */
int iBVal[2500];     

double ComputeAvgMm( int iNum, MMSTUFF Mm[], int *piNumMm )
{                                  
   double  dMmAvg;                
   double  dMmSumMod;             
   int     i;                                   
   int     iMmCountMod;           

   *piNumMm          = 0;
   dMmAvg            = 0.;
   iMmCountMod       = 0;
   dMmSumMod         = 0.0;

   for ( i=0; i<iNum; i++ )
   {              
      if ( Mm[i].dMmMax > 0.0 )
      {
         *piNumMm += 1;
         dMmAvg += Mm[i].dMmMax;
      }
   }

   if ( *piNumMm > 0 )  dMmAvg /= (double) *piNumMm;

   for ( i=0; i<iNum; i++ )
      if ( Mm[i].dMmMax > 0.0 )
         if ( fabs( dMmAvg-Mm[i].dMmMax ) < 0.6 )
         {
            iMmCountMod++;
            dMmSumMod += Mm[i].dMmMax;
         }

   if ( iMmCountMod >= 2 ) 
   {
      *piNumMm = iMmCountMod;
      return( dMmSumMod / (double) iMmCountMod );
   }
   else return( dMmAvg );
}
   
double ComputeAvgMS( int iPNum, PPICK P[], int *piNumMS )
{                                  
   double  dMSAvg;                
   double  dMSSumMod;             
   int     i;                                   
   int     iMSCountMod;           

   *piNumMS          = 0;
   dMSAvg            = 0.;
   iMSCountMod       = 0;
   dMSSumMod         = 0.0;

   for ( i=0; i<iPNum; i++ )
   {              
      if ( P[i].dMSMag > 0.0 )
         if ( P[i].dDelta >= 4.0 )         
         {
            *piNumMS = *piNumMS + 1;
            dMSAvg += P[i].dMSMag;
         }
   }                    

   if ( *piNumMS > 0 )  dMSAvg /= (double) *piNumMS;

   for ( i=0; i<iPNum; i++ )
      if ( P[i].dMSMag > 0. )
         if ( P[i].iMSClip == 0 && fabs( dMSAvg-P[i].dMSMag ) < 0.6 )
         {
            iMSCountMod++;
            dMSSumMod += P[i].dMSMag;
         }

   if ( iMSCountMod >= 2 ) 
   {
      *piNumMS = iMSCountMod;
      return( dMSSumMod / (double) iMSCountMod );
   }
   else return( dMSAvg );
}
       
void ComputeMagnitudes( int iPNum, PPICK P[], HYPO *pHypo )
{
   AZIDELT azidelt;               
   double  dSD;                   
   double  dMwpAvgRaw;            
   double  dMbSumMod, dMlSumMod, dMSSumMod, dMwpSumMod, dMwSumMod;
   int     i, j;
   int     iMbCountMod, iMlCountMod, iMSCountMod, iMwpCountMod, iMwCountMod;
   int     iMwpCounted[MAX_STATIONS]; 
   int     iNumMwpRaw;            
   int     iRegion;               
   LATLON  ll;                    
   long    lTime;                  

   GeoGraphic( &ll, (LATLON *) pHypo );
   if ( ll.dLon < 0 ) ll.dLon += 360.;
   iRegion = GetRegion( ll.dLat, ll.dLon );
      
   pHypo->dMbAvg     = 0.;              
   pHypo->iNumMb     = 0;
   pHypo->iNumMbClip = 0;
   pHypo->dMlAvg     = 0.;
   pHypo->iNumMl     = 0;
   pHypo->iNumMlClip = 0;
   pHypo->dMSAvg     = 0.;
   pHypo->iNumMS     = 0;
   pHypo->iNumMSClip = 0;
   pHypo->dMwpAvg    = 0.;
   pHypo->iNumMwp    = 0;
   pHypo->dMwAvg     = 0.;
   pHypo->iNumMw     = 0;
   pHypo->iNumMwClip = 0;
   iNumMwpRaw        = 0;
   iMbCountMod       = 0;
   iMlCountMod       = 0;
   iMSCountMod       = 0;
   iMwpCountMod      = 0;
   iMwCountMod       = 0;
   dMbSumMod         = 0.0;
   dMlSumMod         = 0.0;
   dMSSumMod         = 0.0;
   dMwSumMod         = 0.0;
   dMwpSumMod        = 0.0;
   dMwpAvgRaw        = 0.0;
   dSD               = 0.0;
   for ( i=0; i<MAX_STATIONS; i++ ) iMwpCounted[i] = 0;
   
   time( &lTime );

   for ( i=0; i<iPNum; i++ )
   {   
      if ( i >= MAX_STATIONS ) break;   /* FIX: acotar iMwpCounted[] */
      azidelt = GetDistanceAz( (LATLON *) pHypo, (LATLON *) &P[i] );
      P[i].dDelta = azidelt.dDelta;
      P[i].dAz = azidelt.dAzimuth;
      
      if ( P[i].iUseMe > 0 )
      {                 
         if ( P[i].dMbAmpGM > 0.0 )           
            if ( P[i].dDelta >= 12.0 )        
            {
               P[i].dMbMag = ComputeMbMag( P[i].szChannel, 0., P[i].dMbAmpGM, P[i].dMbPer, P[i].dDelta, pHypo->dDepth );
               if ( P[i].iMbClip == 1 ) pHypo->iNumMbClip++;
               else { pHypo->iNumMb++; pHypo->dMbAvg += P[i].dMbMag; }
            }
                        
         if ( P[i].dMlAmpGM > 0.0 )
            if ( P[i].dDelta <= 4.0 )         
            {
               for ( j=0; j<160; j++ )        
                  if ( dSPDist[j] >= P[i].dDelta ) break;
   
               if ( (lTime-(long)P[i].dPTime > (long) j+20) && (strcmp( P[i].szChannel, "BHZ" )==0 || strcmp( P[i].szChannel, "HHZ" )==0) )
               {
                  P[i].dMlMag = ComputeMlMag( P[i].szChannel, 0., P[i].dMlAmpGM, P[i].dMlPer, P[i].dDelta );
                  if ( iRegion == 0 || iRegion == 1 ) P[i].dMlMag += 0.3;
                  if ( iRegion >= 10 && iRegion <= 13 ) P[i].dMlMag -= 0.3;
                  if ( P[i].iMlClip == 1 ) pHypo->iNumMlClip++;
                  else { pHypo->iNumMl++; pHypo->dMlAvg += P[i].dMlMag; }
               }
            }
                        
         if ( P[i].dMSAmpGM > 0.0 )
            if ( P[i].dDelta >= 4.0 )         
            {
               P[i].dMSMag = ComputeMSMag( P[i].szChannel, 0., P[i].dMSAmpGM, P[i].dMSPer, P[i].dDelta );
               if ( P[i].iMSClip == 1 ) pHypo->iNumMSClip++;
               else { pHypo->iNumMS++; pHypo->dMSAvg += P[i].dMSMag; }
            }
                        
         /* FIX FORENSE: dMwpTime en los mensajes ARC llega corrupto (como Epoch Time).
            Se ha removido su evaluacion. AutoMwp ya asegura que el pico ocurra 
            dentro de la ventana S-P. 
            Mantenemos la proteccion por clipping de near-field (Delta >= 3.0). */
         if ( P[i].dMwpIntDisp > 0. ) {
            
            if ( P[i].dDelta <= 90. && P[i].dDelta >= 3.0 )         
            {
                P[i].dMwpMag = ComputeMwpMag( P[i].dMwpIntDisp, P[i].dDelta );
                pHypo->iNumMwp++;
                pHypo->dMwpAvg += P[i].dMwpMag;
                iMwpCounted[i] = 1;
            }
         }
		
         if ( P[i].dMwMag > 0.0 )
         {
            if ( P[i].iMwClip == 1 ) pHypo->iNumMwClip++;
            else { pHypo->iNumMw++; pHypo->dMwAvg += P[i].dMwMag; }
         }
      }                 
   }                     

   if (pHypo->iNumMwp)
   {
      dMwpAvgRaw = pHypo->dMwpAvg / (double) pHypo->iNumMwp;
      iNumMwpRaw = pHypo->iNumMwp;
   }
                                                               
   if ( pHypo->iNumMb )  pHypo->dMbAvg /= (double) pHypo->iNumMb;
   if ( pHypo->iNumMl )  pHypo->dMlAvg /= (double) pHypo->iNumMl;
   if ( pHypo->iNumMS )  pHypo->dMSAvg /= (double) pHypo->iNumMS;
   if ( pHypo->iNumMwp ) pHypo->dMwpAvg /= (double) pHypo->iNumMwp;
   if ( pHypo->iNumMw )  pHypo->dMwAvg /= (double) pHypo->iNumMw;

   if ( pHypo->iNumMwp > 2 )
   {
      for ( i=0; i<iPNum; i++ )
         if ( P[i].dMwpMag > 0. && iMwpCounted[i] == 1 )
            dSD += ((P[i].dMwpMag-pHypo->dMwpAvg) *
                    (P[i].dMwpMag-pHypo->dMwpAvg));
      dSD = sqrt( dSD / (double) pHypo->iNumMwp );
   }

   /* FIX ESTADISTICO: Umbral de tolerancia corregido a 2.0 * SD, minimo 0.6 */
   double mwp_tolerance = dSD * 2.0;
   if (mwp_tolerance < 0.6) mwp_tolerance = 0.6;
   
   for ( i=0; i<iPNum; i++ )
      if ( P[i].iUseMe > 0 ) 
      {
         if ( P[i].dMbMag > 0. )
            if ( P[i].iMbClip == 0 && fabs( pHypo->dMbAvg-P[i].dMbMag ) < 0.6 ) { iMbCountMod++; dMbSumMod += P[i].dMbMag; }
         if ( P[i].dMlMag > 0. )
            if ( P[i].iMlClip == 0 && fabs( pHypo->dMlAvg-P[i].dMlMag ) < 0.6 ) { iMlCountMod++; dMlSumMod += P[i].dMlMag; }
         if ( P[i].dMSMag > 0. )
            if ( P[i].iMSClip == 0 && fabs( pHypo->dMSAvg-P[i].dMSMag ) < 0.6 ) { iMSCountMod++; dMSSumMod += P[i].dMSMag; }
         if ( P[i].dMwpMag > 0. ) {
            double diff = fabs( pHypo->dMwpAvg-P[i].dMwpMag );
            if ( diff <= mwp_tolerance && iMwpCounted[i] == 1 ) { 
                iMwpCountMod++; dMwpSumMod += P[i].dMwpMag; 
            }
         }
         if ( P[i].dMwMag > 0. )
            if ( P[i].iMwClip == 0 && fabs( pHypo->dMwAvg-P[i].dMwMag ) < 0.6 ) { iMwCountMod++; dMwSumMod += P[i].dMwMag; }
      }

   if ( iMbCountMod >= 2 ) { pHypo->dMbAvg = dMbSumMod / (double) iMbCountMod; pHypo->iNumMb = iMbCountMod; }
   if ( iMlCountMod >= 2 ) { pHypo->dMlAvg = dMlSumMod / (double) iMlCountMod; pHypo->iNumMl = iMlCountMod; }
   if ( iMSCountMod >= 2 ) { pHypo->dMSAvg = dMSSumMod / (double) iMSCountMod; pHypo->iNumMS = iMSCountMod; }
   
   if ( iMwpCountMod >= 2 ) { 
       pHypo->dMwpAvg = dMwpSumMod / (double) iMwpCountMod; pHypo->iNumMwp = iMwpCountMod; 
   } else { 
       pHypo->dMwpAvg = dMwpAvgRaw; pHypo->iNumMwp = iNumMwpRaw; 
   }      
   
   if ( iMwCountMod >= 2 ) { pHypo->dMwAvg = dMwSumMod / (double) iMwCountMod; pHypo->iNumMw = iMwCountMod; }
}

double ComputeMbMag( char *pszChan, double dGain, double dMbAmp, double dMbPer, double dDelta, double dDepth )
{
   double  dAmp;             
   double  dMbPerT;          
   double  dResponse;        
   int     iDelta;           
   int     iDep;             

   dMbPerT = dMbPer;
   dAmp    = dMbAmp;
   
   if ( dMbPer < 0.1 )     dMbPerT = 0.1;
   else if ( dMbPer > 3. ) dMbPerT = 3.;
   dMbPerT *= 10.;

   if ( !strcmp( pszChan, "SHZ" ) || !strcmp( pszChan, "SMZ" ) )	
      dResponse = dSPResp[(int) (dMbPerT-1.+0.5)]; 
   else if ( !strcmp( pszChan, "SLZ" ) )	
      dResponse = dSPLResp[(int) (dMbPerT-1.+0.5)];
   else if ( !strcmp( pszChan, "BHZ" ) || !strcmp( pszChan, "BHE" ) ||
             !strcmp( pszChan, "BHN" ) || !strcmp( pszChan, "HHZ" ) ||
             !strcmp( pszChan, "BHZ10" ) || 
             !strcmp( pszChan, "BHZ00" ) || !strcmp( pszChan, "BHZXX" ) ||
             !strcmp( pszChan, "HHE" ) || !strcmp( pszChan, "HHN" ) )	
      dResponse = dSPFResp[(int) (dMbPerT-1.+0.5)];
   else	
      dResponse = dSPFResp[(int) (dMbPerT-1.+0.5)];

   if ( dGain < 1.E7 && dGain > 0. )  
      dAmp = (dMbAmp*1000.0) / (dGain*dResponse);
   else	if ( dGain > 0. )             
      dAmp = (dMbAmp*1.e9) / (2.*PI*dGain*dResponse*(1./(dMbPerT/10.0)));
   
   iDelta = (int) (dDelta + 0.49999) - 1; 
   if ( iDelta >= 99 ) iDelta = 99;       
   if ( iDelta < 0 )   iDelta = 0;
   iDep = (int) ((dDepth + 37.5) / 25.) - 1; 
   if ( iDep >= 24 ) iDep = 24;
   if ( iDep < 0 )   iDep = 0;   /* FIX: clamp inferior evita iBVal[negativo] */

   if ( dAmp <= 1.0 ) dAmp = 1.0;
   return( log10( dAmp/(dMbPerT/10.) ) + 0.1*(double) iBVal[iDelta*25 + iDep] );
}

double ComputeMBMag( char * pszChan, double dGain, double dMBAmp, double dMBPer, double dDelta, double dDepth )
{
   double  dAmp;             
   double  dMBPerT;          
   double  dResponse;        
   int     iDelta;           
   int     iDep;             

   dMBPerT = dMBPer;
   dAmp    = dMBAmp;
   
   if ( dMBPer < 1. )       dMBPerT = 1.;
   else if ( dMBPer > 30. ) dMBPerT = 30.;
   
   else if ( !strcmp( pszChan, "LHZ" ) || !strcmp( pszChan, "LLZ" ) )	
      dResponse = dLPResp[(int) (dMBPerT-1.+0.5)];  
   else if ( !strcmp( pszChan, "BHZ" ) || !strcmp( pszChan, "BHE" ) ||
             !strcmp( pszChan, "BHN" ) || !strcmp( pszChan, "HHZ" ) ||
             !strcmp( pszChan, "BHZ10" ) || 
             !strcmp( pszChan, "BHZ00" ) || !strcmp( pszChan, "BHZXX" ) ||
             !strcmp( pszChan, "HHE" ) || !strcmp( pszChan, "HHN" ) )	
      dResponse = dLPFResp[(int) (dMBPerT-1.+0.5)]; 
   else
      dResponse = dLPFResp[(int) (dMBPerT-1.+0.5)]; 

   if ( dGain < 1.E7 && dGain > 0. )     
      dAmp = (dMBAmp*1000.0) / (dGain*dResponse);
   else if ( dGain > 0. )                 
      dAmp = (dMBAmp*1.e9) / (2.*PI*dGain*dResponse*(1./dMBPerT));

   iDelta = (int) (dDelta + 0.49999) - 1; 
   if ( iDelta >= 99 ) iDelta = 99;       
   if ( iDelta < 0 )   iDelta = 0;
   iDep = (int) ((dDepth + 37.5) / 25.) - 1; 
   if ( iDep >= 24 ) iDep = 24;
   if ( iDep < 0 )   iDep = 0;   /* FIX: clamp inferior evita iBVal[negativo] */

   if ( dAmp <= 1.0 ) dAmp = 1.0;
   return( log10( dAmp/dMBPerT ) + 0.1*(double) iBVal[iDelta*25 + iDep] );
}

void ComputeMbMl( STATION *Sta, int iIndex, PPICK *pPBuf, int iMbCycles )
{     
   Sta->lCycCnt++;
   Sta->lPer = (long) ((double) ((2 * (Sta->lSampsPerCyc+1)) / Sta->dSampRate)*10. + 0.0001);
					 
   if ( Sta->lPer > 30 ) Sta->lPer = 30;
   if ( Sta->lPer < 3 ) Sta->lPer = 3;
			   
   if ( Sta->lCycCnt == iMbCycles ) Sta->dMaxPk = 0.;   
		 
   if ( MbMlGroundMotion( Sta->szChannel, Sta->dSens, Sta->lPer, labs( Sta->lMDFRunning ) ) >= Sta->dMaxPk )
   {
      Sta->dMaxPk = MbMlGroundMotion( Sta->szChannel, Sta->dSens, Sta->lPer, labs( Sta->lMDFRunning ) );
      if ( Sta->lCycCnt < iMbCycles )
      {
         pPBuf->dMbPer = (double) Sta->lPer / 10.;
         pPBuf->dMbAmpGM = Sta->dMaxPk;
         pPBuf->iMbClip = 0;
         if ( (double) labs( Sta->lMDFRunning ) >= Sta->dClipLevel && Sta->dClipLevel > 0. ) pPBuf->iMbClip = 1;
         pPBuf->dMbTime =  pPBuf->dPTime + (double) iIndex/Sta->dSampRate;			   
      }
      else 
      {
         pPBuf->dMlPer = (double) Sta->lPer / 10.;
         pPBuf->dMlAmpGM = Sta->dMaxPk;
         pPBuf->iMlClip = 0;
         if ( (double) labs( Sta->lMDFRunning ) >= Sta->dClipLevel && Sta->dClipLevel > 0. ) pPBuf->iMlClip = 1;
         pPBuf->dMlTime =  pPBuf->dPTime + (double) iIndex/Sta->dSampRate;			   
      }       
   }
return;
}

double ComputeMlMag( char *pszChan, double dGain, double dMlAmp, double dMlPer, double dDelta )
{
   double  dAmp;              
   double  dDeltaT, dMlPerT;  
   double  dResponse;         

   dMlPerT = dMlPer;
   dDeltaT = dDelta;
   dAmp    = dMlAmp;

   if ( dMlPer < 0.1 )     dMlPerT = 0.1;
   else if ( dMlPer > 3. ) dMlPerT = 3.;
   dMlPerT *= 10.;

   if ( dDelta <= 0.0 ) dDeltaT = 0.5;

   if ( !strcmp( pszChan, "SHZ" ) || !strcmp( pszChan, "SMZ" ) )	
      dResponse = dSPResp[(int) (dMlPerT-1.+0.5)]; 
   else if ( !strcmp( pszChan, "SLZ" ) )	
      dResponse = dSPLResp[(int) (dMlPerT-1.+0.5)];
   else if ( !strcmp( pszChan, "BHZ" ) || !strcmp( pszChan, "BHE" ) ||
             !strcmp( pszChan, "BHN" ) || !strcmp( pszChan, "HHZ" ) ||
             !strcmp( pszChan, "BHZ10" ) || 
             !strcmp( pszChan, "BHZ00" ) || !strcmp( pszChan, "BHZXX" ) ||
             !strcmp( pszChan, "HHE" ) || !strcmp( pszChan, "HHN" ) )	
      dResponse = dSPFResp[(int) (dMlPerT-1.+0.5)];
   else	
      dResponse = dSPFResp[(int) (dMlPerT-1.+0.5)];

   if ( dGain < 1.E7 && dGain > 0. ) 
      dAmp = (dMlAmp*1000.0) / (dGain*dResponse);
   else if ( dGain > 0. )            
      dAmp = (dMlAmp*1.e9) / (2.*PI*dGain*dResponse*(1./(dMlPerT/10.0)));

   if ( dAmp <= 1.0 ) dAmp = 1.0;
   if ( dDeltaT < 1.65 ) return( log10( dAmp/(dMlPerT/10.) )-0.066 + 0.8*(log10( dDeltaT*dDeltaT )) );
   else return( log10( dAmp/(dMlPerT/10.) ) - 0.364 + 1.5*(log10( dDeltaT*dDeltaT )) );
}

double ComputeMSMag( char *pszChan, double dGain, double dMSAmp, double dMSPer, double dDelta )
{
   double  dAmp;             
   double  dCor;             
   double  dDeltaT, dMSPerT; 
   double  dResponse;        

   dMSPerT = dMSPer;
   dDeltaT = dDelta;
   dAmp    = dMSAmp;
   
   if ( dMSPer < 1. )       dMSPerT = 1.;
   else if ( dMSPer > 30. ) dMSPerT = 30.;
   
   if ( dDelta == 0.0 ) dDeltaT = 0.1;

   if ( dDeltaT <= 16.0 ) dCor = 0.53 - (0.033 * dDeltaT);
   else                   dCor = 0.0;

   if ( !strcmp( pszChan, "LHZ" ) || !strcmp( pszChan, "LLZ" ) )	
      dResponse = dLPResp[(int) (dMSPerT-1.+0.5)]; 
   else if ( !strcmp( pszChan, "BHZ" ) || !strcmp( pszChan, "BHE" ) ||
             !strcmp( pszChan, "BHN" ) || !strcmp( pszChan, "HHZ" ) ||
             !strcmp( pszChan, "BHZ10" ) || 
             !strcmp( pszChan, "BHZ00" ) || !strcmp( pszChan, "BHZXX" ) ||
             !strcmp( pszChan, "HHE" ) || !strcmp( pszChan, "HHN" ) )	
      dResponse = dLPFResp[(int) (dMSPerT-1.+0.5)];
   else dResponse = dLPFResp[(int) (dMSPerT-1.+0.5)];

   if ( dGain < 1.E7 && dGain > 0. )  
      dAmp = (dMSAmp*1000.0) / (dGain*dResponse);
   else if ( dGain > 0. )             
      dAmp = (dMSAmp*1.e9) / (2.*PI*dGain*dResponse*(1./dMSPerT));

   if ( dAmp <= 1.0 ) dAmp = 1.0;
   return( log10( dAmp/dMSPerT ) + 1.66*log10( dDeltaT ) + 3.0 + dCor );
}

double ComputeMwMag( double dGain, double dMwAmp, double dMwPer, double dDelta )
{
   double  dAmp;       
   double  dBruneFactor[51] = {24.3,24.4,24.5,24.6,24.7,24.8,24.9,25.0,
    25.1,25.2,25.3,25.4,25.5,25.6,25.7,25.8,25.9,26.0,26.1,26.2,26.4,
    26.6,26.9,27.2,27.4,27.5,27.6,27.7,27.8,27.9,28.0,28.1,28.2,
    28.3,28.4,28.5,28.6,28.7,28.8,28.9,29.0,29.1,29.2,29.3,29.4,
    29.5,29.6,29.7,29.8,29.9,30.0};
   double  dBruneMag[50] = {-4.5,-4.4,-4.29,-4.19,-4.1,-4.,-3.9,-3.8,
    -3.7,-3.6,-3.5,-3.4,-3.3,-3.19,-3.09,-2.99,-2.89,-2.79,-2.69,-2.59,
    -2.42,-2.18,-1.94,-1.71,-1.49,-1.3,-1.21,-1.12,-1.04,-0.95,
    -0.85,-0.77,-0.67,-0.58,-0.49,-0.4,-0.31,-0.22,-0.13,-0.04,0.05,0.13,
    0.22,0.31,0.4,0.5,0.58,0.67,0.76,0.85};
   double  dBruneVal[181] = {0.05,0.05,0.06,0.07,0.07,0.08,0.08,0.09,
    0.09,0.10,0.11,0.12,0.13,0.14,0.15,0.15,0.16,0.17,0.18,0.19,0.20,0.21,0.22,
    0.23,0.25,0.26,0.27,0.28,0.29,0.30,0.31,0.32,0.33,0.34,0.35,0.36,0.37,
    0.38,0.39,0.40,0.41,0.42,0.43,0.44,0.45,0.46,0.47,0.48,0.49,0.50,0.51,
    0.52,0.53,0.54,0.55,0.56,0.57,0.58,0.59,0.60,0.61,0.62,0.63,0.64,0.65,
    0.68,0.69,0.70,0.72,0.73,0.74,0.75,0.77,0.78,0.79,0.80,0.81,0.82,0.84,
    0.85,0.87,0.88,0.90,0.91,0.93,0.94,0.96,0.97,0.99,1.01,1.03,1.04,1.05,
    1.07,1.08,1.09,1.10,1.12,1.13,1.13,1.14,1.14,1.15,1.16,1.17,1.17,1.18,
    1.19,1.19,1.20,1.21,1.21,1.22,1.23,1.24,1.25,1.26,1.26,1.27,1.28,1.28,
    1.29,1.29,1.30,1.30,1.30,1.31,1.31,1.32,1.32,1.32,1.32,1.33,1.33,1.33,
    1.33,1.34,1.34,1.34,1.34,1.32,1.32,1.32,1.31,1.30,1.29,1.28,1.27,1.27,
    1.26,1.23,1.21,1.18,1.16,1.13,1.10,1.08,1.05,1.03,1.00,0.96,0.92,
    0.88,0.84,0.80,0.76,0.72,0.68,0.64,0.60,0.56,0.52,0.48,0.44,0.40,0.36,
    0.32,0.28,0.24,0.24,0.32};
   double  dDeltaT, dMwPerT; 
   double  dMwMag, dMagMm, dBruneMoment;   
   double  dResponse;        
   double  dVLPResp[20] = {0.4,2.1,5.4,12.0,18.,32.,43.,60.,78.,100.,
                           110.,109.,100.,92.,82.,72.,62.,54.,46.,41.};
   int     i;
   int     iMoment;

   dMwPerT = dMwPer;
   dDeltaT = dDelta;
   dAmp    = dMwAmp;
   
   if ( dMwPer < 10. )       dMwPerT = 10.;
   else if ( dMwPer > 200. ) dMwPerT = 200.;
   dMwPerT /= 10.;
   
   if ( dDelta <= 0.0 )  dDeltaT = 50.;
   if ( dDelta > 180.0 ) dDeltaT = 180.;

   if ( dDeltaT >= 15. )
   {                                  
      if ( dGain > 1.E7 && dGain > 0. )  
         dResponse = 1.;                 
      else if ( dGain > 0. )          
         dResponse = dVLPResp[(int) (dMwPerT+0.5)-1] * 0.01;
      else
         dResponse = 1.;
		
      if ( dGain < 1.E7 && dGain > 0.)    
         dAmp = log10( (dMwAmp / ((dGain*1000.) * dResponse * 20.)) * dBruneVal[(int) (dDeltaT+0.5)-1] );
      else if ( dGain > 0. )              
         dAmp = log10( ((dMwAmp*1000.) / (2.*PI*dGain*dResponse*20.)) * dBruneVal[(int) (dDeltaT+0.5)-1] );
      else                                
         dAmp = (dMwAmp/20.) * dBruneVal[(int) (dDeltaT+0.5)-1];
      for ( i=0; i<50; i++ )
         if ( dAmp <= dBruneMag[i] ) break;                        
      i += 1;
	  
      dMagMm = (((double) i - 1.0)*0.1) + 5.0;
	
      iMoment = (int) ((dMagMm - 4.9) * 10. + 0.1);
      if ( iMoment < 1 )  iMoment = 1;    /* FIX: clamp indice dBruneFactor[] */
      if ( iMoment > 51 ) iMoment = 51;
      dBruneMoment = pow( 10., dBruneFactor[iMoment-1] );
      dMwMag = 0.66667 * log10( dBruneMoment ) - 10.7;	
   }
   else         
   {            
      dAmp = dMwAmp / 10.;
      if ( dGain < 1.E7 && dGain > 0.)   
         dAmp = dAmp*0.1 / (dGain*1000.);
      else if ( dGain > 0. )             
         dAmp = (dAmp*1000.) * 0.1 / (2.*PI*dGain/dMwPerT);  
      dAmp *= pow( dDeltaT, 0.6 );
      dMwMag = 0.66667 * log10( dAmp ) + 7.9;
   }
return( dMwMag );
}

void GetMDFFilt( long lBIndex, long lBNum, STATION *Sta )
{
   long    i, lTemp;
   long    lMDFTotal;            

   Sta->lMDFOld = 0;
   lMDFTotal = 0;
   Sta->lMDFRunning = 0;
   Sta->lCycCnt = 0;
   
   for ( i=0; i<lBNum; i++ )
   {
      lTemp = i + lBIndex;
      if ( lTemp >= Sta->lRawCircSize ) lTemp -= Sta->lRawCircSize;
      Sta->lSampNew = Sta->plFiltCircBuff[lTemp];
      if ( i == 0 ) Sta->lSampOld = Sta->lSampNew;
      Sta->lMDFNew = Sta->lSampNew - Sta->lSampOld;
	  
      if ( i > 0 )
      {
         if ( (Sta->lMDFOld <  0 && Sta->lMDFNew <  0) ||
              (Sta->lMDFOld >= 0 && Sta->lMDFNew >= 0) )
            Sta->lMDFRunning += Sta->lMDFNew;
         else  
         {
            Sta->lCycCnt++;
            lMDFTotal += labs( Sta->lMDFRunning );
            Sta->lMDFRunning = Sta->lMDFNew;
         }
      }
      Sta->lSampOld = Sta->lSampNew;
      Sta->lMDFOld = Sta->lMDFNew;
   }
   
   if ( Sta->lCycCnt == 0 ) Sta->lCycCnt = 1;
   Sta->dAveMDF = (double) lMDFTotal / (double) Sta->lCycCnt;
}

void GetNoise( long lBIndex, long lBNum, STATION *Sta )
{
   long    i, lTemp;
   long    lHigh, lLow;         

   lHigh = -10000000;
   lLow = 10000000;
   
   for ( i=0; i<lBNum; i++ )
   {
      lTemp = i + lBIndex;
      if ( lTemp >= Sta->lRawCircSize ) lTemp -= Sta->lRawCircSize;   
      if (Sta->plRawCircBuff[lTemp] > lHigh) lHigh = Sta->plRawCircBuff[lTemp];
      if (Sta->plRawCircBuff[lTemp] < lLow) lLow = Sta->plRawCircBuff[lTemp];
   }   
   lTemp = lHigh - lLow;
   if (lTemp == 0) lTemp = 1;
   Sta->lRawNoiseOrig = lTemp;
}

void GetPreferredMag( HYPO *pHypo )
{
   if ( pHypo->iNumMw >= 6 && pHypo->dMwAvg > 6.6 &&
       (pHypo->dMlAvg == 0. || pHypo->dMlAvg > 5.0) )
   {
      pHypo->iNumPMags = pHypo->iNumMw;
      strcpy( pHypo->szPMagType, "w" );
      pHypo->dPreferredMag = pHypo->dMwAvg;
   }
   else if ( pHypo->iNumMwp >= 3 && pHypo->dMwpAvg > 5.5 && pHypo->dMwpAvg < 10. ) 
   {
      pHypo->iNumPMags = pHypo->iNumMwp;
      strcpy( pHypo->szPMagType, "wp" );
      pHypo->dPreferredMag = pHypo->dMwpAvg;
   }
   else if ( pHypo->iNumMwp >= 3 && pHypo->dMwpAvg > 0. && pHypo->dMwpAvg < 10. &&
       (pHypo->dMlAvg == 0. || pHypo->dMlAvg > 5.0) )
   {
      pHypo->iNumPMags = pHypo->iNumMwp;
      strcpy( pHypo->szPMagType, "wp" );
      pHypo->dPreferredMag = pHypo->dMwpAvg;
   }
   else if ( pHypo->iNumMS && pHypo->dMSAvg > 5.5 )
   {
      pHypo->iNumPMags = pHypo->iNumMS;
      strcpy( pHypo->szPMagType, "S" );
      pHypo->dPreferredMag = pHypo->dMSAvg;
   }
   else if ( (pHypo->iNumMb && pHypo->dMlAvg == 0.) ||
             (pHypo->iNumMb > 2) )
   {
      pHypo->iNumPMags = pHypo->iNumMb;
      strcpy( pHypo->szPMagType, "b" );
      pHypo->dPreferredMag = pHypo->dMbAvg;
   }
   else if ( pHypo->iNumMl )
   {
      pHypo->iNumPMags = pHypo->iNumMl;
      strcpy( pHypo->szPMagType, "l" );
      pHypo->dPreferredMag = pHypo->dMlAvg;
   }
   else
   {
      pHypo->iNumPMags = 0;
      strcpy( pHypo->szPMagType, "X" );
      pHypo->dPreferredMag = 0.;
   }
}

void ZeroMagnitudes( PPICK *P, int iNum )
{
   int     i;

   for ( i=0; i<iNum; i++ )
   {
      P[i].dMbMag  = 0.;
      P[i].dMlMag  = 0.;
      P[i].dMSMag  = 0.;
      P[i].dMwpMag = 0.;
      P[i].dMwMag  = 0.;
      P[i].iMbClip = 0;
      P[i].iMlClip = 0;
      P[i].iMSClip = 0;
      P[i].iMwClip = 0;
   }
}

int LoadBVals( char *pszBValFile )
{
   FILE    *hFile;        
   int     i;

   if ( (hFile = fopen( pszBValFile, "r" )) == NULL )  
   {
      logit( "t", "B-value file not opened - %s\n", pszBValFile );
      return -1;
   }

   for ( i=0; i<2500; i++ )
      if ( fscanf( hFile, "%2d", &iBVal[i]) == EOF )  
      {
         fclose( hFile );
         logit( "t", "B-value file too short; stop at %d\n", i );
         return -1;
      }
   fclose( hFile );
   return 0;
}                

/* Las funciones MbMlCtsFromGM()/MbMlGroundMotion() y MsCtsFromGM()/
   MsGroundMotion() usan dos convenciones de ganancia, seleccionadas por el
   valor de dSens (o dGain):
     - dSens >= 1E7 : ganancia digital broadband (counts/(m/s)). La amplitud
       es proporcional a la velocidad del suelo y requiere la division por
       omega (2*PI*periodo) para obtener el desplazamiento en nm.
     - dSens < 1E7  : ganancia analogica/heli (cuentas analogicas). La
       conversion a desplazamiento (nm) es directa, sin normalizar por omega.
   Ambas ramas devuelven desplazamiento de suelo en nm y cada par
   CtsFromGM/GroundMotion es inverso exacto dentro de su rama. No son
   equivalentes entre si: se conservan ambas por completitud para soportar
   instrumentos de ambas convenciones. (Ver nota original en todo.dat.) */
long MbMlCtsFromGM( char *pszChannel, double dSens, long lPer, double dAmp )
{
   double dResponse;             
   double dSPResp[] = {0.4,2.66,3.58,3.02,2.61,2.05,1.75,1.46,1.2,1.0,
        .8,.64,.53,.43,.36,.31,.27,.21,.19,.16,.13,.11,.1,.09,
        .08,.072,.063,.056,.048,.046,.043,.039,.036,.032,.030,
        .027,.025,.0225,.021,.020 };  
   double dSPFResp[] = {0.0833,0.7420,0.9924,1.0076,1.0076,1.0076,1.0076,1.0076,
        1.0076,1.0076,1.0038,1.0000,1.0000,1.0000,0.9621,0.9242,0.8864,0.8409,
        0.7803,0.7121,0.6515,0.5833,0.5227,0.4621,0.4167,0.3712,0.3333,0.2954,
        0.2727,0.2424,0.2200,0.1970,0.1818,0.1667,0.1515,0.1363,0.1250,0.1136,
        0.1023,0.0909};	              
   double dSPLResp[] = {.08,.18,.40,.70,1.01,1.26,1.33,1.3,1.16,1.0,.88,
        .76,.64,.52,.39,.35,.31,.26,.21,.16,.14,.125,.112,.10,.091,
        .082,.073,.064,.055,.047};    
   long   lAmp;                  

   if ( dSens == 0.) return (0);

   if ( lPer < 1 || lPer > 40 ) return (0);

   if ( !strcmp (pszChannel, "SHZ") || !strcmp (pszChannel, "SMZ") )	
      dResponse = dSPResp[lPer-1];
	  
   else if ( !strcmp (pszChannel, "SLZ") )	
      dResponse = dSPLResp[lPer-1];
	  
   else if ( !strncmp (pszChannel, "BH", 2) ||
             !strncmp (pszChannel, "HH", 2) ||
             !strncmp (pszChannel, "EH", 2) )	
      dResponse = dSPFResp[lPer-1];
	
   else	
      dResponse = dSPFResp[lPer-1];
	  
   if ( dSens < 1.E7 )   
      lAmp = (long) ((dAmp*dSens*dResponse) / 1000.0);
   else   
      lAmp = (long) ((dAmp*2.*PI*dSens*dResponse*1./((double)lPer/10.0)) / 1.E9);
   return( lAmp );	
}

double MbMlGroundMotion( char *pszChannel, double dSens, long lPer, long lAmp )
{
   double dAmp;                  
   double dResponse;             
   double dSPResp[] = {0.4,2.66,3.58,3.02,2.61,2.05,1.75,1.46,1.2,1.0,
        .8,.64,.53,.43,.36,.31,.27,.21,.19,.16,.13,.11,.1,.09,
        .08,.072,.063,.056,.048,.046,.043,.039,.036,.032,.030,
        .027,.025,.0225,.021,.020 };  
   double dSPFResp[] = {0.0833,0.7420,0.9924,1.0076,1.0076,1.0076,1.0076,1.0076,
        1.0076,1.0076,1.0038,1.0000,1.0000,1.0000,0.9621,0.9242,0.8864,0.8409,
        0.7803,0.7121,0.6515,0.5833,0.5227,0.4621,0.4167,0.3712,0.3333,0.2954,
        0.2727,0.2424,0.2200,0.1970,0.1818,0.1667,0.1515,0.1363,0.1250,0.1136,
        0.1023,0.0909};	              
   double dSPLResp[] = {.08,.18,.40,.70,1.01,1.26,1.33,1.3,1.16,1.0,.88,
        .76,.64,.52,.39,.35,.31,.26,.21,.16,.14,.125,.112,.10,.091,
        .082,.073,.064,.055,.047};    
		
   if ( dSens == 0.) return (0.);

   if ( lPer < 1 || lPer > 40 ) return (0.);

   if ( !strcmp (pszChannel, "SHZ") || !strcmp (pszChannel, "SMZ") )	
      dResponse = dSPResp[lPer-1];
	  
   else if ( !strcmp (pszChannel, "SLZ") )	
      dResponse = dSPLResp[lPer-1];
	  
   else if ( !strncmp (pszChannel, "BH", 2) ||
             !strncmp (pszChannel, "HH", 2) ||
             !strncmp (pszChannel, "EH", 2) )	
      dResponse = dSPFResp[lPer-1];
	
   else	
      dResponse = dSPFResp[lPer-1];
	  
   if ( dSens < 1.E7 )   
      dAmp = ((double) lAmp*1000.0) / (dSens*dResponse);
   else   
      dAmp = ((double) lAmp*1.e9) / (2.*PI*dSens*dResponse*
             (1./((double)lPer/10.0)));
   return( dAmp );	
}

long MsCtsFromGM( char *pszChannel, double dSens, long lPer, double dAmp )
{
   double dResponse;             
   double dLPResp[] = {.18,.31,.42,.52,.64,.75,.86,.91,1.1,1.13,1.15,1.17,
        1.18,1.19,1.2,1.16,1.12,1.08,1.04,1.0,.95,.90,.85,.80,.75,
        .71,.67,.63,.59,.55 };   
   double dLPFResp[] = {.001,.001,.001,.001,.001,.013,.023,.033,0.082,0.105,
        0.197,0.303,0.513,0.724,0.855,1.000,1.013,1.026,1.026,1.026,1.020,
        1.013,1.020,1.026,0.974,0.921,0.816,0.724,0.605,0.500};
   long   lAmp;                  

   if ( dSens == 0.) return (0);

   if ( lPer < 1 || lPer > 30 ) return (0);

   if ( !strcmp( pszChannel, "LHZ" ) || !strcmp( pszChannel, "LLZ" ) )	
      dResponse = dLPResp[lPer-1];
	  
   else dResponse = dLPFResp[lPer-1];
	  
   if ( dSens < 1.E7 )        
      lAmp = (long) (dAmp*dSens*dResponse);
   else   
      lAmp = (long) ((dAmp*2.*PI*dSens*dResponse*(1./(double) lPer)) / 1.e6);
   return( lAmp );	
}

double MsGroundMotion( char *pszChannel, double dSens, long lPer, long lAmp )
{
   double dAmp;                  
   double dResponse;             
   double dLPResp[] = {.18,.31,.42,.52,.64,.75,.86,.91,1.1,1.13,1.15,1.17,
        1.18,1.19,1.2,1.16,1.12,1.08,1.04,1.0,.95,.90,.85,.80,.75,
        .71,.67,.63,.59,.55 };   
   double dLPFResp[] = {.001,.001,.001,.001,.001,.013,.023,.033,0.082,0.105,
        0.197,0.303,0.513,0.724,0.855,1.000,1.013,1.026,1.026,1.026,1.020,
        1.013,1.020,1.026,0.974,0.921,0.816,0.724,0.605,0.500};

   if ( dSens == 0.) return (0.);

   if ( lPer < 1 || lPer > 30 ) return (0.);

   if ( !strcmp( pszChannel, "LHZ" ) || !strcmp( pszChannel, "LLZ" ) )	
      dResponse = dLPResp[lPer-1];
	  
   else dResponse = dLPFResp[lPer-1];
	  
   if ( dSens < 1.E7 )        
      dAmp = ((double) lAmp) / (dSens*dResponse);
   else                       
      dAmp = ((double) lAmp*1.e6) / (2.*PI*dSens*dResponse*(1./(double) lPer));
   return( dAmp );	
}
