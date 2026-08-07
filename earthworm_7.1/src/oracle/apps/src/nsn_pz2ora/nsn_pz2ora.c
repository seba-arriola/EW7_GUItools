/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: nsn_pz2ora.c,v 1.1 2004/03/17 18:43:19 davidk Exp $
 *
 *    Revision history:
 *     $Log: nsn_pz2ora.c,v $
 *     Revision 1.1  2004/03/17 18:43:19  davidk
 *     Initial revision
 *
 *     Revision 1.8  2001/07/20 17:42:55  davidk
 *     Added RCS header to record comments in source file.
 *
 *     Revision 1.3  2001/07/05 20:43:28 
 *
 *
 ****************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include <kom.h>

#include <time_functions.h>
#include <ewdb_ora_api.h>
#include <ewdb_apps_utils.h>

#define CALIB_STRING "CAL2"
#define PZ_STRING "PAZ2"
#define DT_STRING "DATA_TYPE"
#define MAX_WORD_LEN 40

#define PZ_PARSE_STATE_NONE 0
#define PZ_PARSE_STATE_CALIB_STRING 1
#define PZ_PARSE_STATE_PZ_STRING 2
#define PZ_PARSE_STATE_DATA_TYPE 3

/* External functions */
void   logit_init( char *prog, short mid, int bufSize, int logflag );
void   logit( char *, char *, ... );



