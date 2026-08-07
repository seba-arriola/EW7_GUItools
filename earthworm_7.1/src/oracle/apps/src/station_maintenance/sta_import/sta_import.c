/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: sta_import.c,v 1.1 2003/12/13 02:30:46 davidk Exp $
 *
 *    Revision history:
 *     $Log: sta_import.c,v $
 *     Revision 1.1  2003/12/13 02:30:46  davidk
 *     Initial revision
 *
 *     Revision 1.3  2003/11/11 17:56:59  davidk
 *     Fixed some bugs in the password handling.
 *     NOTE: Current code writes passport to screen/logfile for testing.
 *
 *
 ************************************************************************/



#include <ewdb_ora_api.h>
#include <string.h>

#define STRTOK_DELIMETERS " \t|\n"

#define PM_DISPLAY_UNDEFINED 0
#define PM_DISPLAY_SITE_INFO 1
#define PM_DISPLAY_COMP_INFO 2
#define PM_DISPLAY_CHAN_INFO 4

#define RUN_QUERY_UNDEFINED 0
#define RUN_QUERY_ALL_SITET 1
#define RUN_QUERY_ALL_COMPT 2
#define RUN_QUERY_ALL_CHAN_W_RESPONSE 3
#define RUN_QUERY_ALL_CHAN_WO_RESPONSE 4

#define DB_ERROR_COMP_ALREADY_EXISTS -2

#define RESP_COMMENT_OFFSET 43


/* define constants for argv positions and argc */
#define ARGC_MIN          5
#define ARGV_PROG_NAME    0
#define ARGV_DB_USER      1
#define ARGV_DB_SID       2
#define ARGV_INFILE       3
#define ARGV_OUTFILE      4

extern int errno;

/* "stadisplay SITE SHOWCOMP RESP SN * UU" */
static char szUsageMessage[] =
    "USAGE: sta_import <DB_USER> <DB_SID> <INPUT_FILENAME> <OUTPUT_FILENAME>\n"
    "\n"
	"  <DB_USER>          Database username                                 \n"
	"  <DB_SID>           Database SID descriptor                           \n"
	"  <INPUT_FILENAME>   Name of file from which station data will be read \n"
	"                      and used to build database                       \n"
	"  <OUTPUT_FILENAME>  Name of file where actions/results will be logged \n"
    "\n"
  ;

static  int iProgramMode;
static  EWDB_ChannelStruct csChannel;
static  int iCriteria = 0;
static  int bShowComp = FALSE;
static  int bShowResponse = FALSE;

static  char * szDBUser = "ewdb_main";
static  char   szDBPwd[40]  = "main";
static  char * szDBSID  = "eqsutah.usgs";
static  char * szOutFile = 0x00;
static  char * szInFile = 0x00;
static  char * szProgramName = 0x00;
static  FILE * fIn = 0x00;
static  FILE * fOut = 0x00;

int PrintUsageMessage();
int ParseCommandLine(int argc, char ** argv);
int CompareSCNLs(EWDB_ChannelStruct * pcs1, EWDB_ChannelStruct * pcs2);
int CompareSites(EWDB_ChannelStruct * pcs1, EWDB_ChannelStruct * pcs2);
int CompareSiteTs(EWDB_ChannelStruct * pcs1, EWDB_ChannelStruct * pcs2);
int CompareChans(EWDB_ChannelStruct * pcs1, EWDB_ChannelStruct * pcs2);
int PrintSiteLine(EWDB_ChannelStruct * pChan, int bPrintSiteTInfo);
int PrintSiteTLine(EWDB_ChannelStruct * pChan);
int PrintSCNLLine(EWDB_ChannelStruct * pChan);
int PrintChanLine(EWDB_ChannelStruct * pChan);
int PrintChanTLine(EWDB_ChannelStruct * pChan, int bPrintResponse);

int ProcessSiteLine(const char * szLine, EWDB_ChannelStruct * pChan, FILE * fOut);
int ProcessSiteTLine(const char * szLine, EWDB_ChannelStruct * pChan, FILE * fOut);
int ProcessCompLine(const char * szLine, EWDB_ChannelStruct * pChan, FILE * fOut);
int ProcessCompTLine(const char * szLine, EWDB_ChannelStruct * pChan, FILE * fOut);
int ProcessChanLine(const char * szLine, EWDB_ChannelStruct * pChan, FILE * fOut);
int ProcessChanTLine(const char * szLine, EWDB_ChannelStruct * pChan, FILE * fOut);
int ProcessRespLine(const char * szLine, EWDB_ChannelStruct * pChan, FILE * fOut);


