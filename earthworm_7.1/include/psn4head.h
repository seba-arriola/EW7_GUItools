/* psn4head.h - Header file for the Psn4Putaway functions.
   Created Dec 14 2005 by Larry Cochrane, Redwood City, PSN
 */

#ifndef PSN4HEAD_H
#define PSN4HEAD_H

/* PSN Type 4 Header Include File */

/* All structures must be byte packed by the compiler */
#pragma pack(1)

/* This structure is used to hold the channel information variable header info */
typedef struct  {
	double sensorOutVolt;
	double ampGain;
	double adcInputVolt;
} ChannelInfo;

/* Time structure used in the PSNType4 header */
typedef struct  {
	WORD 	year;
	char	month;
	char	day;
	char	hour;
	char	minute;
	char	second;
	char	notUsed;
	long	nanoseconds;
} DateTime;

/* Variable information header */
typedef struct  {
	BYTE 	checkNumber;
	BYTE 	id;
	long 	length;				// length of the data that follows this header
} VarHeader;

// flags for flags field below
#define NO_CRC		0x00000001
#define NO_MINMAX	0x00000002

/* See http://www.seismicnet.com/psnformat4.html for more information.
   This structure must me 154 bytes long.
*/
typedef struct  {
	char 	headerID[8];
	long 	variableHdrLength;
	DateTime startTime;
	double 	startTimeOffset;
	double 	spsRate;
	long 	sampleCount;
	long	flags;
	char	timeRefType[3];
	char	timeRefStatus;
	BYTE	sampleType;
	BYTE	sampleCompression;
	double	compIncident;
	double	compAzimuth;
	char	compOrientation;
	BYTE	sensorType;
	double	latitude;
	double	longitude;
	double	elevation;
	char	name[6];
	char	compName[4];
	char 	network[6];
	double	sensitivity;
	double	magCorr;
	short	atodBits;
	double	minimum;
	double	maximum;
	double 	mean;
} PSNType4;

/* Go back to default packing */
#pragma pack()

/* eof psn4head.h */
#endif
