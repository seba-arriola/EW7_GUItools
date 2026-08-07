/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: startstop.c,v 1.20 2007/02/27 05:00:37 stefan Exp $
 *
 *    Revision history:
 *     $Log: startstop.c,v $
 *     Revision 1.20  2007/02/27 05:00:37  stefan
 *     sends stop and restart messages to accommodate new statmgr
 *
 *     Revision 1.19  2007/02/22 21:01:55  stefan
 *     lock changes
 *
 *     Revision 1.18  2006/04/04 18:16:51  stefan
 *     startstop with reconfigure and libraries
 *
 *     Revision 1.17  2005/09/07 14:11:12  friberg
 *     added in restart command from Murray McGowan
 *
 *     Revision 1.16  2002/07/16 19:01:33  davidk
 *     Fixed a bug in a sprintf statement that gets issued when startstop
 *     cannot restart a zombie process.  The bug involved trying to printf
 *     an integer using "%s", and caused startstop to crash on execution.
 *     Startstop now issues the message correctly and restarts the zombie
 *     process.
 *
 *     Revision 1.15  2002/04/22 17:17:34  lucky
 *     Fixed index (compile time) problem
 *
 *     Revision 1.14  2002/04/19 18:51:28  lucky
 *     Moved CloseHandle () calls out of TerminateChild. This was causing a slow
 *     handle leak resulting in system crash. Basically, when a child would shut
 *     itself down, i.e., it was not terminated by startstop, its handles would not be
 *     closed.
 *
 *     Revision 1.13  2002/03/20 16:56:03  patton
 *     made logit chagnes.
 *     JMP 03/20/2002
 *
 *     Revision 1.12  2001/07/16 17:01:02  patton
 *     Changed 'sleeptime' to 'statmgrDelay'.
 *
 *     Revision 1.11  2001/07/02 21:57:36  patton
 *     Modified Previous changes as per Lucky's suggestions.
 *
 *     Revision 1.9  2001/06/08 20:30:14  dietz
 *     Fixed original bug in logit call that caused startstop to crash when
 *     it couldn't start one of its children.  Removed SetErrorMode call and
 *     Alex's search for executable (previous attempts at fixin bug), added
 *     logging, made to shut down properly when CreateProcess fails.
 *
 *     Revision 1.8  2001/06/07 19:27:52  alex
 *     *** empty log message ***
 *
 *     Revision 1.5  2001/06/05 01:15:11  Alex
 *     Alex: added check for binary being in the earthworm bin path.
 *
 *     Revision 1.4  2001/05/08 20:48:53  dietz
 *     Changed RestartChild() so that it gives the child process a chance to
 *     shut down gracefully before it terminates it.
 *
 *     Revision 1.3  2000/08/28 22:45:13  kohler
 *     Added new "minimized console" display option.  WMK
 *
 *     Revision 1.2  2000/07/24 21:14:07  lucky
 *     Implemented global limits to module, installation, ring, and message type strings.
 *
 *     Revision 1.1  2000/02/14 19:37:19  lucky
 *     Initial revision
 *
 *
 */

       /**********************************************************
        *                       startstop.c                      *
        *                                                        *
        *     Program to start and stop the Earthworm system     *
        *                   Windows NT version                   *
        **********************************************************/
/* Changes:
 Lombard: 11/19/98: V4.0 changes: no Y2K date problems
     1) changed argument of logit_init to the config file name
     2) process ID in heartbeat message: not applicable
     3) flush input transport ring: not applicable
     4) add `restartMe' to .desc file: not applicable
     5) multi-threaded logit

  Alex 6/7/1:
  Fix to issue issue warning and continue running if an executable
  is not found. If the executable is not in the traditional place, a warning
  will be issued, and the %PATH% be searched for the executable. TYP for
  finding SetErrorMode().
 */


#include <startstop_winlib.h>

#define VERSION "v7.1 2007-02-26"
#define PROGRAM_NAME "startstop_nt"


METARING metaring;
void Interactive( void * );        /* Interactive thread              */
static char ewstat[MAX_STATUS_LEN];
volatile static int done = 0;
boolean  service = 0;
volatile int checkpoint = 0;        /* Used for the service only    */

int     nChild;             /* number of children */
CHILD   child[MAX_CHILD];

