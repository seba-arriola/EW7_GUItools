/*                                                          *
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE *
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.             *
 *
 *    $Id: sta_maint.c,v 1.2 2003/12/13 02:20:39 davidk Exp $
 *
 *    Revision history:
 *     $Log: sta_maint.c,v $
 *     Revision 1.2  2003/12/13 02:20:39  davidk
 *     Added the RCS Log header.
 *     Fixed the USAGE message so it is now for sta_maint instead
 *     of left over from sta_export.
 *
 *
 *
 ************************************************************/
#include <ewdb_ora_api.h>
#include "sta_maint.h"


/* define constants for argv positions and argc */
#define ARGC_MIN          5
#define ARGV_PROG_NAME    0
#define ARGV_DB_USER      1
#define ARGV_DB_SID       2
#define ARGV_INFILE       3
#define ARGV_OUTFILE      4


/* "sta_maint <DB_USER> <DB_SID> <INPUT_FILENAME> <OUTPUT_FILENAME> " */
static char szUsageMessage[] =
    "USAGE: sta_maint <DB_USER> <DB_SID> <INPUT_FILENAME> <OUTPUT_FILENAME>\n"
    "\n"
	"  <DB_USER>          Database username                                 \n"
	"  <DB_SID>           Database SID descriptor                           \n"
	"  <INPUT_FILENAME>   Name of file from which station data will be read \n"
	"                      and used to build database                       \n"
	"  <OUTPUT_FILENAME>  Name of file where actions/results will be logged \n"
    "\n"
  ;

static  int iProgramMode;
static  EWDB_ChannelStruct csFilterChannel;
static  EWDB_ChannelStruct csFilterMaxChannel;
static  int iCriteria = 0;
static  int bShowComp = FALSE;
static  int bShowResponse = FALSE;

static  char * szDBUser = "ewdb_main";
static  char   szDBPwd[40]  = "";
static  char * szDBSID  = "";
static  char * szInFile = 0x00;
static  char * szOutFile = 0x00;
static  char * szProgramName = 0x00;
static  FILE * fOut = 0x00;

static  EWDB_ChannelStruct csFilterChannel;
static  EWDB_ChannelStruct csFilterMaxChannel;

/* Global */
EWDB_ChannelStruct GlobalChan;

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
int HandleCommand(char * szCmdClass, char * szCmd, char * szLineBuffer, FILE * fOut);

char * strncpy_ew(char * dest, const char * source, size_t iLenIncludingNull);
int init(int argc, char ** argv);

