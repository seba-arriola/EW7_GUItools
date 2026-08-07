
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_apps_GetDBEventInfo.c,v 1.16 2003/09/04 17:20:59 lucky Exp $
 *
 *    Revision history:
 *     $Log: ewdb_apps_GetDBEventInfo.c,v $
 *     Revision 1.16  2003/09/04 17:20:59  lucky
 *     Modified Mag retrieval so that we don't return ERROR where there are more Mag records
 *     than we have room to store.  Now, we'll politely warn the user, and continue on.
 *
 *     Revision 1.15  2003/07/01 19:32:20  patton
 *     added support for magnitudes other than
 *     Md and Ml.
 *     JMP 7-1-2003
 *     3
 *
 *     Revision 1.14  2002/09/10 17:17:04  lucky
 *     Stable scaffold
 *
 *     Revision 1.13  2002/07/16 19:44:48  davidk
 *     Added code to check the current number of parametric data values
 *     for a channel against the number allocated(MDPCPE) before adding
 *     parametric data to the list for the current channel, thus preventing
 *     the code from writing data passed the end of the array and causing
 *     serious problems.
 *
 *     Revision 1.12  2002/06/28 21:06:22  lucky
 *     Lucky's pre-departure checkin. Most changes probably had to do with bug fixes
 *     in connection with the GIOC scaffold.
 *
 *     Revision 1.11  2002/05/28 17:25:41  lucky
 *     *** empty log message ***
 *
 *     Revision 1.10  2001/09/26 21:41:09  lucky
 *     Added support for arranging channels by station. Added code to
 *     build a station array.
 *
 *     Revision 1.9  2001/09/10 20:45:57  lucky
 *     Added support for grouping according to station
 *
 *     Revision 1.8  2001/07/25 22:32:12  lucky
 *     Fixed a cut-and-paste error with bValidResponse flag
 *
 *     Revision 1.6  2001/07/14 07:38:30  davidk
 *     Commented out NumChans logging statements, that filled up makefiles.
 *     Modified the return code handling for ewdb_api_GetTransformFunctionForChan()
 *     so that the code does not log superfluous error messages just because a channel
 *     doesn't have a transfer function.  Removed a superfluous free() call that was
 *     causing the program to crash under NT.
 *
 *     Revision 1.5  2001/07/01 21:55:22  davidk
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
 *     Revision 1.4  2001/06/21 21:37:20  lucky
 *     Fixed magtypes for Local Magnitudes
 *
 *     Revision 1.3  2001/06/21 21:25:00  lucky
 *     State of the code after LocalMag review was implemented and partially tested.
 *
 *     Revision 1.2  2001/06/06 20:54:46  lucky
 *     Changes made to support multitude magnitudes, as well as amplitude picks. This is w
 *     in progress - checkin in for the sake of safety.
 *
 *     Revision 1.1  2001/05/15 02:15:23  davidk
 *     Initial revision
 *
 *     Revision 1.20  2001/02/28 17:29:10  lucky
 *     Massive schema redesign and cleanup.
 *
 *     Revision 1.19  2001/02/06 23:02:34  lucky
 *     CHanged if(1) to if(0) in the infrastructure part -- memory leak in the new stuff
 *
 *     Revision 1.18  2001/01/20 18:57:32  davidk
 *     fixed many logit calls that compiled by did not have a proper flag parameter.
 *
 *     fixed a problem in GetDBEventWaveformInfo(), GetDBEventStaMagInfo(),
 *     and GetDBEventArrivalInfo() where the local buffer variables that are
 *     used for internal calls were not pointed at the global buffer, and thus
 *     bombed NULL checks.
 *     >   // copy the global buffer info to our local vars
 *     >   pBuffer = *ppBuffer;
 *     >   BufferSize = *pBufferSize;
 *
 *     fixed a problem in the routines that retrieve the actual waveform snippets.
 *     Now if they cannot retrieve a snippet, they retrieve any traces of the
 *     waveform descriptor that contained info for the snippet.
 *
 *     Revision 1.17  2001/01/11 22:47:16  davidk
 *     Tried to clean up a bunch of code by placing it into functions.  ewdb_apps_GetDBEventInfoII() is
 *     still 400+ lines, but that is much better than the 1000+ it was before.  Attempted to
 *     fix bug uncovered in v1.15(see v1.16 note).  Tried to make as much of the code that
 *     retrieves arrivals, station magnitudes, and waveform descriptors, and adds them to the
 *     event data, common to all three, instead of having separate sets of common functionality
 *     for each of the three types of data.
 *
 *     Revision 1.16  2001/01/11 17:52:45  lucky
 *     Fixed a bug with allocating more channels. When a new channel is inserted in the
 *     middle of other channels, we have to allocate the extra channels one channel
 *     earlier. This is due to the structure of the k-loop which moves channels
 *     and inserts in place.
 *
 *     Revision 1.15  2001/01/04 01:43:26  davidk
 *     Fixed a bug where multiple arrivals/magnitudes/waveformdescs from the same
 *     channel were appended as different channels at the end of the event channel
 *     list, when the complete original list of channels for the event had already
 *     been traversed.  This problem was most likely to appear when a channel
 *     had two picks for an event.  Each pick would get recorded as a separate channel.
 *     This occured because once the list of channels for an event had been traversed,
 *     each new pick/stationmag/waveformdesc was added as a separate channel.  To fix
 *     this, j(the channel counter for the event channel list) is set to the last element
 *     in the list whenever a new channel is appended to the end of the list.  So that the
 *     next pick/stationmag/waveformdesc is compared against the last channel in the
 *     event channel list, instead of being assumed to be new.
 *
 *     Revision 1.14  2000/12/21 01:28:32  davidk
 *     Replaced ewdb_apps_GetDBEventInfo() with ewdb_apps_GetDBEventInfoII() so that the caller
 *     can now specify what types of information it wants for an event.
 *     Also changed the method of retrieving component information, so that
 *     component information for all channels in the db is retrieved in a single
 *     query and then parsed in the program, instead of getting component information
 *     one channel at a time (one channel/query).  ewdb_apps_GetDBEventInfo() should be
 *     backwards compatible so pre-existing code should still work, but
 *     ewdb_apps_GetDBEventInfo() calls ewdb_apps_GetDBEventInfoII() so new code should call
 *     the Super function as well.
 *
 *     Revision 1.13  2000/10/16 17:24:32  lucky
 *     Removed DEBUG statements.
 *     Check all DB API calls against EWDB_RETURN_FAILURE instead of SUCCESS -- the API
 *     calls return three types of values, with WARNING returned where the Buffer was
 *     filled, but some data couldn't fit.
 *
 *     Revision 1.12  2000/10/10 20:28:46  lucky
 *     Added the call to InitEWEventInfo
 *
 *     Revision 1.11  2000/08/30 17:41:20  lucky
 *     We now (again) allocate channel space on the fly.
 *
 *     Revision 1.10  2000/08/28 19:57:25  lucky
 *     Free the space allocated for binSnippet BEFORE re-initializing the structure.
 *
 *     Revision 1.9  2000/08/28 17:02:29  lucky
 *     *** empty log message ***
 *
 *     Revision 1.8  2000/08/28 15:36:21  lucky
 *     We don't want to return an error when certain functions fail -- just
 *     log the error and continue.
 *
 *     Revision 1.6  2000/08/25 18:19:45  lucky
 *     Major cleanup: pChanInfo is expressly allocated so we no longer need the
 *     many callocs and mallocs. Also, we now check return values of DB calls.
 *
 *     Revision 1.5  2000/06/21 20:41:56  lucky
 *     cleaned-up a whole bunch of timing/debug messages that made it difficult to read log files.
 *
 *     Revision 1.4  2000/06/07 22:15:36  lucky
 *     *** empty log message ***
 *
 *
 */


#include <string.h>
#include <math.h>
#include <time_ew.h>
#include <earthworm.h>
#include <ewdb_apps_utils.h>

int    GetDBDebug=0;


/************************
  Utilities
************************/

static int MergeListDataWithEvent(void * pBuffer, int BufferSize, 
                           int NumItemsInList, 
                           EWEventInfoStruct * pEventInfo, 
                           int ( *GetidChanFunction )
                               (const void * pBuffer, int iArrayOffset),
                           int ( *AddListDataFunction )
                               (EWChannelDataStruct * pChannel, 
                                const void * pBuffer, int iArrayOffset)
                          );
static int ReplaceBuffer(void ** ppBuffer, int OldBufferSize, int NewBufferSize, 
                  int bCopyBuffer);

/************************
  Arrivals
************************/
static int CompareArrivalChans( const void *arg1, const void *arg2 );
static int GetArrivals(char ** ppBuffer, int * pBufferSize, 
                       EWDBid idOrigin, int * pNumItemsRetrieved);
static EWDBid GetidChanForArrivalStruct(const void * pBuffer, int iArrayOffset);
static int AddArrivalToEWChannelDataStruct(EWChannelDataStruct * pChannel, 
                                           const void * pBuffer, int iArrayOffset);
static int GetDBEventArrivalInfo(void ** ppBuffer, int * pBufferSize, 
                                 EWEventInfoStruct * pEventInfo);
int compStaDistance (const void*, const void*);


