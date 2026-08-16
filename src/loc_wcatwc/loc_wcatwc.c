#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <earthworm.h>
#include <transport.h>
#include "loc_wcatwc.h"

/* Global Variables (those needed in threads) */
CITY   city[NUM_CITIES];       
CITY   cityEC[NUM_CITIES_EC];  
EQDEPTHDATA EqDepth[EQSIZE];   
GPARM  Gparm;                  
HYPO   Hypo[MAX_PBUFFS];       
int    iActiveBuffer;          
int    iLastBuffCnt[MAX_PBUFFS];
int    iPBufCnt[MAX_PBUFFS];   
int    iNumPBufRem[MAX_PBUFFS];
int    Nsta;                   
PPICK  *PBuf[MAX_PBUFFS];      
EWH    Ewh;                    
mutex_t mutsem1;               
STATION *StaArray;             
char   szPStnArray[MAX_STATIONS][MAX_NUM_NEAR_STN][TRACE_STA_LEN];
char   szStnRem[MAX_PBUFFS][MAX_STN_REM][TRACE_STA_LEN];

int main( int argc, char **argv )
{
   int           i, j;            
   int           iRC;
   char          PIn[MAX_PICKTWC_SIZE];
   int           lineLen;         
   char          line[40];        
   long          MsgLen;          
   MSG_LOGO      getlogo;         
   MSG_LOGO      logo;            
   MSG_LOGO      hrtlogo;         
   time_t        then;            
   long          InBufl;          
   char          *configfile;     
   pid_t         myPid;           
   static unsigned tidLocate;     
   
   if ( argc != 2 )
   {
      fprintf( stderr, "Usage: loc_wcatwc <configfile>\n" );
      return -1;
   }
   configfile = argv[1];

   if ( GetConfig( configfile, &Gparm ) == -1 )
   {
      fprintf( stderr, "loc_wcatwc: GetConfig() failed. Exiting.\n" );
      return -1;
   }

   if ( GetEwh( &Ewh ) < 0 )
   {
      fprintf( stderr, "loc_wcatwc: GetEwh() failed. Exiting.\n" );
      return -1;
   }

   getlogo.instid = Ewh.GetThisInstId;
   getlogo.mod    = Ewh.GetThisModId;
   getlogo.type   = Ewh.TypePickTWC;

   hrtlogo.instid = Ewh.MyInstId;
   hrtlogo.mod    = Gparm.MyModId;
   hrtlogo.type   = Ewh.TypeHeartBeat;

   logit_init( configfile, Gparm.MyModId, 256, 1 );

   myPid = getpid();
   if ( myPid == -1 )
   {
      logit( "e", "loc_wcatwc: Can't get my pid. Exiting.\n" );
      return -1;
   }

   LogConfig( &Gparm );

   if ( LoadCities( city, 1, Gparm.CityFileWUC, Gparm.CityFileWLC ) < 0)
   {
      logit( "t", "LoadCities failed, exiting\n" );
      return -1;
   }

   if ( LoadCitiesEC( cityEC, 1, Gparm.CityFileEUC, Gparm.CityFileELC ) < 0)
   {
      logit( "t", "LoadCitiesEC failed, exiting\n" );
      return -1;
   }
	
   if ( LoadBVals( Gparm.szBValFile ) < 0 )
   {
      logit( "t", "LoadBVals failed, exiting\n" );
      return -1;
   }

   if ( LoadEQData(Gparm.szDepthDataFile, EQSIZE, EqDepth ) == -1 )
   {
      logit( "t", "LoadEQData failed\n" );
      return -1;
   }

   for ( i=0; i<Gparm.NumPBuffs; i++ )
   {
      InBufl = sizeof( PPICK ) * MAX_STATIONS; 
      PBuf[i] = (PPICK *) malloc( (size_t) InBufl );
      if ( PBuf[i] == NULL )
      {
         logit( "et", "loc_wcatwc: Cannot allocate waveform buffer %d\n", i );
         return -1;
      }
   }

   if ( ReadStationList( &StaArray, &Nsta, Gparm.StaFile, Gparm.StaDataFile,
                         Gparm.ResponseFile, MAX_STATIONS, 0 ) == -1 )
   {
      logit( "", "loc_wcatwc: ReadStationList() failed. Exiting.\n" );
      for ( i=0; i<Gparm.NumPBuffs; i++ ) free( PBuf[i] );
      return -1;
   }
   if ( Nsta == 0 )
   {
      logit( "et", "loc_wcatwc: Empty station list. Exiting." );
      for ( i=0; i<Gparm.NumPBuffs; i++ ) free( PBuf[i] );
      free( StaArray );
      return -1;
   }
   logit( "t", "loc_wcatwc: Displaying %d stations.\n", Nsta );

   for ( i=0; i<Gparm.NumPBuffs; i++ )
   {
      for ( j=0; j<MAX_STATIONS; j++ ) InitP( &PBuf[i][j] );
      iPBufCnt[i] = 0;
      iNumPBufRem[i] = 0;
      InitHypo( &Hypo[i] );
      Hypo[i].iQuakeID = i+1;
      Hypo[i].iVersion = 1;
      Hypo[i].iAlarmIssued = 0;
   }
   iActiveBuffer = 0;

   CreateNearbyStationLookupTable( StaArray, szPStnArray, Nsta, Gparm.iNumNearStn );
   CreateSpecificMutex( &mutsem1 );

   if ( Gparm.OutKey != Gparm.InKey )
   {
      tport_attach( &Gparm.InRegion,  Gparm.InKey );
      tport_attach( &Gparm.OutRegion, Gparm.OutKey );
      tport_attach( &Gparm.AlarmRegion, Gparm.AlarmKey );
   }
   else
   {
      tport_attach( &Gparm.InRegion, Gparm.InKey );
      Gparm.OutRegion = Gparm.InRegion;
   }

   while ( tport_getmsg( &Gparm.InRegion, &getlogo, 1, &logo, &MsgLen, PIn, MAX_PICKTWC_SIZE) != GET_NONE );

   time( &then );
   sprintf( line, "%ld %d\n", (long) then, myPid );
   lineLen = strlen( line );
   if ( tport_putmsg( &Gparm.OutRegion, &hrtlogo, lineLen, line ) != PUT_OK )
   {
      logit( "et", "loc_wcatwc: Error sending 1st heartbeat. Exiting." );
      if ( Gparm.OutKey != Gparm.InKey )
      {
         tport_detach( &Gparm.InRegion );
         tport_detach( &Gparm.OutRegion );
         tport_detach( &Gparm.AlarmRegion );
      }
      else
         tport_detach( &Gparm.InRegion );
      for ( i=0; i<Gparm.NumPBuffs; i++ ) free( PBuf[i] );
      free( StaArray );
      return 0;
   }

   if ( StartThread( LocateThread, 8388608, &tidLocate ) == -1 )
   {
      for ( i=0; i<Gparm.NumPBuffs; i++ ) free( PBuf[i] );
      free( StaArray );
      if ( Gparm.OutKey != Gparm.InKey )
      {
         tport_detach( &Gparm.InRegion );
         tport_detach( &Gparm.OutRegion );
         tport_detach( &Gparm.AlarmRegion );
      }
      else
         tport_detach( &Gparm.InRegion );
      logit( "et", "Error starting Locate thread; exiting!\n" );
      return -1;
   }

   while ( tport_getflag( &Gparm.InRegion ) != TERMINATE )
   {
      int      rc;                
      time_t   now;               
      PPICK    PStruct;           

      time( &now );
      
      if ( (now - then) > 3600 )                               
      {        
         sleep_ew( 5000 );  
         logit( "t", "Large gap noted in locator\n" );
         if ( strlen( Gparm.ATPLineupFileBB ) > 2 )          
         {
            logit( "", "reset StaArray Nsta = %d\n", Nsta );	 
	        free( StaArray );                                  
	        StaArray = (STATION *) calloc( MAX_STATIONS, sizeof(STATION) );
            Nsta = ReadLineupFile( Gparm.ATPLineupFileBB, StaArray );
            logit( "", "New StaArray Nsta = %d\n", Nsta );	 
            if ( Nsta < 1 )
            {
               logit( "", "Bad Lineup File read-%s\n", Gparm.ATPLineupFileBB );
               continue;
            }	   
            CreateNearbyStationLookupTable( StaArray, szPStnArray, Nsta, Gparm.iNumNearStn );
         } 	
      }

      if ( (now - then) >= Gparm.HeartbeatInt )
      {
         then = now;
         sprintf( line, "%ld %d\n", (long) now, myPid );
         lineLen = strlen( line );
         if ( tport_putmsg( &Gparm.OutRegion, &hrtlogo, lineLen, line ) != PUT_OK )
         {
            logit( "et", "loc_wcatwc: Error sending heartbeat." );
            break;
         }
      }

      rc = tport_getmsg( &Gparm.InRegion, &getlogo, 1, &logo, &MsgLen, PIn, MAX_PICKTWC_SIZE);

      if ( rc == GET_NONE ) { sleep_ew( 200 ); continue; }
      if ( rc == GET_NOTRACK ) logit( "et", "loc_wcatwc: Tracking error.\n");
      if ( rc == GET_MISS_LAPPED ) logit( "et", "loc_wcatwc: Got lapped on the ring.\n");
      if ( rc == GET_MISS_SEQGAP ) logit( "et", "loc_wcatwc: Gap in sequence numbers.\n");
      if ( rc == GET_MISS ) logit( "et", "loc_wcatwc: Missed messages.\n");
      if ( rc == GET_TOOBIG )
      {
         logit( "et", "loc_wcatwc: Retrieved message too big (%ld) for msg.\n", MsgLen );
         continue;
      }
	  
      if ( PPickStruct( PIn, &PStruct, Ewh.TypePickTWC ) < 0 ) continue;
	  
      if ( PStruct.iHypoID > 0 && 
           !strcmp( PStruct.szStation, "LOC" ) &&
           !strcmp( PStruct.szChannel, "ATE" ) )    
      {
         logit( "et", "Force location sent from hypo_display.\n" );
         RequestSpecificMutex( &mutsem1 );   
         for ( i=0; i<Gparm.NumPBuffs; i++ )
            if ( Hypo[i].iQuakeID == PStruct.iHypoID )
            {
               logit( "", "Relocate ID %ld\n", PStruct.iHypoID );
               iRC = LocateQuake( PBuf[i], &iPBufCnt[i], &Gparm, &Hypo[i], i,
                                  &Ewh, city, 1, Hypo, iPBufCnt, cityEC,
                                  EqDepth, MAX_STATIONS );
               if ( iRC == -1 )
                  logit( "et", "Problem in LocateQuake-2\n" );
               if ( iRC >= 0 && Hypo[i].iGoodSoln >= 2 ) iActiveBuffer = iRC;
               break;
            }
         if ( i >= Gparm.NumPBuffs )
            logit( "et", "Force location ignored: quake ID %ld no esta activo en los buffers.\n", PStruct.iHypoID );
         ReleaseSpecificMutex( &mutsem1 );   
         continue;
      }

      if ( PStruct.iHypoID > 0 && PStruct.iUseMe == 0 )
      {
         /* PARCHE BORRADO: hypo_display elimino un pick manual (iUseMe==0).
            Se remueve del buffer del sismo y se fuerza relocalizacion. */
         RequestSpecificMutex( &mutsem1 );
         for ( i=0; i<Gparm.NumPBuffs; i++ )
            if ( Hypo[i].iQuakeID == PStruct.iHypoID )
            {
               for ( j=0; j<iPBufCnt[i]; j++ )
                  if ( !strcmp( PBuf[i][j].szStation, PStruct.szStation ) &&
                       !strcmp( PBuf[i][j].szChannel, PStruct.szChannel ) )
                  {
                     logit( "et", "Pick eliminado de %s %s en quake ID %ld (buffer %d).\n",
                            PStruct.szStation, PStruct.szChannel, PStruct.iHypoID, i );
                     RemoveP( PBuf[i], &iPBufCnt[i], j );
                     iLastBuffCnt[i] = -1;
                     break;
                  }
               break;
            }
         if ( i >= Gparm.NumPBuffs )
            logit( "et", "Delete pick ignored: quake ID %ld no esta activo en los buffers.\n", PStruct.iHypoID );
         ReleaseSpecificMutex( &mutsem1 );
         continue;
      }

      if ( PPickMatch( &PStruct, StaArray, Nsta, 2 ) < 0 )
      {
         int      lineLen;
         time_t   errTime;
         char     errmsg[80];
         MSG_LOGO logo;
		 
         logit("e", "DEBUG RECHAZO: El pick de la estacion %s %s %s fue DESCARTADO porque los canales/nombres no coinciden exactamente con el archivo .sta\n", PStruct.szStation, PStruct.szChannel, PStruct.szNetID);

         time( &errTime );
         sprintf( errmsg, "%ld 1 %s %s %s not found in StaDataFile\n",
                  (long) errTime,
                  PStruct.szStation, PStruct.szNetID, PStruct.szChannel );
         lineLen = strlen( errmsg );
         logo.type   = Ewh.TypeError;
         logo.mod    = Gparm.MyModId;
         logo.instid = Ewh.MyInstId;
         tport_putmsg( &Gparm.InRegion, &logo, lineLen, errmsg );
         continue;
      }
	  
      RequestSpecificMutex( &mutsem1 );  
      
      logit("e", "\nDEBUG ASOCIADOR: Pick aceptado de %s. Pasando a LoadUpPBuff...\n", PStruct.szStation);
      
      LoadUpPBuff( &PStruct, PBuf, iPBufCnt, Hypo, &iActiveBuffer, &Gparm,
                   &Ewh, city, iLastBuffCnt, iNumPBufRem, cityEC, EqDepth,
                   szPStnArray, Nsta, Gparm.iNumNearStn, MAX_STATIONS );
                   
      for(int b=0; b < 10; b++) {
          if (iPBufCnt[b] > 0) {
              logit("e", "  -> Status Buffer [%d]: Tiene ahora %d picks (Se necesitan %d).\n", b, iPBufCnt[b], Gparm.MinPs);
          }
      }
      
      ReleaseSpecificMutex( &mutsem1 );  
   }

   if ( Gparm.OutKey != Gparm.InKey )
   {
      tport_detach( &Gparm.InRegion );
      tport_detach( &Gparm.OutRegion );
      tport_detach( &Gparm.AlarmRegion );
   }
   else
      tport_detach( &Gparm.InRegion );
   for ( i=0; i<Gparm.NumPBuffs; i++ ) free( PBuf[i] );
   free( StaArray );
   logit( "t", "Termination requested. Exiting.\n" );
   return 0;
}

