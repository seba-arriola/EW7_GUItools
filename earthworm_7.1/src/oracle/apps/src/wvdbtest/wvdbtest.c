/* wvdbtest.c */
/*  This is a program to test the EWDB API
 *  waveform retrieval functions! 
 *
 *  adapted from smdbtest 20000105 davidk *
 *****************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ewdb_ora_api.h>
#include <ewdb_apps_utils.h>
#include <trace_buf.h>

#define BUFLEN 100
#define DBSTRLEN 30
#define MAX_SNIPPETLENGTH 60000

int print_waveforms(EWDB_WaveformStruct *pWaveforms, int NumWaveforms);
int print_waveforms_and_stations(EWDB_WaveformStruct *pWaveforms, 
                                  EWDB_StationStruct * pStations,
                                  int NumWaveforms);

void logit_init( char *, short, int, int ); /* logit.c      sys-independent  */
void logit( char *, char *, ... );          /* logit.c      sys-independent  */

int main(int argc, char ** argv)
{

  char szDBUser[DBSTRLEN],szDBPassword[DBSTRLEN],szDBService[DBSTRLEN];
  EWDBid idEvent;
  int rc;
  EWDB_StationStruct pStations[BUFLEN];
  EWDB_WaveformStruct pWaveforms[BUFLEN];
  int WDsFound,WDsRetrieved;
  char pSnippets[BUFLEN][MAX_SNIPPETLENGTH];
  int i;

  if(argc != 5)
  {
    printf("USAGE: wvdbtest <DBUSER> <DBPASSWD> <DBSERVICEID> <DBIDEVENT>\n");
    return(-1);
  }

  strcpy(szDBUser,argv[1]);
  strcpy(szDBPassword,argv[2]);
  strcpy(szDBService,argv[3]);
  idEvent=atoi(argv[4]);

  logit_init( "wvdbtest", 0, 512, 1 );

  logit("","retrieving waveforms.\n");
  printf("retrieving waveforms.\n");


  /******************************************************
  INITIALIZE THE DB API
  ******************************************************/
  rc = ewdb_api_Init(szDBUser,szDBPassword,szDBService);
  if (rc!= EWDB_RETURN_SUCCESS)
  {
    printf("ewdb_api_Init() failed with retcode %d!\n",rc);
    logit("","ewdb_api_Init() failed with retcode %d!\n",rc);
    return(-2);
  }

  
  /******************************************************
  GET WAVEFORMS W/ STATION INFO
  ******************************************************/
  rc=ewdb_api_GetWaveformListByEvent(idEvent,pWaveforms,pStations,
                                     TRUE/*bIncludeStationInfo*/,
                                     &WDsFound,&WDsRetrieved,BUFLEN);

  if (rc== EWDB_RETURN_FAILURE)
  {
    printf("ewdb_api_GetWaveformListByEvent() failed with retcode %d!\n",rc);
    logit("","ewdb_api_GetWaveformListByEvent() failed with retcode %d!\n",rc);
    return(-3);
  }

  printf("WDsFound:%d  WDsRetrieved:%d\n",WDsFound,WDsRetrieved);
  logit("","WDsFound:%d  WDsRetrieved:%d\n",WDsFound,WDsRetrieved);

  /* Retrieve the actual snippets Max MAX_SNIPPETS */
  for(i=0;i<WDsRetrieved;i++)
  {
    pWaveforms[i].binSnippet=pSnippets[i];
    rc=ewdb_api_GetWaveformSnippet(pWaveforms[i].idWaveform, 
                               pWaveforms[i].binSnippet,
                               pWaveforms[i].iByteLen);
    if (rc == EWDB_RETURN_FAILURE)
    {
      printf("ewdb_api_GetWaveformSnippet() failed with retcode %d! for %d(%d)\n",
             rc,pWaveforms[i].idWaveform,i);
      logit("","ewdb_api_GetWaveformSnippet() failed with retcode %d! for %d(%d)\n",
             rc,pWaveforms[i].idWaveform,i);
      return(-6);
    }
    printf("First record of Snippet[i]: Pin:%d, Samps:%d,Start:%14.4f\n",
           *(int *)(pWaveforms[i].binSnippet),
           *(int *)((pWaveforms[i].binSnippet)+4),
           *(double *)((pWaveforms[i].binSnippet)+8)
          );

  }

  /* Print the waveforms w/ station info */
  printf("Calling print_waveforms_and_stations.\n");
  print_waveforms_and_stations(pWaveforms,pStations,WDsRetrieved);


  /******************************************************
  GET WAVEFORMS W/O STATION INFO
  ******************************************************/
  rc=ewdb_api_GetWaveformListByEvent(idEvent,pWaveforms,pStations,
                                     FALSE/*bIncludeStationInfo*/,
                                     &WDsFound,&WDsRetrieved,BUFLEN);

  if (rc!= EWDB_RETURN_SUCCESS)
  {
    printf("ewdb_api_GetWaveformListByEvent() failed with retcode %d!\n",rc);
    logit("","ewdb_api_GetWaveformListByEvent() failed with retcode %d!\n",rc);
    return(-4);
  }

  printf("WDsFound:%d  WDsRetrieved:%d\n",WDsFound,WDsRetrieved);
  logit("","WDsFound:%d  WDsRetrieved:%d\n",WDsFound,WDsRetrieved);

  /* Print the waveforms w/o station info */
  printf("Calling print_waveforms_and_stations.\n");
  print_waveforms(pWaveforms,WDsRetrieved);

  return( 0 );
}

int print_waveforms(EWDB_WaveformStruct *pWaveforms, int NumWaveforms)
{
  int i;

  printf("%10s %10s %14.4s %14.4s %6s %10s\n",
        "idWaveform","idChan","tStart","tEnd",
        "Format","ByteLen");
  for(i=0;i < NumWaveforms; i++)
  {
    printf("%10d %10d %14.4f %14.4f %6d %10d\n",
          pWaveforms[i].idWaveform,pWaveforms[i].idChan,
          pWaveforms[i].tStart,pWaveforms[i].tEnd,
          pWaveforms[i].iDataFormat,pWaveforms[i].iByteLen);
  }

  return(0);
}

int print_waveforms_and_stations(EWDB_WaveformStruct *pWaveforms, 
                                  EWDB_StationStruct * pStations,
                                  int NumWaveforms)
{
  int i;
  TRACE_HEADER * ptb;

  printf("%10s %10s (%4s,%4s,%4s) %14.4s %14.4s %6s %10s\n",
        "idWaveform","idChan","Sta","Comp","Net","tStart","tEnd",
        "Format","ByteLen");
  for(i=0;i < NumWaveforms; i++)
  {
    printf("%10d %10d (%4s,%4s,%4s) %14.4f %14.4f %6d %10d\n",
          pWaveforms[i].idWaveform,pWaveforms[i].idChan,
          pStations[i].Sta,pStations[i].Comp,pStations[i].Net,
          pWaveforms[i].tStart,pWaveforms[i].tEnd,
          pWaveforms[i].iDataFormat,pWaveforms[i].iByteLen);
    if(pWaveforms[i].binSnippet && (pWaveforms[i].iDataFormat == 1))
    {
      ptb=(TRACE_HEADER *) pWaveforms[i].binSnippet;
      printf(" %20s (%4s,%4s,%4s) %14.4f %14.4f %6s %4d %5.2f\n",
             "",ptb->sta,ptb->chan,ptb->net,
             ptb->starttime,ptb->endtime,
             ptb->datatype,ptb->nsamp,ptb->samprate
            );
    }
  }

  return(0);
}

