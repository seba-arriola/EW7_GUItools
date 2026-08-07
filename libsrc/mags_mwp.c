/******************************************************************
 * mags_mwp.c (PART 2: MWP & ADVANCED DSP ALGORITHMS)             *
 * *
 * Advanced calculations including auto Mwp, wavelet detrending   *
 * and integral logic. Separated from mags.c to prevent           *
 * compiler and linker overflow issues on 64-bit platforms.       *
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
#include <limits.h>
#include <earthworm.h>
#include "earlybirdlib.h"

#ifndef max
#define max(a,b) ((a) > (b) ? (a) : (b))
#endif

/* Global arrays defined in mags.c */
extern double dSPDist[];
extern int iBVal[];

/* Internal prototypes for algorithms */
double ComputeMwpMag( double dMaxIntDisp, double dDelta );
int detrend( double int_len, double dt, double Z_in[], long ncount,
             double prior_motion, double vZ[], double Z_out[], int order,
             double S_to_N );
double wavelet_decomp( double int_len, double Z_in[ ], long ncount, double dt );
int integrate( double *height1, double *height2, int *n1, int *n2, 
               long ncount, double Z_in[], double dt, double int_len );
double Mwp_adjustment( double dDistance );

/* =========================================================
   SISTEMA DE LOGEO FORENSE HARDCODEADO PARA MWP
   ========================================================= */
#define MWP_LOG_FILE "/home/ew8/earthworm/mwp_debug.log"

void MwpTracerLog(const char *fmt, ...) {
    FILE *fp = fopen(MWP_LOG_FILE, "a");
    if (fp) {
        time_t t = time(NULL);
        struct tm *tm_info = gmtime(&t);
        char time_buf[26];
        strftime(time_buf, 26, "%Y-%m-%d %H:%M:%S", tm_info);
        
        fprintf(fp, "[%s] ", time_buf);
        va_list args;
        va_start(args, fmt);
        vfprintf(fp, fmt, args);
        va_end(args);
        fclose(fp);
    }
}

/* ---------------------------------------------------------
   AutoMwp
   ---------------------------------------------------------*/
