
 /********************************************************************
  *                     Startstop_Unix_Generic                       *
  *                                                                  *
  *  Unlike most of Earthworm where "unix" means "Linux", in this    *
  *  case "unix" means "unix", which covers both Linux and Solaris.  *
  *  This basically was startstop main() for Linux and Solaris.      *
  *  Dropped in this library here so that it can be easily accessed  *
  *  by all unix platforms. If you need to make changes, please      *
  *  put them in here with #ifdefs rather than creating new code     *
  *  branches.                                                       *

  ********************************************************************/
#include <startstop_unix_generic.h>

#define VERSION "v7.1 2007-02-26"
#define PROGRAM_NAME "startstop"

int RunEarthworm ( int argc, char *argv[] )
{
   /* Allot space for status message
    ********************************/
   int  i, j, rc, placeholder;
   char *runPath;
   char      msg[512];
   long      msgsize = 0L;
   MSG_LOGO  getLogo[4]; /* if you change this '3' here, you need to change the parameter to tport_getmsg as well */
   MSG_LOGO  msglogo;
#ifdef _USE_POSIX_SIGNALS
   struct sigaction act;
#endif

   done=0;

   strcpy ( metaring.Version, VERSION );
/* Set name of configuration file
   ******************************/
   strcpy ( metaring.ConfigFile, DEF_CONFIG );
   if ( argc == 2 ) {
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
            logit_init( argv[1], (short) metaring.MyModId, MAXLINE*3, metaring.LogSwitch );
        }
    }

   fprintf( stderr, "startstop: Using config file %s\n", metaring.ConfigFile );

/* Change working directory to environment variable EW_PARAMS value
   ***************************************************************/
   runPath = getenv( "EW_PARAMS" );

   if ( runPath == NULL )
   {
      printf( "startstop: Environment variable EW_PARAMS not defined." );
      printf( " Exiting.\n" );
      exit (-1);
   }

   if ( *runPath == '\0' )
   {
      printf( "startstop: Environment variable EW_PARAMS " );
      printf( "defined, but has no value. Exiting.\n" );
      exit (-1);
   }

   if ( chdir( runPath ) == -1 )
   {
      printf( "startstop: Params directory not found: %s\n", runPath );
      printf( "startstop: Reset environment variable EW_PARAMS." );
      printf( " Exiting.\n" );
      exit (-1);
   }

   LockStartstop(metaring.ConfigFile, argv[0]);

/* Set our "nice" value back to the default value.
 * This does nothing if you started at the default.
 * Requested by D. Chavez, University of Utah.
 *************************************************/
 nice( -nice( 0 ) );

/* Catch process termination signals
   *********************************/
#ifdef _USE_POSIX_SIGNALS
   act.sa_flags = SA_SIGINFO; sigemptyset(&act.sa_mask);
   act.sa_sigaction = SigtermHandler;
   sigaction(SIGTERM, &act, (struct sigaction *)NULL);
   act.sa_flags = 0;
   act.sa_handler = SIG_IGN;
   sigaction(SIGINT, &act, (struct sigaction *)NULL);
   sigaction(SIGQUIT, &act, (struct sigaction *)NULL);
   sigaction(SIGTTIN, &act, (struct sigaction *)NULL);
#else
   sigignore( SIGINT );             /* Control-c */
   sigignore( SIGQUIT );            /* Control-\ */
#ifdef NEEDTOFIXTHIS_DEBUG
   sigset( SIGTERM, SigtermHandler );
#endif

/* Catch tty input signal, in case we are in background */
   sigignore( SIGTTIN );
#endif

/* Allocate array of children
   **************************/
   child = (CHILD *) calloc( MAX_CHILD, sizeof(CHILD) );
   for (i = 0; i < MAX_CHILD; i++) {
     for (j = 0; j < MAX_ARG; j++)
       child[i].argv[j] = NULL;
     child[i].use_uname[0] = '\0';
     child[i].use_gname[0] = '\0';
   }


/* Read configuration parameters
   *****************************/
   nChild    = 0;
   GetConfig();

/* Allocate one region structure per ring
   **************************************/
   metaring.Region = (SHM_INFO *) calloc( (size_t)MAX_RING, sizeof(SHM_INFO) );
   if ( metaring.Region == NULL )
   {
      printf( "startstop: Error allocating region structures. Exiting.\n" );
      return -1;
   }

/* Look up this installation id
   ****************************/
   if ( GetLocalInst ( &(metaring.InstId) ) != 0 )
   {
      printf( "startstop: Error getting local installation id. Exiting.\n" );
      return -1;
   }

/* Look up module id & message types of interest
   *********************************************/
   if ( GetModId( metaring.MyModName, &(metaring.MyModId) ) != 0 )
   {
      printf( "startstop: Invalid module name <%s>. Exiting.\n", metaring.MyModName );
      return -1;
   }

   if ( GetModId( "MOD_WILDCARD", &(metaring.ModWildcard) ) != 0 )
   {
      printf( "startstop: Invalid module name <MOD_WILDCARD>. Exiting.\n" );
      return -1;
   }

   if ( GetType( "TYPE_HEARTBEAT", &(metaring.TypeHeartBeat) ) != 0 )
   {
      printf( "startstop: Unknown message type <TYPE_HEARTBEAT>. Exiting.\n" );
      return -1;
   }

   if ( GetType( "TYPE_ERROR", &(metaring.TypeError) ) != 0 )
   {
      printf( "startstop: Unknown message type <TYPE_ERROR>. Exiting.\n" );
      return -1;
   }

   if ( GetType( "TYPE_RESTART", &(metaring.TypeRestart) ) != 0 )
   {
      printf( "startstop: Unknown message type <TYPE_RESTART>. Exiting.\n" );
      return -1;
   }

   if ( GetType( "TYPE_STOP", &(metaring.TypeStop) ) != 0 )
   {
      printf( "startstop: Unknown message type <TYPE_STOP>. Exiting.\n" );
      return -1;
   }
   if ( GetType( "TYPE_REQSTATUS", &(metaring.TypeReqStatus) ) != 0 )
   {
      printf( "startstop: Unknown message type <TYPE_REQSTATUS>. Exiting.\n" );
      return -1;
   }

   if ( GetType( "TYPE_STATUS", &(metaring.TypeStatus) ) != 0 )
   {
      printf( "startstop: Unknown message type <TYPE_STATUS>. Exiting.\n" );
      return -1;
   }

   if ( GetType( "TYPE_RECONFIG", &(metaring.TypeReconfig) ) != 0 )
   {
      printf( "startstop: Unknown message type <TYPE_RECONFIG>. Exiting.\n" );
      return -1;
   }

   parent.pid = getpid();
   parent.processName = argv[0];
   parent.args = (argc == 2 ? argv[1] : (char *)NULL);

/* Initialize name of log-file & open it
   *************************************/
   if ( argc == 2 )
     logit_init( argv[1], (short) metaring.MyModId, MAXLINE*3, metaring.LogSwitch );
   else
     logit_init( argv[0], (short) metaring.MyModId, MAXLINE*3, metaring.LogSwitch );

   logit( "" , "startstop: Read command file <%s>\n", metaring.ConfigFile );

