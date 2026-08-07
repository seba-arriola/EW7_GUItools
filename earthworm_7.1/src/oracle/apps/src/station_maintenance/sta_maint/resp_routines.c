
#include <ewdb_ora_api.h>
#include "sta_maint.h"




int AddResp(EWDB_ChannelStruct * pResp);
int ModResp(EWDB_ChannelStruct * pResp);
int ModCResp(EWDB_ChannelStruct * pResp);
int EndResp(EWDB_ChannelStruct * pResp);
int ModTResp(EWDB_ChannelStruct * pResp);
int DeleteResp(EWDB_ChannelStruct * pResp);


static char szBuffer[256];


int AddResp(EWDB_ChannelStruct * pResp)
{
  
  int rc;
  /* Add ChanCTF either with idCTF or w/Response function */

  rc = ewdb_api_CreateTransformFunction(&pResp->pChanProps->tfsFunc.idCookedTF, 
                                        &pResp->pChanProps->tfsFunc);

  if(rc == EWDB_RETURN_SUCCESS)
  {
    sprintf(szBuffer,"# RESP ADD - CREATED NEW COOKED TF = %d\n", 
      pResp->pChanProps->tfsFunc.idCookedTF);
  }
  else
  {
    sprintf(szBuffer,"# RESP ADD FAILED return codes (%d)\n", rc);
    rc = EWDB_RETURN_FAILURE;
  }

  WriteToOutputFile(szBuffer);
  return(rc);
}  /* end AddResp() */


int ModCResp(EWDB_ChannelStruct * pResp)
{
  /* Mod Description of Response function for given ID */
  int rc;

  rc = ewdb_api_UpdateTransformFunction(&pResp->pChanProps->tfsFunc, 
                                        FALSE /*update PZ */);

  if(rc == EWDB_RETURN_SUCCESS)
  {
    sprintf(szBuffer,"# RESP MODC - MODIFIED COOKED TF = %d\n", 
      pResp->pChanProps->tfsFunc.idCookedTF);
  }
  else
  {
    sprintf(szBuffer,"# RESP MODC FAILED IDCOOKEDTF =%d return codes (%d)\n", 
      pResp->pChanProps->tfsFunc.idCookedTF, rc);
    rc = EWDB_RETURN_FAILURE;
  }

  WriteToOutputFile(szBuffer);
  return(rc);
}  /* end ModCResp() */


int ModResp(EWDB_ChannelStruct * pResp)
{
  int rc;
  /* Mod Response function for given ID */

  rc = ewdb_api_UpdateTransformFunction(&pResp->pChanProps->tfsFunc, 
                                        TRUE /*update PZ */);

  if(rc == EWDB_RETURN_SUCCESS)
  {
    sprintf(szBuffer,"# RESP MOD - MODIFIED COOKED TF = %d\n", 
      pResp->pChanProps->tfsFunc.idCookedTF);
  }
  else
  {
    sprintf(szBuffer,"# RESP MOD FAILED IDCOOKEDTF =%d return codes (%d)\n", 
      pResp->pChanProps->tfsFunc.idCookedTF, rc);
    rc = EWDB_RETURN_FAILURE;
  }

  WriteToOutputFile(szBuffer);
  return(rc);
}  /* end ModResp() */


