
#include <ewdb_ora_api.h>
#include "sta_maint.h"


/* external functions */
int EndCompT(EWDB_ChannelStruct * pChan);
int DeleteCompT(EWDB_ChannelStruct * pChan);
int EndSiteT(EWDB_ChannelStruct * pChan);
int DeleteSiteT(EWDB_ChannelStruct * pChan);
int DeleteComp(EWDB_ChannelStruct * pChan);

int AddSite(EWDB_ChannelStruct * pChan, char * szComment);
int EndSite(EWDB_ChannelStruct * pChan);
int DeleteSite(EWDB_ChannelStruct * pChan);
int RenameSite(EWDB_ChannelStruct * pChan, EWDB_ChannelStruct * pNewChan);


int AddSite(EWDB_ChannelStruct * pChan, char * szComment)
{

  int rc;
  char szBuffer[256];

  rc=ewdb_api_CreateSite(pChan,szComment);
  if(rc == EWDB_RETURN_SUCCESS)
    sprintf(szBuffer,"# SITE ADD IDSITE = %d\n", pChan->idSite);
  else
    sprintf(szBuffer,"# SITE ADD FAILED return codes (%d,%d)\n", rc, pChan->idSite);

  WriteToOutputFile(szBuffer);

  return(rc);
}


int EndSite(EWDB_ChannelStruct * pChan)
{
  int rc;
  int i;

  EWDB_ChannelStruct csCriteria, csCriteriaMax;
  EWDB_ChannelStruct pCompTBuffer[256];
  int iBufferLen = 256;
  int iNumStationsFound, iNumStationsRetrieved;
  int iCriteria = 0;
  char szBuffer[256];

  /*
  Before we end the site, we have to do the following:
  1)Search for components from this site
  2)End each of those components

  */

  memset(&csCriteria,0, sizeof(EWDB_ChannelStruct));
  memset(&csCriteriaMax,0, sizeof(EWDB_ChannelStruct));
  iCriteria = EWDB_CRITERIA_USE_TIME | EWDB_CRITERIA_USE_SCNL;
  csCriteria.tOn = pChan->tOff;
  csCriteria.tOff = 9999999999;
  strcpy(csCriteria.Comp.Comp, "*");
  strcpy(csCriteria.Comp.Loc, "*");
  strncpy(csCriteria.Comp.Sta, pChan->Comp.Sta, sizeof(csCriteria.Comp.Sta));
  strncpy(csCriteria.Comp.Net, pChan->Comp.Net, sizeof(csCriteria.Comp.Net));


  rc=ewdb_api_GetSelectedStations(pCompTBuffer, iBufferLen, 
                                  &csCriteria, &csCriteriaMax, iCriteria, 
                                  &iNumStationsFound, &iNumStationsRetrieved);


  if(rc != EWDB_RETURN_SUCCESS)
  {

    sprintf(szBuffer,"# SITE END FAILED (ewdb_api_GetSelectedStations)return "
                     "codes (%d,%d,%d)\n", rc,
            iNumStationsFound, iNumStationsRetrieved);
    goto WriteToFile;
  }
  for(i=0; i < iNumStationsRetrieved; i++)
  {
    if(pCompTBuffer[i].tOn < pChan->tOff)
    {
       pCompTBuffer[i].tOff = pChan->tOff;
       rc = EndCompT(&pCompTBuffer[i]);
       if(rc!= EWDB_RETURN_SUCCESS)
       {
         sprintf(szBuffer,"# SITE END FAILED(EndCompT()) return codes (%d,%d)\n", 
                 rc, pCompTBuffer[i].idCompT);
         WriteToOutputFile(szBuffer);
       }
    }
    else
    {
      rc = DeleteCompT(&pCompTBuffer[i]);
      if(rc!= EWDB_RETURN_SUCCESS)
      {
        sprintf(szBuffer,"# SITE END FAILED(DeleteCompT()) return codes (%d,%d)\n", 
                rc, pCompTBuffer[i].idCompT);
        WriteToOutputFile(szBuffer);
      }
    }
  }
  /*
  To end the site, we must do the following
  3)Search for SiteT's from this site.
  4) For each SiteT, check Start/End time and either:
       a) if start/end < tThreshold, ignore!
       b) if start < tThreshold, end > tThreshold, set end=tThreshold
       c) if start > tThreshold, delete!

  */

  rc=ewdb_api_GetSelectedSites(pCompTBuffer, iBufferLen, 
                                  &csCriteria, &csCriteriaMax, iCriteria, 
                                  &iNumStationsFound, &iNumStationsRetrieved);


  if(rc != EWDB_RETURN_SUCCESS)
  {

    sprintf(szBuffer,"# SITE END FAILED (ewdb_api_GetSelectedSites)return "
                     "codes (%d,%d,%d)\n", rc,
            iNumStationsFound, iNumStationsRetrieved);
    goto WriteToFile;
  }
  for(i=0; i < iNumStationsRetrieved; i++)
  {
    if(pCompTBuffer[i].tOn < pChan->tOff)
    {
       pCompTBuffer[i].tOff = pChan->tOff;
       rc = EndSiteT(&pCompTBuffer[i]);
       if(rc!= EWDB_RETURN_SUCCESS)
       {
         sprintf(szBuffer,"# SITE END FAILED(EndSiteT()) return codes (%d,%d)\n",
                 rc, pCompTBuffer[i].idSiteT);
         WriteToOutputFile(szBuffer);
       }
    }
    else
    {
      rc = DeleteSiteT(&pCompTBuffer[i]);
      if(rc!= EWDB_RETURN_SUCCESS)
      {
        sprintf(szBuffer,"# SITE END FAILED(DeleteSiteT()) return codes (%d,%d)\n", 
                rc, pCompTBuffer[i].idSiteT);
        WriteToOutputFile(szBuffer);
      }
    }
  }

  if(rc == EWDB_RETURN_SUCCESS)
    sprintf(szBuffer,"# SITE END IDSITE = %d\n", pChan->idSite);
  else
    sprintf(szBuffer,"# SITE END FAILED return codes (%d,%d)\n", rc, pChan->idSite);

WriteToFile:
  WriteToOutputFile(szBuffer);

  return(rc);
}


