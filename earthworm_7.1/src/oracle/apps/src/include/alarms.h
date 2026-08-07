/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: alarms.h,v 1.14 2004/12/15 18:28:25 mark Exp $
 *
 *    Revision history:
 *     $Log: alarms.h,v $
 *     Revision 1.14  2004/12/15 18:28:25  mark
 *     Made building CUBE and hypoTWC messages available to external functions
 *
 *     Revision 1.13  2002/11/01 16:35:13  lucky
 *     *** empty log message ***
 *
 *     Revision 1.12  2002/05/28 19:33:56  lucky
 *     Added header and footer tag text
 *
 *     Revision 1.11  2002/05/28 17:24:05  lucky
 *     *** empty log message ***
 *
 *     Revision 1.10  2002/02/20 19:52:50  lucky
 *     Added header/footer and bkg color info variables
 *
 *     Revision 1.9  2001/08/06 21:28:23  lucky
 *     Added NT support
 *
 *     Revision 1.8  2001/07/28 00:43:14  lucky
 *     State of the code after debugging and testing prior to v6.0 release freeze.
 *
 *     Revision 1.7  2001/07/20 17:38:48  lucky
 *     Added InvocationString to functions and structs to keep track of who declared the alarm
 *
 *     Revision 1.6  2001/07/11 18:16:12  lucky
 *     Updated prototype for DetermineAlarms to include the InvocationString
 *
 *     Revision 1.5  2001/06/26 18:08:17  lucky
 *     Updated prototype for Determine Alarms now that NetworkCode is a parameter
 *
 *     Revision 1.4  2001/06/26 17:35:34  lucky
 *     State of the code after the Utah specs have been met
 *
 *     Revision 1.3  2001/05/15 18:47:35  davidk
 *     Moved functions around between the apps, DB API, and DB API INTERNAL
 *     levels.  Renamed functions and files.  Added support for amplitude
 *     magnitude types.  Reformatted makefiles.
 *
 *     Revision 1.1  2001/04/13 21:30:27  lucky
 *     Initial revision
 *
 *     Revision 1.1  2001/03/19 17:20:57  lucky
 *     Initial revision
 *
 * 
 */

#ifndef _ALARMS_H
#define _ALARMS_H


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <earthworm.h>
#include <ewdb_ora_api.h>
#include <ew_event_info.h>
#include <ewdb_apps_utils.h>


#define		MAX_RULE_COMMAND_LEN	256
#define 	NEW_ENTRY_FLAG        	-100 
#define 	NONE_ENTRY_FLAG        	-200 
#define		FMT_CHAR    			'~'

/* Update flags */
#define     INFO_NONE  	           	-10
#define     INFO_UPDATE             -20
#define     INFO_NEW 	            -30

/* Config file stuff */
#define EQP_MAXWORD          50
#define MAXPATH         	 512

extern char  envEW_LOG[MAXPATH];        /* where environment variable EW_LOG
                                        will be stored */
extern char  DBservice[EQP_MAXWORD];    /* DBMS instance to interact with */
extern char  DBuser[EQP_MAXWORD];       /* UserId to connect to database as */
extern char  DBpassword[EQP_MAXWORD];   /* Password to datasource */
extern char  WebHost[MAXPATH];          /* machine running the webserver */
extern char  AlarmDir[MAXPATH];         /* tmp directory */
extern char  BackgroundColor[MAXPATH]; 
extern char  HeaderLogo[MAXPATH]; 
extern char  FooterLogo[MAXPATH]; 
extern char  HeaderTag[MAXPATH]; 
extern char  FooterTag[MAXPATH]; 
extern int   DEBUG;
extern int   ValidateQdds;


/* Web parameters */
typedef struct _WebAlarmsOptions
{
  int idRecipient;
  int idRule;
} WebAlarmsOptions;


/* Delivery Methods */
typedef struct del_method
{
	int		idMethod;
	char	label[64];
	char	ThreeLetCode[4];
} DelMethodStruct;


/* 
 * Story: this array keeps track of all delivery methods that 
 * we know about. It consists of three parts:
 *   First entry: Delivery Index which goes into the DelMethodInd
 *     entry of the delivery tables.
 *   Second entry: Name of the mechanism, used in web pages.
 *   Third entry: Three letter code for the mechanism, used
 *     in web pages for passing information about this
 *     mechanism to the CGI programs.
 */
static  DelMethodStruct DelMethodArray[] =
{
  {EWDB_ALARMS_DELIVERY_IND_EMAIL, "Email", "eml"},
  {EWDB_ALARMS_DELIVERY_IND_PAGER, "Pager", "pgr"},
  {EWDB_ALARMS_DELIVERY_IND_PHONE, "Phone", "phl"},
  {EWDB_ALARMS_DELIVERY_IND_QDDS,  "QDDS",  "qds"},
  {EWDB_ALARMS_DELIVERY_IND_CUSTOM,  "Custom",  "cst"},
};


/*
 *  The call to DetermineAlarms was getting out of hand -- too many
 *   parameters, and they kept changing.  So, now we will simply
 *   pass in a pointer to this struct which can keep channging at will
 *   without affecting all the callers.
 */
typedef struct _DetAlms_params
{
	int		isAuto;
	int		isInsert;
	int		minPopulation;
	int		showPopulation;
	char	*networkCode;
	char	*InvocationString;
	
} DetAlmsParams;

int     DetermineAlarms (EWEventInfoStruct *pEvtInfo, DetAlmsParams *pDAP, 
				AlarmList *pAlarms, int AlarmListSize, int *NumAlarmsFound, 
				int *NumAlarmsRetrieved, int *retVal);

int BuildCUBEMessage (char *pMsg, EWEventInfoStruct *pEvtInfo, 
                             int isAuto, int isInsert, char *networkCode);
int BuildhypoTWCMessage (char *pMsg, EWEventInfoStruct *pEvtInfo, 
                             int isAuto, int isInsert, char *networkCode);

int ReadConfig(char *configfile);
/****************************************************************
 * ReadConfig():
 *   Reads an alarms config file and initializes the appropriate
 *   C variables based on the config file entries.
 ****************************************************************/

int Webparse_Client_SetVars(char * szVar, char * szVal, void * pUserParams);
/****************************************************************
 * Webparse_Client_SetVars():
 *   Function initializes alarms C variables, based on variable
 *   information parsed from the web page request.
 ****************************************************************/

#ifdef _WINNT
#define DIR_SLASH   '\\'
#else
#define DIR_SLASH   '/'
#endif


#endif _ALARMS_H
