/*****************************************************************
 * disk_wcatwc.c (PRODUCER-CONSUMER MULTITHREAD ARCHITECTURE)    *
 * *
 * Updated to support TRACEBUF2 Location Codes.                  *
 * RAM QUEUE IMPLEMENTATION: Prevents data loss due to I/O       *
 * blocking when hypo_display reads the disk.                    *
 * DYNAMIC SPS LEARNING: Automatically adjusts Sample Rates      *
 * avoiding file corruption and gaps.                            *
 *****************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <math.h>
#include <time.h>
#include <stdint.h>

#include <earthworm.h>
#include <transport.h>
#include <trace_buf.h>
#include <mem_circ_queue.h> 
#include <swap.h>
#include "disk_wcatwc.h"

/* EW Global Variables exclusive to this module */
unsigned char TypeTraceBuf1 = 0;
unsigned char TypeTraceBuf2 = 0;

GPARM   Gparm;           

/* Global Variables moved for Writer Thread access */
STATION       *StaArray = NULL;
CHNLHEADER    *chn = NULL;
int           Nsta = 0;

/* Queue and Synchronization Structures */
QUEUE MsgQueue;
mutex_t MutexQueue;
static unsigned tidWriter;
static unsigned tidCDel;

/* Parallel array for Location Codes (Avoids modifying base struct) */
char AssignedLoc[MAX_STATIONS][4];

/* Writer Thread Prototype */
thr_ret DiskWriterThread(void *dummy);