int ModChanTResp(EWDB_ChannelStruct * pResp)
{
  /* Mod ChanCTF either with idCTF or w/Response function */
  
  int rc;
  EWDB_ChannelStruct chan;


  memset(&chan,0,sizeof(chan));
  chan.idChanT = pResp->idChanT;

  rc = ewdb_api_GetChanParams(&chan);
  if(rc != EWDB_RETURN_SUCCESS)
  {
      sprintf(szBuffer,"# RESP MOD FAILED IDCHANT =%d  ewdb_api_GetChanParams() return codes (%d,%d)\n", 
              pResp->idChanT, rc, chan.idChanT);
      WriteToOutputFile(szBuffer);
      rc = EWDB_RETURN_FAILURE;
      goto WriteOutputFile;
  }


  if(pResp->pChanProps->tfsFunc.idCookedTF <= 0)
  {
    rc = ewdb_api_CreateTransformFunction(&pResp->pChanProps->tfsFunc.idCookedTF, 
                                          &pResp->pChanProps->tfsFunc);

    if(rc == EWDB_RETURN_SUCCESS)
    {
      sprintf(szBuffer,"# RESP MOD - CREATED NEW COOKED TF = %d\n", 
              pResp->pChanProps->tfsFunc.idCookedTF);
      WriteToOutputFile(szBuffer);
    }
    else
    {
      sprintf(szBuffer,"# RESP ADD FAILED IDCHANT =%d return codes (%d)\n", 
      pResp->idChanT, rc);
      WriteToOutputFile(szBuffer);
      rc = EWDB_RETURN_FAILURE;
      goto WriteOutputFile;
    }
  }
  rc=ewdb_api_SetTransformFuncForChanT(pResp->idChanT, pResp->pChanProps->dGain,
                                       pResp->pChanProps->tfsFunc.idCookedTF,
                                       pResp->pChanProps->dSampRate);
  if(rc == EWDB_RETURN_SUCCESS)
    sprintf(szBuffer,"# RESP MOD IDCHANT = %d\n", pResp->idChanT);
  else
    sprintf(szBuffer,"# RESP MOD FAILED IDCHANT =%d return codes (%d)\n", 
            pResp->idChanT, rc);

WriteOutputFile:
  WriteToOutputFile(szBuffer);
  return(rc);
}  /* end ModResp() */


int DeleteResp(EWDB_ChannelStruct * pResp)
{
  /* Delete ChanCTF */
  WriteToOutputFile("Resp(Delete) command is not supported.\n");
  return(-1);
}


#define STRTOK_DELIMETERS " \t\n()"

int ParsePolesAndZeroes(const char * szInputString, EWDB_TransformFunctionStruct *pFunc,
                        char * szStatusBuffer)
{
  char szPZ[256];
  char * szToken;
  int iOddEven = 0;
  
  /* copy InputString over to local storage for strtok parsing */
  strncpy_ew(szPZ,szInputString,sizeof(szPZ));
  
  /* initialize the iNumPoles/iNumZeroes in the pFunc structure */
  pFunc->iNumPoles = pFunc->iNumZeroes = 0;
  
  /* parse the Poles and Zeroes */
  if((!(szToken = strtok(szPZ , STRTOK_DELIMETERS))) || strcmp(szToken,"PZ"))
  {
    sprintf(szStatusBuffer,"#  ERROR parsing \"PZ\" in PZ string.\n");
    goto PZParseFail;
  }
  if((!(szToken = strtok(NULL , STRTOK_DELIMETERS))) || strcmp(szToken,"P"))
  {
    sprintf(szStatusBuffer,"#  ERROR parsing P in PZ string.\n");
    goto PZParseFail;
  }
  /* keep going until we get all the Poles values */
  while((szToken = strtok(NULL, STRTOK_DELIMETERS)) &&
        strcmp(szToken,"Z")
       )
  {
    if(pFunc->iNumPoles == EWDB_MAX_POLES_OR_ZEROES)
      continue;
    
    if(!iOddEven)
    {
      pFunc->Poles[pFunc->iNumPoles].dReal = atof(szToken);
      iOddEven = 1;
    }
    else
    {
      pFunc->Poles[pFunc->iNumPoles].dImag = atof(szToken);
      pFunc->iNumPoles++;
      iOddEven = 0;
    }
  }
  
  if(iOddEven)
  {
    sprintf(szStatusBuffer,"#  ERROR parsing Pole %d in PZ string. Imaginary portion not found!\n",
      pFunc->iNumPoles);
    goto PZParseFail;
  }
  
  if(!szToken)
  {
    sprintf(szStatusBuffer,"#  ERROR parsing Z in PZ string.\n");
    goto PZParseFail;
  }
  
  /* keep going until we get all the Zeroes values */
  while((szToken = strtok(NULL, STRTOK_DELIMETERS)) &&
        strcmp(szToken,"COMMENT"))
  {
    if(pFunc->iNumZeroes == EWDB_MAX_POLES_OR_ZEROES)
      continue;
    
    if(!iOddEven)
    {
      pFunc->Zeroes[pFunc->iNumZeroes].dReal = atof(szToken);
      iOddEven = 1;
    }
    else
    {
      pFunc->Zeroes[pFunc->iNumZeroes].dImag = atof(szToken);
      pFunc->iNumZeroes++;
      iOddEven = 0;
    }
  }
  
  if(iOddEven)
  {
    sprintf(szStatusBuffer,"#  ERROR parsing Zero %d in PZ string. Imaginary portion not found!\n",
      pFunc->iNumZeroes);
    goto PZParseFail;
  }
  
  return(EWDB_RETURN_SUCCESS);

PZParseFail:
  return(EWDB_RETURN_FAILURE);
}  /* end ParsePolesAndZeroes() */





