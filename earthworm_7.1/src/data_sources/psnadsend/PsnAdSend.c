/*********************************************************************
*									PsnAdSend.c						 *
*																	 *
*	         Digitizing program for the PSN-ADC-SERIAL A/D Board	 *
*********************************************************************/

#include <stdio.h>
#include <math.h>
#include <time.h>
#include <string.h>
#include <process.h>
#include <earthworm.h>
#include <transport.h>
#include <trace_buf.h>
#include "PsnAdSend.h"

/* Global variables */
SHM_INFO OutRegion;				// Info structure for output region
pid_t MyPid;				 	// process id, sent with heartbeat
HANDLE hBoard = 0;				// Handle to the DLL/ADC board
HANDLE outHandle;			  	// The console file handle

/* Configuration information sent to the DLL and ADC board */
AdcBoardConfig config;

BYTE newData[ 1024 ];
BYTE newAdcData[ MAX_ADC_CHANNELS * MAX_SPS_RATE * sizeof(long) ];
StatusInfo statusInfo;
DataHeader dataHdr;
int adcBoardType = BOARD_UNKNOWN;
COORD coordSize;
COORD coordDest = { 0, 0 };
CHAR_INFO charInfo[128];
unsigned dataSize;			// Size of dataBuf in number of samples

/* Messages to display are placed in the array */
char messages[ MAX_MESSAGES ][ MESSAGE_LEN ];