int main(int argc, char ** argv)
{
	
	int iRetCode;
	int iRunQuery = RUN_QUERY_UNDEFINED;
	int BUFFERSIZE= 5000000;
	char szBuffer[1024], szTemp[1024];
	int iNumLinesRead=0, iNumLinesProcessed=0;
	char * szToken;
	
	szProgramName = argv[ARGV_PROG_NAME];
	
	logit_init(szProgramName,0,1024,1);
	
	if(ParseCommandLine(argc,argv) != 0)
	{
		PrintUsageMessage();
		return(-1);
	}
	
	printf("Password: ");
	if(!fgets(szDBPwd,sizeof(szDBPwd),stdin))
		logit("e","Could not obtain password from stdin!\n");
	szDBPwd[strlen(szDBPwd) - 1] = 0x00;  /* remove '\n' char */
	
    memset(&csChannel,0, sizeof(csChannel));
    if(!(csChannel.pChanProps = malloc(sizeof(EWDB_ChanTCTFStruct))))
    {
      logit("e","%s:  Unable to allocate %d bytes for Channel Properties.  Returning.\n",
        sizeof(EWDB_ChanTCTFStruct));
      return(-1);
    }

    memset(szBuffer,0,sizeof(szBuffer));
	
    /* open input file for reading */
    fIn = fopen(szInFile,"r");
	
    if(!fIn)
    {
		logit("e","%s:  Unable to open input file(%s).  errno is %d.\n",
            szProgramName, szInFile, errno);
		return(-1);
    }
	
    /* open output file for writing results */
    fOut = fopen(szOutFile,"w");
	
    if(!fOut)
    {
		logit("e","%s:  Unable to open output file(%s).  errno is %d.\n",
            szProgramName, szOutFile, errno);
		return(-1);
    }
	
    iRetCode = ewdb_api_Init(szDBUser,szDBPwd,szDBSID);
    if(iRetCode != EWDB_RETURN_SUCCESS)
    {
      logit("e","ewdb_api_Init() failed.  Returning!\n");
      return(-1);
    }
    

    /* read in records till we hit EOF, increment "lines read" counter*/
    for(;fgets(szBuffer,sizeof(szBuffer)-1,fIn);iNumLinesRead++)
    {
		/* first log the line to file */
		fputs(szBuffer, fOut);
		
		/* check for comment line - skip*/
		if(szBuffer[0] == '#')
		{
			iNumLinesProcessed++;
			continue;
		}
		
		/* make a copy of line for token parsing. */
		memcpy(szTemp,szBuffer,sizeof(szTemp));
		
		/* discover the line type */
		szToken = strtok(szTemp, STRTOK_DELIMETERS);
		
		/* handle blank or bad lines */
		if(!szToken)
		{
			iNumLinesProcessed++;
			continue; /* bad line - skip*/
		}
		
		if(strcmp(szToken,"SITE") == 0)
			iRetCode = ProcessSiteLine(szBuffer, &csChannel, fOut);
		else if(strcmp(szToken,"SITET") == 0)
			iRetCode = ProcessSiteTLine(szBuffer, &csChannel, fOut); 
		else if(strcmp(szToken,"COMP") == 0)
			iRetCode = ProcessCompLine(szBuffer, &csChannel, fOut); 
		else if(strcmp(szToken,"COMPT") == 0)
			iRetCode = ProcessCompTLine(szBuffer, &csChannel, fOut); 
		else if(strcmp(szToken,"CHAN") == 0)
			iRetCode = ProcessChanLine(szBuffer, &csChannel, fOut); 
		else if(strcmp(szToken,"CHANT") == 0)
			iRetCode = ProcessChanTLine(szBuffer, &csChannel, fOut); 
		else if(strcmp(szToken,"RESP") == 0)
			iRetCode = ProcessRespLine(szBuffer, &csChannel, fOut); 
		else
		{
			fputs("# ERROR - BAD LINE!\n", fOut);
			continue; /* bad line */
		}
  if(iRetCode)
  {
    logit("e", "Error processing line %d.\n", iNumLinesRead+1);
  }
	}  /* end for each input line */
	
	if(feof(fIn))
	{
		fprintf(fOut, "# <EOF>  -  %d of %d lines successfully processed.\n",
			iNumLinesProcessed, iNumLinesRead);
	}
	else
	{
		fprintf(fOut, "# <ERROR>  -  %d of %d lines successfully processed, before read error(%d).\n",
			iNumLinesProcessed, iNumLinesRead, ferror(fIn));
	}
	
	fclose(fIn);
	fclose(fOut);
	
	return(0);
}  /* end main() */