/* Make startstop be the process group leader */
   if ( setpgid( parent.pid, parent.pid ) != 0 )
     logit( "e", "startstop: failed to set process group ID\n" );

/* Set the priority of startstop
   *****************************/
   SetPriority( P_MYID, parent.className, parent.priority );

/* Start everything
   ****************/
   StartEarthworm( argv[0] );
/* Watch transport rings for kill flags, status requests & restart requests
   ************************************************************************/

   for (i = 0; i < 4; i++) {
       getLogo[i].instid = metaring.InstId;
       getLogo[i].mod    = metaring.ModWildcard;
   }

   getLogo[0].type   = metaring.TypeRestart;
   getLogo[1].type   = metaring.TypeReqStatus;
   getLogo[2].type   = metaring.TypeReconfig;
   getLogo[3].type   = metaring.TypeStop;


   while ( !done )
   {
   /* Send hearbeat, if it's time
    *****************************/
      Heartbeat( &metaring );
      for ( i = 0; i < metaring.nRing; i++ )
      {
      /* Is "kill flag" there?
       ***********************/
        if ( tport_getflag( &(metaring.Region[i]) ) == TERMINATE )
         {
            StopEarthworm();
            return 0;
         }

      /* Look for any interesting new messages
       ***************************************/
         rc = tport_getmsg( &(metaring.Region[i]), getLogo, 4,
                            &msglogo, &msgsize, msg, sizeof(msg)-1 );
         if( rc==GET_NONE || rc==GET_TOOBIG ) continue;
      /* Process a new message
       ***********************/
         msg[msgsize]='\0';

         if     ( msglogo.type == metaring.TypeRestart   ){
            RestartChild( msg );
         } else if( msglogo.type == metaring.TypeStatus )  {
            msg[msgsize]=0;
            fprintf(stderr,"%s\n",msg);
         } else if( msglogo.type == metaring.TypeReqStatus ) {
            SendStatus( i );
         } else if( msglogo.type == metaring.TypeReconfig ) {
            logit( "e" , "startstop: Reconfigure: Re-reading command file <%s>\n", metaring.ConfigFile );
            /* We're re-reading the startstop*.d file here, and adding any new modules or rings */
            /* Even if this new GetConfig returns an error, we don't want to bail out on our running earthworm */
            oldNChild = nChild;
            GetUtil_LoadTable(); /* reread earthworm.d and earthworm_global.d */
            GetConfig(); /* reread startstop*.d*/
            for ( j = metaring.nRing; j < (newNRing); j++ ) {
                logit( "e" , "tport_create: creating ring number <%d>\n", j );
                tport_create( &(metaring.Region[j]),
                    1024 * metaring.ringSize[j],
                    metaring.ringKey[j] );
                metaring.nRing ++;
            }
            SpawnChildren();
            logit( "e" , "startstop: Final reconfigure step: Restart statmgr\n" );
            sprintf (msg, "%d", child[metaring.statmgr_location].pid);
            RestartChild( msg  );
         } else if( msglogo.type == metaring.TypeStop   ) {
            StopChild( msg, &placeholder );
         } else logit("e","Got unexpected msg type %d from transport region: %s\n",
                        (int) msglogo.type, metaring.ringName[i] );
      }
      sleep_ew( 1000 );
    }

   StopEarthworm();
   return 0;
} /* end RunEarthworm */

 /********************************************************************
  *                            GetConfig()                           *
  *                                                                  *
  *  Processes command file using kom.c functions.                   *
  *  Exits if any errors are encountered.                            *
  *  Commands are expected in a given order in startstop.cnf         *
  ********************************************************************/