int main( int argc, char *argv[] )
{
	int i, c, j,			// Loop indexes
		traceBufSize;	  	// Size of the trace buffer, in bytes
	long *dataBuf;			// Pointer to received A/D data
	long *lTraceDat;		// Where the data points are stored in the trace msg
	short *sTraceDat;		// Where the data points are stored in the trace msg
	char *traceBuf;			// Where the trace message is assembled
	unsigned samples = 0;	// Number of samples received from the ADC board
	unsigned char InstId;	// Installation id placed in the trace header
	MSG_LOGO logo;		 	// Logo of message to put out
	TRACE2_HEADER *traceHead; // Where the trace header is stored
	struct TIME_BUFF Tbuf; 	// Time read-write structure
	int rc;			  		// Function return code
	long *lTracePtr;	 	    // Pointer into traceBuf for demuxing
	short *sTracePtr;	 	// Pointer into traceBuf for demuxing
	long *lDataPtr;	  		// Long Pointer
	ULONG tm;

	/* Get command line arguments */
	if ( argc < 2 )  {
		printf( "Usage: PsnAdSend <config file>\n" );
		return -1;
	}

	memset( messages, 0, sizeof( messages ) );

	/* Initialize name of log-file & open it */
	logit_init( argv[1], 0, 256, 1 );

	/* Read configuration parameters */
	if ( GetConfig( argv[1] ) < 0 )  {
		logit( "e", "PsnAdSend: Error reading configuration file. Exiting.\n" );
		return -1;
	}

	/* Set up the logo of outgoing waveform messages */
	if ( GetLocalInst( &InstId ) < 0 )  {
		logit( "e", "PsnAdSend: Error getting the local installation id. Exiting.\n" );
		return -1;
	}
	logit( "", "Local InstId:	 %u\n", InstId );

	/* Log the configuration file */
	LogConfig();

	/* Get our Process ID for restart purposes */
	MyPid = _getpid();
	if( MyPid == -1 )  {
		logit("e", "PsnAdSend: Cannot get PID. Exiting.\n" );
		return -1;
	}

	logo.instid = InstId;
	logo.mod = ModuleId;
	GetType( "TYPE_TRACEBUF2", &logo.type );

	/* Allocate some array space */
	dataSize = (unsigned)(ChanRate * Nchan);
	dataBuf = (long *) calloc( dataSize, sizeof(long) );
	if ( dataBuf == NULL )  {
		logit( "", "Cannot allocate the A/D buffer\n" );
		return -1;
	}
	if( AdcDataSize == 4 )
		traceBufSize = sizeof(TRACE2_HEADER) + (ChanRate * sizeof(long));
	else
		traceBufSize = sizeof(TRACE2_HEADER) + (ChanRate * sizeof(short));
	traceBuf = (char *) malloc( traceBufSize );
	if ( traceBuf == NULL )  {
		logit( "", "Cannot allocate the trace buffer\n" );
		return -1;
	}

	traceHead = (TRACE2_HEADER *) &traceBuf[0];
	if( AdcDataSize == 4 )
		lTraceDat  = (long *) &traceBuf[sizeof(TRACE2_HEADER)];
	else
		sTraceDat  = (short *) &traceBuf[sizeof(TRACE2_HEADER)];

	/* Attach to existing transport ring and send first heartbeat */
	tport_attach( &OutRegion, OutKey );
	logit( "", "Attached to transport ring: %d\n", OutKey );
	Heartbeat();

	/* Initialize the console display */
	InitCon();

	/* Initialize the ADC board */
	if( !InitDAQ() )  {
		logit( "", "ADC Init board error\n" );
		return -1;
	}

	/************************* The main program loop	*********************/
	while ( tport_getflag( &OutRegion ) != TERMINATE  && tport_getflag( &OutRegion ) != MyPid )  {
		Heartbeat(); 							/* Beat the heart */

		if( ! GetDAQ( dataBuf, dataSize ) )		/* Get one seconds worth of data */
				break;

		samples += (unsigned)(ChanRate * Nchan); /* Update the samples count */

		tm = CalcPacketTime( &dataHdr.packetTime );
		SetCurPos( 9, 6 );
		for ( i = 0; i < 10; i++ )
			putchar( ' ' );
		SetCurPos( 9, 6 );
		printf( "%u", samples );

		Tbuf.t = (double)tm + 11676096000.0;
		Tbuf.t += (double)dataHdr.packetTime.wMilliseconds / 1000.0;
		SetCurPos( 27, 3 );
		PrintGmtime( Tbuf.t - 11676096000.0, 3 );

		SetCurPos( 15, 4 );
		printf( "Sending Data..." );

		SetCurPos( 23, 5 );
		if( dataHdr.timeRefStatus == TIME_REF_NOT_LOCKED )
			  printf("Not Locked ");
		else if( dataHdr.timeRefStatus == TIME_REF_WAS_LOCKED )
			  printf("Was Locked ");
		else
			  printf("Locked	  ");

	  	/* Position the screen cursor for error messages */
		SetCurPos( 0, 0 );

	  	/* Loop through the trace messages to be sent out */
		for ( i = 0; i < Nchan; i++ )  {
	  		/* Do not send messages for unused channels */
		  	if ( strlen( ChanList[i].sta  ) == 0 && strlen( ChanList[i].net  ) == 0 &&
					strlen( ChanList[i].comp ) == 0 )
			  continue;

		  	/* Fill the trace buffer header */
			traceHead->nsamp = ChanRate;				// Number of samples in message
			traceHead->samprate = ChanRate;			  	// Sample rate; nominal
			traceHead->version[0] = TRACE2_VERSION0;     // Header version number
			traceHead->version[1] = TRACE2_VERSION1;     // Header version number
		  	traceHead->quality[0] = '\0';				// One bit per condition
		  	traceHead->quality[1] = '\0';				// One bit per condition

			if( AdcDataSize == 4 )
			  	strcpy( traceHead->datatype, "i4" );		// Data format code
			else
			  	strcpy( traceHead->datatype, "i2" );		// Data format code
		  	strcpy( traceHead->sta,  ChanList[i].sta ); // Site name
		  	strcpy( traceHead->net,  ChanList[i].net ); // Network name
		  	strcpy( traceHead->chan, ChanList[i].comp ); // Component/channel code
            strcpy( traceHead->loc,  ChanList[i].loc );  // Location code
		  	traceHead->pinno = i+1;						// Pin number

		  	/* Set the trace start and end times. Times are in seconds since midnight 1/1/1970 */
		  	traceHead->starttime = Tbuf.t - 11676096000.;
		  	traceHead->endtime	= traceHead->starttime + (double)(ChanRate - 1) / (double)ChanRate;

		  	/* Set error bits in buffer header */
		  	if( dataHdr.timeRefStatus != TIME_REF_LOCKED )
				traceHead->quality[0] |= TIME_TAG_QUESTIONABLE;

		  	/* Transfer samples from dataBuf to traceBuf */
		  	lDataPtr  = &dataBuf[i];
			if( AdcDataSize == 4 )  {
			  	lTracePtr = &lTraceDat[0];
			  	for ( j = 0; j < ChanRate; j++ )  {
					*lTracePtr++ = *lDataPtr;
				  	lDataPtr += Nchan;
			  	}
			}
			else  {
			  	sTracePtr = &sTraceDat[0];
			  	for ( j = 0; j < ChanRate; j++ )  {
					*sTracePtr++ = (short)*lDataPtr;
				  lDataPtr += Nchan;
			  	}
			}
		  	rc = tport_putmsg( &OutRegion, &logo, traceBufSize, traceBuf );
		  	if ( rc == PUT_TOOBIG )
				printf( "Trace message for channel %d too big\n", i );
		  	else if ( rc == PUT_NOTRACK )
			  	printf( "Tracking error while sending channel %d\n", i );
		}
		SetCurPos( 0, 0 );
	}

	/* Clean up and exit program */
	StopDAQ();
	free( dataBuf );
	free( ChanList );
	logit( "", "PsnAdSend terminating.\n" );
	return 0;
}

