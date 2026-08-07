
#include <ewdb_ora_api.h>
#include "sta_maint.h"





int ModChanT(EWDB_ChannelStruct * pChan);
int ModTChanT(EWDB_ChannelStruct * pChan);
int DeleteChanT(EWDB_ChannelStruct * pChan);
int EndChanT(EWDB_ChannelStruct * pChan);


static char szBuffer[256];

int EndChanT(EWDB_ChannelStruct * pChan)
{
  int rc;
  int iRetCode;
  EWDB_ChannelStruct chan;

  chan.idChanT = pChan->idChanT;
  rc = ewdb_api_GetChanParams(&chan);
  if(rc != EWDB_RETURN_SUCCESS)
  {
    sprintf(szBuffer,"# CHANTT END FAILED (ewdb_api_GetChanTParams(%d)) return "
                     "codes (%d,%d)\n", pChan->idChanT, rc, chan.idChanT);
    goto WriteOutputFile;
  }

  if(chan.tOff < pChan->tOn)
  {
    /* you can't use END command to extend the chant Time */
    sprintf(szBuffer,"# CHANT END FAILED for ChanT(%d) "
                     "(new endtime(%.2f) > old endtime(%.2f)).",
            pChan->idChanT, pChan->tOn, chan.tOff);
    goto WriteOutputFile;
  }
  chan.tOff = pChan->tOn;

  rc = ewdb_api_UpdateChanTTime(&iRetCode, &chan);
  if(rc != EWDB_RETURN_SUCCESS)
  {
    sprintf(szBuffer,"# CHANT END FAILED (ewdb_api_UpdateChanTTime(%d)) return "
                     "codes (%d,%d)\n", pChan->idChanT, rc, iRetCode);
    goto WriteOutputFile;
  }


  sprintf(szBuffer,"# CHANT END idChanT =%d  tOn=%d  tOff=%d.\n", 
          pChan->idChanT, chan.tOn, chan.tOff);

WriteOutputFile:
  WriteToOutputFile(szBuffer);
  return(rc);
}  /* end EndTChanT() */


/*   SPLIT CHANT WAS NEVER FINISHED AND HAS BEEN REMOVED.
     IT WAS NEVER IN USE   DK 04092004 *
int SplitChanT(EWDB_ChannelStruct * pChan)
{
  int rc;
  EWDB_ChannelStruct chan;

  chan.idChanT = pChan->idChanT;
  rc = ewdb_api_GetChanParams(&chan);
  if(rc != EWDB_RETURN_SUCCESS)
  {
    sprintf(szBuffer,"# CHANT SPLIT FAILED (ewdb_api_GetChanTParams(%d)) return "
                     "codes (%d,%d)\n", pChan->idChanT, rc, chan.idChanT);
    goto WriteOutputFile;
  }

  if(chan.tOff < pChan->tOn)
  {
    / * you can't use END command to extend the chant Time * /
    sprintf(szBuffer,"# CHANT SPLIT FAILED for ChanT(%d) "
                     "(split time(%.2f) > endtime(%.2f)).",
            pChan->idChanT, pChan->tOn, chan.tOff);
    goto WriteOutputFile;
  }
  chan.tOff = pChan->tOn;

  rc = ewdb_api_SplitChanT(&chan,&chan.idChanT);
  if(rc != EWDB_RETURN_SUCCESS)
  {
    sprintf(szBuffer,"# CHANT SPLIT FAILED (ewdb_api_SplitChanT(%d,%.2f)) return "
                     "codes (%d,%d)\n", 
            pChan->idChanT, pChan->tOn, rc, chan.idChanT);
    goto WriteOutputFile;
  }


  sprintf(szBuffer,"# CHANT SPLIT: idChanT(%d) New idChanT =%d  tSplit=%d.\n", 
          pChan->idChanT, chan.idChanT, chan.tOn);

WriteOutputFile:
  WriteToOutputFile(szBuffer);
  return(rc);
}  / * end EndTChanT() * /
*****************************************************/

int DeleteChanT(EWDB_ChannelStruct * pChan)
{
  int rc;
  rc = ewdb_api_DeleteChanT(pChan->idChanT);
  if(rc != EWDB_RETURN_SUCCESS)
  {
    sprintf(szBuffer,"# CHANT DELETE FAILED (ewdb_api_DeleteChanT(%d)) return "
                     "code(%d)\n", pChan->idChanT, rc);
    goto WriteOutputFile;
  }

  sprintf(szBuffer,"# CHANT DELETE idChanT =%d.\n", pChan->idChanT);

WriteOutputFile:
  WriteToOutputFile(szBuffer);
  return(rc);

}  /* end DeleteChanT() */


int ModFChanT(EWDB_ChannelStruct * pChan)
{
  int rc;
  rc = AddResp(pChan);
  if(rc == EWDB_RETURN_SUCCESS)
    rc=ModChanT(pChan);

  return(rc);
}  /* end ModFChanT */

