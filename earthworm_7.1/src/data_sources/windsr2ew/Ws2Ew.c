/*********************************************************************
*									Ws2Ew.c						     *
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
#include <socket_ew.h>
#include "Ws2Ew.h"

/* Global variables */
char Host[256];
int Port;
SHM_INFO OutRegion;			// Info structure for output region
pid_t MyPid;				// process id, sent with heartbeat
HANDLE outHandle;			// The console file handle

BYTE inData[ 4096 ];
BYTE packet[ 4096 ];
DataHeader dataHdr;

SOCKET soc;          		// Receive Socket
unsigned  TidReceive;
thr_ret ReceiveThread( void * );
int exitThread = 0, exitAck = 0;
int sock, crcErrors, noDataCounter = 0;
int connected = 0, connecting = 0, restart = 0;
time_t restartTime = 0;

BYTE hdrStr[] = { 0xaa, 0x55, 0x88, 0x44 };

QUEUE tbQ, msgQ;
mutex_t tbMx, msgMx;
MSG_LOGO logo;		 				// Logo of message to put out
unsigned int sampleCount = 0;		// Scan number of first scan in a message
int traceBufSize;					// Size of the trace buffer, in bytes
BYTE *traceBuf;						// Where the trace message is assembled
BYTE *dataBuf;						// Pointer to received A/D data
long *lTraceDat;					// Where the data points are stored in the trace msg
short *sTraceDat;					// Where the data points are stored in the trace msg

/* Messages to display are placed in the array */
char messages[ MAX_MESSAGES ][ MESSAGE_LEN ];