int ProcessSiteLine(const char * szLine, EWDB_ChannelStruct * pChan, FILE * fOut)
{
	static char szTemp[1024];
  char * szToken;
  int iRetCode;
  void * pTemp;


  pTemp = pChan->pChanProps;
	memset(pChan,0,sizeof(EWDB_ChannelStruct));
  pChan->pChanProps = pTemp;

	memcpy(szTemp,szLine, sizeof(szTemp));
	szTemp[sizeof(szTemp) - 1] = 0x00;

	/* SITE */
	if(!(szToken = strtok(szTemp, STRTOK_DELIMETERS)))
  {
    fprintf(fOut,"#  ERROR parsing line.\n");
		goto SiteLineFail;
  }

	/* idSite */
	if(!(szToken = strtok(NULL, STRTOK_DELIMETERS)))
  {
    fprintf(fOut,"#  ERROR parsing idsite in line.\n");
		goto SiteLineFail;
  }


    /* sSta */
	if(!(szToken = strtok(NULL, STRTOK_DELIMETERS)))
  {
    fprintf(fOut,"#  ERROR parsing STA code.\n");
		goto SiteLineFail;
  }
	else
    strncpy(pChan->Comp.Sta, szToken, sizeof(pChan->Comp.Sta) - 1);

    /* sNet */
	if(!(szToken = strtok(NULL, STRTOK_DELIMETERS)))
  {
    fprintf(fOut,"#  ERROR parsing NET code. STA was (%s)\n",pChan->Comp.Sta);
		goto SiteLineFail;
  }
	else
    strncpy(pChan->Comp.Net, szToken, sizeof(pChan->Comp.Net) - 1);

    /* sComment */
	if(!(szToken = strtok(NULL, STRTOK_DELIMETERS)))
		szToken = "";

  /* Create the Site record */
  iRetCode = ewdb_api_CreateSite(pChan,szToken);

  if(iRetCode == EWDB_RETURN_SUCCESS)
  {
    fprintf(fOut,"#  SUCCESS idSite %d created.\n", pChan->idSite);
    return(0);
  }
  else if(iRetCode == EWDB_RETURN_WARNING && pChan->idSite > 0)
  {
    fprintf(fOut,"#  WARNING site (%s/%s) already exists in DB as idSite %d.\n",
            pChan->Comp.Sta, pChan->Comp.Net, pChan->idSite);
    return(1);
  }
  else
  {
    fprintf(fOut,"#  ERROR(%d) received while creating site for (%s/%s).\n",
            pChan->Comp.Sta, pChan->Comp.Net);
		goto SiteLineFail;
  }

SiteLineFail:
  return(-1);

} /* ProcessSiteLine() */

