/* archive_event.c */

#include <sac_2_ewevent.h>
#include "db_cleanup.h"
#include <ewdb_apps_utils.h>
#include <earthworm_defs.h>

static int LargestSnippetSize;

int ArchiveEvent_init(char * szBaseArchiveDirectory, 
                      char * szOutputFormat, 
                      int IN_iLargestSnippetSize, int iDebug)
{
  if(WriteSAC_init(szBaseArchiveDirectory, szOutputFormat, iDebug) != EW_SUCCESS)
  {
    logit("", "ArchiveEvent_init: Call to WriteSAC_init(%s,%s,%d) failed!\n",
          szBaseArchiveDirectory, szOutputFormat, iDebug);
    return(EW_FAILURE);
  }
  else
  {
    LargestSnippetSize = IN_iLargestSnippetSize;
    return(EW_SUCCESS);
  }
}  /* end ArchiveEvent_init() */


int ArchiveEvent(EWDBid idEvent, int iDataToSave, int * pFailureCode)
{

  static EWEventInfoStruct   DBEvent;
  static EWDB_WaveformStruct *pWaveform;
  int                        iRetCode = EW_SUCCESS;
  int                        i,j;
  int                        SnippetSize;
  int                        iNumChannels=0;

  *pFailureCode = FAILURE_NONE;

  if(!(iDataToSave & SAVE_WAVEFORMS))
  {
    logit("","ERROR! ArchiveEvent() called without SAVE_WAVEFORMS option. " 
             "Function does not support archiving of only parametric data. "
             "Waveforms must be included.\n"
         );
    *pFailureCode = FAILURE_SAVE_NO_WAVEFORMS;
    return(EW_FAILURE);
  }
  /*(re)initialize the DBEvent struct. */
  memset(&DBEvent, 0, sizeof(EWEventInfoStruct));

  if(ewdb_apps_GetDBEventInfo(&DBEvent, idEvent) != EWDB_RETURN_SUCCESS)
  {
    logit("","ArchiveEvent(): ERROR: Call to ewdb_apps_GetDBEventInfo "
             "failed for idEvent %d\n",
          idEvent);
    *pFailureCode = FAILURE_GETDBEVENTINFO;
    return(EW_FAILURE);
  }

  if(WriteSAC_StartEvent(&DBEvent) != EW_SUCCESS)
  {
    logit("","ArchiveEvent(): ERROR: Call to WriteSAC_StartEvent failed "
             "for idEvent %d.\n",
          idEvent);
    *pFailureCode = FAILURE_WRITESAC_STARTEVENT;
    return(EW_FAILURE);
  }

  for(i = 0; i < DBEvent.iNumChans; i++)
  {
    if(iDataToSave & SAVE_WAVEFORMS)
    {
      for(j = 0; j < DBEvent.pChanInfo[i].iNumWaveforms; j++)
      {
        /* Retrieve snippets for this channel */
        pWaveform = &(DBEvent.pChanInfo[i].Waveforms[j]);
        
        /* Set snippetsize based on ByteLen and LargestSnippetSize */
        SnippetSize = pWaveform->iByteLen;
        if(SnippetSize > LargestSnippetSize)
        {
          SnippetSize = LargestSnippetSize;
          logit("","WARNING!  Truncated Waveform(%d) from %d to %d bytes.\n",
                pWaveform->idWaveform, pWaveform->iByteLen, LargestSnippetSize);
        }

        pWaveform->binSnippet = malloc(SnippetSize);
        
        if(ewdb_api_GetWaveformSnippet(pWaveform->idWaveform, 
          pWaveform->binSnippet, SnippetSize) 
          != EWDB_RETURN_SUCCESS)
        {
          logit("", "ArchiveEvent(): ERROR!! Call to %s failed "
                    "for Event %d, Chan %d, Waveform %d.\n",
                "ewdb_api_GetWaveformSnippet()", idEvent, i, j);
          free(pWaveform->binSnippet);
          DBEvent.pChanInfo[i].iNumWaveforms = 0;
          
          *pFailureCode = FAILURE_GETWAVEFORM_SNIPPET;
          iRetCode = EW_WARNING;
        }
        
      } /* end for loop over waveforms */
    }  /* end if SAVE_WAVEFORMS */
    
    if(DBEvent.pChanInfo[i].iNumWaveforms > 0)
    {
      if(ProduceSAC_NextStationForEvent(&(DBEvent.pChanInfo[i])) != EW_SUCCESS)
      {
        logit("","ArchiveEvent(): ERROR: Call to %s failed for Channel %d, "
                 "for idEvent %d.\n",
              "ProduceSAC_NextStationForEvent()", i, idEvent);

        for(j = 0; j < DBEvent.pChanInfo[i].iNumWaveforms; j++)
        {
          /* Retrieve snippets for this channel */
          pWaveform = &(DBEvent.pChanInfo[i].Waveforms[j]);
          if(pWaveform)
            free(pWaveform);
        }

        *pFailureCode = FAILURE_PRODUCESAC_NEXTSTATION;
        return(EW_FAILURE);
      }  /* end if ProduceSAC_NextStationForEvent() failed */
      
      
      if(WriteSAC_NextStationForEvent(&(DBEvent.pChanInfo[i])) != EW_SUCCESS)
      {
        logit("","ArchiveEvent(): ERROR: Call to %s failed for Channel %d, "
                 "for idEvent %d.\n",
              "WriteSAC_NextStationForEvent()", i, idEvent);

        for(j = 0; j < DBEvent.pChanInfo[i].iNumWaveforms; j++)
        {
          /* Retrieve snippets for this channel */
          pWaveform = &(DBEvent.pChanInfo[i].Waveforms[j]);
          if(pWaveform)
            free(pWaveform);
        }

        *pFailureCode = FAILURE_WRITESAC_NEXTSTATION;
        return(EW_FAILURE);
      }  /* end if ProduceSAC_NextStationForEvent() failed */
            
           
      /* Free up allocated snippet space */
      for(j = 0; j < DBEvent.pChanInfo[i].iNumWaveforms; j++)
      {
        pWaveform = &(DBEvent.pChanInfo[i].Waveforms[j]);
        free(pWaveform->binSnippet);
        pWaveform->binSnippet = NULL;
      }
      iNumChannels++;
            
    } /* end if waveforms for this channel */
  } /* end for each channel in the DBEvent struct */
  
  logit("t","Successfully archived %d of %d channels for Event %d\n",
        iNumChannels, DBEvent.iNumChans, idEvent);

  if(WriteSAC_EndEvent() != EW_SUCCESS)
  {
    logit("","ArchiveEvent(): ERROR: Call to %s failed for idEvent %d.\n",
          "WriteSAC_NextStationForEvent()", i, idEvent);
    *pFailureCode = FAILURE_WRITESAC_ENDEVENT;
    return(EW_FAILURE);
  }
  
  /* free up channel space alloced in ewdb_apps_GetDBEventInfo
  ********************************************************/
  if(DBEvent.pChanInfo)
      free(DBEvent.pChanInfo);
  
  DBEvent.iNumAllocChans = 0;

  return(iRetCode);
}  /* end ArchiveEvent() */
	
