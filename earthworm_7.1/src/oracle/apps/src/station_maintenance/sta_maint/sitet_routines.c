
#include <ewdb_ora_api.h>
#include "sta_maint.h"


int AddSiteT(EWDB_ChannelStruct * pChan);
int EndSiteT(EWDB_ChannelStruct * pChan);
int DeleteSiteT(EWDB_ChannelStruct * pChan);
int ModSiteT(EWDB_ChannelStruct * pChan);
int ModTSiteT(EWDB_ChannelStruct * pChan);
int SplitSiteT(EWDB_ChannelStruct * pChan);

int AddSiteT(EWDB_ChannelStruct * pChan)
{
  int rc;
  char szBuffer[256];
  EWDB_ChannelStruct chan;

  if(pChan->idSite <= 0)
  {
    /* obtain idSite from sta/net */
    rc = ewdb_api_GetidSite(pChan);
    if(rc != EWDB_RETURN_SUCCESS)
    {
       sprintf(szBuffer,"# SITET ADD FAILED during ewdb_api_GetidSite(). \n"
                        "#   params (%s/%s - %d).  rc = %d.\n", 
                        pChan->Comp.Sta, pChan->Comp.Net, pChan->idSite, rc);
       rc = EWDB_RETURN_FAILURE;
       goto AddSiteT_WriteOutput;
    }

  }
  rc=ewdb_api_CreateSiteT(pChan);
  if(rc == EWDB_RETURN_SUCCESS)
    sprintf(szBuffer,"# SITET ADD IDSITET = %d\n", pChan->idSiteT);
  else
  {
    sprintf(szBuffer,"# SITET ADD FAILED return codes (%d,%d)\n", rc, pChan->idSiteT);
    if(pChan->idSiteT == -3)
    {
      WriteToOutputFile(szBuffer);
      memcpy(&chan,pChan,sizeof(chan));

      rc = ewdb_api_GetSiteTParams(&chan);
      if(rc == EWDB_RETURN_SUCCESS || EWDB_RETURN_WARNING)
      {
        if(rc == EWDB_RETURN_WARNING)
        {
          sprintf(szBuffer,"Mulitple overlapping SiteT records.  Only first one printed.\n");
          WriteToOutputFile(szBuffer);
        }
          sprintf(szBuffer,"Overlapping record:  idSiteT(%d) idSite(%d) "
                           "tOn - tOff (%.2f - %.2f) Lat/Lon/Elev(%d/%d/%d)\n",
                  chan.idSiteT, chan.idSite, chan.tOn, chan.tOff, 
                  chan.Comp.Lat, chan.Comp.Lon, chan.Comp.Elev);
          rc=EWDB_RETURN_FAILURE;
      }
    }  /* end if overlapping existing SiteT records */
  }

AddSiteT_WriteOutput:
  WriteToOutputFile(szBuffer);
  return(rc);
}  /* end AddSiteT() */


int EndSiteT(EWDB_ChannelStruct * pChan)
{
  int rc;
  int iRetCode;
  EWDB_ChannelStruct chan;
  char szBuffer[256];

  chan.idSiteT = pChan->idSiteT;
  rc = ewdb_api_GetSiteTParams(&chan);
  if(rc != EWDB_RETURN_SUCCESS)
  {
    sprintf(szBuffer,"# SITET END FAILED (ewdb_api_GetSiteTParams()) return "
                     "codes (%d,%d)\n", rc, pChan->idSiteT);
    goto WriteOutputFile;
  }

  chan.tOff = pChan->tOff;
  rc = ewdb_api_UpdateSiteT(&iRetCode, &chan, NULL);
  if(rc != EWDB_RETURN_SUCCESS)
  {
    sprintf(szBuffer,"# SITET END FAILED (ewdb_api_GetSiteTParams()) return "
                     "codes (%d,%d)\n", rc, pChan->idSiteT);
    goto WriteOutputFile;
  }

  sprintf(szBuffer,"# SITET END idSiteT =%d  tOn=%.0f  tOff=%.0f.\n", 
          pChan->idSiteT, chan.tOn, chan.tOff);

WriteOutputFile:
  WriteToOutputFile(szBuffer);
  return(rc);
}


