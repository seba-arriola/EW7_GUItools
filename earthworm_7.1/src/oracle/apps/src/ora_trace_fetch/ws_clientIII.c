
/*
*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
*   CHECKED IT OUT USING THE COMMAND CHECKOUT.
*
*    $Id: ws_clientIII.c,v 1.6 2003/02/04 18:00:14 davidk Exp $
*
*    Revision history:
*     $Log: ws_clientIII.c,v $
*     Revision 1.6  2003/02/04 18:00:14  davidk
*     Revamped the debug-logging facility to be a complex matrix of options, including
*     level - error, warning, info, debug and type  server or data.
*
*     In wsGetTraceFromServer() changed AbortThisServer goto-tag in code
*     to HandleErrors as it was more representative of the processing done at the tag.
*
*     Fixed two bugs in wsSearchSCN().
*     1) Removed menup null check in wsSearchSCN(), because menup is allowed to
*     be null, indicating that we want the first server that supports this SCN.
*     2) Corrected the "for loop" conditional used when searching for the next
*     server that has data for a given SCN.  The conditional is supposed to select the
*     next server that has the SCN in menu after the given server.
*     The existing conditional was using pointers to obtain the current menu instead of
*     the next one.  The corrected conditional uses server indexes to obtain the menu
*     of the next server for the given SCN.
*
*     Added a WARNING Comment at the top of wsGetNextTraceFromRequestList()
*     about the memory allocation method used for Snippets in the wsClientIII routines.
*
*     Fixed a bug that was causing a server error to be treated as success.  This resulted
*     in the ora_trace_fetch application trying to store a 0 length snippet.  The problem
*     was a 'goto' jump in the middle of some error handling code.  The goto was jumping
*     out of the error handling code, and the appropriate error processing was not
*     happening..
*
*     Revision 1.5  2002/02/18 18:34:00  davidk
*     Rewrote the main loop within wsMergeMenuIntoPSCNList()
*     that merges two lists of SCNs together.  Fixed memory
*     bugs where the size of the list wasn't being properly
*     set, and resulting allocation sizes were then wrong,
*     resulting in memory overwrites and mysterious problems.
*     (Used purify on Solaris to find problems).
*
*     Fixed a logit statement in wsGetTraceFromServer() that was
*     trying to print the socket(NULL) instead of the port number.
*     Fixed a bug in how wsGetNextTraceFromRequestList() handled
*     errors/warnings from wsGetTraceFromServer().
*
*     Revision 1.4  2002/02/12 04:48:07  davidk
*     removed C++ style comments.
*
*     Revision 1.3  2002/02/12 04:35:45  davidk
*     lots of changes.  ora_trace_fetch now seems to be stable except for a possible
*     memory leak.
*
*     Added lots of debugging statements.
*
*     Revision 1.2  2001/02/21 11:50:54  davidk
*     changed the include for ws_clientIII.h to local instead of system.
*
*     Revision 1.1  2001/01/18 17:15:03  davidk
*     Initial revision
*
*     Revision 1.3  2000/09/17 18:37:57  lombard
*     wsSearchSCN now really returns the first menu with matching scn.
*     wsGetTraceBin will now try another server if the first returns a gap.
*
*     Revision 1.2  2000/03/08 18:14:06  luetgert
*     fixed memmove problem in wsParseAsciiHeaderReply.
*
*     Revision 1.1  2000/02/14 18:51:48  lucky
*     Initial revision
*
*
*/

/* Version III of WaveServerXXX client utility routines.  This file contains
* various routines useful for dealing with WaveServerIV and beyond. This file
* is a copy of ws_clientII.c with a few modifications.*/

/* 5/17/98: Fixed bug in wsWaitAscii: it would fill reply buffer but never
* report overflow. PNL */

/* The philosophy behind the version III changes is:
* - Minimize wave_server(V) connections.
    - Close connections when not in use.
    - Open only one connection at a time.
    - Remove burden of socket work from client application.
* - Routines probably are not as thread-safe, as Dave Kragness does not
*   write thread aware applications as well as PNL. Routines that are
*   a carryover from ws_clientII should still be thread safe;  
* - Routines are layered on top of the socket_ew routines.
* David Kragness Earthworm Development Group; 01/09/2001
*/

/* The philosophy behind the version II changes is:
* - Leave sockets open unless there is an error on the socket; calling 
*   progam should ensure all sockets are closed before it quits.
* - Routines are thread-safe. However, the calling thread must ensure its
*   socket descriptors are unique or mutexed so that requests and replies
*   can't overlap.
* - Routines are layered on top of Dave Kragness's socket wrappers and thus
*   do no timing themselves. Exceptions to this are wsWaitAscii and 
*   wsWaitBinHeader which need special timing of recv().
* Pete Lombard, University of Washington Geophysics; 1/4/98
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <earthworm.h>
#include "ws_clientIII.h"
#include <socket_ew.h>
#include <time_ew.h>

#define WS_MAX_RECV_BUF_LEN 4096

static int menu_reqid = 0;
static int WS_CL_DEBUG = WS_DEBUG_SERVER_ERRORS | WS_DEBUG_DATA_ERRORS;


/* constants */

/* new in III */
#define wsNUMBER_OF_MENUS 20

/* not used yet */
#define wsMAX_SCNs_PER_MENU 256
#define wsMAX_REPLY_LEN  ((wsMAX_SCNs_PER_MENU * 80/*chars per line*/) + 80 /*initial response*/)
/* globals */


static double dAttempting;
static double dNow;

void * pDEBUG;

/* new in III */
int wsFetchMenu( WS_MENU menu, WS_PSCN *pSCNBuffer, 
                 int *pSCNBufferLen, int timeout);
int wsAddMenuToList(wsEnvironmentStruct * pEnv, WS_MENU menu);
int wsMergeMenuIntoPSCNList(wsEnvironmentStruct *pEnv,
                            WS_PSCN SCNReplyList, int SCNsFound,
                            WS_MENU_REC * pMenu);
static int Compare_WS_PSCN_RECs(const void *elem1, const void *elem2);
int wsCombinePSCNs(WS_PSCN pResult, WS_PSCN pIn1, WS_PSCN pIn2);
static int wsConvertLinkedListToArray(WS_MENU menu, WS_PSCN *pSCNBuffer, 
                                      int *pSCNBufferLen);


static int  wsParseMenuReply( WS_MENU, char* );
static void wsSkipN( char*, int, int* );
static int wsRemoveMenuFrompPSCNBuffer(wsHANDLE pEnv, WS_MENU pMenu);
static int wsWaitForServerResponse(WS_MENU menu, char* buf, int buflen, 
                                   int timeout_msec, int iTraceType);

static int wsParseBinHeaderReply( TRACE_REQ* getThis, char* reply );
static int wsParseAsciiHeaderReply( TRACE_REQ* getThis, char* reply );

static int wsCompareMenuNumbers(const void * pElem1, const void * pElem2);
static int wsCompareMenuNumbersForTraceReqs(const void * pElem1, const void * pElem2);


/* socket timing routines */
Time_ew adjustTimeoutLength(int timeout_msec);
struct timeval FAR * resetTimeout(struct timeval FAR *);


/* Unused Protoypes for internal functions 
static int  wsWaitBinHeader( WS_MENU, char*, int, int );
static int  wsParseBinHeaderReply( TRACE_REQ*, char* );
static int  wsParseAsciiHeaderReply( TRACE_REQ*, char* );
*/


/**************************************************************************
*      wsInitialize: Allocates a structure that the ws_client routines    *
*      use for storing data relative to this threads connections and      *
*      and wave_server information.                                       *
**************************************************************************/
int wsInitialize(wsHANDLE* ppEnv, int iMaxSnippetSize)
{
                 /*
                 Arguments:
                  ppEnv:  A pointer to a wsEnvironmentStruct*.  The function
                          allocates a structure and assigns it to the 
                          wsEnvironmentStruct*.

                 Return Codes:  
                  WS_ERR_NONE:  if all went well.
                  WS_ERR_MEMORY: if could not allocate memory.
                 */

  wsEnvironmentStruct * pEnv = NULL;
  int iRetVal = WS_ERR_NONE;

  pEnv = (wsEnvironmentStruct *) malloc(sizeof(wsEnvironmentStruct));
  if(!pEnv)
  {
    iRetVal = WS_ERR_MEMORY;
    goto wsInitialize_Abort;
  }
  /* else */
  memset(pEnv,0,sizeof(wsEnvironmentStruct));

  /* Allocate the trace snippet buffer
  ***********************************/
  if ( (pEnv->bSnippetBuffer = malloc( (size_t)iMaxSnippetSize)) == NULL )
  {
    logit("t","wsInitialize(): Cant allocate snippet buffer of %ld bytes. "
             "Quitting\n",iMaxSnippetSize);
    goto wsInitialize_Abort;
  }

  pEnv->iSnippetBufferSize = iMaxSnippetSize;

  *ppEnv=pEnv;

  return(iRetVal);

wsInitialize_Abort:
  if(pEnv)
  {
    if(pEnv->bSnippetBuffer)
      free(pEnv->bSnippetBuffer);
    free(pEnv);
  }
  return(iRetVal);

}/* End wsInitialize() */



/**************************************************************************
*      wsAddServer: builds a combined menu from many waveservers.        *
*      Called with the address and port of one waveserver.                *
*      On the first call it creates a linked list to store the 'menu'     *
*      reply from the indicated waveserver.                               *
*      On subsequent calls, it appends the menu replies to the list.      *
**************************************************************************/

int wsAddServer(wsEnvironmentStruct * pEnv, 
              char* ipAdr, char* port, int timeout, int refreshtime )
                 /*
                 Arguments:
                  ipAdr:  is the dot form of the IP address as a char string.
                  port:  TCP port number as a char string.
                  timeout:  timeout interval in milliseconds.
                  refreshtime:  time in secs between refreshes for this menu
                                0 = default (8 hours = 28800)
                               -1 = never
                 return:  
                 WS_ERR_NONE:  if all went well.
                 WS_ERR_NO_CONNECTION: if we could not get a connection.
                 WS_ERR_SOCKET: if we could not create a socket.
                 WS_ERR_BROKEN_CONNECTION:  if the connection broke.
                 WS_ERR_TIMEOUT: if a timeout occured.
                  WS_ERR_MEMORY: if out of memory.
                  WS_ERR_INPUT: if bad input parameters.
                 WS_ERR_PARSE: if parser failed.
                 */
{
  int ret,  rc;
  WS_MENU menu = NULL;
  WS_PSCN SCNReplyList = NULL;
  int SCNsFound=0;
  time_t tnow;
  
  if ( !ipAdr || !port ||
    (strlen(ipAdr) >= wsADRLEN) || (strlen(port) >= wsADRLEN) )
  {
    ret = WS_ERR_INPUT;
    if(WS_CL_DEBUG & WS_DEBUG_SERVER_ERRORS) 
      logit("","wsAddServer: bad address: %s port: %s\n",  ipAdr, port);
    goto Abort;
  }

  menu = ( WS_MENU_REC* )calloc(sizeof(WS_MENU_REC),1);
  if(!menu)
  {
    ret = WS_ERR_MEMORY;
    logit("t", "wsAddServer: memory allocation error\n");
    goto Abort;
  }
  
  strcpy( menu->addr, ipAdr );
  strcpy( menu->port, port );
  menu->tRefreshInterval = refreshtime;


  if (WS_CL_DEBUG & WS_DEBUG_OVERKILL) 
    logit("", "wsAddServer: calling wsFetchMenu()\n");

  rc=wsFetchMenu( menu, &SCNReplyList, &SCNsFound,timeout);
  if(rc!= WS_ERR_NONE  && rc!= WS_ERR_BUFFER_OVERFLOW)
  {
    ret = rc;
    if(WS_CL_DEBUG & WS_DEBUG_SERVER_ERRORS)
      logit("t", "wsAddServer: wsFetchMenu() failed with code [%d]\n", rc);
    goto Abort;
  }


  menu->tNextRefresh = time(&tnow) + menu->tRefreshInterval;

  menu->stats.tServerAdded = time(NULL);

  if(WS_CL_DEBUG & WS_DEBUG_OVERKILL) 
    logit("e", "wsAddServer: calling wsAddMenuToList()\n");


  /* Add the menu to the list */
  wsAddMenuToList(pEnv,menu);


  if(WS_CL_DEBUG & WS_DEBUG_OVERKILL) 
    logit("e", "wsAddServer: calling wsMergeMenuIntoPSCNList()\n");

  /* Merge the SCN's into the new list */
  rc=wsMergeMenuIntoPSCNList(pEnv,SCNReplyList,SCNsFound,menu);
  if(rc!= WS_ERR_NONE)
  {
    ret = rc;
    if(WS_CL_DEBUG & WS_DEBUG_SERVER_ERRORS) 
      logit("t", "wsAddServer: wsMergeMenuIntoPSCNList() failed with code [%d]\n", rc);
    goto Abort;
  }

  ret=WS_ERR_NONE;


Abort:
  if(SCNReplyList)
  {
    free(SCNReplyList);
  }

  if (WS_CL_DEBUG & WS_DEBUG_OVERKILL) logit("e", "wsAddServer: done\n");


  return ret;
}  /* end of wsAddServer() */


