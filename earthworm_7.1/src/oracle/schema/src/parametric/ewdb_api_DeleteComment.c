/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_api_DeleteComment.c,v 1.2 2005/06/15 19:03:12 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_api_DeleteComment.c,v $
 *     Revision 1.2  2005/06/15 19:03:12  davidk
 *     DB API Cleanup
 *
 *     Revision 1.1  2005/05/12 20:30:06  mark
 *     Initial checkin
 *
 *
 */

#include <ewdb_ora_api.h>
#include <ewdb_cli_base.h>
#include <ewdb_ew_oci_base.h>
#include <ewdb_internal_functions.h>

static char szTemp[4000];

int ewdb_api_DeleteComment(EWDBid idComment)
{
	EWDBid idTemp, idNext;

	idTemp = idComment;
	do
	{
		if (ewdb_api_GetComment(idTemp, szTemp, &idNext) != EWDB_RETURN_SUCCESS)
		{
			logit("", "ewdb_api_DeleteComment: Error getting %d\n", idTemp);
			return EWDB_RETURN_FAILURE;
		}

		if (ewdb_internal_DeleteComment(idTemp) != EWDB_RETURN_SUCCESS)
		{
			logit("", "ewdb_api_DeleteComment: Error deleting %d\n", idTemp);
			return EWDB_RETURN_FAILURE;
		}

		idTemp = idNext;
	} while (idTemp > 0);

	return EWDB_RETURN_SUCCESS;
}