/************************
  Triggers
************************/
static int CompareTriggerChans( const void *arg1, const void *arg2 );
static int GetTriggers(char ** ppBuffer, int * pBufferSize,
                       EWDBid idCoincidence, int * pNumItemsRetrieved);
static EWDBid GetidChanForTriggerStruct(const void * pBuffer, int iArrayOffset);
static int AddTriggerToEWChannelDataStruct(EWChannelDataStruct * pChannel,
                                           const void * pBuffer, int iArrayOffset);
static int GetDBEventTriggerInfo(void ** ppBuffer, int * pBufferSize,
                                 EWEventInfoStruct * pEventInfo);


/************************
  StaMags
************************/
static int CompareStaMagChans( const void *arg1, const void *arg2 );
static int GetStaMags(char ** ppBuffer, int * pBufferSize, 
                       EWDB_MagStruct *pMag, int * pNumItemsRetrieved);
static EWDBid GetidChanForStaMagStruct(const void * pBuffer, int iArrayOffset);
static int AddStaMagToEWChannelDataStruct(EWChannelDataStruct * pChannel, 
                                           const void * pBuffer, int iArrayOffset);
static int GetDBEventStaMagInfo(void ** ppBuffer, int * pBufferSize, 
                                 EWEventInfoStruct * pEventInfo);


/************************
  Waveform Descs
************************/
static int CompareWaveformChans( const void *arg1, const void *arg2 );
static int GetWaveformDescriptors(char ** ppBuffer, int * pBufferSize, 
                       EWDBid idEvent, int * pNumItemsRetrieved);
static EWDBid GetidChanForWaveformStruct(const void * pBuffer, int iArrayOffset);
static int AddWaveformToEWChannelDataStruct(EWChannelDataStruct * pChannel, 
                                           const void * pBuffer, int iArrayOffset);
static int GetDBEventWaveformInfo(void ** ppBuffer, int * pBufferSize, 
                                 EWEventInfoStruct * pEventInfo);



/***************************
############################
##   UTILITIES SECTION    ##
############################
***************************/

/*************************************************************************
*************************************************************************/
int ewdb_apps_Set_GetDBEventInfo_Debug(int bDebug)
{
   GetDBDebug = bDebug;
   return(EWDB_RETURN_SUCCESS);
}  /* end ewdb_apps_Set_GetDBEventInfo_Debug() */


/*************************************************************************
*************************************************************************/
static int ReplaceBuffer(void ** ppBuffer, int OldBufferSize, int NewBufferSize, 
                  int bCopyBuffer)
{
  /*******************************************************
  ReplaceBuffer() creates a new buffer in place of an old
  one.  ReplaceBuffer() frees the buffer pointed to by 
  (*ppBuffer), creates a new buffer of size NewBufferSize
  bytes, and sets (*ppBuffer) to point at the new buffer.
  It's like a realloc, but the buffer pointer changes.
  *******************************************************/

  void * pBufferOld, *pBufferNew;
  
  /* check input params, we require that the old buffer be valid */
  if(!ppBuffer || !(*ppBuffer))
  {
    logit("t","ReplaceBuffer(): ERROR NULL inputs!\n");
    return(EWDB_RETURN_FAILURE);
  }
  
  pBufferOld = *ppBuffer;
  
  /* allocate the new buffer */
  pBufferNew = malloc(NewBufferSize);
  if(!pBufferNew)
  {
    logit("t","ReplaceBuffer(): malloc failed for %d bytes.\n", NewBufferSize);
    return(EWDB_RETURN_FAILURE);
  }
  /* else */
  
  /* Copy the contents of the old buffer to the new buffer if desired */
  if(bCopyBuffer)
  {
    memcpy(pBufferNew,pBufferOld,OldBufferSize);
  }
  
  /* free the old buffer */
  free(pBufferOld);
  
  /* point the caller's pointer at the new buffer */
  *ppBuffer = pBufferNew;
  
  return(EWDB_RETURN_SUCCESS);
}  /* end ReplaceBuffer() */


/*************************************************************************
*************************************************************************/
static int MergeListDataWithEvent(void * pBuffer, int BufferSize, 
                           int NumItemsInList, 
                           EWEventInfoStruct * pEventInfo, 
                           int ( *GetidChanFunction )
                               (const void * pBuffer, int iArrayOffset),
                           int ( *AddListDataFunction )
                               (EWChannelDataStruct * pChannel, 
                                const void * pBuffer, int iArrayOffset))
{
  
  int i,j,k;
  int toalloc;
  int tmp;
  int rc, ret;

  /* for each pick retrieved, see if it matches any of our current channels */
  /* assume that both data sets are sorted by idChan increasing*/
  
  /* start at the beginning of the arrival struct array */
  i=0;
  
  /* loop through the event channel array */

  for(j=0;i<NumItemsInList;j++)
  {
    if( i >= NumItemsInList)
      break;
    if(j < pEventInfo->iNumChans)
    {

      while( GetidChanFunction(pBuffer,i)<= pEventInfo->pChanInfo[j].idChan)
      {
        if( i >= NumItemsInList)
          break;
        if(pEventInfo->pChanInfo[j].idChan == GetidChanFunction(pBuffer,i))
        {

          AddListDataFunction(&(pEventInfo->pChanInfo[j]), pBuffer, i);
        }  /* if datalist.idChan = eventbuffer.idchan */
        else
        {  
          /* we have a new channel, we need to insert it in place */
          
          /* check to see if we have enough room in our event channel array */
          if(pEventInfo->iNumChans >= pEventInfo->iNumAllocChans)
          {
            /* we need to allocate more */
            
            toalloc = (int) (pEventInfo->iNumAllocChans + CHAN_ALLOC_INCR);
            
            logit ("", "Max Channels (%d) reached. Alloc to %d\n", 
                   pEventInfo->iNumAllocChans, toalloc);
            
            rc= ReplaceBuffer((void **)&(pEventInfo->pChanInfo),
                              pEventInfo->iNumAllocChans * sizeof (EWChannelDataStruct),
                              toalloc * sizeof (EWChannelDataStruct), TRUE
                             );
            if(rc == EWDB_RETURN_FAILURE)
            {
              logit("t","MergeListDataWithEvent(): ERROR %s failed!\n",
                "ReplaceBuffer()");
              ret = EWDB_RETURN_FAILURE;
              goto Abort;
            }

            for (tmp = pEventInfo->iNumChans; tmp < toalloc; tmp++)
            {
              InitEWChan(&pEventInfo->pChanInfo[tmp]);
            }
            
            pEventInfo->iNumAllocChans = toalloc;
            
          }  /* end if iNumChans >= iNumAllocChans */
          
          /* make room for the new record  move every record after j, back 1*/
          for(k=pEventInfo->iNumChans;k>j;k--)
          {
            memcpy(&(pEventInfo->pChanInfo[k]),&(pEventInfo->pChanInfo[k-1]),
              sizeof(pEventInfo->pChanInfo[k]));
          }

          /* reinitialize the channel structure at j */
          InitEWChan(&(pEventInfo->pChanInfo[j]));
          
          pEventInfo->pChanInfo[j].idChan=GetidChanFunction(pBuffer,i);
          
          AddListDataFunction(&(pEventInfo->pChanInfo[j]), pBuffer, i);
          pEventInfo->iNumChans++;
          
        }     /* end else   (pBuffer.idChan < eventbuffer.idchan) */
        i++;  /* move to the next data item in pBuffer */
      }       /* end while (pBuffer.idChan <= eventbuffer.idchan) */
    }         /* end if j < iNumChans  (we've already gone 
                 through all pre-existing chans */
    else
    {
      /* we have a new channel, we need to append it to the end of the list */
      
      /* check to see if we have enough room in our event channel array */
      if(pEventInfo->iNumChans >= pEventInfo->iNumAllocChans)
      {
        /* we need to allocate more */
        toalloc = (int) (pEventInfo->iNumAllocChans + CHAN_ALLOC_INCR);
        
        logit ("", "Max Channels (%d) reached. Alloc to %d\n", 
               pEventInfo->iNumAllocChans, toalloc);
        
        rc=ReplaceBuffer((void **)&(pEventInfo->pChanInfo),
                         pEventInfo->iNumAllocChans * sizeof (EWChannelDataStruct),
                         toalloc * sizeof (EWChannelDataStruct), TRUE
                        );
        if(rc == EWDB_RETURN_FAILURE)
        {
          logit("t","MergeListDataWithEvent(): ERROR %s failed!\n",
            "ReplaceBuffer(2)");
          ret = EWDB_RETURN_FAILURE;
          goto Abort;
        }

        for (tmp = pEventInfo->iNumChans; tmp < toalloc; tmp++)
        {
          InitEWChan (&pEventInfo->pChanInfo[tmp]);
        }
        
        pEventInfo->iNumAllocChans = toalloc;
        
      }  /* end if iNumChans >= iNumAllocChans */
      
      pEventInfo->pChanInfo[j].idChan=GetidChanFunction(pBuffer,i);
      
      AddListDataFunction(&(pEventInfo->pChanInfo[j]), pBuffer, i);
      pEventInfo->iNumChans ++;
      
      /* set j so that it will be the last channel in the Event list once
      we get back to the top of the loop (i<numItemsRetrieved).  To do
      this, we set j= pEventInfo->iNumChans -1 (since our array starts
      at 0 not 1.  Then ew must decrement j here, because it will be
      incremented at the top of the loop.  Kind of a hack, but everything
      else works so well together.  DK 2001/01/03*/
      j = pEventInfo->iNumChans - 1 - 1;  
      
      i++;
    }  /* end else   if j < iNumChans */
  }         /* end for i in NumItemsInList */

  ret = EWDB_RETURN_SUCCESS;
  
Abort:
  return(ret);

}  /* end MergeListDataWithEvent() */