int DeleteSite(EWDB_ChannelStruct * pChan)
{
  int rc;
  int i;

  EWDB_ChannelStruct csCriteria, csCriteriaMax;
  EWDB_ChannelStruct pCompTBuffer[256];
  int iBufferLen = 256;
  int iNumStationsFound, iNumStationsRetrieved;
  int iCriteria = 0;
  char szBuffer[256];



  memset(&csCriteria,0, sizeof(EWDB_ChannelStruct));
  memset(&csCriteriaMax,0, sizeof(EWDB_ChannelStruct));
  iCriteria = EWDB_CRITERIA_USE_TIME | EWDB_CRITERIA_USE_SCNL;
  csCriteria.tOn = 0;
  csCriteria.tOff = 9999999999;
  strcpy(csCriteria.Comp.Comp, "*");
  strcpy(csCriteria.Comp.Loc, "*");
  strncpy(csCriteria.Comp.Sta, pChan->Comp.Sta, sizeof(csCriteria.Comp.Sta));
  strncpy(csCriteria.Comp.Net, pChan->Comp.Net, sizeof(csCriteria.Comp.Net));
  csCriteria.idSite = pChan->idSite;

  rc=ewdb_api_GetSelectedStations(pCompTBuffer, iBufferLen, 
                                  &csCriteria, &csCriteriaMax, iCriteria, 
                                  &iNumStationsFound, &iNumStationsRetrieved);


  if(rc != EWDB_RETURN_SUCCESS)
  {

    sprintf(szBuffer,"# SITE DELETE FAILED (ewdb_api_GetSelectedStations)return "
                     "codes (%d,%d,%d)\n", rc,
            iNumStationsFound, iNumStationsRetrieved);
    goto WriteToFile;
  }
  for(i=0; i < iNumStationsRetrieved; i++)
  {
    rc = DeleteComp(&pCompTBuffer[i]);
    if(rc!= EWDB_RETURN_SUCCESS)
    {
      sprintf(szBuffer,"# SITE DELETE FAILED(DeleteComp()) return codes (%d,%d)\n", 
        rc, pChan->idCompT);
      WriteToOutputFile(szBuffer);
    }

  }

  rc=ewdb_api_GetSelectedSites(pCompTBuffer, iBufferLen, 
                                  &csCriteria, &csCriteriaMax, iCriteria, 
                                  &iNumStationsFound, &iNumStationsRetrieved);


  if(rc != EWDB_RETURN_SUCCESS)
  {

    sprintf(szBuffer,"# SITE DELETE FAILED (ewdb_api_GetSelectedSites)return "
                     "codes (%d,%d,%d)\n", 
            rc, iNumStationsFound, iNumStationsRetrieved);
    goto WriteToFile;
  }
  for(i=0; i < iNumStationsRetrieved; i++)
  {
    rc = DeleteSiteT(&pCompTBuffer[i]);
    if(rc!= EWDB_RETURN_SUCCESS)
    {
      sprintf(szBuffer,"# SITE DELETE FAILED(DeleteSiteT()) return codes (%d,%d)\n", 
        rc, pChan->idCompT);
      WriteToOutputFile(szBuffer);
    }
  }

  rc = ewdb_api_DeleteSite(pChan->idSite);
  if(rc == EWDB_RETURN_SUCCESS)
      sprintf(szBuffer,"# SITE DELETE SUCCESSFUL  idSite = (%d)\n", 
              pChan->idSite);
  else
      sprintf(szBuffer,"# SITE DELETE FAILED(DeleteSiteT()) return codes (%d,%d)\n", 
              rc, pChan->idSite);

WriteToFile:
  WriteToOutputFile(szBuffer);

  return(rc);
}  /* end DeleteSite() */