int CreateNearbyStationLookupTable( STATION *Sta,
     char pszPStnArray[][MAX_NUM_NEAR_STN][TRACE_STA_LEN], int iNSta,
     int iNumNearStn  )
{
   AZIDELT azidelt;
   int     i, j, jj;
   LATLON  ll;
   STATION StaTemp[MAX_STATIONS];
   
   for ( i=0; i<iNSta; i++ )
   {
      strcpy( StaTemp[i].szStation, Sta[i].szStation );
      strcpy( StaTemp[i].szChannel, Sta[i].szChannel );
      strcpy( StaTemp[i].szNetID, Sta[i].szNetID );
      ll.dLat = Sta[i].dLat;       
      ll.dLon = Sta[i].dLon;
      GeoCent( &ll );              
      GetLatLonTrig( &ll );
      StaTemp[i].dLat = ll.dLat;
      StaTemp[i].dLon = ll.dLon;
      StaTemp[i].dCoslat = ll.dCoslat;
      StaTemp[i].dSinlat = ll.dSinlat;
      StaTemp[i].dCoslon = ll.dCoslon;
      StaTemp[i].dSinlon = ll.dSinlon;
      StaTemp[i].dDelta = Sta[i].dDelta;
   }
   for ( i=0; i<iNSta; i++ )
   {
      for ( j=0; j<iNSta; j++ )
      {                            
         if ( !strcmp( StaTemp[j].szChannel, "SHZ" ) || 
              !strcmp( StaTemp[j].szChannel, "HHZ" ) || 
              !strcmp( StaTemp[j].szChannel, "EHZ" ) || 
              !strcmp( StaTemp[j].szChannel, "SZ" ) || 
              !strncmp( StaTemp[j].szChannel, "BHZ", 3 ) )
            azidelt = GetDistanceAz( (LATLON *) &StaTemp[i], (LATLON *) &StaTemp[j] );
         else
            azidelt.dDelta = 179.;	   
         Sta[j].dDelta = azidelt.dDelta;
         strcpy( Sta[j].szStation, StaTemp[j].szStation );
         strcpy( Sta[j].szChannel, StaTemp[j].szChannel );
         strcpy( Sta[j].szNetID, StaTemp[j].szNetID );
         Sta[j].dLat = StaTemp[j].dLat;
         Sta[j].dLon = StaTemp[j].dLon;
         Sta[j].dCoslat = StaTemp[j].dCoslat;
         Sta[j].dSinlat = StaTemp[j].dSinlat;
         Sta[j].dCoslon = StaTemp[j].dCoslon;
         Sta[j].dSinlon = StaTemp[j].dSinlon;
      }	    	  
      qsort( (void *) Sta, iNSta, sizeof( STATION ), SortAllByDistance );
      
      j = 0;
      jj = 0;
      /* PARCHE SALVAVIDAS: El bucle ahora se detiene si se acaban las estaciones (j < iNsta) */
      while ( jj < iNumNearStn && j < iNSta ) 
      {
         if ( (jj == 0) ||
              (jj > 0 && strcmp( Sta[j].szStation, pszPStnArray[i][jj-1] )) )
         {
            strcpy( pszPStnArray[i][jj], Sta[j].szStation );
            jj++;
         }
         j++;
      }
      /* Llenar el resto con nulos para evitar lecturas de basura que causan el SegFault */
      while ( jj < iNumNearStn ) {
         pszPStnArray[i][jj][0] = '\0';
         jj++;
      }
   }
   for ( i=0; i<iNSta; i++ )
   {
      logit( "", "%s - ", StaTemp[i].szStation );
      for ( j=0; j<iNumNearStn; j++ )
         if (pszPStnArray[i][j][0] != '\0') 
            logit( "", "%s ", pszPStnArray[i][j] );
      logit( "", "\n" );
   }
   return ( 1 );
}

