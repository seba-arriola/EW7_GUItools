
#include <ewdb_ora_api.h>
#include "sta_maint.h"

int AddCompT(EWDB_ChannelStruct * pChan, int bForce);
int EndCompT(EWDB_ChannelStruct * pChan);
int DeleteCompT(EWDB_ChannelStruct * pChan);
int ModCompT(EWDB_ChannelStruct * pChan);
int ModTCompT(EWDB_ChannelStruct * pChan);
int SplitCompT(EWDB_ChannelStruct * pChan);

static  char szBuffer[256];

int AddCompT(EWDB_ChannelStruct * pChan, int bForce)
{
  int rc;
  EWDB_ChannelStruct chan;
  
  if(pChan->Comp.idComp <= 0)
  {
    /* obtain idComp from sta/comp/net/loc */
    rc = ewdb_api_GetidComp(pChan);
    if(rc != EWDB_RETURN_SUCCESS)
    {
       sprintf(szBuffer,"# COMPT ADD FAILED during ewdb_api_GetidComp(). \n"
                        "#   params (%s/%s/%s/%s - %d).  rc = %d.\n", 
                        pChan->Comp.Sta, pChan->Comp.Comp, 
                        pChan->Comp.Net, pChan->Comp.Loc,
                        pChan->Comp.idComp, rc);
       rc = EWDB_RETURN_FAILURE;
       goto WriteOutputFile;
    }

  }

  if(bForce)
  {
    memcpy(&chan,pChan,sizeof(chan));
    rc=ewdb_api_SetCompParams(&chan.Comp, chan.tOn, chan.tOff,
                              &chan.idCompT, "");
    if(rc == EWDB_RETURN_SUCCESS)
      sprintf(szBuffer,"# COMPT ADD(force) IDCOMPT = %d\n", pChan->idCompT);
    else
      sprintf(szBuffer,"# COMPT ADD(force) FAILED ewdb_api_SetCompParams() "
                       "return codes (%d,%d)\n", 
              rc, pChan->idCompT);
    goto WriteOutputFile;
  }
  else
  {
    rc=ewdb_api_CreateCompT(pChan); /* no forcing */
    if(rc == EWDB_RETURN_SUCCESS)
      sprintf(szBuffer,"# COMPT ADD IDCOMPT = %d\n", pChan->idCompT);
    else
    {
      sprintf(szBuffer,"# COMPT ADD(force) FAILED ewdb_api_SetCompParams() "
                       "return codes (%d,%d)\n", 
              rc, pChan->idCompT);
      WriteToOutputFile(szBuffer);
      return(rc);
    }
    if(rc == 1)
    {
      WriteToOutputFile(szBuffer);
      memcpy(&chan,pChan,sizeof(chan));
      
      rc = ewdb_api_GetCompTParams(&chan);
      if(rc == EWDB_RETURN_SUCCESS || EWDB_RETURN_WARNING)
      {
        if(rc == EWDB_RETURN_WARNING)
        {
          sprintf(szBuffer,"Mulitple overlapping CompT records.  Only first one printed.\n");
          WriteToOutputFile(szBuffer);
        }
        sprintf(szBuffer,"Overlapping record:  idCompT(%d) idComp(%d) "
                          "tOn - tOff (%.2f - %.2f) Lat/Lon/Elev(%d/%d/%d)\n",
                chan.idCompT, chan.Comp.idComp, chan.tOn, chan.tOff, 
                chan.Comp.Lat, chan.Comp.Lon, chan.Comp.Elev);
        rc=EWDB_RETURN_FAILURE;
        goto WriteOutputFile;
      }
    }  /* end if overlapping existing CompT records */
  }
  
WriteOutputFile:
  WriteToOutputFile(szBuffer);
  return(rc);
}  /* end AddCompT() */