/***************************
############################
## END UTILITIES SECTION  ##
############################
***************************/


/***************************
############################
##    ARRIVALS SECTION    ##
############################
***************************/

/*************************************************************************
*************************************************************************/
static int CompareArrivalChans( const void *arg1, const void *arg2 )
{
  EWDB_ArrivalStruct *par1=(EWDB_ArrivalStruct *)arg1;
  EWDB_ArrivalStruct *par2=(EWDB_ArrivalStruct *)arg2;
  if(par1->idChan < par2->idChan)
    return(-1);
  else if (par1->idChan == par2->idChan)
    return(0);
  else
    return(1);
}


/*************************************************************************
*************************************************************************/
int CompareArrivalDistsForChan( const void *arg1, const void *arg2 )
{
  EWChannelDataStruct *pcds1=(EWChannelDataStruct *)arg1;
  EWChannelDataStruct *pcds2=(EWChannelDataStruct *)arg2;
  float dist1,dist2;

  /* calculate dist1, use maxint if there is no arrival struct */
  if(pcds1->iNumArrivals == 0)
    dist1=(float)0xFFFFFFFF;
  else
    dist1=pcds1->Arrivals[0].dDist;

  /* calculate dist2, use maxint if there is no arrival struct */
  if(pcds2->iNumArrivals == 0)
    dist2=(float)0xFFFFFFFF;
  else
    dist2=pcds2->Arrivals[0].dDist;

  if(dist1 < dist2)
    return(-1);
  else if (dist1 == dist2)
    return(0);
  else
    return(1);
}


/*************************************************************************
*************************************************************************/
EWDBid GetidChanForArrivalStruct(const void * pBuffer, int iArrayOffset)
{
  return(((EWDB_ArrivalStruct *)pBuffer)[iArrayOffset].idChan);
  
}


/*************************************************************************
*************************************************************************/
int AddArrivalToEWChannelDataStruct(EWChannelDataStruct * pChannel, 
                                    const void * pBuffer, int iArrayOffset)
{
  int tmp;
  
  tmp = pChannel->iNumArrivals;
  if(tmp < MDPCPE)
  {
    memcpy(&(pChannel->Arrivals[tmp]), 
           &(((EWDB_ArrivalStruct *)pBuffer)[iArrayOffset]),
           sizeof(EWDB_ArrivalStruct));
    pChannel->iNumArrivals++;
  }
  /* else  WE SHOULD PROBABLY RETURN WARNING, to indicate that we
           had too much data, and arrival didn't get added.
     DK 071002
   ************************************************************/
  
  return(EWDB_RETURN_SUCCESS);
}


/*************************************************************************
*************************************************************************/
static int GetArrivals(char ** ppBuffer, int * pBufferSize, 
                       EWDBid idOrigin, int * pNumItemsRetrieved)
{
  int NumItemsFound, NumItemsRetrieved;
  int BufferSize;
  char * pBuffer;
  int ret;
  
  if(!(ppBuffer && (*ppBuffer) && pBufferSize && pNumItemsRetrieved))
  {
    logit("t","%s:  Error NULL input param\n", "GetArrivals()");
    return(EWDB_RETURN_FAILURE);
  }
  

	if (idOrigin < 0)
	{
		logit ("", "GetArrivals: idOrigin is %d; skipping Pick retrieval.\n", idOrigin);
		return EWDB_RETURN_SUCCESS;
	}
		
  /* set pBuffer and BufferSize */
  pBuffer    = *ppBuffer;
  BufferSize = *pBufferSize;
  
  /* Retrieve the Arrivals for this event. */

  if(ewdb_api_GetArrivals(idOrigin,(EWDB_ArrivalStruct *)pBuffer,
    &NumItemsFound,&NumItemsRetrieved, 
    BufferSize/sizeof(EWDB_ArrivalStruct)) == EWDB_RETURN_FAILURE)
  {
    logit ("", "%s:  Call to ewdb_api_GetArrivals failed.\n",
      "GetDBEventArrivalInfo()");
    ret = EWDB_RETURN_FAILURE;
    goto Abort;
  }
  
  
  /* check to see if our buffer was large enough to retrieve all the arrivals */
  if(NumItemsFound > NumItemsRetrieved)
  {  
    if(ReplaceBuffer( (void**)(&pBuffer),
      0, NumItemsFound * sizeof(EWDB_ArrivalStruct), FALSE)
      != EWDB_RETURN_SUCCESS)
    {
      logit("","GetDBEventArrivalInfo(): ERROR %s failed!\n", 
        "ReplaceBuffer()");
      ret = EWDB_RETURN_FAILURE;
      goto Abort;
    }
    
    BufferSize = NumItemsFound * sizeof(EWDB_ArrivalStruct);
    
    
    /* GetArrivals() attempt 2 */
    if(ewdb_api_GetArrivals(idOrigin,(EWDB_ArrivalStruct *)pBuffer,
      &NumItemsFound,&NumItemsRetrieved, 
      BufferSize/sizeof(EWDB_ArrivalStruct)) == EWDB_RETURN_FAILURE)
    {
      logit ("", "%s:  Call(2) to ewdb_api_GetArrivals failed.\n",
        "GetDBEventArrivalInfo()");
      ret = EWDB_RETURN_FAILURE;
      goto Abort;
    }
    
  }  /* end  if(NumItemsFound > NumItemsRetrieved) */

  ret = EWDB_RETURN_SUCCESS;
  
Abort:
  *ppBuffer    = pBuffer;
  *pBufferSize = BufferSize;
  *pNumItemsRetrieved = NumItemsRetrieved;
  
  return(ret);
}  /* end GetArrivals() */


/*************************************************************************
*************************************************************************/
int GetDBEventArrivalInfo(void ** ppBuffer, int * pBufferSize, 
                          EWEventInfoStruct * pEventInfo)
{  
  
  char * pBuffer;
  int BufferSize;
  int ret,rc;
  int NumItemsRetrieved;

  if(!(ppBuffer && *ppBuffer && pBufferSize && pEventInfo))
  {
    logit("t", "GetDBEventArrivalInfo(): ERROR:  NULL input params!\n");
    return(EWDB_RETURN_FAILURE);
  }

  /* copy the global buffer info to our local vars */
  pBuffer = *ppBuffer;
  BufferSize = *pBufferSize;

	if (pEventInfo->PrefOrigin.idOrigin < 0)
	{
		logit ("", "idOrigin is %d; skipping retrieval of Picks.\n",
				pEventInfo->PrefOrigin.idOrigin);
		return EWDB_RETURN_SUCCESS;
	}


  rc = GetArrivals(&pBuffer, &BufferSize, pEventInfo->PrefOrigin.idOrigin,
                   &NumItemsRetrieved);

  if(rc == EWDB_RETURN_FAILURE)
  {
    logit("t","GetDBEventArrivalInfo():  GetArrivals() failed!\n");
    ret = EWDB_RETURN_FAILURE;
    goto Abort;
  }

  /* sort the list of arrivals */
  qsort(pBuffer,NumItemsRetrieved,sizeof(EWDB_ArrivalStruct),CompareArrivalChans);


  /* add the arrivals the event info list */
  rc = MergeListDataWithEvent(pBuffer, BufferSize, NumItemsRetrieved, 
                              pEventInfo, GetidChanForArrivalStruct, 
                              AddArrivalToEWChannelDataStruct);

  if(rc == EWDB_RETURN_FAILURE)
  {
    logit("t","GetDBEventArrivalInfo(): ERROR %s failed!\n",
          "MergeListDataWithEvent()");
    ret = EWDB_RETURN_FAILURE;
    goto Abort;
  }

  ret = EWDB_RETURN_SUCCESS;

Abort:
  *ppBuffer = pBuffer;
  *pBufferSize = BufferSize;
  
  return(ret);
}


/***************************
############################
## END ARRIVALS SECTION   ##
############################
***************************/


/***************************
############################
##    TRIGGERS SECTION    ##
############################
***************************/

/*************************************************************************
*************************************************************************/
static int CompareTriggerChans( const void *arg1, const void *arg2 )
{
  EWDB_TriggerStruct *par1=(EWDB_TriggerStruct *)arg1;
  EWDB_TriggerStruct *par2=(EWDB_TriggerStruct *)arg2;
  if(par1->idChan < par2->idChan)
    return(-1);
  else if (par1->idChan == par2->idChan)
    return(0);
  else
    return(1);
}


