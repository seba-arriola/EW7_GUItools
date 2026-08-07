/*********************************************************************
*									Ws2EwUtils.c					 *
*																	 *
*********************************************************************/

#include <stdio.h>
#include <math.h>
#include <time.h>
#include <string.h>
#include <process.h>
#include <earthworm.h>
#include <transport.h>
#include <trace_buf.h>
#include "Ws2Ew.h"

/* Global variables */
extern BYTE newData[ 1024 ];
extern DataHeader dataHdr;

extern char messages[MAX_MESSAGES][MESSAGE_LEN];

extern SOCKET soc;          // Socket
struct sockaddr_in saddr;   // Socket address structure

COORD coordSize;
COORD coordDest = { 0, 0 };
CHAR_INFO charInfo[128];

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
	COORD coord;
	DWORD numWritten;
	WORD color;
	int cnt;

	/* Get the console handle */
	outHandle = GetStdHandle( STD_OUTPUT_HANDLE );

	coordSize.X = coordSize.Y = 80;
	coord.X = coord.Y = 0;

	/* Set foreground and background colors */
	color = BACKGROUND_BLUE | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
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
	printf( "WinSDR to Earthworm" );

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
	long msgLen;				// Length of the heartbeat message
	char msg[40];				// To hold the heartbeat message
	static int first = TRUE;	// 1 the first time Heartbeat() is called
	static time_t time_prev;	// When Heartbeat() was last called
	time_t time_now;			// The current time
	static MSG_LOGO	logo;		// Logo of heartbeat messages

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
		logWrite( "Error sending heartbeat!" );

	time_prev = time_now;
}

BYTE CalcCRC( BYTE *cp, int cnt )
{
	BYTE crc = 0;
	while(cnt--)
		crc ^= *cp++;
	return(crc);
}

int InitSocket()
{
	struct hostent *hp;
	int addr;

	SocketSysInit();   /* This exits on failure */

	/* Set the socket address structure */
	memset(&saddr, 0, sizeof(struct sockaddr_in));
	saddr.sin_family = AF_INET;
	saddr.sin_port = htons( (unsigned short)Port );

	if( ( addr = inet_addr( Host ) ) != INADDR_NONE )
		saddr.sin_addr.s_addr = addr;
	else  {
		if( ( hp = gethostbyname( Host ) ) == NULL)  {
			logWrite( "Invalid Host Address <%s>\n", Host );
			return FALSE;
		}
	  	memcpy((void *) &saddr.sin_addr, (void*)hp->h_addr, hp->h_length);
	}
	return TRUE;
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