void GetConfig()
{
   int      state;        /* the part of the config file you are reading  */
   char     missed[30];
   char    *com;
   char    *str;
   char    saveStr[MAXLINE];
   char    *ptr;
   int      nfiles;
   int      i, ir, j;
   int      ichild;
   boolean  firstTimeThrough = FALSE;
   boolean  duplicate = FALSE;

/* Definitions for state while reading file
 ******************************************/
   #define READ_NRING       0
   #define READ_RINGINFO    1
   #define READ_MODID       2
   #define READ_HEARTBEAT   3
   #define READ_MYCLASS     4
   #define READ_MYPRIORITY  5
   #define READ_LOGSWITCH   6
   #define READ_KILLDELAY   7
   #define READ_SLEEPTIME   8
   #define READ_PROCESS     9
   #define READ_PRIORITY   10
   #define READ_AGENT      11

/* Initialize some things
 ************************/
   state     = READ_NRING;
   missed[0] = '\0';

   if (nChild == 0){
       firstTimeThrough = TRUE;
       oldNChild = 0;
   }
/* Open the main configuration file
 **********************************/
   nfiles = k_open( metaring.ConfigFile );
   if ( nfiles == 0 )
   {
        printf( "startstop: Error opening command file <%s>. Exiting.\n",
                 metaring.ConfigFile );
        exit( -1 );
   }

/* Process all command files
   *************************/
   while (nfiles > 0)              /* While there are command files open */
   {
        while (k_rd())             /* Read next line from active file  */
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
               **********************************/
            case READ_NRING:
                if ( !k_its( "nRing" ) )
                {
                    strcpy( missed, "nRing" );
                    break;
                }
                if (firstTimeThrough) {/* Only if we're reading the config for the first time do we keep track */
                    metaring.nRing  = k_int();
                }
                state  = READ_RINGINFO;
                if (firstTimeThrough) {
                    ir = 0;
                } else {
                    ir = metaring.nRing;
                }
                break;

            /* Read the transport ring names & keys & size
               *******************************************/
            case READ_RINGINFO:
                if ( !k_its( "Ring" ) )
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
                if ( ir == MAX_RING )
                {
                    logit( "e" , "startstop: Too many Ring commands, max=%d;"
                            " exiting!\n", MAX_RING );
                    exit( -1 );
                }
                str = k_str();
                if( !str ) break;
                if ( strlen( str ) > 19 )
                {
                  printf( "startstop: Ring name <%s> too long; exiting!\n",
                      str );
                  exit( -1 );
                }
                for ( i = 0; i < ir; i++ ) {
                    if ( strcmp (str, metaring.ringName[i]) == 0 ){
                        duplicate = TRUE;
                    }
                }
                if (duplicate) {
                    if (firstTimeThrough) {
                        logit( "e", "Duplicate ring name <%s>; exiting!\n", str);
                    }
                    duplicate = FALSE;
                } else {
                    strcpy( metaring.ringName[ir], str );
                    metaring.ringSize[ir] = k_int();
                    if ( (metaring.ringKey[ir]= GetKey(metaring.ringName[ir])) == -1 )
                    {
                       logit( "e" , "startstop: Invalid Ring name <%s>; exiting!\n",
                               metaring.ringName[ir] );
                       exit( -1 );
                    }
                    if ( ++ir == metaring.nRing )  state = READ_MODID;
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
                if ( !str ) break;
                if ( strlen( str ) > 39 )
                {
                  logit( "e" , "startstop: MyModuleId name <%s> too long;"
                                  " exiting!\n", str );
                  exit ( -1 );
                }
                strcpy( metaring.MyModName, str );
                state = READ_HEARTBEAT;
                break;

            case READ_HEARTBEAT:
                if ( !k_its("HeartbeatInt") )
                {
                    strcpy( missed, "HeartbeatInt" );
                    break;
                }
                metaring.HeartbeatInt = k_int();
                state = READ_MYCLASS;
                break;

            case READ_MYCLASS:
                if ( !k_its("MyClassName") )
                {
                    strcpy( missed, "MyClassName" );
                    break;
                }
                str = k_str();
                if ( !str ) break;
        if ( strlen( str ) > 3 )
        {
          logit( "e" , "startstop: MyClassName <%s> too long; exiting!\n",
              str );
          exit( -1 );
        }
                strcpy( parent.className, str );
                state = READ_MYPRIORITY;
                break;

            case READ_MYPRIORITY:
                if ( !k_its("MyPriority") )
                {
                    strcpy( missed, "MyPriority" );
                    break;
                }
                parent.priority = k_int();
                state  = READ_LOGSWITCH;
                break;

            case READ_LOGSWITCH:
                if ( !k_its("LogFile") )
                {
                    strcpy( missed, "LogFile" );
                    break;
                }
                metaring.LogSwitch = k_int();
                state     = READ_KILLDELAY;
                break;

        case READ_KILLDELAY:
          if ( !k_its("KillDelay") )
          {
        strcpy( missed, "KillDelay" );
        break;
          }
          metaring.KillDelay = k_int();
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
          metaring.statmgr_sleeptime = (k_int() * 1000);
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
                Get_Process:  /* back door from optional "Agent" (below) */
                if ( nChild == MAX_CHILD )
                {
                    logit( "e" , "startstop: Too many child processes in file %s, max=%d;"
                            " exiting!\n", metaring.ConfigFile, MAX_CHILD );
                    exit( -1 );
                }
                str = k_str();
                if ( !str ) break;
                if ( strlen( str ) > MAXLINE - 1 )
                {
                    logit( "e" , "startstop: Process command line <%s> too long in file %s,"
                            " max=%d; exiting!\n", str, metaring.ConfigFile, MAXLINE - 1 );
                    exit( -1 );
                }
               duplicate = FALSE;
               for ( ichild = 0; ichild < nChild; ichild++ ) {
                   if ( strcmp (str, child[ichild].commandLine) == 0 ) {
                       /* logit ( "", "Skipping twin, no duplicate children allowed: %s\n", str); */
                       duplicate = TRUE;
                   }
               }
               if (duplicate) {
                   state = READ_PRIORITY;
                   break;
               }
                strcpy( child[nChild].commandLine, str );
                strcpy( child[nChild].parm, str );


                /* Cut the command line into tokens
                ********************************/
                j = 0;

                ptr = strtok( child[nChild].parm, " " );
                child[nChild].argv[j++] = ptr;

                do
                {
                    ptr = child[nChild].argv[j++] = strtok( NULL , " " );
                }
                while ( ptr != NULL );

                child[nChild].processName = child[nChild].argv[0];

                if (strcmp( child[nChild].processName, "statmgr") == 0)
                {
                    /* Store statmgr's location in the child array so that we can start it first */
                    metaring.statmgr_location = nChild;
                }

                state = READ_PRIORITY;
                break;

            /* Read the child's priority
               *************************/
            case READ_PRIORITY:

                if ( !k_its("Class/Priority") )
                {
                    strcpy( missed, "Class/Priority" );
                    break;
                }
                str = k_str();
                if ( !str ) break;
                if (duplicate) {
                   state = READ_AGENT;
                   break;
                }
                if ( strlen ( str ) > 3 )
                {
                    logit( "e" , "startstop: Class name <%s> too long; exiting!\n",
                            str );
                    exit( -1 );
                }
                strcpy( child[nChild].className, str );
                child[nChild].priority = k_int();
                (nChild)++;
                state = READ_AGENT;
                break;

        /* Read the child's agent
           **********************/
        case READ_AGENT:
                            if ( !k_its("Agent") )
            {   /* "Agent" is optional; if it's missing then the
                 * the command should be another "Process" */
                if ( k_its("Process") ) goto Get_Process;
                break;
            }
            str = k_str();
            if ( !str ) break;
            if (duplicate) {
               state = READ_PROCESS;
               break;
            }
            if ( strlen( str ) > LOGNAME_MAX )
            {
                logit( "e" , "startstop: Agent user <%s> too long; exiting!\n",
                        str );
                exit( -1 );
            }
            strcpy( child[nChild - 1].use_uname, str );
            str = k_str();
           if (( !str ) || duplicate) break;
            if ( strlen( str ) > LOGNAME_MAX )
            {
                logit( "e" , "startstop: Agent group <%s> too long; exiting!\n",
                        str );
                exit( -1 );
            }
            strcpy( child[nChild - 1].use_gname, str );
            state = READ_PROCESS;
            break;
        } /*end switch*/

        /* Complain if we got an unexpected command
           ****************************************/
            if( missed[0] )
            {
                logit( "e" , "startstop:  Expected: <%s>  Found: <%s>\n",
                         missed, com );
                logit( "e" , "startstop:  Incorrect command order in <%s>;",
                         metaring.ConfigFile );
                logit( "e" , " exiting!\n" );
                exit( -1 );
            }

        /* See if there were any errors processing this command
           ****************************************************/
            if( k_err() )
            {
               logit( "e" , "startstop: Bad <%s> command in <%s>; exiting!\n",
                        com, metaring.ConfigFile );
               logit( "e" , "Offending line: %s\n", k_com() );
               exit( -1 );
            }
        }
        nfiles = k_close();
   }
   return;
}

 /******************************************************************
  *                             Threads()                          *
  ******************************************************************/

void Threads( void *fun( void * ), thread_t *tid ) /* *tid is pointer to thread id */
{
   const size_t stackSize = 0;          /* Use default values */
#ifdef _USE_PTHREADS
   /* Just use the ew library call (since POSIX doesn't do daemons) */
   if ( StartThread(fun,stackSize,tid) == -1 ) {
    fprintf(stderr, "startstop: can't create thread\n");
    exit( -1 );
   }
#else
   int rc;  /* used in non-P_THREADS situations */
   rc = thr_create( (void *)0, stackSize, fun, (void *)0,
                    THR_DETACHED|THR_NEW_LWP|THR_DAEMON, tid );
   if ( rc != 0 )
   {
    perror( "startstop: thr_create" );
    exit( -1 );
   }
#endif
}

  /******************************************************************
   *                         SigtermHandler()                       *
   *                                                                *
   *             Stop the whole system based on SIGTERM             *
   ******************************************************************/

#ifdef _USE_POSIX_SIGNALS
void SigtermHandler( int sig, siginfo_t *sip, void *up )
#else
void SigtermHandler( int sig )
#endif
{
    StopEarthworm();
    exit(0);
}


  /******************************************************************
   *                         StopEarthworm()                        *
   *                                                                *
   *                      Stop the whole system                     *
   ******************************************************************/