/*************************************************************************
*************************************************************************/
int CompareTriggerDistsForChan( const void *arg1, const void *arg2 )
{
  EWChannelDataStruct *pcds1 = (EWChannelDataStruct *)arg1;
  EWChannelDataStruct *pcds2 = (EWChannelDataStruct *)arg2;
  float dist1,dist2;

  /* calculate dist1, use maxint if there is no arrival struct */
  if(pcds1->iNumTriggers == 0)
    dist1=(float)0xFFFFFFFF;
  else
    dist1 = (float) pcds1->Triggers[0].dDist;

  /* calculate dist2, use maxint if there is no arrival struct */
  if(pcds2->iNumTriggers == 0)
    dist2=(float)0xFFFFFFFF;
  else
    dist2 = (float) pcds2->Triggers[0].dDist;

  if(dist1 < dist2)
    return(-1);
  else if (dist1 == dist2)
    return(0);
  else
    return(1);
}


/*************************************************************************
*************************************************************************/
EWDBid GetidChanForTriggerStruct(const void * pBuffer, int iArrayOffset)
{
  return(((EWDB_TriggerStruct *)pBuffer)[iArrayOffset].idChan);
  
}


/*************************************************************************
*************************************************************************/
int AddTriggerToEWChannelDataStruct(EWChannelDataStruct * pChannel, 
                                    const void * pBuffer, int iArrayOffset)
{
  int tmp;
  
  tmp = pChannel->iNumTriggers;
  if(tmp < MDPCPE)
  {
    memcpy(&(pChannel->Triggers[tmp]), 
           &(((EWDB_TriggerStruct *)pBuffer)[iArrayOffset]),
           sizeof(EWDB_TriggerStruct));
    pChannel->iNumTriggers++;
  }
  /* else  WE SHOULD PROBABLY RETURN WARNING, to indicate that we
           had too much data, and trigger didn't get added.
     DK 071002
   ************************************************************/
  
  return(EWDB_RETURN_SUCCESS);
}


/*************************************************************************
*************************************************************************/
static int GetTriggers(char ** ppBuffer, int * pBufferSize, 
                       EWDBid idCoincidence, int * pNumItemsRetrieved)
{
  int NumItemsFound, NumItemsRetrieved;
  int BufferSize;
  char * pBuffer;
  int ret;
  
  if(!(ppBuffer && (*ppBuffer) && pBufferSize && pNumItemsRetrieved))
  {
    logit("t","%s:  Error NULL input param\n", "GetTriggers()");
    return(EWDB_RETURN_FAILURE);
  }

	if (idCoincidence < 0)
	{
/*
		logit ("", "GetTriggers: idCoincidence = %d; skipping Trigger retrieval\n",
						idCoincidence);
*/
		return EWDB_RETURN_SUCCESS;
	}
  
  /* set pBuffer and BufferSize */
  pBuffer    = *ppBuffer;
  BufferSize = *pBufferSize;
  
  /* Retrieve the Triggers for this event. */
  if (ewdb_api_GetTriggerList (idCoincidence, (EWDB_TriggerStruct *)pBuffer,
		BufferSize/sizeof(EWDB_TriggerStruct), (double) -1.0, (double) -1.0, 
	    &NumItemsFound, &NumItemsRetrieved) == EWDB_RETURN_FAILURE)
  {
    logit ("", "%s:  Call to ewdb_api_GetTriggerList failed.\n",
   										   "GetDBEventTriggerInfo()");
    ret = EWDB_RETURN_FAILURE;
    goto Abort;
  }
  
  /* check to see if our buffer was large enough to retrieve all the arrivals */
  if(NumItemsFound > NumItemsRetrieved)
  {  
    if(ReplaceBuffer( (void**)(&pBuffer),
      0, NumItemsFound * sizeof(EWDB_TriggerStruct), FALSE)
      != EWDB_RETURN_SUCCESS)
    {
      logit("","GetDBEventTriggerInfo(): ERROR %s failed!\n", 
        "ReplaceBuffer()");
      ret = EWDB_RETURN_FAILURE;
      goto Abort;
    }
    
    BufferSize = NumItemsFound * sizeof(EWDB_TriggerStruct);
    
    /* GetTriggers() attempt 2 */
    if (ewdb_api_GetTriggerList (idCoincidence, (EWDB_TriggerStruct *)pBuffer,
		BufferSize/sizeof(EWDB_TriggerStruct), (double) -1.0, (double) -1.0, 
	    &NumItemsFound, &NumItemsRetrieved) == EWDB_RETURN_FAILURE)
    {
      logit ("", "%s:  Call(2) to ewdb_api_GetTriggerList failed.\n",
        "GetDBEventTriggerInfo()");
      ret = EWDB_RETURN_FAILURE;
      goto Abort;
    }
  }  /* end  if(NumItemsFound > NumItemsRetrieved) */

  ret = EWDB_RETURN_SUCCESS;
  
Abort:
  *ppBuffer    = pBuffer;
  *pBufferSize = BufferSize;
  *pNumItemsRetrieved = NumItemsRetrieved;
  
  return(ret);
}  /* end GetTriggers() */


/*************************************************************************
*************************************************************************/
int GetDBEventTriggerInfo(void ** ppBuffer, int * pBufferSize, 
                          EWEventInfoStruct * pEventInfo)
{  
  
  char * pBuffer;
  int BufferSize;
  int ret,rc;
  int NumItemsRetrieved;

  if(!(ppBuffer && *ppBuffer && pBufferSize && pEventInfo))
  {
    logit("t", "GetDBEventTriggerInfo(): ERROR:  NULL input params!\n");
    return(EWDB_RETURN_FAILURE);
  }

  /* copy the global buffer info to our local vars */
  pBuffer = *ppBuffer;
  BufferSize = *pBufferSize;

	if (pEventInfo->CoincEvt.idCoincidence < 0)
	{
/*
		logit ("", "GetTriggers: idCoincidence = %d; skipping Trigger retrieval\n",
						pEventInfo->CoincEvt.idCoincidence);
*/
		return EWDB_RETURN_SUCCESS;
	}

  rc = GetTriggers(&pBuffer, &BufferSize, pEventInfo->CoincEvt.idCoincidence,
                   &NumItemsRetrieved);
  
  if(rc == EWDB_RETURN_FAILURE)
  {
    logit("t","GetDBEventTriggerInfo():  GetTriggers() failed!\n");
    ret = EWDB_RETURN_FAILURE;
    goto Abort;
  }
  
  /* sort the list of arrivals */
  qsort(pBuffer,NumItemsRetrieved,sizeof(EWDB_TriggerStruct),CompareTriggerChans);
  
  /* add the arrivals the event info list */
  rc = MergeListDataWithEvent(pBuffer, BufferSize, NumItemsRetrieved, 
                              pEventInfo, GetidChanForTriggerStruct, 
                              AddTriggerToEWChannelDataStruct);
  if(rc == EWDB_RETURN_FAILURE)
  {
    logit("t","GetDBEventTriggerInfo(): ERROR %s failed!\n",
          "MergeListDataWithEvent()");
    ret = EWDB_RETURN_FAILURE;
    goto Abort;
  }

  ret = EWDB_RETURN_SUCCESS;

Abort:
  *ppBuffer = pBuffer;
  *pBufferSize = BufferSize;
  
  return(ret);
}


/***************************
############################
## END TRIGGERS SECTION   ##
############################
***************************/


/***************************
############################
##     STAMAGS SECTION    ##
############################
***************************/

/*************************************************************************
*************************************************************************/
static int CompareStaMagChans( const void *arg1, const void *arg2 )
{
  EWDB_StationMagStruct *psm1=(EWDB_StationMagStruct *)arg1;
  EWDB_StationMagStruct *psm2=(EWDB_StationMagStruct *)arg2;
  if(psm1->idChan < psm2->idChan)
    return(-1);
  else if (psm1->idChan == psm2->idChan)
    return(0);
  else
    return(1);
}


/*************************************************************************
*************************************************************************/
EWDBid GetidChanForStaMagStruct(const void * pBuffer, int iArrayOffset)
{
  return(((EWDB_StationMagStruct *)pBuffer)[iArrayOffset].idChan);
  
}


/*************************************************************************
*************************************************************************/
int AddStaMagToEWChannelDataStruct(EWChannelDataStruct * pChannel, 
                                    const void * pBuffer, int iArrayOffset)
{
  int tmp;
  
  tmp = pChannel->iNumStaMags;
  if(tmp < MDPCPE)
  {
    memcpy(&(pChannel->Stamags[tmp]), 
           &(((EWDB_StationMagStruct *)pBuffer)[iArrayOffset]),
           sizeof(EWDB_StationMagStruct));
    pChannel->iNumStaMags++;
  }
  /* else  WE SHOULD PROBABLY RETURN WARNING, to indicate that we
           had too much data, and stamag didn't get added.
     DK 071002
   ************************************************************/
  
  return(EWDB_RETURN_SUCCESS);
}