int GetDAQ( long *adcData, unsigned long points )
{
	int timeout, boardType, count;
	DWORD type, dataLen, sts;
	char *lockStr, buffer[ 256 ];
	SYSTEMTIME *st;
	long *lPtr;
	short *sPtr;

	while( TRUE)  {
		timeout = TimeoutNoSend * 100;
		while( TRUE )  {
			sts = PSNGetBoardData( hBoard, &type, newData, newAdcData, &dataLen );
			if( sts == ADC_GOOD_DATA )
				break;
			if( sts == ADC_BOARD_ERROR )
				return 0;
			if( sts == ADC_NO_DATA )  {
				--timeout;
				if( !timeout )
					return 0;
			}
			_sleep( 10 );
		}

		if( adcBoardType == BOARD_UNKNOWN && type == ADC_AD_DATA )  {
			if( !PSNGetBoardInfo( hBoard, ADC_GET_BOARD_TYPE, &boardType ) ||
					( boardType == BOARD_UNKNOWN ) )  {
				logWrite( "ADC Board Type Error" );
				return 0;
			}
			adcBoardType = boardType;
			if( boardType == BOARD_VM && AdcDataSize == 2 )  {
				logWrite( "Configuration Error. Must use AdcDataSize of 4 for this ADC board." );
				return 0;
			}
		}

		if( type == ADC_AD_DATA )  {
			memcpy( &dataHdr, newData, sizeof( dataHdr ) );
			if( adcBoardType == BOARD_VM )  {
				memcpy( adcData, newAdcData, dataLen );
				return 1;
			}
			sPtr = (short *)newAdcData;
			lPtr = adcData;
			count= dataSize;
			while( count-- )
				*lPtr++ = *sPtr++;
			return 1;
		}

		if( type == ADC_MSG || type == ADC_ERROR || type == ADC_AD_MSG )  {
			if( LogMessages )
				logWrite( "%s", newData );
		}
		else if( type == ADC_SAVE_TIME_INFO )
			SaveTimeInfo( (TimeInfo *)newData );
		else if( type == ADC_STATUS )
			memcpy( &statusInfo, newData, sizeof( statusInfo ) );
		else
			logWrite( "Unknown ADC DLL Message Type");
	}
	return 0;
}

BOOL InitDAQ()
{
	DWORD version;
	TimeInfo timeInfo;
	DWORD sts;

	hBoard = PSNOpenBoard();
	if( !hBoard )
		return FALSE;

	if( !PSNGetBoardInfo( hBoard, ADC_GET_DLL_VERSION, &version ) )  {
		logit( "", "Get DLL version error\n" );
		PSNCloseBoard( hBoard );
		return 0;
	}
	logWrite( "PSNADBoard DLL Version %d.%1d", version / 10, version % 10 );

	ReadTimeInfo( &timeInfo );

	memset( &config, 0, sizeof( config ) );
	config.commPort = CommPort;
	config.commSpeed = PortSpeed;
	config.numberChannels = Nchan;
	config.sampleRate = ChanRate;
	config.timeRefType = TimeRefType;
	config.addDropTimer = timeInfo.addDropCount;
	config.addDropMode = timeInfo.addDropFlag;
	config.pulseWidth = timeInfo.pulseWidth;

	config.highToLowPPS = HighToLowPPS;
	config.noPPSLedStatus = NoPPSLedStatus;
	config.timeOffset = TimeOffset;
	config.setPCTime = UpdateSysClock;

	sts = PSNConfigBoard( hBoard, &config, NULL );
	if( !sts )  {
		logWrite( "Config ADC Board Error" );
		PSNCloseBoard( hBoard );
		return FALSE;
	}

	sts = PSNStartStopCollect( hBoard, TRUE );
	if( !sts )  {
		logWrite( "PSNStartStopCollect Error" );
		PSNCloseBoard( hBoard );
		return FALSE;
	}

	return TRUE;
}

