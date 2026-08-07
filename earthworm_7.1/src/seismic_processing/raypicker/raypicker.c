/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: raypicker.c,v 1.1 2006/01/30 19:29:30 friberg Exp $
 *
 *
 */

      /*****************************************************************
       *                           raypicker.c                         *
       *                                                               *
       *  This is the new Earthworm global picker.  The program is     *
       *  based on Ray Buland's FORTRAN code, c. 198x.                 *
       *                                                               *
       *****************************************************************/

/* system includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <search.h>       /* qsort() */
#include <malloc.h>

/* earthworm includes */
#include <earthworm.h>
#include <transport.h>
#include <trace_buf.h>
#include <mem_circ_queue.h>
#include <swap.h>

/* hydra includes */
#include <ioc_defs.h>
#include <watchdog_client.h>

/* raypicker includes */
#include "raypicker.h"
#include "rate_constants.h"
#include "pre_filter.h"
#include "compare.h"
#include "config.h"
#include "pick_channel_info.h"
#include "pick_series.h"
#include "tracebuf2double.h"  /* NewBuffer(), NextBufferValue() */
#include "returncodes.h"
#include "rp_messaging.h"

/* Functions in this source file 
 *******************************/
int             GetEWParams(EWParameters *ewp);
thr_ret         MessageStacker(void *dummy);
thr_ret         Processor(void *dummy);
SCNL           *find_SCNL(TRACE2_HEADER *TraceHeader);

pid_t           myPid;	       /* Hold our process ID to be sent with heartbeats */
short           nLogo = 1;     /* number of logos to get */

/* The message queue
 *******************/
QUEUE 	MsgQueue;              /* from queue.h */

/* Thread stuff */
#define THREAD_STACK 8192
#define THREAD_ALIVE 1
#define THREAD_ERR  -1
static unsigned tidProcessor;  /* Processor thread id */
static unsigned tidStacker;    /* Thread moving messages from InRing */
                               /* to MsgQueue */
int MessageStackerStatus = THREAD_ALIVE; /* ALIVE=> Stacker thread ok. ERR => dead */
int ProcessorStatus = THREAD_ALIVE;     /* ALIVE=> Processor thread ok. ERR => dead */

MSG_LOGO        getLogo;       /* Logo of requested waveforms */
SCNL           *scnl;          /* pointer to array of SCNL structures */
RParams         params;        /* stucture containing configuration parameters  */
EWParameters    EwParameters;  /* structure containing ew-type parameters */

int             numSCNL = 0;   /* number of SCNLs */

/***********************************************************
 *              The main program starts here.              *
 *                                                         *
 ***********************************************************/
