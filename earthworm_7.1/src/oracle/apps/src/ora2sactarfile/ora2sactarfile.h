/*
 *   This file is under RCS - do not modify unless you have
 *   checked it out using the command checkout.
 *
 *    $Id: ora2sactarfile.h,v 1.1 2001/07/01 21:55:26 davidk Exp $
 *    Revision history:
 *    $Log: ora2sactarfile.h,v $
 *    Revision 1.1  2001/07/01 21:55:26  davidk
 *    Initial revision
 *
 *    Revision 1.5  2001/06/07 17:16:25  lucky
 *
 *********************************************************************/




/* functions from htmlreply.c */
void html_header( void );                  /* htmlreply.c */
void html_tableheader( void );             /* htmlreply.c */
void html_shorttableheader( void );        /* htmlreply.c */
void html_shorttrailer( void );            /* htmlreply.c */
void html_tablerow(int eventid, char *ftphost, char *ftpdir, 
                   char *yrmondir, char *eventdir, int nfiles );
void html_shorttablerow( int eventid, char *dir );


/* functions from ora2sactarfile.c */
static int InputEventIds(int ** pEventList, int * pNumEventsToRetrieve); 

/* functions from config.c */
int ReadConfig(char *configfile);