void StopEarthworm()
{
   int    i;
   int    status;
   char   tstr[TIMESTR_LEN];
#ifdef _USE_POSIX_SIGNALS
   struct    sigaction act;
#endif

/* Say goodbye
   ***********/
   GetCurrentUTC( tstr );
   logit( "t", " Earthworm stopping at local time: %s\n", tstr );

/* Set kill flag, and wait for the children to terminate.
   If the children don't die in KillDelay seconds, send them a SIGTERM signal.
   ******************************************************************/
   for ( i = 0; i < metaring.nRing; i++ )
      tport_putflag( &(metaring.Region[i]), TERMINATE );

   printf( "Earthworm will stop in %d seconds...\n", metaring.KillDelay );
   for ( i = 0; i < metaring.KillDelay; i++ )
   {
      if ( waitpid( (pid_t)0, &status, WNOHANG ) == -1 && errno == ECHILD )
         break;
      sleep_ew( 1000 );
   }

/* Catch process termination signals.
   Send a kill signal to the current process group.
   Wait for all children to die.
   ************************************************/
#ifdef _USE_POSIX_SIGNALS
   act.sa_flags = 0; sigemptyset(&act.sa_mask);
   act.sa_handler = SIG_IGN;
   sigaction(SIGTERM, &act, (struct sigaction *)NULL);
#else
   sigignore( SIGTERM );
#endif
   kill( (pid_t)0, SIGTERM );
   while ( waitpid( (pid_t)0, &status, 0 ) > 0 );

/* Destroy shared memory regions
   *****************************/
   for( i = 0; i < metaring.nRing; i++ )  tport_destroy( &(metaring.Region[i]) );

/* Free allocated space
   ********************/
   free( metaring.Region );
   free( child  );


}



 /******************************************************************
 *                          EncodeStatus()                        *
 *                                                                *
 *                    Encode the status message                   *
 ******************************************************************/

void EncodeStatus( char *statusMsg )
{
   FILE *fp;
   char phrase[MAXLINE];
   char line[MAXLINE];
   char string[20];
   char tty[20], tim[20];
   char tstr[TIMESTR_LEN];
   int i, j;
   int dummy;
   struct statvfs buf;
   struct utsname uts;
   pid_t status, pid;

#ifdef _USE_SCHED
   struct sched_param sc_prm;
   int sc_pol;
#else
   union
   {
      tsparms_t tsp;
      rtparms_t rtp;
      long      clp[PC_CLPARMSZ];
   } u;

   pcinfo_t  cid;
   pcparms_t pcp;
#endif

#if defined(_LINUX) || defined(_MACOSX) /* Tested on RedHat and Fedora Core */
   gid_t groupid; /* need <unistd.h> and <sys/types.h> */

/* Get cpu used for the parent and each child
   ******************************************/
   groupid = getgid(); /* gets group ID for current process */
   sprintf( line, "/bin/ps -G %d \0", groupid );
#else /* below line works for Solaris */
   sprintf( line, "/usr/bin/ps -g %d\0", parent.pid );
#endif
   fp = popen( line, "r" );
   if ( fp == NULL )
   {
      printf( "startstop/EncodeStatus: popen failed.\n" );
      return;
   }

   strcpy( parent.tcum, "0:00" );             /* Initialize to zero cpu used */
   for ( i = 0; i < nChild; i++ )
      strcpy( child[i].tcum, "0:00" );

   fgets( line, 100, fp );                    /* Skip heading */
   while ( fgets( line, 100, fp ) != NULL )
   {
      sscanf( line, "%d%s%s", &pid, tty, tim );
      if ( parent.pid == pid )
         strcpy( parent.tcum, tim );
      for ( i = 0; i < nChild; i++ )
         if ( child[i].pid == pid )
            strcpy( child[i].tcum, tim );
   }
   pclose( fp );

/* Get system information
   **********************/
   if ( uname( &uts ) == -1 )
      printf( "startstop/EncodeStatus: Error getting system information\n" );

   sprintf( statusMsg, "                    EARTHWORM SYSTEM STATUS\n\n" );

   sprintf( line, "        Hostname-OS:            %s - %s %s\n",
                          uts.nodename, uts.sysname, uts.release );
   strcat( statusMsg, line );
   sprintf( line, "        Start time (UTC):       %s", metaring.tstart );
   strcat( statusMsg, line );

   GetCurrentUTC( tstr );
   sprintf( line, "        Current time (UTC):     %s", tstr );
   strcat( statusMsg, line );

   if ( statvfs( ".", &buf ) == -1 )
      printf( "startstop: Error getting file system info\n" );
   else {
      sprintf( line, "        Disk space avail:       %d kb\n",
        buf.f_frsize ? (buf.f_bavail * buf.f_frsize) / 1024 :
        buf.f_bsize ? (buf.f_bavail * buf.f_bsize) / 1024 : buf.f_bavail );
      strcat( statusMsg, line );
   }

   for ( i = 0; i < metaring.nRing; i++ )
   {
      sprintf( line, "        Ring %2d name/key/size:  %s / %d / %d kb\n",
               i+1, metaring.ringName[i], metaring.ringKey[i], metaring.ringSize[i] );
      strcat( statusMsg, line );
   }

   sprintf( line,    "        Stopstart Version:      %s\n", metaring.Version );
   strcat( statusMsg, line );

/* Get and print status of the parent process
   ******************************************/
   sprintf( line, "\n         Process  Process           Class/    CPU\n" );
   strcat( statusMsg, line );
   sprintf( line,   "          Name      Id     Status  Priority   Used  Argument\n" );
   strcat( statusMsg, line );
   sprintf( line,   "         -------  -------  ------  --------   ----  --------\n" );
   strcat( statusMsg, line );

   sprintf( line, "%16s", parent.processName );
   sprintf( phrase, " %7d", parent.pid );
   strcat( line, phrase );
   sprintf( phrase, "   Alive " );
   strcat( line, phrase );

#ifdef _USE_SCHED
   /* get POSIX scheduling policy */
#ifdef _MACOSX
   strcat( line, "    **" );
   strcat( line, "/**" );
#else /* _MACOSX */
   if ( (sc_pol = sched_getscheduler(parent.pid)) == -1 ) {
      printf( "startstop/EncodeStatus: parent sched_getscheduler error: %s\n",strerror(errno));
      strcat( line, "    **" );
   } else {
      strcat( line, phrase );
   }
   /* get POSIX scheduling priority */
   if ( sched_getparam(parent.pid, &sc_prm) ) {
      printf( "startstop/EncodeStatus: parent sched_getparam error: %s\n",strerror(errno));
      strcat( line, "/**" );
   } else {
      sprintf( phrase, "/%2d", sc_prm.sched_priority );
      strcat( line, phrase );
   }
#endif /* _MACOSX */
#else  /* _USE_SCHED */
/* Get and print class of parent
   *****************************/
   pcp.pc_cid = PC_CLNULL;
   if ( priocntl( P_PID, parent.pid, PC_GETPARMS,
                  (caddr_t)&pcp ) == -1 )
      printf( "startstop/EncodeStatus: Error getting parent parameters\n" );

   cid.pc_cid = pcp.pc_cid;
   if ( priocntl( (idtype_t)0, (id_t)0, PC_GETCLINFO,
                  (caddr_t)&cid ) == -1 )
      printf( "startstop/EncodeStatus: priocntl getclinfo error\n" );
   sprintf( phrase, "   %s", cid.pc_clname );
   strcat( line, phrase );

/* Get and print priority of parent
   ********************************/
   for ( j = 0; j < PC_CLPARMSZ; j++ )
      u.clp[j] = pcp.pc_clparms[j];

   if ( strcmp( cid.pc_clname, "RT" ) == 0 )
   {
      sprintf( phrase, "/%2d", u.rtp.rt_pri );
      strcat( line, phrase );
   }

   if ( strcmp( cid.pc_clname, "TS" ) == 0 )
   {
      sprintf( phrase, "/%2d", u.tsp.ts_upri );
      strcat( line, phrase );
   }
#endif  /* _USE_SCHED */

/* Print cumulative cpu time used by parent
   ****************************************/
   sprintf( phrase, "%9s", parent.tcum );
   strcat( line, phrase );

/* Print the parent argument list without the command itself */
   if ( parent.args != (char *)NULL ) {
    sprintf( phrase, "  %s\n", parent.args );
    strcat( line, phrase );
    if ( strlen(line) > 80 ) {
        line[79] = '\n';
        line[80] = '\0';
    }
   } else
    strcat(line,"  -\n");
   strcat( statusMsg, line );

/* Get and print status of each child
   **********************************/
   /* build a line in `line[]', to check its length */
   for ( i = 0; i < nChild; i++ )
   {
      sprintf( line, "%16s", child[i].processName );
      sprintf( phrase, " %7d", child[i].pid );
      strcat( line, phrase );
#ifdef _LINUX /* Linux-only __WALL wait-all */
      status = waitpid( child[i].pid, &dummy, WNOHANG | __WALL );
#else /*_LINUX */
      status = waitpid( child[i].pid, &dummy, WNOHANG );
#endif /*_LINUX */
      strcpy( string,    "   Zombie " );
      if ( status == 0 )
         strcpy( string, "   Alive " );
      if ( (status == -1) && (errno == ECHILD) ) {
          if( strcmp( child[i].status, "Stopped" ) == 0 ) {
              strcpy( string, "   Stop   " );
          } else {
              strcpy( string, "   Dead   " );
          }
      }
      sprintf( phrase, "%s", string );
      strcat( line, phrase );

/* Get and print class of each child
   *********************************/
      if (status == 0) {
#ifdef _USE_SCHED
        /* get POSIX scheduling policy */
#ifdef _MACOSX
        strcat( line, "    **" );
        strcat( line, "/**" );
#else /* _MACOSX */
        if ( (sc_pol = sched_getscheduler(child[i].pid)) == -1 ) {
           printf( "startstop/EncodeStatus: child (%d) sched_getscheduler error: %s\n",i,strerror(errno));
           strcat( line, "    **" );
        } else {
           sprintf( phrase, " %s",
#if !defined(_LINUX)
        sc_pol == SCHED_NP ? "  NP" :
        sc_pol == SCHED_TS ? "  TS" :
#endif /* ndef _LINUX */
        sc_pol == SCHED_FIFO ? "FIFO" :
        sc_pol == SCHED_RR ? "  RR" : "  ??");
           strcat( line, phrase );
        }
        /* get POSIX scheduling priority */
        if ( sched_getparam(child[i].pid, &sc_prm) ) {
           printf( "startstop/EncodeStatus: child (%d) sched_getparam error: %s\n",i,strerror(errno));
           strcat( line, "/**" );
        } else {
           sprintf( phrase, "/%2d", sc_prm.sched_priority );
           strcat( line, phrase );
        }
#endif /* _MACOSX */
#else /* _USE_SCHED */
        pcp.pc_cid = PC_CLNULL;
        if ( priocntl( P_PID, child[i].pid, PC_GETPARMS,
                       (caddr_t)&pcp ) == -1 )
          printf( "startstop/EncodeStatus: Error getting child parameters\n" );

        cid.pc_cid = pcp.pc_cid;
        if ( priocntl( (idtype_t)0, (id_t)0, PC_GETCLINFO,
                       (caddr_t)&cid ) == -1 )
          printf( "startstop/EncodeStatus: priocntl getclinfo error\n" );
        sprintf( phrase, "   %s", cid.pc_clname );
        strcat( line, phrase );

        /* Get and print priority of each child
        ************************************/
        for ( j = 0; j < PC_CLPARMSZ; j++ )
          u.clp[j] = pcp.pc_clparms[j];

        if ( strcmp( cid.pc_clname, "RT" ) == 0 ) {
          sprintf( phrase, "/%2d", u.rtp.rt_pri );
        } else if ( strcmp( cid.pc_clname, "TS" ) == 0 ) {
          sprintf( phrase, "/%2d", u.tsp.ts_upri );
        }
        strcat( line, phrase );
#endif /* _USE_SCHED */
        /* Print cumulative cpu time used by each child
        ********************************************/
        sprintf( phrase, "%9s ", child[i].tcum );
        strcat( line, phrase );
      } else {  /* status != 0 */
        sprintf( phrase, "                 " );
        strcat( line, phrase );
      }

      /* Print the argument list without the command itself
       ****************************************************/
      for (j = 1; j < MAX_ARG; j++) {
        if (child[i].argv[j] == NULL ) break;
        /* do we have enough room on 80-column screen (including the space) */
        if ( (strlen(line) + strlen(child[i].argv[j]) ) > 79 ) break;
        sprintf( phrase, " %s", child[i].argv[j] );
        strcat( line, phrase );
      }
      strcat( statusMsg, line );

/* Attach a newline to the end of the line
   ***************************************/
      sprintf( line, "\n" );
      strcat( statusMsg, line );
   }
   return;
} /* end EncodeStatus */


  /******************************************************************
   *                          ConstructIds                         *
   *     Look up user id and group id numbers for Agents            *
   ******************************************************************/
