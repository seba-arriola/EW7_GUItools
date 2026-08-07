
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_apps_DB2ArcFile.c,v 1.2 2001/07/01 21:55:22 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_apps_DB2ArcFile.c,v $
 *     Revision 1.2  2001/07/01 21:55:22  davidk
 *     Cleanup of the Earthworm Database API and the applications that utilize it.
 *     The "ewdb_api" was cleanup in preparation for Earthworm v6.0, which is
 *     supposed to contain an API that will be backwards compatible in the
 *     future.  Functions were removed, parameters were changed, syntax was
 *     rewritten, and other stuff was done to try to get the code to follow a
 *     general format, so that it would be easier to read.
 *
 *     Applications were modified to handle changed API calls and structures.
 *     They were also modified to be compiler warning free on WinNT.
 *
 *     Revision 1.1  2001/05/15 02:15:22  davidk
 *     Initial revision
 *
 *     Revision 1.8  2001/02/28 17:29:10  lucky
 *     Massive schema redesign and cleanup.
 *
 *     Revision 1.7  2000/10/10 20:16:09  lucky
 *     Fixed the call to InitEWEvent
 *
 *     Revision 1.6  2000/09/07 21:16:21  lucky
 *     Final version after the Review pages were demonstrated.
 *
 *     Revision 1.5  2000/08/30 17:40:25  lucky
 *     The EWEvent structure is now dynamically allocated, either in ewdb_apps_GetDBEventInfo
 *     or in the format converter functions. A new call to InitEWEvent is added.
 *     A new field in the structure, iNumAllocChans, specifies how many channles
 *     have been allocated.
 *
 *     Revision 1.4  2000/08/28 15:36:09  lucky
 *     Changed name of get_db_event.h to db_event.h
 *
 *     Revision 1.3  2000/08/25 18:13:56  lucky
 *     Freed up allocated ArcMsg space before exiting.
 *
 *     Revision 1.2  2000/06/16 18:35:15  lucky
 *     Minor fix of a logit message.
 *
 *     Revision 1.1  2000/06/07 22:15:36  lucky
 *     Initial revision
 *
 *
 */


#include <earthworm.h>
#include <string.h>
#include <ew_event_info.h>
#include <ewdb_apps_utils.h>


#define		ARC_MSG_LEN			100000

int ewdb_apps_DB2ArcFile(EWDBid idEvent, char *ArcFileName)
{

	EWEventInfoStruct	EventInfo;
	char				*ArcMsg;
	FILE				*fp;


	if ((idEvent < 0) || (ArcFileName == NULL))
	{
		logit ("", "ewdb_apps_DB2ArcFile: invalid arguments passed in.\n");
		return EW_FAILURE;
	}

	if ((ArcMsg = (char *) malloc (ARC_MSG_LEN * sizeof (char))) == NULL)
	{
		logit ("", "ewdb_apps_DB2ArcFile: can't malloc ArcMsg.\n");
		return EW_FAILURE;
	}

	if (InitEWEvent (&EventInfo) != EW_SUCCESS)
	{
		logit ("", "ewdb_apps_DB2ArcFile: Call to InitEWEvent failed\n");
		return EW_FAILURE;
	}

	if (ewdb_apps_GetDBEventInfo (&EventInfo, idEvent) != EWDB_RETURN_SUCCESS)
	{
		logit ("", "ewdb_apps_DB2ArcFile: Call to ewdb_apps_GetDBEventInfo failed\n");
		return EW_FAILURE;
	}


	/* Make sure that we got the right event */
	if (idEvent != EventInfo.Event.idEvent)
	{
		logit ("", "ewdb_apps_DB2ArcFile: error retrieving event %d.\n", idEvent);
		return EW_FAILURE;
	}

	if (EWEvent2ArcMsg (&EventInfo, ArcMsg, ARC_MSG_LEN) != EW_SUCCESS)
	{
		logit ("", "ewdb_apps_DB2ArcFile: Call to EWEvent2ArcMsg failed.\n");
		return EW_FAILURE;
	}


	free (EventInfo.pChanInfo);

	if ((fp = fopen (ArcFileName, "wt")) == NULL)
	{
		logit ("", "ewdb_apps_DB2ArcFile: can't open %s\n", ArcFileName);
		return EW_FAILURE;
	}

	fprintf (fp, "%s", ArcMsg);

	fclose (fp);

	/* Free up space */
	free (ArcMsg);


	return(EW_SUCCESS);
}  /* ewdb_apps_DB2ArcFile() */