int ProcessSiteTLine(const char * szLine, EWDB_ChannelStruct * pChan, FILE * fOut)
{
	static char szTemp[1024];
  char * szToken;
  int iRetCode;


  /* make sure we have a valid idSite */
  if(pChan->idSite <= 0)
  {
    fprintf(fOut,"#  ERROR  - %d is not a vaild idSite.  There must be a valid "
                 " id Site from a previous line.\n", pChan->idSite);
  }
  /*  save the Sta, Net, and idSite.  clear out everything else */

  pChan->Comp.idChan = 0;
  pChan->idChanT = 0;
  pChan->idCompT = 0;
  pChan->idSiteT = 0;
  pChan->tOff = 0;
  pChan->tOn = 0;

  /* copy the line to temporary buffer */
	memcpy(szTemp,szLine, sizeof(szTemp));
	szTemp[sizeof(szTemp) - 1] = 0x00;

	/* SITET */
	if(!(szToken = strtok(szTemp, STRTOK_DELIMETERS)))
  {
    fprintf(fOut,"#  ERROR parsing line.\n");
		goto SiteTLineFail;
  }

	/* idSiteT */
	if(!(szToken = strtok(NULL, STRTOK_DELIMETERS)))
  {
    fprintf(fOut,"#  ERROR parsing idComp from line.\n");
		goto SiteTLineFail;
  }

    /* dLat */
	if(!(szToken = strtok(NULL, STRTOK_DELIMETERS)))
  {
    fprintf(fOut,"#  ERROR parsing Latitude.\n");
		goto SiteTLineFail;
  }
	else
    pChan->Comp.Lat = (float) atof(szToken);

    /* dLon */
	if(!(szToken = strtok(NULL, STRTOK_DELIMETERS)))
  {
    fprintf(fOut,"#  ERROR parsing Longitude.\n");
		goto SiteTLineFail;
  }
	else
    pChan->Comp.Lon = (float) atof(szToken);

    /* dElev */
	if(!(szToken = strtok(NULL, STRTOK_DELIMETERS)))
  {
    fprintf(fOut,"#  ERROR parsing Elevation.\n");
		goto SiteTLineFail;
  }
	else
    pChan->Comp.Elev = (float) atof(szToken);

    /* tOn */
	if(!(szToken = strtok(NULL, STRTOK_DELIMETERS"()-")))
  {
    fprintf(fOut,"#  ERROR parsing tOn.\n");
		goto SiteTLineFail;
  }
	else
    pChan->tOn = atof(szToken);

    /* tOff */
	if(!(szToken = strtok(NULL, STRTOK_DELIMETERS"()-")))
  {
    fprintf(fOut,"#  ERROR parsing tOff.\n");
		goto SiteTLineFail;
  }
	else
    pChan->tOff = atof(szToken);


  /* Create the SiteT record */
  iRetCode = ewdb_api_CreateSiteT(pChan);


  if(iRetCode == EWDB_RETURN_SUCCESS)
  {
    fprintf(fOut,"#  SUCCESS idSiteT %d created.\n", pChan->idSiteT);
    return(0);
  }
  else
  {
    fprintf(fOut,"#  ERROR(%d) received while creating sitet for (%s/%s) (%.2f-%.2f).\n",
            pChan->idSiteT, pChan->Comp.Sta, pChan->Comp.Net,
            pChan->tOn, pChan->tOff);
		goto SiteTLineFail;
  }


SiteTLineFail:
  return(-1);

} /* ProcessSiteTLine() */


int ProcessCompLine(const char * szLine, EWDB_ChannelStruct * pChan, FILE * fOut)
{
	static char szTemp[1024];
  char * szToken;
  int iRetCode;


  EWDB_ChannelStruct csTemp;

  /*  save the Sta, Net, and idSite.  clear out everything else */
	memset(&csTemp,0,sizeof(EWDB_ChannelStruct));
  memcpy(csTemp.Comp.Sta, pChan->Comp.Sta, sizeof(csTemp.Comp.Sta));
  memcpy(csTemp.Comp.Net, pChan->Comp.Net, sizeof(csTemp.Comp.Net));
  csTemp.idSite = pChan->idSite;
  csTemp.pChanProps = pChan->pChanProps;
  memcpy(pChan, &csTemp, sizeof(EWDB_ChannelStruct));


  /* copy the line to temporary buffer */
	memcpy(szTemp,szLine, sizeof(szTemp));
	szTemp[sizeof(szTemp) - 1] = 0x00;

	/* COMP */
	if(!(szToken = strtok(szTemp, STRTOK_DELIMETERS)))
  {
    fprintf(fOut,"#  ERROR parsing line.\n");
		goto CompLineFail;
  }

	/* idComp */
	if(!(szToken = strtok(NULL, STRTOK_DELIMETERS)))
  {
    fprintf(fOut,"#  ERROR parsing idComp from line.\n");
		goto CompLineFail;
  }

    /* sComp */
	if(!(szToken = strtok(NULL, STRTOK_DELIMETERS)))
  {
    fprintf(fOut,"#  ERROR parsing COMP code.\n");
		goto CompLineFail;
  }
	else
    strncpy(pChan->Comp.Comp, szToken, sizeof(pChan->Comp.Comp) - 1);

    /* sLoc */
	if(!(szToken = strtok(NULL, STRTOK_DELIMETERS)))
    /* Loc can be NULL.  - deal with it */
    pChan->Comp.Loc[0] = 0x00;
	else
    strncpy(pChan->Comp.Loc, szToken, sizeof(pChan->Comp.Loc) - 1);

    /* sComment */
	if(!(szToken = strtok(NULL, STRTOK_DELIMETERS)))
		szToken = "";

  /* Create the Comp record */
  iRetCode = ewdb_api_CreateComponent(&pChan->Comp.idComp, pChan->Comp.Sta,
                                      pChan->Comp.Comp, pChan->Comp.Net, 
                                      pChan->Comp.Loc, szToken);


  if(iRetCode == EWDB_RETURN_SUCCESS)
  {
    fprintf(fOut,"#  SUCCESS idComp %d created.\n", pChan->Comp.idComp);
    return(0);
  }
  else if(pChan->Comp.idComp == DB_ERROR_COMP_ALREADY_EXISTS)
  {
    fprintf(fOut,"#  WARNING Comp (%s/%s/%s/%s) already exists in DB.\n",
            pChan->Comp.Sta, pChan->Comp.Comp, pChan->Comp.Net, pChan->Comp.Loc);
    return(1);
  }
  else
  {
    fprintf(fOut,"#  ERROR(%d) received while creating comp for (%s/%s/%s/%s).\n",
            pChan->Comp.Sta, pChan->Comp.Comp, pChan->Comp.Net, pChan->Comp.Loc);
		goto CompLineFail;
  }

CompLineFail:
  return(-1);

} /* ProcessCompLine() */



