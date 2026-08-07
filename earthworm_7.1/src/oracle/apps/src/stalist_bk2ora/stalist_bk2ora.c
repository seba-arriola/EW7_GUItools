 /*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: stalist_bk2ora.c,v 1.2 2002/11/03 21:30:38 lombard Exp $
 *
 *    Revision history:
 *     $Log: stalist_bk2ora.c,v $
 *     Revision 1.2  2002/11/03 21:30:38  lombard
 *     Corrected mode of fopen() call.
 *
 *     Revision 1.1  2002/05/01 22:01:11  dietz
 *     Initial revision
 *
 *
 *
 */
  
/* stalist_bk2ora: 
 *  This program is used to load channel name and location data
 *  from the UCB channel list file into an Earthworm DBMS.
 *  You can find the up-to-date UCB file online at:
 *
 *  ftp://quake.geo.berkeley.edu/pub/doc/BK.info/BK.channel.summary.day
 *
 *  This list contains the entire history. The currently active channels
 *  have an off date of 3000/01/01,00:00:00.  This program only loads
 *  currently active channels.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ewdb_ora_api.h>
#include <ewdb_apps_utils.h>

#define LOC_LEN          2
#define LINE_BUFFER_SIZE 256

char DBuser[50];
char DBpassword[50];
char DBservice[50];

void dbvar_init(int argc, char ** argv)
{
  if(argc < 5 )
  {
    printf("usage:  stalist_bk2ora <STATION_LIST_FILENAME> "
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
  int i,j,done=0,retval;
  int rc;
  int yearoff = 0;
  FILE * fptr;

  logit_init("stalist_bk2ora",0,1024,1); 
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

 /* Read the line: */ 
    rc = sscanf( pBuffer,"%s %s %s %s %*f %*s %d%*s %f %f %f",
                 SCN.Station.Sta,
                 SCN.Station.Net,
                 SCN.Station.Comp,
                 SCN.Station.Loc,
                 &yearoff,
                 &SCN.Station.Lat,
                 &SCN.Station.Lon,
                 &SCN.Station.Elev );

    if( rc != 8 ) {
        logit("e","Error reading this line:\n" );
        logit("e","%s\n", pBuffer );
    }

 /* make Null location code null string */
    if( SCN.Station.Loc[0] == '-' ) { 
        for(j=0;j<LOC_LEN;j++) SCN.Station.Loc[j]='\0';
    }
 
    if( yearoff < 3000 ) continue;  /* skip it; channel no longer active */

/*   logit("e", "%s %s %s %s  %.6f %.6f %.1f %d\n",
                 SCN.Station.Sta,
                 SCN.Station.Net,
                 SCN.Station.Comp,
                 SCN.Station.Loc,
                 SCN.Station.Lat,
                 SCN.Station.Lon,
                 SCN.Station.Elev, yearoff ); */ /*DEBUG*/

    if(ewdb_api_CreateOrAlterExternalStation(&SCN) == EWDB_RETURN_FAILURE)
    {
      logit("e","******%s() failed for %s %s %s %s.\n",
           "ewdb_api_CreateOrAlterExternalStation",
            SCN.Station.Sta,SCN.Station.Comp,SCN.Station.Net,SCN.Station.Loc);
    }
    else
    {
      logit("e","%s() successful for %s %s %s %s %.6f %.6f %.1f\n",
            "ewdb_api_CreateOrAlterExternalStation",
             SCN.Station.Sta,SCN.Station.Comp,SCN.Station.Net,SCN.Station.Loc,
             SCN.Station.Lat,SCN.Station.Lon,SCN.Station.Elev);
    }
 
  }
  ewdb_api_Shutdown();
  return(0);
}


