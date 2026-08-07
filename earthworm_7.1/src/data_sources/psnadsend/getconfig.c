#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <kom.h>
#include <earthworm.h>
#include "PsnAdSend.h"

/* Function declaration */
int IsComment( char [] );	 // Defined below

/* The configuration file parameters */
unsigned char ModuleId;		// Module id of this program
int	ChanRate;			  	// Rate in samples per second per channel
long OutKey;				// Key to ring where traces will live
int HeartbeatInt;		 	// Heartbeat interval in seconds
int UpdateSysClock;	  		// Use a good IRIGE time to update the PC clock
SCNL *ChanList;			 	// Array to fill with SCNL values
int Nchan = 0;			 	// Number of channels: either specified, or 64* (# of mux's)
int TimeoutNoSend;			// Take action if no data sent for this many seconds
int TimeoutNoSynch;	  		// Take action if no time synch for this many seconds
int TimeRefType;			// Time reference type. See .d file for types
int CommPort;				// Comm port number to use
long PortSpeed;				// Comm port baud rate
int HighToLowPPS;			// 1PPS input signal direction
int	NoPPSLedStatus;			// Stops blinking of the LED
int	TimeOffset;				// Time reference offset in milliseonds
int LogMessages;			// Log messages from the DLL and ADC board
int AdcDataSize;			// Data size sent to the trace buffer. Either 2 or 4 Bytes

char TimeFilePath[ 256 ];	// Where to save the time.dat file

#define NCOMMAND	16		// Number of mandatory commands in the config file

int GetConfig( char *configfile )
{
	const int ncommand = NCOMMAND;
	char	  init[NCOMMAND];	  /* Flags, one for each command */
	int		nmiss;				  /* Number of commands that were missed */
	int		nfiles;
	int		i;
	int		nChanLines = 0;		/* number of 'chan' specifiers. Must == Nchan */
	char	  str[ 256 ];

	/* Set to zero one init flag for each required command */
	for ( i = 0; i < ncommand; i++ )
		init[i] = 0;

	/* Open the main configuration file */
	nfiles = k_open( configfile );
	if ( nfiles == 0 )  {
		logit("", "PsnAdSend: Error opening configuration file <%s>\n", configfile );
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
					logit("", "PsnAdSend: Error opening command file <%s>.\n", &com[1] );
					return -1;
				}
				continue;
			}

			/* Read configuration parameters */
			else if ( k_its( "ModuleId" ) )  {
				if ( str = k_str() )  {
					if ( GetModId(str, &ModuleId) == -1 )  {
						logit("", "PsnAdSend: Invalid ModuleId <%s>. Exiting.\n", str );
						return -1;
					}
				}
				init[0] = 1;
			}
		 	else if( k_its( "TimeFilePath" ) && ( str = k_str() ) )  {
				strcpy( TimeFilePath, str );
				init[1] = 1;
		 	}
			else if ( k_its( "ChanRate" ) )  {
				ChanRate = k_int();
				init[2] = 1;
			}
			else if ( k_its( "TimeRefType" ) )  {
				TimeRefType = k_int();
				init[3] = 1;
			}
			else if ( k_its( "OutRing" ) )  {
				if ( str = k_str() )  {
					if ( (OutKey = GetKey(str)) == -1 )  {
						logit("", "PsnAdSend: Invalid OutRing <%s>. Exiting.\n", str );
						return -1;
					}
				}
				init[4] = 1;
			}
			else if ( k_its( "HeartbeatInt" ) )  {
				HeartbeatInt = k_int();
				init[5] = 1;
			}
			else if ( k_its( "PortSpeed" ) )  {
				PortSpeed = k_int();
				init[6] = 1;
			}
			else if ( k_its( "UpdateSysClock" ) )  {
				UpdateSysClock = k_int();
				init[7] = 1;
			}
			else if ( k_its( "CommPort" ) )  {
				CommPort = k_int();
				init[8] = 1;
			}
		 	else if ( k_its( "TimeoutNoSend" ))  {
				TimeoutNoSend = k_int();
				init[9] = 1;
			}
		 	else if ( k_its( "TimeoutNoSynch" ))  {
				TimeoutNoSynch = k_int();
				init[10] = 1;
			}
		 	else if ( k_its( "HighToLowPPS" ))  {
				HighToLowPPS = k_int();
				init[11] = 1;
			}
		 	else if ( k_its( "NoPPSLedStatus" ))  {
				NoPPSLedStatus = k_int();
				init[12] = 1;
			}
			else if ( k_its( "TimeOffset" ))  {
				TimeOffset = k_int();
				init[13] = 1;
			}
			else if ( k_its( "LogMessages" ))  {
				LogMessages = k_int();
				init[14] = 1;
			}
			else if ( k_its( "AdcDataSize" ))  {
				AdcDataSize = k_int();
				init[15] = 1;
			}
			else if ( k_its( "Nchan" ) )
				Nchan = k_int();
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
				nChanLines++;
			}
			/* An unknown parameter was encountered */
			else  {
				logit("", "PsnAdSend: <%s> unknown parameter in <%s>\n", com, configfile );
				return -1;
			}
			/* See if there were any errors processing the command */
			if ( k_err() )  {
				logit("", "PsnAdSend: Bad <%s> command in <%s>.\n", com, configfile );
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
		logit("", "PsnAdSend: ERROR, no " );
		if ( !init[0]  ) logit("", "<ModuleId> " );
		if ( !init[1]  ) logit("", "<TimeFilePath> " );
		if ( !init[2]  ) logit("", "<ChanRate> " );
		if ( !init[3]  ) logit("", "<TimeRefType> " );
		if ( !init[4]  ) logit("", "<OutRing> " );
		if ( !init[5] ) logit("", "<HeartbeatInt> " );
		if ( !init[6] ) logit("", "<PortSpeed> " );
		if ( !init[7] ) logit("", "<UpdateSysClock> " );
		if ( !init[8] ) logit("", "<CommPort> " );
		if ( !init[9] ) logit("", "<TimeoutNoSend> " );
		if ( !init[10] ) logit("", "<TimeoutNoSynch> " );
		if ( !init[11] ) logit("", "<HighToLowPPS> " );
		if ( !init[12] ) logit("", "<NoPPSLedStatus> " );
		if ( !init[13] ) logit("", "<TimeOffset> " );
		if ( !init[14] ) logit("", "<LogMessages> " );
		if ( !init[15] ) logit("", "<AdcDataSize> " );
		logit("", "command(s) in <%s>.\n", configfile );
		return -1;
	}

	return 0;
}