/* RESP COMMANDS
RESP     ADD                                 CMG-3N standard Function PZ P 123.45e+0 122.27e+0 127.57e+0 0 Z 0.0e+0 0.0e+0 0.0e+0 0.0e+0 0.0e+0 0.0e+0
RESP     MOD      $(PREV)                    CMG-3N NSN standard Function PZ P 123.45e+0 125.27e+0 Z 0.0e+0 0.1e+0 0.0e+0 0.0e+0 
CHANT    MOD      $(PREV)                    $(PREV) 40.00 8.13260e+009  CMG-3N - ISCO,BHE,US
CHANT    MODF     1000002251                 40.00 7.26260e+010 PZ P 123.45e+0 122.27e+0 Z 0.0e+0 0.0e+0 0.0e+0 0.0e+0 COMMENT CMG-3N - ISCO,BHE,US
RESP     MODC     $(PREV)                    This is the Response function for an STS-2
************************************************************************/
int HandleRespCommand(char * szCmd, char * szLineBuffer, FILE * fOut)
{
  
  EWDB_ChannelStruct chan;
  char szBuffer[256];
  char * szTemp;
  int rc = EWDB_RETURN_FAILURE;
  int iLen;
  int iRetCode;
  EWDBid idCookedTF;

  memset(&chan, 0 , sizeof(EWDB_ChannelStruct));
  if(!(chan.pChanProps = GlobalChan.pChanProps))
  {
    logit("e","Error in HandleChanTCommand.  GlobalChan.pChanProps is NULL.\n");
    WriteToOutputFile("# Error in HandleChanTCommand.  GlobalChan.pChanProps is NULL.\n");
    return(-1);
  }
  memset(szBuffer, 0, sizeof(szTemp));

  /* attempt to parse the id, sta, net, comp, loc */
  /* idCookedTF */
  strncpy_ew(szBuffer, &szLineBuffer[STA_FILE_OFFSET_ID], STA_FILE_LEN_ID + 1);
  /* check for prev */
  szTemp = strtok(szBuffer," \t\n");
  if(!szTemp)
    chan.pChanProps->tfsFunc.idCookedTF = 0;
  else
  {
    if(!strcmp(szTemp,SPECIAL_USE_PREVIOUS))
    {
      idCookedTF = SPECIAL_IDPREVIOUS;
    }
    else
    {
      idCookedTF = atoi(szBuffer);
    }
  }
  
  /* now handle the command */
  if(strcmp(szCmd, "ADD") == 0)
  {
    /* description */
    /* ensure line is long enough to contain description */
    if(strlen(szLineBuffer) < STA_FILE_OFFSET_VAR)
    {
      sprintf(szBuffer,"#  ERROR parsing line.  Insufficient length(%d).\n",
        strlen(szTemp));
      WriteToOutputFile(szBuffer);
      goto AddRespFail;
    }

    /* check for "PZ" keyword which truncates the description
       and starts the POLES/ZEROES section
     *********************************************************/
    szTemp = strstr(&szLineBuffer[STA_FILE_OFFSET_VAR], " PZ ");
    if(!szTemp)
    {
      sprintf(szBuffer,"#  ERROR searching for PZ token.\n");
      WriteToOutputFile(szBuffer);
      goto AddRespFail;
    }

    /* copy the Description to szCookedTFDesc, but don't overwrite
       the end of the buffer
     *********************************************************/
    if((szTemp - &szLineBuffer[STA_FILE_OFFSET_VAR] + 1) > 
       sizeof(chan.pChanProps->tfsFunc.szCookedTFDesc))
      iLen = sizeof(chan.pChanProps->tfsFunc.szCookedTFDesc);
    else
      iLen = szTemp - &szLineBuffer[STA_FILE_OFFSET_VAR] + 1;
    strncpy_ew(chan.pChanProps->tfsFunc.szCookedTFDesc,
      &szLineBuffer[STA_FILE_OFFSET_VAR], iLen);
    
    /* Poles and Zeroes */
    iRetCode = ParsePolesAndZeroes(szTemp, &chan.pChanProps->tfsFunc, szBuffer);
    if(iRetCode != EWDB_RETURN_SUCCESS)
    {
      logit("e","# ADD RESP - ERROR: No PZ Function parsed.\n");
      WriteToOutputFile("# ADD RESP - ERROR: No PZ Function parsed.\n");
      WriteToOutputFile(szBuffer);
      goto AddRespFail;
    }
    
    logit("e","Attempting to ADD RESP for Chan(%d) Hz = %7.2f Gain: %.4e \n", 
      chan.Comp.idChan,  chan.pChanProps->dSampRate, chan.pChanProps->dGain);
    rc = AddResp(&chan);
    if(chan.pChanProps->tfsFunc.idCookedTF)
    {
      logit("e","PZ Function (%d) created with %d Poles / %d Zeroes.\n",
        chan.pChanProps->tfsFunc.idCookedTF, chan.pChanProps->tfsFunc.iNumPoles,
        chan.pChanProps->tfsFunc.iNumZeroes);
    }
AddRespFail:;
  }  /* end ADD */
  else if(strcmp(szCmd, "MOD") == 0)
  {
    /* check for PREV value for idCookedTF */
    if(idCookedTF == SPECIAL_IDPREVIOUS)
      chan.pChanProps->tfsFunc.idCookedTF = GlobalChan.pChanProps->tfsFunc.idCookedTF;
    else
      chan.pChanProps->tfsFunc.idCookedTF = idCookedTF;


    /* validate idCookedTF */
    if(chan.pChanProps->tfsFunc.idCookedTF <= 0 )
    {
      sprintf(szBuffer,"#  ERROR parsing line.  Invalid idCookedTF(%d).\n",
              chan.pChanProps->tfsFunc.idCookedTF);
      WriteToOutputFile(szBuffer);
      goto ModRespFail;
    }

    /* description */
    /* ensure line is long enough to contain description */
    if(strlen(szLineBuffer) < STA_FILE_OFFSET_VAR)
    {
      sprintf(szBuffer,"#  ERROR parsing line.  Insufficient length(%d).\n",
        strlen(szTemp));
      WriteToOutputFile(szBuffer);
      goto ModRespFail;
    }

    /* check for "PZ" keyword which truncates the description
       and starts the POLES/ZEROES section
     *********************************************************/
    szTemp = strstr(&szLineBuffer[STA_FILE_OFFSET_VAR], " PZ ");
    if(!szTemp)
    {
      sprintf(szBuffer,"#  ERROR searching for PZ token.\n");
      WriteToOutputFile(szBuffer);
      goto ModRespFail;
    }

    /* copy the Description to szCookedTFDesc, but don't overwrite
       the end of the buffer
     *********************************************************/
    if((szTemp - &szLineBuffer[STA_FILE_OFFSET_VAR] + 1) > 
       sizeof(chan.pChanProps->tfsFunc.szCookedTFDesc))
      iLen = sizeof(chan.pChanProps->tfsFunc.szCookedTFDesc);
    else
      iLen = szTemp - &szLineBuffer[STA_FILE_OFFSET_VAR] + 1;
    strncpy_ew(chan.pChanProps->tfsFunc.szCookedTFDesc,
      &szLineBuffer[STA_FILE_OFFSET_VAR], iLen);
    
    /* Poles and Zeroes */
    iRetCode = ParsePolesAndZeroes(szTemp, &chan.pChanProps->tfsFunc, szBuffer);
    if(iRetCode != EWDB_RETURN_SUCCESS)
    {
      logit("e","# MOD RESP - ERROR: No PZ Function parsed.\n");
      WriteToOutputFile("# MOD RESP - ERROR: No PZ Function parsed.\n");
      WriteToOutputFile(szBuffer);
      goto ModRespFail;
    }
    
    logit("e","Attempting to MOD RESP for CookedTF(%d).\n", 
          chan.pChanProps->tfsFunc.idCookedTF);
    rc = ModResp(&chan);
    if(rc == EWDB_RETURN_SUCCESS)
    {
      logit("e","PZ Function (%d) created with %d Poles / %d Zeroes.\n",
        chan.pChanProps->tfsFunc.idCookedTF, chan.pChanProps->tfsFunc.iNumPoles,
        chan.pChanProps->tfsFunc.iNumZeroes);
    }
    else
    {
      /* issue an error message */
      goto ModRespFail;
    }
ModRespFail:;
  }  /* end MOD */
  else if(strcmp(szCmd, "MODC") == 0)
  {
     /* check for PREV value for idCookedTF */
    if(idCookedTF == SPECIAL_IDPREVIOUS)
      chan.pChanProps->tfsFunc.idCookedTF = GlobalChan.pChanProps->tfsFunc.idCookedTF;
    else
      chan.pChanProps->tfsFunc.idCookedTF = idCookedTF;

    /* validate idCookedTF */
    if(chan.pChanProps->tfsFunc.idCookedTF <= 0)
    {
      sprintf(szBuffer,"#  ERROR parsing line.  Invalid idCookedTF(%d).\n",
              chan.pChanProps->tfsFunc.idCookedTF);
      WriteToOutputFile(szBuffer);
      goto ModCRespFail;
    }

    /* copy the Description to szCookedTFDesc, but don't overwrite
       the end of the buffer
     *********************************************************/
    if(strlen(&szLineBuffer[STA_FILE_OFFSET_VAR]) >= 
       sizeof(chan.pChanProps->tfsFunc.szCookedTFDesc))
      iLen = sizeof(chan.pChanProps->tfsFunc.szCookedTFDesc);
    else
      iLen = strlen(&szLineBuffer[STA_FILE_OFFSET_VAR]) + 1;
    strncpy_ew(chan.pChanProps->tfsFunc.szCookedTFDesc,
      &szLineBuffer[STA_FILE_OFFSET_VAR], iLen);
    

    logit("e","Attempting to Mod Response(%d) Comment.\n",
      chan.pChanProps->tfsFunc.idCookedTF);
    rc = ModCResp(&chan);
ModCRespFail:;
  }
  else if(strcmp(szCmd, "DEL") == 0)
  {
    logit("e","Attempting to delete ChanT Response(%d).\n",
      chan.pChanProps->idChanT);
    rc = DeleteResp(&chan);
  }
  else
  {
    sprintf(szBuffer,"#ERROR: Unsupported RESP command (%s).\n", szCmd);
    WriteToOutputFile(szBuffer);
    rc = EWDB_RETURN_FAILURE;
  }


  if(rc == EWDB_RETURN_SUCCESS)
  {
    GlobalChan.pChanProps->tfsFunc.idCookedTF = chan.pChanProps->tfsFunc.idCookedTF;
  }

  return(rc);
}  /* end HandleRespCommand() */