int main(int argc, char **argv)
{
    char         *configfile;       /* pointer to name of config file              */
    char          logFileName[256]; /* name of disk log file                       */
    MSG_LOGO      hrtlogo;          /* Logo of outgoing heartbeats                 */
    char         *flushmsg;         /* pointer to junk messages in ring at startup */
 
    MSG_LOGO      logo;             /* Logo of retrieved msg                       */
    long          msgLen;           /* Size of retrieved message                   */
    time_t        timelastBeat;     /* Previous heartbeat time                     */
    time_t        now;              /* Current time                                */
    int           i;                /* loop counter                                */

    char          version[] = "May 26, 2005 1437 UT"; /* version number */


    /* Check command line arguments
    *******************************/
    if (argc != 2)
    {
      fprintf( stderr, "Usage: raypicker <configfile>\n" );
      exit(MODULE_RET_ERROR);
    }
    configfile = argv[1];


    /* Initialize error reporting system */
    strcpy(logFileName, "raypicker");

    if (reportErrorInit(10240, 1, logFileName) != EW_SUCCESS)
    {
      fprintf( stderr, "Could not initialize error reporting system; EXIT!\n");
      exit(MODULE_RET_ERROR);
    }

    /* Alex's canary - not a fatal error 
     ***********************************/
    reportError(WD_FATAL_ERROR, GENFATERR, "Raypicker canary: raypicker starting (again). \n");

     /* Write version number to logfile 
     *********************************/
    logit("", "raypicker version %s\n", version);

    /* Set defaults for some of the configuration parameters
    ********************************************************/
    if (raypicker_default_params(&params) != EW_SUCCESS)
    {
      reportError(WD_FATAL_ERROR, GENFATERR, "raypicker: raypicker_default_params() failed. Exiting.\n");
      exit(MODULE_RET_ERROR);
    }

    /* Get parameters from the configuration files
    **********************************************/
    if (raypicker_config(configfile, &params, &scnl, &numSCNL) != EW_SUCCESS)
      exit(MODULE_RET_ERROR);
	
    /* Sanity check of the configuration parameters
    ***********************************************/
    if (CheckConfig(params, numSCNL)!= EW_SUCCESS)
      exit(MODULE_RET_ERROR);

    /* Allocate PICK_CHANNEL_INFO structure for each SCNL
     ****************************************************/
    for (i = 0; i < numSCNL; i++)
    {
      if ((scnl[i].pchanInfo = (PICK_CHANNEL_INFO *)malloc(sizeof(PICK_CHANNEL_INFO))) == NULL)
      {
        reportError(WD_FATAL_ERROR, MEMALLOC, "raypicker: cannot allocate space for PICK_CHANNEL_INFO for <%s:%s:%s:%s>\n",
              scnl[i].sta, scnl[i].chan, scnl[i].net, scnl[i].loc);
        exit(MODULE_RET_ERROR);
      }

      /* if allocation is successful, initialize the SCNL info */
      if (InitChannelInfo(scnl[i].pchanInfo, params.MaxSampleRate) != EW_SUCCESS)
      {
        logit("", "raypicker: InitChannelInfo unsuccessful for <%s:%s;%s;%s>\n",
              scnl[i].sta, scnl[i].chan, scnl[i].net, scnl[i].loc);
        exit(MODULE_RET_ERROR);
      }
    }

    /* Sort the station list by SCNL number
    ***************************************/
    qsort(scnl, numSCNL, sizeof(SCNL), CompareSCNLs);

    /* Log the configuration parameters
    ***********************************/
    LogConfig(params, scnl, numSCNL);

    /* Look up info in the earthworm.h tables
    *****************************************/
    if (GetEWParams(&EwParameters) != EW_SUCCESS)
      exit(MODULE_RET_ERROR);

    /* Get our process ID or restart purposes
    *****************************************/
    if ((myPid = getpid ()) == -1)
    {
      reportError(WD_FATAL_ERROR, GENFATERR, "raypicker: Can't get my pid. Exiting.\n" );
      exit(MODULE_RET_ERROR);
    }

    /* Specify logos outgoing heartbeats
    ************************************/
    hrtlogo.instid = EwParameters.MyInstId;
    hrtlogo.mod    = params.MyModId;
    hrtlogo.type   = EwParameters.TypeHeartBeat;

    /* Specify logos of incoming waveforms
    **************************************/
    getLogo.instid = EwParameters.GetThisInstId;
    getLogo.mod    = EwParameters.GetThisModId;
    getLogo.type   = EwParameters.TypeTracebuf2;

    /* Initialize rate constants
    ****************************/
    if (InitRateConstants(params.MaxPreFilters) != EW_SUCCESS)
      exit(MODULE_RET_ERROR);

    /*  Initialize all prefilters
    *****************************/
    if (initAllFilters(params.MaxPreFilters) != EW_SUCCESS)
      exit(MODULE_RET_ERROR);

    /* Initialize and allocate global memory pointers for SERIES_DATA arrays
    ************************************************************************/    
    if (AllocChannelGlobals(params.MaxSampleRate) != EW_SUCCESS)
      exit(MODULE_RET_ERROR);

    /* Initialize transfer function 
    *******************************/
    if (initTransferFn() != EW_SUCCESS)
      exit(MODULE_RET_ERROR);

    /* Attach to transport rings
    ****************************/
    if (params.OutKey != params.InKey)
    {
      tport_attach(&EwParameters.InRing,  params.InKey);
      tport_attach(&EwParameters.OutRing, params.OutKey);
    }
    else
    {
      tport_attach(&EwParameters.InRing, params.InKey);
      EwParameters.OutRing = EwParameters.InRing;
    }

    /* Flush the input ring (NB: MAY NEED TO INCREAZE nLogo = 1 
     * if further specification on desired messages)
    ***********************************************************/
    if ((flushmsg = (char *)malloc(MAX_TRACEBUF_SIZ)) ==  NULL)
    {
      reportError(WD_FATAL_ERROR, MEMALLOC, "raypicker: can't allocate flushmsg; exiting.\n");
      exit(MODULE_RET_ERROR);
    }

    while (tport_getmsg(&EwParameters.InRing, &getLogo, 1, &logo, &msgLen,
                          flushmsg, (MAX_TRACEBUF_SIZ - 1)) != GET_NONE);
    free(flushmsg);

    /* Force a heartbeat to be issued in first pass thru main loop
    **************************************************************/
    timelastBeat = time(&now) - params.HeartBeatInterval - 1;

    /* Create a Mutex to control access to queue
    ********************************************/
    CreateMutex_ew();

    /* Initialize the message queue
    *******************************/
    initqueue(&MsgQueue, (unsigned long)params.QueueSize, (unsigned long)(MAX_TRACEBUF_SIZ));

    /* Start message stacking thread which will read 
    messages from the InRing and put them into the Queue 
    ****************************************************/
    if (StartThread(MessageStacker, (unsigned) THREAD_STACK, &tidStacker) == -1)
    {
      logit("e", "raypicker: Error starting MessageStacker thread. Exiting.\n");
      if (params.OutKey != params.InKey)
      {
          tport_detach(&EwParameters.InRing);
          tport_detach(&EwParameters.OutRing);
      }
      else
          tport_detach(&EwParameters.InRing);

      return EW_FAILURE;
    }

    MessageStackerStatus = THREAD_ALIVE; /*assume the best*/

    /* Start decimator thread which will read messages from
    the Queue, process them and write them to the OutRing
    *******************************************************/
    if (StartThread(Processor, (unsigned) THREAD_STACK, &tidProcessor) == -1)
    {
      logit("e", "raypicker: Error starting Processor thread. Exiting.\n");
      if (params.OutKey != params.InKey)
      {
          tport_detach(&EwParameters.InRing);
          tport_detach(&EwParameters.OutRing);
      }
      else
          tport_detach(&EwParameters.InRing);

      return EW_FAILURE;
    }
    ProcessorStatus = THREAD_ALIVE; /*assume the best*/

/*--------------------- setup done; start main loop -------------------------*/
    while (tport_getflag(&EwParameters.InRing) != TERMINATE  &&
           tport_getflag(&EwParameters.InRing) != myPid)
    {
      /* Send a heartbeat to the transport ring
      *****************************************/
      if ((time(&now) - timelastBeat) >= params.HeartBeatInterval)
      {
          timelastBeat = now;

          WriteHeartbeat(EwParameters, params,myPid);
      }

      /* Check on our threads
      ***********************/
      if (MessageStackerStatus != THREAD_ALIVE)
      {
          logit ("et", "raypicker: MessageStacker thread died. Exiting\n");
          fprintf (stderr, "raypicker: MessageStacker thread died. Exiting\n");
          exit(MODULE_RET_ERROR);
      }

      if (ProcessorStatus != THREAD_ALIVE)
      {
          logit ("et", "raypicker: Processor thread died. Exiting\n");
          fprintf (stderr, "raypicker: Processor thread died. Exiting\n");
          exit(MODULE_RET_ERROR);
      }

      /* Bonk our threads on the head; make the threads reset them to alive 
       * in some reasonable time frame
      **********************************************************************/
      MessageStackerStatus = THREAD_ERR;
      ProcessorStatus = THREAD_ERR;

      sleep_ew(params.HeartBeatInterval * 1000);

    } /* wait until TERMINATE is raised  */  

    /* Termination has been requested
    *********************************/
    if (params.OutKey != params.InKey)
    {
      tport_detach(&EwParameters.InRing);
      tport_detach(&EwParameters.OutRing);
    }
    else
      tport_detach(&EwParameters.InRing);

    logit ("t", "raypicker: Termination requested; exiting!\n" );

    /* reportErrorCleanup(); */
    return EW_SUCCESS;
}