void AutoMwp( STATION *Sta, PPICK *pPBuf, double dSN, int iMwpSeconds, int iS )
{
   double  dHighSave = 0.0;           
   double  dLowSave = 0.0;            
   double  dOldestTime;        
   double  dTotalDisp;         
   double  dVelMSData[MAXMWPARRAY];
   double  dWinLen;            
   double  dZDispData[MAXMWPARRAY];  
   long    i, iRC;
   int     iHighSave = 0, iLowSave = 0;
   long    lBIndex;            
   long    lBNum;              
   long    lNum;               
   long    lNumInBuff;         
   long    lPIndex;            
   long    lTemp, lTemp2;      
   static  double  x, x2_bar, xcount, S_to_N_prior;
   int     contador_aux;
   int     orig_window = iMwpSeconds;
   double  local_mean = 0.0;
   
   MwpTracerLog("\n--- INICIANDO AutoMwp para %s.%s (Delta: %.2f) ---\n", Sta->szStation, Sta->szChannel, pPBuf->dDelta);

/* Initialize return values early */
   pPBuf->dMwpIntDisp = 0.;
   pPBuf->dMwpMag = 0.;
   pPBuf->dMwpTime = 0.;

/* Is P-time within buffer? */
   if ( iS == 1 ) 
   {
      dOldestTime = Sta->dEndTime - ((double) Sta->lRawCircSize/Sta->dSampRate) + 1./Sta->dSampRate;
      if ( pPBuf->dPTime < dOldestTime || pPBuf->dPTime > Sta->dEndTime ) {
          MwpTracerLog("  [%s] ABORT: P-time (%.2f) fuera del buffer circular [%.2f - %.2f]\n", Sta->szStation, pPBuf->dPTime, dOldestTime, Sta->dEndTime);
          return;
      }
   }                                
   
/* What is buffer index of P-time? */   
   if ( iS == 1 ) 
   {
      lPIndex = Sta->lSampIndexF - (long) ((Sta->dEndTime-pPBuf->dPTime) * Sta->dSampRate) - 1;
      while ( lPIndex <  0 )                 lPIndex += Sta->lRawCircSize;
      while ( lPIndex >= Sta->lRawCircSize ) lPIndex -= Sta->lRawCircSize; 
   }
   else lPIndex = 0;

/* Dynamic Window: Limit integration length avoiding the S-wave arrival */
   if (iMwpSeconds < 200 && pPBuf->dDelta > 0.0)
   {
       int sp_time = 9999;
       if (pPBuf->dDelta < 14.3) {
           for (contador_aux = 0; contador_aux < 160; contador_aux++) {
               if (dSPDist[contador_aux] >= pPBuf->dDelta) {
                   sp_time = contador_aux;
                   break;
               }
           }
       }
       if (sp_time > 2 && sp_time < iMwpSeconds)
       {
           iMwpSeconds = sp_time - 2;
           if (iMwpSeconds < 5) iMwpSeconds = 5; 
           MwpTracerLog("  [%s] Ajuste dinamico por onda S: Ventana reducida de %d a %d segundos (S-P: %d s)\n", Sta->szStation, orig_window, iMwpSeconds, sp_time);
       } else {
           MwpTracerLog("  [%s] Ventana mantenida en %d segundos (S-P estimado: %d s)\n", Sta->szStation, iMwpSeconds, sp_time);
       }
   }

/* How many points to evaluate? */   
   if ( iS == 1 ) 
   {
      lNumInBuff = (long) ((Sta->dEndTime-pPBuf->dPTime) * Sta->dSampRate);
      lNum = (long) (Sta->dSampRate * (double) iMwpSeconds);
      if ( lNum > lNumInBuff ) 
      {
         /* FIX CRITICO: En sistemas en tiempo real NO truncamos la ventana si falta data.
            Abortamos la funcion silenciosamente (retornando con Mwp=0) para que el modulo padre
            vuelva a intentarlo en el proximo latido/segundo. */
         MwpTracerLog("  [%s] ABORT: Faltan datos en buffer. Necesito %ld muestras (%d s), tengo %ld\n", Sta->szStation, lNum, iMwpSeconds, lNumInBuff);
         return;
      }
   }
   else lNum = Sta->lMwpCtr;
   
   /* --- FORENSIC FIX: CALCULATE LOCAL MEAN TO ELIMINATE DC OFFSET --- */
   if ( iS == 1 ) 
   {
      lBNum = (long) ((double) MWP_BACKGROUND_TIME * Sta->dSampRate);
      lBIndex = lPIndex - lBNum;
      while ( lBIndex < 0 ) lBIndex += Sta->lRawCircSize; 
      
      double sum_noise = 0.0;
      double count_noise = 0.0;
      for ( i=0; i<lBNum; i++ ) {
         lTemp = i + lBIndex;
         if ( lTemp >= Sta->lRawCircSize ) lTemp -= Sta->lRawCircSize;   
         if (Sta->plRawCircBuff[lTemp] != INT_MAX) {
             sum_noise += (double)Sta->plRawCircBuff[lTemp];
             count_noise += 1.0;
         }
      }
      if (count_noise > 0.0) local_mean = sum_noise / count_noise;
      else local_mean = Sta->dAveLDCRaw;
      
      x2_bar = 0.;
      xcount = 0.;
      S_to_N_prior = 1.e+50;
      for ( i=0; i<lBNum; i++ )
      {
         lTemp = i + lBIndex;
         if ( lTemp >= Sta->lRawCircSize ) lTemp -= Sta->lRawCircSize;   
         if (Sta->plRawCircBuff[lTemp] != INT_MAX) {
             x = ((double) Sta->plRawCircBuff[lTemp] - local_mean) / Sta->dSens;
             x2_bar += (x * x);
             xcount += 1.;
         }
      }
      if ( xcount > 500. ) 
      {
         x2_bar /= xcount;
         S_to_N_prior = sqrt( x2_bar );
      }
   }
   else {
      local_mean = Sta->dAveLDCRaw; /* En pick_wcatwc, usamos la línea base global calculada */
      S_to_N_prior = Sta->dAveRawNoiseOrig;
   }
   
   dTotalDisp = 0.;
   for ( i=0; i<lNum-1; i++ )
   {
      if ( iS == 1 ) 
      {
         lTemp = i + lPIndex;
         if ( lTemp >= Sta->lRawCircSize ) lTemp -= Sta->lRawCircSize;
         lTemp2 = lTemp + 1;
         if ( lTemp2 >= Sta->lRawCircSize ) lTemp2 -= Sta->lRawCircSize;
         
         double val1 = (Sta->plRawCircBuff[lTemp] == INT_MAX) ? local_mean : (double)Sta->plRawCircBuff[lTemp];
         double val2 = (Sta->plRawCircBuff[lTemp2] == INT_MAX) ? local_mean : (double)Sta->plRawCircBuff[lTemp2];
         
         Sta->pdRawDispData[i] = dTotalDisp + 1./Sta->dSampRate*0.5*
          ((val1 - local_mean)/Sta->dSens + (val2 - local_mean)/Sta->dSens);
         dVelMSData[i] = (val1 - local_mean) / Sta->dSens;
      }
      else 
      {
         double val1 = (double)Sta->plRawData[i];
         double val2 = (double)Sta->plRawData[i+1];
         Sta->pdRawDispData[i] = dTotalDisp + 1./Sta->dSampRate*0.5*
          ((val1 - local_mean)/Sta->dSens + (val2 - local_mean)/Sta->dSens);
         dVelMSData[i] = (val1 - local_mean) / Sta->dSens;
      }
      dTotalDisp = Sta->pdRawDispData[i];
   }
   
   MwpTracerLog("  [%s] Ejecutando detrend()...\n", Sta->szStation);
   iRC = detrend( 200., 1./Sta->dSampRate, Sta->pdRawDispData, lNum,
                  S_to_N_prior, dVelMSData, dZDispData, 1, dSN );
   if ( iRC == -1 || iRC == -2 ) {
       MwpTracerLog("  [%s] ABORT: detrend() fallo con codigo %d (Señal muy ruidosa o termino lineal grande)\n", Sta->szStation, iRC);
       return;
   }
			
   for (i=0; i<lNum; i++) Sta->pdRawDispData[i] = dZDispData[i];			
   
   if ( iRC == -2 ) dWinLen = 20.;
   else dWinLen = wavelet_decomp( (double) iMwpSeconds, dZDispData, lNum, 1./Sta->dSampRate );		  
   MwpTracerLog("  [%s] wavelet_decomp() asigno una longitud de ventana dWinLen = %.2f s\n", Sta->szStation, dWinLen);
   
   if (dWinLen <= 0.) {
       MwpTracerLog("  [%s] ABORT: dWinLen <= 0 (%.2f)\n", Sta->szStation, dWinLen);
       return;
   }
   if ( dWinLen > (double) iMwpSeconds ) dWinLen = (double)iMwpSeconds;
   lNum = (long) ( dWinLen*Sta->dSampRate );
   if ( iS == 0 ) if ( lNum > Sta->lMwpCtr ) lNum = Sta->lMwpCtr;
   if ( lNum > MAXMWPARRAY ) lNum = MAXMWPARRAY-1;

   dTotalDisp = 0;
   for (i=0; i<lNum-1; i++)
   {
      Sta->pdRawIDispData[i] = dTotalDisp + 1./Sta->dSampRate*0.5*(dZDispData[i]+dZDispData[i+1]);
      dTotalDisp = Sta->pdRawIDispData[i];
   }
   
   MwpTracerLog("  [%s] Ejecutando integrate()...\n", Sta->szStation);
   iRC = integrate (&dHighSave, &dLowSave, &iHighSave, &iLowSave, lNum, dZDispData, 1./Sta->dSampRate, dWinLen);
   if (iRC < 0) {
       MwpTracerLog("  [%s] ABORT: integrate() fallo devolviendo %d\n", Sta->szStation, iRC);
       return;
   }
      
   pPBuf->dMwpIntDisp = fabs( dHighSave - dLowSave );
   pPBuf->dMwpTime = max( (double) iHighSave/Sta->dSampRate, (double) iLowSave/Sta->dSampRate );
						  
   if ( (iHighSave == 0 && iLowSave == 0) || pPBuf->dMwpTime < 2.01 )
   {
      MwpTracerLog("  [%s] ABORT: Integral extrema nula (High=%d, Low=%d) o MwpTime muy corto (%.2f)\n", Sta->szStation, iHighSave, iLowSave, pPBuf->dMwpTime);
      pPBuf->dMwpIntDisp = 0.;
      pPBuf->dMwpMag = 0.;
      pPBuf->dMwpTime = 0.;
   } else {
      MwpTracerLog("  [%s] EXITO en AutoMwp: dMwpIntDisp = %g, dMwpTime = %.2f\n", Sta->szStation, pPBuf->dMwpIntDisp, pPBuf->dMwpTime);
   }
}