int main( int argc, char *argv[] )
{
   unsigned  stackSize;
   unsigned  tid;
   int       err;


/* set some default values that can't be set directly in the struct */
   metaring.statmgr_sleeptime = 1000;
   metaring.statmgr_location = (MAX_CHILD + 1);
   strcpy ( metaring.ConfigFile, DEF_CONFIG );
   strcpy ( metaring.Version, VERSION );


/* Set name of configuration file
   ******************************/
   if ( argc == 2 )
   {
        if ((strlen(argv[1]) == 2) /* checking for /v or -v or /h or -h */
                && ((argv[1][0] == '/')
                || (argv[1][0] == '-'))) {
            if ((argv[1][1] == 'v') || (argv[1][1] == 'V')) {
                printf("%s %s\n",PROGRAM_NAME, VERSION);
            } else if ((argv[1][1] == 'h') || (argv[1][1] == 'H')) {
                printf("%s %s\n",PROGRAM_NAME, VERSION);
                printf("usage: %s <config file name>\n", PROGRAM_NAME);
                printf("       If no config file name is used, then the default config file is used,\n");
                printf("       in this case %s.\n", DEF_CONFIG);
            }
            exit (0);
        } else {
            strcpy ( metaring.ConfigFile, argv[1] );
        }

   }
   else
   {
        fprintf ( stderr, "startstop: using default config file <%s>\n",
                  metaring.ConfigFile );
   }

   /* lock this instance, so another startstop can't startup using the same config file */
   LockStartstop(metaring.ConfigFile, argv[0]);

/* Initialize name of log-file & open it
   *************************************/
   logit_init( metaring.ConfigFile, 0, 1024, 1 );

   err = StartstopSetup ( &metaring, &checkpoint, service, child, &nChild );
   if (err == -1) {
        UnlockStartstop();
        return -1;
    }


/* Start the interactive thread
   ****************************/
   stackSize = 0;
   StartThread( Interactive, stackSize, &tid );

   err = FinalLoop (&metaring, &done, ewstat, &checkpoint, service, child, &nChild );
   if (err == -1) {
            UnlockStartstop();
            return -1;
    }
    UnlockStartstop();
    return 0;
}


/************************ Interactive ****************************
*           Thread to get commands from keyboard                *
*****************************************************************/

void Interactive( void *dummy )
{
  char line[100];
  char param[100];
  char fish[100];
  int  i, placeholder;
  MSG_LOGO  logo;
  char      message[512];

  EncodeStatus( ewstat, &metaring, child, &nChild );                   /* One free status */
  fprintf( stderr, "\n%s", ewstat );

  do
  {
     fprintf( stderr, "\n   Press return to print Earthworm status, or\n" );
     fprintf( stderr, "   type 'restart nnn<cr>' where nnn is either a module name or Process ID, or\n");
     fprintf( stderr, "   type 'quit<cr>' to stop Earthworm.\n\n" );
     line[0] = 0;
     param[0] = 0;
     gets(fish);
     sscanf(fish,"%[^ \t]%*[ \t]%[^ \t]",line,param);
     if ( strlen( line ) == 0 )
     {
        EncodeStatus( ewstat, &metaring, child, &nChild  );
        fprintf( stderr, "\n%s", ewstat );
     }

     {
      char *ptr=line;
      while (*ptr) *(ptr++)=tolower(*ptr);
     }

     if (strncmp( line, "restart", 7)==0)
     {
       for( i = 0; i < nChild; i++ )
       {
         if( strcmp(child[i].progName, param)==0 )
         {
           itoa(child[i].procInfo.dwProcessId, param, 10);
           break;
         }
       }
       
       fprintf( stderr, "sending restart message to the ring for %s\n", param);
      /* sending it to the ring rather than starting directly so statmgr can keep track */       
      /* RestartChild(param, &metaring, child, &nChild ); */
       SendRestartReq(&metaring, param);
     }
     if ((strncmp( line, "stop", 4)==0) || (strncmp( line, "stopmodule", 10)==0))
     {
       for( i = 0; i < nChild; i++ )
       {
         if( strcmp(child[i].progName, param)==0 )
         {
           itoa(child[i].procInfo.dwProcessId, param, 10);
           break;
         }
       }
       
       fprintf( stderr, "sending stop message to the ring for %s\n", param);
       /* sending it to the ring rather than starting directly so statmgr can keep track */       
       /* StopChild(param, &metaring, child, &nChild, &placeholder ); */
       SendStopReq(&metaring, param);
     }

     if ((strncmp( line, "recon", 5)==0) || (strncmp( line, "reconfigure", 11)==0)) { /* reconfigure */
        /* Send a message requesting reconfigure */
        /* message is just MyModId
           ****************************/
           sprintf(message,"%d\n", metaring.MyModId );
        /* Set logo values of message
           **************************/
           logo.type   = metaring.TypeReconfig;
           logo.mod    = metaring.MyModId;
           logo.instid = metaring.InstId;

        /* Send status message to transport ring
           **************************************/
           if ( tport_putmsg( &(metaring.Region[0]), &logo, strlen(message), message ) != PUT_OK ) {
                logit( "e" , "status: Error sending message to transport region.\n" );
                return;
           }
     }

  } while ( strcmp( line, "quit" ) != 0 );

  done = 1;                 /* The main thread checks this flag */

  sleep_ew( 1000000 );      /* Sleep a long time; let main thread exit */
}






