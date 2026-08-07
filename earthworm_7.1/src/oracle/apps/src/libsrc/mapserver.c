/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: mapserver.c,v 1.5 2002/03/15 02:23:54 davidk Exp $
 *
 *    Revision history:
 *     $Log: mapserver.c,v $
 *     Revision 1.5  2002/03/15 02:23:54  davidk
 *     changed a bug in the code, so that the Map Database Resolution option "-Dx"
 *     gets concated to the correct string (gmt_str not cmd).  Now we get better
 *     resolution maps for small areas.
 *
 *     Revision 1.4  2001/08/29 17:15:41  lucky
 *     *** empty log message ***
 *
 *     Revision 1.3  2001/08/06 16:56:05  lucky
 *     Added support for map generation under NT
 *
 *     Revision 1.2  2001/07/20 17:40:05  lucky
 *     State of the code after much of v6.0 testing and debugging
 *
 *     Revision 1.1  2001/02/28 17:29:10  lucky
 *     Initial revision
 *
 *     Revision 1.4  2000/06/21 22:45:07  lucky
 *     Cleaned up logit calls to make log files more readable.
 *
 *     Revision 1.3  2000/03/14 01:45:39  davidk
 *     changed the lat/lon numbers outputted from the draw_map .c routines
 *     to GMT, to have 3 digits of precision after the decimal point instead of
 *     two.  The two digits was causing rounding anomalies between the size
 *     of the GIF that the program was expecting back from make_map, and
 *     the size that GMT was generating.  This is discoverable by comparing the
 *     actual GIF size with the expected GIF size in the getlist or eqparam2html
 *     web pages.  This problem was also causing the locations of station/quake
 *     objects on the GIF to be misaligned with the client-side-image-map locations
 *     of the objects (you wouldn't get the little hand pointer when you put the
 *     mouse on a station, but you would get it if you put the mouse just below
 *     the station).
 *
 *     Revision 1.2  2000/03/13 23:32:33  davidk
 *     removed an old incorrect logit() statement.
 *
 *     Revision 1.1  1999/11/09 17:40:05  lucky
 *     Initial revision
 *
 *
 */


#include <mapserver.h>


/* EWDB_MS_  is the prefix for mapserver related structures and fucntions */
#ifdef _WINNT
  int getpid( void ); /* from process.h on WinNT */
#endif _WINNT
/* External functions */
void   logit( char *, char *, ... );  /* Logging Function  */

/* end Externally defined functions */

/*
 * The functions in this file implement a simple map server
 * that returns a map (as a GIF image) created by the GMT's
 * pscoast utility.  The map is created using the GMT parameters
 * passed in via the MapInfo structure. This structure must first 
 * be allocated and initialized using the InitMapStruct() call.
 *
 * CreateMap() function builds a command line system () call to 
 * a script called MAKE_MAP (defined in mapserver.h). This script
 * then calls pscoast to create a PostScript version of the map, 
 * and ps2gif to crop the image and convert it into GIF format.
 * The resulting map image is written to the file specified as the 
 * second argument to CreateMap. 
 *
 *   Lucky Vidmar Fri Jan 22 16:12:47 MST 1999
 *
 */


/*
 *  Allocate the MapInfo structure and fill it with 
 *  default values. Return NULL in case of any errors.
 *
 *  This function should be called, and its success determined,
 *  before the call to CreateMap is made.
 */
EWDB_MS_MapInfoStruct	*EWDB_MS_InitMapStruct (void)
{

	EWDB_MS_MapInfoStruct		*map;


	/** Allocate the map info structure **/
	if ((map = (EWDB_MS_MapInfoStruct *) malloc (sizeof (EWDB_MS_MapInfoStruct))) == NULL)
	{
		fprintf (stderr, "Couldn't malloc map.\n");
		return NULL;
	}


	/** Fill the structure with defaults **/

	map->lat1 = -55;
	map->lat2 = 305;
	map->lon1 = -90;
	map->lon2 = 90;
	map->x_annot = 30;
	map->x_grid = 30;
	map->x_frame = 15;
	map->y_annot = 30;
	map->y_grid = 30;
	map->y_frame = 15;
	map->fill_shade = 200;
	map->boundaries = TRUE;
	return (map);

}  


/*
 * Call make_map script with the following arguments:
 * 
 *    1. Name of the resulting map file
 *    2. Name of the file containing the error indication
 *    3. List of map making arguments 
 *
 *
 *  Check the error file - in case of any failures report
 *  them to stderr and return MAP_FAILURE value. If the map  
 *  has been generated successfully, return MAP_SUCCESS.
 *
 */