int main( int argc, char *argv[] )
{
	int i, j;				// Loop indexes
	unsigned dataSize;		// Size of dataBuf in number of samples
	unsigned char InstId;	// Installation id placed in the trace header
	TRACE2_HEADER *traceHead; // Where the trace header is stored
	struct TIME_BUFF Tbuf; 	// Time read-write structure
	int rc;			  		// Function return code
	BYTE *ptr, *dataPtr;
	ULONG tm;
	int max, len, vmBoard;
	short *sDataPtr;	 	// ptr for demuxing
	long *lDataPtr;	 		// ptr for demuxing
	long *lTracePtr;	 	// ptr into traceBuf for demuxing
	long *lPtr;
	short *sTracePtr;	 	// ptr into traceBuf for demuxing
	short *sPtr;

	/* Get command line arguments */
	if ( argc < 2 )  {
		printf( "Usage: Ws2Ew <config file>\n" );
		return -1;
	}

	memset( messages, 0, sizeof( messages ) );

	/* Initialize name of log-file & open it */
	logit_init( argv[1], 0, 256, 1 );

	/* Initialize the console display */
	InitCon();

	/* Read configuration parameters */
	if ( GetConfig( argv[1] ) < 0 )  {
		printf( "Ws2Ew: Error reading configuration file. Exiting.\n" );
		return -1;
	}

	/* Set up the logo of outgoing waveform messages */
	if ( GetLocalInst( &InstId ) < 0 )  {
		printf( "Ws2Ew: Error getting the local installation id. Exiting.\n" );
		return -1;
	}
	logit( "", "Local InstId:	 %u\n", InstId );

	/* Log the configuration file */
	LogConfig();

	/* Get our Process ID for restart purposes */
	MyPid = _getpid();
	if( MyPid == -1 )  {
		logit("e", "Ws2Ew: Cannot get PID. Exiting.\n" );
		return -1;
	}

	logo.instid = InstId;
	logo.mod = ModuleId;
	GetType( "TYPE_TRACEBUF2", &logo.type );

	/* Allocate some array space */
	dataSize = (unsigned)(ChanRate * Nchan);
	dataBuf = (BYTE *)calloc( dataSize, sizeof(long) );
	if ( dataBuf == NULL )  {
		logit( "", "Cannot allocate the A/D buffer\n" );
		return -1;
	}
	if(AdcDataSize == 2 )
		traceBufSize = sizeof(TRACE2_HEADER) + (ChanRate * sizeof(short));
	else
		traceBufSize = sizeof(TRACE2_HEADER) + (ChanRate * sizeof(long));
	traceBuf = (char *) malloc( traceBufSize );
	if ( traceBuf == NULL )  {
		logit( "", "Cannot allocate the trace buffer\n" );
		free( dataBuf );
		free( ChanList );
		return -1;
	}
	traceHead = (TRACE2_HEADER *)&traceBuf[0];
	ptr = &traceBuf[sizeof(TRACE2_HEADER)];
	if( AdcDataSize == 4 )
		lTraceDat  = (long *)ptr;
	else
		sTraceDat  = (short *)ptr;

	/* Attach to existing transport ring and send first heartbeat */
	tport_attach( &OutRegion, OutKey );
	logit( "", "Attached to transport ring: %d\n", OutKey );
	Heartbeat();

	/* Init tcp stuff */
	if( !InitSocket() )  {
		free( traceBuf );
		free( dataBuf );
		free( ChanList );
		tport_detach( &OutRegion );
		logit( "", "InitSocket Error\n" );
		return -1;
	}

	/* Init queue stuff. One for the EW ring receive data thread and the othr for messages */
  	initqueue( &tbQ, 10, 4096 );
  	initqueue( &msgQ, 10, 256 );
	CreateSpecificMutex( &tbMx );
	CreateSpecificMutex( &msgMx );

	logWrite("Starting Receive Thread");
	exitAck = 0;
	if( StartThread( ReceiveThread, (unsigned)THREAD_STACK, &TidReceive ) == -1 )  {
		logWrite( "Error starting receive thread. Exiting." );
		free( traceBuf );
		free( dataBuf );
		free( ChanList );
		tport_detach( &OutRegion );
      	return -1;
      }

	/************************* The main program loop	*********************/
	while ( tport_getflag( &OutRegion ) != TERMINATE  && tport_getflag( &OutRegion ) != MyPid )  {

		if( ! ( len = GetInQueue( inData ) ) )  {
			CheckStatus();
			_sleep( 100 );
			continue;
		}
		noDataCounter = 0;
		memcpy( &dataHdr, &inData[ sizeof(PreHdr) ], sizeof( dataHdr ) );
		dataPtr = &inData[ sizeof( DataHeader )+sizeof(PreHdr) ];
		if( dataHdr.flags & 0x40 )  {				// check for VM board
			vmBoard = TRUE;
			lDataPtr = (long *)dataPtr;
		}
		else  {
			vmBoard = FALSE;
			sDataPtr = (short *)dataPtr;
		}
		sampleCount += (unsigned)(ChanRate * Nchan);		/* Update the sample count */

		tm = CalcPacketTime( &dataHdr.packetTime );
		Tbuf.t = (double)tm + 11676096000.0;
		Tbuf.t += (double)dataHdr.packetTime.wMilliseconds / 1000.0;

		SetCurPos( 27, 3 );
		PrintGmtime( Tbuf.t - 11676096000.0, 3 );

		SetCurPos( 23, 5 );
		if( dataHdr.timeRefStatus == TIME_REF_NOT_LOCKED )
			printf("Not Locked ");
		else if( dataHdr.timeRefStatus == TIME_REF_WAS_LOCKED )
			printf("Was Locked ");
		else
			printf("Locked	   ");

		SetCurPos( 0, 0 );

	  	/* Loop through the trace messages to be sent out */
		for ( i = 0; i < Nchan; i++ )  {
	  		/* Do not send messages for unused channels */
		  	if ( !ChanList[i].send || !ChanList[i].sta[0] )
			  continue;

		  	/* Fill the trace buffer header */
			traceHead->nsamp = ChanRate;				// Number of samples in message
			traceHead->samprate = ChanRate;			  	// Sample rate; nominal
			traceHead->version[0] = TRACE2_VERSION0;     // Header version number
			traceHead->version[1] = TRACE2_VERSION1;     // Header version number
		  	traceHead->quality[0] = '\0';				// One bit per condition
		  	traceHead->quality[1] = '\0';				// One bit per condition

			if( AdcDataSize == 2 )
			  	strcpy( traceHead->datatype, "i2" );	// Data format code = short
			else
			  	strcpy( traceHead->datatype, "i4" );	// Data format code = long
		  	strcpy( traceHead->sta,  ChanList[i].sta ); // Site name
		  	strcpy( traceHead->net,  ChanList[i].net ); // Network name
		  	strcpy( traceHead->chan, ChanList[i].comp ); // Component/channel code
		  	strcpy( traceHead->loc, ChanList[i].loc );  // Location code
		  	traceHead->pinno = i+1;						// Pin number

		  	/* Set the trace start and end times. Times are in seconds since midnight 1/1/1970 */
		  	traceHead->starttime = Tbuf.t - 11676096000.;
		  	traceHead->endtime	= traceHead->starttime + (double)(ChanRate - 1) / (double)ChanRate;

		  	/* Set error bits in buffer header */
		  	if( dataHdr.timeRefStatus != TIME_REF_LOCKED )
				traceHead->quality[0] |= TIME_TAG_QUESTIONABLE;

		  	/* Transfer samples to the traceBuf */

			if( vmBoard )  {
			  	lTracePtr = lTraceDat;
			  	lPtr  = &lDataPtr[i];
			  	for ( j = 0; j < ChanRate; j++ )  {
					*lTracePtr++ = *lPtr;
			  		lPtr += Nchan;
		  		}
			}
			else if( AdcDataSize == 4 )  {
			  	lTracePtr = lTraceDat;
			  	sPtr  = &sDataPtr[i];
			  	for ( j = 0; j < ChanRate; j++ )  {
					*lTracePtr++ = *sPtr;
			  		sPtr += Nchan;
		  		}
			}
			else  {
			  	sTracePtr = sTraceDat;
			  	sPtr  = &sDataPtr[i];
			  	for ( j = 0; j < ChanRate; j++ )  {
					*sTracePtr++ = *sPtr;
			  		sPtr += Nchan;
		  		}
			}
		  	rc = tport_putmsg( &OutRegion, &logo, traceBufSize, traceBuf );
		  	if ( rc == PUT_TOOBIG )
				logWrite( "Trace message for channel %d too big", i );
		  	else if ( rc == PUT_NOTRACK )
			  	logWrite( "Tracking error while sending channel %d", i );
		}
		SetCurPos( 0, 0 );
	}
	exitThread = TRUE;
	max = 100;
	while( max-- )  {
		if( exitAck )
			break;
		_sleep( 50 );
	}

	/* Clean up and exit program */
	free( traceBuf );
	free( dataBuf );
	free( ChanList );
	tport_detach( &OutRegion );
	logit( "", "Ws2Ew terminating.\n" );
	return 0;
}

