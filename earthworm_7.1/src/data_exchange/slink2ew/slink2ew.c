/***************************************************************************
 * 
 * A SeedLink client for Earthworm.  Collects Mini-SEED data records
 * from a SeedLink server and inserts TRACE_BUF messages into an
 * Earthworm ring.
 *
 * Written by Chad Trabant, ORFEUS/EC-Project MEREDIAN
 * The liss2ew sources were used as a template, thanks Pete Lombard.
 *
 * modified 2006.066
 ***************************************************************************/

/* System includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* Earthworm includes */
#include <earthworm.h>
#include <kom.h>
#include <transport.h>
#include <trace_buf.h>

#include <libslink.h>
#include "slink2ew.h"


/***************************************************************************
 *
 * Global parameters
 *
 ***************************************************************************/
SLCD         *slconn;	         /* SeedLink connection parameters       */
short int     verbose   = 0;     /* Verbosity level                      */
char         *statefile = 0;	 /* State file                           */
int           stateint  = 100;   /* Interval (packets recv'd) to save state */
				
SHM_INFO      regionOut;         /* Shared memory region                 */
pid_t         myPid;             /* Process ID                           */

char  ringName[MAXRINGNAMELEN];  /* Name of destination ring for data    */
long  ringKey;                   /* Key to output shared memory region   */
char  myModName[MAXMODNAMELEN];  /* Name of module instance              */
int   forcetracebuf = 0;         /* Switch to force TRACEBUF             */

unsigned char myModId;           /* ID of this module                    */
unsigned char myInstId;          /* Installation running this module     */
unsigned char typeError;         /* Error message type                   */
unsigned char typeHeartbeat;     /* Heartbeat message type               */
unsigned char typeWaveform;      /* Waveform message type TRACEBUF       */
unsigned char typeWaveform2 = 0; /* Waveform message type TRACEBUF2      */

MSG_LOGO      hrtLogo;           /* Heartbeat message logo               */
MSG_LOGO      waveLogo;          /* Waveform message logo                */
MSG_LOGO      errLogo;           /* Error message logo                   */

int           heartbeatInt;      /* Heartbeat interval (seconds)         */
int           logSwitch;         /* 1 -> write log, 0 -> no log          */
                          /* 2 -> write module log but not stderr/stdout */


int
main (int argc, char **argv)
{
  SLpacket * slpack;
  int seqnum;
  int ptype;
  int retval;
  int packetcnt = 0;
  time_t timeNow;                /* current time                  */
  time_t timeLastBeat = 0;       /* time last heartbeat was sent  */

  int prethrottle = 0;
  int throttlecnt = 1;
  int throttlewait = 0;
  
  /* Allocate and initialize a new connection description */
  slconn = sl_newslcd();
  
  /* Check number of command line arguments */
  if (argc != 2)
    {
      usage(argv[0]);
      exit (EW_FAILURE);
    }

  /* Process configuration parameters */
  configure (argv);

  /* Attach to Output transport ring */
  tport_attach (&regionOut, ringKey);
  
  logit ("t", "slink2ew version %s\n", VERSION);
  
  /* Loop with the connection manager */
  while ( (retval = sl_collect_nb (slconn, &slpack)) )
    {
      /* Check if we are being asked to terminate */
      if ( ! slconn->terminate &&
	   ( tport_getflag (&regionOut) == TERMINATE ||
	     tport_getflag (&regionOut) == myPid ))
	{
	  logit ("t", "slink2ew terminating on request\n");
	  sl_terminate(slconn);
	}
      
      /* Check if we need to send heartbeat message */
      if ( time( &timeNow ) - timeLastBeat >= heartbeatInt )
	{
	  timeLastBeat = timeNow;
	  report_status ( &hrtLogo, 0, "" ); 
	}
      
      /* Check if a packet arrived */
      if ( retval == SLPACKET )
	{
	  ptype  = sl_packettype (slpack);
	  seqnum = sl_sequence (slpack);
	  
	  packet_handler ((char *) &slpack->msrecord,
			  ptype, seqnum, SLRECSIZE);
	  
	  /* Save intermediate state file */
	  if ( statefile && stateint )
	    {
	      if ( ++packetcnt >= stateint )
		{
		  sl_savestate (slconn, statefile);
		  packetcnt = 0;
		}
	    }
	  
	  prethrottle = 0;
	}
      else /* Otherwise throttle the loop */
	{
	  /* Throttling scheme:
	   *
	   * First let the loop query the non-blocking socket 10 times (prethrottle),
	   * this should allow the TCP subsystem time enough when data has reached the
	   * host and is in the stack buffers.
	   *
	   * Next progressively throttle the loop by sleeping for 5ms and increasing
	   * the sleep time by 5ms increments until a maximum of 100ms.  Continue to
	   * throttle the loop by sleeping for 100ms until data arrives.
	   */
	  prethrottle++;
	  
	  if ( prethrottle >= 10 )
	    {
	      if ( throttlewait < 100 || throttlecnt == 1 )
		throttlewait = 5 * throttlecnt;
	      
	      throttlecnt++;
	      
	      sleep_ew (throttlewait);
	    }
	  else
	    {
	      throttlecnt = 1;
	    }
	}
    }
  
  /* Make sure everything is shut down and save the state file */
  if ( slconn->link != -1 )
    sl_disconnect (slconn);
  
  if ( statefile )
    sl_savestate (slconn, statefile);
  
  tport_detach(&regionOut);

  logit("t","%s terminated\n", argv[0]);

  return 0;
}				/* End of main() */


