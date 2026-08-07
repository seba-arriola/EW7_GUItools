/*
 *   THIS FILE IS UNDER CVS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND edit.
 *
 *    $Id: ewdb_apps_UpdateFullComment.c,v 1.2 2005/05/16 17:04:33 mark Exp $
 *
 *    Revision history:
 *     $Log: ewdb_apps_UpdateFullComment.c,v $
 *     Revision 1.2  2005/05/16 17:04:33  mark
 *     Added check for new comment
 *
 *     Revision 1.1  2005/05/12 20:49:49  mark
 *     Initial checkin
 *
 */

#include <stdlib.h>
#include <ewdb_ora_api.h>

int ewdb_apps_UpdateFullComment(EWDBid *idComment, char *szComment)
{
	if (*idComment > 0)
	{
		if (ewdb_api_DeleteComment(*idComment) != EWDB_RETURN_SUCCESS)
		{
			return EWDB_RETURN_FAILURE;
		}
	}

	*idComment = 0;

	if (ewdb_api_CreateComment(idComment, szComment) != EWDB_RETURN_SUCCESS)
	{
		return EWDB_RETURN_FAILURE;
	}

	return EWDB_RETURN_SUCCESS;
}

