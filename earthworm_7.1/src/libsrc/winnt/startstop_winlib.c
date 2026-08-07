
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *
 *    Revision history:
 *     $Log: startstop_winlib.c,v $
 *     Revision 1.5  2007/02/22 21:03:44  stefan
 *     lock changes
 *
 *     Revision 1.4  2007/02/20 23:01:53  stefan
 *     stop, reconfigure fixes, lock start
 *
 *     Revision 1.3  2007/02/08 22:43:36  stefan
 *     removed duplicate children chatter to screen
 *
 *     Revision 1.2  2006/06/06 21:13:59  stefan
 *     hydra console incorporation
 *
 *     Revision 1.0  2006/02/23 10:44:30  lisowski
 *     Initial revision. Removed common code from
 *     system_control\startstop_nt\startstop.c and
 *     system_control\startstop_service\startstop_service.c
 *     and put it here.
 *
 #    return ERROR_FILE_NOT_FOUND; <- these are Microsoft #defined integers
 *
 */

/********************************************************************
 *                 startstop_lib.c    for   Windows 32              *
 ********************************************************************/

#include <startstop_winlib.h>

/********************************************************************
  * GetConfig()  Processes command file using kom.c functions        *
  *                     Exits if any errors are encountered          *
  *                     Commands are expected in a given order       *
  *                     Nesting is not allowed for startstop.cnf!    *
  ********************************************************************/
int GetConfig( METARING *metaring, CHILD    child[MAX_CHILD], int *nChild )
{
   int      state;        /* which part of the config file you are reading   */
   char     missed[30];
   char    *ptr;
   char    *com;
   char    *str;
   int      nfiles;
   int      ir;
   int      i;
   int      ichild;
   boolean  firstTimeThrough = FALSE;
   boolean  duplicate = FALSE;

/* Definitions for state while reading file
   ****************************************/
   #define READ_NRING             0
   #define READ_RINGINFO          1
   #define READ_MODID             2
   #define READ_HEARTBEAT         3
   #define READ_MYPRIORITYCLASS   4
   #define READ_LOGSWITCH         5
   #define READ_KILLDELAY         6
   #define READ_SLEEPTIME         7
   #define READ_PROCESS           8
   #define READ_PRIORITYCLASS     9
   #define READ_THREADPRIORITY   10
   #define READ_DISPLAY          11


/* Initialize some things
   **********************/
   state     = READ_NRING;
   missed[0] = '\0';

   if (*nChild == 0){
       firstTimeThrough = TRUE;
       oldNChild = 0;
   }


/* Open the main configuration file
   ********************************/
   nfiles = k_open( metaring->ConfigFile );
   if ( nfiles == 0 )
   {
        logit( "e" , "startstop: error opening command file <%s>; exiting!\n",
                 metaring->ConfigFile );
        return ERROR_FILE_NOT_FOUND;
   }


/* Process all command files
   *************************/
   while(nfiles > 0)   /* While there are command files open */
   {

        while(k_rd())        /* Read next line from active file  */
        {
            com = k_str();         /* Get the first token from line */

        /* Ignore blank lines & comments
           *****************************/
            if( !com )           continue;
            if( com[0] == '#' )  continue;

        /* Process anything else as a command;
           Expect commands in a certain order!
           ***********************************/
            switch (state)
            {
            /* Read the number of transport rings
            *************************************/
            case READ_NRING:
                if ( !k_its( "nRing" ) )
                {
                    strcpy( missed, "nRing" );
                    break;
                }
                if (firstTimeThrough) {/* Only if we're reading the config for the first time do we keep track */
                    metaring->nRing  = k_int();
                }
                state  = READ_RINGINFO;
                if (firstTimeThrough) {
                    ir = 0;
                } else {
                    ir = metaring->nRing;
                }
                break;

            /* Read the transport ring names & keys & size
             *********************************************/
            case READ_RINGINFO:
                if( !k_its( "Ring" ) )
                {
                    if ((k_its( "MyModuleId" )) && (!firstTimeThrough)) {
                        /* OK to skip MyModuleId on 2nd time through; we already know it */
                        state = READ_HEARTBEAT;
                        /* if this isn't the first time through we want to add the rings we just found to nRing */
                        newNRing = ir;
                    } else {
                       strcpy( missed, "Ring" );
                    }
                    break;
                }
                if( ir == MAX_RING )
                {
                    logit( "e" ,
                           "Too many transport rings, max=%d; exiting!\n",
                            MAX_RING );
                    return ERROR_OUTOFMEMORY;
                }
                str = k_str();
                if(!str)  break;

                for ( i = 0; i < ir; i++ ) {
                    if ( strcmp (str, metaring->ringName[i]) == 0 ){
                        duplicate = TRUE;
                    }
                }
                if (duplicate) {
                    if (firstTimeThrough) {
                        logit( "e", "Duplicate ring name <%s>; exiting!\n", str);
                        return ERROR_INVALID_NAME;
                    }
                    duplicate = FALSE;
                } else {
                    strcpy( metaring->ringName[ir], str );
                    metaring->ringSize[ir] = k_int();
                    if( (metaring->ringKey[ir]= GetKey(metaring->ringName[ir])) == -1 )
                    {
                       logit( "e" , "Invalid ring name <%s>; exiting!\n",
                               metaring->ringName[ir] );
                       return ERROR_INVALID_NAME;
                    }
                    if( ++ir == metaring->nRing )  state = READ_MODID;
                }
                break;

            /* Read stuff concerning startstop itself
               **************************************/
            case READ_MODID:
                if ( !k_its("MyModuleId") )
                {
                    strcpy( missed, "MyModuleId" );
                    break;
                }
                str = k_str();
                if(!str)  break;
                strcpy( metaring->MyModName, str );
                if( GetModId(metaring->MyModName, &(metaring->MyModId)) == -1 )
                {
                   logit( "e" , "Invalid MyModuleId <%s>; exiting!\n",
                           metaring->MyModName );
                   return ERROR_INVALID_MODULETYPE;
                }
                state = READ_HEARTBEAT;
                break;

            case READ_HEARTBEAT:
                if ( !k_its("HeartbeatInt") )
                {
                    strcpy( missed, "HeartbeatInt" );
                    break;
                }
                metaring->HeartbeatInt = k_int();
                state = READ_MYPRIORITYCLASS;
                break;

            case READ_MYPRIORITYCLASS:
                if ( !k_its("MyPriorityClass") )
                {
                    strcpy( missed, "MyPriorityClass" );
                    break;
                }
                str = k_str();
                if ( str ) strcpy( metaring->MyPriorityClass, str );
                state  = READ_LOGSWITCH;
                break;

            case READ_LOGSWITCH:
                if ( !k_its("LogFile") )
                {
                    strcpy( missed, "LogFile" );
                    break;
                }
                metaring->LogSwitch = k_int();
                state     = READ_KILLDELAY;
                break;

            case READ_KILLDELAY:
              if ( !k_its("KillDelay") )
              {
                strcpy( missed, "KillDelay" );
                break;
              }
              metaring->KillDelay = k_int();
              state     = READ_SLEEPTIME;
              break;

            /* Optional command to tell startstop to wait a number
             of seconds after starting statmgr John Patton*/
            case READ_SLEEPTIME:
              if ( !k_its("statmgrDelay") )
              {
                state     = READ_PROCESS;

                /* since we're optonal, and not present, jump to the next */
                /*  command in line instead of declaring an error */
                goto no_sleep_time;
              }
              metaring->statmgr_sleeptime = (k_int() * 1000);
              state     = READ_PROCESS;
              break;


           /* Read a command to start a child
              *******************************/
            case READ_PROCESS:
                no_sleep_time:
                if ( !k_its("Process") )
                {
                    strcpy( missed, "Process" );
                    break;
                }
                if ( *nChild == MAX_CHILD )
                {
                    logit( "e" ,
                           "Too many child processes, max=%d; exiting!\n",
                            MAX_CHILD );
                    return ERROR_TOO_MANY_MODULES;
                }
                str = k_str();
                if ( str )
                {
                   duplicate = FALSE;
                   for ( ichild = 0; ichild < *nChild; ichild++ ) {
                       if ( strcmp (str, child[ichild].commandLine) == 0 ) {
                           logit ( "", "Skipping twin, no duplicate children allowed: %s\n", str);
                           duplicate = TRUE;
                       }
                   }
                   if (duplicate) {
                       state = READ_PRIORITYCLASS;
                       break;
                   }

                   strcpy( child[*nChild].commandLine, str );


                   ptr = strtok( str, " \t" );            /* Get program name */
                   if ( ptr == NULL )
                   {
                      logit( "e" , "Bad program name.\n");
                      break;
                   }
                   strcpy( child[*nChild].progName, ptr );
                }

                /* If this process is statmgr, store it's location in the child array
                 so that we can find it again later */
                /* Modified by Alex (WOW) to support searching for
                 statmgr2 for hydra
                 */
                if ((strcmp( child[*nChild].progName, "statmgr") == 0)||
                    (strcmp( child[*nChild].progName, "statmgr2") == 0))
                {
                    metaring->statmgr_location = *nChild;
                }

                state = READ_PRIORITYCLASS;
                break;

            /* Read the child's priority class
               *******************************/
            case READ_PRIORITYCLASS:
                if ( !k_its("PriorityClass") )
                {
                    strcpy( missed, "PriorityClass" );
                    break;
                }
                str = k_str();
                if (( str ) && (!duplicate))
                   strcpy( child[*nChild].priorityClass, str );
                state = READ_THREADPRIORITY;
                break;

            /* Read the child's thread priority
               ********************************/
            case READ_THREADPRIORITY:
                if ( !k_its("ThreadPriority") )
                {
                    strcpy( missed, "ThreadPriority" );
                    break;
                }
                str = k_str();
                if (( str ) && (!duplicate))
                   strcpy( child[*nChild].threadPriority, str );
                state = READ_DISPLAY;
                break;

            /* Read the child's startup state
               ******************************/
            case READ_DISPLAY:
                if ( !k_its("Display") )
                {
                    strcpy( missed, "Display" );
                    break;
                }
                str = k_str();
                if (( str ) && (!duplicate)) {
                   strcpy( child[*nChild].display, str );
                   (*nChild)++;
                }
                state = READ_PROCESS;
                break;



            }

        /* Complain if we got an unexpected command
           ****************************************/
            if( missed[0] )
            {
                logit( "e" , "Incorrect command order in <%s>; exiting!\n",
                         metaring->ConfigFile );
                logit( "e" , "Expected: <%s>  Found: <%s>\n",
                         missed, com );
                logit( "e" , "Offending line: %s\n", k_com() );
               return ERROR_BAD_CONFIGURATION;
            }

        /* See if there were any errors processing this command
           ****************************************************/
            if( k_err() )
            {
               logit( "e" , "Bad <%s> command in <%s>; exiting!\n",
                        com, metaring->ConfigFile );
               logit( "e" , "Offending line: %s\n", k_com() );
               return ERROR_BAD_COMMAND;
            }
        }
        nfiles = k_close();
   }
   return 0;
}   /* End GetConfig */





