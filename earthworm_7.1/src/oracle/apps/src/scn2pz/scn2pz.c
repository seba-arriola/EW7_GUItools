
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: scn2pz.c,v 1.4 2001/07/01 21:55:34 davidk Exp $
 *
 *    Revision history:
 *     $Log: scn2pz.c,v $
 *     Revision 1.4  2001/07/01 21:55:34  davidk
 *     Cleanup of the Earthworm Database API and the applications that utilize it.
 *     The "ewdb_api" was cleanup in preparation for Earthworm v6.0, which is
 *     supposed to contain an API that will be backwards compatible in the
 *     future.  Functions were removed, parameters were changed, syntax was
 *     rewritten, and other stuff was done to try to get the code to follow a
 *     general format, so that it would be easier to read.
 *
 *     Applications were modified to handle changed API calls and structures.
 *     They were also modified to be compiler warning free on WinNT.
 *
 *     Revision 1.3  2001/05/15 02:15:45  davidk
 *     Moved functions around between the apps, DB API, and DB API INTERNAL
 *     levels.  Renamed functions and files.  Added support for amplitude
 *     magnitude types.  Reformatted makefiles.
 *
 *     Revision 1.2  2001/02/28 17:29:10  lucky
 *     Massive schema redesign and cleanup.
 *
 *     Revision 1.1  2001/02/21 17:27:19  lucky
 *     Initial revision
 *
 *
 *
 */

#include <ewdb_ora_api.h>
#include <earthworm.h>
#include <ewdb_apps_utils.h>
#include <parse_trig.h>  /* needed for t_atodbl() func prototype */

#define APP_MAXPATH 480
#define APP_MAXWORD 50
#define NUMIDCHANS 10


int ConvertTime(char *pStart, double *start);  /* scn2pz.c */
int ReadConfig(char *configfile);              /* config.c */

/* Database connection things */
char  DBservice[APP_MAXWORD];        /* DBMS instance to interact with    */
char  DBuser[APP_MAXWORD];           /* UserId to connect to database as  */
char  DBpassword[APP_MAXWORD];       /* Password to datasource            */

char  OutDir[APP_MAXPATH+1];
char  envEW_LOG[APP_MAXPATH+8];        /* where environment variable EW_LOG
                                        will be stored                  */

extern int errno;   /* system error variable */