/* ---------------------------------------------------------
   ComputeMwpMag (Whitmore distance adjustment)
   ---------------------------------------------------------*/
double ComputeMwpMag( double dMaxIntDisp, double dDelta )
{
   double  dDeltaT;            
   double  dMoment;            
   double  dMwp;               
   double V_p;

   dDeltaT = dDelta;
   
   if ( dDelta == 0.0 ) dDeltaT = 0.1;
   
   V_p = (160.0*dDeltaT + 7900.0);
   
   dMoment = 4*PI*3400*V_p*V_p*V_p* dDeltaT * 111194.9 * dMaxIntDisp;
   if ( dMoment > 0. )
   {
      dMwp = 1./1.5*(log10( dMoment ) - 9.1);
      
      double adj = Mwp_adjustment(dDelta);
      dMwp += adj;
      
      MwpTracerLog("    [ComputeMwpMag] dDelta=%.2f, dMoment=%g, Ajuste=%.2f -> Mwp Final=%.2f\n", dDelta, dMoment, adj, dMwp);
      return( dMwp ); 
   }
   else {
      MwpTracerLog("    [ComputeMwpMag] ABORT: dMoment no es > 0 (%g)\n", dMoment);
      return( 0.0 );
   }
}

/* ---------------------------------------------------------
   detrend
   ---------------------------------------------------------*/