thr_ret ReceiveThread( void *parm )
{
	int ret, sps, numChan, cnt, curCnt, hdrState, inHdr, dataLen, packLen;
	char spsStr[256], chanStr[256];
	BYTE *curPtr, data, crc, *currPkt = packet;
	short *sptr;

	sampleCount = 0;

	connecting = TRUE;

	if( ( sock = socket_ew( AF_INET, SOCK_STREAM, 0)) == -1 )  {
		logWriteThread("Error Opening Socket. Exiting" );
		KillReceiveThread();
		return;
	}
	if( connect_ew( sock, (struct sockaddr*) &saddr, sizeof(saddr), SocketTimeout ) == -1 )  {
		logWriteThread("Error Connecting to %s; Exiting", Host );
		KillReceiveThread();
	}

	if( send_ew( sock, "SEND AD", 7, 0, 10000 ) != 7 )  {
		logWriteThread("Error Connecting to %s; Exiting", Host );
		closesocket_ew( sock, SOCKET_CLOSE_IMMEDIATELY_EW );
		KillReceiveThread();
	}

	ret = recv_ew( sock, inData, 4095, 0, SocketTimeout );
	if( ret <= 0 )  {
		logWriteThread("Bad Response from WinSDR at %s; Exiting", Host );
		closesocket_ew( sock, SOCKET_CLOSE_IMMEDIATELY_EW );
		KillReceiveThread();
	}
	cnt = sscanf( inData, "%s %d %s %d", spsStr, &sps, chanStr, &numChan );
	if( cnt != 4 )  {
		logWriteThread("Bad Response from %s sscanf count=%d; Exiting", Host, cnt );
		closesocket_ew( sock, SOCKET_CLOSE_IMMEDIATELY_EW );
		KillReceiveThread();
	}
	if( sps != ChanRate )  {
		logWriteThread("Incorrect Sample Rate; WinSDR = %d Ws2Ew = %d; Exiting", sps, ChanRate );
		closesocket_ew( sock, SOCKET_CLOSE_IMMEDIATELY_EW );
		KillReceiveThread();
	}
	if( Nchan != numChan )  {
		logWriteThread("Incorrect Number of Channels; WinSDR = %d Ws2Ew = %d; Exiting", numChan, Nchan );
		closesocket_ew( sock, SOCKET_CLOSE_IMMEDIATELY_EW );
		KillReceiveThread();
	}

	logWriteThread("Connected to WinSDR at %s", Host );
	connecting = FALSE;
	connected = TRUE;

	hdrState = curCnt = packLen = crcErrors = 0;
	inHdr = 1;

	while( !exitThread )  {
		if(!curCnt)  {
			ret = recv_ew( sock, inData, 4095, 0, 100 );
			if( ret < 1 )  {
				if( socketGetError_ew() == WOULDBLOCK_EW )
					continue;
				logWriteThread("TCP/IP Connection Lost with %s", Host);
				exitThread = TRUE;
			}
			curPtr = inData;
			curCnt = ret;
		}
		while(curCnt)  {
			--curCnt;
			data = *curPtr++;
			if(inHdr)  {
				if(hdrState == 7)   {				// get flags
					sptr = (short *)&packet[4];
					dataLen = *sptr;
					if(dataLen >= 0 && dataLen <= 4000)  {
						*currPkt++ = data;
						++packLen;
						inHdr = 0;
					}
					else  {
						currPkt = packet;
						packLen = hdrState = 0;
					}
				}
				else if(hdrState == 4 || hdrState == 5 || hdrState == 6)  {	// data len info
					*currPkt++ = data;
					++packLen;
					++hdrState;
				}
				else if(data == hdrStr[hdrState])  {		// see if hdr signature
					*currPkt++ = data;
					++packLen;
					++hdrState;
				}
				else  {
					currPkt = packet;
					packLen = hdrState = 0;
				}
			}
			else  {
				*currPkt++ = data;
				++packLen;
				--dataLen;
				if(!dataLen)  {
					crc = CalcCRC(&packet[4], (short)(packLen - 5));
					if(crc != packet[packLen-1])  {
						++crcErrors;
						packLen = 0;
					}
					else
						NewData();
					inHdr = 1;
					currPkt = packet;
					packLen = hdrState = 0;
				}
			}
		}
	}
	closesocket_ew( sock, SOCKET_CLOSE_IMMEDIATELY_EW );
	KillReceiveThread();
}