int DeleteSiteT(EWDB_ChannelStruct * pChan)
{
  char szBuffer[256];
  int rc;
  EWDB_ChannelStruct chan;
  EWDB_ChannelStruct pChannelBuffer[10];
  int iNumStationsFound, iNumStationsRetrieved;
  int i;

  /* search for compt's first within the SiteT's time range */

  /* Get SiteT information */
  memset(&chan,0,sizeof(chan));
  chan.idSiteT = pChan->idSiteT;
  rc = ewdb_api_GetSiteTParams(&chan);
  if(rc != EWDB_RETURN_SUCCESS)
  {
      sprintf(szBuffer,"# SITET DEL FAILED (ewdb_api_GetSiteTParams()) idSiteT=%d returned "
                       "ERROR - idSiteT Key not found in DB.\n",pChan->idSiteT);
      rc = EWDB_RETURN_FAILURE;
      goto WriteOutputFile;

  }

  /* Get all CompT records with matching Sta/Net and overlapping
     time range  
   ************************************************************/
  memset(pChannelBuffer,0,sizeof(pChannelBuffer));
  strcpy(chan.Comp.Comp,"*");
  strcpy(chan.Comp.Loc,"*");
  rc = ewdb_api_GetSelectedStations(pChannelBuffer,
                                    sizeof(pChannelBuffer)/sizeof(EWDB_ChannelStruct),
                                    &chan, NULL,
                                    EWDB_CRITERIA_USE_TIME | EWDB_CRITERIA_USE_SCNL,
                                    &iNumStationsFound, &iNumStationsRetrieved);

  if(rc == EWDB_RETURN_SUCCESS)
  {
    /* delete each and continue */
    for(i=0;i<iNumStationsRetrieved;i++)
    {
      rc = ewdb_api_DeleteCompT(pChannelBuffer[i].idCompT);
      if(rc == EWDB_RETURN_SUCCESS)
      {
        sprintf(szBuffer,"# SITET DEL  - COMPT(%d) SUCCESSFULLY DELETED.",
                pChannelBuffer[i].idCompT);
        WriteToOutputFile(szBuffer);
      }
       else
      {
        sprintf(szBuffer,"# SITET DEL WARNING (ewdb_api_DeleteCompT(%d)) "
                         "FAILED WITH ERROR(%d)\n",
                pChannelBuffer[i].idCompT,rc);
        WriteToOutputFile(szBuffer);
      }
    }
  }
  else if(rc == EWDB_RETURN_SUCCESS)
  {
    /* ah crap.  We need to malloc a larger buffer, and that's a lot 
       of work in this space.
     ********************************************************************/
    rc = EWDB_RETURN_FAILURE;
    sprintf(szBuffer,"# SITET DEL FAILED (ewdb_api_GetSelectedStations()) sta/net (%s/%s) (%.2f-%.2f) returned "
                     "UNKNOWN ERROR!\n",pChan->Comp.Sta, pChan->Comp.Net, 
                      pChan->tOn, pChan->tOff);
    goto WriteOutputFile;
  }
  else
  {
    rc = EWDB_RETURN_FAILURE;
    sprintf(szBuffer,"# SITET DEL FAILED (ewdb_api_GetSelectedStations()) sta/net (%s/%s) (%.2f-%.2f) returned "
                     "TOO MANY COMPT RECORDS TO DELETE(%d)  - MAX(%d) supported!\n",
                      pChan->Comp.Sta, pChan->Comp.Net, pChan->tOn, pChan->tOff,
                      iNumStationsFound, iNumStationsRetrieved);

    goto WriteOutputFile;
  }

  rc = ewdb_api_DeleteSiteT(pChan->idSiteT);

  if(rc != EWDB_RETURN_SUCCESS)
  {
    if(rc == EWDB_RETURN_WARNING)
      sprintf(szBuffer,"# SITET DEL FAILED (ewdb_api_DeleteSiteT()) idSite=%d returned "
                       "FOREIGN KEY CONSTRAINT WARNING!\n",pChan->idSiteT);
    else
      sprintf(szBuffer,"# SITET DEL FAILED (ewdb_api_DeleteSiteT()) idSiteT=%d returned "
                       "UNKNOWN ERROR!\n",pChan->idSiteT);

    goto WriteOutputFile;
  }

  sprintf(szBuffer,"# SITET DEL idSiteT =%d  SUCCESSFUL.\n", 
          pChan->idSiteT);

WriteOutputFile:
  WriteToOutputFile(szBuffer);
  return(rc);
}