/*************************************************************************
*************************************************************************/
static int GetStaMags(char ** ppBuffer, int * pBufferSize, 
                       EWDB_MagStruct *pMag, int * pNumItemsRetrieved)
{
  int NumItemsFound, NumItemsRetrieved;
  int BufferSize;
  char * pBuffer;
  int ret;
  
  if(!(ppBuffer && (*ppBuffer) && pBufferSize && 
    pNumItemsRetrieved && pMag))
  {
    logit("t","%s:  Error NULL input param\n", "GetStaMags()");
    return(EWDB_RETURN_FAILURE);
  }
  
  /* set pBuffer and BufferSize */
  pBuffer    = *ppBuffer;
  BufferSize = *pBufferSize;
  
  /* Retrieve the StaMags for this event. */
  if(ewdb_api_GetStaMags(pMag->idMag,(EWDB_StationMagStruct *)pBuffer,
    &NumItemsFound,&NumItemsRetrieved, 
    BufferSize/sizeof(EWDB_StationMagStruct)) == EWDB_RETURN_FAILURE)
  {
    logit ("", "%s:  Call to ewdb_api_GetStaMags failed.\n",
      "GetStaMags()");
    ret = EWDB_RETURN_FAILURE;
    goto Abort;
  }
  
  /* check to see if our buffer was large enough to retrieve all the arrivals */
  if(NumItemsFound > NumItemsRetrieved)
  {  
    if(ReplaceBuffer( (void**)(&pBuffer),
      0, NumItemsFound * sizeof(EWDB_StationMagStruct), FALSE)
      != EWDB_RETURN_SUCCESS)
    {
      logit("","GetStaMags(): ERROR %s failed!\n", 
        "ReplaceBuffer()");
      ret = EWDB_RETURN_FAILURE;
      goto Abort;
    }
    
    BufferSize = NumItemsFound * sizeof(EWDB_StationMagStruct);
    
    /* GetStaMags() attempt 2 */
    if(ewdb_api_GetStaMags(pMag->idMag,(EWDB_StationMagStruct *)pBuffer,
      &NumItemsFound,&NumItemsRetrieved, 
      BufferSize/sizeof(EWDB_StationMagStruct)) == EWDB_RETURN_FAILURE)
    {
      logit ("", "%s:  Call(2) to ewdb_api_GetStaMags failed.\n",
        "GetStaMags()");
      ret = EWDB_RETURN_FAILURE;
      goto Abort;
    }
    
  }  /* end  if(NumItemsFound > NumItemsRetrieved) */
  
  ret = EWDB_RETURN_SUCCESS;
  
Abort:
  *ppBuffer    = pBuffer;
  *pBufferSize = BufferSize;
  *pNumItemsRetrieved = NumItemsRetrieved;
  
  return(ret);
}  /* end GetStaMags() */


/*************************************************************************
*************************************************************************/
int GetDBEventStaMagInfo(void ** ppBuffer, int * pBufferSize, 
                          EWEventInfoStruct * pEventInfo)
{  
  
  char * pBuffer;
  int BufferSize;
  int ret;
  int NumItemsRetrieved;

  int i;

  if(!(ppBuffer && *ppBuffer && pBufferSize && pEventInfo))
  {
    logit("t", "GetDBEventStaMagInfo(): ERROR:  NULL input params!\n");
    return(EWDB_RETURN_FAILURE);
  }

  /* copy the global buffer info to our local vars */
  pBuffer = *ppBuffer;
  BufferSize = *pBufferSize;


  for (i = 0; i < pEventInfo->iNumMags; i++)
  {
	   if (GetStaMags(&pBuffer, &BufferSize, &(pEventInfo->Mags[i]),
   	                            &NumItemsRetrieved) == EWDB_RETURN_FAILURE)
       {
   	      logit("t", "Call to GetStaMags() failed for magnitude!\n");
      	   ret = EWDB_RETURN_FAILURE;
      	   goto Abort;
  	   }

       /* sort the list of stamags */
       qsort(pBuffer,NumItemsRetrieved,sizeof(EWDB_StationMagStruct),
             CompareStaMagChans);

       /* add the stamags the event info list */
       if (MergeListDataWithEvent(pBuffer, BufferSize, NumItemsRetrieved, 
                                pEventInfo, GetidChanForStaMagStruct, 
                                AddStaMagToEWChannelDataStruct) == EWDB_RETURN_FAILURE)
       {
         logit("t","GetDBEventStaMagInfo(): ERROR %s failed!\n",
                                        "MergeListDataWithEvent()");
         ret = EWDB_RETURN_FAILURE;
         goto Abort;
       }
  }

  ret = EWDB_RETURN_SUCCESS;

Abort:
  *ppBuffer = pBuffer;
  *pBufferSize = BufferSize;
  
  return(ret);
}

/***************************
############################
##  END STAMAGS SECTION   ##
############################
***************************/


/***************************
############################
## WAVEFORM DESCS SECTION ##
############################
***************************/

/*************************************************************************
*************************************************************************/
static int CompareWaveformChans( const void *arg1, const void *arg2 )
{
  EWDB_WaveformStruct *pwf1=(EWDB_WaveformStruct *)arg1;
  EWDB_WaveformStruct *pwf2=(EWDB_WaveformStruct *)arg2;
  if(pwf1->idChan < pwf2->idChan)
    return(-1);
  else if (pwf1->idChan == pwf2->idChan)
    return(0);
  else
    return(1);
}


/*************************************************************************
*************************************************************************/
EWDBid GetidChanForWaveformStruct(const void * pBuffer, int iArrayOffset)
{
  return(((EWDB_WaveformStruct *)pBuffer)[iArrayOffset].idChan);
  
}


/*************************************************************************
*************************************************************************/
int AddWaveformToEWChannelDataStruct(EWChannelDataStruct * pChannel, 
                                    const void * pBuffer, int iArrayOffset)
{
  int tmp;
  
  tmp = pChannel->iNumWaveforms;
  if(tmp < MDPCPE)
  {
    memcpy(&(pChannel->Waveforms[tmp]), 
           &(((EWDB_WaveformStruct *)pBuffer)[iArrayOffset]),
           sizeof(EWDB_WaveformStruct));
    pChannel->iNumWaveforms++;
  }
  /* else  WE SHOULD PROBABLY RETURN WARNING, to indicate that we
           had too much data, and waveforms didn't get added.
     DK 071002
   ************************************************************/
  
  return(EWDB_RETURN_SUCCESS);
}


/*************************************************************************
*************************************************************************/
static int GetWaveformDescriptors(char ** ppBuffer, int * pBufferSize, 
                       EWDBid idEvent, int * pNumItemsRetrieved)
{
  /* for performance time measurments */
  
  int NumItemsFound, NumItemsRetrieved;
  int BufferSize;
  char * pBuffer;
  int ret;
  
  if(!(ppBuffer && (*ppBuffer) && pBufferSize && 
    pNumItemsRetrieved && idEvent))
  {
    logit("t","%s:  Error NULL input param\n", "GetWaveformDescriptors()");
    return(EWDB_RETURN_FAILURE);
  }
  
  /* set pBuffer and BufferSize */
  pBuffer    = *ppBuffer;
  BufferSize = *pBufferSize;
  
  /* Retrieve the Waveforms for this event. */
  if(ewdb_api_GetWaveformListByEvent(idEvent,(EWDB_WaveformStruct *)pBuffer,
                                  (EWDB_StationStruct *)NULL,
                                  FALSE /* include station info */,
                                  &NumItemsFound,&NumItemsRetrieved,
                                  BufferSize/sizeof(EWDB_WaveformStruct)) 
      == EWDB_RETURN_FAILURE)
  {
    logit ("", "%s:  Call to EWDB_GetWaveforms failed.\n",
      "GetWaveformDescriptors()");
    ret = EWDB_RETURN_FAILURE;
    goto Abort;
  }
  
  /* check to see if our buffer was large enough to retrieve all the arrivals */
  if(NumItemsFound > NumItemsRetrieved)
  {  
    if(ReplaceBuffer((void**)(&pBuffer),
                     0, NumItemsFound * sizeof(EWDB_WaveformStruct), FALSE)
       != EWDB_RETURN_SUCCESS)
    {
      logit("","GetWaveformDescriptors(): ERROR %s failed!\n", 
        "ReplaceBuffer()");
      ret = EWDB_RETURN_FAILURE;
      goto Abort;
    }
    
    BufferSize = NumItemsFound * sizeof(EWDB_WaveformStruct);
    
    
    /* GetWaveformDescriptors() attempt 2 */
    if(ewdb_api_GetWaveformListByEvent(idEvent,(EWDB_WaveformStruct *)pBuffer,
                                    (EWDB_StationStruct *)NULL,
                                    FALSE /* include station info */,
                                    &NumItemsFound,&NumItemsRetrieved,
                                    BufferSize/sizeof(EWDB_WaveformStruct)) 
        == EWDB_RETURN_FAILURE)
    {
      logit ("", "%s:  Call(2) to EWDB_GetWaveforms failed.\n",
        "GetWaveformDescriptors()");
      ret = EWDB_RETURN_FAILURE;
      goto Abort;
    }
  }  /* end  if(NumItemsFound > NumItemsRetrieved) */
  
  ret = EWDB_RETURN_SUCCESS;
  
Abort:
  *ppBuffer    = pBuffer;
  *pBufferSize = BufferSize;
  *pNumItemsRetrieved = NumItemsRetrieved;

  return(ret);
}  /* end GetWaveformDescriptors() */


