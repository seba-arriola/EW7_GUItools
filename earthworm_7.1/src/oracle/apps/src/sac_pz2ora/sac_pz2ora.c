#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include <kom.h>

#include <time_functions.h>
#include <ewdb_ora_api.h>
#include <ewdb_apps_utils.h>
#include <transfer.h>

/* External functions */
void   logit_init( char *prog, short mid, int bufSize, int logflag );
void   logit( char *, char *, ... );


int main(int argc, char ** argv)
{

	ResponseStruct				Resp;
	int 						RetCode, i;
  	char 						tmpstr[10], szSta[10], szComp[10], szNet[10];
	char						*t;
	time_t 						tStart;
	EWDBid 						idChan;
	EWDB_External_StationStruct Station;
	EWDB_ChanTCTFStruct 		cctfsFunction;

	if (argc != 5)
	{
		printf("Usage: sac_pz2ora <SAC_PZ FILENAME> <DBUSER> <DBPASSWD> <DBSID>\n");
		return(-1);
	}


	/* 
	 * Extract station, channel and network code from 
	 * the filename of the format sta.chan.net.resp
	 */
	strcpy (tmpstr, argv[1]);
	t = strrchr (tmpstr, '.');	/* find the last . */
	*t = '\0'; /* this is the .resp part -- ignore */

	t = strrchr (tmpstr, '.');	/* find the last . */
	strcpy (szNet, t+1);
	for (i = 0; i < strlen (szNet); i++)
		szNet[i] = toupper (szNet[i]);

	*t = '\0';
	t = strrchr (tmpstr, '.');	/* find the last . */
	strcpy (szComp, t+1);
	for (i = 0; i < strlen (szComp); i++)
		szComp[i] = toupper (szComp[i]);

	*t = '\0';
	strcpy (szSta, tmpstr);
	for (i = 0; i < strlen (szSta); i++)
		szSta[i] = toupper (szSta[i]);

	logit_init ("sac_pz2ora", (short)1, 1024, 1);

	if (ewdb_api_Init (argv[2], argv[3], argv[4]) == EWDB_RETURN_FAILURE)
	{
		logit("","Failed to connect to DB using User:%s, SID:%s\n", argv[2], argv[4]);
		return (-1);
	}

	/* Read the PZ File -- Convert to ResponseStruct */
	RetCode = readPZ (argv[1], &Resp);

	if (RetCode != 0)
	{
		logit ("e", "Call to readPZ failed, RetCode = %d\n", RetCode);
		return -1;
	}


	/* Convert to Cooked infrastructure structure to be inserted */

	cctfsFunction.dGain = Resp.dGain;
	cctfsFunction.tfsFunc.iNumPoles = Resp.iNumPoles;
	cctfsFunction.tfsFunc.iNumZeroes = Resp.iNumZeros;

	for (i = 0; i < Resp.iNumPoles; i++)
	{
		cctfsFunction.tfsFunc.Poles[i].dReal = Resp.Poles[i].dReal;
		cctfsFunction.tfsFunc.Poles[i].dImag = Resp.Poles[i].dImag;
	}

	for (i = 0; i < Resp.iNumZeros; i++)
	{
		cctfsFunction.tfsFunc.Zeroes[i].dReal = Resp.Zeros[i].dReal;
		cctfsFunction.tfsFunc.Zeroes[i].dImag = Resp.Zeros[i].dImag;
	}




	/* we need to get idChan and insert the record into the DB */
	logit ("e", "Inserting function for %s,%s,%s\n", szSta, szComp, szNet);

	memset (&Station, 0, sizeof(Station));
	strcpy (Station.Station.Sta, szSta);
	strcpy (Station.Station.Comp, szComp);
	strcpy (Station.Station.Net, szNet);
	sprintf(cctfsFunction.tfsFunc.szCookedTFDesc, "%s,%s,%s", szSta,szComp,szNet);

	if (ewdb_api_CreateOrAlterExternalStation (&Station) == EWDB_RETURN_FAILURE)
	{
		logit("","Call to ewdb_api_CreateOrAlterExternalStation failed.\n"); 
		return -1;
	}

	if (ewdb_api_GetIdChanFromStationExternal (&idChan, Station.StationID) == EWDB_RETURN_FAILURE)
	{
		logit("","Call to ewdb_api_GetIdChanFromStationExternal failed.\n");
		return -1;
	}

	if (ewdb_api_CreateTransformFunction(&(cctfsFunction.tfsFunc.idCookedTF),
										&(cctfsFunction.tfsFunc)) == EWDB_RETURN_FAILURE)
	{
		logit("", "Call to ewdb_api_CreateTransformFunction failed.\n");
		return -1;
	}

	tStart = time (NULL);
	if (ewdb_api_GetidChanT(idChan, tStart, &(cctfsFunction.idChanT)) == EWDB_RETURN_FAILURE)
	{
		logit("","Call to ewdb_api_GetidChanT failed .\n");
		return -1;
	}

	cctfsFunction.dSampRate = 100.00;
	if (ewdb_api_SetTransformFuncForChanT (cctfsFunction.idChanT, cctfsFunction.dGain,
				cctfsFunction.tfsFunc.idCookedTF, cctfsFunction.dSampRate) == EWDB_RETURN_FAILURE)
	{
		logit("","Call to ewdb_api_SetTransformFuncForChanT failed. \n"); 
		return -1;
	}


	logit ("e", "Successfuly inserted poles and zeroes for (%s,%s,%s)\n", szSta,szComp,szNet);
  
	return(0);
}