int ProcessCompTLine(const char * szLine, EWDB_ChannelStruct * pChan, FILE * fOut)
{
	static char szTemp[1024];
  char * szToken;
  int iRetCode;


  /* make sure we have a valid idComp */
  if(pChan->Comp.idComp <= 0)
  {
    fprintf(fOut,"#  ERROR  - %d is not a vaild idComp.  There must be a valid "
                 " id comp from a previous line.\n", pChan->Comp.idComp);
  }
  /*  save the Sta, Net, and idSite.  clear out everything else */

  pChan->Comp.idChan = 0;
  pChan->idChanT = 0;
  pChan->idCompT = 0;
  pChan->tOff = 0;
  pChan->tOn = 0;

  /* clear out any response information */
  if(pChan->pChanProps)
    memset(pChan->pChanProps,0,sizeof(EWDB_ChanTCTFStruct));

  /* copy the line to temporary buffer */
	memcpy(szTemp,szLine, sizeof(szTemp));
	szTemp[sizeof(szTemp) - 1] = 0x00;

	/* COMPT */
	if(!(szToken = strtok(szTemp, STRTOK_DELIMETERS)))
  {
    fprintf(fOut,"#  ERROR parsing line.\n");
		goto CompTLineFail;
  }

	/* idCompT */
	if(!(szToken = strtok(NULL, STRTOK_DELIMETERS)))
  {
    fprintf(fOut,"#  ERROR parsing idComp from line.\n");
		goto CompTLineFail;
  }

    /* dLat */
	if(!(szToken = strtok(NULL, STRTOK_DELIMETERS)))
  {
    fprintf(fOut,"#  ERROR parsing Latitude.\n");
		goto CompTLineFail;
  }
	else
    pChan->Comp.Lat = (float) atof(szToken);

    /* dLon */
	if(!(szToken = strtok(NULL, STRTOK_DELIMETERS)))
  {
    fprintf(fOut,"#  ERROR parsing Longitude.\n");
		goto CompTLineFail;
  }
	else
    pChan->Comp.Lon = (float) atof(szToken);

    /* dElev */
	if(!(szToken = strtok(NULL, STRTOK_DELIMETERS)))
  {
    fprintf(fOut,"#  ERROR parsing Elevation.\n");
		goto CompTLineFail;
  }
	else
    pChan->Comp.Elev = (float) atof(szToken);

    /* dAzm */
	if(!(szToken = strtok(NULL, STRTOK_DELIMETERS)))
  {
    fprintf(fOut,"#  ERROR parsing Azimuth.\n");
		goto CompTLineFail;
  }
	else
    pChan->Comp.Azm = (float) atof(szToken);

    /* dDip */
	if(!(szToken = strtok(NULL, STRTOK_DELIMETERS)))
  {
    fprintf(fOut,"#  ERROR parsing Dip.\n");
		goto CompTLineFail;
  }
	else
    pChan->Comp.Dip = (float) atof(szToken);

    /* tOn */
	if(!(szToken = strtok(NULL, STRTOK_DELIMETERS"()-")))
  {
    fprintf(fOut,"#  ERROR parsing tOn.\n");
		goto CompTLineFail;
  }
	else
    pChan->tOn = atof(szToken);

    /* tOff */
	if(!(szToken = strtok(NULL, STRTOK_DELIMETERS"()-")))
  {
    fprintf(fOut,"#  ERROR parsing tOff.\n");
		goto CompTLineFail;
  }
	else
    pChan->tOff = atof(szToken);


  /* Create the CompT record */
  iRetCode = ewdb_api_CreateCompT(pChan);


  if(iRetCode == EWDB_RETURN_SUCCESS)
  {
    fprintf(fOut,"#  SUCCESS idCompT %d created.\n", pChan->idCompT);
    return(0);
  }
  else
  {
    fprintf(fOut,"#  ERROR(%d) received while creating compt for (%s/%s/%s/%s) (%.2f-%.2f).\n",
            pChan->idCompT, pChan->Comp.Sta, pChan->Comp.Comp, pChan->Comp.Net, pChan->Comp.Loc,
            pChan->tOn, pChan->tOff);
		goto CompTLineFail;
  }

CompTLineFail:
  return(-1);

} /* ProcessCompTLine() */


