/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: concierge_api_utils.c,v 1.2 2002/07/16 20:47:31 davidk Exp $
 *
 *    Revision history:
 *     $Log: concierge_api_utils.c,v $
 *     Revision 1.2  2002/07/16 20:47:31  davidk
 *     Added stub for ewdb_apps_PreprocessSnippetReqs(), a function to check snippet requests
 *     before they are inserted into the database
 *     for the purpose of eliminating overlapping snippets.
 *
 *     Revision 1.1  2002/07/16 19:33:34  davidk
 *     Initial revision
 *
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <chron3.h>
#include <earthworm.h>
#include <trace_buf.h>
#include <read_arc.h>
#include <kom.h>
#include <transport.h>
#include <ewdb_ora_api.h> 
#include <ew_event_info.h> 
#include "concierge.h"

static double Get_P_time( double ot, double depth, double dist, 
			double pTT[NDMAX][NZMAX], int Nz, int Nd, double Ddepth, double Ddist);

static double Get_S_time( double ot, double depth, double dist, 
			double sTT[NDMAX][NZMAX], int Nz, int Nd, double Ddepth, double Ddist);
static      int sonOfSiteRead(char *name);
static  int readTTval(double* val);
static int match( char* sta, char* comp, char* net, EWDB_StationStruct* staStruct);


static 	EWDB_StationStruct	StationList[MAX_STA];
static	int					numStalistRet;


/***************************************************************************
   Derived from arc2trig and arc2trig2.  This function produces a list of 
   snippet requests in the list of SNIPPET structures (from parse_trig.h). 
   These can then be passed on to the concierge api call to be loaded 
   into the db.

   The configuration options to be used in determining which snippets to  
   requests are stored in the TraceRequestOptions structure passed in.

   The function will return at most NumSnipAlloc snippet requests, with the
   number of snippets saved in NumSnipRet.

***************************************************************************/

/* this was the code pre 2002/07/02  
   DavidK 
   ************************************/


/* 
Real crude and conservative: We're only in the business of making 
sure we compute a large enough snippet. So for P, get the next smaller 
P value from the table for this depth and distance 
*/
static	double Get_P_time (double ot, double depth, double dist, 
			double pTT[NDMAX][NZMAX], int Nz, int Nd, double Ddepth, double Ddist)
{ 
	int iz,id;
	iz= (int)(depth/Ddepth);
	id= (int)(dist/Ddist);
	if(iz>=Nz) iz=Nz-1;
	if(id>=Nd) id=Nd-1;
	return (ot + pTT[id][iz]);
}


static double Get_S_time( double ot, double depth, double dist, 
			double sTT[NDMAX][NZMAX], int Nz, int Nd, double Ddepth, double Ddist)
{ 
/* And by the same logic, get the next larger value for S */
	int iz,id;
	iz= (int)(depth/Ddepth);
	id= (int)(dist/Ddist);
	if(iz>=Nz) iz=Nz-1;
	if(id>=Nd) id=Nd-1;
	return(ot + sTT[id][iz]);  
}