int ModSiteT(EWDB_ChannelStruct * pChan)
{
  int rc;
  int iRetCode;
  char szBuffer[256];

  rc = ewdb_api_ModifySiteParams(&iRetCode, pChan, NULL);
  if(rc != EWDB_RETURN_SUCCESS)
  {
    sprintf(szBuffer,"# SITET MOD FAILED (ewdb_api_GetSiteTParams()) return "
                     "codes (%d,%d)\n", rc, pChan->idSiteT);
    goto WriteOutputFile;
  }

  sprintf(szBuffer,"# SITET MOD SUCCESSFUL idSiteT =%d.\n", pChan->idSiteT);
WriteOutputFile:
  WriteToOutputFile(szBuffer);
  return(rc);
}


int ModTSiteT(EWDB_ChannelStruct * pChan)
{
  int rc;
  int iRetCode;
  char szBuffer[256];

  rc = ewdb_api_UpdateSiteT(&iRetCode, pChan, NULL);
  if(rc != EWDB_RETURN_SUCCESS)
  {
    sprintf(szBuffer,"# SITET MODT FAILED (ewdb_api_UpdateSiteT()) return "
                     "codes (%d,%d)\n", rc, pChan->idSiteT);
    goto WriteOutputFile;
  }

  sprintf(szBuffer,"# SITET MODT idSiteT =%d  tOn=%.0f  tOff=%.0f.\n", 
          pChan->idSiteT, pChan->tOn,pChan->tOff);
WriteOutputFile:
  WriteToOutputFile(szBuffer);
  return(rc);
}


int SplitSiteT(EWDB_ChannelStruct * pChan)
{
  int rc;
  char szBuffer[256];

  rc = ewdb_api_SplitSiteT(pChan, &pChan->idSiteT);
  if(rc != EWDB_RETURN_SUCCESS)
  {
    sprintf(szBuffer,"# SITET SPLIT FAILED (ewdb_api_SplitSiteT()) return "
                     "codes (%d,%d)\n", rc, pChan->idSiteT);
    goto WriteOutputFile;
  }

  sprintf(szBuffer,"# SITET MODT idSiteT =%d  tOn=%.0f  tOff=%.0f.\n", 
          pChan->idSiteT, pChan->tOn,pChan->tOff);
WriteOutputFile:
  WriteToOutputFile(szBuffer);
  return(rc);
}