void KillReceiveThread()
{
	connected = connecting = FALSE;
	restartTime = time( 0 );
	restart = exitAck = TRUE;
	KillSelfThread();
}

void NewData()
{
	PreHdr *phdr = (PreHdr *)packet;
	DWORD dataLen = ( phdr->len - 1 ) + sizeof( PreHdr );
	BYTE type = phdr->type;
	if( type == ADC_AD_DATA )  {
		RequestSpecificMutex( &tbMx );
		enqueue( &tbQ, packet, dataLen, logo );
    	ReleaseSpecificMutex( &tbMx );
	}
	else if( !NoHostMessages && ( type == ADC_MSG || type == ADC_ERROR || type == ADC_AD_MSG ) )  {
		packet[ dataLen ] = 0;
		logWriteThread("%s", &packet[ sizeof( PreHdr ) ] );
	}
}

void logWriteThread(char *pszFormat, ...)
{
	char buff[256], *pszArguments = (char*)&pszFormat + sizeof( pszFormat );
	vsprintf( buff, pszFormat, pszArguments );
	RequestSpecificMutex( &msgMx );
	enqueue( &msgQ, buff, strlen(buff)+1, logo );
   	ReleaseSpecificMutex( &msgMx );
}

int GetInQueue( BYTE *data )
{
	MSG_LOGO recLogo;
	int dataLen, ret;

	RequestSpecificMutex(&tbMx);
	ret = dequeue(&tbQ, data, &dataLen, &recLogo );
    ReleaseSpecificMutex(&tbMx);
	if( ret < 0 )
		return 0;
	return dataLen;
}

void CheckStatus()
{
	static int test = 0;
	int max, i, diff = 0;
	MSG_LOGO recLogo;
	int msgLen, ret;
	char newMsg[256];

	RequestSpecificMutex(&msgMx);
	ret = dequeue(&msgQ, newMsg, &msgLen, &recLogo );
    ReleaseSpecificMutex(&msgMx);
	if( ret >= 0 && msgLen )
		logWrite("%s", newMsg );

	if( ++test < 10 )
		return;
	test = 0;

	Heartbeat();		/* Beat the heart once per second */

	if( !connected && restart )  {
		diff = time(0) - restartTime;
		if( diff >= RestartWaitTime )  {
			noDataCounter = restart = exitThread = 0;
			logWrite("Restarting Receive Thread");
			if( StartThread( ReceiveThread, (unsigned)THREAD_STACK, &TidReceive ) == -1 )  {
				logWrite( "Error starting receive thread. Exiting." );
				free( traceBuf );
				free( dataBuf );
				free( ChanList );
				tport_detach( &OutRegion );
      			exit( 0 );
			}
		}
	}
	else if( ++noDataCounter >= NoDataWaitTime )  {
		connected = 0;
		noDataCounter = 0;
		exitThread = TRUE;
		max = 100;
		while( max-- )  {
			if( exitAck )
				break;
			_sleep( 50 );
		}
		restart = TRUE;
		restartTime = time( 0 );
	}

	ClearToEOL( 9, 6 );
	if( connected )
		printf( "%u", sampleCount );
	else
		printf( "No Data" );

	ClearToEOL( 15, 4 );
	if( connected )
		printf( "OK - Connected to WinSDR" );
	else if( diff > 0 )
		printf( "Error - Not Connected Restart in %d seconds", RestartWaitTime - diff );
	else if( connecting )
		printf( "Connecting to WinSDR at %s", Host );

	if( !connected )  {
		SetCurPos( 23, 5 );
		printf("????          ");
	}
	SetCurPos(0, 0);
}
