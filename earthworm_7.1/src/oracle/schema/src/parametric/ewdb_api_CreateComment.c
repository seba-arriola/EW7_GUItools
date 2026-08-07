/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_api_CreateComment.c,v 1.2 2005/06/15 19:03:12 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_api_CreateComment.c,v $
 *     Revision 1.2  2005/06/15 19:03:12  davidk
 *     DB API Cleanup
 *
 *     Revision 1.1  2005/05/12 20:29:42  mark
 *     Initial checkin
 *
 *
 */

#include <ewdb_ora_api.h>
#include <ewdb_cli_base.h>
#include <ewdb_ew_oci_base.h>
#include <ewdb_oci_base_internal.h>
#include <ewdb_internal_functions.h>

static char Local_szCommentChunk[EWDB_MAX_SQL_STRING_LEN];

int ewdb_api_CreateComment(EWDBid *pidComment, char *IN_szComment)
{
	char * szTemp;
	EWDBid idTemp, idPrevious;

	*pidComment = -1;
	idPrevious = -1;

  for(szTemp = IN_szComment; *szTemp != 0x00; szTemp+= strlen(Local_szCommentChunk),idPrevious = idTemp)
	{
		/* Copy the next block of characters into our local buffer (leave room for NULL terminator) */
		strncpy(Local_szCommentChunk, szTemp, sizeof(Local_szCommentChunk) - 1);
		Local_szCommentChunk[sizeof(Local_szCommentChunk) - 1] = 0x00;

		if (ewdb_internal_CreateComment(&idTemp, Local_szCommentChunk) == EWDB_RETURN_FAILURE)
		{
			logit("", "ewdb_api_CreateComment(): ewdb_internal_CreateComment() failed for chunk:\n####\n%s\n####\n",
            Local_szCommentChunk);
			return EWDB_RETURN_FAILURE;
		}
		
		if(*pidComment == -1)
			*pidComment = idTemp;

		if(idPrevious != -1)
		{
			if (ewdb_internal_UpdateCommentNext(idPrevious, idTemp) == EWDB_RETURN_FAILURE)
			{
				logit("", "ewdb_api_CreateComment: Error ewdb_internal_UpdateCommentNext(%d/%d) failed!\n",
              idPrevious, idTemp);
				return EWDB_RETURN_FAILURE;
			}
		}
  }  /* end for() */

	return EWDB_RETURN_SUCCESS;
}  /* end ewdb_api_CreateComment() */