int detrend( double int_len, double dt, double Z_in[], long ncount,
             double prior_motion, double vZ[], double Z_out[], int order,
             double S_to_N )
{
   double x, z, sum1 = 0., sum2 = 0., sum3 = 0.;
   double sum4 = 0., sum5 = 0., sum6 = 0.;
   double z1 = 0., z2 = 0., z3 = 0.;
   double x1, x2, x3;
   double slope, slope2, curv, cube, denom;
   double error_linear, error_quadratic, rms_signal_amp;
   long  n, ntop;
   int top, result = 0;
  
   for ( n=0; n<ncount; n++ ) Z_out[n] = Z_in[n] - Z_in[0];
	
   ncount -= 2;

   error_linear = 0.;
   error_quadratic = 0.;
   rms_signal_amp = 0.;
   slope = 0.;
   slope2 = 0.;
   x = 0.;
   ntop = (int) (int_len / dt);
   if (ntop > ncount) ntop = ncount;
   top = (int) (20. / dt);
   if (top > ntop) top = ntop;

   for ( n=0; n<top; n++ ) rms_signal_amp += (vZ[n] * vZ[n]);
   rms_signal_amp = sqrt( rms_signal_amp / (double) top );
  
   if ( prior_motion == 0.0 || (rms_signal_amp / prior_motion) < S_to_N ) {
       MwpTracerLog("    [detrend] SNR rechazado (RMS/Prior = %g < %g)\n", (prior_motion>0.0)?(rms_signal_amp/prior_motion):0.0, S_to_N);
       return (-1);
   }

   if ( order == 1 )
   {
      for ( n=0; n<ntop; n++ )
      {
         x = (double) n;
         sum1 += (Z_out[n] * x);
         sum2 += (x * x);
      }
      slope = sum1 / sum2;
      for ( n=0; n<ncount; n++ )
      {
         x = (double) n;
         Z_out[n] -= (slope * x); 
      }   
      result = 1;
      x = (double) ntop;
      error_linear = sqrt( slope*slope * x*x / 3. );
	
      sum1 = 0.; sum2 = 0.; sum3 = 0.; sum4 = 0.; sum5 = 0.;
      for ( n=0; n<ntop; n++ )
      {
         x = (double) n;
         z = Z_in[n] - Z_in[0];
         sum1 += (z * x);
         sum2 += (x * x);
         sum3 += (x * x * x);
         sum4 += (z * x * x);
         sum5 += (x * x * x * x);
      }
      double denom_quad = (sum2 * sum5 - sum3 * sum3);
      if (denom_quad != 0.0) {
          slope2 = (sum1 * sum5 - sum4 * sum3) / denom_quad;
          curv = (sum2 * sum4 - sum1 * sum3) / denom_quad;
      } else {
          slope2 = 0.0; curv = 0.0;
      }
      error_quadratic = slope2 * slope2 / 3.;
      error_quadratic += (curv * slope2 * x / 2.);
      error_quadratic += (curv * curv * x * x / 5.);
      if (error_quadratic > 0) error_quadratic = sqrt(error_quadratic) * x;
      else error_quadratic = 0;
	
      if (rms_signal_amp > 0) {
          error_linear = error_linear / rms_signal_amp;
          error_quadratic = error_quadratic / rms_signal_amp;
      }

      if ( (rms_signal_amp/prior_motion) < 3.5 )
      {
        MwpTracerLog("    [detrend] S/N marginal o termino lineal grande (SNR=%g, ErrLin=%e)\n", rms_signal_amp / prior_motion, error_linear); 
        return (-2);
      } 
   }
	
   if ( order == 2 )
   {
      for ( n=0; n<ntop; n++ )
      {
         x = (double) n;
         sum1 += (Z_out[n] * x);
         sum2 += (x * x);
         sum3 += (x * x * x);
         sum4 += (Z_out[n] * x * x);
         sum5 += (x * x * x * x);
      }
      double denom_quad = (sum2 * sum5 - sum3 * sum3);
      if (denom_quad != 0.0) {
          slope = (sum1 * sum5 - sum4 * sum3) / denom_quad;
          curv = (sum2 * sum4 - sum1 * sum3) / denom_quad;
      } else {
          slope = 0.0; curv = 0.0;
      }

      for ( n=0; n<ncount; n++ )
      {
         x = (double) n;
         Z_out[n] -= ((slope + curv*x) * x); 
      }   
      result = 2;
   }

   if ( order == 3 )
   {
      for ( n=0; n<ntop; n++ )
      {
         x1 = (double) n;
         x2 = x1 * x1;
         x3 = x2 * x1;
         z1 += (Z_out[n] * x1);
         z2 += (Z_out[n] * x2);
         z3 += (Z_out[n] * x3);
         sum2 += x2;
         sum3 += x3;
         sum4 += (x2 * x2);
         sum5 += (x3 * x2);
         sum6 += (x3 * x3);
      }

      denom = (sum4*sum6 - sum5*sum5) * (sum3*sum4 - sum2*sum5) - (sum3*sum5 - sum4*sum4) * (sum5*sum4 - sum6*sum3);
      if (denom != 0.0) {
          slope = ((sum4*sum6 - sum5*sum5) * (sum4*z2 - sum5*z1) - (sum3*sum5 - sum4*sum4) * (sum5*z3 - sum6*z2)) / denom;
          double denom_curv = (sum3*sum5 - sum4*sum4);
          if (denom_curv != 0.0) curv = (sum5*z1 - sum4*z2 - slope * (sum5*sum2 - sum3*sum4)) / denom_curv;
          else curv = 0.0;
          if (sum4 != 0.0) cube = (z1 - slope*sum2 - curv*sum3) / sum4;
          else cube = 0.0;
      } else {
          slope = 0.0; curv = 0.0; cube = 0.0;
      }

      for ( n=0; n<ncount; n++ )
      {
         x = (double) n;
         Z_out[n] -= ((slope + curv*x + cube*x*x) * x); 
      }
      result = 3;
   }
   return ( result );
}            

