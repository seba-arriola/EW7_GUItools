
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_apps_ArcFile2DB.c,v 1.2 2001/07/01 21:55:21 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_apps_ArcFile2DB.c,v $
 *     Revision 1.2  2001/07/01 21:55:21  davidk
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
 *     Revision 1.7  2001/02/28 17:29:10  lucky
 *     Massive schema redesign and cleanup.
 *
 *     Revision 1.6  2000/09/07 21:16:21  lucky
 *     Final version after the Review pages were demonstrated.
 *
 *     Revision 1.5  2000/08/30 17:40:25  lucky
 *     The DBEvent structure is now dynamically allocated, either in ewdb_apps_GetDBEventInfo
 *     or in the format converter functions. A new call to InitEWEvent is added.
 *     A new field in the structure, iNumAllocChans, specifies how many channles
 *     have been allocated.
 *
 *     Revision 1.4  2000/08/28 15:36:09  lucky
 *     Changed name of get_db_event.h to db_event.h
 *
 *     Revision 1.3  2000/08/25 18:08:56  lucky
 *     Added free call to free up allocated ArcMsg space (memory leak fix)
 *
 *     Revision 1.2  2000/06/13 18:55:12  lucky
 *     Cleaned up debugging statements.
 *
 *     Revision 1.1  2000/06/07 22:15:36  lucky
 *     Initial revision
 *
 *
 */



#include <string.h>
#include <earthworm.h>
#include <ew_event_info.h>
#include <ewdb_apps_utils.h>


#define		ARC_MSG_LEN			100000

int ewdb_apps_ArcFile2DB(char *ArcFileName, char *author, char *szEventID)
{

	EWEventInfoStruct	EventInfo;
	char				*ArcMsg;
	FILE				*fp;
	int					retval;


	if ((ArcFileName == NULL) || (author == NULL))
	{
		logit ("", "ewdb_apps_ArcFile2DB: invalid arguments passed in.\n");
		return EW_FAILURE;
	}

	if ((ArcMsg = (char *) malloc (ARC_MSG_LEN * sizeof (char))) == NULL)
	{
		logit ("", "ewdb_apps_ArcFile2DB: can't malloc ArcMsg.\n");
		return EW_FAILURE;
	}


	/* Read the contents of the Arc File */

	if ((fp = fopen (ArcFileName, "rt")) == NULL)
	{
		logit ("", "ewdb_apps_ArcFile2DB: can't open %s.n", ArcFileName);
		return EW_FAILURE;
	}


	retval = fread ((void *) ArcMsg, sizeof (char), ARC_MSG_LEN, fp);

	fclose (fp);


	if (ArcMsg2EWEvent (&EventInfo, ArcMsg, retval) != EW_SUCCESS)
	{
		logit ("", "ewdb_apps_ArcFile2DB: Call to ArcMsg2EWEvent failed.\n");
		return EW_FAILURE;
	}


	/* Free up allocated space */
	free (ArcMsg);

	if (ewdb_apps_PutDBEventInfo (&EventInfo, author, szEventID, 0) 
											!= EWDB_RETURN_SUCCESS)
	{
		logit ("", "ewdb_apps_ArcFile2DB: Call to ewdb_apps_PutDBEventInfo failed\n");
		return EW_FAILURE;
	}


	/* Free pChans */
	free (EventInfo.pChanInfo);

	return EW_SUCCESS;
}  /* end ewdb_apps_ArcFile2DB() */