/***************************** EncodeStatus ***********************
 *                    Encode the status message                   *
 ******************************************************************/

void EncodeStatus( char *statusMsg, METARING *metaring, CHILD   child[MAX_CHILD], int *nChild  )
{
   char   line[100];
   char   timenow[26];
   int    i;

   sprintf( statusMsg, "                      EARTHWORM STATUS\n\n" );

   sprintf( line,    "    Start time (UTC):           %s", metaring->tstart );
   strcat( statusMsg, line );

   GetCurrentUTC( timenow );
   sprintf( line,    "    Current time (UTC):         %s", timenow );
   strcat( statusMsg, line );

   for ( i = 0; i < metaring->nRing; i++ )
   {
      /* if you change "name/key/size:" then statmgr.c which looks for this key needs to change too */
      sprintf( line, "    Ring %2d name/key/size:      %s / %d / %d kb\n", i+1,
               metaring->ringName[i], metaring->ringKey[i], metaring->ringSize[i] );
      strcat( statusMsg, line );
   }
   sprintf( line,    "    Startstop's Config File:    %s\n", metaring->ConfigFile );
   strcat( statusMsg, line );
   sprintf( line,    "    Startstop's Priority Class: %s\n", metaring->MyPriorityClass );
   strcat( statusMsg, line );
   sprintf( line,    "    Startstop Version:          %s\n", metaring->Version );
   strcat( statusMsg, line );

/* Print stuff about each child process
   ************************************/
   strcat( statusMsg, "\n          Process                 Process          Priority   Thread\n" );
   strcat( statusMsg,   "        Command Line                Id     Status   Class    Priority  Console\n" );
   strcat( statusMsg,   " ------------------------         -------  ------  --------  --------  -------\n" );

   for ( i = 0; i < *nChild; i++ )
   {
      DWORD status;
      char  statusStr[6];

/* See if the child process has died
   *********************************/
      statusStr[0] = '\0';

     if( strcmp( child[i].status, "Stopped" ) == 0 ) {
        strcpy( statusStr, "Stop" );
     } else if( strcmp( child[i].status, "DOA" ) == 0 ) {
        strcpy( statusStr, "DOA" );
     } else {
         status = WaitForSingleObject( child[i].procInfo.hProcess, 0 );
         if ( status == WAIT_OBJECT_0 ) {
              strcpy( statusStr, "Dead" );
         } else if ( status == WAIT_TIMEOUT ) {
              strcpy( statusStr, "Alive" );
         }
     }

/* Show the child's the command line
 ***********************************/
      sprintf( line, " %-80s", child[i].commandLine );
      line[33] = '\0'; /* only show first 32 chars of the command line */
      strcat( statusMsg, line );

/* Add other info about the child
 ********************************/
      sprintf( line, " %-8d",    child[i].procInfo.dwProcessId );
      strcat( statusMsg, line );
      sprintf( line, " %-7s",    statusStr );
      strcat( statusMsg, line );
      sprintf( line, " %-9s",    child[i].priorityClass );
      strcat( statusMsg, line );
      sprintf( line, " %-12s",   child[i].threadPriority );
      line[9] = '\0'; /* only show first 8 chars of threadPriority */
      strcat( statusMsg, line );
      if( strcmp( child[i].display, "NoNewConsole" ) == 0 )
           sprintf( line, "  NoNew" );
      else if( strcmp( child[i].display, "MinimizedConsole" ) == 0 )
           sprintf( line, "  Minim" );
      else
           sprintf( line, "  New" );
      strcat( statusMsg, line );

/* Append a newline character
 ****************************/
      strcat( statusMsg, "\n" );
   }
}/* end EncodeStatus */

  /******************************************************************
   *                           StartChild                           *
   *                                                                *
   * Start a specific child given by the index in the child array   *
   ******************************************************************/