/* ---------------------------------------------------------
   wavelet_decomp
   ---------------------------------------------------------*/
double wavelet_decomp( double int_len, double Z_in[ ], long ncount, double dt )						
{
   int     i, k, l, m, n, ntop, n_remaining;
   static  double  x[MAXMWPARRAY], x_next[MAXMWPARRAY], w[MAXMWPARRAY];
   int     ic, index, f_index = 0, mult = 1, division_counter;
   double  y, z, smooth[4], nonsmooth[4];
   double  smooth2[6], nonsmooth2[6];
   double  avg_count = 0., avg = 0., two = 2.;
   double  average[100], xform[2][100], error[2][500];
   double  xmin, tau, a, fc, wi;
   double  sum0, sum1, sum2, sum3;
   double  z1 = 0., z2 = 0., z3 = 0.;
   static  double  decomp_mag[MAXMWPARRAY];
   double  weight[100];
   
   ncount -= 2;
   ntop = (int) (int_len / dt);
   if (ntop > (ncount-2)) ntop = ncount - 2;
   
   for ( n=0; n<100; n++ ) weight[n] = 1.;
   for ( n=1; n<20; n++ ) weight[n] = 2. * weight[n-1];
  
   for ( n=0; n<ntop; n++ )
   {
     w[n] = 0.0;
     x[n] = Z_in[n];
     x_next[n] = Z_in[n];
   }
  
   smooth[0] = (1. + sqrt(3.0)) / (4. * sqrt(2.));
   smooth[1] = (3. + sqrt(3.0)) / (4. * sqrt(2.));
   smooth[2] = (3. - sqrt(3.0)) / (4. * sqrt(2.));
   smooth[3] = (1. - sqrt(3.0)) / (4. * sqrt(2.));
   
   smooth2[0] = .332671;
   smooth2[1] = .806891;
   smooth2[2] = .459877;
   smooth2[3] = -.135011;
   smooth2[4] = -.085441;
   smooth2[5] = .035226;
  
   z = 1.;
   for (n = 0; n < 6; n++)
   {
      nonsmooth2[n] = z * smooth2[5 - n];
      if (n < 4) nonsmooth[n] = z * smooth[3 - n];
      z *= -1.;
   }

   n_remaining = ntop;
   m = 0;
   division_counter = 0;
   while ( n_remaining >= 5 )
   {
      k = 0;
      for ( n=0; n<n_remaining; n+=2 )
      {
         if ( (n + 5) < n_remaining )
         {
            x_next[k] = 0.0;
            w[m] = 0.0;
            for ( l=0; l<6; l++ )
            {
               x_next[k] += smooth2[l] * x[n + l];
               w[m] += nonsmooth2[l] * x[n + l];
            }
            avg += (w[m] * w[m]);
            decomp_mag[k] = w[m] * w[m];
            avg_count += 1.;
		
            m += 1;
            k += 1;
         }                                        
         else
         {
            x_next[k] = 0.0;
            w[m] = 0.0;
            for ( l=0; l<6; l++ )
            {
               if ( (n + l)<n_remaining ) 
               {
                  x_next[k] += smooth2[l] * x[n + l];
                  w[m] += nonsmooth2[l] * x[n + l];
               }
               else
               {
                  x_next[k] += smooth2[l] * x[n + l - n_remaining];
                  w[m] += nonsmooth2[l] * x[n + l - n_remaining];
               }		  
            }
            avg += (w[m] * w[m]);
            decomp_mag[k] = w[m] * w[m];
            avg_count += 1.;		
            m += 1;
            k += 1;
         }
      }
	
      if ( avg_count > 0.5 )
         average[division_counter] = sqrt(avg / avg_count);
      else
         average[division_counter] = 0.0;
	 
      n_remaining  = k;
      avg = 0.;
      avg_count = 0.;
      division_counter += 1;
      for ( n=0; n<k; n++ ) x[n] = x_next[n];
   }
   
   if (average[division_counter-1] < 1.e-50 && division_counter > 0) division_counter -= 1;

   for ( n=0; n<ntop; n++ )
      if ( w[n] < 0.0 ) w[n] = -w[n];

   y = 1. / (two * dt);
   for ( n=1; n<division_counter; n++ )
   y /= two;

   for ( n=(division_counter-1); n>=0; n-- )
   {
      xform[0][division_counter-n-1] = y;
      xform[1][n] = average[division_counter-n-1];
      y *= two;
   }

   for ( index=0; index<500; index++ )
   {
      fc = .005 + .001*(double) index;
      ic = 0;
      while ( xform[0][ic]<fc && ic<division_counter ) ic += 1;

      sum0 = 0.; sum1 = 0.; sum2 = 0.; sum3 = 0.;
      if ( ic>0 )
         for (i = 0; i < ic; i++)
         {
            sum0 += weight[i];
            sum1 += weight[i] * xform[1][i];
         }
      for ( i=ic; i<(division_counter-1); i++ )
      {
         wi = xform[0][i];
         sum2 += weight[i] * xform[1][i] / wi;
         sum3 += weight[i] / (wi * wi);
      }
      double denom_a = (sum0 + fc*fc*sum3);
      if (denom_a != 0.0) a = (sum1 + fc*sum2) / denom_a;
      else a = -1.0;
      
      if (a < 0.0)
         error[1][index] = 2.e+20;
      else if (fc <= xform[0][0])
         error[1][index] = 2.e+20; 
      else
      {
         z = 0.;
         if ( ic>0 )
            for ( i=0; i<ic; i++ )
               z += weight[i] * ((xform[1][i] - a) * (xform[1][i] - a));
         for ( i=ic; i<(division_counter-1); i++ )
         {
            wi = xform[0][i];
            y = (wi != 0.0) ? xform[1][i] - a*fc/wi : 0.0;
            z += weight[i] * (y*y);
         }
         error[0][index] = a;
         error[1][index] = z;
      }
   }

   xmin = 1.e+50;
   for ( i=0; i<500; i++ )
      if ( error[1][i] < xmin )
      {
         ic = i;
         xmin = error[1][i];
      }

   fc = .005 + .001 * (double) ic;
   a = error[0][ic];

   for ( i=0; i<division_counter; i++ )
   {
      wi = xform[0][i];
      if ( wi < fc ) z = a;
      else           z = (wi != 0.0) ? a * fc * fc / (wi * wi) : a; 
   }
   
   if (fc != 0.0) tau = 2.5 / fc; 
   else tau = 200.0;
   
   if ( tau < 5. ) tau = 5.;
   else if ( tau > 200. ) tau = 200.;
   if ( tau > (dt * (double) ncount) ) tau = dt * (double) ncount;

   return( tau );
}

