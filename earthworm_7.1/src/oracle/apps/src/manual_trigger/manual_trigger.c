
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: manual_trigger.c,v 1.1 2004/07/01 19:13:35 labcvs Exp $
 *
 *    Revision history:
 *     $Log: manual_trigger.c,v $
 *     Revision 1.1  2004/07/01 19:13:35  labcvs
 *     Moved from src/data_sources to src/oracle/apps/src JMP
 *
 *     Revision 1.2  2002/02/25 18:30:44  lucky
 *     Added idEvent
 *
 *     Revision 1.1  2001/11/12 18:16:01  lucky
 *     Initial revision
 *
 *
 *
 */

  /*****************************************************************
   *                   manual_trigger.c                            *
   *                                                               *
   * Prompts the user for the hypocenter and magnitude information *
   * which is written out to the output ring as a hypoinverse      *
   * archive message suitable for processing with orareport.       *
   *                                                               *
   * Usage: manual_trigger OUT_RING                                *
   *                                                               *
   *      Lucky Vidmar, 10/23/2001                                 *
   *****************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <transport.h>
#include <earthworm.h>
#include <ew_event_info.h>
#include <ewdb_ora_api.h>

static int ConvertTime (char *, double *);

#define		MODULE_ID	"MOD_MANUAL_TRIGGER"

int main (int argc, char *argv[])
{
	char		tmpstr[100];
	char		ArcMsg[512];
	SHM_INFO	region;
	MSG_LOGO	logo;
	long		RingKey;         /* Key to the transport ring to write to */
	int			type, mod, inst;
	EWEventInfoStruct	Event;
	

	/* Check command line arguments
	 ****************************/
	if (argc < 2)
	{
		printf ("Usage: manual_trigger <output ring>\n");
		exit( -1 );
	}

	/* Attach to the output ring
	 **************************/
	if ((RingKey = GetKey (argv[1])) == -1)
	{
		printf ("Invalid RingName <%s> ; exiting!\n", argv[1]);
		exit (-1);
	}
	tport_attach (&region, RingKey);

	/* Get local installation id for the logo */
	if (GetLocalInst (&logo.instid) != 0)
	{
		printf ("Could not determine local installation id; exit!\n");
		exit (-1);
	}

	/* Get the module id -- this must be in earthworm.d */
	if (GetModId (MODULE_ID, &logo.mod) != 0)
	{
		printf ("Could not determine module id for <%s>; exit!\n", MODULE_ID);
		exit (-1);
	}

	/* Get the message type -- always HypoArc */
	if (GetType ("TYPE_HYP2000ARC", &logo.type) != 0)
	{
		printf ("Could not determine module id for <%s>; exit!\n", MODULE_ID);
		exit (-1);
	}



    /* Initialize the Event structure */
    if (InitEWEvent (&Event) != EW_SUCCESS)
    {
        printf ("Could not initialize the Event structure; exit!\n");
        exit (-1);
    }


	/* Prompt the user 
	 ******************/
	printf ("======== MANUAL EVENT TRIGGER ========\n\n"
			"You will be prompted to enter the event's\n"
			"time, hypocenter, and magnitude information.\n\n");
 

	/* Event Time */
	printf ("Enter Event Time (YYYYMMDDHHMMSS): ");
	scanf ("%s", tmpstr);

	if (ConvertTime (tmpstr, &Event.PrefOrigin.tOrigin) != EW_SUCCESS)
	{
		printf ("Call to ConvertTime failed; exiting\n");
		exit (-1);
	}

	/* Event ID */
	Event.Event.idEvent = (EWDBid) time (NULL);

	/* Latitude */
	printf ("Enter Event Latitude (+/-DD.MM): ");
	scanf ("%s", tmpstr);
	Event.PrefOrigin.dLat = atof (tmpstr);

	/* Longtidue */
	printf ("Enter Event Longitude (+/-DDD.MM): ");
	scanf ("%s", tmpstr);
	Event.PrefOrigin.dLon = atof (tmpstr);

	/* Depth */
	printf ("Enter Event Depth (in km): ");
	scanf ("%s", tmpstr);
	Event.PrefOrigin.dDepth = atof (tmpstr);

	/* magnitude */
	printf ("Enter Event Magnitude (mm.m): ");
	scanf ("%s", tmpstr);
	Event.Mags[0].dMagAvg = atof (tmpstr);

	/* Fill in other required fields */
	Event.iNumChans = 0;
	Event.iNumMags = 1;
	Event.iPrefMag = 0;
	Event.iMd = 0;
	Event.iML = -1;

	if (EWEvent2ArcMsg (&Event, ArcMsg, 512) != EW_SUCCESS)
	{
		printf ("Call to EWEvent2ArcMsg failed; event not triggered!\n");
		exit (-1);
	}


	if (tport_putmsg (&region, &logo, strlen (ArcMsg), ArcMsg) != PUT_OK)
		printf ("Error sending message to region %d\n",region.key);
	else
		printf ("Message was written to ring.\n"); 


/* We're done
   **********/
   tport_detach( &region );
   return 0;
}


/***********************************************************************
 * ConvertTime () - given pStart return a double representing          *
 *     number of seconds since 1970                                    *
 ***********************************************************************/
static int ConvertTime (char *pStart, double *start)
{
  char	YYYYMMDD[9];
  char	HHMMSS[12];

  if (pStart == NULL) 
  {
    printf ("Invalid parameters passed in.\n");
    return EW_FAILURE;
  }

  strncpy (YYYYMMDD, pStart, (size_t) 8);
  YYYYMMDD[8] = '\0';

  HHMMSS[0] = pStart[8];
  HHMMSS[1] = pStart[9];
  HHMMSS[2] = ':';
  HHMMSS[3] = pStart[10];
  HHMMSS[4] = pStart[11];
  HHMMSS[5] = ':';
  HHMMSS[6] = pStart[12];
  HHMMSS[7] = pStart[13];
  HHMMSS[8] = '.';
  HHMMSS[9] = '0';
  HHMMSS[10] = '0';
  HHMMSS[11] = '\0';

  if (t_atodbl (YYYYMMDD, HHMMSS, start) < 0)
  {
    printf ("Can't convert StartTime %s -> %s %s: t_atodbl failed.\n", 
           pStart, YYYYMMDD, HHMMSS);
    return EW_FAILURE;
  }
	
  return EW_SUCCESS;
}