/*******************************************************
 *                 GetEWParams()                       *
 *                                                     *
 *      Get parameters from the earthworm.h file.      *
 *******************************************************/
int GetEWParams(EWParameters *ewp)
{
    if (GetLocalInst(&ewp->MyInstId) != 0)
    {
      fprintf(stderr, "raypicker: Error getting MyInstId.\n");
      return EW_FAILURE;
    }

    if (GetInst("INST_WILDCARD", &ewp->GetThisInstId) != 0)
    {
      fprintf(stderr, "raypicker: Error getting GetThisInstId.\n");
      return EW_FAILURE;
    }
    if (GetModId("MOD_WILDCARD", &ewp->GetThisModId) != 0)
    {
      fprintf(stderr, "raypicker: Error getting GetThisModId.\n");
      return EW_FAILURE;
    }
    if (GetType("TYPE_HEARTBEAT", &ewp->TypeHeartBeat) != 0)
    {
      fprintf(stderr, "raypicker: Error getting TypeHeartBeat.\n");
      return EW_FAILURE;
    }
    if ( GetType("TYPE_ERROR", &ewp->TypeError) != 0)
    {
      fprintf(stderr, "raypicker: Error getting TypeError.\n");
      return EW_FAILURE;
    }
    if (GetType("TYPE_PICK_GLOBAL", &ewp->TypeGlobalPick) != 0)
    {
      fprintf(stderr, "raypicker: Error getting TypeGlobalPick.\n");
      return EW_FAILURE;
    }
    if (GetType("TYPE_AMP_GLOBAL", &ewp->TypeGlobalAmp) != 0)
    {
      fprintf(stderr, "raypicker: Error getting TypeGlobalAmp.\n");
      return EW_FAILURE;
    }
    if (GetType("TYPE_TRACEBUF2", &ewp->TypeTracebuf2) != 0)
    {
      fprintf(stderr, "raypicker: Error getting TYPE_TRACEBUF2.\n");
      return EW_FAILURE;
    }
    return EW_SUCCESS;
}