int StartChild( int ich, METARING *metaring, CHILD  child[MAX_CHILD]  )
{
   STARTUPINFO startUpInfo;
   BOOL  success;
   DWORD priorityClass;
   int   threadPriority;
   DWORD display = CREATE_NEW_CONSOLE;
   char title[80];
   DWORD previousPID;

/* Retrieve the STARTUPINFO structure for the current process.
   Use this structure for the child processes.
   ***********************************************************/
      GetStartupInfo( &startUpInfo );


      previousPID = child[ich].procInfo.dwProcessId;
/* Set the priority class
   **********************/
      if ( strcmp( child[ich].priorityClass, "Idle"     ) == 0 )
         priorityClass = IDLE_PRIORITY_CLASS;
      else if ( strcmp( child[ich].priorityClass, "Normal"   ) == 0 )
         priorityClass = NORMAL_PRIORITY_CLASS;
      else if ( strcmp( child[ich].priorityClass, "High"     ) == 0 )
         priorityClass = HIGH_PRIORITY_CLASS;
      else if ( strcmp( child[ich].priorityClass, "RealTime" ) == 0 )
         priorityClass = REALTIME_PRIORITY_CLASS;
      else
      {
         logit( "et", "Invalid PriorityClass: %s  Exiting.\n",
                child[ich].priorityClass );
         return -1;
      }

/* Set the display type
   ********************/
      if ( strcmp( child[ich].display, "NewConsole" ) == 0 )
         display = CREATE_NEW_CONSOLE;
      if ( strcmp( child[ich].display, "NoNewConsole" ) == 0 )
         display = 0;
      if ( strcmp( child[ich].display, "MinimizedConsole" ) == 0 )
      {
         display = CREATE_NEW_CONSOLE;
         startUpInfo.dwFlags |= STARTF_USESHOWWINDOW;
         startUpInfo.wShowWindow = SW_SHOWMINNOACTIVE;
      }

/* Set the title of the child's window to the child's name
   *******************************************************/
      if ( display == CREATE_NEW_CONSOLE )
      {
         strcpy( title, "Console Window for <" );
         strcat( title, child[ich].commandLine );
         strcat( title, ">" );
         startUpInfo.lpTitle = (LPTSTR)title;

         /* Tell Windows to use the "standard" station and desktop, rather than what's inherited.
          * Otherwise, console's won't be displayed at all, which can be frustrating...
          * This is only needed for service startstop */
         startUpInfo.lpDesktop = "WinSta0\\Default";
      }
      else
         startUpInfo.lpTitle = NULL;



/* Start a child process
   *********************/
      success = CreateProcess( 0,
                               child[ich].commandLine, /* Command line to invoke child */
                               0, 0,                   /* No security attributes */
                               FALSE,                  /* No inherited handles */
                               display |               /* Child may get its own window */
                               priorityClass,          /* Priority class of child process */
                               0,                      /* Not passing environmental vars */
                               0,                      /* Current dir same as calling process */
                               &startUpInfo,           /* Attributes of process window */
                               &(child[ich].procInfo) ); /* Attributes of child process */
      if ( !success )
      {
         LPVOID lpMsgBuf;
         DWORD  lasterror = GetLastError();

         FormatMessage(
             FORMAT_MESSAGE_ALLOCATE_BUFFER |
             FORMAT_MESSAGE_FROM_SYSTEM |
             FORMAT_MESSAGE_IGNORE_INSERTS,
             NULL,
             lasterror,
             MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
             (LPTSTR) &lpMsgBuf,
             0,
             NULL );
         child[ich].procInfo.dwProcessId = previousPID;
         strcpy( child[ich].status, "DOA" ); /* Dead On Arrival */
         logit( "et", "Trouble creating child process <%s>; Error %d: %s\n",
                child[ich].commandLine, lasterror, lpMsgBuf );

         LocalFree( lpMsgBuf );
         return -1;
      }

/* Set the thread priority of the child process
   ********************************************/
      if ( strcmp( child[ich].threadPriority, "Lowest"     ) == 0 )
         threadPriority = THREAD_PRIORITY_LOWEST;
      else if ( strcmp( child[ich].threadPriority, "BelowNormal"   ) == 0 )
         threadPriority = THREAD_PRIORITY_BELOW_NORMAL;
      else if ( strcmp( child[ich].threadPriority, "Normal"     ) == 0 )
         threadPriority = THREAD_PRIORITY_NORMAL;
      else if ( strcmp( child[ich].threadPriority, "AboveNormal" ) == 0 )
         threadPriority = THREAD_PRIORITY_ABOVE_NORMAL;
      else if ( strcmp( child[ich].threadPriority, "Highest" ) == 0 )
         threadPriority = THREAD_PRIORITY_HIGHEST;
      else if ( strcmp( child[ich].threadPriority, "TimeCritical" ) == 0 )
         threadPriority = THREAD_PRIORITY_TIME_CRITICAL;
      else if ( strcmp( child[ich].threadPriority, "Idle" ) == 0 )
         threadPriority = THREAD_PRIORITY_IDLE;
      else
      {
         logit( "et", "Invalid ThreadPriority: %s  Exiting.\n",
                child[ich].threadPriority );
         return -1;
      }
      success = SetThreadPriority( child[ich].procInfo.hThread, /* Thread handle */
                                   threadPriority );
      if ( !success )
      {
         logit( "et", "Error setting child thread priority.  Exiting.\n" );
         return -1;
      }
      strcpy( child[ich].status, "Alive" ); /* this just changes the flag, when status checks later it'll confirm this */
      return 0;
} /* end StartChild */

  /******************************************************************
   *                      TerminateChild                            *
   *                                                                *
   * Kill a specific child given by the index in the child array.   *
   ******************************************************************/