int EndCompT(EWDB_ChannelStruct * pChan)
{
  int rc;
  int iRetCode;
  EWDB_ChannelStruct chan, chan2;
  EWDB_ChannelStruct pChannelBuffer[20];
  int iNumStationsFound, iNumStationsRetrieved, i;

  chan.idCompT = pChan->idCompT;
  rc = ewdb_api_GetCompTParams(&chan);
  if(rc != EWDB_RETURN_SUCCESS)
  {
    sprintf(szBuffer,"# COMPT END FAILED (ewdb_api_GetCompTParams(%d)) return "
                     "codes (%d,%d)\n", pChan->idCompT, rc, chan.idCompT);
    goto WriteOutputFile;
  }

  memcpy(&chan2, &chan, sizeof(EWDB_ChannelStruct));

  chan.tOff = pChan->tOff;
  rc = ewdb_api_UpdateCompTTime(&iRetCode, &chan);
  if(rc != EWDB_RETURN_SUCCESS)
  {
    sprintf(szBuffer,"# COMPT END FAILED (ewdb_api_UpdateCompTTime(%d)) return "
                     "codes (%d,%d)\n", pChan->idCompT, rc, iRetCode);
    goto WriteOutputFile;
  }

  chan2.tOn = pChan->tOn;
  /* now adjust all of the affected chanT's */
  rc = ewdb_api_GetSelectedChannels(pChannelBuffer, 
                                    sizeof(pChannelBuffer)/sizeof(EWDB_ChannelStruct),
                                    &chan2, &chan2, 
                                    EWDB_CRITERIA_USE_TIME | EWDB_CRITERIA_USE_SCNL, 
                                    &iNumStationsFound, &iNumStationsRetrieved);

  if(rc != EWDB_RETURN_SUCCESS)
  {
    sprintf(szBuffer,"# COMPT END FAILED (ewdb_api_GetSelectedChannels(%d)) return "
                     "codes (%d,%d)\n", pChan->idCompT, rc, iRetCode);
    goto WriteOutputFile;
  }

  for(i=0; i < iNumStationsRetrieved; i++)
  {
    if(pChannelBuffer[i].idCompT != pChan->idCompT)
      continue;
    if(pChannelBuffer[i].tOn < chan2.tOn && pChannelBuffer[i].tOff > chan2.tOn)
    {
      pChannelBuffer[i].tOff = chan2.tOff;
      rc = ewdb_api_UpdateChanTTime(&iRetCode, &chan);
      if(rc != EWDB_RETURN_SUCCESS)
      {
        sprintf(szBuffer,"# COMPT END WARNING: UPDATE CHANT FAILED "
                         "(ewdb_api_UpdateChanTTime(%d-%d)) return "
                          "codes (%d,%d)\n", 
                pChan->idCompT, pChan->idChanT, rc, iRetCode);
        WriteToOutputFile(szBuffer);
      }
    }
    else
    {
      rc = ewdb_api_DeleteChanT(pChannelBuffer[i].idChanT);
      if(rc != EWDB_RETURN_SUCCESS)
      {
        sprintf(szBuffer,"# COMPT END WARNING: DELETE CHANT FAILED "
                         "(ewdb_api_DeleteChanT(%d-%d)) return "
                          "codes (%d)\n", 
                pChan->idCompT, pChan->idChanT, rc);
        WriteToOutputFile(szBuffer);
      }
    }

  }


  sprintf(szBuffer,"# COMPT END idCompT =%d  tOn=%d  tOff=%d.\n", 
          pChan->idCompT, chan.tOn, chan.tOff);

WriteOutputFile:
  WriteToOutputFile(szBuffer);
  return(rc);
}


int DeleteCompT(EWDB_ChannelStruct * pChan)
{
  int rc;
  EWDB_ChannelStruct chan;
  EWDB_ChannelStruct pChannelBuffer[20];
  int iNumStationsFound, iNumStationsRetrieved, i;

  chan.idCompT = pChan->idCompT;
  rc = ewdb_api_GetCompTParams(&chan);
  if(rc != EWDB_RETURN_SUCCESS)
  {
    sprintf(szBuffer,"# COMPT DELETE FAILED (ewdb_api_GetCompTParams(%d)) return "
                     "codes (%d,%d)\n", pChan->idCompT, rc, chan.idCompT);
    goto WriteOutputFile;
  }


  /* now adjust all of the affected chanT's */
  rc = ewdb_api_GetSelectedChannels(pChannelBuffer, 
                                    sizeof(pChannelBuffer)/sizeof(EWDB_ChannelStruct),
                                    &chan, &chan, 
                                    EWDB_CRITERIA_USE_TIME | EWDB_CRITERIA_USE_SCNL, 
                                    &iNumStationsFound, &iNumStationsRetrieved);
  if(rc != EWDB_RETURN_SUCCESS)
  {
    sprintf(szBuffer,"# COMPT END FAILED (ewdb_api_GetSelectedChannels(%d)) return "
                     "codes (%d)\n", pChan->idCompT, rc);
    goto WriteOutputFile;
  }

  for(i=0; i < iNumStationsRetrieved; i++)
  {
    rc = ewdb_api_DeleteChanT(pChannelBuffer[i].idChanT);
    if(rc != EWDB_RETURN_SUCCESS)
    {
      sprintf(szBuffer,"# COMPT END WARNING: DELETE CHANT FAILED "
        "(ewdb_api_DeleteChanT(%d-%d)) return "
        "codes (%d)\n", 
        pChan->idCompT, pChan->idChanT, rc);
      WriteToOutputFile(szBuffer);
    }
  }

  rc = ewdb_api_DeleteCompT(pChan->idCompT);
  if(rc != EWDB_RETURN_SUCCESS)
  {
    sprintf(szBuffer,"# COMPT DELETE FAILED (ewdb_api_DeleteCompT(%d)) return "
                     "codes (%d)\n", pChan->idCompT, rc);
    goto WriteOutputFile;
  }

  sprintf(szBuffer,"# COMPT DELETE idCompT =%d  tOn=%d  tOff=%d.\n", 
          pChan->idCompT, chan.tOn, chan.tOff);


WriteOutputFile:
  WriteToOutputFile(szBuffer);
  return(rc);
}