#define DEFAULT_NO_TRACEBUF_TIMEOUT_SECONDS 30
/******************************************************************
 *                      Message Stacking Thread                   *
 *           Move messages from transport to memory queue         *
 ******************************************************************/
thr_ret	MessageStacker (void *dummy)
{
    char           *msg;            /* "raw" retrieved message   */
    int             rc;             /* return code               */
    long            msgLen;         /* size of retrieved message */
    MSG_LOGO        logo;           /* logo of retrieved message */
    TRACE2_HEADER   *TraceHeader; 	/* header of tracebuf msg    */    
    SCNL           *thisScnl;       /* Pointer to the station being processed */
    int             tSleepCtr=0;
    int             tMaxSleepThreshold;



    /* set the amount of time to sleep before issuing a "no tracebufs message"
       to be the lesser of 30 seconds or (HeartBeatInterval-2) seconds,
       so that we are reasonably certain to issue a log message about not 
       seeing tracebufs before we get squashed by the main thread for not
       updating our status.  DK 122805
     ************************************************************************/
    tMaxSleepThreshold = (params.HeartBeatInterval - 2) * 1000;
    if(tMaxSleepThreshold > ( DEFAULT_NO_TRACEBUF_TIMEOUT_SECONDS * 1000))
      tMaxSleepThreshold = DEFAULT_NO_TRACEBUF_TIMEOUT_SECONDS * 1000;

    /* Allocate space for input/output messages
    *******************************************/
    if ((msg = (char *)malloc(MAX_TRACEBUF_SIZ)) == NULL)
    {
      logit ("e", "raypicker: error allocating msg; exiting!\n");
      goto error;
    }

    TraceHeader = (TRACE2_HEADER *)msg;

    /* Tell the main thread we're ok
    ********************************/
    MessageStackerStatus = THREAD_ALIVE;

    /* Start service loop, picking up trigger messages
    **************************************************/
    while (1)
    {
      /* Get a message from transport ring
      ************************************/
      rc = tport_getmsg(&EwParameters.InRing, &getLogo, nLogo, &logo, &msgLen,
                         msg, MAX_TRACEBUF_SIZ - 1);
 
      if (rc == GET_NONE) 
      {
          sleep_ew(100); 
          tSleepCtr += 100;
          if(tSleepCtr > tMaxSleepThreshold)
          {
            reportError(WD_INFO, NORESULT, "raypicker: No new tracebufs in last 30 seconds.\n");
            tSleepCtr = 0;
          }
          continue;
      } /* wait if no messages for us */
      else
        tSleepCtr = 0;

  
      /* Check return code; report errors
      ***********************************/
      if (rc != GET_OK)
      {
          if (rc == GET_TOOBIG)
          {
              reportError(WD_FATAL_ERROR, ERR_TOOBIG, "Retrieved message[%ld] i%d m%d t%d too long for target\n",
							msgLen, (int) logo.instid, (int) logo.mod, (int)logo.type);
              continue;
          }
          else if (rc == GET_MISS)
              reportError(WD_FATAL_ERROR, ERR_MISSMSG, "Missed msg(s) i%d m%d t%d in Ring\n",
						(int) logo.instid, (int) logo.mod, (int)logo.type);
          else if (rc == GET_NOTRACK)
              reportError(WD_FATAL_ERROR, ERR_NOTRACK, "no tracking for logo i%d m%d t%d in Ring\n",
						(int) logo.instid, (int) logo.mod, (int)logo.type);
          else if (rc == GET_MISS_LAPPED)
              reportError(WD_FATAL_ERROR, ERR_MISS_LAPPED, "Got lapped on the ring for logo i%d m%d t%d\n",
						(int) logo.instid, (int) logo.mod, (int)logo.type);
          else if (rc == GET_MISS_SEQGAP)
              reportError(WD_FATAL_ERROR, ERR_MISS_SEQGAP, "Gap in sequence numbers for logo i%d m%d t%d\n",
						(int) logo.instid, (int) logo.mod, (int)logo.type);
      }

      /* If necessary, swap bytes in the wave message
      ***********************************************/
      if (WaveMsg2MakeLocal(TraceHeader) < 0)
      {
          logit ("et", "raypicker: Unknown waveform type.\n");
          continue;
      }

      /* Only TYPE_TRACEBUF2 messages are desired
      ******************************************/
      if (logo.type == EwParameters.TypeTracebuf2) 
      {
          /* Check to see if we want this SCNL
          ************************************/
          thisScnl = find_SCNL(TraceHeader);

          if (thisScnl == NULL) /* SCNL not found */
          {
               if (params.Debug >= 3)

                  /*logit("e", "raypicker: SCNL <%s %s %s --> not selected to be queued.\n", TraceHeader->sta, 
						TraceHeader->chan, TraceHeader->net); */
                  logit("e", "raypicker: SCNL <%s %s %s %s> not selected to be queued.\n", TraceHeader->sta, 
						TraceHeader->chan, TraceHeader->net, TraceHeader->loc);
              continue;
          }
          else                       /* want this message! Queue it.*/
          {
              if (params.Debug >= 3)
                  /*logit("e", "raypicker: SCNL <%s %s %s> selected to be queued.\n", 
			      TraceHeader->sta, TraceHeader->chan, TraceHeader->net); */
                  logit("e", "raypicker: SCNL <%s %s %s %s> selected to be queued.\n", TraceHeader->sta, 
						TraceHeader->chan, TraceHeader->net, TraceHeader->loc);

              /* Queue retrieved msg (rc = GET_OK,GET_MISS,GET_NOTRACK)
              *********************************************************/
              RequestMutex();
              /* put it into the queue */
              rc = enqueue(&MsgQueue, msg, msgLen, logo); 
              ReleaseMutex_ew();

              if (rc != 0)
              {
                  if (rc == -2)  /* Serious: quit */
                  {
                      reportError(WD_FATAL_ERROR, ERR_QUEUE, "internal queue error. Terminating.\n");
				      goto error;
                  }
                  if (rc == -1)
                  {
                      reportError(WD_WARNING_ERROR, ERR_QUEUE, "queue cannot allocate memory. Lost message.\n");
                      continue;
                  }
                  if (rc == -3) 
                  {
                      reportError(WD_WARNING_ERROR, ERR_QUEUE, "Queue full. Message lost.\n");
                      continue;
                  }
              } /* problem from enqueue */

          } /* if we have a desired message */

      } /* TYPE_TRACEBUF2 message */

      /* Since main loop bonked us over the head, tell the main thread we're ok
      *************************************************************************/
      MessageStackerStatus = THREAD_ALIVE;

    } /* while (1) */

    /* we're quitting
    *****************/
    error:
    MessageStackerStatus = THREAD_ERR; /* file a complaint to the main thread */
    KillSelfThread(); /* main thread will restart us */

}

