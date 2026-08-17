/* Enable LFS (Large File Support) for offsets > 2GB: MUST be defined before
   any system include so that off_t is widened to 64 bits. */
#define _FILE_OFFSET_BITS 64

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <stdint.h>
#include <kom.h>
#include <transport.h>
#include <earthworm.h>
#include <trace_buf.h>
#include "disk_wcatwc.h"
#ifndef _WIN32
#include <sys/stat.h>
#include <sys/types.h>
#endif

int InitDiskFile( char *pszFile, STATION StaArray[], double dFileTime,
                  int iNumStas, char *pszRootDir, int iFileSize, 
                  double dPacketTime, CHNLHEADER chn[], int iDebug ) 
{
   static DISKHEADER  dh;      
   FILE        *hFile;         
   int         i;              
   time_t      itime;          
   
   int32_t     *plZero;        
   
   char        *psz;           
   char        szDir[80];      
   static struct tm *tmLoc;    

   dh.iNumChans = iNumStas;
   dh.iChnHdrSize = sizeof (CHNLHEADER);
   
   itime = (time_t) (floor( dFileTime ) );
   tmLoc = TWCgmtime( itime );
   ConvertTM2ST( tmLoc, &dh.stStartTime );
   
   for ( i=0; i<iNumStas; i++ )
   {
      strcpy( chn[i].szStation, StaArray[i].szStation );
      strcpy( chn[i].szChannel, StaArray[i].szChannel );
      strcpy( chn[i].szNetID, StaArray[i].szNetID );
      chn[i].dSampRate      = StaArray[i].dSampRate;
      chn[i].lNumSamps      = (long)((double)iFileSize*60.*chn[i].dSampRate + 0.01);
      chn[i].iBytePerSamp   = sizeof (int32_t);   
      chn[i].iTrigger       = 0;
      chn[i].iSignalToNoise = StaArray[i].iSignalToNoise;
      chn[i].iPickStatus    = StaArray[i].iPickStatus;
      chn[i].iStationType   = StaArray[i].iStationType;
      chn[i].dLat           = StaArray[i].dLat;
      chn[i].dLon           = StaArray[i].dLon;
      chn[i].dElevation     = StaArray[i].dElevation;
      chn[i].dGain          = StaArray[i].dSens;
      chn[i].dGainCalibration = StaArray[i].dGainCalibration;
      chn[i].dClipLevel     = StaArray[i].dClipLevel;
      chn[i].dTimeCorrection = StaArray[i].dTimeCorrection;
      chn[i].dScaleFactor   = StaArray[i].dScaleFactor;
      CopyDate (&dh.stStartTime, &chn[i].stStartTime);
   }		

   psz = strrchr( pszFile, '\\' );   
   if (psz == NULL) psz = strrchr( pszFile, '/' ); 
   
   if (psz != NULL) {
       strncpy( szDir, pszFile, (psz-pszFile) ); 
       szDir[psz-pszFile] = '\0';
       #ifdef _WIN32
       if ( !CreateDirectory( szDir, NULL ) )
       #else
       if ( mkdir( szDir, 0755 ) != 0 )
       #endif
          if ( iDebug ) logit( "t", "Dir: %s not created in InitDiskFile\n", szDir );
   }

   if ( ( hFile = fopen( pszFile, "wb" ) ) == NULL ) {
      logit( "t", "File: %s not opened in InitDiskFile\n", pszFile ); return( -1 );
   }

   fwrite( &dh, sizeof(dh), 1, hFile );
   
   for ( i=0; i<iNumStas; i++ )
      fwrite( &chn[i], sizeof(CHNLHEADER), 1, hFile );

   for ( i=0; i<iNumStas; i++ )
   {
      plZero = (int32_t *) malloc( chn[i].iBytePerSamp*chn[i].lNumSamps );
      if ( plZero == NULL )
      {
         logit( "et", "disk_wcatwc: Cannot allocate waveform buffer\n" ); return( -1 );
      }
      memset( plZero, 0, chn[i].iBytePerSamp*chn[i].lNumSamps );
      fwrite( plZero, 1, chn[i].iBytePerSamp*chn[i].lNumSamps, hFile );
      free( plZero );
   }
   fclose( hFile );
   return( 0 );
}