int wsMergeMenuIntoPSCNList(wsEnvironmentStruct *pEnv,
                            WS_PSCN SCNReplyList, int SCNsFound,
                            WS_MENU_REC * pMenu)
{
  WS_PSCN pTemp  = NULL;
  WS_PSCN pTemp2 = NULL;
  int ret, rc;
  int i,j;
  int rcComp;
  int iNumCurrentSCNs = 0;
  int ctr;

  pTemp = (WS_PSCN) malloc((pEnv->iNumPSCNs + SCNsFound)  
                            * sizeof(WS_PSCN_REC));
  if(WS_CL_DEBUG & WS_DEBUG_OVERKILL) 
    logit("", "wsMergeMenuIntoPSCNList: after malloc\n");
  if(!pTemp)
  {
    ret = WS_ERR_MEMORY;
    logit("t", "wsMergeMenuIntoPSCNList: memory allocation error for %d bytes\n",
          (pEnv->iNumPSCNs + SCNsFound) * sizeof(WS_PSCN_REC));
    goto Abort;
  }
  
  /* start at the beginning of the SCNReplyList */
  i=0;

  if (WS_CL_DEBUG & WS_DEBUG_OVERKILL) 
    logit("", "wsMergeMenuIntoPSCNList: looping through existing PSCNs\n");

  /* loop through the PSCNs in the existing pPSCNBuffer */
  for(j=0;i<SCNsFound && j<pEnv->iNumPSCNs;j++)
  {
    if(WS_CL_DEBUG & WS_DEBUG_OVERKILL) 
      logit("", "wsMergeMenuIntoPSCNList: top of PSCN loop\n");

    while((rcComp = 
           Compare_WS_PSCN_RECs(&(SCNReplyList[i]), &(pEnv->pPSCNBuffer[j])))
          < 0)
    {
      
      /* we have a new SCN */
      memcpy(&(pTemp[iNumCurrentSCNs]), &(SCNReplyList[i]),  sizeof(WS_PSCN_REC));
      pTemp[iNumCurrentSCNs].MenuList[0] = pMenu;
      pTemp[iNumCurrentSCNs].iNumMenus = 1;
      iNumCurrentSCNs++;
      
      if(WS_CL_DEBUG & WS_DEBUG_OVERKILL) 
        logit("", "wsMergeMenuIntoPSCNList: in loop after Compare_WS_PSCN_RECs\n");
      
      i++;  /* move to the next SCNReplyList SCN */
      
      if( i >= SCNsFound)
        break;
    }       /* end while (SCNReplyList.SCN < pPSCNBuffer.SCN) */


    if(rcComp < 0)
    {
      /* we must have dropped out because i >= SCNsFound, meaning we're done with
         the "new" menu, but not with the existing combined menu.
         Drop out of the loop, and the logic below will grab the remaining PSCNs.
         from the existing menu base.
      ****************************************************/
      break;
    }
    else if(rcComp == 0)
    {
      /* we found a matching SCN in the new menu.  Merge it
         with the existing one.
      ****************************************************/
      SCNReplyList[i].iNumMenus=1;
      SCNReplyList[i].MenuList[0]=pMenu;
      rc = wsCombinePSCNs(&(pTemp[iNumCurrentSCNs]), &(pEnv->pPSCNBuffer[j]), &(SCNReplyList[i]));
      if(rc == WS_ERR_BUFFER_OVERFLOW)
      {
        if(WS_CL_DEBUG & WS_DEBUG_DATA_WARNINGS)
          logit("t","Overflow of server menus supporting [%s,%s,%s]."
                "  Some discarded.\n",
                SCNReplyList[i].sta, SCNReplyList[i].chan, 
                SCNReplyList[i].net
               );
      }
      else if(rc == WS_WRN_FLAGGED)
      {
        /* ah crud, we have an existing reference to the new server.
           this shouldn't happen.
        ***********************************************************/
        logit("t","wsMergeMenuIntoPSCNList():  WARNING WARNING!! "
          "Bug in SCN processing, existing matching server reference "
          "found.  Deleting current reference.\n"
          );
      }
      else if(rc != WS_ERR_NONE)
      {
        if(WS_CL_DEBUG & WS_DEBUG_SERVER_ERRORS)
          logit("t","%s: unexpected retcode[%d] from %s\n",
                "wsMergeMenuIntoPSCNList()", rc,
                "wsCombinePSCNs()");
      }
      
      iNumCurrentSCNs++;
      /* we should probably do something about storing
      tStart and tEnd for each server's data for each PSCN
      *******************************************************/
    }  /* if SCNReplyList.SCN = pPSCNBuffer.SCN */
    else
    {
      /* business as usual.  The current record from the new menu comes after the
         current record from the combined menu source.  Add the record from the
         combined menu, and continue to the top of the loop.
      ****************************************************/
      if(WS_CL_DEBUG & WS_DEBUG_OVERKILL) 
        logit("", "wsMergeMenuIntoPSCNList: in loop after while-sub\n");
      
      /* we have an existing PSCN to be copied over to the new list. */
      memcpy(&(pTemp[iNumCurrentSCNs]), &(pEnv->pPSCNBuffer[j]),  sizeof(WS_PSCN_REC));
      iNumCurrentSCNs++;
    }

  }  /* end for i and j in PSCN lists */

  if(WS_CL_DEBUG & WS_DEBUG_OVERKILL) 
    logit("", "wsMergeMenuIntoPSCNList: after looping through existing PSCNs\n");
  
  if(j < pEnv->iNumPSCNs  &&  i < SCNsFound)
  {
    logit("et","wsMergeMenuIntoPSCNList():  ERROR ERROR Internal error!  Bug!\n");
    ret = WS_ERR_UNKNOWN;
    goto Abort;
  }

  if(WS_CL_DEBUG & WS_DEBUG_OVERKILL) 
    logit("", "wsMergeMenuIntoPSCNList: checking(pEnv->iNumPSCNs)\n");

  if(j < pEnv->iNumPSCNs)
  {
    /* the easy case.  Just copy the rest of the old ones over */
    memcpy(&(pTemp[iNumCurrentSCNs]), &(pEnv->pPSCNBuffer[j]), 
      (pEnv->iNumPSCNs - j) * sizeof(WS_PSCN_REC));
    iNumCurrentSCNs += pEnv->iNumPSCNs - j;
  }
  else if (i < SCNsFound)
  {
    for(ctr=i; ctr < SCNsFound; ctr++)
    {
      memcpy(&(pTemp[iNumCurrentSCNs]), &(SCNReplyList[ctr]),  sizeof(WS_PSCN_REC));
      pTemp[iNumCurrentSCNs].MenuList[0] = pMenu;
      pTemp[iNumCurrentSCNs].iNumMenus = 1;
      iNumCurrentSCNs++;
    }
  }

  if(WS_CL_DEBUG & WS_DEBUG_OVERKILL) 
    logit("e", "wsMergeMenuIntoPSCNList: after checking(pEnv->iNumPSCNs)\n");
  /* now everything is merged into the pTemp list. */
  /* swap lists, so that pPSCNBuffer now points at the new list,
     and pTemp points at the old. */
  pTemp2 = pTemp;
  pTemp = pEnv->pPSCNBuffer;
  pEnv->pPSCNBuffer = pTemp2;

  /* update the number of PSCNs in the environment struct */
  pEnv->iNumPSCNs = iNumCurrentSCNs;

  /* set the return code to no error */
  ret = WS_ERR_NONE;

  if(WS_CL_DEBUG & WS_DEBUG_OVERKILL) 
    logit("", "wsMergeMenuIntoPSCNList: freeing(pTemp)\n");

Abort:
  if(pTemp)
    free(pTemp);

  if(WS_CL_DEBUG & WS_DEBUG_OVERKILL) 
    logit("", "wsMergeMenuIntoPSCNList: end!\n");

  return(ret);

}  /* end wsMergeMenuIntoPSCNList() */



int wsCombinePSCNs(WS_PSCN pResult, WS_PSCN pIn1, WS_PSCN pIn2)
{

  int i,j,k;
  int bMatches  =FALSE;
  int bOverFlow =FALSE;
  int iRemainingMenusToCopy;
  int ctr;


  /* initialize the counters for the input WS_PSCNs */
  i=j=0;

  /* copy the the bulk of pIn1 over to pResult */   
  memcpy(pResult, pIn1,  sizeof(WS_PSCN_REC));

  /* now merge the two menulists */
  for(k=0; i< pIn1->iNumMenus   &&  
           j < pIn2->iNumMenus  &&
           k < (sizeof(pResult->MenuList)/sizeof(WS_MENU)); k++)
  {
    if(pIn1->MenuList[i]->menunum < pIn2->MenuList[j]->menunum)
    {
      /* process 1 */
      pResult->MenuList[k]=pIn1->MenuList[i];
      i++;
    }
    else if(pIn1->MenuList[i]->menunum > pIn2->MenuList[j]->menunum)
    {
      /* process 2 */
      pResult->MenuList[k]=pIn2->MenuList[j];
      j++;
    }
    else
    {
      /* ruh roh!  We have a double match.  Return, success, but with matches */
      bMatches = TRUE;

      /* process both 1&2 */
      pResult->MenuList[k]=pIn1->MenuList[i];
      i++; 
      j++;
    }
  }  /* end for merge of pin1 menus and pin2 menus */

  /* we hit the end of either pIn1, pIn2, or pResult */

  /* check for pResult overflow */
  if(k == (sizeof(pResult->MenuList)/sizeof(WS_MENU)))
  {
    /* we overflowed pResult */
    bOverFlow = TRUE;
  }
  else if(i< pIn1->iNumMenus)  
  {
    iRemainingMenusToCopy = pIn1->iNumMenus - i;
    if((iRemainingMenusToCopy + k) > (sizeof(pResult->MenuList)/sizeof(WS_MENU)))
    {
      iRemainingMenusToCopy = (sizeof(pResult->MenuList)/sizeof(WS_MENU)) - k;
      bOverFlow = TRUE;
    }
    for(ctr=i; ctr < i+ iRemainingMenusToCopy; ctr++)
    {
      pResult->MenuList[k] = pIn1->MenuList[ctr];
      k++;
    }
  }
  else if(j< pIn2->iNumMenus)  
  {
    iRemainingMenusToCopy = pIn2->iNumMenus - j;
    if((iRemainingMenusToCopy + k) > (sizeof(pResult->MenuList)/sizeof(WS_MENU)))
    {
      iRemainingMenusToCopy = (sizeof(pResult->MenuList)/sizeof(WS_MENU)) - k;
      bOverFlow = TRUE;
    }
    for(ctr=j; ctr < j+ iRemainingMenusToCopy; ctr++)
    {
      pResult->MenuList[k] = pIn2->MenuList[ctr];
      k++;
    }
  } 

  /* DK 021402  Oops we forgot to do this earlier.  Doh!!! */
  pResult->iNumMenus = k;

  if(bOverFlow)
    return(WS_ERR_BUFFER_OVERFLOW);
  else if(bMatches)
    return(WS_WRN_FLAGGED);
  else
    return(WS_ERR_NONE);
}  /* end wsCombinePSCNs() */



