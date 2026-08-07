
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: truetime.h,v 1.1 2000/02/14 20:05:54 lucky Exp $
 *
 *    Revision history:
 *     $Log: truetime.h,v $
 *     Revision 1.1  2000/02/14 20:05:54  lucky
 *     Initial revision
 *
 *
 */


    /**********************************************
     *                truetime.h                  *
     **********************************************/

#ifndef TRUETIME_H
#define TRUETIME_H

typedef struct _TrueTimeStruct
{
   char DataReady;
   char UnitMillisecs;
   char TensMillisecs;
   char HundredsMillisecs;
   char UnitSeconds;
   char TensSeconds;
   char UnitMinutes;
   char TensMinutes;
   char UnitHours;  
   char TensHours;  
   char UnitDays;   
   char TensDays;   
   char HundredsDays;
   char Status;
} TrueTimeStruct;

int OpenTrueTime( int );
int CloseTrueTime( void );
int GetTrueTime( TrueTimeStruct * );

#endif

