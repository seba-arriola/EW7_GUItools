// PsnAdSend.h

#define ADC_CMD_EXIT			0
#define ADC_CMD_SEND_STATUS		1
#define ADC_CMD_RESET_GPS		2
#define ADC_CMD_FORCE_TIME_TEST	3
#define ADC_CMD_CLEAR_COUNTERS	4
#define ADC_CMD_GPS_DATA_ON_OFF	5
#define ADC_CMD_GPS_ECHO_MODE	6
#define ADC_CMD_RESET_BOARD		7
#define ADC_CMD_GOTO_BOOTLOADER	8
#define ADC_CMD_SEND_TIME_INFO	9

#define ADC_GET_BOARD_TYPE		0
#define ADC_GET_DLL_VERSION		1
#define ADC_GET_DLL_INFO		2

#define ADC_MSG					0
#define ADC_ERROR				1
#define ADC_AD_MSG				2
#define ADC_AD_DATA				3
#define ADC_GPS_DATA			4
#define ADC_STATUS				5
#define ADC_SAVE_TIME_INFO		6
#define TCP_CHANNEL_INFO		7

#define ADC_BOARD_ERROR			0
#define ADC_NO_DATA				1
#define ADC_GOOD_DATA			2

#define TIME_REF_NOT_LOCKED		0
#define TIME_REF_WAS_LOCKED		1
#define TIME_REF_LOCKED			2

#define	MAX_ADC_CHANNELS		8
#define MAX_SPS_RATE			200

#define TIMEUPDATE    			600		// Update PC clock every X times through

#define MESSAGE_LEN				79
#define MAX_MESSAGES			10

#define MESSAGE_START_ROW		10

/* ADC Board Types */
#define BOARD_UNKNOWN			0
#define BOARD_RABBIT			1
#define BOARD_PICC				2
#define BOARD_VM				3

#pragma pack(1)

typedef struct  {
	DWORD maxInQueue;
	DWORD maxUserQueue;
	DWORD maxOutQueue;
	DWORD crcErrors;
	DWORD userQueueFullCount;
	DWORD xmitQueueFullCount;
	DWORD cpuLoopErrors;
} DLLInfo;

typedef struct  {
	ULONG addDropTimer;
	BYTE addTimeFlag;
} AdjTimeInfo;

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
	ULONG commPort;
	ULONG commSpeed;
	ULONG numberChannels;
	ULONG sampleRate;
	ULONG timeRefType;
	ULONG addDropTimer;
	ULONG pulseWidth;
	ULONG mode12BitFlags;
	ULONG highToLowPPS;
	ULONG noPPSLedStatus;
	ULONG checkPCTime;
	ULONG setPCTime;
	long addDropMode;
	long timeOffset;
} AdcBoardConfig;

typedef struct  {
	char addDropFlag, flags;
	WORD pulseWidth;
	long addDropCount, timeLocked, adjNum;
	short avgDiff, timeOffset;
} TimeInfo;

#define BOARD_TYPE_SDR_SERVER		0
#define BOARD_TYPE_RABBIT_CPU		1		// V1.x boards
#define BOARD_TYPE_PICC_CPU			2		// V2.x boards

typedef struct  {
	BYTE boardType;
	BYTE majorVersion;
	BYTE minorVersion;
	BYTE lockStatus;
	BYTE numChannels;
	BYTE spsRate;
	ULONG crcErrors;
	ULONG numProcessed;
	ULONG numRetran;
	ULONG numRetranErr;
	ULONG packetsRcvd;
	TimeInfo timeInfo;
} StatusInfo;

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
} SCNL;

struct Greg {
	int year;
	int month;
	int day;
	int hour;
	int minute;
};

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
extern int  CommPort;			// Comm port number to use
extern long	PortSpeed;			// Comm port baud rate
extern int  TimeRefType;		// Time reference type. See .d file for types
extern int  HighToLowPPS;
extern int  NoPPSLedStatus;
extern int  TimeOffset;
extern int	TimeoutNoSend;      // Take action if no data sent for this many seconds
extern int  HeartbeatInt;     	// Heartbeat interval in seconds
extern char TimeFilePath[256];	// Path to where the the time.dat file should be placed
extern int	LogMessages;		// Log messages from the DLL and ADC board
extern int  AdcDataSize;		// data byte size; Can be 2 or 4

/* Functions prototypes */
#ifdef __cplusplus
extern "C" {
#endif

#define PSNADBOARD_API __declspec(dllimport)
PSNADBOARD_API HANDLE PSNOpenBoard();
PSNADBOARD_API BOOL PSNCloseBoard( HANDLE );
PSNADBOARD_API BOOL PSNConfigBoard( HANDLE, AdcBoardConfig *, void (*f)( DWORD, void *, void *, DWORD ) );
PSNADBOARD_API BOOL PSNStartStopCollect( HANDLE, DWORD start );
PSNADBOARD_API DWORD PSNGetBoardData( HANDLE hBoard, DWORD *type, void *data, void *data1, DWORD *dataLen );
PSNADBOARD_API BOOL PsnAdSendBoardCommand( HANDLE hBoard, DWORD cmd, void *data );
PSNADBOARD_API BOOL PSNGetBoardInfo( HANDLE hBoard, DWORD type, void *data );

#ifdef __cplusplus
}
#endif

int  GetArgs( int, char **, char ** );
int  GetConfig( char * );
int  GetDAQ( long *, unsigned long );
BOOL InitDAQ();
void Heartbeat( void );
void InitCon( void );
void LogConfig( void );
void LogSta( int, SCNL * );
void PrintGmtime( double, int );
void LogitGmtime( double, int );
void ReadTimeInfo( TimeInfo *info );
void SaveTimeInfo( TimeInfo *info );
int  ReadSta( char *, unsigned char, int, SCNL * );
void ReportError( int, char * );
void SetCurPos( int, int );
void StopDAQ( void );
ULONG CalcPacketTime( SYSTEMTIME *st );
void ClearToEOL( int, int );
void logWrite(char *, ...);
