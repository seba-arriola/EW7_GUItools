/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: merge.h,v 1.2 2002/05/28 19:33:56 lucky Exp $
 *
 *    Revision history:
 *     $Log: merge.h,v $
 *     Revision 1.2  2002/05/28 19:33:56  lucky
 *     Added header and footer tag text
 *
 *     Revision 1.1  2002/05/28 17:24:05  lucky
 *     Initial revision
 *
 * 
 */

#ifndef _MERGE_H
#define _MERGE_H


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <earthworm.h>
#include <ewdb_ora_api.h>
#include <ew_event_info.h>


/* Config file stuff */
#define EQP_MAXWORD          50
#define MAXPATH         	 512
#define MAX_MERGES_PER_EVENT         	 20

extern char  envEW_LOG[MAXPATH];        /* where environment variable EW_LOG
                                        will be stored */
extern char  DBservice[EQP_MAXWORD];    /* DBMS instance to interact with */
extern char  DBuser[EQP_MAXWORD];       /* UserId to connect to database as */
extern char  DBpassword[EQP_MAXWORD];   /* Password to datasource */
extern char  WebHost[MAXPATH];          /* machine running the webserver */
extern char  BackgroundColor[MAXPATH]; 
extern char  HeaderLogo[MAXPATH]; 
extern char  FooterLogo[MAXPATH]; 
extern char  HeaderTag[MAXPATH]; 
extern char  FooterTag[MAXPATH]; 
extern int   DEBUG;


/* Basic Merge Criteria */
typedef struct _MergeCriteria
{
	double	OriginTimeDiff;
	float	LatDiff;
	float	LonDiff;
	float	DepthDiff;
	float	MagDiff;
} EWDB_MergeCriteria;


typedef struct _CandidateList
{
	EWDBid	idPh;
	EWDBid	idPrefEvent;
} EWDB_CandidateList;

typedef struct _consolidated_merge
{
    EWDBid                  idPh;
    EWDB_EventListStruct    PrefEvt;
    char                    szSource[256];
    char                    szHumanReadable[256];
    int                     NumMerges;
} EWDB_PhenomenaMergeList;



#ifdef _WINNT
#define DIR_SLASH   '\\'
#else
#define DIR_SLASH   '/'
#endif



/* In merge_api_utils.c */
int     EWDB_RetrieveMerges (EWEventInfoStruct *pEvent, 
			EWDB_CandidateList *pCandidateList, int ListLen, int *NumMerges, 
			EWDB_MergeCriteria *pCrit);

int		EWDB_CheckMergePreference (EWEventInfoStruct *pEvent,
							EWDBid idPrefEvent, int *Preference);

int     EWDB_ConsolidateMergeList (EWDB_MergeList *pMergeList, int MergeLen,
                        EWDB_PhenomenaMergeList *pConsMerge, int *NumCons);



#endif _MERGE_H
