#include <windows.h>
#include <sys/types.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <process.h>
#include <transport.h>

#include "earthworm.h"
void tport_create( SHM_INFO *region,   /* info structure for memory region  */
                   long      nbytes,   /* size of shared memory region      */
                   long      memkey )  /* key to shared memory region       */
{
}


/******************** function tport_destroy *************************/
/*                Destroy a shared memory region.                    */
/*********************************************************************/

void tport_destroy( SHM_INFO *region )
{
}

/******************** function tport_attach *************************/
/*            Map to an existing shared memory region.              */
/********************************************************************/

void tport_attach( SHM_INFO *region,   /* info structure for memory region  */
                   long      memkey )  /* key to shared memory region       */
{
}

/******************** function tport_detach **************************/
/*                Detach from a shared memory region.                */
/*********************************************************************/

void tport_detach( SHM_INFO *region )
{
}



/*********************** function tport_putmsg ***********************/
/*            Put a message into a shared memory region.             */
/*            Assigns a transport-layer sequence number.             */
/*********************************************************************/

int tport_putmsg( SHM_INFO *region,    /* info structure for memory region    */
                  MSG_LOGO *putlogo,   /* type, module, instid of incoming msg */
                  long      length,    /* size of incoming message            */
                  char     *msg )      /* pointer to incoming message         */
{
   int retval = PUT_OK;                /* return value for this function      */

   return( retval ); 
}


/*********************** function tport_getmsg ***********************/
/*                 Get a message out of shared memory.               */
/*********************************************************************/

int tport_getmsg( SHM_INFO  *region,   /* info structure for memory region  */
                  MSG_LOGO  *getlogo,  /* requested logo(s)                 */
                  short      nget,     /* number of logos in getlogo        */
                  MSG_LOGO  *logo,     /* logo of retrieved msg             */
                  long      *length,   /* size of retrieved message         */
                  char      *msg,      /* retrieved message                 */
                  long       maxsize ) /* max length for retrieved message  */
{
   int               status = GET_OK;  /* how did retrieval go?             */
   return( GET_NONE);

}


/********************* function tport_putflag ************************/
/*           Puts the kill flag into a shared memory region.         */
/*********************************************************************/

void tport_putflag( SHM_INFO *region,  /* shared memory info structure     */
                    int       flag )   /* tells attached processes to exit */
{
   return;
}



/*********************** function tport_getflag **********************/
/*         Returns the kill flag from a shared memory region.        */
/*********************************************************************/

int tport_getflag( SHM_INFO *region )

{
   return( (int)0 );
}


/************************** tport_bufthr ****************************/
/*     Thread to buffer input from one transport ring to another.   */
/********************************************************************/
void tport_bufthr( void *dummy )
{
}


/************************** tport_buffer ****************************/
/*       Function to initialize the input buffering thread          */
/********************************************************************/
int tport_buffer( SHM_INFO  *region1,      /* transport ring             */
                  SHM_INFO  *region2,      /* private ring               */
                  MSG_LOGO  *getlogo,      /* array of logos to copy     */
                  short      nget,         /* number of logos in getlogo */
                  unsigned   maxMsgSize,   /* size of message buffer     */
                  unsigned char module,    /* module id of main thread   */
                  unsigned char instid )   /* instid id of main thread   */
{
   return 0;
}


/********************** function tport_copyfrom *********************/
/*      get a message out of public shared memory; save the         */
/*     sequence number from the transport layer, with the intent    */
/*       of copying it to a private (buffering) memory ring         */
/********************************************************************/

int tport_copyfrom( SHM_INFO  *region,   /* info structure for memory region */
                    MSG_LOGO  *getlogo,  /* requested logo(s)                */
                    short      nget,     /* number of logos in getlogo       */
                    MSG_LOGO  *logo,     /* logo of retrieved message        */
                    long      *length,   /* size of retrieved message        */
                    char      *msg,      /* retrieved message                */
                    long       maxsize,  /* max length for retrieved message */
                    unsigned char *seq ) /* TPORT_HEAD seq# of retrieved msg */
{
   int               status = GET_OK;  /* how did retrieval go?             */
   return( status );

}


/*********************** function tport_copyto ***********************/
/*           Puts a message into a shared memory region.             */
/*    Preserves the sequence number (passed as argument) as the      */
/*                transport layer sequence number                    */
/*********************************************************************/

int tport_copyto( SHM_INFO    *region,  /* info structure for memory region     */
                  MSG_LOGO    *putlogo, /* type, module, instid of incoming msg */
                  long         length,  /* size of incoming message             */
                  char        *msg,     /* pointer to incoming message          */
                  unsigned char seq )   /* preserve as sequence# in TPORT_HEAD  */
{
   int retval = PUT_OK;                /* return value for this function      */
   return( retval ); 
}


/************************* tport_buferror ***************************/
/*  Build an error message and put it in the public memory region   */
/********************************************************************/
void tport_buferror( short  ierr,       /* 2-byte error word       */
                     char  *note  )     /* string describing error */
{
        return;
}


/************************ function tport_syserr **********************/
/*                 Print a system error and terminate.               */
/*********************************************************************/

void tport_syserr( char *msg,   /* message to print (which routine had an error) */
                   long  key )  /* identifies which memory region had the error  */
{
   exit( 1 );
}