/*************************************************************************
*************************************************************************/
int GetDBEventWaveformInfo(void ** ppBuffer, int * pBufferSize, 
                          EWEventInfoStruct * pEventInfo)
{  
  
  char * pBuffer;
  int BufferSize;
  int ret,rc;
  int NumItemsRetrieved;

  if(!(ppBuffer && *ppBuffer && pBufferSize && pEventInfo))
  {
    logit("t", "GetWaveformDescriptors(): ERROR:  NULL input params!\n");
    return(EWDB_RETURN_FAILURE);
  }

  /* copy the global buffer info to our local vars */
  pBuffer = *ppBuffer;
  BufferSize = *pBufferSize;
  
  rc = GetWaveformDescriptors(&pBuffer, &BufferSize, 
                              pEventInfo->Event.idEvent,
                              &NumItemsRetrieved);
  
  if(rc == EWDB_RETURN_FAILURE)
  {
    logit("t","GetWaveformDescriptors():  GetWaveformDescriptors() failed!\n");
    ret = EWDB_RETURN_FAILURE;
    goto Abort;
  }
  
  /* sort the list of arrivals */
  qsort(pBuffer,NumItemsRetrieved,sizeof(EWDB_WaveformStruct),
        CompareWaveformChans);
  
  /* add the arrivals the event info list */
  rc = MergeListDataWithEvent(pBuffer, BufferSize, NumItemsRetrieved, 
                              pEventInfo, GetidChanForWaveformStruct, 
                              AddWaveformToEWChannelDataStruct);
  if(rc == EWDB_RETURN_FAILURE)
  {
    logit("t","GetWaveformDescriptors(): ERROR %s failed!\n",
          "MergeListDataWithEvent()");
    ret = EWDB_RETURN_FAILURE;
    goto Abort;
  }

  ret = EWDB_RETURN_SUCCESS;

Abort:
  *ppBuffer = pBuffer;
  *pBufferSize = BufferSize;
  
  return(ret);
}

/***************************
############################
## END WAVEFORM D SECTION ##
############################
***************************/


/***************************
############################
##    MAIN  SECTION       ##
############################
***************************/

/*************************************************************************
*************************************************************************/
int ewdb_apps_GetDBEventInfo(EWEventInfoStruct * pEventInfo, EWDBid idEvent)
{
  int DBEventOptions;

  DBEventOptions = GETEWEVENTINFO_SUMMARYINFO | GETEWEVENTINFO_PICKS
                   | GETEWEVENTINFO_STAMAGS | GETEWEVENTINFO_COMPINFO
                   | GETEWEVENTINFO_COOKEDTF | GETEWEVENTINFO_WAVEFORM_DESCS;

  /* ewdb_apps_Set_GetDBEventInfo_Debug(1); */

  return(ewdb_apps_GetDBEventInfoII(pEventInfo,idEvent,-1,DBEventOptions));
}  /* end int ewdb_apps_GetDBEventInfo() */