int	 	RunConciergeConfig (TraceRequestOptions *pOptions)
{
	
	int 	id,iz, ret;
	char	*str;

	if (pOptions == NULL)
	{
		logit ("", "Invalid arguments passed in.\n");
		return EW_FAILURE;
	}


	ret = 1;  /* 1- not found, 0- found, -1- error */
	
	/* Process command looking for concierge specific stuff 
	*******************************************************/
	if( k_its("DBLatRange") ) 
	{
		pOptions->StaLatMin = k_val(); 
		pOptions->StaLatMax = k_val();
		pOptions->StalistFromDB = 1;
		ret = 0;
	}
	else if( k_its("DBLonRange") ) 
	{
		pOptions->StaLonMin = k_val(); 
		pOptions->StaLonMax = k_val();
		pOptions->StalistFromDB = 1;
		ret = 0;
	}
	/* Is there a station list
	**************************/
	else if( k_its("StaListFileName") ) 
	{
		str=k_str();
		if(str) 
			strcpy(pOptions->StationListFile, str);
		pOptions->StalistFromDB = FALSE;
		ret = 0;
	}			
			
	else if( k_its("SaveOriginDist") ) 
	{
		pOptions->SaveOriginDist = k_val();
		ret = 0;
	}

	/* Mandatory stations to include
	********************************/
	else if(k_its("Channel") ) 
	{
		if((str = k_str()) != NULL) 
			strcpy( pOptions->MandSta[pOptions->numMand].Sta,str);

		if((str = k_str()) != NULL) 
			strcpy( pOptions->MandSta[pOptions->numMand].Comp,str);

		if((str = k_str()) != NULL) 
			strcpy( pOptions->MandSta[pOptions->numMand].Net,str);

		pOptions->numMand++;

		if(pOptions->numMand == MAX_MAND_STA)
		{
			logit ("e", "Error. More than %d mandatory channels specified. Exiting\n",
									MAX_MAGDIST);
			return -1;
		}
		ret = 0;
	}

	/* Magnitude/Distance table
	***************************/
	else if(k_its("MagDist") ) 
	{
		pOptions->Mag[pOptions->numMagDist] = k_val();
		pOptions->Dist[pOptions->numMagDist]=k_val();

		pOptions->numMagDist++;

		if (pOptions->numMagDist == MAX_MAGDIST)
		{
			logit ("e", "Error. More than %d MagDist specified. Exiting\n",MAX_MAGDIST);
			return -1;
		}
		ret = 0;
	}

	else if( k_its("PreTime") ) 
	{
		pOptions->PrePickTime = k_val();
		ret = 0;
	}
	else if( k_its("PostTime") ) 
	{
		pOptions->PostPickTime = k_val();
		ret = 0;
	}

	else if( k_its("FixedDuration") ) 
	{
		pOptions->FixedDuration = k_val();
		ret = 0;
	}
	/* Read the travel time table
	*****************************/
	else if( k_its("Nz") ) 
	{
		pOptions->numZ = k_int();
		if(pOptions->numZ > NZMAX) 
		{
			logit ("e", "Too many depth values:%d. max=%d; exiting!\n", 
								pOptions->numZ, NZMAX);
			return -1;
		}
		ret = 0;
	}

	else if( k_its("Nd") ) 
	{
		pOptions->numDist = k_int();
		if(pOptions->numDist > NDMAX) 
		{
			logit ("e", "Too many distance values:%d. max=%d; exiting!\n", 
							pOptions->numDist,NDMAX);
			return -1;
		}
		ret = 0;
	}

	else if( k_its("Ddepth") ) 
	{
		pOptions->deltaZ = k_val();
		ret = 0;
	}
	else if( k_its("Ddist") ) 
	{
		pOptions->deltaDist = k_val();
		ret = 0;
	}
	else if( k_its("Pphase") ) 
	{
		for (id=0;id<pOptions->numDist;id++)
		{
			for(iz=0;iz<pOptions->numZ;iz++)
			{
				if(readTTval( &(pOptions->PTT[id][iz]) )<0)
				{
					logit ("e", "Error reading P travel time table; exit.\n");
					return -1;
				}
			}
		}
		ret = 0;
	}

	else if( k_its("Sphase") ) 
	{
		for(id=0;id<pOptions->numDist;id++)	
		{
			for(iz=0;iz<pOptions->numZ;iz++)
			{
				if(readTTval( &(pOptions->STT[id][iz]) )<0)
				{
					logit ("e", "Error reading S travel time table; exit.\n");
					return -1;
				}
			}
		}
		ret = 0;
	}
			
   return ret;
}


int	 	InitConciergeConfig (TraceRequestOptions *pOptions)
{

	if (pOptions == NULL)
	{
		logit ("", "Invalid arguments passed in.\n");
		return EW_FAILURE;
	}

	pOptions->StalistFromDB = 0;
	pOptions->numMand = 0;
	pOptions->numMagDist = 0;
	pOptions->SaveOriginDist = 0.0;
	pOptions->PrePickTime = 0.0;
	pOptions->PostPickTime = 0.0;
	pOptions->numZ = 0;
	pOptions->numDist = 0;
	pOptions->deltaZ = 0.0;
	pOptions->deltaDist = 0.0;
	pOptions->FixedDuration = 60.0;
	pOptions->iNumAttempts = 3;
	pOptions->iRequestGroup = 0;

  return(EW_SUCCESS);
}





/****************************** match  *****************************************
*                                                                              *
*	helper routine for snippet2trReq above: See if the SCN names               *
*	match. Wildcards allowed				                                   *
*******************************************************************************/
static int match( char* sta, char* comp, char* net, EWDB_StationStruct* staStruct)
{
	int staWild =0;
	int netWild =0;
	int chanWild=0;
	
	int staMatch =0;
	int netMatch =0;
	int chanMatch=0;
	
	if (strcmp( sta , "*") == 0)  staWild =1;
	if (strcmp( comp, "*") == 0)  chanWild=1;
	if (strcmp( net , "*") == 0)  netWild =1;

	if ( strcmp( sta, staStruct->Sta )==0 ) staMatch=1;
	if ( strcmp( net, staStruct->Net )==0 ) netMatch=1;
	if ( strcmp( comp, staStruct->Comp )==0 ) chanMatch=1;

	if ( staWild  ==1 ||  staMatch==1)  staMatch =1;
	if ( netWild  ==1 ||  netMatch==1)  netMatch =1;
	if ( chanWild ==1 || chanMatch==1) chanMatch =1;

	if (staMatch+netMatch+chanMatch == 3) 
		return(1);
	else
	   	return(0);
}