int wsAddMenuToList(wsEnvironmentStruct * pEnv, WS_MENU menu)
{

  WS_MENU * pTemp;
  int ret;

  if ( !pEnv || !menu)
  {
    ret = WS_ERR_INPUT;
    logit("t","wsAddMenuToList: bad params pEnv(%u), menu(%u)\n", pEnv,menu);
    goto Abort;
  }

  /* allocate a larger list if we don't have one or it is already full */
  if(pEnv->iNumMenusInList == pEnv->iMenuListSize)
  {
    pTemp = pEnv->MenuList;
    /* create a new menu list (allocate an array of menu pointers) */
    pEnv->MenuList = (WS_MENU *) 
        malloc((wsNUMBER_OF_MENUS+pEnv->iMenuListSize)  * sizeof(WS_MENU));
    if(!pEnv->MenuList)
    {
      ret = WS_ERR_MEMORY;
      logit("t", "wsAddMenuToList: memory allocation error for %d bytes\n",
            (wsNUMBER_OF_MENUS+pEnv->iMenuListSize)  * sizeof(WS_MENU));
      goto Abort;
    }
    /* DK Change 02/14/02 */
    pEnv->iMenuListSize += wsNUMBER_OF_MENUS;

    if(pTemp)
    {
      /* DK fixed memory error resulting in segfault 02/14/02 */
      memcpy(pEnv->MenuList,pTemp,pEnv->iNumMenusInList * sizeof(WS_MENU));
      /* end DK 02/14/02 */
      free(pTemp);
    }
  }

  /* append the menu ptr to the end of the list */
  pEnv->MenuList[pEnv->iNumMenusInList] = menu;

  /* get a new menu# */
  (pEnv->MenuList[pEnv->iNumMenusInList])->menunum = pEnv->iNextMenuNum;
  pEnv->iNextMenuNum++;
  pEnv->iNumMenusInList++;
  ret = WS_ERR_NONE;

Abort:

  return(ret);
}  /* end wsAddMenuToList() */


/**************************************************************************
*      wsFetchMenu: fetches a menu from a single wave_server.  Starts and *
*      finishes with closed sockets.                                      *
**************************************************************************/

int wsFetchMenu( WS_MENU menu, WS_PSCN *pSCNBuffer, 
                 int *pSCNBufferLen, int timeout)
                 /*
                 Arguments:
                  menu:       WS_MENU containing info for the server from 
                              which we are retrieving a menu.
                  pSCNBuffer: pointer to a WS_PSCN.  The function will set
                              the WS_PSCN to a buffer allocated and filled
                              with SCNs by the function.
                  pSCNBufferLen:
                              pointer to a caller allocated integer where 
                              the function will write the number of SCNs
                              retrieved.
                  timeout:    timeout interval in milliseconds.

                 return:  
                  WS_ERR_NONE:  if all went well.
                  WS_ERR_NO_CONNECTION: if we could not get a connection.
                  WS_ERR_SOCKET: if we could not create a socket.
                  WS_ERR_BROKEN_CONNECTION:  if the connection broke.
                  WS_ERR_TIMEOUT: if a timeout occured.
                  WS_ERR_MEMORY: if out of memory.
                  WS_ERR_INPUT: if bad input parameters.
                  WS_ERR_PARSE: if parser failed.
                 */
{


  char request[wsREQLEN];
  int len, ret;
  char reply[wsREPLEN];
  int rc;



  /* Initialize space for reply from wave_server */
  memset(reply,0,wsREPLEN);
  
  /* Connect to the server */
  if ( wsAttachServer( menu, timeout) != WS_ERR_NONE )
  {
    ret = WS_ERR_NO_CONNECTION;
    goto Abort;
  }

  /* Generate a menu request */
  sprintf( request, "MENU: %d \n", menu_reqid++ );
  len = strlen(request);

  /* Send the menu request */
  if(( ret =  send_ew( menu->sock, request, len, 0, timeout ) ) != len ) 
  {
    if(ret < 0 )
    {
      if(WS_CL_DEBUG & WS_DEBUG_SERVER_ERRORS)
        logit("t", "wsFetchMenu: connection broke to server %s:%s\n",
              menu->addr, menu->port);
      ret = WS_ERR_BROKEN_CONNECTION;
    } 
    else 
    {
     if(WS_CL_DEBUG & WS_DEBUG_SERVER_ERRORS)
       logit("t", "wsFetchMenu: server %s:%s timed out\n",
             menu->addr, menu->port);
      ret = WS_ERR_TIMEOUT;
    }
    goto Abort;
  }
  
  /* Get the menu reply */
  ret = wsWaitForServerResponse( menu, reply, wsREPLEN, timeout, WS_TRACE_ASCII );
  if ( ret != WS_ERR_NONE && ret != WS_ERR_BUFFER_OVERFLOW )
    /* we might have received some useful data in spite of the overflow */
  {
    goto Abort;
  }

  /* We have received the server's reply, so detach! */
  if(WS_CL_DEBUG & WS_DEBUG_SERVER_INFO)
    logit("t","Menu fetched, detaching server\n");
  wsDetachServer(menu, 0);

  if(WS_CL_DEBUG & WS_DEBUG_OVERKILL) 
    logit("e", "wsFetchMenu: calling wsParseMenuReply()\n");

  /* Parse the menu reply */
  if((rc = wsParseMenuReply( menu, reply)) 
     != WS_ERR_NONE )
  {
    if ( ret == WS_ERR_BUFFER_OVERFLOW && rc == WS_ERR_PARSE )
    {
      logit("", "wsFetchMenu: buffer overflow; parse failure\n");
    } 
    else
    {
      ret = rc;
    }
    goto Abort;
  }

  /* make sure that we got something back in the menu reply */
  if ( menu->pscn == NULL )
  {
    if(WS_CL_DEBUG & WS_DEBUG_SERVER_ERRORS)
      logit("t", "wsFetchMenu: no SCN at server %s:%s\n", 
            menu->addr, menu->port);
    ret = WS_ERR_EMPTY_MENU;
    goto Abort;
  }

  if(WS_CL_DEBUG & WS_DEBUG_OVERKILL) 
    logit("", "wsFetchMenu: calling wsConvertLinkedListToArray()\n");

  /* convert from the linked list of SCNs for the menu to an array */
  if((rc = wsConvertLinkedListToArray(menu, pSCNBuffer, pSCNBufferLen))
     != WS_ERR_NONE)
  {
    ret=rc;
    logit("","wsConvertLinkedListToArray() failed, returning [%d]\n", rc);
    goto Abort;
  }

  if(WS_CL_DEBUG & WS_DEBUG_OVERKILL) 
    logit("", "wsFetchMenu: calling qsort(pSCNBuffer)\n");
  /* sort the array of SCNs */
  qsort(*pSCNBuffer, *pSCNBufferLen, sizeof(WS_PSCN_REC), Compare_WS_PSCN_RECs);

  /*we're done, pack it up and go home. */
  ret = WS_ERR_NONE;

Abort:
  if(WS_CL_DEBUG & WS_DEBUG_OVERKILL) 
    logit("", "wsFetchMenu: done.\n");
  return(ret);
}  /* end wsFetchMenu() */


static int wsConvertLinkedListToArray(WS_MENU menu, WS_PSCN *pSCNBuffer, 
                                      int *pSCNBufferLen)
{
  /* 
      params:
        menu   a menu ptr for a single server
        pSCNBuffer
  
      return codes:
        WS_ERR_NONE
        WS_ERR_PARSE
        WS_ERR_MEMORY
  *****************/

  int iNumElements=0;
  WS_PSCN pSCN;
  WS_PSCN SCNList=NULL;
  int ret;

  /* traverse the linked list of PSCNs */
  pSCN=menu->pscn;
  for(iNumElements = 0; pSCN!=NULL; pSCN = pSCN->next)
  {
    iNumElements++;
  }

  /* if we didn't find any PSCNs then something went wrong.  Blame
     wsParseMenuReply(), it should not have handed out an empty list.
     Maybe it is wsFetchMenu()'s fault, but it may slap us around if
     we blame it.
  ******************************************************************/
  if(!iNumElements)
  {
    ret = WS_ERR_PARSE;
    logit("t","ERROR: wsConvertLinkedListToArray() could not make sense of "
          "SCN list generated by wsParseMenuReply() for [%s,%s]\n",
          menu->addr,menu->port);
    goto Abort;
  }
  /*else*/

  /* allocate an array of PSCNs based on the number of elements we found
     in the linked list above. 
  ******************************************************************/
  SCNList = (WS_PSCN) malloc(iNumElements * sizeof(WS_PSCN_REC));

  if(!SCNList)
  {
    ret = WS_ERR_MEMORY;
    logit("t", "wsConvertLinkedListToArray(): memory allocation "
          " error for %d WS_PSCN_RECs\n",
          iNumElements);
    goto Abort;
  }


  /* traverse the linked list of PSCNs, and copy each one. */
  pSCN=menu->pscn;
  for(iNumElements = 0; pSCN!=NULL; pSCN = pSCN->next)
  {
    memcpy(&(SCNList[iNumElements]),pSCN,sizeof(WS_PSCN_REC));
    iNumElements++;
  }

  /* set the length of the output buffer to the number of elements we 
     found and copied while traversing the list.
  ******************************************************************/
  *pSCNBufferLen = iNumElements;

  /* set the caller's SCNBuffer to point at the new SCNList */
  *pSCNBuffer = SCNList;

  /* delete the linked list */
  wsKillPSCN(menu->pscn);

  ret = WS_ERR_NONE;

Abort:
  return(ret);
}  /* end wsConvertLinkedListToArray() */


/* wsParseMenuReply is untouched from ws_clientII */
/***********************************************************************
* wsParseMenuReply: parse the reply we got from the waveserver into   *
* a menu list. Handles replies to MENU, MENUPIN and MENUSCN requests. *
***********************************************************************/
static int wsParseMenuReply( WS_MENU menu, char* reply )
{
/* Arguments:
*       menu: pointer to menu structure to be allocated and filled in.
*      reply: pointer to reply to be parsed.
*   Returns: WS_ERR_NONE:  if all went well
*            WS_ERR_INPUT: if bad input parameters
*            WS_ERR_PARSE: if we couldn't parse the reply
*            WS_ERR_MEMORY: if out of memory
  */
  int reqid = 0;
  int pinno = 0;
  char sta[7];
  char chan[9];
  char net[9];
  double tankStarttime = 0.0, tankEndtime = 0.0;
  char datatype[3];
  int scn_pos = 0;
  
  if ( !reply || !menu )
  {
    if(WS_CL_DEBUG & WS_DEBUG_SERVER_ERRORS) 
      logit("t", "wsParseMenuReply: WS_ERR_INPUT\n");
    return WS_ERR_INPUT;
  }
  
  if ( sscanf( &reply[scn_pos], "%d", &reqid ) < 1 )
  {
    if(WS_CL_DEBUG & WS_DEBUG_SERVER_ERRORS) 
      logit( "t","wsParseMenuReply(): error parsing reqid\n" );
    return WS_ERR_PARSE;
  }
  wsSkipN( reply, 1, &scn_pos );
  while ( reply[scn_pos] && reply[scn_pos] != '\n' )
  {
    WS_PSCN pscn = NULL;
    if ( sscanf( &reply[scn_pos], "%d %s %s %s %lf %lf %s",
      &pinno, sta, chan, net,
      &tankStarttime, &tankEndtime, datatype ) < 7 )
    {
      if(WS_CL_DEBUG & WS_DEBUG_SERVER_ERRORS) 
        logit( "t","wsParseMenuReply(): error decoding reply<%s>\n",
              &reply[scn_pos] );
      return WS_ERR_PARSE;
    }
    pscn = ( WS_PSCN_REC* )calloc(sizeof(WS_PSCN_REC),1);
    if ( !pscn )
    {
        logit("t", "wsParseMenuReply(): error allocating memory(%d)\n",
              sizeof(WS_PSCN_REC));
      return WS_ERR_MEMORY;
    }
    
    pscn->next = menu->pscn;
    pscn->pinno = pinno;
    strcpy( pscn->sta, sta );
    strcpy( pscn->chan, chan );
    strcpy( pscn->net, net );
    pscn->tankStarttime = tankStarttime;
    pscn->tankEndtime = tankEndtime;
    menu->pscn = pscn;
    wsSkipN( reply, 7, &scn_pos );
  }
  
  return WS_ERR_NONE;
}  /* wsParseMenuReply() */



static int Compare_WS_PSCN_RECs(const void *elem1, const void *elem2) 
{
  WS_PSCN pscn1,pscn2;
  int ret;

  pscn1 = (WS_PSCN) elem1;
  pscn2 = (WS_PSCN) elem2;

  ret= strcmp(pscn1->sta, pscn2->sta);
  if(ret)
    return(ret);

  /*else*/
  ret= strcmp(pscn1->chan, pscn2->chan);
  if(ret)
    return(ret);

  /*else*/
  ret= strcmp(pscn1->net, pscn2->net);
  return(ret);
}  /* end Compare_WS_PSC_RECs() */