/* ---------------------------------------------------------
   integrate (Shielded against Buffer Overflows)
   ---------------------------------------------------------*/
int integrate( double *height1, double *height2, int *n1, int *n2, 
               long ncount, double Z_in[], double dt, double int_len )
{
   double  max1, max2;
   double  test;
   int     n_max, n, i, array_top, n_start; 
   
   double *zT = (double *)calloc(ncount + 10, sizeof(double));
   double *extrema_val = (double *)calloc(ncount + 10, sizeof(double));
   int    *extrema_idx = (int *)calloc(ncount + 10, sizeof(int));
   
   if (!zT || !extrema_val || !extrema_idx) {
       MwpTracerLog("    [integrate] ABORT: Falla en asignacion de memoria!\n");
       if (zT) free(zT); if (extrema_val) free(extrema_val); if (extrema_idx) free(extrema_idx);
       return -1;
   }

   n_max = 0;
   n_start = (int) (2.5/dt);  
   array_top = (int) (int_len / dt);
   if ( array_top > ncount ) array_top = (int) ncount;
   *n1 = 0;
   *n2 = 0;
   *height1 = 0.;
   *height2 = 0.;
     
   zT[0] = 0.;
   for ( n=1; n<array_top; n++ )
      zT[n] = zT[n-1] + 0.5*dt*(Z_in[n]+Z_in[n-1]);
  
   for ( n=n_start; n<(array_top-1); n++)    
   {
      if ( zT[n] > zT[n-1] && zT[n] > zT[n+1] && zT[n] > 0.0 )
      {
         extrema_val[n_max] = zT[n] * zT[n];
         extrema_idx[n_max] = n;
         n_max += 1;
      }
      else if ( zT[n] < zT[n-1] && zT[n] < zT[n+1] && zT[n] < 0.0 )
      {
         extrema_val[n_max] = zT[n] * zT[n];
         extrema_idx[n_max] = n;
         n_max += 1;
      }
   }

   if ( n_max < 1 )  
   {
      MwpTracerLog("    [integrate] ABORT: Muy pocos extremos encontrados (n_max = %d)\n", n_max);
      free(zT); free(extrema_val); free(extrema_idx);
      return( -1 );
   }
   else if ( n_max == 1 )
   {
      *n1 = extrema_idx[0];
      *height1 = zT[*n1];
      free(zT); free(extrema_val); free(extrema_idx);
      return( 1 );
   }

   max1 = -1.e+10;
   max2 = -1.e+10;
   for ( n=0; n<n_max; n++ )
      if ( extrema_val[n] > max1 )
      {
         max1 = extrema_val[n];
         *n1 = extrema_idx[n];
         *height1 = zT[*n1];
      }

   for ( n=0; n<n_max; n++ )
   {
      i = extrema_idx[n];
      if ( extrema_val[n] > max2 && i != *n1 )
      {
         test = (zT[*n1] != 0.0) ? zT[i] / zT[*n1] : 0.0;
         if ( test < 0.0 ) 
         {
            max2 = extrema_val[n];
            *n2 = i;
         }
      }
   }
   if ( *n2 > 0 )
   {
      *height2 = zT[*n2];
      free(zT); free(extrema_val); free(extrema_idx);
      return ( 1 );
   }                                              
 
   for ( n=0; n<n_max; n++ ) 
   {
      i = extrema_idx[n];
      if ( extrema_val[n] > max2 && i != *n1 )
      {
         test = (zT[*n1] != 0.0) ? zT[i] / zT[*n1] : 0.0;
         if (test > 0.2)   
         {
            max2 = extrema_val[n];
            *n2 = i;
         }
      }
   }
   *height2 = zT[*n2];
  
   if ( *n1 > *n2 )
   {
      *height1 = zT[*n2];
      *n1 = *n2;
   }
   *height2 = 0.0;
   *n2 = 0;

   free(zT); free(extrema_val); free(extrema_idx);
   return( 1 );
}

