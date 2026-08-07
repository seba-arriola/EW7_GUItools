/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_apps_putaway.c,v 1.6 2005/04/25 17:06:19 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_apps_putaway.c,v $
 *     Revision 1.6  2005/04/25 17:06:19  davidk
 *     Added include of ewdb_apps_utils.h which includes function prototypes
 *     and now EWDB_SnippetStruct definition.
 *
 *     Revision 1.5  2003/02/04 17:55:45  davidk
 *     Removed superfluous logit
 *
 *     Revision 1.4  2002/04/16 20:39:52  davidk
 *     modified the params for ewdb_api_ProcessSnippetReqs() to fit concierge-2.
 *
 *     Revision 1.2  2001/07/01 21:55:22  davidk
 *     Cleanup of the Earthworm Database API and the applications that utilize it.
 *     The "ewdb_api" was cleanup in preparation for Earthworm v6.0, which is
 *     supposed to contain an API that will be backwards compatible in the
 *     future.  Functions were removed, parameters were changed, syntax was
 *     rewritten, and other stuff was done to try to get the code to follow a
 *     general format, so that it would be easier to read.
 *
 *     Applications were modified to handle changed API calls and structures.
 *     They were also modified to be compiler warning free on WinNT.
 *
 *     Revision 1.1  2001/05/15 02:15:24  davidk
 *     Initial revision
 *
 *     Revision 1.1  2001/02/28 17:29:10  lucky
 *     Initial revision
 *
 *
 */

#include <stdio.h>
#include <string.h>
#include <ewdb_ora_api.h>
#include <ewdb_apps_utils.h>

/********************************************************************
********************************************************************/
int ewdb_apps_putaway_StartSnippetEvent(EWDBid * pidEvent, 
                                        char * sSourceEventID,
                                        char * sAuthor, int Debug)
{
  EWDB_EventStruct Event;
  int RetCode;


  Event.iEventType = 2;
  Event.iDubiocity = 0;
  Event.szComment[0] = 0;
  strcpy(Event.szSource,sAuthor);
  strcpy(Event.szSourceEventID,sSourceEventID);

  RetCode=ewdb_api_CreateEvent(&Event);
  if(RetCode == EWDB_RETURN_FAILURE)
  {
    logit("","Call to ewdb_api_CreateEvent() Failed!\n");
  }
  *pidEvent = Event.idEvent;

  return(RetCode);
}  /* end ewdb_apps_putaway_StartSnippetEvent() */


/********************************************************************
********************************************************************/
int ewdb_apps_putaway_NextSnippetForAnEvent(EWDBid idEvent, 
                                            EWDB_SnippetStruct * pSnippet,
                                            EWDBid * OUT_pidWaveform,
                                            EWDBid * INOUT_pidChan)
{
  EWDB_External_StationStruct Station;
  EWDBid idChan;
  EWDB_WaveformStruct Waveform;
  int RetCode;

  memset(&Station,0,sizeof(EWDB_External_StationStruct));
  strcpy(Station.Station.Sta,pSnippet->sta);
  strcpy(Station.Station.Comp,pSnippet->chan);
  strcpy(Station.Station.Net,pSnippet->net);


  if(!(*INOUT_pidChan))
  {
    
    RetCode=ewdb_api_CreateOrAlterExternalStation(&Station);
    if(RetCode == EWDB_RETURN_FAILURE)
    {
      logit("","Call to ewdb_api_CreateOrAlterExternalStation() Failed!\n");
      return(RetCode);
    }
    
    
    RetCode=ewdb_api_GetIdChanFromStationExternal(&idChan,Station.StationID);
    if(RetCode == EWDB_RETURN_FAILURE)
    {
      logit("","Call to %s() failed for StationID(%d)!\n",
            "ewdb_api_GetIdChanFromStationExternal");
      return(RetCode);
    }
  }
  else
  {
    idChan = *INOUT_pidChan;
  }


  memset(&Waveform,0,sizeof(EWDB_WaveformStruct));

  Waveform.idChan = idChan;
  Waveform.tStart = pSnippet->actStarttime;
  Waveform.tEnd = pSnippet->actEndtime;
  Waveform.iDataFormat = EWDB_WAVEFORM_FORMAT_EW_TRACE_BUF;
  Waveform.iByteLen = pSnippet->actLen;
  Waveform.binSnippet = (char *)(pSnippet->pBuf);

/***
  logit("","ewdb_apps_putaway_NextSnippetForAnEvent: idChan %d, tStart %.4f, tEnd %.4f\n"
        "iDataFormat %d, iByteLen %d, binSnippet %u\n",
        Waveform.idChan, Waveform.tStart, Waveform.tEnd, Waveform.iDataFormat,
        Waveform.iByteLen, Waveform.binSnippet); 
***/

  RetCode=ewdb_api_CreateWaveform(&Waveform,idEvent);

  if(RetCode == EWDB_RETURN_FAILURE)
  {
    logit("","Call to ewdb_api_CreateWaveform() Failed!\n");
  }

  *OUT_pidWaveform = Waveform.idWaveform;
  *INOUT_pidChan = Waveform.idChan;

  return(RetCode);
}

/********************************************************************
********************************************************************/
int ewdb_apps_putaway_NextSnipReqForAnEvent(EWDBid idEvent, 
                                            EWDB_SnippetStruct * pSnippet,
                                            EWDBid * OUT_pidWaveform,
                                            EWDBid * INOUT_pidChan)
{

  return(ewdb_api_ProcessSnippetReqs(pSnippet->sta, pSnippet->chan, 
                                     pSnippet->net, "" /*szLoc*/,
                                     pSnippet->reqStarttime, pSnippet->reqEndtime, 
                                     idEvent, 0/*iNumAttempts*/, 0/*iRequestGroup*/));
  /* DK CLEANUP maybe we should provide a better bridge between the two
     functions than this, even if it means breaking the call param 
     compatability with ewdb_apps_putaway_NextSnipReqForAnEvent() 
     041102 
     *****************************************************************/
}  /* ewdb_apps_putaway_NextSnipReqForAnEvent() */


/********************************************************************
********************************************************************/
int ewdb_apps_putaway_CloseSnippetEvent()
{
  return(EWDB_RETURN_SUCCESS);
} /* end ewdb_apps_putaway_CloseSnippetEvent() */