int main( int argc, char **argv )
{
   char          *WaveBuf;        
   int           lineLen;         
   char          line[40];        
   long          MsgLen;          
   MSG_LOGO      logo;            
   MSG_LOGO      hrtlogo;         
   time_t        then;            
   long          InBufl;          
   EWH           Ewh;             
   char          *configfile;     
   pid_t         myPid;           
   int           i;
   int           iNumMsgDrop = 0;

   for (i=0; i<MAX_STATIONS; i++) AssignedLoc[i][0] = '\0';

   if ( argc != 2 ) {
      fprintf( stderr, "Usage: disk_wcatwc <configfile>\n" ); return -1;
   }
   configfile = argv[1];

   if ( GetConfig( configfile, &Gparm ) == -1 ) {
      fprintf( stderr, "disk_wcatwc: GetConfig() failed. Exiting.\n" ); return -1;
   }

   if ( GetEwh( &Ewh ) < 0 ) {
      fprintf( stderr, "disk_wcatwc: GetEwh() failed. Exiting.\n" ); return -1;
   }

   hrtlogo.instid = Ewh.MyInstId;
   hrtlogo.mod    = Gparm.MyModId;
   hrtlogo.type   = Ewh.TypeHeartBeat;

   logit_init( configfile, Gparm.MyModId, 256, 1 );

   myPid = getpid();
   if ( myPid == -1 ) {
      logit( "e", "disk_wcatwc: Can't get my pid. Exiting.\n" ); return -1;
   }

   LogConfig( &Gparm );

   InBufl = MAX_TRACEBUF_SIZ;
   WaveBuf = (char *) malloc( (size_t) InBufl );
   if ( WaveBuf == NULL ) {
      logit( "et", "disk_wcatwc: Cannot allocate waveform buffer\n" ); return -1;
   }

   if ( ReadStationList( &StaArray, &Nsta, Gparm.StaFile, Gparm.StaDataFile, Gparm.ResponseFile, MAX_STATIONS, 0 ) == -1 ) {
      logit( "", "disk_wcatwc: ReadStationList() failed. Exiting.\n" ); free( WaveBuf ); return -1;
   }
   if ( Nsta == 0 ) {
      logit( "et", "disk_wcatwc: Empty station list. Exiting." ); free( WaveBuf ); free( StaArray ); return -1;
   }
   logit( "t", "disk_wcatwc: Displaying %d stations.\n", Nsta );

   LogStaList( StaArray, Nsta );

   chn = (CHNLHEADER *) calloc( Nsta, sizeof(CHNLHEADER) );
   if ( chn == NULL ) {
      logit( "et", "pick_wcatwc: Cannot allocate the chnlheader array\n" );
      free( StaArray ); free( WaveBuf ); return -1;
   }

   /* INITIALIZE RAM CIRCULAR QUEUE (Anti-collapse Buffer)
      Size: 5000 messages. Enough to support disk freezes of several seconds */
   CreateSpecificMutex(&MutexQueue);
   initqueue(&MsgQueue, (unsigned long)5000, (unsigned long)(MAX_TRACEBUF_SIZ + sizeof(MSG_LOGO)));
   logit("t", "disk_wcatwc: RAM Queue (5000 packets) initialized successfully.\n");

   /* Start Writer Thread (Consumer) */
   if ( StartThread( DiskWriterThread, 81920, &tidWriter ) == -1 ) {
      logit( "et", "Error starting DiskWriter thread; exiting!\n" );
      free( StaArray ); free( chn ); free( WaveBuf ); return -1;
   }

   if ( Gparm.CircDeleteHours > 0 )
      if ( StartThread( CircDeleteThread, 8192, &tidCDel ) == -1 ) {
         logit( "et", "Error starting CircDelete thread; exiting!\n" );
      }

   tport_attach( &Gparm.InRegion, Gparm.InKey );

   /* Simultaneous subscription to TRACEBUF and TRACEBUF2 */
   MSG_LOGO getlogo[2];
   int nlogos = 0;
   
   if (TypeTraceBuf1 != 0) {
       getlogo[nlogos].instid = Ewh.GetThisInstId; getlogo[nlogos].mod = Ewh.GetThisModId; getlogo[nlogos].type = TypeTraceBuf1; nlogos++;
   }
   if (TypeTraceBuf2 != 0) {
       getlogo[nlogos].instid = Ewh.GetThisInstId; getlogo[nlogos].mod = Ewh.GetThisModId; getlogo[nlogos].type = TypeTraceBuf2; nlogos++;
   }
   if (nlogos == 0) {
       logit("e", "FATAL: No TYPE_TRACEBUF or TYPE_TRACEBUF2 defined.\n"); exit(-1);
   }
	  
   while ( tport_getmsg( &Gparm.InRegion, getlogo, nlogos, &logo, &MsgLen, WaveBuf, MAX_TRACEBUF_SIZ) != GET_NONE );

   time( &then );
   sprintf( line, "%ld %d\n", (long) then, myPid );
   lineLen = strlen( line );
   if ( tport_putmsg( &Gparm.InRegion, &hrtlogo, lineLen, line ) != PUT_OK ) {
      logit( "et", "disk_wcatwc: Error sending 1st heartbeat. Exiting." );
      tport_detach( &Gparm.InRegion ); free( WaveBuf ); free( StaArray ); free( chn ); return 0;
   }
	  
   /* ====================================================================
      MAIN LOOP (PRODUCER): Reads from ring and pushes to RAM
      ==================================================================== */
   while ( tport_getflag( &Gparm.InRegion ) != TERMINATE )
   {
      int     rc;               
      time_t  now;              

      time( &now );
      if ( (now - then) >= Gparm.HeartbeatInt ) {
         then = now;
         sprintf( line, "%ld %d\n", (long) now, myPid );
         lineLen = strlen( line );
         if ( tport_putmsg( &Gparm.InRegion, &hrtlogo, lineLen, line ) != PUT_OK ) {
            logit( "et", "disk_wcatwc: Error sending heartbeat. Exiting." ); break;
         }
      }

      rc = tport_getmsg( &Gparm.InRegion, getlogo, nlogos, &logo, &MsgLen, WaveBuf, MAX_TRACEBUF_SIZ);

      if ( rc == GET_NONE ) { sleep_ew( 50 ); continue; }
      if ( rc == GET_NOTRACK ) logit( "et", "disk_wcatwc: Tracking error.\n");
      if ( rc == GET_MISS_LAPPED ) logit( "et", "disk_wcatwc: Got lapped on the ring (Ring too fast/small).\n");
      if ( rc == GET_MISS_SEQGAP ) logit( "et", "disk_wcatwc: Gap in sequence numbers.\n");
      if ( rc == GET_MISS ) logit( "et", "disk_wcatwc: Missed messages.\n");
      if ( rc == GET_TOOBIG ) { logit( "et", "disk_wcatwc: Retrieved message too big (%d).\n", MsgLen ); continue; }

      /* Save packet immediately to RAM queue. Do not process here to avoid blocking. */
      RequestSpecificMutex(&MutexQueue);
      int retq = enqueue(&MsgQueue, WaveBuf, MsgLen, logo);
      ReleaseSpecificMutex(&MutexQueue);

      if (retq != 0) {
          iNumMsgDrop++;
          if (iNumMsgDrop % 100 == 1) {
              logit("et", "disk_wcatwc: [WARNING] RAM Queue is FULL. Disk is too slow. %d packets dropped.\n", iNumMsgDrop);
          }
      }
   }

   tport_detach( &Gparm.InRegion ); 
   /* We don't free global memory here immediately to give the writer thread time to exit cleanly */
   logit( "t", "Termination requested. Exiting.\n" );
   return 0;
}

/* ====================================================================
   WRITER THREAD (CONSUMER): Reads from RAM, formats and writes to Disk
   ==================================================================== */