main (int argc, char **argv)
{

	int						NumidChans, NumidChansFound,  i, idChan;
	EWDBid					*idChanBuf;
	double					pztime;
	EWDB_ChanTCTFStruct		ResponseInfo;
	char					filename[256];
	FILE					*fp;


	/* Introduce ourselves
	 **********************/
	if (argc != 6)
	{
		fprintf (stderr, "Usage: scn2pz <config_file> STA COMP NET time\n");		
		fprintf (stderr, "     time: specify 0 to get the latest PZ; OR,\n");
		fprintf (stderr, "     time: specify YYYYMMDDHHMM to get PZ for specific time.\n");
		exit (0);
	}
   
   /* Read the configuration file (path hardcoded relative to executable)
   *********************************************************************/
	ReadConfig (argv[1]); /* it exits if it isn't happy */

   /* Start up Logging
   *******************/
	logit_init ("scn2pz",1,1024,1);

	logit("","\n/*************************************\n"
	          "  Initializing ORA_API                 \n"
	          "*************************************/\n");

	/* Open connection to database
	*****************************/
	if( ewdb_api_Init (DBuser, DBpassword, DBservice) != 0 )
	{
		logit( "e", "Trouble connecting to database; exiting!\n" );
		exit (0);
	}

	logit ("t", "Connected to the DB!\n");

	/* convert start time */
	if (ConvertTime (argv[5], &pztime) != EW_SUCCESS)
	{
		logit ("e", "Call to ConvertTime failed.\n");
		ewdb_api_Shutdown();
		exit (0);
	}

	logit ("e","Retrieve PZ for %s.%s.%s at time %0.2f\n", 
					argv[2], argv[3], argv[4], pztime);


	idChanBuf = (EWDBid *) malloc (NUMIDCHANS * sizeof (EWDBid));


	/* Get idChan for this SCN */
	if (ewdb_api_GetidChansFromSCNLT (idChanBuf, argv[2], argv[3], argv[4],
                                    NULL, pztime, pztime, &NumidChansFound, 
                                    &NumidChans, NUMIDCHANS) 
      == EWDB_RETURN_FAILURE)
	{
		logit ("e", "Call to ewdb_api_GetidChansFromSCNLT failed\n");
		ewdb_api_Shutdown();
		exit (0);
	}

	if (NumidChans > 0)
	{
		/* If we got one or more idChans, use the last one */
		idChan = idChanBuf[NumidChans-1];
		logit ("", "Got idChan=%d, last of %d\n", idChan, NumidChans);

		/* Get response */
		if (ewdb_api_GetTransformFunctionForChan(idChan, (int)pztime, &(ResponseInfo))
   			== EWDB_RETURN_FAILURE)
		{
			logit ("e", "Call to GetTransformFunctionForChan failed.\n");
			exit (0);
		}

		sprintf (filename, "%s.%s.%s.pz", argv[2], argv[3], argv[4]);
		logit ("", "Got responses -- writing file %s.\n", filename);

		if ((fp = fopen (filename, "wt")) == NULL)
		{
			logit ("e", "Can't open PZ file %s\n", filename);
			ewdb_api_Shutdown();
			exit (0);
		}

		fprintf (fp, "POLES %d\n", ResponseInfo.tfsFunc.iNumPoles);
		for (i = 0; i < ResponseInfo.tfsFunc.iNumPoles; i++)
		{
			fprintf (fp, "%f %f\n", ResponseInfo.tfsFunc.Poles[i].dReal,
										ResponseInfo.tfsFunc.Poles[i].dImag);
		}

		fprintf (fp, "ZEROS %d\n", ResponseInfo.tfsFunc.iNumZeroes);
		for (i = 0; i < ResponseInfo.tfsFunc.iNumZeroes; i++)
		{
			fprintf (fp, "%f %f\n", ResponseInfo.tfsFunc.Zeroes[i].dReal,
										ResponseInfo.tfsFunc.Zeroes[i].dImag);
		}

		fprintf (fp, "CONSTANT %f\n", ResponseInfo.dGain);

		fclose(fp);

		logit ("e", "PZ file written: %s\n", filename);
	}
	else
	{
		logit ("e", "ERROR: Could not get any idChans for %s.%s.%s at time %0.2f\n",
							argv[2], argv[3], argv[4], pztime);
		ewdb_api_Shutdown();
		exit (0);
	}


	logit("t","scn2pz: terminating\n" );
	ewdb_api_Shutdown();

	return( 0 );

}  /* end main */


/***********************************************************************
 * ConvertTime () - given pStart return a double representing          *
 *     number of seconds since 1970                                    *
 ***********************************************************************/
int     ConvertTime (char *pStart, double *start)
{

    char    YYYYMMDD[9];
    char    HHMMSS[12];


	/* pStart: either 0 or YYYYMMDDHHMM */

    if (pStart == NULL)
    {
        logit ("", "Invalid parameters passed in.\n");
        return EW_FAILURE;
    }


	/* return current time */
	if (pStart[0] == '0')
	{
		*start = (double) time(NULL);
		return EW_SUCCESS;
	}

	/* convert time string */
    strncpy (YYYYMMDD, pStart, (size_t) 8);
        YYYYMMDD[8] = '\0';

    HHMMSS[0] = pStart[8];
    HHMMSS[1] = pStart[9];
    HHMMSS[2] = ':';
    HHMMSS[3] = pStart[10];
    HHMMSS[4] = pStart[11];
    HHMMSS[5] = ':';
    HHMMSS[6] = '0';
    HHMMSS[7] = '0';
    HHMMSS[8] = '.';
    HHMMSS[9] = '0';
    HHMMSS[10] = '0';
    HHMMSS[11] = '\0';


    if (t_atodbl (YYYYMMDD, HHMMSS, start) < 0)
    {
        logit ("", "Can't convert StartTime %s -> %s %s: t_atodbl failed.\n",
                        pStart, YYYYMMDD, HHMMSS);
        return EW_FAILURE;
    }

    return EW_SUCCESS;

}

