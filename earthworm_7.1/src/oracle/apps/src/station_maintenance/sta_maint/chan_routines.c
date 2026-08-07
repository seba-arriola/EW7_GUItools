
#include <ewdb_ora_api.h>
#include "sta_maint.h"






int AddChan(EWDB_ChannelStruct * pChan);
int EndChan(EWDB_ChannelStruct * pChan);
int DeleteChan(EWDB_ChannelStruct * pChan);
int AssocChan(EWDB_ChannelStruct * pChan);
int DeleteChanT(EWDB_ChannelStruct * pChan);
int EndChanT(EWDB_ChannelStruct * pChan);


static char szBuffer[256];

int AddChan(EWDB_ChannelStruct * pChan)
{
  int rc;
  rc=ewdb_api_CreateChannel(&pChan->Comp.idChan, "");
  if(rc == EWDB_RETURN_SUCCESS)
    sprintf(szBuffer,"# CHAN ADD IDCHAN = %d\n", pChan->Comp.idChan);
  else
    sprintf(szBuffer,"# CHAN ADD FAILED return codes (%d,%d)\n", rc, pChan->Comp.idChan);

  WriteToOutputFile(szBuffer);

  return(rc);
}


int EndChan(EWDB_ChannelStruct * pChan)
{
  int rc;
  int i;

  EWDB_ChannelStruct csCriteria, csCriteriaMax;
  EWDB_ChannelStruct pChanTBuffer[256];
  int iBufferLen = 256;
  int iNumStationsFound, iNumStationsRetrieved;
  int iCriteria = 0;

  /*
  Before we end the comp, we have to do the following:
  1)Search for chant's for the involved time range >tOff,
     and truncate/delete each of them.

  */

  memset(&csCriteria,0, sizeof(EWDB_ChannelStruct));
  memset(&csCriteriaMax,0, sizeof(EWDB_ChannelStruct));
  memset(pChanTBuffer,0, sizeof(pChanTBuffer));
  iCriteria = EWDB_CRITERIA_USE_TIME | EWDB_CRITERIA_USE_IDCHAN;
  csCriteria.tOn = pChan->tOff;
  csCriteria.tOff = 9999999999;
  csCriteria.Comp.idChan = pChan->Comp.idChan;

  rc=ewdb_api_GetSelectedChannels(pChanTBuffer, iBufferLen, 
                                  &csCriteria, &csCriteriaMax, iCriteria, 
                                  &iNumStationsFound, &iNumStationsRetrieved);


  if(rc != EWDB_RETURN_SUCCESS)
  {

    sprintf(szBuffer,"# CHAN END FAILED (ewdb_api_GetSelectedChannels)return "
                     "codes (%d,%d,%d)\n", rc,
            iNumStationsFound, iNumStationsRetrieved);
    goto WriteToFile;
  }
  for(i=0; i < iNumStationsRetrieved; i++)
  {
    if(pChanTBuffer[i].tOn < pChan->tOff)
    {
       pChanTBuffer[i].tOn = pChan->tOff;
       rc = EndChanT(&pChanTBuffer[i]);
       if(rc!= EWDB_RETURN_SUCCESS)
       {
         sprintf(szBuffer,"# CHAN END FAILED(EndChanT()) return codes (%d,%d)\n", 
                 rc, pChan->idChanT);
         WriteToOutputFile(szBuffer);
       }
       else
       {
         pChan->idChanT = pChanTBuffer[i].idChanT;
       }
    }
    else
    {
      rc = DeleteChanT(&pChanTBuffer[i]);
      if(rc!= EWDB_RETURN_SUCCESS)
      {
        sprintf(szBuffer,"# CHAN END FAILED(DeleteChanT()) return codes (%d,%d)\n", 
                rc, pChan->idChanT);
        WriteToOutputFile(szBuffer);
      }
    }
  }
  if(rc == EWDB_RETURN_SUCCESS)
    sprintf(szBuffer,"# CHAN END IDCHAN = %d\n", pChan->Comp.idChan);
  else
    sprintf(szBuffer,"# CHAN END FAILED return codes (%d,%d)\n", rc, pChan->Comp.idChan);

WriteToFile:
  WriteToOutputFile(szBuffer);
  return(rc);
}