int RenameSite(EWDB_ChannelStruct * pChan, EWDB_ChannelStruct * pNewChan)
{
  WriteToOutputFile("Site(Rename) command is not supported.\n");
  return(-1);
}  /* end RenameSite() */



/* SITE COMMANDS
SITE    |ADD  |              ISCO DK         "Fake station added by DK"
SITE    |END  |   1000001038 ISCO US         1300000000.24
SITE    |DEL  |   1000001038 ISCO DK
SITE    |RENM |   1000001038 ISCO DK         ISCDK US
************************************************************************/
int HandleSiteCommand(char * szCmd, char * szLineBuffer, FILE * fOut)
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
  /* idsite */
  strncpy_ew(szBuffer, &szLineBuffer[STA_FILE_OFFSET_ID], STA_FILE_LEN_ID + 1);
  chan.idSite = atoi(szBuffer);
  
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
    szTemp = strtok(&szLineBuffer[STA_FILE_OFFSET_VAR], "\"");
    if(szTemp)
    {
      strncpy_ew(szComment, szTemp, sizeof(szComment));
    }
    else
    {
      szComment[0] = 0x00;
    }
    
    logit("e","Attempting to add Site(%s.%s).\n  Comment:\"%s\"\n", 
      chan.Comp.Sta, chan.Comp.Net, szComment);
    rc = AddSite(&chan, szComment);
  }
  else
  {
    if(!chan.idSite)
    {
      rc = ewdb_api_GetidSite(&chan);
      if(rc != EWDB_RETURN_SUCCESS)
      {
        sprintf(szBuffer,"# PREP for SITE (%s)  FAILED ewdb_api_GetidSite(). "
                         "Return code for (%s/%s) (%d,%d)\n", 
                szCmd, chan.Comp.Sta,chan.Comp.Net, rc, chan.idSite);
        WriteToOutputFile(szBuffer);
        return(rc);
      }
    }
    if(strcmp(szCmd, "END") == 0)
    {
      chan.tOff = atof(&szLineBuffer[STA_FILE_OFFSET_VAR]);
      logit("e","Attempting to end Site(%s.%s: %d) at %.2f.\n",
        chan.Comp.Sta, chan.Comp.Net, chan.idSite, chan.tOff);
      rc = EndSite(&chan);
    }
    else if(strcmp(szCmd, "DEL") == 0)
    {
      logit("e","Attempting to delete Site(%s.%s: %d).\n",
        chan.Comp.Sta, chan.Comp.Net, chan.idSite);
      rc = DeleteSite(&chan);
    }
    else if(strcmp(szCmd, "RENM") == 0)
    {
      szTemp = strtok(&szLineBuffer[STA_FILE_OFFSET_VAR], " \t\n");
      if(szTemp)
      {
        strncpy_ew(chan.Comp.Comp, szTemp, sizeof(chan.Comp.Comp));
        szTemp = strtok(NULL, " \t\n");
        if(szTemp)
          strncpy_ew(newchan.Comp.Sta, szTemp, sizeof(newchan.Comp.Sta));
      }
      logit("e","Attempting to rename Site(%s.%s: %d) to (%s.%s).\n",
        chan.Comp.Sta, chan.Comp.Net, chan.idSite,
        newchan.Comp.Sta, newchan.Comp.Net);
      rc = RenameSite(&chan,&newchan);
    }
    else
    {
      fprintf(fOut,"#ERROR: Unsupported SITE command (%s).\n", szCmd);
      rc = -1;
    }
  }  /* end else "ADD" command */
  
  if(rc == EWDB_RETURN_SUCCESS)
    GlobalChan.idSite = chan.idSite;
  
  return(rc);

}  /* end HandleSiteCommand() */