int main(int argc, char ** argv)
{
  int RetCode;
  char * szDRMFile=argv[1];
  int iParseState=0;
  char * szLineType;
  char szSta[10],szComp[10],szNet[10];
  char szSensorType[10];
  double dSensitivity,dRefPeriod;
  char * szTemp, * szTemp2;
  char szTempTime[MAX_WORD_LEN];
  char * szDBUser=argv[2], * szDBPassword=argv[3], * szDBService=argv[4];
  time_t tStart;
  
  char MY_PROG_NAME[MAX_WORD_LEN];
  int iNumPolesRcvd,iNumZeroesRcvd;
  EWDB_ChanTCTFStruct cctfsFunction;
  
  EWDB_External_StationStruct Station;
  EWDBid idChan;

  if(argc!=5)
  {
    printf("Usage: nsn_pz2ora <AUT-DRM FILENAME> <DBUSER> <DBPASSWD> <DBSID>\n");
    return(-1);
  }

  memset(&cctfsFunction,0,sizeof(cctfsFunction));
  
  strcpy(MY_PROG_NAME,"nsn_pz2ora.d");
  MY_PROG_NAME[strlen(MY_PROG_NAME)-2]=0;  /* erase the ".d" */
  
  
  szTemp = getenv( "EW_LOG" );
  
  if(szTemp == NULL)
  {
    putenv("EW_LOG=j:\\src\\usgs\\home\\working\\apps\\src\\nsn_pz2ora\\");
  }
  
  logit_init(MY_PROG_NAME,(short)1/*MyModID*/,1024,1);

  RetCode=ewdb_api_Init(szDBUser,szDBPassword,szDBService);
  if(RetCode == EWDB_RETURN_FAILURE)
  {
    logit("","Failed to connect to DB using User:%s, SID:%s\n",
          szDBUser,szDBService);
    return(-1);
  }
  /* Open the PZ File
  **********************************/
  RetCode = k_open( szDRMFile ); 
  if ( RetCode == 0 ) 
  {
    fprintf( stdout,
      "main(): Error opening PZ file <%s>; exiting!\n", 
      szDRMFile );
    return(-1 );
  }
  
  while(k_rd())    /* Read next line from file  */
  {
    szLineType=k_str(); /* get the first token, which describes the line type */
    if(szLineType)
    {
      if(!strcmp(szLineType,CALIB_STRING))
      {
        if(iParseState && iParseState != PZ_PARSE_STATE_DATA_TYPE)
        {
          logit("","main():ERROR!!  %s line found with a parse state of %d.\n",
            CALIB_STRING,iParseState);
          logit("","Error in line %s.\n",k_com());
          iParseState=0;
          continue;
        }
        iParseState = PZ_PARSE_STATE_CALIB_STRING;
        strcpy(szSta,k_str());
        strcpy(szComp,k_str());
        strcpy(szNet,k_str());
        strcpy(szSensorType,k_str());
        dSensitivity=k_val();
        dRefPeriod=k_val();
        cctfsFunction.dSampRate=k_val();
        szTemp=k_str();
        szTemp2=k_str();
        /* read in the date and time strings, and then
        concatenate them together with some additional
        grammar in order to compose a string of the form
        YYYY/MM/DD, HH:MM:SS */
        sprintf(szTempTime,"%s, %s:00",szTemp,szTemp2);
        tStart=EWDB_atot(szTempTime);
      }
      else if(!strcmp(szLineType,PZ_STRING))
      {
        if(iParseState != PZ_PARSE_STATE_CALIB_STRING)
        {
          logit("","main():ERROR!!  %s line found with a parse state of %d.\n",
            PZ_STRING,iParseState);
          logit("","Error in line %s.\n",k_com());
          iParseState=0;
          continue;
        }
        iParseState = PZ_PARSE_STATE_PZ_STRING;
        szTemp=k_str(); /* 1 */
        szTemp=k_str(); /* C */
        cctfsFunction.dGain=k_val();
        cctfsFunction.tfsFunc.iNumPoles=k_int();
        cctfsFunction.tfsFunc.iNumZeroes=k_int();
        szTemp=k_str(); /* sensor type again */
        iNumPolesRcvd=0;
        iNumZeroesRcvd=0;
      }
      else if(!strcmp(szLineType,DT_STRING))
      {
        if(iParseState && iParseState != PZ_PARSE_STATE_DATA_TYPE)
        {
          logit("","main():ERROR!!  %s line found with a parse state of %d.\n",
            DT_STRING,iParseState);
          logit("","Error in line %s.\n",k_com());
          iParseState=0;
          continue;
        }
        iParseState = PZ_PARSE_STATE_DATA_TYPE;
        logit("","%s\n",k_com());
      }
      else
      {
        if(iParseState==PZ_PARSE_STATE_CALIB_STRING)
        {
          logit("","main():ERROR!!  line found with a parse state of %d.\n",
            iParseState);
          logit("","Error in line %s.\n",k_com());
          iParseState=0;
          continue;
        }
        else if(iParseState == PZ_PARSE_STATE_PZ_STRING)
        {
          /* This should be a poles or zeroes line.*/
          if(iNumPolesRcvd < cctfsFunction.tfsFunc.iNumPoles)
          {
            /* it's a pole */
            cctfsFunction.tfsFunc.Poles[iNumPolesRcvd].dReal=atof(szLineType);
            cctfsFunction.tfsFunc.Poles[iNumPolesRcvd].dImag=k_val();
            iNumPolesRcvd++;
          }
          else if (iNumZeroesRcvd < cctfsFunction.tfsFunc.iNumZeroes)
          {
            /* it's a zero */
            cctfsFunction.tfsFunc.Zeroes[iNumZeroesRcvd].dReal=atof(szLineType);
            cctfsFunction.tfsFunc.Zeroes[iNumZeroesRcvd].dImag=k_val();
            iNumZeroesRcvd++;
          }
          else
          {
            logit("","main():ERROR!!  unexpected line found after "
              "parsing %d poles and %d zeroes.\n",
              iNumPolesRcvd,iNumZeroesRcvd);
            logit("","Error in line %s.\n",k_com());
            iParseState=0;
            continue;
          }
        }
        else if(iParseState==PZ_PARSE_STATE_DATA_TYPE)
        {
          logit("","Line: %s.\n",k_com());
        }
        else
        {
          logit("","main():ERROR!!  line found with a parse state of %d.\n",
            iParseState);
          logit("","Error in line %s.\n",k_com());
          iParseState=0;
          continue;
        }  /* (iParseState== XXX) No recognized token */
      } /* (strcmp(szLineType,XXX)) Do we recognize the first token */
    }  /* if(szLineType) */
    else
    {
      /* blank line */
      if (iParseState == PZ_PARSE_STATE_PZ_STRING)
      {
        if(   iNumPolesRcvd == cctfsFunction.tfsFunc.iNumPoles
          && iNumZeroesRcvd == cctfsFunction.tfsFunc.iNumZeroes
          )
        {
          /* we need to get idChan and insert the record into the DB */

          logit("","Inserting function for %s,%s,%s\n",
                szSta,szComp,szNet);
          memset(&Station,0,sizeof(Station));
          strcpy(Station.Station.Sta,szSta);
          strcpy(Station.Station.Comp,szComp);
          strcpy(Station.Station.Net,szNet);
          sprintf(cctfsFunction.tfsFunc.szCookedTFDesc,"%s - %s,%s,%s",
                  szSensorType,szSta,szComp,szNet);

          RetCode=ewdb_api_CreateOrAlterExternalStation (&Station);
          if(RetCode == EWDB_RETURN_FAILURE)
          {
            logit("","EWDB_CreateOrAlterUsnsnStation() failed for %s,%s,%s\n",
                  szSta,szComp,szNet);
            goto pz_blank_line_init;
          }
          RetCode=ewdb_api_GetIdChanFromStationExternal(&idChan, Station.StationID);
          if(RetCode == EWDB_RETURN_FAILURE)
          {
            logit("","EWDB_GetidChanFromStationExternal() failed for %s,%s,%s\n",
                  szSta,szComp,szNet);
            goto pz_blank_line_init;
          }
          RetCode=ewdb_api_CreateTransformFunction(&(cctfsFunction.tfsFunc.idCookedTF),
                                               &(cctfsFunction.tfsFunc));
          if(RetCode == EWDB_RETURN_FAILURE)
          {
            logit("","ewdb_api_CreateTransformFunction() failed for %s,%s,%s\n",
                  szSta,szComp,szNet);
            goto pz_blank_line_init;
          }
          RetCode=ewdb_api_GetidChanT(idChan, tStart, &(cctfsFunction.idChanT));
          if(RetCode == EWDB_RETURN_FAILURE)
          {
            logit("","ewdb_api_GetidChanT() failed for %s,%s,%s,%.2f\n",
                  szSta,szComp,szNet,tStart);
            goto pz_blank_line_init;
          }
          RetCode=ewdb_api_SetTransformFuncForChanT
                      (cctfsFunction.idChanT,
                       cctfsFunction.dGain,
                       cctfsFunction.tfsFunc.idCookedTF, 
                       cctfsFunction.dSampRate);
          if(RetCode == EWDB_RETURN_FAILURE)
          {
            logit("","ewdb_api_SetTransformFuncForChanT() failed for %s,%s,%s,%.2f\n",
                  szSta,szComp,szNet,tStart);
            goto pz_blank_line_init;
          }
        }
        printf("Successfuly inserted poles and zeroes for (%s,%s,%s)\n",
               szSta,szComp,szNet);

        pz_blank_line_init:
          memset(&cctfsFunction,0,sizeof(cctfsFunction));
          iNumPolesRcvd=iNumZeroesRcvd=0;
          szSta[0]=szComp[0]=szNet[0]=0;
      }
      if (iParseState != PZ_PARSE_STATE_DATA_TYPE)
      {
        iParseState=PZ_PARSE_STATE_NONE;
      }
    }
    k_err();  /* clear the error value */
  }  /* while(k_rd())  while there are still lines in the file */
  
  return(0);
}

