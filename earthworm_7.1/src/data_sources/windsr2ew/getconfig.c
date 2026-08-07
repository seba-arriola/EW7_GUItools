#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <kom.h>
#include <earthworm.h>
#include "Ws2Ew.h"

/* Function declaration */
int IsComment( char [] );	 // Defined below

/* The configuration file parameters */
unsigned char ModuleId;		// Module id of this program
char Host[256];				// Host name or IP Address of WinSDR system
int Port;					// Port to use when connection to WinSDR
int	ChanRate;			  	// Rate in samples per second per channel
long OutKey;				// Key to ring where traces will live
int HeartbeatInt;		 	// Heartbeat interval in seconds
int Nchan = 0;			 	// Number of channels: either specified, or 64* (# of mux's)
int SocketTimeout = 60000;	// Time in milliseconds
int NoDataWaitTime = 45;	// Time to wait for no data. If no data reset recv thread. In seconds.
int RestartWaitTime = 60;	// Time to wait between reconnects. In seconds.
int NoHostMessages = 0;		// If set to 1, no log messages from the host will be processed
int AdcDataSize = 2;		// Data size sent to the trace buffer. Either 2 or 4 Bytes
SCNL *ChanList;			 	// Array to fill with SCNL values

char TimeFilePath[ 256 ];	// Where to save the time.dat file

#define NCOMMAND	8		// Number of mandatory commands in the config file

int GetConfig( char *configfile )
{
	const int ncommand = NCOMMAND;
	char	  init[NCOMMAND];	  /* Flags, one for each command */
	int		nmiss;				  /* Number of commands that were missed */
	int		nfiles;
	int		i;
	int		nChanLines = 0;		/* number of 'chan' specifiers. Must == Nchan */
	char	str[ 256 ], tmp[ 256 ];

	/* Set to zero one init flag for each required command */
	for ( i = 0; i < ncommand; i++ )
		init[i] = 0;

	/* Open the main configuration file */
	nfiles = k_open( configfile );
	if ( nfiles == 0 )  {
		logit("", "Ws2Ew: Error opening configuration file <%s>\n", configfile );
		return -1;
	}

	/* Process all nested configuration files */
	while ( nfiles > 0 )  {			 	/* While there are config files open */
		while ( k_rd() )  {			  	/* Read next line from active file  */
			int  success;
			char *com;
			char *str;

			com = k_str();			 /* Get the first token from line */

			if ( !com ) continue;				 /* Ignore blank lines */
			if ( com[0] == '#' ) continue;	 /* Ignore comments */

			/* Open another configuration file */
			if ( com[0] == '@' )  {
				success = nfiles + 1;
				nfiles  = k_open( &com[1] );
				if ( nfiles != success )  {
					logit("", "Ws2Ew: Error opening command file <%s>.\n", &com[1] );
					return -1;
				}
				continue;
			}

			/* Read configuration parameters */
			else if ( k_its( "ModuleId" ) )  {
				if ( str = k_str() )  {
					if ( GetModId(str, &ModuleId) == -1 )  {
						logit("", "Ws2Ew: Invalid ModuleId <%s>. Exiting.\n", str );
						return -1;
					}
				}
				init[0] = 1;
			}
			else if ( k_its( "Host" ) )  {
				str = k_str();
				strcpy( Host, str );
				init[1] = 1;
			}
			else if ( k_its( "Port" ) )  {
				Port = k_int();
				init[2] = 1;
			}
			else if ( k_its( "ChanRate" ) )  {
				ChanRate = k_int();
				init[3] = 1;
			}
			else if ( k_its( "OutRing" ) )  {
				if ( str = k_str() )  {
					if ( (OutKey = GetKey(str)) == -1 )  {
						logit("", "Ws2Ew: Invalid OutRing <%s>. Exiting.\n", str );
						return -1;
					}
				}
				init[4] = 1;
			}
			else if ( k_its( "HeartbeatInt" ) )  {
				HeartbeatInt = k_int();
				init[5] = 1;
			}
			else if ( k_its( "Nchan" ) )  {
				Nchan = k_int();
				init[6] = 1;
			}
			else if ( k_its( "AdcDataSize" ))  {
				AdcDataSize = k_int();
				init[7] = 1;
			}
			/* Option parameters */
			else if ( k_its( "SocketTimeout" ) )
				SocketTimeout = k_int() * 1000;
			else if ( k_its( "NoDataWaitTime" ) )
				NoDataWaitTime = k_int();
			else if ( k_its( "RestartWaitTime" ) )
				RestartWaitTime = k_int();
			else if ( k_its( "NoHostMessages" ) )
				NoHostMessages = k_int();

			/* Get the channel list */
			else if ( k_its( "Chan" ) )  {		// Scnl value for each channel
				static int first = 1;
				int chan;
				int rc;							// kom function return code

				if ( first )  {						// First time a Chan line was found
					ChanList = (SCNL *) calloc( Nchan, sizeof(SCNL) );
					if ( ChanList == NULL )  {
						logit("", "Error. Cannot allocate the channel list.\n" );
						return -1;
					}
					first = 0;
				}
				chan = k_int();				  	// Get channel number
				if ( chan>=Nchan || chan<0 )  {
					logit("", "Error. Bad channel number (%d) in config file.\n", chan );
					return -1;
				}
				strcpy( ChanList[chan].sta,  k_str() );  // Save Scnl value in chan list
				strcpy( ChanList[chan].comp, k_str() );
				strcpy( ChanList[chan].net,  k_str() );
				strcpy( ChanList[chan].loc,  k_str() );
				strcpy( tmp, k_str() );
				if( tmp[0] == 'Y' || tmp[0] == 'y' )
					ChanList[chan].send = TRUE;
				else
					ChanList[chan].send = FALSE;
				nChanLines++;
			}
			/* An unknown parameter was encountered */
			else  {
				logit("", "Ws2Ew: <%s> unknown parameter in <%s>\n", com, configfile );
				return -1;
			}
			/* See if there were any errors processing the command */
			if ( k_err() )  {
				logit("", "Ws2Ew: Bad <%s> command in <%s>.\n", com, configfile );
				return -1;
			}
		}
		nfiles = k_close();
	}

	/* After all files are closed, check flags for missed commands */
	nmiss = 0;
	for ( i = 0; i < ncommand; i++ )
		if ( !init[i] )
			nmiss++;
	if ( nmiss > 0 )  {
		logit("", "Ws2Ew: ERROR, no " );
		if ( !init[0]  ) logit("", "<ModuleId> " );
		if ( !init[1]  ) logit("", "<Host> " );
		if ( !init[2]  ) logit("", "<Port> " );
		if ( !init[3]  ) logit("", "<ChanRate> " );
		if ( !init[4]  ) logit("", "<OutRing> " );
		if ( !init[5] ) logit("", "<HeartbeatInt> " );
		if ( !init[6] ) logit("", "<Nchan> " );
		if ( !init[7] ) logit("", "<AdcDataSize> " );
		logit("", "command(s) in <%s>.\n", configfile );
		return -1;
	}

	return 0;
}