int ModTChanT(EWDB_ChannelStruct * pChan)
{
  int rc;
  int iRetCode;
  EWDB_ChannelStruct chan;

  chan.idChanT = pChan->idChanT;
  rc = ewdb_api_GetChanParams(&chan);
  if(rc != EWDB_RETURN_SUCCESS)
  {
    sprintf(szBuffer,"# CHANT MODT FAILED (ewdb_api_GetChanTParams(%d)) return "
                     "codes (%d,%d)\n", pChan->idChanT, rc, chan.idChanT);
    goto WriteOutputFile;
  }

  chan.tOn = pChan->tOn;
  chan.tOff = pChan->tOff;

  rc = ewdb_api_UpdateChanTTime(&iRetCode, &chan);
  if(rc != EWDB_RETURN_SUCCESS)
  {
    sprintf(szBuffer,"# CHANT MODT FAILED (ewdb_api_UpdateChanTTime(%d)) return "
                     "codes (%d,%d)\n", pChan->idChanT, rc, iRetCode);
  }

  
  sprintf(szBuffer,"# CHANT MODT idChanT =%d  tOn=%.0f  tOff=%.0f.\n", 
          pChan->idChanT, chan.tOn, chan.tOff);

WriteOutputFile:
  WriteToOutputFile(szBuffer);
  return(rc);
}  /* end ModTChanT() */


int ModChanT(EWDB_ChannelStruct * pChan)
{
  int rc;

  /* Create/Update a ChanCTF record */
  rc=ewdb_api_SetTransformFuncForChanT(pChan->idChanT, pChan->pChanProps->dGain,
    pChan->pChanProps->tfsFunc.idCookedTF,
    pChan->pChanProps->dSampRate);
  if(rc == EWDB_RETURN_SUCCESS)
  {
    sprintf(szBuffer,"# CHANT MOD SUCCESSFUL idChanT =%d  dGain=%.2e dSampRate=%.2f, idCookedTF=%d.\n", 
      pChan->idChanT, pChan->pChanProps->dGain, pChan->pChanProps->dSampRate,
      pChan->pChanProps->tfsFunc.idCookedTF);
    WriteToOutputFile(szBuffer);
  }
  else
  {
    sprintf(szBuffer,"# CHANT MOD FAILED (ewdb_api_SetTransformFuncForChanT(%d)) "
                     " return codes (%d)\n", 
            pChan->idChanT, rc);
    WriteToOutputFile(szBuffer);
  }
  return(rc);
}  /* end ModChanT() */


/* CHANT COMMANDS
CHANT    MODT     1000002251                1100000000 1140000000 
CHANT    SPLIT    1000002251                1129000000
CHANT    END      1000002251                1139000000
CHANT    DEL      1000002251                
************************************************************************/


