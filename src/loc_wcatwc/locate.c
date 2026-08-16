#include <stdio.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <earthworm.h>
#include <transport.h>
#include "loc_wcatwc.h"

/* fPP defined in iaspei91.h.  Must be defined as extern for compilation */
extern float fPP[27436];  
extern char  szStnRem[MAX_PBUFFS][MAX_STN_REM][TRACE_STA_LEN];/* Removed stns */

/* --- INICIO PARCHE DE 64 BITS (PROTOTIPOS MATEMATICOS FALTANTES) --- */
void InitialLocator( int, int, int, PPICK *, HYPO *, double, double );
void QuakeSolveIasp( int, PPICK *, HYPO *, EQDEPTHDATA *, int );
void IsItGoodSoln( int, PPICK *, HYPO *, int );
void FindBadPs( int, int, PPICK *, HYPO *, double, double, int, EQDEPTHDATA * );
void ZeroMagnitudes( PPICK *, int );
void ComputeMagnitudes( int, PPICK *, HYPO * );
void GetPreferredMag( HYPO * );
int  IsItSameQuake( HYPO *, HYPO * );
void QuakeLog( int, PPICK *, HYPO *, CITY *, CITY *, char *, char *, char *, char *, int, char * );
void QuakeLog2( char *, HYPO * );
int  SortAllByPTime( const void *, const void * );
void RemoveP( PPICK *, int *, int );
void LoadPagerString( HYPO *, char *, CITY *, CITY *, GPARM * );
void MakeH71Msg( HYPO *, char * );
void MakeTWCMsg( HYPO *, char * );
void NearestCitiesEC( LATLON *, CITY *, CITYDIS * );
void NearestCities( LATLON *, CITY *, CITYDIS * );
char *namnum( double, double, int *, char *, char *, char * );
int  GetRegion( double, double );
/* --- FIN PARCHE DE 64 BITS --- */
      
void AddInMw( char *pszFile, HYPO *pHypo )
{
   double  dMw[MAX_STATIONS];           
   double  dMwSumMod;                   
   FILE    *hFile;                      
   int     i;
   int     iMwCountMod;                 
   HYPO    HypoT;                       
   char    szNet[6], szStn[6], szChn[6];

   pHypo->dMwAvg = 0.;
   pHypo->iNumMw = 0;
   iMwCountMod = 0;
   dMwSumMod = 0.;

   if ( (hFile = (FILE *) fopen( pszFile, "r" )) == NULL ) return;
   
   fscanf( hFile, "%lf\n", &HypoT.dLat );
   fscanf( hFile, "%lf\n", &HypoT.dLon );
   fscanf( hFile, "%lf\n", &HypoT.dOriginTime );  

   if ( IsItSameQuake( &HypoT, pHypo ) )
   {
      while ( !feof( hFile ) )
      {                                   
         fscanf( hFile, "%s %s %s %lf\n", szStn, szChn, szNet, &dMw[pHypo->iNumMw] );
         pHypo->dMwAvg += dMw[pHypo->iNumMw];
	     pHypo->iNumMw += 1;
      }
      if ( pHypo->iNumMw ) pHypo->dMwAvg /= (double) pHypo->iNumMw;
      if ( pHypo->dMwAvg > 0. )
         for ( i=0; i<pHypo->iNumMw; i++ )
            if ( dMw[i] > 0. )
               if ( fabs( pHypo->dMwAvg-dMw[i] ) < 0.6 )
               {
                  iMwCountMod++;
                  dMwSumMod += dMw[i];
               }
      if ( iMwCountMod ) pHypo->dMwAvg = dMwSumMod / (double) iMwCountMod;
      if ( iMwCountMod ) pHypo->iNumMw = iMwCountMod;
   }
fclose (hFile);      	
return;
}
	  
