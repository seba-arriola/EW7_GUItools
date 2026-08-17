/*****************************************************************
* pick_wcatwc.c (REMASTERED AND 64-BIT ARMORED)                 *
 * *                                                             *
 * Modernized to support strict TRACEBUF/2 validation and        *
 * absolute protection against memory overflows in Mwp.          *
 *****************************************************************/
	   
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <malloc.h>
#include <stdint.h> 

#include <earthworm.h>
#include <transport.h>
#include <trace_buf.h>
#include <swap.h>
#include <kom.h>
#include "pick_wcatwc.h"

/* Prototype adjusted to accept a generic header */
void Interpolate64( STATION *Sta, long *WaveLong, TRACE_HEADER *WaveHead, int GapSize );

/* EW global variables exclusive to this module */
unsigned char TypeTraceBuf1 = 0;
unsigned char TypeTraceBuf2 = 0;

int main( int argc, char **argv )
{
   static ALARMSTRUCT   *AS;       
   char          *configfile;      
   static double dLastEndTime;     
   EWH           Ewh;              
   GPARM         Gparm;            
   MSG_LOGO      hrtlogo;          
   int           i, iTemp;         
   long          InBufl;           
   static int    iNoDataAlarmIssued=0; 
   static int    iNumRegions;      
   char          line[40];         
   int           lineLen;          
   time_t        lLastData;        
   MSG_LOGO      logo;             
   long          MsgLen;           
   pid_t         myPid;            
   int           Nsta;             
   long          RawBufl;          
   static STATION  *StaArray;      
   time_t        then;             
   char          *WaveBuf;         
   
   /* Force the use of the base TRACE_HEADER to map times/samprate generically */
   TRACE_HEADER  *WaveHead;        
   short         *WaveShort;       
   
   long          WaveLongArr[MAX_TRACEBUF_SIZ]; 
   long          WaveRaw[MAX_TRACEBUF_SIZ];
   
   /* Parallel array to map Location Codes without altering the old STATION structure */
   char          AssignedLoc[MAX_STATIONS][4];
   for (i=0; i<MAX_STATIONS; i++) AssignedLoc[i][0] = '\0';
   

   if ( argc != 2 ) {
      fprintf( stderr, "Usage: pick_wcatwc <configfile>\n" );
      return -1;
   }
   configfile = argv[1];

   if ( GetConfig( configfile, &Gparm ) == -1 ) {
      fprintf( stderr, "pick_wcatwc: GetConfig() failed. Exiting.\n" );
      return -1;
   }

   if ( GetEwh( &Ewh ) < 0 ) {
      fprintf( stderr, "pick_wcatwc: GetEwh() failed. Exiting.\n" );
      return -1;
   }

   hrtlogo.instid = Ewh.MyInstId;
   hrtlogo.mod    = Gparm.MyModId;
   hrtlogo.type   = Ewh.TypeHeartBeat;

   logit_init( configfile, Gparm.MyModId, 256, 1 );

   myPid = getpid();
   if ( myPid == -1 ) {
      logit( "e", "pick_wcatwc: Can't get my pid. Exiting.\n" );
      return -1;
   }

   LogConfig( &Gparm );
   
   if ( Gparm.TwoStnAlarmOn == 1 )
      if ( ReadAlarmParams( &AS, &iNumRegions, Gparm.TwoStnAlarmFile ) == 0 ) {
         logit( "e", "pick_wcatwc: can't read alarm file %s; Exiting.\n", Gparm.TwoStnAlarmFile );
         return -1;
      }              
	  
   InBufl = MAX_TRACEBUF_SIZ*2 + sizeof(long)*(Gparm.MaxGap-1);
   WaveBuf = (char *) malloc( (size_t) InBufl );
   if ( WaveBuf == NULL ) {
      logit( "et", "pick_wcatwc: Cannot allocate waveform buffer\n" );
      free( AS );
      return -1;
   }

   WaveHead  = (TRACE_HEADER *) WaveBuf;

   if ( ReadStationList( &StaArray, &Nsta, Gparm.StaFile, Gparm.StaDataFile, Gparm.ResponseFile, MAX_STATIONS, 1 ) == -1 ) {
      logit( "", "pick_wcatwc: ReadStationList() failed. Exiting.\n" );
      free( WaveBuf ); free( AS ); return -1;
   }
   if ( Nsta == 0 ) {
      logit( "et", "pick_wcatwc: Empty station list. Exiting." );
      free( AS ); free( WaveBuf ); free( StaArray ); return -1;
   }
   logit( "t", "pick_wcatwc: Picking %d stations\n", Nsta );
	 	  
   for ( i=0; i<Nsta; i++ ) {
      StaArray[i].iAlarmSpeak = 0;
      StaArray[i].iAlarmPage = 0;
      if ( StaArray[i].iAlarmStatus == 1 ) { StaArray[i].iAlarmPage = 1; StaArray[i].iAlarmSpeak = 0; }
      if ( StaArray[i].iAlarmStatus == 2 ) { StaArray[i].iAlarmPage = 0; StaArray[i].iAlarmSpeak = 1; StaArray[i].iAlarmStatus = 1; }
      if ( StaArray[i].iAlarmStatus == 3 ) { StaArray[i].iAlarmPage = 1; StaArray[i].iAlarmSpeak = 1; StaArray[i].iAlarmStatus = 1; }
   }

   LogStaListP( StaArray, Nsta );

   if ( Gparm.OutKey != Gparm.InKey ) {
      tport_attach( &Gparm.InRegion,  Gparm.InKey );
      tport_attach( &Gparm.OutRegion, Gparm.OutKey );
      tport_attach( &Gparm.AlarmRegion, Gparm.AlarmKey );
   } else {
      tport_attach( &Gparm.InRegion, Gparm.InKey );
      Gparm.OutRegion = Gparm.InRegion;
   }

   /* SHIELD: Simultaneous subscription to TRACEBUF and TRACEBUF2 so no data is lost */
   MSG_LOGO getlogo[2];
   int nlogos = 0;
   
   if (TypeTraceBuf1 != 0) {
       getlogo[nlogos].instid = Ewh.GetThisInstId;
       getlogo[nlogos].mod    = Ewh.GetThisModId;
       getlogo[nlogos].type   = TypeTraceBuf1;
       nlogos++;
   }
   if (TypeTraceBuf2 != 0) {
       getlogo[nlogos].instid = Ewh.GetThisInstId;
       getlogo[nlogos].mod    = Ewh.GetThisModId;
       getlogo[nlogos].type   = TypeTraceBuf2;
       nlogos++;
   }
   
   if (nlogos == 0) {
       logit("e", "FATAL: Neither TYPE_TRACEBUF nor TYPE_TRACEBUF2 definition was found.\n");
       exit(-1);
   }

   while ( tport_getmsg( &Gparm.InRegion, getlogo, nlogos, &logo, &MsgLen, WaveBuf, MAX_TRACEBUF_SIZ) != GET_NONE );

   time( &then );
   sprintf( line, "%ld %d\n", (long) then, myPid );
   lineLen = strlen( line );
   if ( tport_putmsg( &Gparm.InRegion, &hrtlogo, lineLen, line ) != PUT_OK ) {
      logit( "et", "pick_wcatwc: Error sending 1st heartbeat. Exiting." );
      if ( Gparm.OutKey != Gparm.InKey ) { tport_detach( &Gparm.InRegion ); tport_detach( &Gparm.OutRegion ); tport_detach( &Gparm.AlarmRegion ); }
      else tport_detach( &Gparm.InRegion );
      free( AS ); free( WaveBuf ); free( StaArray ); return 0;
   }                                                                    
   lLastData = then;

   dLastEndTime = 0.;              
   while ( tport_getflag( &Gparm.InRegion ) != TERMINATE )
   {
      char    type[3];
      static  STATION *Sta;     
      int     rc;               
      static  time_t  now;      
      double  GapSizeD;         
      long    GapSize;          

      time( &now );
      if ( (now - then) >= Gparm.HeartbeatInt ) { 
         then = now;
         sprintf( line, "%ld %d\n", (long) now, myPid );
         lineLen = strlen( line );
         if ( tport_putmsg( &Gparm.InRegion, &hrtlogo, lineLen, line ) != PUT_OK ) {
            logit( "et", "pick_wcatwc: Error sending heartbeat. Exiting." ); break;
         }                              
      }                    

      if ( strlen( Gparm.ATPLineupFileBB ) < 3 )  
         if ( labs( (now-lLastData) ) > Gparm.AlarmTime ) {
            if ( iNoDataAlarmIssued == 0 ) {
               logit( "t", "No Data Alarm Activated\n" );
               ReportAlarm( StaArray, Gparm.MyModId, Gparm.AlarmRegion, Ewh.TypeAlarm, Ewh.MyInstId, 5, "", 0 );
            }
            iNoDataAlarmIssued++;
         }

      /* Listen on both TRACEBUF and TRACEBUF2 */
      rc = tport_getmsg( &Gparm.InRegion, getlogo, nlogos, &logo, &MsgLen, WaveBuf, MAX_TRACEBUF_SIZ);

      if ( rc == GET_NONE ) { sleep_ew( 50 ); continue; } 
      
      if ( rc == GET_NOTRACK ) logit( "et", "pick_wcatwc: Tracking error.\n");
      if ( rc == GET_MISS_LAPPED ) logit( "et", "pick_wcatwc: Got lapped on the ring.\n");
      if ( rc == GET_MISS_SEQGAP ) logit( "et", "pick_wcatwc: Gap in sequence numbers.\n");
      if ( rc == GET_MISS ) logit( "et", "pick_wcatwc: Missed messages.\n");
      if ( rc == GET_TOOBIG ) {
         logit( "et", "pick_wcatwc: Retrieved message too big (%ld) for msg.\n", MsgLen );
         continue;
      }

      /* Process the bytes depending on whether it arrived as TRACEBUF2 or legacy TRACEBUF */
      if (logo.type == TypeTraceBuf2) {
          if ( WaveMsg2MakeLocal( (TRACE2_HEADER *)WaveHead ) < 0 ) continue;
      } else {
          if ( WaveMsgMakeLocal( WaveHead ) < 0 ) continue;
      }
      
      /* PACKET SHIELD: Prevents crashes from corrupt packets */
      if ( WaveHead->nsamp <= 0 || WaveHead->nsamp > MAX_TRACEBUF_SIZ || WaveHead->samprate <= 0.0 ) {
          continue; 
      }

      if ( strlen( Gparm.ATPLineupFileBB ) > 2 )  
         if ( WaveHead->starttime-(int) dLastEndTime > 3600) {
	        free( StaArray );                                  
	        StaArray = (STATION *) calloc( MAX_STATIONS, sizeof(STATION) ); 
            Nsta = ReadLineupFile( Gparm.ATPLineupFileBB, StaArray );
            if ( Nsta < 2 ) { logit( "", "Bad Lineup File read %s\n", Gparm.ATPLineupFileBB ); continue; }	   
            for ( i=0; i<Nsta; i++ ) {
               StaArray[i].iFirst = 1; InitVar( &StaArray[i] );	  
               free( StaArray[i].plRawData ); free( StaArray[i].pdRawDispData );
               free( StaArray[i].pdRawIDispData ); free( StaArray[i].plRawTempBuff );
            }
            for (i=0; i<MAX_STATIONS; i++) AssignedLoc[i][0] = '\0';
         } 	

      /* Recover Location Code assuming -- if it is the legacy format */
      char loc_code[3] = "--";
      if (logo.type == TypeTraceBuf2) {
          strncpy(loc_code, ((TRACE2_HEADER*)WaveBuf)->loc, 2); 
          loc_code[2] = '\0';
      }
      if (strcmp(loc_code, "  ") == 0 || strlen(loc_code) == 0) strcpy(loc_code, "--");

      Sta = NULL;								  
      for ( i=0; i<Nsta; i++ ) {
         if ( !strcmp( WaveHead->sta,  StaArray[i].szStation ) &&
              !strcmp( WaveHead->chan, StaArray[i].szChannel ) &&
              !strcmp( WaveHead->net,  StaArray[i].szNetID ) ) 
         {
            /* Safe and tolerant location code mapping */
            if ( AssignedLoc[i][0] == '\0' ) {
                strcpy(AssignedLoc[i], loc_code);
            }
            if ( !strcmp( loc_code, AssignedLoc[i] ) ) {
                Sta = (STATION *) &StaArray[i];
                break;
            }
         }
      }
      if ( Sta == NULL ) continue;
		
      if ( WaveHead->endtime > (double) now + 86400.0 ) {
         continue;
      }

      if ( Sta->iFirst == 1 )
      {
         Sta->lPickIndex = 1;
         Sta->dSampRate = WaveHead->samprate;
         Sta->dEndTime = WaveHead->endtime;
         Sta->dStartTime = WaveHead->starttime;
         Sta->dDataEndTime = WaveHead->endtime;
         Sta->iFirst = 0;
         ResetFilter( Sta );
		
         /* Allocate ABSOLUTE MAXIMUM memory instead of dynamic memory. */
         RawBufl = MAXMWPARRAY * sizeof (long);
         Sta->plRawData = (long *) malloc( (size_t) RawBufl );
         if ( Sta->plRawData == NULL ) {
            logit( "et", "pick_wcatwc: Can't allocate max raw buffer for %s\n", Sta->szStation );
            free( AS ); free( WaveBuf ); free( StaArray ); return -1;
         }
		
         RawBufl = MAXMWPARRAY * sizeof (double);
         Sta->pdRawDispData = (double *) malloc( (size_t) RawBufl );
         if ( Sta->pdRawDispData == NULL ) {
            logit( "et", "pick_wcatwc: Can't allocate max disp buffer for %s\n", Sta->szStation );
            free( AS ); free( WaveBuf ); free( StaArray ); return -1;
         }
		
         Sta->pdRawIDispData = (double *) malloc( (size_t) RawBufl );
         if ( Sta->pdRawIDispData == NULL ) {
            logit( "et", "pick_wcatwc: Cannot allocate max int disp buffer for %s\n", Sta->szStation );
            free( AS ); free( WaveBuf ); free( StaArray ); return -1;
         }
		 		
         long max_temp = (long)(200.0 * (1./Gparm.MinFreq) + 0.1); 
         if (max_temp < 1000) max_temp = 1000;
         
         Sta->lRawTempSize = (long) (WaveHead->samprate*(1./Gparm.MinFreq)+0.1);
         RawBufl = sizeof (long) * max_temp;
         Sta->plRawTempBuff = (long *) malloc( (size_t) RawBufl );
         if ( Sta->plRawTempBuff == NULL ) {
            logit( "et", "pick_wcatwc: Can't allocate temp buffer for %s\n", Sta->szStation );
            free( AS ); free( WaveBuf ); free( StaArray ); return -1;
         }
         
         dLastEndTime = WaveHead->endtime;
         continue; 
      }
      else {
         Sta->dSampRate = WaveHead->samprate;
      }
	  
      if ( Sta->dEndTime > WaveHead->starttime+0.001 ) continue;
      
      lLastData = now; /* Confirmed! Data flow refreshes the timeout */        
      iNoDataAlarmIssued = 0;

      /* 64-bit waveform parsing */
      strcpy( type, WaveHead->datatype );
      
      /* The header is 64 bytes for both TRACEBUF and TRACEBUF2 */
      WaveShort = (short *) (WaveBuf + sizeof(TRACE_HEADER));
      int32_t *Wave32 = (int32_t *) (WaveBuf + sizeof(TRACE_HEADER));

      if ( (strcmp( type,"i2" ) == 0) || (strcmp( type,"s2" ) == 0) ) {
         for ( i = 0; i < WaveHead->nsamp; i++ )
            WaveLongArr[i] = (long) WaveShort[i];
      } else {
         for ( i = 0; i < WaveHead->nsamp; i++ )
            WaveLongArr[i] = (long) Wave32[i];
      }
      
      GapSizeD = WaveHead->samprate * (WaveHead->starttime - Sta->dEndTime);
      if ( GapSizeD < 0. ) GapSize = 0;
      else GapSize  = (long) (GapSizeD + 0.5);

      /* SHIELD: Prevent Interpolate64 from writing outside WaveLongArr */
      if ( (GapSize > 1) && (GapSize <= Gparm.MaxGap) ) {
         if (WaveHead->nsamp + GapSize <= MAX_TRACEBUF_SIZ) {
             Interpolate64( Sta, WaveLongArr, WaveHead, GapSize );
         } else {
             GapSize = Gparm.MaxGap + 1; /* Force a restart if the gap exceeds the array memory */
         }
      }

      /* Save the RAW waveform AFTER interpolation */
      for ( i = WaveHead->nsamp - 1; i >= 0; i-- ) {
         WaveRaw[i] = WaveLongArr[i];      
      }

      if ( GapSize > Gparm.MaxGap ) {
         if ( Gparm.Debug )
            logit( "t", "pick_wcatwc: Restarting %-5s%-2s %-3s. GapSize = %ld\n",
                     Sta->szStation, Sta->szNetID, Sta->szChannel, GapSize ); 
         Sta->iPickStatus = 1; ResetFilter( Sta ); InitVar( Sta );
         if ( Sta->iAlarmStatus >= 1 ) Sta->iAlarmStatus = 1;
      }
	  
      if ( Sta->iFiltStatus == 1 )
         FilterPacket ( WaveLongArr, Sta, WaveHead, Gparm.LowCutFilter, Gparm.HighCutFilter, 3.*(1./Gparm.LowCutFilter) );

      Sta->lEndData = WaveLongArr[WaveHead->nsamp - 1];
      Sta->dEndTime = WaveHead->endtime;
      Sta->dDataEndTime = WaveHead->endtime;
      Sta->dDataStartTime = WaveHead->starttime;
      dLastEndTime = WaveHead->endtime;
	  
      PickV( Sta, WaveHead->starttime, Gparm.AlarmOn, Gparm.TwoStnAlarmOn,
             Gparm.AlarmTimeout, Gparm.MinFreq, Gparm.LTASeconds,
             Gparm.MwpSeconds, Gparm.MwpSigNoise, Gparm.LGSeconds,
             Gparm.MbCycles, Gparm.dSNLocal, Gparm.dMinFLoc, Gparm.MyModId,
             Gparm.AlarmRegion, Gparm.OutRegion, Ewh.TypeAlarm, Ewh.TypePickTWC,
             Ewh.MyInstId, WaveRaw, WaveLongArr, AS, iNumRegions, 1, &iTemp );
   }

   if ( Gparm.OutKey != Gparm.InKey ) {
      tport_detach( &Gparm.InRegion ); tport_detach( &Gparm.OutRegion ); tport_detach( &Gparm.AlarmRegion );
   } else tport_detach( &Gparm.InRegion );
   
   free( AS ); free( WaveBuf );                     
   for ( i=0; i<Nsta; i++ ) {
      free( StaArray[i].plRawData ); free( StaArray[i].pdRawDispData );
      free( StaArray[i].pdRawIDispData ); free( StaArray[i].plRawTempBuff );
   }
   free( StaArray );
   logit( "t", "Termination requested. Exiting.\n" );
   return 0;
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
   if ( GetType( "TYPE_PICK_GLOBAL", &Ewh->TypePickGlobal ) != 0 ) return -9;
   
   /* Register the available types for dual ring read */
   GetType( "TYPE_TRACEBUF", &TypeTraceBuf1 );
   GetType( "TYPE_TRACEBUF2", &TypeTraceBuf2 );
   
   return 0;
}