/*************************************************************************
*************************************************************************/
int ewdb_apps_GetDBEventInfoII(EWEventInfoStruct * pEventInfo, 
				EWDBid idEvent, EWDBid idOrigin, int Flags)
{
/* Currently supported flags:
                             GETEWEVENTINFO_PICKS
                             GETEWEVENTINFO_STAMAGS
                             GETEWEVENTINFO_COMPINFO
                             GETEWEVENTINFO_COOKEDTF
                             GETEWEVENTINFO_WAVEFORM_DESCS
                             GETEWEVENTINFO_WAVEFORMS
  Summary info is always retrieved!!!!!
**********************************************************************************/
	char 					*pBuffer;
	int 					BufferSize=400000;  /* DK 121500 upped from 60000 */
	EWDBid 					Pref_idOrigin,Pref_idMag,Pref_idMech;
	int 					i, j, ctr;
	EWDB_WaveformStruct		*pWaveform;
    int                     rc, ret, NumFound, NumRetr, ParamsTime;
 
	if (pEventInfo == NULL)
	{
		logit ("",  "Invalid arguments passed in.\n");
		return  (EWDB_RETURN_FAILURE);
	}

	/* Initialize the Event structure */
	if (InitEWEvent (pEventInfo) != EW_SUCCESS)
	{
		logit ("", "Call to InitEWEvent failed.\n");
		return EWDB_RETURN_FAILURE;
	}

  if((pBuffer= (char *) calloc(1,BufferSize)) == NULL)
  {
    logit("","ewdb_apps_GetDBEventInfo(): ERROR! failed to malloc %d bytes!\n",
          BufferSize);
    return(EWDB_RETURN_FAILURE);
  }


	/* get the summary info */
    if (ewdb_api_GetEventInfo(idEvent, &(pEventInfo->Event)) != EWDB_RETURN_SUCCESS)
    {
      logit ("", "Call to ewdb_api_GetEventInfo failed.\n");
      free (pBuffer);
      return EWDB_RETURN_FAILURE;
    }
    
	pEventInfo->GotTrigger = FALSE;
	pEventInfo->GotLocation = FALSE;
	pEventInfo->PrefOrigin.idOrigin = -1;
	pEventInfo->CoincEvt.idCoincidence = -1;
    
	if (pEventInfo->Event.iEventType == EWDB_EVENT_TYPE_COINCIDENCE)
	{
        if (ewdb_api_GetCoincidence (pEventInfo->Event.idEvent, EWDB_EVENT_TYPE_COINCIDENCE,
					&pEventInfo->CoincEvt, 1, -1, -1, 
					&NumFound, &NumRetr) == EWDB_RETURN_FAILURE)
        {
            logit ("", "Call to ewdb_api_GetCoincidence failed.\n");
            free (pBuffer);
            return EWDB_RETURN_FAILURE;
        }

		if (NumRetr > 0)
		{
			pEventInfo->GotTrigger = TRUE;
			strcpy (pEventInfo->Event.szSource,
					pEventInfo->CoincEvt.szSource);
		}
	}
	else
	{
       if (ewdb_api_GetPreferredSummaryInfo(idEvent,&Pref_idOrigin,&Pref_idMag,&Pref_idMech)
   													      != EWDB_RETURN_SUCCESS)
       {
         logit ("", "Call to ewdb_api_GetPreferredSummaryInfo failed.\n");
         free (pBuffer);
         return EWDB_RETURN_FAILURE;
       }
    
		if (idOrigin <= 0)
		{
			pEventInfo->PrefOrigin.idOrigin = Pref_idOrigin;
		}
		else
		{
			pEventInfo->PrefOrigin.idOrigin = idOrigin;
		}


       if (ewdb_api_GetOrigin(pEventInfo->PrefOrigin.idOrigin,
						&(pEventInfo->PrefOrigin)) != EWDB_RETURN_SUCCESS)
       {
         logit ("", "Call to ewdb_api_GetOrigin failed.\n");
         free (pBuffer);
         return EWDB_RETURN_FAILURE;
       }

    
     	/* retrieve all magnitudes for this origin */
	    if (ewdb_api_GetMagsForOrigin(pEventInfo->PrefOrigin.idOrigin, pEventInfo->Mags, 
							&NumFound, &NumRetr, MAX_MAGS_PER_ORIGIN) != EWDB_RETURN_SUCCESS)
        {
			if (NumFound > NumRetr)
			{
				logit ("", "Warning: Only retrieved %d out of %d magnitudes.\n",
								NumRetr, NumFound);
			}
			else
			{
				logit ("", "Call to ewdb_api_GetMagsForOrigin failed.\n");
				free (pBuffer);
				return EWDB_RETURN_FAILURE;
			}
        }

		pEventInfo->iPrefMag = -1;
		pEventInfo->iMd = -1;
		pEventInfo->iML = -1;

		pEventInfo->iNumMags = NumRetr;
		for (i = 0; i < NumRetr; i++)
		{
			if (pEventInfo->Mags[i].idMag == Pref_idMag)
				pEventInfo->iPrefMag = i; 
	
			if ((pEventInfo->Mags[i].iMagType == MAGTYPE_LOCAL_PEAK2PEAK) || 
			    (pEventInfo->Mags[i].iMagType == MAGTYPE_LOCAL_ZERO2PEAK))
				pEventInfo->iML = i; 
			else if (pEventInfo->Mags[i].iMagType == MAGTYPE_DURATION)
				pEventInfo->iMd = i; 
		}

		if (NumRetr <= 0)
		{
			/*
			logit ("", "No magnitudes retrieved, numRetr %d, numFound %d.\n", NumRetr, NumFound);
			*/
			pEventInfo->iPrefMag = -1;
      	}
		else if (pEventInfo->iPrefMag == -1)
		{
			/*
			logit ("", "Preferred magnitude record was not retrieved. Setting to 0.\n");
			*/
			pEventInfo->iPrefMag = 0;
      	}
	
		pEventInfo->iNumMags = NumRetr;
		pEventInfo->GotLocation = TRUE;

		strcpy (pEventInfo->Event.szSource, pEventInfo->PrefOrigin.sRealSource);

		/* 
		 * Yup, we do this again. Even if this is a located event,
		 * we may have a coincidence associated with it.
	 	 */
        if (ewdb_api_GetCoincidence (pEventInfo->Event.idEvent, EWDB_EVENT_TYPE_QUAKE,
					&pEventInfo->CoincEvt, 1, -1, -1, 
					&NumFound, &NumRetr) == EWDB_RETURN_FAILURE)
        {
            logit ("", "Call to ewdb_api_GetCoincidence failed.\n");
            free (pBuffer);
            return EWDB_RETURN_FAILURE;
        }

		if (NumRetr > 0)
			pEventInfo->GotTrigger = TRUE;

	} /* End if this is not a coincidence */


  /*logit("t","NOTE** iNumChans= %d before arrivals\n",pEventInfo->iNumChans);*/
  /***********************************/
  /* Retrieve arrivals section       */
  /***********************************/
  if(Flags & GETEWEVENTINFO_PICKS)
  {
        rc = GetDBEventTriggerInfo((void **)&pBuffer, &BufferSize, pEventInfo);
        if(rc == EWDB_RETURN_FAILURE)
        {
          logit("t","ewdb_apps_GetDBEventInfoII():  GetDBEventTriggerInfo() failed!\n");
          ret = EWDB_RETURN_FAILURE;
          goto Abort;
        }


        rc = GetDBEventArrivalInfo((void **)&pBuffer, &BufferSize, pEventInfo);
        if(rc == EWDB_RETURN_FAILURE)
        {
          logit("t","ewdb_apps_GetDBEventInfoII():  GetDBEventArrivalInfo() failed!\n");
          ret = EWDB_RETURN_FAILURE;
          goto Abort;
        }
  }      /* end if(Flags & GETEWEVENTINFO_ARRIVALS) */

  /*logit("t","NOTE** iNumChans= %d before stamags\n",pEventInfo->iNumChans);*/
  /***********************************/
  /* Retrieve stamags section       */
  /***********************************/
  if(Flags & GETEWEVENTINFO_STAMAGS)
  {
	if (pEventInfo->Event.iEventType != EWDB_EVENT_TYPE_COINCIDENCE)
	{

       rc = GetDBEventStaMagInfo((void **)&pBuffer, &BufferSize, pEventInfo);
       if(rc == EWDB_RETURN_FAILURE)
       {
         logit("t","ewdb_apps_GetDBEventInfoII():  GetDBEventStaMagInfo() failed!\n");
         ret = EWDB_RETURN_FAILURE;
         goto Abort;
       }
	}
  }      /* end if(Flags & GETEWEVENTINFO_STAMAGS) */
  
  
  /*logit("t","NOTE** iNumChans= %d before waveform descs\n",pEventInfo->iNumChans);*/
  /***********************************/
  /* Retrieve waveform descs section */
  /***********************************/
  if(Flags & GETEWEVENTINFO_WAVEFORM_DESCS)
  {

    rc = GetDBEventWaveformInfo((void **)&pBuffer, &BufferSize, pEventInfo);
    if(rc == EWDB_RETURN_FAILURE)
    {
      logit("t","ewdb_apps_GetDBEventInfoII():  GetDBEventWaveformInfo() failed!\n");
      ret = EWDB_RETURN_FAILURE;
      goto Abort;
    }
  }      /* end if(Flags & GETEWEVENTINFO_WAVEFORM_DESCS) */


  /*logit("t","NOTE** iNumChans= %d before waveforms\n",pEventInfo->iNumChans);*/
  /***********************************/
  /* Retrieve waveforms section       */
  /***********************************/
  if(Flags & GETEWEVENTINFO_WAVEFORMS)
  {
    for(j=0;j<pEventInfo->iNumChans;j++)
    {
      for(i=0;i < pEventInfo->pChanInfo[j].iNumWaveforms; i++)
      {
        /* Now get the actual waveform */
        pWaveform = &(pEventInfo->pChanInfo[j].Waveforms[i]);
        
        pWaveform->binSnippet = NULL;
        
        if ((pWaveform->binSnippet = malloc(pWaveform->iByteLen)) == NULL)
        {
          logit ("", "Could not malloc binSnippet.\n");
          free (pBuffer);
          return EWDB_RETURN_FAILURE;
        }
        
        if (ewdb_api_GetWaveformSnippet(pWaveform->idWaveform, 
                                    pWaveform->binSnippet,
                                    pWaveform->iByteLen) 
            != EWDB_RETURN_SUCCESS)
        {
          logit ("", "ERROR:  Call to ewdb_api_GetWaveformSnippet() for Snippet %d "
                 "for chan %d failed\n",
                 i,pEventInfo->pChanInfo[j].idChan);
          free(pWaveform->binSnippet);
          pWaveform->binSnippet = NULL;
        }
        
      }  /* end for i < iNumWaveforms for Channel j */

      /* remove any waveform descriptors for which we 
         were not able to get waveforms */
      for(i=0;i < pEventInfo->pChanInfo[j].iNumWaveforms; i++)
      {
        if(pEventInfo->pChanInfo[j].Waveforms[i].binSnippet == NULL)
        {
          for(ctr=i; ctr < pEventInfo->pChanInfo[j].iNumWaveforms - 1; ctr++)
          {
            memcpy(&(pEventInfo->pChanInfo[j].Waveforms[i]), 
                   &(pEventInfo->pChanInfo[j].Waveforms[i+1]),
                   sizeof(EWDB_WaveformStruct)
                  );
          }
          i--;
          pEventInfo->pChanInfo[j].iNumWaveforms--;
        }  /* if binSnippet is NULL */
      }  /* end for i < iNumWaveforms */
    }    /* end for j < iNumChans */
  }      /* end if Flags & GETEWEVENTINFO_WAVEFORMS */


  /***********************************/
  /* Retrieve component info section */
  /***********************************/

  if(Flags & GETEWEVENTINFO_COMPINFO)
  {
    /* Here is where it gets ugly!!!
       Each call to GetComponentInfo() takes
       a fairly short amount of time: 5+ ms
       However, when you have to make a call
       for each of 400 channels for which you
       have data for an event, then the sum
       time of the calls is more than 2 seconds,
       which is a long time if you are trying
       to populate a web page.  What we would
       like to do is retrieve component info for
       all channels in the db at once, and then
       grab only the data we want. */

 
/* 
 Actually, the number of channels would have to be really large
 in order for the first part (get everything) to pay off. I ran some
 tests with 50 events of about 150 channels each, and the second
 method of grabbing individual component info was MUCH faster 
 (about three times faster) LV 09/07/2001
*/

    if(0)  /* change this to pEventInfo->iNumStations > XX */
    {
      EWDB_StationStruct *pStations;
      int iNumStationsInBuffer, iNumStationsRetrieved, iNumStationsFound;
      int rc;
      time_t tNow;
      
      if( (pStations= (EWDB_StationStruct *) 
        malloc(sizeof(EWDB_StationStruct) * MAX_NUM_STATIONS)
        ) == NULL)
      {
        logit("","ewdb_apps_GetDBEventInfo(): ERROR! failed to malloc %d "
          "bytes for Component info!\n",
          sizeof(EWDB_StationStruct) * MAX_NUM_STATIONS);
        return(EWDB_RETURN_FAILURE);
      }
      iNumStationsInBuffer = MAX_NUM_STATIONS;

      /* Get the request time from the preferred origin time */
      tNow = (time_t)(pEventInfo->PrefOrigin.tOrigin);
      if(!tNow)
      {
        /* Use the current time as a backup, in case there is 
           no preferred origin for this event */
        time(&tNow);
      }

      rc=ewdb_api_GetStationListWithoutLocation((double)tNow, pStations,
                                                &iNumStationsFound,
                                                &iNumStationsRetrieved, 
                                                iNumStationsInBuffer);
      if(rc!=EWDB_RETURN_SUCCESS)
      {
        if(rc == EWDB_RETURN_FAILURE)
        {
          logit("t","ewdb_apps_GetDBEventInfo(): ERROR! %s failed with unknown error!",
            "ewdb_api_GetStationListWithoutLocation()");
          free(pStations);
          free(pBuffer);
          return(EWDB_RETURN_FAILURE);
        }
        else
        {
          logit("t","ewdb_apps_GetDBEventInfo(): WARNING! %s did not retrieve all stations.\n"
            "Only %d of %d total stations retrieved.\n",
            "ewdb_api_GetStationListWithoutLocation()",
            iNumStationsRetrieved,iNumStationsFound);
        }
      }
      
      /* loop through the event channel array */
      i=0;
      for(j=0;j < pEventInfo->iNumChans;j++)
      {
        if( i >= iNumStationsRetrieved)
          break;
        while(pStations[i].idChan <= pEventInfo->pChanInfo[j].idChan)
        {
          if( i >= iNumStationsRetrieved)
            break;
          if(pEventInfo->pChanInfo[j].idChan == (pStations[i].idChan))
          {
            memcpy (&(pEventInfo->pChanInfo[j].Station), 
              &(pStations[i]),
              sizeof(EWDB_StationStruct));
          }  /* if stationlist.idChan = eventbuffer.idchan */
          i++;
        } /* end while i.idChan <= j.idChan */
      }  /* end for j < pEventInfo->iNumChans */

      free(pStations);
      
    }  /* end if(0) */
    else
    {

      
      for(i=0;i<pEventInfo->iNumChans;i++)
      {
		if (pEventInfo->Event.iEventType == EWDB_EVENT_TYPE_COINCIDENCE)
			ParamsTime = (int) pEventInfo->CoincEvt.tCoincidence;
		else
			ParamsTime = (int) pEventInfo->PrefOrigin.tOrigin;

        if (ewdb_api_GetComponentInfo(pEventInfo->pChanInfo[i].idChan, ParamsTime, 
								&(pEventInfo->pChanInfo[i].Station)) == EWDB_RETURN_FAILURE)
        {
          logit ("t", "ewdb_apps_GetDBEventInfo(): "
						"ERROR! Call to ewdb_api_GetComponentInfo failed "
	                    "for idChan %d!\n", pEventInfo->pChanInfo[i].idChan);
          continue;
        }
      }  /* end for i < iNumChans */
      
    } /* end else if(1) */
  }  /* end if(Flags & GETEWEVENTINFO_COMPINFO) */


  /***********************************/
  /* Retrieve cooked infra section */
  /***********************************/
  if(Flags & GETEWEVENTINFO_COOKEDTF)
  {
    for(i=0;i<pEventInfo->iNumChans;i++)
    {
      rc = ewdb_api_GetTransformFunctionForChan(pEventInfo->pChanInfo[i].idChan,
                                                (int)(pEventInfo->PrefOrigin.tOrigin),
                                                &(pEventInfo->pChanInfo[i].ResponseInfo));
      if(rc == EWDB_RETURN_FAILURE)
      {
        logit ("", "Call to GetTransformFunctionForChan failed.\n");
        pEventInfo->pChanInfo[i].bResponseIsValid = FALSE;
      }
      else if(rc == EWDB_RETURN_WARNING)
      {
        /* No transfer function available for this channel */
        pEventInfo->pChanInfo[i].bResponseIsValid = FALSE;
      }
      else
      {
        pEventInfo->pChanInfo[i].bResponseIsValid = TRUE;
      }
    }  /* end for i < iNumChans */

  }  /* end if(Flags & GETEWEVENTINFO_COOKEDTF) */


  /* sort by arrival.distance  (from epicenter) */
    qsort(pEventInfo->pChanInfo,pEventInfo->iNumChans,
          sizeof(EWChannelDataStruct),CompareArrivalDistsForChan);
  
  ret = EWDB_RETURN_SUCCESS;

Abort:

  if(pBuffer)
    free(pBuffer);

  return(ret);
}  /* end int ewdb_apps_GetDBEventInfoII() */