void CheckPBuffTimes( PPICK **pPBuf, int iPBufCnt[], HYPO Hypo[], int iActive,
                      GPARM *Gparm, int iLastCnt[], int iNumRem[],
                      char pszPStnArray[][MAX_NUM_NEAR_STN][TRACE_STA_LEN],
                      int iNumPStn, int iNumNearStn, int iMaxStn )
{
   AZIDELT azidelt;                
   double  dCorr;                  
   double  dTimeExpected;          
   double  dTTDif;                 
   double  dTTDifMax;              
   double  dTTDifQ;                
   int     i, j, k, kk, iNum, iTemp;
   int     iAlreadyUsed;           
   int     iBuff[MAX_PBUFFS];      
   static  int  iDLev;             
   int     iMatch, iPCnt, iNearbyPMatch;
   int     iStnIndex;              

   /* === BLINDAJE DE MEMORIA 1: LIMITAR VARIABLES GLOBALES AL TAMAÑO COMPILADO === */
   int numPBuffs = Gparm->NumPBuffs;
   if (numPBuffs > MAX_PBUFFS) numPBuffs = MAX_PBUFFS;
   
   int safeNumNearStn = iNumNearStn;
   if (safeNumNearStn > MAX_NUM_NEAR_STN) safeNumNearStn = MAX_NUM_NEAR_STN;
   /* ============================================================================= */
	   
   if ( iPBufCnt[iActive] > Gparm->MinPs+1 )
      for ( k=0; k<iPBufCnt[iActive]; k++ )
      {
         if ( pPBuf[iActive][k].iUseMe < 2  &&
              pPBuf[iActive][k].dPTime > 0. &&
              Hypo[iActive].iGoodSoln >= 2  &&
              Hypo[iActive].iNumPs    >= 10 &&
              fabs( pPBuf[iActive][k].dRes ) > 10. &&
              fabs( pPBuf[iActive][k].dRes ) < 20. )
         {
            if (iNumRem[iActive] < MAX_STN_REM) {
                strcpy( szStnRem[iActive][iNumRem[iActive]], pPBuf[iActive][k].szStation );
                iNumRem[iActive]++;
            }
            goto Remove;
         }
			  
         if (((((fabs( pPBuf[iActive][k].dRes ) > 10. &&
             Hypo[iActive].iGoodSoln >= 1) ||
            (Hypo[iActive].iNumPs >= 10 &&
             Hypo[iActive].iGoodSoln >= 2 &&
             fabs( pPBuf[iActive][k].dRes ) > 7.5)) &&
             pPBuf[iActive][k].iUseMe < 2) ||
            (fabs( pPBuf[iActive][k].dRes ) > 180.)) &&
             pPBuf[iActive][k].dPTime > 0. )
         {
            for ( i=0; i<numPBuffs; i++ ) iBuff[i] = -1;   
            for ( i=0; i<numPBuffs; i++ )
            {
               iTemp = iActive+i;
               if ( iTemp >= numPBuffs ) iTemp -= numPBuffs;
               if ( Hypo[iTemp].iNumPs >= Gparm->MinPs &&
                    Hypo[iTemp].iGoodSoln >= 2 && iTemp != iActive )
               {
                  iBuff[iTemp] = -2;
                  iDLev = (int) ( (Hypo[iTemp].dDepth+0.01) / (double) IASP_DEPTH_INC) * IASP_NUM_PER_DEP;
                  if (iDLev < 0) iDLev = 0;
                  if (iDLev >= 27436 - 362) iDLev = 27436 - 362; 
	  
                  azidelt = GetDistanceAz( (LATLON *) &Hypo[iTemp], (LATLON *) &pPBuf[iActive][k] );
                  dTTDif = pPBuf[iActive][k].dPTime - Hypo[iTemp].dOriginTime;
                  dCorr = azidelt.dDelta*(1./IASP_DIST_INC) - floor( azidelt.dDelta*(1./IASP_DIST_INC) );
                  
                  int fpp_idx = (int) (azidelt.dDelta*(1./IASP_DIST_INC));
                  if(fpp_idx < 0) fpp_idx = 0;
                  if(fpp_idx > 360) fpp_idx = 360; 

                  dTTDifQ = fPP[fpp_idx + iDLev] + dCorr*(fPP[fpp_idx + 1 + iDLev] - fPP[fpp_idx + iDLev]);				
				   
                  for ( j=0; j<iPBufCnt[iTemp]; j++ )
                     if ( !strcmp( pPBuf[iTemp][j].szStation, pPBuf[iActive][k].szStation ) &&
                          !strcmp( pPBuf[iTemp][j].szChannel, pPBuf[iActive][k].szChannel ) )
                        goto NextBuffer;
					
                  if ( fabs( dTTDifQ - dTTDif ) <= 5. )
                  {
                     if ( iPBufCnt[iTemp] < iMaxStn )
                     {
                        CopyPBuf( &pPBuf[iActive][k], &pPBuf[iTemp][iPBufCnt[iTemp]] );
                        iPBufCnt[iTemp]++;
                        goto Remove;
                     }
                  }
               }
NextBuffer:;}
	  
            for ( i=0; i<numPBuffs; i++ )
               if ( iBuff[i] != -2 ) 
               {
                  iNum = 0;
                  for ( j=0; j<numPBuffs; j++ )
                     if ( iBuff[j] != -2 )
                     {
                        iAlreadyUsed = 0;
                        /* EL PARCHE kk QUE PREVIENE LA CORRUPCION DEL BUCLE */
                        for ( kk=0; kk<i; kk++ )
                           if ( iBuff[kk] == j ) iAlreadyUsed = 1;
                        if ( iAlreadyUsed == 0 )
                           if ( iPBufCnt[j] > iNum )
                           {
                              iBuff[i] = j;
                              iNum = iPBufCnt[j];
                           }
                     }
               }
            for ( i=0; i<numPBuffs; i++ )
            {
               iTemp = iBuff[i];
               if ( iTemp >= 0 && iTemp != iActive )
               {              
                  iMatch = 0; iNearbyPMatch = 0; iPCnt = 0;
                  for ( iStnIndex=0; iStnIndex<iNumPStn; iStnIndex++ )
                     if ( !strcmp( pPBuf[iActive][k].szStation, pszPStnArray[iStnIndex][0] ) )
                        break;
                        
                  if ( iStnIndex == iNumPStn ) {
                     iNearbyPMatch = 1;
                  } else {
                     int limit_j = iPBufCnt[iTemp]; if(limit_j > iMaxStn) limit_j = iMaxStn;
                     for ( j=0; j<limit_j; j++ )
                        /* EL PARCHE kk QUE PREVIENE LA CORRUPCION DEL BUCLE Y SAFE_NEAR_STN */
                        for ( kk=1; kk<safeNumNearStn; kk++ ) 
                           if ( pszPStnArray[iStnIndex][kk][0] != '\0' && !strcmp( pPBuf[iTemp][j].szStation, pszPStnArray[iStnIndex][kk] ) )
                              iNearbyPMatch = 1;   
                  }

                  iDLev = (int) ( ((double) DEPTHKM+0.01) / (double) IASP_DEPTH_INC) * IASP_NUM_PER_DEP;
                  if (iDLev < 0) iDLev = 0;
                  if (iDLev >= 27436 - 362) iDLev = 27436 - 362;
			  
                  if ( iNearbyPMatch == 1 )
                  {
                     int limit_j = iPBufCnt[iTemp]; if(limit_j > iMaxStn) limit_j = iMaxStn;
                     for ( j=0; j<limit_j; j++ )
                     {
                        azidelt = GetDistanceAz( (LATLON *) &pPBuf[iActive][k], (LATLON *) &pPBuf[iTemp][j] );
                        dTTDif = fabs( pPBuf[iActive][k].dPTime - pPBuf[iTemp][j].dPTime );

                        int fpp_idx = (int) (azidelt.dDelta*(1./IASP_DIST_INC));
                        if(fpp_idx < 0) fpp_idx = 0;
                        if(fpp_idx > 360) fpp_idx = 360;

                        dTTDifMax = fPP[fpp_idx + 1 + 0];
                        if ( dTTDif > (dTTDifMax+10.) && pPBuf[iTemp][j].dPTime > 0.1 ) iPCnt++;
                        if ( !strcmp( pPBuf[iActive][k].szStation, pPBuf[iTemp][j].szStation ) ) iMatch = 1;
                     }
                  }
                     
                  if ( iMatch == 0 && iPCnt == 0 && iNearbyPMatch == 1 )	
                  {
                     if ( iPBufCnt[iTemp] < iMaxStn )
                     {
                        CopyPBuf( &pPBuf[iActive][k], &pPBuf[iTemp][iPBufCnt[iTemp]] );
                        iPBufCnt[iTemp]++;
                        goto Remove;
                     }
                  }
               }
            }

            for ( i=0; i<numPBuffs; i++ )
            {
               iTemp = iActive+i;
               if ( iTemp >= numPBuffs ) iTemp -= numPBuffs;
               if ( iTemp != iActive )
               {
                  if ( iPBufCnt[iTemp] == 0 )
                  {
                     CopyPBuf( &pPBuf[iActive][k], &pPBuf[iTemp][iPBufCnt[iTemp]] );
                     iPBufCnt[iTemp]++;
                     goto Remove;
                  }
               } 
            }
	   
Remove:     RemoveP( pPBuf[iActive], &iPBufCnt[iActive], k );
            k--;             
         }
      }

   if ( Hypo[iActive].iNumPs >= Gparm->MinPs && Hypo[iActive].iGoodSoln >= 2 )
   {
      iDLev = (int) ( (Hypo[iActive].dDepth+0.01) / (double) IASP_DEPTH_INC) * IASP_NUM_PER_DEP;
      if (iDLev < 0) iDLev = 0;
      if (iDLev >= 27436 - 362) iDLev = 27436 - 362;
   
      for ( i=0; i<numPBuffs; i++ )
         if ( i != iActive )
            for ( j=0; j<iPBufCnt[i]; j++ )
               if ( ((Hypo[i].iGoodSoln >= 2 && pPBuf[i][j].iUseMe < 1) ||
                      Hypo[i].iGoodSoln < 2) && pPBuf[i][j].dPTime > 0. &&
                      Hypo[i].iNumPs < MAX_SCAVENGE )
               {				   
                  for ( kk=0; kk<iPBufCnt[iActive]; kk++ )
                     if ( !strcmp( pPBuf[i][j].szStation, pPBuf[iActive][kk].szStation ) &&
                          !strcmp( pPBuf[i][j].szChannel, pPBuf[iActive][kk].szChannel ) )
                        goto NextP;
						
                  azidelt = GetDistanceAz( (LATLON *) &Hypo[iActive], (LATLON *) &pPBuf[i][j] );
                  dCorr = azidelt.dDelta*(1./IASP_DIST_INC) - floor( azidelt.dDelta*(1./IASP_DIST_INC) );
                  
                  int fpp_idx = (int) (azidelt.dDelta*(1./IASP_DIST_INC));
                  if(fpp_idx < 0) fpp_idx = 0;
                  if(fpp_idx > 360) fpp_idx = 360;
                  
                  dTimeExpected = Hypo[iActive].dOriginTime + fPP[fpp_idx + iDLev] + dCorr*(fPP[fpp_idx + 1 + iDLev] - fPP[fpp_idx + iDLev]);				
                  
                  if ( fabs( dTimeExpected-pPBuf[i][j].dPTime ) <= 5. )
                  {
                     if ( iPBufCnt[iActive] < iMaxStn )
                     {
                        CopyPBuf( &pPBuf[i][j], &pPBuf[iActive][iPBufCnt[iActive]] );
                        iPBufCnt[iActive]++;
                        RemoveP( pPBuf[i], &iPBufCnt[i], j );
                     }
                  }
NextP:;        }
   }
}			
	  
