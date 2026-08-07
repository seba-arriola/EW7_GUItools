/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: sta_export.c,v 1.1 2003/12/13 02:30:09 davidk Exp $
 *
 *    Revision history:
 *     $Log: sta_export.c,v $
 *     Revision 1.1  2003/12/13 02:30:09  davidk
 *     Initial revision
 *
 *     Revision 1.3  2003/11/11 17:56:59  davidk
 *     Fixed some bugs in the password handling.
 *     NOTE: Current code writes passport to screen/logfile for testing.
 *
 *
 ************************************************************************/



#include <ewdb_ora_api.h>


#define PM_DISPLAY_UNDEFINED 0
#define PM_DISPLAY_SITE_INFO 1
#define PM_DISPLAY_COMP_INFO 2
#define PM_DISPLAY_CHAN_INFO 4

#define RUN_QUERY_UNDEFINED 0
#define RUN_QUERY_ALL_SITET 1
#define RUN_QUERY_ALL_COMPT 2
#define RUN_QUERY_ALL_CHAN_W_RESPONSE 3
#define RUN_QUERY_ALL_CHAN_WO_RESPONSE 4

/* define constants for argv positions and argc */
#define ARGC_MIN          4
#define ARGV_PROG_NAME    0
#define ARGV_DB_USER      1
#define ARGV_DB_SID       2
#define ARGV_OUTFILE      3

extern int errno;

/* "sta_export <DB_USER> <DB_SID> <OUTPUT_FILENAME> " */
static char szUsageMessage[] =
    "USAGE: sta_export <DB_USER> <DB_SID> <OUTPUT_FILENAME>\n"
    "\n"
    "\n"
  ;

static  EWDB_ChannelStruct csFilterChannel;
static  EWDB_ChannelStruct csFilterMaxChannel;
static  int iCriteria = 0;

static  char * szDBUser = "ewdb_main";
static  char   szDBPwd[40]  = "main";
static  char * szDBSID  = "eqsutah.usgs";
static  char * szOutFile = 0x00;
static  char * szProgramName = 0x00;
static  FILE * fOut = 0x00;

static char * pBuffer;
static  int BUFFERSIZE= 5000000;
static  int iBufferLen;

int PrintUsageMessage();
int ParseCommandLine(int argc, char ** argv);
int CompareSCNLs(EWDB_ChannelStruct * pcs1, EWDB_ChannelStruct * pcs2);
int CompareSites(EWDB_ChannelStruct * pcs1, EWDB_ChannelStruct * pcs2);
int CompareSiteTs(EWDB_ChannelStruct * pcs1, EWDB_ChannelStruct * pcs2);
int CompareChans(EWDB_ChannelStruct * pcs1, EWDB_ChannelStruct * pcs2);
int CompareCompTs(EWDB_ChannelStruct * pcs1, EWDB_ChannelStruct * pcs2);

int PrintSiteLine(EWDB_ChannelStruct * pChan, int bPrintSiteTInfo);
int PrintSiteTLine(EWDB_ChannelStruct * pChan);
int PrintSCNLLine(EWDB_ChannelStruct * pChan);
int PrintChanLine(EWDB_ChannelStruct * pChan);
int PrintChanTLine(EWDB_ChannelStruct * pChan, int bPrintResponse);
int PrintCompTLine(EWDB_ChannelStruct * pChan);

int run_query(int iRunQuery, int bShowComp);
int init(int argc, char ** argv);

int main(int argc, char ** argv)
{

  int iRetCode;
  
  iRetCode = init(argc,argv);
  if(iRetCode == EWDB_RETURN_WARNING)
  {
    logit("t","init() returned warning!\n");
  }
  else if(iRetCode == EWDB_RETURN_FAILURE)
  {
    logit("t","init() returned error! Returning!\n");
    return(-1);
  }
  
  iRetCode = run_query( RUN_QUERY_ALL_SITET, FALSE);
  if(iRetCode == EWDB_RETURN_WARNING)
  {
    logit("t","run_query(RUN_QUERY_ALL_SITET) returned warning!\n");
  }
  else if(iRetCode == EWDB_RETURN_FAILURE)
  {
    logit("t","run_query(RUN_QUERY_ALL_SITET) returned error! Returning!\n");
    return(-1);
  }
  
  iRetCode = run_query( RUN_QUERY_ALL_CHAN_W_RESPONSE, TRUE);
  if(iRetCode == EWDB_RETURN_WARNING)
  {
    logit("t","run_query(RUN_QUERY_ALL_CHAN_W_RESPONSE) returned warning!\n");
  }
  else if(iRetCode == EWDB_RETURN_FAILURE)
  {
    logit("t","run_query(RUN_QUERY_ALL_CHAN_W_RESPONSE) returned error! Returning!\n");
    return(-1);
  }
  
  
  return(0);
}  /* end main() */