int TerminateChild( int ich, METARING *metaring, CHILD  child[MAX_CHILD] )
{
   int status = 0;
   int wait_timeout = metaring->KillDelay;

/* Terminate the child process.  NT allows this to be done, but warns:
   "Use it only in extreme circumstances.  The state of global data
    maintained by dynamic-link libraries (DLLs) may be compromised if
    TerminateProcess is used rather than ExitProcess"
 *********************************************************************/
   TerminateProcess( child[ich].procInfo.hProcess, -1 );

   status = WaitForSingleObject( child[ich].procInfo.hProcess, wait_timeout*1000 );
   if ( status != WAIT_OBJECT_0 ) return -1;

   return 0;
} /* end TerminateChild */

  /******************************************************************
   *                           RestartChild                         *
   *                                                                *
   *     Restart a specific child given a TYPE_RESTART message      *
   ******************************************************************/

void RestartChild( char *restartmsg, METARING *metaring, CHILD  child[MAX_CHILD], int *nChild  )
{

   int ich = 0, ir, ret;
   DWORD currentPID;
   char ErrText[512];
   ret = StopChild( restartmsg, metaring, child, nChild, &ich );
   if (ret == EXIT) {
       return;
   }


/* Restart the child!
 ********************/
   if( StartChild( ich, metaring, child ) != 0 )
   {
      sprintf( ErrText, "restart of <%s> failed\n",
               child[ich].commandLine );
      ReportError( ERR_STARTCHILD, ErrText, metaring );
      for( ir=0; ir<metaring->nRing; ir++ ) tport_putflag( &(metaring->Region[ir]), 0 );
   }
   currentPID = child[ich].procInfo.dwProcessId;
   if( currentPID < 0 ) {
       strcpy( child[ich].status, "Alive" );
   }
   logit( "et", "startstop: restarted <%s>\n",
          child[ich].commandLine );
} /* end RestartChild */



  /******************************************************************
   *                           StopChild                            *
   *                                                                *
   *     Stop a specific child given a TYPE_RESTART or              *
   *     TYPE_STOP message                                          *
   ******************************************************************/

int StopChild( char *restartmsg, METARING *metaring, CHILD  child[MAX_CHILD], int *nChild, int *ich )
{
   int procId = 0;
   int status = 0;
   int ir, childNum;
   char ErrText[512];


/* Find this process id in the list of children
 **********************************************/
   procId = atoi(restartmsg);

   for( childNum = 0; childNum < *nChild; childNum++ )
   {
      if( (int) child[childNum].procInfo.dwProcessId == procId ) break;
   }

   if( childNum == *nChild )
   {
      logit( "et", "startstop: Cannot stop process_id %d; "
                    "it is not my child!\n", procId );
      return EXIT;
   }
   /* our records show this child has already been stopped. so we'll not try to stop it again */
   if( strcmp( child[childNum].status, "Stopped" ) == 0 ) {
       goto cleanup;
   }
   /* if we have a negative process ID, we never successfully started this child */
   if ( procId < 0 ) {
      /* strcpy( child[childNum].status, "Stopped" ); */
      goto cleanup;
   }

/* Give child a chance to shut down gracefully...
 ************************************************/
   for( ir=0; ir<metaring->nRing; ir++ ) tport_putflag( &(metaring->Region[ir]), procId );
   status = WaitForSingleObject( child[childNum].procInfo.hProcess, metaring->KillDelay*1000 );

/* ...but if it takes too long, then kill the child process
 **********************************************************/
   if( status != WAIT_OBJECT_0 )
   {
      logit( "et", "startstop: <%s> (pid=%d) did not shut down in %d sec;"
             " terminating it!\n", child[childNum].commandLine, procId, metaring->KillDelay );

      if( TerminateChild( childNum, metaring, child ) != 0 )
      {
         sprintf( ErrText, "termination of <%s> failed; "
                       "waited %d seconds; cannot restart!\n",
                  child[childNum].commandLine, metaring->KillDelay );
         ReportError( ERR_STARTCHILD, ErrText, metaring );
         return EXIT; /* we don't want a start to happen if the stop failed */
      }
   }

   /* Close child process and thread handles
    * NB: These CloseHandle () calls used to be
    * in TerminateChild().  But, this causes
    * a very nasty, little bug whereby the handles
    * would not be cleaned up if the child terminated
    * itself, i.e., upon not being able to connect
    * to a K2.  This caused a slow handle leak which,
    * alas, caused the system to crash eventually.
    *
    *   Lucky Vidmar 19 April 2002
    *************************************************/
   if( CloseHandle( child[childNum].procInfo.hProcess ) == 0 ) {
      logit("e","startstop: error closing child process handle: %s\n",
            GetLastError() );
      return EXIT; /* we don't want a start to happen if the stop failed */
   }
   if( CloseHandle( child[childNum].procInfo.hThread ) == 0 ) {
      logit("e","startstop: error closing child thread handle: %s\n",
            GetLastError() );
      return EXIT; /* we don't want a start to happen if the stop failed */
   }

   strcpy( child[childNum].status, "Stopped" );

cleanup:
   for( ir=0; ir<metaring->nRing; ir++ ) tport_putflag( &(metaring->Region[ir]), 0 );
   *ich = childNum;
   return SUCCESS;
} /* end StopChild */