int main(int argc, char ** argv)
{

  int iRetCode;
  int iRunQuery = RUN_QUERY_UNDEFINED;
  int BUFFERSIZE= 5000000;
  char szCurrentDirectory[256];
  char szCmdTemp[10];
  FILE * fIn=NULL;
  int rc=0;
  int iLineCtr;
  char * szCmdClass, * szCmd;

  char szLineBuffer[256];
  char szOrigLineBuffer[256];
  int  iLineBufferLen;

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
  

  memset(szOrigLineBuffer,0,sizeof(szOrigLineBuffer));
  iLineBufferLen = sizeof(szOrigLineBuffer) - 1;

  szProgramName = argv[0];

  memset(szCmdTemp,0,sizeof(szCmdTemp));
  

  logit_init(szProgramName,0,1024,1);

  if(ParseCommandLine(argc,argv) != 0)
  {
    PrintUsageMessage();
    return(-1);
  }

  /* open the input and output files, so we can issue
     errors before doing anything complicated */
  fIn = fopen(szInFile, "r");
  if(!fIn)
  {
    logit("e","Could not open input file: (%s).  Current directory is (%s).\n",
          szInFile, getcwd(szCurrentDirectory,sizeof(szCurrentDirectory)));
    rc = -1;
    goto Cleanup;
  }
  fOut = fopen(szOutFile,"w+");
  if(!fOut)
  {
    logit("e","Could not open input file: (%s).  Current directory is (%s).\n",
          szOutFile, getcwd(szCurrentDirectory,sizeof(szCurrentDirectory)));
    rc = -1;
    goto Cleanup;
  }

  iLineCtr = 0;
  /* read the input file one line at a time */
  while(fgets(szOrigLineBuffer, iLineBufferLen, fIn))
  {

    iLineCtr++;
    if(szOrigLineBuffer[strlen(szOrigLineBuffer)-1] == '\n')
      szOrigLineBuffer[strlen(szOrigLineBuffer)-1] = 0x00;

    memcpy(szLineBuffer,szOrigLineBuffer, sizeof(szOrigLineBuffer));

    if(szLineBuffer[0] == '#')
    {
      /* this line is a comment, ignore */
      continue;
    }

    szCmdClass = strtok(szLineBuffer," ");
    if(!szCmdClass)
    {
      /* this line is blank, ignore */
      continue;
    }

    strncpy_ew(szCmdTemp, &szLineBuffer[STA_FILE_OFFSET_CMD],STA_FILE_LEN_CMD + 1);  /* magic numbers abound */
    szCmd = strtok(szCmdTemp," ");
    if(!szCmd)
    {
      /* could not parse the command */
      logit("e", "Error parsing line(%d): Could not parse command for line type(%s)\n", 
            iLineCtr, szCmdClass);
      continue;
    }

    logit("e","Line(%d): Command found!  (%s:%s)\n", iLineCtr,szCmdClass, szCmd);

    /* parse and process each line */

    fprintf(fOut,"%s\n",szOrigLineBuffer);
    iRetCode = HandleCommand(szCmdClass, szCmd, szOrigLineBuffer, fOut);
    if(iRetCode)
    {
      logit("e","Line(%d): ERROR processing command(%s:%s)!\n", iLineCtr,szCmdClass, szCmd);
    }
  }

  if(feof(fIn))
  {
    /* all done, goto cleanup */
    goto Cleanup;
  }
  else
  {
    logit("et","Error reading line %d from %s.\n", ++iLineCtr, szInFile);
    rc = -1;
    goto Cleanup;
  }


Cleanup:

  /* close the input file */
  if(fIn)
  {
    fclose(fIn);
    fIn = NULL;
    
  }
  
  /* close the output file */
  if(fOut)
  {
    fclose(fOut);
    fOut = NULL;
    
  }

  return(rc);
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

  /* initialize in global/static variables */
  memset(&GlobalChan,0, sizeof(GlobalChan));
  if(!(GlobalChan.pChanProps = malloc(sizeof(EWDB_ChanTCTFStruct))))
  {
    logit("e","%s:  Unable to allocate (%d) bytes for GlobalChan.pChanProps.  Errno=%d.\n",
          szProgramName, sizeof(EWDB_ChanTCTFStruct), errno);
    return(EWDB_RETURN_FAILURE);
  }

  return(EWDB_RETURN_SUCCESS);

}  /* end init() */


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

  szInFile = argv[ARGV_INFILE];
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


int HandleCommand(char * szCmdClass, char * szCmd, char * szLineBuffer, FILE * fOut)
{

  int rc;


  if(strcmp(szCmdClass, "SITE") == 0)
  {
    rc=HandleSiteCommand(szCmd,szLineBuffer,fOut);
  }
  else if(strcmp(szCmdClass, "SITET") == 0)
  {
    rc=HandleSiteTCommand(szCmd,szLineBuffer,fOut);
  }
  else if(strcmp(szCmdClass, "COMP") == 0)
  {
    rc=HandleCompCommand(szCmd,szLineBuffer,fOut);
  }
  else if(strcmp(szCmdClass, "COMPT") == 0)
  {
    rc=HandleCompTCommand(szCmd,szLineBuffer,fOut);
  }
  else if(strcmp(szCmdClass, "CHAN") == 0)
  {
    rc=HandleChanCommand(szCmd,szLineBuffer,fOut);
  }
  else if(strcmp(szCmdClass, "CHANT") == 0)
  {
    rc=HandleChanTCommand(szCmd,szLineBuffer,fOut);
  }
  else if(strcmp(szCmdClass, "RESP") == 0)
  {
    rc=HandleRespCommand(szCmd,szLineBuffer,fOut);
  }
  else
  {
    fprintf(fOut,"#ERROR: Unsupported command type(%s).\n", szCmdClass);
    rc = -2;
  }
  return(rc);
}  /* end HandleCommand() */



char * strncpy_ew(char * dest, const char * source, size_t iLenIncludingNull)
{
  strncpy(dest,source,iLenIncludingNull - 1);
  dest[iLenIncludingNull - 1] = 0x00;

  return(dest);
}  /* end strncpy_ew() */



int WriteToOutputFile(char * szOutputString)
{
  if(fOut)
    return(fprintf(fOut,szOutputString));
  else
    return(0);
}