/***************************************************************************
 * packet_handler():
 * Process a received packet based on packet type.
 ***************************************************************************/
void
packet_handler (char *msrecord, int packet_type,
		int seqnum, int packet_size)
{
  /* The following is dependent on the packet type values in libslink.h */
  char *type[]  = { "Data", "Detection", "Calibration", "Timing",
		    "Message", "General", "Request", "Info",
                    "Info (terminated)", "KeepAlive" };

  /* Process waveform data */
  if ( packet_type == SLDATA )
    {
      sl_log (0, 1, "seq %d, Received %s blockette\n",
	      seqnum, type[packet_type]);

      mseed2ewring (msrecord, &regionOut, &waveLogo);
    }
  else if ( packet_type == SLKEEP )
    {
      sl_log (0, 2, "Keep alive packet received\n");
    }
  else
    {
      sl_log (0, 1, "seq %d, Received %s blockette\n",
	      seqnum, type[packet_type]);
    }
}				/* End of packet_handler() */


/***************************************************************************
 * mseed2ewring():
 * Unpack a Mini-SEED record, populate a TracePacket and send it to
 * the ring.
 *
 ***************************************************************************/
int
mseed2ewring (char *msrecord, SHM_INFO *pregionOut, MSG_LOGO *pwaveLogo)
{
  static MSrecord * msr = NULL;
  TracePacket tbuf;
  int tracebuf2 = 0;   /* TRACEBUF2 => 0: none, 1: available, 2: populated */
  int len;
  uint32_t *samples;
  
  msr_parse (slconn->log, msrecord, &msr, 1, 1);
  
  /* TRACE_HEADER and TRACE2_HEADER are the same size */
  memset (&tbuf, 0, sizeof(TRACE_HEADER));
  
  /* Log packet details if the verbosity is high */
  if ( verbose > 1 )
    msr_print (slconn->log, msr, verbose - 1);
  
  /* Create a TRACEBUF2 message if supported */
#ifdef TRACE2_STA_LEN
  tracebuf2 = 1;
  
  if ( ! forcetracebuf )
    {
      tbuf.trh2.pinno = 0;
      tbuf.trh2.nsamp = msr->numsamples;
      
      tbuf.trh2.starttime = msr_depochstime(msr);
      msr_dsamprate (msr, &tbuf.trh2.samprate);
      tbuf.trh2.endtime = (tbuf.trh2.starttime +
			   ((tbuf.trh2.nsamp - 1) / tbuf.trh2.samprate));
      
      strncpclean(tbuf.trh2.net, msr->fsdh.network, 2);
      strncpclean(tbuf.trh2.sta, msr->fsdh.station, 5);
      strncpclean(tbuf.trh2.chan, msr->fsdh.channel, 3);
      
      if ( strncmp(msr->fsdh.location, "  ", 2) == 0 )
	strncpclean(tbuf.trh2.loc, LOC_NULL_STRING, 2);
      else
	strncpclean(tbuf.trh2.loc, msr->fsdh.location, 2);
      
      tbuf.trh2.version[0] = TRACE2_VERSION0;
      tbuf.trh2.version[1] = TRACE2_VERSION1;
      
      /* The decoding always produces 32-bit integers in host byte order */
#ifdef _INTEL
      strcpy(tbuf.trh2.datatype, "i4");
#endif
#ifdef _SPARC
      strcpy(tbuf.trh2.datatype, "s4");
#endif
      
      tbuf.trh2.quality[0] = msr->fsdh.dq_flags;
      tbuf.trh2.quality[1] = 0;
      
      tracebuf2 = 2;
    }
#endif
  
  if ( tracebuf2 != 2 )
    {
      /* Create a TRACEBUF message otherwise */
      tbuf.trh.pinno = 0;
      tbuf.trh.nsamp = msr->numsamples;
      
      tbuf.trh.starttime = msr_depochstime(msr);
      msr_dsamprate (msr, &tbuf.trh.samprate);
      tbuf.trh.endtime = (tbuf.trh.starttime +
			  ((tbuf.trh.nsamp - 1) / tbuf.trh.samprate));
      
      strncpclean(tbuf.trh.net, msr->fsdh.network, 2);
      strncpclean(tbuf.trh.sta, msr->fsdh.station, 5);
      strncpclean(tbuf.trh.chan, msr->fsdh.channel, 3);
      
      /* The decoding always produces 32-bit integers in host byte order */
#ifdef _INTEL
      strcpy(tbuf.trh.datatype, "i4");
#endif
#ifdef _SPARC
      strcpy(tbuf.trh.datatype, "s4");
#endif
      
      tbuf.trh.quality[0] = msr->fsdh.dq_flags;
      tbuf.trh.quality[1] = 0;
    }


  /* SeedLink always uses 512-byte Mini-SEED records, all of the samples
     should always fit into a single TracePacket if MAX_TRACEBUF_SIZ
     remains defined in Trace_buf.h as 4096 or greater */
  
  samples = (uint32_t *) ((char *)&tbuf + sizeof(TRACE_HEADER));
  memcpy (samples, msr->datasamples, msr->numsamples * sizeof(uint32_t));
  
  len = (msr->numsamples * sizeof(uint32_t)) + sizeof(TRACE_HEADER);
  
  /* Set the approriate TRACE type in the logo */
  if ( tracebuf2 == 2 )
    {
      if ( typeWaveform2 == 0 )
	{
	  logit("et", "slink2ew: Error - created TRACE2_HEADER but TYPE_TRACEBUF2 is unknown\n");
	  return EW_FAILURE;
	}
      else
	{
	  pwaveLogo->type = typeWaveform2;
	}
    }
  else
    {
      pwaveLogo->type = typeWaveform;
    }
  
  if ( tport_putmsg( pregionOut, pwaveLogo, len, (char*)&tbuf )
       != PUT_OK )
    {
      logit("et", "slink2ew: Error sending message via transport.\n");
      return EW_FAILURE;
    }
  
  return EW_SUCCESS;
}				/* End of mseed2ewring() */


