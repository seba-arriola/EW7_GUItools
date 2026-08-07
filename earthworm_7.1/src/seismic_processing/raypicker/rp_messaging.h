/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: rp_messaging.h,v 1.1 2006/01/30 19:29:30 friberg Exp $
 *
 *    Revision history:
 *     $Log: rp_messaging.h,v $
 *     Revision 1.1  2006/01/30 19:29:30  friberg
 *     first earthworm checkin of raypicker
 *
 *     Revision 1.1.1.1  2005/06/22 19:30:50  michelle
 *     new directory tree built from files in HYDRA_NEWDIR_2005-06-20 tagged hydra and earthworm projects
 *
 *     Revision 1.5  2005/06/20 20:57:47  cjbryan
 *     update error messaging to use reportError
 *
 *     Revision 1.4  2005/02/28 20:35:41  davidk
 *     Moved code that issues earthworm hearbeat into WriteHeartbeat() function
 *     in rp_messaging.c.
 *
 *     Revision 1.3  2004/11/01 02:03:27  cjbryan
 *     removed all amplitude determination code from the picker and changed all error
 *     codes to positive numbers
 *     CVS ----------------------------------------------------------------------
 *
 *     Revision 1.2  2004/04/23 17:37:10  cjbryan
 *     changed bool to int
 *
 *     Revision 1.1.1.1  2004/03/31 18:43:27  michelle
 *     New Hydra Import
 *
 *
 */
/*
 * Declaration of utility functions used in the raypicker to
 * send error and output messages.
 * 
 * @author Dale Hanych, Genesis Business Group (dbh)
 * @version 1.0 August 2003, dbh
 */

#ifndef RP_MESSAGING_H
#define RP_MESSAGING_H

/* earthworm includes */
#include <transport.h>

/* raypicker includes */
#include "raypicker.h"

int         WriteHeartbeat(EWParameters ewp, RParams rparams, pid_t MyPid);
static int  SendMessageToRing(MSG_LOGO *logo , char *buffer, int length, SHM_INFO *OutRing);
static long GetPickID(RParams params);
int         ReportPick(SCNL *scnl, RParams rparams, EWParameters ewp);

#endif /*  RP_MESSAGING_H  */