int ProcessChanLine(const char * szLine, EWDB_ChannelStruct * pChan, FILE * fOut)
{
	static char szTemp[1024];
  int iRetCode;


  /* make sure we have a valid idComp */
  if(pChan->Comp.idComp <= 0)
  {
    fprintf(fOut,"#  ERROR  - %d is not a vaild idComp.  There must be a valid "
                 " id comp from a previous line.\n", pChan->Comp.idComp);
  }
  /*  save the Sta, Net, and idSite.  clear out everything else */

  pChan->Comp.idChan = 0;
  pChan->idChanT = 0;
  pChan->tOff = 0;
  pChan->tOn = 0;

  /* clear out any response information */
  if(pChan->pChanProps)
    memset(pChan->pChanProps,0,sizeof(EWDB_ChanTCTFStruct));

  iRetCode = ewdb_api_CreateChannel(&(pChan->Comp.idChan), "");

  if(iRetCode == EWDB_RETURN_SUCCESS)
  {
    fprintf(fOut,"#  SUCCESS idChan %d created.\n", pChan->Comp.idChan);
    return(0);
  }
  else
  {
    fprintf(fOut,"#  ERROR(%d) received while creating channel.\n", 
            pChan->Comp.idChan);
		return(-1);
  }

} /* ProcessChanLine() */