void ConstructIds( char *user, char *group, uid_t *uid, gid_t *gid )
{
    struct group grp;
    struct passwd passwd;
    char    grbuffer[BUFSIZ];
    char    pwbuffer[BUFSIZ];
    uid_t   my_uid;
    struct passwd my_passwd;
    char   mypwbuffer[BUFSIZ];

#if _USE_PTHREADS
    struct passwd pwe, *pwptr;
    struct group gre, *grptr;
#endif

    my_uid = getuid();
#if _USE_PTHREADS
    pwptr = & my_passwd;
    getpwuid_r( my_uid, &pwe, mypwbuffer, BUFSIZ, &pwptr );
#else
    getpwuid_r( my_uid, &my_passwd, mypwbuffer, BUFSIZ );
#endif

    if( !strcmp( user, "" ) )
    {   /* Use the real userID if user hasn't been specified */
        *uid = getuid();
    }

    /* Don't allow "root" as the Agent user; use the real uid instead */
    else if ( !strcmp( user, "root") && my_uid != 0 )
    {
      fprintf( stderr, "`root' user Agent not permitted; using `%s'\n",
           my_passwd.pw_name );
      *uid = my_uid;
    }
#if _USE_PTHREADS
    else if( ! (pwptr = &passwd, getpwnam_r( user, &pwe, &pwbuffer[0], BUFSIZ, &pwptr )) )
#else
    else if( ! getpwnam_r( user, &passwd, &pwbuffer[0], BUFSIZ ) )
#endif
    {
        fprintf( stderr, "Failed to find password entry for user %s\n",
             user );
        *uid = my_uid;
    }
    else
    {
        *uid = passwd.pw_uid;
    }

    if( !strcmp( group, "" ) )
    {   /* Use the real groupIP if group hasn't been specified */
        *gid = getgid();
    }
#if _USE_PTHREADS
    else if( ! (grptr = &grp, getgrnam_r( group, &gre, &grbuffer[0], BUFSIZ, &grptr )) )
#else
    else if( ! getgrnam_r( group, &grp, &grbuffer[0], BUFSIZ ) )
#endif
    {
        fprintf( stderr, "Failed to find entry for group %s\n", group );
        *gid = getgid();
    }
    else
    {
        *gid = grp.gr_gid;
    }

    return;
} /* end ConstructIds */



  /******************************************************************
   *                           RestartChild                         *
   *                                                                *
   *      Restart a specific child, given a TYPE_RESTART msg        *
   ******************************************************************/