/************************************************************************************
	To read the next travel time value. 
	Carl's k_val routine is a bit on the lame side.
	The idea is to find the next floating point value while
	stepping over comments and blank lines.
**********************************************************************************/
static	int readTTval(double* val)	
{
	char* nxt;

	/* get next token */
	nxt=k_str();

	/* or die trying */
	while(nxt==NULL || k_err() || nxt[0]=='#') {
		if( k_rd()==0 ) return(-1); /* eof found */
		nxt=k_str();
	}

	/* convert to floating point */
	*val= atof(nxt);
	return(1);
}	

/************************* sonOfSiteRead.c ********************************
 * Derived from site_read, below. Modified to load DB station structures  *
 *  site_read(name)  Read in a HYPOINVERSE format, universal station      *
 *                   code file                                            *
 **************************************************************************/

/* Sample station line:
R8075 MN  BHZ  41 10.1000N121 10.1000E   01.0     0.00  0.00  0.00  0.00 1  0.00
0123456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789 
*/

static		int sonOfSiteRead(char *name)
{
	FILE  *stafile;
	char   line[256];
	int    dlat, dlon, elev;
	double  mlat, mlon;
	char   comp, ns, ew;
	int    n;
	
	/* open station file
	*****************/
	if( (stafile = fopen( name, "r" )) == (FILE *) NULL ) {
		logit("e","sonOfSiteRead: Cannot open station list <%s>; exiting!\n", name);
		return(-1);
	}
	
	/* read in one line of the site file at a time
	*******************************************/
	while( fgets( line, sizeof(line), stafile ) != (char *) NULL )
	{
        /* see if internal site table has room left */
		if( numStalistRet >= MAX_STA ) 
		{
			logit("e","sonOfSiteRead: StationList table full; cannot load entire file <%s>\n", name );
			return(-1);
		}
        /* decode each line of the file */
		sscanf( &line[0] , "%s", &StationList[numStalistRet].Sta);
		sscanf( &line[6] , "%s", &StationList[numStalistRet].Net);
		sscanf( &line[10], "%s", &StationList[numStalistRet].Comp);
		comp = line[9];
		
		line[42] = '\0';
		n = sscanf( &line[38], "%d", &elev );
		if( n < 1 ) 
		{
			logit("e","sonOfSiteRead: Error reading elevation from line in station file\n%s\n",line );
			continue;
		}
		
		ew       = line[37];
		line[37] = '\0';
		n = sscanf( &line[26], "%d %f", &dlon, &mlon );
		if( n < 2 ) 
		{
			logit("e","sonOfSiteRead: Error reading longitude from line in station file\n%s\n",line );
			continue;
		}
		
		ns       = line[25];
		line[25] = '\0';
		n = sscanf( &line[15], "%d %f", &dlat, &mlat );
		if ( n < 2 ) 
		{
			logit("e","sonOfSiteRead: Error reading latitude from line in station file\n%s\n",
				line );
			continue;
		}
		
        /*      printf( "%-5s %-2s %-3s %d %.4f%c%d %.4f%c%4d\n",
		StationList[numStalistRet].Sta, StationList[numStalistRet].Net, StationList[numStalistRet].Comp,
		dlat, mlat, ns,
		dlon, mlon, ew, elev ); */ /*DEBUG*/
		
        /* use one-letter component if there is no 3-letter component given */
		if ( !strcmp(StationList[numStalistRet].Comp, "   ") ) sprintf( StationList[numStalistRet].Comp, "%c  ", comp );
		
        /* convert to decimal degrees */
		if ( dlat < 0 ) dlat = -dlat;
		if ( dlon < 0 ) dlon = -dlon;
		StationList[numStalistRet].Lat = (float)((double) dlat + (mlat/60.0));
		StationList[numStalistRet].Lon = (float)((double) dlon + (mlon/60.0));
		
        /* make south-latitudes and west-longitudes negative */
		if ( ns=='s' || ns=='S' )
			StationList[numStalistRet].Lat = -StationList[numStalistRet].Lat;
		if ( ew=='w' || ew=='W' || ew==' ' )
			StationList[numStalistRet].Lon = -StationList[numStalistRet].Lon;
		StationList[numStalistRet].Elev = (float)((double) elev/1000.);
		
        /*      printf("%-5s %-2s %-3s %.4f %.4f %.0f\n\n",
		StationList[numStalistRet].Sta, StationList[numStalistRet].Net, StationList[numStalistRet].Comp,
		StationList[numStalistRet].Lat, StationList[numStalistRet].Lon, StationList[numStalistRet].Elev ); */ /*DEBUG*/
		
		/* update the total number of stations loaded */
		if(numStalistRet < MAX_STA) ++numStalistRet;
		
	} /*end while*/
	
	fclose( stafile );
	return(1);
}