/* warning this function will update all params for the given idCompT,
   so make sure you fill in all parameters from DB before calling update
   with several new values.
  ************************************************************************/
int ModCompT(EWDB_ChannelStruct * pChan)
{
  int rc;
  EWDBid idCompT = pChan->idCompT;

  rc = ewdb_api_SetComptParams(pChan, "");
  if(rc != EWDB_RETURN_SUCCESS)
  {
    sprintf(szBuffer,"# COMPT MOD FAILED (ewdb_api_SetComptParams(%d)) return "
                     "codes (%d)\n", 
            pChan->idCompT, rc);
    rc = EWDB_RETURN_FAILURE;
  }
  else
  {
    sprintf(szBuffer,"# COMPT MOD idCompT =%d.\n", pChan->idCompT);
  }

  WriteToOutputFile(szBuffer);
  return(rc);
}


int ModTCompT(EWDB_ChannelStruct * pChan)
{
  int rc;
  int iRetCode;
  EWDB_ChannelStruct chan, chanMax, chanMin;
  EWDB_ChannelStruct pChannelBuffer[20];
  int iNumStationsFound, iNumStationsRetrieved, i;

  /* set idCompT */
  chan.idCompT = pChan->idCompT;

  /* initialize other input params to 0 */
  chan.tOff = chan.tOn = 0.0;
  chan.Comp.idComp = 0;
  rc = ewdb_api_GetCompTParams(&chan);
  if(rc != EWDB_RETURN_SUCCESS)
  {
    sprintf(szBuffer,"# COMP MODT FAILED (ewdb_api_GetCompTParams(%d)) return "
                     "codes (%d,%d)\n", pChan->idCompT, rc, chan.idCompT);
    goto WriteOutputFile;
  }

  memcpy(&chanMax, &chan, sizeof(EWDB_ChannelStruct));
  memcpy(&chanMin, &chan, sizeof(EWDB_ChannelStruct));

  chan.tOn = pChan->tOn;
  chan.tOff = pChan->tOff;

  rc = ewdb_api_UpdateCompTTime(&iRetCode, &chan);
  if(rc != EWDB_RETURN_SUCCESS)
  {
    sprintf(szBuffer,"# COMP MODT FAILED (ewdb_api_UpdateCompTTime(%d)) return "
                     "codes (%d,%d)\n", pChan->idCompT, rc, iRetCode);
    goto WriteOutputFile;
  }


  /* now adjust all of the affected chanT's < new tOn*/
  chanMin.tOff = pChan->tOn;
  if(chanMin.tOff > chanMin.tOn)
  {
    rc = ewdb_api_GetSelectedChannels(pChannelBuffer, 
      sizeof(pChannelBuffer)/sizeof(EWDB_ChannelStruct),
      &chanMin, &chanMin, 
      EWDB_CRITERIA_USE_TIME | EWDB_CRITERIA_USE_SCNL, 
      &iNumStationsFound, &iNumStationsRetrieved);
    
    if(rc != EWDB_RETURN_SUCCESS)
    {
      sprintf(szBuffer,"# COMP MODT FAILED (ewdb_api_GetSelectedChannels(%d)) return "
        "codes (%d)\n", pChan->idCompT, rc);
      goto WriteOutputFile;
    }
    
    for(i=0; i < iNumStationsRetrieved; i++)
    {
      if(pChannelBuffer[i].idCompT != pChan->idCompT)
        continue;
      if(pChannelBuffer[i].tOff > chanMin.tOff)
      {
        pChannelBuffer[i].tOn = chanMin.tOff;
        rc = ewdb_api_UpdateChanTTime(&iRetCode, &pChannelBuffer[i]);
        if(rc != EWDB_RETURN_SUCCESS)
        {
          sprintf(szBuffer,"# COMP MODT WARNING: UPDATE CHANT FAILED "
            "(ewdb_api_UpdateChanTTime(%d-%d)) return "
            "codes (%d,%d)\n", 
            pChan->idCompT, pChannelBuffer[i].idChanT, rc, iRetCode);
          WriteToOutputFile(szBuffer);
        }
      }
      else
      {
        rc = ewdb_api_DeleteChanT(pChannelBuffer[i].idChanT);
        if(rc != EWDB_RETURN_SUCCESS)
        {
          sprintf(szBuffer,"# COMP MODT WARNING: DELETE CHANT FAILED "
            "(ewdb_api_DeleteChanT(%d-%d)) return "
            "codes (%d)\n", 
            pChan->idCompT, pChannelBuffer[i].idChanT, rc);
          WriteToOutputFile(szBuffer);
        }
      }
      
    }
  }
  

  /* now adjust all of the affected chanT's  > new tOff*/
  chanMax.tOn = pChan->tOff;
  if(chanMax.tOn < chanMin.tOff)
  {
    rc = ewdb_api_GetSelectedChannels(pChannelBuffer, 
      sizeof(pChannelBuffer)/sizeof(EWDB_ChannelStruct),
      &chanMax, &chanMax, 
      EWDB_CRITERIA_USE_TIME | EWDB_CRITERIA_USE_SCNL, 
      &iNumStationsFound, &iNumStationsRetrieved);
    
    if(rc != EWDB_RETURN_SUCCESS)
    {
      sprintf(szBuffer,"# COMP MODT FAILED (ewdb_api_GetSelectedChannels(%d)) return "
        "codes (%d)\n", pChan->idCompT, rc);
      goto WriteOutputFile;
    }
    
    for(i=0; i < iNumStationsRetrieved; i++)
    {
      if(pChannelBuffer[i].idCompT != pChan->idCompT)
        continue;
      if(pChannelBuffer[i].tOn > chanMax.tOn)
      {
        pChannelBuffer[i].tOff = chanMax.tOn;
        rc = ewdb_api_UpdateChanTTime(&iRetCode, &pChannelBuffer[i]);
        if(rc != EWDB_RETURN_SUCCESS)
        {
          sprintf(szBuffer,"# COMP MODT WARNING: UPDATE CHANT FAILED "
            "(ewdb_api_UpdateChanTTime(%d-%d)) return "
            "codes (%d,%d)\n", 
            pChan->idCompT, pChannelBuffer[i].idChanT, rc, iRetCode);
          WriteToOutputFile(szBuffer);
        }
      }
      else
      {
        rc = ewdb_api_DeleteChanT(pChannelBuffer[i].idChanT);
        if(rc != EWDB_RETURN_SUCCESS)
        {
          sprintf(szBuffer,"# COMP MODT WARNING: DELETE CHANT FAILED "
            "(ewdb_api_DeleteChanT(%d-%d)) return "
            "codes (%d)\n", 
            pChan->idCompT, pChannelBuffer[i].idChanT, rc);
          WriteToOutputFile(szBuffer);
        }
      }
      
    }
  }
  
  sprintf(szBuffer,"# COMP MODT idCompT =%d  tOn=%d  tOff=%d.\n", 
          pChan->idCompT, chan.tOn, chan.tOff);

WriteOutputFile:
  WriteToOutputFile(szBuffer);
  return(rc);
}


