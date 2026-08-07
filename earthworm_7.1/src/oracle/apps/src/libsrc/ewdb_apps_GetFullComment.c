/*
 *   THIS FILE IS UNDER CVS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND edit.
 *
 *    $Id: ewdb_apps_GetFullComment.c,v 1.1 2005/05/12 20:49:49 mark Exp $
 *
 *    Revision history:
 *     $Log: ewdb_apps_GetFullComment.c,v $
 *     Revision 1.1  2005/05/12 20:49:49  mark
 *     Initial checkin
 *
 */

#include <stdlib.h>
#include <ewdb_ora_api.h>

static char szInputBuffer[4000];

int ewdb_apps_GetFullComment(EWDBid idComment, char *szComment, int *iCommentSize)
{
	long len = 0;
	EWDBid idNextComment, idTemp;
	int rc;

	/* Clear out the input buffer */
	strcpy(szComment, "");

	idTemp = idComment;
	do
	{
		/* Get the next snippet of the comment. */
		rc = ewdb_api_GetComment(idTemp, szInputBuffer, &idNextComment);
		if (rc != EWDB_RETURN_SUCCESS)
		{
			if (rc == EWDB_RETURN_WARNING)
			{
				logit ("", "ewdb_api_GetComment: Can't find ID %d\n", idTemp);
			}
			else
				logit("", "Error calling ewdb_api_GetComment\n");

			return EWDB_RETURN_FAILURE;
		}

		/* If this snippet will fit inside the input buffer, copy it to the end of the buffer. */
		if (strlen(szInputBuffer) + len < *iCommentSize)
		{
			strcat(szComment, szInputBuffer);
		}
		len += strlen(szInputBuffer);

		/* Set ourselves up to get the next comment snippet (if there is one) */
		idTemp = idNextComment;
	} while (idNextComment > 0);

	/* Add one for the terminating NULL char */
	len++;

	if (len > *iCommentSize)
	{
		*iCommentSize = len;
		rc = EWDB_RETURN_WARNING;
	}
	else
		rc = EWDB_RETURN_SUCCESS;

	return rc;
}