void LoadUpPBuff( PPICK *pPStruct, PPICK **pPBuf, int iPBufCnt[], HYPO Hypo[],
                  int *piActive, GPARM *Gparm, EWH *Ewh, CITY city[],
                  int iLastCnt[], int iNumRem[], CITY cityEC[],
                  EQDEPTHDATA pEqDepth[],
                  char pszPStnArray[][MAX_NUM_NEAR_STN][TRACE_STA_LEN],
                  int iNumPStn, int iNumNearStn, int iNumMax )
{
   AZIDELT azidelt;                
   double  dCorr;                  
   double  dMin;                   
   double  dTTDif;                 
   double  dTTDifMax;              
   double  dTTDifQ;                
   int     i, j, k, iNum, iTemp;   
   int     iAlreadyUsed;           
   int     iBuff[MAX_PBUFFS];      
   int     iIndex;                 
   int     iMatch, iPCnt, iNearbyPMatch;
   static  int  iDLev;             
   int     iStnIndex;              
   long    lTime;                  

   /* === BLINDAJE DE MEMORIA 2: LIMITAR VARIABLES GLOBALES === */
   int numPBuffs = Gparm->NumPBuffs;
   if (numPBuffs > MAX_PBUFFS) numPBuffs = MAX_PBUFFS;
   
   int safeNumNearStn = iNumNearStn;
   if (safeNumNearStn > MAX_NUM_NEAR_STN) safeNumNearStn = MAX_NUM_NEAR_STN;
   /* ========================================================= */

   logit("e", "DEBUG TRACER [1]: Iniciando LoadUpPBuff para estacion %s\n", pPStruct->szStation);
   
   if ( pPStruct->dPTime < 0.1 ) {
      for ( i=0; i<numPBuffs; i++ )
         if ( Hypo[i].iQuakeID == pPStruct->lPickIndex ) 
         {		 
            int limit_j = iPBufCnt[i]; if(limit_j > iNumMax) limit_j = iNumMax;
            for ( j=0; j<limit_j; j++ )
               if ( !strcmp( pPBuf[i][j].szStation, pPStruct->szStation ) &&
                    !strcmp( pPBuf[i][j].szChannel, pPStruct->szChannel ) )
               {
                  pPBuf[i][j].dMSAmpGM = pPStruct->dMSAmpGM;
                  pPBuf[i][j].dMSPer   = pPStruct->dMSPer;
                  pPBuf[i][j].dMSTime  = pPStruct->dMSTime;
				  
                  if ( LocateQuake( pPBuf[i], &iPBufCnt[i], Gparm, &Hypo[i], i, Ewh, city, 0, Hypo, iPBufCnt, cityEC, pEqDepth, iNumMax ) < 0 )
                     logit( "et", "Problem in LocateQuake\n" );
                  return;
               }
            if ( iPBufCnt[i] < iNumMax )
            {
               CopyPBuf( pPStruct, &pPBuf[i][iPBufCnt[i]] );
               iPBufCnt[i]++;
               return;
            }
            return;
         }
   }
		 
   if ( pPStruct->dPTime < 0.1 ) return;
   
   logit("e", "DEBUG TRACER [3]: Verificando timeout de picks...\n");
   time( &lTime );
   if ( lTime-(long) pPStruct->dPTime > PPICK_TIMEOUT ) return;
   
   logit("e", "DEBUG TRACER [4]: Verificando picks forzados...\n");
   for ( i=0; i<numPBuffs; i++ )
      if ( Hypo[i].iQuakeID == pPStruct->iHypoID )
      {             
         int limit_j = iPBufCnt[i]; if(limit_j > iNumMax) limit_j = iNumMax;
         for ( j=0; j<limit_j; j++ )
            if ( !strcmp( pPBuf[i][j].szStation, pPStruct->szStation ) &&
                 !strcmp( pPBuf[i][j].szChannel, pPStruct->szChannel ) )
            {
               CopyPBuf( pPStruct, &pPBuf[i][j] );
               /* PARCHE REPICK: pick manual reemplazado -> forzar relocalizacion
                  en tiempo real (el contador no cambia, LocateThread no dispararia) */
               iLastCnt[i] = -1;
               return;
            }
         if ( iPBufCnt[i] < iNumMax )
         {
            CopyPBuf( pPStruct, &pPBuf[i][iPBufCnt[i]] );
            iPBufCnt[i]++;
            return;
         }
         return;
      }
   
   logit("e", "DEBUG TRACER [5]: Verificando duplicados...\n");
   for ( i=0; i<numPBuffs; i++ ) {
      int limit_j = iPBufCnt[i]; if(limit_j > iNumMax) limit_j = iNumMax;
      for ( j=0; j<limit_j; j++ ) {
         if ( (pPBuf[i][j].lPickIndex == pPStruct->lPickIndex &&
               fabs( pPStruct->dPTime-pPBuf[i][j].dPTime ) < Gparm->MaxTimeBetweenPicks*60.) ||
              (!strcmp( pPBuf[i][j].szStation, pPStruct->szStation ) &&
               !strcmp( pPBuf[i][j].szChannel, pPStruct->szChannel ) &&
               fabs( pPStruct->dPTime-pPBuf[i][j].dPTime ) < (double) BUFFER_TIMEOUT) )
         {
            if ( pPStruct->iUseMe == 1 && pPBuf[i][j].iUseMe == 2 ) return;
            CopyPBuf( pPStruct, &pPBuf[i][j] );
            /* PARCHE REPICK: solo picks manuales (iUseMe==2) fuerzan
               relocalizacion en tiempo real; auto picks mantienen el flujo normal */
            if ( pPStruct->iUseMe == 2 ) iLastCnt[i] = -1;
            return;
         }
      }
   }

   for ( i=0; i<numPBuffs; i++ ) iBuff[i] = -1;   
   
   logit("e", "DEBUG TRACER [6]: Revisando buffers existentes...\n");
   for ( i=0; i<numPBuffs; i++ )
   {
      iTemp = *piActive+i;
      if ( iTemp >= numPBuffs ) iTemp -= numPBuffs;
	  
      /* PARCHE 3: Evitar lectura fuera de memoria de szStnRem */
      int limit_rem = iNumRem[iTemp];
      if (limit_rem > MAX_STN_REM) limit_rem = MAX_STN_REM;
      for ( j=0; j<limit_rem; j++ ) {
         if ( !strcmp( szStnRem[iTemp][j], pPStruct->szStation ) ) return;
      }
	                              
      if ( Hypo[iTemp].iNumPs >= Gparm->MinPs && Hypo[iTemp].iGoodSoln >= 2 )
      {	  
         iDLev = (int) ( (Hypo[iTemp].dDepth+0.01) / (double) IASP_DEPTH_INC) * IASP_NUM_PER_DEP;
         if (iDLev < 0) iDLev = 0;
         if (iDLev >= 27436 - 362) iDLev = 27436 - 362;
         iBuff[iTemp] = -2;
         
         azidelt = GetDistanceAz( (LATLON *) &Hypo[iTemp], (LATLON *) pPStruct);
         dTTDif = pPStruct->dPTime - Hypo[iTemp].dOriginTime;
         dCorr = azidelt.dDelta*(1./IASP_DIST_INC) - floor( azidelt.dDelta*(1./IASP_DIST_INC) );
         
         int fpp_idx = (int) (azidelt.dDelta*(1./IASP_DIST_INC));
         if(fpp_idx < 0) fpp_idx = 0;
         if(fpp_idx > 360) fpp_idx = 360;
         
         dTTDifQ = fPP[fpp_idx + iDLev] + dCorr*(fPP[fpp_idx + 1 + iDLev] - fPP[fpp_idx + iDLev]);
         
         if ( pPStruct->dFreq < FREQ_MIN || azidelt.dDelta < DELTA_TELE || Hypo[iTemp].dDepth > 100. ) {
            if ( fabs( dTTDifQ - dTTDif ) <= 10. )
            {
               if ( iPBufCnt[iTemp] < iNumMax )
               {
                  CopyPBuf( pPStruct, &pPBuf[iTemp][iPBufCnt[iTemp]] );
                  iPBufCnt[iTemp]++;
                  return;
               }
               return;
            }
         }
      }
   }
   
   logit("e", "DEBUG TRACER [6.1]: Comprobando fase baja frecuencia...\n");
   
   /* FIX SEGURO: Bloque rediseñado. Se calcula el tiempo esperado cinemático
    * para descartar el pick de baja frecuencia en base a velocidades 
    * teóricas sin usar GetPhaseTime (evitando crasheos por punteros no inicializados).
    */
   if ( pPStruct->dFreq > 0.0 && pPStruct->dFreq < FREQ_MIN ) {
      for ( i=0; i<numPBuffs; i++ )
      {
         iTemp = *piActive+i;
         if ( iTemp >= numPBuffs ) iTemp -= numPBuffs;
	  
         if ( Hypo[iTemp].iNumPs >= Gparm->MinPs+10 && 
              Hypo[iTemp].iGoodSoln >= 2 && 
              Hypo[iTemp].dPreferredMag >= 6.0 )
         {
            /* Calculamos la distancia desde el sismo activo al pick nuevo */
            azidelt = GetDistanceAz( (LATLON *) &Hypo[iTemp], (LATLON *) pPStruct );
            double dist_km = azidelt.dDelta * 111.19;
            
            /* Tiempos de viaje aproximados: Ondas Secundarias (~4.5 km/s) y Superficiales (~3.0 km/s) */
            double t_S = Hypo[iTemp].dOriginTime + (dist_km / 4.5);   
            double t_Surf = Hypo[iTemp].dOriginTime + (dist_km / 3.0);  
            
            dTTDif = pPStruct->dPTime;
            
            /* Rechazar si coincide en el espacio de la onda secundaria de este gran evento */
            if ( fabs( dTTDif - t_S ) <= 45.0 || fabs( dTTDif - t_Surf ) <= 90.0 ) 
            {
               logit("e", "DEBUG TRACER [SAFE]: Fase de baja frecuencia descartada en %s (coincide con coda/superficial de sismo grande QID %d)\n", 
                     pPStruct->szStation, Hypo[iTemp].iQuakeID);
               return;
            }                     
         }
      }
   }
   
   logit("e", "DEBUG TRACER [6.2]: Ordenando buffers...\n");
   for ( i=0; i<numPBuffs; i++ )
      if ( iBuff[i] != -2 )      
      {
         iNum = 0;
         for ( j=0; j<numPBuffs; j++ )
            if ( iBuff[j] != -2 )
            {
               iAlreadyUsed = 0;
               for ( k=0; k<i; k++ )
                  if ( iBuff[k] == j ) iAlreadyUsed = 1;
               if ( iAlreadyUsed == 0 )
                  if ( iPBufCnt[j] > iNum )
                  {
                     iBuff[i] = j;
                     iNum = iPBufCnt[j];
                  }
            }
      }
   
   iDLev = (int) ((double) (DEPTHKM+0.01) / IASP_DEPTH_INC)*IASP_NUM_PER_DEP;	   
   if(iDLev < 0) iDLev = 0;
   if(iDLev >= 27436 - 362) iDLev = 27436 - 362;
   
   logit("e", "DEBUG TRACER [6.3]: Buscando estacion vecina...\n");
   for ( i=0; i<numPBuffs; i++ )
   {
      iTemp = iBuff[i];
      if ( iTemp >= 0 && iTemp != *piActive )      
      {	  
         iMatch = 0; iNearbyPMatch = 0; iPCnt = 0;
         for ( iStnIndex=0; iStnIndex<iNumPStn; iStnIndex++ )
            if ( !strcmp( pPStruct->szStation, pszPStnArray[iStnIndex][0] ) )
               break;
               
         if ( iStnIndex == iNumPStn ) {
             iNearbyPMatch = 1;
         } else {
             int limit_j = iPBufCnt[iTemp]; if(limit_j > iNumMax) limit_j = iNumMax;
             for ( j=0; j<limit_j; j++ )
                for ( k=1; k<safeNumNearStn; k++ ) /* USANDO VARIABLE SEGURA */
                   if ( pszPStnArray[iStnIndex][k][0] != '\0' && !strcmp( pPBuf[iTemp][j].szStation, pszPStnArray[iStnIndex][k] ) )
                      iNearbyPMatch = 1;   
         }

         if ( iNearbyPMatch == 1 ) {
            int limit_j = iPBufCnt[iTemp]; if(limit_j > iNumMax) limit_j = iNumMax;
            for ( j=0; j<limit_j; j++ )
            {
               azidelt = GetDistanceAz( (LATLON *) pPStruct, (LATLON *) &pPBuf[iTemp][j] );
               dTTDif = fabs( pPStruct->dPTime - pPBuf[iTemp][j].dPTime );
               
               int fpp_idx = (int) (azidelt.dDelta*(1./IASP_DIST_INC));
               if(fpp_idx < 0) fpp_idx = 0;
               if(fpp_idx > 360) fpp_idx = 360;

               dTTDifMax = fPP[fpp_idx + 1 + 0];
               if ( dTTDif > (dTTDifMax+10.) && pPBuf[iTemp][j].dPTime > 0.1 ) iPCnt++;
               if ( !strcmp( pPStruct->szStation, pPBuf[iTemp][j].szStation ) ) iMatch = 1;
            }                    		                             
         }

         if ( iMatch == 0 && iPCnt == 0 && iNearbyPMatch == 1 )
         {
            if ( iPBufCnt[iTemp] < iNumMax )
            {
               CopyPBuf( pPStruct, &pPBuf[iTemp][iPBufCnt[iTemp]] );
               iPBufCnt[iTemp]++;
               return;
            }
            return;
         }
      }
   }

   logit("e", "DEBUG TRACER [7]: Creando un nuevo buffer para %s...\n", pPStruct->szStation);
   for ( i=0; i<numPBuffs; i++ )
   {
      iTemp = *piActive+i;
      if ( iTemp >= numPBuffs ) iTemp -= numPBuffs;
         if ( iPBufCnt[iTemp] == 0 )
         {
            CopyPBuf( pPStruct, &pPBuf[iTemp][iPBufCnt[iTemp]] );
            iPBufCnt[iTemp]++;
            return;
         }
   }

   logit("e", "DEBUG TRACER [8]: Reciclando el buffer mas antiguo...\n");
   iIndex = 0; dMin = 1.E20; time( &lTime );
   for ( i=0; i<numPBuffs; i++ ) {
      if ( Hypo[i].dMSAvg == 0. || ((double) lTime-Hypo[i].dOriginTime) > 7200. ) {
         int limit_j = iPBufCnt[i]; if(limit_j > iNumMax) limit_j = iNumMax;
         for ( j=0; j<limit_j; j++ ) {
            if ( pPBuf[i][j].dPTime < dMin && pPBuf[i][j].dPTime > 0.0 )
            {
               dMin = pPBuf[i][j].dPTime;
               iIndex = i;
            }
         }
      }
   }
   
   iPBufCnt[iIndex] = 0; iNumRem[iIndex] = 0; iLastCnt[iIndex] = 0;
   for ( i=0; i<iNumMax; i++ ) InitP( &pPBuf[iIndex][i] );
   InitHypo( &Hypo[iIndex] );
   Hypo[iIndex].iQuakeID += numPBuffs;
   if ( Hypo[iIndex].iQuakeID >= 10000 ) Hypo[iIndex].iQuakeID -= 10000;
   Hypo[iIndex].iVersion = 1; Hypo[iIndex].iAlarmIssued = 0;
   CopyPBuf( pPStruct, &pPBuf[iIndex][iPBufCnt[iIndex]] );
   iPBufCnt[iIndex]++;
   
   logit("e", "DEBUG TRACER [9]: Finalizado correctamente.\n");
}				  