/***************************************************************************
 * report_status():
 * Send error and hearbeat messages to transport ring.
 *
 ***************************************************************************/
void
report_status( MSG_LOGO * pLogo, short code, char * message )
{
  char          outMsg[MAXMESSAGELEN];  /* The outgoing message.        */
  time_t        msgTime;        /* Time of the message.                 */

  /*  Get the time of the message                                       */
  time( &msgTime );
  
  /* Build & process the message based on the type */
  if ( pLogo->type == typeHeartbeat )
  {
    sprintf( outMsg, "%ld %ld\n\0", (long) msgTime, (long) myPid );
    
    /* Write the message to the output region */
    if ( tport_putmsg( &regionOut, &hrtLogo, (long) strlen( outMsg ),
		       outMsg ) != PUT_OK )
    {
      /* Log an error message */
      logit( "et", "slink2ew: Failed to send a heartbeat message (%d).\n",
             code );
    }
  }
  else
  {
    if ( message ) 
    {
      sprintf( outMsg, "%ld %hd %s\n\0", (long) msgTime, code, message );
      logit("t","Error:%d (%s)\n", code, message );
    }
    else 
    {
      sprintf( outMsg, "%ld %hd\n\0", (long) msgTime, code );
      logit("t","Error:%d (No description)\n", code );
    }
    
    /* Write the message to the output region  */
    if ( tport_putmsg( &regionOut, &errLogo, (long) strlen( outMsg ),
		       outMsg ) != PUT_OK )
    {
      /*     Log an error message                                    */
      logit( "et", "slink2ew: Failed to send an error message (%d).\n",
             code );
    }
    
  }
}				/* End of report_status() */