/* SITE COMMANDS
SITET   |ADD  |   1000001038 ISCO US         37.5 -121.5 23 924000000 1250000000 "New fake SITET by DK"
SITET   |DEL  |   ?          ISCO US         
SITET   |MOD  |   ?          ISCO US         37.30000    -121.20000     24.0
SITET   |SPLIT|   ?          ISCO US         1020000000
SITET   |END  |   ?          ISCO US         1250000000
************************************************************************/
int HandleSiteTCommand(char * szCmd, char * szLineBuffer, FILE * fOut)
{
  
  EWDB_ChannelStruct chan, newchan;
  char szBuffer[256];
  char * szTemp;
  char szComment[256];
  int rc = 0;
  
  memset(&chan, 0 , sizeof(EWDB_ChannelStruct));
  memset(&newchan, 0 , sizeof(EWDB_ChannelStruct));
  memset(szBuffer, 0, sizeof(szTemp));
  
  /* attempt to parse the id, sta, net */
  /* idsiteT */
  strncpy_ew(szBuffer, &szLineBuffer[STA_FILE_OFFSET_ID], STA_FILE_LEN_ID + 1);
  /* check for prev */
  szTemp = strtok(szBuffer," \t\n");
  if(!szTemp)
    chan.idSiteT = 0;
  else
  {
    if(!strcmp(szTemp,SPECIAL_USE_PREVIOUS))
    {
      chan.idSiteT = SPECIAL_IDPREVIOUS;
    }
    else
    {
      chan.idSiteT = atoi(szBuffer);
    }
  }
  
  /* sta */
  strncpy_ew(szBuffer,&szLineBuffer[STA_FILE_OFFSET_STA],STA_FILE_LEN_STA + 1);
  szTemp = strtok(szBuffer," ");
  if(szTemp)
    strncpy_ew(chan.Comp.Sta, szTemp, sizeof(chan.Comp.Sta));
  
  /* net */
  strncpy_ew(szBuffer,&szLineBuffer[STA_FILE_OFFSET_NET],STA_FILE_LEN_NET + 1);
  szTemp = strtok(szBuffer," ");
  if(szTemp)
    strncpy_ew(chan.Comp.Net, szTemp, sizeof(chan.Comp.Net));
  
  /* now handle the command */
  if(strcmp(szCmd, "ADD") == 0)
  {
    if(chan.idSiteT == SPECIAL_IDPREVIOUS)
      chan.idSite = GlobalChan.idSite;
    else
      chan.idSite = chan.idSiteT;
    
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
    logit("e","Attempting to add SiteT(%s.%s)-%d: \n"
      "Lat/Lon/Elev: (%.2f / %.2f / %.0f) - On-Off(%.0f - %.0f)\n"
      "  Comment:\"%s\"\n", 
      chan.Comp.Sta, chan.Comp.Net, chan.idSite,
      chan.Comp.Lat, chan.Comp.Lon, chan.Comp.Elev,
      chan.tOn, chan.tOff, szComment);
    rc = AddSiteT(&chan);
  }
  else
  {
    if(chan.idSiteT == SPECIAL_IDPREVIOUS)
      chan.idSiteT = GlobalChan.idSiteT;
    
    if(strcmp(szCmd, "END") == 0)
    {
      chan.tOff = atof(&szLineBuffer[STA_FILE_OFFSET_VAR]);
      logit("e","Attempting to end SiteT(%s.%s: %d) at %.2f.\n",
        chan.Comp.Sta, chan.Comp.Net, chan.idSiteT, chan.tOff);
      rc = EndSiteT(&chan);
    }
    else if(strcmp(szCmd, "DEL") == 0)
    {
      logit("e","Attempting to delete SiteT(%s.%s: %d).\n",
        chan.Comp.Sta, chan.Comp.Net, chan.idSiteT);
      rc = DeleteSiteT(&chan);
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
              strncpy_ew(szComment, szTemp, sizeof(szComment));
              szComment[sizeof(szComment)-1] = 0x00;
            }
          }
        }
      }
      logit("e","Attempting to mod SiteT(%s.%s: %d).\n"
        "Lat/Lon/Elev: (%.2f / %.2f / %.0f).\n"
        "  Comment:\"%s\"\n", 
        chan.Comp.Sta, chan.Comp.Net, chan.idSiteT,
        chan.Comp.Lat, chan.Comp.Lon, chan.Comp.Elev,
        szComment);
      rc = ModSiteT(&chan);
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
      logit("e","Attempting to mod time for SiteT(%s.%s: %d) New On-Off(%.0f - %.0f)\n",
        chan.Comp.Sta, chan.Comp.Net, chan.idSiteT, chan.tOn, chan.tOff);
      rc = ModTSiteT(&chan);
    }
    else if(strcmp(szCmd, "SPLIT") == 0)
    {
      szTemp = strtok(&szLineBuffer[STA_FILE_OFFSET_VAR], " \t");
      if(szTemp)
      {
        chan.tOn = (float)atof(szTemp);
      }
      logit("e","Attempting to Split SiteT(%s.%s: %d) at time %.2f.\n",
        chan.Comp.Sta, chan.Comp.Net, chan.idSiteT,
        chan.tOn);
      rc = SplitSiteT(&chan);
    }
    else
    {
      fprintf(fOut,"#ERROR: Unsupported SITET command (%s).\n", szCmd);
      rc = -1;
    }
  }  /* end else if command = ADD */
  
  if(rc == EWDB_RETURN_SUCCESS)
    GlobalChan.idSiteT = chan.idSiteT;
  
  return(rc);
  
}  /* end HandleSiteTCommand() */