void StopDAQ()
{
	PSNCloseBoard( hBoard );
}

/* Save time adjustment information to a file. Called when a ADC_SAVE_TIME_INFO message is sent by the DLL */
void SaveTimeInfo( TimeInfo *info )
{
	FILE *fp;
	char buff[ 256 ];

	sprintf( buff, "%s\\Time.dat", TimeFilePath );
	if( ! (fp = fopen( buff, "w") ) )
		return;
	fprintf(fp, "%d %d %d\n", info->addDropFlag, info->addDropCount, info->pulseWidth );
	fclose( fp );
}

/* Read the time adjustment information from a file. This information is then sent to the DLL.*/
void ReadTimeInfo( TimeInfo *info )
{
	FILE *fp;
	char buff[ 256 ];
	int cnt, flag, addDrop, width;

	memset( info, 0, sizeof(TimeInfo) );
	sprintf(buff, "%s\\Time.dat", TimeFilePath );
	if( !(fp = fopen( buff, "r") ) )
		return;
	fgets( buff, 127, fp );
	fclose( fp );
	cnt = sscanf( buff, "%d %d %d", &flag, &addDrop, &width );
	if( cnt != 3 )
		return;
	info->addDropFlag = flag;
	info->addDropCount = addDrop;
	info->pulseWidth = width;
}

/* returns the time as a long based on a SYSTEMTIME input structure  */
ULONG CalcPacketTime( SYSTEMTIME *st )
{
	long tmpZone = _timezone, ret;
	int tmpDay = _daylight;
	struct tm t;

	_timezone = 0;
	_daylight = 0;
	t.tm_wday = t.tm_yday = 0;
	t.tm_isdst = 0;
	t.tm_hour = st->wHour;
	t.tm_min = st->wMinute;
	t.tm_sec = st->wSecond;
	t.tm_mday = st->wDay;
	t.tm_mon = st->wMonth-1;
	t.tm_year = st->wYear - 1900;
	ret = mktime(&t);
	_timezone = tmpZone;
	_daylight = tmpDay;
	return ret;
}

void SetCurPos( int x, int y )
{
	COORD	  coord;

	coord.X = x;
	coord.Y = y;
	SetConsoleCursorPosition( outHandle, coord );
	return;
}

void InitCon( void )
{
	time_t current_time;
	CHAR_INFO *to = &charInfo[0];
	WORD color;
	COORD coord;
	DWORD numWritten;
	int cnt;

	/* Get the console handle */
	outHandle = GetStdHandle( STD_OUTPUT_HANDLE );

	/* Set foreground and background colors */
	color = BACKGROUND_BLUE | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;

	coordSize.X = coordSize.Y = 80;
	coord.X = coord.Y = 0;

	FillConsoleOutputAttribute( outHandle, color, 2000, coord, &numWritten );
	SetConsoleTextAttribute( outHandle, color );

	cnt = 127;
	while(cnt--)  {
		to->Char.AsciiChar = ' ';
		to->Attributes = color;
		++to;
	}
	/* Fill in the labels */
	SetCurPos( 30, 0 );
	printf( "PSN 16-Bit Digitizer" );

	SetCurPos( 0, 2 );
	printf( "Program Start Time (UTC): " );
	time( &current_time );
	SetCurPos( 26, 2 );
	PrintGmtime( (double)current_time, 0 );

	SetCurPos( 0, 3 );
	printf( "TraceBuf Start Time (UTC): " );

	SetCurPos( 0, 4 );
	printf( "System Status: " );

	SetCurPos( 0, 5 );
	printf( "Time Reference Status: " );

	SetCurPos( 0, 6 );
	printf( "Samples:" );

	SetCurPos( 0, 9 );
	printf( "Messages:" );

	SetCurPos( 0, 0 );
	return;
}

