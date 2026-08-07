/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: html_common.h,v 1.4 2002/05/28 19:33:56 lucky Exp $
 *    Revision history:
 *
 *    $Log: html_common.h,v $
 *    Revision 1.4  2002/05/28 19:33:56  lucky
 *    Added header and footer tag text
 *
 *    Revision 1.3  2002/05/28 17:24:05  lucky
 *    *** empty log message ***
 *
 *    Revision 1.2  2002/02/19 16:52:59  lucky
 *    added header logo prototype and changed the one for trailer
 *
 *    Revision 1.1  2001/07/01 21:55:16  davidk
 *    Initial revision
 *
 *
 *
 *
 *
 *****************************************************************/

#include <ew_event_info.h>

#define MAX_TGDS 				10
#define TGD_STRING_LEN 			50
#define TGD_DESCRIPTION_LEN 	80

typedef struct _TraceGifDisplayStruct
{
  int    bUseSeparateWindow;
  char   szProgramName[TGD_STRING_LEN];
  char   szTargetName[TGD_STRING_LEN];
  char   szDescription[TGD_DESCRIPTION_LEN];
} TraceGifDisplayStruct;


void html_break(void);
/****************************************************************
 * html_break():
 *   Function writes an html newline to stdout.
 ****************************************************************/


void html_trailer(char *webhost, char *FooterLogo, char *FooterTag);
/****************************************************************
 * html_trailer():
 *   Function writes html document termination to stdout.  If
 *   webhost is not NULL, then the function will write a blurb
 *   about sending comments to webmaster@webhost, before 
 *   terminating the document.
 ****************************************************************/

void html_header (char *BackgroundColor, char *HeaderLogo, char *HeaderTag);

void html_principal_summary (EWEventInfoStruct *pEvt, int bReview);

void html_coincidence_summary (EWEventInfoStruct *pEvt, int bReview);

void html_links2ora2rsec_gif (int EventID, int NumDispOpt,
								TraceGifDisplayStruct *pDispOpt);