int init(int argc, char ** argv)
{

  int iRetCode;

  szProgramName = argv[ARGV_PROG_NAME];

  logit_init(szProgramName,0,1024,1);

  if(ParseCommandLine(argc,argv) != 0)
  {
    PrintUsageMessage();
    return(-1);
  }

  printf("Password:  ");
  if(!fgets(szDBPwd,sizeof(szDBPwd),stdin))
	  logit("e","Could not obtain password from stdin!\n");
  szDBPwd[strlen(szDBPwd) - 1] = 0x00;  /* remove '\n' char */
  pBuffer = calloc(1,BUFFERSIZE);

  iRetCode = ewdb_api_Init(szDBUser,szDBPwd,szDBSID);
  if(iRetCode != EWDB_RETURN_SUCCESS)
  {
    logit("e","ewdb_api_Init() failed.  Returning!\n");
    return(EWDB_RETURN_FAILURE);
  }
  
  /* open output file for writing */
  fOut = fopen(szOutFile,"w");
  
  if(!fOut)
  {
    logit("e","%s:  Unable to open output file(%s).  errno is %d.\n",
      szProgramName, szOutFile, errno);
    return(EWDB_RETURN_FAILURE);
  }
  fclose(fOut);

  return(EWDB_RETURN_SUCCESS);

}  /* end init() */


int run_query(int iRunQuery, int bShowComp)
{
  EWDB_ChannelStruct * pChanBuffer, * pPrevChannel;
  int i;
  int iNumRecordsFound, iNumRecordsRetrieved;
  int iRetCode;

  if(iRunQuery == RUN_QUERY_ALL_CHAN_W_RESPONSE)
  {
    pChanBuffer = (EWDB_ChannelStruct *) pBuffer;
    iBufferLen = BUFFERSIZE / sizeof(EWDB_ChannelStruct);
    iRetCode = ewdb_api_GetSelectedStations_w_Response(
                                            pChanBuffer, iBufferLen, &csFilterChannel, &csFilterMaxChannel,
                                            iCriteria, &iNumRecordsFound, &iNumRecordsRetrieved);
  }
  else if(iRunQuery == RUN_QUERY_ALL_SITET)
  {
    pChanBuffer = (EWDB_ChannelStruct *) pBuffer;
    iBufferLen = BUFFERSIZE / sizeof(EWDB_ChannelStruct);
    iRetCode = ewdb_api_GetSelectedSites(
                                            pChanBuffer, iBufferLen, &csFilterChannel, &csFilterMaxChannel,
                                            iCriteria, &iNumRecordsFound, &iNumRecordsRetrieved);
  }

  if(iRetCode == EWDB_RETURN_FAILURE)
  {
    logit("e","ERROR: Unable to retrieve requested data from the DB.  Please\n"
              "see logfile for details.\n");
  }
  else
  {
    if(iRetCode == EWDB_RETURN_WARNING  && iNumRecordsFound > iNumRecordsRetrieved)
    {
      logit("e","WARNING: Only able to retrieve %d of %d records available.  Extend the\n"
                "BufferSize in the config file or narrow your search.\n",
            iNumRecordsRetrieved, iNumRecordsFound);
    }
    pPrevChannel = &csFilterChannel;
    memset(pPrevChannel,0, sizeof(csFilterChannel));

    /* open output file for writing */
    fOut = fopen(szOutFile,"a");

    if(!fOut)
    {
      logit("e","%s:  Unable to open output file(%s).  errno is %d.\n",
            szProgramName, szOutFile, errno);
      return(EWDB_RETURN_FAILURE);
    }

    /* print out records */
    for(i=0; i < iNumRecordsRetrieved; i++)
    {
      if(iRunQuery == RUN_QUERY_ALL_SITET && !bShowComp)
      {
        /* we're displaying only site-level info */
        if(CompareSites(pPrevChannel, &(pChanBuffer[i])))
        {
          /* print Site Line */
          PrintSiteLine(&(pChanBuffer[i]), FALSE);
        }
        if(CompareSiteTs(pPrevChannel, &(pChanBuffer[i])))
        {
          /* print SiteT Line */
          PrintSiteTLine(&(pChanBuffer[i]));
        }

      }
      else
      {
        /* we're displaying info for multiple levels */
        if(CompareSites(pPrevChannel, &(pChanBuffer[i])))
        {
          /* print Site Line */
          PrintSiteLine(&(pChanBuffer[i]), TRUE);
        }
        if(CompareSCNLs(pPrevChannel, &(pChanBuffer[i])))
        {
          /* print SCNL Line */
          PrintSCNLLine(&(pChanBuffer[i]));
        }
        if(CompareCompTs(pPrevChannel, &(pChanBuffer[i])))
        {
          /* print SCNL Line */
          PrintCompTLine(&(pChanBuffer[i]));
        }
        if(CompareChans(pPrevChannel, &(pChanBuffer[i])))
        {
          /* print Chan Line */
          if(pChanBuffer[i].Comp.idChan != 0)
            PrintChanLine(&(pChanBuffer[i]));
        }
        
        /* print ChanT Line */
        PrintChanTLine(&(pChanBuffer[i]), TRUE /*bShowResponse*/);
      }
        
      /* update the Previous Channel (pPrevChannel) */
      pPrevChannel = &(pChanBuffer[i]);
    }  /* end for(iNumRecordsRetrieved) */

    /* close the output file */
    fclose(fOut);

    /* end func: RUN QUERY */

  }
  return(EWDB_RETURN_SUCCESS);
}  /* end main() */