void ReportError( int errNum, char *errmsg )
{
	int lineLen;
	time_t time_now;			// The current time
	static MSG_LOGO	logo;		// Logo of error messages
	static int first = TRUE;	// TRUE the first time this function is called
	char outmsg[100];			// To hold the complete message
	static time_t time_prev;	// When Heartbeat() was last called

	/* Initialize the error message logo */
	if ( first )  {
		GetLocalInst( &logo.instid );
		logo.mod = ModuleId;
		GetType( "TYPE_ERROR", &logo.type );
		first = FALSE;
	}

	/* Encode the output message and send it */
	time( &time_now );
	sprintf( outmsg, "%d %d ", time_now, errNum );
	strcat( outmsg, errmsg );
	lineLen = strlen( outmsg );
	tport_putmsg( &OutRegion, &logo, lineLen, outmsg );
	return;
}

void PrintGmtime( double tm, int dec )
{
	int  hsec, ms, whole;
	struct tm *gmt;
	time_t ltime = (time_t)tm;

	gmt = gmtime( &ltime );

	printf( "%02d/%02d/%04d %02d:%02d:%02d", gmt->tm_mon+1,
		gmt->tm_mday, gmt->tm_year + 1900, gmt->tm_hour, gmt->tm_min, gmt->tm_sec );

	if ( dec == 2 )  {				  	// Two decimal places
		hsec = (int)(100. * (tm - floor(tm)));
		printf( ".%02d", hsec );
	}
	else if ( dec == 3 )  {				// Three decimal places
		double d = tm - floor(tm);
		d *= 1000.0;
	  	d += 0.1;
		printf( ".%03d", (int)d );
	}
}

void Heartbeat( void )
{
	long				  msgLen;			  // Length of the heartbeat message
	char				  msg[40];			 // To hold the heartbeat message
	static int		  first = TRUE;	  // 1 the first time Heartbeat() is called
	static time_t	  time_prev;		  // When Heartbeat() was last called
	time_t				time_now;			// The current time
	static MSG_LOGO	logo;				 // Logo of heartbeat messages

	/* Initialize the heartbeat variables */
	if ( first )  {
		GetLocalInst( &logo.instid );
		logo.mod = ModuleId;
		GetType( "TYPE_HEARTBEAT", &logo.type );
		time_prev = 0;  // force heartbeat first time thru
		first = FALSE;
	}

	/* Is it time to beat the heart? */
	time( &time_now );
	if ( (time_now - time_prev) < HeartbeatInt )
		return;

	/* It's time to beat the heart */
	sprintf( msg, "%d %d\n", time_now, MyPid );
	msgLen = strlen( msg );

	if ( tport_putmsg( &OutRegion, &logo, msgLen, msg ) != PUT_OK )
		logWrite( "Error sending heartbeat" );

	time_prev = time_now;
	return;
}

void ClearToEOL( int startCol, int startRow )
{
	SMALL_RECT region;
	int cols = coordSize.X - startCol;

	region.Left = startCol;
	region.Right = startCol+cols;
	region.Top = startRow;
	region.Bottom = startRow;
	WriteConsoleOutput( outHandle, &charInfo[0], coordSize, coordDest, &region);
	SetCurPos( startCol, startRow );
}

void logWrite(char *pszFormat, ...)
{
	int i;
	char buff[256], viewStr[256], sdate[32], stime[32],
		*pszArguments = (char*)&pszFormat + sizeof( pszFormat );

	vsprintf( buff, pszFormat, pszArguments );
	_strdate( sdate );
	sdate[5] = 0;
	_strtime( stime );
	sprintf( viewStr, "%s %s %s", sdate, stime, buff );
	if( strlen(viewStr) >= (MESSAGE_LEN-1) )
		viewStr[ MESSAGE_LEN-1] = 0;

	for( i = MAX_MESSAGES-1; i != 0; i-- )
		strcpy(messages[i], messages[i-1] );
	strcpy( messages[0], viewStr );
	for( i = 0; i != MAX_MESSAGES; i++ )
		ClearToEOL( 0, MESSAGE_START_ROW + i );
	for( i = 0; i != MAX_MESSAGES; i++ )  {
		SetCurPos( 0, MESSAGE_START_ROW + i );
		printf("%s", messages[ i ] );
	}
	logit("", "%s\n", viewStr );
}