/* wsAttachServer is untouched from ws_clientII */
/***********************************************************************
*  wsAttachServer: Open a connection to a server. The timeout starts  *
*    when connect() is called by connect_ew() in socket_ew_common.c   *
***********************************************************************/
int wsAttachServer( WS_MENU menu, int timeout )
/*
Arguemnts:
menu: pointer to the menu of the server
timeout: time interval in milliseconds; use -1 for no timeout.
returns: WS_ERR_NONE: if all went well.
WS_ERR_INPUT: if menu is missing.
WS_ERR_SOCKET: if a socket error occurred.
WS_ERR_NO_CONNECTION: if a connection could not be established
*/
{
  int                ret = WS_ERR_NONE;
  int                sock = 0;   /* Socket descriptor                  */
  struct sockaddr_in s_in ;      /* Server's socket address stucture   */
  
  if ( !menu )
  {
    ret = WS_ERR_INPUT;
      logit( "", "wsAttachServer(): WS_ERR_INPUT\n");
    goto Abort;
  }
  if ( menu->sock > 0 )  /* maybe already connected, so disconnect first */
  {
    if(WS_CL_DEBUG & WS_DEBUG_SERVER_INFO) 
      logit("","Existing socket during wsAttachServer(), detaching server\n");
    wsDetachServer( menu, 0);
  }
  
  /* open a non_blocking socket
  *****************************/
  if ( ( sock = socket_ew( AF_INET, SOCK_STREAM, 0 ) ) == -1 )
  {
    ret = WS_ERR_SOCKET;
    if(WS_CL_DEBUG & WS_DEBUG_SERVER_ERRORS) 
      logit( "", "wsAttachServer(): socket_ew() call failed\n" );
    goto Abort;
  }
  
  /* Stuff address and port into socket structure
  ********************************************/
  memset( (char *)&s_in, '\0', sizeof(s_in) );
  s_in.sin_family = AF_INET;
  s_in.sin_port   = htons( (short)atoi(menu->port) );
  
#ifdef _LINUX
  if ((int)(s_in.sin_addr.s_addr = inet_addr(menu->addr)) == -1)
#else
    if ((int)(s_in.sin_addr.S_un.S_addr = inet_addr(menu->addr)) == -1)
#endif
    {
      ret = WS_ERR_NO_CONNECTION;
      if(WS_CL_DEBUG & WS_DEBUG_SERVER_ERRORS) 
        logit( "", "wsAttachServer(): inet_addr failed on <%s>\n",
        menu->addr );
      goto Abort;
    }

    if(WS_CL_DEBUG & WS_DEBUG_SERVER_INFO) 
      logit("t","\n  Connecting Server (%s:%s) - ", menu->addr,menu->port);

    if(menu->stats.bConnected)
    {
      logit("t","wsAttachServer(): Warning: error in stats package, "
            "connecting to an already active connection(%s:%s).\n",
            menu->addr,menu->port);
    }
    hrtime_ew(&dAttempting);
    menu->stats.iNumTotalConnectionsAttempted++;

    if ( connect_ew( sock, (struct sockaddr *)&s_in, sizeof(s_in), timeout) == -1 )
    {
      if(WS_CL_DEBUG & WS_DEBUG_SERVER_INFO) 
        logit("t","FAILED\n");
      else if(WS_CL_DEBUG & WS_DEBUG_SERVER_ERRORS) 
        logit("t","\n  Connecting Server (%s:%s) - FAILED!\n",
              menu->addr,menu->port);

      ret = WS_ERR_NO_CONNECTION;
      if(WS_CL_DEBUG & WS_DEBUG_SERVER_ERRORS) 
      {
        int iSocketError = socketGetError_ew();
        logit( "t", "wsAttachServer(): connect() call failed:");
        if(iSocketError == CONNECT_WOULDBLOCK_EW)
         logit("","  connection TIMED OUT after %d seconds\n", timeout/1000);
        else if(iSocketError == CONNREFUSED_EW)
         logit("","  connection REFUSED.\n");
        else
         logit("","  unrecognized error(%d).\n",iSocketError);
      }

        
      goto Abort;
    }
    menu->sock = sock;
    if(WS_CL_DEBUG & WS_DEBUG_SERVER_INFO) 
      logit("e","SUCCESS\n");

    hrtime_ew(&dNow);
    menu->stats.dTotalTimeOverhead += dNow - dAttempting;
    menu->stats.bConnected = TRUE;
    menu->stats.dConnectedSince = dNow;
    menu->stats.iNumTotalConnections++;


    ret = WS_ERR_NONE;
    return ret;
    
    /* An error occured;
    * don't blab about here since we already did earlier. */
Abort:
    menu->sock = -1; /* mark the socket as dead */
    return ret;
}


/* wsDetachServer is untouched from ws_clientII */
/*********************************************************************
* wsDetachServer: Immediately disconnect from a socket if it's open *
*********************************************************************/
void wsDetachServer( WS_MENU menu, int iError )
/*  
Arguments:
menu: menu of server to be detached
*/
{

  if ( !menu || menu->sock == -1 )
    return;

  if(WS_CL_DEBUG & WS_DEBUG_SERVER_INFO) 
    logit("t","Disconnecting Server (%s:%s)\n", menu->addr,menu->port);

  if(!menu->stats.bConnected)
  {
    logit("t","wsDetachServer(): Warning: error in stats package, "
      "disconnecting an inactive connection(%s:%s).\n",
      menu->addr,menu->port);
  }
  hrtime_ew(&dAttempting);
 
  
  closesocket_ew( menu->sock, SOCKET_CLOSE_IMMEDIATELY_EW );
  menu->sock = -1;


  hrtime_ew(&dNow);
  menu->stats.dTotalTimeOverhead += dNow - dAttempting;
  if(menu->stats.bConnected)
    menu->stats.dTotalTimeConnected += dAttempting - menu->stats.dConnectedSince;
  menu->stats.dConnectedSince = 0;
  menu->stats.bConnected = FALSE;
  if(iError)
    menu->stats.iNumErrors++;


}  /* end wsDetachServer() */





/* untouched from ws_clientII */
/**************************************************************************
*      wsSkipN: moves forward the pointer *posp in buf by moving forward *
*      cnt words.  Words are delimited by either space or horizontal     *
*      tabs; newline marks the end of the buffer.                        *
**************************************************************************/
static void wsSkipN( char* buf, int cnt, int* posp )
{
  int pos = *posp;
  
  while ( cnt )
  {
    while ( buf[pos] != ' ' && buf[pos] != '\t' )
    {
      if ( !buf[pos] )
      {
        goto done;
      }
      if ( buf[pos] == '\n' )
      {
        ++pos;
        goto done;
      }
      ++pos;
    }
    --cnt;
    while ( buf[pos] == ' ' || buf[pos] == '\t' )
    {
      ++pos;
    }
  }
done:
  *posp = pos;
}

/* untouched from ws_clientII */
int setWsClient_ewDebug(int debug)
{
/* setWsClient_ewDebug() turns debugging on or off for 
the ws_clientII routines
  */

  if(debug)
    logit("t", "Setting WS_CL_DEBUG to (0x%x).\n",debug);

  WS_CL_DEBUG=debug;
  return(0);
}


int wsRemoveServer(wsHANDLE pEnv, char * szIPAddr, char * szPort)
{
  int i, rc;
  int iMenuPos;
  WS_MENU pMenu=NULL;

  for(i=0; i< pEnv->iNumMenusInList; i++)
  {
    if(!strcmp(pEnv->MenuList[i]->addr,szIPAddr))
    {
      if(!strcmp(pEnv->MenuList[i]->port,szPort))
      {
        pMenu = pEnv->MenuList[i];
        break;
      }
    }
  }

  iMenuPos = i;

  if(!pMenu)
  {
    return(WS_ERR_MENU_NOT_FOUND);
  }
  /* else */
  /* now we must delete the menu */

  rc = wsRemoveMenuFrompPSCNBuffer(pEnv, pMenu);
  if(rc == WS_WRN_FLAGGED)
  {
    logit("t","%s: Warning menu [%s:%s] not found in pPSCNBuffer.  Bug!\n",
          "wsRemoveServer()",pMenu->addr, pMenu->port);
  }

  /* first remove it from the environment's menulist */
  for(i=iMenuPos; i < pEnv->iNumMenusInList - 1; i++)
  {
    pEnv->MenuList[i] = pEnv->MenuList[i+1];
  }
  
  /* free the menu */
  free(pMenu);

  return(WS_ERR_NONE);
}  /* end wsRemoveServer() */



static int wsRemoveMenuFrompPSCNBuffer(wsHANDLE pEnv, WS_MENU pMenu)
{

  int i,j,ctr;
  int bMenuFound = FALSE;

  /* we now have a WS_MENU that we must delete from the pPSCNBuffer */
  for(i=0; i < pEnv->iNumPSCNs; i++)
  {
    for(j=0; j < pEnv->pPSCNBuffer[i].iNumMenus; j++)
    {
      if(pEnv->pPSCNBuffer[i].MenuList[j] == pMenu)
      {
        bMenuFound = TRUE;
        for(ctr=j; ctr < pEnv->pPSCNBuffer[i].iNumMenus - 1; ctr++)
        {
          pEnv->pPSCNBuffer[i].MenuList[ctr] = 
            pEnv->pPSCNBuffer[i].MenuList[ctr+1];
        }
        break;
      }
    }  /* end for j in iNumMenus for current PSCN */
  }    /* end for i in iNumPSCNs for current environment */

  if(bMenuFound)
    return(WS_ERR_NONE);
  else
    return(WS_WRN_FLAGGED);

}  /* end wsRemoveMenuFrompPSCNBuffer() */


void wsDestroy(wsHANDLE * ppEnv )
{
  /* we could use wsRemoveServer() to drop each server,
     but much more efficient to just wipe it all out. */

  int i;
  wsHANDLE pEnv = *ppEnv;

  /* go through each menu */
  for(i=0; i < pEnv->iNumMenusInList; i++);
  {
    /* close socket if neccessary */
    if(pEnv->MenuList[i]->sock != -1)
    {
      if(WS_CL_DEBUG & WS_DEBUG_SERVER_INFO) 
        logit("t","wsDestroy(), detaching server\n");
      wsDetachServer(pEnv->MenuList[i],0);
    }

    /* free menu */
    free(pEnv->MenuList[i]);
  }

  /* free PSCN buffer */
  free(pEnv->pPSCNBuffer);

  /* free anything else? */

  /* free the environment (kinda sounds like greenpeace) */
  free(pEnv);

  /* null out the handle */
  *ppEnv = NULL;
}  /* end wsDestroy() */


/*********************************************************************
* wsGetTrace: retrieves the piece of raw trace data specified in     *
* the structure 'getThis': The current menu list, as built by the    *
* routines above will be searched for a matching SCN. If a match     *
* is found, the associated wave server will be contacted, and a      *
* request for the trace snippet will be made.                        *
*********************************************************************/
/* THIS ROUTINE IS UNTESTED!! ora_trace_fetch uses wsGetNextTraceFromRequestList() instead */
int wsGetTrace(TRACE_REQ* getThis, wsHANDLE pEnv, int iTraceType, int timeout_msec)
                  /*
                  Arguments:
                  getThis:   a TRACE_REQ structure (see ws_client.h), with the
                  request portion filled in.
                  wsHANDLE:  a handle for the current environment
                  timeout:   Time in milliseconds to wait for reply
                  return:   WS_ERR_NONE: all went well.
                  WS_WRN_FLAGGED: wave server returned error flag instead
                  of trace data.
                  WS_ERR_EMPTY_MENU: No menu list found.
                  WS_ERR_SCN_NOT_IN_MENU: SCN not found in menu list.
                  WS_ERR_NO_CONNECTION: if socket was already closed
                  The socket will be closed for the following:
                  WS_ERR_BUFFER_OVERFLOW: trace buffer supplied too small.
                  WS_ERR_TIMEOUT: if a timeout occured
                  WS_ERR_BROKEN_CONNECTION: if a connection broke.
                  WS_ERR_SOCKET: problem changing socket options.
                  */
{
  int rc;
  int ret = WS_ERR_TIMEOUT;
  int first_error = WS_ERR_NONE;

  WS_MENU menu = NULL;
  double dnow, dTimeoutStart, dTimeoutSeconds;
  int  bTimeout;

  if(timeout_msec != -1)
  {
    bTimeout = TRUE;
    dTimeoutStart = hrtime_ew(&dnow);
    dTimeoutSeconds = (double)(timeout_msec)/1000;
    if(hrtime_ew(&dnow) > dTimeoutStart + dTimeoutSeconds)
      return(WS_ERR_TIMEOUT);
  }
  else
  {
    bTimeout = FALSE;
  }

  /* check pointer input params */
  if (!(getThis && pEnv))
  {
    logit("t", "wsGetTrace: NULL input ptr: getThis[%u], pEnv[%u]\n",
          getThis, pEnv);
    return WS_ERR_INPUT;
  }

  /* check iTraceType input param */
  if(!(iTraceType == WS_TRACE_BINARY || iTraceType == WS_TRACE_ASCII))
  {
    logit("t", "wsGetTrace: iTraceType is not valid!  "
          "Must be either WS_TRACE_BINARY or WS_TRACE_ASCII\n");
    return WS_ERR_INPUT;
  }

  while(rc = wsSearchSCN( getThis, &menu, pEnv) == WS_ERR_NONE)
  {
    if(bTimeout)
    {
      /* reset the timeout, since we are potentially processing
         multiple servers here (iteratively) and we don't want
         to preclude one server's chances to answer the request
         just because another timed out.
         DK 021702
      **********************************************************/
       dTimeoutStart = hrtime_ew(&dnow);
       if(hrtime_ew(&dnow) > dTimeoutStart + dTimeoutSeconds)
         break;
    }

    if(bTimeout)
    {
      rc = wsGetTraceFromServer(getThis, menu, iTraceType, 
                                (int)((dTimeoutStart + dTimeoutSeconds - hrtime_ew(&dnow)) * 1000));
    }
    else
    {
      rc = wsGetTraceFromServer(getThis, menu, iTraceType, -1/*timeout*/);
    }

    if(rc == WS_ERR_NONE || rc == WS_ERR_BUFFER_OVERFLOW)
    {
      ret = rc;
      break;
    }
    else if(first_error == WS_ERR_NONE)
    {
      first_error = rc;
    }
  }  /* end while wsSearchSCN */

  /* now the tricky part of getting the error code right */
  /* we have only set ret if we got the best data possible,
     meaning success, or buffer overflow due to the 
     client's buffer wasn't big enough.  So unless we 
     accomplished our mission, ret is still WS_ERR_TIMEOUT.
     Now we have to figure out what ret should be.  First check
     first_error, as this should be the first and most meaningful
     error we received.  If first_error is error free, then next
     check err, in case we ran into problems locating the PSCN.
     If err has no error information, then we must have timed out
     at the top of the loop.
  ****************************************************************/
  if(ret == WS_ERR_TIMEOUT)
  {
    if(first_error != WS_ERR_NONE)
    {
      ret = first_error;
    }
    else if(rc != WS_ERR_NONE)
    {
      ret = rc;
    }
    /* else it was a genuine timeout, so change nothing */
  }

  return(ret);
} /* end wsGetTrace() */