int SplitCompT(EWDB_ChannelStruct * pChan)
{
  int rc;
  EWDB_ChannelStruct chan;

  
  chan.idCompT = pChan->idCompT;
  rc = ewdb_api_GetCompTParams(&chan);
  if(rc != EWDB_RETURN_SUCCESS)
  {
    sprintf(szBuffer,"# COMP SPLIT FAILED (ewdb_api_GetCompTParams(%d)) return "
                     "codes (%d,%d)\n", pChan->idCompT, rc, chan.idCompT);
    rc = EWDB_RETURN_FAILURE;
    goto WriteOutputFile;
  }

  chan.tOn = pChan->tOff;

  rc=ewdb_api_SetCompParams(&chan.Comp, chan.tOn, chan.tOff,
    &chan.idCompT, "");
  if(rc == EWDB_RETURN_SUCCESS)
    sprintf(szBuffer,"# COMPT SPLIT IDCOMPT = %d NEW IDCOMPT = %d\n", 
            pChan->idCompT, chan.idCompT);
  else
  {
    sprintf(szBuffer,"# COMPT SPLIT FAILED ewdb_api_SetCompParams() "
                    "return codes (%d,%d)\n", 
            rc, pChan->idCompT);
    rc = EWDB_RETURN_FAILURE;
  }

WriteOutputFile:
  WriteToOutputFile(szBuffer);
  return(rc);
}  /* end SplitCompT() */