int		EWDB_MS_CreateMap (EWDB_MS_MapInfoStruct *map, char *filename)
{

	char 	cmd[EWDB_MS_MAX_COMMAND_LENGTH];
	char 	gmt_str[EWDB_MS_MAX_COMMAND_LENGTH];
	char 	tmp[EWDB_MS_MAX_COMMAND_LENGTH];

	if ((map == NULL) || (filename == NULL))
	{
		fprintf (stderr, "Invalid arguments passed in.\n");
		return(EWDB_MS_MAP_FAILURE);
	}

	
	/* Build  GMT arguments */

	/* Map region */
	sprintf (gmt_str, "-P -R%.3f/%.3f/%.3f/%.3f ", map->lon1, map->lon2, 
							map->lat1, map->lat2);

	/* Map Projection */
	sprintf(tmp, "-Jx%.4f/%.4fd ",
           1.0/map->lon_degrees_per_inch,1.0/map->lat_degrees_per_inch);

	strcat (gmt_str, tmp);

	/* Border attributes */
	sprintf (tmp, "-B%.2fg%.2ff%.2f/%.2fg%.2ff%.2fwsen ", 
				map->x_annot, map->x_grid, map->x_frame,
				map->y_annot, map->y_grid, map->y_frame);

	strcat (gmt_str, tmp);


	/* Fill color */
	sprintf (tmp, "-G255/204/153 ");
	strcat (gmt_str, tmp);


	/* Boundaries */
	if (map->boundaries)
  {
    if(map->boundaries & 1)
      strcat(gmt_str,"-N1 ");
    if(map->boundaries & 2)
      strcat(gmt_str,"-N2 ");
    if(map->boundaries & 4)
      strcat(gmt_str,"-N3 ");
    if(map->boundaries & 8)
      strcat(gmt_str,"-Na ");
  }

  /* Only use intermediate for small images (in terms of degrees) */
  if (((map->lat1 - map->lat2) <= 6.0) &&  ((map->lat2 - map->lat1) <= 6.0))
  {
    strcat(gmt_str,"-Di ");
  }
  else
  {
		;
  }

	/* Boundaries */
	if (map->boundaries)
  {
    if(map->boundaries & 1)
      strcat(gmt_str,"-N1/1/102/102/102 ");
    if(map->boundaries & 2)
      strcat(gmt_str,"-N2/1/102/102/102 ");
    if(map->boundaries & 4)
      strcat(gmt_str,"-N3/1/102/102/102 ");
    if(map->boundaries & 8)
      strcat(gmt_str,"-Na/1/102/102/102 ");
  }

	/* Rivers */
	if (map->rivers)
  {
    if(map->rivers & 1)
      strcat(gmt_str,"-I1/3/51/204/255 ");
    if(map->rivers & 2)
      strcat(gmt_str,"-I2/2/51/204/255 ");
    if(map->rivers & 4)
      strcat(gmt_str,"-I3/1/51/204/255 ");
    if(map->rivers & 8)
      strcat(gmt_str,"-I4/1/51/204/255 ");
    if(map->rivers & 16)
      strcat(gmt_str,"-I5/1/51/204/255 ");
    if(map->rivers & 32)
      strcat(gmt_str,"-I6/1/51/204/255 ");
    if(map->rivers & 64)
      strcat(gmt_str,"-I7/1/51/204/255 ");
    if(map->rivers & 128)
      strcat(gmt_str,"-I8/1/51/204/255 ");
    if(map->rivers & 256)
      strcat(gmt_str,"-I9/1/51/204/255 ");
    if(map->rivers & 512)
      strcat(gmt_str,"-I10/1/51/204/255 ");
    if(map->rivers & 1024)
      strcat(gmt_str,"-Ia/1/51/204/255 ");
    if(map->rivers & 2048)
      strcat(gmt_str,"-Ir/1/51/204/255 ");
      strcat(gmt_str,"-I1/3/51/204/255 ");
    if(map->rivers & 4096)
      strcat(gmt_str,"-Ii/1/51/204/255 ");
      strcat(gmt_str,"-I1/3/51/204/255 ");
    if(map->rivers & 8192)
      strcat(gmt_str,"-Ic/1/51/204/255 ");
  }


  /* Coastlines */
  strcat(gmt_str,"-W2/102/153/255 ");

  /* Color of Water */
  strcat(gmt_str,"-S204/255/255 ");


#ifndef _WINNT

	/* Build the solaris make_map call string */
	sprintf (cmd, "%s %s %s_%d \"%s\" %d", EWDB_MS_MAKE_MAP, filename, EWDB_MS_ERROR_FILE, 
										getpid(), gmt_str, getpid());
	system (cmd);

	/** Remove the error file **/
	sprintf (tmp, "%s_%d ", EWDB_MS_ERROR_FILE,getpid());
	sprintf (cmd, "rm -f %s", tmp);
	system (cmd);

#else

	/* Build the map on NT */

	/* Initialize GMT */
    sprintf (cmd, "\\gmt\\bin\\gmtset DEGREE_FORMAT 3");
    system (cmd);

	/* Create desired map */
    sprintf (cmd, "\\gmt\\bin\\pscoast %s > \\gmttmp\\EWPS_tmpfile_%d",
                                            gmt_str, getpid());
    system (cmd);

    /* 
	 * The call above should have created a PS file under \gmttmp.
	 * The next call will convert it into a cropped GIF image.
	 */
    sprintf (cmd, 
		"\\imagemagick\\convert -crop 0x0 -colors 128 \\gmttmp\\EWPS_tmpfile_%d %s", 
							getpid(), filename);
    system (cmd);

#endif 

	return EWDB_MS_MAP_SUCCESS;

}  