void Interpolate64( STATION *Sta, long *WaveLong, TRACE_HEADER *WaveHead, int GapSize )
{
   int      i;
   int      j = 0;
   int      nInterp = GapSize - 1;
   double   SampleInterval = 1. / WaveHead->samprate;
   double   delta = (double)(WaveLong[0] - Sta->lEndData) / GapSize;

   for ( i = WaveHead->nsamp - 1; i >= 0; i-- )
      WaveLong[i + nInterp] = WaveLong[i];

   for ( i = 0; i < nInterp; i++ )
      WaveLong[i] = (long) (Sta->lEndData + (++j * delta) + 0.5);

   WaveHead->nsamp += nInterp;
   WaveHead->starttime = Sta->dEndTime + SampleInterval;
}

void LogStaListP( STATION *Sta, int Nsta )
{
   int i;
   logit( "", "\nStation List:\n" );
   for ( i = 0; i < Nsta; i++ )
   {
      logit( "", "%4s",     Sta[i].szStation );
      logit( "", " %3s",    Sta[i].szChannel );
      logit( "", " %2s",    Sta[i].szNetID );
      logit( "", " %1d",    Sta[i].iPickStatus );
      logit( "", " %1d",    Sta[i].iFiltStatus );
      logit( "", " %2d",    Sta[i].iSignalToNoise );
      logit( "", " %1d",    Sta[i].iAlarmStatus );
      logit( "", " %10.8lf",Sta[i].dAlarmAmp );
      logit( "", " %6.1lf", Sta[i].dAlarmDur );
      logit( "", " %6.3lf", Sta[i].dAlarmMinFreq );
      logit( "", " %1d",    Sta[i].iComputeMwp );
      logit( "", "\n" );
   }
   logit( "", "\n" );
}