int CompareSites(EWDB_ChannelStruct * pcs1, EWDB_ChannelStruct * pcs2)
{
  int rc;
  if(!(pcs1 && pcs2))
  {
    /* NULL pointer  sugah frugah JR@#L@#T!!! */
    return(0);
  }
  /* check the sta names */
  if(rc=strcmp(pcs1->Comp.Sta, pcs2->Comp.Sta))
     return(rc);

  /* then check the net names */
  if(rc=strcmp(pcs1->Comp.Net, pcs2->Comp.Net))
     return(rc);

  return(0);  /* they match */

}

int CompareSiteTs(EWDB_ChannelStruct * pcs1, EWDB_ChannelStruct * pcs2)
{
  int rc;

  
  if(!(pcs1 && pcs2))
  {
    /* NULL pointer  sugah frugah JR@#L@#T!!! */
    return(0);
  }

  rc = CompareSites(pcs1,pcs2);
  if(rc)
    return(rc);

  if(pcs1->tOn < pcs2->tOn)
    return(-1);
  else if(pcs1->tOn > pcs2->tOn)
    return(1);
  else
    return(0);  /* they match */

}

int CompareSCNLs(EWDB_ChannelStruct * pcs1, EWDB_ChannelStruct * pcs2)
{
  int rc;
  if(!(pcs1 && pcs2))
  {
    /* NULL pointer  sugah frugah JR@#L@#T!!! */
    return(0);
  }
  /* check the sta names */
  if(rc=strcmp(pcs1->Comp.Sta, pcs2->Comp.Sta))
     return(rc);

  /* then check the net names */
  if(rc=strcmp(pcs1->Comp.Net, pcs2->Comp.Net))
     return(rc);

  /* check the comp names */
  if(rc=strcmp(pcs1->Comp.Comp, pcs2->Comp.Comp))
     return(rc);

  /* check the loc names */
  if(rc=strcmp(pcs1->Comp.Loc, pcs2->Comp.Loc))
     return(rc);


  return(0);  /* they match */

}

int CompareCompTs(EWDB_ChannelStruct * pcs1, EWDB_ChannelStruct * pcs2)
{
  if(!(pcs1 && pcs2))
  {
    /* NULL pointer  sugah frugah JR@#L@#T!!! */
    return(0);
  }
  /* check the idCompT values */
  if(pcs1->idCompT < pcs2->idCompT)
    return(-1);
  if(pcs1->idCompT > pcs2->idCompT)
    return(1);

  return(0);  /* they match */
}


int CompareChans(EWDB_ChannelStruct * pcs1, EWDB_ChannelStruct * pcs2)
{
  if(!(pcs1 && pcs2))
  {
    /* NULL pointer  sugah frugah JR@#L@#T!!! */
    return(0);
  }
  /* check the sta names */
  if(pcs1->Comp.idChan < pcs2->Comp.idChan)
    return(-1);
  if(pcs1->Comp.idChan > pcs2->Comp.idChan)
    return(1);

  return(0);  /* they match */
}


int ParseCommandLine(int argc, char ** argv)
{
  
  if(argc < ARGC_MIN)
  {
    logit("e","%s:ParseCommandLine() ERROR: Insufficient number of command line arguments.\n",
          szProgramName);
    return(-1);
  }
  szDBUser = argv[ARGV_DB_USER];
  szDBSID  = argv[ARGV_DB_SID];

  szOutFile = argv[ARGV_OUTFILE];

  memset(&csFilterChannel,0,sizeof(csFilterChannel));
  memset(&csFilterMaxChannel,0,sizeof(csFilterMaxChannel));

return(EWDB_RETURN_SUCCESS);


}  /* end ParseCommandLine() */