thr_ret DiskWriterThread(void *dummy)
{
    char          LocalBuf[MAX_TRACEBUF_SIZ];
    long          MsgLen;
    MSG_LOGO      logo;
    TRACE_HEADER  *WaveHead;
    int32_t       *WaveLong;
    short         *WaveShort;
    char          type[3];
    int           i, rc;
    STATION       *Sta;
    time_t        now;

    WaveHead  = (TRACE_HEADER *) LocalBuf;
    WaveLong  = (int32_t *) (LocalBuf + sizeof(TRACE_HEADER));
    WaveShort = (short *) (LocalBuf + sizeof(TRACE_HEADER));

    logit("t", "DiskWriterThread: Writer thread initialized and ready.\n");

    while(1)
    {
        /* Try to dequeue a message from RAM */
        RequestSpecificMutex(&MutexQueue);
        rc = dequeue(&MsgQueue, LocalBuf, &MsgLen, &logo);
        ReleaseSpecificMutex(&MutexQueue);

        if (rc < 0) {
            /* If queue is empty, thread sleeps shortly to avoid CPU burning */
            sleep_ew(50);
            continue;
        }

        /* --- PACKET PROCESSING --- */
        if (logo.type == TypeTraceBuf2) {
            if ( WaveMsg2MakeLocal( (TRACE2_HEADER *)WaveHead ) < 0 ) continue;
        } else {
            if ( WaveMsgMakeLocal( WaveHead ) < 0 ) continue;
        }
        
        if ( WaveHead->samprate == 0. ) continue;

        /* Retrieve Location Code and map it dynamically to avoid overwriting */
        char loc_code[3] = "--";
        if (logo.type == TypeTraceBuf2) {
            strncpy(loc_code, ((TRACE2_HEADER*)LocalBuf)->loc, 2); 
            loc_code[2] = '\0';
        }
        if (strcmp(loc_code, "  ") == 0 || strlen(loc_code) == 0) strcpy(loc_code, "--");

        Sta = NULL;                                  
        for ( i=0; i<Nsta; i++ ) {
            if ( !strcmp( WaveHead->sta,  StaArray[i].szStation ) &&
                 !strcmp( WaveHead->chan, StaArray[i].szChannel ) &&
                 !strcmp( WaveHead->net,  StaArray[i].szNetID ) )
            {
                if ( AssignedLoc[i][0] == '\0' ) { strcpy(AssignedLoc[i], loc_code); }
                if ( !strcmp( loc_code, AssignedLoc[i] ) ) {
                    Sta = (STATION *) &StaArray[i];
                    break;
                }
            }
        }

        if ( Sta == NULL ) continue;
        
        /* === SMART SPS LEARNING === 
           If incoming packet has a different SPS than the one configured in .sta file,
           we update it dynamically in memory. The next generated .S26 file will
           automatically allocate the correct size for this station, permanently 
           avoiding block overflow gaps. */
        if (WaveHead->samprate > 0.0 && fabs(WaveHead->samprate - Sta->dSampRate) > 0.01) {
            logit("t", "disk_wcatwc: [AUTO-TUNE] Sample Rate for %s.%s.%s updated from %.2f to %.2f Hz\n",
                  Sta->szStation, Sta->szChannel, loc_code, Sta->dSampRate, WaveHead->samprate);
            Sta->dSampRate = WaveHead->samprate;
        }

        time(&now);
        if ( WaveHead->endtime > (double) now+FUTURE_TOL || WaveHead->endtime < (double) (now-TOO_OLD) ) 
        {
            continue;
        }

        strcpy( type, WaveHead->datatype );
        if ( (strcmp(type,"i2")==0) || (strcmp(type,"s2")==0) )
            for ( i = WaveHead->nsamp - 1; i > -1; i-- )
                WaveLong[i] = (int32_t) WaveShort[i];

        /* DISK WRITING (May take milliseconds, but will not block the ring) */
        if ( WriteDiskData( Sta, (long*)WaveLong, &Gparm, WaveHead, Nsta, StaArray, chn ) < 0 ) {
            logit( "t", "WriteDiskData Failure: %s %s %s\n", Sta->szStation, Sta->szChannel, loc_code );
        }
    }
    return NULL;
}
	   
thr_ret CircDeleteThread( void *dummy )
{
   while ( 1 ) { sleep_ew( 60000 ); }
   return NULL;
}

int GetEwh( EWH *Ewh )
{
   if ( GetLocalInst( &Ewh->MyInstId ) != 0 ) return -1;
   if ( GetInst( "INST_WILDCARD", &Ewh->GetThisInstId ) != 0 ) return -2;
   if ( GetModId( "MOD_WILDCARD", &Ewh->GetThisModId ) != 0 ) return -3;
   if ( GetType( "TYPE_HEARTBEAT", &Ewh->TypeHeartBeat ) != 0 ) return -4;
   if ( GetType( "TYPE_ERROR", &Ewh->TypeError ) != 0 ) return -5;
   
   GetType( "TYPE_TRACEBUF", &TypeTraceBuf1 );
   GetType( "TYPE_TRACEBUF2", &TypeTraceBuf2 );
   
   return 0;
}
