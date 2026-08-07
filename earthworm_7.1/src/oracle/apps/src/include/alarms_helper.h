/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: alarms_helper.h,v 1.1 2001/07/01 21:55:16 davidk Exp $
 *    Revision history:
 *
 *    $Log: alarms_helper.h,v $
 *    Revision 1.1  2001/07/01 21:55:16  davidk
 *    Initial revision
 *
 *
 *
 *
 *
 *****************************************************************/


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