int ProcessChanTLine(const char * szLine, EWDB_ChannelStruct * pChan, FILE * fOut)
{
	static char szTemp[1024];
  char * szToken;
  int iRetCode;


  /* make sure we have a valid idChan */
  if(pChan->Comp.idChan <= 0)
  {
    fprintf(fOut,"#  ERROR  - %d is not a vaild idChan.  There must be a valid "
                 " idChan from a previous line.\n", pChan->Comp.idChan);
  }

  /* make sure we have a valid idCompT */
  if(pChan->idCompT <= 0)
  {
    fprintf(fOut,"#  ERROR  - %d is not a vaild idCompT.  There must be a valid "
                 " idCompT from a previous line.\n", pChan->idCompT);
  }

  /* make sure we have a valid pChanProps */
  if(!pChan->pChanProps)
  {
    fprintf(fOut,"#  ERROR  - pChan->pChanProps pointer is NULL.\n");
		goto ChanTLineFail;
  }

  /*  save the Sta, Net, and idSite.  clear out everything else */
  pChan->idChanT = 0;
  pChan->tOff = 0;
  pChan->tOn = 0;

  /* clear out any response information */
  if(pChan->pChanProps)
    memset(pChan->pChanProps,0,sizeof(EWDB_ChanTCTFStruct));

  /* copy the line to temporary buffer */
	memcpy(szTemp,szLine, sizeof(szTemp));
	szTemp[sizeof(szTemp) - 1] = 0x00;

	/* CHANT */
	if(!(szToken = strtok(szTemp, STRTOK_DELIMETERS)))
  {
    fprintf(fOut,"#  ERROR parsing line.\n");
		goto ChanTLineFail;
  }

	/* idChanT */
	if(!(szToken = strtok(NULL, STRTOK_DELIMETERS)))
  {
    fprintf(fOut,"#  ERROR parsing idComp from line.\n");
		goto ChanTLineFail;
  }

  /*************************************************
  *  NOTE: dSampRate and dGain do not get written
  *        to the database as part of this line.
  *        They get recorded to the ChannelStruct
  *        as part of this line, and then get written
  *        to the DB as part of the RESP line.
  **************************************************/
    /* dSampRate */
	if(!(szToken = strtok(NULL, STRTOK_DELIMETERS)))
  {
    fprintf(fOut,"#  ERROR parsing Sample Rate.\n");
		goto ChanTLineFail;
  }
	else
    pChan->pChanProps->dSampRate = atof(szToken);

    /* dGain */
	if(!(szToken = strtok(NULL, STRTOK_DELIMETERS)))
  {
    fprintf(fOut,"#  ERROR parsing Gain.\n");
		goto ChanTLineFail;
  }
	else
    pChan->pChanProps->dGain = atof(szToken);

    /* tOn */
	if(!(szToken = strtok(NULL, STRTOK_DELIMETERS"()-")))
  {
    fprintf(fOut,"#  ERROR parsing tOn.\n");
		goto ChanTLineFail;
  }
	else
    pChan->tOn = atof(szToken);

    /* tOff */
	if(!(szToken = strtok(NULL, STRTOK_DELIMETERS"()-")))
  {
    fprintf(fOut,"#  ERROR parsing tOff.\n");
		goto ChanTLineFail;
  }
	else
    pChan->tOff = atof(szToken);


  /* Create the ChanT record */
  iRetCode = ewdb_internal_SetChanParams(pChan);


  if(iRetCode == EWDB_RETURN_SUCCESS)
  {
    fprintf(fOut,"#  SUCCESS idChanT %d created.\n", pChan->idChanT);
    return(0);
  }
  else
  {
    fprintf(fOut,"#  ERROR(%d) received while creating chant for chan %d CompT %d (%.0f - %.0f)\n",
            pChan->Comp.idChan, pChan->idCompT, pChan->tOn, pChan->tOff);
		goto ChanTLineFail;
  }

ChanTLineFail:
  return(-1);

}  /* end ProcessChanTLine() */


