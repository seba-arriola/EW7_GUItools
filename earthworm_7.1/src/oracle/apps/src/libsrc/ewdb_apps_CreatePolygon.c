/*
 *   THIS FILE IS UNDER CVS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND edit.
 *
 *    $Id: ewdb_apps_CreatePolygon.c,v 1.1 2004/11/23 20:41:49 mark Exp $
 *
 *    Revision history:
 *     $Log: ewdb_apps_CreatePolygon.c,v $
 *     Revision 1.1  2004/11/23 20:41:49  mark
 *     Initial checkin
 *
 */

#include <stdlib.h>
#include <ewdb_ora_api.h>

int ewdb_apps_CreatePolygon(EWDB_PolygonStruct *pPolygon, EWDB_PolygonVertexStruct *pVertices, int numVertices)
{
	EWDB_PolygonStruct PolyStruct;
	int i;

	if (pPolygon == NULL || pVertices == NULL || numVertices <= 0)
	{
		logit("", "Invalid parameters passed in to ewdb_apps_CreatePolygon\n");
		return EWDB_RETURN_FAILURE;
	}

	if (ewdb_api_CreatePolygon(pPolygon) != EWDB_RETURN_SUCCESS)
		return EWDB_RETURN_FAILURE;

	for (i = 0; i < numVertices; i++)
	{
		pVertices[i].idPolygon = pPolygon->idPolygon;
		if (ewdb_api_CreatePolygonVert(&pVertices[i])  != EWDB_RETURN_SUCCESS)
			return EWDB_RETURN_FAILURE;
	}

	return EWDB_RETURN_SUCCESS;
}