/******************************************************************
 *                    Message Processing Thread                   *
 *                    The real work occurs here                   *
 ******************************************************************/
thr_ret	Processor(void *dummy)
{
    char            *traceBuf;        /* string to hold trace_but messages */
    long             traceBufLen;     /* length of trace_buf message       */
    TRACE2_HEADER    *traceHeader;     /* ptr to Trace data buffer          */
    MSG_LOGO         recLogo;         /* logo of retrieved message         */
    int              gotMsg;
    SCNL            *thisScnl;        /* pointer to desired SCNL           */
    PICK_CHANNEL_INFO *thisChannel;   /* pointer to channel info in SCNL   */
	
    double           sampleInterval;
    double           gapCheckLength;  /* gap in data                       */
    int              p, s;

    long             avelen;          /* number of points needed for initial 
                                         averaging for given rate          */
    double           sum;             /* sum for averaging for mean        */
   
    int              isBroadband;     /* broadband or short-period data?   */
    int              rc;              /* return code                       */


    /* Allocate the waveform buffer
     *******************************/
    traceBuf = (char *)malloc(MAX_TRACEBUF_SIZ);

    if (traceBuf == NULL)
    {
      logit ("et", "raypicker: Cannot allocate buffer for waveform data\n");
      ProcessorStatus = THREAD_ERR;
      KillSelfThread();
    }

    /* Point to header and data portions of waveform message
     *****************************************************/
    traceHeader = (TRACE2_HEADER *)traceBuf;

    while (1)
    {
      gotMsg = FALSE;
      while (gotMsg == FALSE)
      {
          RequestMutex();
          rc = dequeue(&MsgQueue, traceBuf, &traceBufLen, &recLogo);
          ReleaseMutex_ew();

          /* Since main loop bonked us over the head, tell the main thread we're ok
          *************************************************************************/
          ProcessorStatus = THREAD_ALIVE;
 
          if (rc < 0)
          {
             /* empty queue */
              sleep_ew (1000);
          }
          else
              gotMsg = TRUE;
      } /* while no messages are dequeued */

      /* find the SCNL for this message */
      thisScnl = find_SCNL(traceHeader);

      /* could not find this SCNL in list; continue */
      if (thisScnl == NULL)
          continue;

      thisChannel = thisScnl->pchanInfo;

      /* Sample rate too high for allocated buffers */
      if (thisChannel->sampleRate > params.MaxSampleRate)
      {
          reportError(WD_WARNING_ERROR, NORESULT, "Sampling rate [%f] for %s:%s:%s:%s greater than configured maximum [%f]\n", 
				  thisChannel->sampleRate, thisScnl->sta, thisScnl->chan, 
				  thisScnl->net, thisScnl->loc, params.MaxSampleRate);
          continue;
      }
      
      /* initial call set channel sample rate; subsequent calls check for mismatch */
      if ((rc = SetChannelSampleRate(traceHeader->samprate, thisChannel)) != EW_SUCCESS)
          continue; /* can't do anything with these data; look for more */

      sampleInterval = (1.0 / traceHeader->samprate);
          
      switch (thisChannel->state)
      {
          case PKCH_STATE_NEW:
              /* prepare for start of raw time series */
              InitChannelForSeries(traceHeader->starttime, thisChannel);
            
              /* set last data sample time to a notional point just prior to 
               * this tracebuf, so that there will appear to be no gap.
               * (This really just prevents status/error messaging) */
              thisChannel->rawLastTime = traceHeader->starttime - sampleInterval;
              break;
                 
          case PKCH_STATE_WAIT:
              /* Must not reset the rawStartTime while waiting for the
               * required initialization buffer length */
              break;
                 
          case PKCH_STATE_LOOK:
          case PKCH_STATE_NEWPICK:
          case PKCH_STATE_PICKSENT:
            
              /* Expectation is that -- unless sample is being accumulated
               * for initial averaging (PKCH_STATE_WAIT) -- the sample in
               * the buffer will always be copied into the zeroth index
               * and the start time will match the start of the arriving trace_buf. */
              if (params.Debug >=2)
                  logit("", "PICKSENT: RESTARTING CHANNEL SCNL %s %s %s %s starttime: %lf endtime: %lf samprate: %lf nsamp: %d \n",
                             thisScnl->sta, thisScnl->chan, thisScnl->net, thisScnl->loc, traceHeader->starttime,
                              traceHeader->endtime, traceHeader->samprate, traceHeader->nsamp);

              thisChannel->rawLength = 0;
              thisChannel->rawStartTime = traceHeader->starttime;
              break;
                 
          default:
              continue;
      }

      /*
       * Determine maximum allowed gap
       * 
       * NOTE: When the channel is encountered for the first time,
       *       the gap check length will be this first value.
       *       Since the channel's rawLastTime will have been set
       *       to PICK_CHAN_INVALID_TIME (-1), it will appear that
       *       the time since the last data on the channel will be
       *       one second more than (now - time_epoch_start[~1970]),
       *       therefore it will look like years....
       */

      if (thisChannel->isPrelim == 0  && thisChannel->isContinuing == 0)
      {
          /* no trigger ongoing */
        /*  gapCheckLength = params.MaxGapNoTriggerSecs; */
          if (params.MaxGapNoTriggerSecs < (sampleInterval / 2.0))
              gapCheckLength = sampleInterval / 2.0;
          else
              gapCheckLength = params.MaxGapNoTriggerSecs;
      }
      else
      {
          /* Some kind of triggering happening
           * Zero commonly used for MaxGapInTriggerSecs,
           * so give half a sample interval for floating point leeway */
          if (params.MaxGapInTriggerSecs < (sampleInterval / 2.0))
              gapCheckLength = sampleInterval / 2.0;
          else
              gapCheckLength = params.MaxGapInTriggerSecs;
      }
      
      /* Check for a gap longer than allowed */
      if ((traceHeader->starttime - thisChannel->rawLastTime - sampleInterval) > gapCheckLength)
      {
          /* Gap detected, re-initialize the channel series */
          if (params.Debug >= 2)
               logit("et", "Gap detected in channel %s:%s:%s:%s traceStart %lf channelEnd %lf, reinitializing series processing\n", 
                   thisScnl->sta, thisScnl->chan, thisScnl->net, thisScnl->loc, 
                   traceHeader->starttime, thisChannel->rawLastTime);
             
          /* prepare for a new start of the raw time series */
          InitChannelForSeries(traceHeader->starttime, thisChannel);
      }
      
      /* Add or append this trace buffer's data
       * 
       * Although this is extra work, it removes later complexity
       * relating to undetermined data storage types. */
                      
      /* prepare to obtain sample points regardless of physical storage type */
      if ((rc = NewBuffer(traceHeader->datatype, (char *)(traceBuf + sizeof(TRACE2_HEADER)))) != EW_SUCCESS)
      {
          reportError(WD_WARNING_ERROR, NORESULT, "Error encountered preparing series buffer for channel %s:%s:%s:%s\n", 
					 thisScnl->sta, thisScnl->chan, thisScnl->net, thisScnl->loc);
          continue;
      }

      /* Copy sample points into the channel's common physical-type storage */
      for (p = 0, s = thisChannel->rawLength; p < traceHeader->nsamp; p++, s++)
      {
          if (s == thisChannel->rawAlloc)
          {
               /* Index s exceeds buffer allocation length */
              reportError(WD_FATAL_ERROR, GENFATERR, "Data copied exceeds initialization buffer for channel %s:%s:%s:%s (programming error)\n", 
                      thisScnl->sta, thisScnl->chan, thisScnl->net, thisScnl->loc);
              ProcessorStatus = THREAD_ERR;
              KillSelfThread();
          }             
          thisChannel->rawBuffer[s] = NextBufferValue();
      }

      /* update number of points in the sample buffer */
      thisChannel->rawLength += traceHeader->nsamp;

      /* Update sample end time */
      thisChannel->rawLastTime = traceHeader->starttime + 
		  sampleInterval * (double)traceHeader->nsamp - sampleInterval;
                                          
      /* Pre-filter averaging buffer not previously completed,
       * Check if there is enough now.
       * Note that there may be more than enough data as we 
       * work only on entire tracebuf messages */
      if (thisChannel->state == PKCH_STATE_WAIT )
      {  

          if (params.Debug >= 2) 
              logit("", "Channel state is PKCH_STATE_WAIT, checking length [%d] vs %d\n",
				  thisChannel->rawLength, GetTriggerLevelCalcLength(traceHeader->samprate));
                  
          /* check the current number of points
           * 
           * NOTE: THIS CODE REQUIRES rawLength to be reset to zero after a gap. */
             
          /* sufficient points in the buffer to proceed */
          if (GetTriggerLevelCalcLength(traceHeader->samprate) <= thisChannel->rawLength)
          {   
              /* calculate points for initial averaging */
              avelen = (long)(PREFILTER_INIT_AVR_LEN * traceHeader->samprate + 0.5);
                
              sum = 0.0;     
                
              /* Sum for obtaining the iir initialization mean. */
              for (p = 0; p < avelen; p++)
                  sum += thisChannel->rawBuffer[p];

              /* Is this a broadband station? */
              switch(thisScnl->chan[0]) 
              {
                  case 'B':
                      isBroadband = TRUE;
                      break;
                  case 'H':
                      isBroadband = TRUE;
                      break;
                  default:
                      isBroadband = FALSE;
                      break;
              }
                

              switch (initChannelFilter(thisChannel->sampleRate, sum / (double)avelen,
                          isBroadband, &thisChannel->pre_filter, params.MaxPreFilters))
              {
                  case  EW_SUCCESS:
                        /* may proceed to trigger processing */
                        if (params.Debug >= 2)
                            logit("ot", "Pre-filter ready for channel %s:%s:%s:%s\n", 
                                thisScnl->sta,thisScnl->chan,thisScnl->net,thisScnl->loc);

                        thisChannel->state = PKCH_STATE_INIT;
                        break;
                        
                  case EW_WARNING:
                        thisChannel->state = PKCH_STATE_BAD;

                        reportError(WD_WARNING_ERROR, NORESULT, "Unable to obtain a find an existing pre-filter or create a new (%s) pre-filter for sample rate %f; setting channel %s:%s:%s:%s bad\n", 
								(isBroadband ? "broadband" : "narrowband"),thisChannel->sampleRate, 
								thisScnl->sta ,thisScnl->chan, thisScnl->net ,thisScnl->loc);
                        break;
                        
                  case EW_FAILURE:
                        thisChannel->state = PKCH_STATE_BAD;

                        reportError(WD_FATAL_ERROR, GENFATERR, "Parameter passed in NULL; setting channel %s:%s:%s:%s bad\n",
                               thisChannel->sampleRate, thisScnl->sta, thisScnl->chan, thisScnl->net,thisScnl->loc);
                        break;

                  default:
                        thisChannel->state = PKCH_STATE_BAD;

                        reportError(WD_WARNING_ERROR, NORESULT, "Unknown error in obtaining a pre-filter for sample rate %f; setting channel %s:%s:%s:%s bad\n", 
                               thisChannel->sampleRate, thisScnl->sta,thisScnl->chan, thisScnl->net,thisScnl->loc);
                        break;
              }
          } /* sufficient sample in pre-filter averaging buffer */
      } /* waiting for complete pre-filter averaging buffer */
   
      if (params.Debug >= 2)
          logit("", "Channel state = %d\n", thisChannel->state);

      /* No longer waiting for buffer, ready to process */
      if (PKCH_STATE_WAIT < thisChannel->state) 
      {   
          /* Process data */
          if ((rc = ProcessChannelMessage(thisScnl, params, EwParameters)) != EW_SUCCESS)
              reportError(WD_DEBUG, NORESULT, "Processing of data message failed for channel %s:%s:%s:%s\n", 
                      thisScnl->sta, thisScnl->chan, thisScnl->net, thisScnl->loc);
      }
    } /* end while */
}