int HandleChanTCommand(char * szCmd, char * szLineBuffer, FILE * fOut)
{
  
  EWDB_ChannelStruct chan;
  char szBuffer[256];
  char * szTemp;
  char szComment[256];
  int rc = EWDB_RETURN_FAILURE;

  memset(&chan, 0 , sizeof(EWDB_ChannelStruct));
  if(!(chan.pChanProps = GlobalChan.pChanProps))
  {
    logit("e","Error in HandleChanTCommand.  GlobalChan.pChanProps is NULL.\n");
    WriteToOutputFile("# Error in HandleChanTCommand.  GlobalChan.pChanProps is NULL.\n");
    return(-1);
  }
  memset(szBuffer, 0, sizeof(szTemp));
  
 /* attempt to parse the id, sta, net, comp, loc */
  /* idChanT */
  strncpy_ew(szBuffer, &szLineBuffer[STA_FILE_OFFSET_ID], STA_FILE_LEN_ID + 1);
  chan.Comp.idChan = atoi(szBuffer);

  /* check for prev */
  szTemp = strtok(szBuffer," \t\n");
  if(!szTemp)
    chan.idChanT = 0;
  else
  {
    if(!strcmp(szTemp,SPECIAL_USE_PREVIOUS))
    {
      chan.idChanT = SPECIAL_IDPREVIOUS;
    }
    else
    {
      chan.idChanT = atoi(szBuffer);
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
  
    if(chan.idChanT == SPECIAL_IDPREVIOUS)
      chan.idChanT = GlobalChan.idChanT;

  /* now handle the command */
  if(strcmp(szCmd, "END") == 0)
  {
    chan.tOff = atof(&szLineBuffer[STA_FILE_OFFSET_VAR]);
    logit("e","Attempting to end Chan(%d) at %.2f.  Assocd w/Chan (%s.%s.%s.%s).\n",
      chan.Comp.idChan, chan.tOff,
      chan.Comp.Sta, chan.Comp.Comp, chan.Comp.Net, chan.Comp.Loc);
    EndChanT(&chan);
  }    /* the id field value here is idChanT not idChan */
  if(strcmp(szCmd, "SPLIT") == 0)
  {
    chan.tOff = atof(&szLineBuffer[STA_FILE_OFFSET_VAR]);
    logit("e","Attempting to end Chan(%d) at %.2f.  Assocd w/Chan (%s.%s.%s.%s).\n",
      chan.Comp.idChan, chan.tOff,
      chan.Comp.Sta, chan.Comp.Comp, chan.Comp.Net, chan.Comp.Loc);
    EndChanT(&chan);
  }    /* the id field value here is idChanT not idChan */
  else if(strcmp(szCmd, "DEL") == 0)
  {
    logit("e","Attempting to delete Chan(%d) assoc w/Chan(%s.%s.%s.%s).\n",
      chan.Comp.idChan,
      chan.Comp.Sta, chan.Comp.Comp, chan.Comp.Net, chan.Comp.Loc
      );
    DeleteChanT(&chan);
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
    logit("e","Attempting to mod time for ChanT(%s.%s%s.%s: %d) New On-Off(%.0f - %.0f)\n",
      chan.Comp.Sta, chan.Comp.Comp, chan.Comp.Sta, chan.Comp.Loc,
      chan.idChanT, chan.tOn, chan.tOff);
    ModTChanT(&chan);
  }
  else if(strcmp(szCmd, "MOD") == 0)
  {
    /* initialize the  parameters on the line */
    chan.pChanProps->dGain = 0.0;
    chan.pChanProps->dSampRate = 0.0;
    szComment[0] = 0x00;

    szTemp = strtok(&szLineBuffer[STA_FILE_OFFSET_VAR], " \t");
    if(szTemp)
    {
      /* check for prev */
      if(!strcmp(szTemp,SPECIAL_USE_PREVIOUS))
      {
        chan.pChanProps->tfsFunc.idCookedTF = GlobalChan.pChanProps->tfsFunc.idCookedTF;
      }
      else
      {
        chan.pChanProps->tfsFunc.idCookedTF = atoi(szTemp);
      }
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
    logit("e","Attempting to MOD ChanT(%d) with (%d, %.2f %.4e): \n"
      "  Comment:\"%s\"\n", 
      chan.idChanT, chan.pChanProps->tfsFunc.idCookedTF,
      chan.pChanProps->dSampRate, chan.pChanProps->dGain, szComment);
    rc = ModChanT(&chan);
  }
  else if(strcmp(szCmd, "MODF") == 0)
  {
    /* initialize the  parameters on the line */
    chan.pChanProps->dGain = 0.0;
    chan.pChanProps->dSampRate = 0.0;
    szComment[0] = 0x00;

    szTemp = strtok(&szLineBuffer[STA_FILE_OFFSET_VAR], " \t");
    if(szTemp)
    {
      chan.pChanProps->dSampRate = atof(szTemp);
      szTemp = strtok(NULL, " \t");
      if(szTemp)
      {
        chan.pChanProps->dGain = atof(szTemp);
        /* parse poles and zeroes */
        rc=ParsePolesAndZeroes(szTemp + strlen(szTemp) + 1, 
                               &chan.pChanProps->tfsFunc,
                               szBuffer);
        if(rc == EWDB_RETURN_SUCCESS)
        {
          szTemp = strstr(szTemp + strlen(szTemp) + 1, " COMMENT ");
          if(szTemp)
          {
            strncpy_ew(szComment, szTemp + strlen(" COMMENT "), sizeof(szComment));
          }
        }
        else
        {
          logit("e","# CHANT MODF - ERROR: No PZ Function parsed.\n");
          WriteToOutputFile("# CHANT MODF - ERROR: No PZ Function parsed.\n");
          WriteToOutputFile(szBuffer);
          goto ModFFail;
        }
      }
      else
      {
        sprintf(szBuffer,"# ERROR:  Failed to parse Gain\n");
        WriteToOutputFile(szBuffer);
        goto ModFFail;
      }
    }
    else
    {
      sprintf(szBuffer,"# ERROR:  Failed to parse Sample Rate\n");
      WriteToOutputFile(szBuffer);
      goto ModFFail;
    }
    logit("e","Attempting to MOD ChanT(%d) with (%d, %.2f %.4e): \n"
      "  Comment:\"%s\"\n", 
      chan.idChanT, chan.pChanProps->tfsFunc.idCookedTF,
      chan.pChanProps->dSampRate, chan.pChanProps->dGain, szComment);
    rc = ModFChanT(&chan);
ModFFail:;
  }   /* end MODF */
  else
  {
    sprintf(szBuffer,"#ERROR: Unsupported CHANT command (%s).\n", szCmd);
    WriteToOutputFile(szBuffer);
    rc = EWDB_RETURN_FAILURE;
  }

  if(rc == EWDB_RETURN_SUCCESS)
  {
    GlobalChan.idChanT = chan.idChanT;
  }

  return(rc);
}  /* end HandleChanTCommand() */