void LoadPagerString( HYPO *pHypo, char *pszMsg, CITY *pcity, CITY *pcityEC, GPARM *Gparm )
{
   CITYDIS CityDis;           
   char    cNS, cEW;          
   int     iFERegion;         
   int     iRegion;           
   int     iTemp;
   LATLON  ll;                
   long    lTime;             
   char    *psz;              
   char    szTemp[64];        
   struct  tm    *tm; 
   
   lTime = (long) (pHypo->dOriginTime+0.5);
   tm = TWCgmtime( lTime );
   if (tm == NULL) {
       strcpy(pszMsg, "POOR ALARM ORIGIN TIME\n");
       return;
   }
   
   GeoGraphic( &ll, (LATLON *) pHypo );
   if ( ll.dLon < 0 ) ll.dLon += 360.;
   iRegion = GetRegion( ll.dLat, ll.dLon );
   if ( ll.dLon > 180. ) ll.dLon -= 360.;
   cNS = 'N';
   if ( ll.dLat < 0. ) cNS = 'S';
   cEW = 'E';
   if ( ll.dLon < 0. ) cEW = 'W';
   
   if ( iRegion >= 10 )                  
      NearestCitiesEC( (LATLON *) pHypo, pcityEC, &CityDis ); 
   else
      NearestCities( (LATLON *) pHypo, pcity, &CityDis );
  
   if ( CityDis.iDis[1] < 320 || CityDis.iDis[0] < 320 )
   {
      if ( CityDis.iDis[1] < CityDis.iDis[0] ) iTemp = 1;
      else                                     iTemp = 0;
      sprintf( szTemp, "%d %s %s", CityDis.iDis[iTemp],
               CityDis.pszDir[iTemp], CityDis.pszLoc[iTemp] );
   }
   else          
   {
      psz = namnum( ll.dLat, ll.dLon, &iFERegion, Gparm->szIndexFile, Gparm->szLatFile, Gparm->szNameFile );
      /* BLINDAJE 64 BITS: Proteger contra lectura de archivos .fer corruptos (puntero nulo) */
      if (psz != NULL && strlen(psz) > 0) {
          psz[strlen (psz)-1] = '\0';
          strcpy( szTemp, psz );
      } else {
          strcpy( szTemp, "Unknown Region" );
      }
   }
   
/* --- INICIO PARCHE DE 64 BITS EN SPRINTF --- */
   sprintf( pszMsg, "%s  M%s %3.1lf  %d STN  %.1lf%c %.1lf%c  "
    "%02d:%02d:%02dZ %d/%d  RES %.1lf  AZM %d ",
    szTemp, pHypo->szPMagType,
    pHypo->dPreferredMag, (int)pHypo->iNumPs, fabs( ll.dLat ), cNS, fabs( ll.dLon ),
    cEW, (int)tm->tm_hour, (int)tm->tm_min, (int)tm->tm_sec, (int)tm->tm_mon+1, (int)tm->tm_mday,
    pHypo->dAvgRes, (int)pHypo->iAzm );
/* --- FIN PARCHE DE 64 BITS --- */
}

