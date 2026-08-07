/*
 *   This file is under RCS - do not modify unless you have
 *   checked it out using the command checkout.
 *
 *    $Id: stalist_shkmap2ora.c,v 1.7 2002/11/03 21:31:22 lombard Exp $
 *    Revision history:
 *
 *    $Log: stalist_shkmap2ora.c,v $
 *    Revision 1.7  2002/11/03 21:31:22  lombard
 *    Corrected mode of fopen() call.
 *
 *    Revision 1.6  2001/07/01 21:55:36  davidk
 *    Cleanup of the Earthworm Database API and the applications that utilize it.
 *    The "ewdb_api" was cleanup in preparation for Earthworm v6.0, which is
 *    supposed to contain an API that will be backwards compatible in the
 *    future.  Functions were removed, parameters were changed, syntax was
 *    rewritten, and other stuff was done to try to get the code to follow a
 *    general format, so that it would be easier to read.
 *
 *    Applications were modified to handle changed API calls and structures.
 *    They were also modified to be compiler warning free on WinNT.
 *
 *    Revision 1.5  2001/05/15 02:15:50  davidk
 *    Moved functions around between the apps, DB API, and DB API INTERNAL
 *    levels.  Renamed functions and files.  Added support for amplitude
 *    magnitude types.  Reformatted makefiles.
 *
 *    Revision 1.4  2001/02/28 17:29:10  lucky
 *    Massive schema redesign and cleanup.
 *
 *    Revision 1.3  2000/11/22 00:34:06  dietz
 *    Added support for Location (L field of SCNL).
 *
 *    Revision 1.2  2000/03/30 18:25:44  davidk
 *    updated to use schema2 Station External routines from schema1
 *    earthworm SCN routines.
 *
 *    Revision 1.1  2000/03/16 21:04:50  dietz
 *    Initial revision
 *
 *    Revision 1.3  1999/06/14 23:19:30  lucky
 *    STA_LEN changed to 5
 *
 *    Revision 1.2  1999/06/09 19:01:00  lucky
 *    Changed STA_LEN from 3 to 6.
 *
 *    Revision 1.1  1999/05/05 18:50:18  lucky
 *    Initial revision
 *
 *
 */
  

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ewdb_ora_api.h>
#include <ewdb_apps_utils.h>


/* Column positions of various fields from shakemap feeder file */
#define STA_POS 1
#define NET_POS 14
#define CHAN_POS 9
#define LOC_POS 17
#define LATD_POS 39
#define LOND_POS 50
#define STA_LEN 5
#define NET_LEN 2
#define CHAN_LEN 3
#define LOC_LEN 2

#define LINE_BUFFER_SIZE 200


char DBuser[50];
char DBpassword[50];
char DBservice[50];

void dbvar_init(int argc, char ** argv)
{
  if(argc < 5 )
  {
    printf("usage:  stalist_shkmap2ora <STATION_LIST_FILENAME> "
           "<DBUSER> <DBPWD> <DBSID>\n");
    exit(0);
  }
  /* else */
  strcpy(DBuser,argv[2]);
  strcpy(DBpassword,argv[3]);
  strcpy(DBservice,argv[4]);

}


int main(int argc, char ** argv)
{
  EWDB_External_StationStruct SCN;
  char pBuffer[LINE_BUFFER_SIZE];
  int i,done=0,retval;
  FILE * fptr;

  logit_init("stalist_shkmap2ora",0,1024,1); 
  dbvar_init(argc,argv);
  

  if((fptr=fopen(argv[1],"r")) == NULL ) 
  {
    logit("et","Could not open input file, pass filename as arg1\n");
    exit(-1);
  }
  
  if(ewdb_api_Init(DBuser,DBpassword,DBservice) != EWDB_RETURN_SUCCESS)
  {
    logit("et","ewdb_api_Init() did not return success.  Exiting!!\n");
    return(EWDB_RETURN_FAILURE);
  }

  for(i=0; ; i++)
  {
    memset(&SCN,0,sizeof(SCN));
    retval=(int)fgets(pBuffer,LINE_BUFFER_SIZE,fptr);

    if (retval == 0)
    {
      if(feof(fptr))
      {
        logit("et","Reached end of file %s\n",argv[1]);
        break;
      }
      else
      {
        logit("et","Error %d occured while reading from file %s\n",
              ferror(fptr),argv[1]);
      }
    }

    if(strlen(pBuffer) < 60 /* Magic Number */)
      continue;


    /* Don't worry about stripping the blanks off, 
      the API code below us will do that for us. */
    strncpy(SCN.Station.Sta,&pBuffer[STA_POS-1],STA_LEN);
    SCN.Station.Sta[STA_LEN]=0;
    strncpy(SCN.Station.Net,&pBuffer[NET_POS-1],NET_LEN);
    SCN.Station.Net[NET_LEN]=0;
    strncpy(SCN.Station.Comp,&pBuffer[CHAN_POS-1],CHAN_LEN);
    SCN.Station.Comp[CHAN_LEN]=0;
    strncpy(SCN.Station.Loc,&pBuffer[LOC_POS-1],LOC_LEN);
    SCN.Station.Loc[LOC_LEN]=0;


    SCN.Station.Lat=(float)atof(&pBuffer[LATD_POS-1]);
    SCN.Station.Lon=(float)atof(&pBuffer[LOND_POS-1]);
    SCN.Station.Elev=(float)0;

    if(ewdb_api_CreateOrAlterExternalStation(&SCN) == EWDB_RETURN_FAILURE)
    {
      logit("e","******%s() failed for %s %s %s %s.\n",
            "ewdb_api_CreateOrAlterExternalStation",SCN.Station.Sta,SCN.Station.Comp,SCN.Station.Net,SCN.Station.Loc);
    }
    else
    {
      logit("e","%s() successful for %s %s %s %s.\n",
            "ewdb_api_CreateOrAlterExternalStation",SCN.Station.Sta,SCN.Station.Comp,SCN.Station.Net,SCN.Station.Loc);
    }
  }
  ewdb_api_Shutdown();
  return(0);
}


