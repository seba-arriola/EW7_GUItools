
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: get_transfunc.c,v 1.5 2001/07/20 17:32:49 davidk Exp $
 *
 *    Revision history:
 *     $Log: get_transfunc.c,v $
 *     Revision 1.5  2001/07/20 17:32:49  davidk
 *     Added a usage message, and changed behavior when EW_LOG is not specified.
 *
 *     Revision 1.4  2001/05/15 02:15:19  davidk
 *     Moved functions around between the apps, DB API, and DB API INTERNAL
 *     levels.  Renamed functions and files.  Added support for amplitude
 *     magnitude types.  Reformatted makefiles.
 *
 *     Revision 1.3  2001/02/28 17:29:10  lucky
 *     Massive schema redesign and cleanup.
 *
 *     Revision 1.2  2000/02/15 19:25:52  lucky
 *     *** empty log message ***
 *
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include <ewdb_ora_api.h>
#include <ewdb_apps_utils.h>
#include <time_functions.h>

/* External functions */
void   logit_init( char *prog, short mid, int bufSize, int logflag );
void   logit( char *, char *, ... );

#define MAX_WORD_LEN 40


int main(int argc, char ** argv)
{
  int RetCode;
  char * szDBUser=argv[2], * szDBPassword=argv[3], * szDBService=argv[4];
  time_t tStart;
  char MY_PROG_NAME[MAX_WORD_LEN];
  EWDB_ChanTCTFStruct cctfsFunction;
  EWDBid idChan;
  char * szTemp;
  int i;
  char szTime[MAX_WORD_LEN];

  idChan=atoi(argv[1]);
  tStart=time(NULL);

  if(argc < 5)
  {
    fprintf(stderr, "USAGE: get_transfunc <idChan> <DBUser> <DBPassword> <DBService> [<time>]\n"
                    "\n"
                    "      idChan is the channel for which the caller wishes to retrieve a \n"
                    "       transfer function\n"
                    "      DBUser is the Database User to use to access the DB.\n"
                    "      DBPassword is the password of DBUser.\n"
                    "      DBService is the service ID of the database.\n"
                    "      time(optional) is the time for which the caller wants the transfer\n"
                    "       function for the channel.  If no time is given, then the desired\n"
                    "       time is assumed to be Now.  (seconds since 1970).\n\n"
            );
  }

  if(argc > 5)
  {
    tStart=atoi(argv[5]);
  }


  memset(&cctfsFunction,0,sizeof(cctfsFunction));
  
  strcpy(MY_PROG_NAME,"get_transfunc.d");
  MY_PROG_NAME[strlen(MY_PROG_NAME)-2]=0;  /* erase the ".d" */
  
  
  szTemp = getenv( "EW_LOG" );
  
  if(szTemp == NULL)
  {
    fprintf(stderr,"EW_LOG environment variable not set, cannot initialize "
                   "logging.  Exiting!\n");
    return(-1);
  }

  szTemp = getenv( "TZ" );
  
  if(szTemp == NULL)
  {
    putenv("TZ=UTC");
  }
  
  logit_init(MY_PROG_NAME,(short)1/*MyModID*/,1024,1);

  RetCode=ewdb_api_Init(szDBUser,szDBPassword,szDBService);
  if(RetCode == EWDB_RETURN_FAILURE)
  {
    logit("","Failed to connect to DB using User:%s, SID:%s\n",
          szDBUser,szDBService);
    return(-1);
  }

  RetCode=ewdb_api_GetTransformFunctionForChan (idChan,tStart,&cctfsFunction);
  if(RetCode == EWDB_RETURN_FAILURE)
  {
    logit("","ewdb_api_GetTransformFunctionForChan() failed for %d\n",
          idChan);
    return(-1);
  }

  printf("Transform Function for idChan %d at time %s\n",
         idChan,EWDB_ttoa(&tStart,szTime));
  printf("%s\n",cctfsFunction.tfsFunc.szCookedTFDesc);
  printf("Gain:              %.4E\n",cctfsFunction.dGain);
  printf("Sample Rate:       %.4f\n",cctfsFunction.dSampRate);
  printf("Poles:\n");
  for(i=0;i<cctfsFunction.tfsFunc.iNumPoles;i++)
    printf("\t%20.4f\t%20.4f\n",cctfsFunction.tfsFunc.Poles[i]);
  printf("Zeroes:\n");
  for(i=0;i<cctfsFunction.tfsFunc.iNumZeroes;i++)
    printf("\t%20.4f\t%20.4f\n",cctfsFunction.tfsFunc.Zeroes[i]);
  printf("<EOF>\n");
  return(0);
}