int PrintUsageMessage()
{
  printf(szUsageMessage);
  return(0);
}  /* end PrintUsageMessage() */



int PrintSiteLine(EWDB_ChannelStruct * pChan, int bPrintSiteTInfo)
{
  if(bPrintSiteTInfo)
    fprintf(fOut, "\n"
           "SITE    |     |%13d %-4s %-2s        %7.4f %9.4f %7.1f          (%12.2f - %12.2f)\n",
           pChan->idSite, pChan->Comp.Sta, pChan->Comp.Net, 
           pChan->Comp.Lat, pChan->Comp.Lon, pChan->Comp.Elev,
           pChan->tOn, pChan->tOff);
  else
    fprintf(fOut, "\n"
           "SITE    |     |%13d %-4s %-2s\n",
           pChan->idSite, pChan->Comp.Sta, pChan->Comp.Net);

  return(0);
}  /* end PrintSiteLine() */

int PrintSiteTLine(EWDB_ChannelStruct * pChan)
{
  fprintf(fOut, "SITET   |     |%13d                %7.4f %9.4f %7.1f          (%12.2f - %12.2f)\n",
         pChan->idSiteT, 
         pChan->Comp.Lat, pChan->Comp.Lon, pChan->Comp.Elev,
         pChan->tOn, pChan->tOff);

  return(0);
}  /* end PrintSiteLine() */


int PrintSCNLLine(EWDB_ChannelStruct * pChan)
{
  fprintf(fOut, "COMP    |     |%13d         %-4s %2s          \n",
         pChan->Comp.idComp, pChan->Comp.Comp, pChan->Comp.Loc);
  return(0);
}  /* end PrintSCNLLine() */

int PrintCompTLine(EWDB_ChannelStruct * pChan)
{
  fprintf(fOut, "COMPT   |     |%13d                %7.4f %9.4f %7.1f  %3.0f %3.0f (%12.2f - %12.2f)\n",
         pChan->idCompT, pChan->Comp.Lat, pChan->Comp.Lon, pChan->Comp.Elev,
         pChan->Comp.Azm, pChan->Comp.Dip,
         pChan->tOnCompT, pChan->tOffCompT);
  return(0);
}  /* end PrintSCNLLine() */

int PrintChanLine(EWDB_ChannelStruct * pChan)
{
  fprintf(fOut, "CHAN    |     |%13d                   \n", pChan->Comp.idChan);
  return(0);
}

int PrintChanTLine(EWDB_ChannelStruct * pChan, int bPrintResponse)
{
  static char szPoles[1024];
  static char szZeroes[1024];
  int i;
  static char szNum[20];

  if(pChan->pChanProps)
  {
    fprintf(fOut, "CHANT   |     |%13d               %6.2f %.5e                 (%12.2f - %12.2f) %s\n",
           pChan->pChanProps->idChanT, pChan->pChanProps->dSampRate, pChan->pChanProps->dGain,
           pChan->tOn, pChan->tOff, pChan->pChanProps->tfsFunc.szCookedTFDesc
           );
    if(bPrintResponse)
    {
      if( pChan->pChanProps->tfsFunc.iNumPoles || pChan->pChanProps->tfsFunc.iNumZeroes)
      {
        szPoles[0] = 0x00;
        szZeroes[0] = 0x00;
        for(i=0; i < pChan->pChanProps->tfsFunc.iNumPoles; i++)
        {
          sprintf(szNum, "%f %f ", pChan->pChanProps->tfsFunc.Poles[i].dReal,
                  pChan->pChanProps->tfsFunc.Poles[i].dImag);
          strcat(szPoles, szNum);
        }

        for(i=0; i < pChan->pChanProps->tfsFunc.iNumZeroes; i++)
        {
          sprintf(szNum, "%f %f ", pChan->pChanProps->tfsFunc.Zeroes[i].dReal,
                  pChan->pChanProps->tfsFunc.Zeroes[i].dImag);
          strcat(szZeroes, szNum);
        }

        fprintf(fOut, "RESP    |     |%13d               %s PZ P %s Z %s\n",
               pChan->pChanProps->tfsFunc.idCookedTF,
               pChan->pChanProps->tfsFunc.szCookedTFDesc,
               szPoles, szZeroes
              );
      }  /* end if Poles or Zeroes found */
    }
  }  /* end if pchan->pChanProps */
  else
  {
    if(pChan->idChanT)
    {
      fprintf(fOut, "CHANT   |     |%13d               %6.2f %.5e                 (%12.2f - %12.2f) %s\n",
        pChan->idChanT, 0.0, 0.0,
        pChan->tOn, pChan->tOff, ""
        );
    }
  }  /* end else (pchan->pChanProps) */

    return(0);
}

