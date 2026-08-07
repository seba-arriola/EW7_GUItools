/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: gm_sac.h,v 1.1 2001/03/30 19:14:25 lombard Exp $
 *
 *    Revision history:
 *     $Log: gm_sac.h,v $
 *     Revision 1.1  2001/03/30 19:14:25  lombard
 *     Initial revision
 *
 *
 *
 */
/*
 * gm_sac.h
 */

#ifndef GM_SAC_H
#define GM_SAC_H

#include "gm.h"
#include "gm_util.h"

int isMatch(char *, char *);
int initSACBuf( long );
void initSACSave(EVENT *, GMPARAMS *);
void saveSACGMTraces( DATABUF *, STA *, COMP3 *, EVENT *, GMPARAMS *, 
                     SM_INFO *);
void termSACSave(EVENT *, GMPARAMS *);

#endif