int StartstopSetup ( METARING *metaring, volatile int *checkpoint, boolean service, CHILD   child[MAX_CHILD], int *nChild ) {
    HANDLE  MyHandle;
    char    *runPath;
    int     i;
    int     ichild;
    BOOL    success;
    int     err;

    badProcessID = -100;

    /* Change working directory to environment variable EW_PARAMS value
   ***************************************************************/
   runPath = getenv( "EW_PARAMS" );

   if ( runPath == NULL )
   {
      logit( "e" ,
              "Environment variable EW_PARAMS not defined; exiting!\n" );
      if ( service ) {
        set_service_status(SERVICE_STOPPED, ERROR_BAD_ENVIRONMENT, 0, 0);
      }
      return -1;
   }

   if ( *runPath == '\0' )
   {
      logit( "e" , "Environment variable EW_PARAMS " );
      logit( "e" , "defined, but has no value; exiting!\n" );
      if ( service ) {
       set_service_status(SERVICE_STOPPED, ERROR_BAD_ENVIRONMENT, 0, 0);
      }
      return -1;
   }
   success = SetCurrentDirectory( runPath );

   if ( !success )
   {
      logit( "e" , "Params directory not found: %s\n", runPath );
      logit( "e" ,
             "Please reset environment variable EW_PARAMS; exiting!\n" );
      if ( service ) {
       set_service_status(SERVICE_STOPPED, ERROR_BAD_ENVIRONMENT, 0, 0);
      }
      return -1;
   }

/* Read configuration parameters
   *****************************/
   *nChild = 0;
   metaring->nRing = 0;
   err = GetConfig( metaring, child, nChild  );
   if ( service ) {
       if (err != 0)
       {
           set_service_status(SERVICE_STOPPED, err, 0, 0);
           return -1;
       }
   }
   logit( "e" , "startstop: Read command file <%s>\n", metaring->ConfigFile );
   if ( service ) {
       set_service_status(SERVICE_START_PENDING, 0, *checkpoint, 10000);
       *checkpoint = *checkpoint + 1;
   }

/* Reinitialize the logging level
   *************************************/
   logit_init( metaring->ConfigFile, 0, 1024, metaring->LogSwitch );
   if ( service ) {
        set_service_status(SERVICE_START_PENDING, 0, *checkpoint, 10000);
        *checkpoint = *checkpoint + 1;
   }

/* Get the local installation id from the
   environmental variable EW_INSTALLATION
   **************************************/
   if ( GetLocalInst( &(metaring->InstId) ) < 0 )
   {
      logit( "e" , "Error getting the local installation id; exiting!\n" );
      if ( service ) {
       set_service_status(SERVICE_STOPPED, ERROR_BAD_ENVIRONMENT, 0, 0);
      }
      return -1;
   }

/* Look up module id and message type numbers
   ******************************************/
   if ( GetModId( "MOD_WILDCARD", &(metaring->ModWildcard) ) != 0 )
   {
      printf( "startstop: Invalid module name <MOD_WILDCARD>. Exiting.\n" );
      if ( service ) {
       set_service_status(SERVICE_STOPPED, ERROR_INVALID_MODULETYPE, 0, 0);
      }
      return -1;
   }
   if ( GetType( "TYPE_HEARTBEAT", &(metaring->TypeHeartBeat) ) != 0 )
   {
      printf( "startstop: Invalid message type <TYPE_HEARTBEAT>. Exiting.\n" );
      if ( service ) {
       set_service_status(SERVICE_STOPPED, ERROR_INVALID_MESSAGENAME, 0, 0);
      }
      return -1;
   }
   if ( GetType( "TYPE_ERROR", &(metaring->TypeError) ) != 0 )
   {
      printf( "startstop: Invalid message type <TYPE_ERROR>. Exiting.\n" );
      if ( service ) {
       set_service_status(SERVICE_STOPPED, ERROR_INVALID_MESSAGENAME, 0, 0);
      }
      return -1;
   }
   if ( GetType( "TYPE_RESTART", &(metaring->TypeRestart) ) != 0 )
   {
      printf( "startstop: Invalid message type <TYPE_RESTART>. Exiting.\n" );
      if ( service ) {
       set_service_status(SERVICE_STOPPED, ERROR_INVALID_MESSAGENAME, 0, 0);
      }
      return -1;
   }
   if ( GetType( "TYPE_STOP", &(metaring->TypeStop) ) != 0 )
   {
      printf( "startstop: Invalid message type <TYPE_STOP>. Exiting.\n" );
      if ( service ) {
       set_service_status(SERVICE_STOPPED, ERROR_INVALID_MESSAGENAME, 0, 0);
      }
      return -1;
   }
   if ( GetType( "TYPE_REQSTATUS", &(metaring->TypeReqStatus) ) != 0 )
   {
      printf( "startstop: Invalid message type <TYPE_REQSTATUS>. Exiting.\n" );
      if ( service ) {
       set_service_status(SERVICE_STOPPED, ERROR_INVALID_MESSAGENAME, 0, 0);
      }
      return -1;
   }
   if ( GetType( "TYPE_STATUS", &(metaring->TypeStatus) ) != 0 )
   {
      printf( "startstop: Invalid message type <TYPE_STATUS>. Exiting.\n" );
      if ( service ) {
       set_service_status(SERVICE_STOPPED, ERROR_INVALID_MESSAGENAME, 0, 0);
      }
      return -1;
   }
   if ( GetType( "TYPE_RECONFIG", &(metaring->TypeReconfig) ) != 0 )
   {
      printf( "startstop: Invalid message type <TYPE_RECONFIG>. Exiting.\n" );
      if ( service ) {
       set_service_status(SERVICE_STOPPED, ERROR_INVALID_MESSAGENAME, 0, 0);
      }
      return -1;
   }


/* Set startstop's priority class
   ******************************/
   MyHandle = GetCurrentProcess();
   if ( strcmp( metaring->MyPriorityClass, "Idle" ) == 0 )
      SetPriorityClass( MyHandle, IDLE_PRIORITY_CLASS );
   else if ( strcmp( metaring->MyPriorityClass, "Normal" ) == 0 )
      SetPriorityClass( MyHandle, NORMAL_PRIORITY_CLASS );
   else if ( strcmp( metaring->MyPriorityClass, "High" ) == 0 )
      SetPriorityClass( MyHandle, HIGH_PRIORITY_CLASS );
   else if ( strcmp( metaring->MyPriorityClass, "RealTime" ) == 0 )
      SetPriorityClass( MyHandle, REALTIME_PRIORITY_CLASS );
   else
   {
      logit( "et", "Invalid MyPriorityClass: %s  Exiting.\n", metaring->MyPriorityClass );
      if ( service ) {
       set_service_status(SERVICE_STOPPED, ERROR_INVALID_PRIORITY, 0, 0);
      }
      return -1;
   }

/* Get UTC start time and convert to a 26-char string
   **************************************************/
   GetCurrentUTC( metaring->tstart );

/* Allocate region structures for transport rings
   **********************************************/

   /* from memory.h: typedef unsigned int size_t; */
   /* allocating MAX_RING worth of region space rather than metaring->nRing so we can */
   /* add more rings later */
   metaring->Region = (SHM_INFO *) calloc( (size_t)MAX_RING, sizeof(SHM_INFO) );
   if ( metaring->Region == NULL )
   {
      logit( "et", "Error allocating region structures. Exiting.\n" );
      if ( service ) {
       set_service_status(SERVICE_STOPPED, ERROR_NOT_ENOUGH_MEMORY, 0, 0);
      }
      return -1;
   }

  if ( service ) {
   set_service_status(SERVICE_START_PENDING, 0, *checkpoint, 10000);
   *checkpoint = *checkpoint + 1;
  }

/* Create the transport rings
   **************************/
   for ( i = 0; i < metaring->nRing; i++ )
      tport_create( &(metaring->Region[i]), 1024 * metaring->ringSize[i], metaring->ringKey[i] );

/* Start the heart beating
   ***********************/
   Heartbeat( metaring );

   if ( service ) {
       set_service_status(SERVICE_START_PENDING, 0, *checkpoint, 10000);
       *checkpoint = *checkpoint + 1;
   }

    /* start statmgr child first if present */
   if (metaring->statmgr_location != (MAX_CHILD + 1)) {
        ichild = metaring->statmgr_location;

        if ( StartChild( ichild, metaring, child ) != 0 )
        {
            StartError(ichild, child[metaring->statmgr_location].commandLine, metaring, nChild);
        }

        logit("et","startstop: process <%s> started.\n",
                child[metaring->statmgr_location].commandLine );

        /* Sleep after starting statmgt to allow statmgr to come up */
        logit("et","startstop: sleeping <%d> second(s) for statmgr startup.\n",
                (metaring->statmgr_sleeptime/1000) );

       if ( service ) {
        set_service_status(SERVICE_START_PENDING, 0, *checkpoint, metaring->statmgr_sleeptime + 10000);
        *checkpoint = *checkpoint + 1;
       }
        sleep_ew(metaring->statmgr_sleeptime);

   } // Done  starting statmgr
   /* start the other children that aren't statmanager */
   SpawnChildren( metaring, child, nChild, service, checkpoint);

   return 0;

} /* end StartstopSetup */