/***************************************************************************
 * configure():
 * Process configuration parameters.
 *
 ***************************************************************************/
void
configure (char ** argvec)
{

  /* Initialize name of log-file & open it */
  logit_init (argvec[1], 0, 512, 1);

  /* Read module config file */
  if ( proc_configfile (argvec[1]) == EW_FAILURE )
  {
    fprintf (stderr, "%s: configure() failed \n", argvec[0]);
    exit (EW_FAILURE);
  }

  /* Read node configuration info */
  if ( GetLocalInst( &myInstId) != 0 )
  {
    fprintf(stderr, "slink2ew: Error getting myInstId.\n" );
    exit (EW_FAILURE);
  }
  
  /* Lookup the ring key */
  if ((ringKey = GetKey (ringName) ) == -1) 
  {
    fprintf (stderr,
	     "slink2ew:  Invalid ring name <%s>; exitting!\n", ringName);
    exit (EW_FAILURE);
  }
  
  /* Look up message types of interest */
  if (GetType ("TYPE_HEARTBEAT", &typeHeartbeat) != 0) 
  {
    fprintf (stderr, 
             "slink2ew: Invalid message type <TYPE_HEARTBEAT>; exitting!\n");
    exit (EW_FAILURE);
  }
  if (GetType ("TYPE_ERROR", &typeError) != 0) 
  {
    fprintf (stderr, 
             "slink2ew: Invalid message type <TYPE_ERROR>; exitting!\n");
    exit (EW_FAILURE);
  }

  if (GetType ("TYPE_TRACEBUF", &typeWaveform) != 0) 
  {
    fprintf (stderr, 
             "slink2ew: Invalid message type <TYPE_TRACEBUF>; exitting!\n");
    exit (EW_FAILURE);
  }
  
  /* No GetType error checking as this type will not exist in all versions */
  GetType ("TYPE_TRACEBUF2", &typeWaveform2);
  
  /* Set up logos for outgoing messages */
  hrtLogo.instid = myInstId;
  hrtLogo.mod    = myModId;
  hrtLogo.type   = typeHeartbeat;
  
  errLogo.instid = myInstId;
  errLogo.mod    = myModId;
  errLogo.type   = typeError;
  
  waveLogo.instid = myInstId;
  waveLogo.mod    = myModId;
  waveLogo.type   = 0;  /* This gets set to the appropriate type later */
  
  /* Get my process ID so I can let statmgr restart me */
  myPid = getpid();
  
  logit ("et" , "%s(%s): Read command file <%s>\n",
	 argvec[0], myModName, argvec[1]);

  /* Initialize libslink logging facility */
  sl_loginit (verbose,
	      &logit_msg, "",
	      &logit_err, "error: ");
  
  /* Attempt to recover sequence numbers from state file */
  if ( statefile )
  {
    if (sl_recoverstate (slconn, statefile) < 0)
    {
      sl_log (1, 0, "state recovery failed\n");
    }
  }
  
  /* Reinitialize the logging level */
  logit_init (argvec[1], 0, 512, logSwitch);

  return;
}				/* End of configure() */


/***************************************************************************
 * proc_configfile():
 * Process the module configuration parameters.
 *
 ***************************************************************************/
