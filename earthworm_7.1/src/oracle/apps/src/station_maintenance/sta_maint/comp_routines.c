
#include <ewdb_ora_api.h>
#include "sta_maint.h"


/* functions defined externally */
int EndCompT(EWDB_ChannelStruct * pChan);
int DeleteCompT(EWDB_ChannelStruct * pChan);


/* functions in this file */
int AddComp(EWDB_ChannelStruct * pChan);
int EndComp(EWDB_ChannelStruct * pChan);
int DeleteComp(EWDB_ChannelStruct * pChan);
int RenameComp(EWDB_ChannelStruct * pChan, EWDB_ChannelStruct * pNewChan);

static char szBuffer[256];

int AddComp(EWDB_ChannelStruct * pChan)
{
  int rc;
  rc=ewdb_api_CreateComponent(&pChan->Comp.idComp, pChan->Comp.Sta, pChan->Comp.Comp, 
                              pChan->Comp.Net, pChan->Comp.Loc, "");
  if(rc == EWDB_RETURN_SUCCESS)
    sprintf(szBuffer,"# COMP ADD IDCOMP = %d\n", pChan->Comp.idComp);
  else
    sprintf(szBuffer,"# COMP ADD FAILED return codes (%d,%d)\n", rc, pChan->Comp.idComp);

  WriteToOutputFile(szBuffer);

  return(rc);
}


int EndComp(EWDB_ChannelStruct * pChan)
{
  int rc;
  int i;

  EWDB_ChannelStruct csCriteria, csCriteriaMax;
  EWDB_ChannelStruct pCompTBuffer[256];
  int iBufferLen = 256;
  int iNumStationsFound, iNumStationsRetrieved;
  int iCriteria = 0;

  /*
  Before we end the comp, we have to do the following:
  1)Search for compt's for the involved time range >tOff,
     and truncate/delete each of them.

  */

  memset(&csCriteria,0, sizeof(EWDB_ChannelStruct));
  memset(&csCriteriaMax,0, sizeof(EWDB_ChannelStruct));
  iCriteria = EWDB_CRITERIA_USE_TIME | EWDB_CRITERIA_USE_SCNL;
  csCriteria.tOn = pChan->tOff;
  csCriteria.tOff = 9999999999;
  strncpy(csCriteria.Comp.Comp, pChan->Comp.Comp, sizeof(csCriteria.Comp.Comp));
  strncpy(csCriteria.Comp.Loc, pChan->Comp.Loc, sizeof(csCriteria.Comp.Loc));
  strncpy(csCriteria.Comp.Sta, pChan->Comp.Sta, sizeof(csCriteria.Comp.Sta));
  strncpy(csCriteria.Comp.Net, pChan->Comp.Net, sizeof(csCriteria.Comp.Net));


  rc=ewdb_api_GetCompTInfo(pCompTBuffer, iBufferLen, 
                                  &csCriteria, &csCriteriaMax, iCriteria, 
                                  &iNumStationsFound, &iNumStationsRetrieved);


  if(rc != EWDB_RETURN_SUCCESS)
  {

    sprintf(szBuffer,"# COMP END FAILED (ewdb_api_GetCompTInfo)return "
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
         sprintf(szBuffer,"# COMP END FAILED(EndCompT()) return codes (%d,%d)\n", 
                 rc, pCompTBuffer[i].idCompT);
         WriteToOutputFile(szBuffer);
       }
    }
    else
    {
      rc = DeleteCompT(&pCompTBuffer[i]);
      if(rc!= EWDB_RETURN_SUCCESS)
      {
        sprintf(szBuffer,"# COMP END FAILED(DeleteCompT()) return codes (%d,%d)\n", 
                rc, pCompTBuffer[i].idCompT);
        WriteToOutputFile(szBuffer);
      }
    }
  }
  if(rc == EWDB_RETURN_SUCCESS)
    sprintf(szBuffer,"# COMP END IDCOMP = %d\n", pChan->Comp.idComp);
  else
    sprintf(szBuffer,"# COMP END FAILED return codes (%d,%d)\n", rc, pChan->Comp.idComp);

WriteToFile:
  WriteToOutputFile(szBuffer);

  return(rc);
}