int wsGetTraceFromServer(TRACE_REQ* getThis, WS_MENU menu, int iTraceType, int timeout_msec)
                  /*
                  Arguments:
                  getThis:   a TRACE_REQ structure (see ws_client.h), with the
                  request portion filled in.
                  wsHANDLE:  a handle for the current environment
                  timeout:   Time in milliseconds to wait for reply
                  return:   WS_ERR_NONE: all went well.
                  WS_WRN_FLAGGED: wave server returned error flag instead
                  of trace data.
                  WS_ERR_EMPTY_MENU: No menu list found.
                  WS_ERR_SCN_NOT_IN_MENU: SCN not found in menu list.
                  WS_ERR_NO_CONNECTION: if socket was already closed
                  The socket will be closed for the following:
                  WS_ERR_BUFFER_OVERFLOW: trace buffer supplied too small.
                  WS_ERR_TIMEOUT: if a timeout occured
                  WS_ERR_BROKEN_CONNECTION: if a connection broke.
                  WS_ERR_SOCKET: problem changing socket options.
                  */

  {

  int len,rc;
  int err = WS_ERR_NONE;

  double dnow, dTimeoutStart, dTimeoutSeconds;
  char request[wsREQLEN];
  
  int  bTimeout;

  if(timeout_msec != -1)
  {
    bTimeout = TRUE;
    dTimeoutStart = hrtime_ew(&dnow);
    dTimeoutSeconds = (double)(timeout_msec)/1000;
    if(hrtime_ew(&dnow) > dTimeoutStart + dTimeoutSeconds)
      return(WS_ERR_TIMEOUT);
  }
  else
  {
    bTimeout = FALSE;
  }

  /* check pointer input params */
  if (!(getThis && menu))
  {
    logit("t", "wsGetTrace: NULL input ptr: getThis[%u], menu[%u]\n",
          getThis, menu);
    return WS_ERR_INPUT;
  }

  /* check iTraceType input param */
  if(!(iTraceType == WS_TRACE_BINARY || iTraceType == WS_TRACE_ASCII))
  {
    logit("t", "wsGetTrace: iTraceType is not valid!  "
          "Must be either WS_TRACE_BINARY or WS_TRACE_ASCII\n");
    return WS_ERR_INPUT;
  }

  /* check for live socket.  Open if not already connected. */
  if (menu->sock < 0) 
  {
    if(bTimeout)
    {
      rc=wsAttachServer(menu,(int)((dTimeoutStart + dTimeoutSeconds - hrtime_ew(&dnow)) * 1000));
    }
    else
    {
      rc=wsAttachServer(menu, -1/*timeout*/);
    }
    if(rc!= WS_ERR_NONE)
    {
      err=rc;
      goto HandleErrors;
    }
  }
  
  /* create the request string based on the iTraceType */
  if(iTraceType == WS_TRACE_BINARY)
  {
    sprintf( request, "GETSCNRAW: %d %s %s %s %lf %lf\n",
            menu_reqid++, getThis->sta, getThis->chan, getThis->net,
            getThis->reqStarttime, getThis->reqEndtime );
  }
  else if(iTraceType == WS_TRACE_ASCII)
  {
    sprintf( request, "GETSCN: %d %s %s %s %lf %lf %d\n",
            menu_reqid++, getThis->sta, getThis->chan, getThis->net,
            getThis->reqStarttime, getThis->reqEndtime, getThis->fill );
  }
  else
  {
    logit("t", "wsGetTrace: iTraceType is not valid! (check2) "
          "Must be either WS_TRACE_BINARY or WS_TRACE_ASCII\n");
    return WS_ERR_INPUT;
  }
  
  /* Calculate the length of our request */
  len = strlen(request);

  /* Send the trace request */
  if(bTimeout)
  {
    rc = send_ew(menu->sock, request, len, 0, 
                 (int)((dTimeoutStart + dTimeoutSeconds - hrtime_ew(&dnow)) * 1000));
  }
  else
  {
    rc = send_ew(menu->sock, request, len, 0, -1 /* tiemout */);
  }

  if(rc != len)
  {
    if (rc < 0 )
      err = WS_ERR_BROKEN_CONNECTION;
    else 
      err = WS_ERR_TIMEOUT;

    if(WS_CL_DEBUG & WS_DEBUG_SERVER_ERRORS) 
      logit("t","Error(%d) received while sending data to wave_server!"
                "%d bytes sent.\n",err,rc);
    
    goto HandleErrors;
  }
  
  /* Retrieve the reply to the request.  This takes different code 
  depending on the iTraceType */
  if(iTraceType == WS_TRACE_BINARY)
  {

    /* get the header for a Binary trace reply */
    if(bTimeout)
    {
      rc = wsWaitForServerResponse(menu, getThis->pBuf, getThis->bufLen, 
                                   (int)((dTimeoutStart + dTimeoutSeconds - hrtime_ew(&dnow)) * 1000),
                                   WS_TRACE_BINARY);
    }
    else
    {
      rc = wsWaitForServerResponse(menu, getThis->pBuf, getThis->bufLen, 
                                   -1 /*timeout*/, WS_TRACE_BINARY);
    }

    switch (rc) 
    {
    case WS_ERR_INPUT:
      logit("t","wsGetTrace() Error!  wsWaitBinHeader() returned InputError\n");
      err = rc;
      goto HandleErrors;
      break;
    case WS_ERR_NONE:
      break;
    default:
      /* buffer overflow, timeout, socket error, or broken connection */
      if(WS_CL_DEBUG & WS_DEBUG_SERVER_ERRORS) 
        logit("t","wsGetTrace() Error!  wsWaitBinHeader() returned %d\n",rc);
      err = rc;
      goto HandleErrors;
    }  /* end switch(rc=wsWaitForServerResponse) */
    
    /* parse the header for a Binary trace reply */
    rc = wsParseBinHeaderReply( getThis, getThis->pBuf );
    switch (rc) 
    {
    case WS_ERR_NONE:
      break;
    case WS_WRN_FLAGGED:
      getThis->actLen = 0;
      err = rc;
      if(WS_CL_DEBUG & WS_DEBUG_DATA_WARNINGS) 
        logit("","Server(%s:%s) sent flag(F%c) for req (%s %s %s) (%.3f-%.3f)\n",
              menu->addr, menu->port, getThis->retFlag, 
              getThis->sta, getThis->chan, getThis->net,
              getThis->reqStarttime, getThis->reqEndtime);
      goto HandleErrors;
      break;
    default:  /* unexpected error */
      if(WS_CL_DEBUG & WS_DEBUG_DATA_ERRORS) 
        logit("t","wsGetTrace() Error!  wsParseBinHeaderReply() returned %d\n"
                  " for req (%s %s %s) (%.3f-%.3f) - server (%s:%s)\n",
              rc, getThis->sta, getThis->chan, getThis->net,
              getThis->reqStarttime, getThis->reqEndtime, 
              menu->addr, menu->port);
      err = rc;
      goto HandleErrors;
      break;
    }  /* end switch(rc=wsParseBinHeaderReply) */
    
    /* get the actual trace data */
    if ( getThis->actLen )  /* why shouldn't it be ???? DK 01/12/01 */
    {
      long int binLen = getThis->actLen;
      
      if ( (int)getThis->bufLen < binLen )
        binLen = (int)getThis->bufLen;

      if(bTimeout)
      {
        rc = recv_all(menu->sock, getThis->pBuf, binLen, 0, 
          (int)((dTimeoutStart + dTimeoutSeconds - hrtime_ew(&dnow)) * 1000));
      }
      else
      {
        rc = recv_all(menu->sock, getThis->pBuf, binLen, 0, -1/*timeout*/);
      }
      
      if ( rc != binLen ) 
      {
        if ( rc < 0 )
        {
          err = WS_ERR_BROKEN_CONNECTION;
        } 
        else 
        {
          err = WS_ERR_TIMEOUT;
        }
        goto HandleErrors;
      }
      
      if ( binLen < getThis->actLen ) 
      {
        if(WS_CL_DEBUG & WS_DEBUG_DATA_ERRORS) 
          logit("t","wsGetTraceFromServer() WARNING!  snippet size(%d) "
                    "exceeded buffer len(%d), trace truncated\n"
                    " for req (%s %s %s) (%.3f-%.3f) - server (%s:%s)\n",
                getThis->actLen, binLen,
                getThis->sta, getThis->chan, getThis->net,
                getThis->reqStarttime, getThis->reqEndtime, 
                menu->addr, menu->port);

        getThis->actLen = binLen;
        err = WS_ERR_BUFFER_OVERFLOW;
      }
      else
      {
        err = WS_ERR_NONE;
      }
    }  /* end if actLen > 0 */
  }  /* end if WS_TRACE_BINARY */
  else if(iTraceType == WS_TRACE_ASCII)
  {
    /* get the Ascii reply */
    if(bTimeout)
    {
    rc = wsWaitForServerResponse( menu, getThis->pBuf, getThis->bufLen, 
                                 (int)((dTimeoutStart + dTimeoutSeconds - hrtime_ew(&dnow)) * 1000),
                                 WS_TRACE_ASCII );
    }
    else
    {
      rc = wsWaitForServerResponse(menu, getThis->pBuf, getThis->bufLen, 
        -1, WS_TRACE_ASCII );
    }
    if ( rc != WS_ERR_NONE && rc != WS_ERR_BUFFER_OVERFLOW )
    {
      err = rc;
      goto HandleErrors;
    }
    else
    {
      /* copy the return code into our return code */
      err = rc;
      
      /* parse the header and the trace itself */
      rc = wsParseAsciiHeaderReply( getThis, getThis->pBuf );
      if ( rc < WS_ERR_NONE )
      {
        if (err == WS_ERR_BUFFER_OVERFLOW)
          if(WS_CL_DEBUG & WS_DEBUG_DATA_ERRORS) 
            logit("t","wsGetTrace() Error!  wsParseAsciiHeaderReply overflowed buffer "
                      " for req (%s %s %s) (%.3f-%.3f) - server (%s:%s)\n",
                  getThis->sta, getThis->chan, getThis->net,
                  getThis->reqStarttime, getThis->reqEndtime, 
                  menu->addr, menu->port);
      }
      
      /* wsParseAsciiReply puts the trace data into getThis, 
      so now we're done */
    }
  }  /* end if WS_TRACE_ASCII */
  else
  {
    logit("t", "wsGetTrace: iTraceType is not valid! (check3) "
          "Must be either WS_TRACE_BINARY or WS_TRACE_ASCII\n");
    return WS_ERR_INPUT;
  }

  
HandleErrors:

  if(WS_CL_DEBUG & WS_DEBUG_SERVER_ERRORS) 
  {
    if  ( err == WS_ERR_TIMEOUT ) {
      logit( "e","wsGetTraceBin(): server %s:%s timed out\n", menu->addr,
            menu->port );
    } else if ( err == WS_ERR_BROKEN_CONNECTION ) {
      logit( "e","wsGetTraceBin(): broken connection to server %s:%s\n",
            menu->addr, menu->port);
    }
  }

  if(err != WS_ERR_NONE && err != WS_ERR_BUFFER_OVERFLOW && err != WS_WRN_FLAGGED)
  {
    if(WS_CL_DEBUG & WS_DEBUG_SERVER_ERRORS) 
      logit("t","\nAbortServer due to error(%d)in wsGetTraceFromServer()\n "
            "\t\t for SCN (%s,%s,%s) (%.3f-%.3f).  \n\t\t Detaching server.\n",err,
            getThis->sta, getThis->chan, getThis->net, 
            getThis->reqStarttime, getThis->reqEndtime);

    wsDetachServer( menu, err); 
  }



  return(err);
}  /* end wsGetTraceFromServer() */


