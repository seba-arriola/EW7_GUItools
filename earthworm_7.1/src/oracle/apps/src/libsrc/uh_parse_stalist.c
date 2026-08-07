


/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: uh_parse_stalist.c,v 1.2 2002/05/28 17:25:41 lucky Exp $
 *
 *    Revision history:
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <kom.h>
#include <earthworm.h>

#include <urban_hazards.h>

int		FillChannelsArray (ChannelInfo *pChans, int *numChans, char *filename)
{

	char	*str;

	if ((pChans == NULL) || (numChans == NULL) || (filename == NULL))
	{
		logit ("", "Invalid arguments passed in.\n");
		return EW_FAILURE;
	}

	if (k_open (filename) == 0)
	{
		logit ("", "Error opening file %s\n", filename);
		return EW_FAILURE;
	}

	*numChans = 0;
	while (k_rd ())
	{

		/* get STA code */
		if ((str = k_str()) != NULL)
			strcpy (pChans[*numChans].Sta, str);
		else
		{
			logit ("", "Invalid station file format - STA code not found.\n");
			return EW_FAILURE;
		}

		/* get COMP code */
		if ((str = k_str()) != NULL)
			strcpy (pChans[*numChans].Comp, str);
		else
		{
			logit ("", "Invalid station file format - COMP code not found.\n");
			return EW_FAILURE;
		}

		/* get NET code */
		if ((str = k_str()) != NULL)
			strcpy (pChans[*numChans].Net, str);
		else
		{
			logit ("", "Invalid station file format - NET code not found.\n");
			return EW_FAILURE;
		}

		/* get latitude */
		pChans[*numChans].Lat = (float) k_val ();

		/* get longitude */
		pChans[*numChans].Lon = (float) k_val ();
	
		/* get elevation */
		pChans[*numChans].Elev = (float) k_val ();

		/* get azimuth */
		pChans[*numChans].Azm = (float) k_int ();
		
		/* get dip */
		pChans[*numChans].Dip = (float) k_int ();

		/* get sampling rate */
		pChans[*numChans].SampRate = (float) k_val ();
		
		/* get gain */
		pChans[*numChans].Gain = (int) k_int ();

		/* get Fullscale */
		pChans[*numChans].Fullscale = (float) k_val ();

		/* get sensitivity */
		pChans[*numChans].Sensitivity = (float) k_val ();

		/* get natural frequency */
		pChans[*numChans].NaturalFrequency = (float) k_val ();

		/* get damping */
		pChans[*numChans].Damping = (float) k_val ();

		/* get sensor type */
		pChans[*numChans].SensorType = (int) k_int ();


		/* get P&Z file name */
		if ((str = k_str()) != NULL)
			strcpy (pChans[*numChans].PZfile, str);
		else
		{
			logit ("", "Invalid station file format - PZfile name not found.\n");
			return EW_FAILURE;
		}

		*numChans = *numChans + 1;

	} /* while k_rd () */

	k_close ();

	return EW_SUCCESS;

}