int DeleteComp(EWDB_ChannelStruct * pChan)
{
  EWDB_ChannelStruct chan;
  int rc;

  memcpy(&chan,pChan,sizeof(chan));
  chan.tOff = -1.0;

  rc = EndComp(&chan);

  if(rc != EWDB_RETURN_SUCCESS)
  {
    sprintf(szBuffer,"# COMP DELETE FAILED EndComp(%d) return codes (%d,%d)\n", 
            pChan->Comp.idComp, rc, chan.Comp.idComp);
    goto WriteToFile;
  }

  rc = ewdb_api_DeleteComp(pChan->Comp.idComp);
  
  if(rc != EWDB_RETURN_SUCCESS)
  {
    sprintf(szBuffer,"# COMP DELETE FAILED ewdb_api_DeleteComp(%d) return codes (%d,%d)\n", 
            pChan->Comp.idComp, rc, chan.Comp.idComp);
    goto WriteToFile;
  }

    sprintf(szBuffer,"# COMP DELETE IDCOMP = %d\n", pChan->Comp.idComp);

WriteToFile:
  WriteToOutputFile(szBuffer);

  return(rc);
}


int RenameComp(EWDB_ChannelStruct * pChan, EWDB_ChannelStruct * pNewChan)
{
  WriteToOutputFile("Comp(Rename) command is not supported.\n");
  return(-1);
}


/* COMP COMMANDS
COMP     ADD                 ISCO US GHZ  -- "New fake component from DK"
COMP     RENM     1000008645         GHZ      EHZ 01
COMP     END      1000008645                  1240000000 
COMP     DEL      1000008645                  TRUE
************************************************************************/
int HandleCompCommand(char * szCmd, char * szLineBuffer, FILE * fOut)
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
  /* idComp */
  strncpy_ew(szBuffer, &szLineBuffer[STA_FILE_OFFSET_ID], STA_FILE_LEN_ID + 1);
  chan.Comp.idComp = atoi(szBuffer);
  
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
    
    logit("e","Attempting to add Comp(%s.%s.%s.%s).\n  Comment:\"%s\"\n", 
      chan.Comp.Sta, chan.Comp.Comp, chan.Comp.Net, chan.Comp.Loc, 
      szComment);
    rc = AddComp(&chan);
  }
  else
  {
    if(!chan.Comp.idComp)
    {
      rc = ewdb_api_GetidComp(&chan);
      if(rc != EWDB_RETURN_SUCCESS)
      {
        sprintf(szBuffer,"# PREP for COMP (%s)  FAILED ewdb_api_GetidSite(). Return "
                         "code for (%s/%s%s/%s) (%d)\n", 
                szCmd, chan.Comp.Sta, chan.Comp.Comp, 
                chan.Comp.Net, chan.Comp.Loc, rc);
        WriteToOutputFile(szBuffer);
        return(rc);
      }
    }
    if(strcmp(szCmd, "RENM") == 0)
    {
      szTemp = strtok(&szLineBuffer[STA_FILE_OFFSET_VAR], " \t\n");
      if(szTemp)
      {
        strncpy_ew(newchan.Comp.Comp, szTemp, sizeof(newchan.Comp.Comp));
        szTemp = strtok(NULL, " \t\n");
        if(szTemp  && strcmp(szTemp,SPECIAL_NULL_LOC_STRING))
          strncpy_ew(newchan.Comp.Loc, szTemp, sizeof(newchan.Comp.Loc));
      }
      logit("e","Attempting to rename Site(%s.%s) - Comp(%s.%s: %d) to (%s.%s).\n",
        chan.Comp.Sta, chan.Comp.Net, chan.Comp.Comp, chan.Comp.Loc, chan.Comp.idComp,
        newchan.Comp.Comp, newchan.Comp.Loc);
      rc = RenameComp(&chan,&newchan);
    }
    else if(strcmp(szCmd, "END") == 0)
    {
      chan.tOff = atof(&szLineBuffer[STA_FILE_OFFSET_VAR]);
      logit("e","Attempting to end Comp(%s.%s.%s.%s: %d) at %.2f.\n",
        chan.Comp.Sta, chan.Comp.Comp, chan.Comp.Net, chan.Comp.Loc,
        chan.Comp.idComp, chan.tOff);
      rc = EndComp(&chan);
    }
    else if(strcmp(szCmd, "DEL") == 0)
    {
      logit("e","Attempting to delete Comp((%s.%s.%s.%s: %d).\n",
        chan.Comp.Sta, chan.Comp.Comp, chan.Comp.Net, chan.Comp.Loc,
        chan.Comp.idComp);
      rc = DeleteComp(&chan);
    }
    else
    {
      fprintf(fOut,"#ERROR: Unsupported COMP command (%s).\n", szCmd);
      rc = -1;
    }
  }  /* else command=ADD */
  
  if(rc == EWDB_RETURN_SUCCESS)
    GlobalChan.Comp.idComp = chan.Comp.idComp;
  
  return(rc);


}  /* end HandleCompCommand() */

