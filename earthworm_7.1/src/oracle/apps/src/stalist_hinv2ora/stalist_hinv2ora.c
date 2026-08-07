
/*
 *   This file is under RCS - do not modify unless you have
 *   checked it out using the command checkout.
 *
 *    $Id: stalist_hinv2ora.c,v 1.13 2002/11/03 21:31:04 lombard Exp $
 *    Revision history:
 *
 *    $Log: stalist_hinv2ora.c,v $
 *    Revision 1.13  2002/11/03 21:31:04  lombard
 *    Corrected mode of fopen() call.
 *
 *    Revision 1.12  2002/09/05 17:26:14  lucky
 *    Fixed the reading of Sta and Net fields
 *
 *    Revision 1.11  2001/07/01 21:55:36  davidk
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
 *    Revision 1.10  2001/05/15 02:15:07  davidk
 *    Moved functions around between the apps, DB API, and DB API INTERNAL
 *    levels.  Renamed functions and files.  Added support for amplitude
 *    magnitude types.  Reformatted makefiles.
 *
 *    Revision 1.9  2001/03/19 23:30:42  dietz
 *    Really and truely fixed elevation bug, going to Hypoinverse manual.
 *    Note, all positions here are given in Fortranese (start counting at 1).
 *    Elevation starts at position 39 and is 4 chars long.
 *    Added checks on the NS, EW flags for latitude and longitude.
 *
 *    Revision 1.8  2001/02/28 17:29:10  lucky
 *    Massive schema redesign and cleanup.
 *
 *    Revision 1.7  2000/06/26 17:00:26  lucky
 *    Fixed elevation bug, correctly this time. Elevation starts at position 38, and
 *    we are interested in 5 characters.
 *
 *    Revision 1.6  2000/06/26 16:53:01  lucky
 *    Fixed Elevation Bug -- for some reason we were reading one character too few.
 *
 *    Revision 1.5  2000/03/30 18:22:16  davidk
 *    modified to use schema2 Station External routines instead of schema1
 *    Earthworm SCN routines.
 *
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
  
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ewdb_ora_api.h>
#include <ewdb_apps_utils.h>


/* Column positions of various fields from hypoinv_sta.format file */
#define STA_POS 1
#define NET_POS 7
#define CHAN_POS 11
#define LATD_POS 16
#define LATM_POS 19
#define NS_POS   26
#define LOND_POS 27
#define LONM_POS 31
#define EW_POS   38
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
  EWDB_External_StationStruct SCN;
  char pBuffer[LINE_BUFFER_SIZE];
  int i,done=0,retval;
  char szTemp[20];
  char flag;
  char *tmpstr;
  FILE * fptr;

  logit_init("stalist_hinv2ora",27,1024,1); 
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

    if(strlen(pBuffer) < 43 /* Magic Number */)
      continue;

    strncpy(SCN.Station.Sta,&pBuffer[STA_POS-1],STA_LEN);
    SCN.Station.Sta[STA_LEN]=0;
    strncpy(SCN.Station.Net,&pBuffer[NET_POS-1],NET_LEN);
    SCN.Station.Net[NET_LEN]=0;
    strncpy(SCN.Station.Comp,&pBuffer[CHAN_POS-1],CHAN_LEN);
    SCN.Station.Comp[CHAN_LEN]=0;

	/* Fix station code */
	if ((tmpstr = strchr (SCN.Station.Sta, ' ')) != NULL)
		*tmpstr = '\0';

	/* Fix network code to handle GIOC empty fields */
	if (SCN.Station.Net[0] == ' ')
		strcpy (SCN.Station.Net, "??");

 /* Latitude: degrees converted from degrees + minutes */
 /*  flag: 'N','n',' ' for North (+) latitude; 'S','s' for South (-) */
    strncpy(szTemp,&pBuffer[LATD_POS-1],LATD_LEN);
    szTemp[LATD_LEN]=0;
    SCN.Station.Lat = (float) atof(szTemp);
    strncpy(szTemp,&pBuffer[LATM_POS-1],LATM_LEN);
    szTemp[LATM_LEN]=0;
    SCN.Station.Lat += (float)(atof(szTemp)/60.0);
    flag = pBuffer[NS_POS-1]; 
    if( flag=='S' || flag=='s' ) SCN.Station.Lat *= -1.0;


 /* Longitude: degrees converted from degrees + minutes */
 /*  flag: 'W','w',' ' for West (-) longitude; 'E','e' for East (+) */
    strncpy(szTemp,&pBuffer[LOND_POS-1],LOND_LEN);
    szTemp[LOND_LEN]=0;
    SCN.Station.Lon = (float) atof(szTemp);
    strncpy(szTemp,&pBuffer[LONM_POS-1],LONM_LEN);
    szTemp[LONM_LEN]=0;
    SCN.Station.Lon += (float)(atof(szTemp)/60.0);
    flag = pBuffer[EW_POS-1]; 
    if( flag==' ' || flag=='W' || flag=='w' ) SCN.Station.Lon *= -1.0;

 /* Elevation: meters */
    strncpy(szTemp,&pBuffer[ELEV_POS-1],ELEV_LEN);
    szTemp[ELEV_LEN]=0;
    SCN.Station.Elev=(float)atof(szTemp);


 /* Stuff it into DBMS */
    if(ewdb_api_CreateOrAlterExternalStation(&SCN) == EWDB_RETURN_FAILURE)
    {
      logit("e","******%s() failed for %s, %s, %s.\n",
            "ewdb_api_CreateOrAlterExternalStation",SCN.Station.Sta,SCN.Station.Comp,SCN.Station.Net);
    }
    else
    {
      logit("e","%s() successful for %s.%s.%s %.6f %.6f %6.2f.\n",
            "ewdb_api_CreateOrAlterExternalStation",SCN.Station.Sta,SCN.Station.Comp,SCN.Station.Net,
                        SCN.Station.Lat, SCN.Station.Lon, SCN.Station.Elev);
    }
  }
  ewdb_api_Shutdown();
  return(0);
}