void SpawnChildren (METARING *metaring, CHILD child[MAX_CHILD], int *nChild, boolean service, volatile int *checkpoint  ){
    /* Start the child processes
    *************************/
    int ichild;

    /* The following changes were made by John Patton to fix the
     processes dying before statmgr comes up bug */
   for ( ichild = oldNChild; ichild < *nChild; ichild++ )
   {
        /* To prevent starting statmgr a second time, we'll just skip over it
          If it's not there, then the index to skip defaults to one past MAXCHILD, so
          no other modules will be skipped. */
        if (ichild != metaring->statmgr_location)
        {
            if ( StartChild( ichild, metaring, child ) != 0 )
            {
                StartError(ichild, child[ichild].commandLine, metaring, nChild);
                child[ichild].procInfo.dwProcessId = badProcessID--;
                strcpy( child[ichild].status, "DOA" ); /* Dead On Arrival */
            } else {
                logit("et","startstop: process <%s> started.\n",
                  child[ichild].commandLine );
            }

            if ( service ) {
                set_service_status(SERVICE_START_PENDING, 0, *checkpoint, 10000);
                *checkpoint = *checkpoint + 1;
            }
        }
   }
} /* end SpawnChildren */

int FinalLoop ( METARING *metaring, volatile int *done, char ewstat[MAX_STATUS_LEN], volatile int *checkpoint, boolean service, CHILD child[MAX_CHILD], int *nChild  ) {
/* Wait for the Interactive thread to set the done flag
   or for a TERMINATE flag to show up in any memory ring.
   In the meantime, check for any ReqStatus or Restart messages...
   ***************************************************************/
   MSG_LOGO  getLogo[4]; /* if you change this 4 here, you need to change the parameter to tport_getmsg as well */
   MSG_LOGO  msglogo;
   int       i, j;
   int       rc, placeholder;
   int       BytesRead;
   int       err;
   long      msgsize = 0L;
   char      msg[512];
   static char szNamedPipeBuffer[128];
   HANDLE hGUIPipe;

   for (i = 0; i < 4; i++) {
       getLogo[i].instid = metaring->InstId;
       getLogo[i].mod    = metaring->ModWildcard;
   }
   getLogo[0].type   = metaring->TypeRestart;
   getLogo[1].type   = metaring->TypeReqStatus;
   getLogo[2].type   = metaring->TypeReconfig;
   getLogo[3].type   = metaring->TypeStop;


   /* Create a named pipe.  This is used to receive commands to create a new console in
      the LocalService space. */
   hGUIPipe = CreateNamedPipe("\\\\.\\pipe\\HydraStartstopPipe",
                                PIPE_ACCESS_INBOUND,
                                PIPE_TYPE_BYTE | PIPE_NOWAIT,
                                1,
                                1024,
                                1024,
                                100,
                                NULL);
   if (hGUIPipe == INVALID_HANDLE_VALUE)
   {
        err = GetLastError();
        logit( "et", "Unable to create named pipe: Error %d.  Exiting.\n", err);
        set_service_status(SERVICE_STOPPED, err, 0, 0);
        return;
   }


   logit("et", "Beginning main loop.\n");

   while( !*done )
   {

   /* Send hearbeat, if it's time
    *****************************/
      Heartbeat( metaring );
      for ( i = 0; i < metaring->nRing; i++ )
      {
      /* Is "kill flag" there?
       ***********************/
         if ( tport_getflag( &(metaring->Region[i]) ) == TERMINATE )
         {
            logit("et", "'done' set from kill flag.\n");
            *done = 1;
            continue;
         }

      /* Look for any interesting new messages
       ***************************************/
         rc = tport_getmsg( &(metaring->Region[i]), getLogo, 4,
                            &msglogo, &msgsize, msg, sizeof(msg)-1 );

         if( rc==GET_NONE || rc==GET_TOOBIG ) continue;
      /* Process a new message
       ***********************/
         msg[msgsize]='\0';  /* Null-terminate it for easy handling */
         if     ( msglogo.type == metaring->TypeRestart   )  RestartChild( msg, metaring, child, nChild  );
         else if( msglogo.type == metaring->TypeStop )  StopChild( msg, metaring, child, nChild, &placeholder);
         else if( msglogo.type == metaring->TypeReqStatus )  SendStatus( i, metaring, child, nChild);
         else if( msglogo.type == metaring->TypeReconfig ) {
             GetUtil_LoadTable(); /* reread earthworm.d and earthworm_global.d */
             logit( "e" , "startstop: Reconfigure: Re-reading command file <%s>\n", metaring->ConfigFile );
           /* We're re-reading the startstop*.d file here, and adding any new modules or rings */
           /* Even if this new GetConfig returns an error, we don't want to bail out on our running earthworm */
           oldNChild = *nChild;
           GetConfig( metaring, child, nChild );

           for ( j = metaring->nRing; j < (newNRing); j++ ) {
                logit( "e" , "tport_create: creating ring number <%d>\n", j );
                tport_create( &(metaring->Region[j]),
                    1024 * metaring->ringSize[j],
                    metaring->ringKey[j] );
                metaring->nRing ++;
           }

           SpawnChildren( metaring, child, nChild, service, checkpoint );
           logit( "e" , "startstop: Final reconfigure step: Restart statmgr\n" );
           sprintf (msg, "%d", child[metaring->statmgr_location].procInfo.dwProcessId);
           RestartChild( msg, metaring, child, nChild );
         } else
            logit("et","Got unexpected msg type %d from transport region: %s\n",
                        (int) msglogo.type, metaring->ringName[i] );
      }
      /* Look for a "create new console" message on our named pipe. */
      if (!ConnectNamedPipe(hGUIPipe, NULL))
      {
          err = GetLastError();
          if (err == ERROR_PIPE_LISTENING)
          {
              /* Nothing to do...nobody to talk to. */
          }
          else if (err == ERROR_PIPE_CONNECTED)
          {
              /* We've got a client on the other end.  See if we should open a console. */
              if (!ReadFile(hGUIPipe, szNamedPipeBuffer, 128, &BytesRead, NULL))
              {
                  err = GetLastError();
                  if (err != ERROR_NO_DATA)
                    logit("et", "Error reading data over named pipe: %d\n", err);
              }
              else if (BytesRead > 0)
              {
                  if (strcmp(szNamedPipeBuffer, "NewConsole") == 0)
                  {
                      LaunchNewConsole();
                  }
              }
          }
          else if (err == ERROR_NO_DATA)
          {
              /* The client we had on the other end has disconnected. */
              if (!DisconnectNamedPipe(hGUIPipe))
              {
                  err = GetLastError();
                  logit("et", "Error disconnecting client from named pipe: %d\n", err);
              }
          }
          else
              logit("et", "Error connecting to client over named pipe: %d\n", err);
      }

      sleep_ew( 1000 );
   }
   logit("et", "Main loop finished.\n");
   goto ShutDown;

ShutDown:

/* Set the TERMINATE flag in each memory ring
   ******************************************/
   for ( i = 0; i < metaring->nRing; i++ )
      tport_putflag( &(metaring->Region[i]), TERMINATE );

   logit( "et","Earthworm will stop in %d seconds...\n", metaring->KillDelay );

/* Wait for all processes to terminate
   ***********************************/
   {
      int nChildActive; /* Number of children that actually ever had a real process ID.
                           Dead On Arrival children missing binaries or whatever never
                           got a process ID, so we won't consider them active. (We can't
                           wait for them to quit, since they never started.) */
      DWORD  status;
      HANDLE *handles;
      const BOOL waitForAll = TRUE;
      const DWORD timeout = metaring->KillDelay*1000;       /* milliseconds */

      nChildActive = *nChild;

      for ( i = 0; i < *nChild; i++ ) {
          if (strcmp( child[i].status, "DOA") == 0) {
              nChildActive --;
          }
      }

      handles = (HANDLE *)malloc( nChildActive * sizeof(HANDLE) );
      if ( handles == NULL )
      {
         logit( "et", "Cannot allocate the handles array.\n" );

        if ( service ) {
            set_service_status(SERVICE_STOPPED, ERROR_NOT_ENOUGH_MEMORY, 0, 0);
            return 0;
        }
        return -1;

      }
      j = 0;
      for ( i = 0; i < *nChild; i++ ) {
          if (strcmp( child[i].status, "DOA") != 0) {
              handles[j] = child[i].procInfo.hProcess;
              j++;
          }
      }

      if ( service ) {
          set_service_status(SERVICE_STOP_PENDING, 0, *checkpoint, timeout + 500);
          *checkpoint = *checkpoint + 1;
      }
      status = WaitForMultipleObjects( (DWORD)nChildActive, handles, waitForAll, timeout );
      if ( status == WAIT_FAILED )
      {
         logit( "e", "WaitForMultipleObjects() failed. Error: %d\n", GetLastError() );
      }
      else if ( status == WAIT_TIMEOUT )
      {
         if ( service ) {
            set_service_status(SERVICE_STOP_PENDING, 0, *checkpoint, timeout + 500);
            *checkpoint = *checkpoint + 1;
         }
         logit( "e", "\nThe following child processes did not self-terminate:\n" );
         for ( i = 0; i < *nChild; i++ )
         {
            if ( WaitForSingleObject( child[i].procInfo.hProcess, 0 ) == WAIT_TIMEOUT )
            {
               if( TerminateChild( i, metaring, child ) == 0 )
               {
                  logit( "e", " <%s>  KILLED!\n", child[i].commandLine );
                  /* Close child process and thread handles
                   * NB: These CloseHandle () calls used to be
                   * in TerminateChild().  But, this causes
                   * a very nasty, little bug whereby the handles
                   * would not be cleaned up if the child terminated
                   * itself, i.e., upon not being able to connect
                   * to a K2.  This caused a slow handle leak which,
                   * alas, caused the system to crash eventually.
                   *
                   *   Lucky Vidmar 19 April 2002
                   *************************************************/
                   if( CloseHandle( child[i].procInfo.hProcess ) == 0 )
                       logit("e","startstop: error closing child process handle: %s\n",
                             GetLastError() );
                   if( CloseHandle( child[i].procInfo.hThread ) == 0 )
                       logit("e","startstop: error closing child thread handle: %s\n",
                             GetLastError() );
                   if ( service ) {
                       set_service_status(SERVICE_STOP_PENDING, 0, *checkpoint, timeout + 500);
                       *checkpoint = *checkpoint + 1;
                   }
               }
               else
                  logit( "e", " <%s>  Failed to kill\n", child[i].commandLine );
            }
         }
      }
      else
      {
         logit( "e", "\nAll child processes have terminated" );
      }
   }
   if ( service ) {
       set_service_status(SERVICE_STOP_PENDING, 0, *checkpoint, 2000);
       *checkpoint = *checkpoint + 1;
   }
/* Destroy shared memory regions
   *****************************/
   for( i = 0; i < metaring->nRing; i++ )
        tport_destroy( &(metaring->Region[i]) );
   logit( "e", ", done destroying shared memory regions" );

/* Free allocated space
   ********************/
   free( metaring->Region );
   /*****************
    * child is not an allocated pointer but a static array.  It cannot be free'd.
    * DK 2006/05/30
    * free( child  );
    ************************************************/
   CloseHandle(hGUIPipe);

   logit( "e", ", freeing allocated space.\n" );
   if ( service ) {
        set_service_status(SERVICE_STOPPED, 0, 0, 0);
   }

   return 0;

} /* end FinalLoop */