void LogConfig( void )
{
	int i;

	logit( "", "ModuleId:        %u\n", ModuleId );
	logit( "", "Comm Port        %d\n", CommPort );
	logit( "", "Port Speed       %d\n", PortSpeed );
	logit( "", "ChanRate:        %d\n", ChanRate );
	logit( "", "TimeFilePath:    %s\n",  TimeFilePath );
	logit( "", "TimeRefType:     %d\n", TimeRefType );
	logit( "", "OutKey:          %d\n", OutKey );
	logit( "", "HeartbeatInt:    %d\n", HeartbeatInt );
	logit( "", "UpdateSysClock:  %d\n", UpdateSysClock );
	logit( "", "TimeoutNoSend:   %d\n", TimeoutNoSend );
	logit( "", "TimeoutNoSynch:  %d\n", TimeoutNoSynch );
	logit( "", "HighToLowPPS:    %d\n", HighToLowPPS );
	logit( "", "NoPPSLedStatus:  %d\n", NoPPSLedStatus );
	logit( "", "TimeOffset:      %d\n", TimeOffset );
	logit( "", "LogMessages:     %d\n", LogMessages );
	logit( "", "AdcDataSize:     %d\n", AdcDataSize );
	logit( "", "Total Channels:  %d\n", Nchan );

	/* Log the channel list */
	logit( "", "\n" );
	logit( "", "chan Sta  Comp Net Loc\n" );
	logit( "", "---- ---  ---- --- ---\n" );

	for ( i = 0; i < Nchan; i++ )  {
		if ( strlen( ChanList[i].sta  ) > 0 )  {	 /* This channel is used */
			logit( "", "%d    %-4s %s  %s  %s\n", i, ChanList[i].sta, ChanList[i].comp,
				ChanList[i].net, ChanList[i].loc );
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