int ReadAlarmParams( ALARMSTRUCT **pAS, int *piNumRegs, char *pszFile )
{
   int     i, ii, j, k, nfiles;
   long    InBufl;                   
   static  ALARMSTRUCT *ptAS;         
   
   nfiles = k_open( pszFile );
   if ( nfiles == 0 ) return( 0 );
   
   while ( nfiles > 0 ) {
      while ( k_rd() ) {
         char *com; char *str;
         com = k_str(); 
         if ( !com || com[0] == '#' ) continue; 

         if ( k_its( "iNumRegions" ) ) {
            *piNumRegs = k_int();
            InBufl = sizeof( ALARMSTRUCT ) * *piNumRegs; 
            ptAS = (ALARMSTRUCT *) calloc( *piNumRegs, sizeof( ALARMSTRUCT ) );
            if ( ptAS == NULL ) {
               logit( "et", "pick_wcatwc: Cannot allocate alarm buffer\n"); nfiles = k_close(); return( 0 );
            }
            for ( i=0; i<*piNumRegs; i++ ) {
               for ( ii=0; ii<MAX_ALARM_STN; ii++ ) strcpy( ptAS[i].szStnAlarm[ii], "\0" );
               ptAS[i].dLastTime = 0.; ptAS[i].iNumPicksCnt = 0;
               for ( j=0; j<5; j++ ) {
                  k_rd(); com = k_str(); 
                  if ( k_its( "Region" ) ) { if ( (str = k_str()) ) strcpy( ptAS[i].szRegionName, str ); }
                  else if ( k_its( "AlarmThresh" ) ) {  ptAS[i].iAlarmThresh = k_int(); }
                  else if ( k_its( "PStrength" ) ) {  ptAS[i].dThresh = k_val(); }
                  else if ( k_its( "MaxTime" ) ) {  ptAS[i].dMaxTime = k_val(); }
                  else if ( k_its( "NumStnInReg" ) ) {
                     ptAS[i].iNumStnInReg = k_int();
                     if ( ptAS[i].iNumStnInReg >= MAX_ALARM_STN ) {
                        logit( "et", "Too many alarm stns - %ld\n", ptAS[i].iNumStnInReg );
                        nfiles = k_close(); free( ptAS ); return( 0 );
                     }
                     for ( k=0; k<ptAS[i].iNumStnInReg; k++ ) {
                        k_rd(); com = k_str(); 
                        if ( k_its( "Station" ) ) {  if ( (str = k_str()) ) strcpy( ptAS[i].szStation[k], str ); }
                     }
                  }
               }				  
            }
         }
      }              
      nfiles = k_close();
   }
   
   logit( "", "NumAlarmRegions = %d\n", *piNumRegs );
   for ( i=0; i<*piNumRegs; i++ ) {
      logit( "", "%s\n", ptAS[i].szRegionName );
      logit( "", "Alarm Threshold %d\n", ptAS[i].iAlarmThresh );
      logit( "", "PStrength Threshold %lf\n", ptAS[i].dThresh );
      logit( "", "MaxTime %lf\n", ptAS[i].dMaxTime );
      logit( "", "NumStnInReg %d\n", ptAS[i].iNumStnInReg );
      for ( k=0; k<ptAS[i].iNumStnInReg; k++ ) logit( "", "%s\n", ptAS[i].szStation[k] );
   }
   *pAS = ptAS;
   return( 1 );
}