/************************************************
 *              find_SCNL                       *
 *  Finds SCNL in list of desired SCNLs         *
 ************************************************/
SCNL *find_SCNL(TRACE2_HEADER *TraceHeader)
{
    SCNL            key;            /* key for binary search               */
    SCNL           *thisSCNL;       /* pointer to location of desired SCNL */
    int             j;

   /* Check to see if this SCNL is in list
    ***********************************/
    for (j = 0; j < TRACE2_STA_LEN; j++) 
      key.sta[j] = TraceHeader->sta[j];
      key.sta[TRACE2_STA_LEN - 1] = '\0';

    for (j = 0; j < TRACE2_CHAN_LEN; j++) 
      key.chan[j] = TraceHeader->chan[j];
      key.chan[TRACE2_CHAN_LEN - 1] = '\0';
      
    for (j = 0; j < TRACE2_NET_LEN; j++) 
      key.net[j]  = TraceHeader->net[j];
      key.net[TRACE2_NET_LEN - 1] = '\0';

    for (j = 0; j < TRACE2_LOC_LEN; j++) 
      key.loc[j]  = TraceHeader->loc[j];
      key.loc[TRACE2_LOC_LEN - 1] = '\0';

	  /*
      however, since loc currently is not carried in a trace_buf message, 
      for now hardwire net.loc= "--" 
    strcpy(key.loc, "--");
	*/

    thisSCNL = (SCNL *)bsearch(&key, scnl, numSCNL, sizeof(SCNL), CompareSCNLs);

    if (thisSCNL == NULL) /* SCNL not found */
    {
      if (params.Debug >= 3)
          logit("e", "raypicker: SCNL <%s %s %s %s> not found in list of desired SCNLs. \n", 
              key.sta, key.chan, key.net, key.loc);
    }
    else                  /* SCNL found! */
    {
      if (params.Debug >= 3)
          logit("e", "raypicker: SCNL <%s %s %s %s> found in list of selected SCNLs.\n", 
              key.sta, key.chan, key.net, key.loc);
    }

    return thisSCNL;
}