int    DetermineSnippetRequests(EWEventInfoStruct *pEvent, EWDB_SnippetRequestStruct *pSnippet,
                                TraceRequestOptions *pOptions, int NumSnipAlloc, int *NumSnipRet)
{

/*
  int         i, iMand, iSta, numStalistFound;
  int          ConcDebug = TRUE;
  double         critDist = 0;
  double         epiDist = 0;
  double         epiAzm, sTime, pTime;
  SEL_STA        SelSta[MAX_STA];
*/
  int iNumSnippetRequests = 0;
  int tPTime;
  int i;


  if ((pEvent == NULL) || (pSnippet == NULL) || 
            (pOptions == NULL) || (NumSnipAlloc <= 0))
  {
    logit ("", "DetermineSnippetRequests(): ERROR: Invalid arguments passed in.\n");
    return EW_FAILURE;
  }


  /* first do picked stations */
  for (i = 0 ; i < pEvent->iNumChans; i++)
  {
    memset(&(pSnippet[iNumSnippetRequests]),0,sizeof(EWDB_SnippetRequestStruct));

    /* copy the station info */
    memcpy(&(pSnippet[iNumSnippetRequests].ComponentInfo),
           &(pEvent->pChanInfo[i].Station), sizeof(EWDB_StationStruct));

    /* set the estimated P-Time */
    /* Note:  Eventually we will need to check the sPhase to make sure the pick is a 'P' */
    /* Note:  Currently we expect channel in this structure to have an arrival, and expect
        that arrival to be a P 
        Current global DB-statistics for Phase Type are:
           SPHASE COUNT(IDPICK)
           ------ -------------
           P               6958
           PcP              398
           Pdiff             33
           Pup             1222
        The assumption that the phase is a P, is of course a HACK!!!
    */

    if(pEvent->pChanInfo[i].iNumArrivals > 0)
      tPTime = (int) pEvent->pChanInfo[i].Arrivals[0].tCalcPhase;
    else
      tPTime = (int) Get_P_time(pEvent->PrefOrigin.tOrigin, 
					         pEvent->PrefOrigin.dDepth, 
                   pEvent->pChanInfo[i].Arrivals[0].dDist * (360./EARTH_CIRCUM),
					         pOptions->PTT, pOptions->numZ, pOptions->numDist,
					         pOptions->deltaZ, pOptions->deltaDist);

    /* set the start time based on P-Time */
    pSnippet[iNumSnippetRequests].tStart = tPTime - pOptions->PrePickTime;

    /* set the end time based on P-Time */
    pSnippet[iNumSnippetRequests].tEnd = tPTime + pOptions->PostPickTime;

    /* up the number of requests */
    iNumSnippetRequests++;
  }



  /* next do SPECIAL stations */
  /*  --- we're not doing SPECIAL right now */


  /* save the number of snippet requests to the caller's var */
  *NumSnipRet = iNumSnippetRequests;


  return EW_SUCCESS;
}  /* end DetermineSnippetRequests() */


int ewdb_apps_PreprocessSnippetReqs(EWDB_SnippetRequestStruct * pRequest)
{
  /* Return Codes
   *  EWDB_RETURN_SUCCESS:  Success, continue and request the Snippet.
   *  EWDB_RETURN_WARNING:  Warning, this function decided that the
                            request is not needed.  Possible it was
                            able to find/modify an existing snippet/request
                            so that the existing request/snippet could
                            incorporate the new data.
                            DROP THE CURRENT REQUEST!!!
   *  EWDB_RETURN_FAILURE   Something bad happened, 
                            ALERT THE LOCAL AUTHORITIES!!!
   **************************************************************/


  return(EWDB_RETURN_SUCCESS);
  
}  /* end ewdb_apps_PreprocessSnippetReqs() */