void MakeH71Msg( HYPO *pHypo, char *pszMsg )
{
   double       dRes;                  
   double       dTemp;                 
   LATLON       ll;                    
   long         lTime;                 
   char         szTemp[20];
   struct tm    *tm;                   

   strcpy( pszMsg, "\0" );

   lTime = (long) (pHypo->dOriginTime);
   tm = TWCgmtime( lTime );
   if (tm == NULL) {
       strcpy(pszMsg, "TIME_ERROR\n");
       return;
   }
   
   itoaX( (int) tm->tm_year+1900, szTemp ); 
   PadZeroes( 4, szTemp );
   strcpy( pszMsg, szTemp );
   itoaX( (int) tm->tm_mon+1, szTemp );     
   PadZeroes( 2, szTemp );
   strcat( pszMsg, szTemp );
   itoaX( (int) tm->tm_mday, szTemp );      
   PadZeroes( 2, szTemp );             
   strcat( pszMsg, szTemp );
   strcat( pszMsg, " " );                   
   itoaX( (int) tm->tm_hour, szTemp );      
   PadZeroes( 2, szTemp );
   strcat( pszMsg, szTemp );
   itoaX( (int) tm->tm_min, szTemp );       
   PadZeroes( 2, szTemp );
   strcat( pszMsg, szTemp );
   itoaX( (int) tm->tm_sec, szTemp );       
   PadZeroes( 3, szTemp );
   strcat( pszMsg, szTemp );
   strcat( pszMsg, "." );                   
   itoaX( (int) ((pHypo->dOriginTime-floor( pHypo->dOriginTime ))*100.), szTemp );
   PadZeroes( 2, szTemp );                  
   strcat( pszMsg, szTemp );
   strcat( pszMsg, " " );                   
   
   GeoGraphic( &ll, (LATLON *) pHypo );
   itoaX( abs ((int) ll.dLat), szTemp );    
   PadZeroes( 2, szTemp );
   strcat( pszMsg, szTemp );
   if ( ll.dLat > 0 ) strcpy( szTemp, " " );
   else               strcpy( szTemp, "S" );
   strcat( pszMsg, szTemp );               
   dTemp = 60. * fabs( ll.dLat - (int) ll.dLat );
   itoaX( (int) dTemp, szTemp );           
   PadZeroes( 2, szTemp );
   strcat( pszMsg, szTemp );
   strcat( pszMsg, "." );                  
   itoaX( (int) ((dTemp-(int)dTemp)*100.), szTemp );
   PadZeroes( 2, szTemp );                 
   strcat( pszMsg, szTemp );
   strcat( pszMsg, " " );                  
   while ( ll.dLon > 180. ) ll.dLon -= 360.;
   itoaX( abs( (int) ll.dLon ), szTemp );
   PadZeroes( 3, szTemp );                 
   strcat( pszMsg, szTemp );
   if ( ll.dLon > 0 ) strcpy( szTemp, "E" );
   else               strcpy( szTemp, " " );
   strcat( pszMsg, szTemp );               
   dTemp = 60. * fabs( ll.dLon - (int) ll.dLon );
   itoaX( (int) dTemp, szTemp );           
   PadZeroes( 2, szTemp );
   strcat( pszMsg, szTemp );
   strcat( pszMsg, "." );                  
   itoaX( (int) ((dTemp-(int)dTemp)*100.), szTemp );
   PadZeroes( 2, szTemp );                 
   strcat( pszMsg, szTemp );
   strcat( pszMsg, " " );                  
   
   itoaX( (int) (pHypo->dDepth + 0.5), szTemp );
   PadZeroes( 3, szTemp );                 
   strcat( pszMsg, szTemp );
   strcat( pszMsg, ".00" );                
   strcat( pszMsg, " " );                  
   
   strncat( pszMsg, pHypo->szPMagType, 1 );
   strcat( pszMsg, "\0" );                 
   strcat( pszMsg, " " );                  
   itoaX( (int) pHypo->dPreferredMag, szTemp );
   strcat( pszMsg, szTemp );               
   strcat( pszMsg, "." );                  
   itoaX( (int) ((pHypo->dPreferredMag-(int)pHypo->dPreferredMag)*100.), szTemp );                
   PadZeroes( 2, szTemp );
   strcat( pszMsg, szTemp );
   
   itoaX( pHypo->iNumPs, szTemp );         
   PadZeroes( 3, szTemp );
   strcat( pszMsg, szTemp );
   strcat( pszMsg, " " );                  
   
   itoaX( (360-pHypo->iAzm), szTemp );
   PadZeroes( 3, szTemp );                 
   strcat( pszMsg, szTemp );
   
   itoaX( (int) pHypo->dNearestDist, szTemp );
   PadZeroes( 3, szTemp );                 
   strcat( pszMsg, szTemp );
   strcat( pszMsg, "." );                  
   itoaX( (int) ((pHypo->dNearestDist-(int)pHypo->dNearestDist)*10.), szTemp );                
   PadZeroes( 1, szTemp );
   strcat( pszMsg, szTemp );
   
   dRes = pHypo->dAvgRes;
   if ( dRes > 99.99 ) dRes = 99.99;       
   itoaX( (int) dRes, szTemp );            
   PadZeroes( 2, szTemp );
   strcat( pszMsg, szTemp );
   strcat( pszMsg, "." );                  
   itoaX( (int) ((dRes-(int)dRes)*100.), szTemp );
   PadZeroes( 2, szTemp );                 
   strcat( pszMsg, szTemp );
   
   strcat( pszMsg, "            " );       
   
   strcat( pszMsg, "P" );                  
   strcat( pszMsg, "  " );                 
   itoaX( pHypo->iQuakeID, szTemp);        
   PadZeroes( 9, szTemp );
   strcat( pszMsg, szTemp );
   itoaX( pHypo->iVersion, szTemp);        
   PadZeroes( 2, szTemp );
   strcat( pszMsg, szTemp );
   pszMsg[95] = '\n';
   pszMsg[96] = '\0';
}

void MakeTWCMsg( HYPO *pHypo, char *pszMsg )
{
   strcpy( pszMsg, "\0" );

/* --- INICIO PARCHE DE 64 BITS EN SPRINTF --- */
   sprintf( pszMsg, "%d %d %lf %lf %lf %lf %d %d %lf %d %lf %s %d %lf "
                    "%d %lf %d %lf %d %lf %d %lf %d %d  \0",
    pHypo->iQuakeID, pHypo->iVersion, 
    pHypo->dOriginTime, pHypo->dLat, pHypo->dLon,
    pHypo->dDepth, pHypo->iNumPs, pHypo->iAzm,
    pHypo->dAvgRes, pHypo->iGoodSoln, 
	pHypo->dPreferredMag, pHypo->szPMagType, pHypo->iNumPMags,
    pHypo->dMSAvg, pHypo->iNumMS, pHypo->dMwpAvg, pHypo->iNumMwp, 
    pHypo->dMbAvg, pHypo->iNumMb, pHypo->dMlAvg, pHypo->iNumMl, pHypo->dMwAvg,
	pHypo->iNumMw, pHypo->iMagOnly );	
/* --- FIN PARCHE DE 64 BITS --- */
}