void RestartChild( char *restartmsg )
{
    boolean NotInitialStartup = TRUE;

    int ich = 0, ir, ret;

    /* stop */
    ret = StopChild( restartmsg, &ich );
    if (ret == EXIT) {
       return;
    }

    /* and now start */
    switch ( StartChild( ich, NotInitialStartup ) )
    {
        case -1:
            logit("et","startstop: failed to restart <%s>\n",
                  child[ich].processName);
            break;
        case 0:
            logit("et","startstop: successfully restarted <%s>\n",
                  child[ich].processName);
            break;
        default:
            return;
    }


} /* end RestartChild */



  /******************************************************************
   *                           StopChild                            *
   *                                                                *
   *     Stop a specific child given a TYPE_RESTART or              *
   *     TYPE_STOP message                                          *
   ******************************************************************/
int StopChild ( char *restartmsg, int *ich )
{
   int ir, childNum;
   int status  = 0;
   int procId  = 0;
   int nsec    = 0;
   int timeout = 30;
   char ErrText[MAXLINE];

   /* Find this process id in the list of children
    **********************************************/
      procId = atoi( restartmsg );

      for ( childNum = 0; childNum < nChild; childNum++ )
      {
         if ( child[childNum].pid == procId ) break;
      }

      if( childNum==nChild )
      {
         sprintf( ErrText, "Cannot restart pid=%d; it is not my child!\n",
                  procId );
         ReportError( ERR_STARTCHILD, ErrText, &metaring );
         return EXIT;
      }
   #ifdef _UNIX
   /* Kill the current incarnation of the child
    *******************************************/
      kill( (pid_t)child[childNum].pid, SIGKILL );
      while( waitpid( (pid_t)child[childNum].pid, &status, 0 ) > 0 )
      {
         sleep_ew(1000);
         nsec++;
         if ( nsec > timeout )
         {
           sprintf( ErrText, "terminating <%s> (pid=%d) in %d sec failed;"
                   " cannot restart!\n",
                    child[childNum].processName, procId, timeout );
           ReportError( ERR_STARTCHILD, ErrText, &metaring );
           return EXIT;
   #else

   /* Give child a chance to shut down gracefully...
    ************************************************/
      for( ir=0; ir<metaring.nRing; ir++ ) tport_putflag( &(metaring.Region[ir]), child[childNum].pid );
      nsec = 0;
      while( waitpid((pid_t)child[childNum].pid, &status, WNOHANG) == 0 )
      {
         sleep_ew(1000);
         nsec++;

      /* ...but if it takes too long:
         Kill the current incarnation of the child. Use SIGTERM - by
         default it causes an exit, but it can be blocked, allowing us
         to protect writes to shared memory (SIGKILL cannot be blocked)
       ****************************************************************/
         if( nsec > metaring.KillDelay ) {
            sigsend( P_PID, (pid_t)child[childNum].pid, SIGTERM );
            logit( "et", "startstop: <%s> (pid=%d) did not shut down in %d sec;"
                   " terminating it!\n",
                    child[childNum].processName, procId, metaring.KillDelay );
            nsec = 0;
            while( waitpid((pid_t)child[childNum].pid, &status, WNOHANG) == 0 )
            {
               sleep_ew(1000);
               nsec++;
               if ( nsec > metaring.KillDelay )
               {
                 sprintf( ErrText, "terminating <%s> (pid=%d) in %d sec failed;"
                         " cannot restart!\n",
                          child[childNum].processName, procId, metaring.KillDelay );
                 ReportError( ERR_STARTCHILD, ErrText, &metaring );
                 return EXIT;
               }
            }
            break;
   #endif
         }
    }
    strcpy( child[childNum].status, "Stopped" );
    *ich = childNum;
    return SUCCESS;
}


int StartChild ( int ich, boolean NotInitialStartup ) {
    char ErrText[512];
    /*
     *     From fork(2) man page:
     *     In applications that use the Solaris threads API rather than
     *     the POSIX threads API (applications linked with -lthread but
     *     not -lpthread),fork() duplicates in the  child  process  all
     *     threads  (see  thr_create(3THR)) and LWPs in the parent pro-
     *     cess. The  fork1()  function  duplicates  only  the  calling
     *     thread (LWP) in the child process.
     *
     *     Thus, we use fork1 for restarts to take fewer resources
     */
    if ( NotInitialStartup ) { /* not initial startup, so use fork1 */
        child[ich].pid = fork1();
    } else {
        child[ich].pid = fork();
    }
    switch ( child[ich].pid )
    {
      case -1:
         perror( "startstop: fork" );
         sprintf( ErrText, "fork failed; cannot start <%s>\n",
                  child[ich].processName );
         ReportError( ERR_STARTCHILD, ErrText, &metaring );
         sleep_ew( 500 );
         return(-1);
         break;
      case 0:   /* the child */
         setgid( child[ich].use_gid );
         setuid( child[ich].use_uid );
          /* fprintf(stderr,"PIDS at exec %d %d\n",getpid(),getppid()); */
         fprintf(stderr,"%s %s %s \n",child[ich].processName,child[ich].argv[0],child[ich].argv[1] );
         execvp( child[ich].processName, child[ich].argv );
         /* logit("e", "debug: error with: child[ich].processName = %s\n", child[ich].processName); */
         perror( "startstop: execvp" );
         sprintf( ErrText, "execvp failed; cannot restart <%s>\n",
            child[ich].processName );
         ReportError( ERR_STARTCHILD, ErrText, &metaring );
         StartError( ich, child[ich].processName, &metaring, &nChild );
         /* StartError ends the child process completely */
         logit("e", "debug2: error with: child[ich].processName = %s %d\n", child[ich].processName, child[ich].pid);
         sleep_ew( 500 );
         return(1);
         break;
      default:  /* the parent */
        break;
    }
    SetPriority( child[ich].pid, child[ich].className, child[ich].priority );
    strcpy( child[ich].status, "Alive" );
    /* Sleep for 0.5 second to allow fork to complete it's business.
    This was motivated by message:
    "startstop: fork: Resource temporarily unavailable" on Sparc5 - newt
    ********************************************************************/
    return(0);

} /* end StartChild */

/******************************************************************
 *                         StartEarthworm()                       *
 *                                                                *
 *                      Start the whole system                    *
 ******************************************************************/

void StartEarthworm( char *ProgName )
{
   int      i;
   thread_t tid;
   boolean RestartStatus = FALSE;


/* Print start time
   ****************/
   GetCurrentUTC( metaring.tstart );         /* Get UTC as a 26 char string */
   logit( "t", " Earthworm starting at local time: %s\n", metaring.tstart );

/* Create the transport rings
   **************************/
   for ( i = 0; i < metaring.nRing; i++ )
      tport_create( &(metaring.Region[i]), 1024 * metaring.ringSize[i], metaring.ringKey[i] );

/* Spawn the child processes.  Results are
   unpredictable if other threads start before this loop.
   *****************************************************/
#ifdef _SOLARIS

/* Start Statmgr first if present */

    if (metaring.statmgr_location != (MAX_CHILD + 1))
        /* aka if we did find a statmgr in GetConfig */
    {

        ConstructIds( child[metaring.statmgr_location].use_uname,
                      child[metaring.statmgr_location].use_gname,
                      &child[metaring.statmgr_location].use_uid,
                      &child[metaring.statmgr_location].use_gid );

        switch( child[metaring.statmgr_location].pid = fork() )
        {
            case -1:
                perror( "startstop: fork" );
                exit( 1 );
                break;
            case 0: /* the child */
                setgid( child[metaring.statmgr_location].use_gid );
                setuid( child[metaring.statmgr_location].use_uid );
                execvp( child[metaring.statmgr_location].processName, child[metaring.statmgr_location].argv );
                perror( "startstop: execvp" );
                StartError( metaring.statmgr_location, child[metaring.statmgr_location].processName, &metaring, &nChild );
                break;
            default:    /* the parent */
                break;
        }
        SetPriority( child[metaring.statmgr_location].pid, child[metaring.statmgr_location].className, child[metaring.statmgr_location].priority );

        /* Tell the user why it's taking so long to start up */
        logit("et","startstop: sleeping <%d> second(s) for statmgr startup.\n",
              (metaring.statmgr_sleeptime/1000) );

        /* Sleep after starting statmgr to allow statmgr to come up first */
        sleep_ew(metaring.statmgr_sleeptime);
    }
#endif /* _SOLARIS */

   for ( i = 0; i < nChild; i++ )
   {
#ifdef _SOLARIS
        /* To prevent starting statmgr a second time, we'll just skip over it
         If it's not there, then the index to skip defaults to one past MAXCHILD, so
         no other modules will be skipped. */
         if (i != metaring.statmgr_location)
         {
#endif /* _SOLARIS */
            ConstructIds( child[i].use_uname,
                          child[i].use_gname,
                          &child[i].use_uid,
                          &child[i].use_gid );
            if ( StartChild ( i, RestartStatus ) != 0 ){
                logit("et","startstop: process <%s> <%d> failed to start.\n",
                                  child[i].parm, child[i].pid );
            } else {
                logit("et","startstop: process <%s> <%d> started.\n",
                                  child[i].parm, child[i].pid );
            }

#ifdef _SOLARIS
         }
#endif /* _SOLARIS */
   }


/* Start the interactive thread
   ****************************/
    Threads( Interactive, &tid );
   return;
} /* end StartEarthworm */


 /********************************************************************
  *                            SetPriority()                         *
  *                                                                  *
  *        Set the priorities of all threads in this process.        *
  *         Valid TS/RT priority ranges depend on O/S and POSIX      *
  *         compliance (RT 0 to 59 for solaris w/o POSIX).           *
  ********************************************************************/

void SetPriority( pid_t pid, char *ClassName, int Priority )
{

#ifdef _USE_SCHED
   struct sched_param sc_prm;
   int sc_pol, sc_pri_min, sc_pri_max;

   /* get POSIX scheduling policy for requested class */
   sc_pol =
    strcmp( ClassName, "FIFO" ) == 0 ? SCHED_FIFO : /* FIFO (RR with no time out) */
    strcmp( ClassName, "RR" ) == 0 ? SCHED_RR : /* Round robbin */
    strcmp( ClassName, "RT" ) == 0 ? SCHED_RR : /* SOLARIS compatibility = RR */
#if !defined(_UNIX)
    strcmp( ClassName, "TS" ) == 0 ? SCHED_TS : /* Time share */
    strcmp( ClassName, "NP" ) == 0 ? SCHED_NP : /* who knows */
#endif
    strcmp( ClassName, "OTHER" ) == 0 ? SCHED_OTHER : -1;   /* OTHER -> revert to TS */
   if ( sc_pol == -1 ) {
      printf( "startstop: unknown class: %s\n",ClassName);
      return;
   }
   /* get POSIX min and max priority for requested policy */
   if ( (sc_pri_min = sched_get_priority_min(sc_pol)) == -1 ) {
      printf( "startstop: sched_get_priority_min error: %s\n",strerror(errno));
      return;
   }
   if ( (sc_pri_max = sched_get_priority_max(sc_pol)) == -1 ) {
      printf( "startstop: sched_get_priority_max error: %s\n",strerror(errno));
      return;
   }
   /* keep requested priority within allowed min and max priority */
   sc_prm.sched_priority = MIN(MAX(Priority,sc_pri_min),sc_pri_max);
   if ( sc_prm.sched_priority != Priority )
       printf( "startstop: requested %s priority (%d) not between %d and %d: reset to %d\n",
                ClassName,Priority,sc_pri_min,sc_pri_max,sc_prm.sched_priority);
   /* for safety, keep real-time priorities lower than all system and device processes */
# ifdef __sgi
#  define MAX_USR_PRI 89
   if ( (sc_pol == SCHED_FIFO || sc_pol == SCHED_RR) && sc_prm.sched_priority > MAX_USR_PRI ) {
      sc_prm.sched_priority = MAX_USR_PRI;
      printf( "startstop: requested priority (%d) too high: set to %d\n",
                Priority,sc_prm.sched_priority);
   }
#  undef MAX_USR_PRI
# endif
   /* set POSIX scheduling policy and priority */
#ifndef _MACOSX
   if ( sched_setscheduler(pid, sc_pol, &sc_prm) == -1 ) {
#if !defined(_UNIX)
      printf( "startstop: sched_setscheduler error: PID %d (policy/priority %s/%d):\n\t%s\n",
            pid, (sc_pol == SCHED_FIFO ? "FIFO" : sc_pol == SCHED_RR ? "RR" :
            sc_pol == SCHED_TS ? "TS" : sc_pol == SCHED_NP ? "NP" : "??"),
        sc_prm.sched_priority, strerror(errno));
#else /* _UNIX */
      printf( "startstop: sched_setscheduler error: PID %d (policy/priority %s/%d):\n\t%s\n",
            pid, (sc_pol == SCHED_FIFO ? "FIFO" : sc_pol == SCHED_RR ? "RR" :
             "??"),
        sc_prm.sched_priority, strerror(errno));
#endif /* _UNIX */
      printf( "\tCheck that startstop is setuid root!!\n");
      return;
   }
#endif /* ndef _MACOSX */
#else
   int i;
   union{
      tsparms_t tsp;
      rtparms_t rtp;
      long      clp[PC_CLPARMSZ];
   } u;

   pcinfo_t  cid;
   pcparms_t pcp;
   rtparms_t rtp;
   strcpy( cid.pc_clname, ClassName );
   if ( priocntl( P_PID, (id_t)pid, PC_GETCID, (caddr_t)&cid ) == -1 )
      perror( "startstop: priocntl getcid" );

   pcp.pc_cid = cid.pc_cid;

   if ( strcmp( ClassName, "TS" ) == 0 )
   {
      u.tsp.ts_uprilim = TS_NOCHANGE;            /* Use the default */
      u.tsp.ts_upri    = Priority;               /* Desired priority */
   }

   if ( strcmp( ClassName, "RT" ) == 0 )
   {
      u.rtp.rt_pri     = Priority;               /* Desired priority */
      u.rtp.rt_tqsecs  = 0;
      u.rtp.rt_tqnsecs = RT_TQDEF;               /* Use default time quantum */
   }
   for ( i = 0; i < PC_CLPARMSZ; i++ )
      pcp.pc_clparms[i] = u.clp[i];

   if ( priocntl( P_PID, (id_t)pid, PC_SETPARMS, (caddr_t)&pcp ) == -1 )
      perror( "startstop: priocntl setparms" );
#endif
} /* end SetPriority */

  /*********************************************************************
   *                           Interactive()                           *
   *                                                                   *
   *          Thread function to get commands from keyboard            *
   *********************************************************************/

void *Interactive( void *dummy) {
   char ewstat[MAX_STATUS_LEN];
   char      message[512];/**/
   MSG_LOGO  logo;/**/
   int  i, scan_return, placeholder;
   char line[MAXLINE];
   char param[MAXLINE];

#ifdef _LINUX

   EncodeStatus( ewstat );                          /* One free status */
   printf( "\n%s\n", ewstat );
#else /* if not LINUX */
   int z;
   int quit = 0;

   for ( z = 0; z < metaring.nRing; z++ )
   {
      if ( tport_getflag( &(metaring.Region[z]) ) == TERMINATE )
      {
          quit = 1;
      }
   }

   if ( quit != 1 )
   {
      EncodeStatus( ewstat );/* One free status */
      printf( "\n%s", ewstat );
   }
#endif /* ifdef _LINUX */
   do
   {
      printf( "\n   Press return to print Earthworm status, or\n" );
      printf( "   type restart nnn where nnn is proc id or name to restart, or\n" );
      printf( "   type quit<cr> to stop Earthworm.\n\n" );

/* With SIGTTIN ignored, reading from tty (stdin) when in background
 * will return with errno set to EIO. Or if startstop is run from a script,
 * errno will be set to ESPIPE.
 * If that happens, just return from this thread.
 */

      fgets(line, MAXLINE, stdin );
#ifdef _LINUX
      {
      if ((errno == EIO) || (errno == ESPIPE) ||(errno == ENOTTY))
      {
    fprintf(stderr,"XXX %d %s ZZZ\n",errno,strerror(errno));
    thr_exit( (void *)0 );
      }
      }
#endif

      if ((strlen(line) == 0) && ((errno == EIO) || (errno == ESPIPE))) {
#ifdef _LINUX
    fprintf(stderr,"XX1X %d %s ZZZ\n",errno,strerror(errno));
#endif
        thr_exit( (void *)0 );
      }

      if ( strlen( line ) == 1 )
      {
#ifdef _LINUX

/* Build status request message
   ****************************/
   sprintf(message,"%d\n", metaring.MyModId );

/* Set logo values of message
   **************************/
   logo.type   = metaring.TypeReqStatus;
   logo.mod    = metaring.MyModId;
   logo.instid = metaring.InstId;

/* Send status message to transport ring
   **************************************/
   if ( tport_putmsg( &(metaring.Region[0]), &logo, strlen(message), message ) != PUT_OK )
   {
    fprintf(stderr, "status: Error sending message to transport region.\n" );
    return ( 1 );
   }
#endif /* if not _LINUX IGD 2006/11/27 moved the condition up to allow EncodeStatus on Linux*/
         EncodeStatus( ewstat );
         printf( "\n%s", ewstat );
/* #endif IGD 2006/11/27 commented out*/
      }


      for ( i = 0; i < (int)strlen( line ); i++ )
         line[i] = tolower( line[i] );
      param[0] = 0;
     scan_return = sscanf(line, "%*s %s", param);

     if (strncmp( line, "restart", 7)==0 && scan_return == 1)
     {
       for( i = 0; i < nChild; i++ )
       {
         if( strcmp(child[i].processName, param)==0 )
         {
           sprintf(param,"%d", child[i].pid);
           break;
         }
       }

       fprintf( stderr, "sending restart message to the ring for %s\n", param);      /* sending it to the ring rather than starting directly so statmgr can keep track */
      /* RestartChild(param ); */
       SendRestartReq(&metaring, param);
     }

     if (((strncmp( line, "stop", 4)==0) || (strncmp( line, "stopmodule", 10)==0)) && scan_return == 1)
     {
       for( i = 0; i < nChild; i++ )
       {
         if( strcmp(child[i].processName, param)==0 )
         {
           sprintf(param,"%d", child[i].pid);
           break;
         }
       }
       fprintf( stderr, "sending stop message to the ring for %s\n", param);
       /* sending it to the ring rather than starting directly so statmgr can keep track */
       /* StopChild(param, &placeholder);; */
       SendStopReq(&metaring, param);

     }

     /* reconfigure */
     if ((strncmp( line, "recon", 5)==0) || (strncmp( line, "reconfigure", 11)==0)) {
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
                return 0;
           }
     }



   } while ( strcmp( line, "quit\n" ) != 0 );

   done = 1;

   thr_exit( (void *)0 );
   return 0;
} /* end Interactive */

/******************************************************************
 *                            SendStatus()                        *
 *    Build a status message and put it in a transport ring       *
 ******************************************************************/

void SendStatus( int iring )
{
   MSG_LOGO logo;
   int length;
   char ewstat[MAX_STATUS_LEN];

   logo.instid = metaring.InstId;
   logo.mod    = metaring.MyModId;
   logo.type   = metaring.TypeStatus;

   EncodeStatus( ewstat );
   length = strlen( ewstat );

   if ( tport_putmsg( &(metaring.Region[iring]), &logo, length, ewstat ) != PUT_OK )
      logit("t", "startstop: Error sending status msg to transport region: %s\n",
             metaring.ringName[iring] );
   return;
} /* end SendStatus */

void SpawnChildren (){
    /* Start the child processes
    *************************/
    int ichild;
    boolean NotInitialStartup = TRUE;

    /* The following changes were made by John Patton to fix the
     processes dying before statmgr comes up bug */
   for ( ichild = oldNChild; ichild < nChild; ichild++ )
   {
        /* To prevemt starting statmgr a second time, we'll just skip over it
          If it's not there, then the index to skip defaults to one past MAXCHILD, so
          no other modules will be skipped. */
        if (ichild != metaring.statmgr_location)
        {
            if ( StartChild( ichild, NotInitialStartup ) == 0 )
            {
                logit("et","startstop: process <%s> <%d> started.\n",
                  child[ichild].parm, child[ichild].pid );
            } else {
                logit("et","startstop: process <%s> <%d> failed to start.\n",
                                  child[ichild].parm, child[ichild].pid );
            }
        }
   }
} /* end SpawnChildren */