/******************************************************************
* wsWaitForServerResponse(): Retrieve a message.  The message     *
* will be terminated by a newline.  If the message is of type     *
* ASCII, then  nothing else is expected after the newline, so we  *
* can read several characters at a time without fear of reading   *
* past the newline.  If it is of type BINARY then there is binary *
* data after the newline, so we need to read 1 character at a     *
* time, so that we don't accidentally grab some binary data.      *
*                                                                 *
* This message may have internal nulls which will be converted to *
* spaces.                                                         *
* Returns after newline is read, when timeout expires if set,     *
* or on error.                                                    *
******************************************************************/
static int wsWaitForServerResponse(WS_MENU menu, char* buf, int buflen, 
                                   int timeout_msec, int iTraceType)
/*
Arguments:
menu: menu of server from which message is received
buf: buffer in which to place the message, terminated by null.
buflen: number of bytes in the buffer.
timeout_msec: timout interval in milliseconds. 
return: WS_ERR_NONE: all went well.
WS_ERR_BUFFER_OVERFLOW: ran out of space before the message
end; calling program will have to decide how serious this is.
WS_ERR_INPUT: missing input parameters.
WS_ERR_SOCKET: error setting socket options.
WS_ERR_TIMEOUT: time expired before we read everything.
WS_ERR_BROKEN_CONNECTION: if the connection failed.
*/
{
  int ii, ir = 0;  /* character counters */
  int nr = 0;
  int done = FALSE;
  int len = 0;
  int ret;
  double dnow, dTimeoutStart, dTimeoutSeconds;
  int  bTimeout;

  if(timeout_msec != -1)
  {
    bTimeout = TRUE;
    dTimeoutStart = hrtime_ew(&dnow);
    dTimeoutSeconds = ((double)timeout_msec)/1000;
    if(hrtime_ew(&dnow) > dTimeoutStart + dTimeoutSeconds)
      return(WS_ERR_TIMEOUT);
  }
  else
  {
    bTimeout = FALSE;
  }

  if ( !buf || buflen <= 0 )
  {
    logit( "e", "wsWaitForServerResponse(): no input buffer\n");
    return WS_ERR_INPUT;
  }

  /* check iTraceType input param */
  if(!(iTraceType == WS_TRACE_BINARY || iTraceType == WS_TRACE_ASCII))
  {
    logit("t", "wsWaitForServerResponse: iTraceType is not valid!  "
          "Must be either WS_TRACE_BINARY or WS_TRACE_ASCII\n");
    return WS_ERR_INPUT;
  }
  
  while (!done)
  {
    if ((bTimeout) && hrtime_ew(&dnow) > dTimeoutStart + dTimeoutSeconds ) 
    {
      ret = WS_ERR_TIMEOUT;

      if(WS_CL_DEBUG & WS_DEBUG_SERVER_ERRORS) 
        logit("t", "wsWaitForServerResponse timed out\n");
      goto Done;
    }
    if ( ir >= buflen - 2 )
    {
      if(WS_CL_DEBUG & WS_DEBUG_SERVER_ERRORS) 
        logit( "e", "wsWaitForServerResponse(): reply buffer overflows\n" );
      ret = WS_ERR_BUFFER_OVERFLOW;
      goto Done;
    }
    /* old code 02/05/2002
    len = WS_MAX_RECV_BUF_LEN;
    if ( ir + len >= buflen - 1 )
    {
      if(iTraceType == WS_TRACE_BINARY)
        len = 1;
      else if (iTraceType == WS_TRACE_ASCII)
        len = buflen - ir - 2; /* leave room for the terminating null * /
      else
      {
        ret = WS_ERR_UNKNOWN;
        goto Done;
      }
    }
    **************************************************/
    /* new code 02/05/2002 DK */
    if(iTraceType == WS_TRACE_BINARY)
      len = 1;
    else if (iTraceType == WS_TRACE_ASCII)
      len = buflen - ir - 2; /* leave room for the terminating null */
    else
    {
      ret = WS_ERR_UNKNOWN;
      goto Done;
    }
    /* end new code 02/05/2002 */

    if(bTimeout)
      nr = recv_ew( menu->sock, &buf[ir], len, 0, (int)((dTimeoutStart + dTimeoutSeconds - hrtime_ew(&dnow)) * 1000));
    else
      nr = recv_ew( menu->sock, &buf[ir], len, 0, -1);

    if(nr == SOCKET_ERROR)  
    {
      if(socketGetError_ew() == WOULDBLOCK_EW)
      {
        ret = WS_ERR_TIMEOUT;
      }
      else
      {
        if(WS_CL_DEBUG & WS_DEBUG_SERVER_ERRORS) 
          logit("e", "wsWaitForServerResponse(): Error[%d] on socket recv()\n",
                socketGetError_ew());
        ret = WS_ERR_BROKEN_CONNECTION;
      }
      goto Done;
    }
    if ( nr > 0 )
    {
      ii = 0;
      /* got something, adjust ir and c  */
      ir += nr;
      
      /* replace NULL char in ascii string with SPACE char */
      for (ii = ir-nr; ii < ir; ii++)
      {
        if ( !buf[ii] ) buf[ii] = ' ';
        else if(buf[ii] == '\n')
        {
          done = TRUE;
           /* we are having problems with data not catching the \n because we
              are retrieving blocks of data at a time, and are only checking
              at the top of the loop.  So we have changed to check here, where
              we are examining every character.
           */
          continue;
        }
      }
    }  /* end if nr > 0 */
  }    /* end while (!done) */
  
  ret = WS_ERR_NONE;

Done:

  buf[ir] = '\0';                 /* null-terminate the reply      */

  return ret;
}


/* untouched from ws_clientII.c */
/***********************************************************************
* wsParseBinHeaderReply: parse the reply we got from the waveserver   *
* into a TRACE_REQ structure. Handles the header for replies reply to *
* GETSCNRAW requests.                                                 *
***********************************************************************/
static int wsParseBinHeaderReply( TRACE_REQ* getThis, char* reply )
{
/* Arguments:
*    getThis: pointer to TRACE_REQ structure to be filled with reply info
*      reply: pointer to reply to be parsed.
*   Returns: WS_ERR_NONE:  if all went well
*            WS_ERR_INPUT: if bad input parameters
*            WS_ERR_PARSE: if we couldn't parse part of the reply
*            WS_WRN_FLAGGED: server sent us a no-data flag
  */
  int reqid = 0;
  int pinno = 0;
  char sta[7];
  char chan[9];
  char net[9];
  char flag[9];
  char datatype[3];
  double tankStarttime = 0.0, tankEndtime = 0.0;
  int bin_len = 0;
  int scn_pos = 0;
  char *buf = NULL;
  
  if ( !reply || !getThis )
  {
    logit( "", "wsParseBinHeaderReply(): bad input parameters\n");
    return WS_ERR_INPUT;
  }
  
  if ( sscanf( &reply[scn_pos], "%d %d", &reqid, &pinno ) < 2 )
  {
    if(WS_CL_DEBUG & WS_DEBUG_SERVER_ERRORS) 
      logit( "","wsParseBinHeaderReply(): error parsing reqid/pinno\n" );
    return WS_ERR_PARSE;
  }
  wsSkipN( reply, 2, &scn_pos );
  
  if ( sscanf( &reply[scn_pos], "%s %s %s", sta, chan, net ) < 3 )
  {
    if(WS_CL_DEBUG & WS_DEBUG_SERVER_ERRORS) 
      logit( "","wsParseBinHeaderReply(): error parsing SCN\n" );
    return WS_ERR_PARSE;
  }
  wsSkipN( reply, 3, &scn_pos );
  
  if ( sscanf( &reply[scn_pos], "%s %s", flag, datatype ) < 2 )
  {
    if(WS_CL_DEBUG & WS_DEBUG_SERVER_ERRORS) 
      logit( "","wsParseBinHeaderReply(): error parsing flag/datatype\n" );
    return WS_ERR_PARSE;
  }
  wsSkipN( reply, 2, &scn_pos );
  
  if ( strlen(flag) == 1 )
  {
    if ( sscanf( &reply[scn_pos], "%lf %lf", &tankStarttime, 
      &tankEndtime ) < 2 )
    {
      if(WS_CL_DEBUG & WS_DEBUG_SERVER_ERRORS) 
        logit( "","wsParseBinHeaderReply(): error parsing starttime/endtime\n" );
      return WS_ERR_PARSE;
    }
    wsSkipN( reply, 2, &scn_pos );
    
    if ( sscanf( &reply[scn_pos], "%d", &bin_len ) < 1 )
    {
      if(WS_CL_DEBUG & WS_DEBUG_SERVER_ERRORS) 
        logit( "","wsParseBinHeaderReply(): error parsing bin_len\n" );
      return WS_ERR_PARSE;
    }
    wsSkipN( reply, 1, &scn_pos );
    
  }
  else if ( strlen(flag) == 2 )
  {
    tankStarttime = 0.0;
    tankEndtime = 0.0;
    bin_len = 0;
    if ( strcmp(flag,"FL") == 0 )
    {
      if ( sscanf( &reply[scn_pos], "%lf", &tankStarttime) < 1 )
      {
        if(WS_CL_DEBUG & WS_DEBUG_SERVER_ERRORS) 
          logit( "e","wsParseBinHeaderReply(): error parsing tank starttime\n" );
        return WS_ERR_PARSE;
      }
      wsSkipN( reply, 1, &scn_pos );
    }
    else if ( strcmp(flag,"FR") == 0 )
    {
      if ( sscanf( &reply[scn_pos], "%lf", &tankEndtime) < 1 )
      {
        if(WS_CL_DEBUG & WS_DEBUG_SERVER_ERRORS) 
          logit( "e","wsParseBinHeaderReply(): error parsing tank endtime\n" );
        return WS_ERR_PARSE;
      }
      wsSkipN( reply, 1, &scn_pos );
    }
  }
  else
  {
    if(WS_CL_DEBUG & WS_DEBUG_SERVER_ERRORS) 
      logit( "","wsParseBinHeaderReply(): bad flag[%s]\n", flag );
    return WS_ERR_PARSE;
  }
  
  getThis->pinno = pinno;
  getThis->actStarttime = tankStarttime;
  getThis->actEndtime = tankEndtime;
  getThis->samprate = (double) 0.0; /* server doesn't send this */
  getThis->actLen = bin_len;
  if ( strlen( flag ) >= 2 ) {
    getThis->retFlag = flag[1];
    return WS_WRN_FLAGGED;
  } else {
    getThis->retFlag = '\0';
    return WS_ERR_NONE;
  }
}  /* end wsParseBinHeaderReply() */