void RemoveP( PPICK *pPBuf, int *iPBufCnt, int iIndex )
{
   int     i;
   /* BLINDAJE DE MEMORIA 3: Evitar Indices Negativos por Reduccion Constante */
   if (*iPBufCnt <= 0) return;
   
   for ( i=0; i<*iPBufCnt; i++ )
      if ( i > iIndex )
         CopyPBuf( &pPBuf[i], &pPBuf[i-1] );
   InitP( &pPBuf[*iPBufCnt-1] );
   *iPBufCnt -= 1;
}

int LocateQuake( PPICK *pPBuf, int *piPBufCnt, GPARM *Gparm, HYPO *pHypo,
                 int iIndex, EWH *Ewh, CITY *pcity, int iLoc, HYPO HypoStruct[],
                 int iBufCntStruct[], CITY *pcityEC, EQDEPTHDATA pEqDepth[],
                 int iNumMax )
{
   FILE    *hFile;          
   static  HYPO    HypoLast;
   int     i, j, iCnt;
   int     iBadPIndices[16];
   int     iBufferMatch;    
   static int iInRegion;    
   int     iFirst;          
   int     iMatch;          										
   int     iNewAlarm;       
   int     iNumNewPs;       
   int     iOrigGoodSoln;   
   int     iPCnt;           										
   static int iRegion;      
   static int iRespond;     
   int     iSendIt;         
   static  LATLON ll, ll2;  
   static  OLDQUAKE OldQuake[MAX_QUAKES]; 
   STATION *Sta = NULL;     
   char    szFile[128];      
   char    szPageMsg[256];  
   char    szMessage[512];  
   char    szLastSta[5][8]; 
   char    szTemp[32];      
   char    szTWCMsg[MAX_HYPO_SIZE]; 
   
   /* === BLINDAJE DE MEMORIA 4 === */
   int numPBuffs = Gparm->NumPBuffs;
   if (numPBuffs > MAX_PBUFFS) numPBuffs = MAX_PBUFFS;
   /* ============================= */
   
   logit("e", "DEBUG LQ 1: Entrando a LocateQuake (Picks: %d, iLoc: %d)\n", *piPBufCnt, iLoc);

   iFirst = 1;
   if ( iLoc == 1 ) pHypo->iMagOnly = 0;
   else             pHypo->iMagOnly = 1;

   if ( iLoc == 1 )
   {
      for ( i=0; i<*piPBufCnt; i++ )
         if ( pPBuf[i].iUseMe == 0 ) pPBuf[i].iUseMe = 1;

      iOrigGoodSoln = pHypo->iGoodSoln;
      if ( (pHypo->iNumPs+pHypo->iNumBadPs) >= Gparm->MinPs )
      {
         /* PARCHE DE SEGURIDAD PARA INUMNEWPS */
         iNumNewPs = *piPBufCnt - (pHypo->iNumPs+pHypo->iNumBadPs);
         if ( iNumNewPs < 0 ) iNumNewPs = 0; 
         if ( iNumNewPs > 5 ) iNumNewPs = 5;
         
         for ( i=0; i<iNumNewPs; i++ )
            strcpy( szLastSta[i], pPBuf[*piPBufCnt-(i+1)].szStation );
      }
ReTry:            
      iPCnt = 0;
      for ( i=0; i<*piPBufCnt; i++ )
         if ( pPBuf[i].iUseMe > 0 && pPBuf[i].dPTime > 0. ) iPCnt++;
      if ( iPCnt < Gparm->MinPs )
      {
         logit( "", "Not enough Ps; need %ld, have %ld; buffer %ld; PCnt %ld\n", Gparm->MinPs, *piPBufCnt, iIndex, iPCnt );
         return -1;
      }
	  
      logit("e", "DEBUG LQ 2: Llamando a qsort para ordenar %d picks\n", *piPBufCnt);
      qsort( (void *) pPBuf, *piPBufCnt, sizeof( PPICK ), SortAllByPTime );
	
      pHypo->iDepthControl = 3;                           
      pHypo->dDepth = DEPTHKM;                        
      InitHypo( pHypo );                                 
      
      logit("e", "DEBUG LQ 3: Llamando a InitialLocator (Metodo 1)\n");
      InitialLocator( *piPBufCnt, 3, 1, pPBuf, pHypo, 0., 0. );
      
      logit("e", "DEBUG LQ 4: Llamando a QuakeSolveIasp (Metodo 1)\n");
      QuakeSolveIasp( *piPBufCnt, pPBuf, pHypo, pEqDepth, 1 ); 
      
      logit("e", "DEBUG LQ 5: Llamando a IsItGoodSoln\n");
      IsItGoodSoln( *piPBufCnt, pPBuf, pHypo, Gparm->MinPs );
   
      if ( pHypo->iGoodSoln != 3 )
      {
         logit("e", "DEBUG LQ 6: Solucion no fue 3. Llamando a InitialLocator (Metodo 2)\n");
         InitHypo( pHypo );		
         InitialLocator( *piPBufCnt, 3, 2, pPBuf, pHypo, 0., 0. );
         QuakeSolveIasp( *piPBufCnt, pPBuf, pHypo, pEqDepth, 1 );
         IsItGoodSoln( *piPBufCnt, pPBuf, pHypo, Gparm->MinPs );
      }
   
      if ( pHypo->iGoodSoln != 3 && MAX_TO_KO > 0 )
      {
         logit("e", "DEBUG LQ 7: Llamando a FindBadPs\n");
         InitHypo( pHypo );		
         FindBadPs( *piPBufCnt, 3, pPBuf, pHypo, 0., 0., Gparm->MinPs, pEqDepth );
         IsItGoodSoln( *piPBufCnt, pPBuf, pHypo, Gparm->MinPs );
      }
   
      if ( pHypo->iGoodSoln > 0 && pHypo->iAzm >= 180 &&
           pHypo->iNumPs >= Gparm->MinPs+2 )
      {
         logit("e", "DEBUG LQ 8: Liberando profundidad (Depth float)\n");
         pHypo->iDepthControl = 4;                        
         iCnt = 0;
         for ( i=0; i<*piPBufCnt; i++ )
            if ( pPBuf[i].iUseMe == 0 && pPBuf[i].dRes < 20. )
            {
               pPBuf[i].iUseMe = 1;
               iBadPIndices[iCnt] = i;
               iCnt++;
               if ( iCnt > 15 ) break;
            }
         QuakeSolveIasp( *piPBufCnt, pPBuf, pHypo, pEqDepth, 1 );
         IsItGoodSoln( *piPBufCnt, pPBuf, pHypo, Gparm->MinPs );
         if  ( pHypo->iGoodSoln < 2 )
         {
            for ( i=0; i<iCnt; i++ )
               pPBuf[iBadPIndices[i]].iUseMe = 0;
            pHypo->iDepthControl = 3;                 
            pHypo->dDepth = DEPTHKM;                  
            InitialLocator( *piPBufCnt, 3, 1, pPBuf, pHypo, 0., 0. );
            QuakeSolveIasp( *piPBufCnt, pPBuf, pHypo, pEqDepth, 1 );
            IsItGoodSoln( *piPBufCnt, pPBuf, pHypo, Gparm->MinPs );
         }
      }
     
      if ( iOrigGoodSoln >= 2 && pHypo->iGoodSoln <= 1 &&
           *piPBufCnt > Gparm->MinPs+2 && iFirst == 1 )
      {
         logit("e", "DEBUG LQ 9: Descartando ultimo pick y reintentando (ReTry)\n");
         for ( j=0; j<iNumNewPs; j++ )
            for ( i=0; i<*piPBufCnt; i++ )
               if ( !strcmp( szLastSta[j], pPBuf[i].szStation ) )
               {
                  RemoveP( pPBuf, piPBufCnt, i );
               }
         iFirst = 0;
         goto ReTry;
      }
   }
   else    
   {
      for ( i=0; i<*piPBufCnt; i++ )
         if ( pPBuf[i].dPTime <= 1. ) pPBuf[i].dRes = 0.;
   }
   
   logit("e", "DEBUG LQ 10: Calculando magnitudes...\n");
   ZeroMagnitudes( pPBuf, iNumMax );
   ComputeMagnitudes( *piPBufCnt, pPBuf, pHypo );
   AddInMw( Gparm->szMwFile, pHypo );
   GetPreferredMag( pHypo );
   
   iBufferMatch = -1;
   for ( i=0; i<numPBuffs; i++ )
      if ( i != iIndex && (HypoStruct[i].dLat != 0. || HypoStruct[i].dLon != 0.) )	  
         if ( IsItSameQuake( pHypo, &HypoStruct[i] ) == 1 )
            if ( iBufCntStruct[i] > *piPBufCnt )   
               iBufferMatch = i;
               
   for ( i=0; i<numPBuffs; i++ )
      if ( i != iIndex && HypoStruct[i].dPreferredMag > 5.8 &&
           pHypo->dNearestDist > 40. && pHypo->iGoodSoln >= 2 &&
           pHypo->dOriginTime-HypoStruct[i].dOriginTime < 3600. &&
           pHypo->dOriginTime-HypoStruct[i].dOriginTime > 0. )	  
      {
         pHypo->iGoodSoln = 1;
      }

   iInRegion = 0;
   GeoGraphic( &ll, (LATLON *) pHypo );
   iRegion = GetRegion( ll.dLat, ll.dLon );     
   if ( ll.dLon > 180. ) ll.dLon -= 360.;
   if ( Gparm->SouthernLat <= ll.dLat &&
        Gparm->NorthernLat >= ll.dLat &&
        Gparm->WesternLon <= ll.dLon &&
        Gparm->EasternLon >= ll.dLon ) iInRegion = 1;
   
   if ( iLoc == 1 && pHypo->iVersion == 1 && iBufferMatch < 0 && iInRegion == 1 )
   {
      logit("e", "DEBUG LQ 11: Escribiendo en %s...\n", Gparm->szAutoLoc);
      if ( (hFile = fopen( Gparm->szAutoLoc, "w" )) != NULL )
      {
         fprintf( hFile, "%lf\n", pHypo->dFirstPTime );
         fclose( hFile );
      }
   }
	
   if ( pHypo->iGoodSoln >= 2 && pHypo->iNumPs >= Gparm->MinPs &&
        iBufferMatch < 0  && iLoc == 1 && pHypo->iNumPs < 15 && iInRegion == 1 )
   {
      logit("e", "DEBUG LQ 12: Escribiendo Dummy Data en %s...\n", Gparm->szDummyFile);
      WriteDummyData( pHypo, Gparm->szDummyFile, 1, 1 );
   }
   if ( pHypo->iGoodSoln >= 2 && pHypo->iNumPs >= Gparm->MinPs &&
        iBufferMatch < 0  && iLoc == 1 && iInRegion == 1 )
   {	
      logit("e", "DEBUG LQ 13: Escribiendo P-Time Files (%s)...\n", Gparm->szRTPFile);
      WritePTimeFile( pHypo->iNumPs, pPBuf, Gparm->szRTPFile );     
      strcpy( szFile, Gparm->szPFilePath );     
      sprintf( szTemp, "%d", pHypo->iQuakeID );     
      PadZeroes( 4, szTemp );
      strcat( szFile, szTemp );
      strcat( szFile, ".dat" );
      WritePTimeFile( pHypo->iNumPs, pPBuf, szFile );     
   }

   if ( pHypo->iGoodSoln >= 2 && pHypo->iNumPs >= Gparm->MinPs &&
        iBufferMatch < 0 && iInRegion == 1 )
   {
      logit("e", "DEBUG LQ 14: Enviando reportes al anillo...\n");
      MakeH71Msg( pHypo, szPageMsg );
      MakeTWCMsg( pHypo, szTWCMsg );
      ReportHypo( szPageMsg, szTWCMsg, Gparm->MyModId, Gparm->OutRegion,
                  Ewh->TypeHypoTWC, Ewh->TypeH71Sum2K, Ewh->MyInstId,
                  iLoc, pHypo->iNumPs );
   }
	  
   if ( pHypo->iNumPs >= Gparm->MinPs && pHypo->iGoodSoln >= 2 && iLoc == 1 &&
        iBufferMatch < 0 && iInRegion == 1 )
   {        
      iRespond = 0;
      iSendIt = 0;
      if ( ((((iRegion < 5 || iRegion == 8)  && pHypo->dPreferredMag >= 6.0) ||
           (iRegion >= 10 && iRegion <= 13 && pHypo->dPreferredMag >  5.5) ||
           ((iRegion == 5 || iRegion == 9) && pHypo->dPreferredMag >= 6.0) ||
           (iRegion >= 14 && pHypo->dPreferredMag >= 6.0)) &&
            pHypo->iNumPs < 32 && (pHypo->iNumPs%6) == 0) ||
           (pHypo->dPreferredMag >= Gparm->MinMagToSend &&
            pHypo->iAlarmIssued == 0))
         iRespond = 1;
      if ( ((iRegion < 5 || iRegion == 8)  && pHypo->dPreferredMag >= 3.2) ||
           (iRegion >= 10 && iRegion <= 13 && pHypo->dPreferredMag >  3.2) ||
           ((iRegion == 5 || iRegion == 9) && pHypo->dPreferredMag >= 5.5) ||
           (iRegion >= 14 && pHypo->dPreferredMag >= 5.5) ) iSendIt = 1;
                     
      GeoGraphic( &ll2, (LATLON *) &HypoLast );
      while ( ll2.dLon > 180. ) ll2.dLon -= 360.;
      iNewAlarm = 1; 	  
      if ( fabs( ll2.dLat-ll.dLat ) < 0.5 &&
           fabs( ll2.dLon-ll.dLon ) < 0.5 &&
           fabs( HypoLast.dPreferredMag-pHypo->dPreferredMag ) < 0.2 &&
           fabs( HypoLast.dOriginTime-pHypo->dOriginTime ) < 5.0 &&
           abs( HypoLast.iNumPs-pHypo->iNumPs ) < 5 ) iNewAlarm = 0;
      if ( pHypo->iNumPs < 31 && iSendIt )
      {                    
         if ( iNewAlarm == 1 ) 
         {                   
            strcpy( szMessage, "\0" );
            LoadPagerString( pHypo, szMessage, pcity, pcityEC, Gparm );
            ReportAlarm( Sta, Gparm->MyModId, Gparm->AlarmRegion,
                Ewh->TypeAlarm, Ewh->MyInstId, 4, szMessage, iRespond );
            HypoLast = *pHypo;
         }
      }
   }
   
   if ( pHypo->iNumPs > 11 && pHypo->iGoodSoln >= 2 &&
        pHypo->iAlarmIssued == 0 && iBufferMatch < 0 && iInRegion == 1 )	  
      pHypo->iAlarmIssued = 2;
	   
   if ( pHypo->iNumPs > 12 && pHypo->iGoodSoln < 2 &&
        pHypo->iAlarmIssued == 0 && iLoc == 1 && iBufferMatch < 0 )
   {
      strcpy( szMessage, "\0" );
      sprintf( szMessage, "POOR LOCATION; %d STATIONS", pHypo->iNumPs );
      ReportAlarm( Sta, Gparm->MyModId, Gparm->AlarmRegion,
                   Ewh->TypeAlarm, Ewh->MyInstId, 4, szMessage, 0 );
      pHypo->iAlarmIssued = 1;
   }
	   
   if ( iLoc == 0 && pHypo->iAlarmIssued == 0 &&
        pHypo->dMSAvg > 6.2 && pHypo->iNumMS > 3 && iInRegion == 1 )
   {
      if ( iRegion < 20 )                            
      {
         strcpy( szMessage, "\0" );
         LoadPagerString( pHypo, szMessage, pcity, pcityEC, Gparm );
         ReportAlarm( Sta, Gparm->MyModId, Gparm->AlarmRegion,
                      Ewh->TypeAlarm, Ewh->MyInstId, 4, szMessage, 0 );
         pHypo->iAlarmIssued = 1;
      }
   }

   if ( pHypo->iGoodSoln >= 2 && pHypo->iNumPs >= Gparm->MinPs &&
        iBufferMatch < 0 && iInRegion == 1 )
   {
      logit("e", "DEBUG LQ 15: Entrando a bloque OldQuakes (%s)...\n", Gparm->szOldQuakes);
      if ( (hFile = fopen( Gparm->szOldQuakes, "r" )) != NULL )
      {
         for ( i=0; i<MAX_QUAKES; i++ ) {
            /* --- INICIO PARCHE DE 64 BITS EN FSCANF --- */
            if ( fscanf( hFile, "%lf %lf %lf %lf %d %s %d %d %d %d %lf "
                                "%lf %lf %d %lf %d %lf %d %lf %d %lf %d "
                                "%lf %d\n",
                &OldQuake[i].dOTime, &OldQuake[i].dLat, &OldQuake[i].dLon,
                &OldQuake[i].dPreferredMag, &OldQuake[i].iNumPMags,
                OldQuake[i].szPMagType, &OldQuake[i].iDepth,
                &OldQuake[i].iQuakeID, &OldQuake[i].iVersion,
                &OldQuake[i].iNumPs, &OldQuake[i].dAvgRes,
                &OldQuake[i].dAzm, &OldQuake[i].dMbAvg, &OldQuake[i].iNumMb,
                &OldQuake[i].dMlAvg, &OldQuake[i].iNumMl,
                &OldQuake[i].dMSAvg, &OldQuake[i].iNumMS,
                &OldQuake[i].dMwpAvg, &OldQuake[i].iNumMwp,
                &OldQuake[i].dMwAvg, &OldQuake[i].iNumMw,
                &OldQuake[i].d1stPTime, &OldQuake[i].iGoodSoln ) != 24 ) break;
            /* --- FIN PARCHE DE 64 BITS --- */
         }
         fclose( hFile );
			
         iMatch = -1;
         for ( i=0; i<MAX_QUAKES; i++ )
            if ( pHypo->iQuakeID == OldQuake[i].iQuakeID &&
                 pHypo->iVersion >= OldQuake[i].iVersion &&  
                 pHypo->dOriginTime < OldQuake[i].dOTime + 1200. &&
                 pHypo->dOriginTime > OldQuake[i].dOTime - 1200. )
               iMatch = i;			
               
         hFile = fopen( Gparm->szOldQuakes, "w" );
         if ( iMatch == -1 )          
         {
            /* --- INICIO PARCHE DE 64 BITS EN FPRINTF --- */
            fprintf( hFile, "%lf %lf %lf %lf %d %s %d %d %d %d %lf "
                            "%lf %lf %d %lf %d %lf %d %lf %d %lf %d "
                            "%lf %d\n",
                     pHypo->dOriginTime, ll.dLat, ll.dLon, pHypo->dPreferredMag,
                     pHypo->iNumPMags, pHypo->szPMagType,
                     (int) (pHypo->dDepth + 0.5), pHypo->iQuakeID, 
                     pHypo->iVersion, pHypo->iNumPs, 
                     pHypo->dAvgRes, (double) pHypo->iAzm, pHypo->dMbAvg,
                     pHypo->iNumMb, pHypo->dMlAvg, pHypo->iNumMl, pHypo->dMSAvg,
                     pHypo->iNumMS, pHypo->dMwpAvg, pHypo->iNumMwp,
                     pHypo->dMwAvg, pHypo->iNumMw,
                     pHypo->dFirstPTime, pHypo->iGoodSoln );
            for ( i=0; i<MAX_QUAKES-1; i++ ) {
               if (OldQuake[i].dOTime <= 0.0) break; 
               fprintf( hFile, "%lf %lf %lf %lf %d %s %d %d %d %d %lf "
                               "%lf %lf %d %lf %d %lf %d %lf %d %lf %d "
                               "%lf %d\n",
                        OldQuake[i].dOTime, OldQuake[i].dLat, OldQuake[i].dLon,
                        OldQuake[i].dPreferredMag, OldQuake[i].iNumPMags,
                        OldQuake[i].szPMagType, OldQuake[i].iDepth,
                        OldQuake[i].iQuakeID, OldQuake[i].iVersion,
                        OldQuake[i].iNumPs, OldQuake[i].dAvgRes,
                        OldQuake[i].dAzm, OldQuake[i].dMbAvg,OldQuake[i].iNumMb,
                        OldQuake[i].dMlAvg, OldQuake[i].iNumMl,
                        OldQuake[i].dMSAvg, OldQuake[i].iNumMS,
                        OldQuake[i].dMwpAvg, OldQuake[i].iNumMwp,
                        OldQuake[i].dMwAvg, OldQuake[i].iNumMw,
                        OldQuake[i].d1stPTime, OldQuake[i].iGoodSoln );
            }
            /* --- FIN PARCHE DE 64 BITS --- */
         }
         else             
         {         		
            for ( i=0; i<MAX_QUAKES; i++ ) {
               if ( i == iMatch ) {
                  /* --- INICIO PARCHE DE 64 BITS EN FPRINTF --- */
                  fprintf( hFile, "%lf %lf %lf %lf %d %s %d %d %d %d %lf "
                           "%lf %lf %d %lf %d %lf %d %lf %d %lf %d "
                           "%lf %d\n",
                    pHypo->dOriginTime, ll.dLat, ll.dLon, pHypo->dPreferredMag,
                    pHypo->iNumPMags, pHypo->szPMagType,
                    (int) (pHypo->dDepth + 0.5), pHypo->iQuakeID, 
                    pHypo->iVersion, pHypo->iNumPs, 
                    pHypo->dAvgRes, (double) pHypo->iAzm, pHypo->dMbAvg,
                    pHypo->iNumMb, pHypo->dMlAvg, pHypo->iNumMl, pHypo->dMSAvg,
                    pHypo->iNumMS, pHypo->dMwpAvg, pHypo->iNumMwp,
                    pHypo->dMwAvg, pHypo->iNumMw,
                    pHypo->dFirstPTime, pHypo->iGoodSoln );
               } else {
                  if (OldQuake[i].dOTime <= 0.0) break;
                  fprintf( hFile, "%lf %lf %lf %lf %d %s %d %d %d %d %lf "
                           "%lf %lf %d %lf %d %lf %d %lf %d %lf %d "
                           "%lf %d\n",
                    OldQuake[i].dOTime, OldQuake[i].dLat, OldQuake[i].dLon,
                    OldQuake[i].dPreferredMag, OldQuake[i].iNumPMags,
                    OldQuake[i].szPMagType, OldQuake[i].iDepth,
                    OldQuake[i].iQuakeID, OldQuake[i].iVersion,
                    OldQuake[i].iNumPs, OldQuake[i].dAvgRes,
                    OldQuake[i].dAzm, OldQuake[i].dMbAvg, OldQuake[i].iNumMb,
                    OldQuake[i].dMlAvg, OldQuake[i].iNumMl,
                    OldQuake[i].dMSAvg, OldQuake[i].iNumMS,
                    OldQuake[i].dMwpAvg, OldQuake[i].iNumMwp,
                    OldQuake[i].dMwAvg, OldQuake[i].iNumMw,
                    OldQuake[i].d1stPTime, OldQuake[i].iGoodSoln );
                  /* --- FIN PARCHE DE 64 BITS --- */
               }
            }
         }
         fclose( hFile );
      }
   }
   
   if ( iLoc == 1 )
   {
      logit("e", "DEBUG LQ 16: Escribiendo el log final con QuakeLog...\n");
      QuakeLog( *piPBufCnt, pPBuf, pHypo, pcity, pcityEC, Gparm->szNameFile,
                 Gparm->szNameFileLC, Gparm->szIndexFile, Gparm->szLatFile, 1,
                 NULL );
      QuakeLog2( Gparm->szQLogFile, pHypo );
   }
   
   if ( iLoc == 1 ) pHypo->iVersion++;   
   
   logit("e", "DEBUG LQ 17: FIN DEL PROCESO. Localizacion %d exitosa.\n", pHypo->iQuakeID);
   
   if ( iBufferMatch >= 0 ) return iBufferMatch;
   else                     return iIndex;
}