/* ---------------------------------------------------------
   Mwp_adjustment (Atenuation correction)
   ---------------------------------------------------------*/
double Mwp_adjustment( double dDistance )
{
   double  dMwp_distance_correction[50] = {
    0.280154895, 0.248159091, 0.192827333, 0.113355523, 0.134518163,
    0.142274094, 0.141015269, 0.225545292, 0.270142832, 0.292807679, 
    0.279098314, 0.083464936, 0.032658602, 0.017405582, 0.03411125, 
   -0.01300572, -0.087751188,-0.105951149,-0.159169491,-0.109741881,
   -0.040873702, -0.127091897,-0.0291207,-0.051879539,-0.063962226,
    0.000433825, 0.020766625,-0.036926385,-0.042950165,-0.006198259,
    0.013929801, 0.007052744,0.087026743,-0.007465525,-0.021245992,
    0.039889117, 0.052830981,-0.005707771,-0.022900097,-0.009290172,
   -0.028246945, -0.032688366,-0.020939191,0.096496123,-0.025355755,
   -0.04532547, -0.048598167,-0.105345188,-0.309972643,-0.299542571 };
   int     iIndex;         

   iIndex = (int)(dDistance/2);

   if ( dDistance <= 0.0 ) return 0.0;
   if ( iIndex < 0 || iIndex >= 50 ) return 0.0;
  
   return dMwp_distance_correction[iIndex];
}