int ProcessRespLine(const char * szLine, EWDB_ChannelStruct * pChan, FILE * fOut)
{
	static char szTemp[1024];
  char * szToken;
  int iRetCode;
  char * szComment;
  char * pDelim;
  int iOddEven=0;


  /* make sure we have a valid idChan */
  if(pChan->idChanT <= 0)
  {
    fprintf(fOut,"#  ERROR  - %d is not a vaild idChanT.  There must be a valid "
                 " idChan from a previous line.\n", pChan->idChanT);
  }

  /* make sure we have a valid pChanProps */
  if(!pChan->pChanProps)
  {
    fprintf(fOut,"#  ERROR  - pChan->pChanProps pointer is NULL.\n");
		goto RespLineFail;
  }

  /*  clear out the pChanProps */
  memset(&(pChan->pChanProps->tfsFunc),0,sizeof(EWDB_TransformFunctionStruct));

  /* copy the line to temporary buffer */
	memcpy(szTemp,szLine, sizeof(szTemp));
	szTemp[sizeof(szTemp) - 1] = 0x00;


  if(strlen(szTemp) < RESP_COMMENT_OFFSET)
  {
    fprintf(fOut,"#  ERROR parsing line.  Insufficient length(%d).\n",
            strlen(szTemp));
		goto RespLineFail;
  }

    /* comment */
  szComment = &(szTemp[RESP_COMMENT_OFFSET]);
  pDelim = strstr(szTemp, " PZ ");
  if(!pDelim)
  {
    fprintf(fOut,"#  ERROR searching for PZ token.\n");
		goto RespLineFail;
  }
  *pDelim = 0x00;

  /* parse the Poles and Zeroes */
	if(!(szToken = strtok(pDelim+3/*" PZ "*/, STRTOK_DELIMETERS)))
  {
    fprintf(fOut,"#  ERROR parsing P in PZ string.\n");
		goto RespLineFail;
  }
	else
  {
    if(strcmp(szToken,"P") != 0)
    {
      fprintf(fOut,"#  ERROR parsing P in PZ string.  Found (%s) instead of P.\n",
              szToken);
      goto RespLineFail;
    }
  }
  /* keep going until we get all the Poles values */
  while((szToken = strtok(NULL, STRTOK_DELIMETERS)) &&
        strcmp(szToken,"Z")
       )
  {
    if(pChan->pChanProps->tfsFunc.iNumPoles == EWDB_MAX_POLES_OR_ZEROES)
      continue;

    if(!iOddEven)
    {
    pChan->pChanProps->tfsFunc.Poles[pChan->pChanProps->tfsFunc.iNumPoles].dReal =
       atof(szToken);
    iOddEven = 1;
    }
    else
    {
    pChan->pChanProps->tfsFunc.Poles[pChan->pChanProps->tfsFunc.iNumPoles].dImag =
       atof(szToken);
    pChan->pChanProps->tfsFunc.iNumPoles++;
    iOddEven = 0;
    }
  }
  
  if(iOddEven)
  {
    fprintf(fOut,"#  ERROR parsing Pole %d in PZ string. Imaginary portion not found!\n",
            pChan->pChanProps->tfsFunc.iNumPoles);
		goto RespLineFail;
  }

  if(!szToken)
  {
    fprintf(fOut,"#  ERROR parsing Z in PZ string.\n");
		goto RespLineFail;
  }

  /* keep going until we get all the Zeroes values */
  while(szToken = strtok(NULL, STRTOK_DELIMETERS))
  {
    if(pChan->pChanProps->tfsFunc.iNumZeroes == EWDB_MAX_POLES_OR_ZEROES)
      continue;

    if(!iOddEven)
    {
    pChan->pChanProps->tfsFunc.Zeroes[pChan->pChanProps->tfsFunc.iNumZeroes].dReal =
       atof(szToken);
    iOddEven = 1;
    }
    else
    {
    pChan->pChanProps->tfsFunc.Zeroes[pChan->pChanProps->tfsFunc.iNumZeroes].dImag =
       atof(szToken);
    pChan->pChanProps->tfsFunc.iNumZeroes++;
    iOddEven = 0;
    }
  }
  
  if(iOddEven)
  {
    fprintf(fOut,"#  ERROR parsing Zero %d in PZ string. Imaginary portion not found!\n",
            pChan->pChanProps->tfsFunc.iNumZeroes);
		goto RespLineFail;
  }

  /* Create the Resp record */
  iRetCode = ewdb_api_CreateTransformFunction(&pChan->pChanProps->tfsFunc.idCookedTF, 
                                          &pChan->pChanProps->tfsFunc);

    if(iRetCode == EWDB_RETURN_SUCCESS)
  {
    fprintf(fOut,"#  SUCCESS 1 idCookedTF %d created.\n", pChan->pChanProps->tfsFunc.idCookedTF);
  }
  else
  {
    fprintf(fOut,"#  ERROR 1 (%d) received while creating cookedtf.\n",
            pChan->pChanProps->tfsFunc.idCookedTF);
		goto RespLineFail;
  }

  iRetCode = ewdb_api_SetTransformFuncForChanT(pChan->idChanT, pChan->pChanProps->dGain,
                                       pChan->pChanProps->tfsFunc.idCookedTF,
                                       pChan->pChanProps->dSampRate);
  if(iRetCode == EWDB_RETURN_SUCCESS)
  {
    fprintf(fOut,"#  SUCCESS 2 idCookedTF %d associated with idChanT %d.\n", 
            pChan->pChanProps->tfsFunc.idCookedTF, pChan->idChanT);
    return(0);
  }
  else
  {
    fprintf(fOut,"#  ERROR 2 (%d) received while associated cookedtf %d and idChanT %d.\n",
            pChan->pChanProps->tfsFunc.idCookedTF, pChan->idChanT);
		goto RespLineFail;
  }

RespLineFail:
  return(-1);

}  /* end ProcessRespLine() */


int ParseCommandLine(int argc, char ** argv)
{

  if(argc < ARGC_MIN)
  {
    logit("e","%s:ParseCommandLine() ERROR: Insufficient number of command line arguments.\n",
          szProgramName);
    goto UsageError;
  }
  szDBUser = argv[ARGV_DB_USER];
  szDBSID  = argv[ARGV_DB_SID];

  szInFile  = argv[ARGV_INFILE];
  szOutFile = argv[ARGV_OUTFILE];

  return(0);

UsageError:
    return(-1);

}  /* end ParseCommandLine() */


int PrintUsageMessage()
{
  printf(szUsageMessage);
  return(0);
}  /* end PrintUsageMessage() */