int DeleteChan(EWDB_ChannelStruct * pChan)
{
  WriteToOutputFile("Chan(Delete) command is not supported.\n");
  return(-1);
}

int ewdb_internal_SetChanParams(EWDB_ChannelStruct * pChan);

int AssocChan(EWDB_ChannelStruct * pChan)
{

  /* associate a channel with a component for a given time
     interval
   **********************************************************/
  EWDB_ChannelStruct pBuffer[10];
  int rc;
  int iCriteria;
  int iNumItemsFound, iNumItemsRetrieved;
  double tOffLast;
  int i;


  iCriteria = EWDB_CRITERIA_USE_TIME | EWDB_CRITERIA_USE_SCNL;

  rc=ewdb_api_GetCompTInfo(pBuffer, sizeof(pBuffer) / sizeof(EWDB_ChannelStruct),
                           pChan, pChan, iCriteria,
                           &iNumItemsFound, &iNumItemsRetrieved);

  if(rc == EWDB_RETURN_FAILURE)
  {
    sprintf(szBuffer,"# CHAN ASSOC FAILED in ewdb_api_GetCompTInfo().  "
      "return codes (%d,%d,%d)\n", 
      rc, iNumItemsFound, iNumItemsRetrieved);
    
    WriteToOutputFile(szBuffer);
    return(rc);
  }
  else if(rc == EWDB_RETURN_FAILURE)
  {
    sprintf(szBuffer,"# CHAN ASSOC FAILED in ewdb_api_GetCompTInfo().  "
            "return codes (%d,%d,%d).  Too many compt records over given "
            "timerange (%.0f - %.0f)\n", 
            rc, iNumItemsFound, iNumItemsRetrieved, pChan->tOn, pChan->tOff);
    WriteToOutputFile(szBuffer);
    return(rc);
  }

  tOffLast = pChan->tOn;
  for(i=0; i < iNumItemsRetrieved; i++)
  {
    if(pBuffer[i].tOn < tOffLast)
    {
      if(i==0) /* first record */
      {
        pBuffer[i].tOn = pChan->tOn;
      }
      else
      {
      sprintf(szBuffer,"# CHAN ASSOC FAILED while processing compt records.\n"
        "#    records (%d, %.0f-%.0f) (%d, %.0f-%.0f) overlap!\n",
        pBuffer[i-1].idCompT, pBuffer[i-1].tOn, pBuffer[i-1].tOff,
        pBuffer[i].idCompT, pBuffer[i].tOn, pBuffer[i].tOff);
      WriteToOutputFile(szBuffer);
      return(-1);
      }
    }
    if(pBuffer[i].tOff > pChan->tOff)
      pBuffer[i].tOff = pChan->tOff;

    pBuffer[i].Comp.idChan = pChan->Comp.idChan;

    rc=ewdb_internal_SetChanParams(&pBuffer[i]);
    if(rc == EWDB_RETURN_SUCCESS)
    {
      sprintf(szBuffer,"# CHAN ASSOC IDCHAN=%d IDCHANT=%d, IDCOMPT=%d (%.0f = %.0f)\n", 
              pBuffer[i].Comp.idChan, pBuffer[i].idChanT, pBuffer[i].idCompT, 
              pBuffer[i].tOn, pBuffer[i].tOff);
      WriteToOutputFile(szBuffer);
      pChan->idChanT = pBuffer[i].idChanT;
      if(pChan->pChanProps->dGain ||
           pChan->pChanProps->dSampRate  ||
           pChan->pChanProps->tfsFunc.idCookedTF 
        ) 
      {
        /* we got some piece of channel/response info.  Create a ChanCTF record */
        rc=ewdb_api_SetTransformFuncForChanT(pBuffer[i].idChanT, pChan->pChanProps->dGain,
                                             pChan->pChanProps->tfsFunc.idCookedTF,
                                             pChan->pChanProps->dSampRate);
        if(rc == EWDB_RETURN_SUCCESS)
        {
          sprintf(szBuffer,"# CHAN ASSOC CHANCTF SUCCESSFUL\n");
          WriteToOutputFile(szBuffer);
        }
        else
        {
          sprintf(szBuffer,"# CHAN ASSOC CHANCTF FAILED ewdb_api_SetTransformFuncForChanT() "
                  "return codes (%d)\n", 
                  rc);
        }
      }  /* end if ChanCTF params */
    }  /* end if SetChanParams() successful */
    else
    {
      sprintf(szBuffer,"# CHAN ASSOC FAILED ewdb_internal_SetChanParams() "
                       "return codes (%d,%d)\n", 
              rc, pBuffer[i].idChanT);
      WriteToOutputFile(szBuffer);
    }
    
    tOffLast = pBuffer[i].tOff;
  }
  return(rc);
}