/* untouched from ws_clientII.c */
/***********************************************************************
* wsParseAsciiHeaderReply: parse the reply we got from the waveserver *
* into a TRACE_REQ structure. Handles the header for replies reply to *
* GETSCN and GETPIN requests.                                         *
***********************************************************************/
static int wsParseAsciiHeaderReply( TRACE_REQ* getThis, char* reply )
{
/* Arguments:
*    getThis: pointer to TRACE_REQ structure to be filled with reply info
*      reply: pointer to reply to be parsed.
*   Returns: WS_ERR_NONE:  if all went well
*            WS_ERR_INPUT: if bad input parameters
*            WS_ERR_PARSE: if we couldn't parse part of the reply
*            WS_WRN_FLAGGED: server sent us a no-data flag
  */
  int reqid = 0;
  int pinno = 0;
  char sta[7];
  char chan[9];
  char net[9];
  char flag[9];
  char datatype[3];
  double tankStarttime = 0.0, samprate = 0.0;
  double tankEndtime =0.0;
  int scn_pos = 0;
  
  if ( !reply )
  {
    logit( "", "wsParseAsciiHeaderReply(): bad input parameters\n");
    return WS_ERR_INPUT;
  }
  
  if ( sscanf( &reply[scn_pos], "%d %d", &reqid, &pinno ) < 2 )
  {
    if(WS_CL_DEBUG & WS_DEBUG_SERVER_ERRORS) 
      logit( "e","wsParseAsciiHeaderReply(): error parsing reqid/pinno\n" );
    return WS_ERR_PARSE;
  }
  wsSkipN( reply, 2, &scn_pos );
  
  if ( sscanf( &reply[scn_pos], "%s %s %s", sta, chan, net ) < 3 )
  {
    if(WS_CL_DEBUG & WS_DEBUG_SERVER_ERRORS) 
      logit( "e","wsParseAsciiHeaderReply(): error parsing SCN\n" );
    return WS_ERR_PARSE;
  }
  wsSkipN( reply, 3, &scn_pos );
  
  if ( sscanf( &reply[scn_pos], "%s %s", flag, datatype ) < 2 )
  {
    if(WS_CL_DEBUG & WS_DEBUG_SERVER_ERRORS) 
      logit( "e","wsParseAsciiHeaderReply(): error parsing flag/datatype\n" );
    return WS_ERR_PARSE;
  }
  wsSkipN( reply, 2, &scn_pos );
  
  if ( strlen(flag) == 1 || strcmp(flag,"FG") == 0 )
  {
    if ( sscanf( &reply[scn_pos], "%lf %lf", &tankStarttime, &samprate ) < 2 )
    {
      if(WS_CL_DEBUG & WS_DEBUG_SERVER_ERRORS) 
        logit( "e","wsParseAsciiHeaderReply(): error parsing startT/samprate\n" );
      return WS_ERR_PARSE;
    }
    wsSkipN( reply, 2, &scn_pos );
  }
  else if ( strlen(flag) == 2 )
  {
    if ( strcmp(flag,"FL") == 0 )
    {
      if ( sscanf( &reply[scn_pos], "%lf", &tankStarttime) < 1 )
      {
        if(WS_CL_DEBUG & WS_DEBUG_SERVER_ERRORS) 
          logit( "e","wsParseAsciiHeaderReply(): error parsing startTime\n" );
        return WS_ERR_PARSE;
      }
      wsSkipN( reply, 1, &scn_pos );
    }
    else if ( strcmp(flag,"FR") == 0 )
    {
      /* DK 2001/10/01  if wave_serverV sends an "FR" flag, then it
         sends the tankEndtime as the only double after the flags,
         not the sample rate.
      if ( sscanf( &reply[scn_pos], "%lf", &samprate) < 1 )
      **************************************************************/
      if ( sscanf( &reply[scn_pos], "%lf", &tankEndtime) < 1 )
      {
        if(WS_CL_DEBUG & WS_DEBUG_SERVER_ERRORS) 
          logit( "e","wsParseAsciiHeaderReply(): error parsing samprate\n" );
        return WS_ERR_PARSE;
      }
      wsSkipN( reply, 1, &scn_pos );
    }
  }
  
  getThis->pinno = pinno;
  getThis->actStarttime = tankStarttime;
  getThis->actEndtime = tankEndtime;
  getThis->samprate = samprate;
  getThis->actLen = strlen( reply ) - scn_pos;
  memmove(reply, &reply[scn_pos], getThis->actLen);
  reply[getThis->actLen] = 0;
  
  if ( strlen( flag ) >= 2 ) {
    getThis->retFlag = flag[1];
    return WS_WRN_FLAGGED;
  } else {
    getThis->retFlag = '\0';
    return WS_ERR_NONE;
  }
}  /* end wsParseAsciiHeaderReply() */


/*******************************************
*  wsKillPSCN: Deallocates the PSCN list  *
*******************************************/
void wsKillPSCN( WS_PSCN pscn )
/*
Arguments:
pscn: pointer to a list of scn structures
*/
{
  WS_PSCN next;

  while ( pscn )
  {
    next = pscn->next;
    
    free( pscn );
    pscn = next;
  }
  return;
}




/* finds a PSCN.  returns the menu that contained the PSCN.
   if *menup is NULL then it starts at the beginning of the
   list of menus.  If *menup is NOT NULL, then it starts searching
   at the menu immediately after *menup in the list.
*******************************************************************/
int wsSearchSCN( TRACE_REQ* getThis, WS_MENU* menup, wsHANDLE pEnv)
{
  int i;
  WS_PSCN_REC PSCN, *pPSCN;
  WS_MENU menu_start;

  if(!(getThis && pEnv))
  {
    logit("t","wsSearchSCN()  Error NULL pointer input params!\n");
    return(WS_ERR_INPUT);
  }
          
  menu_start = *menup;

  
  memset(&PSCN, 0, sizeof(WS_PSCN_REC));
  strncpy(PSCN.sta,  getThis->sta,  sizeof(PSCN.sta - 1));
  strncpy(PSCN.chan, getThis->chan, sizeof(PSCN.chan - 1));
  strncpy(PSCN.net,  getThis->net,  sizeof(PSCN.net - 1));


  /* find the PSCN */
  pPSCN = bsearch(&PSCN, pEnv->pPSCNBuffer, pEnv->iNumPSCNs, 
                  sizeof(WS_PSCN_REC), Compare_WS_PSCN_RECs);

  if(!pPSCN)
  {
    if(WS_CL_DEBUG & WS_DEBUG_OVERKILL) 
      logit("t","wsSearchSCN():  Error [%s,%s,%s] not found.\n",
                          PSCN.sta,PSCN.chan,PSCN.net);
    return(WS_ERR_SCN_NOT_IN_MENU);
  }
  else
  {
    if(WS_CL_DEBUG & WS_DEBUG_DATA_INFO) 
      logit("","SCN (%s,%s,%s) found in menulist!\n", 
            PSCN.sta, PSCN.chan, PSCN.net);
  }

  if(menu_start)
  {
    /* find the NEXT available menu, not the current one!!!! DK 2003/01/27 */
    for(i=0;i < pPSCN->iNumMenus && pPSCN->MenuList[i]->menunum <= menu_start->menunum; i++);

    if(i == pPSCN->iNumMenus)
    {
      if(WS_CL_DEBUG & WS_DEBUG_DATA_WARNINGS) 
        logit("t","wsSearchSCN(): WARNING additional menu for (%s,%s,%s) not found.\n", 
            pPSCN->sta, pPSCN->chan, pPSCN->net);
      return(WS_ERR_MENU_NOT_FOUND);
    }
    /* move to the next menu */
    i++;
  }
  else
  {
    i=0;  /* start at beginning of list */
  }

  if(i >= pPSCN->iNumMenus)
  {
    *menup = NULL;
    return(WS_WRN_FLAGGED);
  }
  else
  {
    *menup = pPSCN->MenuList[i];
    return(WS_ERR_NONE);
  }
}  /* end wsSearchSCN() */

      
WS_MENU wsSearchMenu(wsHANDLE pEnv, char * addr, char * port)
{
  int i;

  for(i=0; i < pEnv->iNumMenusInList; i++)
  {
    if(strcmp(pEnv->MenuList[i]->addr,addr))
      if(strcmp(pEnv->MenuList[i]->port,port))
        return(pEnv->MenuList[i]);

  }
  return(NULL);
}  /* end wsSearchMenu() */






/* Return the pscn list for this server *
 *****************************************************************************/
int wsGetServerPSCN( char* addr, char* port, WS_PSCN* pscnp, wsHANDLE pEnv )
{
  int iNumPSCNs = 0;
  int i, j;
  WS_MENU menu;
  WS_PSCN pFirstPSCN, pLastPSCN, pNewPSCN;

  if(!(pFirstPSCN = malloc(sizeof(WS_PSCN_REC))))
  {
    logit("t","wsGetServerPSCN(): Error! Failed to allocate %d bytes.\n",
      sizeof(WS_PSCN_REC));
    return(WS_ERR_MEMORY);
  }

  memset(pFirstPSCN, 0, sizeof(WS_PSCN_REC));
  pLastPSCN = pFirstPSCN;

  menu = wsSearchMenu(pEnv, addr, port);

  if(!menu)
  {
    if(WS_CL_DEBUG & WS_DEBUG_DATA_ERRORS) 
      logit("t","wsGetServerPSCN(): error [%s:%s] not in menulist.\n", addr,port);
    return(WS_ERR_MENU_NOT_FOUND);
  }
  
  for(i=0; i < pEnv->iNumPSCNs; i++)
  {
    for(j=0; j < pEnv->pPSCNBuffer[i].iNumMenus; j++)
    {
      if(pEnv->pPSCNBuffer[i].MenuList[j] == menu)
      {
        if(!(pNewPSCN = malloc(sizeof(WS_PSCN_REC))))
        {
          logit("t","wsGetServerPSCN(): Error! Failed to allocate %d bytes.\n",
                sizeof(WS_PSCN_REC));
          return(WS_ERR_MEMORY);
        }
        memcpy(pNewPSCN, &(pEnv->pPSCNBuffer[i]), sizeof(WS_PSCN_REC));
        pLastPSCN->next = pNewPSCN;
        pLastPSCN = pNewPSCN;
        break;
      }
    }
  }

  /* set the caller's pointer, remember the first PSCN is blank
     so we want to bypass it. */
  *pscnp = pFirstPSCN->next;
  free(pFirstPSCN);

  if(*pscnp == NULL)
  {
    if(WS_CL_DEBUG & WS_DEBUG_SERVER_ERRORS) 
      logit("t","wsGetServerPSCN(): [%s:%s]  Empty Menu!\n", addr,port);
    return(WS_ERR_EMPTY_MENU);
  }
  else
  {
    return(WS_ERR_NONE);
  }
}  /* end wsGetServerPSCN() */


static int wsCompareMenuNumbersForTraceReqs(const void * pElem1, const void * pElem2)
{

  WS_MENU menu1, menu2;

  menu1 = ((TRACE_REQ *)pElem1)->menu;
  menu2 = ((TRACE_REQ *)pElem2)->menu;

  /* put the NULLs at the front of the list.  less extra baggage
     to carry around.  */
  if(!menu1)
  {
    if(!menu2)
      return(0);
    else
      return(-1);
  }
  if(!menu2)
    return(1);
  else
    return(wsCompareMenuNumbers(menu1,menu2));
}  /* end wsCompareMenuNumbersforTraceReqs() */


static int wsCompareMenuNumbers(const void * pElem1, const void * pElem2)
{

  if(((WS_MENU) pElem1)->menunum == ((WS_MENU) pElem2)->menunum)
    return(0);
  else if(((WS_MENU) pElem1)->menunum < ((WS_MENU) pElem2)->menunum) 
    return(-1);
  else
    return(1);
}  /* end wsCompareMenuNumbers()*/

/***************************************/
int wsPrepRequestList(TRACE_REQ* RequestList, int iNumReqsInList, 
                      int iTraceType, int timeout_sec, wsHANDLE pEnv)
{
  int i;
  int rc;


  /* check input params */
  if(!(RequestList && iNumReqsInList && pEnv))
  {
    logit("t", "wsPrepRequestList: NULL input params!\n");
    return WS_ERR_INPUT;
  }
  /* check iTraceType input param */
  if(!(iTraceType == WS_TRACE_BINARY || iTraceType == WS_TRACE_ASCII))
  {
    logit("t", "wsPrepRequestList: iTraceType is not valid!  "
          "Must be either WS_TRACE_BINARY or WS_TRACE_ASCII\n");
    return WS_ERR_INPUT;
  }

  /* set the iCurrentTraceReq to the beginning of the list */
  pEnv->iCurrentTraceReq = 0;
  /* set iNumTraceReqsInList to the number of reqs in the new list. */
  pEnv->iNumTraceReqsInList = iNumReqsInList;

  /* set the iTraceType */
  pEnv->iTraceTypeForList = iTraceType;

  /* set the CurrentMenu */
  pEnv->CurrentMenu = NULL;

  /* set the timeout period for each call */
  if(timeout_sec <= 0)
    pEnv->iTimeoutMsec = -1;
  else
    pEnv->iTimeoutMsec = 1000 * timeout_sec;

  /* for each server in the menu:
     make sure status is initialized to healthy,
     make sure socket is closed, so we can operate
     with one connection at a time.  */

  for(i=0; i < pEnv->iNumMenusInList; i++)
  {
    /* initialize server status */
    pEnv->MenuList[i]->serverstatus = WS_SERVER_STATUS_HEALTHY;

    /* close the socket if open */
    wsDetachServer(pEnv->MenuList[i],0);
  }

  /* run wsSearchSCN for each trace req */
  for(i=0; i < iNumReqsInList; i++)
  {
    rc = wsSearchSCN(&(RequestList[i]), &(RequestList[i].menu), pEnv);
    if(rc != WS_ERR_NONE)
    {
      RequestList[i].menu = NULL;
    }
    RequestList[i].wsError = WS_ERR_NONE;
  }

    /* sort the RequestList by menu */
  qsort(RequestList,iNumReqsInList,sizeof(TRACE_REQ),
        wsCompareMenuNumbersForTraceReqs);

  /* return success */
  return(WS_ERR_NONE);

}  /* end wsPrepRequestList() */