/* ====================================================================
   FUNCIONES FALTANTES RESTAURADAS PARA EVITAR ERROR DE ENLAZADO (LD)
   ==================================================================== */

int GetMbMl( STATION *Sta, int iIndex, unsigned char ucMyModID,
             SHM_INFO siPRegion, unsigned char ucEWHTypePickTWC,
             unsigned char ucEWHMyInstID, int iMbCycles, int iRT )
{
   PPICK  PTemp;        

   if ( Sta->dSens > 0. )
   {     
      Sta->lCycCnt++;
      if ( Sta->lCycCnt >= 3 )
      {     
         Sta->lMagAmp += labs (Sta->lMDFRunning);  
         Sta->dAvAmp = (double) Sta->lMagAmp / (double) (Sta->lCycCnt - 2);

         if ( (double) labs (Sta->lMDFRunning) < (Sta->dAvAmp+Sta->dAvAmp/5.) &&
              (double) labs (Sta->lMDFRunning) > (Sta->dAvAmp-Sta->dAvAmp/5.) &&
              Sta->dAvAmp > (2.*Sta->dAveMDF) &&
              Sta->dAvAmp < Sta->dClipLevel ) Sta->lSWSim++;
      }

      if ( (2*(Sta->lSampsPerCyc+1) <= (long) (Sta->dSampRate + Sta->dSampRate/5.)) &&
           (2*(Sta->lSampsPerCyc+1) >= (long) (Sta->dSampRate - Sta->dSampRate/5.)) ) Sta->lSWSim++;

      if ( Sta->lCycCnt <= 13 && Sta->lSWSim >= 18 )
      {
         Sta->iCal = 1;
         logit( "t", "cal on %s\n", Sta->szStation );
      }

      if ( !Sta->iCal )
      {
         Sta->lPer = (long) ((double) (2 * (Sta->lSampsPerCyc + 1)) / Sta->dSampRate * 10. + 0.0001);
					
         if ( Sta->lPer > 30 ) Sta->lPer = 30;
			   
         if ( Sta->lPer < 3 ) Sta->lPer = 3;
			   
         if ( Sta->lCycCnt == iMbCycles ) Sta->dMaxPk = 0.;   
		 
         if ( MbMlGroundMotion( Sta->szChannel, Sta->dSens, Sta->lPer, labs( Sta->lMDFRunning ) ) > Sta->dMaxPk )
         {
            Sta->dMaxPk = MbMlGroundMotion( Sta->szChannel, Sta->dSens, Sta->lPer, labs( Sta->lMDFRunning ) );

            if ( Sta->lCycCnt < iMbCycles )
            {
               Sta->lMbPer = Sta->lPer;
               Sta->dMbAmpGM = Sta->dMaxPk;
               Sta->dMbTime = Sta->dStartTime + (double) iIndex/Sta->dSampRate;			   
               if ( Sta->iPickStatus == 3 && iRT == 1)
                  ReportPick( &PTemp, Sta, ucMyModID, siPRegion, ucEWHTypePickTWC, ucEWHMyInstID, 4 );
            }
            else 
            {
               Sta->lMlPer = Sta->lPer;
               Sta->dMlAmpGM = Sta->dMaxPk;
               Sta->dMlTime = Sta->dStartTime + (double) iIndex/Sta->dSampRate;			   
               if ( Sta->iPickStatus == 3 && iRT == 1 ) 
                  ReportPick( &PTemp, Sta, ucMyModID, siPRegion, ucEWHTypePickTWC, ucEWHMyInstID, 4 );
            }       
         } 
      }
      else
      {
         if ( Sta->dStartTime >= Sta->dTrigTime + 80. )
            {
               Sta->iPickStatus = 1;
               InitVar( Sta );
               return (-1);
            }
      }
   }
return (0);
}

   