int WriteDiskData( STATION *Sta, long *WaveLong, GPARM *Gparm, 
                   TRACE_HEADER *Wavehead, int iNumStas, STATION StaArray[],
                   CHNLHEADER *chn ) 
{
   FILE     *hFile;                     
   DISKHEADER dh;                       
   static   double  dFileTime;          
   double   dInt;                       
   int      i;
   static   int      iFirst = 1;        
   static   int      iIndex;            
   static   int      iInt;              
   static   long     lNumToWrite;       
   
   /* FIX: Force 64 bits type for File Offset (LFS) */
   static   off_t    lPos;              
   static   SYSTEMTIME st;              
   char     *pszFile;                   
   struct tm *tm;                       
   time_t   itime;                      

   int32_t  *Wave32 = (int32_t *)WaveLong;

   dInt = 1. / Sta->dSampRate;          
   itime = (time_t) (floor( Wavehead->starttime ) );
   tm = TWCgmtime( itime );
   iInt = (int) ((((double) (tm->tm_min % Gparm->FileLength)*60.) +
                   (double)  tm->tm_sec +
                    Wavehead->starttime - floor( Wavehead->starttime )) /
                    dInt + 0.0000000001);
                    
   /* SAFETY CHECK: Prevent negative offsets causing file corruption */
   if (iInt < 0) {
       long skip = -iInt;
       if (skip >= Wavehead->nsamp) return 0;
       iInt = 0;
       Wavehead->nsamp -= skip;
       Wave32 += skip;
   }

   dFileTime = Wavehead->starttime - ((double) iInt * dInt);
   itime = (time_t) (floor( dFileTime ) );
   tm = TWCgmtime( itime );
   ConvertTM2ST( tm, &st );
   st.wMilliseconds = (int) ((dFileTime - floor( dFileTime )) * 1000.);

   pszFile = CreateFileName( dFileTime, Gparm->FileLength, Gparm->DiskWritePath, Gparm->FileSuffix );

   if ( ( hFile = fopen( pszFile, "rb+" ) ) == NULL || iFirst )
   {
      if ( Gparm->Debug > 0 )
         logit( "t", "Must init %s - %lf - %s SR=%lf - WH-start %lf\n",
                pszFile, dFileTime, Wavehead->sta, Sta->dSampRate, Wavehead->starttime );
                
      if ( InitDiskFile( pszFile, StaArray, dFileTime, iNumStas,
                         Gparm->DiskWritePath, Gparm->FileLength, 
                         Wavehead->starttime, chn, Gparm->Debug ) < 0 )
      {
         logit( "t", "File: %s not opened in WriteDiskData\n", pszFile ); return( -1 );
      }
      else
      {
         if ( ( hFile = fopen( pszFile, "rb+" ) ) == NULL ) {
            logit( "t", "File: %s not opened after init\n", pszFile ); return( -1 );
         }
         iFirst = 0;
      }
   }

   /* FIX: Offset Calculation supporting massive LFS files (>2GB) */
   lPos = (off_t)sizeof (DISKHEADER);
   lPos += (off_t)iNumStas * sizeof (CHNLHEADER);
   iIndex = (Sta - StaArray);
   for ( i=0; i<iIndex; i++ )
      lPos += (off_t)sizeof (int32_t) * (off_t) ((double) Gparm->FileLength * 60. * chn[i].dSampRate + 0.0000001);
      
   lPos += (off_t)sizeof (int32_t) * (off_t)iInt;
   
   lNumToWrite = Wavehead->nsamp;
   if ( iInt+Wavehead->nsamp > chn[iIndex].lNumSamps ) 
      lNumToWrite = chn[iIndex].lNumSamps - iInt;

   /* ANTI-OVERFLOW SHIELD: If station increased SPS dynamically, the allocated 
      block in the CURRENT file might run out of space. We gracefully drop the
      overflow to prevent corrupting the adjacent station's data. */
   if (lNumToWrite <= 0) {
       fclose(hFile);
       return 0; 
   }
 
   fread( &dh, sizeof (dh), 1, hFile );
   for ( i=0; i<iNumStas; i++ )
      fread( &chn[i], sizeof (CHNLHEADER), 1, hFile );
   rewind( hFile );

   CopyDate (&st, &chn[iIndex].stStartTime);

   fwrite( &dh, sizeof (dh), 1, hFile );
   for ( i=0; i<iNumStas; i++ )
      fwrite( &chn[i], sizeof (CHNLHEADER), 1, hFile );

   /* FIX: Use fseeko to support 64-bit off_t variables in Linux */
   #ifdef _WIN32
   if ( _fseeki64( hFile, lPos, SEEK_SET ) )
   #else
   if ( fseeko( hFile, lPos, SEEK_SET ) )
   #endif
   {
      logit( "t", "fseeko failed for File: %s\n", pszFile );
      fclose( hFile ); return( -1 );
   }

   if ( (long) fwrite( Wave32, sizeof (int32_t), lNumToWrite, hFile) < lNumToWrite )
   {
      logit( "t", "fwrite failed for File: %s\n", pszFile);
      fclose( hFile ); return( -1 );
   }
   fclose( hFile );

   if ( lNumToWrite < Wavehead->nsamp )
   {
      pszFile = CreateFileName( Wavehead->starttime +
       (double) Wavehead->nsamp / Wavehead->samprate, Gparm->FileLength, 
        Gparm->DiskWritePath, Gparm->FileSuffix);

      lPos = (off_t)sizeof (DISKHEADER);
      lPos += (off_t)iNumStas * sizeof (CHNLHEADER);
      for ( i=0; i<iIndex; i++ )
         lPos += (off_t)sizeof (int32_t) * (off_t) ((double) Gparm->FileLength * 60. * chn[i].dSampRate + 0.0000001);

      if ( ( hFile = fopen( pszFile, "rb+" ) ) == NULL )
      {
         if ( Gparm->Debug > 0 )
            logit( "t", "Must init %s - %lf - %s SR=%lf - WH-start %lf\n",
                   pszFile, dFileTime, Wavehead->sta, Sta->dSampRate, Wavehead->starttime );
                   
         if ( InitDiskFile( pszFile, StaArray,
              dFileTime+(double)Gparm->FileLength*60.,
              iNumStas, Gparm->DiskWritePath, Gparm->FileLength, 
              Wavehead->starttime, chn, Gparm->Debug ) < 0 )
         {
            logit( "t", "File: %s not opened in WriteDiskData - 2\n", pszFile ); return( -1 );
         }
         else
         {
            if ( ( hFile = fopen( pszFile, "rb+" ) ) == NULL ) {
               logit( "t", "File: %s not opened after init-2\n", pszFile ); return( -1 );
            }
         }
      }
 
      #ifdef _WIN32
      if ( _fseeki64( hFile, lPos, SEEK_SET ) )
      #else
      if ( fseeko( hFile, lPos, SEEK_SET ) )
      #endif
      {
         logit( "t", "fseeko failed for File2: %s\n", pszFile );
         fclose( hFile); return( -1 );
      }
      
      long remaining = Wavehead->nsamp - lNumToWrite;
      if (remaining > 0) {
          if ( (long) fwrite (&Wave32[lNumToWrite], sizeof (int32_t), remaining, hFile) < remaining)
          {
             logit( "t", "fwrite failed for File2: %s, index=%ld\n", pszFile, iIndex );
             fclose( hFile ); return( -1 );
          }
      }
      fclose (hFile);
   }
return( 0 );
}
