/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: urban_hazards.h,v 1.2 2002/05/28 17:24:05 lucky Exp $
 *
 *    Revision history:
 *
 */


typedef struct chaninfo
{
  char    	Sta[10];
  char    	Comp[10];
  char    	Net[10];
  float   	Lat;				/* latitude (North=positive)       */
  float   	Lon;				/* longitude(East=positive)        */
  float   	Elev;			    /* elevation                       */
  float   	Azm;				/* component horizontal angle      */
  float   	Dip;				/* component vertical angle        */
  float   	Fullscale;
  float   	Sensitivity;
  float   	SampRate;
  float   	NaturalFrequency;
  float   	Damping;
  char    	PZfile[100];		
  int		Gain;
  int		SensorType;
} ChannelInfo;
