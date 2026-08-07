/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: config.h,v 1.1 2006/01/30 19:29:30 friberg Exp $
 *
 *	  config.h
 *
 *    Revision history:
 *     $Log: config.h,v $
 *     Revision 1.1  2006/01/30 19:29:30  friberg
 *     first earthworm checkin of raypicker
 *
 *     Revision 1.1.1.1  2005/06/22 19:30:49  michelle
 *     new directory tree built from files in HYDRA_NEWDIR_2005-06-20 tagged hydra and earthworm projects
 *
 *     Revision 1.2  2004/06/10 20:22:35  cjbryan
 *     re-engineered array initialization
 *
 *     Revision 1.1.1.1  2004/03/31 18:43:27  michelle
 *     New Hydra Import
 *
 *
 */

#ifndef CONFIG_H
#define CONFIG_H

#include "raypicker.h"

int  raypicker_default_params(RParams *params);
int  raypicker_config(char *config_file, RParams *params, SCNL **Scnl, int *num_SCNL);
int  CheckConfig(RParams parameters, int numSCNL);
void LogConfig(RParams params, SCNL *scnl, int num_SCNL);

#endif /*  CONFIG_H  */