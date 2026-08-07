/*
 * crc32.h  CRC used by Nanometrcis
 */

#ifndef _CRC32_H
#define _CRC32_H

void crc32_init( void );
void crc32_update( unsigned char *blk_adr, unsigned long blk_len );
unsigned long crc32_value( void );

#endif 
