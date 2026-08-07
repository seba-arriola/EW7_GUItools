
/*
 *   This file is under RCS - do not modify unless you have
 *   checked it out using the command checkout.
 *
 *    $Id: stalist_hinv2ora.c,v 1.4 1999/07/05 19:39:39 davidk Exp $
 *    Revision history:
 *
 *    $Log: stalist_hinv2ora.c,v $
 *    Revision 1.4  1999/07/05 19:39:39  davidk
 *    Removed hardcoded DBUser,DBPassword,DBSID.  DB parameters
 *    are now entered on the command line.  Added a new "usage"
 *    description reply line when the user calls the program with
 *    the wrong number of args.
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
  
/* stalist_hinv2ora.c */

#include <stdlib.h>
#include <stdio.h>
#include <rdbms_all.h>


/* Column positions of various fields from hypoinv_sta.format file */
#define STA_POS 1
#define NET_POS 7
#define CHAN_POS 11
#define LATD_POS 16
#define LATM_POS 19
#define LOND_POS 27
#define LONM_POS 31
#define ELEV_POS 39
#define PERD_POS 43
#define STA_LEN 5
#define NET_LEN 2
#define CHAN_LEN 3
#define LATD_LEN 2
#define LATM_LEN 7
#define LOND_LEN 3
#define LONM_LEN 7
#define ELEV_LEN 4
#define PERD_LEN 3

#define LINE_BUFFER_SIZE 200


char DBuser[50];
char DBpassword[50];
char DBservice[50];

void dbvar_init(int argc, char ** argv)
{
  if(argc < 5 )
  {
    printf("usage:  stalist_hinv2ora <STATION_LIST_FILENAME> "
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
  StationStruct SCN;
  char pBuffer[LINE_BUFFER_SIZE];
  int i,done=0,retval;
  char szTemp[20];
  FILE * fptr;

  logit_init("Hinv_to_Ora_StaList",27,1024,1); 
  dbvar_init(argc,argv);
  

  if((fptr=fopen(argv[1],"r+")) == NULL ) 
  {
    logit("et","Could not open input file, pass filename as arg1\n");
    exit(-1);
  }
  
  OraAPIInit();

  for(i=0; ; i++)
  {
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

    if(strlen(pBuffer) < 43 /* Magic Number */)
      continue;

    strncpy(SCN.sta,&pBuffer[STA_POS-1],STA_LEN);
    SCN.sta[STA_LEN]=0;
    strncpy(SCN.net,&pBuffer[NET_POS-1],NET_LEN);
    SCN.net[NET_LEN]=0;
    strncpy(SCN.chan,&pBuffer[CHAN_POS-1],CHAN_LEN);
    SCN.chan[CHAN_LEN]=0;

    
    SCN.lat=atof(&pBuffer[LATD_POS-1])+ atof(&pBuffer[LATM_POS-1])/60.0;
      /* degrees converted from degrees + minutes*/
    SCN.lon=atof(&pBuffer[LOND_POS-1])+ atof(&pBuffer[LONM_POS-1])/60.0;
      /* degrees converted from degrees + minutes*/;
    strncpy(szTemp,&pBuffer[ELEV_POS-1],ELEV_LEN);
    szTemp[ELEV_LEN]=0;
    SCN.elev=atof(szTemp);

    /* Added because I can't find a direction variable in the calsta.hinv,
       and because other programs regard the lon in california as negative. 
       DK 03/02/99 */
    if(SCN.lon > 0.0)
    {
      SCN.lon = 0 - SCN.lon;
    }

    if(PutStation(&SCN) == RETURN_FAILURE)
    {
      logit("e","******PutStation failed for %s, %s, %s.\n",
            SCN.sta,SCN.chan,SCN.net);
    }
    else
    {
      logit("e","PutStation successful for %s, %s, %s.\n",
        SCN.sta,SCN.chan,SCN.net);
    }
  }
  OraAPIShutdown();
  return(0);
}