/* CHAN COMMANDS
CHAN    |ADD  |   1000008645 ISCO CH GHZ  999999999 1100000000 "COMMENT"
CHAN    |END  |   1000002450                 1099999999
CHAN    |DEL  |   1000002450
CHAN    |ASSOC|   1000008645 ISCO CH GHZ  999999999 1100000000
************************************************************************/
int HandleChanCommand(char * szCmd, char * szLineBuffer, FILE * fOut)
{
  
  EWDB_ChannelStruct chan;
  char szBuffer[256];
  char * szTemp;
  char szComment[256];
  int rc = 0;

  memset(&chan, 0 , sizeof(EWDB_ChannelStruct));
  if(!(chan.pChanProps = GlobalChan.pChanProps))
  {
    logit("e","Error in HandleChanCommand.  GlobalChan.pChanProps is NULL.\n");
    WriteToOutputFile("# Error in HandleChanCommand.  GlobalChan.pChanProps is NULL.\n");
    return(-1);
  }
  memset(szBuffer, 0, sizeof(szTemp));
  
 /* attempt to parse the id, sta, net, comp, loc */
  /* idChan */
  strncpy_ew(szBuffer, &szLineBuffer[STA_FILE_OFFSET_ID], STA_FILE_LEN_ID + 1);
  chan.Comp.idChan = atoi(szBuffer);

  /* check for prev */
  szTemp = strtok(szBuffer," \t\n");
  if(!szTemp)
    chan.Comp.idChan = 0;
  else
  {
    if(!strcmp(szTemp,SPECIAL_USE_PREVIOUS))
    {
      chan.Comp.idChan = SPECIAL_IDPREVIOUS;
    }
    else
    {
      chan.Comp.idChan = atoi(szTemp);
    }
  }


  /* sta */
  strncpy_ew(szBuffer,&szLineBuffer[STA_FILE_OFFSET_STA], STA_FILE_LEN_STA + 1);
  szTemp = strtok(szBuffer," ");
  if(szTemp)
    strncpy_ew(chan.Comp.Sta, szTemp, sizeof(chan.Comp.Sta));
  
  /* net */
  szBuffer[3] = 0x00;  /* null terminate the string */
  strncpy_ew(szBuffer,&szLineBuffer[STA_FILE_OFFSET_NET], STA_FILE_LEN_NET + 1);
  szTemp = strtok(szBuffer," ");
  if(szTemp)
    strncpy_ew(chan.Comp.Net, szTemp, sizeof(chan.Comp.Net));
  
  /* comp */
  strncpy_ew(szBuffer,&szLineBuffer[STA_FILE_OFFSET_CMP], STA_FILE_LEN_CMP + 1);
  szBuffer[5] = 0x00;  /* null terminate the string */
  szTemp = strtok(szBuffer," ");
  if(szTemp)
    strncpy_ew(chan.Comp.Comp, szTemp, sizeof(chan.Comp.Comp));
  
  /* loc */
  strncpy_ew(szBuffer,&szLineBuffer[STA_FILE_OFFSET_LOC], STA_FILE_LEN_LOC + 1);
  szTemp = strtok(szBuffer," ");
  if(szTemp  && strcmp(szTemp,SPECIAL_NULL_LOC_STRING))
    strncpy_ew(chan.Comp.Loc, szTemp, sizeof(chan.Comp.Loc));
  
  /* now handle the command */
  if(strcmp(szCmd, "ADD") == 0)
  {

    logit("e","Attempting to add Chan.\n");
    rc = AddChan(&chan);
  }
  else 
  {
    if(chan.Comp.idChan == SPECIAL_IDPREVIOUS)
      chan.Comp.idChan = GlobalChan.Comp.idChan;


  if(strcmp(szCmd, "END") == 0)
  {
    chan.tOff = atof(&szLineBuffer[STA_FILE_OFFSET_VAR]);
    logit("e","Attempting to end Chan(%d) at %.2f.  Assocd w/Chan (%s.%s.%s.%s).\n",
      chan.Comp.idChan, chan.tOff,
      chan.Comp.Sta, chan.Comp.Comp, chan.Comp.Net, chan.Comp.Loc);
    rc = EndChan(&chan);
  }
  else if(strcmp(szCmd, "DEL") == 0)
  {
    logit("e","Attempting to delete Chan(%d) assoc w/Chan(%s.%s.%s.%s).\n",
      chan.Comp.idChan,
      chan.Comp.Sta, chan.Comp.Comp, chan.Comp.Net, chan.Comp.Loc
      );
    rc = DeleteChan(&chan);
  }
  else if(strcmp(szCmd, "ASSOC") == 0)
  {
    /* initialize the  parameters on the line */
    chan.tOff = chan.tOn = 0.0;
    chan.pChanProps->tfsFunc.szCookedTFDesc[0] = 0x00;
    chan.pChanProps->tfsFunc.idCookedTF = 0;
    chan.pChanProps->dGain = 0.0;
    chan.pChanProps->dSampRate = 0.0;
    szComment[0] = 0x00;

    szTemp = strtok(&szLineBuffer[STA_FILE_OFFSET_VAR], " \t");
    if(szTemp)
    {
      chan.tOn = atof(szTemp);
      szTemp = strtok(NULL, " \t");
      if(szTemp)
      {
        chan.tOff = atof(szTemp);
        szTemp = strtok(NULL, " \t");
        if(szTemp)
        {
          chan.pChanProps->tfsFunc.idCookedTF = atoi(szTemp);
          szTemp = strtok(NULL, " \t");
          if(szTemp)
          {
            chan.pChanProps->dSampRate = atof(szTemp);
            szTemp = strtok(NULL, " \t");
            if(szTemp)
            {
              chan.pChanProps->dGain = atof(szTemp);
              szTemp = strtok(NULL, "\"\n");
              if(szTemp)
              {
                strncpy_ew(szComment, szTemp, sizeof(szComment));
                szComment[sizeof(szComment)-1] = 0x00;
              }
            }
          }
        }
      }
    }
    logit("e","Attempting to ASSOC Chan(%d) with ChanT(%s.%s.%s.%s - %d): \n"
      "On-Off(%.0f - %.0f)\n"
      "  Comment:\"%s\"\n", 
      chan.Comp.idChan, chan.Comp.Sta, chan.Comp.Comp, chan.Comp.Net, chan.Comp.Loc, 
      chan.idChanT,
      chan.tOn, chan.tOff, szComment);
    rc = AssocChan(&chan);
  }
  else
  {
    fprintf(fOut,"#ERROR: Unsupported CHAN command (%s).\n", szCmd);
    rc = -1;
  }
  }  /* end else ADD */

  if(rc == EWDB_RETURN_SUCCESS)
  {
    GlobalChan.Comp.idChan = chan.Comp.idChan;
    if(chan.idChanT)
      GlobalChan.idChanT = chan.idChanT;
  }

  return(rc);
}  /* end HandleChanCommand() */

