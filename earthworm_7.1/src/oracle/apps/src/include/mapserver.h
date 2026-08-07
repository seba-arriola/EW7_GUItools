
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: mapserver.h,v 1.1 2001/02/28 17:29:10 lucky Exp $
 *    Revision history:
 *
 *    $Log: mapserver.h,v $
 *    Revision 1.1  2001/02/28 17:29:10  lucky
 *    Initial revision
 *
 *    Revision 1.2  1999/11/09 16:55:39  lucky
 *    *** empty log message ***
 *
 *    Revision 1.1  1999/10/18 15:56:20  davidk
 *    Initial revision
 *
 *    Revision 1.1  1999/05/05 18:22:00  lucky
 *    Initial revision
 *
 *
 */
  
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


typedef struct EWDB_MS_MapInfoStruct {

	float		lat1;
	float		lat2;
	float		lon1;
	float		lon2;
	float		x_annot;
	float		x_grid;
	float		x_frame;
	float		y_annot;
	float		y_grid;
	float		y_frame;
	int		fill_shade;
	int		boundaries;
  double lon_degrees_per_inch;
  double lat_degrees_per_inch;
  int   rivers;
	
} EWDB_MS_MapInfoStruct;


#define	EWDB_MS_MAP_SUCCESS		 0
#define	EWDB_MS_MAP_FAILURE		-1
#define	TRUE			1
#define	FALSE			0


#define	EWDB_MS_MAX_COMMAND_LENGTH 	1024
#define	EWDB_MS_ERROR_FILE 	"/tmp/EWMS_error_file"

#define	EWDB_MS_MAKE_MAP 	"./make_map"


/* Functions defined in mapserver.c */
EWDB_MS_MapInfoStruct * EWDB_MS_InitMapStruct (void);
int  EWDB_MS_CreateMap (EWDB_MS_MapInfoStruct *, char *);
/* End functions defined in mapserver.c */