int
proc_configfile (char * configfile)
{
  char    		*com;
  char    		*str;
  int      		nfiles;
  int      		success;

  int    slport = 0;
  char  *slhost = 0;
  char   sladdr[100];
  char  *selectors = 0;
  char  *paramdir = 0;

  /* Some important initial values or defaults */
  ringName[0]   = '\0';
  myModName[0] = '\0';
  heartbeatInt = -1;
  logSwitch    = -1;

  /* Open the main configuration file */
  nfiles = k_open (configfile);
  if (nfiles == 0)
  {
    fprintf (stderr,
             "slink2ew: Error opening command file <%s>; exiting!\n", 
             configfile);
    return EW_FAILURE;
  }

  /* Process all command files */
  while (nfiles > 0)   /* While there are command files open */
  {
    while (k_rd ())        /* Read next line from active file  */
    {
      com = k_str ();         /* Get the first token from line */

      /* Ignore blank lines & comments */
      if (!com)
        continue;
      if (com[0] == '#')
        continue;

      /* Open a nested configuration file */
      if (com[0] == '@')
      {
        success = nfiles + 1;
        nfiles  = k_open (&com[1]);
        if (nfiles != success) 
        {
          fprintf (stderr, 
                   "slink2ew: Error opening command file <%s>; exiting!\n", 
                   &com[1]);
          return EW_FAILURE;
        }
        continue;
      }

      /* Process anything else as a command */
      if (k_its ("MyModuleId")) 
      {
        if ( (str = k_str ()) )
	{
          if (strlen(str) >= MAXMODNAMELEN)
          {
            fprintf(stderr, "MyModId too long; max is %d\n", MAXMODNAMELEN -1);
            return EW_FAILURE;
          }

          strcpy (myModName, str);

	  /* Lookup module ID */
	  if ( GetModId( myModName, &myModId) != 0 )
	  {
	    fprintf( stderr, "slink2ew: Error getting myModId.\n" );
	    exit (EW_FAILURE);
	  }
        }
      }

      else if (k_its ("RingName")) 
      {
        if ( (str = k_str ()) )
        {
          if (strlen(str) >= MAXRINGNAMELEN)
          {
            fprintf(stderr, "OutRing name too long; max is %d\n", 
                    MAXRINGNAMELEN - 1);
            return EW_FAILURE;
          }

	  strcpy (ringName, str);
        }
      }

      else if (k_its ("HeartBeatInterval")) 
      {
        heartbeatInt = k_long ();
      }

      else if (k_its ("LogFile"))
      {
        logSwitch = k_int();
      }

      else if ( k_its("Verbosity") )
      {
      	if ( (str = k_str ()) )
	{
	  verbose = atoi (str);
	}
	if ( !str )
	{
          fprintf(stderr, "Verbosity is unspecified, quiting\n");
          return EW_FAILURE;
        }
      }

      else if (k_its ("SLhost"))
      {
        if ( (str = k_str ()) )
        {
          if (strlen(str) >= MAXADDRLEN)
          {
            fprintf(stderr, "SLaddr too long; max is %d characters\n",
		    MAXADDRLEN);
            return EW_FAILURE;
          }
          slhost = strdup(str);
        }
      }

      else if ( k_its ("SLport"))
      {
        slport = k_int();
        if (slport < 1)
        {
          fprintf(stderr, "SLport is 0 or junk, quiting.\n");
          return EW_FAILURE;
        }
      }
      
      else if (k_its ("StateFile"))
      {
	/* Build a state file name of the form 'slink<modid>.state' and
	   prepend the parameter directory */
	paramdir = getenv ( "EW_PARAMS" );
	statefile = (char *) malloc (strlen(paramdir) + 16);

        #ifdef _INTEL
	sprintf (statefile, "%s\\slink%d.state", paramdir, myModId);
        #else  /* *nix brand */
	sprintf (statefile, "%s/slink%d.state", paramdir, myModId);
        #endif
      }

      else if ( k_its ("StateFileInt"))
      {
	if ( (str = k_str ()) )
	{
	  stateint = atoi (str);
	}
	if ( !str || stateint < 0 )
	{
          fprintf(stderr, "StateFileInt is unspecified or negative, quiting\n");
          return EW_FAILURE;
        }
      }

      else if ( k_its ("NetworkTimeout"))
      {
	if ( (str = k_str ()) )
	{
	  slconn->netto = atoi (str);
	}
	if ( !str || slconn->netto < 0 )
	{
          fprintf(stderr, "NetworkTimeout is unspecified or negative, quiting\n");
          return EW_FAILURE;
        }
      }

      else if ( k_its ("NetworkDelay"))
      {
	if ( (str = k_str ()) )
	{
	  slconn->netdly = atoi (str);
	}
	if ( !str || slconn->netdly < 0 )
	{
          fprintf(stderr, "NetworkDelay is unspecified or negative, quiting\n");
          return EW_FAILURE;
        }
      }

      else if ( k_its ("KeepAlive"))
      {
	if ( (str = k_str ()) )
	{
	  slconn->keepalive = atoi (str);
	}
	if ( !str || slconn->keepalive < 0 )
	{
          fprintf(stderr, "KeepAlive is unspecified or negative, quiting\n");
          return EW_FAILURE;
        }
      }
      
      else if (k_its ("ForceTraceBuf1"))
      {
        forcetracebuf = k_int();
      }
      
      else if ( k_its ("Selectors"))
      {
        if ( (str = k_str ()) )
        {
          if (strlen(str) >= 100)
          {
            fprintf(stderr, "Selectors too long; max is 100 characters\n");
            return EW_FAILURE;
          }
          selectors = strdup(str);
        }
      }
      
      else if (k_its ("Stream"))
      {
	if ( (str = k_str ()) )
        {
	  char *net;
	  char *sta;
	  char netsta[20];
	  char streamselect[100];

	  streamselect[0] = '\0';

	  /* Collect the stream key (the NET_STA specifier) */
	  strncpy (netsta, str, sizeof(netsta));
	  net = netsta;
	  if ( (sta = strchr (netsta, '_')) == NULL )
	  {
	    fprintf(stderr, "Could not parse stream key: %s\n", str);
	    return EW_FAILURE;
	  }
	  else
          {
	    *sta++ = '\0';
          }
	  
	  /* Build a selector list from an optional 3rd value */
	  if ( (str = k_str ()) )
	  {
	    strncpy (streamselect, str, sizeof(streamselect));
	  }
	  else
	    k_err();  /* Clear the error if there was no selectors */
	  
	  if ( streamselect[0] != '\0' )
	    sl_addstream (slconn, net, sta, streamselect, -1, NULL);
	  else
	    sl_addstream (slconn, net, sta, selectors, -1, NULL);
	}
      }
      
      /* Unknown command */ 
      else 
      {
        fprintf (stderr, "slink2ew: <%s> Unknown command in <%s>.\n", 
                 com, configfile);
        continue;
      }

      /* See if there were any errors processing the command */
      if (k_err ()) 
      {
        fprintf (stderr, 
                 "slink2ew: Bad command in <%s>; exiting!\n\t%s\n",
                 configfile, k_com());
        return EW_FAILURE;
      }

    } /** while k_rd() **/

    nfiles = k_close ();

  } /** while nfiles **/

  /* Configure uni-station mode if no streams were specified */
  if ( slconn->streams == NULL )
    sl_setuniparams (slconn, selectors, -1, 0);

  /* Check for required parameters */
  if ( myModName[0] == '\0' )
  {
    fprintf (stderr, "slink2ew: No MyModId parameter found in %s\n",
	     configfile);
    return EW_FAILURE;
  }
  if ( ringName[0] == '\0' )
  {
    fprintf (stderr, "slink2ew: No OutRing parameter found in %s\n",
	     configfile);
    return EW_FAILURE;
  }
  if ( heartbeatInt == -1 )
  {
    fprintf (stderr, "slink2ew: No HeartBeatInterval parameter found in %s\n",
	     configfile);
    return EW_FAILURE;
  }
  if ( logSwitch == -1 )
  {
    fprintf (stderr, "slink2ew: No LogFile parameter found in %s\n",
	     configfile);
    return EW_FAILURE;
  }
  if ( !slhost )
  {
    fprintf (stderr, "slink2ew: No SLhost parameter found in %s\n",
	     configfile);
    return EW_FAILURE;
  }
  if ( !slport )
  {
    fprintf (stderr, "slink2ew: No SLport parameter found in %s\n",
	     configfile);
    return EW_FAILURE;
  }

  /* Configure the SeedLink connection description thing */
  snprintf (sladdr, sizeof(sladdr), "%s:%d", slhost, slport);
  slconn->sladdr = strdup(sladdr);

  return EW_SUCCESS;
}				/* End of proc_configfile() */


/***************************************************************************
 * logit_msg() and logit_err():
 * 
 * Hooks for Earthworm logging facility.  These are used via function
 * pointers by the SeedLink library.
 ***************************************************************************/
void
logit_msg (const char *msg)
{
  logit ("t", (char *) msg);
}

void
logit_err (const char *msg)
{
  logit ("et", (char *) msg);
}


/***************************************************************************
 * usage():
 * Print the usage message and exit.
 ***************************************************************************/
void
usage (char * progname)
{
  fprintf (stderr, "slink2ew version %s\n", VERSION);
  fprintf (stderr, "Usage: %s <config file>\n", progname);
}				/* End of usage() */