/***************************************/
/***********************************************************
   WARNING WARNING WARNING WARNING WARNING WARNING WARNING
   The wsClientIII routines use a single snippet buffer that
   is malloced durin wsInitialize().  The caller should
   NEVER NEVER NEVER Free ptrCurrent->pBuf, and should copy
   the contents of ptrCurrent->pBuf before the next call
   to wsGetNextTraceFromRequestList(), because the old data
   will be overwritten!!!!!!
   DK 0130 2003
   WARNING WARNING WARNING WARNING WARNING WARNING WARNING
***********************************************************/
int wsGetNextTraceFromRequestList(TRACE_REQ* RequestList, wsHANDLE pEnv, 
                           TRACE_REQ** ppResult)
{

  TRACE_REQ * pReq;
  int ctr;
  int rc, ret;
  WS_MENU error_menu;
  int server_error;
  
  double dnow, dTimeoutStart, dTimeoutSeconds, dTemp;
  int  bTimeout;

  /* DK Change 02/06/02 */
  *ppResult = NULL;
  /* End DK Change 02/06/02 */

  if(pEnv->iTimeoutMsec > 0)
  {
    bTimeout = TRUE;
    dTimeoutStart = hrtime_ew(&dnow);
    dTimeoutSeconds = (double)(pEnv->iTimeoutMsec) / 1000;
  }
  else
  {
    bTimeout = FALSE;
  }

  if(pEnv->iNumTraceReqsInList <= pEnv->iCurrentTraceReq)
  {
    /* all items in list have already been processed */
    pEnv->iNumTraceReqsInList = pEnv->iCurrentTraceReq = 0;
    wsCloseAllSockets(pEnv);
    return(WS_ERR_LIST_FINISHED);
  }

  pReq = &(RequestList[pEnv->iCurrentTraceReq]);

  /* first check for NULL menu */
  if(!pReq->menu)
  {
    /* we may have already had a failed request
       for a wave_server that had this channel in
       the menu 
    **********************************************/
    if(pReq->wsError != WS_ERR_NONE)
      ret = pReq->wsError;
    else
      ret = WS_ERR_SCN_NOT_IN_MENU;

    goto Finished;
  }

  /* copy the snippet buffer to the current req */
  pReq->pBuf = pEnv->bSnippetBuffer;
  pReq->bufLen = pEnv->iSnippetBufferSize;

  /* set the CurrentMenu */
  if(pEnv->CurrentMenu)
  {
    if(pEnv->CurrentMenu != pReq->menu)
    {
      if(WS_CL_DEBUG & WS_DEBUG_SERVER_INFO) 
        logit("","Current menu differs from new menu, detaching server.\n"
              "\t\t\tCurrent Menu (%s:%s), New Menu(%s:%s)\n",
              pEnv->CurrentMenu->addr, pEnv->CurrentMenu->port,
              pReq->menu->addr, pReq->menu->port);

      wsDetachServer(pEnv->CurrentMenu,0);
      pEnv->CurrentMenu = pReq->menu;
    }
  }
  else
  {
    pEnv->CurrentMenu = pReq->menu;
  }

  /* try to process the current TRACE_REQ */

  if(WS_CL_DEBUG & WS_DEBUG_DATA_INFO) 
    logit("","Data reqd for (%s,%s,%s): %.0f+%.0f from(%s:%s) - ",
          pReq->sta, pReq->chan, pReq->net,
          pReq->reqStarttime, (pReq->reqEndtime - pReq->reqStarttime),          
          pReq->menu->addr, pReq->menu->port
         );
  
  
  pReq->menu->stats.iNumSnippetsAttempted++;
  hrtime_ew(&dnow);
  
  if(bTimeout)
  {
    rc = wsGetTraceFromServer(pReq, pReq->menu, pEnv->iTraceTypeForList, 
                              (int)((dTimeoutStart + dTimeoutSeconds - dnow) * 1000));
  }
  else
    rc = wsGetTraceFromServer(pReq, pReq->menu, pEnv->iTraceTypeForList, -1/*timeout*/);
  
  /* check for non server-wide errors */
  if(rc == WS_ERR_NONE || rc == WS_ERR_BUFFER_OVERFLOW)
  {
    pReq->menu->stats.iNumSnippetsRetrieved++;

    if(WS_CL_DEBUG & WS_DEBUG_DATA_INFO) 
      logit("","SUCCESS in %6.2f seconds\n",hrtime_ew(&dTemp) - dnow);

    ret = rc;
    goto Finished;
  }
  else 
  {
    if(rc == WS_WRN_FLAGGED  || rc == WS_ERR_TIMEOUT)
    {
      if(rc == WS_WRN_FLAGGED)
        pReq->menu->stats.iNumSnippetsFlagged++;
      else
        pReq->menu->stats.iNumSnippetsTimedOut++;

      /* don't log FLAG warnings, they are logged in wsGetTraceFromServer(). */
      if((!WS_WRN_FLAGGED) & WS_CL_DEBUG & WS_DEBUG_DATA_WARNINGS) 
        logit("","ERROR(%d) for (%s,%s,%s): %.0f+%.0f from(%s:%s) in %6.2f seconds\n",
              rc, pReq->sta, pReq->chan, pReq->net,
              pReq->reqStarttime, (pReq->reqEndtime - pReq->reqStarttime),
              pReq->menu->addr, pReq->menu->port,
              hrtime_ew(&dTemp) - dnow);

      /* set the error code */
      if(pReq->wsError == WS_ERR_NONE)
        pReq->wsError = rc;

      /* wsSearchSCN using current menu */
      rc = wsSearchSCN(pReq, &(pReq->menu), pEnv);
      if(rc != WS_ERR_NONE)
      {
        pReq->menu = NULL;
        ret = pReq->wsError;
        goto Finished;
      }
    }
    else
    {
      pReq->menu->stats.iNumErrors++;


      if(WS_CL_DEBUG & WS_DEBUG_SERVER_ERRORS) 
        logit("","SERVER ERROR(%d) from(%s:%s) for (%s,%s,%s): %.0f+%.0f "
                 "in %6.2f seconds\n",
              rc, pReq->menu->addr, pReq->menu->port,
              pReq->sta, pReq->chan, pReq->net,
              pReq->reqStarttime, (pReq->reqEndtime - pReq->reqStarttime),
              hrtime_ew(&dTemp) - dnow);

      /* there is a server problem! */
      pReq->menu->serverstatus = WS_SERVER_STATUS_ERROR;
      error_menu = pReq->menu;
      server_error = rc;  /* from wsGetTraceFromServer() */

      for(ctr=pEnv->iCurrentTraceReq; ctr < pEnv->iNumTraceReqsInList; ctr++)
      {
        pReq = &(RequestList[ctr]);
        if(pReq->menu == error_menu)
        {
          if(pReq->wsError == WS_ERR_NONE)
            pReq->wsError = server_error;

          rc = wsSearchSCN(pReq, &(pReq->menu), pEnv);

          if(rc != WS_ERR_NONE && rc != WS_WRN_FLAGGED)
          {
            logit("","wsSearchSCN(): error while searching for SCN (%s,%s,%s)\n",
                   pReq->sta, pReq->chan, pReq->net);
            pReq->menu = NULL;
            /* NO NO NO    goto Finished; */
          }
        }
        else
        {
          /* we only want to update the requests from a matching server ,
             and they are ordered by server */
          break;
        }
      }

    }  /* end else  (not successfullish, and not a flagged warning */

    /* re-sort the remaining RequestList by menu */
    qsort(&(RequestList[pEnv->iCurrentTraceReq]),
          pEnv->iNumTraceReqsInList - pEnv->iCurrentTraceReq,
          sizeof(TRACE_REQ),
          wsCompareMenuNumbersForTraceReqs);

  }    /* end else from if wsGetTraceFromServer() successfullish*/

  /* if we got here, we ran into some sort of processing error. */
  /* to be fair to the next trace received (by giving it a full
     timeout time) we should return to the caller, and tell it
     that we had some temporary error.  It shouldn't expect that
     we lost any data, just that we ran into some error, and we
     should be called again.
  ***************************************************************/
  ret = WS_ERR_TEMPORARY;
  *ppResult = NULL;
  return(ret);

Finished:
  pEnv->iCurrentTraceReq++;

  *ppResult = pReq;

  return(ret);

}  /* end wsGetNextTraceFromRequestList() */

    
/***************************************/
int wsEndRequestList(TRACE_REQ* RequestList, wsHANDLE pEnv)
{
    /* shutdown list params */
    pEnv->iNumTraceReqsInList = pEnv->iCurrentTraceReq = 0;
    wsCloseAllSockets(pEnv);
    return(WS_ERR_NONE);
}  /* end wsEndRequestList() */

/*******************************************************
* Other                                                *
*******************************************************/
void wsCloseAllSockets(wsHANDLE pEnv)
{
  int i;

  /*logit("et","wsCloseAllSockets(): Calling wsDetachServer for each server in menulist. \n");*/
  for(i=0; i< pEnv->iNumMenusInList; i++)
  {
    wsDetachServer(pEnv->MenuList[i],0);
  }

}  /* end wsCloseAllSockets() */


int wsRefreshMenu(WS_MENU menu)
{
  /* DK CLEANUP
   int wsRemoveServer(wsHANDLE pEnv, char * szIPAddr, char * szPort); */

  logit("et","ERROR! wsRefreshMenu() called, but not implemented! Continuing!\n");
  return(WS_ERR_NONE);
}

int wsRefreshAllMenus(wsHANDLE pEnv)
{
  /* for each server, remove from list */
  /* for each server, add to list */
  logit("et","ERROR! wsRefreshAllMenus() called, but not implemented! Continuing!\n");
  return(WS_ERR_NONE);
}

int wsPrintServerStats(wsHANDLE pEnv)
{
  int i;

  logit("e","\n************************************\n");
  logit("e","************************************\n");
  logit("et","Server Statistics\n");
  logit("e","************************************\n");
  logit("e","    Server               Connected    Overhead    Connxs/    Errors  Connected    Snippets Rtrvd/ \n"
           "                           (sec)        (sec)    Attempts            Time (%%)     Flgd/Tmout/Atmptd\n");
  for(i=0; i < pEnv->iNumMenusInList; i++)
  {
    logit("e","(%s:%s)%s\t"  /* server:port <fill>*/
             "  %8.2f "     /* conected time */
             "    %7.2f "     /* overhead time */
             "  %3d/%3d    "    /* connections/attempted */
             "  %3d    "       /* errors */
             " %5.2f%% "  /* connection time % */
             "  %6d/%d/%d/%d\n",
          pEnv->MenuList[i]->addr, pEnv->MenuList[i]->port, 
          (strlen(pEnv->MenuList[i]->addr)<14)?"  ":"",
          pEnv->MenuList[i]->stats.dTotalTimeConnected,
          pEnv->MenuList[i]->stats.dTotalTimeOverhead,
          pEnv->MenuList[i]->stats.iNumTotalConnections,
          pEnv->MenuList[i]->stats.iNumTotalConnectionsAttempted,
          pEnv->MenuList[i]->stats.iNumErrors,
          pEnv->MenuList[i]->stats.dTotalTimeConnected/
           (time(NULL) - pEnv->MenuList[i]->stats.tServerAdded)*100,
          pEnv->MenuList[i]->stats.iNumSnippetsRetrieved,
          pEnv->MenuList[i]->stats.iNumSnippetsFlagged,
          pEnv->MenuList[i]->stats.iNumSnippetsTimedOut,
          pEnv->MenuList[i]->stats.iNumSnippetsAttempted
          );

  }
  logit("e","************************************\n\n");

  return(WS_ERR_NONE);
}