void LogConfig( void )
{
	int i;
	char chr;

	logit( "", "ModuleId:        %u\n", ModuleId );
	logit( "", "Host:            %s\n", Host );
	logit( "", "Port:            %d\n", Port );
	logit( "", "ChanRate:        %d\n", ChanRate );
	logit( "", "OutKey:          %d\n", OutKey );
	logit( "", "HeartbeatInt:    %d\n", HeartbeatInt );
	logit( "", "AdcDataSize:     %d\n", AdcDataSize );
	logit( "", "Total Channels:  %d\n", Nchan );

	/* Log the channel list */
	logit( "", "\n" );
	logit( "", "chan Sta  Comp Net Loc  Send\n" );
	logit( "", "---- ---  ---- --- ---  ----\n" );

	for ( i = 0; i < Nchan; i++ )  {
		if ( strlen( ChanList[i].sta  ) > 0 )  {	 /* This channel is used */
			if( ChanList[i].send )
				chr = 'Y';
			else
				chr = 'N';
			logit( "", "%d    %-4s %-3s  %-2s  %-2s  %c\n", i, ChanList[i].sta, ChanList[i].comp,
				ChanList[i].net, ChanList[i].loc, chr );
		}
	}
	logit("", "-------------------------------------------------------\n");
	return;
}

int IsComment( char string[] )
{
	int i;

	for ( i = 0; i < (int)strlen( string ); i++ )  {
		char test = string[i];
		if ( test!=' ' && test!='\t' )  {
			if ( test == '#'  )
				return 1;			 /* It's a comment line */
			else
				return 0;			 /* It's not a comment line */
		}
	}
	return 1;						 /* It contains only whitespace */
}