/******************************************************************
 *                            SendStatus()                        *
 *    Build a status message and put it in a transport ring       *
 ******************************************************************/
void SendStatus( int iring, METARING *metaring, CHILD child[MAX_CHILD], int *nChild )
{
   MSG_LOGO logo;
   int length;
   char ewstat[MAX_STATUS_LEN];

   logo.instid = metaring->InstId;
   logo.mod    = metaring->MyModId;
   logo.type   = metaring->TypeStatus;

   EncodeStatus( ewstat, metaring, child, nChild );
   length = strlen( ewstat );

   if ( tport_putmsg( &(metaring->Region[iring]), &logo, length, ewstat ) != PUT_OK )
      logit("t", "startstop: Error sending status msg to transport region: %s\n",
             metaring->ringName[iring] );
   return;
} /* end SendStatus */


/*****************************************************************************
 *                            LaunchNewConsole()                             *
 *    Hydra incorporation that launches a C++ named pipes console window     *
 *    to allow control of startstop service without administrator password   *
 *****************************************************************************/
void LaunchNewConsole()
{
    STARTUPINFO startUpInfo;
    PROCESS_INFORMATION procinfo;
    BOOL  success;
    DWORD priorityClass;
    int   threadPriority;
    DWORD display = CREATE_NEW_CONSOLE;
    char title[80];
    DWORD lasterror;

    logit("", "Creating new console window\n");

    //
    // To create a new console window, we'll use the procedure from StartChild, with
    // the command-line interpreter as the program to run.
    //

    // Get the startupinfo for this process; use this structure for the console window.
    GetStartupInfo( &startUpInfo );

    // Set the priority class
    priorityClass = NORMAL_PRIORITY_CLASS;

    // Set the display type
    display = CREATE_NEW_CONSOLE;

    strcpy(title, "Startstop console");
    startUpInfo.lpTitle = (LPTSTR)title;

    // Tell Windows to use the "standard" station and desktop, rather than what's inherited.
    // Otherwise, consoles won't be displayed at all, which can be frustrating...
    startUpInfo.lpDesktop = "WinSta0\\Default";

    // Start a child process
    success = CreateProcess( 0,
                            "cmd.exe",              /* Command line to invoke child */
                            0, 0,                   /* No security attributes */
                            FALSE,                  /* No inherited handles */
                            display |               /* Child may get its own window */
                                priorityClass,      /* Priority class of child process */
                            0,                      /* Not passing environmental vars */
                            0,                      /* Current dir same as calling process */
                            &startUpInfo,           /* Attributes of process window */
                            &procinfo );            /* Attributes of child process */
    if (!success)
    {
        LPVOID lpMsgBuf;
        lasterror = GetLastError();

        FormatMessage(
                FORMAT_MESSAGE_ALLOCATE_BUFFER |
                FORMAT_MESSAGE_FROM_SYSTEM |
                FORMAT_MESSAGE_IGNORE_INSERTS,
                NULL,
                lasterror,
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
                (LPTSTR) &lpMsgBuf,
                0,
                NULL );

        logit("et", "Trouble creating console window; Error %d: %s\n",
                lasterror, lpMsgBuf );
        LocalFree( lpMsgBuf );
        return;
    }

    // Set the thread priority of the child process
    threadPriority = THREAD_PRIORITY_NORMAL;
    success = SetThreadPriority(procinfo.hThread, threadPriority);
    if (!success)
    {
        lasterror = GetLastError();
        logit("et", "Error %d setting child thread priority.  Exiting.\n",
                    lasterror);
        return;
    }

    // Close these handles as soon as we get them.  We don't really care about keeping them
    // around; if we are told to shut down, the user will just have to close any open console
    // windows...
    CloseHandle(procinfo.hProcess);
    CloseHandle(procinfo.hThread);
}
