
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: getavail.c,v 1.1 2000/02/14 18:53:30 lucky Exp $
 *
 *    Revision history:
 *     $Log: getavail.c,v $
 *     Revision 1.1  2000/02/14 18:53:30  lucky
 *     Initial revision
 *
 *
 */

       /******************************************************
        *                    GetDiskAvail                    *
        *                 Windows NT version.                *
        *                                                    *
        *  DiskAvail = Available disk space on the default   *
        *              disk, in kilobytes.                   *
        *                                                    *
        *  This function returns -1 if error,                *
        *                         0 if no error.             *
        ******************************************************/

#include <windows.h>
#include <stdio.h>


int GetDiskAvail( unsigned *DiskAvail )
{
   BOOL          success;
   const char    device[] = "\\";
   unsigned long sectorsPerCluster;
   unsigned long bytesPerSector;
   unsigned long freeClusters;
   unsigned long clusters;

   success = GetDiskFreeSpace( device,
                               &sectorsPerCluster,
                               &bytesPerSector,
                               &freeClusters,
                               &clusters );
   if ( !success )
   {
      printf( "GetDiskAvail() error: %d\n", GetLastError() );
      return -1;
   }

/* printf( "sectorsPerCluster: %u\n", sectorsPerCluster );
   printf( "bytesPerSector: %u\n", bytesPerSector );
   printf( "freeClusters: %u\n", freeClusters );
   printf( "clusters: %u\n", clusters ); */


   *DiskAvail = (unsigned)((double)freeClusters *
                           (double)bytesPerSector *
                           (double)sectorsPerCluster / 1024.);
   return 0;
}