/*************************************************************************
 
   From the array of EWChannelDataStruct, build the array of 
   EWStationDataStruct which presents the same information 
   except that it is organized by station.

   If CompareLocation is set, it checks the location info for the station
   against the location of each channel and complains if lat, lon, 
   or elevation values are apart more than the supplied slack
   values (LatSlack, LonSlack, ElevSlack).

   Otherwise, the station location will be the same as the location 
   of the first channel.

   if CalcDist is set, we assume that the PrefOrigin structure is set 
   and we will attempt to compute the epicentral distance from each station.

*************************************************************************/
int ewdb_apps_BuildStationArray (EWEventInfoStruct *pEventInfo, 
					int CompareLocation, double LatSlack, 
					double LonSlack, double ElevSlack, 
					int CalcDist)
{

	int		i, j, staind, nc;

	if (pEventInfo == NULL)
	{
		logit ("", "Invalid arguments passed in.\n");
		return EWDB_RETURN_FAILURE;
	}

	if (CompareLocation == TRUE)
	{
		if ((LatSlack < 0.0) || (LonSlack < 0.0) || (ElevSlack < 0.0))
		{
			logit ("", "Invalid location slack values passed in.\n");
			return EWDB_RETURN_FAILURE;
		}
	}

	if (pEventInfo->iNumChans <= 0)
	{
/*
		logit ("", "No channels to process; exiting.\n");
*/
		return EWDB_RETURN_FAILURE;
	}
		

	/* 
	 * Allocate space for one station per channel. This is likely
	 *  to be too much, but at least we won't have to realloc
	 *  later on. Maybe we should free up unused space in the end?
	 */

	if ((pEventInfo->pStaInfo = (EWStationDataStruct *) malloc 
			(pEventInfo->iNumChans * sizeof (EWStationDataStruct))) == NULL)
	{
		logit ("", "Could not malloc pStaInfo.\n");
		return EWDB_RETURN_FAILURE;
	}

	pEventInfo->iNumStas = 0;

	for (i = 0; i < pEventInfo->iNumChans; i++)
	{
		/* Find the station for this channel */

		staind = -1;
		for (j = 0; ((j < pEventInfo->iNumStas) && (staind < 0)); j++)
		{
			if ((strcmp (pEventInfo->pChanInfo[i].Station.Sta, 
							pEventInfo->pStaInfo[j].Sta) == 0) &&
				(strcmp (pEventInfo->pChanInfo[i].Station.Net,
							pEventInfo->pStaInfo[j].Net) == 0))
			{
				staind = j;
			}
		}

		if (staind >= 0)
		{
			/* Add channel to this station */
			nc = pEventInfo->pStaInfo[staind].iNumChans;
			pEventInfo->pStaInfo[staind].Chans[nc] =
											&pEventInfo->pChanInfo[i];

			pEventInfo->pStaInfo[staind].iNumChans =
					pEventInfo->pStaInfo[staind].iNumChans + 1;


			if (CompareLocation == TRUE)
			{
				/* sanity check of location */
				if  (fabs (pEventInfo->pStaInfo[staind].Lat -
						pEventInfo->pChanInfo[i].Station.Lat) > LatSlack)
				{
					logit ("", "WARNING: Lattitude difference too large: "
								"Station %s: %f "
								"Channel %s: %f\n",
									pEventInfo->pStaInfo[staind].Sta,
									pEventInfo->pStaInfo[staind].Lat,
									pEventInfo->pChanInfo[i].Station.Comp,
									pEventInfo->pChanInfo[i].Station.Lat);
				}
				if  (fabs (pEventInfo->pStaInfo[staind].Lon -
						pEventInfo->pChanInfo[i].Station.Lon) > LonSlack)
				{
					logit ("", "WARNING: Longitude difference too large: "
								"Station %s: %f "
								"Channel %s: %f\n",
									pEventInfo->pStaInfo[staind].Sta,
									pEventInfo->pStaInfo[staind].Lon,
									pEventInfo->pChanInfo[i].Station.Comp,
									pEventInfo->pChanInfo[i].Station.Lon);
				}
				if  (fabs (pEventInfo->pStaInfo[staind].Elev -
						pEventInfo->pChanInfo[i].Station.Elev) > ElevSlack)
				{
					logit ("", "WARNING: Elevation difference too large: "
								"Station %s: %f "
								"Channel %s: %f\n",
									pEventInfo->pStaInfo[staind].Sta,
									pEventInfo->pStaInfo[staind].Elev,
									pEventInfo->pChanInfo[i].Station.Comp,
									pEventInfo->pChanInfo[i].Station.Elev);
				}

			} /* If CompareLocation is set */
	
		}
		else
		{
			/* Create a new station */
			strcpy (pEventInfo->pStaInfo[pEventInfo->iNumStas].Sta,
							pEventInfo->pChanInfo[i].Station.Sta);

			strcpy (pEventInfo->pStaInfo[pEventInfo->iNumStas].Net,
							pEventInfo->pChanInfo[i].Station.Net);

			pEventInfo->pStaInfo[pEventInfo->iNumStas].Lat = 
							pEventInfo->pChanInfo[i].Station.Lat;
			pEventInfo->pStaInfo[pEventInfo->iNumStas].Lon = 
							pEventInfo->pChanInfo[i].Station.Lon;
			pEventInfo->pStaInfo[pEventInfo->iNumStas].Elev = 
							pEventInfo->pChanInfo[i].Station.Elev;
	

			if (CalcDist == TRUE)
			{
				/* Calculcate epicentral distance */
				if (geo_to_km (pEventInfo->PrefOrigin.dLat,
							pEventInfo->PrefOrigin.dLon,
							pEventInfo->pStaInfo[pEventInfo->iNumStas].Lat,
							pEventInfo->pStaInfo[pEventInfo->iNumStas].Lon,
							&pEventInfo->pStaInfo[pEventInfo->iNumStas].Dist,
							&pEventInfo->pStaInfo[pEventInfo->iNumStas].Azm) != 1)
                {
                    html_logit ("", "Call to geo_to_km() failed.\n");
					return EW_FAILURE;
                }
			}


			/* This is the first channel */
			pEventInfo->pStaInfo[pEventInfo->iNumStas].Chans[0] =
											&pEventInfo->pChanInfo[i];

			pEventInfo->pStaInfo[pEventInfo->iNumStas].iNumChans = 1;

			pEventInfo->iNumStas = pEventInfo->iNumStas + 1;
		}

	} /* for loop (i) over channels */


	/* Sort array by distance */
	if (CalcDist == TRUE)
	{
		qsort (pEventInfo->pStaInfo, pEventInfo->iNumStas, 
								sizeof (EWStationDataStruct), compStaDistance);
	}

	return EWDB_RETURN_SUCCESS;
}

int compStaDistance( const void* r1, const void* r2)
{
    EWStationDataStruct* e1 = (EWStationDataStruct*)r1;
    EWStationDataStruct* e2 = (EWStationDataStruct*)r2;

    if( e1->Dist  < e2->Dist) return(-1);
    if( e1->Dist == e2->Dist) return(0);
    return(1);
}


/***************************
############################
## END    MAIN   SECTION  ##
############################
***************************/

