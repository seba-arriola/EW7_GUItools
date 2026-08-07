// Ws2Ew.h

#include <mem_circ_queue.h>

#define TIME_REF_NOT_LOCKED		0
#define TIME_REF_WAS_LOCKED		1
#define TIME_REF_LOCKED			2

#define	MAX_ADC_CHANNELS		8
#define THREAD_STACK    		8192    /* How big is our thread stack          */

#define ADC_MSG					0
#define ADC_ERROR				1
#define ADC_AD_MSG				2
#define ADC_AD_DATA				3

#define MESSAGE_LEN				79
#define MAX_MESSAGES			10

#define MESSAGE_START_ROW		10

#pragma pack(1)

typedef struct  {
	BYTE hdr[4];
	WORD len;
	BYTE type, flags;
} PreHdr;

typedef struct  {
	SYSTEMTIME packetTime;
	ULONG packetID;
	BYTE timeRefStatus;
	BYTE flags;
} DataHeader;

typedef struct  {
	ULONG timeTick;
	ULONG utcTime;
} NewTimeInfo;

#pragma pack()

typedef struct
{
   char sta[6];
   char comp[4];
   char net[3];
   char loc[3];
   char send;
} SCNL;

#define TIME_OK        1
#define TIME_WAIT      0
#define TIME_NOSYNC   -1
#define TIME_FLAT     -2
#define TIME_NOISE    -3
#define TIME_UNKNOWN -99

struct TIME_BUFF {
	double  t;              /* Time at scan count                   */
};

/* Global configuration parameters */
extern unsigned char ModuleId;  // Data source id placed in the trace header
extern int  ChanRate;          	// Rate in samples per second per channel
extern long OutKey;            	// Key to ring where traces will live
extern int  UpdateSysClock;    	// Update PC clock with good IRIGE time
extern SCNL *ChanList;         	// Array to fill with SCNL values
extern int  Nchan;             	// Number of channels in ChanList
extern int  HeartbeatInt;     	// Heartbeat interval in seconds
extern int Port;				// Port number
extern int SocketTimeout;		// Time in milliseconds
extern int NoDataWaitTime;		// Time to wait for no data. If no data reset recv thread. In seconds.
extern int RestartWaitTime;		// Time to wait between reconnects. In seconds.
extern int NoHostMessages;		// If set to 1, no log messages from the host will be processed
extern int  AdcDataSize;		// data byte size; Can be 2 or 4
extern char Host[256];			// Host name or IP Address
extern SHM_INFO OutRegion;		// Info structure for output region
extern pid_t MyPid;				// process id, sent with heartbeat
extern HANDLE outHandle;		// The console file handle
extern struct sockaddr_in saddr; // Socket address structure

int  GetArgs( int, char **, char ** );
int  GetConfig( char * );
void Heartbeat( void );
void InitCon( void );
void LogAndReportError( int, char * );
void LogConfig( void );
void LogSta( int, SCNL * );
void PrintGmtime( double, int );
//void LogitGmtime( double, int );
int  ReadSta( char *, unsigned char, int, SCNL * );
void ReportError( int, char * );
void SetCurPos( int, int );
ULONG CalcPacketTime( SYSTEMTIME * );
int InitSocket();
int StartReceiveThread();
int GetInQueue( BYTE * );
BYTE CalcCRC( BYTE *, int );
void NewData();
void CheckStatus();
void ClearToEOL( int, int );
void logWrite(char *, ...);
void logWriteThread(char *, ...);
void KillReceiveThread();