/* COMPT COMMANDS
COMPT   |MOD  |   1000008645                 37.3 -121.4 26.0 0 180
COMPT   |MODT |   1000008645                 924000000 1250000000
COMPT   |SPLIT|   1000008645                 1000000000
COMPT   |DEL  |   1000008645                 TRUE
COMPT   |ADD  |              ISCO DK GHZ     37.3 -121.4 26.0 0 180 1000000000 1250000000 "new fake time period"
************************************************************************/
int HandleCompTCommand(char * szCmd, char * szLineBuffer, FILE * fOut)
{
  
  EWDB_ChannelStruct chan, newchan;
  char szBuffer[256];
  char * szTemp;
  char szComment[256];
  int rc = 0;

  memset(&chan, 0 , sizeof(EWDB_ChannelStruct));
  memset(&newchan, 0 , sizeof(EWDB_ChannelStruct));
  memset(szBuffer, 0, sizeof(szTemp));

  /* attempt to parse the id, sta, net, comp, loc */
  /* idCompT */
  strncpy_ew(szBuffer, &szLineBuffer[STA_FILE_OFFSET_ID], STA_FILE_LEN_ID + 1);
  /* check for prev */
  szTemp = strtok(szBuffer," \t\n");
  if(!szTemp)
    chan.idCompT = 0;
  else
  {
    if(!strcmp(szTemp,SPECIAL_USE_PREVIOUS))
    {
      chan.idCompT = SPECIAL_IDPREVIOUS;
    }
    else
    {
      chan.idCompT = atoi(szBuffer);
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
    if(chan.idCompT == SPECIAL_IDPREVIOUS)
      chan.Comp.idComp = GlobalChan.Comp.idComp;
    else
      chan.Comp.idComp = chan.idCompT;
    
    szComment[0] = 0x00;
    szTemp = strtok(&szLineBuffer[STA_FILE_OFFSET_VAR], " \t");
    if(szTemp)
    {
      chan.Comp.Lat = (float)atof(szTemp);
      szTemp = strtok(NULL, " \t");
      if(szTemp)
      {
        chan.Comp.Lon = (float)atof(szTemp);
        szTemp = strtok(NULL, " \t");
        if(szTemp)
        {
          chan.Comp.Elev = (float)atof(szTemp);
          szTemp = strtok(NULL, " \t");
          if(szTemp)
          {
            chan.Comp.Azm = (float)atof(szTemp);
            szTemp = strtok(NULL, " \t");
            if(szTemp)
            {
              chan.Comp.Dip = (float)atof(szTemp);
              szTemp = strtok(NULL, " \t");
              if(szTemp)
              {
                chan.tOn = atof(szTemp);
                szTemp = strtok(NULL, " \t");
                if(szTemp)
                {
                  chan.tOff = atof(szTemp);
                  szTemp = strtok(NULL, "\"");
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
      }
    }
    logit("e","Attempting to add CompT(%s.%s.%s.%s): \n"
      "Lat/Lon/Elev: (%.2f / %.2f / %.0f) - Azm/Dip(%.0f / %.0f)\n"
      "On-Off(%.0f - %.0f)\n"
      "  Comment:\"%s\"\n", 
      chan.Comp.Sta, chan.Comp.Comp, chan.Comp.Net, chan.Comp.Loc, 
      chan.Comp.Lat, chan.Comp.Lon, chan.Comp.Elev,
      chan.Comp.Azm, chan.Comp.Dip,
      chan.tOn, chan.tOff, szComment);
    rc=AddCompT(&chan, FALSE /* don't force */);
    
  }
  else
  {
    if(chan.idCompT == SPECIAL_IDPREVIOUS)
      chan.idCompT = GlobalChan.idCompT;

    if(strcmp(szCmd, "END") == 0)
    {
      chan.tOff = atof(&szLineBuffer[STA_FILE_OFFSET_VAR]);
      logit("e","Attempting to end CompT(%s.%s.%s.%s: %d) at %.2f.\n",
        chan.Comp.Sta, chan.Comp.Comp, chan.Comp.Net, chan.Comp.Loc, 
        chan.idCompT, chan.tOff);
      rc=EndCompT(&chan);
    }
    else if(strcmp(szCmd, "DEL") == 0)
    {
      logit("e","Attempting to delete CompT(%s.%s.%s.%s: %d).\n",
        chan.Comp.Sta, chan.Comp.Comp, chan.Comp.Net, chan.Comp.Loc, 
        chan.idCompT);
      rc=DeleteCompT(&chan);
      if(rc == EWDB_RETURN_SUCCESS)
        chan.idCompT = 0;
    }
    else if(strcmp(szCmd, "MOD") == 0)
    {
      szComment[0] = 0x00;
      szTemp = strtok(&szLineBuffer[STA_FILE_OFFSET_VAR], " \t");
      if(szTemp)
      {
        chan.Comp.Lat = (float)atof(szTemp);
        szTemp = strtok(NULL, " \t");
        if(szTemp)
        {
          chan.Comp.Lon = (float)atof(szTemp);
          szTemp = strtok(NULL, " \t");
          if(szTemp)
          {
            chan.Comp.Elev = (float)atof(szTemp);
            szTemp = strtok(NULL, " \t");
            if(szTemp)
            {
              chan.Comp.Azm = (float)atof(szTemp);
              szTemp = strtok(NULL, " \t");
              if(szTemp)
              {
                chan.Comp.Dip = (float)atof(szTemp);
                szTemp = strtok(NULL, " \t");
                if(szTemp)
                {
                  chan.tOn = atof(szTemp);
                  szTemp = strtok(NULL, " \t");
                  if(szTemp)
                  {
                    chan.tOff = atof(szTemp);
                    szTemp = strtok(NULL, "\"");
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
        }
      }
      logit("e","Attempting to mod CompT(%s.%s.%s.%s: %d).\n"
        "Lat/Lon/Elev: (%.2f / %.2f / %.0f).  Azm/Dip: (%.0f / %.0f)\n"
        "  Comment:\"%s\"\n", 
        chan.Comp.Sta, chan.Comp.Comp, chan.Comp.Sta, chan.Comp.Loc,
        chan.idCompT,
        chan.Comp.Lat, chan.Comp.Lon, chan.Comp.Elev, 
        chan.Comp.Azm, chan.Comp.Dip,
        szComment);
      rc=ModCompT(&chan);
    }
    else if(strcmp(szCmd, "MODT") == 0)
    {
      szTemp = strtok(&szLineBuffer[STA_FILE_OFFSET_VAR], " \t");
      if(szTemp)
      {
        chan.tOn = (float)atof(szTemp);
        szTemp = strtok(NULL, " \t");
        if(szTemp)
        {
          chan.tOff = (float)atof(szTemp);
        }
      }
      logit("e","Attempting to mod time for CompT(%s.%s%s.%s: %d) New On-Off(%.0f - %.0f)\n",
        chan.Comp.Sta, chan.Comp.Comp, chan.Comp.Sta, chan.Comp.Loc,
        chan.idCompT, chan.tOn, chan.tOff);
      rc=ModTCompT(&chan);
    }
    else if(strcmp(szCmd, "SPLIT") == 0)
    {
      szTemp = strtok(&szLineBuffer[STA_FILE_OFFSET_VAR], " \t");
      if(szTemp)
      {
        chan.tOff = (float)atof(szTemp);
      }
      logit("e","Attempting to Split CompT(%s.%s.%s.%s: %d) at time %.2f.\n",
        chan.Comp.Sta, chan.Comp.Comp, chan.Comp.Sta, chan.Comp.Loc,
        chan.idCompT,
        chan.tOff);
      rc=SplitCompT(&chan);
    }
    else
    {
      fprintf(fOut,"#ERROR: Unsupported COMPT command (%s).\n", szCmd);
      rc = -1;
    }
  }  /* end else command=ADD */

  if(rc == EWDB_RETURN_SUCCESS)
    GlobalChan.idCompT = chan.idCompT;
  
  return(rc);
}  /* end HandleCompTCommand() */