int GetEwh( EWH *Ewh )
{
   if ( GetLocalInst( &Ewh->MyInstId ) != 0 ) return -1;
   if ( GetInst( "INST_WILDCARD", &Ewh->GetThisInstId ) != 0 ) return -2;                                       
   if ( GetModId( "MOD_WILDCARD", &Ewh->GetThisModId ) != 0 ) return -3;
   if ( GetType( "TYPE_HEARTBEAT", &Ewh->TypeHeartBeat ) != 0 ) return -4;
   if ( GetType( "TYPE_ERROR", &Ewh->TypeError ) != 0 ) return -5;
   if ( GetType( "TYPE_ALARM", &Ewh->TypeAlarm ) != 0 ) return -6;
   if ( GetType( "TYPE_PICKTWC", &Ewh->TypePickTWC ) != 0 ) return -7;
   if ( GetType( "TYPE_H71SUM2K", &Ewh->TypeH71Sum2K ) != 0 ) return -8;
   if ( GetType( "TYPE_HYPOTWC", &Ewh->TypeHypoTWC ) != 0 ) return -9;
   return 0;
}

thr_ret LocateThread( void *dummy )
{
   double  dMin;                             
   int     i, j;
   int     iFinal;                           
   int     iLoc;                             
   int     iMin;                             
   int     iRC;                              
   static  long    lLastTime[MAX_PBUFFS];    
   long    lTime;                            
   
   for ( i=0; i<Gparm.NumPBuffs; i++ ) iLastBuffCnt[i] = 0;

   for (;;)
   {
      time( &lTime );
      for ( i=0; i<Gparm.NumPBuffs; i++ )
      {
         iFinal = 0;
         if ( Hypo[i].iGoodSoln >= 2 && Hypo[i].iFinalMade == 0 &&
             (lTime-lLastTime[i]) > (long) (Gparm.MaxTimeBetweenPicks*60.) &&
             (lTime-Hypo[i].dOriginTime < 20*60) )
              iFinal = 1;
         if ( (iPBufCnt[i] >= Gparm.MinPs && iPBufCnt[i] != iLastBuffCnt[i]) || iFinal == 1 )
         {		 	   
            RequestSpecificMutex( &mutsem1 );
            iLastBuffCnt[i] = iPBufCnt[i];
			
            iLoc = 0;			
            if ( PBuf[i][iPBufCnt[i]-1].dPTime > 0. &&
                 Hypo[i].iVersion < MAX_VERSIONS &&
                 Hypo[i].iNumPs < MAX_STATIONS ) iLoc = 1;
                 
            logit("e", "\nDEBUG 1: [Buffer %d] Tenemos %d picks (MinPs=%d). Intentando LocateQuake()...\n", i, iPBufCnt[i], Gparm.MinPs);

            iRC = LocateQuake( PBuf[i], &iPBufCnt[i], &Gparm, &Hypo[i], i, &Ewh,
                               city, iLoc, Hypo, iPBufCnt, cityEC, EqDepth, MAX_STATIONS );			
            
            logit("e", "DEBUG 2: [Buffer %d] LocateQuake retorno %d. iGoodSoln = %d\n", i, iRC, Hypo[i].iGoodSoln);
            if (iRC >= 0 && Hypo[i].iGoodSoln >= 2) {
                logit("e", "DEBUG 3: Localizacion BUENA! Lat: %.2f, Lon: %.2f. Revisa si escribio en disco.\n", Hypo[i].dLat, Hypo[i].dLon);
            } else if (iRC >= 0 && Hypo[i].iGoodSoln < 2) {
                logit("e", "DEBUG X: Algoritmo fallo en converger o el residual es muy alto (solucion descartada).\n");
            }

            if ( iRC == -1 )
               logit( "et", "Problem in LocateQuake\n" );
            else
            {   
               if ( iFinal == 0 )
                  CheckPBuffTimes( PBuf, iPBufCnt, Hypo, i, &Gparm,
                                   iLastBuffCnt, iNumPBufRem, szPStnArray,
                                   Nsta, Gparm.iNumNearStn, MAX_STATIONS );
            }

            if ( iRC >= 0 && Hypo[i].iGoodSoln >= 2 ) iActiveBuffer = iRC;			   
			
            ReleaseSpecificMutex( &mutsem1 );  
			
            if ( iFinal == 1 ) Hypo[i].iFinalMade = 1;			   
            lLastTime[i] = lTime;              
         }
      }
		
      for ( i=0; i<Gparm.NumPBuffs; i++ )
         if ( iPBufCnt[i] == 0 ) goto Sleeper;  
		
      iMin = 0;
      dMin = 1.E20;
      for ( i=0; i<Gparm.NumPBuffs; i++ )
         if ( Hypo[i].dMSAvg == 0. ||
             ((double) lTime-Hypo[i].dOriginTime) > 7200. )
            for ( j=0; j<iPBufCnt[i]; j++ )
               if ( PBuf[i][j].dPTime < dMin && PBuf[i][j].dPTime > 0.0 )
               {
                  dMin = PBuf[i][j].dPTime;
                  iMin = i;
               }
      RequestSpecificMutex( &mutsem1 );   
      iPBufCnt[iMin] = 0;
      iNumPBufRem[iMin] = 0;
      iLastBuffCnt[iMin] = 0;
      for ( i=0; i<MAX_STATIONS; i++ ) InitP( &PBuf[iMin][i] );
      InitHypo( &Hypo[iMin] );
      Hypo[iMin].iQuakeID += Gparm.NumPBuffs;
      if ( Hypo[iMin].iQuakeID >= 10000 ) Hypo[iMin].iQuakeID -= 10000;
      Hypo[iMin].iVersion = 1;
      Hypo[iMin].iAlarmIssued = 0;
      ReleaseSpecificMutex( &mutsem1 );   
			
Sleeper:
      sleep_ew( 3000 );   
   }
}

int SortAllByDistance( const void *pP1, const void *pP2 )
{
   STATION   *pP1T, *pP2T;
   pP1T = (STATION *) pP1;
   pP2T = (STATION *) pP2;
   if ( pP1T->dDelta > pP2T->dDelta ) return 1;
   else if ( pP1T->dDelta < pP2T->dDelta ) return -1;
   else return 0;
}
